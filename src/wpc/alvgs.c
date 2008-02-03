/************************************************************************************************
  Alvin G & Co. Sound Hardware
  ----------------------------
  by Steve Ellenoff

  Hardware from 1991-1994

  Generation #1 (Games Up to Al's Garage Band?)
    CPU: 6809 @ 2 Mhz
	I/O: 6255 VIA
	SND: YM3812 (Music), OKI6295 (Speech)

  Generation #2 (All remaining games)
	CPU: 68B09 @ 8 Mhz? (Sound best @ 1 Mhz)
	I/O: buffers
	SND: BSMT2000 @ 24Mhz

*************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "cpu/m6809/m6809.h"
#include "alvg.h"
#include "alvgs.h"
#include "machine/6821pia.h"
#include "sound/3812intf.h"
#include "machine/6522via.h"
#include "sndbrd.h"

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  int data_to_main_cpu;
} alvgslocals;

//common
static WRITE_HANDLER(data_to_main_cpu) { alvgslocals.data_to_main_cpu = data; }


/******************************************************************************************************************
														GENERATION #1
******************************************************************************************************************/

#define ALVGS1_SNDCPU_FREQ  2000000		//Schem shows an 8Mhz clock, but often we need to divide by 4 to make it work in MAME.

/*Declarations*/
extern WRITE_HANDLER(alvg_sndCmd_w);
static void alvgs1_init(struct sndbrdData *brdData);
static WRITE_HANDLER(alvgs1_data_w);
static WRITE_HANDLER(alvgs1_ctrl_w);
static READ_HANDLER(alvgs1_ctrl_r);

