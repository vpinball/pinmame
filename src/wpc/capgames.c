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
CC_ROMSTART_4(ghv101,  "u06_v11.bin", 0x3b6ab802,
                       "u23_v11.bin", 0xf6cac3aa,
                       "u13_v10.bin", 0x1712f21f,
                       "u17_v10.bin", 0xb6a39327)
CC_ROMEND
CORE_GAMEDEFNV(ghv101,"Goofy Hoops",1995,"Romstar",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Pinball Magic (10/95)
/-------------------------------------------------------------------*/
INITGAME(pmv112, 3)
CC_ROMSTART_4(pmv112,  "u1l_v112.bin",0xc8362623,
                       "u1h_v112.bin",0xf6232c74,
                       "u2l_v10.bin", 0xd3e4241d,
                       "u2h_v10.bin", 0x9276fd62)
CC_ROMEND
CORE_GAMEDEFNV(pmv112,"Pinball Magic",1995,"Capcom",cc,GAME_NO_SOUND)

INITGAME(pmv112r, 3)
CC_ROMSTART_4(pmv112r, "u1lv112i.bin",0x28d35969,
                       "u1hv112i.bin",0xf70da65c,
                       "u2l_v10.bin", 0xd3e4241d,
                       "u2h_v10.bin", 0x9276fd62)
CC_ROMEND
CORE_CLONEDEFNV(pmv112r,pmv112,"Pinball Magic (Redemption)",1995,"Capcom",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Airborne (03/96)
/-------------------------------------------------------------------*/
INITGAME(abv106, 3)
CC_ROMSTART_4(abv106,  "u1l_v16.bin", 0x59b258f1,
                       "u1h_v16.bin", 0xa4571905,
                       "u2l_v10.bin", 0xa15b1ec0,
                       "u2h_v10.bin", 0xc22e3338)
CC_ROMEND
CORE_GAMEDEFNV(abv106,"Airborne",1996,"Capcom",cc,GAME_NO_SOUND)

INITGAME(abv106r, 3)
CC_ROMSTART_4(abv106r, "u1l_v16i.bin",0x7d7d2d85,
                       "u1h_v16i.bin",0xb9bc0c5a,
                       "u2l_v10.bin", 0xa15b1ec0,
                       "u2h_v10.bin", 0xc22e3338)
CC_ROMEND
CORE_CLONEDEFNV(abv106r,abv106,"Airborne (Redemption)",1996,"Capcom",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Breakshot (05/96)
/-------------------------------------------------------------------*/
INITGAME(bsv103, 3)
CC_ROMSTART_2(bsv103,  "u1l_v13.bin", 0xf8932dcc,
                       "u1h_v13.bin", 0x508c145d)
CC_ROMEND
CORE_GAMEDEFNV(bsv103,"Breakshot (1.3)",1996,"Capcom",cc,GAME_NO_SOUND)

INITGAME(bsv100r, 3)
CC_ROMSTART_2(bsv100r, "u1l_v10i.bin",0x304b4da8,
                       "u1h_v10i.bin",0xc10b2aff)
CC_ROMEND
CORE_CLONEDEFNV(bsv100r,bsv103,"Breakshot (Redemption 1.0)",1996,"Capcom",cc,GAME_NO_SOUND)

INITGAME(bsv102r, 3)
CC_ROMSTART_2(bsv102r, "u1l_v12i.bin",0xed09e463,
                       "u1h_v12i.bin",0x71bf99e9)
CC_ROMEND
CORE_CLONEDEFNV(bsv102r,bsv103,"Breakshot (Redemption 1.2)",1996,"Capcom",cc,GAME_NO_SOUND)

INITGAME(bsb105, 3)
CC_ROMSTART_2(bsb105,  "bsu1l_b1.05", 0x053684c7,
                       "bsu1h_b1.05", 0xf1dc6db8)
CC_ROMEND
CORE_CLONEDEFNV(bsb105,bsv103,"Breakshot (Beta 1.5)",1996,"Capcom",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Flipper Football (10/96)
/-------------------------------------------------------------------*/
INITGAME(ffv104, 3)
CC_ROMSTART_8(ffv104,  "u1l_v104.bin",0x375f4dd3,
                       "u1h_v104.bin",0x2133fc8e,
                       "u2l_v104.bin",0xb74175ae,
                       "u2h_v104.bin",0x98621d17,
                       "u3l_v104.bin",0xaed63bd0,
                       "u3h_v104.bin",0x9376881e,
                       "u4l_v104.bin",0x912bc445,
                       "u4h_v104.bin",0xfb7012a9)
CC_ROMEND
CORE_GAMEDEFNV(ffv104,"Flipper Football",1996,"Capcom",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Big Bang Bar - Beta (11/96)
/-------------------------------------------------------------------*/
INITGAME(bbb109, 3)
CC_ROMSTART_4(bbb109,  "u1l_b19.bin", 0x32be6cb0,
                       "u1h_b19.bin", 0x2bd1c06d,
                       "u2l_b17.bin", 0x9bebf271,
                       "u2h_b17.bin", 0xafd36d9c)
CC_ROMEND
CORE_GAMEDEFNV(bbb109,"Big Bang Bar (Beta)",1996,"Capcom",cc,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Kingpin (12/96)
/-------------------------------------------------------------------*/
INITGAME(kpv106, 3)
CC_ROMSTART_2X(kpv106, "u1hu1l.bin",  0xd2d42121,
                       "u2hu2l.bin",  0x9cd91371)
CC_ROMEND
CORE_GAMEDEFNV(kpv106,"Kingpin",1996,"Capcom",cc,GAME_NO_SOUND)
