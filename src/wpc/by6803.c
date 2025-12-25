// license:BSD-3-Clause

/* Bally MPU-6803 Hardware
  6803 vectors:
  RES: FFFE-F
  SWI: FFFA-B Software Interrupt (Not Used)
  NMI: FFFC-D (Not Used)
  IRQ: FFF8-9 (c042 in Party Animal)
  ICF: FFF6-7 (Input Capture)~IRQ2 (c06C in Party Animal)
  OCF: FFF4-5 (Output Compare)~IRQ2 (C403 in Party Animal)
  TOF: FFF2-3 (Timer Overflow)~IRQ2 (Not Used)
  SCI: FFF0-1 (Input Capture)~IRQ2  (Not Used)


  Notes:
  It is almost identical to the Bally -35 hardware with the exception of a 6803 CPU and that it addresses the segments directly on the display.
  But here is what Clive writes:

  You see Bally came up with this neat idea:
  Take the xero-crossing (which occurs 120 times/sec for a 60Hz AC wave, and
  generate two 180 degree phase shifted signals - each occuring 60 times/sec.
  They then used this principle to generate two separate phase controllers.
  The first phase controller had phases A and B for the feature lamps, and the
  second had phases C and D for the solenoids/flashers.
  The two phase controllers were totally independent of each other
  (tapped from different secondary windings).
  This way, Bally were able to wire two feature lamps or two solenoids to the
  same driver circuit and toggle them on different phases.
  For instance, the Extra ball lamp and the Multi-ball lamp could be wired to the
  same driver circuit. The EB lamp would be "on" for phase A and "off" for phase B,
  the Multi-ball lamp would be "off" for phase A, but "on" for phase B.
  The driver transistor was obviously on for both phases.
  Bally where able to double their lamps and coils/flashers by using the same
  hardware without resorting to auxiliary lamp or solenoid driver boards.
  Simple...but brilliant.

  Display:
  7 Segment,7 Digit X 4 + 7 Segment, 2 Digit X 2 (Eight Ball Champ->Lady Luck)
  9 Segment, 14 Digit X 2 (Special Force -> Truck Stop)

  Sound Hardware:
  Squalk & Talk (Eight Ball Champ->Beat The Clock)
  Cheap Squeak (Lady Luck)
  Turbo Cheap Squeak (6809,6821,DAC) (Motor Dome -> Black Belt)
  Sounds Deluxe (68000,6821,PAL,DAC) (Special Forces -> Black Water 100)
  Williams System 11C (Truck Stop & Atlantis)

  Lamp numbering (3x15=45 lamps per phase, 16/32/48 are unwired therefore unused):
  Could not find a relationship between the lamp number and the connector
  so here is connector to pinmame number conversion Phase A/C. (Phase B/D = x+48)
  Conn. PinMAME
  J10-01  1     J11-01 43     J12-01 40    J13-01 12
  J10-02  2     J11-02 42     J12-02 41    J13-02 13
  J10-03  3     J11-03 41     J12-03 42    J13-03 14
  J10-04  4     J11-04 40     J12-04 43    J13-04 15
  J10-05  5     J11-05 key    J12-05 key   J13-05 45
  J10-06  6     J11-06 39     J12-06 24    J13-06 44
  J10-07 17     J11-07 27     J12-07 25    J13-07 48 <--- Really ? or maybe 47 as 48 should be unwired ?
  J10-08 18     J11-08  7     J12-08 26    J13-08 28
  J10-09 19     J11-09 26     J12-09 27    J13-09 key
  J10-10 22     J11-10 25     J12-10 39    J13-10 46
  J10-11 20     J11-11 24     J12-11 11    J13-11 31
  J10-12 21     J11-12  8     J12-12 10    J13-12 30
  J10-13 38     J11-13  9     J12-13  9    J13-13 29
  J10-14 37     J11-14 10     J12-14  8
  J10-15 key    J11-15 11     J12-15 28
  J10-16 33     J11-16 23     J12-16 44
  J10-17 34                   J12-17 12
  J10-18 35
  J10-19 36
*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "wmssnd.h"
#include "by6803.h"

#define BY6803_PIA0 0
#define BY6803_PIA1 1

#define BY6803_ZCFREQ    (50*60) /* Zero cross builder frequency (60Hz, divide in 50 steps to implement 2% Schmitt trigger delay) */

