#include "driver.h"
#include "sim.h"
#include "capcom.h"
#include "capcoms.h"
#include "sndbrd.h"

#define FLIP    FLIP_SWNO(5,6)

/*-- DMD 128 X 32 --*/
const core_tLCDLayout cc_dispDMD128x32[] = {
  {0,0,32,128,CORE_DMD,(void *)cc_dmd128x32}, {0}
};

/*-- DMD 256 X 64 --*/
const core_tLCDLayout cc_dispDMD256x64[] = {
  {0,0,64,256,CORE_DMD,(void *)cc_dmd256x64}, {0}
};

#define INITGAME(name, gameno, disp, balls, sb, lamps) \
	CC_INPUT_PORTS_START(name, balls) CC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disp,{FLIP,0,lamps,0,sb,0,gameno}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/*-------------------------------------------------------------------
/ Goofy Hoops (Romstar game) (??/95)
/-------------------------------------------------------------------*/
//Might have different hardware mappings as a quick peek of the code looked different than the pins
INITGAME(ghv101, 0, cc_dispDMD128x32, 3, 0, 0)
CC_ROMSTART_4(ghv101,  "u06_v11.bin", CRC(3b6ab802),
                       "u23_v11.bin", CRC(f6cac3aa),
                       "u13_v10.bin", CRC(1712f21f),
                       "u17_v10.bin", CRC(b6a39327))
