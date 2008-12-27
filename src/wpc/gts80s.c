#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "cpu/tms7000/tms7000.h"
#include "machine/6530riot.h"
#include "machine/6532riot.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "sound/votrax.h"
#include "core.h"
#include "sndbrd.h"
#include "snd_cmd.h"

#include "gts80.h"
#include "gts80s.h"
#include "gts3.h"

//SJE: 03/01/03 - This driver needs to be cleaned up, I just don't have time yet..

/*
    Gottlieb System 80 Sound Boards

    - System 80/80A Sound Board

    - System 80/80A Sound & Speech Board
		- subtype 0: without SC-01 (Votrax) chip installed
		- subtype 1: SC-01 (Votrax) chip installed

    - System 80A Sound Board with a PiggyPack installed
	  (thanks goes to Peter Hall for providing some very useful information)

    - System 80B Sound Board (3 generations, plus an additional DAC board for Bone Busters only)

	- System 3 sound boards

*/


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound Board
/-----------------------------------------*/

#define GTS80S_BUFFER_SIZE 8192

static struct {
	struct sndbrdData boardData;

	int    stream;
	INT16  buffer[GTS80S_BUFFER_SIZE+1];
	double clock[GTS80S_BUFFER_SIZE+1];
	int    buf_pos;

	int    dips;
	UINT8* pRIOT6530_0_ram;
	int	   IRQEnabled;
} GTS80S_locals;

/* digital sound data output */
static WRITE_HANDLER(gts80s_riot6530_0a_w) {
//	logerror("riot6530_0a_w: 0x%02x\n", data);
	if ( GTS80S_locals.buf_pos>=GTS80S_BUFFER_SIZE )
		return;

	GTS80S_locals.clock[GTS80S_locals.buf_pos] = timer_get_time();
	GTS80S_locals.buffer[GTS80S_locals.buf_pos++] = ((data<<7)-0x4000)*2;
}

static WRITE_HANDLER(gts80s_riot6530_0b_w) {
//	logerror("riot6530_0b_w: 0x%02x\n", data);
	/* reset the interupt on the PiggyPack board */
	if ( GTS80S_locals.boardData.subType==1 )
		GTS80S_locals.IRQEnabled = data&0x40;
}

static struct riot6530_interface GTS80S_riot6530_intf = {
 /* 6530RIOT 0 (0x200) Chip U2 */
 /* PA0 - PA7 Digital sound data */
 /* PB0 - PB7 Sound latch & Dip-Switches */
 /* in  : A/B, */ NULL, NULL,
 /* out : A/B, */ gts80s_riot6530_0a_w, gts80s_riot6530_0b_w,
 /* irq :      */ NULL
};

static WRITE_HANDLER(gts80s_riot6530_0_ram_w)
{
	UINT8 *pMem = GTS80S_locals.pRIOT6530_0_ram + (offset%0x40);

	pMem[0x0000] = pMem[0x0040] = pMem[0x0080] = pMem[0x00c0] = \
	pMem[0x0100] = pMem[0x0140] = pMem[0x0180] = pMem[0x01c0] = \
	pMem[0x1000] = pMem[0x1040] = pMem[0x1080] = pMem[0x10c0] = \
	data;
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80S_readmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x03ff, riot6530_0_r},
{ 0x0400, 0x0fff, MRA_ROM},
{ 0x1000, 0x10ff, MRA_RAM},
{ 0xf800, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80S_writemem)
{ 0x0000, 0x01ff, gts80s_riot6530_0_ram_w},
{ 0x0200, 0x03ff, riot6530_0_w},
{ 0x0400, 0x0fff, MWA_ROM},
{ 0x1000, 0x10ff, gts80s_riot6530_0_ram_w},
{ 0xf800, 0xffff, MWA_ROM},
MEMORY_END

static void GTS80S_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;

	dCurrentClock = GTS80S_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80S_locals.clock[0]) / length;

	i = 0;
	GTS80S_locals.clock[GTS80S_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80S_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80S_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80S_locals.clock[0] = dActClock;
	GTS80S_locals.buffer[0] = GTS80S_locals.buffer[GTS80S_locals.buf_pos-1];
	GTS80S_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/

void gts80s_init(struct sndbrdData *brdData) {
	int i = 0;
	UINT8 *pMem;

	memset(&GTS80S_locals, 0x00, sizeof GTS80S_locals);

	GTS80S_locals.boardData = *brdData;
	GTS80S_locals.pRIOT6530_0_ram = memory_region(GTS80S_locals.boardData.cpuNo);

	/* init dips */
	GTS80S_locals.dips =
		((core_getDip(5)&0x01) ? 0x00:0x80) | /* S1: Sound/Tones        */
		((core_getDip(5)&0x02) ? 0x00:0x10);  /* S2: Attract Mode Tunes */

	/* init sound buffer */
	GTS80S_locals.clock[0]  = 0;
	GTS80S_locals.buffer[0] = 0;
	GTS80S_locals.buf_pos   = 1;

	if ( GTS80S_locals.boardData.subType==0 ) {
		/* clear the upper 4 bits, some ROM images aren't 0 */
		/* the 6530 RIOT ROM is not used by the boards which have a PiggyPack installed */
		pMem = memory_region(GTS80S_locals.boardData.cpuNo)+0x0400;
		for(i=0x0400; i<0x0bff; i++)
			*pMem++ &= 0x0f;

		memcpy(memory_region(GTS80S_locals.boardData.cpuNo)+0x1000, memory_region(GTS80S_locals.boardData.cpuNo)+0x0700, 0x100);
	}


	/*
		Init RAM, i.e. set base of all bank to the base of bank 1,
		the memory repeats ever 64 bytes; haven't found another way to
		tell the MAME core this situation; the problem is that the cpu *will*
		execute code in this area, so a usually read/writer handler fails
	*/
	for (i=1;i<=12;i++)
		cpu_setbank(i, memory_region(STATIC_BANK1));

	/* init the RIOT */
    riot6530_config(0, &GTS80S_riot6530_intf);
	riot6530_set_clock(0, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);
	riot6530_reset();

	GTS80S_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80S_Update);
	set_RC_filter(GTS80S_locals.stream, 270000, 15000, 0, 33000);
}

/*--------------
/  exit
/---------------*/

void gts80s_exit(int boardNo)
{
	riot6530_unconfig();
}

/*--------------
/  sound diag
/---------------*/
// NMI line: test switch S3
void gts80s_diag(int state) {
	cpu_set_nmi_line(GTS80S_locals.boardData.cpuNo, state ? ASSERT_LINE : CLEAR_LINE);
}

/*--------------
/  sound cmd
/---------------*/

