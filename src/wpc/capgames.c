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

// inverted switches for each game (by game number)
// sw. row number:  0   1     2     3     4     5     6     7     8
#define capInvSw0  {0}
#define capInvSw1  {0, 0x00, 0x3f, 0x04, 0x00, 0x00, 0x08, 0x00, 0x0e}
#define capInvSw2  {0, 0x00, 0x3f, 0x04, 0x00, 0x00, 0x08, 0x00, 0x0e}
#define capInvSw3  {0, 0x00, 0x70, 0x40, 0x78, 0x00, 0x40, 0x80, 0x03}
#define capInvSw4  {0, 0x00, 0x70, 0x40, 0x78, 0x00, 0x40, 0x80, 0x03}
#define capInvSw5  {0, 0x00, 0x00, 0x38, 0x00, 0x30, 0x00, 0x01}
#define capInvSw6  {0, 0x00, 0x00, 0x38, 0x00, 0x30, 0x00, 0x01}
#define capInvSw7  {0, 0x00, 0x00, 0x38, 0x00, 0x30, 0x00, 0x01}
#define capInvSw8  {0, 0x00, 0x00, 0x38, 0x00, 0x30, 0x00, 0x01}
#define capInvSw9  {0, 0x0f, 0x0f, 0x8c, 0x80}
#define capInvSw10 {0, 0x00, 0x01, 0x78, 0x00, 0x00, 0x01}
#define capInvSw11 {0, 0x01, 0x00, 0x78, 0x88, 0x08, 0x10}
#define capInvSw12 {0, 0x00, 0x00, 0x38, 0x00, 0x30, 0x00, 0x01}

