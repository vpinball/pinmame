// license:BSD-3-Clause

/***************************************************************************

  E.F.O. ZCU pinball game driver.
  Many companies used game and / or sound drivers produced by E.F.O. SA,
  even Playmatic used their sound board on the last two games.
  
  The main CPU is copy protected by a registered PAL chip.
  To this day, that PAL is undecoded.
  Luckily all the games seem to check only for one specific value ($9b),
  so we just return that value whenever U18 is read from.

***************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "machine/z80fmly.h"

static struct {
  int col, row, pos, zc, outSet;
  UINT8 latch;
  void *printfile;

  UINT8 phaseA;
} locals;

static INTERRUPT_GEN(EFO_vblank) {
  core_updateSw(core_getSol(1));
}

static INTERRUPT_GEN(EFO_zc) {
  if (core_gameData->hw.gameSpecific1) {
    z80ctc_2_trg2_w(0, locals.phaseA); // would prevent comeback from running
  }
  if (core_gameData->hw.gameSpecific2) {
    z80ctc_2_trg3_w(0, locals.phaseA); // would prevent cobrapb from running
  }
  if (locals.phaseA) locals.zc = !locals.zc;
  locals.phaseA = !locals.phaseA;
}

extern void ctc_interrupt_0(int state);
extern void ctc_interrupt_1(int state);
static void ctc_interrupt_2(int state) {
  cpu_set_irq_line_and_vector(0, 0, state ? ASSERT_LINE : CLEAR_LINE, Z80_VECTOR(0, state));
}
static WRITE_HANDLER(zc0_2_w) {
  z80ctc_2_trg1_w(0, data);
}
extern WRITE_HANDLER(zc2_0_w);

// keep the CTCs for the sound board in place and add the one for the main CPU as #2
static z80ctc_interface ctc_intf = {
  3,
  { 4000000, 4000000, 0 },
  { 0, 0, 0 },
  { ctc_interrupt_0, ctc_interrupt_1, ctc_interrupt_2 },
  { 0, 0, zc0_2_w },
  { 0, 0, 0 },
  { zc2_0_w, 0, 0 }
};

static Z80_DaisyChain EFO_DaisyChain[] = {
  { z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 2 },
  { 0, 0, 0, -1 },
};
/*
static void ck0_pulse(int param) {
  z80ctc_2_trg0_w(0, 1);
  z80ctc_2_trg0_w(0, 0);
}
*/
static MACHINE_INIT(EFO) {
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_CPU2), NULL, NULL);
  memset(&locals, 0x00, sizeof(locals));
  ctc_intf.baseclock[2] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);
//timer_pulse(TIME_IN_HZ(2000000), 0, ck0_pulse); // probably used to strobe in serial display data
}

static MACHINE_RESET(EFO) {
  z80ctc_2_reset();
}

static MACHINE_STOP(EFO) {
  if (locals.printfile) {
    mame_fclose(locals.printfile);
    locals.printfile = NULL;
  }
  sndbrd_0_exit();
}

static SWITCH_UPDATE(EFO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xff, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x7f, 8);
  }
}

static READ_HANDLER(sw0_r) {
  return (~coreGlobals.swMatrix[0] & 0x7f) | (locals.zc ? 0x80 : 0);
}

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[locals.col + 1];
}

static READ_HANDLER(u16_r) {
  return 0x9b; // we got lucky, this seems to work alright! :)
}

static WRITE_HANDLER(u16_w) {
  logerror("U16 WRITE @%04x: %02x\n", activecpu_get_previouspc(), data);
}

static WRITE_HANDLER(col_w) {
  locals.col = core_BitColToNum(data);
}

static WRITE_HANDLER(row_w) {
  locals.row = !data ? 0 : 1 + core_BitColToNum(data);
  if (locals.row > 7) locals.row = 0;
  locals.pos = 0;
}

static WRITE_HANDLER(disp_w) {
  int segNo = locals.pos * 8 + 7 - locals.row;
  if (segNo == 32 || segNo == 44) data &= 0x7f;
  coreGlobals.segments[segNo].w = data;
  locals.pos++;
}

static WRITE_HANDLER(out_w) {
  if (data & 0x0f) locals.outSet = data & 0x0f;
  if (data & 0x04) {
    sndbrd_0_data_w(0, locals.latch);
  } else if ((data & 0xef) == (data & 0x20)) {
    if (!locals.printfile) {
      char filename[64];
      sprintf(filename,"%s.prt", Machine->gamedrv->name);
      locals.printfile = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_PRINTER, 2); // APPEND write mode
    }
    UINT8 printdata = locals.latch;
    if (locals.printfile) mame_fwrite(locals.printfile, &printdata, 1);
#if MAME_DEBUG
    printf("%c", locals.latch);
#endif
  }
  logerror("  P18: %02x\n", data);
}

