#include "driver.h"
#include "sim.h"
#include "se.h"

/* NO OUTPUT */

//MARTIN: Doing this causes pinmamew to always use COMPACTDMD option..
//static core_tLCDLayout se_dmd128x32[] = {
//  {0,0, 32,128, CORE_DMD}, {0}
//};

#define se_dmd128x32 0

#define INITGAME(name, gen, balls) \
static core_tGameData name##GameData = { \
  gen, se_dmd128x32, \
  {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 2} \
}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
SE_INPUT_PORTS_START(name, balls) SE_INPUT_PORTS_END

#ifndef VPINMAME
   #define GAME_NOCRC 0
#endif


/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Apollo 13
/-------------------------------------------------------------------*/
INITGAME(apollo13,GEN_WS,13)
SE128_ROMSTART(apollo13,"apolcpu.501",0x5afb8801)
SE_DMD524_ROMSTART(     "a13dspa.500",0xbf8e3249)
SE_ROMEND
CORE_GAMEDEFNV(apollo13,"Apollo 13",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Goldeneye
/-------------------------------------------------------------------*/
INITGAME(gldneye,GEN_WS,3/*?*/)
SE128_ROMSTART(gldneye, "bondcpu.404",0x5aa6ffcc)
SE_DMD524_ROMSTART(	"bondispa.400",0x9cc0c710)
SE_ROMEND
CORE_GAMEDEFNV(gldneye,"Goldeneye",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Twister
/-------------------------------------------------------------------*/
INITGAME(twister,GEN_WS,3/*?*/)
SE128_ROMSTART(twister,	"twstcpu.405",0x8c3ea1a8)
SE_DMD524_ROMSTART(     "twstdspa.400",0xa6a3d41d)
SE_ROMEND
CORE_GAMEDEFNV(twister,"Twister",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ ID4: Independence Day
/-------------------------------------------------------------------*/
INITGAME(id4,GEN_WS,3/*?*/)
SE128_ROMSTART(id4, "id4cpu.202",0x108d88fd)
SE_DMD524_ROMSTART( "id4dspa.200",0x2d3fbcc4)
SE_ROMEND
CORE_GAMEDEFNV(id4,"ID4: Independence Day",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
INITGAME(spacejam,GEN_WS,3/*?*/)
SE128_ROMSTART(spacejam,"jamcpu.300",0x9dc8df2e)
SE_DMD524_ROMSTART(	"jamdspa.300",0x198e5e34)
SE_ROMEND
CORE_GAMEDEFNV(spacejam,"Space Jam",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Star Wars Trilogy
/-------------------------------------------------------------------*/
INITGAME(swtril,GEN_WS,3/*?*/)
SE128_ROMSTART(swtril,"swcpu.403",0xd022880e)
SE_DMD524_ROMSTART(   "swsedspa.400",0xb9bcbf71)
SE_ROMEND
CORE_GAMEDEFNV(swtril,"Star Wars Trilogy",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park
/-------------------------------------------------------------------*/
INITGAME(jplstwld,GEN_WS,3/*?*/)
SE128_ROMSTART(jplstwld,"jp2cpu.202",0x0d317e601)
SE_DMD524_ROMSTART(     "jp2dspa.201",0x08fc41ace)
SE_ROMEND
CORE_GAMEDEFNV(jplstwld,"The Lost World: Jurassic Park",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ X-Files
/-------------------------------------------------------------------*/
INITGAME(xfiles,GEN_WS,3/*?*/)
SE128_ROMSTART(xfiles,"xfcpu.303",0xc7ab5efe)
SE_DMD524_ROMSTART(   "xfildspa.300",0x03c96af8)
SE_ROMEND
CORE_GAMEDEFNV(xfiles,"X-Files",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Starship Troopers
/-------------------------------------------------------------------*/
INITGAME(startrp,GEN_WS,3/*?*/)
SE128_ROMSTART(startrp,	"sstcpu.201",0x549699fe)
SE_DMD524_ROMSTART(	"sstdspa.200",0x76a0e09e)
SE_ROMEND
CORE_GAMEDEFNV(startrp,"Starship Troopers",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Viper Night Drivin'
/-------------------------------------------------------------------*/
INITGAME(viprsega,GEN_WS,3/*?*/)
SE128_ROMSTART(viprsega, "vipcpu.201",0x476557aa)
SE_DMD524_ROMSTART(	 "vipdspa.201",0x24b1dc21)
SE_ROMEND
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin'",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
INITGAME(lostspc,GEN_WS, 3/*?*/)
SE128_ROMSTART(lostspc,	"liscpu.101",0x81b2ced8)
SE_DMD524_ROMSTART(	"lisdspa.102",0xe8bf4a58)
SE_ROMEND
CORE_GAMEDEFNV(lostspc,"Lost in Space",1994,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Godzilla
/-------------------------------------------------------------------*/
INITGAME(godzilla,GEN_WS,3/*?*/)
SE128_ROMSTART(godzilla,"gdzcpu.205",0x0156c21c)
SE_DMD524_ROMSTART(	"gzdspa.200",0xa254a01d)
SE_ROMEND
CORE_GAMEDEFNV(godzilla,"Godzilla",1994,"Sega",de_mSE,GAME_NOCRC)


/********************* SEGA GAMES DISTRIBUTED BY STERN  **********************/

/*-------------------------------------------------------------------
/ South Park
/-------------------------------------------------------------------*/
INITGAME(southpk,GEN_WS,3/*?*/)
SE128_ROMSTART(southpk,"spkcpu.103",0x55ca8aa1)
SE_DMD524_ROMSTART(    "spdspa.101",0x048ca598d)
SE_ROMEND
CORE_GAMEDEFNV(southpk,"South Park",1998,"Sega",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Harley Davidson
/-------------------------------------------------------------------*/
INITGAME(harley,GEN_WS,3/*?*/)
SE128_ROMSTART(harley,"harcpu.103",0x2a812c75)
SE_DMD524_ROMSTART(   "hddsp.100",0x0bdeac0fd)
SE_ROMEND
CORE_GAMEDEFNV(harley,"Harley Davidson",1998,"Sega",de_mSE,GAME_NOCRC)


/********************* STERN GAMES  **********************/


/*-------------------------------------------------------------------
/ Striker Extreme
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,3/*?*/)
SE128_ROMSTART(strikext,"sxcpue.101",0xeac29785)
SE_DMD524_ROMSTART(		"sxdispa.101",0x01d2cb240)
SE_ROMEND
CORE_GAMEDEFNV(strikext,"Striker Extreme",1999,"Stern",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout
/-------------------------------------------------------------------*/
INITGAME(shrkysht,GEN_WS_1,3/*?*/)
SE128_ROMSTART(shrkysht,"ssva207.bin",0x7355d65d)
SE_DMD524_ROMSTART(	"ssdispa.201",0x3360300b)
SE_ROMEND
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout",2000,"Stern",de_mSE,GAME_NOCRC)


/*-------------------------------------------------------------------
/ High Roller Casino
/-------------------------------------------------------------------*/
INITGAME(hirolcas,GEN_WS,3/*?*/)
SE128_ROMSTART(hirolcas,"hrccpu.210",0x2e3c682a)
SE_DMD524_ROMSTART(	    "hrcda200.bin",0x0642bdce7)
SE_ROMEND
CORE_GAMEDEFNV(hirolcas,"High Roller Casino",2001,"Stern",de_mSE,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers
/-------------------------------------------------------------------*/
INITGAME(austin,GEN_WS,3/*?*/)
SE128_ROMSTART(austin,"apcpu.210",0xa4ddcdca)
SE_DMD524_ROMSTART(  "apdisp-a.200",0xf3ca7fca)
SE_ROMEND
CORE_GAMEDEFNV(austin,"Austin Powers",2001,"Stern",de_mSE,GAME_NOCRC)