#define INITGAME(name, gameno, disp, balls, sb, lamps) \
	CC_INPUT_PORTS_START(name, balls) CC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disp,{FLIP,0,lamps,0,sb,0,gameno},NULL,{"", capInvSw##gameno}}; \
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
//Version 1.0.12
CC_ROMSTART_4(pmv112,  "u1l_v112.bin",CRC(c8362623) SHA1(ebe37d3273e5cefd4fbc041ea3b15d59010b8160),
                       "u1h_v112.bin",CRC(f6232c74) SHA1(28bab61de2ece27aff4cbdd36b10c136a4b7c936),
                       "u2l_v10.bin", CRC(d3e4241d) SHA1(fe480ea2b3901e2e571f8871a0ebe63fbf152e28),
                       "u2h_v10.bin", CRC(9276fd62) SHA1(b80e6186a6a2ded21bd1d6dbd306590645a50523))
//It seems PM uses the same U24 image.. (still unconfirmed, but it seems to work)
CAPCOMS_SOUNDROM3("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				  "u28_v10.bin", CRC(5c12fc2f) SHA1(2e768fb1b5bf56f97af16c4e5542446ef740db58), \
				  "u29_v10.bin", CRC(74352bcd) SHA1(dc62fd651cf8408330f41b2e5387daecfe1d93d7), \
				  "u30_v16.bin", CRC(a7c29b8f) SHA1(1d623c3a67a8e4bf39c22bbf0e008fb2f8920351))
CC_ROMEND
CORE_GAMEDEFNV(pmv112,"Pinball Magic",1995,"Capcom",cc2,0)
//Redemption Version 1.0.12I
INITGAME(pmv112r, 2, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(pmv112r, "u1lv112i.bin",CRC(28d35969) SHA1(e19856402855847286db73c17510614d8b40c882),
                       "u1hv112i.bin",CRC(f70da65c) SHA1(0f98c95edd6f2821e3a67ff1805aa752a4d018c0),
                       "u2l_v10.bin", CRC(d3e4241d) SHA1(fe480ea2b3901e2e571f8871a0ebe63fbf152e28),
                       "u2h_v10.bin", CRC(9276fd62) SHA1(b80e6186a6a2ded21bd1d6dbd306590645a50523))
CAPCOMS_SOUNDROM3a("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_v10.bin", CRC(5c12fc2f) SHA1(2e768fb1b5bf56f97af16c4e5542446ef740db58), \
				   "u29_v10.bin", CRC(74352bcd) SHA1(dc62fd651cf8408330f41b2e5387daecfe1d93d7), \
				   "u30_v16.bin", CRC(a7c29b8f) SHA1(1d623c3a67a8e4bf39c22bbf0e008fb2f8920351), \
				   "u31_v19i.bin",CRC(24735815) SHA1(6fbc1f86090ce42ea27805c700d8b132eafa271f))
CC_ROMEND
CORE_CLONEDEFNV(pmv112r,pmv112,"Pinball Magic (Redemption)",1995,"Capcom",cc2,0)

/*-------------------------------------------------------------------
/ Airborne (03/96)
/-------------------------------------------------------------------*/
//Version 1.6
INITGAME(abv106, 3, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(abv106,  "u1l_v16.bin", CRC(59b258f1) SHA1(764496114609d65648e1c7b12409ec582037d8df),
                       "u1h_v16.bin", CRC(a4571905) SHA1(62fabc45e81c49125c047c6e5d268d4093b860bc),
                       "u2l_v10.bin", CRC(a15b1ec0) SHA1(673a283ddf670109a9728fefac2bcf493d70f23d),
                       "u2h_v10.bin", CRC(c22e3338) SHA1(1a25c85a1ed59647c40f9a4d417d78cccff7e51c))
CAPCOMS_SOUNDROM3("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				  "u28_v10.bin", CRC(ca3c6954) SHA1(44345c0a720c78c312459425c54180a4c5413c0d), \
				  "u29_v10.bin", CRC(8989d566) SHA1(f1827fb5c1d917a324fffe2035e87fcca77f362f), \
				  "u30_v11.bin", CRC(e16f1c4d) SHA1(9aa0ff87c303c6a8c95ef1c0e5382abad6179e21))
CC_ROMEND
CORE_GAMEDEFNV(abv106,"Airborne",1996,"Capcom",cc2,0)
//Redemption Version 1.6I
INITGAME(abv106r, 4, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(abv106r, "u1l_v16i.bin",CRC(7d7d2d85) SHA1(5c83022d7c0b61b15455942b3bdd0cf89fc75b57),
                       "u1h_v16i.bin",CRC(b9bc0c5a) SHA1(e6fc393b970a2c354e0b0150dafbbbea2a85b92d),
                       "u2l_v10.bin", CRC(a15b1ec0) SHA1(673a283ddf670109a9728fefac2bcf493d70f23d),
                       "u2h_v10.bin", CRC(c22e3338) SHA1(1a25c85a1ed59647c40f9a4d417d78cccff7e51c))
CAPCOMS_SOUNDROM3a("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_v10.bin", CRC(ca3c6954) SHA1(44345c0a720c78c312459425c54180a4c5413c0d), \
				   "u29_v10.bin", CRC(8989d566) SHA1(f1827fb5c1d917a324fffe2035e87fcca77f362f), \
				   "u30_v11.bin", CRC(e16f1c4d) SHA1(9aa0ff87c303c6a8c95ef1c0e5382abad6179e21), \
				   "u31_v11i.bin",CRC(57794507) SHA1(9ec7648d948893a37dcda3a9c5ff56c7ce725291))
CC_ROMEND
CORE_CLONEDEFNV(abv106r,abv106,"Airborne (Redemption)",1996,"Capcom",cc2,0)

/*-------------------------------------------------------------------
/ Break Shot (05/96)
/-------------------------------------------------------------------*/
//Version 1.3
INITGAME(bsv103, 5, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv103,  "u1l_v13.bin", CRC(f8932dcc) SHA1(dab34e6c412655c60abeedc1f62254dce5ebb202),
                       "u1h_v13.bin", CRC(508c145d) SHA1(b019d445f87bca203646c616fdc295066da90921))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				  "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				  "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a))
CC_ROMEND
CORE_GAMEDEFNV(bsv103,"Break Shot",1996,"Capcom",cc1,0)
//Redemption Version 1.0I
INITGAME(bsv100r, 6, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv100r, "u1l_v10i.bin",CRC(304b4da8) SHA1(2643f304adce3543b792bd2d0ec8abe8d9a5478c),
                       "u1h_v10i.bin",CRC(c10b2aff) SHA1(a56af0903c8b8282baf63dc47741ef094cfd6a1c))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2a("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				   "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a), \
				   "u30_v10i.bin",CRC(8b7f6c41) SHA1(b564e5af3b60744df54f22940ab53956c4f89ee6))
CC_ROMEND
CORE_CLONEDEFNV(bsv100r,bsv103,"Break Shot (Redemption 1.0)",1996,"Capcom",cc1,0)
//Redemption Version 1.2I
INITGAME(bsv102r, 7, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsv102r, "u1l_v12i.bin",CRC(ed09e463) SHA1(74b4e3e93648e05e66a20f895133f1a0ba2ecb20),
                       "u1h_v12i.bin",CRC(71bf99e9) SHA1(cb48eb5c5df6b03022d9cb20c84dfdcc34a7d5ac))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2a("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_v11.bin", CRC(b076ad2e) SHA1(1be8e3bda2890545253f6f7e4825d2db1d925255), \
				   "u29_v11.bin", CRC(b251a27c) SHA1(bc30791cb9b5497c11f1cff06c89a729a07b5d4a), \
				   "u30_v10i.bin",CRC(8b7f6c41) SHA1(b564e5af3b60744df54f22940ab53956c4f89ee6))
CC_ROMEND
CORE_CLONEDEFNV(bsv102r,bsv103,"Break Shot (Redemption 1.2)",1996,"Capcom",cc1,0)
//Beta Version 1.5
INITGAME(bsb105, 8, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 0)
CC_ROMSTART_2(bsb105,  "bsu1l_b1.05", CRC(053684c7) SHA1(cf104a6e9c245523e29989389654c12437d32776),
                       "bsu1h_b1.05", CRC(f1dc6db8) SHA1(da209872047a1deac88fe389bcc26bcf353f6df8))
//I'm told BS can run with the U24 from FF
CAPCOMS_SOUNDROM2b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "bsu28_b1.2",  CRC(b65880be) SHA1(d42da68ab58f87516656315ad5d389a444a674ff))
CC_ROMEND
CORE_CLONEDEFNV(bsb105,bsv103,"Break Shot (Beta)",1996,"Capcom",cc1,0)

/*-------------------------------------------------------------------
/ Flipper Football (10/96)
/-------------------------------------------------------------------*/
//Version 1.04
INITGAME(ffv104, 9, cc_dispDMD256x64, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_8(ffv104,  "u1l_v104.bin",CRC(375f4dd3) SHA1(0e3845afccf51a2d20e01afb371b8b7076a1ea79),
                       "u1h_v104.bin",CRC(2133fc8e) SHA1(b4296f890a11aefdd09083636f416112e64fb0be),
                       "u2l_v104.bin",CRC(b74175ae) SHA1(dd0279e20a2ccb03dbea0087ab9d15a973543553),
                       "u2h_v104.bin",CRC(98621d17) SHA1(1656715930af09629b22569ec6b4cde537c2f83f),
                       "u4l_v104.bin",CRC(912bc445) SHA1(01b80ba9353e6096066490943ca4a7c64131023d),
                       "u4h_v104.bin",CRC(fb7012a9) SHA1(2e8717954dab0f30b59e716b5a47acf0f3feb379),
                       "u3l_v104.bin",CRC(aed63bd0) SHA1(06e4943cb06c5027abc1e63358c7c8c55344c8f3),
                       "u3h_v104.bin",CRC(9376881e) SHA1(a84c3fecefbc6fc455719c06bf6e77f81fbcb78c))
CAPCOMS_SOUNDROM4a("u24_v11.bin",  CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_v101.bin", CRC(68b896e0) SHA1(3d8c286d43c1db68c39fb4d130cd3cd679209a22), \
				   "u29_v101.bin", CRC(b79f3e58) SHA1(9abd570590216800bbfe9f12b4660fbe0200679e), \
				   "u30_v101.bin", CRC(f5432518) SHA1(8c26a267335289145f29db822bf7dfcb4730b208), \
				   "u31_v101.bin", CRC(2b14e032) SHA1(c423ae5ed2fcc582201606bac3e766ec332b395a))
CC_ROMEND
CORE_GAMEDEFNV(ffv104,"Flipper Football (v1.04)",1996,"Capcom",cc2,0)
//Version 1.01
INITGAME(ffv101, 12, cc_dispDMD256x64, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_8(ffv101,  "u1l_v100.bin",CRC(1c0b776f) SHA1(a1cabe9646973a97000a8f42295dfcfbed3691fa),
                       "u1h_v100.bin",CRC(13590a38) SHA1(04cb048677b725a2563a12f18853372d5280d464),
                       "u2l_v100.bin",CRC(ee373854) SHA1(cf4814c8c18ab5ca6bf3f134e3c3f95e6f8fe870),
                       "u2h_v100.bin",CRC(8ebe3530) SHA1(903f38111482860eae44d5e601bbf26b50a40e2b),
                       "u4l_v100.bin",CRC(55982601) SHA1(150c22e200855041746ac08f4817dd8d3a04f64d),
                       "u4h_v100.bin",CRC(35b60875) SHA1(e3bc752f77cc6baeb7010e9e95bd10d4935e44da),
                       "u3l_v101.bin",CRC(03319c15) SHA1(dbbdcfe5baab3ec654ddf4b331d1332ec3e47c76),
                       "u3h_v101.bin",CRC(b55532cb) SHA1(b539a62d33eaa43057249450c40905b2d6fe1e1f))
CAPCOMS_SOUNDROM4a("u24_v11.bin",  CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_v100.bin", CRC(78c60574) SHA1(399a98b707b32096da5dc6c902ac10feca371433), \
				   "u29_v100.bin", CRC(8c37fbca) SHA1(5c3a3e1cc076e7a2732f3546005961d191040912), \
				   "u30_v100.bin", CRC(a92885a1) SHA1(b06453c710fd86e97567e70ab7558b0c2fd54c72), \
				   "u31_v100.bin", CRC(358c2727) SHA1(73ac6cc51a6ceb27934607909a0fff369a47ba7d))
CC_ROMEND
CORE_CLONEDEFNV(ffv101,ffv104,"Flipper Football (v1.01)",1996,"Capcom",cc2,0)

/*-------------------------------------------------------------------
/ Big Bang Bar - Beta (11/96)
/-------------------------------------------------------------------*/
//Beta Version 1.9
INITGAME(bbb109, 10, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(bbb109,  "u1l_b19.bin", CRC(32be6cb0) SHA1(e6c73366d5b85c0e96878e275320f82004bb97b5),
                       "u1h_b19.bin", CRC(2bd1c06d) SHA1(ba81faa07b9d53f51bb981a82aa8684905516420),
                       "u2l_b17.bin", CRC(9bebf271) SHA1(01c85396b96ffb04e445c03d6d2d88cce7835664),
                       "u2h_b17.bin", CRC(afd36d9c) SHA1(b9f68b1e5792e293b9b8549dce0379ed3d8d2ceb))
CAPCOMS_SOUNDROM4b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_b17.bin", CRC(af47c0f0) SHA1(09f84b9d1399183298279dfac95367741d6304e5), \
				   "u29_b17.bin", CRC(b5aa0d76) SHA1(c732fc76b992261da8475097adc70514e5a1c2e3), \
				   "u30_b17.bin", CRC(b4b6011b) SHA1(362c11353390f9ed2ee788847e6a2078b29c8806), \
				   "u31_b17.bin", CRC(3016563f) SHA1(432e89dd975559017771da3543e9fe36e425a32b))
CC_ROMEND
CORE_GAMEDEFNV(bbb109,"Big Bang Bar (Beta 1.9 US)",1996,"Capcom",cc2,0)

INITGAME(bbb108, 10, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_4(bbb108,  "u1l_b18.bin", CRC(60a02e1e) SHA1(c2967b4ba0ce01cb9f4ed5ceb4ca5f16596fc75b),
                       "u1h_b18.bin", CRC(7a987a29) SHA1(5307b7feb8d86cf7dd51dd9c501b2539441b684e),
                       "u2l_b17.bin", CRC(9bebf271) SHA1(01c85396b96ffb04e445c03d6d2d88cce7835664),
                       "u2h_b17.bin", CRC(afd36d9c) SHA1(b9f68b1e5792e293b9b8549dce0379ed3d8d2ceb))
CAPCOMS_SOUNDROM4b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274),\
				   "u28_b17.bin", CRC(af47c0f0) SHA1(09f84b9d1399183298279dfac95367741d6304e5), \
				   "u29_b17.bin", CRC(b5aa0d76) SHA1(c732fc76b992261da8475097adc70514e5a1c2e3), \
				   "u30_b17.bin", CRC(b4b6011b) SHA1(362c11353390f9ed2ee788847e6a2078b29c8806), \
				   "u31_b17.bin", CRC(3016563f) SHA1(432e89dd975559017771da3543e9fe36e425a32b))
CC_ROMEND
CORE_CLONEDEFNV(bbb108,bbb109,"Big Bang Bar (Beta 1.8 US)",1996,"Capcom",cc2,0)

/*-------------------------------------------------------------------
/ Kingpin (12/96)
/-------------------------------------------------------------------*/
//Beta Version 1.5 (There should be a regular version 1.6 according to Pfutz!)
INITGAME(kpv106, 11, cc_dispDMD128x32, 3, SNDBRD_CAPCOMS, 8)
CC_ROMSTART_2X(kpv106, "u1hu1l.bin",  CRC(d2d42121) SHA1(c731e0b5c9b211574dda8aecbad799bc180a59db),
                       "u2hu2l.bin",  CRC(9cd91371) SHA1(197a06a0ed6b661d798ed18b1db72215c28e1bc2))
CAPCOMS_SOUNDROM4b("u24_v11.bin", CRC(d46212f4) SHA1(50f1279d995b597c468805b323e0252800b28274), \
				   "u28_b11.bin", CRC(aa480506) SHA1(4fbf384bc5e2d0eec4d1137784006d63091974ca), \
				   "u29_b11.bin", CRC(33345446) SHA1(d229d45228e13e2f02b73ce125eab4f2dd91db6e), \
				   "u30_b11.bin", CRC(fa35a177) SHA1(3c54c12db8e17a8c316a22b9b7ac80b6b3af8474), \
				   "u31_b18.bin", CRC(07a7d514) SHA1(be8cb4b6d70ccae7821110689b714612c8a0b460))
CC_ROMEND
CORE_GAMEDEFNV(kpv106,"Kingpin",1996,"Capcom",cc2,0)
