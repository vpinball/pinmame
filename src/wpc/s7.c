#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "s7.h"

#define S7_VBLANKFREQ    60 /* VBLANK frequency */
#define S7_IRQFREQ     1000
#define S7_PIA0  0
#define S7_PIA1  1
#define S7_PIA2  2
#define S7_PIA3  3
#define S7_PIA4  4
#define S7_PIA5  5
#define S7_BANK0 1

#define S7_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define S7_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S7_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

static NVRAM_HANDLER(s7);


/*----------------
/ Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT32 solsmooth[S7_SOLSMOOTH];
  core_tSeg segments;
  UINT16 alphaSegs;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn; /* Special solenoids and flippers enabled ? */
  int    piaIrq;
  int    s6sound;
  int    rr; /* Tweaks for Rat Race */
  UINT32 custSol;	/* 10 custom solenoids for Defender */
  UINT8  solBits1,solBits2;
} s7locals;
static data8_t *s7_rambankptr, *s7_CMOS;

static void s7_irqline(int state) {
  if (state) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, ASSERT_LINE);
    pia_set_input_ca1(S7_PIA3, core_getSw(S7_SWADVANCE));
    pia_set_input_cb1(S7_PIA3, core_getSw(S7_SWUPDN));
  }
  else if (!s7locals.piaIrq) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(S7_PIA3, 0);
    pia_set_input_cb1(S7_PIA3, 0);
  }
}

static void s7_piaIrq(int state) {
  s7_irqline(s7locals.piaIrq = state);
}

static INTERRUPT_GEN(s7_irq) {
  s7_irqline(1);
  timer_set(TIME_IN_CYCLES(32,0),0,s7_irqline);
}

static INTERRUPT_GEN(s7_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  s7locals.vblankCount += 1;
  /*-- lamps --*/
  if ((s7locals.vblankCount % S7_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if (s7locals.ssEn) {
    int ii;
    s7locals.solenoids |= CORE_SOLBIT(S7_GAMEONSOL);
    /*-- special solenoids updated based on switches --*/
    for (ii = 0; ii < 8; ii++) {
      if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
        s7locals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
  }
  s7locals.solsmooth[s7locals.vblankCount % S7_SOLSMOOTH] = s7locals.solenoids;
#if S7_SOLSMOOTH != 2
#  error "Need to update smooth formula"
#endif
  coreGlobals.solenoids = s7locals.solsmooth[0] | s7locals.solsmooth[1];
  s7locals.solenoids = coreGlobals.pulsedSolState;

  /*-- display --*/
  if ((s7locals.vblankCount % S7_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s7locals.segments, sizeof(coreGlobals.segments));
    memset(s7locals.segments,0,sizeof(s7locals.segments));
    coreGlobals.diagnosticLed = s7locals.diagnosticLed;
  }
  core_updateSw(s7locals.ssEn);
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia2a_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s7locals.lampColumn, s7locals.lampRow = ~data);
}
static WRITE_HANDLER(pia2b_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s7locals.lampColumn = data, s7locals.lampRow);
}

/*-----------------
/ Display handling
/-----------------*/
static WRITE_HANDLER(pia3a_w) {
  s7locals.digSel = data & 0x0f;
  s7locals.diagnosticLed = core_bcd2seg7e[(data>>4)^15];
}
static WRITE_HANDLER(pia3ca2_w) {
  DBGLOG(("pia3ca2_w\n"));
}

static WRITE_HANDLER(pia3b_w) {
  int seg7;
  seg7 = core_bcd2seg[data >> 4] & 0x7F;
  if (seg7) {
    s7locals.segments[   s7locals.digSel].w &= 0x80;
    s7locals.segments[   s7locals.digSel].w |= seg7;
  }
  seg7 = core_bcd2seg[data & 15] & 0x7F;
  if (seg7) {
    s7locals.segments[20+s7locals.digSel].w &= 0x80;
    s7locals.segments[20+s7locals.digSel].w |= seg7;
  }
}

static WRITE_HANDLER(pia0b_w) {
  if (data & 0x80) s7locals.segments[ 1+s7locals.digSel].w |= 0x80;
  if (data & 0x40) s7locals.segments[21+s7locals.digSel].w |= 0x80;
}

/*---------------------------
/ Alpha Display on Hyperball
/----------------------------*/
static WRITE_HANDLER(pia5a_w) {
  s7locals.alphaSegs = data & 0x3f;
  if (data & 0x40) s7locals.alphaSegs |= 0x100;
  if (data & 0x80) s7locals.alphaSegs |= 0x200;
}
static WRITE_HANDLER(pia5b_w) {
  if (data & 0x01) s7locals.alphaSegs |= 0x400;
  if (data & 0x02) s7locals.alphaSegs |= 0x40;
  if (data & 0x04) s7locals.alphaSegs |= 0x800;
  if (data & 0x08) s7locals.alphaSegs |= 0x4000;
  if (data & 0x10) s7locals.alphaSegs |= 0x2000;
  if (data & 0x20) s7locals.alphaSegs |= 0x1000;
  if (data & 0x40) s7locals.alphaSegs |= 0x80;
  if (data & 0x80) s7locals.alphaSegs |= 0x8000;
  s7locals.segments[40+s7locals.digSel].w |= s7locals.alphaSegs;
}
static WRITE_HANDLER(pia5ca2_w) {
  DBGLOG(("pia5ca2_w\n"));
}
static WRITE_HANDLER(pia5cb2_w) {
  DBGLOG(("pia5cb2_w\n"));
}

/*------------
/  Solenoids
/-------------*/
// Custom multiplexed solenoids 51-60 on Defender
int dfndrCustSol(int solno) {
  return s7locals.custSol & (1 << (solno - CORE_FIRSTCUSTSOL));
}

static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (s7locals.rr) { // Rat Race doesn't use the ssEn output, and has the special solenoids non-inverted?
    if (data)
      { coreGlobals.pulsedSolState |= bit;  s7locals.solenoids |= bit; }
    else
      coreGlobals.pulsedSolState &= ~bit;
    return;
  }
  if (s7locals.ssEn & (~data & 1))
    { coreGlobals.pulsedSolState |= bit;  s7locals.solenoids |= bit; }
  else
    coreGlobals.pulsedSolState &= ~bit;
}

