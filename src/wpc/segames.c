#include "driver.h"
#include "core.h"
#include "dedmd.h"
#include "desound.h"
#include "se.h"
#include "vpintf.h"

/* NO OUTPUT */

static struct core_dispLayout se_dmd128x32[] = {
  {0,0, 32,128, CORE_DMD, (void *)dedmd32_update}, {0}
};

#define INITGAME(name, gen, disp, hw) \
static core_tGameData name##GameData = { \
  gen, disp, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 2, 0, 0, hw}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

SE_INPUT_PORTS_START(se, 1) SE_INPUT_PORTS_END

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

//Snd Works but VERY SOFT
/*-------------------------------------------------------------------
/ Apollo 13
/-------------------------------------------------------------------*/
INITGAME(apollo13,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(apollo13,"apolcpu.501",CRC(5afb8801))
DE_DMD32ROM8x(   "a13dspa.500",CRC(bf8e3249))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8),
                  "apollo13.u17",CRC(4e863aca),
                  "apollo13.u21",CRC(28169e37),
                  "apollo13.u36",CRC(cede5e0f))
SE_ROMEND
#define input_ports_apollo13 input_ports_se
CORE_GAMEDEFNV(apollo13,"Apollo 13",1995,"Sega",de_mSES3,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Goldeneye
/-------------------------------------------------------------------*/
INITGAME(gldneye,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(gldneye, "bondcpu.404",CRC(5aa6ffcc))
DE_DMD32ROM8x(   "bondispa.400",CRC(9cc0c710))
DE2S_SOUNDROM144("bondu7.bin" ,CRC(7581a349),
                 "bondu17.bin",CRC(d9c56b9d),
                 "bondu21.bin",CRC(5be0f205))
SE_ROMEND
#define input_ports_gldneye input_ports_se
CORE_GAMEDEFNV(gldneye,"Goldeneye",1996,"Sega",de_mSES3,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Twister
/-------------------------------------------------------------------*/
INITGAME(twister,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(twister, "twstcpu.405",CRC(8c3ea1a8))
DE_DMD32ROM8x(   "twstdspa.400",CRC(a6a3d41d))
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798),
                 "twstsnd.u17",CRC(0e35d640),
                 "twstsnd.u21",CRC(c3eae590))
SE_ROMEND
#define input_ports_twister input_ports_se
CORE_GAMEDEFNV(twister,"Twister",1996,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ ID4: Independence Day
/-------------------------------------------------------------------*/
INITGAME(id4,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4, "id4cpu.202",CRC(108d88fd))
DE_DMD32ROM8x(    "id4dspa.200",CRC(2d3fbcc4))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37),
                  "id4sdu17.400",CRC(385034f1),
                  "id4sdu21.400",CRC(f384a9ab))
SE_ROMEND
#define input_ports_id4 input_ports_se
CORE_GAMEDEFNV(id4,"ID4: Independence Day",1996,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
INITGAME(spacejam,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejam,"jamcpu.300",CRC(9dc8df2e))
DE_DMD32ROM8x(  "jamdspa.300",CRC(198e5e34))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853),
                  "spcjam.u17",CRC(ccefe457),
                  "spcjam.u21",CRC(9e7fe0a6),
                  "spcjam.u36",CRC(7d11e1eb))
SE_ROMEND
#define input_ports_spacejam input_ports_se
CORE_GAMEDEFNV(spacejam,"Space Jam",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Star Wars Trilogy
/-------------------------------------------------------------------*/
INITGAME(swtril,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(swtril,"swcpu.403",CRC(d022880e))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71))
DE2S_SOUNDROM144("sw0219.u7" ,CRC(cd7c84d9),
                 "sw0211.u17",CRC(6863e7f6),
                 "sw0211.u21",CRC(6be68450))
SE_ROMEND
#define input_ports_swtril input_ports_se
CORE_GAMEDEFNV(swtril,"Star Wars Trilogy",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park
/-------------------------------------------------------------------*/
INITGAME(jplstwld,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(jplstwld,"jp2cpu.202",CRC(d317e601))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace))
DE2S_SOUNDROM144("jp2_u7.bin" ,CRC(73b74c96),
                 "jp2_u17.bin",CRC(8d6c0dbd),
                 "jp2_u21.bin",CRC(c670a997))
