/************************************************************************************************
 Atari Pinball
 -----------------

   Generation 1: Hardware: (From Atarians -> Space Riders)
		CPU: M6800? (Fixed Frequency of 1MHz)
			 INT: NMI @ 250Hz

   Generation 2: Hardware: (Superman -> Hercules)
		CPU: M6800 (Alternating frequency of 1MHz & 0.667MHz)
			 INT: IRQ - Fixed @ 488Hz?
		IO: DMA (Direct Memory Access/Address)
		DISPLAY: 5x6 Digit 7 Segment Display
		SOUND:	 Frequency + Noise Generator, Programmable Timers
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "atari.h"

#define ATARI_VBLANKFREQ      60 /* VBLANK frequency in HZ*/
#define ATARI_IRQFREQ        488 /* IRQ interval in HZ */
#define ATARI_NMIFREQ        250 /* NMI frequency in HZ */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    diagnosticLed;
  int    soldisable;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
} locals;

static int dispPos[] = { 49, 1, 9, 21, 29, 41, 61, 69 };

static INTERRUPT_GEN(ATARI1_nmihi) {
  //cpu_set_nmi_line(ATARI_CPU, state ? ASSERT_LINE : CLEAR_LINE);
	cpu_set_nmi_line(ATARI_CPU, PULSE_LINE);
}

static void ATARI2_irq(int state) {
  cpu_set_irq_line(ATARI_CPU, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(ATARI2_irqhi) {
  ATARI2_irq(1);
}

static WRITE_HANDLER(intack_w) {
  ATARI2_irq(0);
}

/* only needed for real hardware */
static WRITE_HANDLER(watchdog_w) {
//logerror("Watchdog reset!\n");
}

static INTERRUPT_GEN(ATARI_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/

  locals.vblankCount += 1;
  /*-- lamps --*/
  if ((locals.vblankCount % ATARI_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
//	memset(locals.lampMatrix, 0, sizeof(locals.lampMatrix));
  }
  /*-- solenoids --*/

  if ((locals.vblankCount % ATARI_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    if (locals.soldisable) locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % ATARI_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
//  memcpy(locals.segments, locals.pseg, sizeof(locals.segments));

  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(ATARI1) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xf0) | (inports[ATARI_COMINPORT] & 0x0f);
		coreGlobals.swMatrix[3] = (coreGlobals.swMatrix[3] & 0xfc) | ((inports[ATARI_COMINPORT] & 0x0300) >> 8);
	}
}

static SWITCH_UPDATE(ATARI2) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x62) | (inports[ATARI_COMINPORT] & 0x9d);
	}
}

static int ATARI_sw2m(int no) {
	return no + 8;
}

static int ATARI_m2sw(int col, int row) {
	return col*8 + row - 8;
}

/* Switch reading */
// Gen 1
static READ_HANDLER(swg1_r) {
	return (coreGlobals.swMatrix[1+(offset/8)] & (1 << (offset % 8)))?0:0xff;
}

static WRITE_HANDLER(swg1_w) {
	logerror("Test switch write %2x\n", data);
}

static READ_HANDLER(dipg1_r) {
	return (core_getDip(offset/8) & (1 << (offset % 8)))?0:0xff;
}
// Gen 2
static READ_HANDLER(sw_r) {
	return ~coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r)  {
	return ~core_getDip(offset);
}

/* display */
// Gen 2
static WRITE_HANDLER(disp0_w) {
	((int *)locals.pseg)[offset] = core_bcd2seg[data & 0x0f];
}

static WRITE_HANDLER(disp1_w) {
	int ii;
	data &= 0x07;
	for (ii = 0; ii < 7; ii++) {
		((int *)locals.segments)[dispPos[ii]+data] = ((int *)locals.pseg)[ii];
	}
}

/* solenoids */
// Gen 2
static WRITE_HANDLER(sol0_w) {
	data &= 0x0f;
	if (data) {
		locals.solenoids |= 1 << (data-1);
		locals.soldisable = 0;
	} else locals.soldisable = 1;
}

static WRITE_HANDLER(sol1_w) {
	if (offset != 7) {
		if (data & 0x01) locals.solenoids |= (1 << (15 + offset));
		else locals.solenoids &= ~(1 << (15 + offset));
	}
}

/* lamps */
// Gen 1 (all outputs)
static WRITE_HANDLER(latch10_w) {
	locals.lampMatrix[0] = data;
}

