/*
   Bally prototype machines (by gaston, 03/2008)
   ------------------------
   CPU: 68701 (6803 compatible with internal RAM and ROM)
   I/O: 2 x 6821 PIA
        CPU ports
        DMA
   Sound: Bally's regular external sound boards, like S&T (-61)

   In 1981 Bally designed a successor for the BY-35 series of games
   with 9-segment display digits and direct segment access to allow
   for alphanumeric data. They also expanded the switch matrix to
   64 switches, and introduced a diagnostic keypad.

   This setup created the following new features:
   - No more dip switches
   - Support for 120 lamps on-board (sparing an auxiliary lamps board)
   - A total of 26 solenoids as opposed to BY-35's 18
   - Easy access to any game setting by using 2-digit codes
   - Displaying  all the game settings in human-readble form
   - Top five players list with entered initials

   Yet all of this was discarded again and kept in storage until 1985,
   when the 6803 series emerged that used the keypad again. In 1986,
   Bally finally chose to use the 9-segment displays as well.

   Just imagine the impact this very technology would have had
   on the pinball playing world back in 1982!
*/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"

#define BY68701_PIA0 0
#define BY68701_PIA1 1

#define BY68701_SOLSMOOTH       4 /* Smooth the solenoids over this number of VBLANKS */
#define BY68701_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define BY68701_DISPLAYSMOOTH   2 /* Smooth the displays over this number of VBLANKS */

static struct {
  int swCol, lampCol, strobe, commas, solEnable;
  UINT32 solenoids;
  core_tSeg segments;
  int diagnosticLed;
  int vblankCount, zc, irq;
  int inhibit, zcSanity, dispSanity;
  int irqstate, irqstates[4];
} locals;

// determines the amount of bits set in a given value
static int countBits(int value) {
  int amount = 0;
  for (;value;value >>= 1)
    if (value & 1) amount++;
  return amount;
}

static void piaIrq(int num, int state) {
  static int oldstate;
  locals.irqstates[num] = state;
  locals.irqstate = locals.irqstates[0] || locals.irqstates[1] || locals.irqstates[2] || locals.irqstates[3];
  if (oldstate != locals.irqstate) {
    logerror("IRQ state: %d\n", locals.irqstate);
    cpu_set_irq_line(0, M6800_IRQ_LINE, locals.irqstate ? ASSERT_LINE : CLEAR_LINE);
  }
  oldstate = locals.irqstate;
}

static INTERRUPT_GEN(by68701_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % BY68701_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % BY68701_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % BY68701_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0, sizeof(locals.segments));
  }

  coreGlobals.diagnosticLed = locals.diagnosticLed;
  core_updateSw(core_getSol(core_gameData->hw.gameSpecific1 ? 16 : 12));
}

static SWITCH_UPDATE(by68701) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]  >>4,0x03,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]  <<4,0xf0,4);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1]<<4,0xf0,5);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1],   0xf0,6);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1]>>4,0xf0,7);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1]>>8,0xf0,8);
  }
  /*-- Diagnostic button on sound board --*/
  sndbrd_0_diag(core_getSw(-6));
}

static WRITE_HANDLER(port1_w) { // 8BD solenoids 14 - 21
  if (!core_gameData->hw.gameSpecific1) locals.solenoids = (locals.solenoids & 0xffe01fff) | (data << 13);
}
static WRITE_HANDLER(port2_w) {
  if (data & 0xf6) logerror("%04x: p2 write:%02x\n", activecpu_get_previouspc(), data);
  locals.diagnosticLed = (data & 8) >> 3;
}
static WRITE_HANDLER(port4_w) {
  logerror("%04x: p4 write:%02x\n", activecpu_get_previouspc(), data);
}

static WRITE_HANDLER(pp0_a_w) {
  if (data < 0x04) {
    if (core_gameData->hw.gameSpecific1) { // FG solenoids 15 & 16
      locals.solenoids = (locals.solenoids & 0xffff3fff) | ((data & 0x03) << 14);
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff3fff) | ((data & 0x03) << 14);
    } else { // 8BD solenoids 12 & 13
      locals.solenoids = (locals.solenoids & 0xffffe7ff) | ((data & 0x01) << 12) | ((data & 0x02) << 10);
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffe7ff) | ((data & 0x01) << 12) | ((data & 0x02) << 10);
    }
  } else logerror("%04x: PIA 0 A WRITE = %02x\n", activecpu_get_previouspc(), data);
}
static WRITE_HANDLER(pp0_ca2_w) { // goes lo then hi with every zc transition
  locals.zcSanity = data;
}
static WRITE_HANDLER(pp0_cb2_w) { // alternates with every irq pulse
  locals.strobe = 0;
  locals.dispSanity = data;
}

