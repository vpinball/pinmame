/************************************************************************************************
 Wico Af-Tor (Wico's sole commercial game)
 -----------

   Hardware:
   ---------
		CPU:	 2 x 6809 for game and sound, they seem to share one RAM chip!?
			INT: IRQ @ ZC-speed for 1st CPU,
			     NE555 chip for 2nd CPU
		DISPLAY: 9-segment LCD
		SOUND:	 SN76494 (76496-compatible?)
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "gen.h"
#include "cpu/m6809/m6809.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  sndCmd;
  UINT8  swCol;
  UINT8  lampCol;
} locals;

static INTERRUPT_GEN(WICO_irq_0) {
  cpu_set_irq_line(0, M6809_IRQ_LINE, PULSE_LINE);
}

static void WICO_firq_0(int data) {
  static int last;
  cpu_set_irq_line(0, M6809_FIRQ_LINE, (last = !last) ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(WICO_irq_1) {
  cpu_set_irq_line(1, M6809_IRQ_LINE, PULSE_LINE);
}

static void WICO_firq_1(int data) {
  static int last;
  cpu_set_irq_line(1, M6809_FIRQ_LINE, (last = !last) ? ASSERT_LINE : CLEAR_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(WICO_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(WICO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
  cpu_set_nmi_line(0, coreGlobals.swMatrix[0] & 1 ? ASSERT_LINE : CLEAR_LINE);
  if (coreGlobals.swMatrix[0] & 2) WICO_firq_0(0);
  cpu_set_nmi_line(1, coreGlobals.swMatrix[0] & 4 ? ASSERT_LINE : CLEAR_LINE);
  if (coreGlobals.swMatrix[0] & 8) WICO_firq_1(0);
}

static WRITE_HANDLER(io0_w) {
  switch (offset) {
    case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7: case 0xe8: case 0xe9:
      coreGlobals.tmpLampMatrix[offset - 0xe0] = data; break;
    default:
      logerror("io0_w: offset %x, data %02x\n", offset, data);
  }
}
static READ_HANDLER(io0_r) {
  UINT8 ret = 0;
  switch (offset) {
    case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
      ret = coreGlobals.swMatrix[offset - 0xea]; break;
    default:
      logerror("io0_r: offset %x\n", offset);
  }
  return ret;
}

static WRITE_HANDLER(io1_w) {
  switch (offset) {
    case 0xe0:
      locals.solenoids = (locals.solenoids & 0xffffff00) | data; break;
    case 0xe1:
      locals.solenoids = (locals.solenoids & 0xff0ff0ff) | (data << 8); break;
    case 0xe2:
      locals.solenoids = (locals.solenoids & 0xff00ffff) | (data << 16); break;
    case 0xe6:
      locals.solenoids = (locals.solenoids & 0x00ffffff) | (data << 24); break;
    default:
      logerror("io1_w: offset %x, data %02x\n", offset, data);
  }
}
static READ_HANDLER(io1_r) {
  switch (offset) {
    default:
      logerror("io1_r: offset %x\n", offset); return 0;
  }
}

static UINT8 *ram_01;
static READ_HANDLER(ram_01_r) {
  return ram_01[offset];
}
static WRITE_HANDLER(ram_01_w) {
  ram_01[offset] = data;
}

static UINT8 *nvram;
// nvram uses lower nybble only
static READ_HANDLER(nvram_r) {
  return nvram[offset];
}
static WRITE_HANDLER(nvram_w) {
  nvram[offset] = data & 0x0f;
}

static MEMORY_READ_START(WICO_0_readmem)
  {0x0000,0x07ff, ram_01_r},
  {0xf000,0xffff, MRA_ROM},
  {0x1f00,0x1fff, io0_r},
MEMORY_END

static MEMORY_WRITE_START(WICO_0_writemem)
  {0x0000,0x07ff, ram_01_w, &ram_01},
  {0x1f00,0x1fff, io0_w},
MEMORY_END

static MEMORY_READ_START(WICO_1_readmem)
  {0x0000,0x07ff, ram_01_r},
  {0x8000,0x9fff, MRA_ROM},
  {0xc000,0xffff, MRA_ROM},
  {0x1f00,0x1fff, io1_r},
  {0x4000,0x40ff, nvram_r},
MEMORY_END

static MEMORY_WRITE_START(WICO_1_writemem)
  {0x0000,0x07ff, ram_01_w},
  {0x1f00,0x1fff, io1_w},
  {0x4000,0x40ff, nvram_w, &nvram},
MEMORY_END

static MACHINE_INIT(WICO) {
  memset(&locals, 0, sizeof locals);
}

struct SN76496interface WICO_sn76494Int = {
	1,	/* total number of chips in the machine */
	{ 1000000 },	/* base clock */
	{ 75 }	/* volume */
};

MACHINE_DRIVER_START(aftor)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(WICO,NULL,NULL)
  MDRV_SWITCH_UPDATE(WICO)
  MDRV_DIPS(16)
  MDRV_NVRAM_HANDLER(generic_0fill)

  MDRV_CPU_ADD_TAG("mcpu", M6809, 1500000)
  MDRV_CPU_VBLANK_INT(WICO_vblank, 1)
  MDRV_CPU_MEMORY(WICO_0_readmem, WICO_0_writemem)
  MDRV_CPU_PERIODIC_INT(WICO_irq_0, 120)
//  MDRV_TIMER_ADD(WICO_firq_0, 60)

  // game & sound section
  MDRV_CPU_ADD_TAG("scpu", M6809, 1500000)
  MDRV_CPU_MEMORY(WICO_1_readmem, WICO_1_writemem)
  MDRV_CPU_PERIODIC_INT(WICO_irq_1, 120)
  MDRV_SOUND_ADD(SN76496, WICO_sn76494Int)
//  MDRV_TIMER_ADD(WICO_firq_1, 60)
MACHINE_DRIVER_END

INPUT_PORTS_START(aftor) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT(     0x0001, "Test 1", KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Test 2", KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Test 3", KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Test 4", KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Test 5", KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Test 6", KEYCODE_6) \
    COREPORT_BIT(     0x0040, "Test 7", KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Test 8", KEYCODE_8) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
INPUT_PORTS_END

ROM_START(aftor) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("u25.bin", 0xf000, 0x1000, CRC(d66e95ff) SHA1(f7e8c51f1b37e7ef560406f1968c12a2043646c5)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("u52.bin", 0x8000, 0x2000, CRC(8035b446) SHA1(3ec59015e259c315bf09f4e2046f9d98e2d7a732)) \
    ROM_LOAD("u48.bin", 0xe000, 0x2000, CRC(b4406563) SHA1(6d1a9086eb1f6f947eae3a92ccf7a9b7375d85d3)) \
ROM_END

static core_tLCDLayout dispAftor[] = {
  {0, 0, 0,16,CORE_SEG10},
  {2, 0,16,16,CORE_SEG10},
  {0}
};
static core_tGameData aftorGameData = {GEN_ALVG,dispAftor,{FLIP_SW(FLIP_L),0,2}};
static void init_aftor(void) { core_gameData = &aftorGameData; }

CORE_GAMEDEFNV(aftor,"Af-Tor",1984,"Wico",aftor,GAME_NOT_WORKING)
