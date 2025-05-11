/************************************************************************************************
  Capcom
  -----------------
  by Martin Adrian, Steve Ellenoff, Gerrit Volkenborn (05/07/2002 - 11/10/2003)

  Hardware from 1995-1996

  CPU BOARD:
   CPU: 68306 @ 16.67 Mhz
   I/O: 68306 has 2 8-bit ports and a dual uart with timer
   DMD: Custom programmed U16 chip handles DMD Control Data and some timing
   Size: 128x32 (all games except Flipper Football - 256x64!)

  SOUND BOARD:
   CPU: 87c52 @ 12 Mhz
   I/O: 87c52 has a uart
   SND: 2 x TMS320AV120 MPG DECODER ( Only 1 on Breakshot )
   VOL: x9241 Digital Volume Pot

  Capcom Standard Pins:
  Lamp Matrix     = 2 x (8x8 Matrixs) = 128 Lamps - NOTE: No GI - every single lamp is cpu controlled
  Switch Matrix   = 64 Switches on switch board + 16 Cabinet Switches
  Solenoids	= 32 Solenoids/Flashers
  Sound: 2 Channel Mono Audio

  Capcom "Classic" Pins: (Breakshot)
  Lamp Matrix     = 1 x (8x8 Matrixs) = 64 Lamps - 64 GI Lamps (not directly cpu controlled?)
  Switch Matrix   = 1 x (8x7 Matrix)  = 56 Switches + 9 Cabinet Switches
  Solenoids	= 32 Solenoids/Flashers
  Sound: 1 Channel Mono Audio

  Milestones:
  09/19/03-09/21/03 - U16 Test successfully bypassed, dmd and lamps working quite well
  09/21/03-09/24/03 - DMD working 100% including 256x64 size, switches, solenoids working on most games except kp, and ff
  09/25/03          - First time game booted without any errors.. (Even though in reality, the U16 would still fail if not hacked around)
  11/03/03          - First time sound was working almost fully (although still some glitches and much work left to do)
  11/09/03          - 50V Line finally reports a voltage & KP,FF fire hi-volt solenoids
  11/10/03          - Seem to have found decent IRQ4 freq. to allow KP & FF to fire sols 1 & 2 properly
  11/15/03          - 68306 optimized & true address mappings implemented, major speed improvements!

  Hacks & Issues that need to be looked into:
  #2) U16 Needs to be better understood and emulated more accurately (should fix IRQ4 timing problems)
  #3) IRQ4 appears to somehow control timing for 50V solenoids & Lamps in FF&KP, unknown effect in other games
  #4) Handle opto switches internally? Is this needed?
  #5) Handle EOS switches internally? Is this needed?
  #6) More complete M68306 emulation (although it's fairly good already) - could use some optimization
  #7) Lamps will eventually come on in Kingpin/Flipper Football, why does it take so long? Faster IRQ4 timing will improve response time ( cycles of 2000 for example, but screws up solenoids )
  #8) Not sure best way to emulate 50V line, see TEST50V_TRYx macros below
  #9) Firing of 50V solenoids seems sometimes inconsistent in FF&KP, but IRQ4 timing helps correct it
  #10)How to get varying flipper solenoid strength included and wired to the outside? 
      i.e. KingPin power meter: http://www.krellan.com/pinball/kingpin/ & http://www.freepatentsonline.com/5655770.html
      Capcom only used one single type of coils and was able to set the strength by software: It was a nice feature to ease maintenance.
      This was way better than B/W used to do because you always needed one type of coils and not 5 or 6 different ones.
      The PowerMeter in King Pin works that way. The lower the power is on the display the lower the strength the flippers is applied.
      Also see https://m.facebook.com/story.php?story_fbid=2088173937889558&id=1034214746618821
      -> suggestion: Find mem location (as it's only one machine/ROM version after all!) and use that directly instead of trying to track/map it?
  #11) Flippers are not implemented.  VPinMAME appears to pass flipper switches through to flipper solenoids (always on).   Actual flipper solenoids
       seem to give one very latent pulse instead of staying on.

 [VB 02/01/2024]
 After diving into the schematics and some 68000 disassembler, some changes:
 - Added PWM with physical model outputs (really needed, since the hardware uses this almost everywhere)
 - Replaced diagnostic leds by PWM output in an additional lamp column (they were wrong before, and Capcom use PWM to report ok state)
 - Moved back to 'normal' frequencies: CPU at 16MHz from the schematics, 120Hz for zero cross since these are things we can be sure of
 - Removed locals.driverboard read/write: from the schematics, this is an hardcoded value read from U55 (sheet 9 of driver board schematics, checked for Airborne, Breakshot,...)
 - Adjusted Line_5/Line_V to be read as expected (RC filter with comparator, the CPU measure how long it takes to pass the comparator threshold)
 - Fixed 68306 DUART counter which was counting 2 times too slow. 5V/50V measure is now right (it is used to measure Pulse/Charge/Discharge lengths and likely adjust solenoid PWM to get consistent solenoid strength accross voltage levels)
 - Removed locals.blanking since it is in fact the lamp matrix VSET1/2 which enable/disable lamp matrices (not solenoid). There is also a Blank signal from U16 which is unknown so far
 - Returns the lamp/solenoid state when read since the schematics shows that the read value is the latched one, including overcurrent reset (used in diagnostics section of DMD)
 - Zero cross (IRQ2) is now latched and manually resetted by the CPU as the schematics shows
 - Cleaned up a little bit of memory mapping between Breakshot and other games
 - Implemented U16 programmable interrupt controller. Startup tests are now passing.
 - Adjusted (hacked) IRQ1 timings to fulfill zero cross measurements (in audit, in ROM code, also impacts how IRQ4 line 1 is setup by game code)
 Things are better but would still needs some work:
 - IRQ1 is not yet fully understood, timings used seems good but are nto verified
 - U16 Blank signal is not understood and not simulated, this will likely give wrong dead sol/bulb report in diagnostics, which is annoying since software will disable those dead solenoids
 - VSet1/VSet2 is only partially implemented, this will likely give wrong dead bulb report in diagnostics
 - Level1/Level2 for switches is not implemented (likely no impact, since this may be done to detect 'resistor' switches)

**************************************************************************************/
#include <stdarg.h>
#include <math.h>
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "capcoms.h"
#include "sndbrd.h"

//Turn off when not testing mpg audio
#define TEST_MPGAUDIO      0

// Define to get read/write log to U16
#define VERBOSE_U16        0

// Define to 1 to patch ROM to disable error messages
#ifdef __GNUC__
// For some reason, MinGW builds will fail startup test and report a slightly wrong AC frequency (64Hz instead of the expected 60Hz), all of them demonstrating wrong IRQ timings
#define SKIP_ERROR_MSG     1
#else
#define SKIP_ERROR_MSG     0
#endif

// Define to log lamp strobe timings
#define LOG_LAMP_STROBE    0

// Breakshot has a modified hardware with a single lamp matrix (B), the hardware of the other lamp matrix being used as a switch matrix, allowing to spare the switch board
#define HAS_SWITCH_BOARD (core_gameData->hw.lampCol > 1)

#define CC_ZCFREQ		120			/* Zero cross frequency for 60Hz */

#define CPU_CLOCK		16670000	/* Main frequency of the MC68306 (design frequency, and XTal from schematics) */

