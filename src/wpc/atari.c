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
#include "atari.h"

#define ATARI_VBLANKFREQ      60 /* VBLANK frequency in HZ*/
#define ATARI_IRQFREQ       2048 /* IRQ interval in USEC */
#define ATARI_NMIFREQ        250 /* NMI frequency in HZ */

static void ATARI1_init(void);
static void ATARI1_exit(void);
static void ATARI1_nvram(void *file, int write);

static void ATARI2_init(void);
static void ATARI2_exit(void);
static void ATARI2_nvram(void *file, int write);

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    initDone;
  int    diagnosticLed;
  int    soldisable;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  void   *irqtimer, *nmitimer;
//  int	 sound_data;
} locals;

static int dispPos[] = { 49, 1, 9, 21, 29, 41, 61, 69 };

static void ATARI1_nmihi(int state) {
  //cpu_set_nmi_line(ATARI_CPU, state ? ASSERT_LINE : CLEAR_LINE);
	cpu_set_nmi_line(ATARI_CPU, PULSE_LINE);
}

static void ATARI2_irq(int state) {
  cpu_set_irq_line(ATARI_CPU, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void ATARI2_irqhi(int dummy) {
  ATARI2_irq(1);
}

static WRITE_HANDLER(intack_w) {
  ATARI2_irq(0);
}

/* only needed for real hardware */
static WRITE_HANDLER(watchdog_w) {
//logerror("Watchdog reset!\n");
}

static int ATARI_vblank(void) {
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
  return 0;
}

static void ATARI1_updSw(int *inports) {
}

static void ATARI2_updSw(int *inports) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x62) | (inports[ATARI_COMINPORT] & 0x9d);
	}
}

static WRITE_HANDLER(ATARI_sndCmd_w) {
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	//ATARIS_sound_latch(data);
}

static int ATARI_sw2m(int no) {
	return no + 8;
}

static int ATARI_m2sw(int col, int row) {
	return col*8 + row - 8;
}