SE_ROMEND
#define input_ports_jplstwld input_ports_se
CORE_GAMEDEFNV(jplstwld,"The Lost World: Jurassic Park",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ X-Files
/-------------------------------------------------------------------*/
INITGAME(xfiles,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(xfiles,"xfcpu.303",CRC(c7ab5efe))
DE_DMD32ROM8x(   "xfildspa.300",CRC(03c96af8))
DE2S_SOUNDROM144( "xfsndu7.512" ,CRC(01d65239),
                  "xfsndu17.c40",CRC(40bfc835),
                  "xfsndu21.c40",CRC(b56a5ca6))
SE_ROMEND
#define input_ports_xfiles input_ports_se
CORE_GAMEDEFNV(xfiles,"X-Files",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Starship Troopers
/-------------------------------------------------------------------*/
INITGAME(startrp,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(startrp, "sstcpu.201",CRC(549699fe))
DE_DMD32ROM8x(    "sstdspa.200",CRC(76a0e09e))
DE2S_SOUNDROM1444("u7_b130.512" ,CRC(f1559e4f),
                  "u17_152a.040",CRC(8caeccdb),
                  "u21_0291.040",CRC(0c5321f6),
                  "u36_95a7.040",CRC(c1e4ca6a))
SE_ROMEND
#define input_ports_startrp input_ports_se
CORE_GAMEDEFNV(startrp,"Starship Troopers",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Viper Night Drivin'
/-------------------------------------------------------------------*/
INITGAME(viprsega,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(viprsega, "vipcpu.201",CRC(476557aa))
DE_DMD32ROM8x(   "vipdspa.201",CRC(24b1dc21))
DE2S_SOUNDROM14444("vpru7.dat" ,CRC(f21617d7),
                  "vpru17.dat",CRC(47b1317c),
                  "vpru21.dat",CRC(0e0e2dd6),
                  "vpru36.dat",CRC(7b482876),
                  "vpru37.dat",CRC(0bf23e0e))
SE_ROMEND
#define input_ports_viprsega input_ports_se
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin'",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
INITGAME(lostspc,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspc, "liscpu.101",CRC(81b2ced8))
DE_DMD32ROM8x(  "lisdspa.102",CRC(e8bf4a58))
DE2S_SOUNDROM14444("lisu7.100" ,CRC(96e6b3c4),
                  "lisu17.100",CRC(69076939),
                  "lisu21.100",CRC(56eede09),
                  "lisu36.100",CRC(56f2c53b),
                  "lisu37.100",CRC(f9430c59))
SE_ROMEND
#define input_ports_lostspc input_ports_se
CORE_GAMEDEFNV(lostspc,"Lost in Space",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Golden Cue
/-------------------------------------------------------------------*/
INITGAME(goldcue,GEN_WS_1,se_dmd128x32,0)
SE128_ROMSTART(goldcue,"gc_cpu.210",CRC(8447eaee))
DE_DMD32ROM8x(        "gc_disp.u12",CRC(87f74b9d))
DE2S_SOUNDROM1444(   "gc_sound.u7", CRC(8b559e39),
                     "gc_sound.u17",CRC(28c39bae),
                     "gc_sound.u21",CRC(d3f43a37),
                     "gc_sound.u36",CRC(81f27955))
SE_ROMEND
#define input_ports_goldcue input_ports_se
CORE_GAMEDEFNV(goldcue,"Golden Cue",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Godzilla
/-------------------------------------------------------------------*/
INITGAME(godzilla,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(godzilla,"gdzcpu.205",CRC(0156c21c))
DE_DMD32ROM8x(  "gzdspa.200",CRC(a254a01d))
DE2S_SOUNDROM14444("gdzu7.100" ,CRC(a0afe8b7),
                  "gdzu17.100",CRC(6bba69c8),
                  "gdzu21.100",CRC(db738958),
                  "gdzu36.100",CRC(e3f24234),
                  "gdzu37.100",CRC(2c1acb14))
SE_ROMEND
#define input_ports_godzilla input_ports_se
CORE_GAMEDEFNV(godzilla,"Godzilla",1998,"Sega",de_mSES1,GAME_NOCRC)

/********************* SEGA GAMES DISTRIBUTED BY STERN  **********************/

/*-------------------------------------------------------------------
/ South Park
/-------------------------------------------------------------------*/
INITGAME(southpk,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(southpk,"spkcpu.103",CRC(55ca8aa1))
DE_DMD32ROM8x(    "spdspa.101",CRC(48ca598d))
DE2S_SOUNDROM18888("spku7.101" ,CRC(3d831d3e),
                  "spku17.100",CRC(829262c9),
                  "spku21.100",CRC(3a55eef3),
                  "spku36.100",CRC(bcf6d2cb),
                  "spku37.100",CRC(7d8f6bcb))
SE_ROMEND
#define input_ports_southpk input_ports_se
CORE_GAMEDEFNV(southpk,"South Park",1999,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Harley Davidson
/-------------------------------------------------------------------*/
INITGAME(harley,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(harley,"harcpu.103",CRC(2a812c75))
DE_DMD32ROM8x(   "hddsp.100",CRC(bdeac0fd))
DE2S_SOUNDROM18884("hdsnd.u7" ,CRC(b9accb75),
                  "hdvc1.u17",CRC(0265fe72),
                  "hdvc2.u21",CRC(89230898),
                  "hdvc3.u36",CRC(41239811),
                  "hdvc4.u37",CRC(a1bc39f6))
SE_ROMEND
#define input_ports_harley input_ports_se
CORE_GAMEDEFNV(harley,"Harley Davidson",1999,"Sega",de_mSES1,GAME_NOCRC)


/********************* STERN GAMES  **********************/


/*-------------------------------------------------------------------
/ Striker Extreme (USA)
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(strikext,"sxcpua.102", CRC(5e5f0fb8))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f))
DE2S_SOUNDROM18888("sxsounda.u7" ,CRC(e7e1a0cb),
                  "sxvoicea.u17",CRC(aeeed88f),
                  "sxvoicea.u21",CRC(62c9bfe3),
                  "sxvoicea.u36",CRC(a0bc0edb),
                  "sxvoicea.u37",CRC(4c08c33c))
SE_ROMEND
#define input_ports_strikext input_ports_se
CORE_GAMEDEFNV(strikext,"Striker Extreme",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (UK)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_uk,"sxcpue.101", CRC(eac29785))
DE_DMD32ROM8x(     "sxdispa.101",CRC(1d2cb240))
DE2S_SOUNDROM18888("sxsounda.u7" ,CRC(e7e1a0cb),
                  "soceng.u17",CRC(779d8a98),
                  "soceng.u21",CRC(47b4838c),
                  "soceng.u36",CRC(ab65d6f0),
                  "soceng.u37",CRC(d73254f1))
SE_ROMEND
#define input_ports_strxt_uk input_ports_se
#define init_strxt_uk init_strikext
CORE_CLONEDEFNV(strxt_uk,strikext,"Striker Extreme (UK)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_gr,"sxcpug.102", CRC(2686743b))
DE_DMD32ROM8x(     "sxdispg.103",CRC(eb656489))
DE2S_SOUNDROM18888("sxsoundg.u7" ,CRC(b38ec07d),
                  "sxvoiceg.u17",CRC(19ecf6ca),
                  "sxvoiceg.u21",CRC(ee410b1e),
                  "sxvoiceg.u36",CRC(f0e126c2),
                  "sxvoiceg.u37",CRC(82260f4b))
SE_ROMEND
#define input_ports_strxt_gr input_ports_se
#define init_strxt_gr init_strikext
CORE_CLONEDEFNV(strxt_gr,strikext,"Striker Extreme (Germany)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_fr,"sxcpuf.102", CRC(2804bc9f))
DE_DMD32ROM8x(     "sxdispf.103",CRC(4b4b5c19))
DE2S_SOUNDROM18888("soc.u7" ,CRC(a03131cf),
                  "soc.u17",CRC(e7ee91cb),
                  "soc.u21",CRC(88cbf553),
                  "soc.u36",CRC(35474ad5),
                  "soc.u37",CRC(0e53f2a0))
SE_ROMEND
#define input_ports_strxt_fr input_ports_se
#define init_strxt_fr init_strikext
CORE_CLONEDEFNV(strxt_fr,strikext,"Striker Extreme (France)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_it,"sxcpui.102", CRC(f955d0ef))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2))
DE2S_SOUNDROM18888("s00.u7" ,CRC(80625d23),
                  "s00.u17",CRC(d0b21193),
                  "s00.u21",CRC(5ab3f8f4),
                  "s00.u36",CRC(4ee21ade),
                  "s00.u37",CRC(4427e364))
SE_ROMEND
#define input_ports_strxt_it input_ports_se
#define init_strxt_it init_strikext
CORE_CLONEDEFNV(strxt_it,strikext,"Striker Extreme (Italy)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_sp,"sxcpul.102", CRC(6b1e417f))
DE_DMD32ROM8x(     "sxdispl.103",CRC(3efd4a18))
DE2S_SOUNDROM18888("soc.u7" ,CRC(a03131cf),
                  "soc.u17",CRC(e7ee91cb),
                  "soc.u21",CRC(88cbf553),
                  "soc.u36",CRC(35474ad5),
                  "soc.u37",CRC(0e53f2a0))
SE_ROMEND
#define input_ports_strxt_sp input_ports_se
#define init_strxt_sp init_strikext
CORE_CLONEDEFNV(strxt_sp,strikext,"Striker Extreme (Spain)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (2.11)
/-------------------------------------------------------------------*/
INITGAME(shrkysht,GEN_WS_1,se_dmd128x32,0)
SE128_ROMSTART(shrkysht,"sscpu.211",CRC(66a0e5ce))
DE_DMD32ROM8x(        "ssdispa.201",CRC(3360300b))
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b),
                     "sssndu17.100",CRC(dae78d8d),
                     "sssndu21.100",CRC(e1fa3f2a),
                     "sssndu36.100",CRC(d22fcfa3))
SE_ROMEND
#define input_ports_shrkysht input_ports_se
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout",2000,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_gr,"sscpu.211",CRC(66a0e5ce))
DE_DMD32ROM8x(        "ssdispg.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b),
                     "sssndu17.100",CRC(dae78d8d),
                     "sssndu21.100",CRC(e1fa3f2a),
                     "sssndu36.100",CRC(d22fcfa3))
SE_ROMEND
#define input_ports_shrky_gr input_ports_se
#define init_shrky_gr init_shrkysht
CORE_CLONEDEFNV(shrky_gr,shrkysht,"Sharkey's Shootout (Germany)",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_fr,"sscpu.211",CRC(66a0e5ce))
DE_DMD32ROM8x(        "ssdispf.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b),
                     "sssndu17.100",CRC(dae78d8d),
                     "sssndu21.100",CRC(e1fa3f2a),
                     "sssndu36.100",CRC(d22fcfa3))
SE_ROMEND
#define input_ports_shrky_fr input_ports_se
#define init_shrky_fr init_shrkysht
CORE_CLONEDEFNV(shrky_fr,shrkysht,"Sharkey's Shootout (France)",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_it,"sscpu.211",CRC(66a0e5ce))
DE_DMD32ROM8x(        "ssdispi.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b),
                     "sssndu17.100",CRC(dae78d8d),
                     "sssndu21.100",CRC(e1fa3f2a),
                     "sssndu36.100",CRC(d22fcfa3))
SE_ROMEND
#define input_ports_shrky_it input_ports_se
#define init_shrky_it init_shrkysht
CORE_CLONEDEFNV(shrky_it,shrkysht,"Sharkey's Shootout (Italy)",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (3.0)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispHRC[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 15, CORE_DMD, (void *)seminidmd1_update}, {0}
};
INITGAME(hirolcas,GEN_WS,dispHRC,SE_MINIDMD)
SE128_ROMSTART(hirolcas,"hrccpu.300",CRC(0d1117fa))
DE_DMD32ROM8x(        "hrcdispa.300",CRC(099ccaf0))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7),
                      "hrsndu17.100",CRC(5858dfd0),
                      "hrsndu21.100",CRC(c653de96),
                      "hrsndu36.100",CRC(5634eb4e),
                      "hrsndu37.100",CRC(d4d23c00))
SE_ROMEND
#define input_ports_hirolcas input_ports_se
CORE_GAMEDEFNV(hirolcas,"High Roller Casino",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_fr,"hrccpu.300",CRC(0d1117fa))
DE_DMD32ROM8x(        "hrcdispf.300",NO_DUMP)
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7),
                      "hrsndu17.100",CRC(5858dfd0),
                      "hrsndu21.100",CRC(c653de96),
                      "hrsndu36.100",CRC(5634eb4e),
                      "hrsndu37.100",CRC(d4d23c00))
SE_ROMEND
#define input_ports_hirol_fr input_ports_se
#define init_hirol_fr init_hirolcas
CORE_CLONEDEFNV(hirol_fr,hirolcas,"High Roller Casino (France)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_gr,"hrccpu.300",CRC(0d1117fa))
DE_DMD32ROM8x(        "hrcdispg.300",NO_DUMP)
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7),
                      "hrsndu17.100",CRC(5858dfd0),
                      "hrsndu21.100",CRC(c653de96),
                      "hrsndu36.100",CRC(5634eb4e),
                      "hrsndu37.100",CRC(d4d23c00))
SE_ROMEND
#define input_ports_hirol_gr input_ports_se
#define init_hirol_gr init_hirolcas
CORE_CLONEDEFNV(hirol_gr,hirolcas,"High Roller Casino (Germany)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_it,"hrccpu.300",CRC(0d1117fa))
DE_DMD32ROM8x(        "hrcdispi.300",NO_DUMP)
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7),
                      "hrsndu17.100",CRC(5858dfd0),
                      "hrsndu21.100",CRC(c653de96),
                      "hrsndu36.100",CRC(5634eb4e),
                      "hrsndu37.100",CRC(d4d23c00))
SE_ROMEND
#define input_ports_hirol_it input_ports_se
#define init_hirol_it init_hirolcas
CORE_CLONEDEFNV(hirol_it,hirolcas,"High Roller Casino (Italy)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (3.02)
/-------------------------------------------------------------------*/
INITGAME(austin,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(austin,"apcpu.302",CRC(2920b59b))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb))
DE2S_SOUNDROM18888( "apsndu7.100",CRC(d0e79d59),
                   "apsndu17.100",CRC(c1e33fee),
                   "apsndu21.100",CRC(07c3e077),
                   "apsndu36.100",CRC(f70f2828),
                   "apsndu37.100",CRC(ddf0144b))
SE_ROMEND
#define input_ports_austin input_ports_se
CORE_GAMEDEFNV(austin,"Austin Powers (3.0)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austinf,"apcpu.302",CRC(2920b59b))
DE_DMD32ROM8x(       "apdsp-f.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59),
                    "apsndu17.100",CRC(c1e33fee),
                    "apsndu21.100",CRC(07c3e077),
                    "apsndu36.100",CRC(f70f2828),
                    "apsndu37.100",CRC(ddf0144b))
SE_ROMEND
#define input_ports_austinf input_ports_se
#define init_austinf init_austin
CORE_CLONEDEFNV(austinf,austin,"Austin Powers (France)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austind,"apcpu.302",CRC(2920b59b))
DE_DMD32ROM8x(       "apdsp-g.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59),
                    "apsndu17.100",CRC(c1e33fee),
                    "apsndu21.100",CRC(07c3e077),
                    "apsndu36.100",CRC(f70f2828),
                    "apsndu37.100",CRC(ddf0144b))
SE_ROMEND
#define input_ports_austind input_ports_se
#define init_austind init_austin
CORE_CLONEDEFNV(austind,austin,"Austin Powers (Germany)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austini,"apcpu.302",CRC(2920b59b))
DE_DMD32ROM8x(       "apdsp-i.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59),
                    "apsndu17.100",CRC(c1e33fee),
                    "apsndu21.100",CRC(07c3e077),
                    "apsndu36.100",CRC(f70f2828),
                    "apsndu37.100",CRC(ddf0144b))
SE_ROMEND
#define input_ports_austini input_ports_se
#define init_austini init_austin
CORE_CLONEDEFNV(austini,austin,"Austin Powers (Italy)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (2.01)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austin2,"apcpu.201",CRC(a4ddcdca))
DE_DMD32ROM8x(      "apdisp-a.200",CRC(f3ca7fca))
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59),
                    "apsndu17.100",CRC(c1e33fee),
                    "apsndu21.100",CRC(07c3e077),
                    "apsndu36.100",CRC(f70f2828),
                    "apsndu37.100",CRC(ddf0144b))
SE_ROMEND
#define input_ports_austin2 input_ports_se
#define init_austin2 init_austin
CORE_CLONEDEFNV(austin2,austin,"Austin Powers (2.0)",2001,"Stern",de_mSES2,GAME_NOCRC)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Monopoly (3.03)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispMonopoly[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 15, CORE_DMD, (void *)seminidmd2_update}, {0}
};
INITGAME(monopoly,GEN_WS,dispMonopoly,SE_MINIDMD)
SE128_ROMSTART(monopoly,"moncpu.303",CRC(4a66c9e4))
DE_DMD32ROM8x(        "mondsp-a.301",CRC(c4e2e032))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7),
                      "mnsndu17.100",CRC(595f23ab),
                      "mnsndu21.100",CRC(e0727e1f),
                      "mnsndu36.100",CRC(d66c71a9))
SE_ROMEND
#define input_ports_monopoly input_ports_se
CORE_GAMEDEFNV(monopoly,"Monopoly",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Monopoly (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopolf,"moncpu.303",CRC(4a66c9e4))
DE_DMD32ROM8x(        "mondsp-f.301",NO_DUMP)
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7),
                      "mnsndu17.100",CRC(595f23ab),
                      "mnsndu21.100",CRC(e0727e1f),
                      "mnsndu36.100",CRC(d66c71a9))
SE_ROMEND
#define input_ports_monopolf input_ports_se
#define init_monopolf init_monopoly
CORE_CLONEDEFNV(monopolf,monopoly,"Monopoly (France)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Monopoly (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopold,"moncpu.303",CRC(4a66c9e4))
DE_DMD32ROM8x(        "mondsp-g.301",NO_DUMP)
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7),
                      "mnsndu17.100",CRC(595f23ab),
                      "mnsndu21.100",CRC(e0727e1f),
                      "mnsndu36.100",CRC(d66c71a9))
SE_ROMEND
#define input_ports_monopold input_ports_se
#define init_monopold init_monopoly
CORE_CLONEDEFNV(monopold,monopoly,"Monopoly (Germany)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Monopoly (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopoli,"moncpu.303",CRC(4a66c9e4))
DE_DMD32ROM8x(        "mondsp-i.301",NO_DUMP)
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7),
                      "mnsndu17.100",CRC(595f23ab),
                      "mnsndu21.100",CRC(e0727e1f),
                      "mnsndu36.100",CRC(d66c71a9))
