/* Bally Video/Pinball Combination Hardware Emulation
   by Steve Ellenoff (01/16/2002)

   Games: Baby Pacman (1982)
          Granny & The Gators (1984)

   Hardware:
   (Both Games)
   MPU Board: MPU-133 (Equivalent of MPU-35 except for 1 resistor to diode change) - See note below

   (BabyPacman Only):
   VIDIOT Board: Handles Video/Joystick Switchs/Sound Board
   Chips:		(1 x TMS9928 Video Chip), 6809 CPU (Video), 6803 (Sound), 6821 PIA

   (Granny & Gators Only):
   VIDIOT DELUXE:Handles Video/Joystick and Communication to Cheap Squeak board
   Chips:		(2 x TMS9928 Video Chip - Master/Slave configuration), 6809 CPU, 6821 PIA
   Cheap Squeak Sound Board

   *EMULATION ISSUES*
   Baby Pac: Start up sounds during test flashes is wrong.. should play pac tune?

   G & G: Color blending is not accurately emulated, but transparency is simulated

   Interesting Tech Note:

   The sound board integrated into the vidiot, appears to be
   identical to what later became the separate Cheap Squeak board used on later model
   bally MPU-35 games, as well as Granny & Gators!

   Note from RGP:

		Bally video pins use an AS-2518-133 mpu.
		I believe that some late games such as Grand Slam
		may have also used the AS-2518-133 mpu.

		This is the same as an  AS-2518-35  mpu except that
		R113 which is fed from J4 by +43v is now CR52 a 1N4148
		diode fed by 6.3vac from General Illumination.
		To use the -133 as a -35 change CR52 to be
		a 2k ohm 1/4 watt resistor or to use a -35 as a -133
		change R113 from a resistor to a 1N4148 diode.
		The cathode (ie: stripe end ) connects to capacitor C1
		(it is the end away from J4).

  *All addresses are shown for baby pac
  -------------------------------------
  6800 vectors:
  RES:  597d
  NMI:  5950
  SWI?: 597d
  IRQ:  5955
  FIRQ: 1f87

  6809 vectors:
  RES:  FFFE-F (E016)
  NMI:  FFFC-D (E17C) Used - Test Switch
  SWI:  FFFA-B (801F) Not Used
  IRQ:  FFF8-9 (801C) Used - VDP Triggered
  FIRQ: FFF6-7 (8019) Used - PIA Triggered
  SWI2: FFF4-5 (8016) Not Used
  SWI3: FFF2-3 (8013) *Valid Code?
  Rsvd: FFF0-1 (8010) *Valid Code?

  6803 vectors:
  RES: FFFE-F
  SWI: FFFA-B Software Interrupt (Not Used)
  NMI: FFFC-D (Used)
  IRQ: FFF8-9 (Not Used)
  ICF: FFF6-7 (Input Capture)~IRQ2 (FA6F)
  OCF: FFF4-5 (Output Compare)~IRQ2 (Not Used)
  TOF: FFF2-3 (Timer Overflow)~IRQ2 (Not Used)
  SCI: FFF0-1 (Input Capture)~IRQ2  (Not Used)
*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "vidhrdw/tms9928a.h"
#include "core.h"
#include "byvidpin.h"
#include "snd_cmd.h"

static int cmdnum = 0;

#if 0
	#define mlogerror logerror
#else
	#define mlogerror printf
#endif

#define BYVP_VCPUNO 1		/* Video CPU # */
#define BYVP_SCPUNO 2		/* Sound CPU # */

#define BYVP_VBLANKFREQ    60 /* VBLANK frequency */
#define BYVP_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define BYVP_ZCFREQ       120 /* Zero cross frequency */

static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2, p2_a;
  int lampadr1;
  UINT32 solenoids;
  int diagnosticLed;
  int diagnosticLedV;		//Diagnostic LED for Vidiot Board
  int vblankCount;
  int enable_output;		//Enable Output to Main CPU Flag
  int enable_input;			//Enable Input From Main CPU Flag
  int status_enable;		//Enable Reading of Status Flag
  int vidiot_status;		//Status of Vidiot
  int vidiot_u1_latch;		//U1 Latch -> Data going to Main CPU from Vidiot
  int vidiot_u2_latch;		//U2 Latch -> Data coming from Main CPU
  int u7_portb;				//U7 Port B data
  int u7_portcb2;			//U7 Port CB2
  int snddata;				//Sound Data to 6803
  int snddata_lo;			//Lo Nibble Sound Command
  int snddata_hi;			//Hi Nibble Sound Command
  int lasttin;				//Track Last TIN IRQ Edge
  int phase_a;				//Track Phase A status
  void (*SwitchMapping)(int *);		//Pointer to Switch Mapping
} locals;

extern tPMoptions pmoptions;

static void piaIrq(int state) { cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE); }

