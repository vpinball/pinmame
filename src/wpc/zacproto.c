/* Zaccaria Prototype Solid State machines */
/* CPU: National Semiconductor SC/MP ISP-8A 500D */

#include "driver.h"
#include "cpu/scamp/scamp.h"
#include "core.h"
#include "sim.h"

static struct {
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
} locals;

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids = locals.solenoids;
  coreGlobals.solenoids2 = locals.sols2;
  core_updateSw(1);
}

//Generate the IRQ
static INTERRUPT_GEN(irq) {
  logerror("%04x: IRQ\n",activecpu_get_pc());
  cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static int irq_callback(int int_level) {
  cpu_set_irq_line(0, 0, CLEAR_LINE);
  return 0;
}

static MACHINE_INIT(strike) {
  memset(&locals, 0, sizeof(locals));
  cpu_set_irq_callback(0, irq_callback);
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x0bff, MRA_ROM },
  { 0x0c00, 0x0fff, MRA_RAM },
  { 0x1000, 0x1400, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0c00, 0x0fff, MWA_RAM },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG7},
  {2, 0,16,16,CORE_SEG7},
  {0}
};
static core_tGameData strikeGameData = {GEN_ZAC1,disp};
static void init_strike(void) {
  core_gameData = &strikeGameData;
}

MACHINE_DRIVER_START(strike)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(strike,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", SCAMP, 200000)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
MACHINE_DRIVER_END

INPUT_PORTS_START(strike) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
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
      COREPORT_DIPSET(0x0080, "1" )
INPUT_PORTS_END

ROM_START(strike) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("strike1.bin", 0x0000, 0x0400, CRC(650abc54) SHA1(6a4f83016a38338ba6a04271532f0880264e61a7)) \
    ROM_LOAD("strike2.bin", 0x0400, 0x0400, CRC(13c5a168) SHA1(2da3a5bc0c28a2aacd8c1396dac95cf35f8797cd)) \
    ROM_LOAD("strike3.bin", 0x0800, 0x0400, CRC(ebbbf315) SHA1(c87e961c8e5e99b0672cd632c5e104ea52088b5d)) \
    ROM_LOAD("strike4.bin", 0x1000, 0x0400, CRC(ca0eddd0) SHA1(52f9faf791c56b68b1806e685d0479ea67aba019))
ROM_END

CORE_GAMEDEFNV(strike, "Strike", 1978, "Zaccaria", strike, GAME_USES_CHIMES)
