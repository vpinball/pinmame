#include "driver.h"
#include "sim.h"
#include "GTS3.h"
#include "gts80s.h"
#include "sndbrd.h"

/* ROM STATUS
N = No Lead, L = Have a Lead on Rom
-----------------------------------------------
(N)Bell Ringer 1990
(N)Nudge It 1990
(N)Hoops 1991
(L)Strikes 'N Spares 1995 (sound Y and D roms)
(?)Brooks & Dunn 1996 (display, sound)
*/
static struct core_dispLayout gts_128x32DMD[] = {
  {0,0,32,128,CORE_DMD,(void *)gts3_dmd128x32}, {0}
};

#define ALPHA	 GTS3_dispAlpha
#define DMD	 gts_128x32DMD
#define FLIP67   FLIP_SWNO(6,7)
#define FLIP2122 FLIP_SWNO(21,22)
#define FLIP4142 FLIP_SWNO(41,42)
#define FLIP4243 FLIP_SWNO(42,43)
#define FLIP4547 FLIP_SWNO(45,47)
#define FLIP5051 FLIP_SWNO(50,51)
#define FLIP8182 FLIP_SWNO(81,82)
#define FLIP8283 FLIP_SWNO(82,83)
#define GDISP_SEG_20(row,type)    {2*row, 0, 20*row, 20, type}

