/* Bally Continental Bingo */
/* CPU: Signetics 2650 (32K Addressable CPU Space) */

#include "driver.h"
#include "cpu/s2650/s2650.h"
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
  core_updateSw(0);
}

//Generate the IRQ
static INTERRUPT_GEN(irq) {
  logerror("%04x: IRQ\n",activecpu_get_previouspc());
  cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static int irq_callback(int int_level) {
  cpu_set_irq_line(0, 0, CLEAR_LINE);
  return 0;
}

static MACHINE_INIT(bingo) {
  memset(&locals, 0, sizeof(locals));
  cpu_set_irq_callback(0, irq_callback);
}

static READ_HANDLER(ctrl_port_r)
{
  logerror("%04x: Ctrl Port Read\n",activecpu_get_previouspc());
  return 0;
}

static READ_HANDLER(data_port_r)
{
  logerror("%04x: Data Port Read\n",activecpu_get_previouspc());
  return 0;
}

static READ_HANDLER(sense_port_r)
{
  logerror("%04x: Sense Port Read\n",activecpu_get_previouspc());
  return 0;
}

static WRITE_HANDLER(ctrl_port_w)
{
  logerror("%04x: Ctrl Port Write=%02x\n",activecpu_get_previouspc(),data);
}

static WRITE_HANDLER(data_port_w)
{
  logerror("%04x: Data Port Write=%02x\n",activecpu_get_previouspc(),data);
}

static WRITE_HANDLER(flag_port_w)
{
  coreGlobals.diagnosticLed = data;
//logerror("%04x: Flag Port Write=%02x\n",activecpu_get_previouspc(),data);
}

struct DACinterface bingo_dacInt =
{
  1,	/* 1 Chip */
  {25}	/* Volume */
};

static WRITE_HANDLER(c_port_w)
{
  DAC_data_w(0, data);
//logerror("%04x: C Port Write=%02x\n",activecpu_get_previouspc(),data);
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x1eff, MRA_ROM },
  { 0x1f00, 0x1fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x1f00, 0x1fff, MWA_RAM },
MEMORY_END

static PORT_WRITE_START( writeport )
  { S2650_CTRL_PORT,  S2650_CTRL_PORT,  ctrl_port_w },
  { S2650_DATA_PORT,  S2650_DATA_PORT,  data_port_w },
  { S2650_SENSE_PORT, S2650_SENSE_PORT, flag_port_w },
  { 0x0c, 0x0c, c_port_w },
PORT_END

static PORT_READ_START( readport )
  { S2650_CTRL_PORT,  S2650_CTRL_PORT,  ctrl_port_r },
  { S2650_DATA_PORT,  S2650_DATA_PORT,  data_port_r },
  { S2650_SENSE_PORT, S2650_SENSE_PORT, sense_port_r },
PORT_END

static core_tLCDLayout dispBingo[] = {
  {0, 0, 0,16,CORE_SEG7},
  {2, 0,16,16,CORE_SEG7},
  {0}
};
static core_tGameData bingoGameData = {GEN_ZAC1,dispBingo};
static void init_bingo(void) {
  core_gameData = &bingoGameData;
}

MACHINE_DRIVER_START(bingo)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(bingo,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 1000000)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_PORTS(readport, writeport)
//MDRV_CPU_PERIODIC_INT(irq, 3906)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_DIAGNOSTIC_LEDH(1)

  MDRV_SOUND_ADD(DAC, bingo_dacInt)
MACHINE_DRIVER_END

INPUT_PORTS_START(bingo) \
  CORE_PORTS \
  SIM_PORTS(5) \
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

ROM_START(bingo) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("bingo.u37", 0x1800, 0x0800, CRC(3b21b22c) SHA1(21b002dd0dd11ee55674955c67c627470f427591)) \
    ROM_LOAD("bingo.u40", 0x1000, 0x0800, CRC(67160fc8) SHA1(6b93c1a7edcd7079a1e7d8a926e72febe2b39e9e)) \
    ROM_LOAD("bingo.u44", 0x0800, 0x0800, CRC(068acc49) SHA1(34fa2977513276bd5adc0b06cf258bb5a3702ed2)) \
    ROM_LOAD("bingo.u48", 0x0000, 0x0800, CRC(81bbcb19) SHA1(17c3d900d1cbe3cb5332d830288ef2c578afe8f8))
ROM_END

CORE_GAMEDEFNV(bingo,"Continental Bingo",1980,"Bally",bingo,GAME_NOT_WORKING)
