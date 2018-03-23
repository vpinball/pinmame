#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"

/*
   Bally's Bow & Arrow (1976),
   their prototype game for the -17/35 line of games.

   Note there is no extra display for ball in play,
   they just used 5 lights on the backglass.
   Also the lamps are accessed along with the displays!
   Since this required a different lamp strobing,
   I introduced a new way of arranging the lamps which
   makes it easier to map the lights, and saves a row.
 */

#define BY35_PIA0 0
#define BY35_PIA1 1

#define BY35_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKs */
#define BY35_SOLSMOOTH       2 /* Smooth the solenoids over this numer of VBLANKs */
#define BY35_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKs */

static struct {
  int a0, a1, b1, ca10, ca11, ca20, cb21;
  int bcd[7], lastbcd;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int vblankCount, resetSw;
} locals;

static void piaIrq(int num, int state) {
  static int irqstates[2], oldstate;
  int irqstate;
  irqstates[num] = state;
  irqstate = irqstates[0] || irqstates[1];
  if (oldstate != irqstate) {
    cpu_set_irq_line(0, M6800_IRQ_LINE, irqstate ? ASSERT_LINE : CLEAR_LINE);
  }
  oldstate = irqstate;
}

static void piaIrq0(int state) { piaIrq(0, state); }
static void piaIrq1(int state) { piaIrq(1, state); }

static void by35_dispStrobe(int mask) {
  int digit = locals.a1 & 0xfe;
  int ii,jj;

  for (ii = 0; digit; ii++, digit>>=1) {
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          locals.segments[jj*8+ii].w |= locals.pseg[jj*8+ii].w = core_bcd2seg7[locals.bcd[jj] & 0x0f];
    }
  }
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
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY35_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % BY35_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
  }
  core_updateSw(core_getSol(18));
}

static SWITCH_UPDATE(by35) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>12,0x01,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x01,3);
    CORE_SETKEYSW(inports[CORE_COREINPORT],    0x1f,4);
  }
  locals.resetSw = coreGlobals.swMatrix[0];
}

static void by35p_lampStrobe(void) {
  int strobe = locals.a1 >> 2;
  int ii,jj;
  for (ii = 0; strobe; ii++, strobe>>=1) {
    if (strobe & 0x01)
      for (jj = 0; jj < 5; jj++) {
        int lampdata = (locals.bcd[jj]>>4)^0x0f;
        int lampadr = ii*5 + jj;
        coreGlobals.tmpLampMatrix[lampadr/2] |= (lampadr%2 ? lampdata << 4 : lampdata);
      }
  }
}

// buffer lamps & display digits
static WRITE_HANDLER(piap0a_w) {
  locals.a0 = data;
  if (!locals.ca20 && locals.lastbcd)
    locals.bcd[--locals.lastbcd] = data;
}

// switches & dips (inverted)
static READ_HANDLER(piap0b_r) {
  UINT8 sw = 0xff;
  if (!locals.resetSw)
    sw = core_revbyte(memory_region(REGION_CPU1)[0x200]); // reads back credits if locals.a0 is 0 (at game start)
  if (locals.a0 & 0x10) sw = core_getDip(0); // DIP#1 1-8
  else if (locals.a0 & 0x20) sw = core_getDip(1); // DIP#2 9-16
  else if (locals.a0 & 0x40) sw = core_getDip(2); // DIP#3 17-24
  else if (locals.a0 & 0x80) sw = core_getDip(3); // DIP#4 25-32
  else if (locals.a0) sw = core_getSwCol(locals.a0 & 0x0f);
  return core_revbyte(sw);
}

// writes out credits when NMI is fired, so buffer it
static WRITE_HANDLER(piap0b_w) {
  memory_region(REGION_CPU1)[0x200] = data;
}

