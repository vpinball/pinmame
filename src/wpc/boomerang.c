#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"

/*
   Bally's Boomerang (1975), their prototype game *before* Bow & Arrow.
   No game ever saw the light of day; only one engineering sample was made for testing at Bally factory.

   Notes:
   - The original code was scanned in from US patent #4,198,051 using OCR software, and manually corrected afterwards.
     This patent code contains numerous typos, omissions, and plain errors, so it's well possible there are still a few bugs left.
   - No test routine exists.
   - No non-volatile RAM exists. A fake NVRAM handler is in place to rescue the DIP settings.
   - Setting the "replay score to beat" threshold is quite crude;
     once a 10,000 multiplier is set, all scores starting from 10,000 * x + 1,000 award a replay, disregarding the 1,000 multiplier.
     So when a threshold of 55,000 points is defined, a replay will be awarded for 51,000 points already!
     Only when the 10,000 multiplier is set to 0, a replay is awarded when reaching a score of 1,000 * x + 1,000.
   - Setting a value of 0 in total will give a replay for every game played.
   - Scores above 100,000 points will *not* award a replay unless the 10,000 units surpass the threshold again.
   - Score replays will only be awarded at the end of the game, after the match routine.
   Bugs / fixed in the bootleg version:
   - Extra ball outhole lamps are not correctly alternating for bonus >= 8,000
   - Backglass lamps for Ball in play #5 and #6 are not driven
   - Triggering the slam switch will put the code in an endless loop until game is reset
 */

#define PIA1 0
#define PIA2 1
#define PIA3 2

static struct {
  int a1, b1, a2, b2, cb12, a3;
  int dspIrq, zc, regNo, lampCol;
  UINT8 segs[6];
} locals;

static READ_HANDLER(pia1a_r) {
  int swCol = core_BitColToNum(locals.a1 & 0x7f);
  if (swCol < 4) {
    return (locals.a1 & 0x7f) | ((coreGlobals.swMatrix[swCol + 1] << locals.regNo) & 0x80);
  } else {
    return (locals.a1 & 0x7f) | ((core_getDip(swCol - 4) << locals.regNo) & 0x80);
  }
}

static READ_HANDLER(pia1b_r) {
//  logerror("PIA1 B R %02x\n", locals.b1);
  return locals.b1;
}

static WRITE_HANDLER(pia1a_w) {
  locals.a1 = data;
}

static WRITE_HANDLER(pia1b_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xff00) | (data ^ 0xff);
  locals.b1 = data;
}

static WRITE_HANDLER(pia1ca2_w) {
  if (data) {
    locals.regNo++;
  }
}

static READ_HANDLER(pia2b_r) {
//  logerror("PIA2 B R %02x\n", locals.b2);
  return locals.b2;
}

static READ_HANDLER(pia2cb1_r) { // slam switch
  return locals.cb12;
}

static READ_HANDLER(pia2cb2_r) { // zc detection
//  logerror("PIA2 CB2 R %x (ZC)\n", locals.zc);
  return locals.zc;
}

static WRITE_HANDLER(pia2a_w) {
  if (locals.lampCol >= 0) {
    coreGlobals.lampMatrix[locals.lampCol] = ~data;
    locals.lampCol = -1;
  }
}

static WRITE_HANDLER(pia2b_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x00ff) | ((~data & 0xf8) << 5);
  locals.b2 = data;
  locals.lampCol = data & 0x07;
}

static WRITE_HANDLER(pia2cb2_w) {
  if (data) {
    locals.regNo = 0;
  }
}

static READ_HANDLER(pia3a_r) {
//  logerror("PIA3 A R %02x\n", locals.a3);
  return locals.a3;
}

static READ_HANDLER(pia3ca1_r) { // display interrupt
  return locals.dspIrq;
}

static READ_HANDLER(pia3ca2_r) {
//  logerror("PIA3 CA2 R\n");
  return locals.dspIrq;
}

static WRITE_HANDLER(pia3a_w) {
  int i, digit = data & 0x0f;
  if (digit < 6) {
    locals.segs[digit] = core_bcd2seg7[data >> 4];
	} else if (digit > 8 && digit < 15) {
    digit = 14 - digit;
    for (i = 0; i < 6; i++) {
      coreGlobals.segments[6 * i + digit].w = locals.segs[i];
    }
  }
  locals.a3 = data;
}

static WRITE_HANDLER(pia3b_w) {
  logerror("PIA3 B W %02x\n", data);
}

static WRITE_HANDLER(pia3ca2_w) {
  logerror("PIA3 CA2 W %x\n", data);
}

