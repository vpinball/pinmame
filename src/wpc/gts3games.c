#include "driver.h"
#include "sim.h"
#include "GTS3.h"
#include "gts80s.h"
#include "sndbrd.h"

/* ROM STATUS
N = No Lead, L = Have a Lead on Rom
-----------------------------------------------
(N)Lights, Camera, Action 1989 (Sound roms only)
(N)Bell Ringer 1990
(N)Nudge It 1990
(N)Hoops 1991
(N)Stargate (Bad sound rom)
??Strikes 'N Spares 1995 (????????)
*/
static struct core_dispLayout gts_128x32DMD[] = {
  {0,0,32,128,CORE_DMD,(void *)gts3_dmd128x32}, {0}
};

#define ALPHA	 GTS3_dispAlpha
#define DMD	 gts_128x32DMD
#define FLIP67   FLIP_SWNO(6,7)
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
/ Lights, Camera, Action (#720)
/-------------------------------------------------------------------*/
INITGAME(lca, ALPHA, FLIP_SW(FLIP_L), 3 /*?*/, SNDBRD_NONE, 4)
GTS3ROMSTART32(lca,	"gprom.bin",0x937a8426)
GTS3_ROMEND
CORE_GAMEDEFNV(lca,"Lights, Camera, Action",1989,"Gottlieb",mGTS3,GAME_NO_SOUND)
//62c0beda Rev 1?
//52957d70 Rev 3?

/*-------------------------------------------------------------------
/ Bell Ringer
/-------------------------------------------------------------------*/
INITGAME(bellring, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_NONE, 5)
GTS3ROMSTART(bellring,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(bellring,"Bell Ringer",1990,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Silver Slugger (#722)
/-------------------------------------------------------------------*/
INITGAME(silvslug, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(silvslug,	"gprom.bin",0xa6c524e2)
GTS3SOUND3232(			"yrom1.bin",0x20bc9797,
						"drom1.bin",0xeac3e1cc)
GTS3_ROMEND
CORE_GAMEDEFNV(silvslug,"Silver Slugger",1990,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Vegas (#723)
/-------------------------------------------------------------------*/
INITGAME(vegas, ALPHA, FLIP5051, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(vegas,	"gprom.bin",0x48189981)
GTS3SOUND3232(		"yrom1.bin",0xaf1095f1,
					"drom1.bin",0x46eb5755)
GTS3_ROMEND
CORE_GAMEDEFNV(vegas,"Vegas",1990,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Deadly Weapon (#724)
/-------------------------------------------------------------------*/
INITGAME(deadweap, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(deadweap,	"gprom.bin",0x07d84b32)
GTS3SOUND3232(			"yrom1.bin",0x93369ed3,
						"drom1.bin",0xf55dd7ec)
GTS3_ROMEND
CORE_GAMEDEFNV(deadweap,"Deadly Weapon",1990,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Title Fight (#726)
/-------------------------------------------------------------------*/
INITGAME(tfight, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(tfight,	"gprom.bin",0x43b3193a)
GTS3SOUND3232(			"yrom1.bin",0x8591d421,
						"drom1.bin",0x9514739f)
GTS3_ROMEND
CORE_GAMEDEFNV(tfight,"Title Fight",1990,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Nudge It
/-------------------------------------------------------------------*/
INITGAME(nudgeit, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_NONE, 5)
GTS3ROMSTART(nudgeit,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(nudgeit,"Nudge It",1990,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Car Hop (#725)
/-------------------------------------------------------------------*/
INITGAME(carhop, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(carhop,	"gprom.bin",0x164b2c9c)
GTS3SOUND3232(			"yrom1.bin",0x831ee812,
						"drom1.bin",0x9dec74e7)
GTS3_ROMEND
CORE_GAMEDEFNV(carhop,"Car Hop",1991,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Hoops
/-------------------------------------------------------------------*/
INITGAME(hoops, ALPHA, FLIP_SW(FLIP_L), 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(hoops,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(hoops,"Hoops",1991,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Cactus Jack (#729)
/-------------------------------------------------------------------*/
INITGAME(cactjack, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(cactjack,	"gprom.bin",0x5661ab06)
GTS3SOUND32128(			"yrom1.bin",0x4554ed0d,
						"drom1.bin",0x78c099e1,
						"arom1.bin",0xc890475f,
						"arom2.bin",0xaba8fd98)
GTS3_ROMEND
CORE_GAMEDEFNV(cactjack,"Cactus Jack",1991,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Class of 1812 (#730)
/-------------------------------------------------------------------*/
INITGAME(clas1812, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(clas1812,	"gprom.bin",0x564349bf)
GTS3SOUND32128(			"yrom1.bin",0x4ecf6ecb,
						"drom1.bin",0x3863a9df,
 						"arom1.bin",0x357b0069,
						"arom2.bin",0x5be02ff7)
GTS3_ROMEND
CORE_GAMEDEFNV(clas1812,"Class of 1812",1991,"Gottlieb",mGTS3S,GAME_IMPERFECT_SOUND)

/************************************************/
/*Start of 2nd generation Alpha Numeric Hardware*/
/************************************************/

/*-------------------------------------------------------------------
/ Surf'n Safari (#731)
/-------------------------------------------------------------------*/
INITGAME1(surfnsaf, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 4)
GTS3ROMSTART(surfnsaf,	"gprom.bin",0xac3393bd)
GTS3SOUND32256A(		"yrom1.bin",0xa0480418,
						"drom1.bin",0xec8fc963,
						"arom1.bin",0x38b569b2,
						"arom2.bin",0x224c2021)
GTS3_ROMEND
CORE_GAMEDEFNV(surfnsaf,"Surf'n Safari",1991,"Gottlieb",mGTS3BS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Operation Thunder (#732)
/-------------------------------------------------------------------*/
INITGAME1(opthund, ALPHA, FLIP67, 3/*?*/, SNDBRD_GTS3, 5)
GTS3ROMSTART(opthund,	"gprom.bin",0x96a128c2)
GTS3SOUND32256A(		"yrom1.bin",0x169816d1,
						"drom1.bin",0xdb28be69,
						"arom1.bin",0x0fbb130a,
						"arom2.bin",0x0f7632b3)
GTS3_ROMEND
CORE_GAMEDEFNV(opthund,"Operation Thunder",1992,"Gottlieb",mGTS3BS,GAME_IMPERFECT_SOUND)

/*************************
 ***Start of DMD 128x32***
 *************************/

/*-------------------------------------------------------------------
/ Super Mario Brothers
/-------------------------------------------------------------------*/
INITGAME1(smb, DMD, FLIP4547, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(smb,		"gprom.bin", 0xfa1f6e52)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0x59639112)
GTS3SOUND32256(			"yrom1.bin",0xe1379106,
						"drom1.bin",0x6f1d0a3e,
						"arom1.bin",0xe9cef116,
						"arom2.bin",0x0acdfd49)
GTS3_ROMEND
CORE_GAMEDEFNV(smb,"Super Mario Brothers",1992,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Super Mario Brothers Mushroom World
/-------------------------------------------------------------------*/
INITGAME1(smbmush, DMD, FLIP8182, 2, SNDBRD_GTS3, 5)
GTS3ROMSTART(smbmush,	"gprom.bin", 0x45f6d0cc)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0xdda6c8be)
GTS3SOUND32256(			"yrom1.bin",0x09712c37,
						"drom1.bin",0x6f04a0ac,
						"arom1.bin",0xedce7951,
						"arom2.bin",0xdd7ea212)
GTS3_ROMEND
CORE_GAMEDEFNV(smbmush,"Super Mario Brothers Mushroom World",1992,"Gottlieb",mGTS3DMDS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Cue Ball Wizard
/-------------------------------------------------------------------*/
INITGAME2(cueball, DMD, FLIP8182, 3, SNDBRD_GTS3, 4)
GTS3ROMSTART(cueball,	"gprom.bin",0x3437fdd8)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0x3cc7f470)
GTS3SOUND32256(			"yrom1.bin",0xc22f5cc5,
						"drom1.bin",0x9fd04109,
						"arom1.bin",0x476bb11c,
						"arom2.bin",0x23708ad9)
GTS3_ROMEND
CORE_GAMEDEFNV(cueball,"Cue Ball Wizard",1992,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST */
/************************************************/

/*-------------------------------------------------------------------
/ Street Fighter 2
/-------------------------------------------------------------------*/
INITGAME2(sfight2, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2,  "gprom.bin",0x299ad173)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xe565e5e9)
GTS3SOUND32256(			"yrom1.bin",0x9009f461,
						"drom1.bin",0xf5c13e80,
						"arom1.bin",0x8518ff55,
						"arom2.bin",0x85a304d9)
GTS3_ROMEND
CORE_GAMEDEFNV(sfight2,"Street Fighter 2",1993,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

INITGAME2(sfight2a, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(sfight2a,  "gprom2.bin",0x26d24c06)
GTS3_DMD512_ROMSTART(	"dsprom2.bin",0x80eb7513)
GTS3SOUND32256(			"yrom1.bin",0x9009f461,
						"drom1.bin",0xf5c13e80,
						"arom1.bin",0x8518ff55,
						"arom2.bin",0x85a304d9)
GTS3_ROMEND
CORE_CLONEDEFNV(sfight2a,sfight2,"Street Fighter 2 (V.2)",1993,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Teed Off
/-------------------------------------------------------------------*/
INITGAME2(teedoff, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(teedoff,	"gprom.bin",0xd7008579)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x24f10ad2)
GTS3SOUND32256(			"yrom1.bin",0xc51d98d8,
						"drom1.bin",0x3868e77a,
						"arom1.bin",0x9e442b71,
						"arom2.bin",0x3dad9508)
GTS3_ROMEND
CORE_GAMEDEFNV(teedoff,"Teed Off",1993,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Wipeout
/-------------------------------------------------------------------*/
INITGAME2(wipeout, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(wipeout,	"gprom.bin",0x1161cdb7)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xcbdec3ab)
GTS3SOUND32256(			"yrom1.bin",0xf08e6d7f,
						"drom1.bin",0x98ae6da4,
						"arom1.bin",0xcccdf23a,
						"arom2.bin",0xd4cc44a1)
GTS3_ROMEND
CORE_GAMEDEFNV(wipeout,"Wipeout",1993,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Gladiators
/-------------------------------------------------------------------*/
INITGAME2(gladiatr, DMD, FLIP8283, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(gladiatr,	"gprom.bin", 0x40386cf5)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xfdc8baed)
GTS3SOUND32256(			"yrom1.bin",0xc5b72153,
						"drom1.bin",0x60779d60,
						"arom1.bin",0x85cbdda7,
						"arom2.bin",0xda2c1073)
GTS3_ROMEND
CORE_GAMEDEFNV(gladiatr,"Gladiators",1993,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ World Challenge Soccer
/-------------------------------------------------------------------*/
INITGAME2(wcsoccer, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(wcsoccer,	"gprom.bin", 0x6382c32e)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x71ba5263)
GTS3SOUND32256(			"yrom1.bin",0x8b2795b0,
						"drom1.bin",0x18d5edf3,
						"arom1.bin",0xece4eebf,
						"arom2.bin",0x4e466500)
GTS3_ROMEND
CORE_GAMEDEFNV(wcsoccer,"World Challenge Soccer",1994,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Rescue 911
/-------------------------------------------------------------------*/
INITGAME2(rescu911, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(rescu911,	"gprom.bin", 0x943a7597)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x9657ebd5)
GTS3SOUND32512256(		"yrom1.bin",0x14f86b56,
						"drom1.bin",0x034c6bc3,
						"arom1.bin",0xf6daa16c,
						"arom2.bin",0x59374104)
GTS3_ROMEND
CORE_GAMEDEFNV(rescu911,"Rescue 911",1994,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Freddy: A Nightmare on Elm Street
/-------------------------------------------------------------------*/
INITGAME2(freddy, DMD, FLIP8182, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(freddy,	"gprom.bin", 0xf0a6f3e6)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xd78d0fa3)
GTS3SOUND32512256(		"yrom1.bin",0x4a748665,
						"drom1.bin",0xd472210c,
						"arom1.bin",0x6bec0567,
						"arom2.bin",0xf0e9284d)
GTS3_ROMEND
CORE_GAMEDEFNV(freddy,"Freddy: A Nightmare on Elm Street",1994,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Shaq Attaq
/-------------------------------------------------------------------*/
INITGAME2(shaqattq, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(shaqattq,	"gprom.bin", 0x7a967fd1)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xd6cca842)
GTS3SOUND32512256(		"yrom1.bin",0xe81e2928,
						"drom1.bin",0x16a03261,
						"arom1.bin",0x019014ec,
						"arom2.bin",0xcc5f157d)
GTS3_ROMEND
CORE_GAMEDEFNV(shaqattq,"Shaq Attaq",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/************************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST & UTILITIES */
/************************************************************/

/*-------------------------------------------------------------------
/ Stargate
/-------------------------------------------------------------------*/
INITGAME2(stargate, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargate,	"gprom.bin",0x567ecd88)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x91c1b01a)
GTS3SOUND32512256(		"yrom1.bin",0x53123fd4,
						"drom1.bin",0x781b2b27,
						"arom1.bin",0xa0f62605,
						"arom2.bin",0x00000000)
GTS3_ROMEND
CORE_GAMEDEFNV(stargate,"Stargate",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

INITGAME2(stargat2, DMD, FLIP8182, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(stargat2,	"gprom2.bin",0x862920f8)
GTS3_DMD512_ROMSTART(	"dsprom2.bin",0xd0205e03)
GTS3SOUND32512256(		"yrom1.bin",0x53123fd4,
						"drom1.bin",0x781b2b27,
						"arom1.bin",0xa0f62605,
						"arom2.bin",0x00000000)
GTS3_ROMEND
CORE_CLONEDEFNV(stargat2,stargate,"Stargate (V.2)",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Big Hurt
/-------------------------------------------------------------------*/
INITGAME2(bighurt, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(bighurt,	"gprom.bin", 0x92ce9353)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xbbe96c5e)
GTS3SOUND32512256(		"yrom1.bin",0xc58941ed,
						"drom1.bin",0xd472210c,
						"arom1.bin",0xb3def376,
						"arom2.bin",0x59789e66)
GTS3_ROMEND
CORE_GAMEDEFNV(bighurt,"Big Hurt",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Waterworld
/-------------------------------------------------------------------*/
INITGAME2(waterwld, DMD, FLIP4142, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(waterwld,	"gprom.bin", 0xdb1fd197)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x79164099)
GTS3SOUND32512(			"yrom1.bin",0x6dddce0a,
						"drom1.bin",0x2a8c5d04,
						"arom1.bin",0x3ee37668,
						"arom2.bin",0xa631bf12)
GTS3_ROMEND
CORE_GAMEDEFNV(waterwld,"Waterworld",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Mario Andretti
/-------------------------------------------------------------------*/
INITGAME2(andretti, DMD, FLIP8283, 4, SNDBRD_GTS3, 5)
GTS3ROMSTART(andretti,	"gprom.bin", 0xcffa788d)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x1f70baae)
GTS3SOUND32512256(		"yrom1.bin",0x4ffb15b0,
						"drom1.bin",0xd472210c,
						"arom1.bin",0x918c3270,
						"arom2.bin",0x3c61a2f7)
GTS3_ROMEND
CORE_GAMEDEFNV(andretti,"Mario Andretti",1995,"Gottlieb",mGTS3DMDS, GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Barb Wire
/-------------------------------------------------------------------*/
INITGAME2(barbwire, DMD, FLIP4243, 4, SNDBRD_GTS3, 4)
GTS3ROMSTART(barbwire,	"gprom.bin", 0x2e130835)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x2b9533cd)
GTS3SOUND32512256(		"yrom1.bin",0x7c602a35,
						"drom1.bin",0xebde41b0,
						"arom1.bin",0x7171bc86,
						"arom2.bin",0xce83c6c3)
GTS3_ROMEND
CORE_GAMEDEFNV(barbwire,"Barb Wire",1996,"Gottlieb",mGTS3DMDSA, GAME_IMPERFECT_SOUND)