static WRITE_HANDLER(latch_w) {
  locals.latch = data;
  logerror("LATCH: %02x\n", data);
}

static WRITE_HANDLER(lamp_w) {
  if (locals.outSet == 1) {
    coreGlobals.lampMatrix[2 * offset + locals.zc] = data;
  }
}

static WRITE_HANDLER(sol_w) {
  if (offset) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0x00ff) | (data << 8);
  } else {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xff00) | data;
  }
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x7fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0xa000, 0xa000, u16_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x0000, 0x7fff, MWA_NOP},
  {0x8000, 0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xa000, 0xa000, u16_w},
  {0xc000, 0xc004, lamp_w},
  {0xc005, 0xc006, sol_w},
  {0xe000, 0xe000, saa1099_write_port_0_w},
  {0xe001, 0xe001, saa1099_control_port_0_w},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x00, 0x03, z80ctc_2_r},
  {0x04, 0x04, sw0_r},
  {0x08, 0x08, sw_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x00, 0x03, z80ctc_2_w},
  {0x0c, 0x0c, col_w},
  {0x10, 0x10, disp_w},
  {0x14, 0x14, row_w},
  {0x18, 0x18, out_w},
  {0x1c, 0x1c, latch_w},
PORT_END

static struct SAA1099_interface EFO_saa1099Int = {
  1,
  { {100, 100} }
};

static int efo_sw2m(int no) {
  return (no / 10) * 8 + (no % 10) - 1;
}

static int efo_m2sw(int col, int row) {
  return col * 10 + row;
}