SE_ROMEND
#define input_ports_monopoli input_ports_se
#define init_monopoli init_monopoly
CORE_CLONEDEFNV(monopoli,monopoly,"Monopoly (Italy)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Monopoly (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopole,"moncpu.303",CRC(4a66c9e4))
DE_DMD32ROM8x(        "mondsp-s.301",NO_DUMP)
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7),
                      "mnsndu17.100",CRC(595f23ab),
                      "mnsndu21.100",CRC(e0727e1f),
                      "mnsndu36.100",CRC(d66c71a9))
SE_ROMEND
#define input_ports_monopole input_ports_se
#define init_monopole init_monopoly
CORE_CLONEDEFNV(monopole,monopoly,"Monopoly (Spain)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (5.0)
/-------------------------------------------------------------------*/
INITGAME(playboys,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(playboys, "pbcpu.500",    CRC(e4d924ae))
DE_DMD32ROM8x(           "pbdispa.500",  CRC(40450165))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34),
                         "pbsndu17.100", CRC(f5502fec),
                         "pbsndu21.100", CRC(7869d34f),
                         "pbsndu36.100", CRC(d10f14a3),
                         "pbsndu37.100", CRC(6642524a))
SE_ROMEND
#define input_ports_playboys input_ports_se
CORE_GAMEDEFNV(playboys,"Playboy (Stern)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyf, "pbcpu.500",    CRC(e4d924ae))
DE_DMD32ROM8x(           "pbdispf.500",  CRC(aedc6c32))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34),
                         "pbsndu17.100", CRC(f5502fec),
                         "pbsndu21.100", CRC(7869d34f),
                         "pbsndu36.100", CRC(d10f14a3),
                         "pbsndu37.100", CRC(6642524a))
