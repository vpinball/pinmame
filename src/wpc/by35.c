// license:BSD-3-Clause

#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "stsnd.h"
#include "hnks.h"
#include "by35.h"

#if defined(PINMAME) && defined(LISY_SUPPORT)
 #include "lisy/lisy35.h"
#endif /* PINMAME && LISY_SUPPORT */

#define BY35_DEBUG_KEY_SUPPORT 0

#define BY35_PIA0 0
#define BY35_PIA1 1

#define BY35_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define BY35_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define BY35_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*--------------------------------------------------
/ There are a few variants on the BY35 hardware
/ PIA1:A1  - SOUNDE (BY35), 7th display digit (Stern)
/ PIA1:A0  - credit/ball strobe. Positive edge (BY17,Stern), Negative edge (BY35)
/ PIA1:CA2 - sound control (HNK), lamp strobe 2 (BY17, BY35, stern)
/ 4th DIP bank (not HNK)
/ 9 segment display (HNK)
----------------------------------------------------*/
#define BY35HW_SOUNDE    0x01 // supports 5th sound line (BY35 only)
#define BY35HW_INVDISP4  0x02 // inverted strobe to credit/ball (BY17+stern)
#define BY35HW_DIP4      0x04 // got 4th DIP bank (not HNK)
#define BY35HW_REVSW     0x08 // reversed switches (HNK only)
#define BY35HW_SCTRL     0x20 // uses lamp2 as sound ctrl (HNK)

static char debugms[] = "01234567";
// ok
static READ_HANDLER(snd300_r) {
  logerror("%04x: snd300_r adress A%c data latch %02x give %02x \n", activecpu_get_previouspc(), debugms[offset],snddatst300.ax[offset],snddatst300.axb[offset]);
  snddatst300.axb[2] = snddatst300.timer1 / 256;
  snddatst300.axb[3] = snddatst300.timer1 -  (snddatst300.axb[2] * 256);
  snddatst300.axb[4] = snddatst300.timer2 / 256;
  snddatst300.axb[5] = snddatst300.timer2 -  (snddatst300.axb[4] * 256);
  snddatst300.axb[6] = snddatst300.timer3 / 256;
  snddatst300.axb[7] = snddatst300.timer3 -  (snddatst300.axb[6] * 256);

  return snddatst300.axb[offset];
}

static WRITE_HANDLER(snd300_w) {
  snddatst300.ax[offset]=data;
  sndbrd_0_data_w(0,offset);
}

static WRITE_HANDLER(snd300_wex) {
  sndbrd_0_ctrl_w(0,data);
}

static struct {
  int a0, a1, b0, b1, ca10, ca11, ca20, ca21, cb10, cb11, cb20, cb21;
  int swData;
  int bcd[7], lastbcd;
  const int *bcd2seg;
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int diagnosticLed;
  int vblankCount;
  int hw;
  int irqstates[4];
} locals;

static void piaIrq(int num, int state) {
  static int oldstate;
  int irqstate;
  locals.irqstates[num] = state;
  irqstate = locals.irqstates[0] || locals.irqstates[1] || locals.irqstates[2] || locals.irqstates[3];
  if (oldstate != irqstate) {
//    logerror("IRQ state: %d\n", irqstate);
    cpu_set_irq_line(0, M6800_IRQ_LINE, irqstate ? ASSERT_LINE : CLEAR_LINE);
  }
  oldstate = irqstate;
}

static void piaIrq0(int state) { piaIrq(0, state); }
static void piaIrq1(int state) { piaIrq(1, state); }
static void piaIrq2(int state) { piaIrq(2, state); }
static void piaIrq3(int state) { piaIrq(3, state); }

