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

*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "by6803.h"
#include "by6803snd.h"
#include "by35snd.h"

#define BY6803_VBLANKFREQ    60 /* VBLANK frequency */
#define BY6803_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define BY6803_ZCFREQ       240 /* Zero cross frequency (PHASE A is 1/2 this value)*/

//#define mlogerror printf
#define mlogerror logerror
/*
static void drawit(int seg) {
	int segs[8] = {0};
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
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2;
  int bcd[6];
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int dispcol, disprow;
  int diagnosticLed;
  int sounddiagnosticLed;
  int vblankCount;
  int initDone;
  int phase_a, p21;
  int sndint;
  int snddata;
  void *zctimer;
  void (*SOUNDINIT)(void);
  void (*SOUNDEXIT)(void);
  WRITE_HANDLER((*SOUNDCOMMAND));
  void (*SOUNDDIAG)(void);
  void (*DISPSTROBE)(int mask);
  WRITE_HANDLER((*SEGWRITE));
  WRITE_HANDLER((*DISPDATA));
} locals;

static void by6803_exit(void);
static void by6803_nvram(void *file, int write);

static void piaIrq(int state) {
  //DBGLOG(("IRQ = %d\n",state));
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
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
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
		//Digit Column/Select is 1-16 Demultiplexed!
		int col = 16 - (data >> 4);
		// very odd row / column assignment, but it works!
		if (row == 14) {			//1110 ~= 0001
			locals.disprow = 0;
			locals.dispcol = col-1;
		} else if (row == 13) {		//1101 ~= 0010
			locals.disprow = 1;
		} else if (row == 1) {		//0001 ~= 1110
			locals.dispcol = col;
		} else locals.disprow = 2;
		locals.DISPSTROBE(0);
	}
}

static void by6803_dispStrobe2(int mask) {
	//Segments H&J is inverted bit 0 (but it's bit 8 in core.c) - Not sure why it's inverted, this is not shown on the schematic
	int data = (locals.p1_a >> 1) | (((locals.p1_a & 1)<<7)^0x80);
	if(locals.disprow < 2)
		locals.segments[locals.disprow][locals.dispcol].lo |= locals.pseg[locals.disprow][locals.dispcol].lo = data;
}


static void by6803_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = ((locals.p0_a>>4)^0x0f) & 0x07;
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+8*board];
    int bit = 1<<(lampadr & 0x07);

    DBGLOG(("adr=%x data=%x\n",lampadr,lampdata));
    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
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
  by6803_lampStrobe(locals.phase_a<2,locals.lampadr1);
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
static READ_HANDLER(pia0b_r) {
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x80)>>2));
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  //DBGLOG(("PIA0:CB2=%d PC=%4x\n",data,cpu_get_pc()));
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}
/* PIA1:CA2-W Lamp Strobe #2 */
static WRITE_HANDLER(pia1ca2_w) {
  //DBGLOG(("PIA1:CA2=%d\n",data));
  if (locals.p1_ca2 & ~data) locals.lampadr2 = locals.p0_a & 0x0f;
  locals.diagnosticLed = data;
  locals.p1_ca2 = data;
}

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
  //DBGLOG(("PIA1:bw=%d\n",data));
}

/* PIA1:CB2-W Solenoid Select */
static WRITE_HANDLER(pia1cb2_w) {
  //DBGLOG(("PIA1:CB2=%d\n",data));
  locals.p1_cb2 = data;
}

static int by6803_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BY6803_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY6803_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BY6803_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed | (locals.sounddiagnosticLed<<1);
    locals.diagnosticLed = locals.sounddiagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
  return 0;
}

static void by6803_updSw(int *inports) {
  int ext = coreData.coreDips?1:0;
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
  if (core_getSw(BY6803_SWSOUNDDIAG)) locals.SOUNDDIAG();
  /*-- coin door switches --*/
  pia_set_input_ca1(0, !core_getSw(BY6803_SWSELFTEST));
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
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,0,pia1cb2_w,
/* IRQ: A/B              */  0,0
}};

static int by6803_irq(void) {
  static int last = 0;
  pia_set_input_ca1(1, last = !last);
//  DBGLOG(("irq=%d\n",last));
  return 0;
}

static WRITE_HANDLER(by6803_soundCmd) {
	locals.SOUNDCOMMAND(offset,data);
}

