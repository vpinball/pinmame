/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "driver.h"
#if defined(LISY_SUPPORT) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
 #include "fm.h"
#endif
#if (HAS_YM2151_ALT)
 #include "ym2151.h"
#endif
#if (HAS_YM2151_NUKED)
 #include "ym2151_opm.h"
 #include "ym2151_opm.c"
 static opm_t chip[MAX_2151];
#endif
#if (HAS_YM2151_NUKED || HAS_YM2151_LLE)
 static mame_timer * update_timer; // shared by both nukeykt low level cores, see below
#endif
#if (HAS_YM2151_LLE)
 #include "ym2151_lle.h"
 #include "ym2151_lle.c" // which #includes fmopm.c, like the Nuked core above
#endif
#if (HAS_YM2151_YMFM)
 #include "../ext/vgm/vgmwrite.h"

 void* ymfm_ym2151_create(int baseindex, void(*irqhandler)(int irq), mem_write_handler porthandler, double baseclock, void(*callback)(int param));
 void ymfm_ym2151_destroy(void* obj);

 void ymfm_ym2151_reset(void* obj);

 void ymfm_ym2151_callback(void* obj, int param);

 void ymfm_ym2151_invalidate_caches(void* obj);

 uint8_t ymfm_ym2151_read(void* obj, uint32_t offset);

 void ymfm_ym2151_write(void* obj, uint32_t offset, uint8_t data);

 void ymfm_ym2151_generate(void* obj, int16_t** output, uint32_t numsamples);

 static void* chip[MAX_2151];
 static unsigned short vgm_idx[MAX_2151];
#endif

/* for stream system */
static int stream[MAX_2151];

/* last value written to the address port, shared by every core: the register/data port handlers latch it here and the data write pairs with it */
static unsigned char lastreg[MAX_2151];

static const struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4	/* use Tatsuyuki's FM.C */
#define CHIP_YM2151_ALT 5	/* use Jarek's YM2151.C */
#define CHIP_YM2151_NUKED 6	/* use Nuked-OPM */
#define CHIP_YM2151_YMFM 7	/* use Aarons unified FM */
#define CHIP_YM2151_LLE  8	/* use nukeykt's YM2151-LLE */

#define YM2151_NUMBUF 2

#if (HAS_YM2151)
static void *Timer[MAX_2151][2];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(intf->irqhandler[n]) intf->irqhandler[n](irq);
}

