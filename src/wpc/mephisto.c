#include "driver.h"
#include "core.h"
#include "cpu/i86/i88intf.h"
#include "cpu/i8051/i8051.h"
/*
static struct {
  UINT8 col;
} locals;
*/

static WRITE_HANDLER(ic4_w) {
//  logerror("Write to  IC4:  %02x:%02x\n", offset, data);
}

static READ_HANDLER(ic4_r) {
//  logerror("Read from IC4:  %02x\n", offset);
  return 0;
}

static WRITE_HANDLER(ic20_w) {
  logerror("Write to  IC20: %x:%02x\n", offset, data);
}

static READ_HANDLER(ic20_r) {
  logerror("Read from IC20: %x\n", offset);
  return 0;
}

static WRITE_HANDLER(ic9_w) {
  logerror("Write to  IC9:  %x:%02x\n", offset, data);
}

static READ_HANDLER(ic9_r) {
  logerror("Read from IC9:  %x\n", offset);
  return 0;
}

static WRITE_HANDLER(shift_w) {
  logerror("Write to SHIFT: %02x\n", data);
}

static WRITE_HANDLER(diag_w) {
  logerror("DIAG %x\n", offset);
}

static MEMORY_WRITE_START(mephisto_writemem)
  {0x10000,0x107ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x12000,0x1201f, ic4_w},
  {0x13000,0x130ff, MWA_RAM},
  {0x13800,0x13807, ic20_w},
  {0x14000,0x140ff, MWA_RAM},
  {0x14800,0x14807, ic9_w},
  {0x16000,0x16000, shift_w},
  {0x17000,0x17001, diag_w},
MEMORY_END

static MEMORY_READ_START(mephisto_readmem)
  {0x00000,0x0ffff, MRA_ROM},
  {0x10000,0x107ff, MRA_RAM},
  {0x12000,0x1201f, ic4_r},
  {0x13000,0x130ff, MRA_RAM},
  {0x13800,0x13807, ic20_r},
  {0x14000,0x140ff, MRA_RAM},
  {0x14800,0x14807, ic9_r},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(cirsa_writemem)
  {0x20000,0x21fff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x2a000,0x2a01f, ic4_w},
  {0x2b000,0x2b0ff, MWA_RAM},
  {0x2b800,0x2b807, ic20_w},
  {0x2c000,0x2c0ff, MWA_RAM},
  {0x2c800,0x2c807, ic9_w},
  {0x2e000,0x2e000, shift_w},
  {0x2f000,0x2f000, diag_w},
MEMORY_END

static MEMORY_READ_START(cirsa_readmem)
  {0x00000,0x0ffff, MRA_ROM},
  {0x20000,0x21fff, MRA_RAM},
  {0x2a000,0x2a01f, ic4_r},
  {0x2b000,0x2b0ff, MRA_RAM},
  {0x2b800,0x2b807, ic20_r},
  {0x2c000,0x2c0ff, MRA_RAM},
  {0x2c800,0x2c807, ic9_r},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MACHINE_INIT(CIRSA) {
}

static SWITCH_UPDATE(CIRSA) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x01, 0);
  }
}

static INTERRUPT_GEN(cirsa_vblank) {
  core_updateSw(TRUE);
}

static INTERRUPT_GEN(cirsa_irq) {
  static int state;
  cpu_set_irq_line(0, 0, (state = !state));
}

static READ_HANDLER(ay8910_porta_r)	  { return coreGlobals.swMatrix[0]; }
static READ_HANDLER(ay8910_portb_r)   { return coreGlobals.swMatrix[1]; }
static WRITE_HANDLER(ay8910_porta_w)	{ coreGlobals.tmpLampMatrix[0] = data; }
static WRITE_HANDLER(ay8910_portb_w)	{ coreGlobals.tmpLampMatrix[1] = data; }