static data16_t *rom_base;
static data16_t *ramptr;

//declares
extern void capcoms_manual_reset(void);

static struct {
  UINT32 solenoids; // romstar only
  UINT8 visible_page;
  int zero_cross, zero_acq; // Raised by zero cross comparator (120Hz) latched, acked when IRQ2 is processed
  int swCol; // Breakshot only: active switch column
  int vset;
  int greset;
  UINT16 parallelA, parallelB;
  int pulse;
  double pulseVoltage, lastPulseFlip;
  int first_sound_reset;
  int lampAColDisable, lampBColDisable;
  UINT16 lampA, lampB;

  UINT16 u16a[4], u16b[4];
  int irq1State, irq4State;
  int u16IRQ4Mask, u16IRQState, u16IRQ1Adjust;
  double u16IRQ1Period, u16IRQ4Line1Period, u16IRQ4Line2Period, u16IRQ4Line3Period;
  mame_timer *u16IRQ1timer, *u16IRQ4Line1timer, *u16IRQ4Line2timer, *u16IRQ4Line3timer;
} locals;

static NVRAM_HANDLER(cc);
static void Skip_Error_Msg(void);

static INTERRUPT_GEN(cc_vblank) {
  /*-- update leds (they are PWM faded, so uses physic output) --*/
  coreGlobals.diagnosticLed = (coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + 8 * 8 + (core_gameData->hw.lampCol - 1) * 8    ].value >= 0.5f ? 1 : 0)
                            | (coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + 8 * 8 + (core_gameData->hw.lampCol - 1) * 8 + 1].value >= 0.5f ? 2 : 0);

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(cc) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],0xcf,9);
    if (HAS_SWITCH_BOARD)
      CORE_SETKEYSW(inports[CORE_COREINPORT]>>8,0xff,0);
    else
      CORE_SETKEYSW(((inports[CORE_COREINPORT]>>8)<<4),0xf0,2);
  }
}

/*********************/
/* IRQ2 - ZERO CROSS */
/*********************/
static void cc_zeroCross(int data) {
  if (!locals.zero_acq) {
    //printf("%8.5f, Zero Cross\n", timer_get_time());
    locals.zero_cross = 1;
    cpu_set_irq_line(0, MC68306_IRQ_2, ASSERT_LINE);
  }
}

/***************/
/* PORT A READ */
/***************/
//PA0   - J3 - Pin 7 - Token/Coin Meter/Printer Interface Board(Unknown purpose)
//PA1   - J3 - Pin 8 - Token/Coin Meter/Printer Interface Board(Unknown purpose)
//PA2   - J3 - Pin 9 - Token/Coin Meter/Printer Interface Board(Unknown purpose)
//PA3   - LED (output only)
//PA4   - LINE_5 (input: Measures +5V Low power D/C Line)
//PA5   - LINE_V (input: Measures 50V High Power D/C Line for Solenoids)
//PA6   - VSET (output only)
//PA7   - GRESET (output only)
static READ16_HANDLER(cc_porta_r) {
  int data = locals.parallelA & 0xCF;

  // Hardware measure 5V DV and 50V DC voltage by performing pulses (using PULSE out signal) through a RC low pass filter (time constant is 363us, since it is a 11k resistor with a 33nF capacitor)
  // then reading the binary compared value against voltage divided from 5V or 50V line. So we compute the voltage at the capacitor and perform the comparison like the hardware
  // Some games seems to also have 5V line voltage with the same RC schematics as for 50V (f.e. in Airborne scematics but missing in Breakshot ones). There is no audit result for it.
  // For simpler implementation, we can use a threshold for charge/discharge computed as -RC.ln(1-Vth/V0) for charge and -RC.ln(Vth/V0) for discharge, V0 is 5V, Vth is the comparator threshold (from the voltage divider of 11k/11k applied to 5V so 2.5V)
  double now = timer_get_time(), v;
  if (locals.pulse) // Charging from initial voltage (locals.pulseVoltage) toward 5V
    v = locals.pulseVoltage + (5.0 - locals.pulseVoltage) * (1.0 - exp(-(now - locals.lastPulseFlip) / (11000.0 * 33.0e-9)));
  else // Discharging from initial voltage (locals.pulseVoltage)
    v = locals.pulseVoltage * exp(-(now - locals.lastPulseFlip) / (11000.0 * 33.0e-9));
   
  if (v > (55.0 - 1.1 - 1.1) * 2000.0 / (56200.0 + 2000.0)) // 55V AC through MB352W bridge rectifier (2 x 1.1V voltage drop)
  //if ((locals.pulse && now - locals.pulseStart > 0.00015083) || (!locals.pulse && now - locals.pulseEnd < 0.00039161))
    data |= 1 << 5; // Line 50V

  if (v > 5.0 * 11000.0 / (11000.0 + 11000.0))
  //if ((locals.pulse && now - locals.pulseStart > 0.00025161) || (!locals.pulse && now - locals.pulseEnd < 0.00025161))
    data |= 1 << 4; // Line 5V

  DBGLOG(("Port A read\n"));
  //printf("[%08x] Port A read = %x\n",activecpu_get_previouspc(),data);
  return data;
}
/***************/
/* PORT B READ */
/***************/
//PB0 OR IACK2 - ZERO X ACK (Output Only)
//PB1 OR IACK3 - PULSE (To J2) (Output Only)
//PB2 OR IACK5 - J3 - Pin 13 - Token/Coin Meter/Printer Interface Board - SW3(Unknown Purpose)(Output Only)
//PB3 OR IACK6 - J3 - Pin 12 - Token/Coin Meter/Printer Interface Board - SW4(Unknown Purpose)(Output Only)
//PB4 OR IRQ2  - ZERO CROSS IRQ (Input)
//PB5 OR IRQ3  - NOT USED?
//PB6 OR IRQ5  - J3 - Pin 11 - Token/Coin Meter/Printer Interface Board - SW6(Unknown Purpose)(Output Only)
//PB7 OR IRQ6  - NOT USED?
static READ16_HANDLER(cc_portb_r) {
  int data = locals.parallelB & 0xEF;
  data |= locals.zero_cross << 4;
  DBGLOG(("Port B read = %x\n",data));
  return data;
}

