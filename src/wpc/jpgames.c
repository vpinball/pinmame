#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "jp.h"
#include "sndbrd.h"

#define GEN_JP 0

#define INITGAME(name, disptype, balls) \
	JP_INPUT_PORTS_START(name, balls) JP_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_JP,disptype,{FLIP_SW(FLIP_L), 0,8,0, SNDBRD_SPINB}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout jpDisp6[] = {
  {0, 0, 0, 6,CORE_SEG7}, {3, 0, 6, 6,CORE_SEG7},
  {0,22,12, 6,CORE_SEG7}, {3,22,18, 6,CORE_SEG7},
  {2,17,24, 2,CORE_SEG7S},{2,23,26, 1,CORE_SEG7S},{2,26,27, 1,CORE_SEG7S},
  {0}
};

static core_tLCDLayout jpDisp7[] = {
  {0, 0, 0, 7,CORE_SEG87}, {3, 0, 7, 7,CORE_SEG87},
  {0,24,14, 7,CORE_SEG87}, {3,24,21, 7,CORE_SEG87},
  {1,20,28, 2,CORE_SEG7S},{1,26,30, 1,CORE_SEG7S},{1,29,31, 1,CORE_SEG7S},
  {3,18,32, 1,CORE_SEG8},
  {0}
};