static void ym3812_irq(int irq) {
  cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct AY8910interface cirsa_ay8910Int = {
  1,			/* 1 chip */
  1500000,	/* 1.5 MHz */
  { 50 },		/* Volume */
  { ay8910_porta_r },
  { ay8910_portb_r },
  { ay8910_porta_w },
  { ay8910_portb_w },
};

static struct YM3812interface cirsa_ym3812Int = {
  1,						/* 1 chip */
  3579545,				/* NTSC clock */
  { 50 },				/* volume */
  { ym3812_irq },			/* IRQ Callback */
};

static struct DACinterface cirsa_dacInt = { 1, { 50 }};

static WRITE_HANDLER(bank_w) {
  cpu_setbank(1, memory_region(REGION_SOUND1) + data * 0x8000);
  logerror("SND BANK %x:%02x\n", offset, data);
}

static READ_HANDLER(port_r) {
  logerror("SND PORT %x READ\n", offset);
  return 0;
}

static WRITE_HANDLER(port_w) {
  logerror("SND PORT %x:%02x\n", offset, data);
}

static MEMORY_READ_START(cirsa_readsnd)
  { 0x00000, 0x07fff, MRA_ROM },
  { 0x08000, 0x0ffff, MRA_BANKNO(1) },
  { 0x10000, 0x107ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(cirsa_writesnd)
  { 0x00000, 0x07fff, MWA_ROM },
  { 0x10000, 0x107ff, MWA_RAM },
  { 0x10800, 0x10800, bank_w },
  { 0x11000, 0x11000, DAC_0_data_w },
MEMORY_END

static PORT_READ_START(cirsa_readsndport)
  { 1, 1, AY8910_read_port_0_r },
  { 0, 3, port_r },
PORT_END

static PORT_WRITE_START(cirsa_writesndport)
  { 1, 1, AY8910_control_port_0_w },
  { 3, 3, AY8910_write_port_0_w },
  { 0, 3, port_w },
PORT_END

MACHINE_DRIVER_START(mephisto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(CIRSA,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", I88, 6000000)
  MDRV_CPU_MEMORY(mephisto_readmem, mephisto_writemem)
  MDRV_CPU_PERIODIC_INT(cirsa_irq, 100)
  MDRV_CPU_VBLANK_INT(cirsa_vblank, 1)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(CIRSA)

  MDRV_CPU_ADD_TAG("scpu", I8051, 12000000)
  MDRV_CPU_MEMORY(cirsa_readsnd, cirsa_writesnd)
  MDRV_CPU_PORTS(cirsa_readsndport, cirsa_writesndport)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ADD(AY8910, cirsa_ay8910Int)
  MDRV_SOUND_ADD(YM3812, cirsa_ym3812Int)
  MDRV_SOUND_ADD(DAC, cirsa_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(cirsa)
  MDRV_IMPORT_FROM(mephisto)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(cirsa_readmem, cirsa_writemem)
MACHINE_DRIVER_END

INPUT_PORTS_START(cirsa)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT(     0x0100, "Test", KEYCODE_0)
INPUT_PORTS_END

core_tLCDLayout cirsa_disp[] = {
  {0, 0, 0, 7,CORE_SEG8D}, {0,16, 7, 7,CORE_SEG8D},
  {3, 0,14, 7,CORE_SEG8D}, {3,16,21, 7,CORE_SEG8D},
  {6, 8,28, 2,CORE_SEG8D}, {6,14,30, 1,CORE_SEG8D}, {6,18,31, 2,CORE_SEG8D},
  {0}
};
static core_tGameData cirsaGameData = {0,cirsa_disp,{FLIP_SW(FLIP_L),0,8}};
static void init_cirsa(void) {
  core_gameData = &cirsaGameData;
}

ROM_START(mephisto)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("cpu_ver1.2", 0x00000, 0x8000, CRC(845c8eb4) SHA1(2a705629990950d4e2d3a66a95e9516cf112cc88))
      ROM_RELOAD(0x08000, 0x8000)
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("ic15_02", 0x00000, 0x8000, CRC(2accd446) SHA1(7297e4825c33e7cf23f86fe39a0242e74874b1e2))
  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("ic14_s0", 0x00000, 0x8000, CRC(7cea3018) SHA1(724fe7a4456cbf2ac01466d946668ee86f4410ae))
    ROM_LOAD("ic13_s1", 0x08000, 0x8000, CRC(5a9e0f1d) SHA1(dbfd307706c51f8809f4867a199b4b62beb64379))
    ROM_LOAD("ic12_s2", 0x10000, 0x8000, CRC(b3cc962a) SHA1(521376cab7e917a5d5f5f183bccb21bd13327c48))
    ROM_LOAD("ic11_s3", 0x18000, 0x8000, CRC(8aaa21ec) SHA1(29f17249cac62128fd8b0eee415ce399ee2ec672))
    ROM_LOAD("ic16_c",  0x20000, 0x8000, CRC(5f12b4f4) SHA1(73fbdb57fca0dbc918e6665a6cb949e741f2720a))
    ROM_LOAD("ic17_d",  0x28000, 0x8000, CRC(d17e18a8) SHA1(372eaf209ea5d26f3c096aadd7d028ef68bfb68e))
    ROM_LOAD("ic18_e",  0x30000, 0x8000, CRC(eac6dbba) SHA1(f4971c8b0aa3a72c396b943a0ee3094afb902ec1))
    ROM_LOAD("ic19_f",  0x38000, 0x8000, CRC(cc4bb629) SHA1(db46be2a8034bbd106b7dd80f50988c339684b5e))
ROM_END
#define init_mephisto init_cirsa
#define input_ports_mephisto input_ports_cirsa
CORE_GAMEDEFNV(mephisto,"Mephisto (rev. 1.2)",1986,"Stargame",mephisto,GAME_NOT_WORKING)

ROM_START(mephist1)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("cpu_ver1.1", 0x00000, 0x8000, CRC(ce584902) SHA1(dd05d008bbd9b6588cb204e8d901537ffe7ddd43))
      ROM_RELOAD(0x08000, 0x8000)
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("ic15_02", 0x00000, 0x8000, CRC(2accd446) SHA1(7297e4825c33e7cf23f86fe39a0242e74874b1e2))
  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("ic14_s0", 0x00000, 0x8000, CRC(7cea3018) SHA1(724fe7a4456cbf2ac01466d946668ee86f4410ae))
    ROM_LOAD("ic13_s1", 0x08000, 0x8000, CRC(5a9e0f1d) SHA1(dbfd307706c51f8809f4867a199b4b62beb64379))
    ROM_LOAD("ic12_s2", 0x10000, 0x8000, CRC(b3cc962a) SHA1(521376cab7e917a5d5f5f183bccb21bd13327c48))
    ROM_LOAD("ic11_s3", 0x18000, 0x8000, CRC(8aaa21ec) SHA1(29f17249cac62128fd8b0eee415ce399ee2ec672))
    ROM_LOAD("ic16_c",  0x20000, 0x8000, CRC(5f12b4f4) SHA1(73fbdb57fca0dbc918e6665a6cb949e741f2720a))
    ROM_LOAD("ic17_d",  0x28000, 0x8000, CRC(d17e18a8) SHA1(372eaf209ea5d26f3c096aadd7d028ef68bfb68e))
    ROM_LOAD("ic18_e",  0x30000, 0x8000, CRC(eac6dbba) SHA1(f4971c8b0aa3a72c396b943a0ee3094afb902ec1))
    ROM_LOAD("ic19_f",  0x38000, 0x8000, CRC(cc4bb629) SHA1(db46be2a8034bbd106b7dd80f50988c339684b5e))
ROM_END
#define init_mephist1 init_cirsa
#define input_ports_mephist1 input_ports_cirsa
CORE_CLONEDEFNV(mephist1,mephisto,"Mephisto (rev. 1.1)",1986,"Stargame",mephisto,GAME_NOT_WORKING)

ROM_START(sport2k)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("u1_256.bin", 0x00000, 0x8000, CRC(403f9000) SHA1(376dc17355c9569bd1ed9b19dbc322bfd69bf938))
    ROM_LOAD("u2_256.bin", 0x08000, 0x8000, CRC(4a88cc10) SHA1(591568dc60c40cc058f45a144c098faccb4e970c))
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("c541_256.bin", 0x00000, 0x8000, CRC(7ca4a952) SHA1(6b01f7f79fa88c4ae71a6a19341760fa256b9958))
  NORMALREGION(0x50000, REGION_SOUND1)
    ROM_LOAD("s117_512.bin", 0x00000, 0x10000, CRC(035d302e) SHA1(f207ea239e5a34839366cc19a569ab5f3d1e1a60))
    ROM_LOAD("s211_512.bin", 0x10000, 0x10000, CRC(61cf84f9) SHA1(4c5680fbf48f30fbe0e15f4194ab708955df7721))
    ROM_LOAD("s311_512.bin", 0x20000, 0x10000, CRC(162cd1ff) SHA1(4d9ad7a839cc16e74abfc77c92674608ccba8cc3))
    ROM_LOAD("s411_512.bin", 0x30000, 0x10000, CRC(4deffaa0) SHA1(98a20a01437ea060ac5c6fb52f4da892fee1fb75))
    ROM_LOAD("s511_512.bin", 0x40000, 0x10000, CRC(ca9afa80) SHA1(6f219bdc1ad06e340b2930610897b70369a43684))
ROM_END
#define init_sport2k init_cirsa
#define input_ports_sport2k input_ports_cirsa
CORE_GAMEDEFNV(sport2k,"Sport 2000",1988,"Cirsa",cirsa,GAME_NOT_WORKING)
