#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "snd_cmd.h"
#include "s11csoun.h"
#include "s11.h"

#define S9_SOUND \
  { SOUND_DAC,     &s9s_dacInt }, \
  { SOUND_HC55516, &s9s_hc55516Int }, \
  { SOUND_SAMPLES, &samples_interface}

#define S11B3_SOUND   S9_SOUND

#define S11_VBLANKFREQ    60 /* VBLANK frequency */

#define S11_IRQFREQ     1000

struct DACinterface s11s_dacInt = { 2, { 50,50 }},
                    s9s_dacInt  = { 1, { 50 }};

struct hc55516_interface s11s_hc55516Int = { 2, { 80,80 }},
                         s9s_hc55516Int  = { 1, { 80 }};

static void s11_exit(void);
static void s11_nvram(void *file, int write);

core_tLCDLayout s11_dispS9[] = {
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_CREDIT(16,24,CORE_SEG7S),DISP_SEG_BALLS(0,8,CORE_SEG7S),{0}
};
core_tLCDLayout s11_dispS11[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_CREDIT(16,24,CORE_SEG7S),DISP_SEG_BALLS(0,8,CORE_SEG7SH),{0}
};
core_tLCDLayout s11_dispS11a[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),{0}
};
core_tLCDLayout s11_dispS11b_2[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

/*----------------
/  Local varibles
/-----------------*/
static struct {
  int    vblankCount;
  int    initDone;
  int    sCPUNo; /* CPU number for sound CPU on CPU board */
  UINT32 solenoids;
  core_tSeg segments, pseg;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn; /* Special solenoids and flippers enabled ? */
  int    sndCmd, extSol; /* external sound board cmd */
  int    mainIrq;
} s11locals;

static WRITE_HANDLER(s11s_bankSelect) {
  cpu_setbank(1,memory_region(S11_MEMREG_SROM2)+0x8000+((data&0x01)<<14));
  cpu_setbank(2,memory_region(S11_MEMREG_SROM2)+0x0000+((data&0x02)<<13));
}
static WRITE_HANDLER(s11b3s_bankSelect) {
  cpu_setbank(1,memory_region(S11_MEMREG_SROM1)+0x8000+((data&0x01)<<14));
  cpu_setbank(2,memory_region(S11_MEMREG_SROM1)+0x0000+((data&0x02)<<13));
}
static void s11_piaMainIrq(int state) {
  s11locals.mainIrq = state;
  cpu_set_irq_line(0, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static int s11_irq(void) {
  if (s11locals.mainIrq == 0) /* Don't send IRQ if already active */
    cpu_set_irq_line(S11_CPUNO, M6808_IRQ_LINE, HOLD_LINE);
  return 0;
}
static void s11_piaSoundIrq(int state) {
  cpu_set_irq_line(s11locals.sCPUNo, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
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
  if (core_gameData->gen & (GEN_S11|GEN_S9))
    s11locals.diagnosticLed |= core_bcd2seg[(data & 0x70)>>4];
  else
    s11locals.diagnosticLed |= (data & 0x10)>>4;
}

static WRITE_HANDLER(pia2b_w) {
  if (core_gameData->gen & GEN_S9) {
    s11locals.segments[0][s11locals.digSel].lo |=
         s11locals.pseg[0][s11locals.digSel].lo = core_bcd2seg[data&0x0f];
    s11locals.segments[1][s11locals.digSel].lo |=
         s11locals.pseg[1][s11locals.digSel].lo = core_bcd2seg[data>>4];
  }
  else if (core_gameData->gen & (GEN_S11B_2|GEN_S11B_2x|GEN_S11B_3|GEN_S11C))
    s11locals.segments[1][s11locals.digSel].hi |=
         s11locals.pseg[1][s11locals.digSel].hi = ~data;
  else
    s11locals.segments[1][s11locals.digSel].lo |=
         s11locals.pseg[1][s11locals.digSel].lo = data;
}
static WRITE_HANDLER(pia5a_w) {
  if (core_gameData->gen & (GEN_S11B_2|GEN_S11B_2x|GEN_S11B_3|GEN_S11C))
    s11locals.segments[1][s11locals.digSel].lo |=
         s11locals.pseg[1][s11locals.digSel].lo = ~data;
}
static WRITE_HANDLER(pia3a_w) {
  if (core_gameData->gen & (GEN_S11B_2|GEN_S11B_2x|GEN_S11B_3|GEN_S11C))
    data = ~data;
  s11locals.segments[0][s11locals.digSel].lo |=
       s11locals.pseg[0][s11locals.digSel].lo = data;
}
static WRITE_HANDLER(pia3b_w) {
  if (core_gameData->gen & (GEN_S11B_2|GEN_S11B_2x|GEN_S11B_3|GEN_S11C))
    data = ~data;
  s11locals.segments[0][s11locals.digSel].hi |=
       s11locals.pseg[0][s11locals.digSel].hi = data;
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

/*--------------------
/  Diagnostic buttons
/---------------------*/
static READ_HANDLER (pia2ca1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSwSeq(S11_SWADVANCE) : 0; }
static READ_HANDLER (pia2cb1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSwSeq(S11_SWUPDN)    : 0; }

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
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

static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 5); }
static WRITE_HANDLER(pia1cb2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia3ca2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 2); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 3); }

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { s11locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(s11locals.swCol); }

/*-------
/  Sound
/--------*/
/*-- CPU board sound command --*/
static WRITE_HANDLER(pia0a_w) { snd_cmd_log(0); snd_cmd_log(data); soundlatch_w(0,data); }
/*-- Sound board sound command--*/
static WRITE_HANDLER(pia5b_w) {
  s11locals.sndCmd = data; snd_cmd_log(1); snd_cmd_log(data); soundlatch2_w(0,data);
}

/*-- CPU board sound command available --*/
static void pia0ca2_sync(int data) { pia_set_input_ca1(6, data); }
static WRITE_HANDLER(pia0ca2_w) {
  if (core_gameData->gen & ~GEN_S11C) timer_set(TIME_NOW,data,pia0ca2_sync);
}
/*-- Sound board sound command available --*/
static void pia5cb2_sync(int data) {  pia_set_input_cb1(7, data); }
static WRITE_HANDLER(pia5cb2_w) {
  /* don't pass to sound board if a sound overlay board is available */
  //printf("sndcmd:%2x avail=%d\n",s11locals.sndCmd,data);
  if ((core_gameData->gen & GEN_S11B_2x) &&
      ((s11locals.sndCmd & 0xe0) == 0)) {
    if (!data) s11locals.extSol = (~s11locals.sndCmd) & 0x1f;
  }
  else if (core_gameData->gen & ~(GEN_S11B_3|GEN_S9)) timer_set(TIME_NOW,data,pia5cb2_sync);
}
/*-- reset sound baord CPU --*/
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
  else if ((core_gameData->gen & (GEN_S9|GEN_S11B_3)) ||
           ((soundSys == 0) && !(core_gameData->gen & GEN_S11C)))
    { soundlatch_w(0,data); pia0ca2_w(0,1); pia0ca2_w(0,0); }
  else
    { soundlatch2_w(0,data); pia5cb2_w(0,1); pia5cb2_w(0,0); }
  soundSys = -1;
}