SE_ROMEND
#define input_ports_playboyf input_ports_se
#define init_playboyf init_playboys
CORE_CLONEDEFNV(playboyf,playboys,"Playboy (France)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyd, "pbcpu.500",    CRC(e4d924ae))
DE_DMD32ROM8x(           "pbdispg.500",  CRC(681392fe))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34),
                         "pbsndu17.100", CRC(f5502fec),
                         "pbsndu21.100", CRC(7869d34f),
                         "pbsndu36.100", CRC(d10f14a3),
                         "pbsndu37.100", CRC(6642524a))
SE_ROMEND
#define input_ports_playboyd input_ports_se
#define init_playboyd init_playboys
CORE_CLONEDEFNV(playboyd,playboys,"Playboy (Germany)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyi, "pbcpu.500",    CRC(e4d924ae))
DE_DMD32ROM8x(           "pbdispi.500",  CRC(d90d7ec6))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34),
                         "pbsndu17.100", CRC(f5502fec),
                         "pbsndu21.100", CRC(7869d34f),
                         "pbsndu36.100", CRC(d10f14a3),
                         "pbsndu37.100", CRC(6642524a))
SE_ROMEND
#define input_ports_playboyi input_ports_se
#define init_playboyi init_playboys
CORE_CLONEDEFNV(playboyi,playboys,"Playboy (Italy)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboye, "pbcpu.500",    CRC(e4d924ae))
DE_DMD32ROM8x(           "pbdispl.500",  CRC(b019f0f6))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34),
                         "pbsndu17.100", CRC(f5502fec),
                         "pbsndu21.100", CRC(7869d34f),
                         "pbsndu36.100", CRC(d10f14a3),
                         "pbsndu37.100", CRC(6642524a))