/*
PIA1:
I: A7     serial input
O: A0-A6  switch strobe
   B      solenoid data
   CA2    shift register clock

PIA2:
I: CB1    slam switch
   CB2    zc detection
O: A      lamp data
   B0-B2  lamp row
   B3-B7  solenoid data
   CB2    toggle parallel / serial mode

PIA3:
I: CA1    display interrupt (360 Hz)
O: A0-A3  display address
   A4-A7  BCD data
*/
static struct pia6821_interface pia[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  pia1a_r,pia1b_r,0,0,0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,0,
/* IRQ: A/B              */  0,0
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,pia2b_r,0,pia2cb1_r,0,pia2cb2_r,
/* O:  A/B,CA2/B2        */  pia2a_w,pia2b_w,0,pia2cb2_w,
/* IRQ: A/B              */  0,0
},{
/* I:  A/B,CA1/B1,CA2/B2 */  pia3a_r,0,pia3ca1_r,0,pia3ca2_r,0,
/* O:  A/B,CA2/B2        */  pia3a_w,pia3b_w,pia3ca2_w,0,
/* IRQ: A/B              */  0,0
}};

static INTERRUPT_GEN(boomerang_vblank) {
  core_updateSw(core_getSol(2));
}

static INTERRUPT_GEN(boomerang_irq) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, PULSE_LINE);
}

static void boomerang_zeroCross(int data) {
  locals.zc = !locals.zc;
//  pia_set_input_cb2(PIA2, locals.zc);
}

static void boomerang_dspIrq(int data) {
  locals.dspIrq = !locals.dspIrq;
//  pia_set_input_ca1(PIA3, locals.dspIrq);
}

static SWITCH_UPDATE(boomerang) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],      0x01,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT],      0x80,3);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xe0,4);
  }
  pia_set_input_cb1(PIA2, (locals.cb12 = coreGlobals.swMatrix[0] & 1));
}

static MACHINE_INIT(boomerang) {
  memset(&locals, 0, sizeof(locals));
  pia_config(PIA1, PIA_STANDARD_ORDERING, &pia[0]);
  pia_config(PIA2, PIA_STANDARD_ORDERING, &pia[1]);
  pia_config(PIA3, PIA_STANDARD_ORDERING, &pia[2]);
}

