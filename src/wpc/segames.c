#include "driver.h"
#include "sim.h"
#include "se.h"
#include "sesound.h"

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

//Snd Works but VERY SOFT
/*-------------------------------------------------------------------
/ Apollo 13
/-------------------------------------------------------------------*/
INITGAME(apollo13,GEN_WS,13)
SE128_ROMSTART(apollo13,"apolcpu.501",0x5afb8801)
SE_DMD524_ROMSTART(     "a13dspa.500",0xbf8e3249)
SES_SOUNDROM0888( "apollo13.u7" ,0xe58a36b8,
                  "apollo13.u17",0x4e863aca,
                  "apollo13.u21",0x28169e37,
				  "apollo13.u36",0xcede5e0f)
SE_ROMEND
CORE_GAMEDEFNV(apollo13,"Apollo 13",1994,"Sega",de_mSES3,GAME_NOCRC)

//Snd Works but VERY SOFT
/*-------------------------------------------------------------------
/ Goldeneye
/-------------------------------------------------------------------*/
INITGAME(gldneye,GEN_WS,3/*?*/)
SE128_ROMSTART(gldneye, "bondcpu.404",0x5aa6ffcc)
SE_DMD524_ROMSTART(	"bondispa.400",0x9cc0c710)
SES_SOUNDROM088 ( "bondu7.bin" ,0x7581a349,
                  "bondu17.bin",0xd9c56b9d,
                  "bondu21.bin",0x5be0f205)
