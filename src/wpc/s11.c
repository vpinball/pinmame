#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "snd_cmd.h"
#include "wmssnd.h"
#include "desound.h"
#include "dedmd.h"
#include "s11.h"

// TODO:
// nvram save for DE games
// de buttons
// DE display layouts
#define S11_PIA0 0
#define S11_PIA1 1
#define S11_PIA2 2
#define S11_PIA3 3
#define S11_PIA4 4
#define S11_PIA5 5

#define S11_VBLANKFREQ    60 /* VBLANK frequency */

#define S11_IRQFREQ     1000

static void s11_exit(void);
static void s11_nvram(void *file, int write);

const core_tLCDLayout s11_dispS9[] = {
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_CREDIT(20,28,CORE_SEG7S),DISP_SEG_BALLS(0,8,CORE_SEG7S),{0}
};
const core_tLCDLayout s11_dispS11[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG8), DISP_SEG_7(1,1,CORE_SEG8),
  DISP_SEG_CREDIT(20,28,CORE_SEG7S),DISP_SEG_BALLS(0,8,CORE_SEG7SH),{0}
};
const core_tLCDLayout s11_dispS11a[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG8), DISP_SEG_7(1,1,CORE_SEG8) ,{0}
};
const core_tLCDLayout s11_dispS11b2[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

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
  int    sndCmd, extSol; /* external sound board cmd */
  int    piaIrq;
} s11locals;

static void s11_irqline(int state) {
  int deGame = core_gameData->gen & (GEN_DE|GEN_DEDMD16|GEN_DEDMD32|GEN_DEDMD64);
  if (state) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, ASSERT_LINE);
    if (deGame) {
      pia_set_input_ca1(S11_PIA2, !core_getSw(DE_SWADVANCE));
      pia_set_input_cb1(S11_PIA2, !core_getSw(DE_SWUPDN));
    }
    else {
      pia_set_input_ca1(S11_PIA2, core_getSw(S11_SWADVANCE));
      pia_set_input_cb1(S11_PIA2, core_getSw(S11_SWUPDN));
    }
  }
  else if (!s11locals.piaIrq) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(S11_PIA2, deGame);
    pia_set_input_cb1(S11_PIA2, deGame);
  }
}

static void s11_piaMainIrq(int state) {
  s11_irqline(s11locals.piaIrq = state);
}

static int s11_irq(void) {
  s11_irqline(1); timer_set(TIME_IN_CYCLES(32,0),0,s11_irqline);
  return 0;
}