static WRITE_HANDLER(gts80s_data_w)
{
	// logerror("sound latch: 0x%02x\n", data);
  if (Machine->drv->pinmame.coreDips < 32) { // this is a System1 game, so use all the bits!
	riot6530_set_input_b(0, data);
  } else {
	data &= 0x0f;
	riot6530_set_input_b(0, GTS80S_locals.dips | 0x20 | data);
	/* the PiggyPack board is really firing an interrupt; sound board layout
	   for theses game is different, because the IRQ line is connected */
	if ( (GTS80S_locals.boardData.subType==1) && data && GTS80S_locals.IRQEnabled )
		cpu_set_irq_line(GTS80S_locals.boardData.cpuNo, 0, PULSE_LINE);
  }
}

/* only in at the moment for the pinmame startup information */
struct CustomSound_interface GTS80S_customsoundinterface = {
	NULL, NULL, NULL
};

const struct sndbrdIntf gts80sIntf = {
  "GTS80", gts80s_init, gts80s_exit, gts80s_diag, gts80s_data_w, gts80s_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gts80s_s)
  MDRV_CPU_ADD_TAG("scpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80S_readmem, GTS80S_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, GTS80S_customsoundinterface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound & Speech Board
/-----------------------------------------*/

#define GTS80SS_BUFFER_SIZE 4096

static struct {
	struct sndbrdData boardData;
	int    device;
	int    diag;
	UINT8  riot3a;

	int    dips;
	UINT8* pRIOT6532_3_ram;

	int    stream;
	INT16  buffer[GTS80SS_BUFFER_SIZE+1];
	double clock[GTS80SS_BUFFER_SIZE+1];
	int	   buf_pos;

	void   *timer;
} GTS80SS_locals;

static void GTS80SS_irq(int state) {
	// logerror("IRQ: %i\n",state);
	cpu_set_irq_line(GTS80SS_locals.boardData.cpuNo, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static void GTS80SS_nmi(int state)
{
	// the Votrax chip is connected to the NMI line, simply return if no Votrax is installed (subtype=0)
	if ( GTS80SS_locals.boardData.subType==0 )
		return;

	// logerror("NMI: %i\n",state);
	cpu_set_nmi_line(GTS80SS_locals.boardData.cpuNo, state ? CLEAR_LINE : ASSERT_LINE);
}

static READ_HANDLER(GTS80SS_riot3a_r) {
	return GTS80SS_locals.riot3a;
}

/* Switch settings, test switch and NMI */
READ_HANDLER(GTS80SS_riot3b_r)  {
	if ( GTS80SS_locals.boardData.subType==0 )
		return (GTS80SS_locals.diag ? 0 : 0x40) | (GTS80SS_locals.dips^0x3f);
	else
		return (votraxsc01_status_r(0)?0x80:0x00) | (GTS80SS_locals.diag ? 0 : 0x40) | (GTS80SS_locals.dips^0x3f);
}

static WRITE_HANDLER(GTS80SS_riot3b_w) { logerror("riot3b_w: 0x%02x\n", data);}

/* D/A converter for volume */
static WRITE_HANDLER(GTS80SS_da1_latch_w) {
//	logerror("da1_w: 0x%02x\n", data);
	if ( GTS80SS_locals.buf_pos>=GTS80SS_BUFFER_SIZE )
		return;

	GTS80SS_locals.clock[GTS80SS_locals.buf_pos] = timer_get_time();
	GTS80SS_locals.buffer[GTS80SS_locals.buf_pos++] = ((data<<7)-0x4000)*2;

	//mixer_set_volume(GTS80SS_locals.stream, 100 * data / 255);
	GTS80SS_locals.device = 1;
}

/* D/A converter for voice clock */
static WRITE_HANDLER(GTS80SS_da2_latch_w) {
//	logerror("da2_w: 0x%02x\n", data);
	if (GTS80SS_locals.boardData.subType)
		votraxsc01_set_base_frequency(11025+(data*100));
	GTS80SS_locals.device = 3;
}

/* expansion board */
static READ_HANDLER(GTS80SS_ext_board_1_r) {
	GTS80SS_locals.device = 4;
	logerror("ext_board_1_r\n");
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_1_w) {
	GTS80SS_locals.device = 4;
	logerror("ext_board_1_w: 0x%02x\n", data);
}

static READ_HANDLER(GTS80SS_ext_board_2_r) {
	GTS80SS_locals.device = 5;
	logerror("ext_board_2_r\n");
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_2_w) {
	GTS80SS_locals.device = 5;
	logerror("ext_board_2_w: 0x%02x\n", data);
}

static READ_HANDLER(GTS80SS_ext_board_3_r) {
	GTS80SS_locals.device = 6;
	logerror("ext_board_3_r\n");
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_3_w) {
	GTS80SS_locals.device = 6;
	logerror("ext_board_3_w: 0x%02x\n", data);
}

/* voice synt latch */
static WRITE_HANDLER(GTS80SS_vs_latch_w) {
	logerror("vs_latch: %03x: %02x / %d\n", offset, data, GTS80SS_locals.device);
	if (GTS80SS_locals.boardData.subType) {
		if (GTS80SS_locals.device < 7) {
			mixer_set_volume(0, 100);
			mixer_set_volume(1, 100);
			mixer_set_volume(2, 100);
			mixer_set_volume(3, 100);
			votraxsc01_w(0, data^0x3f);
		}
	}
//	GTS80SS_locals.device = 2;
}

static struct riot6532_interface GTS80SS_riot6532_intf = {
 /* 6532RIOT 3: Sound/Speech board Chip U15 */
 /* in  : A/B, */ GTS80SS_riot3a_r, GTS80SS_riot3b_r,
 /* out : A/B, */ 0, GTS80SS_riot3b_w,
 /* irq :      */ GTS80SS_irq
};

static WRITE_HANDLER(GTS80SS_riot6532_3_ram_w)
{
	UINT8 *pMem = GTS80SS_locals.pRIOT6532_3_ram + (offset%0x80);

	pMem[0x0000] = pMem[0x0080] = pMem[0x0100] = pMem[0x0180] = data;
}

static READ_HANDLER(GTS80SS_riot6532_3_ram_r)
{
	return GTS80SS_locals.pRIOT6532_3_ram[offset%0x80];
}

/* A write here will cause the speech chip to mute */
static WRITE_HANDLER(empty_w)
{
	if (GTS80SS_locals.boardData.subType) {
		mixer_set_volume(0, 0);
		mixer_set_volume(1, 0);
		mixer_set_volume(2, 0);
		mixer_set_volume(3, 0);
	}
	GTS80SS_locals.device = 7;
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80SS_readmem)
{ 0x0000, 0x01ff, GTS80SS_riot6532_3_ram_r},
{ 0x0200, 0x0fff, riot6532_3_r},
{ 0x4000, 0x4fff, GTS80SS_ext_board_1_r},
{ 0x5000, 0x5fff, GTS80SS_ext_board_2_r},
{ 0x6000, 0x6fff, GTS80SS_ext_board_3_r},
{ 0x7000, 0x7fff, MRA_ROM},
{ 0x8000, 0x81ff, GTS80SS_riot6532_3_ram_r},
{ 0x8200, 0x8fff, riot6532_3_r},
{ 0xc000, 0xcfff, GTS80SS_ext_board_1_r},
{ 0xd000, 0xdfff, GTS80SS_ext_board_2_r},
{ 0xe000, 0xefff, GTS80SS_ext_board_3_r},
{ 0xf000, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80SS_writemem)
{ 0x0000, 0x01ff, GTS80SS_riot6532_3_ram_w},
{ 0x0200, 0x0fff, riot6532_3_w},
{ 0x1000, 0x1fff, GTS80SS_da1_latch_w},
{ 0x2000, 0x2fff, GTS80SS_vs_latch_w},
{ 0x3000, 0x3fff, GTS80SS_da2_latch_w},
{ 0x4000, 0x4fff, GTS80SS_ext_board_1_w},
{ 0x5000, 0x5fff, GTS80SS_ext_board_2_w},
{ 0x6000, 0x6fff, GTS80SS_ext_board_3_w},
{ 0x7000, 0x7fff, empty_w}, // the soundboard does fake writes to the ROM area (used for a delay function)
{ 0x8000, 0x81ff, GTS80SS_riot6532_3_ram_w},
{ 0x8200, 0x8fff, riot6532_3_w},
{ 0x9000, 0x9fff, GTS80SS_da1_latch_w},
{ 0xa000, 0xafff, GTS80SS_vs_latch_w},
{ 0xb000, 0xbfff, GTS80SS_da2_latch_w},
{ 0xc000, 0xcfff, GTS80SS_ext_board_1_w},
{ 0xd000, 0xdfff, GTS80SS_ext_board_2_w},
{ 0xe000, 0xefff, GTS80SS_ext_board_3_w},
{ 0xf000, 0xffff, empty_w}, // the soundboard does fake writes to the ROM area (used for a delay function)
MEMORY_END

static void GTS80_ss_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;

	dCurrentClock = GTS80SS_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80SS_locals.clock[0]) / length;

	if ( GTS80SS_locals.buf_pos>1 )
		GTS80SS_locals.buf_pos = GTS80SS_locals.buf_pos;

	i = 0;
	GTS80SS_locals.clock[GTS80SS_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80SS_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80SS_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80SS_locals.clock[0] = dActClock;
	GTS80SS_locals.buffer[0] = GTS80SS_locals.buffer[GTS80SS_locals.buf_pos-1];
	GTS80SS_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/

void gts80ss_init(struct sndbrdData *brdData) {
	int i;

	memset(&GTS80SS_locals, 0x00, sizeof GTS80SS_locals);
	GTS80SS_locals.boardData = *brdData;

	GTS80SS_locals.pRIOT6532_3_ram = memory_region(GTS80SS_locals.boardData.cpuNo);

	/* int dips */
	GTS80SS_locals.dips =
		  ((core_getDip(4)&0x01) ? 0x08:0x00) /* S1: Self-Test */
//		| ((core_getDip(4)&0x02) ? 0x40:0x00) /* S2: not used (goes to the expansion board, pin J1-1) */
		| ((core_getDip(4)&0x04) ? 0x20:0x00) /* S3: speech in attract mode (0) */
		| ((core_getDip(4)&0x08) ? 0x10:0x00) /* S4: speech in attract mode (1) */
		| ((core_getDip(4)&0x10) ? 0x04:0x00) /* S5: background music enabled */
		| ((core_getDip(4)&0x20) ? 0x02:0x00) /* S6: speech enabled */
		| ((core_getDip(4)&0x40) ? 0x01:0x00) /* S7: connected, usage unknown */
//		| ((core_getDip(4)&0x80) ? 0x80:0x00) /* S8: not used (goes to the expansion board, pin J1-17) */
    ;

	/*
		Init RAM, i.e. set base of all banks to the base of bank 1,
		the memory repeats ever 128 bytes; haven't found another way to
		tell the MAME core this situation; the problem is that the cpu *may*
		execute code in this area, so a usually read/writer handler fails
	*/
	for (i=1;i<=4;i++)
		cpu_setbank(i, memory_region(STATIC_BANK1));

	/* init RIOT */
    riot6532_config(3, &GTS80SS_riot6532_intf);
	riot6532_set_clock(3, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);

	GTS80SS_locals.clock[0]  = 0;
	GTS80SS_locals.buffer[0] = 0;
	GTS80SS_locals.buf_pos   = 1;

	for(i = 0; i<8; i++)
		memcpy(memory_region(GTS80SS_locals.boardData.cpuNo)+0x8000+0x1000*i, memory_region(GTS80SS_locals.boardData.cpuNo)+0x7000, 0x1000);

	GTS80SS_locals.stream = stream_init("SND DAC", 50, 11025, 0, GTS80_ss_Update);
	set_RC_filter(GTS80SS_locals.stream, 270000, 15000, 0, 10000);
}

/*--------------
/  exit
/---------------*/

void gts80ss_exit(int boardNo)
{
	if ( GTS80SS_locals.timer ) {
		timer_remove(GTS80SS_locals.timer);
		GTS80SS_locals.timer = 0;
	}
}

/*--------------
/  sound diag
/---------------*/
// RRIOT port B 0x40: test switch SW1
void gts80ss_diag(int state) {
	GTS80SS_locals.diag = state ? 1 : 0;
}

/*--------------
/  sound cmd
/---------------*/

WRITE_HANDLER(gts80ss_data_w)
{
	data &= 0x3f;
	GTS80SS_locals.riot3a = data | (data & 0x0f ? 0x80 : 0x00); /* | 0x40 */
	// logerror("sound_latch: 0x%02x\n", data);
	riot6532_set_input_a(3, GTS80SS_locals.riot3a);
}

/* only in at the moment for the pinmame startup information */
struct CustomSound_interface GTS80SS_customsoundinterface = {
	NULL, NULL, NULL
};

struct VOTRAXSC01interface GTS80SS_votrax_sc01_interface = {
	1,						/* 1 chip */
	{ 75 },					/* master volume */
	{ 7000 },				/* initial sampling frequency */
	{ &GTS80SS_nmi }		/* set NMI when busy signal get's low */
};

const struct sndbrdIntf gts80ssIntf = {
  "GTS80SS", gts80ss_init, gts80ss_exit, gts80ss_diag, gts80ss_data_w, gts80ss_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gts80s_ss)
  MDRV_CPU_ADD_TAG("scpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80SS_readmem, GTS80SS_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, GTS80SS_customsoundinterface)
  MDRV_SOUND_ADD(VOTRAXSC01, GTS80SS_votrax_sc01_interface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

/*----------------------------------------
/ Gottlieb Sys 80B Sound Board
/-----------------------------------------*/

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    cpuNo;             // # of the fist sound CPU
  int    ay_latch;			// Data Latch to AY-8913 chips
  int    ym2151_port;		// Bit determines if Registor or Data is written to YM2151
  int    nmi_rate;			// Programmable NMI rate
  void   *nmi_timer;		// Timer for NMI
  UINT8  dac_volume, dac2_volume;
  UINT8  dac_data, dac2_data;
  UINT8  speechboard_drq;	// Gen 1 Only
  UINT8  sp0250_latch;		// Gen 1 Only
  int firstCmd;
  int u2_latch;				// GTS3 - Store U2 data
  int enable_cs;			// GTS3 - OKI6295 ~CS pin
  int u3_latch;				// GTS3 - Latch U3
  int enable_w;				// GTS3 - OKI6295 ~WR pin
  int rom_cs;				// GTS3 - OKI6295 Rom Bank Select
  int nmi_enable;			// GTS3 - Enable NMI triggered by Programmable Circuit
} GTS80BS_locals;

//Speechboard IRQ callback, will be set to 1 while speech is busy..
static void speechboard_drq_w(int level)
{
	GTS80BS_locals.speechboard_drq = (level == ASSERT_LINE);
}

struct sp0250_interface GTS80BS_sp0250_interface =
{
	50,					/*Volume*/
	speechboard_drq_w	/*IRQ Callback*/
};


//Latch data to the SP0250
static WRITE_HANDLER(sp0250_latch) {
	GTS80BS_locals.sp0250_latch = data;
}

// Latch data for AY chips
static WRITE_HANDLER(s80bs_ay8910_latch_w)
{
	GTS80BS_locals.ay_latch = data;
}

//NMI Timer - Setup the next frequency for the timer to fire, and Trigger an NMI if enabled
static void nmi_callback(int param)
{
	//Reset the timing frequency
	double interval;
	int cl1, cl2;
	cl1 = 16-(GTS80BS_locals.nmi_rate&0x0f);
	cl2 = 16-((GTS80BS_locals.nmi_rate&0xf0)>>4);
	interval = (250000>>8);
	if(cl1>0)	interval /= cl1;
	if(cl2>0)	interval /= cl2;

	//Set up timer to fire again
	timer_set(TIME_IN_HZ(interval), 0, nmi_callback);

	//If enabled, fire the NMI for the Y CPU
	if(GTS80BS_locals.nmi_enable) {
		//logerror("PULSING NMI for Y-CPU\n");
		//timer_set(TIME_NOW, 1, NULL);
		cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_USEC(800));
		cpu_set_nmi_line(GTS80BS_locals.cpuNo, PULSE_LINE);
	}
}

static WRITE_HANDLER(s80bs_nmi_rate_w)
{
	GTS80BS_locals.nmi_rate = data;
	logerror("NMI RATE SET TO %d\n",data);
}

//Fire the NMI for the D CPU
static WRITE_HANDLER(s80bs_cause_dac_nmi_w)
{
	//if(!keyboard_pressed_memory_repeat(KEYCODE_A,2)) {
	//	logerror("PULSING NMI for D-CPU\n");
	//	timer_set(TIME_NOW, 1, NULL);
	cpu_set_nmi_line(GTS80BS_locals.cpuNo+1, PULSE_LINE);
	/* if this is BoneBusters, trigger the 2nd DAC CPU's NMI */
	if (GTS80BS_locals.cpuNo == 1 && cpu_gettotalcpu() == 4) {
		//logerror("BoneBusters NMI\n");
		cpu_set_nmi_line(GTS80BS_locals.cpuNo+2, PULSE_LINE);
	}
	//}
}

static READ_HANDLER(s80bs_cause_dac_nmi_r)
{
	s80bs_cause_dac_nmi_w(offset, 0);
	return 0;
}

#define test1

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(s80bs_sh_w)
{
#ifdef test1
	if(data != 0xff)
	{
		soundlatch_w(offset,data);
		cpu_set_irq_line(GTS80BS_locals.cpuNo, 0, HOLD_LINE);
		cpu_set_irq_line(GTS80BS_locals.cpuNo+1, 0, HOLD_LINE);
		/* if this is BoneBusters, trigger the 2nd DAC CPU's interrupt */
		if (GTS80BS_locals.cpuNo == 1 && cpu_gettotalcpu() == 4) {
			//logerror("BoneBusters IRQ\n");
			cpu_set_irq_line(GTS80BS_locals.cpuNo+2, 0, HOLD_LINE);
		}
	}
#else
	int clear_irq = 0;

	/*GTS3 Specific stuff*/
	if (core_gameData->hw.soundBoard & SNDBRD_GTS3)
	{
		clear_irq = (data==0xff);  /* clear the IRQ when ALL bits are specified */
	}
	else
	{
		clear_irq = ((data&0x0f)==0x0f); /* interrupt trigered by four low bits (not all 1's) - Not sure if this is TRUE, comes from the MAME driver*/
		data &= 0x3f;					 //Not sure if this is needed for ALL generations (but definitely not gts3!)
	}

	if (clear_irq) 	{
		cpu_set_irq_line(GTS80BS_locals.cpuNo, 0, CLEAR_LINE);
		cpu_set_irq_line(GTS80BS_locals.cpuNo+1, 0, CLEAR_LINE);
		if (GTS80BS_locals.cpuNo == 1 && cpu_gettotalcpu() == 4) {
			cpu_set_irq_line(GTS80BS_locals.cpuNo+2, 0, CLEAR_LINE);
		}
	}
	else {
		soundlatch_w(offset,data);
		cpu_set_irq_line(GTS80BS_locals.cpuNo, 0, HOLD_LINE);
		cpu_set_irq_line(GTS80BS_locals.cpuNo+1, 0, HOLD_LINE);
		if (GTS80BS_locals.cpuNo == 1 && cpu_gettotalcpu() == 4) {
			cpu_set_irq_line(GTS80BS_locals.cpuNo+2, 0, HOLD_LINE);
		}
	}
#endif
}

//Generation 1 Specific
/* bits 0-3 are probably unused (future expansion) */
/* bits 4 & 5 are two dip switches. Unused? */
/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */
/* bit 7 comes from the speech chip DATA REQUEST pin */
// GV 07/17/05: re-using the sound dips introduced with the older sound boards for now.
static READ_HANDLER(s80bs1_sound_input_r)
{
	int data = (core_getDip(4) >> 4) | ((core_getDip(5) & 0x03) << 4) | ((~core_getDip(4) & 0x01) << 6);
	if(GTS80BS_locals.speechboard_drq)	data |= 0x80;
	return data;
}

//Common to All Generations - Set NMI Timer Enable
static WRITE_HANDLER( common_sound_control_w )
{
	GTS80BS_locals.nmi_enable = data&0x01;
}

//Generation 1 sound control
static WRITE_HANDLER( s80bs1_sound_control_w )
{
	static int last;

	common_sound_control_w(offset, data);

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_0_w(0,GTS80BS_locals.ay_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_1_w(0,GTS80BS_locals.ay_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */
	//NO IDEA WHAT THIS DOES - No interface in the sp0250 emulation for it yet?

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
		sp0250_w(0,GTS80BS_locals.sp0250_latch);
	}

	/* bit 7 goes to the speech chip RESET pin */
	//No interface in the sp0250 emulation for it yet?
	last = data & 0x44;
}


//Generation 3 sound control
static WRITE_HANDLER( s80bs3_sound_control_w )
{
	common_sound_control_w(offset, data);
	/* Bit 7 selects YM2151 register or data port */
	GTS80BS_locals.ym2151_port = data & 0x80;
}

//Determine whether to write data to YM2151 Registers or Data Port
static WRITE_HANDLER( s80bs_ym2151_w )
{
	if (GTS80BS_locals.ym2151_port)
		YM2151_data_port_0_w(offset, data);
	else
		YM2151_register_port_0_w(offset, data);
}

//DAC Handling.. Set volume
static WRITE_HANDLER( s80bs_dac_vol_w )
{
	GTS80BS_locals.dac_volume = data;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
	//logerror("volume = %x\n",data);
	//DAC_data_w(0,data);
}
static WRITE_HANDLER( s80bs_dac2_vol_w )
{
	GTS80BS_locals.dac2_volume = data;
	DAC_data_16_w(1, GTS80BS_locals.dac2_volume * GTS80BS_locals.dac2_data);
}
//DAC Handling.. Set data to send
static WRITE_HANDLER( s80bs_dac_data_w )
{
	GTS80BS_locals.dac_data = data;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
	//DAC_data_w(0,data);
}
static WRITE_HANDLER( s80bs_dac2_data_w )
{
	GTS80BS_locals.dac2_data = data;
	DAC_data_16_w(1, GTS80BS_locals.dac2_volume * GTS80BS_locals.dac2_data);
}

//Process command from Main CPU
WRITE_HANDLER(gts80b_data_w)
{
    // ignore the first byte (probably comes in too early)
    if (!GTS80BS_locals.firstCmd) { GTS80BS_locals.firstCmd = 1; return; }
	data ^= 0xff;	/*Data is inverted from main cpu*/
	s80bs_sh_w(0,data);
//	snd_cmd_log(data);
}

/***************************/
/* GENERATION 1 MEMORY MAP */
/***************************/
//Y-CPU
MEMORY_READ_START( GTS80BS1_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x6000, 0x6000, s80bs1_sound_input_r },
	{ 0xa800, 0xa800, soundlatch_r },
	{ 0xb000, 0xb000, s80bs_cause_dac_nmi_r }, /*Trigger D-CPU NMI*/
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x2000, sp0250_latch },	/* speech chip. The game sends strings */
										/* of 15 bytes (clocked by 4000). The chip also */
										/* checks a DATA REQUEST bit in 6000. */
	{ 0x4000, 0x4000, s80bs1_sound_control_w },
	{ 0x8000, 0x8000, s80bs_ay8910_latch_w },
	{ 0xa000, 0xa000, s80bs_nmi_rate_w },	   /* set Y-CPU NMI rate */
	{ 0xb000, 0xb000, s80bs_cause_dac_nmi_w }, /*Trigger D-CPU NMI*/
MEMORY_END
//D-CPU
MEMORY_READ_START( GTS80BS1_readmem2 )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem2 )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4000, s80bs_dac_vol_w },
	{ 0x4001, 0x4001, s80bs_dac_data_w},
MEMORY_END
/***************************/
/* GENERATION 2 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, s80bs1_sound_input_r }, // at least Victory is using the test dip switch!
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x2000, 0x2000, MWA_NOP },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x8000, 0x8000, s80bs_ay8910_latch_w},
{ 0xa000, 0xa000, s80bs1_sound_control_w},
{ 0xa001,0xffff, MWA_ROM},
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

/***************************/
/* GENERATION 3 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_yreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_ywritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0xa000, 0xa000, s80bs3_sound_control_w },
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_dreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_dwritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_d2writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac2_vol_w },
{ 0x8001, 0x8001, s80bs_dac2_data_w},
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/
static struct DACinterface GTS80BS_dacInt =
{
	2,			/* 2 Chips - but it seems we only access 1, except for BoneBusters */
	{50,50}		/* Volume */
};

