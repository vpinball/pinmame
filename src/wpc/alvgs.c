/*Alvin G & Co. Sound Hardware
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/m6809/m6809.h"
#include "alvg.h"
#include "alvgs.h"
#include "machine/6821pia.h"
#include "sound/3812intf.h"
#include "sndbrd.h"

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

#define ALVGS1_SNDCPU_FREQ  2000000						//Schem shows an 8Mhz clock, but often we need to divide by 4 to make it work in MAME.

/*Declarations*/
extern WRITE_HANDLER(alvg_sndCmd_w);
static void alvgs1_init(struct sndbrdData *brdData);
static WRITE_HANDLER(alvgs1_data_w);
static WRITE_HANDLER(alvgs1_ctrl_w);
static READ_HANDLER(alvgs1_ctrl_r);

/* handler called by the 3812 when the internal timers cause an IRQ */
static void ym3812_irq(int irq)
{
	cpu_set_irq_line(ALVGS_CPUNO, M6809_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Interfaces*/

static struct YM3812interface alvgs1_ym3812_intf =
{
	1,					/* 1 chip */
	4000000,			/* 4 MHz */
	{ 100 },			/* volume */
	{ ym3812_irq },
};
static struct OKIM6295interface alvgs1_okim6295_intf =
{
	1,					/* 1 chip */
	{ 8000 },			/* 8000Hz playback */
	{ REGION_SOUND1 },
	{ 100 }
};

/* Sound board */
const struct sndbrdIntf alvgs1Intf = {
   "OKI", alvgs1_init, NULL, NULL, alvg_sndCmd_w, alvgs1_data_w, NULL, alvgs1_ctrl_w, alvgs1_ctrl_r, SNDBRD_NODATASYNC
};

/*Functions*/

//SOUND DATA & CONTROL FUNCTIONS
static WRITE_HANDLER(alvgs1_data_w){ soundlatch_w(0,data); }
static WRITE_HANDLER(alvgs1_ctrl_w){	cpu_set_irq_line(ALVGS_CPUNO, 0, PULSE_LINE); }
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


static MEMORY_READ_START(alvgs1_readmem)
  { 0x3000, 0x3fff, MRA_RAM },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(alvgs1_writemem)
  { 0x3000, 0x3fff, MWA_RAM },
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


static void alvgs1_init(struct sndbrdData *brdData) {
  memset(&alvgslocals, 0, sizeof(alvgslocals));
  alvgslocals.brdData = *brdData;
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

/* Newer 12 Voice Style BSMT Chip */
static struct BSMT2000interface alvgs_bsmt2000Int = {
  1, {24000000}, {12}, {ALVGS_ROMREGION}, {100}, {0000}, 0, 1, 1
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
static WRITE_HANDLER(alvgs_ctrl_w){	cpu_set_irq_line(ALVGS_CPUNO, 0, PULSE_LINE); }
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
