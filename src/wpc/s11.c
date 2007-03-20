/* Williams System 9, 11, and All Data East Hardware */
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

#define FIXMUX  // DataEast Playboy 35th fix

// TODO:
// DE display layouts
#define S11_PIA0 0
#define S11_PIA1 1
#define S11_PIA2 2
#define S11_PIA3 3
#define S11_PIA4 4
#define S11_PIA5 5

#define S11_VBLANKFREQ    60 /* VBLANK frequency */

#define S11_IRQFREQ     1000

#define S11_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define S11_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S11_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

static MACHINE_STOP(s11);
static NVRAM_HANDLER(s11);
static NVRAM_HANDLER(de);

const struct core_dispLayout s11_dispS9[] = {
  {4, 0, 1,7, CORE_SEG87}, {4,16, 9,7, CORE_SEG87},
  {0, 0,21,7, CORE_SEG87}, {0,16,29,7, CORE_SEG87},
  DISP_SEG_CREDIT(0,8,CORE_SEG7S),DISP_SEG_BALLS(20,28,CORE_SEG7S),{0}
};
const struct core_dispLayout s11_dispS11[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG8), DISP_SEG_7(1,1,CORE_SEG8),
  {2,8,0,1,CORE_SEG7S},{2,10,8,1,CORE_SEG7S}, {2,2,20,1,CORE_SEG7S},{2,4,28,1,CORE_SEG7S}, {0}
};
const struct core_dispLayout s11_dispS11a[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG8), DISP_SEG_7(1,1,CORE_SEG8) ,{0}
};
const struct core_dispLayout s11_dispS11b2[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids, solsmooth[S11_SOLSMOOTH];
  UINT32 extSol, extSolPulse;
  core_tSeg segments, pseg;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn;				/* Special solenoids and flippers enabled ? */
  int    sndCmd;	/* external sound board cmd */
  int    piaIrq;
  int	 deGame;	/*Flag to see if it's a Data East game running*/
#ifdef FIXMUX
  UINT8  solBits1,solBits2;
  UINT8  solBits2prv;
#endif
} locals;

static void s11_irqline(int state) {
  if (state) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, ASSERT_LINE);
	/*Set coin door inputs, differs between S11 & DE*/
    if (locals.deGame) {
      pia_set_input_ca1(S11_PIA2, !core_getSw(DE_SWADVANCE));
      pia_set_input_cb1(S11_PIA2, core_getSw(DE_SWUPDN));
    }
    else {
      pia_set_input_ca1(S11_PIA2, core_getSw(S11_SWADVANCE));
      pia_set_input_cb1(S11_PIA2, core_getSw(S11_SWUPDN));
    }
  }
  else if (!locals.piaIrq) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(S11_PIA2, locals.deGame);
    pia_set_input_cb1(S11_PIA2, locals.deGame);
  }
}

static void s11_piaMainIrq(int state) {
  s11_irqline(locals.piaIrq = state);
}

static INTERRUPT_GEN(s11_irq) {
  s11_irqline(1); timer_set(TIME_IN_CYCLES(32,0),0,s11_irqline);
}