// display strobe
static WRITE_HANDLER(piap0ca2_w) {
  if (data & ~locals.ca20) {
    locals.lastbcd = 5;
  } else if (~data & locals.ca20) {
    by35_dispStrobe(0x1f);
    by35p_lampStrobe();
  }
  locals.ca20 = data;
}

// set display row
static WRITE_HANDLER(piap1a_w) {
  locals.a1 = data;
}

// solenoids
static WRITE_HANDLER(piap1b_w) {
  locals.b1 = data;
  coreGlobals.pulsedSolState = 0;
  if (locals.cb21)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff87fff) | ((data & 0xf0)<<11);
  locals.solenoids |= (data & 0xf0)<<11;
}

//diag. LED
static WRITE_HANDLER(piap1ca2_w) {
  coreGlobals.diagnosticLed = data;
}

// solenoid control?
static WRITE_HANDLER(piap1cb2_w) {
  locals.cb21 = data;
}

static READ_HANDLER(piap0ca1_r) {
  return locals.ca10;
}

static READ_HANDLER(piap1ca1_r) {
  return locals.ca11;
}

static struct pia6821_interface by35Proto_pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, piap0b_r, piap0ca1_r,0, 0,0,
/* O:  A/B,CA2/B2        */  piap0a_w,piap0b_w, piap0ca2_w,0,
/* IRQ: A/B              */  piaIrq0,0
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0, 0, piap1ca1_r,0, 0,0,
/* O:  A/B,CA2/B2        */  piap1a_w,piap1b_w, piap1ca2_w,piap1cb2_w,
/* IRQ: A/B              */  piaIrq1,0
}};

static INTERRUPT_GEN(byProto_irq) {
  locals.ca10 = !locals.ca10;
  pia_set_input_ca1(BY35_PIA0, locals.ca10);
}

static void by35p_zeroCross(int data) {
  locals.ca11 = !locals.ca11;
  pia_set_input_ca1(BY35_PIA1, locals.ca11);
}

static MACHINE_INIT(by35Proto) {
  static int afterInit;
  int resetSw = afterInit ? locals.resetSw : 0;
  memset(&locals, 0, sizeof(locals));
  locals.resetSw = resetSw;

  pia_config(BY35_PIA0, PIA_STANDARD_ORDERING, &by35Proto_pia[0]);
  pia_config(BY35_PIA1, PIA_STANDARD_ORDERING, &by35Proto_pia[1]);
  afterInit = 1;
}

static MACHINE_RESET(by35) {
  pia_reset();
  locals.vblankCount = 1;
}