static void by35_dispStrobe(int mask) {
  int digit = locals.a1 & 0xfe;
  int ii,jj;

  /* This handles O. Kaegi's 7-digit mod wiring */
  if (locals.hw & BY35HW_SOUNDE && !(core_gameData->hw.gameSpecific1 & BY35GD_PHASE))
    digit = (digit & 0xf0) | ((digit & 0x0c) == 0x0c ? 0x02 : (digit & 0x0d));

  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
        {
#ifdef LISY_SUPPORT
          lisy35_display_handler( jj*8+ii, locals.bcd[jj] & 0x0f );
#endif
          locals.segments[jj*8+ii].w |= locals.pseg[jj*8+ii].w = locals.bcd2seg[locals.bcd[jj] & 0x0f];
        }
    }

  /* This handles the fake zero for Nuova Bell games */
  if (core_gameData->hw.gameSpecific1 & BY35GD_FAKEZERO) {
	if (locals.segments[7].w) locals.segments[8].w = locals.pseg[8].w = locals.bcd2seg[0];
	if (locals.segments[15].w) locals.segments[16].w = locals.pseg[16].w = locals.bcd2seg[0];
	if (locals.segments[23].w) locals.segments[24].w = locals.pseg[24].w = locals.bcd2seg[0];
	if (locals.segments[31].w) locals.segments[32].w = locals.pseg[32].w = locals.bcd2seg[0];
  }
}

static void by35_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = (locals.a0>>4)^0x0f;
#ifdef LISY_SUPPORT
    if ( lampdata ) lisy35_lamp_handler( 0, board, lampadr, lampdata);
#endif
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
    int bcdLoad = locals.lastbcd & ~data & 0x0f;
    int ii;

    for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
      if (bcdLoad & 0x01) locals.bcd[ii] = data>>4;
    locals.lastbcd = (locals.lastbcd & 0x10) | (data & 0x0f);
  }
  locals.a0 = data;
  if (core_gameData->hw.gameSpecific1 & BY35GD_PHASE)
    by35_lampStrobe(locals.cb10, locals.lampadr1);
  else {
    by35_lampStrobe(0, locals.lampadr1);
    if (core_gameData->hw.lampCol > 0) by35_lampStrobe(1,locals.lampadr2);
  }
}

/* PIA1:A-W  0,2-7 Display handling */
/*        W  1     Sound E */
static WRITE_HANDLER(pia1a_w) {
  static int counter, pos0, pos1;
  if (locals.hw & BY35HW_SOUNDE)
  {
    sndbrd_0_ctrl_w(0, (locals.cb21 ? 1 : 0) | (data & 0x02));
#ifdef LISY_SUPPORT
    lisy35_sound_handler( LISY35_SOUND_HANDLER_IS_CTRL, (locals.cb21 ? 1 : 0) | (data & 0x02));
#endif
  }

  if (core_gameData->hw.gameSpecific1 & BY35GD_ALPHA) {
    if (data & 0x80) { // 1st alphanumeric display strobe
      if (pos0 < 20)
        locals.segments[pos0++].w = core_ascii2seg16s[locals.a0 & 0x7f] | (locals.a0 & 0x80);
      counter++;
      if (counter > 7) {
        counter = 0;
        pos0 = 0;
        pos1 = 0;
      }
    } else if (data & 0x40) { // 2nd alphanumeric display strobe
      counter = 0;
      if (pos1 < 12)
        locals.segments[20+(pos1++)].w = core_ascii2seg16s[locals.a0 & 0x7f] | (locals.a0 & 0x80);
    }
  } else if (!locals.ca20) {
    if (locals.hw & BY35HW_INVDISP4) {
      if (core_gameData->gen & GEN_BOWLING) {
        if (data & 0x02) {
          locals.bcd[5] = locals.a0>>4;
          by35_dispStrobe(0x20);
        }
        if (data & 0x04) {
          locals.bcd[6] = locals.a0>>4;
          by35_dispStrobe(0x40);
        }
      }
      if (~(locals.lastbcd>>4) & data & 0x01) { // positive edge
        locals.bcd[4] = locals.a0>>4;
        by35_dispStrobe(0x10);
      }
    }
    else if ((locals.lastbcd>>4) & ~data & 0x01) { // negative edge
      locals.bcd[4] = locals.a0>>4;
      by35_dispStrobe(0x10);
    }
    locals.lastbcd = (locals.lastbcd & 0x0f) | ((data & 0x01)<<4);
  }
  locals.a1 = data;
}