static core_tData by6803Data = {
  1, // keypad inports
  by6803_updSw, 2, by6803_soundCmd, "by6803",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static core_tData by6803aData = {
  0, // no keypad
  by6803_updSw, 2, by6803_soundCmd, "by6803",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void by6803_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
  locals.phase_a = (locals.phase_a + 1) & 3;
  pia_set_input_cb1(0, (locals.phase_a == 1) || (locals.phase_a == 2));
  cpu_set_irq_line(0, M6800_TIN_LINE, (locals.phase_a<2 && !locals.p21) ? ASSERT_LINE : CLEAR_LINE);
  DBGLOG(("phase=%d\n",locals.phase_a));
#if 0
  int state = locals.phase_a;

  /*toggle phase_a*/
  locals.phase_a = !locals.phase_a;

  /*Make sure to update the lamps - doesn't seem to help*/
  by6803_lampStrobe(!locals.phase_a,locals.lampadr1);

  //printf("setting phase a to %x, lampadr1=%x\n",locals.phase_a,locals.lampadr1);

  /*set phase b - opposite of phase a*/
  pia_set_input_cb1(0,!locals.phase_a);
  //pia_set_input_cb1(0, 0); pia_set_input_cb1(0, 1);

  /*set 6803 P20 line*/
  cpu_set_irq_line(0, M6800_TIN_LINE, state ? ASSERT_LINE : CLEAR_LINE);
#endif
}

static void by6803_init_common(int hasKeypad) {
  if (hasKeypad?core_init(&by6803Data):core_init(&by6803aData)) return;
  /* init PIAs */
  pia_config(0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  if (coreGlobals.soundEn) locals.SOUNDINIT();
  pia_reset();
  locals.vblankCount = 1;
  locals.zctimer = timer_pulse(TIME_IN_HZ(BY6803_ZCFREQ),0,by6803_zeroCross);

  locals.initDone = TRUE;
}

static void by6803_init1(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit1;
  	locals.SOUNDEXIT = by6803_sndexit1;
	locals.SOUNDCOMMAND = by6803_sndcmd1;
	locals.SOUNDDIAG = by6803_snddiag1;
	locals.DISPSTROBE = by6803_dispStrobe1;
	locals.SEGWRITE = by6803_segwrite1;
	locals.DISPDATA = by6803_dispdata1;
	by6803_init_common(1);
}
static void by6803_init1a(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit1a;
  	locals.SOUNDEXIT = by6803_sndexit1;
	locals.SOUNDCOMMAND = by6803_sndcmd1a;
	locals.SOUNDDIAG = by6803_snddiag1;
	locals.DISPSTROBE = by6803_dispStrobe1;
	locals.SEGWRITE = by6803_segwrite1;
	locals.DISPDATA = by6803_dispdata1;
	by6803_init_common(1);
}
static void by6803_init2(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit2;
  	locals.SOUNDEXIT = by6803_sndexit2;
	locals.SOUNDCOMMAND = by6803_sndcmd2;
	locals.SOUNDDIAG = by6803_snddiag2;
	locals.DISPSTROBE = by6803_dispStrobe2;
	locals.SEGWRITE = by6803_segwrite2;
	locals.DISPDATA = by6803_dispdata2;
	by6803_init_common(1);
}
static void by6803_init3(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit3;
  	locals.SOUNDEXIT = by6803_sndexit3;
	locals.SOUNDCOMMAND = by6803_sndcmd3;
	locals.SOUNDDIAG = by6803_snddiag3;
	locals.DISPSTROBE = by6803_dispStrobe2;
	locals.SEGWRITE = by6803_segwrite2;
	locals.DISPDATA = by6803_dispdata2;
	by6803_init_common(1);
}
static void by6803_init3a(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit3;
  	locals.SOUNDEXIT = by6803_sndexit3;
	locals.SOUNDCOMMAND = by6803_sndcmd3;
	locals.SOUNDDIAG = by6803_snddiag3;
	locals.DISPSTROBE = by6803_dispStrobe2;
	locals.SEGWRITE = by6803_segwrite2;
	locals.DISPDATA = by6803_dispdata2;
	by6803_init_common(0);
}
static void by6803_init4(void) {
	if (locals.initDone) CORE_DOEXIT(by6803_exit);
	memset(&locals, 0, sizeof(locals));
	locals.SOUNDINIT = by6803_sndinit4;
  	locals.SOUNDEXIT = by6803_sndexit4;
	locals.SOUNDCOMMAND = by6803_sndcmd4;
	locals.SOUNDDIAG = by6803_snddiag4;
	locals.DISPSTROBE = by6803_dispStrobe2;
	locals.SEGWRITE = by6803_segwrite2;
	locals.DISPDATA = by6803_dispdata2;
	by6803_init_common(0);
}

static void by6803_exit(void) {
#ifdef PINMAME_EXIT
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
#endif
  if (coreGlobals.soundEn) locals.SOUNDEXIT();
  core_exit();
}

//NA?
static READ_HANDLER(port1_r) {
	int data = 0;
	//logerror("%x: port 1 read: %x\n",cpu_getpreviouspc(),data);
	return data;
}

//Read Phase A Status? (Not sure if this is used)
//JW7 (PB3) should not be set, which breaks the gnd connection, so we set the line high.
static READ_HANDLER(port2_r) {
	//mlogerror("%x: port 2 read: %x\n",cpu_getpreviouspc(),data);
	return (locals.phase_a && !locals.p21) | 0x18;
}

//Sound Data (PB0-3 only connected on schem, but later generations may use all 8 bits)
static WRITE_HANDLER(port1_w) {
	locals.snddata = data;
	//printf("snddata: port 1 write = %x\n",data);
}

//Diagnostic LED & Sound Interrupt
static WRITE_HANDLER(port2_w) {
	int sndint = (data>>4)&1;
	locals.diagnosticLed=((data>>2)&1);
	//Trigger Sound command on positive edge
	if(!locals.sndint && sndint)
		locals.SOUNDCOMMAND(0,locals.snddata);
	locals.sndint = sndint;
	//logerror("port 2 write = %x\n",data);
	locals.p21 = data & 0x02;
	cpu_set_irq_line(0, M6800_TIN_LINE, (locals.phase_a<2 && !locals.p21) ? ASSERT_LINE : CLEAR_LINE);
}

void BY6803_UpdateSoundLED(int data){
	locals.sounddiagnosticLed = data;
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
(in) P20 = Measure Phase A (In conjunction w/P21 output?)
(in) P21 = NA?
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
	{ 0x0020, 0x0023, pia_0_r },
	{ 0x0040, 0x0043, pia_1_r },
	{ 0x0080, 0x00ff, MRA_RAM },	/*Internal 128K RAM*/
	{ 0x1000, 0x17ff, MRA_RAM },	/*External RAM*/
	{ 0x8000, 0xffff, MRA_ROM },	/*U2 & U3 ROM */
MEMORY_END

static MEMORY_WRITE_START(by6803_writemem)
	{ 0x0000, 0x001f, m6803_internal_registers_w },
	{ 0x0020, 0x0023, pia_0_w },
	{ 0x0040, 0x0043, pia_1_w },
	{ 0x0080, 0x00ff, MWA_RAM },	/*Internal 128K RAM*/
	{ 0x1000, 0x17ff, MWA_RAM },	/*External RAM*/
	{ 0x8000, 0xffff, MWA_ROM },	/*U2 & U3 ROM */
MEMORY_END

static PORT_READ_START( by6803_readport )
	{ M6803_PORT1, M6803_PORT1, port1_r },
	{ M6803_PORT2, M6803_PORT2, port2_r },
PORT_END

static PORT_WRITE_START( by6803_writeport )
	{ M6803_PORT1, M6803_PORT1, port1_w },
	{ M6803_PORT2, M6803_PORT2, port2_w },
PORT_END

//6803 - Generation 1 Sound (Squawk & Talk)
struct MachineDriver machine_driver_by6803S1 = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU1},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init1, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN1_SOUND},
  by6803_nvram
};