static MACHINE_STOP(by35Proto) {
  cpu_set_nmi_line(0, PULSE_LINE); // NMI routine saves the credits!
  run_one_timeslice(); // wait two timeslices before shutdown so the NMI routine can finish
  run_one_timeslice();
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
static MEMORY_READ_START(by35_readmem)
  { 0x0000, 0x007f, MRA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_r(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_r(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x0200, MRA_RAM }, /* one byte for keeping credits */
  { 0x1000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(by35_writemem)
  { 0x0000, 0x007f, MWA_RAM }, /* U7 128 Byte Ram*/
  { 0x0088, 0x008b, pia_w(BY35_PIA0) }, /* U10 PIA: Switchs + Display + Lamps*/
  { 0x0090, 0x0093, pia_w(BY35_PIA1) }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0200, 0x0200, MWA_RAM, &generic_nvram, &generic_nvram_size }, /* one byte for keeping credits */
MEMORY_END

MACHINE_DRIVER_START(byProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by35Proto,by35,by35Proto)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 560000)
  MDRV_CPU_MEMORY(by35_readmem, by35_writemem)
  MDRV_CPU_VBLANK_INT(by35_vblank, 1)
  MDRV_CPU_PERIODIC_INT(byProto_irq, 317 * 2)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(by35)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(by35p_zeroCross, 120 * 2) // won't work with 100Hz, ie. not in Europe!
MACHINE_DRIVER_END

#define BY35PROTO_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 0 */ \
    COREPORT_BIT(     0x1000, "Clear Credits",    KEYCODE_0) \
    /* Switch Column 3 */ \
    COREPORT_BIT(     0x0100, "Ball Tilt",        KEYCODE_INSERT) \
    /* Switch Column 4 */ \
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0002, "Self Test",        KEYCODE_7) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0010, "Slam Tilt",        KEYCODE_HOME)  \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0003, 0x0000, "Chute #1 Coins") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0002, "2" ) \
      COREPORT_DIPSET(0x0001, "3" ) \
      COREPORT_DIPSET(0x0003, "4" ) \
    COREPORT_DIPNAME( 0x000c, 0x0000, "Chute #1 Credits") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0008, "2" ) \
      COREPORT_DIPSET(0x0004, "3" ) \
      COREPORT_DIPSET(0x000c, "4" ) \
    COREPORT_DIPNAME( 0x00f0, 0x00f0, "Replay #2/#1+") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "4k" ) \
      COREPORT_DIPSET(0x0040, "8k" ) \
      COREPORT_DIPSET(0x00c0, "12k" ) \
      COREPORT_DIPSET(0x0020, "16k" ) \
      COREPORT_DIPSET(0x00a0, "20k" ) \
      COREPORT_DIPSET(0x0060, "24k" ) \
      COREPORT_DIPSET(0x00e0, "28k" ) \
      COREPORT_DIPSET(0x0010, "32k" ) \
      COREPORT_DIPSET(0x0090, "36k" ) \
      COREPORT_DIPSET(0x0050, "40k" ) \
      COREPORT_DIPSET(0x00d0, "44k" ) \
      COREPORT_DIPSET(0x0030, "48k" ) \
      COREPORT_DIPSET(0x00b0, "52k" ) \
      COREPORT_DIPSET(0x0070, "56k" ) \
      COREPORT_DIPSET(0x00f0, "60k" ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Chute #2 Coins") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0100, "3" ) \
      COREPORT_DIPSET(0x0300, "4" ) \
    COREPORT_DIPNAME( 0x0c00, 0x0000, "Chute #2 Credits") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0800, "2" ) \
      COREPORT_DIPSET(0x0400, "3" ) \
      COREPORT_DIPSET(0x0c00, "4" ) \
    COREPORT_DIPNAME( 0xf000, 0xf000, "Replay #3/#2+") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "4k" ) \
      COREPORT_DIPSET(0x4000, "8k" ) \
      COREPORT_DIPSET(0xc000, "12k" ) \
      COREPORT_DIPSET(0x2000, "16k" ) \
      COREPORT_DIPSET(0xa000, "20k" ) \
      COREPORT_DIPSET(0x6000, "24k" ) \
      COREPORT_DIPSET(0xe000, "28k" ) \
      COREPORT_DIPSET(0x1000, "32k" ) \
      COREPORT_DIPSET(0x9000, "36k" ) \
      COREPORT_DIPSET(0x5000, "40k" ) \
      COREPORT_DIPSET(0xd000, "44k" ) \
      COREPORT_DIPSET(0x3000, "48k" ) \
      COREPORT_DIPSET(0xb000, "52k" ) \
      COREPORT_DIPSET(0x7000, "56k" ) \
      COREPORT_DIPSET(0xf000, "60k" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Blink display") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Melody") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "Incentive") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "Score award") \
      COREPORT_DIPSET(0x0000, "Extra Ball" ) \
      COREPORT_DIPSET(0x0008, "Credit" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "Special award") \
      COREPORT_DIPSET(0x0000, "Extra Ball" ) \
      COREPORT_DIPSET(0x0010, "Credit" ) \
    COREPORT_DIPNAME( 0x00e0, 0x0020, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0080, "2" ) \
      COREPORT_DIPSET(0x0040, "3" ) \
      COREPORT_DIPSET(0x00c0, "4" ) \
      COREPORT_DIPSET(0x0020, "5" ) \
      COREPORT_DIPSET(0x00a0, "6" ) \
      COREPORT_DIPSET(0x0060, "7" ) \
      COREPORT_DIPSET(0x00e0, "8" ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "Match feature") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "Replay #1+128k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Replay #1+64k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Replay #1+32k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Replay #1+16k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Replay #1+8k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Replay #1+4k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "Replay #1+2k") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