static struct AY8910interface GTS80BS_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 25, 25 }, /* Volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2151interface GTS80BS_ym2151Int =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) },
	{ 0 }
};

/* System 3 Sound:

   Hardware is almost the same as Generation 3 System 80b boards,
   except for an additonal board with an OKI 6295 chip on it.

   CPU: 2x(6502): DAC: 1x(AD7528): DSP: 1x(YM2151): OTHER: OKI6295 (Speech)
*/

/*--------------
/  Memory map
/---------------
  CPU #1 (Y-CPU):
  ---------------

  S2 - LS139
  A11 A12
  -------
  0    0 = (0x6000) = Y0 = S5 LS374 Chip Select
  1    0 = (0x6800) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x7000) = Y2 = CPU #2 - Trigger NMI
  1    1 = (0x7800) = Y3 = S4-13 = Clock data to U2 Latch

  T4 - F138
  A13 A14 A15
  -----------
    0   0   0 = Y0 = RAM Enable (0x1fff)
    0   1   0 = Y2 = S4-14 = 2151 Enable  (0x4000)
	1   1   0 = Y3 = S2-LS139 Enable(0x6000)
	1   0   1 = Y5 = Enable G3-LS377 (0xA000)

  CPU #2 (D-CPU):
  ---------------

  S2 - LS139
  A14 A15
  -------
  0    0 = (<0x4000) = Y0 = RAM Enable
  1    0 = (0x4000) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x8000) = Y2 = Enable DAC (E2 - AD7528)

*/

