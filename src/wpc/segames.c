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
SE128_ROMSTART(apollo13,"apolcpu.501",CRC(5afb8801) SHA1(65608148817f487c384dd36c221138962f1d9824))
DE_DMD32ROM8x(   "a13dspa.500",CRC(bf8e3249) SHA1(5e04681901ca794feb970f5388cb355427cf9a9a))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo13 input_ports_se
CORE_GAMEDEFNV(apollo13,"Apollo 13",1995,"Sega",de_mSES3,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Goldeneye
/-------------------------------------------------------------------*/
INITGAME(gldneye,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(gldneye, "bondcpu.404",CRC(5aa6ffcc) SHA1(0c8ffcfe008a650060c42d385e91addf44f5d88e))
DE_DMD32ROM8x(   "bondispa.400",CRC(9cc0c710) SHA1(3c6df97d881aed9d1d08cc2a5d0c4ec020295902))
DE2S_SOUNDROM144("bondu7.bin" ,CRC(7581a349) SHA1(493236bdc52b601a08009f9b03d64b6047d52661),
                 "bondu17.bin",CRC(d9c56b9d) SHA1(df8cde0b63d6a8437a1cb239094547262c3f8774),
                 "bondu21.bin",CRC(5be0f205) SHA1(aaef8f6ee6c8d5ebf08f90368061288adf850a18))
SE_ROMEND
#define input_ports_gldneye input_ports_se
CORE_GAMEDEFNV(gldneye,"Goldeneye",1996,"Sega",de_mSES3,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Twister
/-------------------------------------------------------------------*/
INITGAME(twst,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(twst_405, "twstcpu.405",CRC(8c3ea1a8) SHA1(d495b7dc79186d442a89b6382a6dc1c83e64ef95))
DE_DMD32ROM8x(   "twstdspa.400",CRC(a6a3d41d) SHA1(ad42b3390ceeeea43c1cd47f300bcd4b4a4d2558))
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798) SHA1(ac591c508de8e9687c20b01c298084c99a251016),
                 "twstsnd.u17",CRC(0e35d640) SHA1(ce38a03fcc321cd9af07d24bf7aa35f254b629fc),
                 "twstsnd.u21",CRC(c3eae590) SHA1(bda3e0a725339069c49c4282676a07b4e0e8d2eb))
SE_ROMEND
#define input_ports_twst input_ports_se
CORE_GAMEDEF(twst,405,"Twister (4.05)",1996,"Sega",de_mSES1,GAME_NOCRC)

SE128_ROMSTART(twst_300, "twstcpu.300",CRC(5cc057d4) SHA1(9ff4b6951a974e3013edc30ba2310c3bffb224d2))
DE_DMD32ROM8x(   "twstdspa.301",CRC(78bc45cb) SHA1(d1915fab46f178c9842e44701c91a0db2495e4fd))
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798) SHA1(ac591c508de8e9687c20b01c298084c99a251016),
                 "twstsnd.u17",CRC(0e35d640) SHA1(ce38a03fcc321cd9af07d24bf7aa35f254b629fc),
                 "twstsnd.u21",CRC(c3eae590) SHA1(bda3e0a725339069c49c4282676a07b4e0e8d2eb))
SE_ROMEND
CORE_CLONEDEF(twst,300,405,"Twister (3.00)",1996,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ ID4: Independence Day
/-------------------------------------------------------------------*/
INITGAME(id4,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4, "id4cpu.202",CRC(108d88fd) SHA1(8317944201acfb97dadfdd364696c9e81a21d2c5))
DE_DMD32ROM8x(    "id4dspa.200",CRC(2d3fbcc4) SHA1(0bd69ebb68ae880ac9aae40916f13e1ff84ecfaa))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37) SHA1(06d79967a25af0b90a5f1d6360a5b5fdbb972d5a),
                  "id4sdu17.400",CRC(385034f1) SHA1(0068901f0f3d97cbcbfc1ffb4df5b195e89218e1),
                  "id4sdu21.400",CRC(f384a9ab) SHA1(06bd607e7efd761017a7b605e0294a34e4c6255c))
SE_ROMEND
#define input_ports_id4 input_ports_se
CORE_GAMEDEFNV(id4,"ID4: Independence Day",1996,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
INITGAME(spacejam,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejam,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspa.300",CRC(198e5e34) SHA1(e2ba9ff1cea84c5d41f32afc50229cb5a18c7666))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a),
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b),
                  "spcjam.u21",CRC(9e7fe0a6) SHA1(187e5893f84d0c0fd70d15c3978fc3fc51e12a51),
                  "spcjam.u36",CRC(7d11e1eb) SHA1(96d4635b1edf8a22947a5cd529ce9025cf7d0c71))
SE_ROMEND
#define input_ports_spacejam input_ports_se
CORE_GAMEDEFNV(spacejam,"Space Jam",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Star Wars Trilogy
/-------------------------------------------------------------------*/
INITGAME(swtril,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(swtril,"swcpu.403",CRC(d022880e) SHA1(c1a2acf5740cef6a8aeee9b6a60942b51147963f))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71) SHA1(036f53e638699de0447ecd02221f673a40f656be))
DE2S_SOUNDROM144("sw0219.u7" ,CRC(cd7c84d9) SHA1(55b0208089933e4a30f0eb87b123dd178383ed43),
                 "sw0211.u17",CRC(6863e7f6) SHA1(99f1e0170fbbb91a0eb7a796ab3bf757cb1b23ce),
                 "sw0211.u21",CRC(6be68450) SHA1(d24652f74b109e47eb5d3d02e04f63c99e92c590))
