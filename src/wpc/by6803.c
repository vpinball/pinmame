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
  They then used this principle to generate two seperate phase controllers.
  The first phase controller had phases A and B for the feature lamps, and the
  second had phases C and D for the solenoids/flashers.
  The two phase controllers were totally independant of each other
  (tapped from different secondary windings).
  This way, Bally were able to wire two feature lamps or two solenoids to the
  same driver circuit and toggle them on different phases.
  For instance, the Extra ball lamp and the Multi-ball lamp could be wired to the
  same driver circuit. The EB lamp would be "on" for phase A and "off" for phase B,
  the Multi-ball lamp would be "off" for phase A but "on" for phase B.
  The driver transistor was obviously on for both phases.
  Bally where able to double their lamps and coils/flashers by using the same
  hardware without resorting to auxillary lamp or solenoid driver boards.
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

  Lamp numbering:
  Could not find a relationship between the lamp number and the connector
  so here is connector to pinmame number conversion Phase A/C. (Phase B/D = x+48)
  Conn. PinMAME
  J10-01  1     J11-01 43     J12-01 40    J13-01 12
  J10-02  2     J11-02 42     J12-02 41    J13-02 13
  J10-03  3     J11-03 41     J12-03 42    J13-03 14
  J10-04  4     J11-04 40     J12-04 43    J13-04 15
  J10-05  5     J11-05 key    J12-05 key   J13-05 45
  J10-06  6     J11-06 39     J12-06 24    J13-06 44
  J10-07 17     J11-07 27     J12-07 25    J13-07 48
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

#define BY6803_VBLANKFREQ     60 /* VBLANK frequency */
#define BY6803_IRQFREQ       317 /* IRQ (via PIA) frequency*/
#define BY6803_ZCFREQ        120 /* Zero cross frequency (PHASE A equals this value)*/

#define BY6803_SOLSMOOTH       2 /* Smooth the Solenoids over this number of VBLANKS */
#define BY6803_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define BY6803_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

//#define mlogerror printf
#define mlogerror logerror
/*
static void drawit(int seg) {
	int segs[7] = {0};
	int i;
	seg = seg>>1;	//Remove HJ segment for now
	for(i=0; i<7; i++) {
		segs[i] = seg & 1;
		seg = seg>>1;
	}
	logerror("   %x   \n",segs[0]);
	logerror("%x    %x\n",segs[5],segs[1]);
	logerror("   %x   \n",segs[6]);
	logerror("%x    %x\n",segs[4],segs[2]);
	logerror("   %x   \n",segs[3]);
}
*/
static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p0_cb2, p1_cb2;
  int bcd[6];
  int lampadr;
  UINT32 solenoids;
  core_tSeg segments, pseg;
  int dispcol, disprow, commacol;
  int vblankCount;
  int phase_a, p21;
  void (*DISPSTROBE)(int mask);
  WRITE_HANDLER((*SEGWRITE));
  WRITE_HANDLER((*DISPDATA));

  int old_lampadr;
  UINT8 last;
  core_tWord lampDrivers[3]; // The lamp SCR drivers state
  core_tWord lampConduct[3]; // The lamp SCR conducting state
} locals;

static NVRAM_HANDLER(by6803);
static WRITE_HANDLER(by6803_soundLED);

