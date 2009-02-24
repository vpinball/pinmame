/* Bally Video/Pinball Combination Hardware Emulation
   by Steve Ellenoff (01/16/2002)

   Games: Baby Pacman (1982)
          Granny & The Gators (1984)

   Hardware:
   (Both Games)
   MPU Board: MPU-133 (Equivalent of MPU-35 except for 1 resistor to diode change) - See note below

   (BabyPacman Only):
   VIDIOT Board: Handles Video/Joystick Switchs/Sound Board
   Chips:       (1 x TMS9928 Video Chip), 6809 CPU (Video), 6803 (Sound), 6821 PIA

   (Granny & Gators Only):
   VIDIOT DELUXE:Handles Video/Joystick and Communication to Cheap Squeak board
   Chips:       (2 x TMS9928 Video Chip - Master/Slave configuration), 6809 CPU, 6821 PIA
   Cheap Squeak Sound Board

   *EMULATION ISSUES*
   Baby Pac: working fine
   G & G: Color blending is not accurately emulated, but transparency is simulated

   Interesting Tech Note:

   The sound board integrated into the vidiot is
   identical to what later became the separate Cheap Squeak board used on later model
   bally MPU-35 and early 6803 games!

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
#include "by35.h"
#include "byvidpin.h"
#include "snd_cmd.h"
#include "by35snd.h"
#include "sndbrd.h"

#define BYVP_VCPUNO 1		/* Video CPU # */
#define BYVP_SCPUNO 2		/* Sound CPU # */
#define BYVP_PIA0 0
#define BYVP_PIA1 1
#define BYVP_PIA2 2

#define BYVP_VBLANKFREQ    60 /* VBLANK frequency */

#define BYVP_IRQFREQ       BY35_IRQFREQ /* IRQ (via PIA) frequency*/
//#define BYVP_ZCFREQ        BY35_ZCFREQ  /* Zero cross frequency */
#define BYVP_ZCFREQ        60		/* This is correct based on comparing my real game to the emu SJE 8/23/06 */


/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *byVP_CMOS;
static NVRAM_HANDLER(byVP) {
  core_nvram(file, read_or_write, byVP_CMOS, 0x100, 0xff);
}
// Bally only uses top 4 bits
static WRITE_HANDLER(byVP_CMOS_w) { byVP_CMOS[offset] = data | 0x0f; }


static struct {
  int p0_a, p1_a, p1_b, p0_cb2, p1_cb2, p2_a, p2_b, p2_cb2;
  int lampadr1;
  UINT32 solenoids;
  int diagnosticLed;
  int diagnosticLedV;		//Diagnostic LED for Vidiot Board
  int vblankCount;
  int phase_a;
  int irqstate; // ??? Is this really a toggle
} locals;

static void byVP_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    //int lampdata = ((locals.p0_a>>4)^0x0f) & 0x03; //only 2 lamp drivers
	int lampdata = ((~locals.p0_a)>>6) & 0x03; //only 2 lamp drivers
//    logerror("byvp lampstrobe %d lampdadr %x\n",board,lampadr);
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
  if ((locals.p1_a & 0x04) == 0) soundlatch2_w(0,data ^ 0x0f);
//  logerror("writehandler lampstrobe %d \n",locals.phase_a);
  byVP_lampStrobe(locals.phase_a, locals.lampadr1);
}

/* PIA0:B-R  Get Data depending on PIA0:A
(in)  PB0-1: Vidiot Status Bits 0,1 INVERTED - (When Status Data Enabled Flag is Set)
(in)  PB0-7: Vidiot Output Data (When Enable Output Flag is Set(Active Low) )
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows
(in)  PB0-7: Dip Returns */
static READ_HANDLER(pia0b_r) {
  // Enable Status must be low to return data..
  if ((locals.p1_a & 0x08) == 0) return ~locals.p2_a & 0x03;
  // Enable Output must be low to return data..
  if ((locals.p1_a & 0x02) == 0) return soundlatch_r(0);

  if (locals.p0_a & 0x20) return core_getDip(0); // DIP#1-8
  if (locals.p0_a & 0x40) return core_getDip(1); // DIP#9-16
  if (locals.p0_a & 0x80) return core_getDip(2); // DIP#17-24
  if (locals.p0_cb2)      return core_getDip(3); // DIP#25-32
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x80)>>2));
}