static WRITE_HANDLER(latch14_w) {
	locals.lampMatrix[1] = data;
}

static WRITE_HANDLER(latch18_w) {
	locals.lampMatrix[2] = data;
}

static WRITE_HANDLER(latch1c_w) {
	locals.lampMatrix[3] = data;
}

static WRITE_HANDLER(latch50_w) {
	locals.lampMatrix[4] = data;
}

static WRITE_HANDLER(latch54_w) {
	locals.lampMatrix[5] = data;
}

static WRITE_HANDLER(latch58_w) {
	locals.lampMatrix[6] = data;
}

static WRITE_HANDLER(latch5c_w) {
	locals.lampMatrix[7] = data;
}

// Gen 2
static WRITE_HANDLER(lamp_w) {
	locals.lampMatrix[offset] = data;
}

/* sound */
// Gen 1
static WRITE_HANDLER(soundg1_w) {
	logerror("Play sound %2x\n", data);
}

static WRITE_HANDLER(audiog1_w) {
	logerror("Audio Reset %2x\n", data);
}
// Gen 2
static WRITE_HANDLER(sound0_w) {
	locals.diagnosticLed = data & 0x0f; /* coupled with waveform select */
	sndbrd_0_ctrl_w(1, data);
}

static WRITE_HANDLER(sound1_w) {
	sndbrd_0_data_w(0, data);
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *ATARI1_CMOS;
static WRITE_HANDLER(ATARI1_CMOS_w) {
  ATARI1_CMOS[offset] = data;
}

static NVRAM_HANDLER(ATARI1) {
	core_nvram(file, read_or_write, ATARI1_CMOS, 512, 0x00);
}

static UINT8 *ATARI2_CMOS;
static WRITE_HANDLER(ATARI2_CMOS_w) {
  ATARI2_CMOS[offset] = data;
}

static NVRAM_HANDLER(ATARI2) {
	core_nvram(file, read_or_write, ATARI2_CMOS, 256, 0x00);
}

/*-----------------------------------------
/  Memory map for CPU board (GENERATION 1)
/------------------------------------------
0000-00ff  1K RAM (When installed instead of 2K?)
0300-04ff  2K RAM (When installed instead of 1K?)
1000-10ff  1K RAM Mirror?
1300-14ff  2K RAM Mirror?
2000-????  Switch Read
3000-????  Audio Enable
4000-????  WatchDog Reset
5000-????  Decode 5000 (Testing only?)
6000-????  Audio Reset
*/
static MEMORY_READ_START(ATARI1_readmem)
{0x0000,0x01ff,	MRA_RAM},	/* NVRAM */
{0x1000,0x10ff,	MRA_RAM},	/* RAM */
{0x1800,0x18ff,	MRA_RAM},	/* RAM */
{0x2000,0x200f,	dipg1_r},	/* dips */
{0x2010,0x204f,	swg1_r},	/* inputs */
{0x7000,0x7fff,	MRA_ROM},	/* ROM */
{0xf800,0xffff,	MRA_ROM},	/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI1_writemem)
{0x0000,0x01ff,	ATARI1_CMOS_w, &ATARI1_CMOS},	/* NVRAM */
{0x1080,0x1080,	latch10_w},	/* output */
{0x1084,0x1084,	latch14_w},	/* output */
{0x1088,0x1088,	latch18_w},	/* output */
{0x108c,0x108c,	latch1c_w},	/* output */
{0x200b,0x200b,	swg1_w},	/* test switch write? */
{0x3000,0x3000,	soundg1_w},	/* audio enable? */
{0x4000,0x4000,	watchdog_w},/* watchdog reset? */
{0x5080,0x5080,	latch50_w},	/* output */
{0x5084,0x5084,	latch54_w},	/* output */
{0x5088,0x5088,	latch58_w},	/* output */
{0x508c,0x508c,	latch5c_w},	/* output */
{0x6000,0x6000,	audiog1_w},	/* audio reset? */
{0x7000,0x7fff,	MWA_ROM},	/* ROM */
{0xf800,0xffff,	MWA_ROM},	/* reset vector */
MEMORY_END

