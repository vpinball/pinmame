/***************************************************************************

  E.F.O. ZCU pinball game driver.
  Many companies used game and / or sound drivers produced by E.F.O. SA,
  even Playmatic used their sound board on the last two games.
  
  The main CPU is copy protected by a registered PAL chip.
  This will generate some offset address table in RAM the code jumps to,
  so if the table is wrong, the code will end up doing funny things.
  
  To this day, the PAL is undecoded, so happy hacking everyone... :)

***************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "machine/z80fmly.h"

static struct {
  int col, row, pos;
} locals;

static INTERRUPT_GEN(EFO_vblank) {
	static int vblankCount;
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  if ((vblankCount % 12) == 0) {
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  vblankCount++;
  core_updateSw(TRUE);
}

static INTERRUPT_GEN(EFO_zc) {
  static int zc;
  z80ctc_2_trg2_w(0, zc);
//  z80ctc_2_trg3_w(0, zc); // shown connected on schematics, but would prevent cobrapb from running?!
  zc = !zc;
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

static void ck0_pulse(int param) {
  z80ctc_2_trg0_w(0, 1);
  z80ctc_2_trg0_w(0, 0);
}

static MACHINE_INIT(EFO) {
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_CPU2), NULL, NULL);
  ctc_intf.baseclock[2] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);
	timer_pulse(TIME_IN_HZ(2000000), 0, ck0_pulse);
}

static MACHINE_RESET(EFO) {
  memset(&locals, 0x00, sizeof(locals));
  z80ctc_2_reset();
}

static MACHINE_STOP(EFO) {
  sndbrd_0_exit();
}

static SWITCH_UPDATE(EFO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static READ_HANDLER(sw0_r) {
  return ~coreGlobals.swMatrix[0];
}

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[locals.col + 1];
}

static UINT8 u16_addr[2];
static READ_HANDLER(u16_r) {
  logerror("U16 READ  @%04x\n", activecpu_get_pc());
  return u16_addr[0];
}

static WRITE_HANDLER(u16_w) {
	static int count;
  logerror("U16 WRITE @%04x: %02x\n", activecpu_get_pc(), data);
	u16_addr[count] = data;
	if (!count) coreGlobals.tmpLampMatrix[8] = data;
  count = (count + 1) % 2;
}

static WRITE_HANDLER(col_w) {
  locals.col = core_BitColToNum(data);
}

static WRITE_HANDLER(row_w) {
  locals.row = !data ? 0 : 1 + core_BitColToNum(data);
  locals.pos = 0;
}

static WRITE_HANDLER(disp_w) {
  if (locals.pos < 7 && locals.row < 8) {
    coreGlobals.segments[locals.pos * 8 + 7 - locals.row].w = data;
  }
  locals.pos++;
}

static WRITE_HANDLER(snd_w) {
  sndbrd_0_data_w(0, data);
}

static WRITE_HANDLER(lamp_w) {
	coreGlobals.tmpLampMatrix[offset] |= data;
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x7fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0xa000, 0xa000, u16_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x0000, 0x7fff, MWA_NOP},
  {0x8000, 0x87ff, MWA_RAM/*, &generic_nvram, &generic_nvram_size*/},
  {0xa000, 0xa000, u16_w},
  {0xc000, 0xc007, lamp_w},
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
  {0x1c, 0x1c, snd_w},
PORT_END

static struct SAA1099_interface EFO_saa1099Int = {
  1,
  { {100, 100} }
};

