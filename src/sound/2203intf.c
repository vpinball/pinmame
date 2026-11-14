#include <math.h>
#include "driver.h"
#include "ay8910.h"
#include "fm.h"


static int stream[MAX_2203];

static const struct YM2203interface *intf;

#define YM2203_YMFM 0 // 1: the YM2203 (FM and SSG) runs on ymfm, as in MAME, 0: on the old fm.c (FM) and ay8910.c (SSG) cores

#if YM2203_YMFM
#include "../ext/vgm/vgmwrite.h"
// ymfm OPN glue (see ext/ymfm/ymfm_opn_pinmame_specific.h)
void* ymfm_ym2203_create(int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param),
	mem_read_handler portAread, mem_read_handler portBread, mem_write_handler portAwrite, mem_write_handler portBwrite);
void ymfm_ym2203_destroy(void* obj);
void ymfm_ym2203_reset(void* obj);
void ymfm_ym2203_callback(void* obj, int param);
void ymfm_ym2203_invalidate_caches(void* obj);
unsigned char ymfm_ym2203_read(void* obj, unsigned int offset);
void ymfm_ym2203_write(void* obj, unsigned int offset, unsigned char data);
void ymfm_ym2203_generate(void* obj, INT16* fm, INT32* ssg, unsigned int numsamples);

/* what is left of DC on the (centred, see the ymfm glue) SSG channels is removed with the same 10Hz one-pole as ay8910.c */
#define YMFM_SSG_DC_CUTOFF_HZ 10.0
/* scale of the SSG part relative to ay8910.c, so that the drivers' SSG mixing levels stay valid:
   ymfm's loudest SSG level is 16382 peak to peak per channel, ay8910.c's is 32767 */
#define YMFM_SSG_GAIN 2.0

static void *chip_ymfm[MAX_2203];
static unsigned short vgm_idx_ymfm[MAX_2203];
static UINT8 lastreg_ymfm[MAX_2203];
static double ssg_gain_ymfm[MAX_2203];
static double dc_coeff_ymfm;
static double dc_integ_ymfm[MAX_2203];
static INT32 dc_prev_ymfm[MAX_2203];

#define YMFM_BUF_LEN 256

static void YM2203UpdateYMFM(int num, INT16 **buffer, int length)
{
	float *ssg_out = (float*)buffer[0];
	float *fm_out = (float*)buffer[1];
	INT16 fm[YMFM_BUF_LEN];
	INT32 ssg[YMFM_BUF_LEN];

	while (length > 0)
	{
		const int n = length < YMFM_BUF_LEN ? length : YMFM_BUF_LEN;
		int i;
		ymfm_ym2203_generate(chip_ymfm[num], fm, ssg, n);
		for (i = 0; i < n; i++)
		{
			dc_integ_ymfm[num] = dc_integ_ymfm[num]*dc_coeff_ymfm + (double)(ssg[i] - dc_prev_ymfm[num]);
			dc_prev_ymfm[num] = ssg[i];
			*(ssg_out++) = (float)(dc_integ_ymfm[num] * ssg_gain_ymfm[num]);
			*(fm_out++) = (float)fm[i] * (float)(1. / 32768.);
		}
		length -= n;
	}
}

static void timercallback_ymfm(int timer_num)
{
	// timer_num encodes (chip*2 + timer); see ymfm glue
	ymfm_ym2203_callback(chip_ymfm[timer_num/2], timer_num%2);
}