static void byVP_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
	int lampdata = ((locals.p0_a>>4)^0x0f) & 0x03; //only 2 lamp drivers
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+4*board];
    int bit = 1<<(lampadr & 0x07);
    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}

/* PIA0:A-W  Control what is read from PIA0:B
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-4: Switch Strobe(Columns)
(out) PA4-7: BCD Lamp Data
(out) PA5:   S01-S08 Dip Enable
(out) PA6:   S09-S16 Dip Enable
(out) PA7:   S17-S24 Dip Enable
(out) PA0-7: Vidiot Input Data (When Latch Input Flag is set?) */
static WRITE_HANDLER(pia0a_w) {
  locals.p0_a = data;	//Controls Strobing

  //Write Data To Vidiot Latch Pre-Buffers - Only When Latch Input is Low
  if(locals.enable_input == 0) {
	/*For some reason, code inverts lower nibble*/
	data = (data&0xf0) | (~data & 0x0f);
	locals.vidiot_u2_latch = data;
	//logerror("%x: Writing %x to vidiot u2 buffers: enable_input = %x\n",activecpu_get_previouspc(),data, locals.enable_input);
  }
  byVP_lampStrobe(!locals.phase_a,locals.lampadr1);
}

/* PIA1:A-W: Communications to Vidiot
(out) PA0:	 N/A
(out) PA1:   Video Enable Output	(Active Low)
(out) PA2:   Video Latch Input Data (When Low, Signals to Vidiot, that data is coming, When Transition to High, Latches Data)
(out) PA3:   Video Status Enable	(Active Low)
(out) PA4-7: N/A*/
static WRITE_HANDLER(pia1a_w) {
  int tmp_input = locals.enable_input;
  locals.enable_output = (data>>1)&1;
  locals.enable_input  = (data>>2)&1;
  locals.status_enable = (data>>3)&1;

  //Latch data from pre-buffer to U2 on positive edge (We don't need to code this)
  if(locals.enable_input & ~tmp_input) {
    logerror("%x: Latching data to Vidiot U2: data = %x\n",activecpu_get_previouspc(),locals.vidiot_u2_latch);
  }

  //Update Vidiot PIA - This will trigger an IRQ also, depending on how the ca lines are configured
  pia_set_input_ca1(2, locals.enable_output);
  pia_set_input_ca2(2, locals.enable_input);
}

/* PIA0:B-R  Get Data depending on PIA0:A
(in)  PB0-1: Vidiot Status Bits 0,1 INVERTED - (When Status Data Enabled Flag is Set)
(in)  PB0-7: Vidiot Output Data (When Enable Output Flag is Set(Active Low) )
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  PB0-7: Dip Returns */
static READ_HANDLER(pia0b_r) {
  //Enable Status must be low to return data..
  if (!locals.status_enable) {
	    //logerror("%x: MPU: reading vidiot status %x\n",activecpu_get_previouspc(),locals.vidiot_status);
		return locals.vidiot_status;

  }
  //Enable Output must be low to return data..
  if (!locals.enable_output) {
	    //logerror("%x: MPU: reading vidiot data %x\n",activecpu_get_previouspc(),locals.vidiot_u1_latch);
		return locals.vidiot_u1_latch;
  }

  if (locals.p0_a & 0x20) return core_getDip(0); // DIP#1-8
  if (locals.p0_a & 0x40) return core_getDip(1); // DIP#9-16
  if (locals.p0_a & 0x80) return core_getDip(2); // DIP#17-24
  if (locals.p0_cb2)      return core_getDip(3); // DIP#25-32
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x80)>>2));
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}
/* PIA1:CA2-W Diagnostic LED */
static WRITE_HANDLER(pia1ca2_w) {
  locals.diagnosticLed = data;
  locals.p1_ca2 = data;
}

/* PIA1:B-W Solenoid */
static WRITE_HANDLER(pia1b_w) {
  UINT16 mask = (UINT16)~(0xffff);
  UINT16 sols = 0;
  locals.p1_b = data;
  //Solenoids 1-7 (same as by35)
  sols = (1<<(data & 0x0f)) & 0xff;
  //Solenoids 8-16 are weird (baby only uses 10, granny only 5!)
  if((data & 0x0f) == 0x0f)
	sols |= (((~core_revbyte(data))>>1) & 0x7f)<<7;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  locals.solenoids |= sols;
}

/* PIA1:CB2-W Solenoid Bank Select */
static WRITE_HANDLER(pia1cb2_w) {
  locals.p1_cb2 = data;
}

static INTERRUPT_GEN(byVP_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BYVP_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BYVP_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BYVP_DISPLAYSMOOTH) == 0) {
    /*update leds*/
    coreGlobals.diagnosticLed = (locals.diagnosticLedV<<1)|
								(locals.diagnosticLed);
	//Why is this needed?
    locals.diagnosticLed = 0;
	locals.diagnosticLedV = 0;
  }
  core_updateSw(core_getSol(19));
}