static void timer_callback_2151(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	YM2151TimerOver(n,c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,int count,double stepTime)
{
	if( count == 0 )
	{	/* Reset FM Timer */
		timer_enable (Timer[n][c], 0);
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;
		if (!timer_enable(Timer[n][c], 1))
			timer_adjust (Timer[n][c], timeSec, (c<<7)|n, 0);
	}
}
#endif

/* update request from fm.c */
static void YM2151UpdateRequest(int chip_num)
{
	stream_update(stream[chip_num],0);
}

#if (HAS_YM2151_NUKED)
static void YM2151UpdateNuked(int num, INT16 **buffers, int length)
{
	OPM_GenerateStream(&chip[num], (float**)buffers, length);
}

#endif
#if (HAS_YM2151_NUKED || HAS_YM2151_LLE)
// to keep up with the CPU emulation (i.e. IRQ and port callbacks), trigger the sound updates on a regular basis
static void update_timer_func(int timer_num)
{
	int i;
	for (i = 0; i < intf->num; i++)
		YM2151UpdateRequest(i);
}

/* The update timer above exists only so that the IRQ and port callbacks of the two low level
   cores, Nuked and LLE, reach the rest of the emulation on time: both only advance, and so
   only raise those callbacks, while they are being clocked inside the stream callback.

   The chip itself is cycle exact whatever rate we poll it at, so
   polling slower delays when the host *notices* an interrupt, it does not shift the chip's
   own timing. Running it at the output sample rate (~55.9kHz) is therefore far more than is
   ever needed, so track what the ROM actually programmed and poll just fast enough:

     timer A period = 64*(1024-NA)/clk  =      (1024-NA) output samples
     timer B period = 1024*(256-NB)/clk = 16 * (256-NB)  output samples

   and when neither timer interrupt is enabled, stop the timer altogether. The port callback
   (CT1/CT2) only ever fires as a result of a write, which already pumps the stream, so it
   just needs a floor low enough that the change is not sat on for long */
#define YM2151_PINLEVEL_OVERSAMPLE 2.    /* polls per timer period, to keep the jitter under half of one */
#define YM2151_PINLEVEL_PORT_HZ    1000. /* floor while a port write handler is connected */

static UINT16 pinlevel_timerA[MAX_2151]; /* NA, 10 bits, regs 0x10/0x11 */
static UINT8  pinlevel_timerB[MAX_2151]; /* NB,  8 bits, reg  0x12 */
static UINT8  pinlevel_irqEn[MAX_2151];  /* reg 0x14, bit 2 = timer A, bit 3 = timer B */
static UINT8  pinlevel_hasPort;
static double pinlevel_rate;             /* baseclock/64, the chip's own output sample rate */
static double pinlevel_timerHz;          /* what we last programmed, to avoid pointless re-arming */

static void ym2151_pinlevel_retime(void)
{
	double need = pinlevel_hasPort ? YM2151_PINLEVEL_PORT_HZ : 0.;
	int i;

	if (!update_timer)
		return;

	for (i = 0; i < intf->num; i++)
	{
		if (pinlevel_irqEn[i] & 0x04)
		{
			const double f = YM2151_PINLEVEL_OVERSAMPLE * pinlevel_rate / (double)(1024u - pinlevel_timerA[i]);
			if (f > need) need = f;
		}
		if (pinlevel_irqEn[i] & 0x08)
		{
			const double f = YM2151_PINLEVEL_OVERSAMPLE * pinlevel_rate / (16. * (double)(256u - pinlevel_timerB[i]));
			if (f > need) need = f;
		}
	}
	if (need > pinlevel_rate) need = pinlevel_rate; /* never more than one poll per output sample */

	if (need == pinlevel_timerHz)
		return;
	pinlevel_timerHz = need;

	if (need > 0.)
		timer_adjust(update_timer, TIME_IN_HZ(need), 0, TIME_IN_HZ(need));
	else
		timer_enable(update_timer, 0);
}

/* called for every register write that reaches either low level core, so the shadow above tracks whatever the ROM programmed */
INLINE void ym2151_pinlevel_reg_w(int num, UINT8 reg, UINT8 data)
{
	switch (reg)
	{
	case 0x10: pinlevel_timerA[num] = (UINT16)((pinlevel_timerA[num] & 0x003) | (data << 2));   break;
	case 0x11: pinlevel_timerA[num] = (UINT16)((pinlevel_timerA[num] & 0x3fc) | (data & 0x03)); break;
	case 0x12: pinlevel_timerB[num] = data;        break;
	case 0x14: pinlevel_irqEn[num]  = data & 0x0c; break;
	default: return; /* nothing else changes the rate we need */
	}
	ym2151_pinlevel_retime();
}
#endif
#if (HAS_YM2151_LLE)
static void YM2151UpdateLLE(int num, INT16 **buffers, int length)
{
	ym2151lle_generate(num, buffers, length);
}
#endif
#if (HAS_YM2151_YMFM)
static void YM2151UpdateYMFM(int num, INT16 **buffers, int length)
{
	ymfm_ym2151_generate/*_buffered*/(chip[num], buffers, length);
}

static void timercallback(int timer_num)
{
	// due to C++ reasons in YMFM, we have to remap the timer callbacks in the order they were created and then do the callback passing this way
	ymfm_ym2151_callback(chip[timer_num/2], timer_num%2);
}
#endif

static int my_YM2151_sh_start(const struct MachineSound *msound,const int mode)
{
	int i,j;
	double rate;// = Machine->sample_rate;
	char buf[YM2151_NUMBUF][40];
	const char *name[YM2151_NUMBUF];
	int mixed_vol,vol[YM2151_NUMBUF];

	intf = msound->sound_interface;
	rate = intf->baseclock/64.;

	if ( mode == 1 ) FMMode = CHIP_YM2151_ALT;
	else if ( mode == 2 ) FMMode = CHIP_YM2151_NUKED;
	else if ( mode == 3 ) FMMode = CHIP_YM2151_YMFM;
	else if ( mode == 4 ) FMMode = CHIP_YM2151_LLE;
	else FMMode = CHIP_YM2151_DAC;

	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:	/* Tatsuyuki's */
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			mixed_vol = intf->volume[i];
			/* stream setup */
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,vol,rate,i,OPMUpdateOne);
		}
		/* Set Timer handler */
		for (i = 0; i < intf->num; i++)
		{
			Timer[i][0] = timer_alloc(timer_callback_2151);
			Timer[i][1] = timer_alloc(timer_callback_2151);
		}
		if (OPMInit(intf->num,intf->baseclock,rate,TimerHandler,IRQHandler) == 0)
		{
			/* set port handler */
			for (i = 0; i < intf->num; i++)
				OPMSetPortHander(i,intf->portwritehandler[i]);
			return 0;
		}
		/* error */
		return 1;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:	/* Jarek's */
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			mixed_vol = intf->volume[i];
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,vol,rate,i,YM2151UpdateOne);
		}

		if (YM2151Init(intf->num,intf->baseclock,rate) == 0)
		{
			for (i = 0; i < intf->num; i++)
			{
				YM2151SetIrqHandler(i,intf->irqhandler[i]);
				YM2151SetPortWriteHandler(i,intf->portwritehandler[i]);
			}
			return 0;
		}
		return 1;
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
	{
		UINT8 has_handler = 0;
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			mixed_vol = intf->volume[i];
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi_float(YM2151_NUMBUF,
				name,vol,rate,i,YM2151UpdateNuked,1);

			//OPM_FlushBuffer(&chip[i]);
			OPM_Reset(&chip[i], opm_flags_none, intf->baseclock);

			has_handler |= (intf->irqhandler[i] != 0) | (intf->portwritehandler[i] != 0);

			OPM_SetIrqHandler(&chip[i], intf->irqhandler[i]); // DE & WMS needs this
			OPM_SetPortWriteHandler(&chip[i], intf->portwritehandler[i]); // DE needs this
		}

		// to keep up with the CPU emulation (i.e. IRQ and port callbacks), trigger the sound updates on a regular basis
		if (has_handler) // only stress the emulation with this timer if any external handler needed
		{
			int k;
			for (k = 0; k < MAX_2151; k++)
				{ pinlevel_timerA[k] = 0; pinlevel_timerB[k] = 0; pinlevel_irqEn[k] = 0; }
			pinlevel_hasPort = 0;
			for (k = 0; k < intf->num; k++)
				if (intf->portwritehandler[k]) pinlevel_hasPort = 1;
			pinlevel_rate = rate;
			pinlevel_timerHz = -1.; // force the first retime to program the timer

			update_timer = timer_alloc(update_timer_func);
			ym2151_pinlevel_retime(); // rate follows what the ROM programs, see above
		}
		else
			update_timer = NULL;

		return 0;
	}
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
	{
		UINT8 has_handler = 0;
		for (i = 0;i < intf->num;i++)
		{
			mixed_vol = intf->volume[i];
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,name,vol,rate,i,YM2151UpdateLLE);

			ym2151lle_init(i, 0); /* 0 = YM2151; no PinMAME game uses the YM2164 */
			ym2151lle_set_handlers(i, intf->irqhandler[i], intf->portwritehandler[i]);
			has_handler |= (intf->irqhandler[i] != 0) | (intf->portwritehandler[i] != 0);
		}

		/* this core delivers IRQ and CT only while it is being clocked, exactly like
		   Nuked, so it needs the same update timer - see ym2151_pinlevel_retime() */
		if (has_handler)
		{
			int k;
			for (k = 0; k < MAX_2151; k++)
				{ pinlevel_timerA[k] = 0; pinlevel_timerB[k] = 0; pinlevel_irqEn[k] = 0; }
			pinlevel_hasPort = 0;
			for (k = 0; k < intf->num; k++)
				if (intf->portwritehandler[k]) pinlevel_hasPort = 1;
			pinlevel_rate = rate;
			pinlevel_timerHz = -1.;
			update_timer = timer_alloc(update_timer_func);
			ym2151_pinlevel_retime();
		}
		else
			update_timer = NULL;

		return 0;
	}
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
	{
		UINT8 has_handler = 0;
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			mixed_vol = intf->volume[i];
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,vol,rate,i,YM2151UpdateYMFM);

			// DE & WMS needs irqhandler
			// DE needs portwritehandler
			chip[i] = ymfm_ym2151_create(i*2, intf->irqhandler[i], intf->portwritehandler[i], intf->baseclock, timercallback);
			vgm_idx[i] = vgm_open(VGMC_YM2151, intf->baseclock);

			has_handler |= (intf->irqhandler[i] != 0) | (intf->portwritehandler[i] != 0);
		}

		return 0;
	}