SE_ROMEND
#define input_ports_playboye input_ports_se
#define init_playboye init_playboys
CORE_CLONEDEFNV(playboye,playboys,"Playboy (Spain)",2002,"Stern",de_mSES2,GAME_NOCRC)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (7.01)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispRCT[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 5,21, CORE_DMD, (void *)seminidmd3_update}, {0}
};
INITGAME(rctycn, GEN_WS, dispRCT, SE_MINIDMD)
SE128_ROMSTART(rctycn, "rctcpu.701",CRC(e1fe89f6))
DE_DMD32ROM8x(       "rctdispa.700",CRC(6a8925d7))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1),
                     "rcsndu17.100",CRC(18ba20ec),
                     "rcsndu21.100",CRC(64b19c11),
                     "rcsndu36.100",CRC(05c8bac9))
SE_ROMEND
#define input_ports_rctycn input_ports_se
CORE_GAMEDEFNV(rctycn,"Roller Coaster Tycoon",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnd,"rctcpu.701",CRC(e1fe89f6))
DE_DMD32ROM8x(       "rctdispg.700",CRC(0babf1ed))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1),
                     "rcsndu17.100",CRC(18ba20ec),
                     "rcsndu21.100",CRC(64b19c11),
                     "rcsndu36.100",CRC(05c8bac9))