static SWITCH_UPDATE(byVP) {
  /*Call game specific switch mappings*/
  if (inports) locals.SwitchMapping(inports);

  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(BYVP_SWCPUDIAG))  cpu_set_nmi_line(0, PULSE_LINE);
  if (core_getSw(BYVP_SWSOUNDDIAG)) cpu_set_nmi_line(BYVP_SCPUNO, PULSE_LINE);
  if (core_getSw(BYVP_SWVIDEODIAG)) cpu_set_nmi_line(BYVP_VCPUNO, PULSE_LINE);

  /*-- coin door switches --*/
  pia_set_input_ca1(0, !core_getSw(BYVP_SWSELFTEST));
}

static void BabySwitch(int *inports) {
	//Load switches that don't belong in matrix (Diagnostics, Joystick, etc)
	coreGlobals.swMatrix[0] = inports[BYVP_COMINPORT] & 0xff;

	//Clear only bits we're using
	coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x24));
	coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0xc3));

	//Start Player 2
    coreGlobals.swMatrix[1] |= (inports[BYVP_COMINPORT]>>6) & 0x04;
	//Start Player 1
    coreGlobals.swMatrix[1] |= (inports[BYVP_COMINPORT]>>4) & 0x20;
	//Coin Chute #1 & #2
    coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>10) & 0x03;
	//Ball Tilt/Slam Tilt
	coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>6) & 0xc0;
}

static void GrannySwitch(int *inports) {
	//Load switches that don't belong in matrix (Diagnostics, Joystick, etc)
	coreGlobals.swMatrix[0] = inports[BYVP_COMINPORT] & 0xff;

	//Clear only bits we're using
	coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0xf3));

	//Start Player 2
    coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>4) & 0x10;
	//Start Player 1
    coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>4) & 0x20;
	//Coin Chute #1 & #2
    coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>10) & 0x03;
	//Ball Tilt/Slam Tilt
	coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>6) & 0xc0;
	//Power (Does not belong in the matrix, so we start at switch #41, since 40 is last used switch)
	coreGlobals.swMatrix[6] &= ~(0x1);
	coreGlobals.swMatrix[6] |= (inports[BYVP_COMINPORT]>>14) & 0x1;
}




/* PIA2:B Read */
// Video Switch Returns (Bits 5-7 not connected)
static READ_HANDLER(pia2b_r) {
	//logerror("VID: Reading Switch Returns from %x\n",locals.p2_a);
	if(locals.p2_a & 0x80)
		return coreGlobals.swMatrix[0]&0xf0;
	if(locals.p2_a & 0x40)
		return (coreGlobals.swMatrix[6]&0x1)<<7;	//Power Button for Granny - Mapped to switch #41, since only 40 switches are used in the game.
	return 0;
}

/* PIA2:A Write */
//PA0-3: Status Data Inverted! (Only Bits 0 & 1 Used however)
//PA4-7: Video Switch Strobes (Only Bit 7 is used however)
static WRITE_HANDLER(pia2a_w) {
	locals.p2_a = data;
	locals.vidiot_status = (~data) & 0x03;
	//logerror("%x:VID: Setting status to %4x\n",activecpu_get_previouspc(),locals.vidiot_status);
	//logerror("%x:Setting Video Switch Strobe to %x\n",activecpu_get_previouspc(),data>>4);
}

/* PIA2:B Write*/
//PB0-3: Output to 6803 CPU
//PB4-7: N/A
static WRITE_HANDLER(pia2b_w) {
	//Store the data written here for later use
	locals.snddata = data & 0x0f;
}

/* PIA2:CB2 Write */
// Diagnostic LED & Sound Strobe to 6803
static WRITE_HANDLER(pia2cb2_w) {
	locals.diagnosticLedV = data;

	if(data & ~locals.lasttin) {	//Rising Edge Triggers the IRQ
		cmdnum = 0;
		//Low Nibble was written just prior to raising the TIN_IRQ line
		locals.snddata_lo = locals.snddata;

		//Although the real hardware sets the line here, we need to wait for the 2nd nibble
	}
	else {
		//Hi Nibble was written just prior to clearing the TIN_IRQ line
		locals.snddata_hi = locals.snddata;

		//Log the full command
		snd_cmd_log((locals.snddata_hi<<4) | (locals.snddata_lo));

		//Although the real hardware clears the line here, we pulse it, since
		//we've now collected both lo & hi nibbles!
		cpu_set_irq_line(BYVP_SCPUNO, M6800_TIN_LINE, PULSE_LINE);
	}
	locals.lasttin = data;
}