// Send data to 6295, but only if Chip Selected and Write Enabled!
static void oki6295_w(void)
{
	static int last_d7=0;

	if ( GTS80BS_locals.enable_cs && GTS80BS_locals.enable_w ) {
		if ( !last_d7 && GTS80BS_locals.u2_latch&0x80  )
			logerror("START OF SAMPLE!\n");

		OKIM6295_data_0_w(0, GTS80BS_locals.u2_latch);
		logerror("OKI Data = %x\n", GTS80BS_locals.u2_latch);
	}
	else {
		logerror("NO OKI: cs=%x w=%x\n", GTS80BS_locals.enable_cs, GTS80BS_locals.enable_w);
	}
	last_d7 = (GTS80BS_locals.u2_latch>>7)&1;
}

static WRITE_HANDLER(u2latch_w)
{
	GTS80BS_locals.u2_latch = data;
	logerror("u2_latch: %02x\n", GTS80BS_locals.u2_latch);
}

/*
  G3 - LS377 (clocked at 0xa000)
  ----------
  D0 = Enable CPU #1 NMI - In conjunction with programmable timer circuit
  D1 = Sound CPU #1 Diag LED
  D2-D4 = NA?
  D5 = S4-11 = DCLCK2 = (Clock data into U3 Latch - From U2 Latch or from 6295 if ~CS and ~RD)
  D6 = S4-12 = ~WR = 6295 Write Enabled
  D7 = S4-15 = YM2151 - A0

*/
static WRITE_HANDLER(sound_control_w)
{
	int hold_enable_w, hold_enable_cs, hold_u3;
	hold_enable_w = GTS80BS_locals.enable_w;
	hold_enable_cs = GTS80BS_locals.enable_cs;
	hold_u3 = GTS80BS_locals.u3_latch;

	//Process common bits (D0 for NMI, D7 for YM2151)
	s80bs3_sound_control_w(0, data);

	//D1 = LED
	UpdateSoundLEDS(0,(data>>1)&1);

	//D2 - D4 = NA?

	//D5 = Clock in U3 Data
	GTS80BS_locals.u3_latch = (data>>5)&1;
	//if(GTS80BS_locals.u3_latch != hold_u3)
	  //logerror("U3 Latch = %x\n", GTS80BS_locals.u3_latch);

	//D6 = ~WR = 6295 Write Enabled (Active Low)
	GTS80BS_locals.enable_w = ((~data)>>6)&1;
	//if(GTS80BS_locals.enable_w != hold_enable_w)
	  //logerror("~wr = %x\n", (data>>6)&1);

	//Handle U3 Latch - On Positive Edge
	if (offset && GTS80BS_locals.u3_latch && !hold_u3 ) { // OKI chip related

	/*
		U3 - LS374 (Data is fed from the U2 Latch)
	   ----------
		D0 = VUP/DOWN?? - Connects to optional U12 (volume control?)
		D1 = VSTEP??    - Connects to optional U12 (volume control?)
		D2 = 6295 Chip Select (Active Low)
		D3 = ROM Select (0 = Rom1, 1 = Rom2)
		D4 = 6295 - SS (Data = 1: normal sample rate; Data = 0: decreased by 25 percent)
		D5 = LED (Active low?)
		D6 = SRET1 (Where is this connected?), serves as 2nd chip select line
		D7 = SRET2 (Where is this connected?) + /PGM of roms, with optional jumper to +5 volts
	*/

		//D2 = 6295 Chip Select (Active Low)
		GTS80BS_locals.enable_cs = ((~GTS80BS_locals.u2_latch)>>2)&1;
		//logerror("~cs = %x\n", (GTS80BS_locals.u2_latch>>2)&1);

		//D3 = ROM Select (D6 also used on games with more than 2 x 256K rom area
		GTS80BS_locals.rom_cs = ((GTS80BS_locals.u2_latch>>2)&2) | ((GTS80BS_locals.u2_latch>>6)&1);
		OKIM6295_set_bank_base(0, GTS80BS_locals.rom_cs*0x40000);
		logerror("Setting to rom #%d\n", GTS80BS_locals.rom_cs);

		//D4 = 6295 - SS (Data = 1 = 7.575Khz; Data = 0 = 6.06 kHz (at 1MHz oscillation clock!)
		OKIM6295_set_frequency(0,((GTS80BS_locals.u2_latch>>4)&1)? 7575.76 : 6060.61);

		//D5 = LED (Active low?)
		UpdateSoundLEDS(1,~(GTS80BS_locals.u2_latch>>5)&1);

		//D6 = SRET1 (Where is this connected?)
		//D7 = SRET2 (Where is this connected?) + /PGM of roms, with optional jumper to +5 volts
	}

	//Trigger Command on Positive Edge
	if (offset && GTS80BS_locals.enable_w && !hold_enable_w) oki6295_w();
}