#define BY6803_SOLSMOOTH       2 /* Smooth the Solenoids over this number of VBLANKS */
#define BY6803_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define BY6803_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

static struct {
  UINT32 solenoids;
  int zcCount;
  int vblankCount;

  // PIA 0 & 1 state
  UINT8 p0_a;
  UINT8 p0_ca2;
  UINT8 p0_cb2;
  UINT8 p1_a;
  UINT8 p1_b;
  UINT8 p1_cb2;

  // Alpha display (both generations)
  core_tSeg segments;
  UINT8 dispLatchedBCD[5];
  UINT8 dispPrevSelect;
  UINT8 dispDigitSelect;
  UINT8 dispDigitSegments[2];
  void (*dispUpdate)();

  // Dual lamp array
  int phase_a;
  int phase_b;
  int enablePhaseAEdgeSense;
  int prevPhaseAEdgeSense;
  int lampadr;
  int old_lampadr;
  UINT16 lampCol; // latched lamp column (0..14)
} locals;

static NVRAM_HANDLER(by6803);
static WRITE_HANDLER(by6803_soundLED);

static void piaIrq(int state) {
  cpu_set_irq_line(0, M6803_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/**************************************************/
/* GENERATION 1 Display Handling (Same as MPU-35) */
/**************************************************/

static void by6803_dispUpdate1(void) {
  // ca2 is inhibit/blanking: 0 = enable latching / 1 = enable output
  // 4 highest PIA0_Ax is bcd
  // 4 lowest PIA0_Ax and 1 lowest of PIA1 is display select (latch enable of 14543, inverted)
  // 7 highest PIA1_Ax is digit select
  if (locals.p0_ca2 == 0) {
    const int latchSelect = ((locals.p0_a & 0x0F) | ((locals.p1_a & 0x01) << 4)) ^ 0x1F;
    UINT8 bcd = (UINT8)core_bcd2seg7[locals.p0_a >> 4]; // 7 segments + comma => 8 bits
    if (bcd & 0x24) bcd |= 0x80; // Comma is on when segments 'c' or 'e' are
    for (int display = 0; display < 5; display++) {
      if (latchSelect & (0x01 << display))
        locals.dispLatchedBCD[display] = bcd;
      for (int digit = 0; digit < 7; digit++) {
        const int col = display * 8 + digit + 1;
        core_write_pwm_output_8b(CORE_MODOUT_SEG0 + col * 16, 0);
      }
    }
  }
  else {
    for (int display = 0; display < 5; display++) {
      for (int digit = 0; digit < 7; digit++) {
        const int col = display * 8 + digit + 1;
        const UINT8 segs = (locals.p1_a & (0x02 << digit)) ? locals.dispLatchedBCD[display] : 0x00;
        locals.segments[col].w |= segs;
        core_write_pwm_output_8b(CORE_MODOUT_SEG0 + col * 16, segs);
      }
    }
  }
}

/**************************************************/
/* GENERATION 2 Display Handling                  */
/**************************************************/

static void by6803_dispUpdate2(void) {
  // Recreate legacy ordering bug (see dispBy104 in by6803games.c), this should be: 13 - locals.dispDigitSelect;
  static const int colPos[2][16] = {
    {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, /* unuseds */ 0, 0},
    {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 15, /* unuseds */ 0, 0},
  };

  const UINT8 select = locals.p0_ca2 ? 0 : ~locals.p0_a; // Disp inhibit turn all to 0, data is inverted (U13 4502)
  if (locals.dispPrevSelect & ~select & 0x01) // falling edge latches display's digit select (4514). Note that they are all wired to the first strobe (requiring to latch segment data first on each display)
     locals.dispDigitSelect = locals.p0_a >> 4;
  locals.dispPrevSelect = select;
  // high state latches segment data (74373 transparent latch)
  if (select & 1) locals.dispDigitSegments[0] = locals.p1_a;
  if (select & 2) locals.dispDigitSegments[1] = locals.p1_a;

  // Inhibited or no digit selected, just turn to 0
  if (locals.p0_ca2 == 0 || locals.dispDigitSelect > 14) {
    for (int row = 0; row < 2; row++) {
      for (int digit = 0; digit < 14; digit++) {
        const int col = row * 20 + colPos[row][digit];
        core_write_pwm_output_16b(CORE_MODOUT_SEG0 + col * 16, 0);
      }
    }
  }
  else {
    for (int row = 0; row < 2; row++) {
      for (int digit = 0; digit < 14; digit++) {
        const int col = row * 20 + colPos[row][digit];
        if (digit != locals.dispDigitSelect) {
          core_write_pwm_output_16b(CORE_MODOUT_SEG0 + col * 16, 0);
        }
        else {
          UINT16 segs = locals.dispDigitSegments[row];
          // Segments H&J is bit 0 (but it's bit 9 in core.c), inverted through inverter U13 (schematics clearly states a 4502 with correct pinout but use an incorrect OR gate symbol...)
          segs = (segs >> 1) | ((segs & 1) ? 0 : 0x300);
          // Commas are activated with segments c/e (latched bits 3 or 5) + direct trigger bits (unlatched)
          switch (locals.dispDigitSelect) {
          case  3:
          case  6:
            if (segs & 0x24) {
              if (row == 0)
                segs |= (locals.p1_a & 0x80) ? 0x0080 : 0x0000; // D1J1-14 - CJ2-2
              else
                segs |= (locals.p1_a & 0x20) ? 0x0080 : 0x0000; // D2J1-14 - CJ2-4
            }
            break;
          case 10:
          case 13:
            if (segs & 0x24) {
              if (row == 0)
                segs |= (locals.p1_a & 0x40) ? 0x0080 : 0x0000; // D1J1-11 - CJ2-3
              else
                segs |= (locals.p1_a & 0x10) ? 0x0080 : 0x0000; // D2J1-11 - CJ2-5
            }      
            break;
          default: break;
          }
          locals.segments[col].w |= segs;
          core_write_pwm_output_16b(CORE_MODOUT_SEG0 + col * 16, segs);
        }
      }
    }
  }
}

/**************************************************/
/* Lamp array                                     */
/**************************************************/

static void by6803_lampStrobe(void) {
  int lampadr = locals.lampadr;
  if (lampadr != locals.old_lampadr) {
    int i, lampdata = (locals.p0_a>>5)^0x07;
    volatile UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3) + 6 * (1 - locals.phase_a)];
    const int bit = 1<<(lampadr & 0x07);

    //DBGLOG(("adr=%x data=%x\n",lampadr,lampdata));
    /*if (bit)*/ for (i=0; i < 3; i++) {
      if (lampdata & 0x01) *matrix |= bit; else *matrix &= (0xff ^ bit);
      lampdata >>= 1; matrix += 2;
    }
  }
  locals.old_lampadr = lampadr;
}