SE_ROMEND
CORE_GAMEDEFNV(gldneye,"Goldeneye",1994,"Sega",de_mSES3,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Twister
/-------------------------------------------------------------------*/
INITGAME(twister,GEN_WS,3/*?*/)
SE128_ROMSTART(twister,	"twstcpu.405",0x8c3ea1a8)
SE_DMD524_ROMSTART(     "twstdspa.400",0xa6a3d41d)
SES_SOUNDROM088 ( "twst0430.u7" ,0x5ccf0798,
                  "twst0429.u17",0x0e35d640,
                  "twst0429.u21",0xc3eae590)
SE_ROMEND
CORE_GAMEDEFNV(twister,"Twister",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ ID4: Independence Day
/-------------------------------------------------------------------*/
INITGAME(id4,GEN_WS,3/*?*/)
SE128_ROMSTART(id4, "id4cpu.202",0x108d88fd)
SE_DMD524_ROMSTART( "id4dspa.200",0x2d3fbcc4)
SES_SOUNDROM088 ( "id4sndu7.512",0xdeeaed37,
                  "id4sdu17.400",0x385034f1,
                  "id4sdu21.400",0xf384a9ab)
SE_ROMEND
CORE_GAMEDEFNV(id4,"ID4: Independence Day",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
INITGAME(spacejam,GEN_WS,3/*?*/)
SE128_ROMSTART(spacejam,"jamcpu.300",0x9dc8df2e)
SE_DMD524_ROMSTART(	"jamdspa.300",0x198e5e34)
SES_SOUNDROM0888 ("spcjam.u7" ,0xc693d853,
                  "spcjam.u17",0xccefe457,
                  "spcjam.u21",0x9e7fe0a6,
				  "spcjam.u36",0x7d11e1eb)
SE_ROMEND
CORE_GAMEDEFNV(spacejam,"Space Jam",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Star Wars Trilogy
/-------------------------------------------------------------------*/
INITGAME(swtril,GEN_WS,3/*?*/)
SE128_ROMSTART(swtril,"swcpu.403",0xd022880e)
SE_DMD524_ROMSTART(   "swsedspa.400",0xb9bcbf71)
SES_SOUNDROM088 ( "sw0219.u7" ,0xcd7c84d9,
                  "sw0211.u17",0x6863e7f6,
                  "sw0211.u21",0x6be68450)
SE_ROMEND
CORE_GAMEDEFNV(swtril,"Star Wars Trilogy",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park
/-------------------------------------------------------------------*/
INITGAME(jplstwld,GEN_WS,3/*?*/)
SE128_ROMSTART(jplstwld,"jp2cpu.202",0x0d317e601)
SE_DMD524_ROMSTART(     "jp2dspa.201",0x08fc41ace)
SES_SOUNDROM088 ( "jp2_u7.bin" ,0x73b74c96,
                  "jp2_u17.bin",0x8d6c0dbd,
                  "jp2_u21.bin",0xc670a997)
SE_ROMEND
CORE_GAMEDEFNV(jplstwld,"The Lost World: Jurassic Park",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ X-Files
/-------------------------------------------------------------------*/
INITGAME(xfiles,GEN_WS,3/*?*/)
SE128_ROMSTART(xfiles,"xfcpu.303",0xc7ab5efe)
SE_DMD524_ROMSTART(   "xfildspa.300",0x03c96af8)
SES_SOUNDROM088 ( "xfsndu7.512" ,0x01d65239,
                  "xfsndu17.c40",0x40bfc835,
                  "xfsndu21.c40",0xb56a5ca6)
SE_ROMEND
CORE_GAMEDEFNV(xfiles,"X-Files",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Starship Troopers
/-------------------------------------------------------------------*/
INITGAME(startrp,GEN_WS,3/*?*/)
SE128_ROMSTART(startrp,	"sstcpu.201",0x549699fe)
SE_DMD524_ROMSTART(	"sstdspa.200",0x76a0e09e)
SES_SOUNDROM0888( "u7_b130.512" ,0xf1559e4f,
                  "u17_152a.040",0x8caeccdb,
                  "u21_0291.040",0x0c5321f6,
                  "u36_95a7.040",0xc1e4ca6a)
SE_ROMEND
CORE_GAMEDEFNV(startrp,"Starship Troopers",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Viper Night Drivin'
/-------------------------------------------------------------------*/
INITGAME(viprsega,GEN_WS,3/*?*/)
SE128_ROMSTART(viprsega, "vipcpu.201",0x476557aa)
SE_DMD524_ROMSTART(	 "vipdspa.201",0x24b1dc21)
SES_SOUNDROM08888("vpru7.dat" ,0xf21617d7,
                  "vpru17.dat",0x47b1317c,
                  "vpru21.dat",0x0e0e2dd6,
                  "vpru36.dat",0x7b482876,
                  "vpru37.dat",0x0bf23e0e)
SE_ROMEND
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin'",1994,"Sega",de_mSES1,GAME_NOCRC)

//Length Problems on U17,U21,U36,U37 but sounds ok.
/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
INITGAME(lostspc,GEN_WS, 3/*?*/)
SE128_ROMSTART(lostspc,	"liscpu.101",0x81b2ced8)
SE_DMD524_ROMSTART(	"lisdspa.102",0xe8bf4a58)
SES_SOUNDROM08888("lisu7.100" ,0x96e6b3c4,
                  "lisu17.100",0x4f819b0b,
                  "lisu21.100",0xeef0b9fa,
                  "lisu36.100",0xca115f5c,
                  "lisu37.100",0xb8807887)
SE_ROMEND
CORE_GAMEDEFNV(lostspc,"Lost in Space",1994,"Sega",de_mSES1,GAME_NOCRC)

//Length Problems on U17,U21,U36,U37 but sounds ok.
/*-------------------------------------------------------------------
/ Godzilla
/-------------------------------------------------------------------*/
INITGAME(godzilla,GEN_WS,3/*?*/)
SE128_ROMSTART(godzilla,"gdzcpu.205",0x0156c21c)
SE_DMD524_ROMSTART(	"gzdspa.200",0xa254a01d)
SES_SOUNDROM08888("gdzu7.100" ,0xa0afe8b7,
                  "gdzu17.100",0x67fde624,
                  "gdzu21.100",0x11cf2c4c,
                  "gdzu36.100",0x8db2503a,
                  "gdzu37.100",0x79c2136a)
SE_ROMEND
CORE_GAMEDEFNV(godzilla,"Godzilla",1994,"Sega",de_mSES1,GAME_NOCRC)


/********************* SEGA GAMES DISTRIBUTED BY STERN  **********************/

/*-------------------------------------------------------------------
/ South Park
/-------------------------------------------------------------------*/
INITGAME(southpk,GEN_WS,3/*?*/)
SE128_ROMSTART(southpk,"spkcpu.103",0x55ca8aa1)
SE_DMD524_ROMSTART(    "spdspa.101",0x048ca598d)
SES_SOUNDROM00000("spku7.101" ,0x3d831d3e,
                  "spku17.100",0x829262c9,
                  "spku21.100",0x3a55eef3,
                  "spku36.100",0xbcf6d2cb,
                  "spku37.100",0x7d8f6bcb)
SE_ROMEND
CORE_GAMEDEFNV(southpk,"South Park",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Harley Davidson
/-------------------------------------------------------------------*/
INITGAME(harley,GEN_WS,3/*?*/)
SE128_ROMSTART(harley,"harcpu.103",0x2a812c75)
SE_DMD524_ROMSTART(   "hddsp.100",0x0bdeac0fd)
SES_SOUNDROM00008("hdsnd.u7" ,0xb9accb75,
                  "hdvc1.u17",0x0265fe72,
                  "hdvc2.u21",0x89230898,
                  "hdvc3.u36",0x41239811,
                  "hdvc4.u37",0xa1bc39f6)
SE_ROMEND
CORE_GAMEDEFNV(harley,"Harley Davidson",1998,"Sega",de_mSES1,GAME_NOCRC)


/********************* STERN GAMES  **********************/


/*-------------------------------------------------------------------
/ Striker Extreme
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,3/*?*/)
SE128_ROMSTART(strikext,"sxcpue.101",0xeac29785)
SE_DMD524_ROMSTART(		"sxdispa.101",0x01d2cb240)
SES_SOUNDROM00000("sxsounda.u7" ,0xe7e1a0cb,
                  "sxvoicea.u17",0xaeeed88f,
                  "sxvoicea.u21",0x62c9bfe3,
                  "sxvoicea.u36",0xa0bc0edb,
                  "sxvoicea.u37",0x4c08c33c)
SE_ROMEND
CORE_GAMEDEFNV(strikext,"Striker Extreme",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout
/-------------------------------------------------------------------*/
INITGAME(shrkysht,GEN_WS_1,3/*?*/)
SE128_ROMSTART(shrkysht,"ssva207.bin",0x7355d65d)
SE_DMD524_ROMSTART(	"ssdispa.201",0x3360300b)
SES_SOUNDROM0000("shark101.u7",0xfbc6267b,
                 "shark.u17"  ,0xdae78d8d,
                 "shark.u21"  ,0xe1fa3f2a,
                 "shark.u36"  ,0xd22fcfa3)
SE_ROMEND
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout",2000,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino
/-------------------------------------------------------------------*/
INITGAME(hirolcas,GEN_WS,3/*?*/)
SE128_ROMSTART(hirolcas,"hrccpu.210",0x2e3c682a)
SE_DMD524_ROMSTART(	    "hrcda200.bin",0x0642bdce7)
SES_SOUNDROM00000("hrcu7.dat"   ,0xc41f91a7,
                  "casinou1.dat",0x5858dfd0,
                  "casinou2.dat",0xc653de96,
                  "casinou3.dat",0x5634eb4e,
                  "casino4s.dat",0xd4d23c00)
SE_ROMEND
CORE_GAMEDEFNV(hirolcas,"High Roller Casino",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers
/-------------------------------------------------------------------*/
INITGAME(austin,GEN_WS,3/*?*/)
SE128_ROMSTART(austin,"apcpu.210",0xa4ddcdca)
SE_DMD524_ROMSTART(  "apdisp-a.200",0xf3ca7fca)

#if 0
SES_SOUNDROM00000("apsndu7.100" ,0x3d831d3e,
                  "apsndu17.100",0x829262c9,
                  "apsndu36.100",0xbcf6d2cb,
                  "apsndu21.100",0x3a55eef3,
                  "apsndu37.100",0x7d8f6bcb)
#else
SES_SOUNDROM00000("apsndu7.100" ,0xd0e79d59,
                  "apsndu17.100",0xc1e33fee,
                  "apsndu21.100",0x07c3e077,
                  "apsndu36.100",0xf70f2828,
                  "apsndu37.100",0xddf0144b)
#endif
SE_ROMEND
CORE_GAMEDEFNV(austin,"Austin Powers",2001,"Stern",de_mSES2,GAME_NOCRC)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Monopoly
/-------------------------------------------------------------------*/
INITGAME(monopoly,GEN_WS,3/*?*/)
SE128_ROMSTART(monopoly,"moncpu.233",0xf20a5ca6)
SE_DMD524_ROMSTART(  "mondsp-a.203",0x6e4678fb)
SES_SOUNDROM0000( "mnsndu7.100" ,0x400442e7,
                  "mnsndu17.100",0xf9bc55e8,
                  "mnsndu21.100",0xe0727e1f,
                  "mnsndu36.100",0xc845aa97)
SE_ROMEND
CORE_GAMEDEFNV(monopoly,"Monopoly",2001,"Stern",de_mSES1,GAME_NOCRC)