// sound control with OKI chip
static WRITE_HANDLER(sound_control_oki) {
	sound_control_w(1, data);
}

static READ_HANDLER(s3_soundlatch_y)
{
#ifdef test1
	cpu_set_irq_line(GTS80BS_locals.cpuNo, 0, CLEAR_LINE);
#endif
	return soundlatch_r(0);
}

static READ_HANDLER(s3_soundlatch_d)
{
#ifdef test1
	cpu_set_irq_line(GTS80BS_locals.cpuNo+1, 0, CLEAR_LINE);
#endif
	return soundlatch_r(0);
}

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS3_yreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, s3_soundlatch_y},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_ywritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_w},
{ 0x7800, 0x7800, u2latch_w},
{ 0x8000, 0x8000, MWA_NOP },
{ 0xa000, 0xa000, sound_control_w },
MEMORY_END
MEMORY_WRITE_START(GTS3_ywritemem_oki)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w },
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_w },
{ 0x7800, 0x7800, u2latch_w },
{ 0x8000, 0x8000, MWA_NOP },
{ 0xa000, 0xa000, sound_control_oki },
MEMORY_END

/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS3_dreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, s3_soundlatch_d},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_dwritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

static struct DACinterface GTS3_dacInt =
{
  2,			/*2 Chips - but it seems we only access 1?*/
 {100,100}		/* Volume */
};