static INTERRUPT_GEN(s11_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;
  /*-- lamps --*/
  if ((locals.vblankCount % S11_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if (locals.ssEn) { // set gameon and special solenoids
    int ii;
    locals.solenoids |= CORE_SOLBIT(S11_GAMEONSOL);
    /*-- special solenoids updated based on switches --*/
    for (ii = 0; ii < 6; ii++) {
      if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
        locals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
  }
#ifdef FIXMUX
// mux translation moved
#else
  if ((core_gameData->sxx.muxSol) &&
      (locals.solenoids & CORE_SOLBIT(core_gameData->sxx.muxSol))) {
    if (core_gameData->hw.gameSpecific1 & S11_RKMUX)
      locals.solenoids = (locals.solenoids & 0x00ff8fef) |
                         ((locals.solenoids & 0x00000010)<<20) |
                         ((locals.solenoids & 0x00007000)<<13);
    else
      locals.solenoids = (locals.solenoids & 0x00ffff00) | (locals.solenoids<<24);
  }
#endif
  locals.solsmooth[locals.vblankCount % S11_SOLSMOOTH] = locals.solenoids;
#if S11_SOLSMOOTH != 2
#  error "Need to update smooth formula"
#endif
  coreGlobals.solenoids  = locals.solsmooth[0] | locals.solsmooth[1];
  coreGlobals.solenoids2 = locals.extSol << 8;
  locals.solenoids = coreGlobals.pulsedSolState;
  locals.extSol = locals.extSolPulse;
  /*-- display --*/
  if ((locals.vblankCount % S11_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(locals.ssEn);
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia1a_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, locals.lampColumn, locals.lampRow = ~data);
}
static WRITE_HANDLER(pia1b_w) {
  core_setLamp(coreGlobals.tmpLampMatrix, locals.lampColumn = data, locals.lampRow);
}

/*-- Jumper W7 --*/
static READ_HANDLER (pia2a_r) { return core_getDip(0)<<7; }

/*-----------------
/ Display handling
/-----------------*/
/*NOTE: Data East DMD Games:     data = 0x01, CN1-Pin 7 (Strobe) goes low
				 data = 0x04, CN2-Pin 1 (Enable) goes low
			(currently we don't need to read these values)*/
static WRITE_HANDLER(pia2a_w) {
  locals.digSel = data & 0x0f;
  if (core_gameData->hw.display & S11_BCDDIAG)
    locals.diagnosticLed |= core_bcd2seg[(data & 0x70)>>4];
  else
    locals.diagnosticLed |= (data & 0x10)>>4;
}

static WRITE_HANDLER(pia2b_w) {
  /* Data East writes auxiliary solenoids here for DMD games
     CN3 Printer Data Lines (Used by various games)
       data = 0x01, CN3-Pin 9 (GNR Magnet 3, inverted for Star Trek 25th chase lights)
       data = 0x02, CN3-Pin 8 (GNR Magnet 2, -"-)
       data = 0x04, CN3-Pin 7 (GNR Magnet 1, -"-)
       ....
       data = 0x80, CN3-Pin 1 (Blinder on Tommy)*/
  if (core_gameData->gen & GEN_DEDMD16) {
    if (core_gameData->hw.gameSpecific1 & S11_PRINTERLINE) locals.extSol |= locals.extSolPulse = (data ^ 0xff);
  } else if (core_gameData->gen & (GEN_DEDMD32|GEN_DEDMD64)) {
    if (core_gameData->hw.gameSpecific1 & S11_PRINTERLINE) locals.extSol |= locals.extSolPulse = data;
  }
  else {
    if (core_gameData->hw.display & S11_DISPINV) data = ~data;
    if (core_gameData->hw.display & S11_BCDDISP) {
      locals.segments[locals.digSel].w |=
           locals.pseg[locals.digSel].w = core_bcd2seg[data&0x0f];
      locals.segments[20+locals.digSel].w |=
           locals.pseg[20+locals.digSel].w = core_bcd2seg[data>>4];
    }
    else
      locals.segments[20+locals.digSel].b.lo |=
          locals.pseg[20+locals.digSel].b.lo = data;
  }
}
static WRITE_HANDLER(pia5a_w) { // Not used for DMD
  if (core_gameData->hw.display & S11_DISPINV) data = ~data;
  if (core_gameData->hw.display & S11_LOWALPHA)
    locals.segments[20+locals.digSel].b.hi |=
         locals.pseg[20+locals.digSel].b.hi = data;
}
static WRITE_HANDLER(pia3a_w) {
  if (core_gameData->hw.display & S11_DISPINV) data = ~data;
  locals.segments[locals.digSel].b.hi |= locals.pseg[locals.digSel].b.hi = data;
}
static WRITE_HANDLER(pia3b_w) {
  if (core_gameData->hw.display & S11_DISPINV) data = ~data;
  locals.segments[locals.digSel].b.lo |= locals.pseg[locals.digSel].b.lo = data;
}

static READ_HANDLER(pia3b_dmd_r) {
  if (core_gameData->gen & GEN_DEDMD32)
    return (sndbrd_0_data_r(0) ? 0x80 : 0x00) | (sndbrd_0_ctrl_r(0)<<3);
  else if (core_gameData->gen & GEN_DEDMD64)
    return sndbrd_0_data_r(0) ? 0x80 : 0x00;
  return 0;
}

//NOTE: Unusued in Data East Alpha Games
static WRITE_HANDLER(pia2ca2_w) {
  data = data ? 0x80 : 0x00;
  if (core_gameData->gen & GEN_S9) {
    locals.segments[1+locals.digSel].b.lo |= data;
    locals.pseg[1+locals.digSel].b.lo = (locals.pseg[1+locals.digSel].b.lo & 0x7f) | data;
  } else {
    locals.segments[20+locals.digSel].b.lo |= data;
    locals.pseg[20+locals.digSel].b.lo = (locals.pseg[20+locals.digSel].b.lo & 0x7f) | data;
  }
}
//NOTE: Pin 10 of CN3 for Data East DMD Games (Currently we don't need to read this value)
static WRITE_HANDLER(pia2cb2_w) {
  data = data ? 0x80 : 0x00;
  if (core_gameData->gen & GEN_S9) {
    locals.segments[21+locals.digSel].b.lo |= data;
    locals.pseg[21+locals.digSel].b.lo = (locals.pseg[21+locals.digSel].b.lo & 0x7f) | data;
  } else {
    locals.segments[locals.digSel].b.lo |= data;
    locals.pseg[locals.digSel].b.lo = (locals.pseg[locals.digSel].b.lo & 0x7f) | data;
  }
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
  static const int ssSolNo[2][6] = {{5,4,1,2,0,3},{3,4,5,1,0,2}};
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + ssSolNo[locals.deGame][solNo]);
  if (locals.ssEn & (~data & 1))
    { coreGlobals.pulsedSolState |= bit;  locals.solenoids |= bit; }
  else
    coreGlobals.pulsedSolState &= ~bit;
}

#ifdef FIXMUX

static void updsol(void) {
  /* set new solenoids, preserve SSSol */
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00ff0000)
                             | (locals.solBits2 << 8)
                             | (locals.solBits1     );

  /* if game has a MUX and it's active... */
  if ((core_gameData->sxx.muxSol) &&
      (coreGlobals.pulsedSolState & CORE_SOLBIT(core_gameData->sxx.muxSol))) {
    if (core_gameData->hw.gameSpecific1 & S11_RKMUX) /* special case WMS Road Kings */
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00ff8fef)      |
                                  ((coreGlobals.pulsedSolState & 0x00000010)<<20) |
                                  ((coreGlobals.pulsedSolState & 0x00007000)<<13);
    else
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00ffff00) |
                                   (coreGlobals.pulsedSolState << 24);
  }

  locals.solenoids |= coreGlobals.pulsedSolState;
}