static void updsol(void) {
  /* set new solenoids, preserve SSSol */
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x01ff0000)
                             | (s7locals.solBits2 << 8)
                             | (s7locals.solBits1     );

  /* if game has a MUX ... */
  if (core_gameData->sxx.muxSol) {
    if (coreGlobals.pulsedSolState & (1 << (core_gameData->sxx.muxSol - 1))) {
      /* active mux */
      UINT32 muxsol = core_gameData->hw.gameSpecific2; // mux affected solenoids
      s7locals.custSol            = coreGlobals.pulsedSolState & muxsol;
      coreGlobals.pulsedSolState &= ~muxsol;
    }
    else {
      /* inactive mux */
      s7locals.custSol = 0;
    }
  }

  s7locals.solenoids |= coreGlobals.pulsedSolState;
}

static WRITE_HANDLER(pia1b_w) {
  if (data != s7locals.solBits2) {
    s7locals.solBits2 = data;
    updsol();
  }
  if (s7locals.s6sound) {
    sndbrd_0_data_w(0, ~data); data &= 0xe0; /* mask of sound command bits */
  }
}

static WRITE_HANDLER(pia1a_w) {
  if (data != s7locals.solBits1) {
    s7locals.solBits1 = data;
    updsol();
  }
  // the following lines draw the extra lamp columns on Hyperball!
  if (s7locals.lampColumn & 0x01) core_setLamp(coreGlobals.tmpLampMatrix, 0x100, ~data & 0x0f);
  if (s7locals.lampColumn & 0x02) core_setLamp(coreGlobals.tmpLampMatrix, 0x100, ~data << 4);
  if (s7locals.lampColumn & 0x04) core_setLamp(coreGlobals.tmpLampMatrix, 0x200, ~data & 0x0f);
  if (s7locals.lampColumn & 0x08) core_setLamp(coreGlobals.tmpLampMatrix, 0x200, ~data << 4);
  if (s7locals.lampColumn & 0x10) core_setLamp(coreGlobals.tmpLampMatrix, 0x400, ~data & 0x0f);
  if (s7locals.lampColumn & 0x20) core_setLamp(coreGlobals.tmpLampMatrix, 0x400, ~data << 4);
  if (s7locals.lampColumn & 0x40) core_setLamp(coreGlobals.tmpLampMatrix, 0x800, ~data & 0x0f);
  if (s7locals.lampColumn & 0x80) core_setLamp(coreGlobals.tmpLampMatrix, 0x800, ~data << 4);
}

