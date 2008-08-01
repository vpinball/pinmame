#include "driver.h"
#include "core.h"
#include "dedmd.h"
#include "desound.h"
#include "se.h"
#include "vpintf.h"

/* NO OUTPUT */
static PINMAME_VIDEO_UPDATE(led_update) {
  return 1;
}

static struct core_dispLayout se_dmd128x32[] = {
  {0,0, 32,128, CORE_DMD, (void *)dedmd32_update}, {0}
};

#define INITGAME(name, gen, disp, hw) \
static core_tGameData name##GameData = { \
  gen, disp, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 2, 0, 0, hw}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

SE_INPUT_PORTS_START(se, 1) SE_INPUT_PORTS_END

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/* Games below support Stereo */

/*-------------------------------------------------------------------
/ Apollo 13
/-------------------------------------------------------------------*/
static struct core_dispLayout se_apollo[] = {
  {0,0, 32,128, CORE_DMD, (void *)dedmd32_update},
  {7,0,  0,  1, CORE_SEG7, (void *)led_update},
  {0}
};
INITGAME(apollo13,GEN_WS,se_apollo,SE_DIGIT)
SE128_ROMSTART(apollo13,"apolcpu.501",CRC(5afb8801) SHA1(65608148817f487c384dd36c221138962f1d9824))
DE_DMD32ROM8x(   "a13dspa.500",CRC(bf8e3249) SHA1(5e04681901ca794feb970f5388cb355427cf9a9a))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo13 input_ports_se
CORE_GAMEDEFNV(apollo13,"Apollo 13",1995,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(gldneye,"Goldeneye",1996,"Sega",de_mSES1,0)

/* Stereo? Speaker test shows Left Channel & Both Speaker Test Only */

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
CORE_GAMEDEF(twst,405,"Twister (4.05)",1996,"Sega",de_mSES1,0)

SE128_ROMSTART(twst_404, "twstcpu.404",CRC(43ac7f8b) SHA1(fc087b741c00baa9093dfec009440a7d64f4ca65))
DE_DMD32ROM8x(   "twstdspa.400",CRC(a6a3d41d) SHA1(ad42b3390ceeeea43c1cd47f300bcd4b4a4d2558))
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798) SHA1(ac591c508de8e9687c20b01c298084c99a251016),
                 "twstsnd.u17",CRC(0e35d640) SHA1(ce38a03fcc321cd9af07d24bf7aa35f254b629fc),
                 "twstsnd.u21",CRC(c3eae590) SHA1(bda3e0a725339069c49c4282676a07b4e0e8d2eb))
SE_ROMEND
CORE_CLONEDEF(twst,404,405,"Twister (4.04)",1996,"Sega",de_mSES1,0)

SE128_ROMSTART(twst_300, "twstcpu.300",CRC(5cc057d4) SHA1(9ff4b6951a974e3013edc30ba2310c3bffb224d2))
DE_DMD32ROM8x(   "twstdspa.301",CRC(78bc45cb) SHA1(d1915fab46f178c9842e44701c91a0db2495e4fd))
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798) SHA1(ac591c508de8e9687c20b01c298084c99a251016),
                 "twstsnd.u17",CRC(0e35d640) SHA1(ce38a03fcc321cd9af07d24bf7aa35f254b629fc),
                 "twstsnd.u21",CRC(c3eae590) SHA1(bda3e0a725339069c49c4282676a07b4e0e8d2eb))
SE_ROMEND
CORE_CLONEDEF(twst,300,405,"Twister (3.00)",1996,"Sega",de_mSES1,0)

/* Games below support Mono Only (Externally)! */

/*-------------------------------------------------------------------
/ ID4: Independence Day
/-------------------------------------------------------------------*/
INITGAME(id4,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4, "id4cpu.202",CRC(108d88fd) SHA1(8317944201acfb97dadfdd364696c9e81a21d2c5))
DE_DMD32ROM8x(    "id4dspa.200",CRC(2d3fbcc4) SHA1(0bd69ebb68ae880ac9aae40916f13e1ff84ecfaa))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37) SHA1(06d79967a25af0b90a5f1d6360a5b5fdbb972d5a),
                  "id4sdu17.400",CRC(89ffeca3) SHA1(b94c60e3a433f797d6c5ea793c3ecff0a3b6ba60),
                  "id4sdu21.400",CRC(f384a9ab) SHA1(06bd607e7efd761017a7b605e0294a34e4c6255c))
SE_ROMEND
#define input_ports_id4 input_ports_se
CORE_GAMEDEFNV(id4,"ID4: Independence Day",1996,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
INITGAME(spacejam,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejam,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspa.300",CRC(198e5e34) SHA1(e2ba9ff1cea84c5d41f32afc50229cb5a18c7666))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a),
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b),
                  "spcjam.u21",CRC(14cb71cb) SHA1(46752c1792c26345abb4d5219917a1cda50c600b),
                  "spcjam.u36",CRC(7f61143c) SHA1(40695d1d14695d3e4991ed39f4a354c16227975e))
SE_ROMEND
#define input_ports_spacejam input_ports_se
CORE_GAMEDEFNV(spacejam,"Space Jam",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Space Jam (Germany)
/-------------------------------------------------------------------*/
INITGAME(spacejmg,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejmg,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspg.300",CRC(41f6e188) SHA1(da2247022aadb0ead5a3b1d7b829c13ff1153ec8))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a),
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b),
                  "spcjam.u21",CRC(9e7fe0a6) SHA1(187e5893f84d0c0fd70d15c3978fc3fc51e12a51),
                  "spcjam.u36",CRC(7d11e1eb) SHA1(96d4635b1edf8a22947a5cd529ce9025cf7d0c71))
SE_ROMEND
#define input_ports_spacejmg input_ports_se
#define init_spacejmg init_spacejam
CORE_CLONEDEFNV(spacejmg,spacejam,"Space Jam (Germany)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Space Jam (France)
/-------------------------------------------------------------------*/
INITGAME(spacejmf,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejmf,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspf.300",CRC(1683909f) SHA1(e14810a5d8704ea052fddcb3b54043bf9d57b296))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a),
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b),
                  "spcjam.u21",CRC(9e7fe0a6) SHA1(187e5893f84d0c0fd70d15c3978fc3fc51e12a51),
                  "spcjam.u36",CRC(7d11e1eb) SHA1(96d4635b1edf8a22947a5cd529ce9025cf7d0c71))
SE_ROMEND
#define input_ports_spacejmf input_ports_se
#define init_spacejmf init_spacejam
CORE_CLONEDEFNV(spacejmf,spacejam,"Space Jam (France)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Space Jam (Italy)
/-------------------------------------------------------------------*/
INITGAME(spacejmi,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejmi,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspi.300",CRC(eb9b5971) SHA1(0bfac0511d0cd9d56eee59782c199ee0a78abe5e))
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a),
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b),
                  "spcjam.u21",CRC(9e7fe0a6) SHA1(187e5893f84d0c0fd70d15c3978fc3fc51e12a51),
                  "spcjam.u36",CRC(7d11e1eb) SHA1(96d4635b1edf8a22947a5cd529ce9025cf7d0c71))
SE_ROMEND
#define input_ports_spacejmi input_ports_se
#define init_spacejmi init_spacejam
CORE_CLONEDEFNV(spacejmi,spacejam,"Space Jam (Italy)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Star Wars Trilogy (4.03)
/-------------------------------------------------------------------*/
INITGAME(swtril43,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(swtril43,"swcpu.403",CRC(d022880e) SHA1(c1a2acf5740cef6a8aeee9b6a60942b51147963f))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71) SHA1(036f53e638699de0447ecd02221f673a40f656be))
DE2S_SOUNDROM144("sw0219.u7" ,CRC(cd7c84d9) SHA1(55b0208089933e4a30f0eb87b123dd178383ed43),
                 "sw0211.u17",CRC(6863e7f6) SHA1(99f1e0170fbbb91a0eb7a796ab3bf757cb1b23ce),
                 "sw0211.u21",CRC(6be68450) SHA1(d24652f74b109e47eb5d3d02e04f63c99e92c590))
