/************************************************************************************************
 Atari Pinball generation 1 (Atarians to Space Riders)
 Atari Pinball generation 2 (Superman and later games)

 CPU: 6800
 I/O: DMA
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "atari.h"

#define ATARI_VBLANKFREQ      60 /* VBLANK frequency */
#define ATARI_IRQFREQ        500 /* IRQ frequency */
#define ATARI_DMAFREQ       2000 /* DMA frequency */
#define ATARI_NMIFREQ        250 /* NMI frequency */

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
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  void   *irqtimer, *dmatimer, *nmitimer;
//  int	 sound_data;
} locals;

static void ATARI1_dmahi(int state) {
	printf("DMA ");
}

static void ATARI1_nmihi(int state) {
	printf("NMI ");
}

static void ATARI2_irq(int state) {
  cpu_set_irq_line(ATARI_CPU, M6800_INT_IRQ, state ? ASSERT_LINE : CLEAR_LINE);
}

static void ATARI2_irqhi(int dummy) {
	printf("h");
  ATARI2_irq(1);
}

static WRITE_HANDLER(intack_w) {
	printf("l");
  ATARI2_irq(0);
}

static WRITE_HANDLER(watchdog_w) {
	printf("WATCHDOG!\n");
}

static int ATARI_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/

  locals.vblankCount += 1;
  /*-- lamps --*/
  if ((locals.vblankCount % ATARI_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
//    memset(locals.lampMatrix, 0, sizeof(locals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((locals.vblankCount % ATARI_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % ATARI_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
//    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));

  }
  /*update leds*/
  coreGlobals.diagnosticLed = 0;
  core_updateSw(TRUE); /* assume flipper enabled */
  return 0;
}

static void ATARI_updSw(int *inports) {
	if (inports) {
		coreGlobals.swMatrix[0] = inports[ATARI_COMINPORT]<<7;
	}
}

static WRITE_HANDLER(ATARI_sndCmd_w) {
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	//ATARIS_sound_latch(data);
}

static core_tData ATARI1Data = {
  32, /* 32 dip switches */
  ATARI_updSw,
  1,
  ATARI_sndCmd_w, "ATARI1",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static core_tData ATARI2Data = {
  32, /* 32 dip switches */
  ATARI_updSw,
  1,
  ATARI_sndCmd_w, "ATARI2",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

/* Switch reading */
static READ_HANDLER(sw_r) {
	return core_getSwCol(offset);
}

static READ_HANDLER(dip_r)  {
	return core_getDip(offset);
}

/* display */
static WRITE_HANDLER(disp0_w)
{
	((int *)locals.segments)[offset] |= core_bcd2seg[data & 0x0f];
	return;
}

static WRITE_HANDLER(disp1_w)
{
	printf("Digit select: %d\n", data & 0x07);
}

/* solenoids */
static WRITE_HANDLER(sol0_w) {
	locals.solenoids |= (data & 0x0f);
}

static WRITE_HANDLER(sol1_w) {
	printf("Control Output #%d=%d\n", offset, data & 1);
}

/* lamps */
static WRITE_HANDLER(lamp_w) {
	locals.lampMatrix[offset] = data;
}

/* sound */
//ATARI_sndCmd_w(0,!(data&0x10)?(~data)&0x0f:0x00);

/* RAM */
static UINT8 RAM_256[0x100];
static UINT8 NVRAM_256[0x100];

static READ_HANDLER(ram_r) {
	return RAM_256[offset%0x100];
}

static WRITE_HANDLER(ram_w) {
	RAM_256[offset%0x100] = data;
}

static READ_HANDLER(nvram_r) {
	return NVRAM_256[offset%0x100] & 0x0f;
}

static WRITE_HANDLER(nvram_w) {
	NVRAM_256[offset%0x100] = data & 0x0f;
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(ATARI1_readmem)
{0x0000,0x07ff,	MRA_ROM},	/* ROM #1 */
{0x0800,0x0fff,	MRA_ROM},	/* ROM #2 */
MEMORY_END

static MEMORY_WRITE_START(ATARI1_writemem)
{0x0000,0x07ff,	MWA_ROM},	/* ROM #1 */
{0x0800,0x0fff,	MWA_ROM},	/* ROM #2 */
MEMORY_END

static MEMORY_READ_START(ATARI2_readmem)
{0x0000,0x00ff,	ram_r},	/* RAM */
{0x0800,0x08ff,	nvram_r},	/* NVRAM */
{0x1000,0x1007,	sw_r},	/* inputs */
{0x2000,0x2003,	dip_r},	/* dip switches */
{0x3000,0x37ff,	MRA_ROM},	/* ROM #2 */
{0x3800,0x3fff,	MRA_ROM},	/* ROM #3 */
{0xa800,0xafff,	MRA_ROM},	/* ROM #1 */
MEMORY_END

static MEMORY_WRITE_START(ATARI2_writemem)
{0x0000,0x00ff,	ram_w},	/* RAM */
{0x0800,0x08ff,	nvram_w},	/* NVRAM */
//{0x1800,0x1800,	sound1_w},	/* sound */
//{0x1820,0x1820,	sound2_w},	/* sound */
{0x1840,0x1846,	disp0_w},	/* display data output */
{0x1847,0x1847,	disp1_w},	/* display digit enable */
{0x1860,0x1867,	lamp_w},	/* lamp output */
{0x1880,0x1880,	sol0_w},	/* solenoid output */
{0x18a0,0x18a7,	sol1_w},	/* solenoid enable & independent control output */
{0x18c0,0x18c0,	watchdog_w},	/* watchdog reset */
{0x18e0,0x18e0,	intack_w},	/* interrupt acknowledge (resets IRQ state) */
{0x3000,0x37ff,	MWA_ROM},	/* ROM #2 */
{0x3800,0x3fff,	MWA_ROM},	/* ROM #3 */
{0xa800,0xafff,	MWA_ROM},	/* ROM #1 */
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

  //cpu_irq_line_vector_w(ATARI_CPU, M6800_INT_IRQ, 0x18e0);
  locals.dmatimer = timer_pulse(TIME_IN_HZ(ATARI_DMAFREQ),0,ATARI1_dmahi);
  locals.nmitimer = timer_pulse(TIME_IN_HZ(ATARI_NMIFREQ),0,ATARI1_nmihi);

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	//ATARIS_init();
  }

  locals.initDone = TRUE;
}

static void ATARI1_exit(void) {
  if (locals.dmatimer) { timer_remove(locals.dmatimer); locals.dmatimer = NULL; }
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

  //cpu_irq_line_vector_w(ATARI_CPU, M6800_INT_IRQ, 0x18e0);
  locals.irqtimer = timer_pulse(TIME_IN_HZ(ATARI_IRQFREQ),0,ATARI2_irqhi);

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
	core_nvram(file, write, NVRAM_256, sizeof NVRAM_256, 0x00);
}

void ATARI2_nvram(void *file, int write) {
	core_nvram(file, write, NVRAM_256, sizeof NVRAM_256, 0x00);
}
