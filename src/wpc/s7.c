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
#define S7_BANK0 1

static void s7_exit(void);
static void s7_nvram(void *file, int write);

/*----------------
/  Local varibles
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments, pseg;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn; /* Special solenoids and flippers enabled ? */
  int    piaIrq;
} s7locals;
static data8_t *s7_rambankptr, *s7_CMOS;

static void s7_irqline(int state) {
  if (state) {
    cpu_set_irq_line(S7_CPUNO, M6808_IRQ_LINE, ASSERT_LINE);
    pia_set_input_ca1(S7_PIA3, core_getSw(S7_SWADVANCE));
    pia_set_input_cb1(S7_PIA3, core_getSw(S7_SWUPDN));
  }
  else if (!s7locals.piaIrq) {
    cpu_set_irq_line(S7_CPUNO, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(S7_PIA3, 0);
    pia_set_input_cb1(S7_PIA3, 0);
  }
}

static void s7_piaIrq(int state) {
  s7_irqline(s7locals.piaIrq = state);
}

static int s7_irq(void) {
  s7_irqline(1);
  timer_set(TIME_IN_CYCLES(32,0),0,s7_irqline);
  return 0;
}

static int s7_vblank(void) {
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
  if ((s7locals.vblankCount % S7_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = s7locals.solenoids;
    if (s7locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(S7_GAMEONSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 8; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    s7locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((s7locals.vblankCount % S7_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s7locals.segments, sizeof(coreGlobals.segments));
    memcpy(s7locals.segments, s7locals.pseg, sizeof(s7locals.segments));
    coreGlobals.diagnosticLed = s7locals.diagnosticLed;
    s7locals.diagnosticLed = 0;
  }
  core_updateSw(s7locals.ssEn);
  return 0;
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
  if (core_gameData->gen & (GEN_S7|GEN_S9))
    s7locals.diagnosticLed |= core_bcd2seg[(data & 0x70)>>4];
  else
    s7locals.diagnosticLed |= (data & 0x10)>>4;
}
static WRITE_HANDLER(pia3ca2_w) {
  DBGLOG(("pia3ca2_w\n",data));
}

static WRITE_HANDLER(pia3b_w) {
  s7locals.segments[0][s7locals.digSel].lo |=
    s7locals.pseg[0][s7locals.digSel].lo = core_bcd2seg[data&0x0f];
  s7locals.segments[1][s7locals.digSel].lo |=
    s7locals.pseg[1][s7locals.digSel].lo = core_bcd2seg[data>>4];
}
static WRITE_HANDLER(pia0b_w) {
  s7locals.pseg[0][s7locals.digSel].lo &= 0x7f;
  s7locals.pseg[1][s7locals.digSel].lo &= 0x7f;
  if (data & 0x80) {
    s7locals.segments[1][s7locals.digSel].lo |= 0x80;
    s7locals.pseg[1][s7locals.digSel].lo |= 0x80;
  }
  if (data & 0x40) {
    s7locals.segments[0][s7locals.digSel].lo |= 0x80;
    s7locals.pseg[0][s7locals.digSel].lo |= 0x80;
  }
}

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (s7locals.ssEn & (~data & 1))
    { coreGlobals.pulsedSolState |= bit;  s7locals.solenoids |= bit; }
  else
    coreGlobals.pulsedSolState &= ~bit;
}

static WRITE_HANDLER(pia1b_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
  s7locals.solenoids |= (data<<8);
}
static WRITE_HANDLER(pia1a_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  s7locals.solenoids |= data;
}
static WRITE_HANDLER(pia1cb2_w) { s7locals.ssEn = data;}
static WRITE_HANDLER(pia0ca2_w) { setSSSol(data, 7); }
static WRITE_HANDLER(pia0cb2_w) { setSSSol(data, 6); }
static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia2ca2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia2cb2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 5); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 3); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 2); }

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { s7locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(s7locals.swCol); }

static struct pia6821_interface s7_pia[] = {
{  /* PIA 0 (2100)
    PA0-4 Sound
    PB5   CA1
    PB6   Comma 3+4
    PB7   Comma 1+2
    CA2   SS8
    CB2   SS7
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ sndbrd_0_data_w, pia0b_w, pia0ca2_w, pia0cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 1 (2200)
    PA0-7 Sol 1-8
    PB0-7 Sol 9-16
    CA2   SS5
    CB2   GameOn (0)
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ s7_piaIrq, s7_piaIrq
},{/* PIA 2 (2400)
    PA0-7  Lamp return
    PB0-7  Lamp Strobe
    CA2    SS2
    CB2    SS1
    CA1,CB1 NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
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
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0,0, 0, 0,
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
}};

static void s7_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S7_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S7_COMINPORT];
  }
  /*-- Generate interupts for diganostic keys --*/
  cpu_set_nmi_line(0, core_getSw(S7_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(S7_SWSOUNDDIAG));
}

const static core_tData s7Data = {
  2, /* On sound board */
  s7_updSw, CORE_DIAG7SEG, sndbrd_0_data_w, "s7",
  core_swSeq2m, core_swSeq2m,core_m2swSeq,core_m2swSeq
};

static void s7_init(void) {
  if (core_gameData == NULL) return;
  if (core_init(&s7Data)) return;
  pia_config(S7_PIA0, PIA_STANDARD_ORDERING, &s7_pia[0]);
  pia_config(S7_PIA1, PIA_STANDARD_ORDERING, &s7_pia[1]);
  pia_config(S7_PIA2, PIA_STANDARD_ORDERING, &s7_pia[2]);
  pia_config(S7_PIA3, PIA_STANDARD_ORDERING, &s7_pia[3]);
  pia_config(S7_PIA4, PIA_STANDARD_ORDERING, &s7_pia[4]);
  sndbrd_0_init(SNDBRD_S67S, 1, NULL, NULL, NULL);
  pia_reset();
  cpu_setbank(S7_BANK0, s7_rambankptr);
}

static void s7_exit(void) {
  sndbrd_0_exit(); core_exit();
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
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(s7_writemem)
  { 0x0000, 0x00ff, MWA_BANKNO(S7_BANK0) },
  { 0x0100, 0x01ff, s7_CMOS_w, &s7_CMOS },
  { 0x1000, 0x13ff, MWA_RAM, &s7_rambankptr }, /* CMOS */
  { 0x2100, 0x2103, pia_w(S7_PIA0)},
  { 0x2200, 0x2203, pia_w(S7_PIA1)},
  { 0x2400, 0x2403, pia_w(S7_PIA2)},
  { 0x2800, 0x2803, pia_w(S7_PIA3)},
  { 0x3000, 0x3003, pia_w(S7_PIA4)},
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
const struct MachineDriver machine_driver_s7_s = {
  {{  CPU_M6808, 3.58e6/4,
      s7_readmem, s7_writemem, NULL, NULL,
      s7_vblank, 1, s7_irq, S7_IRQFREQ
  }, S67S_SOUNDCPU
  },
  S7_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s7_init, s7_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S67S_SOUND },
  s7_nvram
};

const struct MachineDriver machine_driver_s7 = {
  {{  CPU_M6808, 3.58e6/4,
      s7_readmem, s7_writemem, NULL, NULL,
      s7_vblank, 1, s7_irq, S7_IRQFREQ
  }},
  S7_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s7_init, s7_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{ 0 }},
  s7_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void s7_nvram(void *file, int write) {
  core_nvram(file, write, s7_CMOS, 0x0100, 0xff);
}