static WRITE_HANDLER(pia1cb2_w) { s7locals.ssEn = data; }
static WRITE_HANDLER(pia0ca2_w) { setSSSol(data, 7); }
static WRITE_HANDLER(pia0cb2_w) { setSSSol(data, 6); }
static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia2ca2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia2cb2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 5); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 3); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 2); }

/*-----------------------------------------------------------------
  Dip switch support
-------------------------------------------------------------------
  Dips are returned to a4-7 and are read by pulsing of a0-3 (same as digSel).
  Dips are matrixed.. Column 0 = Function Switches 1-4, Column 1 = Function Switches 5-8
  		      Column 2 = Data Switches 1-4, Column 3 = Data Switches 5-8
  Game expects on=0, off=1 from dip switches.
  Game expects 0xFF for function dips when ENTER-key is not pressed.
*/
static READ_HANDLER(s7_dips_r) {
  int val=0;
  int dipcol = (s7locals.digSel & 0x03);  /* which column is requested */
  if (core_getSw(S7_ENTER))               /* enter switch must be down to return dips */
    val = (core_getDip(dipcol/2+1) << (4*(1-(dipcol&0x01)))) & 0xf0;
  return ~val;                            /* game wants bits inverted */
}

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { s7locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(s7locals.swCol); }

/*---------------
/  Sound command
/----------------*/
static WRITE_HANDLER(pia0a_w) {
  if (s7locals.rr) { // Rat Race needs the system 9 sound PIA triggered as well
    sndbrd_0_data_w(0, data & 0x80 ? ~data : data); // just so diag sounds are working too... what the heck! :)
    pia_set_input_ca1(6, 1);
    pia_set_input_ca1(6, 0);
  } else
    sndbrd_0_data_w(0, data);
}

static struct pia6821_interface s7_pia[] = {
{  /* PIA 0 (2100)
    PA0-4 Sound
    PB5   CA1
    PB6   Comma 3+4
    PB7   Comma 1+2
    CA2   SS8
    CB2   SS7
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, PIA_UNUSED_VAL(0x3f), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 1 (2200)
    PA0-7 Sol 1-8 (Extra lamp strobe on Hyperball)
    PB0-7 Sol 9-16
    CA2   SS5
    CB2   GameOn (0)
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 2 (2400)
    PA0-7  Lamp return
    PB0-7  Lamp Strobe
    CA2    SS2
    CB2    SS1
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, PIA_UNUSED_VAL(0), 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 3 (2800)
    PA0-3  Digit Select
    PA4-7  Diagnostic LED
    PB0-7  BCD output
    CA1    Diag In
    CB1    Diag In
    CB2    SS6
    CA2    Diagnostic LED control? */
 /* in  : A/B,CA/B1,CA/B2 */ s7_dips_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia3a_w, pia3b_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 4 (3000)
    PA0-7  Switch return
    PB0-7  Switch drive
    CB2    SS3
    CA2    SS4 */
 /* in  : A/B,CA/B1,CA/B2 */ pia4a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia4b_w, pia4ca2_w, pia4cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 5 (4000)
    PA0-7  Digit Select
    PB0-7  alphanumeric output */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
}};

static SWITCH_UPDATE(s7) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S7_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S7_COMINPORT];
  }

  /*-- Generate interupts for diganostic keys --*/
  cpu_set_nmi_line(0,core_getSw(S7_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);

  sndbrd_0_diag(core_getSw(S7_SWSOUNDDIAG));

  core_textOutf(40,30,BLACK,core_getSw(S7_SWUPDN) ? "Up/Auto " : "Down/Man");
}

