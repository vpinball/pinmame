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

#ifndef VPINMAME
   #define GAME_NOCRC 0
#endif

#define INITGAME(name, gen, balls) \
static core_tGameData name##GameData = { \
  gen, se_dmd128x32, \
  {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 2} \
}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
SE_INPUT_PORTS_START(name, balls) SE_INPUT_PORTS_END

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
SES_SOUNDROM088 ( "twstsnd.u7" ,0x5ccf0798,
                  "twstsnd.u17",0x0e35d640,
                  "twstsnd.u21",0xc3eae590)
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

/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
INITGAME(lostspc,GEN_WS, 3/*?*/)
SE128_ROMSTART(lostspc,	"liscpu.101",0x81b2ced8)
SE_DMD524_ROMSTART(	"lisdspa.102",0xe8bf4a58)
SES_SOUNDROM08888("lisu7.100" ,0x96e6b3c4,
                  "lisu17.100",0x69076939,
                  "lisu21.100",0x56eede09,
                  "lisu36.100",0x56f2c53b,
                  "lisu37.100",0xf9430c59)
SE_ROMEND
CORE_GAMEDEFNV(lostspc,"Lost in Space",1994,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Godzilla
/-------------------------------------------------------------------*/
INITGAME(godzilla,GEN_WS,3/*?*/)
SE128_ROMSTART(godzilla,"gdzcpu.205",0x0156c21c)
SE_DMD524_ROMSTART(	"gzdspa.200",0xa254a01d)
SES_SOUNDROM08888("gdzu7.100" ,0xa0afe8b7,
                  "gdzu17.100",0x6bba69c8,
                  "gdzu21.100",0xdb738958,
                  "gdzu36.100",0xe3f24234,
                  "gdzu37.100",0x2c1acb14)
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
/ Striker Extreme (USA)
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,3/*?*/)
SE128_ROMSTART(strikext,"sxcpua.102", 0x5e5f0fb8)
SE_DMD524_ROMSTART(     "sxdispa.103",0xe4cf849f)
SES_SOUNDROM00000("sxsounda.u7" ,0xe7e1a0cb,
                  "sxvoicea.u17",0xaeeed88f,
                  "sxvoicea.u21",0x62c9bfe3,
                  "sxvoicea.u36",0xa0bc0edb,
                  "sxvoicea.u37",0x4c08c33c)