static void update_lamps(void) {
  // Lamp columns are latched through PIA0:CB2 (15 outputs, 16th is unconnected), all at once for the 3 banks
  // Lamp banks are directly enabled by PIA:A5..7 (inverted)
  // All of this is done in sync with zero cross to drive 2 AC lamps per output
  // The lamps are driven by SCR which turns off when AC goes through zero cross (like triacs but conducting only in one current direction)
  // Roughly the duty cycle is 43.4% (SCR are triggered 1.1ms after zerocross for the rest of the AC positive/negative wave)
  if (locals.p0_cb2 == 1) // Latch lamp column while CB02 is high (4514 is state driven, not edge)
    locals.lampCol = locals.p0_a & 0x0F;
  if (locals.lampCol < 15) {
    if ((locals.p0_a & 0x20) == 0) {
      core_write_pwm_output(CORE_MODOUT_LAMP0 +  0 + locals.lampCol, 1, 1); // Phase A/C lamps (A is 20.5V AC for inserts, C is 48V AC for flashers, named bright lights)
      core_write_pwm_output(CORE_MODOUT_LAMP0 + 48 + locals.lampCol, 1, 1); // Phase B/D lamps (negative/positive is handled during PWM integration)
    }
    if ((locals.p0_a & 0x40) == 0) {
      core_write_pwm_output(CORE_MODOUT_LAMP0 + 16 + locals.lampCol, 1, 1);
      core_write_pwm_output(CORE_MODOUT_LAMP0 + 64 + locals.lampCol, 1, 1);
    }
    if ((locals.p0_a & 0x80) == 0) {
      core_write_pwm_output(CORE_MODOUT_LAMP0 + 32 + locals.lampCol, 1, 1);
      core_write_pwm_output(CORE_MODOUT_LAMP0 + 80 + locals.lampCol, 1, 1);
    }
  }
}