/* PIA0:B-R  Get Data depending on PIA0:A */
static READ_HANDLER(pia0b_r) {
#ifndef LISY_SUPPORT
  if (locals.a0 & 0x20) return core_getDip(0); // DIP#1 1-8
  if (locals.a0 & 0x40) return core_getDip(1); // DIP#2 9-16
  if (locals.a0 & 0x80) return core_getDip(2); // DIP#3 17-24
  if ((locals.hw & BY35HW_DIP4) && locals.cb20) return core_getDip(3); // DIP#4 25-32
#else
  int ret;
  if (locals.a0 & 0x20)
   {  ret = lisy35_get_mpudips(0); if (ret<0) return core_getDip(0); else return ret; } // DIP#1 1-8
  if (locals.a0 & 0x40)
   {  ret = lisy35_get_mpudips(1); if (ret<0) return core_getDip(1); else return ret; } // DIP#1 9-16
  if (locals.a0 & 0x80)
   {  ret = lisy35_get_mpudips(2); if (ret<0) return core_getDip(2); else return ret; } // DIP#1 17-24
  if ((locals.hw & BY35HW_DIP4) && locals.cb20)
   {  ret = lisy35_get_mpudips(3); if (ret<0) return core_getDip(3); else return ret; } // DIP#1 25-32
#endif
  {
    int col = locals.a0 & 0x1f;
    UINT8 sw;
    if (core_gameData->hw.gameSpecific1 & BY35GD_SWVECTOR) col |= (locals.b1 & 0x10)<<1;
    else                                                   col |= (locals.b1 & 0x80)>>2;
    if (!col && (core_gameData->hw.gameSpecific1 & BY35GD_MARAUDER)) {
      sw = coreGlobals.swMatrix[3];
    } else {
#ifndef LISY_SUPPORT
      sw = core_getSwCol(col);
#else
      sw = lisy35_switch_handler(col); //get the switches from LISY35
#endif
    }
    return (locals.hw & BY35HW_REVSW) ? core_revbyte(sw) : sw;
  }
}

/* PIA0:CB1-R ZC state */
static READ_HANDLER(pia0cb1_r) {
  return locals.cb10;
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  if (locals.cb20 & ~data) locals.lampadr1 = locals.a0 & 0x0f;
  locals.cb20 = data;
// ok
  if (sb == SNDBRD_ST300V) {
    if (S14001A_bsy_0_r()) {
   	  pia_set_input_cb1(BY35_PIA1,0);
   	} else {
   	  pia_set_input_cb1(BY35_PIA1,data);
   	}
  }
}

/* PIA1:CA2-W Lamp Strobe #2 */
static WRITE_HANDLER(pia1ca2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  if (locals.ca21 & ~data) {
    locals.lampadr2 = locals.a0 & 0x0f;
    if (core_gameData->hw.display & 0x01)
      { locals.bcd[6] = locals.a0>>4; by35_dispStrobe(0x40); }
  }
  if (locals.hw & BY35HW_SCTRL)
  {
    sndbrd_0_ctrl_w(0, data);
    //Note: HNK only, not for LISY at the moment
  }
//  ok
  if ((sb == SNDBRD_ST300V) && (data)) {
    sndbrd_0_diag(1); // gv - switches over to voice board
    sndbrd_0_ctrl_w(0, locals.a0);
  }
  locals.ca21 = locals.diagnosticLed = data;
#ifdef LISY_SUPPORT
  coil_bally_led_set(locals.diagnosticLed);
#endif
}

/* PIA0:CA2-W Display Strobe */
static WRITE_HANDLER(pia0ca2_w) {
  locals.ca20 = data;
  if (data && !(core_gameData->hw.gameSpecific1 & BY35GD_ALPHA)) by35_dispStrobe(0x0f);
}

/* PIA1:B-W Solenoid/Sound output */
static WRITE_HANDLER(pia1b_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
#ifdef LISY_SUPPORT
     //send to solenoid_handler already here with locals.cb21=Soundselect
     //as sometimes continous solenoid data changes with Soundselect==0
     lisy35_solenoid_handler( data , locals.cb21);
#endif
  // check for extra display connected to solenoids
  if (~locals.b1 & data & core_gameData->hw.display & 0xf0)
    { locals.bcd[5] = locals.a0>>4; by35_dispStrobe(0x20); }
  locals.b1 = data;
  if ((sb & 0xff00) != SNDBRD_ST300 && sb != SNDBRD_ASTRO && (sb & 0xff00) != SNDBRD_ST100 && sb != SNDBRD_GRAND)
  {
    sndbrd_0_data_w(0, data & 0x0f); 	// ok
#ifdef LISY_SUPPORT
    if (locals.cb21) lisy35_sound_handler( LISY35_SOUND_HANDLER_IS_DATA, data & 0x0f );
#endif
  }
  coreGlobals.pulsedSolState = 0;
  if (!locals.cb21) {
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  } else if (core_gameData->hw.gameSpecific1 & BY35GD_MARAUDER) {
    coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  }
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
}