SE_ROMEND
#define input_ports_rctycnd input_ports_se
#define init_rctycnd init_rctycn
CORE_CLONEDEFNV(rctycnd,rctycn,"Roller Coaster Tycoon (Germany)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnf,"rctcpu.701",CRC(e1fe89f6))
DE_DMD32ROM8x(       "rctdispf.700",CRC(2de0eec8))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1),
                     "rcsndu17.100",CRC(18ba20ec),
                     "rcsndu21.100",CRC(64b19c11),
                     "rcsndu36.100",CRC(05c8bac9))
SE_ROMEND
#define input_ports_rctycnf input_ports_se
#define init_rctycnf init_rctycn
CORE_CLONEDEFNV(rctycnf,rctycn,"Roller Coaster Tycoon (France)",2002,"Stern",de_mSES1,GAME_NOCRC)


/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycni,"rctcpu.701",CRC(e1fe89f6))
DE_DMD32ROM8x(       "rctdispi.700",CRC(0d07d8f2))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1),
                     "rcsndu17.100",CRC(18ba20ec),
                     "rcsndu21.100",CRC(64b19c11),
                     "rcsndu36.100",CRC(05c8bac9))
SE_ROMEND
#define input_ports_rctycni input_ports_se
#define init_rctycni init_rctycn
CORE_CLONEDEFNV(rctycni,rctycn,"Roller Coaster Tycoon (Italy)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycne,"rctcpu.701",CRC(e1fe89f6))
DE_DMD32ROM8x(       "rctdisps.700",CRC(6921d8cc))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1),
                     "rcsndu17.100",CRC(18ba20ec),
                     "rcsndu21.100",CRC(64b19c11),
                     "rcsndu36.100",CRC(05c8bac9))