/* PIA0:A-W  Control what is read from PIA0:B
(out) PA0-3: Display Latch Strobe (Select 1 of 4 display modules)			(SAME AS BALLY MPU35 - Only 2 Strobes used instead of 4)
(out) PA4-7: BCD Lamp Data													(SAME AS BALLY MPU35)
(out) PA4-7: BCD Display Data (Digit Select 1-16 for 1 disp module)			(SAME AS BALLY MPU35)
(out) PA0-3: Lamp column select
(out) PA5-7: Enable lamp bank (inverted)
*/
static WRITE_HANDLER(pia0a_w) {
  //if (activecpu_get_pc() < 0xc400) // Filter out display rasterizer (to see lamp strobing)
  // printf("%8.5f PIA0 PAx: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
  locals.p0_a = data;
  locals.dispUpdate();

  if (locals.lampadr != 0x0f) by6803_lampStrobe();
  update_lamps();
}

/* PIA0:B-R  Switch & Cabinet Returns */
/* p0_a bits 0-4 ==> switch columns 1-5
   p1_b bit    4 ==> switch column    6 */
static READ_HANDLER(pia0b_r) {
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x10)<<1));
}

/* PIA0:CB2-W Lamp Strobe, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  //if (activecpu_get_pc() < 0xc400) // Filter out display rasterizer (to see lamp strobing)
  //printf("%8.5f PIA0 CB2: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
  if (locals.p0_cb2 & ~data) locals.lampadr = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
  update_lamps();
}

/* PIA1:A-W  0,2-7 Display handling:
(out) PA0 = PA10 = SEG DATA H+J
(out) PA1 = J2-8 = SEG DATA A
(out) PA2 = J2-7 = SEG DATA B
(out) PA3 = J2-6 = SEG DATA C
(out) PA4 = J2-5 = SEG DATA D
(out) PA5 = J2-4 = SEG DATA E
(out) PA6 = J2-3 = SEG DATA F
(out) PA7 = J2-2 = SEG DATA G
*/
static WRITE_HANDLER(pia1a_w) {
  //printf("%8.5f PIA1 PAx: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
  locals.p1_a = data;
  locals.dispUpdate();
}

/* PIA1:CA2-W Diagnostic LED (earlier games) */
#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER(pia1ca2_w) {
  //DBGLOG(("PIA1:CA2=%d\n",data));
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x02) | data;
}
#endif

/* PIA0:CA2-W Display Blanking/Select */
static WRITE_HANDLER(pia0ca2_w) {
  //printf("%8.5f PIA0 CA2: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
  locals.p0_ca2 = data;
  locals.dispUpdate();
}

/* PIA1:B-W Solenoid output */
static WRITE_HANDLER(pia1b_w) {
  locals.p1_b = data;
  coreGlobals.pulsedSolState = 0;
  if (!locals.p1_cb2)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
  
  core_tWord switchedSol;
  switchedSol.w = locals.p1_cb2 ? 0 : ((1 << (data & 0x0f)) & 0x7fff);
  core_write_pwm_output_16b(CORE_MODOUT_SOL0, switchedSol.w);
  core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 16, (data & 0xf0) >> 4, 0x0f);

  //DBGLOG(("PIA1:bw=%d\n",data));
}

/* PIA1:CB2-W Solenoid Select */
static WRITE_HANDLER(pia1cb2_w) {
  //DBGLOG(("PIA1:CB2=%d\n",data));
  locals.p1_cb2 = data;
}

static SWITCH_UPDATE(by6803) {
  int ext = (core_gameData->gen & GEN_BY6803A) ? 0 : 1;
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[BY6803_COMINPORT]>>13) & 0x03;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (ext?0xd0:0xdf)) |
                              ((inports[BY6803_COMINPORT]) & 0x20) |
                              (ext?(inports[BY6803_COMINPORT+1] & 0x0f):0);
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (ext?0x90:0x98)) |
                              ((inports[BY6803_COMINPORT]>>6) & 0x67) |
                              (ext?((inports[BY6803_COMINPORT+1]>>4) & 0x0f):0);
    if (ext) {
        coreGlobals.swMatrix[3] = (coreGlobals.swMatrix[3] & 0xf0) |
                                  ((inports[BY6803_COMINPORT+1]>>8) & 0x0f);
        coreGlobals.swMatrix[4] = (coreGlobals.swMatrix[4] & 0xf0) |
                                  ((inports[BY6803_COMINPORT+1]>>12) & 0x0f);
    }
  }

  /*-- Diagnostic buttons on CPU board --*/
  //if (core_getSw(BY6803_SWCPUDIAG))  cpu_set_nmi_line(0, PULSE_LINE);