static void piaIrq(int state) {
  cpu_set_irq_line(0, M6803_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/**************************************************/
/* GENERATION 1 Display Handling (Same as MPU-35) */
/**************************************************/
/*Same as Bally MPU-35*/
static WRITE_HANDLER(by6803_segwrite1) {
  int tmp = locals.p1_a;
  locals.p1_a = data;
  if (!locals.p0_ca2) {
    if (tmp & ~data & 0x01) { // Positive edge
      locals.bcd[4] = locals.p0_a>>4;
      locals.DISPSTROBE(0x10);
    }
  }
}

/*Same as Bally MPU-35*/
static WRITE_HANDLER(by6803_dispdata1) {
  if (!locals.p0_ca2) {
    int bcdLoad = locals.p0_a & ~data & 0x0f;
    int ii;
    for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
      if (bcdLoad & 0x01) locals.bcd[ii] = data>>4;
  }
}

/*Same as Bally MPU-35*/
static void by6803_dispStrobe1(int mask) {
  int digit = locals.p1_a & 0xfe;
  int ii,jj;
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          locals.segments[jj*8+ii].w |= locals.pseg[jj*8+ii].w = core_bcd2seg[locals.bcd[jj]];
    }
}

/**************************************************/
/* GENERATION 2 Display Handling				  */
/**************************************************/

static WRITE_HANDLER(by6803_segwrite2) {
/*
  if(data>1 && !locals.p0_ca2) {
    mlogerror("seg_w %x : module=%x : digit=%x : blank=%x\n",data,
              locals.p0_a & 0x0f, locals.p0_a>>4, locals.p0_ca2);
    drawit(data);
  }
*/
  /*Save segment for later*/
  /*Output is not changed, when PA0 is high*/
  if(locals.p0_a&1)
    locals.p1_a = data;
}

static WRITE_HANDLER(by6803_dispdata2) {
/*
	int tmp;

	logerror("pia0a_w: Module 0-3 [%x][%x][%x][%x] = %x\n",
		(data & 0x0f & 1)?1:0, (data & 0x0f & 2)?1:0,(data & 0x0f & 4)?1:0, (data & 0x0f & 8)?1:0, data & 0x0f);
	logerror("pia0a_w: Digit  4-7 = %x\n",data>>4);
	*/

	/*Row/Column Data can only change if blanking is lo..*/
	if(!locals.p0_ca2) {
		//Store Row for later
		int row = data & 0x0f;
		// very odd row / column assignment, but it works!
		if (row == 14) {			//1110 ~= 0001
			locals.disprow = 0;
			//Column Select is demultiplexed!
			locals.dispcol = 15 - (data >> 4);
			locals.DISPSTROBE(0);
		} else if (row == 13) {		//1101 ~= 0010
			locals.disprow = 1;
			locals.DISPSTROBE(0);
		} else if (row == 15) { // activate comma segments when PIA0 CA2 line goes low again.
			locals.disprow = 2;
		}
	}
}

static void by6803_dispStrobe2(int mask) {
	int data;
	if (locals.disprow > 1) {
		// Comma segments
		data = locals.p1_a;
		locals.pseg[9].w  = (data & 0x80) ? 0x80 : 0;
		locals.pseg[12].w = (data & 0x80) ? 0x80 : 0;
		locals.pseg[28].w = (data & 0x20) ? 0x80 : 0;
		locals.pseg[31].w = (data & 0x20) ? 0x80 : 0;
		locals.pseg[2].w  = (data & 0x40) ? 0x80 : 0;
		locals.pseg[5].w  = (data & 0x40) ? 0x80 : 0;
		locals.pseg[35].w = (data & 0x10) ? 0x80 : 0;
		locals.pseg[24].w = (data & 0x10) ? 0x80 : 0;
	} else {
		//Segments H&J is inverted bit 0 (but it's bit 9 in core.c) - Not sure why it's inverted, this is not shown on the schematic
		data = (locals.p1_a >> 1) | ((locals.p1_a & 1) ? 0 : 0x300);
		if (data)
			locals.segments[locals.disprow*20+locals.dispcol].w = data | locals.pseg[locals.disprow*20+locals.dispcol].w;
		else
			locals.segments[locals.disprow*20+locals.dispcol].w = 0;
	}
}

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

  // Lamps column is latched through PIA0:CB2 (15 outputs, 16th is unconnected, so used as all off)
  // Lamps bank is directly enabled by PA5..7 (inverted)
  // All of this is done in sync with zero cross to drive 2 lamps per output (handled in PWM integration, so we update both indifferently for PWM)
  // The lamps are driven by SCR which turns off when AC goes through zero cross (like triacs but conducting only in one current diection)
  UINT8 lampBank = (locals.p0_a >> 5) ^ 0x07; // Selected banks
  int pos = CORE_MODOUT_LAMP0;
  for (int i = 0; i < 3; i++)
  {
    core_tWord prev = locals.lampConduct[i];
    locals.lampDrivers[i].w = (lampBank & (1 << i)) ? (1 << locals.lampadr) & 0x7FFF : 0;
    locals.lampConduct[i].w |= locals.lampDrivers[i].w;
    if (prev.w != locals.lampConduct[i].w)
    {
      //printf("%8.5f %04x  d=%8.5fms  l=%8.5fms  Phase: %d\n", timer_get_time(), locals.lampConduct[i].w, timer_get_time() - coreGlobals.lastACZeroCrossTimeStamp, 1.0/120.0 - (timer_get_time() - coreGlobals.lastACZeroCrossTimeStamp), locals.phase_a);
      // Phase A lamps
      core_write_pwm_output_8b(pos + 0, locals.lampConduct[i].b.lo);
      core_write_pwm_output_8b(pos + 8, locals.lampConduct[i].b.hi);
      // Phase B lamps
      core_write_pwm_output_8b(pos + 48, locals.lampConduct[i].b.lo);
      core_write_pwm_output_8b(pos + 56, locals.lampConduct[i].b.hi);
    }
    pos += 16;
  }
}