static const core_tLCDLayout dispBA[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 2,34,2,CORE_SEG7}, {4, 8,38,2,CORE_SEG7},{0}
};
static core_tGameData bowarrowGameData = {GEN_BYPROTO,dispBA,{FLIP_SW(FLIP_L),0,7}};
static void init_bowarrow(void) {
  core_gameData = &bowarrowGameData;
}
ROM_START(bowarrow) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("b14.bin", 0x1400, 0x0200, CRC(d4d0f92a) SHA1(b996cbe9762fafd64115dc78e24626cf08f8abf7)) \
    ROM_LOAD("b16.bin", 0x1600, 0x0200, CRC(ad2102e7) SHA1(86887beea5e03e80f60c947d6d71431e5eab3d1b)) \
    ROM_LOAD("b18.bin", 0x1800, 0x0200, CRC(5d84656b) SHA1(d17350f5a0cc0cd00b60df4903034489dce7ade5)) \
    ROM_LOAD("b1a.bin", 0x1a00, 0x0200, CRC(6f083ce6) SHA1(624b00e72e223c6b9fbf38b831200c9a7aa0d8f7)) \
    ROM_LOAD("b1c.bin", 0x1c00, 0x0200, CRC(6ed4d39e) SHA1(1f6c57c7274c76246dd2f0b70ec459857a5cf1eb)) \
    ROM_LOAD("b1e.bin", 0x1e00, 0x0200, CRC(ff2f97de) SHA1(28a8fdeccb1382d3a1153c97466426459c9fa075)) \
      ROM_RELOAD( 0xfe00, 0x0200) \
ROM_END
INPUT_PORTS_START(bowarrow) \
  CORE_PORTS \
  SIM_PORTS(1) \
  BY35PROTO_COMPORTS \
INPUT_PORTS_END
CORE_GAMEDEFNV(bowarrow,"Bow & Arrow (Prototype, rev. 23)",1976,"Bally",byProto,GAME_USES_CHIMES)

ROM_START(bowarroa) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("u42704", 0x1400, 0x0200, CRC(b2ccd455) SHA1(07ba19ce2bcd0d2d0d27cab5aafd510423d2e8fe)) \
    ROM_LOAD("u32704", 0x1600, 0x0200, CRC(ec673d77) SHA1(f350df32182da4958b4607db6952ffce89cda18f)) \
    ROM_LOAD("u22704", 0x1800, 0x0200, CRC(4b4512da) SHA1(b427c504667769bd3b5c78ad3866c750cb7b1ed4)) \
    ROM_LOAD("u12704", 0x1a00, 0x0200, CRC(a35e7473) SHA1(d5cadd968cad931dfe92d6ba4f013844f5b5d244)) \
    ROM_LOAD("u62704", 0x1c00, 0x0200, CRC(8c421ae4) SHA1(93fcf26667308467215d35685c1acb04b0555d6b)) \
    ROM_LOAD("u52704", 0x1e00, 0x0200, CRC(a6644eee) SHA1(8afa941d1dd94308272bbc1874603d3ed41d3b89)) \
      ROM_RELOAD( 0xfe00, 0x0200) \
ROM_END
#define init_bowarroa init_bowarrow
#define input_ports_bowarroa input_ports_bowarrow
CORE_CLONEDEFNV(bowarroa,bowarrow,"Bow & Arrow (Prototype, rev. 22)",1976,"Bally",byProto,GAME_USES_CHIMES)