extern MACHINE_DRIVER_EXTERN(ZSU);
MACHINE_DRIVER_START(EFO)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(EFO,EFO,EFO)
  MDRV_SWITCH_UPDATE(EFO)
  MDRV_NVRAM_HANDLER(generic_0fill)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 4000000)
  MDRV_CPU_CONFIG(EFO_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(EFO_vblank, 1)
  MDRV_CPU_PERIODIC_INT(EFO_zc, 100)
  MDRV_SOUND_ADD(SAA1099, EFO_saa1099Int)

  MDRV_IMPORT_FROM(ZSU)
MACHINE_DRIVER_END

#define EFO_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0001, "Key 1",      KEYCODE_1) \
    COREPORT_BIT   (0x0002, "Key 2",      KEYCODE_2) \
    COREPORT_BIT   (0x0004, "Key 3",      KEYCODE_3) \
    COREPORT_BIT   (0x0008, "Key 4",      KEYCODE_4) \
    COREPORT_BIT   (0x0010, "Key 5",      KEYCODE_5) \
    COREPORT_BIT   (0x0020, "Key 6",      KEYCODE_6) \
    COREPORT_BIT   (0x0040, "Key 7",      KEYCODE_7) \
    COREPORT_BIT   (0x0080, "Key 8",      KEYCODE_8) \
  INPUT_PORTS_END

static core_tLCDLayout dispEB[] = {
  { 0,0,48,2,CORE_SEG8D},{ 0,4,10,1,CORE_SEG8D},{ 0,6,51,3,CORE_SEG8D},{ 0,12,14,1,CORE_SEG7},{ 0,14,55,1,CORE_SEG8D},
  { 2,0,40,2,CORE_SEG8D},{ 2,4, 2,1,CORE_SEG8D},{ 2,6,43,3,CORE_SEG8D},{ 2,12, 6,1,CORE_SEG7},{ 2,14,47,1,CORE_SEG8D},
  { 4,0,32,2,CORE_SEG8D},{ 4,4,50,1,CORE_SEG8D},{ 4,6,35,3,CORE_SEG8D},{ 4,12,54,1,CORE_SEG7},{ 4,14,39,1,CORE_SEG8D},
  { 6,0,24,2,CORE_SEG8D},{ 6,4,42,1,CORE_SEG8D},{ 6,6,27,3,CORE_SEG8D},{ 6,12,46,1,CORE_SEG7},{ 6,14,31,1,CORE_SEG8D},
  { 8,0,16,2,CORE_SEG8D},{ 8,4,34,1,CORE_SEG8D},{ 8,6,19,3,CORE_SEG8D},{ 8,12,38,1,CORE_SEG7},{ 8,14,23,1,CORE_SEG8D},
  {10,0, 8,2,CORE_SEG8D},{10,4,26,1,CORE_SEG8D},{10,6,11,3,CORE_SEG8D},{10,12,30,1,CORE_SEG7},{10,14,15,1,CORE_SEG8D},
  {12,0, 0,2,CORE_SEG8D},{12,4,18,1,CORE_SEG8D},{12,6, 3,3,CORE_SEG8D},{12,12,22,1,CORE_SEG7},{12,14, 7,1,CORE_SEG8D},
  {0}
};

static core_tGameData ebalchmbGameData = {0,dispEB,{FLIP_SW(FLIP_L),0,1,0,SNDBRD_ZSU}};
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
CORE_GAMEDEFNV(ebalchmb, "Eight Ball Champ (Maibesa)", 198?, "Maibesa", EFO, GAME_NOT_WORKING)

static core_tGameData comebackGameData = {0,dispEB,{FLIP_SW(FLIP_L),0,1,0,SNDBRD_ZSU}};
static void init_comeback(void) { core_gameData = &comebackGameData; }
ROM_START(comeback)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("jeb_5a0.u18", 0x0000, 0x8000, CRC(87615a7d) SHA1(b27ca2d863040a2641f88f9bd3143467a83f181b))
  SOUNDREGION(0x10000, REGION_CPU2)
    ROM_LOAD("cbs_3a0.u3", 0x0000, 0x8000, CRC(d0f55dc9) SHA1(91186e2cbe248323380418911240a9a5887063fb))
  SOUNDREGION(0x20000, REGION_USER1)
    ROM_LOAD("cbs_3b0.u4", 0x00000, 0x8000, CRC(1da16d36) SHA1(9f7a27ae23064c1183a346ff042e6cba148257c7))
    ROM_LOAD("cbs_1c0.u5", 0x08000, 0x8000, CRC(794ae588) SHA1(adaa5e69232523369a6a2da865ac05102cc04ec8))
ROM_END
EFO_COMPORTS(comeback, 1)
CORE_GAMEDEFNV(comeback, "Come Back", 198?, "Nondum / CIFA", EFO, GAME_NOT_WORKING)

static core_tLCDLayout dispCB[] = {
  { 0,0,24,3,CORE_SEG8D},{ 0, 6,31,1,CORE_SEG8D},{ 0, 8,18,1,CORE_SEG8D},{ 0,10,23,1,CORE_SEG8D},{ 0,12,29,1,CORE_SEG8D},{ 0,14,27,1,CORE_SEG8D},
  { 2,0,28,1,CORE_SEG8D},{ 2, 2,21,1,CORE_SEG8D},{ 2, 4,30,1,CORE_SEG8D},{ 2, 6,16,2,CORE_SEG8D},{ 2,10,22,1,CORE_SEG8D},{ 2,12,20,1,CORE_SEG8D},{ 2,14,19,1,CORE_SEG8D},
  { 4,0, 8,3,CORE_SEG8D},{ 4, 6,15,1,CORE_SEG8D},{ 4, 8, 2,1,CORE_SEG8D},{ 4,10, 7,1,CORE_SEG8D},{ 4,12,13,1,CORE_SEG8D},{ 4,14,11,1,CORE_SEG8D},
  { 6,0,12,1,CORE_SEG8D},{ 6, 2, 5,1,CORE_SEG8D},{ 6, 4,14,1,CORE_SEG8D},{ 6, 6, 0,2,CORE_SEG8D},{ 6,10, 6,1,CORE_SEG8D},{ 6,12, 4,1,CORE_SEG8D},{ 6,14, 3,1,CORE_SEG8D},
  { 8,0,40,3,CORE_SEG8D},{ 8, 6,47,1,CORE_SEG8D},{ 8, 8,34,1,CORE_SEG8D},{ 8,10,39,1,CORE_SEG8D},{ 8,12,45,1,CORE_SEG8D},{ 8,14,43,1,CORE_SEG8D},
  {10,0,44,1,CORE_SEG8D},{10, 2,37,1,CORE_SEG8D},{10, 4,46,1,CORE_SEG8D},{10, 6,32,2,CORE_SEG8D},{10,10,38,1,CORE_SEG8D},{10,12,36,1,CORE_SEG8D},{10,14,35,1,CORE_SEG8D},
  {12,0,48,8,CORE_SEG8D},
  {0}
};

static core_tGameData cobrapbGameData = {0,dispCB,{FLIP_SW(FLIP_L),0,1,0,SNDBRD_ZSU}};
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
EFO_COMPORTS(cobrapb, 1)
CORE_GAMEDEFNV(cobrapb, "Cobra (Playbar)", 1987, "Playbar", EFO, GAME_NOT_WORKING)