SE_ROMEND
#define input_ports_swtril input_ports_se
CORE_GAMEDEFNV(swtril,"Star Wars Trilogy",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park
/-------------------------------------------------------------------*/
INITGAME(jplstwld,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(jplstwld,"jp2cpu.202",CRC(d317e601) SHA1(fb4fee5fd08e0f1b5afb9817654bdb3d54a5220d))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace) SHA1(9d11f7623eec41972d2be4313c7715e30116d889))
DE2S_SOUNDROM144("jp2_u7.bin" ,CRC(73b74c96) SHA1(ffa47cbf1491ed4fbadc984189abbfffc70c9888),
                 "jp2_u17.bin",CRC(8d6c0dbd) SHA1(e1179b2c94927a07efa7d16cf841d5ff7334ff36),
                 "jp2_u21.bin",CRC(c670a997) SHA1(1576e11ec3669f61ff16188de31b9ef3a067c473))
SE_ROMEND
#define input_ports_jplstwld input_ports_se
CORE_GAMEDEFNV(jplstwld,"The Lost World: Jurassic Park",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ X-Files
/-------------------------------------------------------------------*/
INITGAME(xfiles,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(xfiles,"xfcpu.303",CRC(c7ab5efe) SHA1(dcb4b89adfb5ba39e59c1118a00b29941d3ea4e9))
DE_DMD32ROM8x(   "xfildspa.300",CRC(03c96af8) SHA1(06a26116f863bb9b2d127e18c5ba500537923d62))
DE2S_SOUNDROM144( "xfsndu7.512" ,CRC(01d65239) SHA1(9e680de940a15ef85a5615b789c58cd5973ff11b),
                  "xfsndu17.c40",CRC(40bfc835) SHA1(2d6ac82acbbf9645bcb84fab7f285f2373e516a8),
                  "xfsndu21.c40",CRC(b56a5ca6) SHA1(5fa23a8bb57e45aca159882226e603d9a6be078b))
SE_ROMEND
#define input_ports_xfiles input_ports_se
CORE_GAMEDEFNV(xfiles,"X-Files",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Starship Troopers
/-------------------------------------------------------------------*/
INITGAME(startrp,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(startrp, "sstcpu.201",CRC(549699fe) SHA1(7d3a12c7a41716ee38b822f924ca94c6932ebc4e))
DE_DMD32ROM8x(    "sstdspa.200",CRC(76a0e09e) SHA1(a4103aeee752d824a3811124079e40acc7286271))
DE2S_SOUNDROM1444("u7_b130.512" ,CRC(f1559e4f) SHA1(82b56f097412052bc1638a3f1c1319009df707f4),
                  "u17_152a.040",CRC(8caeccdb) SHA1(390f07e48a176a24fe99a202f3fa2b9767d84230),
                  "u21_0291.040",CRC(0c5321f6) SHA1(4a51daa16d489ab61d462d44f887c8422f863c5c),
                  "u36_95a7.040",CRC(c1e4ca6a) SHA1(487de78ebf1ee8cc721f2ef7b1bd42d2f7b27456))
SE_ROMEND
#define input_ports_startrp input_ports_se
CORE_GAMEDEFNV(startrp,"Starship Troopers",1997,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Viper Night Drivin'
/-------------------------------------------------------------------*/
INITGAME(viprsega,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(viprsega, "vipcpu.201",CRC(476557aa) SHA1(9018bb6850a3f4b37cc05d9c3ce6e2d9a1931bfd))
DE_DMD32ROM8x(   "vipdspa.201",CRC(24b1dc21) SHA1(73d92083c4795e143e7c34f52032292a142534f4))
DE2S_SOUNDROM14444("vpru7.dat" ,CRC(f21617d7) SHA1(78d1ade400b83c62bb6288bccf386ef34050dd04),
                  "vpru17.dat",CRC(47b1317c) SHA1(32259965b5a12f63267af96eef8396bf71895a65),
                  "vpru21.dat",CRC(0e0e2dd6) SHA1(b409c837a52eb399c9a4896ca0c502360c93dcc9),
                  "vpru36.dat",CRC(7b482876) SHA1(c8960c2d45a77a35d22408c7bb8ba322e7af36f0),
                  "vpru37.dat",CRC(0bf23e0e) SHA1(b5724ed6cfe791320a8cf208cc20a2d3f0db85c8))
SE_ROMEND
#define input_ports_viprsega input_ports_se
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin'",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
INITGAME(lostspc,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspc, "liscpu.101",CRC(81b2ced8) SHA1(a1933e2686b2a4e48d0f327593df95a927b132cb))
DE_DMD32ROM8x(  "lisdspa.102",CRC(e8bf4a58) SHA1(572313fb79e5a0c0034938a09b04ef43fc235c84))
DE2S_SOUNDROM14444("lisu7.100" ,CRC(96e6b3c4) SHA1(5cfb43b8c182aed4b49ad1b8803812a18c6c8b6f),
                  "lisu17.100",CRC(69076939) SHA1(f2cdf61a2b469d1a69eb3f08fc6e511d72336586),
                  "lisu21.100",CRC(56eede09) SHA1(9ff53d7a188bd7293ad92089d143bd54623a50d4),
                  "lisu36.100",CRC(56f2c53b) SHA1(5c2daf17116016fbead1320eb150cf655984662b),
                  "lisu37.100",CRC(f9430c59) SHA1(f0f7169e63fc12d29fe39cd24dd67c5fb17779f7))
SE_ROMEND
#define input_ports_lostspc input_ports_se
CORE_GAMEDEFNV(lostspc,"Lost in Space",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Golden Cue
/-------------------------------------------------------------------*/
INITGAME(goldcue,GEN_WS_1,se_dmd128x32,0)
SE128_ROMSTART(goldcue,"gc_cpu.210",CRC(8447eaee) SHA1(dcca699508809d66dcb2887968c7628067927798))
DE_DMD32ROM8x(        "gc_disp.u12",CRC(87f74b9d) SHA1(e8610ba2409dc6c5070fd413597e3629851b6106))
DE2S_SOUNDROM1444(   "gc_sound.u7", CRC(8b559e39) SHA1(59c33615b53864cd542c8bd3be2ba18e91c57dfd),
                     "gc_sound.u17",CRC(28c39bae) SHA1(a11343a4043d8d5a8eaec383e1bb1f42016e33d2),
                     "gc_sound.u21",CRC(d3f43a37) SHA1(e845370e75200570f828b8452453287b5f599276),
                     "gc_sound.u36",CRC(81f27955) SHA1(eba4250898f6de96111232e49d965b78fc6ee2e2))
SE_ROMEND
#define input_ports_goldcue input_ports_se
CORE_GAMEDEFNV(goldcue,"Golden Cue",1998,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Godzilla
/-------------------------------------------------------------------*/
INITGAME(godzilla,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(godzilla,"gdzcpu.205",CRC(0156c21c) SHA1(37dcbe84c54e1b8f279f78e7a11544040e98c0b3))
DE_DMD32ROM8x(  "gzdspa.200",CRC(a254a01d) SHA1(e624a81437ab4d4b3c133baf47993facf6079f4b))
DE2S_SOUNDROM14444("gdzu7.100" ,CRC(a0afe8b7) SHA1(33e4a824b26b58e8f963fa8a525a64f4779b45db),
                  "gdzu17.100",CRC(6bba69c8) SHA1(51341e188b4191eb1836349dfdd456163d464ad6),
                  "gdzu21.100",CRC(db738958) SHA1(23082cf98bbcc6d356145414267da887a5ca9305),
                  "gdzu36.100",CRC(e3f24234) SHA1(eb123200928221a647e10839ebb7f4628501c581),
                  "gdzu37.100",CRC(2c1acb14) SHA1(4d710e09f5500da937932b4b01d862abb4a89e5a))
SE_ROMEND
#define input_ports_godzilla input_ports_se
CORE_GAMEDEFNV(godzilla,"Godzilla",1998,"Sega",de_mSES1,GAME_NOCRC)

/********************* SEGA GAMES DISTRIBUTED BY STERN  **********************/

/*-------------------------------------------------------------------
/ South Park
/-------------------------------------------------------------------*/
INITGAME(sprk,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(sprk_103,"spkcpu.103",CRC(55ca8aa1) SHA1(ee3dda7d7e6ad32072cbf3acc8087a27b95cc68d))
DE_DMD32ROM8x(    "spdspa.101",CRC(48ca598d) SHA1(0827ac7bb5cf12b0e63860b73a808273d984509e))
DE2S_SOUNDROM18888("spku7.101" ,CRC(3d831d3e) SHA1(fd12e4639bf806c518a2733c32572b051517ff27),
                  "spku17.100",CRC(829262c9) SHA1(88adb13a6773f88658d6b8d6520a03ecd377e4e7),
                  "spku21.100",CRC(3a55eef3) SHA1(2a643bd7a433a39d764294c1569182e6ff0af240),
                  "spku36.100",CRC(bcf6d2cb) SHA1(8e8186f08ff1e39a7889469ec1fdfa9402a8695c),
                  "spku37.100",CRC(7d8f6bcb) SHA1(579cfef19cf9b5c91151ae833bc6c21734589849))
SE_ROMEND
#define input_ports_sprk input_ports_se
CORE_GAMEDEF(sprk,103,"South Park (1.03)",1999,"Sega",de_mSES1,GAME_NOCRC)

SE128_ROMSTART(sprk_090,"spkcpu.090",CRC(bc3f8531) SHA1(5408e008c4f545bb4f82308b118d15525f8a263a))
DE_DMD32ROM8x(    "spdspa.101",CRC(48ca598d) SHA1(0827ac7bb5cf12b0e63860b73a808273d984509e))
DE2S_SOUNDROM18888("spku7.090",CRC(19937fbd) SHA1(ebd7c8f1604accbeb7c00066ecf811193a2cb588),
                  "spku17.090",CRC(05a8670e) SHA1(7c0f1f0c9b94f0327c820f002bffc4ea05670ec8),
                  "spku21.090",CRC(c8629ee7) SHA1(843a742cb5cfce21a83618d14ae08ee1930d36cc),
                  "spku36.090",CRC(727d4624) SHA1(9019014e6057d279a37cc3ce269a1c68baeb9673),
                  "spku37.090",CRC(0c01b0c7) SHA1(76b5af50514d110b49721e6916dd16b3e3a2f5fa))
SE_ROMEND
CORE_CLONEDEF(sprk,090,103,"South Park (0.90)",1999,"Sega",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Harley Davidson
/-------------------------------------------------------------------*/
INITGAME(harley,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(harley,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x(   "hddsp.100",CRC(bdeac0fd) SHA1(5aa1392a13f3c632b660ea6cb3dee23327404d80))
DE2S_SOUNDROM18884("hdsnd.u7" ,CRC(b9accb75) SHA1(9575f1c372ec5603322255778fc003047acc8b01),
                  "hdvc1.u17",CRC(0265fe72) SHA1(7bd7b321bfa2a5092cdf273dfaf4ccdb043c06e8),
                  "hdvc2.u21",CRC(89230898) SHA1(42d225e33ac1d679415e9dbf659591b7c4109740),
                  "hdvc3.u36",CRC(41239811) SHA1(94fceff4dbefd3467ecb8b19e4b8baf24ddd68a3),
                  "hdvc4.u37",CRC(a1bc39f6) SHA1(25af40cb3d8f774e1e37cbef9166e41753440460))
SE_ROMEND
#define input_ports_harley input_ports_se
CORE_GAMEDEFNV(harley,"Harley Davidson",1999,"Sega",de_mSES1,GAME_NOCRC)


/********************* STERN GAMES  **********************/


/*-------------------------------------------------------------------
/ Striker Extreme (USA)
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(strikext,"sxcpua.102", CRC(5e5f0fb8) SHA1(1425d66064e59193ce7cacb496c12ae956670435))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE2S_SOUNDROM18888("sxsounda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                  "sxvoicea.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                  "sxvoicea.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                  "sxvoicea.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                  "sxvoicea.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strikext input_ports_se
CORE_GAMEDEFNV(strikext,"Striker Xtreme (1.02)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (UK)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_uk,"sxcpue.101", CRC(eac29785) SHA1(42e01c501b4a0a7eaae244040777be8ba69860d5))
DE_DMD32ROM8x(     "sxdispa.101",CRC(1d2cb240) SHA1(ab9156281374694ee214d4c05d08eecfdf364f9f))
DE2S_SOUNDROM18888("sxsounda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                  "soceng.u17",CRC(779d8a98) SHA1(c9d57f86ea0059406fec381fbfec0eab75db9b93),
                  "soceng.u21",CRC(47b4838c) SHA1(a954451fc82de6b6de7ac9f6e33f9481088912d7),
                  "soceng.u36",CRC(ab65d6f0) SHA1(4af45213fb2dd923c2ade428b5d96f7c6741c633),
                  "soceng.u37",CRC(d73254f1) SHA1(57e82dc3e582da783b1cc9a822d9b67c4b106803))
SE_ROMEND
#define input_ports_strxt_uk input_ports_se
#define init_strxt_uk init_strikext
CORE_CLONEDEFNV(strxt_uk,strikext,"Striker Xtreme (UK)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_gr,"sxcpug.102", CRC(2686743b) SHA1(891bd5b3952a5e11c7bbf50c6cb25b495cf3a408))
DE_DMD32ROM8x(     "sxdispg.103",CRC(eb656489) SHA1(476315a5d22b6d8c63e9a592167a00f0c87e86c9))
DE2S_SOUNDROM18888("sxsoundg.u7" ,CRC(b38ec07d) SHA1(239a3a51c049b007d4c16c3bd1e003a5dfd3cecc),
                  "sxvoiceg.u17",CRC(19ecf6ca) SHA1(f61f9e821bb0cf7978073a2d2cb939999265277b),
                  "sxvoiceg.u21",CRC(ee410b1e) SHA1(a0f7ff46536060be8f7c2c0e575e85814cd183e1),
                  "sxvoiceg.u36",CRC(f0e126c2) SHA1(a8c5eed27b33d20c2ff3bfd3d317c8b56bfa3625),
                  "sxvoiceg.u37",CRC(82260f4b) SHA1(6c2eba67762bcdd01e7b0c1b8b03b91b778444d4))
SE_ROMEND
#define input_ports_strxt_gr input_ports_se
#define init_strxt_gr init_strikext
CORE_CLONEDEFNV(strxt_gr,strikext,"Striker Xtreme (Germany)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_fr,"sxcpuf.102", CRC(2804bc9f) SHA1(3c75d8cc4d2833baa163d99c709dcb9475ba0831))
DE_DMD32ROM8x(     "sxdispf.103",CRC(4b4b5c19) SHA1(d2612a2b8099b45cb67ac9b55c88b5b10519d49b))
DE2S_SOUNDROM18888("soc.u7" ,CRC(a03131cf) SHA1(e50f665eb15cef799fdc0d1d88bc7d5e23390225),
                  "soc.u17",CRC(e7ee91cb) SHA1(1bc9992601bd7d194e2f33241179d682a62bff4b),
                  "soc.u21",CRC(88cbf553) SHA1(d6afd262b47e31983c734c0054a7af2489da2f13),
                  "soc.u36",CRC(35474ad5) SHA1(818a0f86fa4aa0b0457c16a20f8358655c42ea0a),
                  "soc.u37",CRC(0e53f2a0) SHA1(7b89989ff87c25618d6f1c6479efd45b57f850fb))
SE_ROMEND
#define input_ports_strxt_fr input_ports_se
#define init_strxt_fr init_strikext
CORE_CLONEDEFNV(strxt_fr,strikext,"Striker Xtreme (France)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_it,"sxcpui.102", CRC(f955d0ef) SHA1(0f4ee87715bc085e2fb05e9ebdc89403f6bac444))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2) SHA1(a5e37ecf3b9772736ac88256c470f785dc113aa1))
DE2S_SOUNDROM18888("s00.u7" ,CRC(80625d23) SHA1(3956744f20c5a3281715ff813a8fd37cd8179342),
                  "s00.u17",CRC(d0b21193) SHA1(2e5f92a67f0f18913e5d0af9936ab8694d095c66),
                  "s00.u21",CRC(5ab3f8f4) SHA1(44982725eb31b0b144e3ad6549734b5fc46cd8c5),
                  "s00.u36",CRC(4ee21ade) SHA1(1887f81b5f6753ce75ddcd0d7557c1644a925fcf),
                  "s00.u37",CRC(4427e364) SHA1(7046b65086aafc4c14793d7036bc5130fe1e7dbc))
SE_ROMEND
#define input_ports_strxt_it input_ports_se
#define init_strxt_it init_strikext
CORE_CLONEDEFNV(strxt_it,strikext,"Striker Xtreme (Italy)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Striker Extreme (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_sp,"sxcpul.102", CRC(6b1e417f) SHA1(b87e8659bc4481384913a28c1cb2d7c96532b8e3))
DE_DMD32ROM8x(     "sxdispl.103",CRC(3efd4a18) SHA1(64f6998f82947a5bd053ad8dd56682adb239b676))
DE2S_SOUNDROM18888("soc.u7" ,CRC(a03131cf) SHA1(e50f665eb15cef799fdc0d1d88bc7d5e23390225),
                  "soc.u17",CRC(e7ee91cb) SHA1(1bc9992601bd7d194e2f33241179d682a62bff4b),
                  "soc.u21",CRC(88cbf553) SHA1(d6afd262b47e31983c734c0054a7af2489da2f13),
                  "soc.u36",CRC(35474ad5) SHA1(818a0f86fa4aa0b0457c16a20f8358655c42ea0a),
                  "soc.u37",CRC(0e53f2a0) SHA1(7b89989ff87c25618d6f1c6479efd45b57f850fb))
SE_ROMEND
#define input_ports_strxt_sp input_ports_se
#define init_strxt_sp init_strikext
CORE_CLONEDEFNV(strxt_sp,strikext,"Striker Xtreme (Spain)",1999,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (2.11)
/-------------------------------------------------------------------*/
INITGAME(shrkysht,GEN_WS_1,se_dmd128x32,0)
SE128_ROMSTART(shrkysht,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispa.201",CRC(3360300b) SHA1(3169651a49bb7168fc04b2ae678b696ec6a21c85))
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrkysht input_ports_se
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout (2.11)",2000,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_gr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispg.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrky_gr input_ports_se
#define init_shrky_gr init_shrkysht
CORE_CLONEDEFNV(shrky_gr,shrkysht,"Sharkey's Shootout (Germany)",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_fr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispf.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrky_fr input_ports_se
#define init_shrky_fr init_shrkysht
CORE_CLONEDEFNV(shrky_fr,shrkysht,"Sharkey's Shootout (France)",2001,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_it,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispi.201",NO_DUMP)
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
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
SE128_ROMSTART(hirolcas,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispa.300",CRC(099ccaf0) SHA1(2e0c2706881208f08e8a1d30915424c8f9b1cf67))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirolcas input_ports_se
CORE_GAMEDEFNV(hirolcas,"High Roller Casino (3.00)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_fr,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispf.300",NO_DUMP)
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_fr input_ports_se
#define init_hirol_fr init_hirolcas
CORE_CLONEDEFNV(hirol_fr,hirolcas,"High Roller Casino (France)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (Germany, 2.10)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_gr,"hrccpu.210",CRC(2e3c682a) SHA1(d9993ae7a0aad80e1eeff226a635873cb25437ce))
DE_DMD32ROM8x(        "hrcdispg.201",CRC(57b95712) SHA1(f7abe7511aa8b258615cd844dc76f3d2f9b47c31))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_gr input_ports_se
#define init_hirol_gr init_hirolcas
CORE_CLONEDEFNV(hirol_gr,hirolcas,"High Roller Casino (Germany, 2.10)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ High Roller Casino (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_it,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispi.300",NO_DUMP)
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_it input_ports_se
#define init_hirol_it init_hirolcas
CORE_CLONEDEFNV(hirol_it,hirolcas,"High Roller Casino (Italy)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (3.02)
/-------------------------------------------------------------------*/
INITGAME(austin,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(austin,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb) SHA1(952a7873067b8c70043a38a39a8f65089103336b))
DE2S_SOUNDROM18888( "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                   "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                   "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                   "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                   "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austin input_ports_se
CORE_GAMEDEFNV(austin,"Austin Powers (3.02)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austinf,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-f.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austinf input_ports_se
#define init_austinf init_austin
CORE_CLONEDEFNV(austinf,austin,"Austin Powers (France)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austing,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-g.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austing input_ports_se
#define init_austing init_austin
CORE_CLONEDEFNV(austing,austin,"Austin Powers (Germany)",2001,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Austin Powers (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austini,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-i.300",NO_DUMP)
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austini input_ports_se
#define init_austini init_austin
CORE_CLONEDEFNV(austini,austin,"Austin Powers (Italy)",2001,"Stern",de_mSES2,GAME_NOCRC)

// Monopoly moved to its own sim file (gv)

/*-------------------------------------------------------------------
/ Playboy (5.0)
/-------------------------------------------------------------------*/
INITGAME(playboys,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(playboys, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispa.500",  CRC(40450165) SHA1(80295466838cb08fe1499b19a98bf759fb7a306e))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playboys input_ports_se
CORE_GAMEDEFNV(playboys,"Playboy (5.00)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyf, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispf.500",  CRC(aedc6c32) SHA1(c930ae1b1308ae641553de34f8249b17f408be56))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playboyf input_ports_se
#define init_playboyf init_playboys
CORE_CLONEDEFNV(playboyf,playboys,"Playboy (France)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyg, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispg.500",  CRC(681392fe) SHA1(23011d538282da144b8ff9cbb7c5655567017e5e))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playboyg input_ports_se
#define init_playboyg init_playboys
CORE_CLONEDEFNV(playboyg,playboys,"Playboy (Germany)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyi, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispi.500",  CRC(d90d7ec6) SHA1(7442160f403a8ccabfc9dc8dc53f8b44f5df26bc))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playboyi input_ports_se
#define init_playboyi init_playboys
CORE_CLONEDEFNV(playboyi,playboys,"Playboy (Italy)",2002,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Playboy (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playboyl, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispl.500",  CRC(b019f0f6) SHA1(184a9905cd3af9d272577e43666aed5e5a8a5281))
DE2S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playboyl input_ports_se
#define init_playboyl init_playboys
CORE_CLONEDEFNV(playboyl,playboys,"Playboy (Spain)",2002,"Stern",de_mSES2,GAME_NOCRC)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (7.01)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispRCT[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 5,21, CORE_DMD|CORE_DMDNOAA, (void *)seminidmd3_update}, {0}
};
INITGAME(rctycn, GEN_WS, dispRCT, SE_MINIDMD)
SE128_ROMSTART(rctycn, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdispa.700",CRC(6a8925d7) SHA1(82a6c069f1e8f8e053fec708f56c8ffe56d70fc8))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycn input_ports_se
CORE_GAMEDEFNV(rctycn,"Roller Coaster Tycoon (7.01)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycng,"rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdispg.700",CRC(0babf1ed) SHA1(db683aa392968d255d355d4a1b0c9d8d4fb9e799))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycng input_ports_se
#define init_rctycng init_rctycn
CORE_CLONEDEFNV(rctycng,rctycn,"Roller Coaster Tycoon (Germany)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnf,"rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdispf.700",CRC(2de0eec8) SHA1(5b48eabddc1fb735866767ae008baf58205954db))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycnf input_ports_se
#define init_rctycnf init_rctycn
CORE_CLONEDEFNV(rctycnf,rctycn,"Roller Coaster Tycoon (France)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycni,"rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdispi.700",CRC(0d07d8f2) SHA1(3ffd8ad7183ba20844c1e5d1933c8002ca4707aa))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycni input_ports_se
#define init_rctycni init_rctycn
CORE_CLONEDEFNV(rctycni,rctycn,"Roller Coaster Tycoon (Italy)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnl,"rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdisps.700",CRC(6921d8cc) SHA1(1ada415af8e949829ceac75da507982ea2091f4d))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycnl input_ports_se
#define init_rctycnl init_rctycn
CORE_CLONEDEFNV(rctycnl,rctycn,"Roller Coaster Tycoon (Spain)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (4.00)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispSPP[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10,10,14, CORE_DMD|CORE_DMDNOAA, (void *)seminidmd4_update}, {0}
};
static core_tGameData simpprtyGameData = { \
  GEN_WS, dispSPP, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 4, 0, 0, SE_MINIDMD2}}; \
static void init_simpprty(void) { core_gameData = &simpprtyGameData; }
SE128_ROMSTART(simpprty, "spp-cpu.400",CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspa.400",CRC(cd5eaab7) SHA1(a06bef6fc0e7f3c0616439cb0e0431a3d52cdfa1))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17", CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21", CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36", CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37", CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprty input_ports_se
CORE_GAMEDEFNV(simpprty,"The Simpsons Pinball Party (4.00)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtg, "spp-cpu.400",CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspg.400",NO_DUMP)
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17", CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21", CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36", CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37", CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtg input_ports_se
#define init_simpprtg init_simpprty
CORE_CLONEDEFNV(simpprtg,simpprty,"The Simpsons Pinball Party (Germany)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtl, "spp-cpu.400",CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspl.400",CRC(a0bf567e) SHA1(ce6eb65da6bff15aeb787fd2cdac7cf6b4300108))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17", CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21", CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36", CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37", CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtl input_ports_se
#define init_simpprtl init_simpprty
CORE_CLONEDEFNV(simpprtl,simpprty,"The Simpsons Pinball Party (Spain)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtf, "spp-cpu.400",CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspf.400",CRC(6cc306e2) SHA1(bfe6ef0cd5d0cb5e3b29d85ade1700005e22d81b))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17", CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21", CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36", CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37", CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtf input_ports_se
#define init_simpprtf init_simpprty
CORE_CLONEDEFNV(simpprtf,simpprty,"The Simpsons Pinball Party (France)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprti, "spp-cpu.400",CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspi.400",CRC(ebe45dee) SHA1(4cdf0f01b1df1fa35df67f19c67b82a39d887be8))
DE2S_SOUNDROM18888(      "spp101.u7",  CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17", CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21", CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36", CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37", CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprti input_ports_se
#define init_simpprti init_simpprty
CORE_CLONEDEFNV(simpprti,simpprty,"The Simpsons Pinball Party (Italy)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (4.00)
/-------------------------------------------------------------------*/
INITGAME(term3,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(term3, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(      "t3dispa.400", CRC(6b7cc4f8) SHA1(214e9b3e45b778841fc166acf4ff5fd634ae2670))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3 input_ports_se
CORE_GAMEDEFNV(term3,"Terminator 3: Rise of the Machines (4.00)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(term3g, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispg.400", CRC(20da21b2) SHA1(9115aef55d9ac36a49ae5c5fd423f05c669b0335))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3g input_ports_se
#define init_term3g init_term3
CORE_CLONEDEFNV(term3g,term3,"Terminator 3: Rise of the Machines (Germany)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(term3l, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3displ.400", CRC(2e21caba) SHA1(d29afa05d68484c762799c799bd1ccd1aad252b7))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3l input_ports_se
#define init_term3l init_term3
CORE_CLONEDEFNV(term3l,term3,"Terminator 3: Rise of the Machines (Spain)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(term3f, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispf.400", CRC(0645fe6d) SHA1(1a7dfa160ba6cc1335a59b018289982f2a46a7bb))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3f input_ports_se
#define init_term3f init_term3
CORE_CLONEDEFNV(term3f,term3,"Terminator 3: Rise of the Machines (France)",2003,"Stern",de_mSES2,GAME_NOCRC)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(term3i, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispi.400", CRC(e8ea9ab8) SHA1(7b25bb7d3095e6bd2d94342d0e078590cb75074b))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3i input_ports_se
#define init_term3i init_term3
CORE_CLONEDEFNV(term3i,term3,"Terminator 3: Rise of the Machines (Italy)",2003,"Stern",de_mSES2,GAME_NOCRC)

// Using 16-bit samples (and a new sound CPU: ARM-7 type) from now on!
/*-------------------------------------------------------------------
/ The Lord Of The Rings (8.00)
/-------------------------------------------------------------------*/
static core_tGameData lotrGameData = { \
  GEN_WS, se_dmd128x32, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 5, 0, 0, SE_LED}}; \
static void init_lotr(void) { core_gameData = &lotrGameData; }
SE128_ROMSTART(lotr, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(      "lotrdspa.800", CRC(2aa1f00d) SHA1(e9df5b61b467c307aacdb5a6980a78af26492e6a))
DE2S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr input_ports_se
CORE_GAMEDEFNV(lotr,"Lord Of The Rings, The (8.00)",2003,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_sp, "lotrcpul.800", CRC(38e83068) SHA1(603b1236bf195c72d32e5a1088a2806e77176436))
DE_DMD32ROM8x(          "lotrdspl.800", CRC(137c1255) SHA1(43d9ffec18ab2aa80f30b195ca5270d4574d7b8d))
DE2S_SOUNDROM18888(      "lotrlu7.100", CRC(980d970a) SHA1(cf70deddcc146ef9eaa64baec74ae800bebf8715),
                         "lotrlu17.100",CRC(c16d3e20) SHA1(43d9f186db361abb3aa119a7252f1bb13bbbbe39),
                         "lotrlu21.100",CRC(f956a1be) SHA1(2980e85463704a154ed81d3241f866442d1ea2e6),
                         "lotrlu36.100",CRC(502c3d93) SHA1(4ee42d70bccb9253fe1f8f441de6b5018257f107),
                         "lotrlu37.100",CRC(61f21c6d) SHA1(3e9781b4981bd18cdb8c59c55b9942de6ae286db))
SE_ROMEND
#define input_ports_lotr_sp input_ports_lotr
#define init_lotr_sp init_lotr
CORE_CLONEDEFNV(lotr_sp,lotr,"Lord Of The Rings, The (Spain)",2003,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_gr, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(         "lotrdspg.800", CRC(55765c23) SHA1(690a72e8cb1099a6873eb3214e72bb0fea54fa22))
DE2S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_gr input_ports_lotr
#define init_lotr_gr init_lotr
CORE_CLONEDEFNV(lotr_gr,lotr,"Lord Of The Rings, The (Germany)",2003,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_fr, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(         "lotrdspf.800", CRC(e1ccc04b) SHA1(1d5c7ea06f0cb2e1965c968ed01330867aae8e2b))
DE2S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_fr input_ports_lotr
#define init_lotr_fr init_lotr
CORE_CLONEDEFNV(lotr_fr,lotr,"Lord Of The Rings, The (France)",2003,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_it, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(         "lotrdspi.800", CRC(b80730d7) SHA1(552c0bfac1c7a6b246829378a30d58769e695f7e))
DE2S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_it input_ports_lotr
#define init_lotr_it init_lotr
CORE_CLONEDEFNV(lotr_it,lotr,"Lord Of The Rings, The (Italy)",2003,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (3.00)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispBION[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 5, CORE_DMD, (void *)seminidmd1a_update},
  {34,18, 7, 5, CORE_DMD, (void *)seminidmd1b_update},
  {34,26, 7, 5, CORE_DMD, (void *)seminidmd1c_update}, {0}
};
INITGAME(ripleys,GEN_WS,dispBION,SE_MINIDMD)
SE128_ROMSTART(ripleys, "ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
DE2S_SOUNDROM18888(      "ripsnd.u7",CRC(e5c7edea) SHA1(c0075fb35c746afe54808d4901ddb61ba396f36d),
                        "ripsnd.u17",CRC(d518f2da) SHA1(e7d75c6b7b45571ae6d39ed7405b1457e475b52a),
                        "ripsnd.u21",CRC(3d8680d7) SHA1(1368965106094d78be6540eb87a478f853ba774f),
                        "ripsnd.u36",CRC(b697b5cb) SHA1(b5cb426201287a6d1c40db8c81a58e2c656d1d81),
                        "ripsnd.u37",CRC(01b9f20e) SHA1(cffb6a0136d7d17ab4450b3bfd97632d8b669d39))
SE_ROMEND
#define input_ports_ripleys input_ports_se
CORE_GAMEDEFNV(ripleys,"Ripley's Believe It or Not! (3.00)",2004,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysf,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispf.300",CRC(b9901941) SHA1(653997ff5d63e7ee0270db08cad952ac8293a8cd))
DE2S_SOUNDROM18888(     "ripsndf.u7",CRC(00fc94de) SHA1(207ceea5ccd39a99c5b5ed1f945dce3f1fc076d7),
                       "ripsndf.u17",CRC(a6793b85) SHA1(96058777346be6e9ea7b1340d9aaf945ac3c853a),
                       "ripsndf.u21",CRC(60c02170) SHA1(900d9de3ccb541019e5f1528e01c57ad96dac262),
                       "ripsndf.u36",CRC(0a57f2fd) SHA1(9dd057888294ee8abeb582e8f6650fd6e32cc9ff),
                       "ripsndf.u37",CRC(5c858958) SHA1(f4a9833b8aee033ed381e3bdf9f801b935d6186a))
SE_ROMEND
#define input_ports_ripleysf input_ports_ripleys
#define init_ripleysf init_ripleys
CORE_CLONEDEFNV(ripleysf,ripleys,"Ripley's Believe It or Not! (France)",2004,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysg,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
DE2S_SOUNDROM18888(     "ripsndg.u7",CRC(ff7b6b55) SHA1(e2e0e1f6cfae0fbcb5151bf2cb17fbb52f2d9f23),
                       "ripsndg.u17",CRC(c387dcf0) SHA1(d4ef65d3f33ab82b63bf2782f335858ab4ad210a),
                       "ripsndg.u21",CRC(6388ae8d) SHA1(a39c7977194daabf3f5b10d0269dcd4118a939bc),
                       "ripsndg.u36",CRC(3143f9d3) SHA1(bd4ce64b245b5fcb9b9694bd8f71a9cd98303cae),
                       "ripsndg.u37",CRC(2167617b) SHA1(62b55a39e2677eec9d56b10e8cc3e5d7c0d3bea5))
SE_ROMEND
#define input_ports_ripleysg input_ports_ripleys
#define init_ripleysg init_ripleys
CORE_CLONEDEFNV(ripleysg,ripleys,"Ripley's Believe It or Not! (Germany)",2004,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysi,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
DE2S_SOUNDROM18888(     "ripsndi.u7",CRC(b966a2c1) SHA1(1aa83dc5b87fc16fc4281f577e61eb2661cfad09),
                       "ripsndi.u17",CRC(a2911df4) SHA1(acb7956a6a30142c8da905b04778a074cb335807),
                       "ripsndi.u21",CRC(1467eaff) SHA1(c6c4ea2abdad4334efbe3a084693e9e4d0dd0fd2),
                       "ripsndi.u36",CRC(6a124fa6) SHA1(752c3d227b9a98dd859e4778ddd527edaa3cf512),
                       "ripsndi.u37",CRC(7933c102) SHA1(f736ee86d7c67dab82c634d125d73a2453249706))
SE_ROMEND
#define input_ports_ripleysi input_ports_ripleys
#define init_ripleysi init_ripleys
CORE_CLONEDEFNV(ripleysi,ripleys,"Ripley's Believe It or Not! (Italy)",2004,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysl,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispl.300",CRC(d2f496bb) SHA1(48622e25171030b83d8d1736735e97a13c5f47c6))
DE2S_SOUNDROM18888(     "ripsndl.u7",CRC(66bfcb7c) SHA1(8bdfaf01f367de8ccf15da59000ecff5268e0a4f),
                       "ripsndl.u17",CRC(a98f4514) SHA1(e87ee8f5a87a8ae9ec996473bf9bc745105ea334),
                       "ripsndl.u21",CRC(141f2b77) SHA1(15bab623beda8ae7ed9908f492ff2baab0a7954e),
                       "ripsndl.u36",CRC(c5461b63) SHA1(fc574d44ad88ce1db590ea371225092c03fc6f80),
                       "ripsndl.u37",CRC(2a58f491) SHA1(1c33f419420b3165ef18598560007ef612b24814))
SE_ROMEND
#define input_ports_ripleysl input_ports_ripleys
#define init_ripleysl init_ripleys
CORE_CLONEDEFNV(ripleysl,ripleys,"Ripley's Believe It or Not! (Spain)",2004,"Stern",de_mSES2,GAME_NO_SOUND|GAME_NOCRC)