SE_ROMEND
#define input_ports_rctycne input_ports_se
#define init_rctycne init_rctycn
CORE_CLONEDEFNV(rctycne,rctycn,"Roller Coaster Tycoon (Spain)",2002,"Stern",de_mSES1,GAME_NOCRC)

//And back to the 12 voice model here!
/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (2.04)
/-------------------------------------------------------------------*/
INITGAME(simpprty,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(simpprty, "spp-cpu.204",CRC(5bc155f7))
DE_DMD32ROM8x(           "sppdspa.201",CRC(f55505a4))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6),
                         "spp100.u17", CRC(65e9344e),
                         "spp100.u21", CRC(17fee0f9),
                         "spp100.u36", CRC(ffb957b0),
                         "spp100.u37", CRC(0738e1fc))
SE_ROMEND
#define input_ports_simpprty input_ports_se
CORE_GAMEDEFNV(simpprty,"The Simpsons Pinball Party",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtd, "spp-cpu.204",CRC(5bc155f7))
DE_DMD32ROM8x(           "sppdspg.201",NO_DUMP)
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6),
                         "spp100.u17", CRC(65e9344e),
                         "spp100.u21", CRC(17fee0f9),
                         "spp100.u36", CRC(ffb957b0),
                         "spp100.u37", CRC(0738e1fc))
SE_ROMEND
#define input_ports_simpprtd input_ports_se
#define init_simpprtd init_simpprty
CORE_CLONEDEFNV(simpprtd,simpprty,"The Simpsons Pinball Party (Germany)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprte, "spp-cpu.204",CRC(5bc155f7))
DE_DMD32ROM8x(           "sppdspl.201",CRC(78a67e23))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6),
                         "spp100.u17", CRC(65e9344e),
                         "spp100.u21", CRC(17fee0f9),
                         "spp100.u36", CRC(ffb957b0),
                         "spp100.u37", CRC(0738e1fc))
