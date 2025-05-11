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
  memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids = locals.solenoids;
  coreGlobals.solenoids2 = locals.sols2;
  core_updateSw(0);
}

//Generate the IRQ
#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static INTERRUPT_GEN(irq) {
  logerror("%04x: IRQ\n",activecpu_get_previouspc());
  cpu_set_irq_line(0, 0, ASSERT_LINE);
}
#endif

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
    ROM_LOAD("bingo.u37", 0x1800, 0x0800, CRC(3b21b22c) SHA1(21b002dd0dd11ee55674955c67c627470f427591))
    ROM_LOAD("bingo.u40", 0x1000, 0x0800, CRC(67160fc8) SHA1(6b93c1a7edcd7079a1e7d8a926e72febe2b39e9e))
    ROM_LOAD("bingo.u44", 0x0800, 0x0800, CRC(068acc49) SHA1(34fa2977513276bd5adc0b06cf258bb5a3702ed2))
    ROM_LOAD("bingo.u48", 0x0000, 0x0800, CRC(81bbcb19) SHA1(17c3d900d1cbe3cb5332d830288ef2c578afe8f8))
ROM_END
#define input_ports_cntinntl input_ports_bingo
#define init_cntinntl init_bingo

CORE_GAMEDEFNV(cntinntl, "Continental (Bingo)", 1980, "Bally", bingo, GAME_NOT_WORKING)

ROM_START(cntintl2) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("u36.bin", 0x1800, 0x0800, CRC(205cca08) SHA1(ae21794a63f1c50e3c7239275f7a58caf701a7bc))
    ROM_LOAD("bingo.u40", 0x1000, 0x0800, CRC(67160fc8) SHA1(6b93c1a7edcd7079a1e7d8a926e72febe2b39e9e))
    ROM_LOAD("bingo.u44", 0x0800, 0x0800, CRC(068acc49) SHA1(34fa2977513276bd5adc0b06cf258bb5a3702ed2))
    ROM_LOAD("u48.bin", 0x0000, 0x0800, CRC(8fda0bf9) SHA1(ea2926bb2c1cc394a060d88cc6ef53b7cf39790b))
ROM_END
#define input_ports_cntintl2 input_ports_bingo
#define init_cntintl2 init_bingo

CORE_CLONEDEFNV(cntintl2, cntinntl, "Continental (Bingo, alternate version)", 1980, "Bally", bingo, GAME_NOT_WORKING)


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

#ifndef PINMAME_NO_UNUSED // currently unused function (GCC 3.4)
static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[1+offset];
}
#endif

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

ROM_START(goldgstake)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "h0127.evn", 0x80000, 0x10000, CRC(477ddee2) SHA1(a4d16b44ee43838120fbc1e4642867c3d375fe5f)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "h0127.ods", 0x80001, 0x10000, CRC(89aea35a) SHA1(65bb5c5448de05180d0fd5b593f783b860de5b7c)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_goldgstake input_ports_splin
#define init_goldgstake init_splin

ROM_START(goldgnew)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "h018.e", 0x80000, 0x10000, CRC(0b209318) SHA1(56e36d6672820f5610dfdcb6dc93c3aa92286992)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "h018.o", 0x80001, 0x10000, CRC(d7ec2522) SHA1(5c09be69db1338483b7b65193f29b5bea4fb6195)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_goldgnew input_ports_splin
#define init_goldgnew init_splin

ROM_START(goldgkit1)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "v87k.e", 0x80000, 0x10000, CRC(57f4f0c4) SHA1(7b8cb888a55a2aa46e7737a8dc44b2f983a189c0)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "v87k.o", 0x80001, 0x10000, CRC(6178d4e5) SHA1(6d4c8056324157c448963761d7f49ce80e42d912)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_goldgkit1 input_ports_splin
#define init_goldgkit1 init_splin

ROM_START(michkit1)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "ac001.e",   0x80000, 0x10000, CRC(c92af347) SHA1(7d46408b37a7f88232fc87b3289beb244a94390c)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "ac001.o",   0x80001, 0x10000, CRC(e321600a) SHA1(3e99a162a34d3ca28f6ca33ee442820bae8a1574)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_michkit1 input_ports_splin
#define init_michkit1 init_splin

ROM_START(michkitb)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "ac0127.evn",0x80000, 0x10000, CRC(cce266d0) SHA1(e45df0bb58758e727767965cc0edcee0f25ce97e)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "ac0127.ods",0x80001, 0x10000, CRC(1cde7f17) SHA1(dbfe9b94f768e24e58ec19d0152e98bfa2965b6f)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_michkitb input_ports_splin
#define init_michkitb init_splin