/* PIA0:A-W  Control what is read from PIA0:B
(out) PA0-3: Display Latch Strobe (Select 1 of 4 display modules)			(SAME AS BALLY MPU35 - Only 2 Strobes used instead of 4)
(out) PA4-7: BCD Lamp Data													(SAME AS BALLY MPU35)
(out) PA4-7: BCD Display Data (Digit Select 1-16 for 1 disp module)			(SAME AS BALLY MPU35)
*/
static WRITE_HANDLER(pia0a_w) {
  locals.DISPDATA(offset,data);
  locals.p0_a = data;
  if (locals.lampadr != 0x0f) by6803_lampStrobe();

  //printf("%8.5f PIA0 PAx: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
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
static WRITE_HANDLER(pia1a_w) { locals.SEGWRITE(offset,data); }

/* PIA0:B-R  Switch & Cabinet Returns */
/* p0_a bits 0-4 ==> switch columns 1-5
   p1_b bit    4 ==> switch column    6 */
static READ_HANDLER(pia0b_r) {
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x10)<<1));
}

/* PIA0:CB2-W Lamp Strobe, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  //DBGLOG(("PIA0:CB2=%d PC=%4x\n",data,cpu_get_pc()));
  if (locals.p0_cb2 & ~data) locals.lampadr = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;

  //printf("%8.5f PIA0 CB2: %02x PC=%04x\n", timer_get_time(), data, activecpu_get_pc());
  update_lamps();
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
  //DBGLOG(("PIA0:CA2=%d\n",data));
  locals.p0_ca2 = data;
  if (!data) locals.DISPSTROBE(0x1f);
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
  core_write_pwm_output_8b(CORE_MODOUT_SOL0, switchedSol.b.lo);
  core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8, switchedSol.b.hi);
  core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 16, (data & 0xf0) >> 4, 0x0f);

  //DBGLOG(("PIA1:bw=%d\n",data));
}

/* PIA1:CB2-W Solenoid Select */
static WRITE_HANDLER(pia1cb2_w) {
  //DBGLOG(("PIA1:CB2=%d\n",data));
  locals.p1_cb2 = data;
}

static void vblank_all(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
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

  core_updateSw(core_getSol(19));
}