/****************/
/* PORT A WRITE */
/****************/
//PA0   - J3 - Pin 7 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA1   - J3 - Pin 8 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA2   - J3 - Pin 9 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA3   - LED
//PA4   - LINE_5 (Input only)
//PA5   - LINE_V (Input only)
//PA6   - VSET
//PA7   - GRESET (to soundboard - Inverted?)
static WRITE16_HANDLER(cc_porta_w) {
  int reset;
  if(data !=0x0048 && data !=0x0040 && data !=0x0008)
    DBGLOG(("Port A write %04x\n",data));

  // Bulletin service 95-010a states that the diag led should go bright then dims 4 times per second => PWM integration on the diag LED...
  core_write_masked_pwm_output_8b(CORE_MODOUT_LAMP0 + 8 * 8 + (core_gameData->hw.lampCol - 1) * 8, (data & 0x08) >> 3, 0x01);

  // Cabinet switch voltage level comparator (either comparator from +5V with 2.2k/1.2k =>1.76V or 2.2k/3.9k => 3.20V), not sure why
  locals.vset = (data>>6) & 1;

  // Manually reset the sound board on 1->0 transition (but don't do it the very first time, ie, when both cpu's first boot up)
  reset = (data >> 7) & 1;
  if (locals.greset && !reset) {
    if(!locals.first_sound_reset)
      locals.first_sound_reset=1;
   else
     capcoms_manual_reset();
  }
  locals.greset = reset;

  locals.parallelA = data;
}
/****************/
/* PORT B WRITE */
/****************/
//PB0 OR IACK2 - ZERO CROSS ACK (Clears IRQ2) - Lower ZC IRQ after serviced
//PB1 OR IACK3 - PULSE (To J2) - Used to measure 5V / 50V voltage
//PB2 OR IACK5 - J3 - Pin 13 - Token/Coin Meter/Printer Interface Board - SW3(Unknown Purpose)(Output Only)
//PB3 OR IACK6 - J3 - Pin 12 - Token/Coin Meter/Printer Interface Board - SW4(Unknown Purpose)(Output Only)
//PB4 OR IRQ2  - ZERO CROSS IRQ (Input Only)
//PB5 OR IRQ3  - NOT USED?
//PB6 OR IRQ5  - J3 - Pin 11 - Token/Coin Meter/Printer Interface Board - SW6(Unknown Purpose)
//PB7 OR IRQ6  - NOT USED?
static WRITE16_HANDLER(cc_portb_w) {
  DBGLOG(("Port B write %04x\n",data));

  int newPulse = data & 2;
  if (locals.pulse != newPulse) {
    double now = timer_get_time();
    //if (!newPulse)
    //printf("PC %08x - %8.5f, Pulse toggle: %d -> %d, length=%8.5f\n", activecpu_get_pc(), timer_get_time(), locals.pulse, newPulse, newPulse ? (now - locals.pulseStart) : (now - locals.pulseEnd));
    if (newPulse) // End of discharge
      locals.pulseVoltage = locals.pulseVoltage * exp(-(now - locals.lastPulseFlip) / (11000.0 * 33.0e-9));
    else // End of charge
      locals.pulseVoltage = locals.pulseVoltage + (5.0 - locals.pulseVoltage) * (1.0 - exp(-(now - locals.lastPulseFlip) / (11000.0 * 33.0e-9)));
    locals.lastPulseFlip = now;
    locals.pulse = newPulse;
  }

  locals.zero_acq = data & 1;
  if (locals.zero_acq) {
    locals.zero_cross = 0;
    cpu_set_irq_line(0, MC68306_IRQ_2, CLEAR_LINE);
  }

  locals.parallelB = data;
}

/*********************************************************************************************************************/
// U16 chip
//
// U16 custom Capcom chip is a programmable gate array with no available documentation. From reverse engineering performed, it is supposed to:
// - Continuously stream data from RAM to DMD display
// - Manage DRAM access to handle muxing access between U16 and CPU (and also address muxing for the 512ko RAM chip HM514260)
// - Provide one independent frequency adjustable interrupt wired to CPU/IRQ1
// - Provide an interrupt controller with 3 lines, 2 of the 3 being frequency adjustable, wired to CPU/IRQ4
// - Generate the 3.6864MHz clock for the serial module of the MC68306 microcontroller (for serial output to printer, also used to measure AC frequency)
// - Generate a general BLANK signal which turns off all outputs
//
// it is controlled through 2x8 bytes of input/output, mapped at $40c0000X / $40c0040X
// Reading:
//   $40c00000  u16????????    is read during startup, if result is not 0x00BC, code will setup u16IRQmode, u16DMDBlock and u16DMDPage, and check again for 0x00BC or fail
//   $40c00002  u16IRQstate    ........ ...??CBA - CBA is active IRQ4 lines (pending ACK), likely also reports IRQ1 but not checked by any code I have looked at
//
// Writing:
//   $40c00002  u16IRQstate    ........ ...XXCBA is used to acknowledge IRQ by writing the inverted bit to ack:
//                             . IRQ1 by writing one of 0xffef (during startup tests) or 0xffe7 (during gameplay)
//                             . IRQ4 by writing one of 0xfffe / 0xfffd / 0xfffb
//   $40c00004  u16DMDBlock    Set DMD Visible Block offset (12 bits)
//   $40c00006  u16DMDPage     Set DMD Visible Page  offset ( 4 bits)
//
//   $40c00401  u16IRQmode     XX???CBA - Set IRQ generation mode
//                             . XX  is IRQ1 mode. This mode is not yet fully understood. Frequencies used have been deduced from startup tests timing (at startup) and 60Hz 
//                                   measured by game code (during gameplay). The only identified difference being the way the IRQ is acknowledge (delay flag ? something unspot yet ?).
//                             . CBA is IRQ4 line enable, bit C is inverted:
//                                 C is a fixed frequency interrupt generator, likely corresponding to DMD VBlank since it is 45.9Hz for FF (64 lines), 91.8Hz for others (32 lines), maybe this flag is DMD steobe on/off
//                                 B use frequency defined by writing at 40C00404
//                                 A use frequency defined by writing at 40C00402
//                             . ??? are unknown. Breakshot, Flipper Football & KingPin always set them to 011, except if an auxiliary board is present, then it is set to 111
//   $40C00402  u16IRQ4line1f  is IRQ4 line 1 frequency (0x1000 - (data & 0x0FFF)) * 88.6489 CPU cycles
//   $40C00404  u16IRQ4line2f  is IRQ4 line 2 frequency (0x1000 - (data & 0x0FFF)) * 88.6489 CPU cycles
//
// No other access were witnessed, so others are likely unused.
//
// TODO:
// - U16 Interrupt controllers are checked at startup by measuring their interrupt frequency against different settings, 
//   which allows to identify the expected values in the specific context of startup. IRQ4 line 1 & 2 seems to be purely
//   frequency driven. IRQ4 line 3 seems ot be the DMD VBlank. IRQ1 is somewhat less understood and hacked to get correct
//   game behavior.
// - blank signal from U16 blanks the solenoids and should be implemented, since solenoids PWM, handled by IRQ2 & IRQ4, 
//   use state readback to identify and disable dead solenoids. blank signal from U16 should also be implemented for 
//   lamps (but the ROM code doesn't disable broken lamps).
/*********************************************************************************************************************/

#if VERBOSE_U16
#define LOG_U16(x)	printf x
#else
#define LOG_U16(x)
#endif

static void cc_u16irq1(int data) {
  //if (!locals.irq1State) printf(">> Raise IRQ1\n");
  locals.irq1State = 1;
  locals.u16IRQState |= 0x10;
  cpu_set_irq_line(0, MC68306_IRQ_1, ASSERT_LINE);
}

static void cc_u16irq4line1(int data) {
  if (locals.u16IRQ4Mask & 1) {
    //if (!locals.irq4State) printf(">> Raise IRQ4\n");
    locals.irq4State = 1;
    locals.u16IRQState |= 0x01;
    cpu_set_irq_line(0, MC68306_IRQ_4, ASSERT_LINE);
  }
}

static void cc_u16irq4line2(int data) {
  if (locals.u16IRQ4Mask & 2) {
    //if (!locals.irq4State) printf(">> Raise IRQ4\n");
    locals.irq4State = 1;
    locals.u16IRQState |= 0x02;
    cpu_set_irq_line(0, MC68306_IRQ_4, ASSERT_LINE);
  }
}