//  if (core_getSw(BY6803_SWSOUNDDIAG)) locals.SOUNDDIAG();
  sndbrd_0_diag(core_getSw(BY6803_SWSOUNDDIAG));
  /*-- coin door switches --*/
  pia_set_input_ca1(BY6803_PIA0, !core_getSw(BY6803_SWSELFTEST));
}

/*
-------
 PIA 0
-------
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows			(SAME AS BALLY MPU35)
(in)  CA1:   Self Test Switch												(SAME AS BALLY MPU35)
(in)  CB1:   Zero Cross/Phase B Detection									(ZERO CROSS IN BALLY MPU35)
(in)  CA2:   N/A															(SAME AS BALLY MPU35)
(in)  CB2:   N/A?
(out) PA0-3: Cabinet Switches Strobe (shared below)							(SAME AS BALLY MPU35 + 2 Extra Columns)
(out) PA0-4: Switch Strobe(Columns)											(SAME AS BALLY MPU35)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)						(SAME AS BALLY MPU35)
(out) PA0-3: Display Latch Strobe (Select 1 of 4 display modules)			(SAME AS BALLY MPU35 - Only 2 Strobes used instead of 4)
(out) PA4-7: BCD Lamp Data													(SAME AS BALLY MPU35)
(out) PA4-7: BCD Display Data (Digit Select 1-16 for 1 disp module)			(SAME AS BALLY MPU35)
(out) CA2:   Display Blanking/(Select?)										(SAME AS BALLY MPU35)
(out) CB2:   Lamp Strobe #1 (Drives Main Lamp Driver - Playfield Lamps)		(SAME AS BALLY MPU35)
	  IRQ:	 Wired to Main 6803 CPU IRQ.									(SAME AS BALLY MPU35)
*/
/*
-------
 PIA 1
-------
(out) PA0 = PA10 = SEG DATA H+J
(out) PA1 = J2-8 = SEG DATA A
(out) PA2 = J2-7 = SEG DATA B
(out) PA3 = J2-6 = SEG DATA C
(out) PA4 = J2-5 = SEG DATA D
(out) PA5 = J2-4 = SEG DATA E
(out) PA6 = J2-3 = SEG DATA F
(out) PA7 = J2-2 = SEG DATA G
(out) PB0-3: Momentary Solenoid												(SAME AS BALLY MPU35 Except no sound data)
(out) PB4-7: Continuous Solenoid											(SAME AS BALLY MPU35)
(out) PB4:   Switch Strobe 5 (only when JW9 installed)
CA1 = N/A
CB1 = N/A
CA2 = N/A
(out) CB2 = Solenoid Select, 0=SOL1-8,1=SOL9-16								(SAME AS BALLY MPU35 Except no sound data)
IRQ:  NOT? Wired to Main 6803 CPU IRQ.
*/
static struct pia6821_interface piaIntf[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, PIA_UNUSED_VAL(1),PIA_UNUSED_VAL(1), 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, PIA_UNUSED_VAL(1),PIA_UNUSED_VAL(1), 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,0,pia1cb2_w,
/* IRQ: A/B              */  0,0
}};

#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER(by6803_soundCmd) {
  sndbrd_0_data_w(0,data);  sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
}
#endif

static void by6803_update_phaseAEdgeSense() {
  int phaseAEdgeSense = locals.enablePhaseAEdgeSense && locals.phase_a;
  if (~locals.prevPhaseAEdgeSense && phaseAEdgeSense)
    cpu_set_irq_line(0, M6800_TIN_LINE, ASSERT_LINE);
  else if (locals.prevPhaseAEdgeSense && ~phaseAEdgeSense)
    cpu_set_irq_line(0, M6800_TIN_LINE, CLEAR_LINE);
  locals.prevPhaseAEdgeSense = phaseAEdgeSense;
}