/*-----------------------------------------
/  Memory map for CPU board (GENERATION 2)
/------------------------------------------*/
static MEMORY_READ_START(ATARI2_readmem)
{0x0000,0x00ff,	MRA_RAM},	/* RAM */
{0x0100,0x01ff,	MRA_NOP},	/* unmapped RAM */
{0x0800,0x08ff,	MRA_RAM},	/* NVRAM */
{0x0900,0x09ff,	MRA_NOP},	/* unmapped RAM */
{0x1000,0x1007,	sw_r},		/* inputs */
{0x2000,0x2003,	dip_r},		/* dip switches */
{0x2800,0x3fff,	MRA_ROM},	/* ROM */
{0xa800,0xbfff,	MRA_ROM},	/* ROM */
{0xf800,0xffff,	MRA_ROM},	/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI2_writemem)
{0x0000,0x00ff,	MWA_RAM},	/* RAM */
{0x0100,0x0100,	MWA_NOP},	/* unmapped RAM */
{0x0700,0x07ff,	MWA_NOP},	/* unmapped RAM */
{0x0800,0x08ff,	ATARI2_CMOS_w, &ATARI2_CMOS},	/* NVRAM */
{0x1800,0x1800,	sound0_w},	/* sound */
{0x1820,0x1820,	sound1_w},	/* sound */
{0x1840,0x1846,	disp0_w},	/* display data output */
{0x1847,0x1847,	disp1_w},	/* display digit enable & diagnostic LEDs */
{0x1860,0x1867,	lamp_w},	/* lamp output */
{0x1880,0x1880,	sol0_w},	/* solenoid output */
{0x18a0,0x18a7,	sol1_w},	/* solenoid enable & independent control output */
{0x18c0,0x18c1,	watchdog_w},/* watchdog reset */
{0x18e0,0x18e0,	intack_w},	/* interrupt acknowledge (resets IRQ state) */
{0x2800,0x3fff,	MWA_ROM},	/* ROM */
{0xa800,0xbfff,	MWA_ROM},	/* ROM */
{0xf800,0xffff,	MWA_ROM},	/* reset vector */
MEMORY_END

static MACHINE_INIT(ATARI1) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_STOP(ATARI1) {
}

static MACHINE_INIT(ATARI2) {
  memset(&locals, 0, sizeof locals);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_SOUND1), NULL, NULL);
}

static MACHINE_STOP(ATARI2) {
  sndbrd_0_exit();
}

MACHINE_DRIVER_START(ATARI1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 1000000)
  MDRV_CPU_MEMORY(ATARI1_readmem, ATARI1_writemem)
  MDRV_CPU_VBLANK_INT(ATARI_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ATARI1_nmihi, ATARI_NMIFREQ)
  MDRV_CORE_INIT_RESET_STOP(ATARI1,NULL,ATARI1)
  MDRV_NVRAM_HANDLER(ATARI1)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(ATARI1)
  MDRV_DIAGNOSTIC_LEDH(4)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("ATARI1")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ATARI2)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ATARI2,NULL,ATARI2)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 1000000)
  MDRV_CPU_MEMORY(ATARI2_readmem, ATARI2_writemem)
  MDRV_CPU_VBLANK_INT(ATARI_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ATARI2_irqhi, ATARI_IRQFREQ)
  MDRV_NVRAM_HANDLER(ATARI2)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(ATARI2)
  MDRV_DIAGNOSTIC_LEDH(4)
  MDRV_SWITCH_CONV(ATARI_sw2m,ATARI_m2sw)

  MDRV_IMPORT_FROM(atari2s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("ATARI2")
MACHINE_DRIVER_END

#if 0
struct MachineDriver machine_driver_ATARI1 = {
  {
    {
      CPU_M6800, 1000000, /* 1 Mhz */
      ATARI1_readmem, ATARI1_writemem, NULL, NULL,
	  ATARI_vblank, 1,
	  NULL, 0
	},
	{ 0 }, /* ATARIS_SOUNDCPU */
  },
  ATARI_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  ATARI1_init,CORE_EXITFUNC(ATARI1_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  ATARI1_nvram
};

struct MachineDriver machine_driver_ATARI2 = {
  {
    {
      CPU_M6800, 1000000, /* 1 Mhz */
      ATARI2_readmem, ATARI2_writemem, NULL, NULL,
	  ATARI_vblank, 1,
	  NULL, 0
	},
	{ 0 }, /* ATARIS_SOUNDCPU */
  },
  ATARI_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  ATARI2_init,CORE_EXITFUNC(ATARI2_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{ATARI_SOUND},
  ATARI2_nvram
};
#endif