static WRITE_HANDLER(pia0b_w) {
  // DataEast Playboy35th needs the MUX delayed my one IRQ:
  if (core_gameData->hw.gameSpecific1 & S11_MUXDELAY) {
    // new solbits are stored, previous solbits are processed
    UINT8 h            = locals.solBits2prv;
    locals.solBits2prv = data;
    data               = h;
  }
  if (data != locals.solBits2) {
    locals.solBits2 = data;
    updsol();
  }
}

static WRITE_HANDLER(latch2200) {
  if (data != locals.solBits1) {
    locals.solBits1 = data;
    updsol();
  }
}

#else
static WRITE_HANDLER(pia0b_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
  locals.solenoids |= (data<<8);
}
static WRITE_HANDLER(latch2200) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  locals.solenoids |= data;
}
#endif

static WRITE_HANDLER(pia0cb2_w) { locals.ssEn = !data;}

static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 0); }
static WRITE_HANDLER(pia1cb2_w) { setSSSol(data, 1); }
static WRITE_HANDLER(pia3ca2_w) { setSSSol(data, 2); }
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 3); }
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 4); }
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 5); }

/*---------------
/ Switch reading
/----------------*/
static WRITE_HANDLER(pia4b_w) { locals.swCol = data; }
static READ_HANDLER(pia4a_r)  { return core_getSwCol(locals.swCol); }