// Zero cross
// Phase A and B are derived from main AC voltage with a slight delay:
// - Phase A is wired to M6803 and is used to generate half of the synchronization using M6803 TIN capture interrupt
// - Phase B is wired to PIA0/CB1 which causes an IRQ interrupt to drive the other half of the synchronization
// In turn, these 2 interrupts allow to setup lamp SCR just after zero cross (and they will continue conducting until next zero cross)
// 
// The signals are built from the raw 20.5V AC sinewave, passed through Schmitt Triggers (14584 U12) with threshold around 2.5V.
// This corresponds to a delay of roughly 2% of the period, so this timer is called 50 x 60Hz, and we rebuild Phase A and Phase B from it
static void by6803_zeroCross(int data) {
  // printf("%8.5f --- ZeroCross IRQ ---\n", timer_get_time());
  locals.zcCount++;
  if (locals.zcCount >= 50)
    locals.zcCount = 0;

  // On real zerocross, synchronize core PWM integration AC signal and reset SCR conducting state
  if (locals.zcCount == 0)
     core_zero_cross(); // Only resync on phase A (we would reverse the sinewave otherwise)
  if (locals.zcCount == 0 || locals.zcCount == 25)
  {
    // A non 0 value at zerocross is a bug as it would fully defeat the phase A/B design, and would lead to a too high bulb duty cycle (bulbs would burn). We use this to detect timing errors
    // Note that this does happen during startup sequence where the PIA are tested and lead to a passing state other a zerocross (but a single time, not great so no burnt bulb)
    for (int i = 0; i < 6; i++)
      core_write_pwm_output_16b(CORE_MODOUT_LAMP0 + i * 16, 0);
  }

  // 0..24 => Positive, 25..49 => Negative
  // Schmitt Trigger create a 2% offset, so we remove one interval
  locals.phase_a = (1 <= locals.zcCount && locals.zcCount <= 23) ? 1 : 0;
  locals.phase_b = (26 <= locals.zcCount && locals.zcCount <= 48) ? 1 : 0;

  // Phase A drives M6803 timer interrupt if enabled through inverted P21
  by6803_update_phaseAEdgeSense();

  // Phase B drives PIA0:CB1 interrupt (in turn triggering M6803 IRQ interrupt)
  pia_set_input_cb1(BY6803_PIA0, locals.phase_b);

  // Copy local data to interface at 60Hz (used to be a fake VBlank, moved here to be in sync with gamecode)
  if (locals.zcCount == 0)
  {
    locals.vblankCount++;

    /*-- lamps --*/
    if ((locals.vblankCount % BY6803_LAMPSMOOTH) == 0) {
      memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    }

    /*-- solenoids --*/
    if ((locals.vblankCount % BY6803_SOLSMOOTH) == 0) {
      coreGlobals.solenoids = locals.solenoids;
      locals.solenoids = coreGlobals.pulsedSolState;
    }

    /*-- display --*/
    if ((locals.vblankCount % BY6803_DISPLAYSMOOTH) == 0) {
      memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
      memset(locals.segments, 0, sizeof(locals.segments));
    }

    core_updateSw(core_getSol(19));
  }
}