static int s11_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  s11locals.vblankCount += 1;
  /*-- lamps --*/
  if ((s11locals.vblankCount % S11_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((s11locals.vblankCount % S11_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = s11locals.solenoids;
    coreGlobals.solenoids2 = (s11locals.extSol<<8);
    if ((core_gameData->sxx.muxSol) &&
        (s11locals.solenoids & CORE_SOLBIT(core_gameData->sxx.muxSol)))
      coreGlobals.solenoids = (s11locals.solenoids & 0x00ffff00) | (s11locals.solenoids<<24);
    if (s11locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(S11_GAMEONSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    s11locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((s11locals.vblankCount % S11_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s11locals.segments, sizeof(coreGlobals.segments));
    memcpy(s11locals.segments, s11locals.pseg, sizeof(s11locals.segments));
    coreGlobals.diagnosticLed = s11locals.diagnosticLed;
    s11locals.diagnosticLed = 0;
  }
  core_updateSw(s11locals.ssEn);
  return 0;
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia1a_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s11locals.lampColumn, s11locals.lampRow = ~data);
}
static WRITE_HANDLER(pia1b_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, s11locals.lampColumn = data, s11locals.lampRow);
}

/*-- Jumper W7 --*/
static READ_HANDLER (pia2a_r) { return core_getDip(0)<<7; }

/*-----------------
/ Display handling
/-----------------*/
static WRITE_HANDLER(pia2a_w) {
  s11locals.digSel = data & 0x0f;
  if (core_gameData->hw.display & S11_BCDDIAG)
    s11locals.diagnosticLed |= core_bcd2seg[(data & 0x70)>>4];
  else
    s11locals.diagnosticLed |= (data & 0x10)>>4;
}

static WRITE_HANDLER(pia2b_w) {
  if (core_gameData->gen & (GEN_DEDMD16|GEN_DEDMD32|GEN_DEDMD64))
    s11locals.extSol = data;
  else {
    if (core_gameData->hw.display & S11_DISPINV) data = ~data;
    if (core_gameData->hw.display & S11_BCDDISP) {
      s11locals.segments[0][s11locals.digSel].lo |=
           s11locals.pseg[0][s11locals.digSel].lo = core_bcd2seg[data&0x0f];
      s11locals.segments[1][s11locals.digSel].lo |=
           s11locals.pseg[1][s11locals.digSel].lo = core_bcd2seg[data>>4];
    }
    else if (core_gameData->hw.display & S11_LOWALPHA)
      s11locals.segments[1][s11locals.digSel].hi |=
          s11locals.pseg[1][s11locals.digSel].hi = data;
    else
      s11locals.segments[1][s11locals.digSel].lo |=
          s11locals.pseg[1][s11locals.digSel].lo = data;
  }
}
static WRITE_HANDLER(pia5a_w) { // Not used for DMD
  if (core_gameData->hw.display & S11_DISPINV) data = ~data;
  if (core_gameData->hw.display & S11_LOWALPHA)
    s11locals.segments[1][s11locals.digSel].lo |=
         s11locals.pseg[1][s11locals.digSel].lo = data;
}
static WRITE_HANDLER(pia3a_w) {
  if (core_gameData->gen & (GEN_DEDMD16|GEN_DEDMD32|GEN_DEDMD64))
    sndbrd_0_data_w(0, data);
  else {
    if (core_gameData->hw.display & S11_DISPINV) data = ~data;
    s11locals.segments[0][s11locals.digSel].lo |=
        s11locals.pseg[0][s11locals.digSel].lo = data;
  }
}
static WRITE_HANDLER(pia3b_w) {
  if (core_gameData->gen & (GEN_DEDMD16|GEN_DEDMD32|GEN_DEDMD64))
    { sndbrd_0_ctrl_w(0, data); DBGLOG(("ctrl_w=%x\n",data)); }
  else {
    if (core_gameData->hw.display & S11_DISPINV) data = ~data;
    s11locals.segments[0][s11locals.digSel].hi |=
        s11locals.pseg[0][s11locals.digSel].hi = data;
  }
}
static READ_HANDLER(pia3b_r) {
  if (core_gameData->gen & GEN_DEDMD32)
    return (sndbrd_0_data_r(0) ? 0x80 : 0x00) | (sndbrd_0_ctrl_r(0)<<3);
  else if (core_gameData->gen & GEN_DEDMD64)
    return sndbrd_0_data_r(0) ? 0x80 : 0x00;
  return 0;
}

static WRITE_HANDLER(pia2ca2_w) {
  data = data ? 0x80 : 0x00;
  s11locals.segments[1][s11locals.digSel].lo |= data;
  s11locals.pseg[1][s11locals.digSel].lo = (s11locals.pseg[1][s11locals.digSel].lo & 0x7f) | data;
}
static WRITE_HANDLER(pia2cb2_w) {
  data = data ? 0x80 : 0x00;
  s11locals.segments[0][s11locals.digSel].lo |= data;
  s11locals.pseg[0][s11locals.digSel].lo = (s11locals.pseg[0][s11locals.digSel].lo & 0x7f) | data;
}

static READ_HANDLER(pia5a_r) {
  if (core_gameData->gen & GEN_DEDMD64)
    return sndbrd_0_ctrl_r(0);
  else if (core_gameData->gen & GEN_DEDMD16)
    return (sndbrd_0_data_r(0) ? 0x01 : 0x00) | (sndbrd_0_ctrl_r(0)<<1);
  return 0;
}

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
                                    /*    WMS          DE */
  static const int ssSolNo[2][6] = {{5,4,1,2,0,4},{3,4,5,1,0,2}};
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + ssSolNo[(core_gameData->gen & GEN_ALLS11) == 0][solNo]);
  if (s11locals.ssEn & (~data & 1))
    { coreGlobals.pulsedSolState |= bit;  s11locals.solenoids |= bit; }
  else
    coreGlobals.pulsedSolState &= ~bit;
}

static WRITE_HANDLER(pia0b_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
  s11locals.solenoids |= (data<<8);
}
static WRITE_HANDLER(latch2200) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  s11locals.solenoids |= data;
}
static WRITE_HANDLER(pia0cb2_w) { s11locals.ssEn = !data;}