/*-------------------------------------------------------------------
/ Petaco
/-------------------------------------------------------------------*/
INPUT_PORTS_START(petaco) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(  0x8000, IPT_START1,         IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0008, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BIT(     0x0400, "Pendulum Tilt",    KEYCODE_INSERT) \
    COREPORT_BIT(     0x4000, "Slam Tilt",        KEYCODE_HOME) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, DEF_STR(Unknown)) \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "Display played games") \
      COREPORT_DIPSET(0x0000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0002, DEF_STR(Yes)) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Display inserted coins") \
      COREPORT_DIPSET(0x0000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0004, DEF_STR(Yes)) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Display running hours") \
      COREPORT_DIPSET(0x0000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0008, DEF_STR(Yes)) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Match feature") \
      COREPORT_DIPSET(0x0010, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0000, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Attract tune") \
      COREPORT_DIPSET(0x0020, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0000, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0300, 0x0200, "Free games") \
      COREPORT_DIPSET(0x0000, "Low" ) \
      COREPORT_DIPSET(0x0100, "Medium" ) \
      COREPORT_DIPSET(0x0200, "High" ) \
      COREPORT_DIPSET(0x0300, "Maximum" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "Balls per game") \
      COREPORT_DIPSET(0x0400, "3" ) \
      COREPORT_DIPSET(0x0000, "5" ) \
    COREPORT_DIPNAME( 0x1800, 0x1000, "Games per coin") \
      COREPORT_DIPSET(0x0000, "Setting #1" ) \
      COREPORT_DIPSET(0x0800, "Setting #2" ) \
      COREPORT_DIPSET(0x1000, "Setting #3" ) \
      COREPORT_DIPSET(0x1800, "Setting #4" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Extra balls") \
      COREPORT_DIPSET(0x2000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0000, DEF_STR(Yes)) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
INPUT_PORTS_END
static core_tGameData petacoGameData = {GEN_JP,jpDisp6,{FLIP_SW(FLIP_L), 0,1,0, SNDBRD_NONE}}; \
static void init_petaco(void) { \
  core_gameData = &petacoGameData; \
}
JP_ROMSTART2(petaco,	"petaco1.cpu", CRC(f4e09939) SHA1(dcc4220b269d271eb0b6ad0a5d3c1a240587a01b),
						"petaco2.cpu", CRC(d29a59ea) SHA1(bb7891e9597bbf5ae6a3276abf2b1247e082d828))
JP_ROMEND
CORE_GAMEDEFNV(petaco,"Petaco",1984,"Juegos Populares",gl_mJP2,0)

/*-------------------------------------------------------------------
/ Faeton
/-------------------------------------------------------------------*/
INITGAME(faeton, jpDisp7, 1)
JP_ROMSTART1(faeton,	"faeton.cpu", CRC(ef7e6915) SHA1(5d3d86549606b3d9134bb3f6d3026d6f3e07d4cd))
JP_ROMEND
CORE_GAMEDEFNV(faeton,"Faeton",1985,"Juegos Populares",gl_mJP,0)

/*-------------------------------------------------------------------
/ Petaco 2
/-------------------------------------------------------------------*/
INITGAME(petaco2, jpDisp7, 1)
JP_ROMSTART1(petaco2,	"petaco2.dat", NO_DUMP)
JP_SNDROM8( "jpsonid0.dat", CRC(1bdbdd60) SHA1(903012e58cdb4041e5546a377f5c9df83dc93737),
			"jpsonid1.dat", CRC(e39da92a) SHA1(79eb60710bdf6b826349e02ae909426cb81e131e),
			"jpsonid2.dat", CRC(88456f1e) SHA1(168fe88ae9da5114d0ef6427df0503ca2eea9089),
			"jpsonid3.dat", CRC(c7597d29) SHA1(45abe1b28ad14610ac8e2bc3a70af46bbe6277f4),
			"jpsonid4.dat", CRC(0d29a028) SHA1(636cc9a1f6128c820b18db4bf764e0be10a46119),
			"jpsonid5.dat", CRC(76790393) SHA1(23df394ecd11205d83073dca160f8f9a98aaa169),
			"jpsonid6.dat", CRC(53c3f0b4) SHA1(dcf4c63636e2b7ff5cd2db99d949db9e33b78fc7),
			"jpsonid7.dat", CRC(ff430b1b) SHA1(423592a40eba174108dfc6817e549c643bb3c80f))
JP_ROMEND
CORE_GAMEDEFNV(petaco2,"Petaco 2",1985,"Juegos Populares",gl_mJPS,0)

/*-------------------------------------------------------------------
/ America 1492
/-------------------------------------------------------------------*/
INITGAME(america, jpDisp7, 1)
JP_ROMSTART1(america,	"cpvi1492.dat", CRC(e1d3bd57) SHA1(049c17cd717404e58339100ab8efd4d6bf8ee791))
JP_SNDROM8( "sbvi1492.dat", CRC(38934e06) SHA1(eef850a5096a7436b728921aed22fe5f3d85b4ee),
			"b1vi1492.dat", CRC(e93083ed) SHA1(6a44675d8cc8b8af40091646f589b833245bf092),
			"b2vi1492.dat", CRC(88be85a0) SHA1(ebf9d88847d6fd787892f0a34258f38e48445014),
			"b3vi1492.dat", CRC(1304c87b) SHA1(f84eb3116dd9841892f46106f9443c09cc094675),
			"b4vi1492.dat", CRC(831e4033) SHA1(f51f3f5a226692caed59e4aac0843cdb40f0667d),
			"b5vi1492.dat", CRC(46ee29a5) SHA1(08d756f5a0430aca723f842951dd8520024859b0),
			"b6vi1492.dat", CRC(5180d751) SHA1(6c2e8edf606d24d86f4ab6da4adaf1d1095e9b19),
			"b7vi1492.dat", CRC(ba98138f) SHA1(2c8ef3b17972b7022afdf89c6280d02038b65501))
JP_ROMEND
CORE_GAMEDEFNV(america,"America 1492",1986,"Juegos Populares",gl_mJPS,0)

/*-------------------------------------------------------------------
/ Aqualand
/-------------------------------------------------------------------*/
INITGAME(aqualand, jpDisp7, 1)
JP_ROMSTART1(aqualand, "jpaqcpu", CRC(53230fab) SHA1(0b049f3be412be598982537e7fa7abf9b2766a16))
JP_SNDROM15( "jpaqsds", CRC(ff1e0cd2) SHA1(ef58d2b59929c7250dd30c413a3ba31ebfd7e09d),
			"jpaq-1sd", CRC(7cdf2f7a) SHA1(e00482a6accd11e96fd0d444b3167b7d36332f7b),
			"jpaq-2sd", CRC(db05c774) SHA1(2d40410b70de6ab0de57e94c6d8ada6e8a4a2050),
			"jpaq-3sd", CRC(df38304e) SHA1(ec6f0c99764e3c3fe7e1de09b2d9b59d85d168d5),
			"jpaq-4sd", CRC(8065c03e) SHA1(0731cb76d3be117a82c4ad5b7e23b53e05b3a95a),
			"jpaq-5sd", CRC(a387a1a6) SHA1(20abee033a33e388a5f2ed3896a650766b62cfa2),
			"jpaq-6sd", CRC(55076afb) SHA1(68b86e6855b2a80e37d2fb172bb0c4fa107d4aba),
			"jpaq-7sd", CRC(67675b5b) SHA1(52b7cb310ddeff0bde7f0dfd37f61ab09964a75d),
			"jpaq-8sd", CRC(c9d2d30e) SHA1(ee504b0e2aa69f541c3f4d245cc6525a7c920fa7),
			"jpaq-9sd", CRC(3bc45f9f) SHA1(6d838b1ba94087f9a29af016b68125400dcf1fe5),
			"jpaq10sd", CRC(239cb7f3) SHA1(1abc59bc73cf84ee3b73d500bf57a2a202291fcb),
			"jpaq11sd", CRC(e5b9e70f) SHA1(7db0a13166120fe20bb76072475b092e942629cf),
			"jpaq12sd", CRC(9aa37260) SHA1(6eec14f0d7152bf0cfadabe5b3017b9b6b7aa2d3),
			"jpaq13sd", CRC(5599792e) SHA1(9d844d9f155f299bbe2d512f8ed84410e7a9cfb3),
			"jpaq14sd", CRC(0bdcbbbd) SHA1(555d8ed846079894cfc60041fb724deeaddc4e89))
JP_ROMEND
CORE_GAMEDEFNV(aqualand,"Aqualand",1986,"Juegos Populares",gl_mJPS,0)

/*-------------------------------------------------------------------
/ Lortium
/-------------------------------------------------------------------*/
INITGAME(lortium, jpDisp7, 1)
JP_ROMSTART2(lortium,	"cpulort1.dat", NO_DUMP,
						"cpulort2.dat", CRC(71eebb26) SHA1(9d49c1012555bda24ac7287499bcb93828cbb57f))
JP_ROMEND
CORE_GAMEDEFNV(lortium,"Lortium",1987,"Juegos Populares",gl_mJP,0)