static WRITE_HANDLER(pp1_a_w) {
  logerror("%04x: PIA 1 A WRITE = %02x\n", activecpu_get_previouspc(), data);
}
static WRITE_HANDLER(pp1_b_w) { // periphal data
  static int solOrder[2][4][4] = {{
    { 2, 3, 4, 5},
    { 8, 9, 1, 0},
    {10,20, 6, 7},
    {21,22,23,24},
  },{
    { 0, 2, 5, 1},
    { 9, 3, 7,16},
    {11,13,10,17},
    { 8,12, 4, 6}
  }};
  static int lampRows[15] = { 4, 8, 9, 10, 11, 12, 13, 14, 5, 0, 6, 7, 1, 2, 3 };
  if (locals.lampCol != 0x0f) {
    UINT8 lampdata = locals.zc ? (data & 0xf0) ^ 0xf0 : (data >> 4) ^ 0x0f;
    coreGlobals.tmpLampMatrix[lampRows[locals.lampCol]] |= lampdata;
  } else if (locals.strobe == 2) { // solenoids
    if (locals.solEnable && countBits(0x0f ^ (data >> 4)) == 1 && countBits(0x0f ^ (data & 0x0f)) == 1) {
      UINT8 solData = ~data;
      locals.solenoids |= 1 << solOrder[core_gameData->hw.gameSpecific1][core_BitColToNum(solData & 0x0f)][core_BitColToNum(solData >> 4)];
      locals.solEnable = 0;
    }
  } else if (locals.strobe == 3) { // sound #2
    if (!(data >> 4)) {
      sndbrd_0_ctrl_w(0, 0); sndbrd_0_ctrl_w(0, 1); sndbrd_0_data_w(0, data);
    }
  } else logerror("%04x: PIA 1 B WRITE = %02x\n", activecpu_get_previouspc(), data);
}
static WRITE_HANDLER(pp1_ca2_w) { // enable comma segments
  locals.commas = data;
}
static WRITE_HANDLER(pp1_cb2_w) { // goes lo then hi after every PIA1 port B write
  locals.inhibit = data;
}

static READ_HANDLER(pp0_b_r) {
  return coreGlobals.swMatrix[1+locals.swCol]; // switch returns
}
static READ_HANDLER(pp0_ca1_r) { // reads (inverted?) zc state for 2nd lamp strobe
  return !locals.zc;
}
static READ_HANDLER(pp0_cb1_r) { // irq read
  return locals.irq;
}

static READ_HANDLER(pp1_a_r) { // will kill lamps on eballdp1 when set
  return core_getSw(-7);
}
static READ_HANDLER(pp1_ca1_r) { // reads zc state for 1st lamp strobe
  return locals.zc;
}
static READ_HANDLER(pp1_cb1_r) { // bus read irq
  return 0;
}

static void pp0_irq_a(int state) { piaIrq(0, state); }
static void pp0_irq_b(int state) { piaIrq(1, state); }
static void pp1_irq_a(int state) { piaIrq(2, state); }
static void pp1_irq_b(int state) { piaIrq(3, state); }

static INTERRUPT_GEN(by68701_irq) {
  pia_set_input_cb1(BY68701_PIA0, locals.irq = !locals.irq);
}
static void by68701_zeroCross(int data) {
  locals.zc = !locals.zc;
  pia_set_input_ca1(BY68701_PIA1, locals.zc);
  pia_set_input_ca1(BY68701_PIA0, !locals.zc);
}

static struct pia6821_interface by68701_pia[] = {{ // PIA0: U20
/* I:  A/B,CA1/B1,CA2/B2 */  0,pp0_b_r, pp0_ca1_r,pp0_cb1_r, 0,0,
/* O:  A/B,CA2/B2        */  pp0_a_w,0, pp0_ca2_w,pp0_cb2_w,
/* IRQ: A/B              */  pp0_irq_a,pp0_irq_b
},{ // PIA1: U35
/* I:  A/B,CA1/B1,CA2/B2 */  pp1_a_r,0, pp1_ca1_r,pp1_cb1_r, 0,0,
/* O:  A/B,CA2/B2        */  pp1_a_w,pp1_b_w, pp1_ca2_w,pp1_cb2_w,
/* IRQ: A/B              */  pp1_irq_a,pp1_irq_b
}};