SE_ROMEND
#define input_ports_swtril43 input_ports_se
CORE_GAMEDEFNV(swtril43,"Star Wars Trilogy (4.03)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Star Wars Trilogy (4.01)
/-------------------------------------------------------------------*/
INITGAME(swtril41,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(swtril41,"swcpu.401",CRC(707dce87) SHA1(45fc3ffe646e5be72af9f7f00990ee5f85338f34))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71) SHA1(036f53e638699de0447ecd02221f673a40f656be))
DE2S_SOUNDROM144("sw0219.u7" ,CRC(cd7c84d9) SHA1(55b0208089933e4a30f0eb87b123dd178383ed43),
                 "sw0211.u17",CRC(6863e7f6) SHA1(99f1e0170fbbb91a0eb7a796ab3bf757cb1b23ce),
                 "sw0211.u21",CRC(6be68450) SHA1(d24652f74b109e47eb5d3d02e04f63c99e92c590))
SE_ROMEND
#define input_ports_swtril41 input_ports_se
#define init_swtril41 init_swtril43
CORE_CLONEDEFNV(swtril41,swtril43,"Star Wars Trilogy (4.01)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park (2.02)
/-------------------------------------------------------------------*/
INITGAME(jplstw22,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(jplstw22,"jp2cpu.202",CRC(d317e601) SHA1(fb4fee5fd08e0f1b5afb9817654bdb3d54a5220d))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace) SHA1(9d11f7623eec41972d2be4313c7715e30116d889))
DE2S_SOUNDROM144("jp2_u7.bin" ,CRC(73b74c96) SHA1(ffa47cbf1491ed4fbadc984189abbfffc70c9888),
                 "jp2_u17.bin",CRC(8d6c0dbd) SHA1(e1179b2c94927a07efa7d16cf841d5ff7334ff36),
                 "jp2_u21.bin",CRC(c670a997) SHA1(1576e11ec3669f61ff16188de31b9ef3a067c473))
SE_ROMEND
#define input_ports_jplstw22 input_ports_se
CORE_GAMEDEFNV(jplstw22,"Lost World: Jurassic Park, The (2.02)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park (2.00)
/-------------------------------------------------------------------*/
INITGAME(jplstw20,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(jplstw20,"jp2cpu.200",CRC(42f5526e) SHA1(66254492c981117c06567305237cadfc0ce391b0))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace) SHA1(9d11f7623eec41972d2be4313c7715e30116d889))
DE2S_SOUNDROM144("jp2_u7.bin" ,CRC(73b74c96) SHA1(ffa47cbf1491ed4fbadc984189abbfffc70c9888),
                 "jp2_u17.bin",CRC(8d6c0dbd) SHA1(e1179b2c94927a07efa7d16cf841d5ff7334ff36),
                 "jp2_u21.bin",CRC(c670a997) SHA1(1576e11ec3669f61ff16188de31b9ef3a067c473))