SE_ROMEND
#define input_ports_simpprte input_ports_se
#define init_simpprte init_simpprty
CORE_CLONEDEFNV(simpprte,simpprty,"The Simpsons Pinball Party (Spain)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtf, "spp-cpu.204",CRC(5bc155f7))
DE_DMD32ROM8x(           "sppdspf.201",CRC(d1c0c484))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6),
                         "spp100.u17", CRC(65e9344e),
                         "spp100.u21", CRC(17fee0f9),
                         "spp100.u36", CRC(ffb957b0),
                         "spp100.u37", CRC(0738e1fc))
SE_ROMEND
#define input_ports_simpprtf input_ports_se
#define init_simpprtf init_simpprty
CORE_CLONEDEFNV(simpprtf,simpprty,"The Simpsons Pinball Party (France)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprti, "spp-cpu.204",CRC(5bc155f7))
DE_DMD32ROM8x(           "sppdspi.201",CRC(b4594819))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6),
                         "spp100.u17", CRC(65e9344e),
                         "spp100.u21", CRC(17fee0f9),
                         "spp100.u36", CRC(ffb957b0),
                         "spp100.u37", CRC(0738e1fc))
SE_ROMEND
#define input_ports_simpprti input_ports_se
#define init_simpprti init_simpprty
CORE_CLONEDEFNV(simpprti,simpprty,"The Simpsons Pinball Party (Italy)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (2.00)
/-------------------------------------------------------------------*/
INITGAME(term3,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3,	 "t3cpu.200",CRC(b5cc4e24))
DE_DMD32ROM8x(           "t3dispa.200",CRC(d664574c))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af),
                         "t3100.u17", CRC(f0c71b5d),
                         "t3100.u21", CRC(694331f7),
                         "t3100.u36", CRC(9eb512e9),
                         "t3100.u37", CRC(3efb0c19))
SE_ROMEND
#define input_ports_term3 input_ports_se
CORE_GAMEDEFNV(term3,"Terminator 3: Rise of the Machines",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

//What happened to the German Version? It's not in the rom!
/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Germany)
/-------------------------------------------------------------------*/
INITGAME(term3g,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3g,	 "t3cpu.200",CRC(b5cc4e24))
DE_DMD32ROM8x(           "t3dispg.200",NO_DUMP)
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af),
                         "t3100.u17", CRC(f0c71b5d),
                         "t3100.u21", CRC(694331f7),
                         "t3100.u36", CRC(9eb512e9),
                         "t3100.u37", CRC(3efb0c19))
SE_ROMEND
#define input_ports_term3g input_ports_se
#define init_term3g init_term3
CORE_CLONEDEFNV(term3g,term3,"Terminator 3: Rise of the Machines (Germany)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Spain)
/-------------------------------------------------------------------*/
INITGAME(term3l,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3l,	 "t3cpu.200",CRC(b5cc4e24))
DE_DMD32ROM8x(           "t3displ.200",CRC(41f14c9d))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af),
                         "t3100.u17", CRC(f0c71b5d),
                         "t3100.u21", CRC(694331f7),
                         "t3100.u36", CRC(9eb512e9),
                         "t3100.u37", CRC(3efb0c19))
SE_ROMEND
#define input_ports_term3l input_ports_se
#define init_term3l init_term3
CORE_CLONEDEFNV(term3l,term3,"Terminator 3: Rise of the Machines (Spain)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (France)
/-------------------------------------------------------------------*/
INITGAME(term3f,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3f,	 "t3cpu.200",CRC(b5cc4e24))
DE_DMD32ROM8x(           "t3dispf.200",CRC(0d3e4c77))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af),
                         "t3100.u17", CRC(f0c71b5d),
                         "t3100.u21", CRC(694331f7),
                         "t3100.u36", CRC(9eb512e9),
                         "t3100.u37", CRC(3efb0c19))
SE_ROMEND
#define input_ports_term3f input_ports_se
#define init_term3f init_term3
CORE_CLONEDEFNV(term3f,term3,"Terminator 3: Rise of the Machines (France)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)


/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Italy)
/-------------------------------------------------------------------*/
INITGAME(term3i,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3i,	 "t3cpu.200",CRC(b5cc4e24))
DE_DMD32ROM8x(           "t3dispi.200",CRC(39dd7f6e))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af),
                         "t3100.u17", CRC(f0c71b5d),
                         "t3100.u21", CRC(694331f7),
                         "t3100.u36", CRC(9eb512e9),
                         "t3100.u37", CRC(3efb0c19))
SE_ROMEND
#define input_ports_term3i input_ports_se
#define init_term3i init_term3
CORE_CLONEDEFNV(term3i,term3,"Terminator 3: Rise of the Machines (Italy)",2003,"Stern",de_mSES2,GAME_IMPERFECT_GRAPHICS|GAME_NOCRC)
