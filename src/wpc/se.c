/************************************************************************************************
  Sega/Stern Pinball

  Hardware from 1994-????

  CPU Boards: Whitestar System

  Display Boards:
    ??

  Sound Board Revisions:
    Integrated with CPU. Similar cap. as 520-5126-xx Series: (Baywatch to Batman Forever)

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "dedmd.h"
#include "se.h"
#include "sesound.h"
#include "vidhrdw/crtc6845.h"

#define SE_VBLANKFREQ      60 /* VBLANK frequency */
#define SE_IRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define SE_DMDFIRQFREQ	  120 /* FIRQ Handler (Guessed!) */

#define SE_CPUNO	0
#define SE_DCPU1	1

static void SE_init(void);
static void SE_exit(void);
static void SE_nvram(void *file, int write);
static WRITE_HANDLER(mcpu_ram8000_w);
/* makes it easier to swap bits */
                          // 0  1  2  3  4  5  6  7  8  9 10,11,12,13,14,15
static const UINT8 swapNyb[16] = { 0, 8, 4,12, 2,10, 6,14, 1, 9, 5,13, 3,11, 7,15};
/*----------------
/  Local varibles
/-----------------*/
struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int	 flipsol, flipsolPulse;
  int    dmdStatus;
  UINT8 *ram8000;
} SElocals;

static int SE_irq(void) {
  cpu_set_irq_line(SE_CPUNO, M6809_FIRQ_LINE,PULSE_LINE);
  return ignore_interrupt();	//NO INT OR NMI GENERATED!
}

static void dmdfirq(int data) {
  cpu_set_irq_line(SE_DCPU1, M6809_FIRQ_LINE, HOLD_LINE);
}

