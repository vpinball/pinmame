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

static core_tLCDLayout jpDisp7[] = {
  {0, 0, 0, 7,CORE_SEG87}, {0,22, 7, 7,CORE_SEG87},
  {3, 0,14, 7,CORE_SEG87}, {3,22,21, 7,CORE_SEG87},
  {1,24,28, 2,CORE_SEG7S},{1,29,30, 2,CORE_SEG7S},
  {3,17,32, 1,CORE_SEG8},
  {0}
};

/*-------------------------------------------------------------------
/ Lortium
/-------------------------------------------------------------------*/
INITGAME(lortium, jpDisp7, 1)
JP_ROMSTART2(lortium,	"cpulort1.dat", NO_DUMP,
						"cpulort2.dat", CRC(71eebb26) SHA1(9d49c1012555bda24ac7287499bcb93828cbb57f))
JP_ROMEND
CORE_GAMEDEFNV(lortium,"Lortium",1987,"Juegos Populares",gl_mJP,GAME_NO_SOUND)

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
CORE_GAMEDEFNV(petaco2,"Petaco 2",198?,"Juegos Populares",gl_mJPS,0)

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
CORE_GAMEDEFNV(america,"America 1492",198?,"Juegos Populares",gl_mJPS,0)