static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia1cb2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia3ca2_w) { setSSSol(data, 2); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 3); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 5); }

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { s11locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(s11locals.swCol); }

/*-------
/  Sound
/--------*/
/*-- CPU board sound command --*/
static WRITE_HANDLER(pia0a_w) { snd_cmd_log(0); snd_cmd_log(data); sndbrd_0_data_w(0,data); }
/*-- Sound board sound command--*/
static WRITE_HANDLER(pia5b_w) {
  s11locals.sndCmd = data; snd_cmd_log(1); snd_cmd_log(data); sndbrd_1_data_w(0,data);
}

/*-- Sound board sound command available --*/
static WRITE_HANDLER(pia5cb2_w) {
  /* don't pass to sound board if a sound overlay board is available */
  if ((core_gameData->hw.gameSpecific1 & S11_SNDOVERLAY) &&
    ((s11locals.sndCmd & 0xe0) == 0)) {
    if (!data) s11locals.extSol = (~s11locals.sndCmd) & 0x1f;
  }
  else sndbrd_1_ctrl_w(0,data);
}
/*-- reset sound board CPU --*/
static WRITE_HANDLER(pia5ca2_w) { /*
  if (core_gameData->gen & ~(GEN_S11B_3|GEN_S9)) {
    cpu_set_reset_line(S11_SCPU1NO, PULSE_LINE);
    s11cs_reset();
  } */
}
static WRITE_HANDLER(s11_sndCmd_w) {
  static int soundSys = -1; /* 0 = CPU board sound, 1 = Sound board */
  if (soundSys < 0)
    soundSys = (data & 0x01);
  else {
    sndbrd_data_w(soundSys, data); sndbrd_ctrl_w(soundSys,1); sndbrd_ctrl_w(soundSys,0);
    soundSys = -1;
  }
}