SE_ROMEND
#define input_ports_jplstw20 input_ports_se
#define init_jplstw20 init_jplstw22
CORE_CLONEDEFNV(jplstw20,jplstw22,"Lost World: Jurassic Park, The (2.00)",1997,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(xfiles,"X-Files",1997,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(startrp,"Starship Troopers",1997,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin'",1998,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(lostspc,"Lost in Space",1998,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(goldcue,"Golden Cue",1998,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(godzilla,"Godzilla",1998,"Sega",de_mSES1,0)

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
CORE_GAMEDEF(sprk,103,"South Park (1.03)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(sprk_090,"spkcpu.090",CRC(bc3f8531) SHA1(5408e008c4f545bb4f82308b118d15525f8a263a))
DE_DMD32ROM8x(    "spdspa.101",CRC(48ca598d) SHA1(0827ac7bb5cf12b0e63860b73a808273d984509e))
DE2S_SOUNDROM18888("spku7.090",CRC(19937fbd) SHA1(ebd7c8f1604accbeb7c00066ecf811193a2cb588),
                  "spku17.090",CRC(05a8670e) SHA1(7c0f1f0c9b94f0327c820f002bffc4ea05670ec8),
                  "spku21.090",CRC(c8629ee7) SHA1(843a742cb5cfce21a83618d14ae08ee1930d36cc),
                  "spku36.090",CRC(727d4624) SHA1(9019014e6057d279a37cc3ce269a1c68baeb9673),
                  "spku37.090",CRC(0c01b0c7) SHA1(76b5af50514d110b49721e6916dd16b3e3a2f5fa))
SE_ROMEND
CORE_CLONEDEF(sprk,090,103,"South Park (0.90)",1999,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Harley Davidson
/-------------------------------------------------------------------*/
#define HARLEY_SOUND \
DE2S_SOUNDROM18884("hdsnd.u7",CRC(b9accb75) SHA1(9575f1c372ec5603322255778fc003047acc8b01), \
                  "hdvc1.u17",CRC(0265fe72) SHA1(7bd7b321bfa2a5092cdf273dfaf4ccdb043c06e8), \
                  "hdvc2.u21",CRC(89230898) SHA1(42d225e33ac1d679415e9dbf659591b7c4109740), \
                  "hdvc3.u36",CRC(41239811) SHA1(94fceff4dbefd3467ecb8b19e4b8baf24ddd68a3), \
                  "hdvc4.u37",CRC(a1bc39f6) SHA1(25af40cb3d8f774e1e37cbef9166e41753440460))
#define input_ports_harl input_ports_se

INITGAME(harl,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(harl_a13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispa.104",CRC(fc7c2924) SHA1(172fceb4d3221608f48a4abe4c4c5f3043834957)) HARLEY_SOUND
SE_ROMEND
CORE_GAMEDEF(harl,a13,"Harley Davidson (1.03)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(harl_a10,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispa.100",CRC(bdeac0fd) SHA1(5aa1392a13f3c632b660ea6cb3dee23327404d80)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,a10,a13,"Harley Davidson (1.03, Display rev. 1.00)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(harl_f13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispf.104",CRC(5f80436e) SHA1(e89e561807670118c3d9e623d4aec2321c774576)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,f13,a13,"Harley Davidson (1.03, France)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(harl_g13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispg.104",CRC(c7f197a0) SHA1(3b7f0699c08d387c67ff6cd185360e60fcd21b9e)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,g13,a13,"Harley Davidson (1.03, Germany)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(harl_i13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispi.104",CRC(387a5aad) SHA1(a0eb99b240f6044db05668c4504e908aee205220)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,i13,a13,"Harley Davidson (1.03, Italy)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(harl_l13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddisps.104",CRC(2d26514a) SHA1(f15b22cad6329f29cd5cccfb91a2ba7ca2cd6d59)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,l13,a13,"Harley Davidson (1.03, Spain)",1999,"Sega",de_mSES1,0)

/********************* STERN GAMES  **********************/

SE128_ROMSTART(harl_a30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispa.300",CRC(61b274f8) SHA1(954e4b3527cefcb24376de9f6f7e5f9192ab3304)) HARLEY_SOUND
SE_ROMEND
CORE_GAMEDEF(harl,a30,"Harley Davidson (3.00)",2004,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_f30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispf.300",CRC(106f7f1f) SHA1(92a8ab7d834439a2211208e0812cdb1199acb21d)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,f30,a30,"Harley Davidson (3.00, France)",2004,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_g30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispg.300",CRC(8f7da748) SHA1(fee1534b76769517d4e6dbed373583e573fb95b6)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,g30,a30,"Harley Davidson (3.00, Germany)",2004,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_i30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispi.300",CRC(686d3cf6) SHA1(fb27e2e4b39abb56deb1e66f012d151126971474)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,i30,a30,"Harley Davidson (3.00, Italy)",2004,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_l30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispl.300",CRC(4cc7251b) SHA1(7660fca37ac9fb442a059ddbafc2fa13f94dfae1)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,l30,a30,"Harley Davidson (3.00, Spain)",2004,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_a18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispa.105",CRC(401a7b9f) SHA1(37e99a42738c1147c073585391772ecc55c9a759)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,a18,a30,"Harley Davidson (1.08)",2003,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_f18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispf.105",CRC(31c77078) SHA1(8a0e2dbb698da77dffa1ab01df0f360fecf6c4c7)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,f18,a30,"Harley Davidson (1.08, France)",2003,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_g18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispg.105",CRC(aed5a82f) SHA1(4c44b052a9b1fa702ff49c9b2fb7cf48173459d2)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,g18,a30,"Harley Davidson (1.08, Germany)",2003,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_i18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispi.105",CRC(49c53391) SHA1(98f88eb8a49bbc59f78996d713c72ec495ba806f)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,i18,a30,"Harley Davidson (1.08, Italy)",2003,"Stern",de_mSES1,0)

SE128_ROMSTART(harl_l18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddisps.105",CRC(6d6f2a7c) SHA1(1609c69a1584398c3504bb5a0c46f878e8dd547c)) HARLEY_SOUND
SE_ROMEND
CORE_CLONEDEF(harl,l18,a30,"Harley Davidson (1.08, Spain)",2003,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Striker Extreme (USA)
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(strikext,"sxcpua.102", CRC(5e5f0fb8) SHA1(1425d66064e59193ce7cacb496c12ae956670435))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE2S_SOUNDROM18888("sxsnda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                  "sxsnda.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                  "sxsnda.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                  "sxsnda.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                  "sxsnda.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strikext input_ports_se
CORE_GAMEDEFNV(strikext,"Striker Xtreme (1.02)",1999,"Stern",de_mSES2,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Striker Extreme (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strknew,"sxcpua.102", CRC(5e5f0fb8) SHA1(1425d66064e59193ce7cacb496c12ae956670435))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE3S_SOUNDROM18888("sxsnda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                  "sxsnda.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                  "sxsnda.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                  "sxsnda.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                  "sxsnda.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strknew input_ports_se
#define init_strknew init_strikext
CORE_CLONEDEFNV(strknew,strikext,"Striker Xtreme (ARM7 Sound Board)",1999,"Stern",de_mSES3,0)
#endif

/*-------------------------------------------------------------------
/ Striker Extreme (UK)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_uk,"sxcpue.101", CRC(eac29785) SHA1(42e01c501b4a0a7eaae244040777be8ba69860d5))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE2S_SOUNDROM18888("sxsnda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                  "sxsnda.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                  "sxsnda.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                  "sxsnda.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                  "sxsnda.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strxt_uk input_ports_se
#define init_strxt_uk init_strikext
CORE_CLONEDEFNV(strxt_uk,strikext,"Striker Xtreme (UK)",1999,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Striker Extreme (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_gr,"sxcpug.103", CRC(d73b9020) SHA1(491cd9db172db0a35870524ed4f1b15fb67f2e78))
DE_DMD32ROM8x(     "sxdispg.103",CRC(eb656489) SHA1(476315a5d22b6d8c63e9a592167a00f0c87e86c9))
DE2S_SOUNDROM18888("sxsndg.u7" ,CRC(b38ec07d) SHA1(239a3a51c049b007d4c16c3bd1e003a5dfd3cecc),
                  "sxsndg.u17",CRC(19ecf6ca) SHA1(f61f9e821bb0cf7978073a2d2cb939999265277b),
                  "sxsndg.u21",CRC(ee410b1e) SHA1(a0f7ff46536060be8f7c2c0e575e85814cd183e1),
                  "sxsndg.u36",CRC(f0e126c2) SHA1(a8c5eed27b33d20c2ff3bfd3d317c8b56bfa3625),
                  "sxsndg.u37",CRC(82260f4b) SHA1(6c2eba67762bcdd01e7b0c1b8b03b91b778444d4))
SE_ROMEND
#define input_ports_strxt_gr input_ports_se
#define init_strxt_gr init_strikext
CORE_CLONEDEFNV(strxt_gr,strikext,"Striker Xtreme (Germany)",1999,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Striker Extreme (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_fr,"sxcpuf.102", CRC(2804bc9f) SHA1(3c75d8cc4d2833baa163d99c709dcb9475ba0831))
DE_DMD32ROM8x(     "sxdispf.103",CRC(4b4b5c19) SHA1(d2612a2b8099b45cb67ac9b55c88b5b10519d49b))
DE2S_SOUNDROM18888("sxsndf.u7" ,CRC(e68b0607) SHA1(cd3a5ff51932914e977fe866f7ab569d0901967a),
                  "sxsndf.u17",CRC(443efde2) SHA1(823f395cc5b0c7f5665bd8c804707fb9bbad1066),
                  "sxsndf.u21",CRC(e8ba1618) SHA1(920cecbdcfc948670ddf11b572af0bb536a1153d),
                  "sxsndf.u36",CRC(89107426) SHA1(9e3c51f17aee0e803e54f9400c304b4da0b8cf7a),
                  "sxsndf.u37",CRC(67c0f1de) SHA1(46867403d4b13d18c4ebcc5b042faf3aca165ffb))
SE_ROMEND
#define input_ports_strxt_fr input_ports_se
#define init_strxt_fr init_strikext
CORE_CLONEDEFNV(strxt_fr,strikext,"Striker Xtreme (France)",1999,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Striker Extreme (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_it,"sxcpui.102", CRC(f955d0ef) SHA1(0f4ee87715bc085e2fb05e9ebdc89403f6bac444))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2) SHA1(a5e37ecf3b9772736ac88256c470f785dc113aa1))
DE2S_SOUNDROM18888("sxsndi.u7" ,CRC(81caf0a7) SHA1(5bb05c5bb49d12417b3ad49398623c3c222fd63b),
                  "sxsndi.u17",CRC(d0b21193) SHA1(2e5f92a67f0f18913e5d0af9936ab8694d095c66),
                  "sxsndi.u21",CRC(5ab3f8f4) SHA1(44982725eb31b0b144e3ad6549734b5fc46cd8c5),
                  "sxsndi.u36",CRC(4ee21ade) SHA1(1887f81b5f6753ce75ddcd0d7557c1644a925fcf),
                  "sxsndi.u37",CRC(4427e364) SHA1(7046b65086aafc4c14793d7036bc5130fe1e7dbc))
SE_ROMEND
#define input_ports_strxt_it input_ports_se
#define init_strxt_it init_strikext
CORE_CLONEDEFNV(strxt_it,strikext,"Striker Xtreme (Italy)",1999,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Striker Extreme (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(strxt_sp,"sxcpul.102", CRC(6b1e417f) SHA1(b87e8659bc4481384913a28c1cb2d7c96532b8e3))
DE_DMD32ROM8x(     "sxdispl.103",CRC(3efd4a18) SHA1(64f6998f82947a5bd053ad8dd56682adb239b676))
DE2S_SOUNDROM18888("sxsndl.u7" ,CRC(a03131cf) SHA1(e50f665eb15cef799fdc0d1d88bc7d5e23390225),
                  "sxsndl.u17",CRC(e7ee91cb) SHA1(1bc9992601bd7d194e2f33241179d682a62bff4b),
                  "sxsndl.u21",CRC(88cbf553) SHA1(d6afd262b47e31983c734c0054a7af2489da2f13),
                  "sxsndl.u36",CRC(35474ad5) SHA1(818a0f86fa4aa0b0457c16a20f8358655c42ea0a),
                  "sxsndl.u37",CRC(0e53f2a0) SHA1(7b89989ff87c25618d6f1c6479efd45b57f850fb))
SE_ROMEND
#define input_ports_strxt_sp input_ports_se
#define init_strxt_sp init_strikext
CORE_CLONEDEFNV(strxt_sp,strikext,"Striker Xtreme (Spain)",1999,"Stern",de_mSES2,0)

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
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout (2.11)",2000,"Stern",de_mSES1,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Sharkey's Shootout (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrknew,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispa.201",CRC(3360300b) SHA1(3169651a49bb7168fc04b2ae678b696ec6a21c85))
DE3S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrknew input_ports_se
#define init_shrknew init_shrkysht
CORE_CLONEDEFNV(shrknew,shrkysht,"Sharkey's Shootout (ARM7 Sound Board)",2001,"Stern",de_mSES3,0)
#endif

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_gr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispg.201",CRC(1ad88b75) SHA1(f82ae59b59e545e2023879aad7e99862d9d339c5))
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrky_gr input_ports_se
#define init_shrky_gr init_shrkysht
CORE_CLONEDEFNV(shrky_gr,shrkysht,"Sharkey's Shootout (Germany)",2001,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_fr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispf.201",CRC(89eaaebf) SHA1(0945a11a8f632cea3b9e9982cc4aed54f12ec55a))
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrky_fr input_ports_se
#define init_shrky_fr init_shrkysht
CORE_CLONEDEFNV(shrky_fr,shrkysht,"Sharkey's Shootout (France)",2001,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Sharkey's Shootout (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(shrky_it,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispi.201",CRC(fb70641d) SHA1(d152838eeacacc5dfe6fc3bc3f26b4d3e9e4c9cc))
DE2S_SOUNDROM1888(    "sssndu7.101",CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4),
                     "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3),
                     "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9),
                     "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))
SE_ROMEND
#define input_ports_shrky_it input_ports_se
#define init_shrky_it init_shrkysht
CORE_CLONEDEFNV(shrky_it,shrkysht,"Sharkey's Shootout (Italy)",2001,"Stern",de_mSES1,0)

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
CORE_GAMEDEFNV(hirolcas,"High Roller Casino (3.00)",2001,"Stern",de_mSES2,0)


#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ High Roller Casino (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hironew,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispa.300",CRC(099ccaf0) SHA1(2e0c2706881208f08e8a1d30915424c8f9b1cf67))
DE3S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hironew input_ports_se
#define init_hironew init_hirolcas
CORE_CLONEDEFNV(hironew,hirolcas,"High Roller Casino (ARM7 Sound Board)",2001,"Stern",de_mSES3,0)
#endif

/*-------------------------------------------------------------------
/ High Roller Casino (2.90) TEST BUILD 1820
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirolcat,"hrccput3.300",CRC(b70e04a0) SHA1(0c8d6c1e488471617ba9e24704d0d44826c1daf3))
DE_DMD32ROM8x(        "hrcdspt3.300",CRC(e262f36c) SHA1(116b2b96adce953e00d1e6d7f2b4ed4cdc4a3f61))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirolcat input_ports_se
#define init_hirolcat init_hirolcas
CORE_CLONEDEFNV(hirolcat,hirolcas,"High Roller Casino (3.00) TEST",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ High Roller Casino (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_fr,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispf.300",CRC(1fb5046b) SHA1(8b121a9c75a7d9a312b8c03615838b748d149819))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_fr input_ports_se
#define init_hirol_fr init_hirolcas
CORE_CLONEDEFNV(hirol_fr,hirolcas,"High Roller Casino (France)",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ High Roller Casino (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_g3,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispg.300",CRC(a880903a) SHA1(4049f50ceaeb6c9e869150ec3d903775cdd865ff))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_g3 input_ports_se
#define init_hirol_g3 init_hirolcas
CORE_CLONEDEFNV(hirol_g3,hirolcas,"High Roller Casino (Germany)",2001,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(hirol_gr,hirolcas,"High Roller Casino (2.10, Germany)",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ High Roller Casino (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(hirol_it,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispi.300",CRC(2734f746) SHA1(aa924d998b6c3fbd80e9325093c9b3267dfaadef))
DE2S_SOUNDROM18888(    "hrsndu7.100",CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d),
                      "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48),
                      "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf),
                      "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739),
                      "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))
SE_ROMEND
#define input_ports_hirol_it input_ports_se
#define init_hirol_it init_hirolcas
CORE_CLONEDEFNV(hirol_it,hirolcas,"High Roller Casino (Italy)",2001,"Stern",de_mSES2,0)

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
CORE_GAMEDEFNV(austin,"Austin Powers (3.02)",2001,"Stern",de_mSES2,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Austin Powers (3.02) - (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austnew,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb) SHA1(952a7873067b8c70043a38a39a8f65089103336b))
DE3S_SOUNDROM18888( "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                   "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                   "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                   "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                   "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austnew input_ports_se
#define init_austnew init_austin
CORE_CLONEDEFNV(austnew,austin,"Austin Powers (ARM7 Sound Board)",2001,"Stern",de_mSES3,0)
#endif

/*-------------------------------------------------------------------
/ Austin Powers (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austinf,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-f.300",CRC(1aeaa83e) SHA1(8a749c0fbf7b03441780c2158e63d4a87c8d0702))
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austinf input_ports_se
#define init_austinf init_austin
CORE_CLONEDEFNV(austinf,austin,"Austin Powers (France)",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Austin Powers (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austing,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-g.300",CRC(28b91cc4) SHA1(037628c78955495f10a60cfc329232289417562e))
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austing input_ports_se
#define init_austing init_austin
CORE_CLONEDEFNV(austing,austin,"Austin Powers (Germany)",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Austin Powers (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(austini,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-i.300",CRC(8b1dd747) SHA1(b29d39a2fb464bd11f4bc5daeb35360126ddf45b))
DE2S_SOUNDROM18888(  "apsndu7.100",CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f),
                    "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a),
                    "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74),
                    "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605),
                    "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))
SE_ROMEND
#define input_ports_austini input_ports_se
#define init_austini init_austin
CORE_CLONEDEFNV(austini,austin,"Austin Powers (Italy)",2001,"Stern",de_mSES2,0)

// Monopoly moved to its own sim file (gv)

/*-------------------------------------------------------------------
/ NFL
/-------------------------------------------------------------------*/
INITGAME(nfl,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(nfl,"nfl_v101.cpu", CRC(eeb81a51) SHA1(c87e5f50cc95b1d0206abc3c132f5f3314a9253c))
DE_DMD32ROM8x(     "nfl_v102.dmd",CRC(fd7bc50a) SHA1(5c92af91e7e12024026a06002e6c6bf68230fcc0))
DE2S_SOUNDROM18888("nfl_v100.u7" ,CRC(3fc766f8) SHA1(27341594e7d4a23146e6e6ec8ebdea125231cf91),
                  "nfl_v100.u17",CRC(f36b72ed) SHA1(f8fcbdb31295d363d1e7ad98dc318ab52bcfc52b),
                  "nfl_v100.u21",CRC(f5a6c053) SHA1(30a9cda6c9d9c43f0f6690138cf74c39c79ba43e),
                  "nfl_v100.u36",CRC(26dae8ac) SHA1(ec18f13578c5c291b777344b2830cde2ecf3581c),
                  "nfl_v100.u37",CRC(375d5a99) SHA1(4b49c58968da645bd0ad60ed16744974b863164e))
SE_ROMEND
#define input_ports_nfl input_ports_se
CORE_GAMEDEFNV(nfl,"NFL",2001,"Stern",de_mSES2,0)


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
CORE_GAMEDEFNV(playboys,"Playboy (5.00)",2002,"Stern",de_mSES2,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Playboy (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(playnew, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispa.500",  CRC(40450165) SHA1(80295466838cb08fe1499b19a98bf759fb7a306e))
DE3S_SOUNDROM18888(      "pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87),
                         "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6),
                         "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0),
                         "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f),
                         "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))
SE_ROMEND
#define input_ports_playnew input_ports_se
#define init_playnew init_playboys
CORE_CLONEDEFNV(playnew,playboys,"Playboy (ARM7 Sound Board)",2002,"Stern",de_mSES3,0)
#endif

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
CORE_CLONEDEFNV(playboyf,playboys,"Playboy (France)",2002,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(playboyg,playboys,"Playboy (Germany)",2002,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(playboyi,playboys,"Playboy (Italy)",2002,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(playboyl,playboys,"Playboy (Spain)",2002,"Stern",de_mSES2,0)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (7.02)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispRCT[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 5,21, CORE_DMD|CORE_DMDNOAA, (void *)seminidmd3_update}, {0}
};
INITGAME(rctycn, GEN_WS, dispRCT, SE_MINIDMD3)
SE128_ROMSTART(rctycn, "rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispa.701",CRC(0d527f13) SHA1(954116a79578b2a7679c401a2bb99b5bbfb603f2))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycn input_ports_se
CORE_GAMEDEFNV(rctycn,"Roller Coaster Tycoon (7.02)",2002,"Stern",de_mSES1,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (ARM7 Sound Board)
/-------------------------------------------------------------------*/

SE128_ROMSTART(rctnew, "rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispa.701",CRC(0d527f13) SHA1(954116a79578b2a7679c401a2bb99b5bbfb603f2))
DE3S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND

#define input_ports_rctnew input_ports_se
#define init_rctnew init_rctycn
CORE_CLONEDEFNV(rctnew,rctycn,"Roller Coaster Tycoon (ARM7 Sound Board)",2002,"Stern",de_mSES3,0)

#endif

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycng,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispg.701",CRC(6c70ab29) SHA1(fa3b713b79c0d724b918fa318795681308a4fce3))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycng input_ports_se
#define init_rctycng init_rctycn
CORE_CLONEDEFNV(rctycng,rctycn,"Roller Coaster Tycoon (Germany)",2002,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnf,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispf.701",CRC(4a3bb40c) SHA1(053f494765ac7708401a7816af7186e71083fe8d))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycnf input_ports_se
#define init_rctycnf init_rctycn
CORE_CLONEDEFNV(rctycnf,rctycn,"Roller Coaster Tycoon (France)",2002,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycni,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispi.701",CRC(6adc8236) SHA1(bb908c88e47987777c921f2948dfb802ee29c868))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycni input_ports_se
#define init_rctycni init_rctycn
CORE_CLONEDEFNV(rctycni,rctycn,"Roller Coaster Tycoon (Italy)",2002,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ Roller Coaster Tycoon (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(rctycnl,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdisps.701",CRC(0efa8208) SHA1(6706ea3e31b478970fc65a8cf9db749db9c0fa39))
DE2S_SOUNDROM1888(    "rcsndu7.100",CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778),
                     "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c),
                     "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb),
                     "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))
SE_ROMEND
#define input_ports_rctycnl input_ports_se
#define init_rctycnl init_rctycn
CORE_CLONEDEFNV(rctycnl,rctycn,"Roller Coaster Tycoon (Spain)",2002,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (5.00)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispSPP[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10,10,14, CORE_DMD|CORE_DMDNOAA, (void *)seminidmd4_update}, {0}
};
static core_tGameData simpprtyGameData = { \
  GEN_WS, dispSPP, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 4, 0, 0, SE_MINIDMD2}}; \
static void init_simpprty(void) { core_gameData = &simpprtyGameData; }
SE128_ROMSTART(simpprty, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispa.500",CRC(c6db83ec) SHA1(6079981e19b4651a074b0005eca85faf0eebcec0))
DE2S_SOUNDROM18888(      "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprty input_ports_se
CORE_GAMEDEFNV(simpprty,"Simpsons Pinball Party, The (5.00)",2003,"Stern",de_mSES2,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpnew, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(          "sppdispa.500",CRC(c6db83ec) SHA1(6079981e19b4651a074b0005eca85faf0eebcec0))
DE3S_SOUNDROM18888(     "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                        "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                        "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                        "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                        "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpnew input_ports_se
#define init_simpnew init_simpprty
CORE_CLONEDEFNV(simpnew,simpprty,"Simpsons Pinball Party, The (ARM7 Sound Board)",2003,"Stern",de_mSES3,0)
#endif

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtg, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispg.500",CRC(6503bffc) SHA1(717aa8b7a0329c886ddb4b167c022b3a2ee3ab2d))
DE2S_SOUNDROM18888(      "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtg input_ports_se
#define init_simpprtg init_simpprty
CORE_CLONEDEFNV(simpprtg,simpprty,"Simpsons Pinball Party, The (Germany)",2003,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtl, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispl.500",CRC(0821f182) SHA1(7998ab29dae59d077b1dedd28a30a3477251d107))
DE2S_SOUNDROM18888(      "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtl input_ports_se
#define init_simpprtl init_simpprty
CORE_CLONEDEFNV(simpprtl,simpprty,"Simpsons Pinball Party, The (Spain)",2003,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprtf, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispf.500",CRC(8d3383ed) SHA1(a56b1043fe1b0280d11386981fe9c181c9b6f1b7))
DE2S_SOUNDROM18888(      "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprtf input_ports_se
#define init_simpprtf init_simpprty
CORE_CLONEDEFNV(simpprtf,simpprty,"Simpsons Pinball Party, The (France)",2003,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(simpprti, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispi.500",CRC(eefe84db) SHA1(97c60f9182bdfe346ca4981b844a71f57414d470))
DE2S_SOUNDROM18888(      "spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f),
                         "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c),
                         "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977),
                         "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a),
                         "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))
SE_ROMEND
#define input_ports_simpprti input_ports_se
#define init_simpprti init_simpprty
CORE_CLONEDEFNV(simpprti,simpprty,"Simpsons Pinball Party, The (Italy)",2003,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (4.00)
/-------------------------------------------------------------------*/
INITGAME(term3,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(term3, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(      "t3dispa.400", CRC(6b7cc4f8) SHA1(214e9b3e45b778841fc166acf4ff5fd634ae2670))
DE2S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND
#define input_ports_term3 input_ports_se
CORE_GAMEDEFNV(term3,"Terminator 3: Rise of the Machines (4.00)",2003,"Stern",de_mSES2,0)

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines (4.00) (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(t3new, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(      "t3dispa.400", CRC(6b7cc4f8) SHA1(214e9b3e45b778841fc166acf4ff5fd634ae2670))
DE3S_SOUNDROM18888(      "t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8),
                         "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a),
                         "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807),
                         "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc),
                         "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))
SE_ROMEND

#define input_ports_t3new input_ports_se
#define init_t3new init_term3
CORE_CLONEDEFNV(t3new,term3,"Terminator 3: Rise of the Machines (ARM7 Sound Board)",2003,"Stern",de_mSES3,0)

#endif

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
CORE_CLONEDEFNV(term3g,term3,"Terminator 3: Rise of the Machines (Germany)",2003,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(term3l,term3,"Terminator 3: Rise of the Machines (Spain)",2003,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(term3f,term3,"Terminator 3: Rise of the Machines (France)",2003,"Stern",de_mSES2,0)

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
CORE_CLONEDEFNV(term3i,term3,"Terminator 3: Rise of the Machines (Italy)",2003,"Stern",de_mSES2,0)

// All games below now using a new CPU/Sound board (520-5300-00) with an Atmel AT91 (ARM7DMI Variant) CPU for sound

/*-------------------------------------------------------------------
/ The Lord Of The Rings (10.00)
/-------------------------------------------------------------------*/
static core_tGameData lotrGameData = { \
  GEN_WS, se_dmd128x32, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 5, 0, 0, SE_LED}}; \
static void init_lotr(void) { core_gameData = &lotrGameData; }
SE128_ROMSTART(lotr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(      "lotrdspa.a00", CRC(99634603) SHA1(c40d1480e5df10a491bcd471c6a3a118a9120bcb))
DE3S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr input_ports_se
CORE_GAMEDEFNV(lotr,"Lord Of The Rings, The (10.00)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_sp, "lotrcpul.a00", CRC(c62aba47) SHA1(2fef599313e5cd9bded3ab00b933631586e2a1e7))
DE_DMD32ROM8x(          "lotrdspl.a00", CRC(2494a5ee) SHA1(5b95711858d88eeb445503cac8b9b754cf8e9960))
DE3S_SOUNDROM18888(      "lotrlu7.100", CRC(980d970a) SHA1(cf70deddcc146ef9eaa64baec74ae800bebf8715),
                         "lotrlu17.100",CRC(c16d3e20) SHA1(43d9f186db361abb3aa119a7252f1bb13bbbbe39),
                         "lotrlu21.100",CRC(f956a1be) SHA1(2980e85463704a154ed81d3241f866442d1ea2e6),
                         "lotrlu36.100",CRC(502c3d93) SHA1(4ee42d70bccb9253fe1f8f441de6b5018257f107),
                         "lotrlu37.100",CRC(61f21c6d) SHA1(3e9781b4981bd18cdb8c59c55b9942de6ae286db))
SE_ROMEND
#define input_ports_lotr_sp input_ports_lotr
#define init_lotr_sp init_lotr
CORE_CLONEDEFNV(lotr_sp,lotr,"Lord Of The Rings, The (Spain)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_gr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspg.a00", CRC(6743a910) SHA1(977773515f00af3937aa59426917e8111ec855ab))
DE3S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_gr input_ports_lotr
#define init_lotr_gr init_lotr
CORE_CLONEDEFNV(lotr_gr,lotr,"Lord Of The Rings, The (Germany)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_fr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspf.a00", CRC(15c26c2d) SHA1(c8e4b442d717aa5881f3d92f044c44d29a14126c))
DE3S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_fr input_ports_lotr
#define init_lotr_fr init_lotr
CORE_CLONEDEFNV(lotr_fr,lotr,"Lord Of The Rings, The (France)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ The Lord Of The Rings (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(lotr_it, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspi.a00", CRC(6c88f395) SHA1(365d5c6908f5861816b73f287194c85d2300635d))
DE3S_SOUNDROM18888(      "lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470),
                         "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab),
                         "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0),
                         "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da),
                         "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
SE_ROMEND
#define input_ports_lotr_it input_ports_lotr
#define init_lotr_it init_lotr
CORE_CLONEDEFNV(lotr_it,lotr,"Lord Of The Rings, The (Italy)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (3.20)
/-------------------------------------------------------------------*/
static struct core_dispLayout dispBION[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 5, CORE_DMD, (void *)seminidmd1a_update},
  {34,18, 7, 5, CORE_DMD, (void *)seminidmd1b_update},
  {34,26, 7, 5, CORE_DMD, (void *)seminidmd1c_update}, {0}
};
INITGAME(ripleys,GEN_WS,dispBION,SE_MINIDMD3)
SE128_ROMSTART(ripleys, "ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
DE3S_SOUNDROM18888(      "ripsnd.u7",CRC(4573a759) SHA1(189c1a2eaf9d92c40a1bc145f59ac428c74a7318),
                        "ripsnd.u17",CRC(d518f2da) SHA1(e7d75c6b7b45571ae6d39ed7405b1457e475b52a),
                        "ripsnd.u21",CRC(3d8680d7) SHA1(1368965106094d78be6540eb87a478f853ba774f),
                        "ripsnd.u36",CRC(b697b5cb) SHA1(b5cb426201287a6d1c40db8c81a58e2c656d1d81),
                        "ripsnd.u37",CRC(01b9f20e) SHA1(cffb6a0136d7d17ab4450b3bfd97632d8b669d39))
SE_ROMEND
#define input_ports_ripleys input_ports_se
CORE_GAMEDEFNV(ripleys,"Ripley's Believe It or Not! (3.20)",2004,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysf,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispf.301",CRC(e5ae9d99) SHA1(74929b324b457d08a925c641430e6a7036c7039d))
DE3S_SOUNDROM18888(     "ripsndf.u7",CRC(5808e3fc) SHA1(0c83399e8dc846607c469b7dd95878f3c2b9cb82),
                       "ripsndf.u17",CRC(a6793b85) SHA1(96058777346be6e9ea7b1340d9aaf945ac3c853a),
                       "ripsndf.u21",CRC(60c02170) SHA1(900d9de3ccb541019e5f1528e01c57ad96dac262),
                       "ripsndf.u36",CRC(0a57f2fd) SHA1(9dd057888294ee8abeb582e8f6650fd6e32cc9ff),
                       "ripsndf.u37",CRC(5c858958) SHA1(f4a9833b8aee033ed381e3bdf9f801b935d6186a))
SE_ROMEND
#define input_ports_ripleysf input_ports_ripleys
#define init_ripleysf init_ripleys
CORE_CLONEDEFNV(ripleysf,ripleys,"Ripley's Believe It or Not! (France)",2004,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysg,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
DE3S_SOUNDROM18888(     "ripsndg.u7",CRC(400b8a45) SHA1(62101995e632264df3c014b746cc4b2ae72676d4),
                       "ripsndg.u17",CRC(c387dcf0) SHA1(d4ef65d3f33ab82b63bf2782f335858ab4ad210a),
                       "ripsndg.u21",CRC(6388ae8d) SHA1(a39c7977194daabf3f5b10d0269dcd4118a939bc),
                       "ripsndg.u36",CRC(3143f9d3) SHA1(bd4ce64b245b5fcb9b9694bd8f71a9cd98303cae),
                       "ripsndg.u37",CRC(2167617b) SHA1(62b55a39e2677eec9d56b10e8cc3e5d7c0d3bea5))
SE_ROMEND
#define input_ports_ripleysg input_ports_ripleys
#define init_ripleysg init_ripleys
CORE_CLONEDEFNV(ripleysg,ripleys,"Ripley's Believe It or Not! (Germany)",2004,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysi,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
DE3S_SOUNDROM18888(     "ripsndi.u7",CRC(86b1b2b2) SHA1(9e2cf7368b31531998d546a1be2af274a9cbbd2f),
                       "ripsndi.u17",CRC(a2911df4) SHA1(acb7956a6a30142c8da905b04778a074cb335807),
                       "ripsndi.u21",CRC(1467eaff) SHA1(c6c4ea2abdad4334efbe3a084693e9e4d0dd0fd2),
                       "ripsndi.u36",CRC(6a124fa6) SHA1(752c3d227b9a98dd859e4778ddd527edaa3cf512),
                       "ripsndi.u37",CRC(7933c102) SHA1(f736ee86d7c67dab82c634d125d73a2453249706))
SE_ROMEND
#define input_ports_ripleysi input_ports_ripleys
#define init_ripleysi init_ripleys
CORE_CLONEDEFNV(ripleysi,ripleys,"Ripley's Believe It or Not! (Italy)",2004,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not! (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(ripleysl,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispl.301",CRC(47c87ad4) SHA1(eb372b9f17b28d0781c49a28cb850916ccec323d))
DE3S_SOUNDROM18888(     "ripsndl.u7",CRC(25fb729a) SHA1(46b9ca8fd5fb5a692adbdb7495af34a1db89dc37),
                       "ripsndl.u17",CRC(a98f4514) SHA1(e87ee8f5a87a8ae9ec996473bf9bc745105ea334),
                       "ripsndl.u21",CRC(141f2b77) SHA1(15bab623beda8ae7ed9908f492ff2baab0a7954e),
                       "ripsndl.u36",CRC(c5461b63) SHA1(fc574d44ad88ce1db590ea371225092c03fc6f80),
                       "ripsndl.u37",CRC(2a58f491) SHA1(1c33f419420b3165ef18598560007ef612b24814))
SE_ROMEND
#define input_ports_ripleysl input_ports_ripleys
#define init_ripleysl init_ripleys
CORE_CLONEDEFNV(ripleysl,ripleys,"Ripley's Believe It or Not! (Spain)",2004,"Stern",de_mSES3,0)

// Elvis moved to its own sim file (gv)

/*-------------------------------------------------------------------
/ The Sopranos (5.00)
/-------------------------------------------------------------------*/
INITGAME(sopranos,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(sopranos, "sopcpua.500", CRC(e3430f28) SHA1(3b942922087cca41701ef70fbc84baaffe8da90d))
DE_DMD32ROM8x(      "sopdspa.500", CRC(170bd8d1) SHA1(df8d240425ac2c1aa4bea560ecdd3d64120faeb7))
DE3SC_SOUNDROM18888("sopsnda.u7",  CRC(47817240) SHA1(da2ff205fb5fe732514e7aa312ff552ecd4b31b7),
                    "sopsnda.u17", CRC(21e0cfd2) SHA1(d2ff1242f1f4a206e0b2884c079ef2be5df143ac),
                    "sopsnda.u21", CRC(10669399) SHA1(6ad31c91ba3c772f7a79e4408a4422332243c7d1),
                    "sopsnda.u36", CRC(f3535025) SHA1(3fc22af5db0a8b744c0513e24a6331c9cf82e3ed),
                    "sopsnda.u37", CRC(4b67fe8a) SHA1(b980b9705b4a41a0524b3b0095d6398bdbed609f))
SE_ROMEND
#define input_ports_sopranos input_ports_se
CORE_GAMEDEFNV(sopranos,"Sopranos, The (5.00)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Sopranos (3.00)
/-------------------------------------------------------------------*/
SE128_ROMSTART(soprano3, "sopcpua.300", CRC(cca0e551) SHA1(ef109b871605879b55abb966d6736d7deeca404f))
DE_DMD32ROM8x(      "sopdspa.300", CRC(aa6306ac) SHA1(80737bd2b93bfc64490d07d2cd482350ed3303b3))
DE3SC_SOUNDROM18888("sopsnd3.u7",  CRC(b22ba5aa) SHA1(8f69932e3b115ae7a6bcb9a15a8b5bf6579e94e0),
                    "sopsnda.u17", CRC(21e0cfd2) SHA1(d2ff1242f1f4a206e0b2884c079ef2be5df143ac),
                    "sopsnd3.u21", CRC(257ab09d) SHA1(1d18e279139b1658ce02160d9a37b4bf043393f0),
                    "sopsnd3.u36", CRC(db33b45c) SHA1(d3285008a3c770371389be470c1ec5ca49c1e568),
                    "sopsnd3.u37", CRC(06a2a6e1) SHA1(fdbe622223724ac2b4c5183c43d3e635654864bf))
SE_ROMEND
#define input_ports_soprano3 input_ports_sopranos
#define init_soprano3 init_sopranos
CORE_CLONEDEFNV(soprano3,sopranos, "Sopranos, The (3.00)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Sopranos (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(sopranog, "sopcpug.500", CRC(e6317603) SHA1(2e7f49cd77f65d0af08ab503fc82ec56c8890926))
DE_DMD32ROM8x(      "sopdspg.500", CRC(d8f365e9) SHA1(395209169e023913bf0bf3c3837e9a1b6b636e75))
DE3SC_SOUNDROM18888("sopsndg.u7",  CRC(bb615e03) SHA1(ce5ef766376c060fc071d54aa878d69b3aa30294),
                    "sopsndg.u17", CRC(cfa7fca1) SHA1(2efbc8c3e8ad6dc39973908e37ecdc7b02be720a),
                    "sopsndg.u21", CRC(7669769b) SHA1(adf0c0dbfbaa981fa90db4e54702a4dbaf474e82),
                    "sopsndg.u36", CRC(08d715b5) SHA1(ddccd311ba2b608ab0845afb3ef63b8d3425d530),
                    "sopsndg.u37", CRC(2405df73) SHA1(b8074610d9d87d3f1c0244ef0f450c766aac8a20))
SE_ROMEND
#define input_ports_sopranog input_ports_sopranos
#define init_sopranog init_sopranos
CORE_CLONEDEFNV(sopranog,sopranos,"Sopranos, The (Germany)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Sopranos (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(sopranof, "sopcpuf.500", CRC(d047d4bd) SHA1(588ba8f6c7af32f6aa94f29e59ad2bcd2bc64968))
DE_DMD32ROM8x(      "sopdspf.500", CRC(e4252fb5) SHA1(be54181af8f3650023f20cf1bf3b8b0310adb1bb))
DE3SC_SOUNDROM18888("sopsndf.u7",  CRC(57426738) SHA1(393e1d654ef09172580ad9a2722f696b6e44ec0f),
                    "sopsndf.u17", CRC(0e34f843) SHA1(2a026bda4c44803239a93e6603a4dfb25e3103b5),
                    "sopsndf.u21", CRC(28726d20) SHA1(63c6bea953cc34b6a3c2c9688ab86641f94cd227),
                    "sopsndf.u36", CRC(99549d4a) SHA1(15e3d35b9cefbc8825a7dee5309adc2526de3dec),
                    "sopsndf.u37", CRC(2b4a9130) SHA1(eed9c84c932bb86954226b0d51461c5094ebe02e))
SE_ROMEND
#define input_ports_sopranof input_ports_sopranos
#define init_sopranof init_sopranos
CORE_CLONEDEFNV(sopranof,sopranos,"Sopranos, The (France)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Sopranos (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(sopranol, "sopcpul.500", CRC(ba978d00) SHA1(afe561cc439d9e51dee0697f423fce103448504c))
DE_DMD32ROM8x(      "sopdspl.500", CRC(a4100c9e) SHA1(08ea2424ff315f6174d56301c7a8164a32629367))
DE3SC_SOUNDROM18888("sopsndl.u7",  CRC(137110f2) SHA1(9bd911fc74b91e811ada4c66bec214d22506a646),
                    "sopsndl.u17", CRC(3d5189e6) SHA1(7d846d0b18678ff7aa44029643571e237bc48d58),
                    "sopsndl.u21", CRC(0ac4f407) SHA1(9473ab42c0f758901644256d7cd1cb47c8396433),
                    "sopsndl.u36", CRC(147c4216) SHA1(ded2917188bea51cb03db72fe53fcd76a3e66ab9),
                    "sopsndl.u37", CRC(cfe814fb) SHA1(51b6b10dda4640f8569e610b41c77e3657eabff2))
SE_ROMEND
#define input_ports_sopranol input_ports_sopranos
#define init_sopranol init_sopranos
CORE_CLONEDEFNV(sopranol,sopranos,"Sopranos, The (Spain)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Sopranos (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(sopranoi, "sopcpui.500", CRC(c6b29efb) SHA1(1d7045c06bd8c6fc2304fba150c201e920ae92b4))
DE_DMD32ROM8x(      "sopdspi.500", CRC(5a3f479b) SHA1(43f36e27549259df172ed4340ae891eca634a594))
DE3SC_SOUNDROM18888("sopsndi.u7",  CRC(afb9c474) SHA1(fd184e8cd6afff61fd2874b08f0e841934916ccb),
                    "sopsndi.u17", CRC(ec8e4e36) SHA1(312f1d86bf6703b8ff6b807a3a2abea9fe0c20b8),
                    "sopsndi.u21", CRC(37727b76) SHA1(8801091870a30222d5a99535bbe15ac97334e368),
                    "sopsndi.u36", CRC(71568348) SHA1(516d5ea35f8323e247c25000cb223f3539796ea1),
                    "sopsndi.u37", CRC(b34c0a5f) SHA1(b84979d6eef7d23e6dd5410993d83fba2121bc6a))
SE_ROMEND
#define input_ports_sopranoi input_ports_sopranos
#define init_sopranoi init_sopranos
CORE_CLONEDEFNV(sopranoi,sopranos,"Sopranos, The (Italy)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Nascar (4.50)
/-------------------------------------------------------------------*/
INITGAME(nascar,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(nascar, "nascpua.450", CRC(da902e01) SHA1(afc6ace2b31c8682fb4d05e1b472c2ec30e7559b))
DE_DMD32ROM8x(      "nasdspa.400", CRC(364878bf) SHA1(a1fb477a37459a3583d3767386f87aa620e31e34))
DE3SC_SOUNDROM18888("nassnd.u7",   CRC(3a3c8203) SHA1(c64c424c01ec91e2578fd6ddc5d3596b8a485c22),
                    "nassnd.u17", CRC(4dcf65fa) SHA1(bc745e16f1f4b92b97fd0536bea789909b9c0c67),
                    "nassnd.u21", CRC(82ac1e4f) SHA1(8a6518885d89651df31afc8119d87a46fd802e16),
                    "nassnd.u36", CRC(2385ada2) SHA1(d3b59beffe6817cc3ea1140698095886ec2f2324),
                    "nassnd.u37", CRC(458ba148) SHA1(594fd9b48aa48ab7b3df921e689b1acba2b09d79))
SE_ROMEND
#define input_ports_nascar input_ports_se
CORE_GAMEDEFNV(nascar,"Nascar (4.50)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Nascar (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(nascarl, "nascpul.450", CRC(3eebae3f) SHA1(654f0e44ce009450e66250423fcf0ff4727e5ee1))
DE_DMD32ROM8x(      "nasdspl.400", CRC(a4de490f) SHA1(bc1aa9fc0182045f5d10044b3e4fa083572be4ac))
DE3SC_SOUNDROM18888("nascsp.u7",   CRC(03a34394) SHA1(d1e3a1a8e14525c40e9f8a5441a106df662608f1),
                    "nassndl.u17", CRC(058c67ad) SHA1(70e22d8a1842108309f6c03dcc6ac23a822da3c3),
                    "nassndl.u21", CRC(e34d3b6f) SHA1(63ef27ed5965d719215d0a469886d3852b6bffb6),
                    "nassndl.u36", CRC(9e2658b1) SHA1(0d93a381a65f11022a1a6da5e5b0e4a0e779f336),
                    "nassndl.u37", CRC(63f084ab) SHA1(519807bf6e868df6f756ad30af2f6636804f167c))
SE_ROMEND
#define input_ports_nascarl input_ports_se
#define init_nascarl init_nascar
CORE_CLONEDEFNV(nascarl,nascar,"Nascar (Spain)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)


/*-------------------------------------------------------------------
/ Grand Prix (4.50)
/-------------------------------------------------------------------*/
INITGAME(gprix,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(gprix, "gpcpua.450", CRC(3e5ae527) SHA1(27d50a0460b1733c6c857968b85da492fa2ad544))
DE_DMD32ROM8x(      "gpdspa.400", CRC(ce431306) SHA1(2573049b52b928052f196371dbc3a5236ce8cfc3))
DE3SC_SOUNDROM18888("gpsnda.u7",  CRC(f784634f) SHA1(40847986003b01c9de5d9af4c66a0f1f9fb0cac8),
                    "gpsnda.u17", CRC(43dca7e2) SHA1(30726897950b168ffa5e0e8a4ff12856fd50f132),
                    "gpsnda.u21", CRC(77717142) SHA1(055f975c3d1cf6560908f5d8553f7e64580a2bba),
                    "gpsnda.u36", CRC(6e414e19) SHA1(5b7c9da9c340ec3b55163f5674d72ab30ffbb866),
                    "gpsnda.u37", CRC(caf4c3f3) SHA1(ebdbaccf951ef6525f0fafa7e23d8140ef6b84e5))
SE_ROMEND
#define input_ports_gprix input_ports_se
CORE_GAMEDEFNV(gprix,"Grand Prix (4.50)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Grand Prix (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(gprixg, "gpcpug.450", CRC(d803128b) SHA1(f914f75f4ec38dcbd2e40818fe8cb0ad446c59bf))
DE_DMD32ROM8x(      "gpdspg.400", CRC(b3f64332) SHA1(84e1b094c74b2dfae8e3cd3ce3f1cd20dc400fd7))
DE3SC_SOUNDROM18888("gpsndg.u7",  CRC(95129e03) SHA1(5fddd9d8213f9f1f68fe9e96c9e78dc6771fab21),
                    "gpsndg.u17", CRC(27ef97ea) SHA1(8ba941d5d4f929b8ec3222f1c91452395e2f690f),
                    "gpsndg.u21", CRC(71391d71) SHA1(690b280710c79d94fc271541066ae90e462bbce2),
                    "gpsndg.u36", CRC(415baa1b) SHA1(cca21e0e5ef0cbe34c9514d72a06fc129990787a),
                    "gpsndg.u37", CRC(e4a6ae7f) SHA1(4a4cd973f90c13ced07459c8f457314c8280dd6a))
SE_ROMEND
#define input_ports_gprixg input_ports_se
#define init_gprixg init_gprix
CORE_CLONEDEFNV(gprixg,gprix,"Grand Prix (Germany)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Grand Prix (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(gprixl, "gpcpul.450", CRC(816bf4a4) SHA1(d5cca282e58d493be36400a7cd7dc4321d98f2f8))
DE_DMD32ROM8x(      "gpdspl.400", CRC(74d9aa40) SHA1(802c6fbe4248a516f18e4b69997254b3dcf27706))
DE3SC_SOUNDROM18888("gpsndl.u7",  CRC(0640fe8f) SHA1(aa45bf89c4cae5b4c2143656cfe19fe8f1ec30a3),
                    "gpsndl.u17", CRC(2581ef04) SHA1(2d85040e355ed410c7d8348ef64fc2c8e76ec0f0),
                    "gpsndl.u21", CRC(f4c97c9e) SHA1(ae04f416a7582efee20469ec686d02727558d850),
                    "gpsndl.u36", CRC(863de01a) SHA1(3f1fd157c2abacdab072146499b64b9e0853fb3e),
                    "gpsndl.u37", CRC(db16b68a) SHA1(815fdcd4ae01c6264133389ce3194da572e1c232))
SE_ROMEND
#define input_ports_gprixl input_ports_se
#define init_gprixl init_gprix
CORE_CLONEDEFNV(gprixl,gprix,"Grand Prix (Spain)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Grand Prix (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(gprixf, "gpcpuf.450", CRC(b14f7d20) SHA1(b91097490ee568e00be58f5dac184c8d47196adc))
DE_DMD32ROM8x(      "gpdspf.400", CRC(f9b1ef9a) SHA1(a7e3c0fc1526cf3632e6b1f22caf7f73749e77a6))
DE3SC_SOUNDROM18888("gpsndf.u7",  CRC(9b34e55a) SHA1(670fe4e4b62c46266667f37c0341bb4266e55067),
                    "gpsndf.u17", CRC(18beb699) SHA1(a3bbd7c9fc1165da5e502e09f68321bd56992e76),
                    "gpsndf.u21", CRC(b64702dd) SHA1(8762fb00d5549649444f7f85c3f6d72f27c6ba41),
                    "gpsndf.u36", CRC(4e41f0bb) SHA1(4a25b472a9435c77712559d7ded1649dffbc885c),
                    "gpsndf.u37", CRC(e6d96767) SHA1(a471d51796edad71eb21aadc4a26bb1529a0b9cc))
SE_ROMEND
#define input_ports_gprixf input_ports_se
#define init_gprixf init_gprix
CORE_CLONEDEFNV(gprixf,gprix,"Grand Prix (France)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Grand Prix (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(gprixi, "gpcpui.450", CRC(f18d8375) SHA1(b7e4e311623babc7b3c5d744122c88d45d77a33b))
DE_DMD32ROM8x(      "gpdspi.400", CRC(88675cdf) SHA1(b305a683350d38b43f2e3c9277af14d5503b3219))
DE3SC_SOUNDROM18888("gpsndi.u7",  CRC(37d66e66) SHA1(219fd734d3a19407d9d47de198429c770d7d8856),
                    "gpsndi.u17", CRC(868b225d) SHA1(bc169cf5882002a1b58973a22a78d8dd4467bc51),
                    "gpsndi.u21", CRC(b6692c39) SHA1(ac36ffb37ad945a857d5098547479c8cd62b6356),
                    "gpsndi.u36", CRC(f8558b24) SHA1(ceb3880b026fb7fcc69eb8d94e33e30c56c24de8),
                    "gpsndi.u37", CRC(a76c6682) SHA1(6d319a8f07c10fe392fc0b8e177cc6abbce0b536))
SE_ROMEND
#define input_ports_gprixi input_ports_se
#define init_gprixi init_gprix
CORE_CLONEDEFNV(gprixi,gprix,"Grand Prix (Italy)",2005,"Stern",de_mSES3,GAME_IMPERFECT_SOUND)