static int SE_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  SElocals.vblankCount = (SElocals.vblankCount+1) % 16;

  /*-- lamps --*/
  if ((SElocals.vblankCount % SE_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, SElocals.lampMatrix, sizeof(SElocals.lampMatrix));
    memset(SElocals.lampMatrix, 0, sizeof(SElocals.lampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids2 = SElocals.flipsol; SElocals.flipsol = SElocals.flipsolPulse;
  if ((SElocals.vblankCount % SE_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = SElocals.solenoids;
    SElocals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((SElocals.vblankCount % SE_DISPLAYSMOOTH) == 0) {
    coreGlobals.diagnosticLed = SElocals.diagnosticLed;
    SElocals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* flippers are CPU controlled */
  return 0;
}

static void SE_updSw(int *inports) {
  if (inports) {
    /*Switch Col 0 = Dedicated Switches - Coin Door Only - Begin at 6th Spot*/
    coreGlobals.swMatrix[0] = (inports[SE_COMINPORT] & 0x000f)<<4;
    /*Switch Col 1 = Coin Switches - (Switches begin at 4th Spot)*/
    coreGlobals.swMatrix[1] = ((inports[SE_COMINPORT] & 0x0f00)>>5); //>>5 = >>8 + <<3
    /*Copy Start, Tilt, and Slam Tilt to proper position in Matrix: Switchs 66,67,68*/
    /*Clear bits 6,7,8 first*/
    coreGlobals.swMatrix[7] &= ~0xe0;
    coreGlobals.swMatrix[7] |= (inports[SE_COMINPORT] & 0x00f0)<<1;	//<<1 = >>4 + <<5
  }
}

static core_tData SEData = {
  8, /* 8 DIPs */
  SE_updSw,
  1,
  ses_soundCmd_w,
  "SE"
};

static void SE_init(void) {
  if (SElocals.initDone) CORE_DOEXIT(SE_exit)
  SElocals.initDone = TRUE;

  /* Copy Last 32K into last 32K of CPU space */
  memcpy(memory_region(SE_MEMREG_CPU) + 0x8000,
         memory_region(SE_MEMREG_ROM) +
	 (memory_region_length(SE_MEMREG_ROM) - 0x8000), 0x8000);

  /*DMD*/
  /* copy last 16K of ROM into last 16K of CPU region*/
  memcpy(memory_region(SE_MEMREG_DCPU1)+0x8000,
         memory_region(SE_MEMREG_DROM1) +
         (memory_region_length(SE_MEMREG_DROM1) - 0x8000), 0x8000);
  /*Set DMD to point to our DMD memory*/
  coreGlobals_dmd.DMDFrames[0] = memory_region(SE_MEMREG_DCPU1);

  timer_pulse(TIME_IN_HZ(SE_DMDFIRQFREQ), 0, dmdfirq);
  // Sharkeys got some extra ram
  if (core_gameData->gen & GEN_WS_1)
    SElocals.ram8000 = install_mem_write_handler(SE_CPUNO,0x8000,0x81ff,mcpu_ram8000_w);
  if (core_init(&SEData)) return;
}

static void SE_exit(void) {
  core_exit();
}

/*-- Main CPU Bank Switch --*/
// D0-D5 Bank Address
// D6    Unused
// D7    Diagnostic LED
static WRITE_HANDLER(mcpu_bank_w) {
  // Should be 0x3f but memreg is only 512K */
  cpu_setbank(1, memory_region(SE_MEMREG_ROM) + (data & 0x1f)* 0x4000);
  SElocals.diagnosticLed = data>>7;
}

/* Sharkey's ShootOut got some ram at 0x8000-0x81ff */
static WRITE_HANDLER(mcpu_ram8000_w) { SElocals.ram8000[offset] = data; }

/*-- Lamps --*/
static WRITE_HANDLER(lampdriv_w) {
  SElocals.lampRow = (swapNyb[data&0x0f]<<4)|swapNyb[data>>4];
  core_setLamp(SElocals.lampMatrix, SElocals.lampColumn, SElocals.lampRow);
}
static WRITE_HANDLER(lampstrb_w) { core_setLamp(SElocals.lampMatrix, SElocals.lampColumn = (SElocals.lampColumn & 0xff00) | data, SElocals.lampRow);}
static WRITE_HANDLER(auxlamp_w) { core_setLamp(SElocals.lampMatrix, SElocals.lampColumn = (SElocals.lampColumn & 0x00ff) | (data<<8), SElocals.lampRow);}

/*-- Switches --*/
static READ_HANDLER(switch_r)	{ return ~core_getSwCol(SElocals.swCol); }
static WRITE_HANDLER(switch_w)	{ SElocals.swCol = data; }

/*-- Dedicated Switches --*/
// Note: active low
// D0 - DED #1 - Left Flipper
// D1 - DED #2 - Left Flipper EOS
// D2 - DED #3 - Right Flipper
// D3 - DED #4 - Right Flipper EOS
// D4 - DED #5 - Not Used (Upper Flipper on some games!)
// D5 - DED #6 - Volume (Red Button)
// D6 - DED #7 - Service Credit (Green Button)
// D7 - DED #8 - Begin Test (Black Button)
static READ_HANDLER(dedswitch_r) {
  /* CORE Defines flippers in order as: RFlipEOS, RFlip, LFlipEOS, LFlip*/
  /* We need to adjust to: LFlip, LFlipEOS, RFlip, RFlipEOS*/
  /* Swap the 4 lowest bits*/
  return ~((coreGlobals.swMatrix[0]) | swapNyb[coreGlobals.swMatrix[11] & 0x0f]);
}

/*-- Dip Switch SW300 - Country Settings --*/
static READ_HANDLER(dip_r) { return ~core_getDip(0); }

/*-- Solenoids --*/
static WRITE_HANDLER(solenoid_w) {
  static const int solmaskno[] = { 8, 0, 16, 24 };
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];

  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    SElocals.flipsol |= SElocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  SElocals.solenoids |= sols;
}
/*-- DMD communication --*/
static WRITE_HANDLER(dmdlatch_w) {
  soundlatch_w(0, data);
  cpu_set_irq_line(SE_DCPU1, M6809_IRQ_LINE, ASSERT_LINE);
  SElocals.dmdStatus |= 0x80; // BUSY
}
static WRITE_HANDLER(dmdreset_w) {
  cpu_set_reset_line(SE_DCPU1, PULSE_LINE);
  SElocals.dmdStatus = 0x00;
  cpu_set_irq_line(SE_DCPU1, M6809_IRQ_LINE, CLEAR_LINE);
}
static READ_HANDLER(dmdstatus_r) { return SElocals.dmdStatus; }

static READ_HANDLER(dmdie_r) { /*What is this for?*/
  DBGLOG(("DMD Input Enable Read PC=%x\n",cpu_getpreviouspc())); return 0x00;
}
static READ_HANDLER(dmdlatch_r) {
  SElocals.dmdStatus &= 0x7f; // Clear BUSY
  cpu_set_irq_line(SE_DCPU1, M6809_IRQ_LINE, CLEAR_LINE);
  return soundlatch_r(0);
}
static WRITE_HANDLER(dmdstatus_w) {
  SElocals.dmdStatus = (SElocals.dmdStatus & 0x80) | ((data & 0x0f) << 3);
}

static WRITE_HANDLER(auxboard_w) { /* logerror("Aux Board Write: Offset: %x Data: %x\n",offset,data); */}
static WRITE_HANDLER(giaux_w)    { /* logerror("GI/Aux Board Write: Offset: %x Data: %x\n",offset,data); */}

/*-- DMD CPU Bank Switch */
static WRITE_HANDLER(dmdcpu_bank_w) {
  cpu_setbank(2, memory_region(SE_MEMREG_DROM1) + (data & 0x1f)*0x4000);
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(SE_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x3000, 0x3000, dedswitch_r },
  { 0x3100, 0x3100, dip_r },
  { 0x3400, 0x3400, switch_r },
  { 0x3500, 0x3500, dmdie_r },
  { 0x3700, 0x3700, dmdstatus_r },
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(SE_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2003, solenoid_w },
  { 0x2006, 0x2007, auxboard_w },
  { 0x2008, 0x2008, lampstrb_w },
  { 0x2009, 0x2009, auxlamp_w },
  { 0x200a, 0x200a, lampdriv_w },
  { 0x200b, 0x200b, giaux_w },
  { 0x3200, 0x3200, mcpu_bank_w },
  { 0x3300, 0x3300, switch_w },
  { 0x3600, 0x3600, dmdlatch_w },
  { 0x3601, 0x3601, dmdreset_w },
  { 0x3800, 0x3800, ses_soundCmd_w },
  { 0x4000, 0x7fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(se_dmdreadmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2000, 0x2fff, MRA_RAM }, /* DMD RAM PAGE 0-7 512 bytes each */
  { 0x3000, 0x3000, crtc6845_register_r },
  { 0x3003, 0x3003, dmdlatch_r },
  { 0x4000, 0x7fff, MRA_BANK2 }, /* Banked ROM */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(se_dmdwritemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2fff, MWA_RAM }, /* DMD RAM PAGE 0-7 512 bytes each*/
  { 0x3000, 0x3000, crtc6845_address_w },
  { 0x3001, 0x3001, crtc6845_register_w },
  { 0x3002, 0x3002, dmdcpu_bank_w }, /* DMD Bank Switching*/
  { 0x4000, 0x4000, dmdstatus_w },   /* DMD Status*/
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

struct MachineDriver machine_driver_SE_1S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  },{ CPU_M6809, 4000000, /* 4 Mhz*/
      se_dmdreadmem, se_dmdwritemem, NULL, NULL,
      NULL, 0,
      ignore_interrupt, 0
  } SES_SOUNDCPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, CORE_EXITFUNC(SE_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ SES_SOUND1 },
  SE_nvram
};

struct MachineDriver machine_driver_SE_2S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  },{ CPU_M6809, 4000000, /* 4 Mhz*/
      se_dmdreadmem, se_dmdwritemem, NULL, NULL,
      NULL, 0,
      ignore_interrupt, 0
  } SES_SOUNDCPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, CORE_EXITFUNC(SE_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ SES_SOUND2 },
  SE_nvram
};

struct MachineDriver machine_driver_SE_3S = {
  {{  CPU_M6809, 2000000, /* 2 Mhz */
      SE_readmem, SE_writemem, NULL, NULL,
      SE_vblank, 1,
      SE_irq, SE_IRQFREQ
  },{ CPU_M6809, 4000000, /* 4 Mhz*/
      se_dmdreadmem, se_dmdwritemem, NULL, NULL,
      NULL, 0,
      ignore_interrupt, 0
  } SES_SOUNDCPU },
  SE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, SE_init, CORE_EXITFUNC(SE_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, de_dmd128x32_refresh,
  SOUND_SUPPORTS_STEREO,0,0,0,{ SES_SOUND3 },
  SE_nvram
};


/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void SE_nvram(void *file, int write) {
  core_nvram(file, write, memory_region(SE_MEMREG_CPU), 0x2000);
}