static struct pia6821_interface s11_pia[] = {
{/* PIA 0 (2100) */
 /* PA0 - PA7 Sound Select Outputs (sound latch) */
 /* PB0 - PB7 Solenoid 9-16 (12 is usually for multiplexing) */
  /* CA2       Sound H.S.  */
 /* CB2       Enable Special Solenoids */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia0a_w, pia0b_w, sndbrd_0_ctrl_w, pia0cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 1 (2400) */
 /* PA0 - PA7 Lamp Matrix Strobe */
 /* PB0 - PB7 Lamp Matrix Return */
 /* CA2       F SS6 */
 /* CB2       E SS5 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 2 (2800) */
 /* PA0 - PA3 Digit Select 1-16 */
 /* PA4       Diagnostic LED */
 /* PA5-PA6   NC */
 /* PA7       (I) Jumper W7 */
 /* PB0 - PB7 Digit BCD */
 /* CA1       (I) Diagnostic Advance */
 /* CB1       (I) Diagnostic Up/dn */
 /* CA2       Comma 3+4 */
 /* CB2       Comma 1+2 */
 /* in  : A/B,CA/B1,CA/B2 */ pia2a_r, 0, 0,0, 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 3 (2c00) */
 /* PA0 - PA7 Display Data (h,j,k,m,n,p,r,dot) */
 /* PB0 - PB7 Display Data (a,b,c,d,e,f,g,com) */
 /* CA1       Widget I/O LCA1 */
 /* CB1       Widget I/O LCB1 */
 /* CA2       (I) B SST2 */
 /* CB2       (I) C SST3 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, pia3b_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia3a_w, pia3b_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 4 (3000) */
 /* PA0 - PA7 Switch Input (row) */
 /* PB0 - PB7 Switch Drivers (column) */
 /* CA1/CB1   GND */
 /* CA2       (I) A SS1 */
 /* CB2       (I) D SS4 */
 /* in  : A/B,CA/B1,CA/B2 */ pia4a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia4b_w, pia4ca2_w, pia4cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 5 (3400) */
 /* PA0 - PA7 Display Data' (h,j,k ,m,n,p,r,dot), DMD status */
 /* PB0 - PB7 Widget I/O MD0-MD7 */
 /* CA1       Widget I/O MCA1 */
 /* CB1       Widget I/O MCB1 */
 /* CA2       Widget I/O MCA2 */
 /* CB2       Widget I/O MCB2 */
 /* in  : A/B,CA/B1,CA/B2 */ pia5a_r, soundlatch3_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
}};

static void s11_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S11_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S11_COMINPORT];
  }
  /*-- Generate interupts for diganostic keys --*/
  cpu_set_nmi_line(0, core_getSw(S11_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(S11_SWSOUNDDIAG));
  if ((core_gameData->hw.gameSpecific1 & S11_MUXSW2) && core_gameData->sxx.muxSol)
    core_setSw(2, core_getSol(core_gameData->sxx.muxSol));
}

// convert lamp and switch numbers
// S11 is 1-64
// convert to 0-64 (+8)
// i.e. 1=8, 2=9...
static int s11_sw2m(int no) { return no+7; }
static int s11_m2sw(int col, int row) { return col*8+row-7; }
static core_tData s11Data = {
  1, /* 1 DIP (actually a jumper) */
  s11_updSw, CORE_DIAG7SEG, s11_sndCmd_w, "s11",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};
