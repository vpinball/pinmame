#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "by35.h"

#define BY35_PIA0 0
#define BY35_PIA1 1

#define BY35_VBLANKFREQ    60 /* VBLANK frequency */
#define BY35_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define BY35_ZCFREQ       100 /* Zero cross frequency */

static struct {
  int a0, a1, b1, ca20, ca21, cb20, cb21;
  int bcd[7];
  int displays; // Number of display boards
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
  void *zctimer;
} locals;

static void by35_exit(void);
static void by35_nvram(void *file, int write);

static void piaIrq(int state) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void by35_dispStrobe(int mask) {
  int digit = locals.a1 & 0xfe;
  int ii,jj;
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
    }
}

static void by35_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = (locals.a0>>4)^0x0f;
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+8*board];
    int bit = 1<<(lampadr & 0x07);

    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}

/* PIA0:A-W  Control what is read from PIA0:B */
static WRITE_HANDLER(pia0a_w) {
  if (!locals.ca20) {
    int bcdLoad = locals.a0 & ~data & 0x0f;
    int ii;

    for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
      if (bcdLoad & 0x01) locals.bcd[ii] = data>>4;
  }
  locals.a0 = data;
  by35_lampStrobe(0,locals.lampadr1);
  if (core_gameData->hw.lampCol > 0) by35_lampStrobe(1,locals.lampadr2);
}
/* PIA1:A-W  0,2-7 Display handling */
/*        W  1     Sound E */
static WRITE_HANDLER(pia1a_w) {
  int tmp = locals.a1;
  locals.a1 = data;

//  sndbrd_0_data_w(0, (locals.cb21<<5) | ((locals.a1 & 0x02)<<3) | (locals.b1 & 0x0f));
  sndbrd_0_data_w(0, ((locals.a1 & 0x02)<<3) | (locals.b1 & 0x0f));

  if (!locals.ca20) {
    if (tmp & ~data & 0x01) { // Positive edge
      locals.bcd[4] = locals.a0>>4;
      by35_dispStrobe(0x10);
    }
  }
}

/* PIA0:B-R  Get Data depending on PIA0:A */
static READ_HANDLER(pia0b_r) {
  if (locals.a0 & 0x20) return core_getDip(0); // DIP#1 1-8
  if (locals.a0 & 0x40) return core_getDip(1); // DIP#2 9-16
  if (locals.a0 & 0x80) return core_getDip(2); // DIP#3 17-24
  if (locals.cb20)      return core_getDip(3); // DIP#4 25-32
  return core_getSwCol((locals.a0 & 0x1f) | ((locals.b1 & 0x80)>>2));
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.cb20 & ~data) locals.lampadr1 = locals.a0 & 0x0f;
  locals.cb20 = data;
}
/* PIA1:CA2-W Lamp Strobe #2 */
static WRITE_HANDLER(pia1ca2_w) {
  if (locals.ca21 & ~data) {
    locals.lampadr2 = locals.a0 & 0x0f;
    if (locals.displays > 6) { locals.bcd[6] = locals.a0>>4; by35_dispStrobe(0x40); }
  }
  locals.diagnosticLed = data;
  locals.ca21 = data;
}

/* PIA0:CA2-W Display Strobe */
static WRITE_HANDLER(pia0ca2_w) {
  locals.ca20 = data;
  if (!data) by35_dispStrobe(0x1f);
}

/* PIA1:B-W Solenoid/Sound output */
static WRITE_HANDLER(pia1b_w) {
  // m_mpac got a 6th display connected to solenoid 20
  if ((locals.displays > 5) && (~locals.b1 & data & 0x80))
    { locals.bcd[5] = locals.a0>>4; by35_dispStrobe(0x20); }
  locals.b1 = data;

//  sndbrd_0_data_w(0, (locals.cb21<<5) | ((locals.a1 & 0x02)<<3) | (data & 0x0f));
  sndbrd_0_data_w(0, ((locals.a1 & 0x02)<<3) | (data & 0x0f));
  coreGlobals.pulsedSolState = 0;
  if (!locals.cb21)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
}

/* PIA1:CB2-W Solenoid/Sound select */
static WRITE_HANDLER(pia1cb2_w) {
  //DBGLOG(("PIA1:CB2=%d\n",data));
  locals.cb21 = data;
//  sndbrd_0_data_w(0, (locals.cb21<<5) | ((locals.a1 & 0x02)<<3) | (locals.b1 & 0x0f));
  sndbrd_0_ctrl_w(0, data);
}

static int by35_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BY35_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY35_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BY35_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
  return 0;
}

static void by35_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[BY35_COMINPORT]>>10) & 0x07;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x60)) |
                              ((inports[BY35_COMINPORT]<<5) & 0x60);
    // Adjust Coins, and Slam Tilt Switches for Stern MPU-200 Games!
    if ((core_gameData->gen & GEN_STMPU200))
      coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x87)) |
                                ((inports[BY35_COMINPORT]>>2) & 0x87);
    else
      coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0x87)) |
                                ((inports[BY35_COMINPORT]>>2) & 0x87);
  }
  /*-- Diagnostic buttons on CPU board --*/
  cpu_set_nmi_line(0, core_getSw(BY35_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(BY35_SWSOUNDDIAG));
  /*-- coin door switches --*/
  pia_set_input_ca1(BY35_PIA0, !core_getSw(BY35_SWSELFTEST));
}