ROM_START(michstake)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "c0128.evn", 0x80000, 0x10000, CRC(68af141a) SHA1(0e94f70c2fd74bfc5829f19ba502e9b288432685)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "c0128.ods", 0x80001, 0x10000, CRC(ff2752f6) SHA1(bd6c45ac0a533aeb5930b5a1705152eec704b5e9)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_michstake input_ports_splin
#define init_michstake init_splin

ROM_START(michnew)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "c017.e",    0x80000, 0x10000, CRC(8a1cf5d7) SHA1(aad87d13503753b13dbdae2a2792afa004747e6e)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "c017.o",    0x80001, 0x10000, CRC(0bfdff4d) SHA1(5dacaa689ae7eaba5f28d88a8d9f3d6de0421744)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_michnew input_ports_splin
#define init_michnew init_splin

ROM_START(michigan)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "c9925_1.e", 0x80000, 0x10000, CRC(ab42e3b8) SHA1(917b5a7a005baf6bae676e54a0292e32d11a7df1)) \
      ROM_RELOAD(0xe0000, 0x10000) \
    ROM_LOAD16_BYTE( "c9925_1.o", 0x80001, 0x10000, CRC(7a0d6c70) SHA1(1d410b9f5df69cc9cbf17dbc9c73fee928e167d7)) \
      ROM_RELOAD(0xe0001, 0x10000) \
ROM_END
#define input_ports_michigan input_ports_splin
#define init_michigan init_splin

ROM_START(montana)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD16_BYTE( "m0128.e", 0x80000, 0x20000, CRC(51a56929) SHA1(4a1d9939ff441f82661e1adcb0d698061f383429)) \
      ROM_RELOAD(0xc0000, 0x20000) \
    ROM_LOAD16_BYTE( "m0128.o", 0x80001, 0x20000, CRC(03431945) SHA1(da441895f3f6db9e573fcb5de8e287e65cc9a00d)) \
      ROM_RELOAD(0xc0001, 0x20000) \
ROM_END
#define input_ports_montana input_ports_splin
#define init_montana init_splin

ROM_START(topgame)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD( "v252l10.p0", 0x0000, 0x8000, CRC(d3f71f05) SHA1(2bbafeee2e5eda6ff4ed8c7d52f2bb33c50f398c)) \
    ROM_LOAD( "v252l10.p1", 0x8000, 0x8000, CRC(f98531d1) SHA1(63ae30821788dccbaa0749db55cd85f5a7a609bf)) \
ROM_END
#define input_ports_topgame input_ports_splin
#define init_topgame init_splin

ROM_START(topgamet)
  NORMALREGION(0x100000, REGION_CPU1) \
    ROM_LOAD( "v252tr.p0", 0x0000, 0x8000, CRC(c947cccc) SHA1(37837ec030b2e86109d40fc19c24fc6aa73a272c)) \
    ROM_LOAD( "v252tr.p1", 0x8000, 0x8000, CRC(00a3ee14) SHA1(5ebf2d0ea891e365f5bd1cc03f0bd913a638b49b)) \
ROM_END
#define input_ports_topgamet input_ports_splin
#define init_topgamet init_splin