/* PIA1:A-W: Communications to Vidiot
(out) PA0:	 N/A
(out) PA1:   Video Enable Output (Active Low)
(out) PA2:   Video Latch Input Data (When Low, Signals to Vidiot, that data is coming, When Transition to High, Latches Data)
(out) PA3:   Video Status Enable (Active Low)
(out) PA4-7: N/A*/
static WRITE_HANDLER(pia1a_w) {
  // Update Vidiot PIA - This will trigger an IRQ also, depending on how the ca lines are configured
  sndbrd_sync_w(CAT3(pia_,BYVP_PIA2,_ca1_w),0,data & 0x02);
  sndbrd_sync_w(CAT3(pia_,BYVP_PIA2,_ca2_w),0,data & 0x04);
  locals.p1_a = data;
}

/* PIA0:CB2-W Lamp Strobe #1, DIPBank3 STROBE */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}

/* PIA1:B-W Solenoid */
static WRITE_HANDLER(pia1b_w) {
  const UINT32 contsols = (~core_revnyb(data>>4) & 0x0f)<<6;
  locals.p1_b = data;
  coreGlobals.pulsedSolState = 0;
  // Momentary Solenoids 1-7
  if (!locals.p1_cb2)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x07)) & 0x7f;
  // Continuos solenoids 8-11
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfffff87f) | contsols;
  locals.solenoids |= contsols;
}

/* PIA1:CA2-W Diagnostic LED */
static WRITE_HANDLER(pia1ca2_w) { locals.diagnosticLed |= data; }

/* PIA1:CB2-W Solenoid Bank Select */
static WRITE_HANDLER(pia1cb2_w) { locals.p1_cb2 = data; }

static void piaIrq(int state) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/* PIA2:A Write */
//PA0-3: Status Data Inverted! (Only Bits 0 & 1 Used however)
//PA4-7: Video Switch Strobes (Only Bit 7 is used however)
static WRITE_HANDLER(pia2a_w) { locals.p2_a = data; }

/* PIA2:B Write*/
//PB0-3: Output to 6803 CPU
//PB4-7: N/A
extern void by45_p21_w(UINT8 data);

static WRITE_HANDLER(pia2b_w) {
	//printf("pb2_w = %x\n",data);
	locals.p2_b = data & 0x0f;
	by45_p21_w(data & 0x02);
}

/* PIA2:B Read */
// Video Switch Returns (Bits 5-7 not connected)
static READ_HANDLER(pia2b_r) {
  if (locals.p2_a & 0x80) return coreGlobals.swMatrix[0]&0xf0;
  if (locals.p2_a & 0x40) return (coreGlobals.swMatrix[6]&0x1)<<7;	//Power Button for Granny - Mapped to switch #41, since only 40 switches are used in the game.
  return 0;
}

static WRITE_HANDLER(pia2cb2_w) {
  locals.diagnosticLedV |= data;
  sndbrd_0_data_w(0, locals.p2_b);
  sndbrd_0_ctrl_w(0, data);
}

