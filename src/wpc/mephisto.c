#include "driver.h"
#include "core.h"
#include "cpu/i86/i88intf.h"
#include "cpu/i8051/i8051.h"
#include "machine/8255ppi.h"
/*
static struct {
  UINT8 col;
} locals;
*/

static WRITE_HANDLER(m120xx_w) {
//  logerror("Write to  $120%02x: %02x\n", offset, data);
  coreGlobals.lampMatrix[offset] = data;
}

static READ_HANDLER(m120xx_r) {
  logerror("Read from $120%02x\n", offset);
  return ~coreGlobals.swMatrix[offset];
}

static MEMORY_WRITE_START(mephisto_writemem)
  {0x00000,0x0ffff, MWA_ROM},
  {0x10000,0x107ff, MWA_RAM},
  {0x12000,0x1201f, m120xx_w},
  {0x13000,0x130ff, MWA_RAM},
  {0x14000,0x140ff, MWA_RAM},
  {0xf8000,0xfffff, MWA_ROM},
MEMORY_END

static MEMORY_READ_START(mephisto_readmem)
  {0x00000,0x0ffff, MRA_ROM},
  {0x10000,0x107ff, MRA_RAM},
  {0x12000,0x1201f, m120xx_r},
  {0x13000,0x130ff, MRA_RAM},
  {0x14000,0x140ff, MRA_RAM},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static READ_HANDLER(ic4_porta_r) {
  logerror("IC4 port A read\n");
  return 0;
}

static READ_HANDLER(ic4_portb_r) {
  logerror("IC4 port B read\n");
  return 0;
}

static READ_HANDLER(ic4_portc_r) {
  logerror("IC4 port C read\n");
  return 0;
}

static READ_HANDLER(ic9_porta_r) {
  logerror("IC9 port A read\n");
  return 0;
}

static READ_HANDLER(ic9_portb_r) {
  logerror("IC9 port B read\n");
  return 0;
}

static READ_HANDLER(ic9_portc_r) {
  logerror("IC9 port C read\n");
  return 0;
}

static READ_HANDLER(ic20_porta_r) {
  logerror("IC20 port A read\n");
  return 0;
}

static READ_HANDLER(ic20_portb_r) {
  logerror("IC20 port B read\n");
  return 0;
}

static READ_HANDLER(ic20_portc_r) {
  logerror("IC20 port C read\n");
  return 0;
}

static WRITE_HANDLER(ic4_porta_w) {
  logerror("IC4 port A write=%02x\n", data);
}

static WRITE_HANDLER(ic4_portb_w) {
  logerror("IC4 port B write=%02x\n", data);
}

static WRITE_HANDLER(ic4_portc_w) {
  logerror("IC4 port C write=%02x\n", data);
}

static WRITE_HANDLER(ic9_porta_w) {
  logerror("IC9 port A write=%02x\n", data);
}

static WRITE_HANDLER(ic9_portb_w) {
  logerror("IC9 port B write=%02x\n", data);
}

static WRITE_HANDLER(ic9_portc_w) {
  logerror("IC9 port C write=%02x\n", data);
}

static WRITE_HANDLER(ic20_porta_w) {
  logerror("IC20 port A write=%02x\n", data);
}

static WRITE_HANDLER(ic20_portb_w) {
  logerror("IC20 port B write=%02x\n", data);
}

static WRITE_HANDLER(ic20_portc_w) {
  logerror("IC20 port C write=%02x\n", data);
}

static ppi8255_interface ppi8255_intf = {
	3,
	{ic4_porta_r, ic9_porta_r, ic20_porta_r},	/* Port A read */
	{ic4_portb_r, ic9_portb_r, ic20_portb_r},	/* Port B read */
	{ic4_portc_r, ic9_portc_r, ic20_portc_r},	/* Port C read */
	{ic4_porta_w, ic9_porta_w, ic20_porta_w},	/* Port A write */
	{ic4_portb_w, ic9_portb_w, ic20_portb_w},	/* Port B write */
	{ic4_portc_w, ic9_portc_w, ic20_portc_w},	/* Port C write */
};

static MACHINE_INIT(MEPHISTO) {
	/* init PPI */
	ppi8255_init(&ppi8255_intf);
}

static SWITCH_UPDATE(MEPHISTO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x01, 0);
  }
}

static INTERRUPT_GEN(mephisto_vblank) {
  core_updateSw(TRUE);
}
static INTERRUPT_GEN(mephisto_irq) {
  static int state;
  cpu_set_irq_line(0, 0, (state = !state));
}

#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER(ay8910_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_data_w)   { AY8910Write(0,1,data); }
#endif
static READ_HANDLER(ay8910_porta_r)	  { return coreGlobals.swMatrix[0]; }
static READ_HANDLER(ay8910_portb_r)   { return coreGlobals.swMatrix[1]; }
static WRITE_HANDLER(ay8910_porta_w)	{ coreGlobals.tmpLampMatrix[0] = data; }
static WRITE_HANDLER(ay8910_portb_w)	{ coreGlobals.tmpLampMatrix[1] = data; }

struct AY8910interface mephisto_ay8910Int = {
	1,			/* 1 chip */
	2000000,	/* 2 MHz */
	{ 50 },		/* Volume */
	{ ay8910_porta_r },
  { ay8910_portb_r },
	{ ay8910_porta_w },
	{ ay8910_portb_w },
};

static struct DACinterface mephisto_dacInt = { 1, { 50 }};

static MEMORY_READ_START(mephisto_readsnd)
	{ 0x00000, 0x0ffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(mephisto_writesnd)
	{ 0x00000, 0x0ffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(mephisto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(MEPHISTO,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", I88, 8000000)
  MDRV_CPU_MEMORY(mephisto_readmem, mephisto_writemem)
  MDRV_CPU_PERIODIC_INT(mephisto_irq, 120)
  MDRV_CPU_VBLANK_INT(mephisto_vblank, 1)
//  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(MEPHISTO)

  MDRV_CPU_ADD_TAG("scpu", I8051, 12000000)
  MDRV_CPU_MEMORY(mephisto_readsnd, mephisto_writesnd)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ADD(AY8910, mephisto_ay8910Int)
  MDRV_SOUND_ADD(DAC, mephisto_dacInt)
MACHINE_DRIVER_END

INPUT_PORTS_START(mephisto)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT(     0x0100, "Test", KEYCODE_0)
INPUT_PORTS_END

core_tLCDLayout mephisto_disp[] = {
  {0, 0, 0, 7,CORE_SEG8D}, {0,16, 7, 7,CORE_SEG8D},
  {3, 0,14, 7,CORE_SEG8D}, {3,16,21, 7,CORE_SEG8D},
  {6, 8,28, 2,CORE_SEG8D}, {6,14,30, 1,CORE_SEG8D}, {6,18,31, 2,CORE_SEG8D},
  {0}
};
static core_tGameData mephistoGameData = {0,mephisto_disp,{FLIP_SW(FLIP_L),0,24}};
static void init_mephisto(void) {
  core_gameData = &mephistoGameData;
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
#define init_mephist1 init_mephisto
#define input_ports_mephist1 input_ports_mephisto
CORE_CLONEDEFNV(mephist1,mephisto,"Mephisto (rev. 1.1)",1986,"Stargame",mephisto,GAME_NOT_WORKING)