CORE_GAMEDEFNV (goldgame,             "Golden Game (Bingo)",                      19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(goldgam2,   goldgame, "Golden Game Kit Bingo Stake 6/10 (Bingo)", 19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(goldgstake, goldgame, "Golden Game Bingo Stake 6/10 (Bingo)",     19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(goldgnew,   goldgame, "Golden Game Bingo New (Bingo)",            19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(goldgkit1,  goldgame, "Golden Game Kit 1 Generation (Bingo)",     19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)

CORE_GAMEDEFNV (michigan,             "Michigan (Bingo)",                         19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(michkit1,   michigan, "Michigan Bingo Kit 1 Generation (Bingo)",  19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(michkitb,   michigan, "Michigan Kit Bingo Stake 6/10 (Bingo)",    19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(michstake,  michigan, "Michigan Bingo Stake 6/10 (Bingo)",        19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(michnew,    michigan, "Michigan Bingo New (Bingo)",               19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)

CORE_GAMEDEFNV (montana,              "Montana Bingo Stake 6/10 (Bingo)",         19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)

CORE_GAMEDEFNV (topgame,              "Top Game Laser L10 (Bingo)",               19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)
CORE_CLONEDEFNV(topgamet,   topgame,  "Top Game Turbo (Bingo)",                   19??, "Splin (Belgium)", splin, GAME_NOT_WORKING)


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

ROM_START(brooklyna)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("o10307-1.epr", 0x8000, 0x8000, CRC(628ac640) SHA1(67edb424f15880e874b03028066e6c0039db21fa)) \
    ROM_LOAD("o10307-2.epr", 0x0000, 0x4000, CRC(c35d83ff) SHA1(e37c03e6960138cb6b628dfc6b12e484bbae96e8)) \
ROM_END
#define input_ports_brooklyna input_ports_seeben
#define init_brooklyna init_seeben

ROM_START(newdixie) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("10307-1.epr", 0x8000, 0x8000, CRC(7b6b2e9c) SHA1(149c9e1d2a3e7db735835c6fa795e41b2eb45175)) \
    ROM_LOAD("10307-2.epr", 0x0000, 0x4000, CRC(d99a7866) SHA1(659a0107bc970d2578dcfd7cdd43661da778fd5c)) \
ROM_END
#define input_ports_newdixie input_ports_seeben
#define init_newdixie init_seeben

ROM_START(superdix)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD( "12906-1.epr", 0x8000, 0x8000, CRC(e90a8aa5) SHA1(88dac74fb020535b535f7c4c245bbece398164ee)) \
    ROM_LOAD( "12906-2.epr", 0x0000, 0x4000, CRC(4875dfb4) SHA1(722bfa89f69d14e24555eea9cc975012098db25b)) \
ROM_END
#define input_ports_superdix input_ports_seeben
#define init_superdix init_seeben

ROM_START(cntine31)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("10107-1.epr", 0x8000, 0x8000, CRC(3b67cce3) SHA1(95f71526c236262ff985148ba7ea057f07fadbe8)) \
    ROM_LOAD("10107-2.epr", 0x0000, 0x4000, CRC(89d08795) SHA1(dc75502580d681d9b4dc878b0d80346bcef407ae)) \
ROM_END
#define input_ports_cntine31 input_ports_seeben
#define init_cntine31 init_seeben

ROM_START(domino2)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("13006.epr", 0x8000, 0x8000, CRC(8ed9b2a5) SHA1(8f3e730cef3e74cb043691a111e1bf6660642a73)) \
ROM_END
#define input_ports_domino2 input_ports_seeben
#define init_domino2 init_seeben

ROM_START(ggate)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD( "13006-1.epr", 0x8000, 0x8000, CRC(6a451fc6) SHA1(93287937c8a679dfca1a162977a62357134673b6)) \
    ROM_LOAD( "13006-2.epr", 0x0000, 0x4000, CRC(217299b0) SHA1(ef3ee8811183dca43699a7c2d75fb99bc3668ae2)) \
ROM_END
#define input_ports_ggate input_ports_seeben
#define init_ggate init_seeben

ROM_START(ggatea)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD( "12906-1.epr", 0x8000, 0x8000, CRC(3792fc4c) SHA1(4ab88b6c73ce1b49e1a4d12cc9fa61c7d74ed780)) \
    ROM_LOAD( "12906-2.epr", 0x0000, 0x4000, CRC(a1115196) SHA1(dfa549a547b5cd7a9369d30fa1e868e6725cb3f1)) \
ROM_END
#define input_ports_ggatea input_ports_seeben
#define init_ggatea init_seeben

ROM_START(tripjok)
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD( "13006-1", 0x8000, 0x8000, CRC(5682ac90) SHA1(c9fa13c56e9178eb861991fcad6b09fd27cca3cb)) \
    ROM_LOAD( "13006-2", 0x0000, 0x4000, CRC(c7104e8f) SHA1(a3737f70cb9c97df24b5da915ef53b6d30f2470d)) \
ROM_END
#define input_ports_tripjok input_ports_seeben
#define init_tripjok init_seeben

CORE_GAMEDEFNV (penalty,           "Penalty (Bingo)",            19??, "Seeben (Belgium)", seeben, GAME_NOT_WORKING)

CORE_GAMEDEFNV (brooklyn,          "Brooklyn (set 1) (Bingo)",   19??, "Seeben (Belgium)", seeben, GAME_NOT_WORKING)
CORE_CLONEDEFNV(brooklyna,brooklyn,"Brooklyn (set 2) (Bingo)",   19??, "Seeben (Belgium)", seeben, GAME_NOT_WORKING)

CORE_GAMEDEFNV (newdixie,          "New Dixieland (Bingo)",      19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
CORE_GAMEDEFNV (superdix,          "Super Dixieland (Bingo)",    19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
CORE_GAMEDEFNV (cntine31,          "Continental 3 in 1 (Bingo)", 19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
CORE_GAMEDEFNV (domino2,           "Domino II (Bingo)",          19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
CORE_GAMEDEFNV (tripjok,           "Triple Joker (Bingo)",       19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)

CORE_GAMEDEFNV (ggate,             "Golden Gate (set 1) (Bingo)",19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
CORE_CLONEDEFNV(ggatea,ggate,      "Golden Gate (set 2) (Bingo)",19??, "Sirmo (Belgium)", seeben, GAME_NOT_WORKING)