static MACHINE_INIT(by68701) {
  int sb = core_gameData->hw.soundBoard;
  memset(&locals, 0, sizeof(locals));

  pia_config(BY68701_PIA0, PIA_STANDARD_ORDERING, &by68701_pia[0]);
  pia_config(BY68701_PIA1, PIA_STANDARD_ORDERING, &by68701_pia[1]);

  sndbrd_0_init(sb, 1, memory_region(REGION_SOUND1), NULL, NULL);
}
static MACHINE_RESET(by68701) {
  pia_reset();
  memset(&locals, 0, sizeof(locals));
}
static MACHINE_STOP(by68701) {
  sndbrd_0_exit();
}

// displays, solenoids, lamp and switch strobe
static WRITE_HANDLER(by68701_m0800_w) {
  static int digit;
  int num;
  locals.strobe = offset;
  switch (offset) {
    case 0: // lamp column
      locals.lampCol = data & 0x0f;
      break;
    case 1: // switch column & display digit
      if (data & 0x08) digit = (1 + (data & 0x07)) % 8;
      else locals.swCol = data & 0x07;
      break;
    case 2: // solenoids, handled completely in PIA write for lack of a better method
      locals.solEnable = 1;
      break;
    case 3: // sound #1
      sndbrd_0_ctrl_w(0, 0); sndbrd_0_ctrl_w(0, 1); sndbrd_0_data_w(0, data);
      break;
    case 4: case 5: case 6: case 7: // player display data
      num = (offset-4)*8 + 7 - digit;
      locals.segments[num].w = (data & 0x7f) | ((data & 0x80) << 1) | ((data & 0x80) << 2);
      if (locals.commas && locals.segments[num].w && (num % 8 == 1 || num % 8 == 4)) locals.segments[num].w |= 0x80;
      break;
    case 10: // BIP / match display data
      locals.segments[32 + 7 - digit].w = (data & 0x7f) | ((data & 0x80) << 1) | ((data & 0x80) << 2);
      break;
    default:
      logerror("%04x: m08%02x write: %02x\n", activecpu_get_previouspc(), offset, data);
  }
}

static READ_HANDLER(by68701_m3000_r) {
  logerror("%04x: m3%03x read\n", activecpu_get_previouspc(), offset);
  return 0;
}

static PORT_READ_START( by68701_readport )
  { M6803_PORT1, M6803_PORT2, MRA_NOP },
PORT_END

static PORT_WRITE_START( by68701_writeport )
  { M6803_PORT1, M6803_PORT1, port1_w },
  { M6803_PORT2, M6803_PORT2, port2_w },
  { M6803_PORT4, M6803_PORT4, port4_w },
PORT_END

static MEMORY_READ_START(by68701_readmem)
/*
  { 0x0004, 0x0004, MRA_NOP },
  { 0x0006, 0x0006, MRA_NOP },
  { 0x000f, 0x000f, MRA_NOP },
*/
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0020, 0x0023, pia_r(BY68701_PIA0) }, // Eight Ball Deluxe
  { 0x0040, 0x0043, pia_r(BY68701_PIA1) }, // Eight Ball Deluxe
  { 0x0044, 0x0047, pia_r(BY68701_PIA0) }, // Flash Gordon
  { 0x0048, 0x004b, pia_r(BY68701_PIA1) }, // Flash Gordon
  { 0x0080, 0x00ff, MRA_RAM },  /*Internal 128B RAM*/
  { 0x0400, 0x04ff, MRA_RAM },  /*NVRAM*/
  { 0x0500, 0x07ff, MRA_RAM },  /*External RAM*/
  { 0x3000, 0x301f, by68701_m3000_r },
  { 0x7000, 0xffff, MRA_ROM },  /*ROM */
MEMORY_END

static MEMORY_WRITE_START(by68701_writemem)
/*
  { 0x0004, 0x0004, MWA_NOP },
  { 0x0006, 0x0006, MWA_NOP },
  { 0x000f, 0x000f, MWA_NOP },
*/
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0020, 0x0023, pia_w(BY68701_PIA0) }, // Eight Ball Deluxe
  { 0x0040, 0x0043, pia_w(BY68701_PIA1) }, // Eight Ball Deluxe
  { 0x0044, 0x0047, pia_w(BY68701_PIA0) }, // Flash Gordon
  { 0x0048, 0x004b, pia_w(BY68701_PIA1) }, // Flash Gordon
  { 0x0080, 0x00ff, MWA_RAM },  /*Internal 128B RAM*/
  { 0x0400, 0x04ff, MWA_RAM, &generic_nvram, &generic_nvram_size }, /*NVRAM*/
  { 0x0500, 0x07ff, MWA_RAM },  /*External RAM*/
  { 0x0800, 0x080f, by68701_m0800_w },