static struct OKIM6295interface GTS3_okim6295_interface = {
	1,						/* 1 chip */
	{ 7575.76 },			/* base frequency */
	{ GTS3_MEMREG_SROM1 },	/* memory region */
	{ 50 }
};

// Init
void gts80b_init(struct sndbrdData *brdData) {
    int drq = GTS80BS_locals.speechboard_drq;
	//GTS80BS_locals.nmi_timer = NULL;
	memset(&GTS80BS_locals, 0, sizeof(GTS80BS_locals));
	GTS80BS_locals.cpuNo = brdData->cpuNo;
	//Start the programmable timer circuit
	timer_set(TIME_IN_HZ(250000>>8), 0, nmi_callback);
	//Must not be cleared here because it's set by the sp0250 chip earlier
	GTS80BS_locals.speechboard_drq = drq;
}

// Cleanup
void gts80b_exit(int boardNo)
{
	if(GTS80BS_locals.nmi_timer)
		timer_remove(GTS80BS_locals.nmi_timer);
	GTS80BS_locals.nmi_timer = NULL;
}

const struct sndbrdIntf gts80bIntf = {
  "GTS80B", gts80b_init, gts80b_exit, NULL, gts80b_data_w, gts80b_data_w, NULL, NULL, NULL, 0 //SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

//System S80B - Gen 1
MACHINE_DRIVER_START(gts80s_b1)
  MDRV_CPU_ADD_TAG("y-cpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS1_readmem, GTS80BS1_writemem)

  MDRV_CPU_ADD_TAG("d-cpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS1_readmem2, GTS80BS1_writemem2)

  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)
  MDRV_SOUND_ADD(AY8910, GTS80BS_ay8910Int)
  MDRV_SOUND_ADD(SP0250, GTS80BS_sp0250_interface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END
//System S80B - Gen 2
MACHINE_DRIVER_START(gts80s_b2)
  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS2_readmem, GTS80BS2_writemem)

  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS2_readmem2, GTS80BS2_writemem2)

  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)
  MDRV_SOUND_ADD(AY8910, GTS80BS_ay8910Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END
//System S80B - Gen 3
MACHINE_DRIVER_START(gts80s_b3)
  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS3_yreadmem, GTS80BS3_ywritemem)

  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS3_dreadmem, GTS80BS3_dwritemem)

  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)
  MDRV_SOUND_ADD(YM2151, GTS80BS_ym2151Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

//System S80B - Gen 3 with additional DAC (Bone Busters only)
MACHINE_DRIVER_START(gts80s_b3a)
  MDRV_IMPORT_FROM(gts80s_b3)

  MDRV_CPU_ADD_TAG("d-cpu2", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS3_dreadmem, GTS80BS3_d2writemem)
MACHINE_DRIVER_END

//System GTS3 without OKI chip
MACHINE_DRIVER_START(gts80s_s3_no)
  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_yreadmem, GTS3_ywritemem)

  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_dreadmem, GTS3_dwritemem)

  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_SOUND_ADD(DAC, GTS3_dacInt)
  MDRV_SOUND_ADD(YM2151,  GTS80BS_ym2151Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_DIAGNOSTIC_LEDH(3)
MACHINE_DRIVER_END

//System GTS3 with OKI chip
MACHINE_DRIVER_START(gts80s_s3)
  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_yreadmem, GTS3_ywritemem_oki)

  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_dreadmem, GTS3_dwritemem)

  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_SOUND_ADD(DAC, GTS3_dacInt)
  MDRV_SOUND_ADD(YM2151,  GTS80BS_ym2151Int)
  MDRV_SOUND_ADD(OKIM6295, GTS3_okim6295_interface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END



/* TECHNOPLAY sound board -

   Pretty much stole the Gottlieb System 80b Generation 1 sound board, then
   added a TMS7000 and an extra DAC for additional sounds.

   In fact, Technoplay's "Scramble" even uses the exact same sound roms
   for the D- and Y-CPUs as Gottlieb's "Raven"!
*/

/*----------------
/  Local variables
/-----------------*/
static struct {
  struct sndbrdData brdData;
  int    ay_latch;			// Data Latch to AY-8913 chips
  int    nmi_rate;			// Programmable NMI rate
  void   *nmi_timer;		// Timer for NMI (NOT USED ANYMORE?)
  int	 nmi_enable;		// Enable NMI triggered by Programmable Circuit
  int    snd_data;			// Command from cpu
  UINT8  dac_volume;
  UINT8  dac_data;
  UINT8  speechboard_drq;
  UINT8  sp0250_latch;
} techno_locals;

// Latch data for AY chips
WRITE_HANDLER(techno_ay8910_latch_w)
{
	techno_locals.ay_latch = data;
}

//NMI Timer - Setup the next frequency for the timer to fire, and Trigger an NMI if enabled
static void techno_nmi_callback(int param)
{
	//Reset the timing frequency
	double interval;
	int cl1, cl2;
	cl1 = 16-(techno_locals.nmi_rate&0x0f);
	cl2 = 16-((techno_locals.nmi_rate&0xf0)>>4);
	interval = (250000>>8);
	if(cl1>0)	interval /= cl1;
	if(cl2>0)	interval /= cl2;

	//Set up timer to fire again
	timer_set(TIME_IN_HZ(interval), 0, techno_nmi_callback);

	//If enabled, fire the NMI for the Y CPU
	if(techno_locals.nmi_enable) {
		//seems to have no effect!
		//cpu_boost_interleave(TIME_IN_USEC(10), TIME_IN_USEC(800));
		cpu_set_nmi_line(techno_locals.brdData.cpuNo, PULSE_LINE);
	}
}

WRITE_HANDLER(techno_nmi_rate_w)
{
	techno_locals.nmi_rate = data;
	logerror("NMI RATE SET TO %d\n",data);
}

//Fire the NMI for the 2nd 6502
WRITE_HANDLER(techno_cause_dac_nmi_w)
{
	cpu_set_nmi_line(techno_locals.brdData.cpuNo+1, PULSE_LINE);
}

READ_HANDLER(techno_cause_dac_nmi_r)
{
	techno_cause_dac_nmi_w(offset, 0);
	return 0;
}

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(techno_sh_w)
{
	techno_locals.snd_data = data;
	cpu_set_irq_line(techno_locals.brdData.cpuNo, 0, HOLD_LINE);
	cpu_set_irq_line(techno_locals.brdData.cpuNo+1, 0, HOLD_LINE);
	//Bit 6 if NOT set, fires TMS IRQ 1
	if(~data & 0x40) cpu_set_irq_line(techno_locals.brdData.cpuNo+2, TMS7000_IRQ1_LINE, ASSERT_LINE);
}

/* bits 0-3 are probably unused (future expansion) */
/* bits 4 & 5 are two dip switches. Unused? */
/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */
/* bit 7 comes from the speech chip DATA REQUEST pin */
READ_HANDLER(techno_sound_input_r)
{
	int data = 0x40;
	if(techno_locals.speechboard_drq)	data |= 0x80;
	return data;
}

//Generation 1 sound control
WRITE_HANDLER( techno_sound_control_w )
{
	static int last;

	techno_locals.nmi_enable = data&0x01;

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,techno_locals.ay_latch);
			else
				AY8910_write_port_0_w(0,techno_locals.ay_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,techno_locals.ay_latch);
			else
				AY8910_write_port_1_w(0,techno_locals.ay_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */
	//NO IDEA WHAT THIS DOES - No interface in the sp0250 emulation for it yet?

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
		sp0250_w(0,techno_locals.sp0250_latch);
	}

	/* bit 7 goes to the speech chip RESET pin */
	//No interface in the sp0250 emulation for it yet?
	last = data & 0x44;
}