static void cc_u16irq4line3(int data) {
  if (locals.u16IRQ4Mask & 4) {
    //if (!locals.irq4State) printf(">> Raise IRQ4\n");
    locals.irq4State = 1;
    locals.u16IRQState |= 0x04;
    cpu_set_irq_line(0, MC68306_IRQ_4, ASSERT_LINE);
  }
}

INLINE void u16UpdateIRQ() {
  if ((locals.u16IRQState & 0x10) == 0) {
    //if (locals.irq1State) printf("<< Drop  IRQ1\n");
    locals.irq1State = 0;
    cpu_set_irq_line(0, MC68306_IRQ_1, CLEAR_LINE);
  }
  if ((locals.u16IRQ4Mask & locals.u16IRQState) == 0) {
    //if (locals.irq4State) printf("<< Drop  IRQ4\n");
    locals.irq4State = 0;
    cpu_set_irq_line(0, MC68306_IRQ_4, CLEAR_LINE);
  }
}

/************/
/* U16 READ */
/************/
static READ16_HANDLER(u16_r) {
  offset &= 0x203;
  data16_t value;
  switch (offset) {
    case 0x000:
      value = 0x00BC;
      LOG_U16(("PC %08x - U16r Health state ?  [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), offset, value, mem_mask));
      break;

    case 0x001:
      value = ~(locals.u16IRQ4Mask & locals.u16IRQState);
      //LOG_U16(("PC %08x - U16r IRQ state=%04x  [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), locals.u16IRQ4Mask & locals.u16IRQState, offset, value, mem_mask));
      break;

    default:
      value = offset < 0x0004 ? locals.u16a[offset] : locals.u16b[offset & 3];
      LOG_U16(("PC %08x - U16r X Unsupported X [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), offset, value, mem_mask));
  }

  //DBGLOG(("U16r [%03x]=%04x (%04x)\n", offset, value, mem_mask));
  return value;
}

/*************/
/* U16 WRITE */
/*************/
static WRITE16_HANDLER(u16_w) {
  offset &= 0x203;
  // DBGLOG(("U16w [%03x]=%04x (%04x)\n",offset,data,mem_mask));

  switch (offset) {
    case 0x001: { // IRQ ACK
      // After startup tests, IRQ1 are acknowledged using 0x18 instead of 0x10. No clue why so far
      //LOG_U16(("PC %08x - U16w IRQ ACK %02x      [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), (~data) & 0x17, offset, data, mem_mask));
      locals.u16IRQState &= data;
      u16UpdateIRQ();
      // FIXME this is clearly a hack but we do not know what actually drives IRQ1 and the ROM code expects frequency around 4KHz when acknowledged with 0xFFE7 (during play) while it expects a 2.96KHz frequency when acked with 0xFFEF (during startup), so...
      if (((data & 0x10) == 0) && (locals.u16IRQ1Adjust == (data & 0x08))) {
         locals.u16IRQ1Adjust = (~data) & 0x08;
         timer_adjust(locals.u16IRQ1timer, locals.u16IRQ1Adjust ? (locals.u16IRQ1Period * 120.0 / 165.0) : locals.u16IRQ1Period, 0, locals.u16IRQ1Adjust ? (locals.u16IRQ1Period * 120.0 / 165.0) : locals.u16IRQ1Period);
      }
      break;
    }

    case 0x002: // DMD Visible Block offset
      LOG_U16(("PC %08x - U16w DMD block  %04x [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), data & 0x0FFF, offset, data, mem_mask));
      locals.visible_page = (locals.visible_page & 0x0f) | (data << 4);
      break;

    case 0x003: // DMD Visible Page offset
      LOG_U16(("PC %08x - U16w DMD page   %04x [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), data >> 12, offset, data, mem_mask));
      locals.visible_page = (locals.visible_page & 0xf0) | (data >> 12);
      break;

    case 0x200: { // IRQ1 Frequency, IRQ4 line 1/2/3 enable
      LOG_U16(("PC %08x - U16w IRQ1 f=%x IRQ4=%x [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), (data >> 6) & 3, (data & 0x7) ^ 4, offset, data, mem_mask));
      //static const int irq1NCycles[] = { 22662, 11308, 5631, 2789 }; // Number of CPU cycles taken from startup check disassembly (value without IRQ1 adjusting on ACK)
      static const int irq1NCycles[] = { 22602, 11248, 5571, 2729 }; // Number of CPU cycles taken from startup check disassembly (value with IRQ1 adjusting on ACK)
      locals.u16IRQ1Period = TIME_IN_SEC(irq1NCycles[(data >> 6) & 3] / (double)CPU_CLOCK);
      timer_adjust(locals.u16IRQ1timer, locals.u16IRQ1Adjust ? (locals.u16IRQ1Period * 120.0 / 165.0) : locals.u16IRQ1Period, 0, locals.u16IRQ1Adjust ? (locals.u16IRQ1Period * 120.0 / 165.0) : locals.u16IRQ1Period);
      timer_adjust(locals.u16IRQ4Line1timer, locals.u16IRQ4Line1Period, 0, locals.u16IRQ4Line1Period);
      timer_adjust(locals.u16IRQ4Line2timer, locals.u16IRQ4Line2Period, 0, locals.u16IRQ4Line2Period);
      timer_adjust(locals.u16IRQ4Line3timer, locals.u16IRQ4Line3Period, 0, locals.u16IRQ4Line3Period);
      locals.u16IRQ4Mask = (data & 0x7) ^ 4;
      locals.u16IRQState = 0; // Not sure if changing the IRQ mode resets the IRQ outputs
      u16UpdateIRQ();
      break;
    }
    case 0x201: { // IRQ4 Line 1 frequency
      LOG_U16(("PC %08x - U16w IRQ4 f1=%03x     [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), data, offset, data, mem_mask));
      locals.u16IRQ4Line1Period = TIME_IN_SEC(((0x1000 - (data & 0x0FFF)) * 88.675) / (double)CPU_CLOCK);
      break;
    }
    case 0x202: { // IRQ4 Line 2 frequency
      LOG_U16(("PC %08x - U16w IRQ4 f2=%03x     [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), data, offset, data, mem_mask));
      locals.u16IRQ4Line2Period = TIME_IN_SEC(((0x1000 - (data & 0x0FFF)) * 88.675) / (double)CPU_CLOCK);
      break;
    }
    default:
      LOG_U16(("PC %08x - U16w X Unsupported X [data@%03x=%04x] (%04x)\n", activecpu_get_pc(), offset, data, mem_mask));
  }

  if (offset < 0x0004)
    locals.u16a[offset] = (locals.u16a[offset] & mem_mask) | data;
  else
    locals.u16b[offset & 3] = (locals.u16b[offset & 3] & mem_mask) | data;
}

/*************/
/* I/O READ  */
/*************/
static READ16_HANDLER(io_r) {
  UINT16 data = 0;

  switch (offset) {
    ////////////////////////////// AUX_I/O => Power Driver Board - Lamp matrices (and strobed switch matrix for Breakshot)
    // Driver Board ID (to be checked from others manuals to be sure they all use the same Board IDs, on sheet 9 of Driver Board/Solenoid schematics)
    case 0x000008:
      data = HAS_SWITCH_BOARD ? 0x0300 : 0x0700; // From Airborne & Breakshot manuals
      break;
    // Read from other boards
    case 0x000006:
    case 0x00000a:
      data=0xffff;
      DBGLOG(("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask));
      //printf("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask);
      break;
    // Latched value of Lamp A & B Matrix Row Status after overcurrent comparator, used to determine non-functioning bulbs in diagnostic menu
    case 0x00000c:
      data = (~locals.lampA & 0xFF00) | (locals.lampA & 0x00FF);
      break;
    case 0x00000d:
      data = (~locals.lampB & 0xFF00) | (locals.lampB & 0x00FF);
      break;
    
    ////////////////////////////// EXT_I/O => Power Driver Board - Solenoids / Switch Board (not present for Breakshot)
    // Playfield Switches from switch board (if present)
    case 0x200008:
    case 0x200009:
    case 0x20000a:
    case 0x20000b:
      if (HAS_SWITCH_BOARD) {
        int swcol = offset-0x200007;
        data = (coreGlobals.swMatrix[swcol+4] << 8 | coreGlobals.swMatrix[swcol]) ^ 0xffff; //Switches are inverted
      }
      break;
    // Solenoid status
    //Sols: 1-8 (hi byte) & 17-24 (lo byte) status, including blank (from U16) and overcurrent state
    case 0x20000c:
      data = coreGlobals.binaryOutputState[(CORE_MODOUT_SOL0 + 16) >> 3] | (coreGlobals.binaryOutputState[(CORE_MODOUT_SOL0) >> 3] << 8);
      data = core_revword(data);
      break;
    //Sols: 9-16 (hi byte) & 24-32 (lo byte) status, including blank (from U16) and overcurrent state
    case 0x20000d:
      data = coreGlobals.binaryOutputState[(CORE_MODOUT_SOL0 + 24) >> 3] | (coreGlobals.binaryOutputState[(CORE_MODOUT_SOL0 + 8) >> 3] << 8);
      data = core_revword(data);
      break;

    ////////////////////////////// SWITCH0 => Read cabinet switches (no address decoding) / strobed switches
    //Cabinet/Coin Door Switches OR 8 cabinet switches and 8 strobed switches for Breakshot (which does not have the switch board)
    case 0x400000:
      if (HAS_SWITCH_BOARD) {
        data = coreGlobals.swMatrix[0] << 8 | coreGlobals.swMatrix[9];
      } else {
        // Cabinet/Coin Door Switches are read as the lower byte on all switch reads
        data = coreGlobals.swMatrix[9];
        switch(locals.swCol) {
          case 0x80: data |= coreGlobals.swMatrix[1]<<8; break;
          case 0x40: data |= coreGlobals.swMatrix[2]<<8; break;
          case 0x20: data |= coreGlobals.swMatrix[3]<<8; break;
          case 0x10: data |= coreGlobals.swMatrix[4]<<8; break;
          case 0x08: data |= coreGlobals.swMatrix[5]<<8; break;
          case 0x04: data |= coreGlobals.swMatrix[6]<<8; break;
          case 0x02: data |= coreGlobals.swMatrix[7]<<8; break;
          case 0x01: data |= coreGlobals.swMatrix[8]<<8; break;
        }
      }
      data ^= 0xffff; //Switches are inverted
      break;

    default:
      DBGLOG(("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask));
  }
  return data;
}

/*************/
/* I/O WRITE */
/*************/
static WRITE16_HANDLER(io_w) {
  UINT16 soldata;
  //DBGLOG(("io_w: [%08x] (%04x) = %x\n",offset,mem_mask,data));

#if LOG_LAMP_STROBE
  static double lampBStart, strobeLength;
#endif

  switch (offset) {
    ////////////////////////////// AUX_I/O => Power Driver Board - Lamp matrices (and strobed switch matrix for Breakshot)
    //Write to other boards
    case 0x000006:
    case 0x000007:
    case 0x00000a:
    case 0x00000b:
      DBGLOG(("PC%08x - io_w: [%08x] (%04x) = %x\n",activecpu_get_pc(),offset,mem_mask,data));
      break;
    
    // Lamp A & B VSET1/2: change voltage comparator reference for columns of matrix A & B => in turn, enable/disable column outputs
    // In fact, this is somewhat more complex since it toggles the VSet of the overcurrent voltage comparator from 0.128V to 0.291V.
    // Since #44 bulbs powered through 18V strobe have a high current surge when turning on a cold bulb of more than 10A leading to 
    // 0.212V at voltage comparator, this prevents turning on bulbs, but this will not turn off bulbs which are already on (resistance 
    // gets higher as filament get hotter, with a stable current around 714mA at 2700K, leading to 0.014V at voltage comparator)
    case 0x000008: {
      locals.lampAColDisable = (data >> 2) & 1;
      locals.lampBColDisable = (data >> 3) & 1;
      #if LOG_LAMP_STROBE
      static double blankingStart = 0.0;
      static double strobeStart = 0.0;
      if (locals.lampBColDisable)
         blankingStart = timer_get_time();
      else {
         printf("t=%8.5f Strobe=%5.2fms, Pulse=%5.2fms, Blanking=%5.2fms\n", timer_get_time(), (timer_get_time() - strobeStart) * 1000.0, (strobeLength / 8.0) * 1000.0, (timer_get_time() - blankingStart) * 1000.0);
         strobeStart = timer_get_time();
         strobeLength = 0.0;
      }
      #endif
      break;
    }
    //Lamp A Matrix OR Switch Strobe Column (Games with only Lamp B Matrix)
    case 0x00000c:
      if (HAS_SWITCH_BOARD) {
        UINT16 lampA = (data & 0xFF00) | (~data & 0x00FF);
        //printf("t=%8.5f Lamp A Write=%04x Data=%04x\n", timer_get_time(), lampA, data);
        if (locals.lampAColDisable) // lampAColDisable only prevents turning on cold bulbs, this is a hacky simplified implementation (we should cross columns and rows but it would be overkill)
          lampA &= 0x00FF; //lampA &= locals.lampA | 0x00FF;
        locals.lampA = lampA;
        core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0, (locals.lampA >> 8) & 0xFF, locals.lampA & 0xff, 8);
      } else {
        locals.swCol = data;
      }
      break;
    //Lamp B Matrix (for Breakshot that only have lamp B, shift to lower half of our lamp matrix for easier numbering)
    case 0x00000d: {
      UINT16 lampB = (data & 0xFF00) | (~data & 0x00FF);
      //printf("t=%8.5f Lamp B Write=%04x Data=%04x\n", timer_get_time(), lampB, data);
      if (locals.lampBColDisable) // lampBColDisable only prevents turning on cold bulbs, this is a hacky simplified implementation (we should cross columns and rows but it would be overkill)
        lampB &= 0x00FF; //lampB &= locals.lampB | 0x00FF;
      locals.lampB = lampB;
      #if LOG_LAMP_STROBE
      if (locals.lampB != 0)
        lampBStart = timer_get_time();
      else if (!locals.lampBColDisable)
        strobeLength += timer_get_time() - lampBStart;
      #endif
      core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + (core_gameData->hw.lampCol - 1) * 8, (locals.lampB >> 8) & 0xFF, locals.lampB & 0xff, 8);
      break;
    }

    ////////////////////////////// EXT_I/O => Power Driver Board - Solenoids / Switch Board (not present for Breakshot)
    // Voltage comparator level for switch banks 0..31 / 32..63
    case 0x200000:
      // Switch inputs can be either compared to GND or 4.2V, not sure why this would be used
      // locals.swLevel1 = data & 1;
      // locals.swLevel2 = (data >> 1) & 1;
      //printf("t=%8.5f Switch levels=%04x\n", timer_get_time(), data);
      break;
    //Sols: 1-8 (hi byte) & 17-24 (lo byte)
    case 0x20000c:
      soldata = core_revword(data^0xffff);
      coreGlobals.pulsedSolState = (soldata & 0x00ff)<<16;
      coreGlobals.pulsedSolState = (soldata & 0xff00)>>8;
      core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 16,  soldata       & 0xFF);
      core_write_pwm_output_8b(CORE_MODOUT_SOL0,      (soldata >> 8) & 0xFF);
      break;
    //Sols: 9-16 (hi byte) & 24-32 (lo byte)
    case 0x20000d:
      soldata = core_revword(data^0xffff);
      coreGlobals.pulsedSolState = (soldata & 0x00ff)<<24;
      coreGlobals.pulsedSolState = (soldata & 0xff00)>>0;
      core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 24,  soldata       & 0xFF);
      core_write_pwm_output_8b(CORE_MODOUT_SOL0 +  8, (soldata >> 8) & 0xFF);
      break;

    ////////////////////////////// SWITCH0 => 16 Cabinet switches
    // No write support

    default:
      DBGLOG(("PC%08x - io_w: [%08x] (%04x) = %x\n",activecpu_get_pc(),offset,mem_mask,data));
  }
}

static MACHINE_INIT(cc) {
  memset(&locals, 0, sizeof(locals));
  locals.u16a[0] = 0x00bc;

  // Force physical output emulation since this is needed for diagnostic LEDs and the driver is based on it
  options.usemodsol |= CORE_MODOUT_FORCE_ON;

  // If wanted, we can skip the startup error messages by patching the rom (NOTE: MUST COME BEFORE WE COPY ROMS BELOW)
#if SKIP_ERROR_MSG
  //NOTE: Due to the way we load the roms, the fixaddress values must remove the top 8th bit, i.e.
  //      0x10092192 becomes 0x00092192
  UINT32 fixaddr = 0;
  switch (core_gameData->hw.gameSpecific1) {
    case 0: break;
    case 1: case 2: fixaddr = 0x0009593c; break; //PM
    case 3:         fixaddr = 0x0008ce04; break; //AB
    case 4:         fixaddr = 0x00088204; break; //ABR
    case 5: case 6: fixaddr = 0x0008eb60; break; //BS
    case 7:         fixaddr = 0x0008a520; break; //BS102R
    case 8:         fixaddr = 0x000805e4; break; //BSB
    case 9:         fixaddr = 0x00000880; break; //FF103,FF104
    case 10:        fixaddr = 0x00051704; break; //BBB
    case 11:        fixaddr = 0x00000894; break; //KP
    case 12:        fixaddr = 0x00000784; break; //FF101
    case 13:        fixaddr = 0x0008a684; break; //PP100
    default: break;
   }
   //Skip Error Message Routine
   *((UINT16 *)(memory_region(REGION_USER1) + fixaddr))   = 0x4e75;	//RTS
#endif

  //Copy roms into correct location (ie, starting at 0x10000000 where they are mapped)
  memcpy(rom_base, memory_region(REGION_USER1), memory_region_length(REGION_USER1));
  //Copy 1st 0x100 bytes of rom into RAM for vector table
  memcpy(ramptr, memory_region(REGION_USER1), 0x100);

  // Initialize outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8; // Lamp Matrix A & B, +1 col for PWM diag LED
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps - 8, CORE_MODOUT_BULB_44_20V_DC_CC);
  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + coreGlobals.nLamps - 8, 8, CORE_MODOUT_NONE); // Unused
  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + coreGlobals.nLamps - 8, 1, CORE_MODOUT_LED); // CPU Board Diagnostic LED
  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + coreGlobals.nLamps - 7, 1, CORE_MODOUT_LED); // Sound Board Diagnostic LED
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 1, CORE_MODOUT_SOL_CUSTOM); // GameOn solenoid for Fast Flips
  // Game specific hardware
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
    rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
  // For flashers, Capcom uses #89 bulb wired through a STP20N10L Mosfet, 0.02 ohms resistor to a 20V DC source
  // which is very similar to what Williams uses on WPC hardware, so just uses CORE_MODOUT_BULB_89_20V_DC_WPC
  if (strncasecmp(gn, "abv", 3) == 0) { // Airborne
    coreGlobals.flipperCoils = 0xFFFFFFFFFFFF0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  } 
  else if (strncasecmp(gn, "bbb", 3) == 0) { // Big Bang Bar
    coreGlobals.flipperCoils = 0xFFFFFFFFFF0A0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "bsv", 3) == 0) { // Breakshot
    coreGlobals.flipperCoils = 0xFFFFFFFFFF0A0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC); // Center pocket Flasher
    // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 27 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC); // Plunger Flasher (appears in doc but was not kept in production)
  }
  else if (strncasecmp(gn, "ffv", 3) == 0) { // Flipper Football
    coreGlobals.flipperCoils = 0xFFFFFFFF0B0A0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "kpb", 3) == 0) { // KingPin
    // To be checked since this is from VPX table (did not find a manual for this one)
    coreGlobals.flipperCoils = 0xFFFFFFFFFFFF0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1,  2, CORE_MODOUT_BULB_89_20V_DC_WPC);
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 11, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "pmv", 3) == 0) { // Pinball Magic
    coreGlobals.flipperCoils = 0xFFFFFFFFFFFF0908ull;
    core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 11, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  // Defaults to 2 state legacy integrator for better backward compatibility
  if ((options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)) == 0)
     for (int i = 0; i < coreGlobals.nSolenoids; i++)
        if (coreGlobals.physicOutputState[i].type != CORE_MODOUT_SOL_2_STATE)
           core_set_pwm_output_type(CORE_MODOUT_SOL0, 1, CORE_MODOUT_LEGACY_SOL_2_STATE);
  if ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_LAMPS) == 0)
     core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80 /*coreGlobals.nLamps*/, CORE_MODOUT_LEGACY_SOL_2_STATE);
  if ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_GI) == 0)
     core_set_pwm_output_type(CORE_MODOUT_GI0, coreGlobals.nGI, CORE_MODOUT_LEGACY_SOL_2_STATE);

  //Init soundboard
  sndbrd_0_init(core_gameData->hw.soundBoard, CAPCOMS_CPUNO, memory_region(CAPCOMS_ROMREGION),NULL,NULL);

  locals.u16IRQ1timer = timer_alloc(cc_u16irq1);
  locals.u16IRQ4Line1timer = timer_alloc(cc_u16irq4line1);
  locals.u16IRQ4Line2timer = timer_alloc(cc_u16irq4line2);
  locals.u16IRQ4Line3timer = timer_alloc(cc_u16irq4line3);
  // defaults IRQ periods
  locals.u16IRQ1Period = TIME_IN_SEC(22668 / (double)CPU_CLOCK);
  locals.u16IRQ4Line1Period = TIME_IN_SEC(((0x1000 - (0x0800 & 0x0FFF)) * 88.675) / (double)CPU_CLOCK);
  locals.u16IRQ4Line2Period = TIME_IN_SEC(((0x1000 - (0x0800 & 0x0FFF)) * 88.675) / (double)CPU_CLOCK);
  // Flipper Football frequency for IRQ4 Line 3 is half of other games => likely DMD vblank since DMD has twice more lines (so 45.9Hz for FF, 91.8Hz for others)
  int defaultLine3 = (core_gameData->hw.gameSpecific1 == 9 || core_gameData->hw.gameSpecific1 == 12) ? 0x0000 : 0x0800;
  locals.u16IRQ4Line3Period = TIME_IN_SEC(((0x1000 - (defaultLine3 & 0x0FFF)) * 88.675) / (double)CPU_CLOCK);