extern MACHINE_DRIVER_EXTERN(ZSU);
static MACHINE_DRIVER_START(EFO)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(EFO,EFO,EFO)
  MDRV_SWITCH_UPDATE(EFO)
  MDRV_SWITCH_CONV(efo_sw2m,efo_m2sw)
  MDRV_NVRAM_HANDLER(generic_0fill)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 4000000)
  MDRV_CPU_CONFIG(EFO_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(EFO_vblank, 1)
  MDRV_CPU_PERIODIC_INT(EFO_zc, 200)
  MDRV_SOUND_ADD(SAA1099, EFO_saa1099Int)

  MDRV_IMPORT_FROM(ZSU)
MACHINE_DRIVER_END

#define EFO_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0040, "Start",                 KEYCODE_1) \
    COREPORT_BIT   (0x0001, "Coin 1",                KEYCODE_3) \
    COREPORT_BIT   (0x0002, "Coin 2",                KEYCODE_4) \
    COREPORT_BIT   (0x0004, "Coin 3",                KEYCODE_5) \
    COREPORT_BIT   (0x0008, "Tilt",                  KEYCODE_DEL) \
    COREPORT_BITTOG(0x0010, "Coin Door",             KEYCODE_END) \
    COREPORT_BIT   (0x0020, "Test",                  KEYCODE_7) \
    COREPORT_BIT   (0x0100, "Port 1",                KEYCODE_1_PAD) \
    COREPORT_BIT   (0x0200, "Port 2",                KEYCODE_2_PAD) \
    COREPORT_BIT   (0x0400, "Port 3",                KEYCODE_3_PAD) \
    COREPORT_BIT   (0x0800, "Port 4",                KEYCODE_4_PAD) \
    COREPORT_BIT   (0x1000, "Port 5",                KEYCODE_5_PAD) \
    COREPORT_BIT   (0x2000, "Port 6",                KEYCODE_6_PAD) \
    COREPORT_BIT   (0x4000, "Port 7 (Printer busy)", KEYCODE_7_PAD) \
  INPUT_PORTS_END

static core_tLCDLayout dispEB[] = {
  {0, 0,24,1,CORE_SEG7},{0, 2,25,1,CORE_SEG8D},{0, 4,42,1,CORE_SEG7},{0, 6,27,1,CORE_SEG7},{0, 8,28,1,CORE_SEG8D},{0,10,29,1,CORE_SEG7},{0,12,46,1,CORE_SEG7},{0,14,31,1,CORE_SEG7},
  {0,20,16,1,CORE_SEG7},{0,22,17,1,CORE_SEG8D},{0,24,34,1,CORE_SEG7},{0,26,19,1,CORE_SEG7},{0,28,20,1,CORE_SEG8D},{0,30,21,1,CORE_SEG7},{0,32,38,1,CORE_SEG7},{0,34,23,1,CORE_SEG7},
  {6, 0, 8,1,CORE_SEG7},{6, 2, 9,1,CORE_SEG8D},{6, 4,26,1,CORE_SEG7},{6, 6,11,1,CORE_SEG7},{6, 8,12,2,CORE_SEG8D},{6,10,13,1,CORE_SEG7},{6,12,30,1,CORE_SEG7},{6,14,15,1,CORE_SEG7},
  {6,20, 0,1,CORE_SEG7},{6,22, 1,1,CORE_SEG8D},{6,24,18,1,CORE_SEG7},{6,26, 3,1,CORE_SEG7},{6,28, 4,1,CORE_SEG8D},{6,30, 5,1,CORE_SEG7},{6,32,22,1,CORE_SEG7},{6,34, 7,1,CORE_SEG7},
                                                                     {3,26,32,2,CORE_SEG7},                                             {3,32,54,1,CORE_SEG7},{3,34,39,1,CORE_SEG7},
#ifdef MAME_DEBUG
  {3, 8,50,1,CORE_SEG7},{3,10,35,3,CORE_SEG7}, // segments used in lamp test
#endif
  {0}
};

static core_tGameData ebalchmbGameData = {0,dispEB,{FLIP_SW(FLIP_L),0,2,0,SNDBRD_ZSU,0,1,1}};
static void init_ebalchmb(void) { core_gameData = &ebalchmbGameData; }
ROM_START(ebalchmb)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("jeb_5a0.u18", 0x0000, 0x8000, CRC(87615a7d) SHA1(b27ca2d863040a2641f88f9bd3143467a83f181b))
  SOUNDREGION(0x10000, REGION_CPU2)
    ROM_LOAD("ebe_2a0.u3", 0x0000, 0x8000, CRC(34be32ee) SHA1(ce0271540164639f28d617753760ecc479b6b0d0))
  SOUNDREGION(0x20000, REGION_USER1)
    ROM_LOAD("ebe_1b0.u4", 0x00000, 0x8000, CRC(d696c4e8) SHA1(501e18c258e6d42819d25d72e1907984a6cfeecb))
    ROM_LOAD("ebe_1c0.u5", 0x08000, 0x8000, CRC(fe78d7ef) SHA1(ed91c51dd230854a007f88446011f786759687ca))
    ROM_LOAD("ebe_2d0.u6", 0x10000, 0x8000, CRC(a507081b) SHA1(72d025852a12f455981c61a405f97eaaac9c6fac))
ROM_END
EFO_COMPORTS(ebalchmb, 1)
CORE_GAMEDEFNV(ebalchmb, "Eight Ball Champ (Maibesa)", 198?, "Maibesa", EFO, GAME_IMPERFECT_SOUND)

static core_tLCDLayout dispCB[] = {
  {0, 0,36,1,CORE_SEG8D},{0, 2,29,1,CORE_SEG7},{0, 4,38,1,CORE_SEG7},{0, 6,24,1,CORE_SEG8D},{0, 8,25,2,CORE_SEG7},                      {0,12,31,1,CORE_SEG7},
  {0,26,28,1,CORE_SEG8D},{0,28,21,1,CORE_SEG7},{0,30,30,1,CORE_SEG7},{0,32,16,1,CORE_SEG8D},{0,34,17,2,CORE_SEG7},                      {0,38,23,1,CORE_SEG7},
  {4, 0,20,1,CORE_SEG8D},{4, 2,13,1,CORE_SEG7},{4, 4,22,1,CORE_SEG7},{4, 6, 8,1,CORE_SEG8D},{4, 8, 9,2,CORE_SEG7},                      {4,12,15,1,CORE_SEG7},
  {4,26,12,1,CORE_SEG8D},{4,28, 5,1,CORE_SEG7},{4,30,14,1,CORE_SEG7},{4,32, 0,1,CORE_SEG8D},{4,34, 1,2,CORE_SEG7},                      {4,38, 7,1,CORE_SEG7},
                                               {2,15,46,1,CORE_SEG7},{2,17,32,1,CORE_SEG8D},                      {2,21,34,1,CORE_SEG7},{2,23,39,1,CORE_SEG7},
#ifdef MAME_DEBUG
  {2,11,44,1,CORE_SEG8D},{2,13,37,1,CORE_SEG7},                                             {2,19,33,1,CORE_SEG7}, // "invalid" segments, used in lamp test
//unused & untested segments: 27, 19, 11, 3, 35
#endif
  {0}
};

static core_tGameData cobrapbGameData = {0,dispCB,{FLIP_SW(FLIP_L),0,2,0,SNDBRD_ZSU,0,1,0}};
static void init_cobrapb(void) { core_gameData = &cobrapbGameData; }
ROM_START(cobrapb)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("jcb_4a0.u18", 0x0000, 0x8000, CRC(c663910e) SHA1(c38692343f114388259c4e7b7943e5be934189ca))
  SOUNDREGION(0x10000, REGION_CPU2)
    ROM_LOAD("scb_1a0.u3", 0x0000, 0x8000, CRC(d3675770) SHA1(882ce748308f2d78cccd28fc8cd64fe69bd223e3))
  SOUNDREGION(0x20000, REGION_USER1)
    ROM_LOAD("scb_1b0.u4", 0x00000, 0x8000, CRC(e8e1bdbb) SHA1(215bdfab751cb0ea47aa529df0ac30976de4f772))
    ROM_LOAD("scb_1c0.u5", 0x08000, 0x8000, CRC(c36340ab) SHA1(cd662457959de3a929ba02779e2046ed18b797e2))
ROM_END
EFO_COMPORTS(cobrapb, 3)
CORE_GAMEDEFNV(cobrapb, "Cobra (Playbar)", 1987, "Playbar", EFO, GAME_IMPERFECT_SOUND)

static core_tLCDLayout dispCO[] = {
  {4, 0,36,1,CORE_SEG8D},{4, 2,29,1,CORE_SEG7},{4, 4,38,1,CORE_SEG7},{4, 6,24,1,CORE_SEG8D},{4, 8,25,2,CORE_SEG7},{4,12,31,1,CORE_SEG7},
  {4,16,28,1,CORE_SEG8D},{4,18,21,1,CORE_SEG7},{4,20,30,1,CORE_SEG7},{4,22,16,1,CORE_SEG8D},{4,24,17,2,CORE_SEG7},{4,28,23,1,CORE_SEG7},
  {4,32,20,1,CORE_SEG8D},{4,34,13,1,CORE_SEG7},{4,36,22,1,CORE_SEG7},{4,38, 8,1,CORE_SEG8D},{4,40, 9,2,CORE_SEG7},{4,44,15,1,CORE_SEG7},
  {4,48,12,1,CORE_SEG8D},{4,50, 5,1,CORE_SEG7},{4,52,14,1,CORE_SEG7},{4,54, 0,1,CORE_SEG8D},{4,56, 1,2,CORE_SEG7},{4,60, 7,1,CORE_SEG7},
  {0,44,44,1,CORE_SEG7} ,{0,46,37,1,CORE_SEG7},{0,49,46,1,CORE_SEG7},{0,52,32,1,CORE_SEG7}, {0,55,33,2,CORE_SEG7},
#ifdef MAME_DEBUG
                                                                                                                  {0,60,39,1,CORE_SEG7},
//unused & untested segments: 27, 19, 11, 3, 35
#endif
  {0}
};

static MACHINE_DRIVER_START(comeback)
  MDRV_IMPORT_FROM(EFO)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END

static core_tGameData comebackGameData = {0,dispCO,{FLIP_SW(FLIP_L),0,2,0,SNDBRD_ZSU,0,0,1}};
static void init_comeback(void) { core_gameData = &comebackGameData; }
ROM_START(comeback)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("jco_6a0.u18", 0x0000, 0x8000, CRC(31268ca1) SHA1(d6132d021e808d107dd29c7da0fbb4bc887339a7))
  SOUNDREGION(0x10000, REGION_CPU2)
    ROM_LOAD("cbs_3a0.u3", 0x0000, 0x8000, CRC(d0f55dc9) SHA1(91186e2cbe248323380418911240a9a5887063fb))
  SOUNDREGION(0x20000, REGION_USER1)
    ROM_LOAD("cbs_3b0.u4", 0x00000, 0x8000, CRC(1da16d36) SHA1(9f7a27ae23064c1183a346ff042e6cba148257c7))
    ROM_LOAD("cbs_1c0.u5", 0x08000, 0x8000, CRC(794ae588) SHA1(adaa5e69232523369a6a2da865ac05102cc04ec8))
ROM_END
EFO_COMPORTS(comeback, 1)
CORE_GAMEDEFNV(comeback, "Come Back", 1988, "Nondum / CIFA", comeback, GAME_IMPERFECT_SOUND)