MEMORY_END

MACHINE_DRIVER_START(by68701_61S)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(by68701,by68701,by68701)
  MDRV_CPU_ADD_TAG("mcpu", M6803, 3579545/4)
  MDRV_CPU_MEMORY(by68701_readmem, by68701_writemem)
  MDRV_CPU_PORTS(by68701_readport, by68701_writeport)
  MDRV_CPU_VBLANK_INT(by68701_vblank, 1)
  MDRV_CPU_PERIODIC_INT(by68701_irq, 4000)
  MDRV_TIMER_ADD(by68701_zeroCross, 120)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(by68701)
  MDRV_DIPS(1) // needed for extra inports
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_IMPORT_FROM(by61)
MACHINE_DRIVER_END

#define BY68701_INPUT_PORTS_START(name) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(1) \
  PORT_START /* 2 */ \
    /* Switch Column 0 */ \
    COREPORT_BIT(   0x0010, "CPU diagnostics", KEYCODE_9) \
    COREPORT_BIT(   0x0020, "Sound diagnostics", KEYCODE_0) \
    /* Switch Column 4 */ \
    COREPORT_BIT(   0x0008, "KP: Game",     KEYCODE_6) \
    COREPORT_BIT(   0x0001, "KP: Enter",    KEYCODE_7) \
    COREPORT_BIT(   0x0004, "KP: Clear",    KEYCODE_8) \
    COREPORT_BIT(   0x0002, "KP: 0",        KEYCODE_0_PAD) \
  PORT_START /* 3 */ \
    /* Switch Columns 5 to 8 */ \
    COREPORT_BIT(   0x0004, "KP: 1",        KEYCODE_1_PAD) \
    COREPORT_BIT(   0x0002, "KP: 2",        KEYCODE_2_PAD) \
    COREPORT_BIT(   0x0001, "KP: 3",        KEYCODE_3_PAD) \
    COREPORT_BIT(   0x0040, "KP: 4",        KEYCODE_4_PAD) \
    COREPORT_BIT(   0x0020, "KP: 5",        KEYCODE_5_PAD) \
    COREPORT_BIT(   0x0010, "KP: 6",        KEYCODE_6_PAD) \
    COREPORT_BIT(   0x0400, "KP: 7",        KEYCODE_7_PAD) \
    COREPORT_BIT(   0x0200, "KP: 8",        KEYCODE_8_PAD) \
    COREPORT_BIT(   0x0100, "KP: 9",        KEYCODE_9_PAD) \
    COREPORT_BIT(   0x0008, "KP: A",        KEYCODE_V) \
    COREPORT_BIT(   0x0080, "KP: B",        KEYCODE_B) \
    COREPORT_BIT(   0x0800, "KP: C",        KEYCODE_C) \
    COREPORT_BIT(   0x8000, "KP: D",        KEYCODE_X) \
    COREPORT_BIT(   0x4000, "KP: E / Coin 1", KEYCODE_3) \
    COREPORT_BIT(   0x2000, "KP: F / Coin 2", KEYCODE_4) \
    COREPORT_BIT(   0x1000, "KP: G / Coin 3", KEYCODE_5) \
    COREPORT_BITDEF(0x0200, IPT_START1,     IP_KEY_DEFAULT)  \
    COREPORT_BIT(   0x0004, "Advance Initial", KEYCODE_2) \
    COREPORT_BIT(   0x0400, "Tilt",         KEYCODE_INSERT) \
    COREPORT_BIT(   0x0100, "Slam Tilt",    KEYCODE_HOME) \
  INPUT_PORTS_END

static const core_tLCDLayout dispBy7p[] = {
  {0, 0, 1,7,CORE_SEG98},{0,18, 9,7,CORE_SEG98},
  {6, 0,17,7,CORE_SEG98},{6,18,25,7,CORE_SEG98},
  {3,20,35,2,CORE_SEG9}, {3,26,38,2,CORE_SEG9}, {0}
};