// VIDEO PIA IRQ - TRIGGER VIDEO CPU FIRQ
static void pia2Irq(int state) {
  cpu_set_irq_line(BYVP_VCPUNO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
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
    coreGlobals.diagnosticLed = (locals.diagnosticLedV<<1)| (locals.diagnosticLed);
    locals.diagnosticLed = locals.diagnosticLedV = 0;
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BYVP_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /* Generate VBLANK interrupt on TMS9928 */
  TMS9928A_interrupt(0); if (core_gameData->hw.display) TMS9928A_interrupt(1);

  core_updateSw(coreGlobals.solenoids & 0x00000080);
}

static SWITCH_UPDATE(byVP) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[BYVP_COMINPORT] & 0xff;
    if (core_gameData->hw.display) { // Granny
      coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0xf3));
      // Start Player 2
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>4) & 0x10;
      // Start Player 1
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>4) & 0x20;
      // Coin Chute #1 & #2
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>10) & 0x03;
      // Ball Tilt/Slam Tilt
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>6) & 0xc0;
      // Power (Does not belong in the matrix, so we start at switch #41, since 40 is last used switch)
      coreGlobals.swMatrix[6] &= ~(0x1);
      coreGlobals.swMatrix[6] |= (inports[BYVP_COMINPORT]>>14) & 0x1;
    }
    else { // BabyPac
      coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x24));
      coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0xc3));
      // Start Player 2
      coreGlobals.swMatrix[1] |= (inports[BYVP_COMINPORT]>>6) & 0x04;
      // Start Player 1
      coreGlobals.swMatrix[1] |= (inports[BYVP_COMINPORT]>>4) & 0x20;
      // Coin Chute #1 & #2
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>10) & 0x03;
      // Ball Tilt/Slam Tilt
      coreGlobals.swMatrix[2] |= (inports[BYVP_COMINPORT]>>6) & 0xc0;
    }
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(BYVP_SWCPUDIAG))   cpu_set_nmi_line(0, PULSE_LINE);
  if (core_getSw(BYVP_SWVIDEODIAG)) cpu_set_nmi_line(BYVP_VCPUNO, PULSE_LINE);
  sndbrd_0_diag(core_getSw(BYVP_SWSOUNDDIAG));

  /*-- coin door switches --*/
  pia_set_input_ca1(BYVP_PIA0, !core_getSw(BYVP_SWSELFTEST));
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
      IRQ:   Wired to Main 6800 CPU IRQ.*/

/*PIA 1*/
/*PIA U11:
(in)  PA0-7  N/A
(in)  PB0-7  N/A
(in)  CA1:   Display Interrupt Generator
(in)  CB1:   J5 - Pin 32 (Marked as Test Connector)
(in)  CA2:   N/A
(in)  CB2:   N/A
(out) PA0:   N/A
(out) PA1:   Video Enable Output
(out) PA2:   Video Latch Input Data
(out) PA3:   Video Status Enable
(out) PA4-7: N/A
(out) PB0-3: Momentary Solenoid
(out) PB4-7: Continuous Solenoid
(out) CA2:   Diag LED
(out) CB2:   Solenoid Bank Select
      IRQ:   Wired to Main 6800 CPU IRQ.*/

/*---VIDIOT BOARD---*/

/*PIA 2*/
/*PIA U7:
(in)  PA0-7  N/A
(in)  PB0-7  Video Switch Returns (Bits 5-7 not connected)
(in)  PB0-3  ?? Input from 6803 CPU?
(in)  CA1:   Enable Data Output to Main CPU
(in)  CB1:   N/A
(in)  CA2:   Main CPU Latch Data Input
(in)  CB2:   J2-1 = N/A?
(out) PA0-3: Status Data (Only Bits 0 & 1 Used however) - Inverted before reaching MPU
(out) PA4-7: Video Switch Strobes (Only Bit 7 is used however)
(out) PB0-3: Output to 6803 CPU
(out) PB4-7: N/A
(out) CA2:   N/A
(out) CB2:   6803 Data Strobe & LED
      IRQ:   Wired to Video 6809 CPU FIRQ*/
static struct pia6821_interface piaIntf[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, 0 ,pia0cb2_w,
/* IRQ: A/B              */  piaIrq, piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
/* IRQ: A/B              */  piaIrq, piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,pia2b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia2a_w, pia2b_w, 0, pia2cb2_w,
/* IRQ: A/B              */  pia2Irq, pia2Irq
}};

//Read Display Interrupt Generator
static INTERRUPT_GEN(byVP_irq) {
  pia_set_input_ca1(BYVP_PIA1, locals.irqstate = !locals.irqstate);
}

//Send a command manually
static WRITE_HANDLER(byVP_soundCmd) {
  if (data) {
//    locals.snddata = data;
//    locals.sndcmdnum = 0;
//    snd_cmd_log(data);
  }
}

/* I read on a post in rgp, that they used both ends of the zero cross, so we emulate it */
static void byVP_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
  pia_set_input_cb1(BYVP_PIA0,locals.phase_a = !locals.phase_a);
//  logerror("zerocross: count %d \n",locals.vblankCount);
}