// Init
static void tsns_init(struct sndbrdData *brdData) {
	memset(&techno_locals, 0, sizeof(techno_locals));
	techno_locals.brdData = *brdData;
	//Start the programmable timer circuit
	timer_set(TIME_IN_HZ(250000>>8), 0, techno_nmi_callback);
	//Set bank
	cpu_setbank(1, techno_locals.brdData.romRegion + 0x10000);
	//Must be 1 to start or speechboard will never work
	techno_locals.speechboard_drq = 1;
}

// Cleanup
void tsns_exit(int boardNo)
{
	if(techno_locals.nmi_timer)
		timer_remove(techno_locals.nmi_timer);
	techno_locals.nmi_timer = NULL;
}

static WRITE_HANDLER(tsns_data_w) {
  data ^= 0xff;	/*Data is inverted from main cpu*/

  //Bit 7 is the strobe - so commands are sent 2x, once with strobe set, then again, with it cleared..
  //Doesn't seem to matter much if we put this in or leave it out..
  //if(data & 0x80)
	//  return;

  logerror("tsns_data_w = %x\n",data);
  techno_sh_w(0,data);
}

static READ_HANDLER(techno_snd_r)
{
//	cpu_set_irq_line(techno_locals.brdData.cpuNo, 0, CLEAR_LINE);
	return techno_locals.snd_data;
}