#if TEST_MPGAUDIO
  //Freeze cpu so it won't slow down the emulation
  cpunum_set_halt_line(0,1);
#endif
}

static MACHINE_STOP(cc)
{
  sndbrd_0_exit();
}

// Show Sound & DMD Diagnostic LEDS
void cap_UpdateSoundLEDS(int data)
{
   core_write_masked_pwm_output_8b(CORE_MODOUT_LAMP0 + 8 * 8 + (core_gameData->hw.lampCol - 1) * 8, data << 1, 0x02);
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*

//See notes near PORTA & B for details on Port Memory

CS0 ROM
CS1 DRAM
CS2 I/O
CS3 NVRAM (D0-D7, A0-A14)
CS4 A20
CS5 A21
CS6 A22
CS7 A23
!I/O & A23

AT BOOT TIME:

CS0
0x00000000-0x000fffff : ROM0 (U1)
0x00400000-0x004fffff : ROM1 (U2)
0x00800000-0x008fffff : ROM2 (U3)
0x00c00000-0x00cfffff : ROM3 (U4)

AFTER CS0 is configured by software:

0x10000000-0x100fffff : ROM0 (U1)
0x10400000-0x104fffff : ROM1 (U2)
0x10800000-0x108fffff : ROM2 (U3)
0x10c00000-0x10cfffff : ROM3 (U4)

CS0 defined by writes to internal registers @ ffc0,ffc2
...
CS7 defined by writes to internal registers @ ffdc,ffde

From the very beginning of program execution, we see writes to the following registers, each bit specifies how CS0-CS7 will act:
Note: Invert the mask to see the end address range

CHIP  DATA        MASK     ADDRESS     RANGE             R/W
--------------------------------------------------------------
CS0 = 1000e680 => ff000000,10000000 => 10000000-10ffffff (R)
CS1 = 0001a2df => fff80000,00000000 => 00000000-0007ffff (RW) * Note: CSFC6,2 = 0, CSFC5,1 = 1
CS2 = 4001a280 => ff000000,40000000 => 40000000-40ffffff (RW)
CS3 = 3001a2f0 => fffe0000,30000000 => 30000000-3001ffff (RW)

On flipper football & kingpin:
CS1 = 0001e6df => fff80000,00000000 => 00000000-0007ffff (RW) * Note: CSFC6,5,2,1 = 1

To optimize speed on the 68306, I removed all handling of the cs/dram registers.
So we simply use the memory mapping that the game software uses when it first writes to those
registers.. The only potential problem is if the game writes different values to cs/dram signals
during program execution which could change these mappings!

*/

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT16 *CMOS;

static NVRAM_HANDLER(cc) {
  core_nvram(file, read_or_write, CMOS, 0x10000, 0x00);
}

static int cc_sw2m(int no) {
  if (no < 9)
    return no + 71;
  else if (no < 81)
    return no - 9;
  return no + 7;
}

static int cc_m2sw(int col, int row) {
  if (col == 9)
    return row + 1;
  return col*8 + row + 9;
}

/*
From watching the program code set the cs registers.. (see explanation above)
CS0 = 10000000-10ffffff (R)  -  ROM MEMORY SPACE
CS1 = 00000000-0007ffff (RW) -  DRAM
CS2 = 40000000-40ffffff (RW) -  I/O ADDRESSES
CS3 = 30000000-3001ffff (RW) -  NVRAM

Therefore:
 B A B=A23,A=A22 - Base address = 40000000
----------------------
 0 0 (40000000) - /AUX I-O => Power Driver Board - Lamp matrices (and strobed switch matrix for Breakshot)
 0 1 (40400000) - /EXT I-O => Power Driver Board - Solenoids / Switch Board (not present for Breakshot)
 1 0 (40800000) - /SWITCH0 => 16 Cabinet switches
 1 1 (40c00000) - /CS      => U16 custom chip
*/
static MEMORY_READ16_START(cc_readmem)
  { 0x00000000, 0x0007ffff, MRA16_RAM },			/* DRAM */
  { 0x10000000, 0x10ffffff, MRA16_ROM },			/* ROMS */
  { 0x30000000, 0x3001ffff, MRA16_RAM },			/* NVRAM */
  { 0x40000000, 0x40bfffff, io_r },					/* I/O */
  { 0x40c00000, 0x40c007ff, u16_r },				/* U16 (A10,A2,A1)*/
MEMORY_END

static MEMORY_WRITE16_START(cc_writemem)
  { 0x00000000, 0x0007ffff, MWA16_RAM, &ramptr },	/* DRAM */
  { 0x10000000, 0x10ffffff, MWA16_ROM, &rom_base },	/* ROMS */
  { 0x30000000, 0x3001ffff, MWA16_RAM, &CMOS },		/* NVRAM */
  { 0x40000000, 0x40bfffff, io_w },					/* I/O */
  { 0x40c00000, 0x40c007ff, u16_w },				/* U16 (A10,A2,A1)*/
MEMORY_END

static PORT_READ16_START(cc_readport)
  { M68306_PORTA_START, M68306_PORTA_END, cc_porta_r },
  { M68306_PORTB_START, M68306_PORTB_END, cc_portb_r },
PORT_END
static PORT_WRITE16_START(cc_writeport)
  { M68306_PORTA_START, M68306_PORTA_END, cc_porta_w },
  { M68306_PORTB_START, M68306_PORTB_END, cc_portb_w },
PORT_END

//Default hardware - no sound support
MACHINE_DRIVER_START(cc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(cc, NULL, cc)
  MDRV_CPU_ADD(M68306, CPU_CLOCK)
  MDRV_CPU_MEMORY(cc_readmem, cc_writemem)
  MDRV_CPU_PORTS(cc_readport, cc_writeport)
  MDRV_CPU_VBLANK_INT(cc_vblank, 1)
  MDRV_NVRAM_HANDLER(cc)
  MDRV_SWITCH_UPDATE(cc)
  MDRV_SWITCH_CONV(cc_sw2m,cc_m2sw)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_TIMER_ADD(cc_zeroCross, CC_ZCFREQ)
MACHINE_DRIVER_END

//Sound hardware - 1 x TMS320AV120
MACHINE_DRIVER_START(cc1)
  MDRV_IMPORT_FROM(cc)
  MDRV_IMPORT_FROM(capcom1s)
MACHINE_DRIVER_END
//Sound hardware - 2 x TMS320AV120
MACHINE_DRIVER_START(cc2)
  MDRV_IMPORT_FROM(cc)
  MDRV_IMPORT_FROM(capcom2s)
MACHINE_DRIVER_END


/********************************/
/*** 128 X 32 NORMAL SIZE DMD ***/
/********************************/
PINMAME_VIDEO_UPDATE(cc_dmd128x32) {
  const UINT16 *RAM = ramptr + 0x800 * locals.visible_page + 0x10;
  UINT8* line = &coreGlobals.dmdDotRaw[0];
  for (int ii = 0; ii < 32; ii++) {
    for (int kk = 0; kk < 16; kk++) {
      UINT16 intens1 = RAM[0];
      for(int jj = 0; jj < 8; jj++) {
         *line++ = (intens1 >> 14) & 0x0003;
         intens1 <<= 2;
      }
      RAM += 1;
    }
    RAM += 16;
  }
  core_dmd_video_update(bitmap, cliprect, layout, NULL);
  return 0;
}

/*******************************/
/*** 256 X 64 SUPER HUGE DMD ***/
/*******************************/
PINMAME_VIDEO_UPDATE(cc_dmd256x64) {
  const UINT16 *RAM = ramptr + 0x800 * locals.visible_page;
  for (int ii = 0; ii < 64; ii++) {
    UINT8 *linel = &coreGlobals.dmdDotRaw[ii * layout->length];
    UINT8 *liner = &coreGlobals.dmdDotRaw[ii * layout->length + 128];
    for (int kk = 0; kk < 16; kk++) {
      UINT16 intensl = RAM[0];
      UINT16 intensr = RAM[0x10];
      for(int jj=0;jj<8;jj++) {
         *linel++ = (intensl >> 14) & 0x0003;
         intensl <<= 2;
         *liner++ = (intensr >> 14) & 0x0003;
         intensr <<= 2;
      }
      RAM += 1;
    }
    RAM += 16;
  }
  core_dmd_video_update(bitmap, cliprect, layout, NULL);
  return 0;
}

// Goofy Hoops - using Q-Sound chip and different memory / port mapping on 68306 MPU 

static data16_t *rom_base2;

static void romstar_irq1(int data) {
  cpu_set_irq_line(0, MC68306_IRQ_1, PULSE_LINE);
}

static MACHINE_INIT(romstar) {
  memset(&locals, 0, sizeof(locals));

  //Copy roms into correct location (ie. starting at 0x10000000 / 0x20000000 where they are mapped)
  memcpy(rom_base, memory_region(REGION_USER1), 0x100000);
  memcpy(rom_base2, memory_region(REGION_USER1) + 0x400000, 0x100000);
  //Copy 1st 0x400 bytes of rom into RAM for vector table
  memcpy(ramptr, memory_region(REGION_USER1), 0x400);

  //Init soundboard (needed for sound commander to work)
  sndbrd_0_init(core_gameData->hw.soundBoard, CAPCOMS_CPUNO, memory_region(CAPCOMS_ROMREGION),NULL,NULL);

  //IRQ1 clock - trying to come as near as possible to the real thing
  timer_pulse(TIME_IN_HZ(970), 0, romstar_irq1);

  //IRQ4 clock - might be unused, still pulse it because we don't know what that U8 chip does...
  /* FIXME timer_pulse(TIME_IN_HZ(100), 0, cc_u16irq4); */
  
  // FIXME missing physic output definition
}

static INTERRUPT_GEN(romstar_vblank) {
  coreGlobals.solenoids = locals.solenoids;
  locals.solenoids = 0;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(romstar) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xcc, 3);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xff, 4);
  }
}