struct pia6821_interface s11_pia_intf[] = {
{/* PIA 0 (2100) */
 /* PA0 - PA7 Sound Select Outputs (sound latch) */
 /* PB0 - PB7 Solenoid 9-16 (12 is usually for multiplexing) */
  /* CA2       Sound H.S.  */
 /* CB2       Enable Special Solenoids */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
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
 /* in  : A/B,CA/B1,CA/B2 */ pia2a_r, 0, pia2ca1_r, pia2cb1_r, 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 3 (2c00) */
 /* PA0 - PA7 Display Data (h,j,k,m,n,p,r,dot) */
 /* PB0 - PB7 Display Data (a,b,c,d,e,f,g,com) */
 /* CA1       Widget I/O LCA1 */
 /* CB1       Widget I/O LCB1 */
 /* CA2       (I) B SST2 */
 /* CB2       (I) C SST3 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
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
 /* PA0 - PA7 Display Data' (h,j,k ,m,n,p,r,dot) */
 /* PB0 - PB7 Widget I/O MD0-MD7 */
 /* CA1       Widget I/O MCA1 */
 /* CB1       Widget I/O MCB1 */
 /* CA2       Widget I/O MCA2 */
 /* CB2       Widget I/O MCB2 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, soundlatch3_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 6 (sound 2000) S11 */
 /* PA0 - PA7 (I) Sound Select Input (soundlatch) */
 /* PB0 - PB7 DAC */
 /* CA1       (I) Sound H.S */
 /* CB1       (I) 1ms */
 /* CA2       55516 Clk */
 /* CB2       55516 Dig */
 /* in  : A/B,CA/B1,CA/B2 */ soundlatch_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, DAC_1_data_w, hc55516_1_clock_w, hc55516_1_digit_w,
 /* irq : A/B             */ s11_piaSoundIrq, s11_piaSoundIrq
},{ /* PIA 6 (sound 2000) S9 */
 /* PA0 - PA7 (I) Sound Select Input (soundlatch) */
 /* PB0 - PB7 DAC */
 /* CA1       (I) Sound H.S */
 /* CB1       (I) 1ms */
 /* CA2       55516 Clk */
 /* CB2       55516 Dig */
 /* in  : A/B,CA/B1,CA/B2 */ soundlatch_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, DAC_0_data_w, hc55516_0_clock_w, hc55516_0_digit_w,
 /* irq : A/B             */ s11_piaSoundIrq, s11_piaSoundIrq
}};

