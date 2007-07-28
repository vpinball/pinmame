/* Bally Continental Bingo */
/* CPU: Signetics 2650 (32K Addressable CPU Space) */

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/pic8259.h"
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

ROM_START(cntinntl) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("bingo.u37", 0x1800, 0x0800, CRC(3b21b22c) SHA1(21b002dd0dd11ee55674955c67c627470f427591)) \
    ROM_LOAD("bingo.u40", 0x1000, 0x0800, CRC(67160fc8) SHA1(6b93c1a7edcd7079a1e7d8a926e72febe2b39e9e)) \
    ROM_LOAD("bingo.u44", 0x0800, 0x0800, CRC(068acc49) SHA1(34fa2977513276bd5adc0b06cf258bb5a3702ed2)) \
    ROM_LOAD("bingo.u48", 0x0000, 0x0800, CRC(81bbcb19) SHA1(17c3d900d1cbe3cb5332d830288ef2c578afe8f8))
ROM_END
#define input_ports_cntinntl input_ports_bingo
#define init_cntinntl init_bingo

CORE_GAMEDEFNV(cntinntl, "Continental (Bingo)", 1980, "Bally", bingo, GAME_NOT_WORKING)


// Splin Bingo (Belgium)

static MACHINE_INIT(splin) {
  memset(&locals, 0, sizeof(locals));
}

static SWITCH_UPDATE(splin) {
  if (keyboard_pressed_memory_repeat(KEYCODE_0, 0)) {
    cpu_set_nmi_line(0, HOLD_LINE);
    //pic8259_0_issue_irq(0);
  } else {
    cpu_set_nmi_line(0, CLEAR_LINE);
  }
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[1+offset];
}

static MEMORY_READ_START(readmem16)
  { 0x00000, 0x0bfff, MRA_RAM },
  { 0x0d900, 0x0d9ff, MRA_RAM },
  { 0xe0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem16)
  { 0x00000, 0x0bfff, MWA_RAM },
  { 0x0d900, 0x0d9ff, MWA_RAM },
  { 0xe0000, 0xfffff, MWA_ROM },
MEMORY_END

static PORT_WRITE_START(writeport16)
//  { 0x0400, 0x0401, pic8259_0_w },
PORT_END

static PORT_READ_START(readport16)
//  { 0x0400, 0x0401, pic8259_0_r },
PORT_END

static core_tLCDLayout dispSplin[] = {
  {0, 0, 0,16,CORE_SEG7},
  {2, 0,16,16,CORE_SEG7},
  {0}
};
static core_tGameData splinGameData = {GEN_ZAC1,dispSplin};
static void init_splin(void) {
  core_gameData = &splinGameData;
  pic8259_0_config(0, 0);
}

MACHINE_DRIVER_START(splin)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(splin,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", I186, 8000000)
  MDRV_CPU_MEMORY(readmem16, writemem16)
  MDRV_CPU_PORTS(readport16, writeport16)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(splin)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

INPUT_PORTS_START(splin) \
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

ROM_START(goldgame) \
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE("h9925_1.e", 0x80000, 0x10000, CRC(c5ec9181) SHA1(fac7fc0fbfddca44c728c78973ee5273a3d0bc43)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE("h9925_1.o", 0x80001, 0x10000, CRC(2a019eea) SHA1(3f013f97b0a92fc9085c7be3903cbf42e67c41e5)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_goldgame input_ports_splin
#define init_goldgame init_splin

ROM_START(goldgam2) \
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE("ah0127.evn", 0x80000, 0x10000, CRC(6456a021) SHA1(98137d3b63aa7453c624f477a0c6ea1e0996d3c2)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE("ah0127.ods", 0x80001, 0x10000, CRC(b538f435) SHA1(4d939554e997d630ffe7337e1f21ee53d6f06130)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_goldgam2 input_ports_splin
#define init_goldgam2 init_splin

CORE_GAMEDEFNV (goldgame,           "Golden Game (Bingo)",            19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(goldgam2, goldgame, "Golden Game Stake 6/10 (Bingo)", 19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)


// Seeben / Sirmo (Belgium)

static MACHINE_INIT(seeben) {
  memset(&locals, 0, sizeof(locals));
}

static SWITCH_UPDATE(seeben) {
  if (keyboard_pressed_memory_repeat(KEYCODE_0, 0)) {
    cpu_set_nmi_line(0, HOLD_LINE);
  } else {
    cpu_set_nmi_line(0, CLEAR_LINE);
  }
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static MEMORY_READ_START(readmems)
MEMORY_END

static MEMORY_WRITE_START(writemems)
MEMORY_END

static core_tLCDLayout dispSeeben[] = {
  {0, 0, 0,16,CORE_SEG7},
  {2, 0,16,16,CORE_SEG7},
  {0}
};
static core_tGameData seebenGameData = {GEN_ZAC1,dispSeeben};
static void init_seeben(void) {
  core_gameData = &seebenGameData;
}

MACHINE_DRIVER_START(seeben)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(seeben,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", 8085A, 1000000)
  MDRV_CPU_MEMORY(readmems, writemems)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(seeben)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

INPUT_PORTS_START(seeben) \
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

ROM_START(penalty) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("13006-1.epr", 0x8000, 0x8000, CRC(93cfbec9) SHA1(c245604ac42c88c647950db4497a6f9dd3504955)) \
    ROM_LOAD("13006-2.epr", 0x0000, 0x4000, CRC(41470cc1) SHA1(7050df563fddbe8216317d96664d12567b618645)) \
ROM_END
#define input_ports_penalty input_ports_seeben
#define init_penalty init_seeben

ROM_START(brooklyn) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("n10207-1.epr", 0x8000, 0x8000, CRC(7851f870) SHA1(8da400108a352954ced8fc942663c0635bec4d1c)) \
    ROM_LOAD("n10207-2.epr", 0x0000, 0x4000, CRC(861dae09) SHA1(d808fbbf6b50e1482a512b9bd1b18a023694adb2)) \
ROM_END
#define input_ports_brooklyn input_ports_seeben
#define init_brooklyn init_seeben

ROM_START(newdixie) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("10307-1.epr", 0x8000, 0x8000, CRC(7b6b2e9c) SHA1(149c9e1d2a3e7db735835c6fa795e41b2eb45175)) \
    ROM_LOAD("10307-2.epr", 0x0000, 0x4000, CRC(d99a7866) SHA1(659a0107bc970d2578dcfd7cdd43661da778fd5c)) \
ROM_END
#define input_ports_newdixie input_ports_seeben
#define init_newdixie init_seeben

CORE_GAMEDEFNV (penalty,  "Penalty (Bingo)",  19??, "Seeben (Belgium)", seeben, GAME_NOT_WORKING)
CORE_GAMEDEFNV (brooklyn, "Brooklyn (Bingo)", 19??, "Seeben (Belgium)", seeben, GAME_NOT_WORKING)

CORE_GAMEDEFNV (newdixie, "New Dixieland (Bingo)", 19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