static MACHINE_INIT(byVP) {
  memset(&locals, 0, sizeof(locals));
  sndbrd_0_init(core_gameData->hw.soundBoard,BYVP_SCPUNO,NULL,NULL,NULL);
  install_mem_write_handler(0,0x0200, 0x02ff, byVP_CMOS_w);
  /* init PIAs */
  pia_config(BYVP_PIA0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(BYVP_PIA1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  pia_config(BYVP_PIA2, PIA_STANDARD_ORDERING, &piaIntf[2]);
  pia_reset();
}

extern void by45snd_reset(void);
static void by_vdp_interrupt(int state);
static MACHINE_RESET(byVP)
{
	//printf("reset\n");
	memset(&locals, 0, sizeof(locals));
	by45snd_reset();
	pia_reset();
	by_vdp_interrupt(0);
	TMS9928A_reset(0);
	pia1a_w(0,0);
}
static MACHINE_STOP(byVP) { sndbrd_0_exit(); }





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
static void by_vdp_interrupt(int state) {
  cpu_set_irq_line(BYVP_VCPUNO, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(TMS9928A_vram_01_w) {
  TMS9928A_vram_0_w(offset,data); TMS9928A_vram_1_w(offset,data);
}

static WRITE_HANDLER(TMS9928A_register_01_w) {
  TMS9928A_register_0_w(offset,data); TMS9928A_register_1_w(offset,data);
}

static VIDEO_START(byVP) {
  if ((TMS9928A_start(0,TMS99x8A, 0x4000)) ||
      (core_gameData->hw.display && TMS9928A_start(1,TMS99x8A, 0x4000)))
    return 1;
  TMS9928A_int_callback(0,by_vdp_interrupt);
  return 0;
}

static VIDEO_STOP(byVP) {
  TMS9928A_stop(core_gameData->hw.display ? 2 : 1);
}

static PINMAME_VIDEO_UPDATE(byVP_update) {
  TMS9928A_refresh((core_gameData->hw.display ? 2 : 1), bitmap, 1);
  return 0;
}

/*-----------------------------------
/  Memory map for MAIN CPU board
/------------------------------------*/
static MEMORY_READ_START(byVP_readmem)
  { 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_r(BYVP_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_r(BYVP_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x0300, 0x031c, MRA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_writemem)
  { 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_w(BYVP_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_w(BYVP_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x02ff, MWA_RAM, &byVP_CMOS }, /* CMOS Battery Backed*/
  { 0x0300, 0x031c, MWA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

/*----------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Board)
/----------------------------------------------------*/
static MEMORY_READ_START(byVP_video_readmem)
  { 0x0000, 0x1fff, soundlatch2_r },
  { 0x2000, 0x2003, pia_r(BYVP_PIA2) }, /* U7 PIA */
  { 0x4000, 0x4000, TMS9928A_vram_0_r },  /* U16 VDP*/
  { 0x4001, 0x4001, TMS9928A_register_0_r },  /* U16 VDP*/
  { 0x6000, 0x6400, MRA_RAM }, /* U13&U14 1024x4 Byte Ram*/
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVP_video_writemem)
  { 0x0000, 0x1fff, soundlatch_w },
  { 0x2000, 0x2003, pia_w(BYVP_PIA2) }, /* U7 PIA */
  { 0x4000, 0x4000, TMS9928A_vram_0_w },  /* U16 VDP*/
  { 0x4001, 0x4001, TMS9928A_register_0_w },  /* U16 VDP*/
  { 0x6000, 0x6400, MWA_RAM }, /* U13&U14 1024x4 Byte Ram*/
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/*-------------------------------------------------------------------
/  Memory map for VIDEO CPU (Located on Vidiot Deluxe Board) - G&G
/------------------------------------------------------------------*/
static MEMORY_READ_START(byVPGG_video_readmem)
  { 0x0000, 0x0001, soundlatch2_r },
  { 0x0002, 0x0002, TMS9928A_vram_0_r },  /* VDP MASTER */
  { 0x0003, 0x0003, TMS9928A_register_0_r },  /* VDP MASTER */
  { 0x0004, 0x0004, TMS9928A_vram_1_r },  /* VDP SLAVE  */
  { 0x0005, 0x0005, TMS9928A_register_1_r },  /* VDP SLAVE  */
  { 0x0008, 0x000b, pia_r(BYVP_PIA2) }, /* PIA */
  { 0x0300, 0x031c, MRA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x2000, 0x27ff, MRA_RAM }, /* 2K RAM */
  { 0x2801, 0x2801, MRA_RAM }, /* ?????? */
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(byVPGG_video_writemem)
  { 0x0000, 0x0001, soundlatch_w },
  { 0x0002, 0x0002, TMS9928A_vram_0_w },  /* VDP MASTER */
  { 0x0003, 0x0003, TMS9928A_register_0_w },  /* VDP MASTER */
  { 0x0004, 0x0004, TMS9928A_vram_1_w },  /* VDP SLAVE  */
  { 0x0005, 0x0005, TMS9928A_register_1_w },  /* VDP SLAVE  */
  { 0x0006, 0x0006, TMS9928A_vram_01_w },  /* VDP Both  */
  { 0x0007, 0x0007, TMS9928A_register_01_w }, /* VDP Both  */
  { 0x0008, 0x000b, pia_w(BYVP_PIA2) }, /* PIA */
  { 0x0300, 0x031c, MWA_RAM }, /* What is this? More CMOS? No 3rd Flash if commented as MWA_RAM */
  { 0x2000, 0x27ff, MWA_RAM }, /* 2K RAM*/
  { 0x2801, 0x2801, MWA_RAM }, /* ????????? */
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

const struct core_dispLayout byVP_dispBabyPac[] = {
  {0,0,192,256,CORE_VIDEO,(void *)byVP_update}, {0}
};
const struct core_dispLayout byVP_dispGranny[] = {
  {0,0,192,256,CORE_VIDEO,(void *)byVP_update}, {0}
};
/*--------------------*/
/* Machine Definition */
/*--------------------*/
/* BABYPAC HARDWARE */
MACHINE_DRIVER_START(byVP1)
  MDRV_IMPORT_FROM(PinMAME)

  MDRV_CPU_ADD_TAG("mcpu", M6800, 3580000/4)
  MDRV_CPU_MEMORY(byVP_readmem, byVP_writemem)
  MDRV_CPU_VBLANK_INT(byVP_vblank, 1)
  MDRV_CPU_PERIODIC_INT(byVP_irq, BYVP_IRQFREQ)

  MDRV_CPU_ADD_TAG("vcpu", M6809, 3580000/4)
  MDRV_CPU_MEMORY(byVP_video_readmem, byVP_video_writemem)
  MDRV_INTERLEAVE(100)
  MDRV_TIMER_ADD(byVP_zeroCross, BYVP_ZCFREQ*2)
  MDRV_CORE_INIT_RESET_STOP(byVP,byVP,byVP)
  MDRV_DIPS(32)
  MDRV_DIAGNOSTIC_LEDV(2)
  MDRV_SWITCH_UPDATE(byVP)
  MDRV_NVRAM_HANDLER(byVP)

  /* video hardware */
  MDRV_SCREEN_SIZE(320,256) // To view matrices and solno
  MDRV_VISIBLE_AREA(0, 319, 0, 255)
  MDRV_GFXDECODE(0)
  MDRV_PALETTE_LENGTH(TMS9928A_PALETTE_SIZE)
  MDRV_COLORTABLE_LENGTH(TMS9928A_COLORTABLE_SIZE)
  MDRV_PALETTE_INIT(TMS9928A)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE)
  MDRV_VIDEO_START(byVP)
  MDRV_VIDEO_STOP(byVP)

  /* sound hardware */
  MDRV_IMPORT_FROM(by45)
  MDRV_SOUND_CMD(byVP_soundCmd)
  MDRV_SOUND_CMDHEADING("byVP")
MACHINE_DRIVER_END

/*GRANNY & THE GATORS HARDWARE*/
MACHINE_DRIVER_START(byVP2)
  MDRV_IMPORT_FROM(byVP1)
  MDRV_CPU_REPLACE("vcpu", M6809, 8000000/4)
  MDRV_CPU_MEMORY(byVPGG_video_readmem, byVPGG_video_writemem)
  MDRV_SCREEN_SIZE(256,256) // 256x192 + matrices
  MDRV_VISIBLE_AREA(0, 255, 0, 255)
MACHINE_DRIVER_END