#define INITGAMEP(name, flip, sb, gs1) \
static core_tGameData name##GameData = {GEN_BYPROTO,dispBy7p,{flip,0,7,0,sb,0,gs1}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \
BY68701_INPUT_PORTS_START(name)

#define BY68701_ROMSTART_CA8(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
  ROM_START(name) \
    NORMALREGION(0x10000, REGION_CPU1) \
      ROM_LOAD( n1, 0xf800, 0x0800, chk1) \
      ROM_LOAD( n2, 0xc000, 0x1000, chk2) \
      ROM_LOAD( n3, 0xa000, 0x1000, chk3) \
      ROM_LOAD( n4, 0x8000, 0x1000, chk4)

#define BY68701_ROMSTART_DC7A(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
  ROM_START(name) \
    NORMALREGION(0x10000, REGION_CPU1) \
      ROM_LOAD( n1, 0xf800, 0x0800, chk1) \
      ROM_LOAD( n2, 0xd000, 0x1000, chk2) \
      ROM_LOAD( n3, 0xc000, 0x1000, chk3) \
      ROM_LOAD( n4, 0x7000, 0x1000, chk4) \
      ROM_LOAD( n5, 0xa000, 0x1000, chk5)

#define BY68701_ROMSTART_DCA(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
  ROM_START(name) \
    NORMALREGION(0x10000, REGION_CPU1) \
      ROM_LOAD( n1, 0xf800, 0x0800, chk1) \
      ROM_LOAD( n2, 0xd000, 0x1000, chk2) \
      ROM_LOAD( n3, 0xc000, 0x1000, chk3) \
      ROM_LOAD( n4, 0xa000, 0x1000, chk4)


// games below

/*------------------
/ Flash Gordon
/------------------*/
INITGAMEP(flashgdp,FLIP_SW(FLIP_L),SNDBRD_BY61,1)
BY68701_ROMSTART_CA8(flashgdp,"fg68701.bin",CRC(e52da294) SHA1(0191ae821fbeae40192d858ca7f2dccda84de73f),
                            "xxx-xx.u10",NO_DUMP,
                            "xxx-xx.u11",CRC(8b0ae6d8) SHA1(2380bd6d354c204153fd44534d617f7be000e46f),
                            "xxx-xx.u12",CRC(57406a1f) SHA1(01986e8d879071374d6f94ae6fce5832eb89f160))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e) SHA1(ecdeb07c31c22ec313b55774f4358a9923c5e9e7),
                          "834-18_5.532",CRC(8799e80e) SHA1(f255b4e7964967c82cfc2de20ebe4b8d501e3cb0))
ROM_END
CORE_CLONEDEFNV(flashgdp,flashgdn,"Flash Gordon (prototype rev. 1)",1981,"Bally",by68701_61S,0)

#define init_flashgp2 init_flashgdp
#define input_ports_flashgp2 input_ports_flashgdp
BY68701_ROMSTART_CA8(flashgp2,"fg6801.bin",CRC(8af7bf77) SHA1(fd65578b2340eb207b2e197765e6721473340565) BAD_DUMP,
                           "xxx-xx2.u10",CRC(0fff825c) SHA1(3c567aa8ec04a8ff9a09b530b6d324fdbe363ab6),
                           "xxx-xx2.u11",CRC(e34b113a) SHA1(b2860d284995db0ec59b22b434ccb9f6721e7d9d),
                           "xxx-xx2.u12",CRC(12bf4e19) SHA1(b0acf069d86f7472728610834ef3ab6da83b4b67))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e) SHA1(ecdeb07c31c22ec313b55774f4358a9923c5e9e7),
                          "834-18_5.532",CRC(8799e80e) SHA1(f255b4e7964967c82cfc2de20ebe4b8d501e3cb0))
ROM_END
CORE_CLONEDEFNV(flashgp2,flashgdn,"Flash Gordon (prototype rev. 2)",1981,"Bally",by68701_61S,0)