static MACHINE_INIT(by6803) {
  memset(&locals, 0, sizeof(locals));
  locals.old_lampadr = 0x0f;
  sndbrd_0_init(core_gameData->hw.soundBoard,1,memory_region(REGION_SOUND1),NULL,by6803_soundLED);
  pia_config(BY6803_PIA0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(BY6803_PIA1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  locals.vblankCount = 1;
  locals.dispUpdate = (core_gameData->hw.display == BY6803_DISPALPHA) ? by6803_dispUpdate2 : by6803_dispUpdate1;

  // Initialize PWM outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, 48, CORE_MODOUT_BULB_44_20V_AC_POS_BY);
  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 48, 48, CORE_MODOUT_BULB_44_20V_AC_NEG_BY);
  if (coreGlobals.nLamps > 96) // Should always be 96 (3x15 x 2 phases), just put there for safety, but the lamps are wrong
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 96, coreGlobals.nLamps - 96, CORE_MODOUT_BULB_44_20V_DC_CC);
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  if (core_gameData->hw.display == BY6803_DISPALPHA) {
    coreGlobals.nAlphaSegs = 2 * 20 * 16; // actually 2x14, but implemented as 2x20
    core_set_pwm_output_led_vfd(CORE_MODOUT_SEG0, 40 * 16, TRUE, 14.65f / 0.9f); // 0.9ms strobing over a 14.65ms period
  }
  else {
    coreGlobals.nAlphaSegs = 5 * 8 * 16; // actually 5x7, but implemented as 5x8
    core_set_pwm_output_led_vfd(CORE_MODOUT_SEG0, 40 * 16, TRUE, 20.5f / 2.9f); // 2.9ms strobing over a 20.5ms period
  }
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
     rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
  if (strncasecmp(gn, "dungdrag", 8) == 0) { // Dungeons & Dragons
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 14      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-3
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 14 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); //      
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 45      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-5
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 45 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); //      
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 29      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-13
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 29 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); //       
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 13      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-2
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 13 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); //      
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 44      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-6
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 44 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); // 
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 28      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-8
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 28 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); // 
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J13-1
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); // 
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 43      - 1, 1, CORE_MODOUT_BULB_89_48V_AC_POS_BY); // J11-1 Flash Left
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 43 + 48 - 1, 1, CORE_MODOUT_BULB_89_48V_AC_NEG_BY); //       Left Sling
  }
}

static MACHINE_RESET(by6803) {
  pia_reset();
}

static MACHINE_STOP(by6803) {
  sndbrd_0_exit();
}

// Port1: noop read, write handled by sound board handler (all are outputs to soundboard)
static READ_HANDLER(port1_r) {
  return 0;
}
static WRITE_HANDLER(by6803_soundLED) {
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x01) | (data << 1);
}

// Port2
// 0 (in)  Phase A status if P21 = 0, 0 otherwise. (Note that P21 is the timer compare register output)
// 1 (out) State of the timer compare register, enable phase A input on P20. This bit is managed by Mame 6803 CPU core (TCSR_OLVL)
// 2 (out) The diagnostic led output
// 3 (in)  JW7 state. JW7 should not be set, which breaks the gnd connection, so we set the line high.
// 4 (out) Sound interrupt: it is hold high unless we are in sound interrupt mode
static READ_HANDLER(port2_r) {
  UINT8 data = 0x00;
  data |= (locals.enablePhaseAEdgeSense && locals.phase_a) ? 0x01 : 0x00;
  data |= 0x08;
  data |= 0x10;
  return data;
}
static WRITE_HANDLER(port2_w) {
  locals.enablePhaseAEdgeSense = (data & 0x02) == 0;
  by6803_update_phaseAEdgeSense();
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x02) | ((data>>2) & 0x01);
  sndbrd_0_ctrl_w(0, (data & 0x10) >> 4);
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*
NMI: = SW1?
Port 1:
(out)P10 = J5-7 ->  SJ1-1 = Sound Data 1
(out)P11 = J5-8 ->  SJ1-2 = Sound Data 2
(out)P12 = J5-9 ->  SJ1-3 = Sound Data 3
(out)P13 = J5-10 -> SJ1-4 = Sound Data 4
(out)P14 = J5-11 ->NA?
(out)P15 = J5-12 ->NA?
(out)P16 = J5-13 ->NA?
(out)P17 = J5-14 ->NA?
Port 2:
(in) P20 = Measure Phase A when P21 is low (interrupt input)
(out)P21 = Controls Reading of Phase A on P20 (CPU timer output)
(out)P22 = Drives LED
(in) P23 = 1 Unless JW7 Installed
(out)P24 = P24 = J5-15 -> SJ1-8 = Sound Interrupt
*/
static MEMORY_READ_START(by6803_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0020, 0x0023, pia_r(BY6803_PIA0) },
  { 0x0040, 0x0043, pia_r(BY6803_PIA1) },
  { 0x0080, 0x00ff, MRA_RAM },	/*Internal 128K RAM*/
  { 0x1000, 0x17ff, MRA_RAM },	/*External RAM*/
  { 0x8000, 0xffff, MRA_ROM },	/*U2 & U3 ROM */
MEMORY_END