static MACHINE_INIT(s7) {
  if (core_gameData == NULL) return;
  memset(&s7locals,0,sizeof(s7locals));
  pia_config(S7_PIA0, PIA_STANDARD_ORDERING, &s7_pia[0]);
  pia_config(S7_PIA1, PIA_STANDARD_ORDERING, &s7_pia[1]);
  pia_config(S7_PIA2, PIA_STANDARD_ORDERING, &s7_pia[2]);
  pia_config(S7_PIA3, PIA_STANDARD_ORDERING, &s7_pia[3]);
  pia_config(S7_PIA4, PIA_STANDARD_ORDERING, &s7_pia[4]);
  pia_config(S7_PIA5, PIA_STANDARD_ORDERING, &s7_pia[5]);
  sndbrd_0_init(SNDBRD_S67S, 1, NULL, NULL, NULL);
  cpu_setbank(S7_BANK0, s7_rambankptr);
}

static MACHINE_INIT(s7S6) {
  machine_init_s7();
  s7locals.s6sound = 1;
}

static MACHINE_RESET(s7) {
  pia_reset();
}

static MACHINE_STOP(s7) {
  sndbrd_0_exit();
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static WRITE_HANDLER(s7_CMOS_w) { s7_CMOS[offset] = data | 0xf0; }

/*---------------------------------
  IC13+
  IC16 RAM (0000-007f, 1000-13ff)
  IC19 RAM (0100-01ff)
  IC22 ROM (6200-63ff)
  IC21 ROM (6000-61ff)
  IC14 ROM (6000-67ff)
  IC17 ROM (7000-77ff)
  IC26 ROM (5800-5fff)
  IC20 ROM (6800-6fff)
-----------------------------------*/
static MEMORY_READ_START(s7_readmem)
  { 0x0000, 0x00ff, MRA_BANKNO(S7_BANK0)},
  { 0x0100, 0x01ff, MRA_RAM},
  { 0x1000, 0x13ff, MRA_RAM}, /* CMOS */
  { 0x2100, 0x2103, pia_r(S7_PIA0)},
  { 0x2200, 0x2203, pia_r(S7_PIA1)},
  { 0x2400, 0x2403, pia_r(S7_PIA2)},
  { 0x2800, 0x2803, pia_r(S7_PIA3)},
  { 0x3000, 0x3003, pia_r(S7_PIA4)},
  { 0x4000, 0x4003, pia_r(S7_PIA5)},
  { 0x5000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(s7_writemem)
  { 0x0000, 0x00ff, MWA_BANKNO(S7_BANK0)},
  { 0x0100, 0x01ff, s7_CMOS_w, &s7_CMOS },
  { 0x1000, 0x13ff, MWA_RAM, &s7_rambankptr }, /* CMOS */
  { 0x2100, 0x2103, pia_w(S7_PIA0)},
  { 0x2200, 0x2203, pia_w(S7_PIA1)},
  { 0x2400, 0x2403, pia_w(S7_PIA2)},
  { 0x2800, 0x2803, pia_w(S7_PIA3)},
  { 0x3000, 0x3003, pia_w(S7_PIA4)},
  { 0x4000, 0x4003, pia_w(S7_PIA5)},
  { 0x5000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
MACHINE_DRIVER_START(s7)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(s7,s7,s7)
  MDRV_CPU_ADD_TAG("mcpu", M6808, 3580000/4)
  MDRV_CPU_MEMORY(s7_readmem, s7_writemem)
  MDRV_CPU_VBLANK_INT(s7_vblank, 1)
  MDRV_CPU_PERIODIC_INT(s7_irq, S7_IRQFREQ)
  MDRV_NVRAM_HANDLER(s7)
  MDRV_DIPS(8+16) /* 0:sound & speech, 1:function dips, 2:data dips*/
  MDRV_SWITCH_UPDATE(s7)
  MDRV_DIAGNOSTIC_LED7
MACHINE_DRIVER_END

MACHINE_DRIVER_START(s7S)
  MDRV_IMPORT_FROM(s7)
  MDRV_IMPORT_FROM(wmssnd_s67s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("s7")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(s7S6)
  MDRV_IMPORT_FROM(s7)
  MDRV_CORE_INIT_RESET_STOP(s7S6,s7,s7)
  MDRV_IMPORT_FROM(wmssnd_s67s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("s7")
MACHINE_DRIVER_END

/*-------------------------
/  Changes for Thunderball
/--------------------------*/
// This game won't use sound dips, but transmit 7 sound bits!
static MACHINE_INIT(s7nd) {
  if (core_gameData == NULL) return;
  memset(&s7locals,0,sizeof(s7locals));
  pia_config(S7_PIA0, PIA_STANDARD_ORDERING, &s7_pia[0]);
  pia_config(S7_PIA1, PIA_STANDARD_ORDERING, &s7_pia[1]);
  pia_config(S7_PIA2, PIA_STANDARD_ORDERING, &s7_pia[2]);
  pia_config(S7_PIA3, PIA_STANDARD_ORDERING, &s7_pia[3]);
  pia_config(S7_PIA4, PIA_STANDARD_ORDERING, &s7_pia[4]);
  sndbrd_0_init(SNDBRD_S7S_ND, 1, NULL, NULL, NULL);
  cpu_setbank(S7_BANK0, s7_rambankptr);
}

MACHINE_DRIVER_START(s7SND)
  MDRV_IMPORT_FROM(s7S)
  MDRV_CORE_INIT_RESET_STOP(s7nd,s7,s7)
MACHINE_DRIVER_END

/*----------------------
/  Changes for Rat Race
/-----------------------*/
// This game doesn't use PIA1, but writes the solenoids directly!
static WRITE_HANDLER(rr_sol_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  s7locals.solenoids |= data;
}

static MEMORY_READ_START(rr_readmem)
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x2100, 0x2103, pia_r(S7_PIA0)},
  { 0x2400, 0x2403, pia_r(S7_PIA2)},
  { 0x2800, 0x2803, pia_r(S7_PIA3)},
  { 0x3000, 0x3003, pia_r(S7_PIA4)},
  { 0x5000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(rr_writemem)
  { 0x0000, 0x06ff, MWA_RAM },
  { 0x0700, 0x07ff, MWA_RAM, &s7_CMOS},
  { 0x2100, 0x2103, pia_w(S7_PIA0)},
  { 0x2200, 0x2200, rr_sol_w},
  { 0x2400, 0x2403, pia_w(S7_PIA2)},
  { 0x2800, 0x2803, pia_w(S7_PIA3)},
  { 0x3000, 0x3003, pia_w(S7_PIA4)},
  { 0x5000, 0xffff, MWA_ROM },
MEMORY_END

static MACHINE_INIT(rr) {
  if (core_gameData == NULL) return;
  memset(&s7locals,0,sizeof(s7locals));
  pia_config(S7_PIA0, PIA_STANDARD_ORDERING, &s7_pia[0]);
  pia_config(S7_PIA2, PIA_STANDARD_ORDERING, &s7_pia[2]);
  pia_config(S7_PIA3, PIA_STANDARD_ORDERING, &s7_pia[3]);
  pia_config(S7_PIA4, PIA_STANDARD_ORDERING, &s7_pia[4]);
  sndbrd_0_init(SNDBRD_S9S, 1, NULL, NULL, NULL);
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  s7locals.ssEn= 1;
  s7locals.rr = 1;
}

MACHINE_DRIVER_START(s7RR)
  MDRV_IMPORT_FROM(s7)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(rr_readmem, rr_writemem)
  MDRV_CORE_INIT_RESET_STOP(rr,s7,s7)
  MDRV_DIPS(8)
  MDRV_NVRAM_HANDLER(s7)
  MDRV_IMPORT_FROM(wmssnd_s9s)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(s7) {
  core_nvram(file, read_or_write, s7_CMOS, 0x0100, 0xff);
}