static int YM2203_sh_start_ymfm(const struct MachineSound *msound)
{
	int i;
	/* same output rate as MAME (OPN_FIDELITY_MED): clock/12, whatever the prescaler */
	const double rate = intf->baseclock/12.;
	const double w = (2.*M_PI*YMFM_SSG_DC_CUTOFF_HZ)/rate;

	dc_coeff_ymfm = (1. - sin(w))/cos(w);

	for (i = 0;i < intf->num;i++)
	{
		char buf[2][40];
		const char *name[2];
		int vol[2];
		/* same channels (and order) as with the old cores: SSG, then FM */
		sprintf(buf[0],"%s #%d",sound_name(msound),i);
		sprintf(buf[1],"%s #%d FM",sound_name(msound),i);
		name[0] = buf[0];
		name[1] = buf[1];
		/* same mixer default as ay8910.c (level applied inside, the mixer channel at 100), so saved mixer settings apply to both cores */
		vol[0] = MIXERG(100,MIXER_GET_GAIN(intf->mixing_level[i] & 0xffff),MIXER_GET_PAN(intf->mixing_level[i] & 0xffff));
		ssg_gain_ymfm[i] = YMFM_SSG_GAIN / 32768. * MIXER_GET_LEVEL(intf->mixing_level[i] & 0xffff) / 100.;
		vol[1] = intf->mixing_level[i] >> 16; /* high 16 bit */
		stream[i] = stream_init_multi_float(2,name,vol,rate,i,YM2203UpdateYMFM,1);
		if (stream[i] == -1)
			return 1;

		chip_ymfm[i] = ymfm_ym2203_create(i*2, intf->handler[i], intf->baseclock, timercallback_ymfm,
			intf->portAread[i], intf->portBread[i], intf->portAwrite[i], intf->portBwrite[i]);
		vgm_idx_ymfm[i] = vgm_open(VGMC_YM2203, intf->baseclock);
#define AY8910_LEGACY_OUTPUT        (0x01)
		vgm_header_set(vgm_idx_ymfm[i], 0x01, AY8910_LEGACY_OUTPUT);
		lastreg_ymfm[i] = 0;
		dc_integ_ymfm[i] = 0.;
		dc_prev_ymfm[i] = 0;
	}
	return 0;
}

static void YM2203Write_ymfm(int n, int a, int v)
{
	if (a & 1) /* data port */
	{
		/* render the stream up to this write first (same as the old core and the ymfm OPL/OPM wrappers) */
		stream_update(stream[n], 0);
		vgm_write(vgm_idx_ymfm[n], 0x00, lastreg_ymfm[n], v);
		ymfm_ym2203_write(chip_ymfm[n], 1, v);
	}
	else /* address port */
	{
		lastreg_ymfm[n] = v;
		/* prescaler select changes the FM and SSG rates */
		if (v >= 0x2d && v <= 0x2f)
		{
			stream_update(stream[n], 0);
			vgm_write(vgm_idx_ymfm[n], 0x00, v, 1);
		}
		ymfm_ym2203_write(chip_ymfm[n], 0, v);
	}
}
#endif /* YM2203_YMFM */

static void *Timer[MAX_2203][2];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(intf->handler[n]) intf->handler[n](irq);
}

/* Timer overflow callback from timer.c */
static void timer_callback_2203(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	YM2203TimerOver(n,c);
}

/* update request from fm.c */
void YM2203UpdateRequest(int chip)
{
	stream_update(stream[chip],0);
}

#if 0
/* update callback from stream.c */
static void YM2203UpdateCallback(int chip,void *buffer,int length)
{
	YM2203UpdateOne(chip,buffer,length);
}
#endif

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,int count,double stepTime)
{
	if( count == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer[n][c], 0);
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;
		if (!timer_enable(Timer[n][c], 1))
			timer_adjust(Timer[n][c], timeSec, (c<<7)|n, 0);
	}
}

static void FMTimerInit( void )
{
	int i;

	for( i = 0 ; i < MAX_2203 ; i++ )
	{
		Timer[i][0] = timer_alloc(timer_callback_2203);
		Timer[i][1] = timer_alloc(timer_callback_2203);
	}
}

