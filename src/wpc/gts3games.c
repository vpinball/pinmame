#include "driver.h"
#include "sim.h"
#include "gts3.h"
#include "gts80s.h"
#include "sndbrd.h"

#define ALPHA     GTS3_dispAlpha
#define ALPHA_LED GTS3_dispAlphaLED
#define DMD       GTS3_dispDMD

#define FLIP67   FLIP_SWNO(6,7)
#define FLIP4142 FLIP_SWNO(41,42)
#define FLIP4243 FLIP_SWNO(42,43)
#define FLIP4547 FLIP_SWNO(45,47)
#define FLIP5051 FLIP_SWNO(50,51)
#define FLIP6061 FLIP_SWNO(60,61)
#define FLIP8182 FLIP_SWNO(81,82)
#define FLIP8283 FLIP_SWNO(82,83)

/* Dot-Matrix display */
static struct core_dispLayout GTS3_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)gts3_dmd128x32}, {0}
};

/* 2 X 20 AlphaNumeric Rows */
static struct core_dispLayout GTS3_dispAlpha[] = {
  {0, 0, 0,20,CORE_SEG16}, {2, 0,20,20,CORE_SEG16}, {0}
};
/* 2 X 20 AlphaNumeric Rows, 4 X 2 7-seg displays */
static struct core_dispLayout GTS3_dispAlphaLED[] = {
  DISP_SEG_IMPORT(GTS3_dispAlpha),
  {5, 0,40,2,CORE_SEG7}, {5,12,42,2,CORE_SEG7}, {5,24,44,2,CORE_SEG7}, {5,36,46,2,CORE_SEG7}, {0}
};

#define INITGAME_IC(name, disptype, flippers, balls, sb, lamps) \
	GTS3_IC_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_GTS3,disptype,{flippers,4,lamps,0,sb,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME(name, disptype, flippers, balls, sb, lamps) \
	GTS3_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_GTS3,disptype,{flippers,4,lamps,0,sb,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME1(name, disptype, flippers, balls, sb, lamps) \
	static core_tGameData name##GameData = {GEN_GTS3,disptype,{flippers,4,lamps,0,sb,0}}; \
	static void init_##name(void) { \
	  core_gameData = &name##GameData; \
	} \
	GTS31_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

#define INITGAME2(name, disptype, flippers, balls, sb, lamps) \
	static core_tGameData name##GameData = {GEN_GTS3,disptype,{flippers,4,lamps,0,sb,0}}; \
	static void init_##name(void) { \
	  core_gameData = &name##GameData; \
	} \
	GTS32_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Unnamed game? by Toptronic HGmbH, Germany