static READ_HANDLER(techno_b_snd_r)
{
//	cpu_set_irq_line(techno_locals.brdData.cpuNo+1, 0, CLEAR_LINE);
	return techno_locals.snd_data;
}

//Read data command
READ_HANDLER(tms_porta_r)
{
	int data = techno_locals.snd_data & 0x1f;	//Only bits 0-4 used
	logerror("reading porta =%x\n",data);
	return data;
}

//Should not be used for input
READ_HANDLER(tms_portb_r)
{
	logerror("reading portb\n");
	return 0;
}
//Should not be used since it's used for address and data lines
READ_HANDLER(tms_portc_r)
{
	logerror("reading portc\n");
	return 0;
}
//Should not be used since it's used for address and data lines
READ_HANDLER(tms_portd_r)
{
	logerror("reading portd\n");
	return 0;
}

//Should not be used for output?
WRITE_HANDLER(tms_porta_w) { logerror("writing port a = %x\n",data); }

/*
D0 = U25 ROM Select
D1 = U36 ROM Select
D2 = NA?
D3 = NA?
D4-D7 = Used for Bus Control
*/
WRITE_HANDLER(tms_portb_w)
{
	cpu_setbank(1, techno_locals.brdData.romRegion + (0x10000) + ((data&2)>>1)*0x8000);
}

//should not be used since it's used for address and data lines
WRITE_HANDLER(tms_portc_w){	logerror("writing port c = %x\n",data); }
WRITE_HANDLER(tms_portd_w){	logerror("writing port d = %x\n",data); }

//Speechboard IRQ callback, will be set to 1 while speech is busy..
static void techno_speechboard_drq_w(int level)
{
	techno_locals.speechboard_drq = (level == ASSERT_LINE);
}

//Latch data to the SP0250
static WRITE_HANDLER(techno_sp0250_latch) {
	techno_locals.sp0250_latch = data;
}

//DAC Handling.. Set volume
static WRITE_HANDLER( techno_dac_vol_w )
{
	techno_locals.dac_volume = data;
	DAC_data_16_w(0, techno_locals.dac_volume * techno_locals.dac_data);
}
//DAC Handling.. Set data to send
static WRITE_HANDLER( techno_dac_data_w )
{
	techno_locals.dac_data = data;
	DAC_data_16_w(0, techno_locals.dac_volume * techno_locals.dac_data);
}

//TMS7000 will use dac chip #1
static WRITE_HANDLER(DAC_TMS_w)
{
	DAC_data_w(1,data);
}

static void tsns_diag(int button) {
  cpu_set_nmi_line(techno_locals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

const struct sndbrdIntf technoIntf =
{"TECHNO", tsns_init, tsns_exit, tsns_diag, tsns_data_w, tsns_data_w, NULL, NULL, NULL, 0 //SNDBRD_NODATASYNC
};

struct AY8910interface techno_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 25, 25 }	/* Volume */
};

struct DACinterface techno_6502dacInt =
{
	2,			/* 2 chips */
	{50,50}		/* Volume */
};

struct sp0250_interface techno_sp0250_interface =
{
	50,							/* Volume */
	techno_speechboard_drq_w	/*IRQ Callback*/
};

//6502 #1 CPU
MEMORY_READ_START( m6502_readmem )
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x6000, 0x6000, techno_sound_input_r },
  { 0xa800, 0xa800, techno_snd_r },
  { 0xb000, 0xb000, techno_cause_dac_nmi_r },	/* Trigger D-CPU NMI */
  { 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( m6502_writemem )
  { 0x0000, 0x07ff, MWA_RAM },
  { 0x2000, 0x2000, techno_sp0250_latch },	/* speech chip. The game sends strings */
											/* of 15 bytes (clocked by 4000). The chip also */
											/* checks a DATA REQUEST bit in 6000. */
  { 0x4000, 0x4000, techno_sound_control_w },
  { 0x8000, 0x8000, techno_ay8910_latch_w },
  { 0xa000, 0xa000, techno_nmi_rate_w },	/* set Y-CPU NMI rate */
  { 0xb000, 0xb000, techno_cause_dac_nmi_w },	/* Trigger D-CPU NMI */
MEMORY_END

//6502 #2 CPU
MEMORY_READ_START( m6502_b_readmem )
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x8000, 0x8000, techno_b_snd_r },
  { 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( m6502_b_writemem )
  { 0x0000, 0x07ff, MWA_RAM },
  { 0x4000, 0x4000, techno_dac_vol_w},
  { 0x4001, 0x4001, techno_dac_data_w},
MEMORY_END

//TMS7000 CPU
static MEMORY_READ_START(tms_readmem)
  { 0x0000, 0x7fff, MRA_BANK1 },		/* ROM BANK */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(tms_writemem)
  { 0x8000, 0x8000, DAC_TMS_w },		/* DAC */
MEMORY_END

static PORT_READ_START(tms_readport)
  { TMS7000_PORTA, TMS7000_PORTA, tms_porta_r },
  { TMS7000_PORTB, TMS7000_PORTB, tms_portb_r },
  { TMS7000_PORTA, TMS7000_PORTC, tms_portc_r },
  { TMS7000_PORTB, TMS7000_PORTD, tms_portd_r },
PORT_END
static PORT_WRITE_START(tms_writeport)
  { TMS7000_PORTA, TMS7000_PORTA, tms_porta_w },
  { TMS7000_PORTB, TMS7000_PORTB, tms_portb_w },
  { TMS7000_PORTA, TMS7000_PORTC, tms_portc_w },
  { TMS7000_PORTB, TMS7000_PORTD, tms_portd_w },
PORT_END

MACHINE_DRIVER_START(techno)
  MDRV_CPU_ADD(M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(m6502_readmem, m6502_writemem)
  MDRV_SOUND_ADD(AY8910, techno_ay8910Int)
  MDRV_SOUND_ADD(SP0250, techno_sp0250_interface)

  MDRV_CPU_ADD(M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(m6502_b_readmem, m6502_b_writemem)
  MDRV_SOUND_ADD(DAC, techno_6502dacInt)

  //MDRV_CPU_ADD(TMS7000, 4000000)
  MDRV_CPU_ADD(TMS7000, 1500000)		//Sounds much better at 1.5Mhz than 4Mhz
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(tms_readmem, tms_writemem)
  MDRV_CPU_PORTS(tms_readport, tms_writeport)
MACHINE_DRIVER_END