static core_tData ATARI1Data = {
  16, /* 16 dip switches */
  ATARI1_updSw,
  4, /* 4 diagnostic LEDs */
  ATARI_sndCmd_w, "ATARI1",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static core_tData ATARI2Data = {
  32, /* 32 dip switches */
  ATARI2_updSw,
  4, /* 4 diagnostic LEDs */
  ATARI_sndCmd_w, "ATARI2",
  ATARI_sw2m, core_swSeq2m, ATARI_m2sw, core_m2swSeq
};

static READ_HANDLER(readCRC) {
	return 0xb8;
}

/* Switch reading */
static READ_HANDLER(swg1_r) {
	return (coreGlobals.swMatrix[1+(offset/8)] & (1 << (offset % 8)))?0:0xff;
}

static READ_HANDLER(dipg1_r) {
	return (core_getDip(offset/8) & (1 << (offset % 8)))?0:0xff;
}

static WRITE_HANDLER(swg1_w) {
	logerror("Test switch write %2x\n", data);
}

static READ_HANDLER(sw_r) {
	return ~coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r)  {
	return ~core_getDip(offset);
}

/* display */
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

static WRITE_HANDLER(lamp_w) {
	locals.lampMatrix[offset] = data;
}

/* sound */
static WRITE_HANDLER(soundg1_w) {
	logerror("Play sound %2x\n", data);
}

static WRITE_HANDLER(audiog1_w) {
	logerror("Audio Reset %2x\n", data);
}

static WRITE_HANDLER(sound0_w) {
	locals.diagnosticLed = data & 0x0f; /* coupled with waveform select */
	if (data & 0x80) logerror("noise on\n");
	if (data & 0x40) logerror("wave on\n");
	if (data & 0x30) logerror("octave=%d\n", data >> 4);
	if (data & 0x0f) logerror("waveform=%d\n", data & 0x0f);
//ATARI_sndCmd_w(0,!(data&0x10)?(~data)&0x0f:0x00);
}

static WRITE_HANDLER(sound1_w) {
	if (data & 0xf0) logerror("freq.div=%d\n", data >> 4);
	if (data & 0x0f) logerror("amplitude=%d\n", data & 0x0f);
//ATARI_sndCmd_w(1,!(data&0x10)?(~data)&0x0f:0x00);
}

/* RAM */
static UINT8 NVRAM_512[0x200];
static UINT8 NVRAM_256[0x100];

static READ_HANDLER(nvram_r) {
	return NVRAM_256[offset] & 0x0f;
}

static WRITE_HANDLER(nvram_w) {
	NVRAM_256[offset] = data & 0x0f;
}

static READ_HANDLER(nvram1_r) {
	return NVRAM_512[offset];
}

static WRITE_HANDLER(nvram1_w) {
	NVRAM_512[offset] = data;
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
{0x0000,0x01ff,	nvram1_r},	/* NVRAM */
{0x1000,0x10ff,	MRA_RAM},	/* RAM */
{0x1800,0x18ff,	MRA_RAM},	/* RAM */
{0x2000,0x200f,	dipg1_r},	/* dips */
{0x2010,0x204f,	swg1_r},	/* inputs */
{0x7000,0x7fff,	MRA_ROM},	/* ROM */
{0xf800,0xffff,	MRA_ROM},	/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI1_writemem)
{0x0000,0x01ff,	nvram1_w},	/* NVRAM */
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
{0x0100,0x01ff,	nvram_r},	/* unmapped RAM */
{0x0800,0x08ff,	nvram_r},	/* NVRAM */
{0x0900,0x09ff,	nvram_r},	/* unmapped RAM */
{0x1000,0x1007,	sw_r},		/* inputs */
{0x2000,0x2003,	dip_r},		/* dip switches */
{0x2800,0x3fff,	MRA_ROM},	/* ROM */
{0xa800,0xbfff,	MRA_ROM},	/* ROM */
{0xf800,0xffff,	MRA_ROM},	/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI2_writemem)
{0x0000,0x00ff,	MWA_RAM},	/* RAM */
{0x0100,0x0100,	MWA_RAM},	/* unmapped RAM */
{0x0700,0x07ff,	MWA_RAM},	/* unmapped RAM */
{0x0800,0x08ff,	nvram_w},	/* NVRAM */
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
  0,0,0,0,{{0}},
  ATARI2_nvram
};

static void ATARI1_init(void) {
  if (locals.initDone) CORE_DOEXIT(ATARI_exit);

  if (core_init(&ATARI1Data)) return;
  memset(&locals, 0, sizeof locals);

  locals.nmitimer = timer_pulse(TIME_IN_HZ(ATARI_NMIFREQ),0,ATARI1_nmihi);

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	//ATARIS_init();
  }

  locals.initDone = TRUE;
}

static void ATARI1_exit(void) {
  if (locals.nmitimer) { timer_remove(locals.nmitimer); locals.nmitimer = NULL; }

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	//ATARIS_exit();
  }

  core_exit();
}

static void ATARI2_init(void) {
  if (locals.initDone) CORE_DOEXIT(ATARI_exit);

  if (core_init(&ATARI2Data)) return;
  memset(&locals, 0, sizeof locals);

  locals.irqtimer = timer_pulse(TIME_IN_USEC(ATARI_IRQFREQ),0,ATARI2_irqhi);

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	//ATARIS_init();
  }

  locals.initDone = TRUE;
}

static void ATARI2_exit(void) {
  if (locals.irqtimer) { timer_remove(locals.irqtimer); locals.irqtimer = NULL; }

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	//ATARIS_exit();
  }

  core_exit();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void ATARI1_nvram(void *file, int write) {
	core_nvram(file, write, NVRAM_512, sizeof NVRAM_512, 0x00);
}

void ATARI2_nvram(void *file, int write) {
	core_nvram(file, write, NVRAM_256, sizeof NVRAM_256, 0x00);
}