#endif
	}
	return 1;
}

void YM2151_set_mixing_levels(int chip_num, int l, int r)
{
	mixer_set_mixing_level(stream[chip_num], l);
	mixer_set_mixing_level(stream[chip_num] + 1, r);
}

#if (HAS_YM2151)
int YM2151_sh_start(const struct MachineSound *msound)
{
	return my_YM2151_sh_start(msound,0);
}
#endif
#if (HAS_YM2151_ALT)
int YM2151_sh_start(const struct MachineSound *msound)
{
	return my_YM2151_sh_start(msound,1);
}
#endif
#if (HAS_YM2151_NUKED)
int YM2151_sh_start(const struct MachineSound *msound)
{
	return my_YM2151_sh_start(msound,2);
}
#endif
#if (HAS_YM2151_YMFM)
int YM2151_sh_start(const struct MachineSound* msound)
{
	return my_YM2151_sh_start(msound,3);
}
#endif
#if (HAS_YM2151_LLE)
int YM2151_sh_start(const struct MachineSound* msound)
{
	return my_YM2151_sh_start(msound,4);
}
#endif

void YM2151_sh_stop(void)
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		OPMShutdown();
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151Shutdown();
		break;
#endif
#if (HAS_YM2151_NUKED || HAS_YM2151_LLE)
	case CHIP_YM2151_NUKED:
	case CHIP_YM2151_LLE:
		if(update_timer)
		{
			timer_remove(update_timer);
			update_timer = NULL;
		}
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		{
		int i;
		for (i = 0; i < intf->num; i++)
			ymfm_ym2151_destroy(chip[i]);
		}
		break;