CC_ROMEND
CORE_GAMEDEFNV(ghv101,"Goofy Hoops",1995,"Romstar",cc,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Pinball Magic (10/95)
/-------------------------------------------------------------------*/
INITGAME(pmv112, 1, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
//Version 1.12
CC_ROMSTART_4(pmv112,  "u1l_v112.bin",CRC(c8362623),
                       "u1h_v112.bin",CRC(f6232c74),
                       "u2l_v10.bin", CRC(d3e4241d),
                       "u2h_v10.bin", CRC(9276fd62))
CAPCOMS_SOUNDROM3("u24_v10.bin", NO_DUMP,\
				  "u28_v11.bin", CRC(5c12fc2f) SHA1(2e768fb1b5bf56f97af16c4e5542446ef740db58), \
				  "u29_v11.bin", CRC(74352bcd) SHA1(dc62fd651cf8408330f41b2e5387daecfe1d93d7), \
				  "u30_v16.bin", CRC(a7c29b8f) SHA1(1d623c3a67a8e4bf39c22bbf0e008fb2f8920351)) 
CC_ROMEND
CORE_GAMEDEFNV(pmv112,"Pinball Magic",1995,"Capcom",cc2,GAME_IMPERFECT_SOUND)
//Redemption Version 1.12
INITGAME(pmv112r, 2, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(pmv112r, "u1lv112i.bin",CRC(28d35969),
                       "u1hv112i.bin",CRC(f70da65c),
                       "u2l_v10.bin", CRC(d3e4241d),
                       "u2h_v10.bin", CRC(9276fd62))
CAPCOMS_SOUNDROM3("u24_v10.bin", NO_DUMP,\
				  "u28_v11.bin", CRC(5c12fc2f) SHA1(2e768fb1b5bf56f97af16c4e5542446ef740db58), \
				  "u29_v11.bin", CRC(74352bcd) SHA1(dc62fd651cf8408330f41b2e5387daecfe1d93d7), \
				  "u30_v16.bin", CRC(a7c29b8f) SHA1(1d623c3a67a8e4bf39c22bbf0e008fb2f8920351)) 
CC_ROMEND
CORE_CLONEDEFNV(pmv112r,pmv112,"Pinball Magic (Redemption)",1995,"Capcom",cc2,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Airborne (03/96)
/-------------------------------------------------------------------*/
//Version 1.06
INITGAME(abv106, 3, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(abv106,  "u1l_v16.bin", CRC(59b258f1),
                       "u1h_v16.bin", CRC(a4571905),
                       "u2l_v10.bin", CRC(a15b1ec0),
                       "u2h_v10.bin", CRC(c22e3338))
CAPCOMS_SOUNDROM3("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				  "u28_v10.bin", CRC(ca3c6954) SHA1(44345c0a720c78c312459425c54180a4c5413c0d), \
				  "u29_v10.bin", CRC(8989d566) SHA1(f1827fb5c1d917a324fffe2035e87fcca77f362f), \
				  "u30_v11.bin", CRC(e16f1c4d) SHA1(9aa0ff87c303c6a8c95ef1c0e5382abad6179e21)) 
CC_ROMEND
CORE_GAMEDEFNV(abv106,"Airborne",1996,"Capcom",cc2,GAME_IMPERFECT_SOUND)
//Redemption Version 1.06
INITGAME(abv106r, 4, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(abv106r, "u1l_v16i.bin",CRC(7d7d2d85),
                       "u1h_v16i.bin",CRC(b9bc0c5a),
                       "u2l_v10.bin", CRC(a15b1ec0),
                       "u2h_v10.bin", CRC(c22e3338))
CAPCOMS_SOUNDROM3("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				  "u28_v10.bin", CRC(ca3c6954) SHA1(44345c0a720c78c312459425c54180a4c5413c0d), \
				  "u29_v10.bin", CRC(8989d566) SHA1(f1827fb5c1d917a324fffe2035e87fcca77f362f), \
				  "u30_v11.bin", CRC(e16f1c4d) SHA1(9aa0ff87c303c6a8c95ef1c0e5382abad6179e21)) 
CC_ROMEND
CORE_CLONEDEFNV(abv106r,abv106,"Airborne (Redemption)",1996,"Capcom",cc2,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Breakshot (05/96)
/-------------------------------------------------------------------*/
//Version 1.03
INITGAME(bsv103, 5, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv103,  "u1l_v13.bin", CRC(f8932dcc),
                       "u1h_v13.bin", CRC(508c145d))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				  "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				  "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a)) 
CC_ROMEND
CORE_GAMEDEFNV(bsv103,"Breakshot (1.3)",1996,"Capcom",cc1,GAME_IMPERFECT_SOUND)
//Redemption Version 1.00
INITGAME(bsv100r, 6, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv100r, "u1l_v10i.bin",CRC(304b4da8),
                       "u1h_v10i.bin",CRC(c10b2aff))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				  "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				  "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a)) 
CC_ROMEND
CORE_CLONEDEFNV(bsv100r,bsv103,"Breakshot (Redemption 1.0)",1996,"Capcom",cc1,GAME_IMPERFECT_SOUND)
//Redemption Version 1.02
INITGAME(bsv102r, 7, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv102r, "u1l_v12i.bin",CRC(ed09e463),
                       "u1h_v12i.bin",CRC(71bf99e9))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				  "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				  "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a)) 
CC_ROMEND
CORE_CLONEDEFNV(bsv102r,bsv103,"Breakshot (Redemption 1.2)",1996,"Capcom",cc1,GAME_IMPERFECT_SOUND)
//Beta Version 1.05
INITGAME(bsb105, 8, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsb105,  "bsu1l_b1.05", CRC(053684c7),
                       "bsu1h_b1.05", CRC(f1dc6db8))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				  "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				  "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a)) 
CC_ROMEND
CORE_CLONEDEFNV(bsb105,bsv103,"Breakshot (Beta 1.5)",1996,"Capcom",cc1,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Flipper Football (10/96)
/-------------------------------------------------------------------*/
//Version 1.04
INITGAME(ffv104, 9, cc_dispDMD256x64, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_8(ffv104,  "u1l_v104.bin",CRC(375f4dd3),
                       "u1h_v104.bin",CRC(2133fc8e),
                       "u2l_v104.bin",CRC(b74175ae),
                       "u2h_v104.bin",CRC(98621d17),
                       "u4l_v104.bin",CRC(912bc445),
                       "u4h_v104.bin",CRC(fb7012a9),
                       "u3l_v104.bin",CRC(aed63bd0),
                       "u3h_v104.bin",CRC(9376881e))
CAPCOMS_SOUNDROM4a("u24_v11.bin",  CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_v101.bin", CRC(68b896e0) SHA1(3d8c286d43c1db68c39fb4d130cd3cd679209a22), \
				   "u29_v101.bin", CRC(b79f3e58) SHA1(9abd570590216800bbfe9f12b4660fbe0200679e), \
				   "u30_v101.bin", CRC(f5432518) SHA1(8c26a267335289145f29db822bf7dfcb4730b208), \
				   "u31_v101.bin", CRC(2b14e032) SHA1(c423ae5ed2fcc582201606bac3e766ec332b395a)) 
CC_ROMEND
CORE_GAMEDEFNV(ffv104,"Flipper Football",1996,"Capcom",cc2,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Big Bang Bar - Beta (11/96)
/-------------------------------------------------------------------*/
//Beta Version 1.09
INITGAME(bbb109, 10, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(bbb109,  "u1l_b19.bin", CRC(32be6cb0),
                       "u1h_b19.bin", CRC(2bd1c06d),
                       "u2l_b17.bin", CRC(9bebf271),
                       "u2h_b17.bin", CRC(afd36d9c))
CAPCOMS_SOUNDROM4b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_b17.bin", CRC(af47c0f0) SHA1(09f84b9d1399183298279dfac95367741d6304e5), \
				   "u29_b17.bin", CRC(b5aa0d76) SHA1(c732fc76b992261da8475097adc70514e5a1c2e3), \
				   "u30_b17.bin", CRC(b4b6011b) SHA1(362c11353390f9ed2ee788847e6a2078b29c8806), \
				   "u31_b17.bin", CRC(3016563f) SHA1(432e89dd975559017771da3543e9fe36e425a32b)) 
CC_ROMEND
CORE_GAMEDEFNV(bbb109,"Big Bang Bar (Beta)",1996,"Capcom",cc2,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Kingpin (12/96)
/-------------------------------------------------------------------*/
//Beta Version 1.06
INITGAME(kpv106, 11, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_2X(kpv106, "u1hu1l.bin",  CRC(d2d42121),
                       "u2hu2l.bin",  CRC(9cd91371))
CAPCOMS_SOUNDROM4b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_b11.bin", CRC(aa480506) SHA1(4fbf384bc5e2d0eec4d1137784006d63091974ca), \
				   "u29_b11.bin", CRC(33345446) SHA1(d229d45228e13e2f02b73ce125eab4f2dd91db6e), \
				   "u30_b11.bin", CRC(fa35a177) SHA1(3c54c12db8e17a8c316a22b9b7ac80b6b3af8474), \
				   "u31_b18.bin", CRC(07a7d514) SHA1(be8cb4b6d70ccae7821110689b714612c8a0b460)) 
CC_ROMEND
CORE_GAMEDEFNV(kpv106,"Kingpin",1996,"Capcom",cc2,GAME_IMPERFECT_SOUND)