static READ16_HANDLER(sw_r) {
  if (offset) {
    return coreGlobals.swMatrix[1] | (coreGlobals.swMatrix[3] << 8);
  }
  return coreGlobals.swMatrix[2] | (coreGlobals.swMatrix[4] << 8);
}

static READ16_HANDLER(snd_r) {
  return qsound_status_r(0);
}

static WRITE16_HANDLER(snd_w) {
  static UINT8 cmd;
  if (offset) {
    qsound_data_h_w(0, data >> 8);
    qsound_data_l_w(0, data & 0xff);
    qsound_cmd_w(0, cmd);
  } else {
    cmd = data & 0xff;
  }
}

static WRITE16_HANDLER(vol_w) {
  if (data == 0x82 && locals.vset) locals.vset--;
  if (data == 0xaa && locals.vset < 100) locals.vset++;
  mixer_set_volume(0, locals.vset);
  mixer_set_volume(1, locals.vset);
}

static WRITE16_HANDLER(disp_w) {
  if (!offset) locals.visible_page = 0x70 | data;
}

static WRITE16_HANDLER(sol_w) {
  locals.solenoids |= data << (offset * 8);
  coreGlobals.solenoids = locals.solenoids;
}

static WRITE16_HANDLER(lamp_w) {
  if (locals.swCol >= 0) {
    coreGlobals.tmpLampMatrix[locals.swCol] |= core_revbyte(data & 0xff);
    coreGlobals.lampMatrix[locals.swCol] = coreGlobals.tmpLampMatrix[locals.swCol];
  }
}