#endif
	}
}

void YM2151_sh_reset(void)
{
	int i;
	for (i = 0; i < intf->num; i++)
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		OPMResetChip(i);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151ResetChip(i);
		break;
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(i);
		OPM_FlushBuffer(&chip[i]);
		OPM_Reset(&chip[i], opm_flags_none, 0);
		pinlevel_timerA[i] = 0; pinlevel_timerB[i] = 0; pinlevel_irqEn[i] = 0;
		ym2151_pinlevel_retime();
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(i);
		ym2151lle_reset(i);
		pinlevel_timerA[i] = 0; pinlevel_timerB[i] = 0; pinlevel_irqEn[i] = 0;
		ym2151_pinlevel_retime();
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(i);
		ymfm_ym2151_invalidate_caches(chip[i]); //!! needed?
		ymfm_ym2151_reset(chip[i]);
		break;
#endif
	}
}

READ_HANDLER( YM2151_status_port_0_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(0,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(0);
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(0);
		return OPM_Read(&chip[0],1);
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(0);
		return ym2151lle_read_status(0);
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(0);
		return ymfm_ym2151_read(chip[0],1);
#endif
	}
	return 0;
}

READ_HANDLER( YM2151_status_port_1_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(1,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(1);
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(1);
		return OPM_Read(&chip[1],1);
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(1);
		return ym2151lle_read_status(1);
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(1);
		return ymfm_ym2151_read(chip[1],1);
#endif
	}
	return 0;
}

READ_HANDLER( YM2151_status_port_2_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(2,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(2);
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(2);
		return OPM_Read(&chip[2],1);
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(2);
		return ym2151lle_read_status(2);
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(2);
		return ymfm_ym2151_read(chip[2],1);
#endif
	}
	return 0;
}

/* These are compiled for every core, not just ALT: they are declared unconditionally in 2151intf.h */
WRITE_HANDLER( YM2151_register_port_0_w )
{
	lastreg[0] = data;
}
WRITE_HANDLER( YM2151_register_port_1_w )
{
	lastreg[1] = data;
}
WRITE_HANDLER( YM2151_register_port_2_w )
{
	lastreg[2] = data;
}