static void s11_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S11_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S11_COMINPORT];
  }
  /*-- Generate interupts for diganostic keys --*/
  if (core_getSwSeq(S11_SWCPUDIAG))   cpu_set_nmi_line(S11_CPUNO, PULSE_LINE);
  if (core_getSwSeq(S11_SWSOUNDDIAG)) cpu_set_nmi_line(s11locals.sCPUNo, PULSE_LINE);
  if ((core_gameData->gen & (GEN_S11B_2|GEN_S11B_2x|GEN_S11B_3|GEN_S11C)) && core_gameData->sxx.muxSol)
    core_setSwSeq(2, core_getSol(core_gameData->sxx.muxSol));
  pia_set_input_ca1(2, core_getSwSeq(S11_SWADVANCE));
  pia_set_input_cb1(2, core_getSwSeq(S11_SWUPDN));
}

static core_tData s11Data = {
  1, /* 1 DIP (actually a jumper) */
  s11_updSw, CORE_DIAG7SEG, s11_sndCmd_w, "s11"
};
static core_tData s11AData = {
  1, /* 1 DIP (actually a jumper) */
  s11_updSw, 1, s11_sndCmd_w, "s11"
};

static void s11_init(void) {
  int ii;

  if (s11locals.initDone) CORE_DOEXIT(s11_exit);
  s11locals.initDone = TRUE;

  if (core_gameData == NULL)  return;

  for (ii = 0; ii < 6; ii++)
    pia_config(ii, PIA_STANDARD_ORDERING, &s11_pia_intf[ii]);

  if      (core_gameData->gen & GEN_S9) {
    if (core_init(&s11Data)) return;
    s11locals.sCPUNo = S11_SCPU1NO;
    pia_config(6, PIA_STANDARD_ORDERING, &s11_pia_intf[7]);
  }
  else if (core_gameData->gen & GEN_S11) {
    if (core_init(&s11Data)) return;
    s11locals.sCPUNo = S11_SCPU2NO;
    pia_config(6, PIA_STANDARD_ORDERING, &s11_pia_intf[6]);
    cpu_setbank(1,memory_region(S11_MEMREG_SROM2)+0xc000);
    cpu_setbank(2,memory_region(S11_MEMREG_SROM2)+0x4000);
    s11cs_init();
  }
  else if (core_gameData->gen & GEN_S11B_3) {
      if (core_init(&s11AData)) return;
    s11locals.sCPUNo = S11_SCPU1NO;
    pia_config(6, PIA_STANDARD_ORDERING, &s11_pia_intf[7]);
    cpu_setbank(1,memory_region(S11_MEMREG_SROM1)+0xc000);
    cpu_setbank(2,memory_region(S11_MEMREG_SROM1)+0x4000);
  }
  else if (core_gameData->gen & GEN_S11C) {
    if (core_init(&s11AData)) return;
    s11locals.sCPUNo = 0; /* Not applicable */
    s11cs_init();
  }
  else { /* all other S11 */
    if (core_init(&s11AData)) return;
    s11locals.sCPUNo = S11_SCPU2NO;
    pia_config(6, PIA_STANDARD_ORDERING, &s11_pia_intf[6]);
    cpu_setbank(1,memory_region(S11_MEMREG_SROM2)+0xc000);
    cpu_setbank(2,memory_region(S11_MEMREG_SROM2)+0x4000);
    s11cs_init();
  }
  pia_reset();
}

