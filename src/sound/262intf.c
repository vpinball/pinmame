/***************************************************************************

  262intf.c

  MAME interface for YMF262 (OPL3) emulator

***************************************************************************/
#include "driver.h"
#include "262intf.h"
#include "ymf262.h"

#if (HAS_YMF262_YMFM)
 #include "../ext/vgm/vgmwrite.h"
 // unified ymfm OPL glue (see ext/ymfm/ymfm_opl_pinmame_specific.h)
 void* ymfm_opl_create(int chiptype, int baseindex, void(*irqhandler)(int irq), double baseclock, void(*callback)(int param));
 void ymfm_opl_destroy(void* obj);
 void ymfm_opl_reset(void* obj);
 void ymfm_opl_callback(void* obj, int param);
 void ymfm_opl_invalidate_caches(void* obj);
 unsigned char ymfm_opl_read(void* obj, unsigned int offset);
 void ymfm_opl_write(void* obj, unsigned int offset, unsigned char data);
 void ymfm_opl_generate(void* obj, INT16** output, unsigned int numsamples, unsigned int numch);
 #define YMFM_OPL_YMF262 2
#endif


#if (HAS_YMF262 || HAS_YMF262_YMFM)

static int  stream_262[MAX_262];
static const struct YMF262interface *intf_262 = NULL;

#if (HAS_YMF262_YMFM)
static void *chip_262[MAX_262];
static unsigned short vgm_idx_262[MAX_262];
static unsigned int vgm_addressa_262[MAX_262];

static void YMF262UpdateYMFM(int num, INT16 **buffers, int length)
{
	ymfm_opl_generate(chip_262[num], buffers, length, 4);
}
static void timercallback_262(int timer_num)
{
	ymfm_opl_callback(chip_262[timer_num/2], timer_num%2);
}

int YMF262_sh_start(const struct MachineSound *msound)
{
	int chip;
	double rate;

	intf_262 = msound->sound_interface;
	if( intf_262->num > MAX_262 ) return 1;

	rate = intf_262->baseclock/(8*36);

	for (chip = 0;chip < intf_262->num; chip++)
	{
		int i;
		int mixed_vol;
		int vol[4];		/* four separate outputs */
		char buf[4][40];
		const char *name[4];

		mixed_vol = intf_262->mixing_levelAB[chip];
		for (i=0; i<4; i++)
		{
			if (i==2) /*channels C ad D use separate field */
				mixed_vol = intf_262->mixing_levelCD[chip];
			vol[i] = mixed_vol & 0xffff;
			mixed_vol >>= 16;
			name[i] = buf[i];
			sprintf(buf[i],"%s #%d ch%c",sound_name(msound),chip,'A'+i);
			logerror("%s #%d ch%c vol %d\n",sound_name(msound),chip,'A'+i,mixed_vol);
		}
		stream_262[chip] = stream_init_multi(4,name,vol,rate,chip,YMF262UpdateYMFM);

		// OPL3 driver handler takes raw irq (0/1); glue passes 1/0
		chip_262[chip] = ymfm_opl_create(YMFM_OPL_YMF262, chip*2, intf_262->handler[chip], intf_262->baseclock, timercallback_262);
		vgm_idx_262[chip] = vgm_open(VGMC_YMF262, intf_262->baseclock);
		vgm_addressa_262[chip] = 0;
	}
	return 0;
}

void YMF262_sh_stop(void)
{
	int i;
	for (i = 0;i < intf_262->num;i++)
		ymfm_opl_destroy(chip_262[i]);
}

void YMF262_sh_reset(void)
{
	int i;
	for (i = 0;i < intf_262->num;i++)
	{
		ymfm_opl_invalidate_caches(chip_262[i]);
		ymfm_opl_reset(chip_262[i]);
	}
}

static void YMF262Write_ymfm(int n, int a, int v)
{
	switch (a & 3)
	{
		case 0: /* address port, register set #1 */
			vgm_addressa_262[n] = v;
			ymfm_opl_write(chip_262[n], 0, v);
			break;
		case 2: /* address port, register set #2 */
			vgm_addressa_262[n] = v | 0x100;
			ymfm_opl_write(chip_262[n], 2, v);
			break;
		case 1: /* data port (A1 ignored) */
		case 3:
			vgm_write(vgm_idx_262[n], vgm_addressa_262[n] >> 8, vgm_addressa_262[n] & 0xFF, v);
			ymfm_opl_write(chip_262[n], a & 3, v);
			break;
	}
}

/* chip #0 */
READ_HANDLER( YMF262_status_0_r ) {
	return ymfm_opl_read(chip_262[0], 0);
}
WRITE_HANDLER( YMF262_register_A_0_w ) {
	YMF262Write_ymfm(0, 0, data);
}
WRITE_HANDLER( YMF262_data_A_0_w ) {
	YMF262Write_ymfm(0, 1, data);
}
WRITE_HANDLER( YMF262_register_B_0_w ) {
	YMF262Write_ymfm(0, 2, data);
}
WRITE_HANDLER( YMF262_data_B_0_w ) {
	YMF262Write_ymfm(0, 3, data);
}