/*-------
/  Sound
/--------*/
/*-- Sound board sound command--*/
static WRITE_HANDLER(pia5b_w) {
  //Data East 128x16 games need to eat the 0xfe command (especially Hook)
  if ((core_gameData->gen & GEN_DEDMD16) && (data == 0xfe)) return;
  locals.sndCmd = data; sndbrd_1_data_w(0,data);
}

/*-- Sound board sound command available --*/
static WRITE_HANDLER(pia5cb2_w) {
  /* don't pass to sound board if a sound overlay board is available */
  if ((core_gameData->hw.gameSpecific1 & S11_SNDOVERLAY) &&
      ((locals.sndCmd & 0xe0) == 0)) {
    if (!data) locals.extSol |= locals.extSolPulse = (~locals.sndCmd) & 0x1f;
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

static WRITE_HANDLER(de_sndCmd_w) {
    sndbrd_data_w(1, data); sndbrd_ctrl_w(1,1); sndbrd_ctrl_w(1,0);
}

//NOTE: Not used for Data East
static WRITE_HANDLER(pia0ca2_w) { sndbrd_0_ctrl_w(0,data); }
//NOTE: Not used for Data East
static READ_HANDLER(pia5b_r) { return sndbrd_1_ctrl_r(0); }

static struct pia6821_interface s11_pia[] = {
{/* PIA 0 (2100) */
 /* PA0 - PA7 Sound Select Outputs (sound latch) */
 /* PB0 - PB7 Solenoid 9-16 (12 is usually for multiplexing) */
  /* CA2       Sound H.S.  */
 /* CB2       Enable Special Solenoids */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ sndbrd_0_data_w, pia0b_w, pia0ca2_w, pia0cb2_w,
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
 /* PA0 - PA7 Display Data' (h,j,k ,m,n,p,r,dot), DMD status */
 /* PB0 - PB7 Widget I/O MD0-MD7 */
 /* CA1       Widget I/O MCA1 */
 /* CB1       Widget I/O MCB1 */
 /* CA2       Widget I/O MCA2 */
 /* CB2       Widget I/O MCB2 */
 /* in  : A/B,CA/B1,CA/B2 */ pia5a_r, pia5b_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
},{ /* PIA 3 (2c00) for DMD games*/
 /* PA0 - PA7 DMD data */
 /* PB0 - PB7 DMD ctrl */
 /* CA1       NC */
 /* CB1       NC */
 /* CA2       (O) B SST2 */
 /* CB2       (O) C SST3 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, pia3b_dmd_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ sndbrd_0_data_w, sndbrd_0_ctrl_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ s11_piaMainIrq, s11_piaMainIrq
}
};

static SWITCH_UPDATE(s11) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[S11_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[S11_COMINPORT];
  }
  /*-- Generate interupts for diganostic keys --*/
  cpu_set_nmi_line(0, core_getSw(S11_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_0_diag(core_getSw(S11_SWSOUNDDIAG));
  if ((core_gameData->hw.gameSpecific1 & S11_MUXSW2) && core_gameData->sxx.muxSol)
    core_setSw(2, core_getSol(core_gameData->sxx.muxSol));
  if (locals.deGame) {
    /* Show Status of Black Advance Switch */
    core_textOutf(40,20,BLACK,core_getSw(DE_SWADVANCE) ? "B-Down" : "B-Up  ");
    /* Show Status of Green Up/Down Switch */
    core_textOutf(40,30,BLACK,core_getSw(DE_SWUPDN)    ? "G-Down" : "G-Up  ");
  }
  else {
    /* Show Status of Advance Switch */
    core_textOutf(40,20,BLACK,core_getSw(S11_SWADVANCE) ? "A-Down" : "A-Up  ");
    /* Show Status of Green Up/Down Switch */
    core_textOutf(40,30,BLACK,core_getSw(S11_SWUPDN)    ? "G-Down" : "G-Up  ");
  }
}

// convert lamp and switch numbers
// S11 is 1-64
// convert to 0-64 (+8)
// i.e. 1=8, 2=9...
static int s11_sw2m(int no) { return no+7; }
static int s11_m2sw(int col, int row) { return col*8+row-7; }

static MACHINE_INIT(s11) {
  if (core_gameData->gen & (GEN_DE | GEN_DEDMD16 | GEN_DEDMD32 | GEN_DEDMD64))
    locals.deGame = 1;
  pia_config(S11_PIA0, PIA_STANDARD_ORDERING, &s11_pia[0]);
  pia_config(S11_PIA1, PIA_STANDARD_ORDERING, &s11_pia[1]);
  pia_config(S11_PIA2, PIA_STANDARD_ORDERING, &s11_pia[2]);
  pia_config(S11_PIA3, PIA_STANDARD_ORDERING, &s11_pia[3]);
  pia_config(S11_PIA4, PIA_STANDARD_ORDERING, &s11_pia[4]);
  pia_config(S11_PIA5, PIA_STANDARD_ORDERING, &s11_pia[5]);

  /*Additional hardware dependent init code*/
  switch (core_gameData->gen) {
    case GEN_S9:
      sndbrd_0_init(SNDBRD_S9S, 1, NULL, NULL, NULL);
      break;
    case GEN_S11:
      sndbrd_0_init(SNDBRD_S11S,  1, memory_region(S11S_ROMREGION), NULL, NULL);
      break;
    case GEN_S11X:
      sndbrd_0_init(SNDBRD_S11XS, 2, memory_region(S11XS_ROMREGION), NULL, NULL);
      sndbrd_1_init(SNDBRD_S11CS, 1, memory_region(S11CS_ROMREGION), pia_5_cb1_w, NULL);
      break;
    case GEN_S11B2:
      sndbrd_0_init(SNDBRD_S11BS, 2, memory_region(S11XS_ROMREGION), NULL, NULL);
      sndbrd_1_init(SNDBRD_S11JS, 1, memory_region(S11JS_ROMREGION), pia_5_cb1_w, NULL);
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
      pia_config(S11_PIA3, PIA_STANDARD_ORDERING, &s11_pia[6]); // PIA 3 is different for DMD games
      sndbrd_0_init(core_gameData->hw.display,    2, memory_region(DE_DMD16ROMREGION),NULL,NULL);
      sndbrd_1_init(core_gameData->hw.soundBoard, 1, memory_region(DE1S_ROMREGION), pia_5_cb1_w, NULL);
      break;
  }
}
static MACHINE_INIT(s9pf) {
  pia_config(S11_PIA0, PIA_STANDARD_ORDERING, &s11_pia[0]);
  pia_config(S11_PIA1, PIA_STANDARD_ORDERING, &s11_pia[1]);
  pia_config(S11_PIA2, PIA_STANDARD_ORDERING, &s11_pia[2]);
  pia_config(S11_PIA3, PIA_STANDARD_ORDERING, &s11_pia[3]);
  pia_config(S11_PIA4, PIA_STANDARD_ORDERING, &s11_pia[4]);
  pia_config(S11_PIA5, PIA_STANDARD_ORDERING, &s11_pia[5]);
  sndbrd_0_init(SNDBRD_S9S, 1, NULL, NULL, NULL);
}
static MACHINE_RESET(s11) {
  pia_reset();
}
static MACHINE_STOP(s11) {
  sndbrd_0_exit(); sndbrd_1_exit();
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
static MACHINE_DRIVER_START(s11)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(s11,s11,s11)
  MDRV_CPU_ADD(M6808, 1000000)
  MDRV_CPU_MEMORY(s11_readmem, s11_writemem)
  MDRV_CPU_VBLANK_INT(s11_vblank, 1)
  MDRV_CPU_PERIODIC_INT(s11_irq, S11_IRQFREQ)
  MDRV_DIPS(1) /* (actually a jumper) */
  MDRV_SWITCH_UPDATE(s11)
MACHINE_DRIVER_END

/* System 9 */
MACHINE_DRIVER_START(s11_s9S)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s9s)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LED7
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* System 11 with S11C sound board, diagnostic digit */
MACHINE_DRIVER_START(s11_s11XS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s11xs)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LED7
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* Pennant Fever */
MACHINE_DRIVER_START(s11_s9PS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s9ps)
  MDRV_CORE_INIT_RESET_STOP(s9pf,s11,s11)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LED7
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* System 11 with S11C sound board, diagnostic LED only */
MACHINE_DRIVER_START(s11_s11XSL)
  MDRV_IMPORT_FROM(s11_s11XS)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

/* System 11a without external sound board*/
MACHINE_DRIVER_START(s11_s11S)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s11s)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* System 11a with S11C sound board */
MACHINE_DRIVER_START(s11_s11aS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s11xs)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* System 11B with Jokerz! sound board*/
MACHINE_DRIVER_START(s11_s11b2S)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s11b2s)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* System 11C */
MACHINE_DRIVER_START(s11_s11cS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(wmssnd_s11cs)
  MDRV_NVRAM_HANDLER(s11)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(s11_sndCmd_w)
  MDRV_SOUND_CMDHEADING("s11")
MACHINE_DRIVER_END

/* DE alpa numeric No Sound */
MACHINE_DRIVER_START(de_a)
  MDRV_IMPORT_FROM(s11)
  MDRV_NVRAM_HANDLER(de)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

/* DE alphanumeric Sound 1 */
MACHINE_DRIVER_START(de_a1S)
  MDRV_IMPORT_FROM(de_a)
  MDRV_IMPORT_FROM(de1s)
  MDRV_SOUND_CMD(de_sndCmd_w)
  MDRV_SOUND_CMDHEADING("DE")
MACHINE_DRIVER_END

/* DE 128x16 Sound 1 */
MACHINE_DRIVER_START(de_dmd161S)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(de1s)
  MDRV_IMPORT_FROM(de_dmd16)
  MDRV_NVRAM_HANDLER(de)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(de_sndCmd_w)
  MDRV_SOUND_CMDHEADING("DE")
MACHINE_DRIVER_END

/* DE 128x16 Sound 2a */
MACHINE_DRIVER_START(de_dmd162aS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(de2aas)
  MDRV_IMPORT_FROM(de_dmd16)
  MDRV_NVRAM_HANDLER(de)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(de_sndCmd_w)
  MDRV_SOUND_CMDHEADING("DE")
MACHINE_DRIVER_END

/* DE 128x32 Sound 2a */
MACHINE_DRIVER_START(de_dmd322aS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(de2aas)
  MDRV_IMPORT_FROM(de_dmd32)
  MDRV_NVRAM_HANDLER(de)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(de_sndCmd_w)
  MDRV_SOUND_CMDHEADING("DE")
MACHINE_DRIVER_END

/* DE 192x64 Sound 2a */
MACHINE_DRIVER_START(de_dmd642aS)
  MDRV_IMPORT_FROM(s11)
  MDRV_IMPORT_FROM(de2aas)
  MDRV_IMPORT_FROM(de_dmd64)
  MDRV_NVRAM_HANDLER(de)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(de_sndCmd_w)
  MDRV_SOUND_CMDHEADING("DE")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(s11) {
  core_nvram(file, read_or_write, memory_region(S11_CPUREGION), 0x0800, 0xff);
}
static NVRAM_HANDLER(de) {
  core_nvram(file, read_or_write, memory_region(S11_CPUREGION), 0x2000, 0xff);
}