/* PIA1:CA1-R IRQ state */
static READ_HANDLER(pia1ca1_r) {
  return locals.ca11;
}

/* PIA1:CB2-W Solenoid/Sound select */
static WRITE_HANDLER(pia1cb2_w) {
  int sb = core_gameData->hw.soundBoard;		// ok
  locals.cb21 = data;
  if (((locals.hw & BY35HW_SCTRL) == 0) && ((sb & 0xff00) != SNDBRD_ST300) && (sb != SNDBRD_ASTRO) && (sb & 0xff00) != SNDBRD_ST100)
   	// ok
    sndbrd_0_ctrl_w(0, (data ? 1 : 0) | (locals.a1 & 0x02));
#ifdef LISY_SUPPORT
    lisy35_sound_handler( LISY35_SOUND_HANDLER_IS_CTRL, (data ? 1 : 0) | (locals.a1 & 0x02));
#endif

}

static INTERRUPT_GEN(by35_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BY35_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
#ifdef LISY_SUPPORT
    lisy35_lamp_handler( 1, 0, 0, 0); //tell the lamp handler that we blank lamps
#endif
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY35_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BY35_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	if (!(core_gameData->hw.gameSpecific1 & BY35GD_ALPHA)) {
	  memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
      memset(locals.pseg,0,sizeof(locals.pseg));
	}
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
}

#if BY35_DEBUG_KEY_SUPPORT
static void adjust_timer(int offset) {
  static char s[2];
  static UINT8 cmd;
  cmd += offset;
  sprintf(s, "%02x", cmd);
  core_textOut(s, 2, 25, 5, 5);
  if (!offset) {
    sndbrd_0_ctrl_w(0, (cmd >> 3) & 0x02);
    sndbrd_0_ctrl_w(0, ((cmd >> 3) & 0x02) | 1);
    sndbrd_0_data_w(0, cmd);
  }
}
#endif /* BY35_DEBUG_KEY_SUPPORT */

static SWITCH_UPDATE(by35) {
#if BY35_DEBUG_KEY_SUPPORT
  if      (keyboard_pressed_memory_repeat(KEYCODE_Z, 2))
    adjust_timer(-0x10);
  else if (keyboard_pressed_memory_repeat(KEYCODE_X, 2))
    adjust_timer(-1);
  else if (keyboard_pressed_memory_repeat(KEYCODE_C, 2))
    adjust_timer(1);
  else if (keyboard_pressed_memory_repeat(KEYCODE_V, 2))
    adjust_timer(0x10);
  else if (keyboard_pressed_memory_repeat(KEYCODE_SPACE, 2))
    adjust_timer(0);
#endif /* BY35_DEBUG_KEY_SUPPORT */
  if (inports) {
    if (core_gameData->gen & (GEN_BY17|GEN_BY35|GEN_STMPU100)) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x60,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x87,2);
    }
    else if (core_gameData->gen & GEN_STMPU200) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x60,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x87,1);
    }
    else if (core_gameData->gen & GEN_HNK) {
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x03,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x06,1);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x81,2);
    }
    else if (core_gameData->gen & GEN_ASTRO) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>8,0x03,1);
      CORE_SETKEYSW((inports[BY35_COMINPORT]&0x20)<<2,0x80,5);
    }
    else if (core_gameData->gen & GEN_BOWLING) {
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x07,0);
      CORE_SETKEYSW(inports[BY35_COMINPORT],   0x20,5);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>7,0x0e,5);
      CORE_SETKEYSW(inports[BY35_COMINPORT]>>15,0x01,5);
    }
  }
  /*-- Diagnostic buttons on CPU board --*/