//VIDEO PIA IRQ - TRIGGER VIDEO CPU (6809) FIRQ
static void pia2Irq(int state) {
  cpu_set_irq_line(BYVP_VCPUNO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

//Vidiot Reads from U2 Latch (Data coming from CPU)
static READ_HANDLER(latch_r)
{
	int data = locals.vidiot_u2_latch;
	//logerror("%x: LATCH_R: offset=%x, data=%x\n",activecpu_get_previouspc(),offset,data);
	return data;
}
//Vidiot Writes to U1 Latch (Data going to CPU)
static WRITE_HANDLER(latch_w)
{
	locals.vidiot_u1_latch = data;
	//logerror("%x: LATCH_W: offset=%x, data=%x\n",activecpu_get_previouspc(),offset,data);
}

/*PIA 0*/
/*PIA U10:
(in)  PB0-1: Vidiot Status Bits 0,1 INVERTED - (When Status Data Enabled Flag is Set)
(in)  PB0-7: Vidiot Output Data (When Enable Output Flag is Set)
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  PB0-7: Dip Returns
(in)  CA1:   Self Test Switch
(in)  CB1:   Zero Cross Detection
(in)  CA2:   N/A
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-4: Switch Strobe(Columns)
(out) PA4-7: BCD Lamp Data
(out) PA5:   S01-S08 Dip Enable
(out) PA6:   S09-S16 Dip Enable
(out) PA7:   S17-S24 Dip Enable
(out) PA0-7: Vidiot Input Data (When Latch Input Flag is set?)
(out) CA2:   NA
(out) CB2:   S25-S32 Dip Enable (shared with Lamp Strobe?)
(out) CB2:   Lamp Strobe
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/

/*PIA 1*/
/*PIA U11:
(in)  PA0-7	 N/A
(in)  PB0-7  N/A
(in)  CA1:   Display Interrupt Generator
(in)  CB1:	 J5 - Pin 32 (Marked as Test Connector)
(in)  CA2:	 N/A
(in)  CB2:	 N/A
(out) PA0:	 N/A
(out) PA1:   Video Enable Output
(out) PA2:   Video Latch Input Data
(out) PA3:   Video Status Enable
(out) PA4-7: N/A
(out) PB0-3: Momentary Solenoid
(out) PB4-7: Continuous Solenoid
(out) CA2:	 Diag LED
(out) CB2:   Solenoid Bank Select
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/

/*---VIDIOT BOARD---*/

/*PIA 2*/
/*PIA U7:
(in)  PA0-7	 N/A
(in)  PB0-7  Video Switch Returns (Bits 5-7 not connected)
(in)  PB0-3  ?? Input from 6803 CPU?
(in)  CA1:   Enable Data Output to Main CPU
(in)  CB1:	 N/A
(in)  CA2:	 Main CPU Latch Data Input
(in)  CB2:	 J2-1 = N/A?
(out) PA0-3: Status Data (Only Bits 0 & 1 Used however) - Inverted before reaching MPU
(out) PA4-7: Video Switch Strobes (Only Bit 7 is used however)
(out) PB0-3: Output to 6803 CPU
(out) PB4-7: N/A
(out) CA2:	 N/A
(out) CB2:   6803 Data Strobe & LED
	  IRQ:	 Wired to Video 6809 CPU FIRQ*/
static struct pia6821_interface piaIntf[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, 0 ,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,pia2b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia2a_w,pia2b_w,0,pia2cb2_w,
/* IRQ: A/B              */  pia2Irq,pia2Irq
}};

//Read Display Interrupt Generator
static INTERRUPT_GEN(byVP_irq) {
  static int last = 0;
  pia_set_input_ca1(1, last = !last);
}

//Send a command manually
static WRITE_HANDLER(byVP_soundCmd) {
	snd_cmd_log(data);
	if(data) {
		locals.snddata_lo = (data & 0x0f) << 1;
		locals.snddata_hi = (data >> 4) << 1;
		cmdnum = 0;
		snd_cmd_log(data);
		/*set 6803 P20 line - Thus triggering a sound command*/
		cpu_set_irq_line(BYVP_SCPUNO, M6800_TIN_LINE, PULSE_LINE);
	}
}

/* I read on a post in rgp, that they used both ends of the zero cross, so we emulate it */
static void byVP_zeroCross(int data) {
  locals.phase_a = !locals.phase_a;
  /*- toggle zero/detection circuit-*/
  pia_set_input_cb1(0,locals.phase_a);
}

static void byVP_common(void) {
//  if (core_init(&byVPData)) return;
  memset(&locals, 0, sizeof(locals));

  /* init PIAs */
  pia_config(0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  pia_config(2, PIA_STANDARD_ORDERING, &piaIntf[2]);
  pia_reset();
  locals.vblankCount = 1;
}
static MACHINE_INIT(byVP1) {
	byVP_common();
	locals.SwitchMapping = BabySwitch;
}
static MACHINE_INIT(byVP2) {
	byVP_common();
	locals.SwitchMapping = GrannySwitch;
}

static MACHINE_STOP(byVP) {
//	core_exit();
}


/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *byVP_CMOS;

static WRITE_HANDLER(byVP_CMOS_w) { byVP_CMOS[offset] = data; }

static NVRAM_HANDLER(byVP) {
  core_nvram(file, read_or_write, byVP_CMOS, 0x100,0xff);
}

/* P10-P17 = What is this for? Only used in G&G*/
static READ_HANDLER(sound_port1_r) {
	return 0;
}

/* P21-24 = Video U7(PB0-PB3)
   NOTE: Sound commands are sent as 2 nibbles, first low byte, then high
         However, it reads the first low nibble 2x, then reads the high nibble
         Since Port 2 begins with P20, we must << 1 the sound commands!
*/
static READ_HANDLER(sound_port2_r) {
	cmdnum++;
	if( cmdnum < 3) //If it's the 1st or 2nd time reading the port..
		return locals.snddata_lo<<1;
	else
		{
			cmdnum = 0;
			return locals.snddata_hi<<1;
		}
}

/* Port 1 = DAC Data*/
static WRITE_HANDLER(sound_port1_w) { DAC_0_data_w(offset,data); }

/* P20(Bit 0) = LED*/
static WRITE_HANDLER(sound_port2_w) {
	locals.diagnosticLedV = (data>>0)&1;
}


//COMMENTS BELOW are directly from MESS source code..
/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static void by_vdp_interrupt (int state)
{
	static int last_state = 0;

	//logerror("vdp_int: state = %x\n",state);

    /* only if it goes up */
	if (state && !last_state) {
		//logerror("VDP Caused Vidiot CPU IRQ\n");
		cpu_set_irq_line(BYVP_VCPUNO, M6809_IRQ_LINE, PULSE_LINE);
	}
	last_state = state;
}

//Video CPU Vertical Blank
static INTERRUPT_GEN(byVP_vvblank) {
    TMS9928A_interrupt(0);
	//The IRQ must be triggered constantly for the video cpu to read/write to the latch
	//I'm not sure why doing it the way it was coded from MESS does not work.
//	return M6809_INT_IRQ;
}

static INTERRUPT_GEN(byVP1_vvblank) {
    TMS9928A_interrupt(0);
	TMS9928A_interrupt(1);
	//The IRQ must be triggered constantly for the video cpu to read/write to the latch
	//I'm not sure why doing it the way it was coded from MESS does not work.
//	return M6809_INT_IRQ;
}

static READ_HANDLER(vdp0_r) {
	//logerror("vdp0_r: offset=%x\n",offset);
	if(offset == 0)
		return TMS9928A_vram_0_r(offset);
	else
		return TMS9928A_register_0_r(offset);
}

static WRITE_HANDLER(vdp0_w) {
	//logerror("%x:vdp0_w: offset=%x, data=%x\n",activecpu_get_previouspc(),offset,data);
	if(offset==0)
		TMS9928A_vram_0_w(offset,data);
	else
		TMS9928A_register_0_w(offset,data);
}

static READ_HANDLER(vdp1_r) {
	//logerror("vdp1_r: offset=%x\n",offset);
	if(offset == 0)
		return TMS9928A_vram_1_r(offset);
	else
		return TMS9928A_register_1_r(offset);
}

static WRITE_HANDLER(vdp1_w) {
	//logerror("%x:vdp1_w: offset=%x, data=%x\n",activecpu_get_previouspc(),offset,data);
	if(offset==0)
		TMS9928A_vram_1_w(offset,data);
	else
		TMS9928A_register_1_w(offset,data);
}

static WRITE_HANDLER(both_w) {
	//logerror("%x:vdp1_w: offset=%x, data=%x\n",activecpu_get_previouspc(),offset,data);
	if(offset==0) {
		TMS9928A_vram_0_w(offset,data);
		TMS9928A_vram_1_w(offset,data);
	}
	else {
		TMS9928A_register_0_w(offset,data);
		TMS9928A_register_1_w(offset,data);
	}
}


static VIDEO_START(by)
{
	if (TMS9928A_start(0,TMS99x8A, 0x4000)) return 1;
	TMS9928A_int_callback(0,by_vdp_interrupt);
	return 0;
}

static VIDEO_START(by1)
{
	if (TMS9928A_start(0,TMS99x8A, 0x4000)) return 1;
	if (TMS9928A_start(1,TMS99x8A, 0x4000)) return 1;
	/*Only master chip controls interrupt*/
	TMS9928A_int_callback(0,by_vdp_interrupt);
	return 0;
}

static VIDEO_STOP(by)
{
  TMS9928A_stop(1);
}

static VIDEO_STOP(by1)
{
  TMS9928A_stop(2);
}

static void by_drawStatus (struct mame_bitmap *bmp);

static VIDEO_UPDATE(by)
{
	TMS9928A_refresh(1, bitmap, 1);
	if(!pmoptions.dmd_only)
		by_drawStatus(bitmap);
}

//#define TESTGRANNY 1

static VIDEO_UPDATE(by1)
{
#ifndef TESTGRANNY

	TMS9928A_refresh(2, bitmap, 1);
	if(!pmoptions.dmd_only)
		by_drawStatus(bitmap);

#define GRAPHICSETUP \
		MDRV_SCREEN_SIZE(256, 192) \
		MDRV_VISIBLE_AREA(0, 255, 0, 191)

#else /* TESTGRANNY */

	TMS9928A_refresh_test(bitmap, 1);
	if(!pmoptions.dmd_only)
		by_drawStatus(bitmap);

#define GRAPHICSETUP \
  		MDRV_SCREEN_SIZE(256*3, 192) \
  		MDRV_VISIBLE_AREA(0, 256*3-1, 0, 191)

#endif /* TESTGRANY */
}

static struct DACinterface by_dacInt =
  { 1, { 50 }};

/*-----------------------------------
/  Memory map for MAIN CPU board
/------------------------------------*/
static MEMORY_READ_START(byVP_readmem)
  { 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_0_r }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_1_r }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x0300, 0x031c, MRA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_writemem)
  { 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_0_w }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_1_w }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, byVP_CMOS_w, &byVP_CMOS }, /* CMOS Battery Backed*/
  { 0x0300, 0x031c, MWA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

/*----------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Board)
/----------------------------------------------------*/
static MEMORY_READ_START(byVP_video_readmem)
	{ 0x0000, 0x1fff, latch_r },
	{ 0x2000, 0x2003, pia_2_r }, /* U7 PIA */
	{ 0x4000, 0x4001, vdp0_r },  /* U16 VDP*/
	{ 0x6000, 0x6400, MRA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_video_writemem)
	{ 0x0000, 0x1fff, latch_w },
	{ 0x2000, 0x2003, pia_2_w }, /* U7 PIA */
	{ 0x4000, 0x4001, vdp0_w },  /* U16 VDP*/
	{ 0x6000, 0x6400, MWA_RAM }, /* U13&U14 1024x4 Byte Ram*/
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/*-------------------------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Deluxe Board) - G&G
/------------------------------------------------------------------*/
static MEMORY_READ_START(byVP2_video_readmem)
	{ 0x0000, 0x0001, latch_r },
	{ 0x0002, 0x0003, vdp0_r },  /* VDP MASTER */
	{ 0x0004, 0x0005, vdp1_r },  /* VDP SLAVE  */
	{ 0x0008, 0x000b, pia_2_r }, /* PIA */
	{ 0x0300, 0x031c, MRA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
	{ 0x2000, 0x27ff, MRA_RAM }, /* 2K RAM */
	{ 0x2801, 0x2801, MRA_RAM }, /* ?????? */
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP2_video_writemem)
	{ 0x0000, 0x0001, latch_w },
	{ 0x0002, 0x0003, vdp0_w },  /* VDP MASTER */
	{ 0x0004, 0x0005, vdp1_w },  /* VDP SLAVE  */
	{ 0x0006, 0x0007, both_w },  /* WRITE TO BOTH  */
	{ 0x0008, 0x000b, pia_2_w }, /* PIA */
	{ 0x0300, 0x031c, MWA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
	{ 0x2000, 0x27ff, MWA_RAM }, /* 2K RAM*/
	{ 0x2801, 0x2801, MWA_RAM }, /* ????????? */
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/*-----------------------------------------------------
/  Memory map for SOUND CPU (Located on Vidiot Board)
/------------------------------------------------------
NMI: = Sound Test Switch
IRQ: = NA
Port 1:
(out)P10 = DAC 8
(out)P11 = DAC 7
(out)P12 = DAC 6
(out)P13 = DAC 5
(out)P14 = DAC 4
(out)P15 = DAC 3
(out)P16 = DAC 2
(out)P17 = DAC 1
Port 2:
(in)P20 = U7(CB2)->Data Strobe Irq
(in)P21 = U7(PB0)->DATA Nibble
(in)P22 = U7(PB1)->DATA Nibble
(in)P23 = U7(PB2)->DATA Nibble
(in)P24 = U7(PB3)->DATA Nibble
(out)P20 = LED & U7(CB2)
*/
static MEMORY_READ_START(byVP_sound_readmem)
	{ 0x0000, 0x001f, m6803_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM },	/*Internal 128K RAM*/
	{ 0xe000, 0xffff, MRA_ROM },	/* U29 ROM */
MEMORY_END

static MEMORY_WRITE_START(byVP_sound_writemem)
	{ 0x0000, 0x001f, m6803_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM },	/*Internal 128K RAM*/
	{ 0xe000, 0xffff, MWA_ROM },	/* U29 ROM */
MEMORY_END

static PORT_READ_START( byVP_sound_readport )
	{ M6803_PORT1, M6803_PORT1, sound_port1_r },
	{ M6803_PORT2, M6803_PORT2, sound_port2_r },
PORT_END

static PORT_WRITE_START( byVP_sound_writeport )
	{ M6803_PORT1, M6803_PORT1, sound_port1_w },
	{ M6803_PORT2, M6803_PORT2, sound_port2_w },
PORT_END

#if 0
static core_tData byVPData = {
  32, /* 32 Dips */
  byVP_updSw, 2 | DIAGLED_VERTICAL, byVP_soundCmd, "byVP",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};
#endif

/*--------------------*/
/* Machine Definition */
/*--------------------*/
/*BABYPAC HARDWARE*/
MACHINE_DRIVER_START(byVP)
  MDRV_IMPORT_FROM(PinMAME)

  MDRV_CPU_ADD_TAG("mcpu", M6800, 3580000/4)
  MDRV_CPU_MEMORY(byVP_readmem, byVP_writemem)
  MDRV_CPU_VBLANK_INT(byVP_vblank, 1)
  MDRV_CPU_PERIODIC_INT(byVP_irq, BYVP_IRQFREQ)

  MDRV_CPU_ADD_TAG("vcpu", M6809, 3580000/4)
  MDRV_CPU_MEMORY(byVP_video_readmem, byVP_video_writemem)
  MDRV_CPU_VBLANK_INT(byVP_vvblank, 1)

  MDRV_CPU_ADD_TAG("scpu", M6803, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(byVP_sound_readmem, byVP_sound_writemem)
  MDRV_CPU_PORTS(byVP_sound_readport,byVP_sound_writeport)

  MDRV_TIMER_ADD(byVP_zeroCross, BYVP_ZCFREQ)
  MDRV_FRAMES_PER_SECOND(BYVP_VBLANKFREQ)
  MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
  MDRV_INTERLEAVE(50)
  MDRV_CORE_INIT_RESET_STOP(byVP1,NULL,byVP)
  MDRV_DIPS(32)
  MDRV_DIAGNOSTIC_LEDV(2)
  MDRV_SWITCH_UPDATE(byVP)
  MDRV_NVRAM_HANDLER(byVP)

  /* video hardware */
  GRAPHICSETUP
  MDRV_GFXDECODE(0)
  MDRV_PALETTE_LENGTH(TMS9928A_PALETTE_SIZE)
  MDRV_COLORTABLE_LENGTH(TMS9928A_COLORTABLE_SIZE)
  MDRV_PALETTE_INIT(TMS9928A)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE)
  MDRV_VIDEO_START(by)
  MDRV_VIDEO_STOP(by)
  MDRV_VIDEO_UPDATE(by)

  /* sound hardware */
  MDRV_SOUND_CMD(byVP_soundCmd)
  MDRV_SOUND_CMDHEADING("byVP")
  MDRV_SOUND_ADD(DAC, by_dacInt)
MACHINE_DRIVER_END

#if 0
 {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900kHz */
      byVP_readmem, byVP_writemem, NULL, NULL,
      byVP_vblank, 1, byVP_irq, BYVP_IRQFREQ
  },
  {  CPU_M6809, 3580000/4, /* 3.58/4 = 900kHz */
      byVP_video_readmem, byVP_video_writemem, NULL, NULL,
      byVP_vvblank, 1
  },
  {  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4, /* 3.58/4 = 900kHz */
      byVP_sound_readmem, byVP_sound_writemem, byVP_sound_readport, byVP_sound_writeport,
      NULL, 1, NULL, 1
  }},
  BYVP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, byVP_init1, CORE_EXITFUNC(byVP_exit),
  256, 192, { 0, 255, 0, 191 }, \
  0, /* gfxdecodeinfo */\
  TMS9928A_PALETTE_SIZE,\
  TMS9928A_COLORTABLE_SIZE,\
  tms9928A_init_palette,\
  VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
  0,
  by_vh_start,by_vh_stop, by_vh_refresh,
  0,0,0,0, {{SOUND_DAC,&by_dacInt}},
  byVP_nvram
#endif

/*GRANNY & THE GATORS HARDWARE*/
MACHINE_DRIVER_START(byVP2)
  MDRV_IMPORT_FROM(byVP)

  MDRV_CPU_REPLACE("vcpu", M6809, 8000000/4)
  MDRV_CPU_MEMORY(byVP2_video_readmem, byVP2_video_writemem)
  MDRV_CPU_VBLANK_INT(byVP1_vvblank, 1)

  MDRV_CORE_INIT_RESET_STOP(byVP2,NULL,byVP)

  /* video hardware */
  MDRV_VIDEO_START(by1)
  MDRV_VIDEO_STOP(by1)
  MDRV_VIDEO_UPDATE(by1)
MACHINE_DRIVER_END

#if 0
 {{  CPU_M6800, 3580000/4, /* 3.58/4 = 900kHz */
      byVP_readmem, byVP_writemem, NULL, NULL,
      byVP_vblank, 1, byVP_irq, BYVP_IRQFREQ
  },
  {  CPU_M6809, 8000000/4, /* 2MHz */
      byVP2_video_readmem, byVP2_video_writemem, NULL, NULL,
      byVP1_vvblank, 1
  },
  {  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4, /* 3.58/4 = 900kHz */
      byVP_sound_readmem, byVP_sound_writemem, byVP_sound_readport, byVP_sound_writeport,
      NULL, 1, NULL, 1
  }},
  BYVP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, byVP_init2, CORE_EXITFUNC(byVP_exit),
  256, 192, { 0, 255, 0, 191 }, \
  0, /* gfxdecodeinfo */\
  TMS9928A_PALETTE_SIZE,\
  TMS9928A_COLORTABLE_SIZE,\
  tms9928A_init_palette,\
  VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
  0,
  by1_vh_start,by1_vh_stop, by1_vh_refresh,
  0,0,0,0, {{SOUND_DAC,&by_dacInt}},
  byVP_nvram
#endif

/*--------------------------------------------
/ Draw status display
/ Lamps, Switches, Solenoids, Diagnostic LEDs
/---------------------------------------------*/
void by_drawStatus(struct mame_bitmap *bitmap) {
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int firstRow = 0;
  int ii, jj, bits;
  BMTYPE dotColor[2];

  dotColor[0] = CORE_COLOR(6); dotColor[1] = CORE_COLOR(10);
  /*-----------------
  /  Draw the lamps
  /------------------*/
  /*-- Normal square lamp matrix layout --*/
  for (ii = 0; ii < CORE_CUSTLAMPCOL + core_gameData->hw.lampCol; ii++) {
      BMTYPE **line = &lines[firstRow];
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
        line[0][ii*2] = dotColor[bits & 0x01];
        line += 2; bits >>= 1;
      }
  }
  osd_mark_dirty(0,firstRow,16,firstRow+ii*2);

  firstRow += 20;
  /*-------------------
  /  Draw the switches
  /--------------------*/
  for (ii = 0; ii < CORE_CUSTSWCOL+core_gameData->hw.swCol; ii++) {
    BMTYPE **line = &lines[firstRow];
    bits = coreGlobals.swMatrix[ii];

    for (jj = 0; jj < 8; jj++) {
      line[0][ii*2] = dotColor[bits & 0x01];
      line += 2; bits >>= 1;
    }
  }
  osd_mark_dirty(0,firstRow,16,firstRow+ii*2);
  firstRow += 20;

  /*------------------------------
  /  Draw Solenoids and Flashers
  /-------------------------------*/
  firstRow += 5;
  {
    BMTYPE **line = &lines[firstRow];
    UINT64 allSol = core_getAllSol();
    for (ii = 0; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
      line[(ii/8)*2][(ii%8)*2] = dotColor[allSol & 0x01];
      allSol >>= 1;
    }
    osd_mark_dirty(0, firstRow, 16,
        firstRow+((CORE_FIRSTCUSTSOL+core_gameData->hw.custSol+7)/8)*2);
  }

  /*------------------------------*/
  /*-- draw diagnostic LEDs     --*/
  /*------------------------------*/
  firstRow += 20;
  {
    BMTYPE **line = &lines[firstRow];
    bits = coreGlobals.diagnosticLed;

    // Draw LEDS Vertically
    if (coreData->diagLEDs & DIAGLED_VERTICAL) {
       for (ii = 0; ii < (coreData->diagLEDs & ~DIAGLED_VERTICAL); ii++) {
	   	  line[0][5] = dotColor[bits & 0x01];
		  line += 2; bits >>= 1;
	   }
    }
    else { // Draw LEDS Horizontally
		for (ii = 0; ii < coreData->diagLEDs; ii++) {
			line[0][5+ii*2] = dotColor[bits & 0x01];
			bits >>= 1;
			}
	}
  }
  osd_mark_dirty(5, firstRow, 21, firstRow+20);

  firstRow += 25;
  if (core_gameData->gen & GEN_ALLWPC) {
    for (ii = 0; ii < CORE_MAXGI; ii++)
      lines[firstRow][ii*2] = dotColor[coreGlobals.gi[ii]>0];
    osd_mark_dirty(0, firstRow, 2*CORE_MAXGI+1, firstRow+2);
  }
  if (coreGlobals.simAvail) sim_draw(firstRow);
  /*-- draw game specific mechanics --*/
  if (core_gameData->hw.drawMech) core_gameData->hw.drawMech((void *)&bitmap->line[firstRow]);
}