int YM2203_sh_start(const struct MachineSound *msound)
{
	int i;
	double rate;

#if YM2203_YMFM
	intf = msound->sound_interface;
	return YM2203_sh_start_ymfm(msound);
#endif

	if (AY8910_sh_start_ym(msound)) return 1;

	intf = msound->sound_interface;

	rate = intf->baseclock/72.; /* ??? */

	/* Timer Handler set */
	FMTimerInit();
	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		int volume;
		char name[20];
		sprintf(name,"%s #%d FM",sound_name(msound),i);
		volume = intf->mixing_level[i]>>16; /* high 16 bit */
		stream[i] = stream_init(name,volume,rate,i,YM2203UpdateOne/*YM2203UpdateCallback*/);
	}
	/* Initialize FM emulator */
	if (YM2203Init(intf->num,intf->baseclock,rate,TimerHandler,IRQHandler) == 0)
	{
		/* Ready */
		return 0;
	}
	/* error */
	/* stream close */
	return 1;
}

void YM2203_sh_stop(void)
{
#if YM2203_YMFM
	{
		int i;
		for (i = 0;i < intf->num;i++)
		{
			ymfm_ym2203_destroy(chip_ymfm[i]);
			chip_ymfm[i] = NULL;
		}
		return;
	}
#endif
	YM2203Shutdown();
	AY8910_sh_stop_ym();
}

void YM2203_sh_reset(void)
{
	int i;

#if YM2203_YMFM
	for (i = 0;i < intf->num;i++)
	{
		ymfm_ym2203_invalidate_caches(chip_ymfm[i]);
		ymfm_ym2203_reset(chip_ymfm[i]);
	}
	return;
#endif
	for (i = 0;i < intf->num;i++)
		YM2203ResetChip(i);
}

#if YM2203_YMFM
#define YM2203_r(n,a) ymfm_ym2203_read(chip_ymfm[n], a)
#define YM2203_w YM2203Write_ymfm
#else
#define YM2203_r YM2203Read
#define YM2203_w YM2203Write
#endif



READ_HANDLER( YM2203_status_port_0_r ) { return YM2203_r(0,0); }
READ_HANDLER( YM2203_status_port_1_r ) { return YM2203_r(1,0); }
READ_HANDLER( YM2203_status_port_2_r ) { return YM2203_r(2,0); }
READ_HANDLER( YM2203_status_port_3_r ) { return YM2203_r(3,0); }
READ_HANDLER( YM2203_status_port_4_r ) { return YM2203_r(4,0); }

READ_HANDLER( YM2203_read_port_0_r ) { return YM2203_r(0,1); }
READ_HANDLER( YM2203_read_port_1_r ) { return YM2203_r(1,1); }
READ_HANDLER( YM2203_read_port_2_r ) { return YM2203_r(2,1); }
READ_HANDLER( YM2203_read_port_3_r ) { return YM2203_r(3,1); }
READ_HANDLER( YM2203_read_port_4_r ) { return YM2203_r(4,1); }

WRITE_HANDLER( YM2203_control_port_0_w )
{
	YM2203_w(0,0,data);
}
WRITE_HANDLER( YM2203_control_port_1_w )
{
	YM2203_w(1,0,data);
}
WRITE_HANDLER( YM2203_control_port_2_w )
{
	YM2203_w(2,0,data);
}
WRITE_HANDLER( YM2203_control_port_3_w )
{
	YM2203_w(3,0,data);
}
WRITE_HANDLER( YM2203_control_port_4_w )
{
	YM2203_w(4,0,data);
}

WRITE_HANDLER( YM2203_write_port_0_w )
{
	YM2203_w(0,1,data);
}
WRITE_HANDLER( YM2203_write_port_1_w )
{
	YM2203_w(1,1,data);
}
WRITE_HANDLER( YM2203_write_port_2_w )
{
	YM2203_w(2,1,data);
}
WRITE_HANDLER( YM2203_write_port_3_w )
{
	YM2203_w(3,1,data);
}
WRITE_HANDLER( YM2203_write_port_4_w )
{
	YM2203_w(4,1,data);
}

WRITE_HANDLER( YM2203_word_0_w )
{
	if (offset)
		YM2203_write_port_0_w(0,data);
	else
		YM2203_control_port_0_w(0,data);
}

WRITE_HANDLER( YM2203_word_1_w )
{
	if (offset)
		YM2203_write_port_1_w(0,data);
	else
		YM2203_control_port_1_w(0,data);
}