/* 2 X 20 AlphaNumeric Rows */
static struct core_dispLayout GTS3_dispAlpha[] = {
	GDISP_SEG_20(0,CORE_SEG16),GDISP_SEG_20(1,CORE_SEG16),{0}
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

// some games produced by Premier for International Concepts
/*-------------------------------------------------------------------
/ Caribbean Cruise (#C102)
/-------------------------------------------------------------------*/
INITGAME_IC(ccruise, ALPHA, FLIP67, 2, SNDBRD_GTS3, 4)
GTS3ROMSTART(ccruise,	"gprom.bin",CRC(668b5757))
GTS3SOUND3232(			"yrom1.bin",CRC(6e424e53),
						"drom1.bin",CRC(4480257e))
GTS3_ROMEND
CORE_GAMEDEFNV(ccruise,"Caribbean Cruise",1989,"International Concepts",mGTS3S,0)

// Night Moves 11/89

// Premier games below
/*-------------------------------------------------------------------
/ Lights, Camera, Action (#720)
/-------------------------------------------------------------------*/
INITGAME(lca, ALPHA, FLIP_SW(FLIP_L), 3 /*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART32(lca,	"gprom.bin",CRC(52957d70))
GTS3SOUND3232(		"yrom1.bin",CRC(20919ebb),
					"drom1.bin",CRC(a258d72d))
GTS3_ROMEND
CORE_GAMEDEFNV(lca,"Lights, Camera, Action",1989,"Gottlieb",mGTS3S,0)
//62c0beda Rev 1?
//937a8426 Rev 2?

/*-------------------------------------------------------------------
/ Bell Ringer
/-------------------------------------------------------------------*/
INITGAME(bellring, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_NONE, 5)
GTS3ROMSTART(bellring,	"gprom.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(bellring,"Bell Ringer",1990,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Silver Slugger (#722)
/-------------------------------------------------------------------*/
INITGAME(silvslug, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(silvslug,	"gprom.bin",CRC(a6c524e2))
GTS3SOUND3232(			"yrom1.bin",CRC(20bc9797),
						"drom1.bin",CRC(eac3e1cc))
GTS3_ROMEND
CORE_GAMEDEFNV(silvslug,"Silver Slugger",1990,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Vegas (#723)
/-------------------------------------------------------------------*/
INITGAME(vegas, ALPHA, FLIP5051, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(vegas,	"gprom.bin",CRC(48189981))
GTS3SOUND3232(		"yrom1.bin",CRC(af1095f1),
					"drom1.bin",CRC(46eb5755))
GTS3_ROMEND
CORE_GAMEDEFNV(vegas,"Vegas",1990,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Deadly Weapon (#724)
/-------------------------------------------------------------------*/
INITGAME(deadweap, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(deadweap,	"gprom.bin",CRC(07d84b32))
GTS3SOUND3232(			"yrom1.bin",CRC(93369ed3),
						"drom1.bin",CRC(f55dd7ec))
GTS3_ROMEND
CORE_GAMEDEFNV(deadweap,"Deadly Weapon",1990,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Title Fight (#726)
/-------------------------------------------------------------------*/
INITGAME(tfight, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(tfight,	"gprom.bin",CRC(43b3193a))
GTS3SOUND3232(			"yrom1.bin",CRC(8591d421),
						"drom1.bin",CRC(9514739f))
GTS3_ROMEND
CORE_GAMEDEFNV(tfight,"Title Fight",1990,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Nudge It
/-------------------------------------------------------------------*/
INITGAME(nudgeit, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_NONE, 5)
GTS3ROMSTART(nudgeit,	"gprom.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(nudgeit,"Nudge It",1990,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Car Hop (#725)
/-------------------------------------------------------------------*/
INITGAME(carhop, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(carhop,	"gprom.bin",CRC(164b2c9c))
GTS3SOUND3232(			"yrom1.bin",CRC(831ee812),
						"drom1.bin",CRC(9dec74e7))
GTS3_ROMEND
CORE_GAMEDEFNV(carhop,"Car Hop",1991,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Hoops
/-------------------------------------------------------------------*/
INITGAME(hoops, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(hoops,	"gprom.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(hoops,"Hoops",1991,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Cactus Jack (#729)
/-------------------------------------------------------------------*/
INITGAME(cactjack, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(cactjack,	"gprom.bin",CRC(5661ab06))
GTS3SOUND32128(			"yrom1.bin",CRC(4554ed0d),
						"drom1.bin",CRC(78c099e1),
						"arom1.bin",CRC(c890475f),
						"arom2.bin",CRC(aba8fd98))
						
GTS3_ROMEND
CORE_GAMEDEFNV(cactjack,"Cactus Jack",1991,"Gottlieb",mGTS3S,0)

/*-------------------------------------------------------------------
/ Class of 1812 (#730)
/-------------------------------------------------------------------*/
INITGAME(clas1812, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(clas1812,	"gprom.bin",CRC(564349bf))
GTS3SOUND32128(			"yrom1.bin",CRC(4ecf6ecb),
						"drom1.bin",CRC(3863a9df),
 						"arom1.bin",CRC(357b0069),
						"arom2.bin",CRC(5be02ff7))
GTS3_ROMEND
CORE_GAMEDEFNV(clas1812,"Class of 1812",1991,"Gottlieb",mGTS3S,0)

/************************************************/
/*Start of 2nd generation Alpha Numeric Hardware*/
/************************************************/

/*-------------------------------------------------------------------
/ Surf'n Safari (#731)
/-------------------------------------------------------------------*/
INITGAME1(surfnsaf, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(surfnsaf,	"gprom.bin",CRC(ac3393bd))
GTS3SOUND32256A(		"yrom1.bin",CRC(a0480418),
						"drom1.bin",CRC(ec8fc963),
						"arom1.bin",CRC(38b569b2),
						"arom2.bin",CRC(224c2021))
GTS3_ROMEND
CORE_GAMEDEFNV(surfnsaf,"Surf'n Safari",1991,"Gottlieb",mGTS3BS,0)

/*-------------------------------------------------------------------
/ Operation Thunder (#732)
/-------------------------------------------------------------------*/
INITGAME1(opthund, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(opthund,	"gprom.bin",CRC(96a128c2))
GTS3SOUND32256A(		"yrom1.bin",CRC(169816d1),
						"drom1.bin",CRC(db28be69),
						"arom1.bin",CRC(0fbb130a),
						"arom2.bin",CRC(0f7632b3))
GTS3_ROMEND
CORE_GAMEDEFNV(opthund,"Operation Thunder",1992,"Gottlieb",mGTS3BS,0)

/*************************
 ***Start of DMD 128x32***
 *************************/

/*-------------------------------------------------------------------
/ Super Mario Brothers
/-------------------------------------------------------------------*/
INITGAME1(smb, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb,		"gprom.bin", CRC(fa1f6e52))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(59639112))
GTS3SOUND32256(			"yrom1.bin",CRC(e1379106),
						"drom1.bin",CRC(6f1d0a3e),
						"arom1.bin",CRC(e9cef116),
						"arom2.bin",CRC(0acdfd49))
GTS3_ROMEND
CORE_GAMEDEFNV(smb,"Super Mario Brothers",1992,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Super Mario Brothers Mushroom World
/-------------------------------------------------------------------*/
INITGAME1(smbmush, DMD, FLIP8182, 2, SNDBRD_GTS3, 5)
GTS3ROMSTART(smbmush,	"gprom.bin", CRC(45f6d0cc))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(dda6c8be))
GTS3SOUND32256(			"yrom1.bin",CRC(09712c37),
						"drom1.bin",CRC(6f04a0ac),
						"arom1.bin",CRC(edce7951),
						"arom2.bin",CRC(dd7ea212))
GTS3_ROMEND
CORE_GAMEDEFNV(smbmush,"Super Mario Brothers Mushroom World",1992,"Gottlieb",mGTS3DMDS,0)

/*-------------------------------------------------------------------
/ Cue Ball Wizard
/-------------------------------------------------------------------*/
INITGAME2(cueball, DMD, FLIP8182, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(cueball,	"gprom.bin",CRC(3437fdd8))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(3cc7f470))
GTS3SOUND32256(			"yrom1.bin",CRC(c22f5cc5),
						"drom1.bin",CRC(9fd04109),
						"arom1.bin",CRC(476bb11c),
						"arom2.bin",CRC(23708ad9))
GTS3_ROMEND
CORE_GAMEDEFNV(cueball,"Cue Ball Wizard",1992,"Gottlieb",mGTS3DMDSA, 0)

/************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST */
/************************************************/

/*-------------------------------------------------------------------
/ Street Fighter 2
/-------------------------------------------------------------------*/
INITGAME2(sfight2, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2,	"gprom.bin",CRC(299ad173))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(e565e5e9))
GTS3SOUND32256(			"yrom1.bin",CRC(9009f461),
						"drom1.bin",CRC(f5c13e80),
						"arom1.bin",CRC(8518ff55),
						"arom2.bin",CRC(85a304d9))
GTS3_ROMEND
CORE_GAMEDEFNV(sfight2,"Street Fighter 2",1993,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(sfight2a, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2a,  "gprom2.bin",CRC(26d24c06))
GTS3_DMD512_ROMSTART(	"dsprom2.bin",CRC(80eb7513))
GTS3SOUND32256(			"yrom1.bin",CRC(9009f461),
						"drom1.bin",CRC(f5c13e80),
						"arom1.bin",CRC(8518ff55),
						"arom2.bin",CRC(85a304d9))
GTS3_ROMEND
CORE_CLONEDEFNV(sfight2a,sfight2,"Street Fighter 2 (V.2)",1993,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Teed Off
/-------------------------------------------------------------------*/
INITGAME2(teedoff, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(teedoff,	"gprom.bin",CRC(d7008579))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(24f10ad2))
GTS3SOUND32256(			"yrom1.bin",CRC(c51d98d8),
						"drom1.bin",CRC(3868e77a),
						"arom1.bin",CRC(9e442b71),
						"arom2.bin",CRC(3dad9508))
GTS3_ROMEND
CORE_GAMEDEFNV(teedoff,"Teed Off",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Wipeout
/-------------------------------------------------------------------*/
INITGAME2(wipeout, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(wipeout,	"gprom.bin",CRC(1161cdb7))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(cbdec3ab))
GTS3SOUND32256(			"yrom1.bin",CRC(f08e6d7f),
						"drom1.bin",CRC(98ae6da4),
						"arom1.bin",CRC(cccdf23a),
						"arom2.bin",CRC(d4cc44a1))
GTS3_ROMEND
CORE_GAMEDEFNV(wipeout,"Wipeout",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Gladiators
/-------------------------------------------------------------------*/
INITGAME2(gladiatr, DMD, FLIP8283, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(gladiatr,	"gprom.bin", CRC(40386cf5))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(fdc8baed))
GTS3SOUND32256(			"yrom1.bin",CRC(c5b72153),
						"drom1.bin",CRC(60779d60),
						"arom1.bin",CRC(85cbdda7),
						"arom2.bin",CRC(da2c1073))
GTS3_ROMEND
CORE_GAMEDEFNV(gladiatr,"Gladiators",1993,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ World Challenge Soccer
/-------------------------------------------------------------------*/
INITGAME2(wcsoccer, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(wcsoccer,	"gprom.bin", CRC(6382c32e))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(71ba5263))
GTS3SOUND32256(			"yrom1.bin",CRC(8b2795b0),
						"drom1.bin",CRC(18d5edf3),
						"arom1.bin",CRC(ece4eebf),
						"arom2.bin",CRC(4e466500))
GTS3_ROMEND
CORE_GAMEDEFNV(wcsoccer,"World Challenge Soccer",1994,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Rescue 911
/-------------------------------------------------------------------*/
INITGAME2(rescu911, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(rescu911,	"gprom.bin", CRC(943a7597))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(9657ebd5))
GTS3SOUND32512256(		"yrom1.bin",CRC(14f86b56),
						"drom1.bin",CRC(034c6bc3),
						"arom1.bin",CRC(f6daa16c),
						"arom2.bin",CRC(59374104))
GTS3_ROMEND
CORE_GAMEDEFNV(rescu911,"Rescue 911",1994,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Freddy: A Nightmare on Elm Street
/-------------------------------------------------------------------*/
INITGAME2(freddy, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(freddy,	"gprom.bin", CRC(f0a6f3e6))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(d78d0fa3))
GTS3SOUND32512256(		"yrom1.bin",CRC(4a748665),
						"drom1.bin",CRC(d472210c),
						"arom1.bin",CRC(6bec0567),
						"arom2.bin",CRC(f0e9284d))
GTS3_ROMEND
CORE_GAMEDEFNV(freddy,"Freddy: A Nightmare on Elm Street",1994,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Shaq Attaq
/-------------------------------------------------------------------*/
INITGAME2(shaqattq, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(shaqattq,	"gprom.bin", CRC(7a967fd1))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(d6cca842))
GTS3SOUND32512256(		"yrom1.bin",CRC(e81e2928),
						"drom1.bin",CRC(16a03261),
						"arom1.bin",CRC(019014ec),
						"arom2.bin",CRC(cc5f157d))
GTS3_ROMEND
CORE_GAMEDEFNV(shaqattq,"Shaq Attaq",1995,"Gottlieb",mGTS3DMDS, 0)

/************************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST & UTILITIES */
/************************************************************/

/*-------------------------------------------------------------------
/ Stargate
/-------------------------------------------------------------------*/
INITGAME2(stargate, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargate,	"gprom.bin",CRC(567ecd88))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(91c1b01a))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4),
						"drom1.bin",CRC(781b2b27),
						"arom1.bin",CRC(a0f62605))
GTS3_ROMEND
CORE_GAMEDEFNV(stargate,"Stargate",1995,"Gottlieb",mGTS3DMDS, 0)

INITGAME2(stargat2, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat2,	"gprom2.bin",CRC(862920f8))
GTS3_DMD512_ROMSTART(	"dsprom2.bin",CRC(d0205e03))
GTS3SOUND32512A(		"yrom1.bin",CRC(53123fd4),
						"drom1.bin",CRC(781b2b27),
						"arom1.bin",CRC(a0f62605))
GTS3_ROMEND
CORE_CLONEDEFNV(stargat2,stargate,"Stargate (V.2)",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Big Hurt
/-------------------------------------------------------------------*/
INITGAME2(bighurt, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(bighurt,	"gprom.bin", CRC(92ce9353))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(bbe96c5e))
GTS3SOUND32512256(		"yrom1.bin",CRC(c58941ed),
						"drom1.bin",CRC(d472210c),
						"arom1.bin",CRC(b3def376),
						"arom2.bin",CRC(59789e66))
GTS3_ROMEND
CORE_GAMEDEFNV(bighurt,"Big Hurt",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Strikes n' Spares (#N111)
/-------------------------------------------------------------------*/
INITGAME2(snspares, DMD, FLIP2122, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(snspares,	"gprom.bin", CRC(9e018496))
GTS3_DMD256_ROMSTART(	"dsprom.bin",CRC(5c901899))
GTS3SOUND32512A(		"yrom1.bin",NO_DUMP,
						"drom1.bin",NO_DUMP,
						"arom1.bin",CRC(e248574a))
GTS3_ROMEND
CORE_GAMEDEFNV(snspares,"Strikes n' Spares",1995,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Waterworld
/-------------------------------------------------------------------*/
INITGAME2(waterwld, DMD, FLIP4142, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(waterwld,	"gprom.bin", CRC(db1fd197))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(79164099))
GTS3SOUND32512(			"yrom1.bin",CRC(6dddce0a),
						"drom1.bin",CRC(2a8c5d04),
						"arom1.bin",CRC(3ee37668),
						"arom2.bin",CRC(a631bf12))
GTS3_ROMEND
CORE_GAMEDEFNV(waterwld,"Waterworld",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Mario Andretti
/-------------------------------------------------------------------*/
INITGAME2(andretti, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(andretti,	"gprom.bin", CRC(cffa788d))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(1f70baae))
GTS3SOUND32512256(		"yrom1.bin",CRC(4ffb15b0),
						"drom1.bin",CRC(d472210c),
						"arom1.bin",CRC(918c3270),
						"arom2.bin",CRC(3c61a2f7))
GTS3_ROMEND
CORE_GAMEDEFNV(andretti,"Mario Andretti",1995,"Gottlieb",mGTS3DMDS, 0)

/*-------------------------------------------------------------------
/ Barb Wire
/-------------------------------------------------------------------*/
INITGAME2(barbwire, DMD, FLIP4243, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(barbwire,	"gprom.bin", CRC(2e130835))
GTS3_DMD512_ROMSTART(	"dsprom.bin",CRC(2b9533cd))
GTS3SOUND32512256(		"yrom1.bin",CRC(7c602a35),
						"drom1.bin",CRC(ebde41b0),
						"arom1.bin",CRC(7171bc86),
						"arom2.bin",CRC(ce83c6c3))
GTS3_ROMEND
CORE_GAMEDEFNV(barbwire,"Barb Wire",1996,"Gottlieb",mGTS3DMDSA, 0)

/*-------------------------------------------------------------------
/ Brooks & Dunn (#749)
/-------------------------------------------------------------------*/
INITGAME2(brooks, DMD, FLIP4243, 4/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(brooks,	"gprom.bin", CRC(26cebf07))
GTS3_DMD512_ROMSTART(	"dsprom.bin",NO_DUMP)
GTS3SOUND32512256(		"yrom1.bin",NO_DUMP,
						"drom1.bin",NO_DUMP,
						"arom1.bin",NO_DUMP,
						"arom2.bin",NO_DUMP)
GTS3_ROMEND
CORE_GAMEDEFNV(brooks,"Brooks & Dunn (Prototype)",1996,"Gottlieb",mGTS3DMDSA, GAME_NO_SOUND)