static MACHINE_RESET(boomerang) {
  memset(&locals, 0, sizeof(locals));
  pia_reset();
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
static MEMORY_READ_START(boomerang_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0080,	MRA_RAM }, /* fake NVRAM */
  { 0x0084, 0x0087, pia_r(PIA1) },
  { 0x0088, 0x008b, pia_r(PIA2) },
  { 0x0090, 0x0093, pia_r(PIA3) },
  { 0x0800, 0x0fff, MRA_ROM },
  { 0xfff0, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(boomerang_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0080,	MWA_RAM, &generic_nvram, &generic_nvram_size }, /* fake NVRAM */
  { 0x0084, 0x0087, pia_w(PIA1) },
  { 0x0088, 0x008b, pia_w(PIA2) },
  { 0x0090, 0x0093, pia_w(PIA3) },
  { 0x0800, 0xffff, MWA_NOP },
MEMORY_END

MACHINE_DRIVER_START(boomerang)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(boomerang,boomerang,NULL)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 500000) // taken from patent
  MDRV_CPU_MEMORY(boomerang_readmem, boomerang_writemem)
  MDRV_CPU_VBLANK_INT(boomerang_vblank, 1)
  MDRV_CPU_PERIODIC_INT(boomerang_irq, 500) // guessed
  MDRV_TIMER_ADD(boomerang_zeroCross, 120)
  MDRV_TIMER_ADD(boomerang_dspIrq, 720) // 360Hz x 2, taken from patent
  MDRV_SWITCH_UPDATE(boomerang)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(24)
MACHINE_DRIVER_END

#define BOOMERANG_COMPORTS \
  PORT_START /* 0 */ \
    COREPORT_BIT(0x2000, "Start",     KEYCODE_1) \
    COREPORT_BIT(0x8000, "Coin #1",   KEYCODE_3) \
    COREPORT_BIT(0x4000, "Coin #2",   KEYCODE_5) \
    COREPORT_BIT(0x0080, "Tilt",      KEYCODE_DEL) \
    COREPORT_BIT(0x0001, "Slam Tilt", KEYCODE_HOME) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0xc000, 0x0000, "Balls / game") \
      COREPORT_DIPSET(0x0000, "3") \
      COREPORT_DIPSET(0x4000, "4") \
      COREPORT_DIPSET(0x8000, "5") \
      COREPORT_DIPSET(0xc000, "6") \
    COREPORT_DIPNAME( 0x00f0, 0x0060, "Score to beat x10,000") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0010, "1") \
      COREPORT_DIPSET(0x0020, "2") \
      COREPORT_DIPSET(0x0030, "3") \
      COREPORT_DIPSET(0x0040, "4") \
      COREPORT_DIPSET(0x0050, "5") \
      COREPORT_DIPSET(0x0060, "6") \
      COREPORT_DIPSET(0x0070, "7") \
      COREPORT_DIPSET(0x0080, "8") \
      COREPORT_DIPSET(0x0090, "9") \
    COREPORT_DIPNAME( 0x000f, 0x0000, "Score to beat  x1,000") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0001, "1") \
      COREPORT_DIPSET(0x0002, "2") \
      COREPORT_DIPSET(0x0003, "3") \
      COREPORT_DIPSET(0x0004, "4") \
      COREPORT_DIPSET(0x0005, "5") \
      COREPORT_DIPSET(0x0006, "6") \
      COREPORT_DIPSET(0x0007, "7") \
      COREPORT_DIPSET(0x0008, "8") \
      COREPORT_DIPSET(0x0009, "9") \
    COREPORT_DIPNAME( 0x3800, 0x0800, "Coins for slot #1") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0800, "1") \
      COREPORT_DIPSET(0x1000, "2") \
      COREPORT_DIPSET(0x1800, "3") \
      COREPORT_DIPSET(0x2000, "4") \
      COREPORT_DIPSET(0x2800, "5") \
      COREPORT_DIPSET(0x3000, "6") \
      COREPORT_DIPSET(0x3800, "7") \
    COREPORT_DIPNAME( 0x0700, 0x0100, "Credits for slot #1") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0100, "1") \
      COREPORT_DIPSET(0x0200, "2") \
      COREPORT_DIPSET(0x0300, "3") \
      COREPORT_DIPSET(0x0400, "4") \
      COREPORT_DIPSET(0x0500, "5") \
      COREPORT_DIPSET(0x0600, "6") \
      COREPORT_DIPSET(0x0700, "7") \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0038, 0x0008, "Coins for slot #2") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0008, "1") \
      COREPORT_DIPSET(0x0010, "2") \
      COREPORT_DIPSET(0x0018, "3") \
      COREPORT_DIPSET(0x0020, "4") \
      COREPORT_DIPSET(0x0028, "5") \
      COREPORT_DIPSET(0x0030, "6") \
      COREPORT_DIPSET(0x0038, "7") \
    COREPORT_DIPNAME( 0x0007, 0x0003, "Credits for slot #2") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0001, "1") \
      COREPORT_DIPSET(0x0002, "2") \
      COREPORT_DIPSET(0x0003, "3") \
      COREPORT_DIPSET(0x0004, "4") \
      COREPORT_DIPSET(0x0005, "5") \
      COREPORT_DIPSET(0x0006, "6") \
      COREPORT_DIPSET(0x0007, "7") \
    COREPORT_DIPNAME( 0x0040, 0x0000, DEF_STR(Unused)) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0040, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0080, 0x0000, DEF_STR(Unused)) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0080, DEF_STR(On))

static const core_tLCDLayout dispBoom[] = {
  {0, 0, 6,6,CORE_SEG7}, {0,16,12,6,CORE_SEG7},
  {3, 0,18,6,CORE_SEG7}, {3,16,24,6,CORE_SEG7},
  {6, 8,30,2,CORE_SEG7}, {6,16,34,2,CORE_SEG7},
  {0}
};
static core_tGameData boomrangGameData = {GEN_BYPROTO,dispBoom,{FLIP_SW(FLIP_L)}};
static void init_boomrang(void) {
  core_gameData = &boomrangGameData;
}
ROM_START(boomrang)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("boom.bin", 0x0800, 0x0800, CRC(d3f5116d) SHA1(2a8aefea137c563dd64dd18684b1fa68fc40e9d6))
    ROM_RELOAD(0xf800, 0x0800)
ROM_END
INPUT_PORTS_START(boomrang)
  CORE_PORTS
  SIM_PORTS(1)
  BOOMERANG_COMPORTS
INPUT_PORTS_END
CORE_GAMEDEFNV(boomrang,"Boomerang (Engineering Prototype, patched patent code)",1975,"Bally",boomerang,GAME_USES_CHIMES)

ROM_START(boomranb)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("boomb.bin", 0x0800, 0x0800, CRC(9e99ba79) SHA1(9291513bd3dcbbbdef29f3a88c52d6c2ebf1c0b2))
    ROM_RELOAD(0xf800, 0x0800)
ROM_END
#define init_boomranb init_boomrang
#define input_ports_boomranb input_ports_boomrang
CORE_CLONEDEFNV(boomranb,boomrang,"Boomerang (MOD Bugfixes patch)",2017,"Bally / Gaston",boomerang,GAME_USES_CHIMES)