SE_ROMEND
CORE_GAMEDEFNV(strikext,"Striker Extreme",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (UK)
/-------------------------------------------------------------------*/
INITGAME(strxt_uk,GEN_WS,3/*?*/)
SE128_ROMSTART(strxt_uk,"sxcpue.101", 0xeac29785)
SE_DMD524_ROMSTART(     "sxdispa.101",0x1d2cb240)
SES_SOUNDROM00000("sxsounda.u7" ,0xe7e1a0cb,
                  "soceng.u17",0x779d8a98,
                  "soceng.u21",0x47b4838c,
                  "soceng.u36",0xab65d6f0,
                  "soceng.u37",0xd73254f1)
SE_ROMEND
CORE_CLONEDEFNV(strxt_uk,strikext,"Striker Extreme (UK)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (DE)
/-------------------------------------------------------------------*/
INITGAME(strxt_gr,GEN_WS,3/*?*/)
SE128_ROMSTART(strxt_gr,"sxcpug.102", 0x2686743b)
SE_DMD524_ROMSTART(     "sxdispg.103",0xeb656489)
SES_SOUNDROM00000("sxsoundg.u7" ,0xb38ec07d,
                  "sxvoiceg.u17",0x19ecf6ca,
                  "sxvoiceg.u21",0xee410b1e,
                  "sxvoiceg.u36",0xf0e126c2,
                  "sxvoiceg.u37",0x82260f4b)
SE_ROMEND
CORE_CLONEDEFNV(strxt_gr,strikext,"Striker Extreme (Germany)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (FR)
/-------------------------------------------------------------------*/
INITGAME(strxt_fr,GEN_WS,3/*?*/)
SE128_ROMSTART(strxt_fr,"sxcpuf.102", 0x2804bc9f)
SE_DMD524_ROMSTART(     "sxdispf.103",0x4b4b5c19)
SES_SOUNDROM00000("soc.u7" ,0xa03131cf,
                  "soc.u17",0xe7ee91cb,
                  "soc.u21",0x88cbf553,
                  "soc.u36",0x35474ad5,
                  "soc.u37",0x0e53f2a0)
SE_ROMEND
CORE_CLONEDEFNV(strxt_fr,strikext,"Striker Extreme (France)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (IT)
/-------------------------------------------------------------------*/
INITGAME(strxt_it,GEN_WS,3/*?*/)
SE128_ROMSTART(strxt_it,"sxcpui.102", 0xf955d0ef)
SE_DMD524_ROMSTART(     "sxdispi.103",0x40be3fe2)
SES_SOUNDROM00000("s00.u7" ,0x80625d23,
                  "s00.u17",0xd0b21193,
                  "s00.u21",0x5ab3f8f4,
                  "s00.u36",0x4ee21ade,
                  "s00.u37",0x4427e364)
SE_ROMEND
CORE_CLONEDEFNV(strxt_it,strikext,"Striker Extreme (Italy)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (SP)
/-------------------------------------------------------------------*/
INITGAME(strxt_sp,GEN_WS,3/*?*/)
SE128_ROMSTART(strxt_sp,"sxcpul.102", 0x6b1e417f)
SE_DMD524_ROMSTART(     "sxdispl.103",0x3efd4a18)
SES_SOUNDROM00000("soc.u7" ,0xa03131cf,
                  "soc.u17",0xe7ee91cb,
                  "soc.u21",0x88cbf553,
                  "soc.u36",0x35474ad5,
                  "soc.u37",0x0e53f2a0)
SE_ROMEND
CORE_CLONEDEFNV(strxt_sp,strikext,"Striker Extreme (Spain)",1999,"Stern",de_mSES2,GAME_NOCRC)

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
/ High Roller Casino (DE)
/-------------------------------------------------------------------*/
INITGAME(hirol_gr,GEN_WS,3/*?*/)
SE128_ROMSTART(hirol_gr,"hrccpu.210",  0x2e3c682a)
SE_DMD524_ROMSTART(	"hrcdg201.bin",0x57b95712)
SES_SOUNDROM00000("hrcu7.dat"   ,0xc41f91a7,
                  "casinou1.dat",0x5858dfd0,
                  "casinou2.dat",0xc653de96,
                  "casinou3.dat",0x5634eb4e,
                  "casino4s.dat",0xd4d23c00)
SE_ROMEND
CORE_CLONEDEFNV(hirol_gr,hirolcas,"High Roller Casino (Germany)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (3.0)
/-------------------------------------------------------------------*/
INITGAME(austin,GEN_WS,3/*?*/)
SE128_ROMSTART(austin,"apcpu.300",   0xa06b2b03)
SE_DMD524_ROMSTART(   "apdsp-a.300", 0xecf2c3bb)
SES_SOUNDROM00000("apsndu7.100" ,0xd0e79d59,
                  "apsndu17.100",0xc1e33fee,
                  "apsndu21.100",0x07c3e077,
                  "apsndu36.100",0xf70f2828,
                  "apsndu37.100",0xddf0144b)
SE_ROMEND
CORE_GAMEDEFNV(austin,"Austin Powers (3.0)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (2.0)
/-------------------------------------------------------------------*/
INITGAME(austin2,GEN_WS,3/*?*/)
SE128_ROMSTART(austin2,"apcpu.201",   0xa4ddcdca)
SE_DMD524_ROMSTART(    "apdisp-a.200",0xf3ca7fca)
SES_SOUNDROM00000("apsndu7.100" ,0xd0e79d59,
                  "apsndu17.100",0xc1e33fee,
                  "apsndu21.100",0x07c3e077,
                  "apsndu36.100",0xf70f2828,
                  "apsndu37.100",0xddf0144b)
SE_ROMEND
CORE_CLONEDEFNV(austin2,austin,"Austin Powers (2.0)",2001,"Stern",de_mSES2,GAME_NOCRC)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Monopoly 2.33
/-------------------------------------------------------------------*/
INITGAME(monopoly,GEN_WS,3/*?*/)
SE128_ROMSTART(monopoly,"moncpu.233",0xf20a5ca6)
SE_DMD524_ROMSTART(     "mondsp-a.206",0x6df6e158)
SES_SOUNDROM0000( "mnsndu7.100" ,0x400442e7,
                  "mnsndu17.100",0xf9bc55e8,
                  "mnsndu21.100",0xe0727e1f,
                  "mnsndu36.100",0xc845aa97)
SE_ROMEND
CORE_GAMEDEFNV(monopoly,"Monopoly",2001,"Stern",de_mSES1,GAME_NOCRC)