static void s11_exit(void) {
  if ((core_gameData->gen & (GEN_S9 | GEN_S11B_3)) == 0) s11cs_exit();
  core_exit();
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(s11_readmem)
  { 0x0000, 0x07ff, MRA_RAM},
  { 0x2100, 0x2103, pia_0_r},
  { 0x2400, 0x2403, pia_1_r},
  { 0x2800, 0x2803, pia_2_r},
  { 0x2c00, 0x2c03, pia_3_r},
  { 0x3000, 0x3003, pia_4_r},
  { 0x3400, 0x3403, pia_5_r},
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(s11_writemem)
  { 0x0000, 0x07ff, MWA_RAM }, /* CMOS */
  { 0x2100, 0x2103, pia_0_w},
  { 0x2200, 0x2200, latch2200},
  { 0x2400, 0x2403, pia_1_w},
  { 0x2800, 0x2803, pia_2_w},
  { 0x2c00, 0x2c03, pia_3_w},
  { 0x3000, 0x3003, pia_4_w},
  { 0x3400, 0x3403, pia_5_w},
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*---------------------------
/  Memory map for sound CPU
/----------------------------*/
static MEMORY_READ_START(s11s_readmem)
  { 0x0000, 0x0fff, MRA_RAM},
  { 0x2000, 0x2003, pia_6_r},
  { 0x8000, 0xbfff, MRA_BANK1}, /* U22 */
  { 0xc000, 0xffff, MRA_BANK2}, /* U21 */
MEMORY_END
#define s11b3s_readmem s11s_readmem
static MEMORY_WRITE_START(s11s_writemem)
  { 0x0000, 0x0fff, MWA_RAM },
  { 0x1000, 0x1000, s11s_bankSelect},
  { 0x2000, 0x2003, pia_6_w},
MEMORY_END

static MEMORY_WRITE_START(s11b3s_writemem)
  { 0x0000, 0x0fff, MWA_RAM },
  { 0x1000, 0x1000, s11b3s_bankSelect},
  { 0x2000, 0x2003, pia_6_w},
MEMORY_END

static MEMORY_READ_START(s9s_readmem)
  { 0x0000, 0x0fff, MRA_RAM},
  { 0x2000, 0x2003, pia_6_r},
  { 0x8000, 0xffff, MRA_ROM}, /* U22 */
MEMORY_END

static MEMORY_WRITE_START(s9s_writemem)
  { 0x0000, 0x0fff, MWA_RAM },
  { 0x2000, 0x2003, pia_6_w},
  { 0x8000, 0xffff, MWA_ROM}, /* U22 */
MEMORY_END

/*-----------------
/  Machine drivers
/------------------*/
struct MachineDriver machine_driver_s9_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  },{ CPU_M6808 | CPU_AUDIO_CPU, 1000000, /* 1 Mhz */
      s9s_readmem, s9s_writemem, NULL, NULL,
      NULL, 0, NULL, 0
  }},
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, CORE_EXITFUNC(s11_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S9_SOUND },
  s11_nvram
};

struct MachineDriver machine_driver_s11a_2_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  } S11C_SOUNDCPU,
  {   CPU_M6808 | CPU_AUDIO_CPU, 1000000, /* 1 Mhz */
      s11s_readmem, s11s_writemem, NULL, NULL,
      NULL, 0, NULL, 0
  }},
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, CORE_EXITFUNC(s11_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11_SOUND },
  s11_nvram
};

struct MachineDriver machine_driver_s11b_3_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  },{ CPU_M6808 | CPU_AUDIO_CPU, 1000000, /* 1 Mhz */
      s11b3s_readmem, s11b3s_writemem, NULL, NULL,
      NULL, 0, NULL, 0
  }},
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, CORE_EXITFUNC(s11_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11B3_SOUND },
  s11_nvram
};

struct MachineDriver machine_driver_s11c_s = {
  {{  CPU_M6808, 1000000, /* 1 Mhz */
      s11_readmem, s11_writemem, NULL, NULL,
      s11_vblank, 1, s11_irq, S11_IRQFREQ
  } S11C_SOUNDCPU },
  S11_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, s11_init, CORE_EXITFUNC(s11_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_SUPPORTS_DIRTY | VIDEO_TYPE_RASTER, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S11C_SOUND },
  s11_nvram
};

struct MachineDriver machine_driver_s11a_2 = {
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
  core_nvram(file, write, memory_region(S11_MEMREG_CPU), 0x0800);
}