/*------------------
/ Eight Ball Deluxe
/------------------*/
INITGAMEP(eballdp1,FLIP_SW(FLIP_L),SNDBRD_BY61,0)
BY68701_ROMSTART_DC7A(eballdp1,"ebd68701.1",CRC(2c693091) SHA1(93ae424d6a43424e8ea023ef555f6a4fcd06b32f),
// the ebd68701.2 boot rom obviously does not survive a slam tilt
//BY68701_ROMSTART_DC7A(eballdp1,"ebd68701.2",CRC(cb90f453) SHA1(e3165b2be8f297ce0e18c5b6261b79b56d514fc0),
                           "720-61.u10",CRC(ac646e58) SHA1(85694264a739118ed249d97c04fe8e9f6edfdd33),
                           "720-62.u14",CRC(b6476a9b) SHA1(1dc92125422908e829ce17aaed5ad49b0dbda0e5),
                           "720-63.u13",CRC(f5d751fd) SHA1(4ab5975d52cdde0e05f2bbea7dcd732882fb1dd5),
                           "838-19.u12",CRC(dfba8976) SHA1(70c19237842ee73cfc8e9607df79a59424d90d99))
BY61_SOUNDROMx080(       "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                         "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                         "838-16_5.532",CRC(63d92025) SHA1(2f8e8435326a39064b99b9971b0d8944586571fb))
ROM_END
CORE_CLONEDEFNV(eballdp1,eballdlx,"Eight Ball Deluxe (prototype rev. 1)",1981,"Bally",by68701_61S,0)

#define init_eballdp2 init_eballdp1
#define input_ports_eballdp2 input_ports_eballdp1
BY68701_ROMSTART_DC7A(eballdp2,"ebd68701.1",CRC(2c693091) SHA1(93ae424d6a43424e8ea023ef555f6a4fcd06b32f),
                           "720-56.u10",CRC(65a3a02b) SHA1(6fa8667509d314f521dce63d9f1b7fc132d85a1f),
                           "720-57.u14",CRC(a7d96074) SHA1(04726af863a2c7589308725f3183112b5e1f84ac),
                           "720-58.u13",CRC(c9585f1f) SHA1(a38b059bb7ef15fccb54bec58d88dd15182b66a6),
                           "838-18.u12",CRC(20fa35e5) SHA1(d8808aa357d2a20fc235da7c80f78c8e5d805ac3))
BY61_SOUNDROMx080(       "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                         "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                         "838-16_5.532",CRC(63d92025) SHA1(2f8e8435326a39064b99b9971b0d8944586571fb))
ROM_END
CORE_CLONEDEFNV(eballdp2,eballdlx,"Eight Ball Deluxe (prototype rev. 2)",1981,"Bally",by68701_61S,0)

#define init_eballdp3 init_eballdp1
#define input_ports_eballdp3 input_ports_eballdp1
BY68701_ROMSTART_DC7A(eballdp3,"ebd68701.1",CRC(2c693091) SHA1(93ae424d6a43424e8ea023ef555f6a4fcd06b32f),
                           "720-xx.u10",CRC(6da34581) SHA1(6e005ceda9a4a23603d5243dfca85ccd3f0e425a),
                           "720-xx.u14",CRC(7079648a) SHA1(9d91cd18fb68f165498de8ac51c1bc2a35bd9468),
                           "xxx-xx.u13",CRC(bda2c78b) SHA1(d5e7d0dd3d44d63b9d4b43bf5f63917b80a7ce23),
                           "838-18.u12",CRC(20fa35e5) SHA1(d8808aa357d2a20fc235da7c80f78c8e5d805ac3))
BY61_SOUNDROMx080(       "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                         "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                         "838-16_5.532",CRC(63d92025) SHA1(2f8e8435326a39064b99b9971b0d8944586571fb))
ROM_END
CORE_CLONEDEFNV(eballdp3,eballdlx,"Eight Ball Deluxe (prototype rev. 3)",1981,"Bally",by68701_61S,0)

#define init_eballdp4 init_eballdp1
#define input_ports_eballdp4 input_ports_eballdp1
BY68701_ROMSTART_DCA(eballdp4,"ebd68701.1",CRC(2c693091) SHA1(93ae424d6a43424e8ea023ef555f6a4fcd06b32f),
                           "720-54.u10",CRC(9facc547) SHA1(a10d7747918b3a1d87bd9caa19e87739631a7566),
                           "720-55.u14",CRC(99080832) SHA1(e1d416b4910ed31b40bde0860e698f0cbe46cc57),
                           "838-17.u12",CRC(43ebffc6) SHA1(bffd41c68430889b3926db9b05c5991185c28053))
BY61_SOUNDROMx080(       "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                         "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                         "838-16_5.532",CRC(63d92025) SHA1(2f8e8435326a39064b99b9971b0d8944586571fb))
ROM_END
CORE_CLONEDEFNV(eballdp4,eballdlx,"Eight Ball Deluxe (prototype rev. 4)",1981,"Bally",by68701_61S,0)