static WRITE16_HANDLER(col_w) {
  if (!data) {
    memset((void*)coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  locals.swCol = data ? 7 - core_BitColToNum(data & 0xff) : -1;
}

static MEMORY_READ16_START(romstar_readmem)
  { 0x00000000, 0x0007ffff, MRA16_RAM },
  { 0x10000000, 0x100fffff, MRA16_ROM },
  { 0x20000000, 0x200fffff, MRA16_ROM },
  { 0x30000000, 0x3000ffff, MRA16_RAM },
  { 0x50000008, 0x50000009, snd_r },
  { 0x60000000, 0x60000003, sw_r },
MEMORY_END

static MEMORY_WRITE16_START(romstar_writemem)
  { 0x00000000, 0x0007ffff, MWA16_RAM, &ramptr },
  { 0x10000000, 0x100fffff, MWA16_ROM, &rom_base },
  { 0x20000000, 0x200fffff, MWA16_ROM, &rom_base2 },
  { 0x30000000, 0x3000ffff, MWA16_RAM, &CMOS },
  { 0x40000000, 0x4000ffff, disp_w },
  { 0x50000008, 0x5000000b, snd_w },
  { 0x5000000c, 0x5000000d, vol_w },
  { 0x60000004, 0x60000009, sol_w },
  { 0x6000000a, 0x6000000b, lamp_w },
  { 0x6000000c, 0x6000000d, col_w },
MEMORY_END

static READ16_HANDLER(romstar_port_r) {
  return 0xffff; // no idea, schematics show all-pullup resistors
}

static WRITE16_HANDLER(romstar_port_w) {
  coreGlobals.diagnosticLed = data & 8 ? 0 : 1;
}

static PORT_READ16_START(romstar_readport)
  {0, 1, romstar_port_r},
PORT_END

static PORT_WRITE16_START(romstar_writeport)
  {0, 1, romstar_port_w},
PORT_END

static struct QSound_interface romstar_qsoundInt = {
  QSOUND_CLOCK,
  CAPCOMS_ROMREGION,
  {MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT)}
};

MACHINE_DRIVER_START(romstar)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(romstar, NULL, cc)
  MDRV_CPU_ADD(M68306, CPU_CLOCK)
  MDRV_CPU_MEMORY(romstar_readmem, romstar_writemem)
  MDRV_CPU_PORTS(romstar_readport, romstar_writeport)
  MDRV_CPU_VBLANK_INT(romstar_vblank, 1)
  MDRV_NVRAM_HANDLER(cc)
  MDRV_SWITCH_UPDATE(romstar)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_ADD(QSOUND, romstar_qsoundInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END