//6803 - Generation 1A Sound (Cheap Squeak)
struct MachineDriver machine_driver_by6803S1a = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU1A},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init1a, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN1A_SOUND},
  by6803_nvram
};

//6803 - Generation 2 Sound (Turbo Cheap Squeak)
struct MachineDriver machine_driver_by6803S2 = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU2},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init2, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN2_SOUND},
  by6803_nvram
};
//6803 - Generation 2A Sound (Turbo Cheap Squeak)
struct MachineDriver machine_driver_by6803S2a = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU2A},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init2, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN2_SOUND},
  by6803_nvram
};
//6803 - Generation 3 Sound (Sounds Deluxe) with keypad
struct MachineDriver machine_driver_by6803S3 = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU3},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init3, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN3_SOUND},
  by6803_nvram
};
//6803 - Generation 3 Sound (Sounds Deluxe) without keypad
struct MachineDriver machine_driver_by6803S3a = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU3},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init3a, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN3_SOUND},
  by6803_nvram
};
//6803 - Generation 4 Sound (Williams System 11C) without keypad
struct MachineDriver machine_driver_by6803S4 = {
  {{  CPU_M6803, 3580000/4, /* 3.58/4 = 900hz */
      by6803_readmem, by6803_writemem, by6803_readport, by6803_writeport,
      by6803_vblank, 1, by6803_irq, BY6803_IRQFREQ
  }
  BY6803_SOUNDCPU4},
  BY6803_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, by6803_init4, CORE_EXITFUNC(by6803_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {BY6803_GEN4_SOUND},
  by6803_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void by6803_nvram(void *file, int write) {
  core_nvram(file, write, memory_region(BY6803_MEMREG_CPU)+0x1000, 0x800,0xff);
}