WRITE_HANDLER( YM2151_data_port_0_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(0,0,lastreg[0]);
		YM2151Write(0,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(0);
		YM2151WriteReg(0,lastreg[0],data);
		break;
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(0);
		/* two calls: Nuked's port is 0 = address / 1 = data, not the register number */
		OPM_Write/*Buffered*/(&chip[0], 0, lastreg[0]);
		OPM_Write/*Buffered*/(&chip[0], 1, data);
		ym2151_pinlevel_reg_w(0, lastreg[0], data);
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(0);
		ym2151lle_write(0, 0, lastreg[0]);
		ym2151lle_write(0, 1, data);
		ym2151_pinlevel_reg_w(0, lastreg[0], data);
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(0);
		vgm_write(vgm_idx[0], 0x00, lastreg[0], data);
		ymfm_ym2151_write/*_buffered*/(chip[0], lastreg[0], data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_data_port_1_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(1,0,lastreg[1]);
		YM2151Write(1,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(1);
		YM2151WriteReg(1,lastreg[1],data);
		break;
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(1);
		/* two calls: Nuked's port is 0 = address / 1 = data, not the register number */
		OPM_Write/*Buffered*/(&chip[1], 0, lastreg[1]);
		OPM_Write/*Buffered*/(&chip[1], 1, data);
		ym2151_pinlevel_reg_w(1, lastreg[1], data);
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(1);
		ym2151lle_write(1, 0, lastreg[1]);
		ym2151lle_write(1, 1, data);
		ym2151_pinlevel_reg_w(1, lastreg[1], data);
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(1);
		vgm_write(vgm_idx[1], 0x00, lastreg[1], data);
		ymfm_ym2151_write/*_buffered*/(chip[1], lastreg[1], data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_data_port_2_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(2,0,lastreg[2]);
		YM2151Write(2,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(2);
		YM2151WriteReg(2,lastreg[2],data);
		break;
#endif
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(2);
		/* two calls: Nuked's port is 0 = address / 1 = data, not the register number */
		OPM_Write/*Buffered*/(&chip[2], 0, lastreg[2]);
		OPM_Write/*Buffered*/(&chip[2], 1, data);
		ym2151_pinlevel_reg_w(2, lastreg[2], data);
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(2);
		ym2151lle_write(2, 0, lastreg[2]);
		ym2151lle_write(2, 1, data);
		ym2151_pinlevel_reg_w(2, lastreg[2], data);
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(2);
		vgm_write(vgm_idx[2], 0x00, lastreg[2], data);
		ymfm_ym2151_write/*_buffered*/(chip[2], lastreg[2], data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_word_0_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(0);
		if (offset & 0x01)
			ym2151_pinlevel_reg_w(0, lastreg[0], data);
		else
			lastreg[0] = data;
		OPM_WriteBuffered(&chip[0], offset, data);
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(0);
		if (offset & 0x01)
			ym2151_pinlevel_reg_w(0, lastreg[0], data);
		else
			lastreg[0] = data;
		ym2151lle_write(0, offset, data);
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(0);
		if (offset & 0x01)
			vgm_write(vgm_idx[0], offset >> 1, lastreg[0], data);
		else
			lastreg[0] = data;
		ymfm_ym2151_write/*_buffered*/(chip[0], offset, data);
		break;
#endif
#if (HAS_YM2151 || HAS_YM2151_ALT)
	default:
		if (offset)
			YM2151_data_port_0_w(0,data);
		else
			YM2151_register_port_0_w(0,data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_word_1_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151_NUKED)
	case CHIP_YM2151_NUKED:
		YM2151UpdateRequest(1);
		if (offset & 0x01)
			ym2151_pinlevel_reg_w(1, lastreg[1], data);
		else
			lastreg[1] = data;
		OPM_WriteBuffered(&chip[1], offset, data);
		break;
#endif
#if (HAS_YM2151_LLE)
	case CHIP_YM2151_LLE:
		YM2151UpdateRequest(1);
		if (offset & 0x01)
			ym2151_pinlevel_reg_w(1, lastreg[1], data);
		else
			lastreg[1] = data;
		ym2151lle_write(1, offset, data);
		break;
#endif
#if (HAS_YM2151_YMFM)
	case CHIP_YM2151_YMFM:
		YM2151UpdateRequest(1);
		if (offset & 0x01)
			vgm_write(vgm_idx[1], offset >> 1, lastreg[1], data);
		else
			lastreg[1] = data;
		ymfm_ym2151_write/*_buffered*/(chip[1], offset, data);
		break;
#endif
#if (HAS_YM2151 || HAS_YM2151_ALT)
	default:
		if (offset)
			YM2151_data_port_1_w(0,data);
		else
			YM2151_register_port_1_w(0,data);
		break;
#endif
	}
}

#if (HAS_YM2151_ALT)
READ16_HANDLER( YM2151_status_port_0_lsb_r )
{
	return YM2151_status_port_0_r(0);
}

READ16_HANDLER( YM2151_status_port_1_lsb_r )
{
	return YM2151_status_port_1_r(0);
}

READ16_HANDLER( YM2151_status_port_2_lsb_r )
{
	return YM2151_status_port_2_r(0);
}


WRITE16_HANDLER( YM2151_register_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_0_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_register_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_1_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_register_port_2_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_2_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_0_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_1_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_2_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_2_w(0, data & 0xff);
}
#endif