#ifndef LISY_SUPPORT
  cpu_set_nmi_line(0, core_getSw(BY35_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
#else
  cpu_set_nmi_line(0, lisy35_get_SW_S33() ? ASSERT_LINE : CLEAR_LINE);
#endif
  sndbrd_0_diag(core_getSw(BY35_SWSOUNDDIAG));
  /*-- coin door switches --*/
#ifndef LISY_SUPPORT
  pia_set_input_ca1(BY35_PIA0, !core_getSw(BY35_SWSELFTEST));
#else
  pia_set_input_ca1(BY35_PIA0, !lisy35_get_SW_Selftest());
#endif
}

/* PIA 0 (U10)
PA0-1: (o) Cabinet Switches Strobe
PA0-4: (o) Switch Strobe(Columns)
PA0-3: (o) Lamp Address (Shared with Switch Strobe)
PA0-3: (o) Display Latch Strobe
PA4-7: (o) Lamp Data & BCD Display Data
PA5:   (o) S01-S08 Dips
PA6:   (o) S09-S16 Dips
PA7:   (o) S17-S24 Dips
PB0-7: (i) Switch Returns/Rows and Cabinet Switch Returns/Rows
PB0-7: (i) Dips returns
CA1:   (i) Self Test Switch
CB1:   (i) Zero Cross Detection
CA2:   (o) Display blanking
CB2:   (o) S25-S32 Dips, Lamp strobe #1
*/
/* PIA 1 (U11)
PA0:   (o) 5th display strobe
PA1:   (o) Sound Module Address E / 7th display digit
PA2-7: (o) Display digit select
PB0-3: (o) Momentary Solenoid/Sound Data
PB4-7: (o) Continuous Solenoid
CA1:   (i) Display Interrupt Generator
CA2:   (o) Diag LED + Lamp Strobe #2/Sound select
CB1:   ?
CB2:   (o) Solenoid/Sound Bank Select
*/
static struct pia6821_interface by35_pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, PIA_UNUSED_VAL(1), pia0cb1_r, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq0,piaIrq1
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, pia1ca1_r, PIA_UNUSED_VAL(1), 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq2,piaIrq3
}};

static INTERRUPT_GEN(by35_irq) {
    pia_set_input_ca1(BY35_PIA1, locals.ca11 = !locals.ca11);
}