static INTERRUPT_GEN(by6803_vblank) {
  /*-- display --*/
  if ((locals.vblankCount % BY6803_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
  }
  vblank_all();
}

static INTERRUPT_GEN(by6803_vblank_alpha) {
  /*-- display (no smoothing needed) --*/
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  vblank_all();
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

static INTERRUPT_GEN(by6803_irq) {
  pia_set_input_ca1(BY6803_PIA1, locals.last = !locals.last);
}

#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER(by6803_soundCmd) {
  sndbrd_0_data_w(0,data);  sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
}
#endif


// Zero cross
// Phase A and B are derived from main AC voltage with a slight delay:
// - Phase A is wired to M6803 and causes a timer capture interrupt
// - Phase B is wired to PIA0/CB1 which causes an IRQ interrupt
// In turn, these 2 interrupts allow to setup lamp SCR just after zero cross (and they will continue conducting until next zero cross)
static void by6803_zeroCross(int data) {
  locals.phase_a = 1 - locals.phase_a;

  // Phase A drives M6803 interrupt if enabled through inverted P21
  cpu_set_irq_line(0, M6800_TIN_LINE, (locals.phase_a && !locals.p21) ? ASSERT_LINE : CLEAR_LINE);

  // Phase B drives PIA0:CB1 interrupt
  pia_set_input_cb1(BY6803_PIA0, !locals.phase_a);

  // Synchronize core PWM integration AC signal, reset SCR conducting state
  core_zero_cross();
  for (int i = 0; i < 3; i++)
    locals.lampConduct[i].w = locals.lampDrivers[i].w;
  update_lamps();

  //DBGLOG(("phase=%d\n",locals.phase_a));
}

static MACHINE_INIT(by6803) {
  memset(&locals, 0, sizeof(locals));
  locals.old_lampadr = 0x0f;
  sndbrd_0_init(core_gameData->hw.soundBoard,1,memory_region(REGION_SOUND1),NULL,by6803_soundLED);
  pia_config(BY6803_PIA0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(BY6803_PIA1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  locals.vblankCount = 1;
  if (core_gameData->hw.display == BY6803_DISPALPHA) {
    locals.DISPSTROBE = by6803_dispStrobe2;
    locals.SEGWRITE = by6803_segwrite2;
    locals.DISPDATA = by6803_dispdata2;
  }
  else {
    locals.DISPSTROBE = by6803_dispStrobe1;
    locals.SEGWRITE = by6803_segwrite1;
    locals.DISPDATA = by6803_dispdata1;
  }

  // Initialize PWM outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, 48, CORE_MODOUT_BULB_44_20V_AC_POS_BY);
  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 48, 48, CORE_MODOUT_BULB_44_20V_AC_NEG_BY);
  if (coreGlobals.nLamps > 96) // Should always be 96 (3x15 x 2 phases), just put there for safety, but the lamps are wrong
    core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 96, coreGlobals.nLamps - 96, CORE_MODOUT_BULB_44_20V_DC_CC);
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  //coreGlobals.nAlphaSegs = xx * 16;
  //core_set_pwm_output_type(CORE_MODOUT_SEG0, xx, CORE_MODOUT_VFD_STROBE_1_xx6MS); // 1ms strobing
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
     rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
}
static MACHINE_RESET(by6803) {
  pia_reset();
}

static MACHINE_STOP(by6803) {
  sndbrd_0_exit();
}

//NA?
static READ_HANDLER(port1_r) { return 0; }

//Read Phase A Status? (Not sure if this is used)
//JW7 (P23) should not be set, which breaks the gnd connection, so we set the line high.
static READ_HANDLER(port2_r) {
   UINT8 data = 0;
   data |= (locals.phase_a && !locals.p21) ? 0x01 : 0; // P20 = Phase A if P21 is low
   // data |= locals.p21 ? 0x02 : 0; // P21 = low to enable Phase A reading on P20
   // data |= 0x04; // P22 ?
   data |= 0x08; // P23 = 1 unless JW7 installed
   data |= 0x10; // P24 = 1 is hold high unless we are in sound interrupt mode
   return data;
}

//Diagnostic LED, Sound Interrupt and PhaseB interrupt control
static WRITE_HANDLER(port2_w) {
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x02) | ((data>>2) & 0x01);
  sndbrd_0_ctrl_w(0, (data & 0x10) >> 4);
  locals.p21 = data & 0x02;
  cpu_set_irq_line(0, M6800_TIN_LINE, (locals.phase_a && !locals.p21) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(by6803_soundLED) { coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x01) | (data << 1); }

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
(in) P20 = Measure Phase A when P21 is low
(in) P21 = low to enable Phase A reading on P20
(in) P22 = NA?
(in) P23 = 1 Unless JW7 Installed?
(out)P20 = NA?
(out)P21 = Controls Reading of Phase A on P20?
(out)P22 = Drives LED?
(out)P23 = NA?
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
  MDRV_CPU_VBLANK_INT(by6803_vblank, 1)
  MDRV_CPU_PERIODIC_INT(by6803_irq, BY6803_IRQFREQ)
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
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_VBLANK_INT(by6803_vblank_alpha, 1)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
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
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_VBLANK_INT(by6803_vblank_alpha, 1)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
  MDRV_IMPORT_FROM(byTCS)
MACHINE_DRIVER_END
//6803 - Generation 2A Sound (Turbo Cheap Squeak 2)
MACHINE_DRIVER_START(by6803_TCS2S)
  MDRV_IMPORT_FROM(by6803)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_VBLANK_INT(by6803_vblank_alpha, 1)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
  MDRV_IMPORT_FROM(byTCS2)
MACHINE_DRIVER_END
//6803 - Generation 3 Sound (Sounds Deluxe) with keypad
MACHINE_DRIVER_START(by6803_SDS)
  MDRV_IMPORT_FROM(by6803)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_VBLANK_INT(by6803_vblank_alpha, 1)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
  MDRV_IMPORT_FROM(bySD)
MACHINE_DRIVER_END
//6803 - Generation 4 Sound (Williams System 11C) without keypad
MACHINE_DRIVER_START(by6803_S11CS)
  MDRV_IMPORT_FROM(by6803)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_VBLANK_INT(by6803_vblank_alpha, 1)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
  MDRV_IMPORT_FROM(wmssnd_s11cs)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(by6803) {
  core_nvram(file, read_or_write, memory_region(BY6803_CPUREGION)+0x1000, 0x800,0xff);
}