/* chip #1 */
READ_HANDLER( YMF262_status_1_r ) {
	return ymfm_opl_read(chip_262[1], 0);
}
WRITE_HANDLER( YMF262_register_A_1_w ) {
	YMF262Write_ymfm(1, 0, data);
}
WRITE_HANDLER( YMF262_data_A_1_w ) {
	YMF262Write_ymfm(1, 1, data);
}
WRITE_HANDLER( YMF262_register_B_1_w ) {
	YMF262Write_ymfm(1, 2, data);
}
WRITE_HANDLER( YMF262_data_B_1_w ) {
	YMF262Write_ymfm(1, 3, data);
}

#else /* legacy ymf262.c core */

static void *Timer_262[MAX_262*2];
static void IRQHandler_262(int n,int irq)
{
	if (intf_262->handler[n]) (intf_262->handler[n])(irq);
}
static void timer_callback_262(int param)
{
	int n=param>>1;
	int c=param&1;
	YMF262TimerOver(n,c);
}

static void TimerHandler_262(int c,double period)
{
	if( period == 0 )
	{	/* Reset FM Timer */
		timer_enable(Timer_262[c], 0);
	}
	else
	{	/* Start FM Timer */
		timer_adjust(Timer_262[c], period, c, 0);
	}
}


int YMF262_sh_start(const struct MachineSound *msound)
{
	int chip;
	double rate;// = Machine->sample_rate;

	intf_262 = msound->sound_interface;
	if( intf_262->num > MAX_262 ) return 1;

	//if (options.use_filter)
		rate = intf_262->baseclock/(8*36);

	/* Timer state clear */
	memset(Timer_262,0,sizeof(Timer_262));

	/* stream system initialize */
	if ( YMF262Init(intf_262->num,intf_262->baseclock,rate) != 0)
		return 1;

	for (chip = 0;chip < intf_262->num; chip++)
	{
		int i;
		int mixed_vol;
		int vol[4];		/* four separate outputs */
		char buf[4][40];
		const char *name[4];

		mixed_vol = intf_262->mixing_levelAB[chip];
		for (i=0; i<4; i++)
		{
			if (i==2) /*channels C ad D use separate field */
				mixed_vol = intf_262->mixing_levelCD[chip];
			vol[i] = mixed_vol & 0xffff;
			mixed_vol >>= 16;
			name[i] = buf[i];
			sprintf(buf[i],"%s #%d ch%c",sound_name(msound),chip,'A'+i);
			logerror("%s #%d ch%c vol %d\n",sound_name(msound),chip,'A'+i,mixed_vol);
		}
		stream_262[chip] = stream_init_multi(4,name,vol,rate,chip,YMF262UpdateOne);

		/* YMF262 setup */
		YMF262SetTimerHandler (chip, TimerHandler_262, chip*2);
		YMF262SetIRQHandler   (chip, IRQHandler_262, chip);
		YMF262SetUpdateHandler(chip, stream_update, stream_262[chip]);

		Timer_262[chip*2+0] = timer_alloc(timer_callback_262);
		Timer_262[chip*2+1] = timer_alloc(timer_callback_262);
	}
	return 0;
}

void YMF262_sh_stop(void)
{
	YMF262Shutdown();
}

/* reset */
void YMF262_sh_reset(void)
{
	int i;

	for (i = 0;i < intf_262->num;i++)
		YMF262ResetChip(i);
}

/* chip #0 */
READ_HANDLER( YMF262_status_0_r ) {
	return YMF262Read(0, 0);
}
WRITE_HANDLER( YMF262_register_A_0_w ) {
	YMF262Write(0, 0, data);
}
WRITE_HANDLER( YMF262_data_A_0_w ) {
	YMF262Write(0, 1, data);
}
WRITE_HANDLER( YMF262_register_B_0_w ) {
	YMF262Write(0, 2, data);
}
WRITE_HANDLER( YMF262_data_B_0_w ) {
	YMF262Write(0, 3, data);
}

/* chip #1 */
READ_HANDLER( YMF262_status_1_r ) {
	return YMF262Read(1, 0);
}
WRITE_HANDLER( YMF262_register_A_1_w ) {
	YMF262Write(1, 0, data);
}
WRITE_HANDLER( YMF262_data_A_1_w ) {
	YMF262Write(1, 1, data);
}
WRITE_HANDLER( YMF262_register_B_1_w ) {
	YMF262Write(1, 2, data);
}
WRITE_HANDLER( YMF262_data_B_1_w ) {
	YMF262Write(1, 3, data);
}

#endif /* HAS_YMF262_YMFM */

#endif