static core_tData s11AData = {
  1, /* 1 DIP (actually a jumper) */
  s11_updSw, 1, s11_sndCmd_w, "s11",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void s11_init(void) {
  if (core_gameData == NULL)  return;
  pia_config(S11_PIA0, PIA_STANDARD_ORDERING, &s11_pia[0]);
  pia_config(S11_PIA1, PIA_STANDARD_ORDERING, &s11_pia[1]);
  pia_config(S11_PIA2, PIA_STANDARD_ORDERING, &s11_pia[2]);
  pia_config(S11_PIA3, PIA_STANDARD_ORDERING, &s11_pia[3]);
  pia_config(S11_PIA4, PIA_STANDARD_ORDERING, &s11_pia[4]);
  pia_config(S11_PIA5, PIA_STANDARD_ORDERING, &s11_pia[5]);
  if (core_init((core_gameData->hw.display & S11_BCDDIAG) ? &s11Data : &s11AData)) return;
  switch (core_gameData->gen) {
    case GEN_S9:
      sndbrd_0_init(SNDBRD_S9S, 1, NULL, NULL, NULL);
      break;
    case GEN_S11:
      sndbrd_0_init(SNDBRD_S11S,  2, memory_region(S11S_ROMREGION), NULL, NULL);
      sndbrd_1_init(SNDBRD_S11CS, 1, memory_region(S11CS_ROMREGION), pia_5_cb1_w, NULL);
      break;
    case GEN_S11B2:
      sndbrd_0_init(SNDBRD_S11S,  1, memory_region(S11B3S_ROMREGION), NULL, NULL);
    //sndbrd_1_init(SNDBRD_???,   2, memory_region(S11_MEMREG_SROM), pia_5_cb1_w, NULL);
      break;
    case GEN_S11C:
      sndbrd_1_init(SNDBRD_S11CS, 1, memory_region(S11CS_ROMREGION), pia_5_cb1_w, NULL);
      break;
    case GEN_DE:
      sndbrd_1_init(SNDBRD_DE1S,  1, memory_region(DE1S_ROMREGION), pia_5_cb1_w, NULL);
      break;
    case GEN_DEDMD16:
    case GEN_DEDMD32:
    case GEN_DEDMD64:
      sndbrd_0_init(core_gameData->hw.display,    2, memory_region(DE_DMD16ROMREGION),NULL,NULL);
      sndbrd_1_init(core_gameData->hw.soundBoard, 1, memory_region(DE1S_ROMREGION), pia_5_cb1_w, NULL);
      break;
  }
  pia_reset();
}

static void s11_exit(void) {
  sndbrd_0_exit(); sndbrd_1_exit(); core_exit();
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(s11_readmem)
  { 0x0000, 0x1fff, MRA_RAM},
  { 0x2100, 0x2103, pia_r(S11_PIA0) },
  { 0x2400, 0x2403, pia_r(S11_PIA1) },
  { 0x2800, 0x2803, pia_r(S11_PIA2) },
  { 0x2c00, 0x2c03, pia_r(S11_PIA3) },
  { 0x3000, 0x3003, pia_r(S11_PIA4) },
  { 0x3400, 0x3403, pia_r(S11_PIA5) },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(s11_writemem)
  { 0x0000, 0x1fff, MWA_RAM }, /* CMOS */
  { 0x2100, 0x2103, pia_w(S11_PIA0) },
  { 0x2200, 0x2200, latch2200},
  { 0x2400, 0x2403, pia_w(S11_PIA1) },
  { 0x2800, 0x2803, pia_w(S11_PIA2) },
  { 0x2c00, 0x2c03, pia_w(S11_PIA3) },
  { 0x3000, 0x3003, pia_w(S11_PIA4) },
  { 0x3400, 0x3403, pia_w(S11_PIA5) },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
const struct MachineDriver machine_driver_s9_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  },S9_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S9_SOUND },
  s11_nvram
};

const struct MachineDriver machine_driver_s11_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, S11C_SOUNDCPU, S11_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11_SOUND },
  s11_nvram
};

const struct MachineDriver machine_driver_s11b2_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, S11_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11B3_SOUND },
  s11_nvram
};

const struct MachineDriver machine_driver_s11c_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, S11C_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11C_SOUND },
  s11_nvram
};

const struct MachineDriver machine_driver_deas1_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, DE1S_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { DE1S_SOUND },
  s11_nvram
};
const struct MachineDriver machine_driver_dedmd16s1_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, DE1S_SOUNDCPU, DE_DMD16CPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  DE_DMD16VIDEO,
  0,0,0,0, { DE1S_SOUND },
  s11_nvram
};
const struct MachineDriver machine_driver_dedmd16s2a_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD16CPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  DE_DMD16VIDEO,
  0,0,0,0, { DE2S_SOUNDA },
  s11_nvram
};
const struct MachineDriver machine_driver_dedmd32s2a_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD32CPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  DE_DMD32VIDEO,
  0,0,0,0, { DE2S_SOUNDA },
  s11_nvram
};
const struct MachineDriver machine_driver_dedmd64s2a_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }, DE2S_SOUNDCPU, DE_DMD64CPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, s11_exit,
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  DE_DMD64VIDEO,
  0,0,0,0, { DE2S_SOUNDA },
  s11_nvram
};
/* No Sound machine */
const struct MachineDriver machine_driver_s11 = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  }},
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, CORE_EXITFUNC(s11_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  s11_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void s11_nvram(void *file, int write) {
  core_nvram(file, write, memory_region(S11_CPUREGION), 0x0800, 0xff);
}