/*PIA 1*/
/*PIA U10:
(in)  PA5:   S01-S08 Dips
(in)  PA6:   S09-S16 Dips
(in)  PA7:   S17-S24 Dips
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  CA1:   Self Test Switch
(in)  CB1:   Zero Cross Detection
(in)  CA2:   N/A
(in)  CB2:   S25-S32 Dips
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-4: Switch Strobe(Columns)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-3: Display Latch Strobe
(out) PA4-7: BCD Lamp Data & BCD Display Data
(out) PB0-7: Dips Column Strobe
(out) CA2:   Display Blanking/(Select?)
(out) CB2:   Lamp Strobe #1 (Drives Main Lamp Driver - Playfield Lamps)
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/
/*PIA 2*/
/*PIA U11:
(in)  PA0-7	 N/A?
(in)  PB0-7  N/A?
(in)  CA1:   Display Interrupt Generator
(in)  CB1:	 J5 - Pin 32
(in)  CA2:	 N/A?
(in)  CB2:	 N/A?
(out) PA0:	 Bit 5 of Display Latch Strobe
(out) PA1:	 Sound Module Address E
(out) PA2-7: Display Digit Enable
(out) PB0-3: Momentary Solenoid/Sound Data
(out) PB4-7: Continuous Solenoid
(out) CA2:	 Diag LED + Lamp Strobe #2 (Drives Aux. Lamp Driver - Backglass Lamps)
(out) CB2:   Solenoid/Sound Bank Select
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/
static struct pia6821_interface by35_pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
}};

static int by35_irq(void) {
  static int last = 0;
  pia_set_input_ca1(BY35_PIA1, last = !last);
  return 0;
}

static core_tData by35Data = {
  32, /* 32 Dips */
  by35_updSw, 1, sndbrd_0_data_w, "by35",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void by35_zeroCross(int data) {
  pia_pulse_cb1(BY35_PIA0, 0);  /*- toggle zero/detection circuit-*/
}
static void by35_init(void) {
  memset(&locals, 0, sizeof(locals));

  if (core_init(&by35Data)) return;
  pia_config(BY35_PIA0, PIA_STANDARD_ORDERING, &by35_pia[0]);
  pia_config(BY35_PIA1, PIA_STANDARD_ORDERING, &by35_pia[1]);
  { int sndType = SNDBRD_NONE;
    switch (core_gameData->gen) {
      case GEN_BY35_32: sndType = SNDBRD_BY32; break;
      case GEN_BY35_50: sndType = SNDBRD_BY50; break;
      case GEN_BY35_51: sndType = SNDBRD_BY51; break;
      case GEN_BY35_61: sndType = SNDBRD_BY61; break;
      case GEN_BY35_61B:sndType = SNDBRD_BY61B; break;
      case GEN_BY35_81: sndType = SNDBRD_BY81; break;
      case GEN_BY35_54: break;
      case GEN_BY35_56: sndType = SNDBRD_BY56; break;
      case GEN_BY35_45: sndType = SNDBRD_BY45; break;
    }
    sndbrd_0_init(sndType, 1, memory_region(BY35_MEMREG_SROM), NULL, NULL);
  }
  locals.vblankCount = 1;
  locals.zctimer = timer_pulse(TIME_IN_HZ(BY35_ZCFREQ),0,by35_zeroCross);
  /* not very pretty or flexible but I don't want to change the gameData structure now */
  if (strcmp(Machine->gamedrv->name,"m_mpac") == 0) locals.displays = 6;
  else if (strcmp(Machine->gamedrv->name,"smman") == 0) locals.displays = 7;
  else locals.displays = 5;
}

static void by35_exit(void) {
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
  sndbrd_0_exit(); core_exit();
}
static UINT8 *by35_CMOS;
// Stern uses 8 bits, Bally 4 top bits
static WRITE_HANDLER(by35_CMOS_w) {
  if ((core_gameData->gen & (GEN_STMPU100|GEN_STMPU200)) == 0) data |= 0x0f;
  by35_CMOS[offset] = data;
}
/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/* Roms: U1: 0x1000-0x1800
         U3: 0x1800-0x2000??
	 U4: 0x2000-0x2800??
	 U5: 0x2800-0x3000??
	 U2: 0x5000-0x5800
	 U6: 0x5800-0x6000
*/
static MEMORY_READ_START(by35_readmem)
  { 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x0088, 0x008b, pia_r(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_r(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(by35_writemem)
  { 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0200, 0x02ff, by35_CMOS_w, &by35_CMOS }, /* CMOS Battery Backed*/
  { 0x0088, 0x008b, pia_w(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_w(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

const struct MachineDriver machine_driver_by35 = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  }},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  by35_nvram
};
const struct MachineDriver machine_driver_by35_32s = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  }},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY32_SOUND},
  by35_nvram
};
const struct MachineDriver machine_driver_by35_51s = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  },BY51_SOUND_CPU},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY51_SOUND},
  by35_nvram
};
const struct MachineDriver machine_driver_by35_56s = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  },BY56_SOUND_CPU},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY56_SOUND},
  by35_nvram
};
const struct MachineDriver machine_driver_by35_61s = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  },BY61_SOUND_CPU},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY61_SOUND},
  by35_nvram
};
const struct MachineDriver machine_driver_by35_45s = {
  {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      by35_readmem, by35_writemem, NULL, NULL,
      by35_vblank, 1, by35_irq, BY35_IRQFREQ
  },BY45_SOUND_CPU},
  BY35_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by35_init, by35_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY45_SOUND},
  by35_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void by35_nvram(void *file, int write) {
  core_nvram(file, write, by35_CMOS, 0x100,0xff);
}
