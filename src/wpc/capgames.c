#include "driver.h"
#include "sim.h"
#include "capcom.h"

#define INITGAME(name, balls) \
	CC_INPUT_PORTS_START(name, balls) CC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/*-------------------------------------------------------------------
/ Goofy Hoops (Romstar game) (??/95)
/-------------------------------------------------------------------*/
INITGAME(ghv101, 3)
CC_ROMSTART_4(ghv101,  "u06_v11.bin", CRC(3b6ab802) SHA1(8c43234ce2af3ba7dc8ab2cd6e2e352b5caa9acc),
                       "u23_v11.bin", CRC(f6cac3aa) SHA1(eb2018f21fdfb27b1e5ca83a09202614fd865b05),
                       "u13_v10.bin", CRC(1712f21f) SHA1(7c0c9d8c28c1c4888f0e6220f3a23d0eb25e218f),
                       "u17_v10.bin", CRC(b6a39327) SHA1(d05eaa767422f19dcf5037ffabdfb3ff55f76c32))
CC_ROMEND
CORE_GAMEDEFNV(ghv101,"Goofy Hoops",1995,"Romstar",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Pinball Magic (10/95)
/-------------------------------------------------------------------*/
INITGAME(pmv112, 3)
CC_ROMSTART_4(pmv112,  "u1l_v112.bin",CRC(c8362623) SHA1(ebe37d3273e5cefd4fbc041ea3b15d59010b8160),
                       "u1h_v112.bin",CRC(f6232c74) SHA1(28bab61de2ece27aff4cbdd36b10c136a4b7c936),
                       "u2l_v10.bin", CRC(d3e4241d) SHA1(fe480ea2b3901e2e571f8871a0ebe63fbf152e28),
                       "u2h_v10.bin", CRC(9276fd62) SHA1(b80e6186a6a2ded21bd1d6dbd306590645a50523))
CC_ROMEND
CORE_GAMEDEFNV(pmv112,"Pinball Magic",1995,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

INITGAME(pmv112r, 3)
CC_ROMSTART_4(pmv112r, "u1lv112i.bin",CRC(28d35969) SHA1(e19856402855847286db73c17510614d8b40c882),
                       "u1hv112i.bin",CRC(f70da65c) SHA1(0f98c95edd6f2821e3a67ff1805aa752a4d018c0),
                       "u2l_v10.bin", CRC(d3e4241d) SHA1(fe480ea2b3901e2e571f8871a0ebe63fbf152e28),
                       "u2h_v10.bin", CRC(9276fd62) SHA1(b80e6186a6a2ded21bd1d6dbd306590645a50523))
CC_ROMEND
CORE_CLONEDEFNV(pmv112r,pmv112,"Pinball Magic (Redemption)",1995,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Airborne (03/96)
/-------------------------------------------------------------------*/
INITGAME(abv106, 3)
CC_ROMSTART_4(abv106,  "u1l_v16.bin", CRC(59b258f1) SHA1(764496114609d65648e1c7b12409ec582037d8df),
                       "u1h_v16.bin", CRC(a4571905) SHA1(62fabc45e81c49125c047c6e5d268d4093b860bc),
                       "u2l_v10.bin", CRC(a15b1ec0) SHA1(673a283ddf670109a9728fefac2bcf493d70f23d),
                       "u2h_v10.bin", CRC(c22e3338) SHA1(1a25c85a1ed59647c40f9a4d417d78cccff7e51c))
CC_ROMEND
CORE_GAMEDEFNV(abv106,"Airborne",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

INITGAME(abv106r, 3)
CC_ROMSTART_4(abv106r, "u1l_v16i.bin",CRC(7d7d2d85) SHA1(5c83022d7c0b61b15455942b3bdd0cf89fc75b57),
                       "u1h_v16i.bin",CRC(b9bc0c5a) SHA1(e6fc393b970a2c354e0b0150dafbbbea2a85b92d),
                       "u2l_v10.bin", CRC(a15b1ec0) SHA1(673a283ddf670109a9728fefac2bcf493d70f23d),
                       "u2h_v10.bin", CRC(c22e3338) SHA1(1a25c85a1ed59647c40f9a4d417d78cccff7e51c))
CC_ROMEND
CORE_CLONEDEFNV(abv106r,abv106,"Airborne (Redemption)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Breakshot (05/96)
/-------------------------------------------------------------------*/
INITGAME(bsv103, 3)
CC_ROMSTART_2(bsv103,  "u1l_v13.bin", CRC(f8932dcc) SHA1(dab34e6c412655c60abeedc1f62254dce5ebb202),
                       "u1h_v13.bin", CRC(508c145d) SHA1(b019d445f87bca203646c616fdc295066da90921))
CC_ROMEND
CORE_GAMEDEFNV(bsv103,"Breakshot (1.3)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

INITGAME(bsv100r, 3)
CC_ROMSTART_2(bsv100r, "u1l_v10i.bin",CRC(304b4da8) SHA1(2643f304adce3543b792bd2d0ec8abe8d9a5478c),
                       "u1h_v10i.bin",CRC(c10b2aff) SHA1(a56af0903c8b8282baf63dc47741ef094cfd6a1c))
CC_ROMEND
CORE_CLONEDEFNV(bsv100r,bsv103,"Breakshot (Redemption 1.0)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

INITGAME(bsv102r, 3)
CC_ROMSTART_2(bsv102r, "u1l_v12i.bin",CRC(ed09e463) SHA1(74b4e3e93648e05e66a20f895133f1a0ba2ecb20),
                       "u1h_v12i.bin",CRC(71bf99e9) SHA1(cb48eb5c5df6b03022d9cb20c84dfdcc34a7d5ac))
CC_ROMEND
CORE_CLONEDEFNV(bsv102r,bsv103,"Breakshot (Redemption 1.2)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

INITGAME(bsb105, 3)
CC_ROMSTART_2(bsb105,  "bsu1l_b1.05", CRC(053684c7) SHA1(cf104a6e9c245523e29989389654c12437d32776),
                       "bsu1h_b1.05", CRC(f1dc6db8) SHA1(da209872047a1deac88fe389bcc26bcf353f6df8))
CC_ROMEND
CORE_CLONEDEFNV(bsb105,bsv103,"Breakshot (Beta 1.5)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Flipper Football (10/96)
/-------------------------------------------------------------------*/
INITGAME(ffv104, 3)
CC_ROMSTART_8(ffv104,  "u1l_v104.bin",CRC(375f4dd3) SHA1(0e3845afccf51a2d20e01afb371b8b7076a1ea79),
                       "u1h_v104.bin",CRC(2133fc8e) SHA1(b4296f890a11aefdd09083636f416112e64fb0be),
                       "u2l_v104.bin",CRC(b74175ae) SHA1(dd0279e20a2ccb03dbea0087ab9d15a973543553),
                       "u2h_v104.bin",CRC(98621d17) SHA1(1656715930af09629b22569ec6b4cde537c2f83f),
                       "u3l_v104.bin",CRC(aed63bd0) SHA1(06e4943cb06c5027abc1e63358c7c8c55344c8f3),
                       "u3h_v104.bin",CRC(9376881e) SHA1(a84c3fecefbc6fc455719c06bf6e77f81fbcb78c),
                       "u4l_v104.bin",CRC(912bc445) SHA1(01b80ba9353e6096066490943ca4a7c64131023d),
                       "u4h_v104.bin",CRC(fb7012a9) SHA1(2e8717954dab0f30b59e716b5a47acf0f3feb379))
CC_ROMEND
CORE_GAMEDEFNV(ffv104,"Flipper Football",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Big Bang Bar - Beta (11/96)
/-------------------------------------------------------------------*/
INITGAME(bbb109, 3)
CC_ROMSTART_4(bbb109,  "u1l_b19.bin", CRC(32be6cb0) SHA1(e6c73366d5b85c0e96878e275320f82004bb97b5),
                       "u1h_b19.bin", CRC(2bd1c06d) SHA1(ba81faa07b9d53f51bb981a82aa8684905516420),
                       "u2l_b17.bin", CRC(9bebf271) SHA1(01c85396b96ffb04e445c03d6d2d88cce7835664),
                       "u2h_b17.bin", CRC(afd36d9c) SHA1(b9f68b1e5792e293b9b8549dce0379ed3d8d2ceb))
CC_ROMEND
CORE_GAMEDEFNV(bbb109,"Big Bang Bar (Beta)",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Kingpin (12/96)
/-------------------------------------------------------------------*/
INITGAME(kpv106, 3)
CC_ROMSTART_2X(kpv106, "u1hu1l.bin",  CRC(d2d42121) SHA1(c731e0b5c9b211574dda8aecbad799bc180a59db),
                       "u2hu2l.bin",  CRC(9cd91371) SHA1(197a06a0ed6b661d798ed18b1db72215c28e1bc2))
CC_ROMEND
CORE_GAMEDEFNV(kpv106,"Kingpin",1996,"Capcom",cc,GAME_NOT_WORKING|GAME_NO_SOUND)