static MEMORY_WRITE_START(by6803_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0020, 0x0023, pia_w(BY6803_PIA0) },
  { 0x0040, 0x0043, pia_w(BY6803_PIA1) },
  { 0x0080, 0x00ff, MWA_RAM },	/*Internal 128K RAM*/
  { 0x1000, 0x17ff, MWA_RAM },	/*External RAM*/
  { 0x8000, 0xffff, MWA_ROM },	/*U2 & U3 ROM */
MEMORY_END

static PORT_READ_START( by6803_readport )
  { M6803_PORT1, M6803_PORT1, port1_r },
  { M6803_PORT2, M6803_PORT2, port2_r },
PORT_END

static PORT_WRITE_START( by6803_writeport )
  { M6803_PORT1, M6803_PORT1, sndbrd_0_data_w }, // PB0-3 connected on schem
  { M6803_PORT2, M6803_PORT2, port2_w },
PORT_END

static MACHINE_DRIVER_START(by6803)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by6803,by6803,by6803)
  MDRV_CPU_ADD_TAG("mcpu", M6803, 3579545./4.)
  MDRV_CPU_MEMORY(by6803_readmem, by6803_writemem)
  MDRV_CPU_PORTS(by6803_readport, by6803_writeport)
  MDRV_NVRAM_HANDLER(by6803)
  MDRV_SWITCH_UPDATE(by6803)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_TIMER_ADD(by6803_zeroCross, BY6803_ZCFREQ)
  MDRV_SOUND_CMD(by6803_soundCmd)
  MDRV_SOUND_CMDHEADING("by6803")
  MDRV_DIPS(1) // needed for extra core inport!
MACHINE_DRIVER_END

//6803 - Generation 1 Sound (Squawk & Talk)
MACHINE_DRIVER_START(by6803_61S)
  MDRV_IMPORT_FROM(by6803)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END
//6803 - Generation 1 Sound (Squawk & Talk), alpha display
MACHINE_DRIVER_START(by6803_61SA)
  MDRV_IMPORT_FROM(by6803)
  MDRV_SCREEN_SIZE(640+CORE_SCREENX_INC,400)
  MDRV_VISIBLE_AREA(0, 639+CORE_SCREENX_INC, 0, 399)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END
//6803 - Generation 1A Sound (Cheap Squeak)
MACHINE_DRIVER_START(by6803_45S)
  MDRV_IMPORT_FROM(by6803)
  MDRV_IMPORT_FROM(by45)
MACHINE_DRIVER_END
//6803 - Generation 2 Sound (Turbo Cheap Squeak)
MACHINE_DRIVER_START(by6803_TCSS)
  MDRV_IMPORT_FROM(by6803)
  MDRV_SCREEN_SIZE(640+CORE_SCREENX_INC,400)
  MDRV_VISIBLE_AREA(0, 639+CORE_SCREENX_INC, 0, 399)
  MDRV_IMPORT_FROM(byTCS)
MACHINE_DRIVER_END
//6803 - Generation 2A Sound (Turbo Cheap Squeak 2)
MACHINE_DRIVER_START(by6803_TCS2S)
  MDRV_IMPORT_FROM(by6803)
  MDRV_SCREEN_SIZE(640+CORE_SCREENX_INC,400)
  MDRV_VISIBLE_AREA(0, 639+CORE_SCREENX_INC, 0, 399)
  MDRV_IMPORT_FROM(byTCS2)
MACHINE_DRIVER_END
//6803 - Generation 3 Sound (Sounds Deluxe) with keypad
MACHINE_DRIVER_START(by6803_SDS)
  MDRV_IMPORT_FROM(by6803)
  MDRV_SCREEN_SIZE(640+CORE_SCREENX_INC,400)
  MDRV_VISIBLE_AREA(0, 639+CORE_SCREENX_INC, 0, 399)
  MDRV_IMPORT_FROM(bySD)
MACHINE_DRIVER_END
//6803 - Generation 4 Sound (Williams System 11C) without keypad
MACHINE_DRIVER_START(by6803_S11CS)
  MDRV_IMPORT_FROM(by6803)
  MDRV_SCREEN_SIZE(640+CORE_SCREENX_INC,400)
  MDRV_VISIBLE_AREA(0, 639+CORE_SCREENX_INC, 0, 399)
  MDRV_IMPORT_FROM(wmssnd_s11cs)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(by6803) {
  core_nvram(file, read_or_write, memory_region(BY6803_CPUREGION)+0x1000, 0x800,0xff);
}