/-------------------------------------------------------------------*/
/* 2 X 20 AlphaNumeric Rows, 4 X 3 7-seg displays */
static struct core_dispLayout dispToptronic[] = {
  DISP_SEG_IMPORT(GTS3_dispAlpha),
  {5,12,40,8,CORE_SEG7}, {0}
};
INITGAME_IC(tt_game, dispToptronic, FLIP67, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(tt_game,   "gprom.bin",CRC(e7944b75) SHA1(b73f2e0004556c8aa88baef0cddcdefb5b905b8d))
GTS3SOUND32128(         "yrom1.bin",NO_DUMP,
                        "drom1.bin",NO_DUMP,
                        "arom1.bin",CRC(b0983d90) SHA1(72e6a71f20fd5849543ca13813f062a3fc1d7dcf),
                        "arom2.bin",CRC(3e31ce58) SHA1(a2ef72d7b2bb821d1f62dce7212e31a1df3e7791))
GTS3_ROMEND
CORE_GAMEDEFNV(tt_game,"Unnamed game",19??,"Toptronic",mGTS3S,GAME_NOT_WORKING)

// Game produced by Premier for International Concepts
/*-------------------------------------------------------------------
/ Caribbean Cruise (#C102)
/-------------------------------------------------------------------*/
INITGAME_IC(ccruise, ALPHA, FLIP67, 2, SNDBRD_GTS3, 4)
GTS3ROMSTART(ccruise,	"gprom.bin",CRC(668b5757) SHA1(8ff955e8598ffdc68eab7fd69c6a67c4eed13f0f))
GTS3SOUND32128(			"yrom1.bin",CRC(6e424e53) SHA1(90a9bf5ce84680972f9d12eb386215494c584b9b),
						"drom1.bin",CRC(4480257e) SHA1(50b93d4496816ef7cdf007ac75c72c6aaa956aba),
                        "arom1.bin",NO_DUMP,
                        "arom2.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(ccruise,"Caribbean Cruise",1989,"International Concepts",mGTS3SNO,0)

// Premier games below
/*-------------------------------------------------------------------
/ Lights, Camera, Action (#720)
/-------------------------------------------------------------------*/
INITGAME(lca, ALPHA, FLIP6061, 3 /*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART32(lca,	"gprom.bin",CRC(52957d70) SHA1(0c24d824b1aa966eb3af3db3ff02870ba463dcd6))
GTS3SOUND3232(		"yrom1.bin",CRC(20919ebb) SHA1(a2ea79863b41a04aa23ea596932825408cca64e3),
					"drom1.bin",CRC(a258d72d) SHA1(eeb4768c8b2f57509a088d3ac8d35aa34f2cfc2c))
GTS3_ROMEND
CORE_GAMEDEFNV(lca,"Lights, Camera, Action",1989,"Gottlieb",mGTS3S80B3,0)

//62c0beda Rev 1?

INITGAME(lca2, ALPHA, FLIP6061, 3 /*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART32(lca2,"gprom2.bin",CRC(937a8426) SHA1(6bc2d1b0c3dc273577376654ba72b60febe32529))
GTS3SOUND3232(		"yrom1.bin",CRC(20919ebb) SHA1(a2ea79863b41a04aa23ea596932825408cca64e3),
					"drom1.bin",CRC(a258d72d) SHA1(eeb4768c8b2f57509a088d3ac8d35aa34f2cfc2c))
GTS3_ROMEND
CORE_CLONEDEFNV(lca2,lca,"Lights, Camera, Action (rev.2)",1989,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Bell Ringer
/-------------------------------------------------------------------*/
INITGAME(bellring, ALPHA, FLIP_SW(FLIP_L), 1, SNDBRD_GTS80B, 4)
GTS3ROMSTART(bellring,	"br_gprom.bin",CRC(a9a59b36) SHA1(ca6d0e54a5c85ef72485975c632660831a3b8c82))
GTS3SOUND3232(			"br_yrom1.bin",CRC(d5aab379) SHA1(b3995f8aa2e54f91f2a0fd010c807fbfbf9ae847),
						"br_drom1.bin",CRC(99f38229) SHA1(f63d743e63e88728e8d53320b21b2fda1b6385f8))
GTS3_ROMEND
CORE_GAMEDEFNV(bellring,"Bell Ringer",1990,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Silver Slugger (#722)
/-------------------------------------------------------------------*/
INITGAME(silvslug, ALPHA_LED, FLIP67, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(silvslug,	"gprom.bin",CRC(a6c524e2) SHA1(dc12dd8e814a37aada021f84c58475efe72cb846))
GTS3SOUND3232(			"yrom1.bin",CRC(20bc9797) SHA1(5d17b5f0423d8854fb7c4816d53a223ecc7c50c6),
						"drom1.bin",CRC(eac3e1cc) SHA1(2725457231854e4f3d54fbba745b8fc6f55b1688))
GTS3_ROMEND
CORE_GAMEDEFNV(silvslug,"Silver Slugger",1990,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Vegas (#723)
/-------------------------------------------------------------------*/
/* 2 X 20 AlphaNumeric Rows, 3 X 14-seg alpha digits (without commas) */
static struct core_dispLayout dispVegas[] = {
  DISP_SEG_IMPORT(GTS3_dispAlpha),
  {5,16,40,1,CORE_SEG16}, {5,19,41,1,CORE_SEG16}, {5,22,42,1,CORE_SEG16}, {0}
};
INITGAME(vegas, dispVegas, FLIP5051, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(vegas,	"gprom.bin",CRC(48189981) SHA1(95144af4b222158becd4d5748d15b7b6c6021bd2))
GTS3SOUND3232(		"yrom1.bin",CRC(af1095f1) SHA1(06609085cd74b969e4f2ec962c427c5c42ebc6ff),
					"drom1.bin",CRC(46eb5755) SHA1(94ec2d0cf41f68a8c3d7505186b11b4abb4803db))
GTS3_ROMEND
CORE_GAMEDEFNV(vegas,"Vegas",1990,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Deadly Weapon (#724)
/-------------------------------------------------------------------*/
INITGAME(deadweap, ALPHA_LED, FLIP67, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(deadweap,	"gprom.bin",CRC(07d84b32) SHA1(25d8772a5c8655b3406df94563076719b07129cd))
GTS3SOUND3232(			"yrom1.bin",CRC(93369ed3) SHA1(3340478ffc00cf9991beabd4f0ecd89d0c88965e),
						"drom1.bin",CRC(f55dd7ec) SHA1(fe306c40bf3d98e4076d0d8a935c3671469d4cff))
GTS3_ROMEND
CORE_GAMEDEFNV(deadweap,"Deadly Weapon",1990,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Title Fight (#726)
/-------------------------------------------------------------------*/
INITGAME(tfight, ALPHA_LED, FLIP67, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(tfight,	"gprom.bin",CRC(43b3193a) SHA1(bd185fe67c147a6acca8e78da4b77c384124fc46))
GTS3SOUND3232(			"yrom1.bin",CRC(8591d421) SHA1(74402cf8b419e0cb05069851b0d5616e66b2f0a9),
						"drom1.bin",CRC(9514739f) SHA1(2794549f549d68e064a9a962a4e91fff7dcf0160))
GTS3_ROMEND
CORE_GAMEDEFNV(tfight,"Title Fight",1990,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Nudge It (N102)
/-------------------------------------------------------------------*/
INITGAME(nudgeit, ALPHA, FLIP_SW(FLIP_L), 1, SNDBRD_GTS80B, 4)
GTS3ROMSTART(nudgeit,	"gprom.bin",CRC(3d9e0309) SHA1(caaa28482e7f260668aa05b39b551acb8e4cc41a))
GTS3SOUND3232(			"yrom1.bin",CRC(65fc2e60) SHA1(6377c220753d9e4b5c76d445056409526d95772f),
						"drom1.bin",CRC(ae0c4b1d) SHA1(c8aa409c9b54fd8ecf70eb2926f4e98fc5eb11fe))
GTS3_ROMEND
CORE_GAMEDEFNV(nudgeit,"Nudge It",1990,"Gottlieb",mGTS3S80B2,0)

/*-------------------------------------------------------------------
/ Car Hop (#725)
/-------------------------------------------------------------------*/
INITGAME(carhop, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(carhop,	"gprom.bin",CRC(164b2c9c) SHA1(49cf7e3a3acb5de8dbfd2ad22f8bd9a352ff2899))
GTS3SOUND3232(			"yrom1.bin",CRC(831ee812) SHA1(57056cde36b17cb7d7f34275b1bb5dc3d52bde4e),
						"drom1.bin",CRC(9dec74e7) SHA1(8234bdca5536d30dc1eabcb3a5505d2fd824ce0f))
GTS3_ROMEND
CORE_GAMEDEFNV(carhop,"Car Hop",1991,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Hoops (#727)
/-------------------------------------------------------------------*/
/* 2 X 20 AlphaNumeric Rows, 4 X 3 7-seg displays */
static struct core_dispLayout dispHoops[] = {
  DISP_SEG_IMPORT(GTS3_dispAlpha),
  {5, 0,40,3,CORE_SEG7}, {5,11,43,3,CORE_SEG7}, {5,23,46,3,CORE_SEG7}, {5,34,49,3,CORE_SEG7}, {0}
};
INITGAME(hoops, dispHoops, FLIP67, 3/*?*/, SNDBRD_GTS80B, 4)
GTS3ROMSTART(hoops,		"gprom.bin",CRC(78391273) SHA1(dbf91597ce2910e526fb5e82355ad862706b4975))
GTS3SOUND3232(			"yrom1.bin",CRC(9718b958) SHA1(bac806267bab4852c0f3fdb48f8d872992f61ace),
						"drom1.bin",CRC(e72c00eb) SHA1(5b9f85083b38d916afb0f9b72b061501504725ff))
GTS3_ROMEND
CORE_GAMEDEFNV(hoops,"Hoops",1991,"Gottlieb",mGTS3S80B3,0)

/*-------------------------------------------------------------------
/ Cactus Jack's (#729)
/-------------------------------------------------------------------*/
INITGAME(cactjack, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(cactjack,	"gprom.bin",CRC(5661ab06) SHA1(12b7066110feab0aef36ff7bdc74690fc8da4ed3))
GTS3SOUND32128(			"yrom1.bin",CRC(4554ed0d) SHA1(df0a9225f961e0ee876c3e63ad54c6e4eac080ae),
						"drom1.bin",CRC(78c099e1) SHA1(953111237fdc3e20562d823eb2b6430e5a4afe4d),
						"arom1.bin",CRC(c890475f) SHA1(1cf6ed0dbd003a76a5cf889f62b489c0a62e9d25),
						"arom2.bin",CRC(aba8fd98) SHA1(81b8af4d2d8e40b5b44f114c095371afe5539549))

GTS3_ROMEND
CORE_GAMEDEFNV(cactjack,"Cactus Jack's",1991,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Class of 1812 (#730)
/-------------------------------------------------------------------*/
INITGAME(clas1812, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(clas1812,	"gprom.bin",CRC(564349bf) SHA1(458eb2ece924a20d309dce7117c94e75b4a21fd7))
GTS3SOUND32128(			"yrom1.bin",CRC(4ecf6ecb) SHA1(92469ccdedcc8e61edcddaedd688ef990a9ad5ad),
						"drom1.bin",CRC(3863a9df) SHA1(1759abbfcb127a6909f70845f41daf3ac8e80cef),
 						"arom1.bin",CRC(357b0069) SHA1(870b0b84c6b3754f89b4e4e0b4594613ef589204),
						"arom2.bin",CRC(5be02ff7) SHA1(51af73a26bbed0915ec57cde8f9cac552978b2dc))
GTS3_ROMEND
CORE_GAMEDEFNV(clas1812,"Class of 1812",1991,"Gottlieb",mGTS3S,0)

/************************************************/
/*Start of 2nd generation Alpha Numeric Hardware*/
/************************************************/

/*-------------------------------------------------------------------
/ Surf'n Safari (#731)
/-------------------------------------------------------------------*/
INITGAME1(surfnsaf, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(surfnsaf,	"gprom.bin",CRC(ac3393bd) SHA1(f9c533b937b5ca5698b805ed6ed573cb22383d9d))
GTS3SOUND32256A(		"yrom1.bin",CRC(a0480418) SHA1(a982564d5dbf52275c2e7223687b07cf4ca0a115),
						"drom1.bin",CRC(ec8fc963) SHA1(247e76d87beb3339e7d55292f9eadd2351621cfa),
						"arom1.bin",CRC(38b569b2) SHA1(93be47916a92541d097233b60a42eb7ca587ce52),
						"arom2.bin",CRC(224c2021) SHA1(6b426097a2870b3b32d786be6e66ba6be9f54c29))
GTS3_ROMEND
CORE_GAMEDEFNV(surfnsaf,"Surf'n Safari",1991,"Gottlieb",mGTS3BS,0)

/*-------------------------------------------------------------------
/ Operation Thunder (#732)
/-------------------------------------------------------------------*/
INITGAME1(opthund, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(opthund,	"gprom.bin",CRC(96a128c2) SHA1(4032c5191b167a0498371207666a1f73155b7a74))
GTS3SOUND32256A(		"yrom1.bin",CRC(169816d1) SHA1(d23b1d8d1b841ca065a485e80805ecc6342ce57b),
						"drom1.bin",CRC(db28be69) SHA1(6c505c34c8bdccc43dd8f310f01dd3a6b49e8059),
						"arom1.bin",CRC(0fbb130a) SHA1(a171c20f861dac5918c5b410e2a2bdd6e7c0553b),
						"arom2.bin",CRC(0f7632b3) SHA1(a122a062448139d5c1a9daa7d827c3073aa194f7))
GTS3_ROMEND
CORE_GAMEDEFNV(opthund,"Operation Thunder",1992,"Gottlieb",mGTS3BS,0)

/*************************
 ***Start of DMD 128x32***
 *************************/

/*-------------------------------------------------------------------
/ Super Mario Brothers (#733) - Only one dsprom dump seems to work?
/-------------------------------------------------------------------*/
INITGAME1(smb, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb,		"gprom.bin", CRC(fa1f6e52) SHA1(d7ade0e129cb399494967e025d25614bf1650db7))
//GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(59639112) SHA1(e0cbd3a38cd99f0f5f6e85d7eee4772f462c1990))
GTS3_DMD256_ROMSTART(	"dsprom2.bin",CRC(181e8234) SHA1(9b22681f61cae401269a88c3cfd783d683390877))
GTS3SOUND32256(			"yrom1.bin",CRC(e1379106) SHA1(10c46bad7cbae528716c5ba0709bb1fd3574a0a8),
						"drom1.bin",CRC(6f1d0a3e) SHA1(c7f665d79b9073f28f90debde16cafa9ab57a47c),
						"arom1.bin",CRC(e9cef116) SHA1(5f710bc24e1a168f296a22417aebecbde3bfaa5c),
						"arom2.bin",CRC(0acdfd49) SHA1(0baabd32b546842bc5c76a61b509b558677b50f9))
GTS3_ROMEND
CORE_GAMEDEFNV(smb,"Super Mario Brothers",1992,"Gottlieb",mGTS3DMDSA, 0)

INITGAME1(smb1, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb1,		"gprom1.bin", CRC(1d8c4df8) SHA1(e301bf3b2a8ed6ef902fe15b890b4c06c4606aa9))
//GTS3_DMD256_ROMSTART(	"dsprom1.bin",CRC(1363af26) SHA1(28c3eb62ea2dd13dd3e18158e91e4f51abca4f55))
GTS3_DMD256_ROMSTART(	"dsprom2.bin",CRC(181e8234) SHA1(9b22681f61cae401269a88c3cfd783d683390877))
GTS3SOUND32256(			"yrom1.bin",CRC(e1379106) SHA1(10c46bad7cbae528716c5ba0709bb1fd3574a0a8),
						"drom1.bin",CRC(6f1d0a3e) SHA1(c7f665d79b9073f28f90debde16cafa9ab57a47c),
						"arom1.bin",CRC(e9cef116) SHA1(5f710bc24e1a168f296a22417aebecbde3bfaa5c),
						"arom2.bin",CRC(0acdfd49) SHA1(0baabd32b546842bc5c76a61b509b558677b50f9))
GTS3_ROMEND
CORE_CLONEDEFNV(smb1,smb,"Super Mario Brothers (rev.1)",1992,"Gottlieb",mGTS3DMDSA, 0)

INITGAME1(smb2, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb2,		"gprom2.bin", CRC(5b0f44c4) SHA1(ca9b0cd82c75612c85c956497c8f9c12992f6ad5))
GTS3_DMD256_ROMSTART(	"dsprom2.bin",CRC(181e8234) SHA1(9b22681f61cae401269a88c3cfd783d683390877))
GTS3SOUND32256(			"yrom1.bin",CRC(e1379106) SHA1(10c46bad7cbae528716c5ba0709bb1fd3574a0a8),
						"drom1.bin",CRC(6f1d0a3e) SHA1(c7f665d79b9073f28f90debde16cafa9ab57a47c),
						"arom1.bin",CRC(e9cef116) SHA1(5f710bc24e1a168f296a22417aebecbde3bfaa5c),
						"arom2.bin",CRC(0acdfd49) SHA1(0baabd32b546842bc5c76a61b509b558677b50f9))
GTS3_ROMEND
CORE_CLONEDEFNV(smb2,smb,"Super Mario Brothers (rev.2)",1992,"Gottlieb",mGTS3DMDSA, 0)

INITGAME1(smb3, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb3,		"gprom3.bin", CRC(5a40822c) SHA1(a87ec6307f848483c76141e47fd67e4549f9c9d3))
GTS3_DMD256_ROMSTART(	"dsprom2.bin",CRC(181e8234) SHA1(9b22681f61cae401269a88c3cfd783d683390877))
//GTS3_DMD256_ROMSTART(	"dsprom3.bin",CRC(40e42f12) SHA1(d053e13e28c73e68e75beb9745947ec72201cead))
GTS3SOUND32256(			"yrom1.bin",CRC(e1379106) SHA1(10c46bad7cbae528716c5ba0709bb1fd3574a0a8),
						"drom1.bin",CRC(6f1d0a3e) SHA1(c7f665d79b9073f28f90debde16cafa9ab57a47c),
						"arom1.bin",CRC(e9cef116) SHA1(5f710bc24e1a168f296a22417aebecbde3bfaa5c),
						"arom2.bin",CRC(0acdfd49) SHA1(0baabd32b546842bc5c76a61b509b558677b50f9))
GTS3_ROMEND
CORE_CLONEDEFNV(smb3,smb,"Super Mario Brothers (rev.3)",1992,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Super Mario Brothers Mushroom World
/-------------------------------------------------------------------*/
INITGAME1(smbmush, DMD, FLIP8182, 2, SNDBRD_GTS3, 5)
GTS3ROMSTART(smbmush,	"gprom.bin", CRC(45f6d0cc) SHA1(a73c71ab64aee293ae46e65c34d70840296778d4))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(dda6c8be) SHA1(b64f73b81afe973674f9543a704b498e31d26c12))
GTS3SOUND32256(			"yrom1.bin",CRC(09712c37) SHA1(e2ee902ea6eac3e6257880949bd07a90de08e7b9),
						"drom1.bin",CRC(6f04a0ac) SHA1(53bbc182a3bd635ad18504692a4454994daef7ef),
						"arom1.bin",CRC(edce7951) SHA1(4a80d6367a5bebf9fee181456280619aa64b441f),
						"arom2.bin",CRC(dd7ea212) SHA1(adaf0262e315c26b1f4d6365e9d465c7afb6984d))
GTS3_ROMEND
CORE_GAMEDEFNV(smbmush,"Super Mario Brothers Mushroom World",1992,"Gottlieb",mGTS3DMDS,0)

/*-------------------------------------------------------------------
/ Cue Ball Wizard
/-------------------------------------------------------------------*/
INITGAME2(cueball, DMD, FLIP8182, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(cueball,	"gprom.bin",CRC(3437fdd8) SHA1(2a0fc9bc8e3d0c430ce2cf8afad378fc93af609d))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(3cc7f470) SHA1(6adf8ac2ff93eb19c7b1dbbcf8fff6cd926dc563))
GTS3SOUND32256(			"yrom1.bin",CRC(c22f5cc5) SHA1(a5bfbc1824bc483eecc961851bd411cb0dbcdc4a),
						"drom1.bin",CRC(9fd04109) SHA1(27864fe4e9c248dce6221c9e56861967d089b216),
						"arom1.bin",CRC(476bb11c) SHA1(ce546df59933cc230a6671dec493bbbe71146dee),
						"arom2.bin",CRC(23708ad9) SHA1(156fcb19403f9845404af1a4ac4edfd3fcde601d))
GTS3_ROMEND
CORE_GAMEDEFNV(cueball,"Cue Ball Wizard",1992,"Gottlieb",mGTS3DMDSA, 0)

/************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST */
/************************************************/

/*-------------------------------------------------------------------
/ Street Fighter 2
/-------------------------------------------------------------------*/
INITGAME2(sfight2, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2,	"gprom.bin", CRC(299ad173) SHA1(95cca8c22cfabc55175a49b0439fc7858bdec1bd))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(e565e5e9) SHA1(c37abf28918feb38bbad6ebb610023d52ba96957))
GTS3SOUND32256(			"yrom1.bin",CRC(9009f461) SHA1(589d94a9ae2269175be9f71b1946107bb85620ee),
						"drom1.bin",CRC(f5c13e80) SHA1(4dd3d35c25e3cb92d6000e463ddce564e112c108),
						"arom1.bin",CRC(8518ff55) SHA1(b31678aa7c1b1240becf0ae0af05b30f7df4a491),
						"arom2.bin",CRC(85a304d9) SHA1(71141dea44e4117cad66089c7a0806de1be1a96a))
GTS3_ROMEND
CORE_GAMEDEFNV(sfight2,"Street Fighter 2",1993,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(sfight2a, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2a,  "gprom1.bin", CRC(5b42c332) SHA1(958e9fe09e587038dc282fc2f276608ef3744b1d))
GTS3_DMD512_ROMSTART(	"dsprom2.bin",CRC(80eb7513) SHA1(d13d44545c7b177e27b596bac6eba173b34a017b))
GTS3SOUND32256(			"yrom1.bin",CRC(9009f461) SHA1(589d94a9ae2269175be9f71b1946107bb85620ee),
						"drom1.bin",CRC(f5c13e80) SHA1(4dd3d35c25e3cb92d6000e463ddce564e112c108),
						"arom1.bin",CRC(8518ff55) SHA1(b31678aa7c1b1240becf0ae0af05b30f7df4a491),
						"arom2.bin",CRC(85a304d9) SHA1(71141dea44e4117cad66089c7a0806de1be1a96a))
GTS3_ROMEND
CORE_CLONEDEFNV(sfight2a,sfight2,"Street Fighter 2 (rev.1)",1993,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(sfight2b, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2b,  "gprom2.bin", CRC(26d24c06) SHA1(c706bd6b2bd5b9ad6a6fb69178169977a54107b5))
GTS3_DMD512_ROMSTART(	"dsprom2.bin",CRC(80eb7513) SHA1(d13d44545c7b177e27b596bac6eba173b34a017b))
GTS3SOUND32256(			"yrom1.bin",CRC(9009f461) SHA1(589d94a9ae2269175be9f71b1946107bb85620ee),
						"drom1.bin",CRC(f5c13e80) SHA1(4dd3d35c25e3cb92d6000e463ddce564e112c108),
						"arom1.bin",CRC(8518ff55) SHA1(b31678aa7c1b1240becf0ae0af05b30f7df4a491),
						"arom2.bin",CRC(85a304d9) SHA1(71141dea44e4117cad66089c7a0806de1be1a96a))
GTS3_ROMEND
CORE_CLONEDEFNV(sfight2b,sfight2,"Street Fighter 2 (rev.2)",1993,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Teed Off
/-------------------------------------------------------------------*/
INITGAME2(teedoff, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(teedoff,	"gprom.bin", CRC(0620365b) SHA1(18887c49a5d3806b725fa6289e50db82974c0f40))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(340b8a49) SHA1(3ac76faf920b00b77c77023c42595307840ed3a7))
GTS3SOUND32256(			"yrom1.bin",CRC(c51d98d8) SHA1(9387a39a03ca90bc8eaddc0c2df8874067a22dea),
						"drom1.bin",CRC(3868e77a) SHA1(2db91c527803a369ca659eaae6022667a126d2ef),
						"arom1.bin",CRC(9e442b71) SHA1(889023af42a2527a51343ccee7f66b089b6e6d01),
						"arom2.bin",CRC(3dad9508) SHA1(70ed49fa82dbe7586bfca72c5020834f9173d563))
GTS3_ROMEND
CORE_GAMEDEFNV(teedoff,"Teed Off",1993,"Gottlieb",mGTS3DMDSA, 0)

INITGAME2(teedoff1, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(teedoff1,	"gprom1.bin", CRC(95760ab1) SHA1(9342128e2de4e81c4b0cfc482bb0650434a04bee))
GTS3_DMD512_ROMSTART(	"dsprom1.bin",CRC(24f10ad2) SHA1(15f44f69d39ca9782410a75070edf348f64dba62))
GTS3SOUND32256(			"yrom1.bin",CRC(c51d98d8) SHA1(9387a39a03ca90bc8eaddc0c2df8874067a22dea),
						"drom1.bin",CRC(3868e77a) SHA1(2db91c527803a369ca659eaae6022667a126d2ef),
						"arom1.bin",CRC(9e442b71) SHA1(889023af42a2527a51343ccee7f66b089b6e6d01),
						"arom2.bin",CRC(3dad9508) SHA1(70ed49fa82dbe7586bfca72c5020834f9173d563))
GTS3_ROMEND
CORE_CLONEDEFNV(teedoff1,teedoff,"Teed Off (rev.1)",1993,"Gottlieb",mGTS3DMDSA, 0)

INITGAME2(teedoff3, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(teedoff3,	"gprom3.bin", CRC(d7008579) SHA1(b7bc9f54340ffb2d684b5df80624e8c01e7fa18b))
GTS3_DMD512_ROMSTART(	"dsprom1.bin",CRC(24f10ad2) SHA1(15f44f69d39ca9782410a75070edf348f64dba62))
GTS3SOUND32256(			"yrom1.bin",CRC(c51d98d8) SHA1(9387a39a03ca90bc8eaddc0c2df8874067a22dea),
						"drom1.bin",CRC(3868e77a) SHA1(2db91c527803a369ca659eaae6022667a126d2ef),
						"arom1.bin",CRC(9e442b71) SHA1(889023af42a2527a51343ccee7f66b089b6e6d01),
						"arom2.bin",CRC(3dad9508) SHA1(70ed49fa82dbe7586bfca72c5020834f9173d563))
GTS3_ROMEND
CORE_CLONEDEFNV(teedoff3,teedoff,"Teed Off (rev.3)",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Wipeout
/-------------------------------------------------------------------*/
INITGAME2(wipeout, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(wipeout,	"gprom.bin",CRC(1161cdb7) SHA1(fdf4c0abb70a41149c69bd55c613849a662944d3))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(cbdec3ab) SHA1(2d70d436783830bf074a7a0590d5c48432136595))
GTS3SOUND32256(			"yrom1.bin",CRC(f08e6d7f) SHA1(284214ac80735ddd36933ecd60debc7aea18403c),
						"drom1.bin",CRC(98ae6da4) SHA1(3842c2c4e708a5deae6b5d9407694d337b62384f),
						"arom1.bin",CRC(cccdf23a) SHA1(1b1e31f04cd60d64f0b9b8ab2c6169dacd0bce69),
						"arom2.bin",CRC(d4cc44a1) SHA1(c68264f00efa9f219fc257061ed39cd789e94126))
GTS3_ROMEND
CORE_GAMEDEFNV(wipeout,"Wipeout (rev.2)",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Gladiators
/-------------------------------------------------------------------*/
INITGAME2(gladiatr, DMD, FLIP8283, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(gladiatr,	"gprom.bin", CRC(40386cf5) SHA1(3139e3707971a708ad98c735deec7e4ee7bb36cd))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(fdc8baed) SHA1(d8ad96665cd9d8b2a6ce94653753c692384685ff))
GTS3SOUND32256(			"yrom1.bin",CRC(c5b72153) SHA1(c5d94f3fa815fc33952107c3a3ad698c3c443ce3),
						"drom1.bin",CRC(60779d60) SHA1(2fa09c65ddd6cf638382229062a48163e8972136),
						"arom1.bin",CRC(85cbdda7) SHA1(4eaea8866cb281034e30f425e864419fdb58081f),
						"arom2.bin",CRC(da2c1073) SHA1(faf58099e78dffdce5c15f393ffa3707ec80dd51))
GTS3_ROMEND
CORE_GAMEDEFNV(gladiatr,"Gladiators",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ World Challenge Soccer
/-------------------------------------------------------------------*/
INITGAME2(wcsoccer, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(wcsoccer,	"gprom.bin", CRC(6382c32e) SHA1(e212f4a9a77d1cf089acb226a8079ac4cae8a96d))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(71ba5263) SHA1(e86c2cc89d31534fb2d9d24fab2fcdb0af7cc73d))
GTS3SOUND32256(			"yrom1.bin",CRC(8b2795b0) SHA1(b838d4e410c815421099c65b0d3b22227dae17c6),
						"drom1.bin",CRC(18d5edf3) SHA1(7d0d46506cf9d4b96b9b93139e3c65643e120c28),
						"arom1.bin",CRC(ece4eebf) SHA1(78f882668967194bd547ace5d22083faeb29ef5e),
						"arom2.bin",CRC(4e466500) SHA1(78c4b41a174d82a7e0e7775713c76e679c8a7e89))
GTS3_ROMEND
CORE_GAMEDEFNV(wcsoccer,"World Challenge Soccer (rev.1)",1994,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(wcsoccd2, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(wcsoccd2,  "gprom.bin", CRC(6382c32e) SHA1(e212f4a9a77d1cf089acb226a8079ac4cae8a96d))
GTS3_DMD512_ROMSTART(   "dsprom2.bin",CRC(4c8ea71d) SHA1(ce751b84e2033e4de2f2c57490867ecafd423aaa))
GTS3SOUND32256(         "yrom1.bin",CRC(8b2795b0) SHA1(b838d4e410c815421099c65b0d3b22227dae17c6),
                        "drom1.bin",CRC(18d5edf3) SHA1(7d0d46506cf9d4b96b9b93139e3c65643e120c28),
                        "arom1.bin",CRC(ece4eebf) SHA1(78f882668967194bd547ace5d22083faeb29ef5e),
                        "arom2.bin",CRC(4e466500) SHA1(78c4b41a174d82a7e0e7775713c76e679c8a7e89))
GTS3_ROMEND
CORE_CLONEDEFNV(wcsoccd2,wcsoccer,"World Challenge Soccer (disp.rev.2)",1994,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Rescue 911
/-------------------------------------------------------------------*/
INITGAME2(rescu911, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(rescu911,	"gprom.bin", CRC(943a7597) SHA1(dcf4151727efa64e8740202b68fc8e76098ff8dd))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(9657ebd5) SHA1(b716daa71f8ec4332bf338f1f976425b6ec781ab))
GTS3SOUND32512256(		"yrom1.bin",CRC(14f86b56) SHA1(2364c284412eba719f88d50dcf47d5482365dbf3),
						"drom1.bin",CRC(034c6bc3) SHA1(c483690a6e4ce533b8939e27547175c301316172),
						"arom1.bin",CRC(f6daa16c) SHA1(be132072b27a94f61653de0a22eecc8b90db3077),
						"arom2.bin",CRC(59374104) SHA1(8ad7f5f0109771dd5cebe13e80f8e1a9420f4447))
GTS3_ROMEND
CORE_GAMEDEFNV(rescu911,"Rescue 911 (rev.1)",1994,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Freddy: A Nightmare on Elm Street
/-------------------------------------------------------------------*/
INITGAME2(freddy, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(freddy,	"gprom.bin", CRC(f0a6f3e6) SHA1(ad9af12260b8adc639fa00de49366b1016df49ed))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(d78d0fa3) SHA1(132c05e71cf5ad53184f044873fb3dd71f6da15f))
GTS3SOUND32512256(		"yrom1.bin",CRC(4a748665) SHA1(9f08b6d0731390c306194808226d2e99fbe9122d),
						"drom1.bin",CRC(d472210c) SHA1(4607e6f928cb9a5f41175210ba0427b6cd50fb83),
						"arom1.bin",CRC(6bec0567) SHA1(510c0e5a5af7573761a69bad5ab36f0019767c48),
						"arom2.bin",CRC(f0e9284d) SHA1(6ffe8286e27b0eecab9620ca613e3d72bb7f77ce))
GTS3_ROMEND
CORE_GAMEDEFNV(freddy,"Freddy: A Nightmare on Elm Street (rev.3)",1994,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Shaq Attaq
/-------------------------------------------------------------------*/
static struct core_dispLayout dispShaq[] = {
  DISP_SEG_IMPORT(GTS3_dispDMD),
  {7, 1, 0,3,CORE_SEG7}, {7, 9, 3,3,CORE_SEG7}, {7,17, 6,3,CORE_SEG7}, {7,25, 9,3,CORE_SEG7}, {0}
};
INITGAME2(shaqattq, dispShaq, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(shaqattq,	"gprom.bin", CRC(7a967fd1) SHA1(c06e2aad9452150d92cfd3ba37b8e4a932cf4324))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(d6cca842) SHA1(0498ab558d252e42dee9636e6736d159c7d06275))
GTS3SOUND32512256(		"yrom1.bin",CRC(e81e2928) SHA1(4bfe57efa99bb762e4de6c7e88e79b8c5ff57626),
						"drom1.bin",CRC(16a03261) SHA1(25f5a3d32d2ec80766381106445fd624360fea78),
						"arom1.bin",CRC(019014ec) SHA1(808a8c3154fca6218fe991b46a2525926d8e51f9),
						"arom2.bin",CRC(cc5f157d) SHA1(81c3dadff1bbf37a1f091ea77d9061879be7d99c))
GTS3_ROMEND
CORE_GAMEDEFNV(shaqattq,"Shaq Attaq (rev.5)",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(shaqatt2, dispShaq, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(shaqatt2,	"gprom2.bin",CRC(494b5cec) SHA1(91511eb9f8b0182ffeff5301fb5bcf4ee9056b3f))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(d6cca842) SHA1(0498ab558d252e42dee9636e6736d159c7d06275))
GTS3SOUND32512256(		"yrom1.bin",CRC(e81e2928) SHA1(4bfe57efa99bb762e4de6c7e88e79b8c5ff57626),
						"drom1.bin",CRC(16a03261) SHA1(25f5a3d32d2ec80766381106445fd624360fea78),
						"arom1.bin",CRC(019014ec) SHA1(808a8c3154fca6218fe991b46a2525926d8e51f9),
						"arom2.bin",CRC(cc5f157d) SHA1(81c3dadff1bbf37a1f091ea77d9061879be7d99c))
GTS3_ROMEND
CORE_CLONEDEFNV(shaqatt2,shaqattq,"Shaq Attaq (rev.2)",1995,"Gottlieb",mGTS3DMDS, 0)

/************************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST & UTILITIES */
/************************************************************/

/*-------------------------------------------------------------------
/ Stargate
/-------------------------------------------------------------------*/
INITGAME2(stargate, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargate,	"gprom.bin", CRC(837e4354) SHA1(b7d1e270309b3d7965dafeec7b81d2dd41e5700c))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(17b89750) SHA1(927702f88013945cb9f2ea8389800b925182c347))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4) SHA1(77fd183a10eea2e04a07edf9da14ef7aadb65f91),
						"drom1.bin",CRC(781b2b27) SHA1(06decd22b9064ee4859618a043055e0b3e3b9e04),
						"arom1.bin",CRC(a0f62605) SHA1(8c39452367150f66271371ab02be2f5a812cb954))
GTS3_ROMEND
CORE_GAMEDEFNV(stargate,"Stargate",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(stargat1, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat1,	"gprom1.bin", CRC(567ecd88) SHA1(2dc4bfbc971cc873af6ec32e5ddbbed001d2e1d2))
GTS3_DMD512_ROMSTART(	"dsprom1.bin",CRC(91c1b01a) SHA1(96eec2e9e52c8278c102f433a554327d420fe131))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4) SHA1(77fd183a10eea2e04a07edf9da14ef7aadb65f91),
						"drom1.bin",CRC(781b2b27) SHA1(06decd22b9064ee4859618a043055e0b3e3b9e04),
						"arom1.bin",CRC(a0f62605) SHA1(8c39452367150f66271371ab02be2f5a812cb954))
GTS3_ROMEND
CORE_CLONEDEFNV(stargat1,stargate,"Stargate (rev.1)",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(stargat2, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat2,	"gprom2.bin", CRC(862920f8) SHA1(cde77e7937782f2f9fe4b7fe27b56206d6f26f63))
GTS3_DMD512_ROMSTART(	"dsprom2.bin",CRC(d0205e03) SHA1(d8dea47f0fa0e46e2bd107a1f57121372fdef0d8))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4) SHA1(77fd183a10eea2e04a07edf9da14ef7aadb65f91),
						"drom1.bin",CRC(781b2b27) SHA1(06decd22b9064ee4859618a043055e0b3e3b9e04),
						"arom1.bin",CRC(a0f62605) SHA1(8c39452367150f66271371ab02be2f5a812cb954))
GTS3_ROMEND
CORE_CLONEDEFNV(stargat2,stargate,"Stargate (rev.2)",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(stargat3, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat3,	"gprom3.bin", CRC(83f0a2e7) SHA1(5d247a3329a946449e4b333b18c13e351caa230b))
GTS3_DMD512_ROMSTART(	"dsprom3.bin",CRC(db483524) SHA1(ea14e8b04c32fc403ce2ff060caed5562104a862))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4) SHA1(77fd183a10eea2e04a07edf9da14ef7aadb65f91),
						"drom1.bin",CRC(781b2b27) SHA1(06decd22b9064ee4859618a043055e0b3e3b9e04),
						"arom1.bin",CRC(a0f62605) SHA1(8c39452367150f66271371ab02be2f5a812cb954))
GTS3_ROMEND
CORE_CLONEDEFNV(stargat3,stargate,"Stargate (rev.3)",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(stargat4, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat4,	"gprom4.bin", CRC(7b8f6920) SHA1(f354593e13c30e15c25580387ef2eb9b23622c89))
GTS3_DMD512_ROMSTART(	"dsprom3.bin",CRC(db483524) SHA1(ea14e8b04c32fc403ce2ff060caed5562104a862))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4) SHA1(77fd183a10eea2e04a07edf9da14ef7aadb65f91),
						"drom1.bin",CRC(781b2b27) SHA1(06decd22b9064ee4859618a043055e0b3e3b9e04),
						"arom1.bin",CRC(a0f62605) SHA1(8c39452367150f66271371ab02be2f5a812cb954))
GTS3_ROMEND
CORE_CLONEDEFNV(stargat4,stargate,"Stargate (rev.4)",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Big Hurt
/-------------------------------------------------------------------*/
INITGAME2(bighurt, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(bighurt,	"gprom.bin", CRC(92ce9353) SHA1(479edb2e39fa610eb2854b028d3a039473e52eba))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(bbe96c5e) SHA1(4aaac8d88e739ccb22a7d87a820b14b6d40d3ff8))
GTS3SOUND32512256(		"yrom1.bin",CRC(c58941ed) SHA1(3b3545b1e8986b06238576a0cef69d3e3a59a325),
						"drom1.bin",CRC(d472210c) SHA1(4607e6f928cb9a5f41175210ba0427b6cd50fb83),
						"arom1.bin",CRC(b3def376) SHA1(94553052cfe80774affebd5b0f99512055552786),
						"arom2.bin",CRC(59789e66) SHA1(08b7f82f83c53f15cafefb009ab9833457c088cc))
GTS3_ROMEND
CORE_GAMEDEFNV(bighurt,"Big Hurt (rev.3)",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Waterworld
/-------------------------------------------------------------------*/
INITGAME2(waterwld, DMD, FLIP4142, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(waterwld,	"gprom.bin", CRC(db1fd197) SHA1(caa22f7e3f52be85da496375115933722a414ee0))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(79164099) SHA1(fa048fb7aa91cadd6c0758c570a4c74337bd7cd5))
GTS3SOUND32512(			"yrom1.bin",CRC(6dddce0a) SHA1(6ad9b023ba8632dda0a4e04a4f66aac52ddd3b09),
						"drom1.bin",CRC(2a8c5d04) SHA1(1a6a698fc05a199923721e91e68aaaa8d3c6a3c2),
						"arom1.bin",CRC(3ee37668) SHA1(9ced05b4f060568bf686974bc2472ff7c05a87c6),
						"arom2.bin",CRC(a631bf12) SHA1(4784da1fabd2858b2c47af71784eb475cbbb4ab5))
GTS3_ROMEND
CORE_GAMEDEFNV(waterwld,"Waterworld (rev.3)",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(waterwl2, DMD, FLIP4142, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(waterwl2,	"gprom2.bin",CRC(c3d64cd7) SHA1(63bfd26fdc7082c2bb60c978508820442ac90f14))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(79164099) SHA1(fa048fb7aa91cadd6c0758c570a4c74337bd7cd5))
GTS3SOUND32512(			"yrom1.bin",CRC(6dddce0a) SHA1(6ad9b023ba8632dda0a4e04a4f66aac52ddd3b09),
						"drom1.bin",CRC(2a8c5d04) SHA1(1a6a698fc05a199923721e91e68aaaa8d3c6a3c2),
						"arom1.bin",CRC(3ee37668) SHA1(9ced05b4f060568bf686974bc2472ff7c05a87c6),
						"arom2.bin",CRC(a631bf12) SHA1(4784da1fabd2858b2c47af71784eb475cbbb4ab5))
GTS3_ROMEND
CORE_CLONEDEFNV(waterwl2,waterwld,"Waterworld (rev.2)",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Mario Andretti
/-------------------------------------------------------------------*/
static struct core_dispLayout dispAndretti[] = {
  DISP_SEG_IMPORT(GTS3_dispDMD),
  { 7, 0,0,2,CORE_SEG7}, {10, 0,2,2,CORE_SEG7}, {13, 0,4,2,CORE_SEG7}, {0}
};
INITGAME2(andretti, dispAndretti, FLIP8283, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(andretti,	"gprom.bin", CRC(cffa788d) SHA1(84646880b09dce73a42a6d87666897f6bd74a8f9))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(1f70baae) SHA1(cf07bb057093b2bd18e6ee45009245ea62094e53))
GTS3SOUND32512256(		"yrom1.bin",CRC(4ffb15b0) SHA1(de4e9b2ccca865deb2595320015a149246795260),
						"drom1.bin",CRC(d472210c) SHA1(4607e6f928cb9a5f41175210ba0427b6cd50fb83),
						"arom1.bin",CRC(918c3270) SHA1(aa57d3bfba01e701b02ca7e4f0946144cfb7d4b1),
						"arom2.bin",CRC(3c61a2f7) SHA1(65cfb5d1261a1b0c219e1786b6635d7b0a188040))
GTS3_ROMEND
CORE_GAMEDEFNV(andretti,"Mario Andretti",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(andrett4, dispAndretti, FLIP8283, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(andrett4,  "gpromt4.bin", CRC(c6f6a23b) SHA1(01ea23a830be1e86f5ecd27d6d56c1c6d5ff3176))
GTS3_DMD512_ROMSTART(   "dsprom.bin",CRC(1f70baae) SHA1(cf07bb057093b2bd18e6ee45009245ea62094e53))
GTS3SOUND32512256(      "yrom1.bin",CRC(4ffb15b0) SHA1(de4e9b2ccca865deb2595320015a149246795260),
                        "drom1.bin",CRC(d472210c) SHA1(4607e6f928cb9a5f41175210ba0427b6cd50fb83),
                        "arom1.bin",CRC(918c3270) SHA1(aa57d3bfba01e701b02ca7e4f0946144cfb7d4b1),
                        "arom2.bin",CRC(3c61a2f7) SHA1(65cfb5d1261a1b0c219e1786b6635d7b0a188040))
GTS3_ROMEND
CORE_CLONEDEFNV(andrett4,andretti,"Mario Andretti (rev.T4)",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Barb Wire
/-------------------------------------------------------------------*/
INITGAME2(barbwire, DMD, FLIP4243, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(barbwire,	"gprom.bin", CRC(2e130835) SHA1(f615eaf1c48851d837c57c17c038cc1d0806f6f7))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(2b9533cd) SHA1(2b154550006e37a9dd1acb0cb832535415a7266b))
GTS3SOUND32512256(		"yrom1.bin",CRC(7c602a35) SHA1(66dbd7679973683c8346836c28c02ff922d17375),
						"drom1.bin",CRC(ebde41b0) SHA1(38a132f815a5270dff58a5e34f5c73701d6e214d),
						"arom1.bin",CRC(7171bc86) SHA1(d9b1f54d34400490c219ca3ba566cc40cac517d7),
						"arom2.bin",CRC(ce83c6c3) SHA1(95a364844525548d28f78d54f9d058728cebf089))
GTS3_ROMEND
CORE_GAMEDEFNV(barbwire,"Barb Wire",1996,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Brooks & Dunn (#749)
/-------------------------------------------------------------------*/
INITGAME2(brooks, DMD, FLIP4243, 4/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(brooks,	"gprom.bin", CRC(26cebf07) SHA1(14741e2d216528f176dc35ade856baffab0f99a0))
GTS3_DMD512_ROMSTART(   "dsprom.bin",NO_DUMP)
GTS3SOUND32512256(      "yrom1.bin",NO_DUMP,
						"drom1.bin",NO_DUMP,
						"arom1.bin",NO_DUMP,
						"arom2.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(brooks,"Brooks & Dunn (rev.T1)",1996,"Gottlieb",mGTS3DMDSA, 0)

// other manufacturers

/*-------------------------------------------------------------------
/ machinaZOIS by www.aksioma.org (Shaq Attaq conversion)
/-------------------------------------------------------------------*/
INITGAME2(mac_zois, dispShaq, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(mac_zois,	"gprom.bin", CRC(7a967fd1) SHA1(c06e2aad9452150d92cfd3ba37b8e4a932cf4324))
GTS3_DMD512_ROMSTART(	"dsprommz.bin",CRC(3b63f9c6) SHA1(b06ea3b8f7d3c4b22a8bbc687698654366c35f22))
GTS3SOUND32512256(		"yrom1.bin",CRC(e81e2928) SHA1(4bfe57efa99bb762e4de6c7e88e79b8c5ff57626),
						"drom1.bin",CRC(16a03261) SHA1(25f5a3d32d2ec80766381106445fd624360fea78),
						"arom1mz.bin",CRC(68ceeb43) SHA1(debe5a0683b1806c9813ba89a6438afb3eecb188),
						"arom2mz.bin",CRC(7dabc8ca) SHA1(ca6dc59891222f8534b0a2de8cd29c52e5b33efc))
GTS3_ROMEND
CORE_CLONEDEFNV(mac_zois,shaqattq,"machinaZOIS Virtual Training Center",2003,"Aksioma",mGTS3DMDS, 0)