/* handler called by the 3812 when the internal timers cause an IRQ */
//Odd - this doesn't seem to get called?
static void ym3812_irq(int irq)
{
	cpu_set_irq_line(ALVGS_CPUNO, M6809_IRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Interfaces*/
static struct YM3812interface alvgs1_ym3812_intf =
{
	1,						/* 1 chip */
	4000000,				/* 4 MHz */
	{ 100 },				/* volume */
	{ ym3812_irq },			/* IRQ Callback */
};
static struct OKIM6295interface alvgs1_okim6295_intf =
{
	1,						/* 1 chip */
	{ 7575.76 },			/* sample rate at 1MHz clock */
	{ ALVGS_ROMREGION },	/* ROM REGION */
	{ 50 }					/* Volume */
};

/* Sound board */
const struct sndbrdIntf alvgs1Intf = {
   "OKI", alvgs1_init, NULL, NULL, alvg_sndCmd_w, alvgs1_data_w, NULL, alvgs1_ctrl_w, alvgs1_ctrl_r, SNDBRD_NODATASYNC
};

/*Functions*/

//SOUND DATA & CONTROL FUNCTIONS
static WRITE_HANDLER(alvgs1_data_w){ soundlatch_w(0,data); }
static WRITE_HANDLER(alvgs1_ctrl_w){
	//simulate transition
	via_2_ca2_w(0,0);
	via_2_ca2_w(0,1);
}
static READ_HANDLER(alvgs1_ctrl_r){	return alvgslocals.data_to_main_cpu; }

/* Addressing..U26 - 74LS138

A14 A13 A12 Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7
  0   0   0  0  1  1  1  1  1  1  1  Y0 = NC?              (<0x1000)
  0   0   1  1  0  1  1  1  1  1  1  Y1 = NC?              (0x1000)
  0   1   0  1  1  0  1  1  1  1  1  Y2 = YM3812 Enable    (0x2000)
  0   1   1  1  1  1  0  1  1  1  1  Y3 = RAM Enable       (0x3000)
  1   0   0  1  1  1  1  0  1  1  1  Y4 = 6295 Enable      (0x4000)
  1   0   1  1  1  1  1  1  0  1  1  Y5 = 6255 Enable      (0x5000)
  1   1   0  1  1  1  1  1  1  0  1  Y6 = Watch Dog Enable (0x6000)
  1   1   1  1  1  1  1  1  1  1  0  Y7 = NC?              (0x7000)
*/


//PA0-7: (IN) - Sound Data from Main CPU
static READ_HANDLER( xvia_2_a_r ) {
	int data = soundlatch_r(0);
	return data;
}

//PB0-7: (OUT) - Bit 6 = LED
static WRITE_HANDLER( xvia_2_b_w ) {
	alvg_UpdateSoundLEDS(0,(data&0x40)>>6);
	//LOG(("WARNING: SOUND VIA -Port B Write = %x\n",data));
}

//IRQ:  FIRQ to Main CPU
static void via_irq(int state) {
	//printf("IN SOUND VIA_IRQ - STATE = %x\n",state);
	cpu_set_irq_line(ALVGS_CPUNO, M6809_FIRQ_LINE, state?ASSERT_LINE:CLEAR_LINE);
}

/**********************************/
/* NONE OF THESE SHOULD BE CALLED */
/**********************************/

//PB0-7: (IN) - N.C.
static READ_HANDLER( xvia_2_b_r ) { LOG(("WARNING: SOUND VIA -Port B Read\n")); return 0; }
//CA1: (IN) - Sound Control
static READ_HANDLER( xvia_2_ca1_r ) { LOG(("WARNING: SOUND VIA CA1 Read\n")); return 0; }
//CB1: (IN) - N.C.
static READ_HANDLER( xvia_2_cb1_r ) { LOG(("WARNING: SOUND VIA CB1 Read\n")); return 0; }
//CA2:  (IN) - N.C.
static READ_HANDLER( xvia_2_ca2_r ) { LOG(("WARNING: SOUND VIA CA2 Read\n")); return 0; }
//CB2: (IN) - N.C.
static READ_HANDLER( xvia_2_cb2_r ) { LOG(("WARNING: SOUND VIA CB2 Read\n")); return 0; }
//PA0-7: (OUT)
static WRITE_HANDLER( xvia_2_a_w ) { LOG(("WARNING: SOUND VIA -Port A Write = %x\n",data)); }
//CA2: (OUT)
static WRITE_HANDLER( xvia_2_ca2_w ) { LOG(("WARNING: SOUND VIA CA2 Write = %x\n",data)); }
//CB2: (OUT)
static WRITE_HANDLER( xvia_2_cb2_w ) { LOG(("WARNING: SOUND VIA CB2 Write = %x\n",data)); }


/* CPU MEMORY MAP */

//Unexplained Read @ 1000 and some data being written to 0f,10 in memory..

static MEMORY_READ_START(alvgs1_readmem)
  { 0x2000, 0x2000, YM3812_status_port_0_r },
  { 0x3000, 0x3fff, MRA_RAM },
  { 0x4000, 0x4000, OKIM6295_status_0_r },
  { 0x5000, 0x500f, via_2_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(alvgs1_writemem)
  { 0x2000, 0x2000, YM3812_control_port_0_w },
  { 0x2001, 0x2001, YM3812_write_port_0_w },
  { 0x3000, 0x3fff, MWA_RAM },
  { 0x4000, 0x4000, OKIM6295_data_0_w },
  { 0x5000, 0x500f, via_2_w },
  { 0x6000, 0x6000, MWA_NOP },	//Watch dog?
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(alvg_s1)
  MDRV_CPU_ADD(M6809, ALVGS1_SNDCPU_FREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(alvgs1_readmem, alvgs1_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD( YM3812, alvgs1_ym3812_intf )
  MDRV_SOUND_ADD( OKIM6295, alvgs1_okim6295_intf )
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static struct via6522_interface via_2_interface =
{
	/*inputs : A/B           */ xvia_2_a_r, xvia_2_b_r,
	/*inputs : CA1/B1,CA2/B2 */ xvia_2_ca1_r, xvia_2_cb1_r, xvia_2_ca2_r, xvia_2_cb2_r,
	/*outputs: A/B,CA2/B2    */ xvia_2_a_w, xvia_2_b_w, xvia_2_ca2_w, xvia_2_cb2_w,
	/*irq                    */ via_irq
};

static void alvgs1_init(struct sndbrdData *brdData) {
  memset(&alvgslocals, 0, sizeof(alvgslocals));
  alvgslocals.brdData = *brdData;

  /* init VIA */
  via_config(2, &via_2_interface);
  via_reset();

  //Start CA2 as Hi Level
  via_2_ca2_w(0,1);

  //watchdog_reset_w(1,0);
}

/******************************************************************************************************************
														GENERATION #2
******************************************************************************************************************/

#define ALVGS2_SNDCPU_FREQ  2000000						//Schem shows an 8Mhz clock, but often we need to divide by 4 to make it work in MAME.
#define ALVGS_SNDFIRQ_FREQ (ALVGS2_SNDCPU_FREQ / 4096)	//Mystery Castle manual shows Jumper J103 set - which divides E signal from 6809 by 4096.

/*Declarations*/
extern WRITE_HANDLER(alvg_sndCmd_w);
static void alvgs_init(struct sndbrdData *brdData);
static INTERRUPT_GEN(alvgs_firq);
static WRITE_HANDLER(alvgs_data_w);
static WRITE_HANDLER(alvgs_ctrl_w);
static READ_HANDLER(alvgs_ctrl_r);

/*Interfaces*/

/* 12 Voice Style BSMT Chip */
/* Schematics (pistol poker) suggest an 8 bit shift (<<8), but sound is way too loud and clips, so we use 3 */
static struct BSMT2000interface alvgs_bsmt2000Int = {
  1, {24000000}, {12}, {ALVGS_ROMREGION}, {100}, 0, 3, 1
};

/* Sound board */
const struct sndbrdIntf alvgs2Intf = {
   "BSMT", alvgs_init, NULL, NULL, alvg_sndCmd_w, alvgs_data_w, NULL, alvgs_ctrl_w, alvgs_ctrl_r, SNDBRD_NODATASYNC
};

/*Functions*/

//WATCHDOG AND LED CHIP WRITE
//D6 = Watchdog
//D5 = Sound LED
static WRITE_HANDLER(watch_w)
{
	alvg_UpdateSoundLEDS(0,(data&0x40)>>6);
	//if(data & 0x80) watchdog_reset_w(1,0);
	if(data > 0 && !(data&0x60 || data&0x40))
		logerror("UNKNOWN DATA IN SOUND WATCH_W: data=%x\n",data);
}

//SOUND DATA & CONTROL FUNCTIONS
static WRITE_HANDLER(alvgs_data_w){ soundlatch_w(0,data); }
static WRITE_HANDLER(alvgs_ctrl_w){	cpu_set_irq_line(ALVGS_CPUNO, M6809_IRQ_LINE, PULSE_LINE); }
static READ_HANDLER(alvgs_ctrl_r){	return alvgslocals.data_to_main_cpu; }

//BSMT DATA
static WRITE_HANDLER(bsmt_write)
{
	static int data_hi = 0;
	static int addr = 0;

	int lo = (offset & 0x01);
	//printf("offset=%x, data=%x\n",offset,data);

	if(lo)
		BSMT2000_data_0_w(addr, ((data_hi<<8)|data), 0);
	else {
		addr = (offset>>1) & 0x7f;
		data_hi = data;
	}
}
//RDSTATE line looks to D6&D7 for a busy from BSMT
static READ_HANDLER(bsmtready_r) { return 0xc0; }

static MEMORY_READ_START(alvgs_readmem)
  { 0x0100, 0x0100, bsmtready_r },
  { 0x0800, 0x0800, soundlatch_r},
  { 0x2000, 0x3fff, MRA_RAM },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(alvgs_writemem)
  { 0x0100, 0x0100, watch_w },
  { 0x0800, 0x0800, data_to_main_cpu },
  { 0x1000, 0x10ff, bsmt_write },
  { 0x2000, 0x3fff, MWA_RAM },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(alvg_s2)
  MDRV_CPU_ADD(M6809, ALVGS2_SNDCPU_FREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(alvgs_readmem, alvgs_writemem)
  MDRV_CPU_PERIODIC_INT(alvgs_firq, ALVGS_SNDFIRQ_FREQ)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD_TAG("bsmt", BSMT2000, alvgs_bsmt2000Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END


static void alvgs_init(struct sndbrdData *brdData) {
  memset(&alvgslocals, 0, sizeof(alvgslocals));
  alvgslocals.brdData = *brdData;
  //watchdog_reset_w(1,0);
}

static INTERRUPT_GEN(alvgs_firq) {
  cpu_set_irq_line(ALVGS_CPUNO, M6809_FIRQ_LINE, PULSE_LINE);
}