static void by35_zeroCross(int data) {
    pia_set_input_cb1(BY35_PIA0, locals.cb10 = !locals.cb10);
#ifdef LISY_SUPPORT
    lisy35_throttle();
#endif
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *by35_CMOS;

static NVRAM_HANDLER(by35) {
  core_nvram(file, read_or_write, by35_CMOS, 0x100, (core_gameData->gen & (GEN_STMPU100|GEN_STMPU200))?0x00:0xff);
#ifdef LISY_SUPPORT
  // 0 = read; 1 = write
  //RTH: new: we use seperate rw partition for nvram file
  //lisy35_nvram_handler(read_or_write, by35_CMOS);
#endif
}
// Bally only uses top 4 bits
static WRITE_HANDLER(by35_CMOS_w) {
  by35_CMOS[offset] = data | ((core_gameData->gen & (GEN_STMPU100|GEN_STMPU200|GEN_ASTRO))? 0x00 : 0x0f);
}

// These games use the A0 memory address for monotone sounds
WRITE_HANDLER(stern100_snd_w) {
  logerror("%04x: stern100_snd_w (a0) data  %02x \n", activecpu_get_previouspc(),data);
  sndbrd_0_data_w(0,data);
//if (data != 0)
//  coreGlobals.pulsedSolState = (data << 24);
//locals.solenoids = (locals.solenoids & 0x00ffffff) | coreGlobals.pulsedSolState;
}

// These games can use the C0 memory address for electronic chime sounds
WRITE_HANDLER(stern100_chm_w) {
  logerror("%04x: stern100_chm_w (c0) data  %02x \n", activecpu_get_previouspc(),data);
  sndbrd_0_ctrl_w(0, ~data & 0x0f);
}

static MACHINE_INIT(by35) {
  int sb = core_gameData->hw.soundBoard;
  memset(&locals, 0, sizeof(locals));

  pia_config(BY35_PIA0, PIA_STANDARD_ORDERING, &by35_pia[0]);
  pia_config(BY35_PIA1, PIA_STANDARD_ORDERING, &by35_pia[1]);
  pia_set_input_cb1(BY35_PIA0, 1);
  pia_set_input_ca1(BY35_PIA1, 1);

//   if ((sb & 0xff00) != SNDBRD_ST300)		// ok
  sndbrd_0_init(sb, 1, memory_region(REGION_SOUND1), NULL, NULL);

  locals.vblankCount = 1;
  // set up hardware
  if (core_gameData->gen & (GEN_BY17|GEN_BOWLING)) {
    locals.hw = BY35HW_INVDISP4|BY35HW_DIP4;
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & GEN_BY35) {
    locals.hw = BY35HW_DIP4;
    if ((core_gameData->hw.gameSpecific1 & BY35GD_NOSOUNDE) == 0)
      locals.hw |= BY35HW_SOUNDE;
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & (GEN_STMPU100|GEN_STMPU200|GEN_ASTRO)) {
    locals.hw = BY35HW_INVDISP4|BY35HW_DIP4;
    locals.bcd2seg = core_bcd2seg;
  }
  else if (core_gameData->gen & GEN_HNK) {
    locals.hw = BY35HW_REVSW|BY35HW_SCTRL|BY35HW_INVDISP4;
    locals.bcd2seg = core_bcd2seg9;
  }

  if ((sb & 0xff00) == SNDBRD_ST300 || sb == SNDBRD_ASTRO) {
    install_mem_write_handler(0,0x00a0, 0x00a7, snd300_w);	// ok
    install_mem_read_handler (0,0x00a0, 0x00a7, snd300_r);    	// ok
    install_mem_write_handler(0,0x00c0, 0x00c0, snd300_wex);	// ok
  } else if (sb == SNDBRD_ST100) {
    install_mem_write_handler(0,0x00a0, 0x00a0, stern100_snd_w); // sounds on (DIP 23 = 1)
    install_mem_write_handler(0,0x00c0, 0x00c0, stern100_chm_w); // chimes on (DIP 23 = 0)
  } else if (sb == SNDBRD_ST100B) {
    install_mem_write_handler(0,0x00a0, 0x00a0, stern100_snd_w);
  } else if (sb == SNDBRD_GRAND) {
    install_mem_write_handler(0,0x0080, 0x0080, sndbrd_0_data_w);
  }
}

static MACHINE_RESET(by35) { pia_reset(); }
static MACHINE_STOP(by35) {
  if ((core_gameData->hw.soundBoard & 0xff00) != SNDBRD_ST300 && core_gameData->hw.soundBoard != SNDBRD_ASTRO)
    sndbrd_0_exit();
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
  { 0x0000, 0x007f, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_r(BY35_PIA0) }, /* U10 PIA: Switches + Display + Lamps*/
  { 0x0090, 0x0093, pia_r(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x1000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(by35_writemem)
  { 0x0000, 0x007f, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_w(BY35_PIA0) }, /* U10 PIA: Switches + Display + Lamps*/
  { 0x0090, 0x0093, pia_w(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, by35_CMOS_w, &by35_CMOS }, /* CMOS Battery Backed*/
MEMORY_END

MACHINE_DRIVER_START(by35)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by35,by35,by35)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 500000)
  MDRV_CPU_MEMORY(by35_readmem, by35_writemem)
  MDRV_CPU_VBLANK_INT(by35_vblank, 1)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_IRQFREQ*2)
  MDRV_NVRAM_HANDLER(by35)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(by35)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(by35_zeroCross,BY35_ZCFREQ*2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st100s)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(st100)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st100bs)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(st100b)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_32S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by32)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_51S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by51)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_51NS)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by51N)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_56S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by56)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_61S)
  MDRV_IMPORT_FROM(by35)
//  MDRV_CPU_REPLACE("mcpu", M6800, 525000)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by35_45S)
  MDRV_IMPORT_FROM(by35)
  MDRV_IMPORT_FROM(by45)
MACHINE_DRIVER_END

#ifdef MAME_DEBUG
MACHINE_DRIVER_START(by6802_61S)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6802, 375000)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_6802IRQFREQ*2)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END
#endif

MACHINE_DRIVER_START(by6802_45S)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6802, 375000)
  MDRV_CPU_PERIODIC_INT(by35_irq, BY35_6802IRQFREQ*2)
  MDRV_IMPORT_FROM(by45)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(st200NS)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu",M6800, 1000000)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200s100b)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st100b)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st200v)
  MDRV_IMPORT_FROM(st200NS)
  MDRV_IMPORT_FROM(st300v)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(hnk)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(NULL, 0) // no irq
  MDRV_DIPS(24)
  MDRV_IMPORT_FROM(hnks)
MACHINE_DRIVER_END
