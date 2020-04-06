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
  {0,0, 32,128, CORE_DMD, (genf *)dedmd32_update, NULL}, {0}
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
  {0,0, 32,128, CORE_DMD, (genf *)dedmd32_update, NULL},
  {7,0,  0,  1, CORE_SEG7 | CORE_NODISP, (genf *)led_update, NULL},
  {0}
};
INITGAME(apollo13,GEN_WS,se_apollo,SE_DIGIT)
SE128_ROMSTART(apollo13,"apolcpu.501",CRC(5afb8801) SHA1(65608148817f487c384dd36c221138962f1d9824))
DE_DMD32ROM8x(    "a13dspa.500", CRC(bf8e3249) SHA1(5e04681901ca794feb970f5388cb355427cf9a9a))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo13 input_ports_se
CORE_GAMEDEFNV(apollo13,"Apollo 13 (5.01, Display 5.00)",1995,"Sega",de_mSES1,0)

INITGAME(apollo1,GEN_WS,se_apollo,SE_DIGIT)
SE128_ROMSTART(apollo1,"a13cpu.100",CRC(5971e956) SHA1(89853912fc569480e66bec4cef369d8320c3a07d))
DE_DMD32ROM8x(    "a13dps.100",  CRC(224f6149) SHA1(b2a1786adc358834615989fce8835e0f039abb24))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo1 input_ports_apollo13
CORE_CLONEDEFNV(apollo1,apollo13,"Apollo 13 (1.00)",1995,"Sega",de_mSES1,0)

INITGAME(apollo2,GEN_WS,se_apollo,SE_DIGIT)
SE128_ROMSTART(apollo2,"a13cpu.203",CRC(4af048fc) SHA1(c82459247707a6cf07a10cc884f1391d0ca536a3))
DE_DMD32ROM8x(    "a13dps.201",  CRC(ab97a71c) SHA1(1e01d3c2ac1b9153fb4f3f888fe01fcebbf853d7))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo2 input_ports_apollo13
CORE_CLONEDEFNV(apollo2,apollo13,"Apollo 13 (2.03)",1995,"Sega",de_mSES1,0)

INITGAME(apollo14,GEN_WS,se_apollo,SE_DIGIT)
SE128_ROMSTART(apollo14,"apolcpu.501",CRC(5afb8801) SHA1(65608148817f487c384dd36c221138962f1d9824))
DE_DMD32ROM8x(    "a13dspa.401", CRC(6516ee16) SHA1(17011df142707917af2e0ec77c0e5ae78df91c0d))
DE2S_SOUNDROM1444("apollo13.u7" ,CRC(e58a36b8) SHA1(ae60470a7b6c41cd40dbb7c0bea6f2f148f7b088),
                  "apollo13.u17",CRC(4e863aca) SHA1(264f9176a1abf758b7a894d83883330ef91b7388),
                  "apollo13.u21",CRC(28169e37) SHA1(df5209d24187b546a4296fc4629c58bf729349d2),
                  "apollo13.u36",CRC(cede5e0f) SHA1(fa3b5820ed58e57b3c6185d91e9aea28aebc28d7))
SE_ROMEND
#define input_ports_apollo14 input_ports_apollo13
CORE_CLONEDEFNV(apollo14,apollo13,"Apollo 13 (5.01, Display 4.01)",1995,"Sega",de_mSES1,0)

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
#define TWST_SND \
DE2S_SOUNDROM144("twstsnd.u7" ,CRC(5ccf0798) SHA1(ac591c508de8e9687c20b01c298084c99a251016), \
                 "twstsnd.u17",CRC(0e35d640) SHA1(ce38a03fcc321cd9af07d24bf7aa35f254b629fc), \
                 "twstsnd.u21",CRC(c3eae590) SHA1(bda3e0a725339069c49c4282676a07b4e0e8d2eb))

INITGAME(twst,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(twst_405, "twstcpu.405",CRC(8c3ea1a8) SHA1(d495b7dc79186d442a89b6382a6dc1c83e64ef95))
DE_DMD32ROM8x(   "twstdspa.400",CRC(a6a3d41d) SHA1(ad42b3390ceeeea43c1cd47f300bcd4b4a4d2558))
TWST_SND
SE_ROMEND
#define input_ports_twst input_ports_se
CORE_GAMEDEF(twst,405,"Twister (4.05)",1996,"Sega",de_mSES1,0)

SE128_ROMSTART(twst_404, "twstcpu.404",CRC(43ac7f8b) SHA1(fc087b741c00baa9093dfec009440a7d64f4ca65))
DE_DMD32ROM8x(   "twstdspa.400",CRC(a6a3d41d) SHA1(ad42b3390ceeeea43c1cd47f300bcd4b4a4d2558))
TWST_SND
SE_ROMEND
CORE_CLONEDEF(twst,404,405,"Twister (4.04)",1996,"Sega",de_mSES1,0)

SE128_ROMSTART(twst_300, "twstcpu.300",CRC(5cc057d4) SHA1(9ff4b6951a974e3013edc30ba2310c3bffb224d2))
DE_DMD32ROM8x(   "twstdspa.301",CRC(78bc45cb) SHA1(d1915fab46f178c9842e44701c91a0db2495e4fd))
TWST_SND
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
CORE_GAMEDEFNV(id4,"ID4: Independence Day (2.02)",1996,"Sega",de_mSES1,0)

INITGAME(id4f,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4f, "id4cpu.202",CRC(108d88fd) SHA1(8317944201acfb97dadfdd364696c9e81a21d2c5))
DE_DMD32ROM8x(    "id4dspf.200",CRC(4b52676b) SHA1(a881efb28d8bab424d8c12be2c16b8afc7472208))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37) SHA1(06d79967a25af0b90a5f1d6360a5b5fdbb972d5a),
                  "id4sdu17.400",CRC(89ffeca3) SHA1(b94c60e3a433f797d6c5ea793c3ecff0a3b6ba60),
                  "id4sdu21.400",CRC(f384a9ab) SHA1(06bd607e7efd761017a7b605e0294a34e4c6255c))
SE_ROMEND
#define input_ports_id4f input_ports_se
CORE_CLONEDEFNV(id4f,id4,"ID4: Independence Day (2.02 French)",1996,"Sega",de_mSES1,0)

INITGAME(id4_201,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4_201, "id4cpu.201",CRC(c0cd47a1) SHA1(63bb6da28b4f6fcc8525a8f1a6d262e35931efc9))
DE_DMD32ROM8x(    "id4dspa.200",CRC(2d3fbcc4) SHA1(0bd69ebb68ae880ac9aae40916f13e1ff84ecfaa))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37) SHA1(06d79967a25af0b90a5f1d6360a5b5fdbb972d5a),
                  "id4sdu17.400",CRC(89ffeca3) SHA1(b94c60e3a433f797d6c5ea793c3ecff0a3b6ba60),
                  "id4sdu21.400",CRC(f384a9ab) SHA1(06bd607e7efd761017a7b605e0294a34e4c6255c))
SE_ROMEND
#define input_ports_id4_201 input_ports_se
CORE_CLONEDEFNV(id4_201,id4,"ID4: Independence Day (2.01)",1996,"Sega",de_mSES1,0)

INITGAME(id4_201f,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(id4_201f, "id4cpu.201",CRC(c0cd47a1) SHA1(63bb6da28b4f6fcc8525a8f1a6d262e35931efc9))
DE_DMD32ROM8x(    "id4dspf.200",CRC(4b52676b) SHA1(a881efb28d8bab424d8c12be2c16b8afc7472208))
DE2S_SOUNDROM144 ("id4sndu7.512",CRC(deeaed37) SHA1(06d79967a25af0b90a5f1d6360a5b5fdbb972d5a),
                  "id4sdu17.400",CRC(89ffeca3) SHA1(b94c60e3a433f797d6c5ea793c3ecff0a3b6ba60),
                  "id4sdu21.400",CRC(f384a9ab) SHA1(06bd607e7efd761017a7b605e0294a34e4c6255c))
SE_ROMEND
#define input_ports_id4_201f input_ports_se
CORE_CLONEDEFNV(id4_201f,id4,"ID4: Independence Day (2.01 French)",1996,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Space Jam
/-------------------------------------------------------------------*/
#define JAM_SND \
DE2S_SOUNDROM1444("spcjam.u7" ,CRC(c693d853) SHA1(3e81e60967dff496c681962f3ff8c7c1fbb7746a), \
                  "spcjam.u17",CRC(ccefe457) SHA1(4186dee689fbfc08e5070ccfe8d4be95220cd87b), \
                  "spcjam.u21",CRC(14cb71cb) SHA1(46752c1792c26345abb4d5219917a1cda50c600b), \
                  "spcjam.u36",CRC(7f61143c) SHA1(40695d1d14695d3e4991ed39f4a354c16227975e))

INITGAME(spacejam,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(spacejam,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspa.300",CRC(198e5e34) SHA1(e2ba9ff1cea84c5d41f32afc50229cb5a18c7666))
JAM_SND
SE_ROMEND
#define input_ports_spacejam input_ports_se
CORE_GAMEDEFNV(spacejam,"Space Jam (3.00)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(spacejm2,"jamcpu.200",CRC(d80c069b) SHA1(bf6e96100b158f058b5f07f537ad0fa0a0fbe31d))
DE_DMD32ROM8x(  "jamdspa.200",CRC(4a05ec31) SHA1(eb962f5f2160508e0f81b252e8644d8aa833d7fd))
JAM_SND
SE_ROMEND
#define input_ports_spacejm2 input_ports_spacejam
#define init_spacejm2 init_spacejam
CORE_CLONEDEFNV(spacejm2,spacejam,"Space Jam (2.00)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(spacejmg,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspg.300",CRC(41f6e188) SHA1(da2247022aadb0ead5a3b1d7b829c13ff1153ec8))
JAM_SND
SE_ROMEND
#define input_ports_spacejmg input_ports_spacejam
#define init_spacejmg init_spacejam
CORE_CLONEDEFNV(spacejmg,spacejam,"Space Jam (3.00 German)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(spacejmf,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspf.300",CRC(1683909f) SHA1(e14810a5d8704ea052fddcb3b54043bf9d57b296))
JAM_SND
SE_ROMEND
#define input_ports_spacejmf input_ports_spacejam
#define init_spacejmf init_spacejam
CORE_CLONEDEFNV(spacejmf,spacejam,"Space Jam (3.00 French)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(spacejmi,"jamcpu.300",CRC(9dc8df2e) SHA1(b3b111afb5b1f1236be73e899b34a5d5a73813e9))
DE_DMD32ROM8x(  "jamdspi.300",CRC(eb9b5971) SHA1(0bfac0511d0cd9d56eee59782c199ee0a78abe5e))
JAM_SND
SE_ROMEND
#define input_ports_spacejmi input_ports_spacejam
#define init_spacejmi init_spacejam
CORE_CLONEDEFNV(spacejmi,spacejam,"Space Jam (3.00 Italian)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Star Wars Trilogy (4.03)
/-------------------------------------------------------------------*/
#define SWTRI_SND \
DE2S_SOUNDROM144("sw0219.u7" ,CRC(cd7c84d9) SHA1(55b0208089933e4a30f0eb87b123dd178383ed43), \
                 "sw0211.u17",CRC(6863e7f6) SHA1(99f1e0170fbbb91a0eb7a796ab3bf757cb1b23ce), \
                 "sw0211.u21",CRC(6be68450) SHA1(d24652f74b109e47eb5d3d02e04f63c99e92c590))

INITGAME(swtril43,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(swtril43,"swcpu.403",CRC(d022880e) SHA1(c1a2acf5740cef6a8aeee9b6a60942b51147963f))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71) SHA1(036f53e638699de0447ecd02221f673a40f656be))
SWTRI_SND
SE_ROMEND
#define input_ports_swtril43 input_ports_se
CORE_GAMEDEFNV(swtril43,"Star Wars Trilogy (4.03)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(swtril41,"swcpu.401",CRC(707dce87) SHA1(45fc3ffe646e5be72af9f7f00990ee5f85338f34))
DE_DMD32ROM8x(    "swsedspa.400",CRC(b9bcbf71) SHA1(036f53e638699de0447ecd02221f673a40f656be))
SWTRI_SND
SE_ROMEND
#define input_ports_swtril41 input_ports_swtril43
#define init_swtril41 init_swtril43
CORE_CLONEDEFNV(swtril41,swtril43,"Star Wars Trilogy (4.01)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ The Lost World: Jurassic Park (2.02)
/-------------------------------------------------------------------*/
#define JURPK_SND \
DE2S_SOUNDROM144("jp2_u7.bin" ,CRC(73b74c96) SHA1(ffa47cbf1491ed4fbadc984189abbfffc70c9888), \
                 "jp2_u17.bin",CRC(8d6c0dbd) SHA1(e1179b2c94927a07efa7d16cf841d5ff7334ff36), \
                 "jp2_u21.bin",CRC(c670a997) SHA1(1576e11ec3669f61ff16188de31b9ef3a067c473))

INITGAME(jplstw22,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(jplstw22,"jp2cpu.202",CRC(d317e601) SHA1(fb4fee5fd08e0f1b5afb9817654bdb3d54a5220d))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace) SHA1(9d11f7623eec41972d2be4313c7715e30116d889))
JURPK_SND
SE_ROMEND
#define input_ports_jplstw22 input_ports_se
CORE_GAMEDEFNV(jplstw22,"Lost World: Jurassic Park, The (2.02)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(jplstw20,"jp2cpu.200",CRC(42f5526e) SHA1(66254492c981117c06567305237cadfc0ce391b0))
DE_DMD32ROM8x(    "jp2dspa.201",CRC(8fc41ace) SHA1(9d11f7623eec41972d2be4313c7715e30116d889))
JURPK_SND
SE_ROMEND
#define input_ports_jplstw20 input_ports_jplstw22
#define init_jplstw20 init_jplstw22
CORE_CLONEDEFNV(jplstw20,jplstw22,"Lost World: Jurassic Park, The (2.00)",1997,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ The X-Files
/-------------------------------------------------------------------*/
#define XFIL_SND \
DE2S_SOUNDROM144( "xfsndu7.512" ,CRC(01d65239) SHA1(9e680de940a15ef85a5615b789c58cd5973ff11b), \
                  "xfsndu17.c40",CRC(40bfc835) SHA1(2d6ac82acbbf9645bcb84fab7f285f2373e516a8), \
                  "xfsndu21.c40",CRC(b56a5ca6) SHA1(5fa23a8bb57e45aca159882226e603d9a6be078b))

INITGAME(xfiles,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(xfiles,"xfcpu.303",CRC(c7ab5efe) SHA1(dcb4b89adfb5ba39e59c1118a00b29941d3ea4e9))
DE_DMD32ROM8x(   "xfildspa.300",CRC(03c96af8) SHA1(06a26116f863bb9b2d127e18c5ba500537923d62))
XFIL_SND
SE_ROMEND
#define input_ports_xfiles input_ports_se
CORE_GAMEDEFNV(xfiles,"X-Files, The (3.03)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(xfilesf,"xfcpu.303",CRC(c7ab5efe) SHA1(dcb4b89adfb5ba39e59c1118a00b29941d3ea4e9))
DE_DMD32ROM8x(   "xfildspf.300",CRC(fe9b1292) SHA1(ead40d2cdff060829008f468e08512c4f5f9e055))
XFIL_SND
SE_ROMEND
#define input_ports_xfilesf input_ports_xfiles
#define init_xfilesf init_xfiles
CORE_CLONEDEFNV(xfilesf,xfiles,"X-Files, The (3.03 French)",1997,"Sega",de_mSES1,0)

SE128_ROMSTART(xfiles2,"xfcpu.204",CRC(a4913128) SHA1(1fe348725e13fd5dc56b6b2dbd173d0b49953483))
DE_DMD32ROM8x(   "xfildspa.201",CRC(bb015f24) SHA1(ca539d978ef0b8244227ea0c60087da5e7f0ee9e))
XFIL_SND
SE_ROMEND
#define input_ports_xfiles2 input_ports_xfiles
#define init_xfiles2 init_xfiles
CORE_CLONEDEFNV(xfiles2,xfiles,"X-Files, The (2.04)",1997,"Sega",de_mSES1,0)

INITGAME(xfiles20,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(xfiles20,"xfcpu.200",CRC(fd9e8ae8) SHA1(7f904eaae437bf938f01e9df875b9415167fc4c5))
DE_DMD32ROM8x(   "xfildspa.200",CRC(3fb161c3) SHA1(ea00c5c5a1e4908fcc34b0558b89325db091595d))
DE2S_SOUNDROM144( "xfsndu7.512" ,CRC(01d65239) SHA1(9e680de940a15ef85a5615b789c58cd5973ff11b),
                  "xfsndu17.c40",CRC(40bfc835) SHA1(2d6ac82acbbf9645bcb84fab7f285f2373e516a8),
                  "xfsndu21.c40",CRC(b56a5ca6) SHA1(5fa23a8bb57e45aca159882226e603d9a6be078b))
SE_ROMEND
#define input_ports_xfiles20 input_ports_xfiles
CORE_CLONEDEFNV(xfiles20,xfiles,"X-Files, The (2.00)",1997,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(startrp,"Starship Troopers (2.01)",1997,"Sega",de_mSES1,0)

INITGAME(startrp2,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(startrp2, "sstcpu.200",CRC(1bd865a5) SHA1(770d87d5108b58e214e551cfdcd4b75a11d6b88b))
DE_DMD32ROM8x(    "sstdspa.200",CRC(76a0e09e) SHA1(a4103aeee752d824a3811124079e40acc7286271))
DE2S_SOUNDROM1444("u7_b130.512" ,CRC(f1559e4f) SHA1(82b56f097412052bc1638a3f1c1319009df707f4),
                  "u17_152a.040",CRC(8caeccdb) SHA1(390f07e48a176a24fe99a202f3fa2b9767d84230),
                  "u21_0291.040",CRC(0c5321f6) SHA1(4a51daa16d489ab61d462d44f887c8422f863c5c),
                  "u36_95a7.040",CRC(c1e4ca6a) SHA1(487de78ebf1ee8cc721f2ef7b1bd42d2f7b27456))
SE_ROMEND
#define input_ports_startrp2 input_ports_startrp
CORE_CLONEDEFNV(startrp2,startrp,"Starship Troopers (2.00)",1997,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(viprsega,"Viper Night Drivin' (2.01)",1998,"Sega",de_mSES1,0)

INITGAME(vipr_102,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(vipr_102, "vipcpu.102",CRC(6046974b) SHA1(56e3de5ccb5a04d6ee5555ee6755835e75e7454f))
DE_DMD32ROM8x(   "vipdspa.100",CRC(25acf3db) SHA1(3476f2b95cfff9dfb4fe9cf7c5cccae85f23343a))
DE2S_SOUNDROM14444("vpru7.dat" ,CRC(f21617d7) SHA1(78d1ade400b83c62bb6288bccf386ef34050dd04),
                  "vpru17.dat",CRC(47b1317c) SHA1(32259965b5a12f63267af96eef8396bf71895a65),
                  "vpru21.dat",CRC(0e0e2dd6) SHA1(b409c837a52eb399c9a4896ca0c502360c93dcc9),
                  "vpru36.dat",CRC(7b482876) SHA1(c8960c2d45a77a35d22408c7bb8ba322e7af36f0),
                  "vpru37.dat",CRC(0bf23e0e) SHA1(b5724ed6cfe791320a8cf208cc20a2d3f0db85c8))
SE_ROMEND
#define input_ports_vipr_102 input_ports_viprsega
CORE_CLONEDEFNV(vipr_102, viprsega,"Viper Night Drivin' (1.02)",1998,"Sega",de_mSES1,0)

/*-------------------------------------------------------------------
/ Lost in Space
/-------------------------------------------------------------------*/
#define LIS_SND \
DE2S_SOUNDROM14444("lisu7.100",CRC(96e6b3c4) SHA1(5cfb43b8c182aed4b49ad1b8803812a18c6c8b6f), \
                  "lisu17.100",CRC(69076939) SHA1(f2cdf61a2b469d1a69eb3f08fc6e511d72336586), \
                  "lisu21.100",CRC(56eede09) SHA1(9ff53d7a188bd7293ad92089d143bd54623a50d4), \
                  "lisu36.100",CRC(56f2c53b) SHA1(5c2daf17116016fbead1320eb150cf655984662b), \
                  "lisu37.100",CRC(f9430c59) SHA1(f0f7169e63fc12d29fe39cd24dd67c5fb17779f7))

INITGAME(lostspc,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspc, "liscpu.101",CRC(81b2ced8) SHA1(a1933e2686b2a4e48d0f327593df95a927b132cb))
DE_DMD32ROM8x("lisdspa.102",CRC(e8bf4a58) SHA1(572313fb79e5a0c0034938a09b04ef43fc235c84))
LIS_SND
SE_ROMEND
#define input_ports_lostspc input_ports_se
CORE_GAMEDEFNV(lostspc,"Lost in Space (1.01, Display 1.02)",1998,"Sega",de_mSES1,0)

INITGAME(lostspcg,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspcg, "liscpu.101",CRC(81b2ced8) SHA1(a1933e2686b2a4e48d0f327593df95a927b132cb))
DE_DMD32ROM8x("lisdspg.102",CRC(66f3feb7) SHA1(a1f718193998f3210fb25c1353e4ae6703802311))
LIS_SND
SE_ROMEND
#define input_ports_lostspcg input_ports_lostspc
CORE_CLONEDEFNV(lostspcg,lostspc,"Lost in Space (1.01 German)",1998,"Sega",de_mSES1,0)

INITGAME(lostspcf,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspcf, "liscpu.101",CRC(81b2ced8) SHA1(a1933e2686b2a4e48d0f327593df95a927b132cb))
DE_DMD32ROM8x("lis_f-102.bin",CRC(422ba6d5) SHA1(0cd09b14a953fda39f8c7e5521c4115d2ada9186))
LIS_SND
SE_ROMEND
#define input_ports_lostspcf input_ports_lostspc
CORE_CLONEDEFNV(lostspcf,lostspc,"Lost in Space (1.01 French)",1998,"Sega",de_mSES1,0)

INITGAME(lostspc1,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(lostspc1, "liscpu.101",CRC(81b2ced8) SHA1(a1933e2686b2a4e48d0f327593df95a927b132cb))
DE_DMD32ROM8x("lisdspa.101",CRC(a8bfa71f) SHA1(45886ae8edcfd26a2225914aaf96eb960fc7e988))
LIS_SND
SE_ROMEND
#define input_ports_lostspc1 input_ports_lostspc
CORE_CLONEDEFNV(lostspc1,lostspc,"Lost in Space (1.01, Display 1.01)",1998,"Sega",de_mSES1,0)

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
CORE_GAMEDEFNV(godzilla,"Godzilla (2.05)",1998,"Sega",de_mSES1,0)

INITGAME(godz_100,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(godz_100,"gdzcpu.100",CRC(55c46a98) SHA1(535c363fed2359add260149b6407dc95da32a1e3))
DE_DMD32ROM8x(  "gzdspa.100",CRC(9b97cd98) SHA1(6fd002a6986aa32832c0628899ba1bafe3642354))
DE2S_SOUNDROM14444("gdzu7.100" ,CRC(a0afe8b7) SHA1(33e4a824b26b58e8f963fa8a525a64f4779b45db),
                  "gdzu17.100",CRC(6bba69c8) SHA1(51341e188b4191eb1836349dfdd456163d464ad6),
                  "gdzu21.100",CRC(db738958) SHA1(23082cf98bbcc6d356145414267da887a5ca9305),
                  "gdzu36.100",CRC(e3f24234) SHA1(eb123200928221a647e10839ebb7f4628501c581),
                  "gdzu37.100",CRC(2c1acb14) SHA1(4d710e09f5500da937932b4b01d862abb4a89e5a))
SE_ROMEND
#define input_ports_godz_100 input_ports_se
CORE_CLONEDEFNV(godz_100, godzilla,"Godzilla (1.00)", 1998, "Sega", de_mSES1, 0)

INITGAME(godz_090,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(godz_090,"gdzcpu.090",CRC(54e3b6d7) SHA1(c11cf71140c00c96c7feea569fa04f75061b9af7))
DE_DMD32ROM8x(  "gzdspa.090",CRC(56dde3a4) SHA1(332f09ade962e07a2979ad7bf743f632ea942440))
DE2S_SOUNDROM14444("gdzu7.090" ,CRC(076401a9) SHA1(17aa63c2b26e6fc4849a5101ff9704606de3de65),
                  "gdzu17.090",CRC(b15be745) SHA1(395631df3fef80641c189e57cddfc0ec5dcdbcef),
                  "gdzu21.090",CRC(019207d5) SHA1(a98d191d686d4a04f7fad90dd0e86e8b48ff3a3b),
                  "gdzu36.090",CRC(3913ccb9) SHA1(ddce224661894438a12135306484f711d10ce8be),
                  "gdzu37.090",CRC(1410ae6b) SHA1(28d025403fd60b1bb132cffcc14be21be48d808c))
SE_ROMEND
#define input_ports_godz_090 input_ports_se
CORE_CLONEDEFNV(godz_090, godzilla,"Godzilla (0.90 Prototype)", 1998, "Sega", de_mSES1, 0)

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

SE128_ROMSTART(sprk_096,"spkcpu.096",CRC(4d7e862a) SHA1(962c129504f678264ab847fac08bac91cbbccb33))
DE_DMD32ROM8x(    "spdspa.101",CRC(48ca598d) SHA1(0827ac7bb5cf12b0e63860b73a808273d984509e))
DE2S_SOUNDROM18888("spku7.090",CRC(19937fbd) SHA1(ebd7c8f1604accbeb7c00066ecf811193a2cb588),
                  "spku17.090",CRC(05a8670e) SHA1(7c0f1f0c9b94f0327c820f002bffc4ea05670ec8),
                  "spku21.090",CRC(c8629ee7) SHA1(843a742cb5cfce21a83618d14ae08ee1930d36cc),
                  "spku36.090",CRC(727d4624) SHA1(9019014e6057d279a37cc3ce269a1c68baeb9673),
                  "spku37.090",CRC(0c01b0c7) SHA1(76b5af50514d110b49721e6916dd16b3e3a2f5fa))
SE_ROMEND
CORE_CLONEDEF(sprk,096,103,"South Park (0.96 Prototype)",1999,"Sega",de_mSES1,0)

SE128_ROMSTART(sprk_090,"spkcpu.090",CRC(bc3f8531) SHA1(5408e008c4f545bb4f82308b118d15525f8a263a))
DE_DMD32ROM8x(    "spdspa.090",CRC(c333dd48) SHA1(fe2be9274c06b2f39fa2e14e0d44ce7213282f3b))
DE2S_SOUNDROM18888("spku7.090",CRC(19937fbd) SHA1(ebd7c8f1604accbeb7c00066ecf811193a2cb588),
                  "spku17.090",CRC(05a8670e) SHA1(7c0f1f0c9b94f0327c820f002bffc4ea05670ec8),
                  "spku21.090",CRC(c8629ee7) SHA1(843a742cb5cfce21a83618d14ae08ee1930d36cc),
                  "spku36.090",CRC(727d4624) SHA1(9019014e6057d279a37cc3ce269a1c68baeb9673),
                  "spku37.090",CRC(0c01b0c7) SHA1(76b5af50514d110b49721e6916dd16b3e3a2f5fa))
SE_ROMEND
CORE_CLONEDEF(sprk,090,103,"South Park (0.90 Prototype)",1999,"Sega",de_mSES1,0)

// Harley Davidson moved to its own sim file (harley)

/*-------------------------------------------------------------------
/ Striker Xtreme
/-------------------------------------------------------------------*/
INITGAME(strikext,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(strikext,"sxcpua.102", CRC(5e5f0fb8) SHA1(1425d66064e59193ce7cacb496c12ae956670435))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE2S_SOUNDROM18888("sxsnda.u7", CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                   "sxsnda.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                   "sxsnda.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                   "sxsnda.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                   "sxsnda.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strikext input_ports_se
CORE_GAMEDEFNV(strikext,"Striker Xtreme (1.02)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_uk,"sxcpue.101", CRC(eac29785) SHA1(42e01c501b4a0a7eaae244040777be8ba69860d5))
DE_DMD32ROM8x(     "sxdispa.103",CRC(e4cf849f) SHA1(27b61f1659762b396ca3af375e28f9c56500f79e))
DE2S_SOUNDROM18888("sxsnda.u7" ,CRC(e7e1a0cb) SHA1(be8b3e4d4232519db8344ae9e75f77d159bb1911),
                   "sxsnda.u17",CRC(aeeed88f) SHA1(e150fd243feffcdc5d66487e840cefdfb50213da),
                   "sxsnda.u21",CRC(62c9bfe3) SHA1(14a65a673a33b7e3d3005f76acf3098dc37958f8),
                   "sxsnda.u36",CRC(a0bc0edb) SHA1(1025a28fe9a0e3681e8e99b513da29ec294da045),
                   "sxsnda.u37",CRC(4c08c33c) SHA1(36bfad0c59fd228db76a6ff36698edd929c11336))
SE_ROMEND
#define input_ports_strxt_uk input_ports_strikext
#define init_strxt_uk init_strikext
CORE_CLONEDEFNV(strxt_uk,strikext,"Striker Xtreme (1.01 English)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_gr,"sxcpug.103", CRC(d73b9020) SHA1(491cd9db172db0a35870524ed4f1b15fb67f2e78))
DE_DMD32ROM8x(     "sxdispg.103",CRC(eb656489) SHA1(476315a5d22b6d8c63e9a592167a00f0c87e86c9))
DE2S_SOUNDROM18888("sxsndg.u7" ,CRC(b38ec07d) SHA1(239a3a51c049b007d4c16c3bd1e003a5dfd3cecc),
                   "sxsndg.u17",CRC(19ecf6ca) SHA1(f61f9e821bb0cf7978073a2d2cb939999265277b),
                   "sxsndg.u21",CRC(ee410b1e) SHA1(a0f7ff46536060be8f7c2c0e575e85814cd183e1),
                   "sxsndg.u36",CRC(f0e126c2) SHA1(a8c5eed27b33d20c2ff3bfd3d317c8b56bfa3625),
                   "sxsndg.u37",CRC(82260f4b) SHA1(6c2eba67762bcdd01e7b0c1b8b03b91b778444d4))
SE_ROMEND
#define input_ports_strxt_gr input_ports_strikext
#define init_strxt_gr init_strikext
CORE_CLONEDEFNV(strxt_gr,strikext,"Striker Xtreme (1.03 German)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_fr,"sxcpuf.102", CRC(2804bc9f) SHA1(3c75d8cc4d2833baa163d99c709dcb9475ba0831))
DE_DMD32ROM8x(     "sxdispf.103",CRC(4b4b5c19) SHA1(d2612a2b8099b45cb67ac9b55c88b5b10519d49b))
DE2S_SOUNDROM18888("sxsndf.u7" ,CRC(e68b0607) SHA1(cd3a5ff51932914e977fe866f7ab569d0901967a),
                   "sxsndf.u17",CRC(443efde2) SHA1(823f395cc5b0c7f5665bd8c804707fb9bbad1066),
                   "sxsndf.u21",CRC(e8ba1618) SHA1(920cecbdcfc948670ddf11b572af0bb536a1153d),
                   "sxsndf.u36",CRC(89107426) SHA1(9e3c51f17aee0e803e54f9400c304b4da0b8cf7a),
                   "sxsndf.u37",CRC(67c0f1de) SHA1(46867403d4b13d18c4ebcc5b042faf3aca165ffb))
SE_ROMEND
#define input_ports_strxt_fr input_ports_strikext
#define init_strxt_fr init_strikext
CORE_CLONEDEFNV(strxt_fr,strikext,"Striker Xtreme (1.02 French)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_it,"sxcpui.102", CRC(f955d0ef) SHA1(0f4ee87715bc085e2fb05e9ebdc89403f6bac444))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2) SHA1(a5e37ecf3b9772736ac88256c470f785dc113aa1))
DE2S_SOUNDROM18888("sxsndi.u7" ,CRC(81caf0a7) SHA1(5bb05c5bb49d12417b3ad49398623c3c222fd63b),
                   "sxsndi.u17",CRC(d0b21193) SHA1(2e5f92a67f0f18913e5d0af9936ab8694d095c66),
                   "sxsndi.u21",CRC(5ab3f8f4) SHA1(44982725eb31b0b144e3ad6549734b5fc46cd8c5),
                   "sxsndi.u36",CRC(4ee21ade) SHA1(1887f81b5f6753ce75ddcd0d7557c1644a925fcf),
                   "sxsndi.u37",CRC(4427e364) SHA1(7046b65086aafc4c14793d7036bc5130fe1e7dbc))
SE_ROMEND
#define input_ports_strxt_it input_ports_strikext
#define init_strxt_it init_strikext
CORE_CLONEDEFNV(strxt_it,strikext,"Striker Xtreme (1.02 Italian)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_it_101,"sxcpui.101", CRC(121e04de) SHA1(90c222106c3422a7cd10a493b2290315d8d7009f))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2) SHA1(a5e37ecf3b9772736ac88256c470f785dc113aa1))
DE2S_SOUNDROM18888("sxsndi.u7" ,CRC(81caf0a7) SHA1(5bb05c5bb49d12417b3ad49398623c3c222fd63b),
                   "sxsndi.u17",CRC(d0b21193) SHA1(2e5f92a67f0f18913e5d0af9936ab8694d095c66),
                   "sxsndi.u21",CRC(5ab3f8f4) SHA1(44982725eb31b0b144e3ad6549734b5fc46cd8c5),
                   "sxsndi.u36",CRC(4ee21ade) SHA1(1887f81b5f6753ce75ddcd0d7557c1644a925fcf),
                   "sxsndi.u37",CRC(4427e364) SHA1(7046b65086aafc4c14793d7036bc5130fe1e7dbc))
SE_ROMEND
#define input_ports_strxt_it_101 input_ports_strikext
#define init_strxt_it_101 init_strikext
CORE_CLONEDEFNV(strxt_it_101,strikext,"Striker Xtreme (1.01 Italian)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_it_100,"sxcpui.100", CRC(ee4742dd) SHA1(1b334f857ccb34f09eba69f3a40b589f6a712811))
DE_DMD32ROM8x(     "sxdispi.103",CRC(40be3fe2) SHA1(a5e37ecf3b9772736ac88256c470f785dc113aa1))
DE2S_SOUNDROM18888("s00.u7"    ,CRC(80625d23) SHA1(3956744f20c5a3281715ff813a8fd37cd8179342), // most likely this is a pre-1.00 ROM, but this set is the closest we have
                   "sxsndi.u17",CRC(d0b21193) SHA1(2e5f92a67f0f18913e5d0af9936ab8694d095c66),
                   "sxsndi.u21",CRC(5ab3f8f4) SHA1(44982725eb31b0b144e3ad6549734b5fc46cd8c5),
                   "sxsndi.u36",CRC(4ee21ade) SHA1(1887f81b5f6753ce75ddcd0d7557c1644a925fcf),
                   "sxsndi.u37",CRC(4427e364) SHA1(7046b65086aafc4c14793d7036bc5130fe1e7dbc))
SE_ROMEND
#define input_ports_strxt_it_100 input_ports_strikext
#define init_strxt_it_100 init_strikext
CORE_CLONEDEFNV(strxt_it_100,strikext,"Striker Xtreme (1.00 Italian)",2000,"Stern",de_mSES2,0)

SE128_ROMSTART(strxt_sp,"sxcpul.102", CRC(6b1e417f) SHA1(b87e8659bc4481384913a28c1cb2d7c96532b8e3))
DE_DMD32ROM8x(     "sxdispl.103",CRC(3efd4a18) SHA1(64f6998f82947a5bd053ad8dd56682adb239b676))
DE2S_SOUNDROM18888("sxsndl.u7" ,CRC(a03131cf) SHA1(e50f665eb15cef799fdc0d1d88bc7d5e23390225),
                   "sxsndl.u17",CRC(e7ee91cb) SHA1(1bc9992601bd7d194e2f33241179d682a62bff4b),
                   "sxsndl.u21",CRC(88cbf553) SHA1(d6afd262b47e31983c734c0054a7af2489da2f13),
                   "sxsndl.u36",CRC(35474ad5) SHA1(818a0f86fa4aa0b0457c16a20f8358655c42ea0a),
                   "sxsndl.u37",CRC(0e53f2a0) SHA1(7b89989ff87c25618d6f1c6479efd45b57f850fb))
SE_ROMEND
#define input_ports_strxt_sp input_ports_strikext
#define init_strxt_sp init_strikext
CORE_CLONEDEFNV(strxt_sp,strikext,"Striker Xtreme (1.02 Spanish)",2000,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Sharkey's Shootout
/-------------------------------------------------------------------*/
#define SSHOT_SND \
DE2S_SOUNDROM1888("sssndu7.101", CRC(fbc6267b) SHA1(e6e70662031e5209385f8b9c59296d7423cc03b4), \
                  "sssndu17.100",CRC(dae78d8d) SHA1(a0e1722a19505e7d08266c490d27f772357722d3), \
                  "sssndu21.100",CRC(e1fa3f2a) SHA1(08731fd53ef81453a8f20602e76d435c6771bbb9), \
                  "sssndu36.100",CRC(d22fcfa3) SHA1(3fa407f72ecc64f9d00b92122c4e4d85022e4202))

INITGAME(shrkysht,GEN_WS_1,se_dmd128x32,0)
SE128_ROMSTART(shrkysht,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispa.201",CRC(3360300b) SHA1(3169651a49bb7168fc04b2ae678b696ec6a21c85))
SSHOT_SND
SE_ROMEND
#define input_ports_shrkysht input_ports_se
CORE_GAMEDEFNV(shrkysht,"Sharkey's Shootout (2.11)",2000,"Stern",de_mSES1,0)

SE128_ROMSTART(shrky_gr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispg.201",CRC(1ad88b75) SHA1(f82ae59b59e545e2023879aad7e99862d9d339c5))
SSHOT_SND
SE_ROMEND
#define input_ports_shrky_gr input_ports_shrkysht
#define init_shrky_gr init_shrkysht
CORE_CLONEDEFNV(shrky_gr,shrkysht,"Sharkey's Shootout (2.11 German)",2001,"Stern",de_mSES1,0)

SE128_ROMSTART(shrky_fr,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispf.201",CRC(89eaaebf) SHA1(0945a11a8f632cea3b9e9982cc4aed54f12ec55a))
SSHOT_SND
SE_ROMEND
#define input_ports_shrky_fr input_ports_shrkysht
#define init_shrky_fr init_shrkysht
CORE_CLONEDEFNV(shrky_fr,shrkysht,"Sharkey's Shootout (2.11 French)",2001,"Stern",de_mSES1,0)

SE128_ROMSTART(shrky_it,"sscpu.211",CRC(66a0e5ce) SHA1(d33615081cd8cdf8a17a44b389c6d9746e093510))
DE_DMD32ROM8x(        "ssdispi.201",CRC(fb70641d) SHA1(d152838eeacacc5dfe6fc3bc3f26b4d3e9e4c9cc))
SSHOT_SND
SE_ROMEND
#define input_ports_shrky_it input_ports_shrkysht
#define init_shrky_it init_shrkysht
CORE_CLONEDEFNV(shrky_it,shrkysht,"Sharkey's Shootout (2.11 Italian)",2001,"Stern",de_mSES1,0)

SE128_ROMSTART(shrky207,"sscpu.207",CRC(7355d65d) SHA1(d501cf1ff8b4be01970d47997f71f539df57b702))
DE_DMD32ROM8x(        "ssdispa.201",CRC(3360300b) SHA1(3169651a49bb7168fc04b2ae678b696ec6a21c85))
SSHOT_SND
SE_ROMEND
#define input_ports_shrky207 input_ports_shrkysht
#define init_shrky207 init_shrkysht
CORE_CLONEDEFNV(shrky207,shrkysht,"Sharkey's Shootout (2.07)",2001,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ High Roller Casino
/-------------------------------------------------------------------*/
#define HIROL_SND \
DE2S_SOUNDROM18888("hrsndu7.100", CRC(c41f91a7) SHA1(2af3be10754ea633558bdbeded21c6f82d85cd1d), \
                   "hrsndu17.100",CRC(5858dfd0) SHA1(0d88daf3b480f0e0a2653d9be37cafed79036a48), \
                   "hrsndu21.100",CRC(c653de96) SHA1(352567f4f4f973ed3d8c018c9bf37aeecdd101bf), \
                   "hrsndu36.100",CRC(5634eb4e) SHA1(8c76f49423fc0d7887aa5c57ad029b7371372739), \
                   "hrsndu37.100",CRC(d4d23c00) SHA1(c574dc4553bff693d9216229ce38a55f69e7368a))

static struct core_dispLayout dispHRC[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 15, CORE_DMD | CORE_NODISP, (genf *)seminidmd1_update, NULL}, {0}
};
INITGAME(hirolcas,GEN_WS,dispHRC,SE_MINIDMD)
SE128_ROMSTART(hirolcas,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispa.300",CRC(099ccaf0) SHA1(2e0c2706881208f08e8a1d30915424c8f9b1cf67))
HIROL_SND
SE_ROMEND
#define input_ports_hirolcas input_ports_se
CORE_GAMEDEFNV(hirolcas,"High Roller Casino (3.00)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirolcat,"hrccput3.300",CRC(b70e04a0) SHA1(0c8d6c1e488471617ba9e24704d0d44826c1daf3))
DE_DMD32ROM8x(        "hrcdspt3.300",CRC(e262f36c) SHA1(116b2b96adce953e00d1e6d7f2b4ed4cdc4a3f61))
HIROL_SND
SE_ROMEND
#define input_ports_hirolcat input_ports_hirolcas
#define init_hirolcat init_hirolcas
CORE_CLONEDEFNV(hirolcat,hirolcas,"High Roller Casino (3.00 TEST BUILD)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirol_g3,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispg.300",CRC(a880903a) SHA1(4049f50ceaeb6c9e869150ec3d903775cdd865ff))
HIROL_SND
SE_ROMEND
#define input_ports_hirol_g3 input_ports_hirolcas
#define init_hirol_g3 init_hirolcas
CORE_CLONEDEFNV(hirol_g3,hirolcas,"High Roller Casino (3.00 German)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirol_fr,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispf.300",CRC(1fb5046b) SHA1(8b121a9c75a7d9a312b8c03615838b748d149819))
HIROL_SND
SE_ROMEND
#define input_ports_hirol_fr input_ports_hirolcas
#define init_hirol_fr init_hirolcas
CORE_CLONEDEFNV(hirol_fr,hirolcas,"High Roller Casino (3.00 French)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirol_it,"hrccpu.300",CRC(0d1117fa) SHA1(a19f9dfc2288fc16cb8e992ffd7f13e70ef042c7))
DE_DMD32ROM8x(        "hrcdispi.300",CRC(2734f746) SHA1(aa924d998b6c3fbd80e9325093c9b3267dfaadef))
HIROL_SND
SE_ROMEND
#define input_ports_hirol_it input_ports_hirolcas
#define init_hirol_it init_hirolcas
CORE_CLONEDEFNV(hirol_it,hirolcas,"High Roller Casino (3.00 Italian)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirol210,"hrccpu.210",CRC(2e3c682a) SHA1(d9993ae7a0aad80e1eeff226a635873cb25437ce))
DE_DMD32ROM8x(        "hrcdispa.200",CRC(642bdce7) SHA1(7cd922a15c1443c6ed7636c9def4bc3ab0b47096))
HIROL_SND
SE_ROMEND
#define input_ports_hirol210 input_ports_hirolcas
#define init_hirol210 init_hirolcas
CORE_CLONEDEFNV(hirol210,hirolcas,"High Roller Casino (2.10)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(hirol_gr,"hrccpu.210",CRC(2e3c682a) SHA1(d9993ae7a0aad80e1eeff226a635873cb25437ce))
DE_DMD32ROM8x(        "hrcdispg.201",CRC(57b95712) SHA1(f7abe7511aa8b258615cd844dc76f3d2f9b47c31))
HIROL_SND
SE_ROMEND
#define input_ports_hirol_gr input_ports_hirolcas
#define init_hirol_gr init_hirolcas
CORE_CLONEDEFNV(hirol_gr,hirolcas,"High Roller Casino (2.10 German)",2001,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Austin Powers
/-------------------------------------------------------------------*/
#define AUSTIN_SND \
DE2S_SOUNDROM18888("apsndu7.100", CRC(d0e79d59) SHA1(7c3f1fa79ff193a976986339a551e3d03208550f), \
                   "apsndu17.100",CRC(c1e33fee) SHA1(5a3581584cc1a841d884de4628f7b65d8670f96a), \
                   "apsndu21.100",CRC(07c3e077) SHA1(d48020f7da400c3682035d537289ce9a30732d74), \
                   "apsndu36.100",CRC(f70f2828) SHA1(9efbed4f68c22eb26e9100afaca5ebe85a97b605), \
                   "apsndu37.100",CRC(ddf0144b) SHA1(c2a56703a41ee31841993d63385491259d5a13f8))

INITGAME(austin,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(austin,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb) SHA1(952a7873067b8c70043a38a39a8f65089103336b))
AUSTIN_SND
SE_ROMEND
#define input_ports_austin input_ports_se
CORE_GAMEDEFNV(austin,"Austin Powers (3.02)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(austinf,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-f.300",CRC(1aeaa83e) SHA1(8a749c0fbf7b03441780c2158e63d4a87c8d0702))
AUSTIN_SND
SE_ROMEND
#define input_ports_austinf input_ports_austin
#define init_austinf init_austin
CORE_CLONEDEFNV(austinf,austin,"Austin Powers (3.02 French)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(austing,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-g.300",CRC(28b91cc4) SHA1(037628c78955495f10a60cfc329232289417562e))
AUSTIN_SND
SE_ROMEND
#define input_ports_austing input_ports_austin
#define init_austing init_austin
CORE_CLONEDEFNV(austing,austin,"Austin Powers (3.02 German)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(austini,"apcpu.302",CRC(2920b59b) SHA1(280cebbb39980fbcfd91fc1cf87a40ad926ffecb))
DE_DMD32ROM8x(       "apdsp-i.300",CRC(8b1dd747) SHA1(b29d39a2fb464bd11f4bc5daeb35360126ddf45b))
AUSTIN_SND
SE_ROMEND
#define input_ports_austini input_ports_austin
#define init_austini init_austin
CORE_CLONEDEFNV(austini,austin,"Austin Powers (3.02 Italian)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(aust301,"apcpu.301",CRC(868d1f38) SHA1(df08b48437f88e66c4caa80602c28a2223f180b9))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb) SHA1(952a7873067b8c70043a38a39a8f65089103336b))
AUSTIN_SND
SE_ROMEND
#define input_ports_aust301 input_ports_austin
#define init_aust301 init_austin
CORE_CLONEDEFNV(aust301,austin,"Austin Powers (3.01)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(aust300,"apcpu.300",CRC(a06b2b03) SHA1(4c36212b43fdc497773425e586f64c3064e7000c))
DE_DMD32ROM8x(      "apdsp-a.300",CRC(ecf2c3bb) SHA1(952a7873067b8c70043a38a39a8f65089103336b))
AUSTIN_SND
SE_ROMEND
#define input_ports_aust300 input_ports_austin
#define init_aust300 init_austin
CORE_CLONEDEFNV(aust300,austin,"Austin Powers (3.00)",2001,"Stern",de_mSES2,0)

SE128_ROMSTART(aust201,"apcpu.201",CRC(a4ddcdca) SHA1(c1eb1ae3b9c9b10410d107165f3bddaa514c2113))
DE_DMD32ROM8x(      "apdsp-a.200",CRC(f3ca7fca) SHA1(b6b702ad7af75b3010a280adb99e4ee484a03242))
AUSTIN_SND
SE_ROMEND
#define input_ports_aust201 input_ports_austin
#define init_aust201 init_austin
CORE_CLONEDEFNV(aust201,austin,"Austin Powers (2.01)",2001,"Stern",de_mSES2,0)

// Monopoly moved to its own sim file (monopoly)

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
/ Playboy
/-------------------------------------------------------------------*/
#define PLAY_SND \
DE2S_SOUNDROM18888("pbsndu7.102",  CRC(12a68f34) SHA1(f2cd42918dec353883bc465f6302c2d94dcd6b87), \
                   "pbsndu17.100", CRC(f5502fec) SHA1(c8edd56e0c4365dd6b4bef0f1c7cc83ea5fd73d6), \
                   "pbsndu21.100", CRC(7869d34f) SHA1(48a051045523c14ca06a7227b34ed9e3818828d0), \
                   "pbsndu36.100", CRC(d10f14a3) SHA1(972b480c23d484b627ecdce0322c08fe760a127f), \
                   "pbsndu37.100", CRC(6642524a) SHA1(9d0c0be5887cf4510c11243ee47b11c08cbae17c))

INITGAME(playboys,GEN_WS,se_dmd128x32,0)
SE128_ROMSTART(playboys, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispa.500",  CRC(40450165) SHA1(80295466838cb08fe1499b19a98bf759fb7a306e))
PLAY_SND
SE_ROMEND
#define input_ports_playboys input_ports_se
CORE_GAMEDEFNV(playboys,"Playboy (Stern, 5.00)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(playboyf, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispf.500",  CRC(aedc6c32) SHA1(c930ae1b1308ae641553de34f8249b17f408be56))
PLAY_SND
SE_ROMEND
#define input_ports_playboyf input_ports_playboys
#define init_playboyf init_playboys
CORE_CLONEDEFNV(playboyf,playboys,"Playboy (Stern, 5.00 French)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(playboyg, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispg.500",  CRC(681392fe) SHA1(23011d538282da144b8ff9cbb7c5655567017e5e))
PLAY_SND
SE_ROMEND
#define input_ports_playboyg input_ports_playboys
#define init_playboyg init_playboys
CORE_CLONEDEFNV(playboyg,playboys,"Playboy (Stern, 5.00 German)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(playboyi, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispi.500",  CRC(d90d7ec6) SHA1(7442160f403a8ccabfc9dc8dc53f8b44f5df26bc))
PLAY_SND
SE_ROMEND
#define input_ports_playboyi input_ports_playboys
#define init_playboyi init_playboys
CORE_CLONEDEFNV(playboyi,playboys,"Playboy (Stern, 5.00 Italian)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(playboyl, "pbcpu.500",    CRC(e4d924ae) SHA1(2aab64eee57e9569c3cc1bde28cda69ff4cacc69))
DE_DMD32ROM8x(           "pbdispl.500",  CRC(b019f0f6) SHA1(184a9905cd3af9d272577e43666aed5e5a8a5281))
PLAY_SND
SE_ROMEND
#define input_ports_playboyl input_ports_playboys
#define init_playboyl init_playboys
CORE_CLONEDEFNV(playboyl,playboys,"Playboy (Stern, 5.00 Spanish)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play401,  "pbcpu.401",    CRC(cb2e2824) SHA1(01413ff1f4fbee9d62776babb60ccc88f97feedf))
DE_DMD32ROM8x(           "pbdispa.400",  CRC(244e9740) SHA1(1a2de2c53104e1123cbcc9ccc92e0888b4cf4bec))
PLAY_SND
SE_ROMEND
#define input_ports_play401 input_ports_playboys
#define init_play401 init_playboys
CORE_CLONEDEFNV(play401,playboys,"Playboy (Stern, 4.01)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play401l, "pbcpu.401",    CRC(cb2e2824) SHA1(01413ff1f4fbee9d62776babb60ccc88f97feedf))
DE_DMD32ROM8x(           "pbdisps.400",  CRC(92b48fdd) SHA1(deebb0bf6ec13b0cead3970c19bd0e8321046786))
PLAY_SND
SE_ROMEND
#define input_ports_play401l input_ports_playboys
#define init_play401l init_playboys
CORE_CLONEDEFNV(play401l,playboys,"Playboy (Stern, 4.01 Spanish)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play401g, "pbcpu.401",    CRC(cb2e2824) SHA1(01413ff1f4fbee9d62776babb60ccc88f97feedf))
DE_DMD32ROM8x(           "pbdispg.400",  CRC(c26a0c73) SHA1(1841ab278e5d3e146cb3b43dfedc208d302dbc17))
PLAY_SND
SE_ROMEND
#define input_ports_play401g input_ports_playboys
#define init_play401g init_playboys
CORE_CLONEDEFNV(play401g,playboys,"Playboy (Stern, 4.01 German)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play401f, "pbcpu.401",    CRC(cb2e2824) SHA1(01413ff1f4fbee9d62776babb60ccc88f97feedf))
DE_DMD32ROM8x(           "pbdispf.400",  CRC(8ccce5d9) SHA1(28370445d38b065001e96b455d283bc047ed0f21))
PLAY_SND
SE_ROMEND
#define input_ports_play401f input_ports_playboys
#define init_play401f init_playboys
CORE_CLONEDEFNV(play401f,playboys,"Playboy (Stern, 4.01 French)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play401i, "pbcpu.401",    CRC(cb2e2824) SHA1(01413ff1f4fbee9d62776babb60ccc88f97feedf))
DE_DMD32ROM8x(           "pbdispi.400",  CRC(90c5afed) SHA1(ea7ab06adb18854c9871b02f39fe616a27293415))
PLAY_SND
SE_ROMEND
#define input_ports_play401i input_ports_playboys
#define init_play401i init_playboys
CORE_CLONEDEFNV(play401i,playboys,"Playboy (Stern, 4.01 Italian)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play303,  "pbcpu.303",    CRC(6a6f6aab) SHA1(cac5d68d699db4016244ffd56355e2834be3da1f))
DE_DMD32ROM8x(           "pbdispa.300",  CRC(2dbb372a) SHA1(b694ae06f380ea9f9730ea6bbfab7f9f7ec7342b))
PLAY_SND
SE_ROMEND
#define input_ports_play303 input_ports_playboys
#define init_play303 init_playboys
CORE_CLONEDEFNV(play303,playboys,"Playboy (Stern, 3.03)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play303l, "pbcpu.303",    CRC(6a6f6aab) SHA1(cac5d68d699db4016244ffd56355e2834be3da1f))
DE_DMD32ROM8x(           "pbdispl.300",  CRC(e7697fc3) SHA1(7a9796e7b99af0d3d2079876a8054209a3067e64))
PLAY_SND
SE_ROMEND
#define input_ports_play303l input_ports_playboys
#define init_play303l init_playboys
CORE_CLONEDEFNV(play303l,playboys,"Playboy (Stern, 3.03 Spanish)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play303g, "pbcpu.303",    CRC(6a6f6aab) SHA1(cac5d68d699db4016244ffd56355e2834be3da1f))
DE_DMD32ROM8x(           "pbdispg.300",  CRC(ed7b7c62) SHA1(28b0ab490f8abd5f29e8cb0996da9e7200918157))
PLAY_SND
SE_ROMEND
#define input_ports_play303g input_ports_playboys
#define init_play303g init_playboys
CORE_CLONEDEFNV(play303g,playboys,"Playboy (Stern, 3.03 German)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play303f, "pbcpu.303",    CRC(6a6f6aab) SHA1(cac5d68d699db4016244ffd56355e2834be3da1f))
DE_DMD32ROM8x(           "pbdispf.300",  CRC(69ab3bb2) SHA1(59d7ad5eca701d1216200cd489d2d07825a0856e))
PLAY_SND
SE_ROMEND
#define input_ports_play303f input_ports_playboys
#define init_play303f init_playboys
CORE_CLONEDEFNV(play303f,playboys,"Playboy (Stern, 3.03 French)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play303i, "pbcpu.303",    CRC(6a6f6aab) SHA1(cac5d68d699db4016244ffd56355e2834be3da1f))
DE_DMD32ROM8x(           "pbdispi.300",  CRC(74c8cedf) SHA1(474ad1939ea0a58852003e549ed85478e239a67c))
PLAY_SND
SE_ROMEND
#define input_ports_play303i input_ports_playboys
#define init_play303i init_playboys
CORE_CLONEDEFNV(play303i,playboys,"Playboy (Stern, 3.03 Italian)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play302,  "pbcpu.302",    CRC(206285ed) SHA1(65ec90b20f7be6fac62170f69b744f9e4eb6254c))
DE_DMD32ROM8x(           "pbdispa.300",  CRC(2dbb372a) SHA1(b694ae06f380ea9f9730ea6bbfab7f9f7ec7342b))
PLAY_SND
SE_ROMEND
#define input_ports_play302 input_ports_playboys
#define init_play302 init_playboys
CORE_CLONEDEFNV(play302,playboys,"Playboy (Stern, 3.02)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play302l, "pbcpu.302",    CRC(206285ed) SHA1(65ec90b20f7be6fac62170f69b744f9e4eb6254c))
DE_DMD32ROM8x(           "pbdispl.300",  CRC(e7697fc3) SHA1(7a9796e7b99af0d3d2079876a8054209a3067e64))
PLAY_SND
SE_ROMEND
#define input_ports_play302l input_ports_playboys
#define init_play302l init_playboys
CORE_CLONEDEFNV(play302l,playboys,"Playboy (Stern, 3.02 Spanish)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play302g, "pbcpu.302",    CRC(206285ed) SHA1(65ec90b20f7be6fac62170f69b744f9e4eb6254c))
DE_DMD32ROM8x(           "pbdispg.300",  CRC(ed7b7c62) SHA1(28b0ab490f8abd5f29e8cb0996da9e7200918157))
PLAY_SND
SE_ROMEND
#define input_ports_play302g input_ports_playboys
#define init_play302g init_playboys
CORE_CLONEDEFNV(play302g,playboys,"Playboy (Stern, 3.02 German)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play302f, "pbcpu.302",    CRC(206285ed) SHA1(65ec90b20f7be6fac62170f69b744f9e4eb6254c))
DE_DMD32ROM8x(           "pbdispf.300",  CRC(69ab3bb2) SHA1(59d7ad5eca701d1216200cd489d2d07825a0856e))
PLAY_SND
SE_ROMEND
#define input_ports_play302f input_ports_playboys
#define init_play302f init_playboys
CORE_CLONEDEFNV(play302f,playboys,"Playboy (Stern, 3.02 French)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play302i, "pbcpu.302",    CRC(206285ed) SHA1(65ec90b20f7be6fac62170f69b744f9e4eb6254c))
DE_DMD32ROM8x(           "pbdispi.300",  CRC(74c8cedf) SHA1(474ad1939ea0a58852003e549ed85478e239a67c))
PLAY_SND
SE_ROMEND
#define input_ports_play302i input_ports_playboys
#define init_play302i init_playboys
CORE_CLONEDEFNV(play302i,playboys,"Playboy (Stern, 3.02 Italian)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play300,  "pbcpu.300",    CRC(d7e5bada) SHA1(e4d5bc015751a559eb95acb6da04246b7418eaf5))
DE_DMD32ROM8x(           "pbdispa.300",  CRC(2dbb372a) SHA1(b694ae06f380ea9f9730ea6bbfab7f9f7ec7342b))
PLAY_SND
SE_ROMEND
#define input_ports_play300 input_ports_playboys
#define init_play300 init_playboys
CORE_CLONEDEFNV(play300,playboys,"Playboy (Stern, 3.00)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play203,  "pbcpu.203",    CRC(50eb01b0) SHA1(1618874f35432bd9fb2592e1a56592e7624257c4))
DE_DMD32ROM8x(           "pbdisp-a.201", CRC(78ec6af8) SHA1(35b8de8ab345cf81eec4f7b7d4f654115fe69ddf))
PLAY_SND
SE_ROMEND
#define input_ports_play203 input_ports_playboys
#define init_play203 init_playboys
CORE_CLONEDEFNV(play203,playboys,"Playboy (Stern, 2.03)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play203l, "pbcpu.203",    CRC(50eb01b0) SHA1(1618874f35432bd9fb2592e1a56592e7624257c4))
DE_DMD32ROM8x(           "pbdisp-l.201", CRC(eaa65c45) SHA1(4ec9f815e40ac2185812faad7b991723f0b5775b))
PLAY_SND
SE_ROMEND
#define input_ports_play203l input_ports_playboys
#define init_play203l init_playboys
CORE_CLONEDEFNV(play203l,playboys,"Playboy (Stern, 2.03 Spanish)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play203g, "pbcpu.203",    CRC(50eb01b0) SHA1(1618874f35432bd9fb2592e1a56592e7624257c4))
DE_DMD32ROM8x(           "pbdisp-g.201", CRC(ff525cc7) SHA1(475578cf8b2262a11f640883b70b706f705d90ff))
PLAY_SND
SE_ROMEND
#define input_ports_play203g input_ports_playboys
#define init_play203g init_playboys
CORE_CLONEDEFNV(play203g,playboys,"Playboy (Stern, 2.03 German)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play203f, "pbcpu.203",    CRC(50eb01b0) SHA1(1618874f35432bd9fb2592e1a56592e7624257c4))
DE_DMD32ROM8x(           "pbdisp-f.201", CRC(eedea4f4) SHA1(31eb1d4de0a4aee73c424c0f21dd2042e6ad0dca))
PLAY_SND
SE_ROMEND
#define input_ports_play203f input_ports_playboys
#define init_play203f init_playboys
CORE_CLONEDEFNV(play203f,playboys,"Playboy (Stern, 2.03 French)",2002,"Stern",de_mSES2,0)

SE128_ROMSTART(play203i, "pbcpu.203",    CRC(50eb01b0) SHA1(1618874f35432bd9fb2592e1a56592e7624257c4))
DE_DMD32ROM8x(           "pbdisp-i.201", CRC(48f190dc) SHA1(bf5c096f755871b12783a63b55908751f9fa5b07))
PLAY_SND
SE_ROMEND
#define input_ports_play203i input_ports_playboys
#define init_play203i init_playboys
CORE_CLONEDEFNV(play203i,playboys,"Playboy (Stern, 2.03 Italian)",2002,"Stern",de_mSES2,0)

//Strange that they went back to the 11 voice model here!
/*-------------------------------------------------------------------
/ RollerCoaster Tycoon
/-------------------------------------------------------------------*/
#define RCT_SND \
DE2S_SOUNDROM1888("rcsndu7.100", CRC(e6cde9b1) SHA1(cbaadafd18ad9c0338bf2cce94b2c2a89e734778), \
                  "rcsndu17.100",CRC(18ba20ec) SHA1(c6e7a8a5fd6029b8e884a6eeb779a10d819e8c7c), \
                  "rcsndu21.100",CRC(64b19c11) SHA1(9ac714d372437cf1d8c4e01512c0647f13e40ddb), \
                  "rcsndu36.100",CRC(05c8bac9) SHA1(0771a393d5361c9a35d42a18b6c6a105b7752e03))

static struct core_dispLayout dispRCT[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 5,21, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)seminidmd3_update, NULL}, {0}
};
INITGAME(rctycn, GEN_WS, dispRCT, SE_MINIDMD3)
SE128_ROMSTART(rctycn, "rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispa.701",CRC(0d527f13) SHA1(954116a79578b2a7679c401a2bb99b5bbfb603f2))
RCT_SND
SE_ROMEND
#define input_ports_rctycn input_ports_se
CORE_GAMEDEFNV(rctycn,"RollerCoaster Tycoon (7.02)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rctycnl,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdisps.701",CRC(0efa8208) SHA1(6706ea3e31b478970fc65a8cf9db749db9c0fa39))
SE_ROMEND
#define input_ports_rctycnl input_ports_rctycn
#define init_rctycnl init_rctycn
CORE_CLONEDEFNV(rctycnl,rctycn,"RollerCoaster Tycoon (7.02 Spanish)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rctycng,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispg.701",CRC(6c70ab29) SHA1(fa3b713b79c0d724b918fa318795681308a4fce3))
RCT_SND
SE_ROMEND
#define input_ports_rctycng input_ports_rctycn
#define init_rctycng init_rctycn
CORE_CLONEDEFNV(rctycng,rctycn,"RollerCoaster Tycoon (7.02 German)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rctycnf,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispf.701",CRC(4a3bb40c) SHA1(053f494765ac7708401a7816af7186e71083fe8d))
RCT_SND
SE_ROMEND
#define input_ports_rctycnf input_ports_rctycn
#define init_rctycnf init_rctycn
CORE_CLONEDEFNV(rctycnf,rctycn,"RollerCoaster Tycoon (7.02 French)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rctycni,"rctcpu.702",CRC(5736a816) SHA1(fcfd06eeca74df0bca2c0bc57aeaa00400e4ab55))
DE_DMD32ROM8x(       "rctdispi.701",CRC(6adc8236) SHA1(bb908c88e47987777c921f2948dfb802ee29c868))
RCT_SND
SE_ROMEND
#define input_ports_rctycni input_ports_rctycn
#define init_rctycni init_rctycn
CORE_CLONEDEFNV(rctycni,rctycn,"RollerCoaster Tycoon (7.02 Italian)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct701, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(       "rctdispa.700",CRC(6a8925d7) SHA1(82a6c069f1e8f8e053fec708f56c8ffe56d70fc8))
RCT_SND
SE_ROMEND
#define input_ports_rct701 input_ports_rctycn
#define init_rct701 init_rctycn
CORE_CLONEDEFNV(rct701,rctycn,"RollerCoaster Tycoon (7.01)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct701l, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(        "rctdisps.700",CRC(6921d8cc) SHA1(1ada415af8e949829ceac75da507982ea2091f4d))
RCT_SND
SE_ROMEND
#define input_ports_rct701l input_ports_rctycn
#define init_rct701l init_rctycn
CORE_CLONEDEFNV(rct701l,rctycn,"RollerCoaster Tycoon (7.01 Spanish)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct701g, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(        "rctdispg.700",CRC(0babf1ed) SHA1(db683aa392968d255d355d4a1b0c9d8d4fb9e799))
RCT_SND
SE_ROMEND
#define input_ports_rct701g input_ports_rctycn
#define init_rct701g init_rctycn
CORE_CLONEDEFNV(rct701g,rctycn,"RollerCoaster Tycoon (7.01 German)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct701f, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(        "rctdispf.700",CRC(2de0eec8) SHA1(5b48eabddc1fb735866767ae008baf58205954db))
RCT_SND
SE_ROMEND
#define input_ports_rct701f input_ports_rctycn
#define init_rct701f init_rctycn
CORE_CLONEDEFNV(rct701f,rctycn,"RollerCoaster Tycoon (7.01 French)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct701i, "rctcpu.701",CRC(e1fe89f6) SHA1(9a76a5c267e055fcf0418394762bcea758da02d6))
DE_DMD32ROM8x(        "rctdispi.700",CRC(0d07d8f2) SHA1(3ffd8ad7183ba20844c1e5d1933c8002ca4707aa))
RCT_SND
SE_ROMEND
#define input_ports_rct701i input_ports_rctycn
#define init_rct701i init_rctycn
CORE_CLONEDEFNV(rct701i,rctycn,"RollerCoaster Tycoon (7.01 Italian)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct600, "rctcpu.600",CRC(2ada30e5) SHA1(fdcd608af155b16c15ec14c83927004c63e9c513))
DE_DMD32ROM8x(       "rctdispa.600",CRC(dbd294e1) SHA1(53c540594bfe5544b4a009665a1ca3edb1cb874c))
RCT_SND
SE_ROMEND
#define input_ports_rct600 input_ports_rctycn
#define init_rct600 init_rctycn
CORE_CLONEDEFNV(rct600,rctycn,"RollerCoaster Tycoon (6.00)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct600l, "rctcpu.600",CRC(2ada30e5) SHA1(fdcd608af155b16c15ec14c83927004c63e9c513))
DE_DMD32ROM8x(        "rctdisps.600",CRC(84a970a3) SHA1(8e00830ba446c8cef99d8e721b65597d9dd98379))
RCT_SND
SE_ROMEND
#define input_ports_rct600l input_ports_rctycn
#define init_rct600l init_rctycn
CORE_CLONEDEFNV(rct600l,rctycn,"RollerCoaster Tycoon (6.00 Spanish)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct600f, "rctcpu.600",CRC(2ada30e5) SHA1(fdcd608af155b16c15ec14c83927004c63e9c513))
DE_DMD32ROM8x(        "rctdispf.600",CRC(50aa2f48) SHA1(e7c0fb30ef15861e342bd6fada45885bd2403547))
RCT_SND
SE_ROMEND
#define input_ports_rct600f input_ports_rctycn
#define init_rct600f init_rctycn
CORE_CLONEDEFNV(rct600f,rctycn,"RollerCoaster Tycoon (6.00 French)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct600i, "rctcpu.600",CRC(2ada30e5) SHA1(fdcd608af155b16c15ec14c83927004c63e9c513))
DE_DMD32ROM8x(        "rctdispi.600",CRC(a55a86c1) SHA1(b18197063e2e4cc50b04f43de41d870eb672ce68))
RCT_SND
SE_ROMEND
#define input_ports_rct600i input_ports_rctycn
#define init_rct600i init_rctycn
CORE_CLONEDEFNV(rct600i,rctycn,"RollerCoaster Tycoon (6.00 Italian)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct400, "rctcpu.400",CRC(4691de23) SHA1(f47fc03f9b030cb7a1201d015bbec5a426176338))
DE_DMD32ROM8x(       "rctdsp-a.400",CRC(d128a8fa) SHA1(8fece3df33d21da020ba06d14d588384d90dd89f))
RCT_SND
SE_ROMEND
#define input_ports_rct400 input_ports_rctycn
#define init_rct400 init_rctycn
CORE_CLONEDEFNV(rct400,rctycn,"RollerCoaster Tycoon (4.00)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct400l, "rctcpu.400",CRC(4691de23) SHA1(f47fc03f9b030cb7a1201d015bbec5a426176338))
DE_DMD32ROM8x(        "rctdsp-s.400",CRC(c06e6669) SHA1(af389ba74e62d7065b3bc7d5f771a3461704cbe3))
RCT_SND
SE_ROMEND
#define input_ports_rct400l input_ports_rctycn
#define init_rct400l init_rctycn
CORE_CLONEDEFNV(rct400l,rctycn,"RollerCoaster Tycoon (4.00 Spanish)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct400f, "rctcpu.400",CRC(4691de23) SHA1(f47fc03f9b030cb7a1201d015bbec5a426176338))
DE_DMD32ROM8x(        "rctdsp-f.400",CRC(5b41893b) SHA1(66930d0f5d6542b41f22995df55447ba0e435576))
RCT_SND
SE_ROMEND
#define input_ports_rct400f input_ports_rctycn
#define init_rct400f init_rctycn
CORE_CLONEDEFNV(rct400f,rctycn,"RollerCoaster Tycoon (4.00 French)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct400g, "rctcpu.400",CRC(4691de23) SHA1(f47fc03f9b030cb7a1201d015bbec5a426176338))
DE_DMD32ROM8x(        "rctdsp-g.400",CRC(c88dc915) SHA1(894c3bb7f5d200448740f021b5b9421f6ee74c5f))
RCT_SND
SE_ROMEND
#define input_ports_rct400g input_ports_rctycn
#define init_rct400g init_rctycn
CORE_CLONEDEFNV(rct400g,rctycn,"RollerCoaster Tycoon (4.00 German)",2002,"Stern",de_mSES1,0)

SE128_ROMSTART(rct400i, "rctcpu.400",CRC(4691de23) SHA1(f47fc03f9b030cb7a1201d015bbec5a426176338))
DE_DMD32ROM8x(        "rctdsp-i.400",CRC(7c1c0adb) SHA1(c9321a64ee4af8d475b441ae2385120bba66ccf9))
RCT_SND
SE_ROMEND
#define input_ports_rct400i input_ports_rctycn
#define init_rct400i init_rctycn
CORE_CLONEDEFNV(rct400i,rctycn,"RollerCoaster Tycoon (4.00 Italian)",2002,"Stern",de_mSES1,0)

/*-------------------------------------------------------------------
/ The Simpsons Pinball Party
/-------------------------------------------------------------------*/
#define SPP_SND \
DE2S_SOUNDROM18888("spp101.u7",   CRC(32efcdf6) SHA1(1d437e8649408be91e0dd10598cc67336203077f), \
                   "spp100.u17",  CRC(65e9344e) SHA1(fe4797ccb71b31aa39d6a5d373a01fc22f9d055c), \
                   "spp100.u21",  CRC(17fee0f9) SHA1(5b5ceb667f3bc9bde4ea08a1ef837e3b56c01977), \
                   "spp100.u36",  CRC(ffb957b0) SHA1(d6876ec63525099a7073c196867c17111272c69a), \
                   "spp100.u37",  CRC(0738e1fc) SHA1(268462c06e5c1f286e5faaee1c0815448cc2eafa))

static struct core_dispLayout dispSPP[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10,10,14, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)seminidmd4_update, NULL}, {0}
};
static core_tGameData simpprtyGameData = { \
  GEN_WS, dispSPP, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 4, 0, 0, SE_MINIDMD2}}; \
static void init_simpprty(void) { core_gameData = &simpprtyGameData; }
SE128_ROMSTART(simpprty, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispa.500",CRC(c6db83ec) SHA1(6079981e19b4651a074b0005eca85faf0eebcec0))
SPP_SND
SE_ROMEND
#define input_ports_simpprty input_ports_se
CORE_GAMEDEFNV(simpprty,"Simpsons Pinball Party, The (5.00)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simpprtg, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispg.500",CRC(6503bffc) SHA1(717aa8b7a0329c886ddb4b167c022b3a2ee3ab2d))
SPP_SND
SE_ROMEND
#define input_ports_simpprtg input_ports_simpprty
#define init_simpprtg init_simpprty
CORE_CLONEDEFNV(simpprtg,simpprty,"Simpsons Pinball Party, The (5.00 German)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simpprtl, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispl.500",CRC(0821f182) SHA1(7998ab29dae59d077b1dedd28a30a3477251d107))
SPP_SND
SE_ROMEND
#define input_ports_simpprtl input_ports_simpprty
#define init_simpprtl init_simpprty
CORE_CLONEDEFNV(simpprtl,simpprty,"Simpsons Pinball Party, The (5.00 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simpprtf, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispf.500",CRC(8d3383ed) SHA1(a56b1043fe1b0280d11386981fe9c181c9b6f1b7))
SPP_SND
SE_ROMEND
#define input_ports_simpprtf input_ports_simpprty
#define init_simpprtf init_simpprty
CORE_CLONEDEFNV(simpprtf,simpprty,"Simpsons Pinball Party, The (5.00 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simpprti, "spp-cpu.500", CRC(215ce09c) SHA1(f3baaaa1b9f12a98109da55746031eb9f5f8790c))
DE_DMD32ROM8x(           "sppdispi.500",CRC(eefe84db) SHA1(97c60f9182bdfe346ca4981b844a71f57414d470))
SPP_SND
SE_ROMEND
#define input_ports_simpprti input_ports_simpprty
#define init_simpprti init_simpprty
CORE_CLONEDEFNV(simpprti,simpprty,"Simpsons Pinball Party, The (5.00 Italian)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp400, "spp-cpu.400", CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspa.400",CRC(cd5eaab7) SHA1(a06bef6fc0e7f3c0616439cb0e0431a3d52cdfa1))
SPP_SND
SE_ROMEND
#define input_ports_simp400 input_ports_simpprty
#define init_simp400 init_simpprty
CORE_CLONEDEFNV(simp400,simpprty,"Simpsons Pinball Party, The (4.00)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp400g, "spp-cpu.400", CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspg.400",CRC(3b408fe2) SHA1(ce8d7f0d58b5f8fb4df0b9811449e4dc0e1e6580))
SPP_SND
SE_ROMEND
#define input_ports_simp400g input_ports_simpprty
#define init_simp400g init_simpprty
CORE_CLONEDEFNV(simp400g,simpprty,"Simpsons Pinball Party, The (4.00 German)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp400l, "spp-cpu.400", CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspl.400",CRC(a0bf567e) SHA1(ce6eb65da6bff15aeb787fd2cdac7cf6b4300108))
SPP_SND
SE_ROMEND
#define input_ports_simp400l input_ports_simpprty
#define init_simp400l init_simpprty
CORE_CLONEDEFNV(simp400l,simpprty,"Simpsons Pinball Party, The (4.00 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp400f, "spp-cpu.400", CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspf.400",CRC(6cc306e2) SHA1(bfe6ef0cd5d0cb5e3b29d85ade1700005e22d81b))
SPP_SND
SE_ROMEND
#define input_ports_simp400f input_ports_simpprty
#define init_simp400f init_simpprty
CORE_CLONEDEFNV(simp400f,simpprty,"Simpsons Pinball Party, The (4.00 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp400i, "spp-cpu.400", CRC(530b9782) SHA1(573b20cac205b7989cdefceb2c31cb7d88c2951a))
DE_DMD32ROM8x(           "sppdspi.400",CRC(ebe45dee) SHA1(4cdf0f01b1df1fa35df67f19c67b82a39d887be8))
SPP_SND
SE_ROMEND
#define input_ports_simp400i input_ports_simpprty
#define init_simp400i init_simpprty
CORE_CLONEDEFNV(simp400i,simpprty,"Simpsons Pinball Party, The (4.00 Italian)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp300, "spp-cpu.300", CRC(d9e02665) SHA1(12875c845c12b6676aa0af7c717fdf074156d938))
DE_DMD32ROM8x(           "sppdspa.300",CRC(57c4f297) SHA1(91ae894293b1252213a7137400f89c7ac2c6e877))
SPP_SND
SE_ROMEND
#define input_ports_simp300 input_ports_simpprty
#define init_simp300 init_simpprty
CORE_CLONEDEFNV(simp300,simpprty,"Simpsons Pinball Party, The (3.00)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp300l, "spp-cpu.300", CRC(d9e02665) SHA1(12875c845c12b6676aa0af7c717fdf074156d938))
DE_DMD32ROM8x(           "sppdspl.300",CRC(d91ec782) SHA1(a01ebecb03200738b47177b02a689148d822ff0e))
SPP_SND
SE_ROMEND
#define input_ports_simp300l input_ports_simpprty
#define init_simp300l init_simpprty
CORE_CLONEDEFNV(simp300l,simpprty,"Simpsons Pinball Party, The (3.00 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp300f, "spp-cpu.300", CRC(d9e02665) SHA1(12875c845c12b6676aa0af7c717fdf074156d938))
DE_DMD32ROM8x(           "sppdspf.300",CRC(cb848e0d) SHA1(ab9f32d3b693ebcef92fe21e04d760756c8f59c2))
SPP_SND
SE_ROMEND
#define input_ports_simp300f input_ports_simpprty
#define init_simp300f init_simpprty
CORE_CLONEDEFNV(simp300f,simpprty,"Simpsons Pinball Party, The (3.00 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp300i, "spp-cpu.300", CRC(d9e02665) SHA1(12875c845c12b6676aa0af7c717fdf074156d938))
DE_DMD32ROM8x(           "sppdspi.300",CRC(31acf30a) SHA1(aad2b363bed93d22613b0530fcd2d7f850f8e616))
SPP_SND
SE_ROMEND
#define input_ports_simp300i input_ports_simpprty
#define init_simp300i init_simpprty
CORE_CLONEDEFNV(simp300i,simpprty,"Simpsons Pinball Party, The (3.00 Italian)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp204, "spp-cpu.204", CRC(5bc155f7) SHA1(78d793cecbc6561a891ff8007f33c63ec5515e9f))
DE_DMD32ROM8x(           "sppdispa.201",CRC(f55505a4) SHA1(5616959caafc836d13db9c1a1e93cb4954f0c321))
SPP_SND
SE_ROMEND
#define input_ports_simp204 input_ports_simpprty
#define init_simp204 init_simpprty
CORE_CLONEDEFNV(simp204,simpprty,"Simpsons Pinball Party, The (2.04)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp204l, "spp-cpu.204", CRC(5bc155f7) SHA1(78d793cecbc6561a891ff8007f33c63ec5515e9f))
DE_DMD32ROM8x(           "sppdispl.201",CRC(78a67e23) SHA1(c4ef2b0301104280410aefdacc847e74a8c6a49f))
SPP_SND
SE_ROMEND
#define input_ports_simp204l input_ports_simpprty
#define init_simp204l init_simpprty
CORE_CLONEDEFNV(simp204l,simpprty,"Simpsons Pinball Party, The (2.04 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp204f, "spp-cpu.204", CRC(5bc155f7) SHA1(78d793cecbc6561a891ff8007f33c63ec5515e9f))
DE_DMD32ROM8x(           "sppdispf.201",CRC(d1c0c484) SHA1(615e3a8ba62b3f6d0ba53fbaf4b7d9e7fcdc9d82))
SPP_SND
SE_ROMEND
#define input_ports_simp204f input_ports_simpprty
#define init_simp204f init_simpprty
CORE_CLONEDEFNV(simp204f,simpprty,"Simpsons Pinball Party, The (2.04 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(simp204i, "spp-cpu.204", CRC(5bc155f7) SHA1(78d793cecbc6561a891ff8007f33c63ec5515e9f))
DE_DMD32ROM8x(           "sppdispi.201",CRC(b4594819) SHA1(4ab83f3b6466eebdec802e57d6542ad4a3cf3fb0))
SPP_SND
SE_ROMEND
#define input_ports_simp204i input_ports_simpprty
#define init_simp204i init_simpprty
CORE_CLONEDEFNV(simp204i,simpprty,"Simpsons Pinball Party, The (2.04 Italian)",2003,"Stern",de_mSES2,0)

/*-------------------------------------------------------------------
/ Terminator 3: Rise of the Machines
/-------------------------------------------------------------------*/
#define T3_SND \
DE2S_SOUNDROM18888("t3100.u7",  CRC(7f99e3af) SHA1(4916c074e2a4c947d1a658300f9f9629da1a8bb8), \
                   "t3100.u17", CRC(f0c71b5d) SHA1(e9f726a232fbd4f34b8b07069f337dbb3daf394a), \
                   "t3100.u21", CRC(694331f7) SHA1(e9ae8c5db2e59c0f9df923c98f6e75896e150807), \
                   "t3100.u36", CRC(9eb512e9) SHA1(fa2fecf6cb0c1af3c6db244f9d94ba53d13e10fc), \
                   "t3100.u37", CRC(3efb0c19) SHA1(6894295eef05891d64c7274512ba27f2b63ca3ec))

INITGAME(term3,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(term3, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(      "t3dispa.400", CRC(6b7cc4f8) SHA1(214e9b3e45b778841fc166acf4ff5fd634ae2670))
T3_SND
SE_ROMEND
#define input_ports_term3 input_ports_se
CORE_GAMEDEFNV(term3,"Terminator 3: Rise of the Machines (4.00)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3l, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3displ.400", CRC(2e21caba) SHA1(d29afa05d68484c762799c799bd1ccd1aad252b7))
T3_SND
SE_ROMEND
#define input_ports_term3l input_ports_se
#define init_term3l init_term3
CORE_CLONEDEFNV(term3l,term3,"Terminator 3: Rise of the Machines (4.00 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3g, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispg.400", CRC(20da21b2) SHA1(9115aef55d9ac36a49ae5c5fd423f05c669b0335))
T3_SND
SE_ROMEND
#define input_ports_term3g input_ports_se
#define init_term3g init_term3
CORE_CLONEDEFNV(term3g,term3,"Terminator 3: Rise of the Machines (4.00 German)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3f, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispf.400", CRC(0645fe6d) SHA1(1a7dfa160ba6cc1335a59b018289982f2a46a7bb))
T3_SND
SE_ROMEND
#define input_ports_term3f input_ports_se
#define init_term3f init_term3
CORE_CLONEDEFNV(term3f,term3,"Terminator 3: Rise of the Machines (4.00 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3i, "t3cpu.400", CRC(872f9351) SHA1(8fa8b503d8c3dbac66df1cb0ba400dbd58ee28ee))
DE_DMD32ROM8x(       "t3dispi.400", CRC(e8ea9ab8) SHA1(7b25bb7d3095e6bd2d94342d0e078590cb75074b))
T3_SND
SE_ROMEND
#define input_ports_term3i input_ports_se
#define init_term3i init_term3
CORE_CLONEDEFNV(term3i,term3,"Terminator 3: Rise of the Machines (4.00 Italian)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3_3, "t3cpu.301", CRC(172a0b83) SHA1(68f6a228182040a0ea6b310cb25d3d5bdd2574bf))
DE_DMD32ROM8x(       "t3dispa.300", CRC(79b68a2f) SHA1(cd466c15ffe09666c115f843775e457138bf23bc))
T3_SND
SE_ROMEND
#define input_ports_term3_3 input_ports_se
#define init_term3_3 init_term3
CORE_CLONEDEFNV(term3_3,term3,"Terminator 3: Rise of the Machines (3.01)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3l_3, "t3cpu.301", CRC(172a0b83) SHA1(68f6a228182040a0ea6b310cb25d3d5bdd2574bf))
DE_DMD32ROM8x(       "t3displ.300", CRC(2df35b3f) SHA1(5716b46c16cc7c4478f3118c4e6c3959b10624f8))
T3_SND
SE_ROMEND
#define input_ports_term3l_3 input_ports_se
#define init_term3l_3 init_term3
CORE_CLONEDEFNV(term3l_3,term3,"Terminator 3: Rise of the Machines (3.01 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3f_3, "t3cpu.301", CRC(172a0b83) SHA1(68f6a228182040a0ea6b310cb25d3d5bdd2574bf))
DE_DMD32ROM8x(       "t3dispf.300", CRC(d5c68903) SHA1(00ca09f087e5b2a742d0bf6f2ff5706a2b83a295))
T3_SND
SE_ROMEND
#define input_ports_term3f_3 input_ports_se
#define init_term3f_3 init_term3
CORE_CLONEDEFNV(term3f_3,term3,"Terminator 3: Rise of the Machines (3.01 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3g_3, "t3cpu.301", CRC(172a0b83) SHA1(68f6a228182040a0ea6b310cb25d3d5bdd2574bf))
DE_DMD32ROM8x(       "t3dispg.300", CRC(9115ea52) SHA1(52bd2cbe609363d9904b82704072fc3c398a7c18))
T3_SND
SE_ROMEND
#define input_ports_term3g_3 input_ports_se
#define init_term3g_3 init_term3
CORE_CLONEDEFNV(term3g_3,term3,"Terminator 3: Rise of the Machines (3.01 German)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3i_3, "t3cpu.301", CRC(172a0b83) SHA1(68f6a228182040a0ea6b310cb25d3d5bdd2574bf))
DE_DMD32ROM8x(       "t3dispi.300", CRC(30573629) SHA1(85ae7183b42a62f62aa3ba6441717fc7a49dd03a))
T3_SND
SE_ROMEND
#define input_ports_term3i_3 input_ports_se
#define init_term3i_3 init_term3
CORE_CLONEDEFNV(term3i_3,term3,"Terminator 3: Rise of the Machines (3.01 Italian)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3_2, "t3cpu.205", CRC(219d8155) SHA1(dac3c390d238872703a0ec65cb3dddc89630359c))
DE_DMD32ROM8x(       "t3dispa.201", CRC(a314acd1) SHA1(4d5072e65f8041d24c1bab2985ef5b30e1895bf3))
T3_SND
SE_ROMEND
#define input_ports_term3_2 input_ports_se
#define init_term3_2 init_term3
CORE_CLONEDEFNV(term3_2,term3,"Terminator 3: Rise of the Machines (2.05)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3l_2, "t3cpu.205", CRC(219d8155) SHA1(dac3c390d238872703a0ec65cb3dddc89630359c))
DE_DMD32ROM8x(       "t3displ.201", CRC(180b55a2) SHA1(1d8161fc806804e0712ee8a07a2cac0561949f0c))
T3_SND
SE_ROMEND
#define input_ports_term3l_2 input_ports_se
#define init_term3l_2 init_term3
CORE_CLONEDEFNV(term3l_2,term3,"Terminator 3: Rise of the Machines (2.05 Spanish)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3f_2, "t3cpu.205", CRC(219d8155) SHA1(dac3c390d238872703a0ec65cb3dddc89630359c))
DE_DMD32ROM8x(       "t3dispf.201", CRC(ced87154) SHA1(893c071bb2427429ca45f4d2088b015c5f638207))
T3_SND
SE_ROMEND
#define input_ports_term3f_2 input_ports_se
#define init_term3f_2 init_term3
CORE_CLONEDEFNV(term3f_2,term3,"Terminator 3: Rise of the Machines (2.05 French)",2003,"Stern",de_mSES2,0)

SE128_ROMSTART(term3i_2, "t3cpu.205", CRC(219d8155) SHA1(dac3c390d238872703a0ec65cb3dddc89630359c))
DE_DMD32ROM8x(       "t3dispi.201", CRC(c1f3604f) SHA1(8a391e6471ced52662aa69261ac29a279c7b8a7d))
T3_SND
SE_ROMEND
#define input_ports_term3i_2 input_ports_se
#define init_term3i_2 init_term3
CORE_CLONEDEFNV(term3i_2,term3,"Terminator 3: Rise of the Machines (2.05 Italian)",2003,"Stern",de_mSES2,0)

// All games below now using a new CPU/Sound board (520-5300-00) with an Atmel AT91 (ARM7DMI Variant) CPU for sound

/*-------------------------------------------------------------------
/ The Lord of the Rings
/-------------------------------------------------------------------*/
#define LOTR_SND \
DE3S_SOUNDROM18888("lotr-u7.101", CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470), \
                   "lotr-u17.100",CRC(d503969d) SHA1(65a4313ca1b93391260c65fef6f878d264f9c8ab), \
                   "lotr-u21.100",CRC(8b3f41af) SHA1(9b2d04144edeb499b4ae68a97c65ccb8ef4d26c0), \
                   "lotr-u36.100",CRC(9575981e) SHA1(38083fd923c4a168a94d998ec3c4db42c1e2a2da), \
                   "lotr-u37.100",CRC(8e637a6f) SHA1(8087744ce36fc143381d49a312c98cf38b2f9854))
                   
#define LOTR_SND_SP \
DE3S_SOUNDROM18888("lotrlu7.100", CRC(980d970a) SHA1(cf70deddcc146ef9eaa64baec74ae800bebf8715), \
                   "lotrlu17.100",CRC(c16d3e20) SHA1(43d9f186db361abb3aa119a7252f1bb13bbbbe39), \
                   "lotrlu21.100",CRC(f956a1be) SHA1(2980e85463704a154ed81d3241f866442d1ea2e6), \
                   "lotrlu36.100",CRC(502c3d93) SHA1(4ee42d70bccb9253fe1f8f441de6b5018257f107), \
                   "lotrlu37.100",CRC(61f21c6d) SHA1(3e9781b4981bd18cdb8c59c55b9942de6ae286db))

static core_tGameData lotrGameData = { \
  GEN_WS, se_dmd128x32, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, 5, 0, 0, SE_LED}}; \
static void init_lotr(void) { core_gameData = &lotrGameData; }
SE128_ROMSTART(lotr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(      "lotrdspa.a00", CRC(99634603) SHA1(c40d1480e5df10a491bcd471c6a3a118a9120bcb))
LOTR_SND
SE_ROMEND
#define input_ports_lotr input_ports_se
CORE_GAMEDEFNV(lotr,"Lord of the Rings, The (10.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp, "lotrcpul.a00", CRC(c62aba47) SHA1(2fef599313e5cd9bded3ab00b933631586e2a1e7))
DE_DMD32ROM8x(          "lotrdspl.a00", CRC(2494a5ee) SHA1(5b95711858d88eeb445503cac8b9b754cf8e9960))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp input_ports_lotr
#define init_lotr_sp init_lotr
CORE_CLONEDEFNV(lotr_sp,lotr,"Lord of the Rings, The (10.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspg.a00", CRC(6743a910) SHA1(977773515f00af3937aa59426917e8111ec855ab))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr input_ports_lotr
#define init_lotr_gr init_lotr
CORE_CLONEDEFNV(lotr_gr,lotr,"Lord of the Rings, The (10.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspf.a00", CRC(15c26c2d) SHA1(c8e4b442d717aa5881f3d92f044c44d29a14126c))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr input_ports_lotr
#define init_lotr_fr init_lotr
CORE_CLONEDEFNV(lotr_fr,lotr,"Lord of the Rings, The (10.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it, "lotrcpua.a00", CRC(00e43b70) SHA1(7100da644a1b166051915870d01cfa6baaf87293))
DE_DMD32ROM8x(         "lotrdspi.a00", CRC(6c88f395) SHA1(365d5c6908f5861816b73f287194c85d2300635d))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it input_ports_lotr
#define init_lotr_it init_lotr
CORE_CLONEDEFNV(lotr_it,lotr,"Lord of the Rings, The (10.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr9, "lotrcpu.900", CRC(eff1ab83) SHA1(cd9cfa3fa150224e44078602db7d8bbfe223b926))
DE_DMD32ROM8x(          "lotrdspa.900", CRC(2b1debd3) SHA1(eab1ffa7b5111bf224c47688bb6c0f40ee6e12fb))
LOTR_SND
SE_ROMEND
#define input_ports_lotr9 input_ports_lotr
#define init_lotr9 init_lotr
CORE_CLONEDEFNV(lotr9,lotr,"Lord of the Rings, The (9.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp9, "lotrcpul.900", CRC(155b5d5b) SHA1(c032e3828ed256240a5155ec4c7820d615a2cbe1))
DE_DMD32ROM8x(          "lotrdspl.900", CRC(00f98242) SHA1(9a0e7e572e209b20691392a694a524192daa0d2a))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp9 input_ports_lotr
#define init_lotr_sp9 init_lotr
CORE_CLONEDEFNV(lotr_sp9,lotr,"Lord of the Rings, The (9.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr9, "lotrcpu.900", CRC(eff1ab83) SHA1(cd9cfa3fa150224e44078602db7d8bbfe223b926))
DE_DMD32ROM8x(          "lotrdspg.900", CRC(f5fdd2c2) SHA1(0c5f1b1efe3d38063e2327e2ccfe40936f3988b8))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr9 input_ports_lotr
#define init_lotr_gr9 init_lotr
CORE_CLONEDEFNV(lotr_gr9,lotr,"Lord of the Rings, The (9.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr9, "lotrcpu.900", CRC(eff1ab83) SHA1(cd9cfa3fa150224e44078602db7d8bbfe223b926))
DE_DMD32ROM8x( "lotrdspf.900", CRC(f2d8296e) SHA1(3eb6e1e6ba299b720816bf165b1e20e02f6c0c1e))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr9 input_ports_lotr
#define init_lotr_fr9 init_lotr
CORE_CLONEDEFNV(lotr_fr9,lotr,"Lord of the Rings, The (9.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it9, "lotrcpu.900", CRC(eff1ab83) SHA1(cd9cfa3fa150224e44078602db7d8bbfe223b926))
DE_DMD32ROM8x(          "lotrdspi.900", CRC(a09407d7) SHA1(2cdb70ee0bae7f67f4bf12b0dd3e6cf574087e3d))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it9 input_ports_lotr
#define init_lotr_it9 init_lotr
CORE_CLONEDEFNV(lotr_it9,lotr,"Lord of the Rings, The (9.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr8, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(          "lotrdspa.800", CRC(2aa1f00d) SHA1(e9df5b61b467c307aacdb5a6980a78af26492e6a))
LOTR_SND
SE_ROMEND
#define input_ports_lotr8 input_ports_lotr
#define init_lotr8 init_lotr
CORE_CLONEDEFNV(lotr8,lotr,"Lord of the Rings, The (8.00)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp8, "lotrcpul.800", CRC(38e83068) SHA1(603b1236bf195c72d32e5a1088a2806e77176436))
DE_DMD32ROM8x(          "lotrdspl.800", CRC(137c1255) SHA1(43d9ffec18ab2aa80f30b195ca5270d4574d7b8d))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp8 input_ports_lotr
#define init_lotr_sp8 init_lotr
CORE_CLONEDEFNV(lotr_sp8,lotr,"Lord of the Rings, The (8.00 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr8, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(          "lotrdspg.800", CRC(55765c23) SHA1(690a72e8cb1099a6873eb3214e72bb0fea54fa22))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr8 input_ports_lotr
#define init_lotr_gr8 init_lotr
CORE_CLONEDEFNV(lotr_gr8,lotr,"Lord of the Rings, The (8.00 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr8, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(          "lotrdspf.800", CRC(e1ccc04b) SHA1(1d5c7ea06f0cb2e1965c968ed01330867aae8e2b))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr8 input_ports_lotr
#define init_lotr_fr8 init_lotr
CORE_CLONEDEFNV(lotr_fr8,lotr,"Lord of the Rings, The (8.00 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it8, "lotrcpu.800", CRC(655e5b3c) SHA1(bd6fd25e17cee40d6bb842367b1ce922bbd46003))
DE_DMD32ROM8x(          "lotrdspi.800", CRC(b80730d7) SHA1(552c0bfac1c7a6b246829378a30d58769e695f7e))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it8 input_ports_lotr
#define init_lotr_it8 init_lotr
CORE_CLONEDEFNV(lotr_it8,lotr,"Lord of the Rings, The (8.00 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr7, "lotrcpu.700", CRC(208a4653) SHA1(570f3070c6b0f128d451f3dea01e41a9944081f2))
DE_DMD32ROM8x(          "lotrdspa.700", CRC(233ef0ad) SHA1(1564ae806639dac49add0c464f4499f46b5589ab))
LOTR_SND
SE_ROMEND
#define input_ports_lotr7 input_ports_lotr
#define init_lotr7 init_lotr
CORE_CLONEDEFNV(lotr7,lotr,"Lord of the Rings, The (7.00)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp7, "lotrcpul.700", CRC(58d8611b) SHA1(ff1e1668993c7c31f04efc22f04ae53112219a1d))
DE_DMD32ROM8x(          "lotrdspl.700", CRC(3be0283d) SHA1(e019c69cd452b67d6427ddda12b5c3f341afb414))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp7 input_ports_lotr
#define init_lotr_sp7 init_lotr
CORE_CLONEDEFNV(lotr_sp7,lotr,"Lord of the Rings, The (7.00 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr7, "lotrcpu.700", CRC(208a4653) SHA1(570f3070c6b0f128d451f3dea01e41a9944081f2))
DE_DMD32ROM8x(          "lotrdspg.700", CRC(137f223c) SHA1(bb06a6f587bf86555aea85bc1c0402e2137e1c76))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr7 input_ports_lotr
#define init_lotr_gr7 init_lotr
CORE_CLONEDEFNV(lotr_gr7,lotr,"Lord of the Rings, The (7.00 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr7, "lotrcpu.700", CRC(208a4653) SHA1(570f3070c6b0f128d451f3dea01e41a9944081f2))
DE_DMD32ROM8x(          "lotrdspf.700", CRC(c98aeb30) SHA1(16b0ae41db8b4083121cc5ebf2706320d554dd08))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr7 input_ports_lotr
#define init_lotr_fr7 init_lotr
CORE_CLONEDEFNV(lotr_fr7,lotr,"Lord of the Rings, The (7.00 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it7, "lotrcpu.700", CRC(208a4653) SHA1(570f3070c6b0f128d451f3dea01e41a9944081f2))
DE_DMD32ROM8x(          "lotrdspi.700", CRC(6a0d2a6d) SHA1(2b3ca8b26d79919b7102c60515972ab142d1cbf1))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it7 input_ports_lotr
#define init_lotr_it7 init_lotr
CORE_CLONEDEFNV(lotr_it7,lotr,"Lord of the Rings, The (7.00 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr6, "lotrcpu.600", CRC(02786892) SHA1(6810d5a5eb80f520e611a46921dbd2906fbebf2f))
DE_DMD32ROM8x(          "lotrdspa.600", CRC(d2098cec) SHA1(06c5c0b29e1442f503b4b374537b9d233721b4b6))
LOTR_SND
SE_ROMEND
#define input_ports_lotr6 input_ports_lotr
#define init_lotr6 init_lotr
CORE_CLONEDEFNV(lotr6,lotr,"Lord of the Rings, The (6.00)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp6, "lotrcpul.600", CRC(af06c560) SHA1(0dec564e8e50ca8e05c462517db38ae48e512e79))
DE_DMD32ROM8x(          "lotrdspl.600", CRC(d664d989) SHA1(575f9dcc7cb8aac4bfdb25575b7b9c00cf6459b9))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp6 input_ports_lotr
#define init_lotr_sp6 init_lotr
CORE_CLONEDEFNV(lotr_sp6,lotr,"Lord of the Rings, The (6.00 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr6, "lotrcpu.600", CRC(02786892) SHA1(6810d5a5eb80f520e611a46921dbd2906fbebf2f))
DE_DMD32ROM8x(          "lotrdspg.600", CRC(b0de0827) SHA1(1a0aa25a3b881148aafa5e2fabb7a3c501343524))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr6 input_ports_lotr
#define init_lotr_gr6 init_lotr
CORE_CLONEDEFNV(lotr_gr6,lotr,"Lord of the Rings, The (6.00 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr6, "lotrcpu.600", CRC(02786892) SHA1(6810d5a5eb80f520e611a46921dbd2906fbebf2f))
DE_DMD32ROM8x(          "lotrdspf.600", CRC(5cf6c0b6) SHA1(d7fde5dda4c48da15b682ed9f52d20d8ea2accc9))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr6 input_ports_lotr
#define init_lotr_fr6 init_lotr
CORE_CLONEDEFNV(lotr_fr6,lotr,"Lord of the Rings, The (6.00 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it6, "lotrcpu.600", CRC(02786892) SHA1(6810d5a5eb80f520e611a46921dbd2906fbebf2f))
DE_DMD32ROM8x(          "lotrdspi.600", CRC(9d0b9b3d) SHA1(21363ddbb2c2510fcc9386020f2fd3f49e9c49c3))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it6 input_ports_lotr
#define init_lotr_it6 init_lotr
CORE_CLONEDEFNV(lotr_it6,lotr,"Lord of the Rings, The (6.00 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr51, "lotrcpu.501", CRC(df9fd692) SHA1(2d06c1a763330b1b9429961f3e13574e0eefe7a7))
DE_DMD32ROM8x(          "lotrdspa.501", CRC(2d555b9f) SHA1(d2d23182dea810624cab010890971d8997f8570c))
LOTR_SND
SE_ROMEND
#define input_ports_lotr51 input_ports_lotr
#define init_lotr51 init_lotr
CORE_CLONEDEFNV(lotr51,lotr,"Lord of the Rings, The (5.01)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_s51, "lotrcpul.501", CRC(4c0c7360) SHA1(f057931eb719a7a6691187fa7ca86cd6e4541d90))
DE_DMD32ROM8x(          "lotrdspl.501", CRC(7e96c0f8) SHA1(2ef63b1b30fb2680b97a9080f7b9d76b4d2a76d4))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_s51 input_ports_lotr
#define init_lotr_s51 init_lotr
CORE_CLONEDEFNV(lotr_s51,lotr,"Lord of the Rings, The (5.01 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_g51, "lotrcpu.501", CRC(df9fd692) SHA1(2d06c1a763330b1b9429961f3e13574e0eefe7a7))
DE_DMD32ROM8x(          "lotrdspg.501", CRC(16984eaa) SHA1(999254d12402b0866e4a6f5bb2c03dc5c1c59c5f))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_g51 input_ports_lotr
#define init_lotr_g51 init_lotr
CORE_CLONEDEFNV(lotr_g51,lotr,"Lord of the Rings, The (5.01 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_f51, "lotrcpu.501", CRC(df9fd692) SHA1(2d06c1a763330b1b9429961f3e13574e0eefe7a7))
DE_DMD32ROM8x(          "lotrdspf.501", CRC(a07596ac) SHA1(99db750971eafc3dba5f5d3e15728de306984ba4))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_f51 input_ports_lotr
#define init_lotr_f51 init_lotr
CORE_CLONEDEFNV(lotr_f51,lotr,"Lord of the Rings, The (5.01 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_i51, "lotrcpu.501", CRC(df9fd692) SHA1(2d06c1a763330b1b9429961f3e13574e0eefe7a7))
DE_DMD32ROM8x(          "lotrdspi.501", CRC(440bbba1) SHA1(e85eca9a1b04ba8bc2784414f7003674bcafba9d))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_i51 input_ports_lotr
#define init_lotr_i51 init_lotr
CORE_CLONEDEFNV(lotr_i51,lotr,"Lord of the Rings, The (5.01 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr5, "lotrcpu.500", CRC(4b45a543) SHA1(e0be3a4244025abc2c109f58b0d637262711b9db))
DE_DMD32ROM8x(          "lotrdspa.500", CRC(19bda8d2) SHA1(a3d9e60f964d100594f82ed361f86e74c8d69748))
LOTR_SND
SE_ROMEND
#define input_ports_lotr5 input_ports_lotr
#define init_lotr5 init_lotr
CORE_CLONEDEFNV(lotr5,lotr,"Lord of the Rings, The (5.00)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp5, "lotrcpul.500", CRC(298d10bd) SHA1(133990aed459d5fcd191a08462a231bbd3449387))
DE_DMD32ROM8x(          "lotrdspl.500", CRC(ab538b24) SHA1(039aa8f4286694971cd9a78805bb9f3acabcd692))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp5 input_ports_lotr
#define init_lotr_sp5 init_lotr
CORE_CLONEDEFNV(lotr_sp5,lotr,"Lord of the Rings, The (5.00 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr5, "lotrcpu.500", CRC(4b45a543) SHA1(e0be3a4244025abc2c109f58b0d637262711b9db))
DE_DMD32ROM8x(          "lotrdspg.500", CRC(39177315) SHA1(13bcf2833ff89fe056517d3ea7b58fb31963cbfc))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr5 input_ports_lotr
#define init_lotr_gr5 init_lotr
CORE_CLONEDEFNV(lotr_gr5,lotr,"Lord of the Rings, The (5.00 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr5, "lotrcpu.500", CRC(4b45a543) SHA1(e0be3a4244025abc2c109f58b0d637262711b9db))
DE_DMD32ROM8x(          "lotrdspf.500", CRC(ee5768a1) SHA1(05e696bfc4a7630b483f2f9acd39e53fefe937ef))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr5 input_ports_lotr
#define init_lotr_fr5 init_lotr
CORE_CLONEDEFNV(lotr_fr5,lotr,"Lord of the Rings, The (5.00 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it5, "lotrcpu.500", CRC(4b45a543) SHA1(e0be3a4244025abc2c109f58b0d637262711b9db))
DE_DMD32ROM8x(          "lotrdspi.500", CRC(ea8d4ac6) SHA1(f46e8c3f344babc67e72f7077880c21df0c42030))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it5 input_ports_lotr
#define init_lotr_it5 init_lotr
CORE_CLONEDEFNV(lotr_it5,lotr,"Lord of the Rings, The (5.00 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr41, "lotrcpu.410", CRC(458af09a) SHA1(2680f16d7f33ffc70b64bfb7d35cccf6989c70e2))
DE_DMD32ROM8x(          "lotrdspa.404", CRC(1aefcbe7) SHA1(b17fc82425dd5a6ea5a17205d4000294324bb5cc))
LOTR_SND
SE_ROMEND
#define input_ports_lotr41 input_ports_lotr
#define init_lotr41 init_lotr
CORE_CLONEDEFNV(lotr41,lotr,"Lord of the Rings, The (4.10)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_g41, "lotrcpu.410", CRC(458af09a) SHA1(2680f16d7f33ffc70b64bfb7d35cccf6989c70e2))
DE_DMD32ROM8x(          "lotrdspg.404", CRC(b78975e5) SHA1(33d9f4d29a83ce0f68e654c15973dfdeee4d224d))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_g41 input_ports_lotr
#define init_lotr_g41 init_lotr
CORE_CLONEDEFNV(lotr_g41,lotr,"Lord of the Rings, The (4.10 German)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_f41, "lotrcpu.410", CRC(458af09a) SHA1(2680f16d7f33ffc70b64bfb7d35cccf6989c70e2))
DE_DMD32ROM8x(          "lotrdspf.404", CRC(ebf4bb43) SHA1(5e392c3363db3d56b2ec66fcc43a59b5e8cdf944))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_f41 input_ports_lotr
#define init_lotr_f41 init_lotr
CORE_CLONEDEFNV(lotr_f41,lotr,"Lord of the Rings, The (4.10 French)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_i41, "lotrcpu.410", CRC(458af09a) SHA1(2680f16d7f33ffc70b64bfb7d35cccf6989c70e2))
DE_DMD32ROM8x(          "lotrdspi.404", CRC(05db2615) SHA1(0146abd3681d351ef6c1160b85be8bed2886fb27))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_i41 input_ports_lotr
#define init_lotr_i41 init_lotr
CORE_CLONEDEFNV(lotr_i41,lotr,"Lord of the Rings, The (4.10 Italian)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr4, "lotrcpu.401", CRC(b69cdecc) SHA1(40f7c2d25a1028255be8fe25e3aa6d11976edd25))
DE_DMD32ROM8x(          "lotrdspa.403", CRC(2630cef1) SHA1(1dfd929e7eb57983f2fd9184d471f2e919359de0))
LOTR_SND
SE_ROMEND
#define input_ports_lotr4 input_ports_lotr
#define init_lotr4 init_lotr
CORE_CLONEDEFNV(lotr4,lotr,"Lord of the Rings, The (4.01)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_sp4, "lotrcpul.401", CRC(a9571728) SHA1(f21dd77003f42fafd9293fab3a077c5abf6d572a))
DE_DMD32ROM8x(          "lotrdspl.403", CRC(6d4075c9) SHA1(7944ba597cb476c33060cead4feaf6dcad4f4b16))
LOTR_SND_SP
SE_ROMEND
#define input_ports_lotr_sp4 input_ports_lotr
#define init_lotr_sp4 init_lotr
CORE_CLONEDEFNV(lotr_sp4,lotr,"Lord of the Rings, The (4.01 Spanish)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_gr4, "lotrcpu.401", CRC(b69cdecc) SHA1(40f7c2d25a1028255be8fe25e3aa6d11976edd25))
DE_DMD32ROM8x(          "lotrdspg.403", CRC(74e925cb) SHA1(2edc8666d53f212a053b7a356d2bf6e3180d7bfb))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_gr4 input_ports_lotr
#define init_lotr_gr4 init_lotr
CORE_CLONEDEFNV(lotr_gr4,lotr,"Lord of the Rings, The (4.01 German)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_fr4, "lotrcpu.401", CRC(b69cdecc) SHA1(40f7c2d25a1028255be8fe25e3aa6d11976edd25))
DE_DMD32ROM8x(          "lotrdspf.403", CRC(d02a77cf) SHA1(8cf4312a04ad486714de5c0041cacb1eb475478f))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_fr4 input_ports_lotr
#define init_lotr_fr4 init_lotr
CORE_CLONEDEFNV(lotr_fr4,lotr,"Lord of the Rings, The (4.01 French)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_it4, "lotrcpu.401", CRC(b69cdecc) SHA1(40f7c2d25a1028255be8fe25e3aa6d11976edd25))
DE_DMD32ROM8x(          "lotrdspi.403", CRC(5922ce10) SHA1(c57f2de4e3344f16056405d71510c0c0b60ef86d))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_it4 input_ports_lotr
#define init_lotr_it4 init_lotr
CORE_CLONEDEFNV(lotr_it4,lotr,"Lord of the Rings, The (4.01 Italian)",2003,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr_le, "lotrcpua.a02", CRC(f732aeb1) SHA1(8c28e91d37525b3e356bddf2cf1be42ea44f3629))
DE_DMD32ROM8x(      "lotrdspa.a00", CRC(99634603) SHA1(c40d1480e5df10a491bcd471c6a3a118a9120bcb))
LOTR_SND
SE_ROMEND
#define input_ports_lotr_le input_ports_lotr
#define init_lotr_le init_lotr
CORE_CLONEDEFNV(lotr_le,lotr,"Lord of the Rings, The (10.02 Limited Edition)",2008,"Stern",de_mSES3,0)

SE128_ROMSTART(lotr3, "lotrcpu.300", CRC(fdc5351d) SHA1(92d58bcdd21026d278b5534d1a6ab299f6fffc60))
DE_DMD32ROM8x(          "lotrdspa.300", CRC(522e3e33) SHA1(11987872604e2a3e2c9567f8f9313c36e6c08cc7))
LOTR_SND
SE_ROMEND
#define input_ports_lotr3 input_ports_lotr
#define init_lotr3 init_lotr
CORE_CLONEDEFNV(lotr3,lotr,"Lord of the Rings, The (3.00)",2003,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Ripley's Believe It or Not!
/-------------------------------------------------------------------*/
#define RBION_SND \
DE3S_SOUNDROM18888("ripsnd.u7",  CRC(4573a759) SHA1(189c1a2eaf9d92c40a1bc145f59ac428c74a7318), \
                   "ripsnd.u17", CRC(d518f2da) SHA1(e7d75c6b7b45571ae6d39ed7405b1457e475b52a), \
                   "ripsnd.u21", CRC(3d8680d7) SHA1(1368965106094d78be6540eb87a478f853ba774f), \
                   "ripsnd.u36", CRC(b697b5cb) SHA1(b5cb426201287a6d1c40db8c81a58e2c656d1d81), \
                   "ripsnd.u37", CRC(01b9f20e) SHA1(cffb6a0136d7d17ab4450b3bfd97632d8b669d39))

#define RBION_SND_SP \
DE3S_SOUNDROM18888("ripsndl.u7", CRC(25fb729a) SHA1(46b9ca8fd5fb5a692adbdb7495af34a1db89dc37), \
                   "ripsndl.u17",CRC(a98f4514) SHA1(e87ee8f5a87a8ae9ec996473bf9bc745105ea334), \
                   "ripsndl.u21",CRC(141f2b77) SHA1(15bab623beda8ae7ed9908f492ff2baab0a7954e), \
                   "ripsndl.u36",CRC(c5461b63) SHA1(fc574d44ad88ce1db590ea371225092c03fc6f80), \
                   "ripsndl.u37",CRC(2a58f491) SHA1(1c33f419420b3165ef18598560007ef612b24814))

#define RBION_SND_GR \
DE3S_SOUNDROM18888("ripsndg.u7", CRC(400b8a45) SHA1(62101995e632264df3c014b746cc4b2ae72676d4), \
                   "ripsndg.u17",CRC(c387dcf0) SHA1(d4ef65d3f33ab82b63bf2782f335858ab4ad210a), \
                   "ripsndg.u21",CRC(6388ae8d) SHA1(a39c7977194daabf3f5b10d0269dcd4118a939bc), \
                   "ripsndg.u36",CRC(3143f9d3) SHA1(bd4ce64b245b5fcb9b9694bd8f71a9cd98303cae), \
                   "ripsndg.u37",CRC(2167617b) SHA1(62b55a39e2677eec9d56b10e8cc3e5d7c0d3bea5))

#define RBION_SND_FR \
DE3S_SOUNDROM18888("ripsndf.u7", CRC(5808e3fc) SHA1(0c83399e8dc846607c469b7dd95878f3c2b9cb82), \
                   "ripsndf.u17",CRC(a6793b85) SHA1(96058777346be6e9ea7b1340d9aaf945ac3c853a), \
                   "ripsndf.u21",CRC(60c02170) SHA1(900d9de3ccb541019e5f1528e01c57ad96dac262), \
                   "ripsndf.u36",CRC(0a57f2fd) SHA1(9dd057888294ee8abeb582e8f6650fd6e32cc9ff), \
                   "ripsndf.u37",CRC(5c858958) SHA1(f4a9833b8aee033ed381e3bdf9f801b935d6186a))

#define RBION_SND_IT \
DE3S_SOUNDROM18888("ripsndi.u7", CRC(86b1b2b2) SHA1(9e2cf7368b31531998d546a1be2af274a9cbbd2f), \
                   "ripsndi.u17",CRC(a2911df4) SHA1(acb7956a6a30142c8da905b04778a074cb335807), \
                   "ripsndi.u21",CRC(1467eaff) SHA1(c6c4ea2abdad4334efbe3a084693e9e4d0dd0fd2), \
                   "ripsndi.u36",CRC(6a124fa6) SHA1(752c3d227b9a98dd859e4778ddd527edaa3cf512), \
                   "ripsndi.u37",CRC(7933c102) SHA1(f736ee86d7c67dab82c634d125d73a2453249706))

static struct core_dispLayout dispBION[] = {
  DISP_SEG_IMPORT(se_dmd128x32),
  {34,10, 7, 5, CORE_DMD | CORE_NODISP, (genf *)seminidmd1s_update},
  {34,18, 7, 5, CORE_DMD | CORE_NODISP, (genf *)seminidmd1s_update},
  {34,26, 7, 5, CORE_DMD | CORE_NODISP, (genf *)seminidmd1s_update}, {0}
};
INITGAME(ripleys,GEN_WS,dispBION,SE_MINIDMD3)
SE128_ROMSTART(ripleys, "ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
RBION_SND
SE_ROMEND
#define input_ports_ripleys input_ports_se
CORE_GAMEDEFNV(ripleys,"Ripley's Believe It or Not! (3.20)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(ripleysf,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispf.301",CRC(e5ae9d99) SHA1(74929b324b457d08a925c641430e6a7036c7039d))
RBION_SND_FR
SE_ROMEND
#define input_ports_ripleysf input_ports_ripleys
#define init_ripleysf init_ripleys
CORE_CLONEDEFNV(ripleysf,ripleys,"Ripley's Believe It or Not! (3.20 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(ripleysg,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
RBION_SND_GR
SE_ROMEND
#define input_ports_ripleysg input_ports_ripleys
#define init_ripleysg init_ripleys
CORE_CLONEDEFNV(ripleysg,ripleys,"Ripley's Believe It or Not! (3.20 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(ripleysi,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
RBION_SND_IT
SE_ROMEND
#define input_ports_ripleysi input_ports_ripleys
#define init_ripleysi init_ripleys
CORE_CLONEDEFNV(ripleysi,ripleys,"Ripley's Believe It or Not! (3.20 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(ripleysl,"ripcpu.320",CRC(aa997826) SHA1(2f9701370e64dd55a9bafe0c65e7eb4b9c5dbdd2))
DE_DMD32ROM8x(        "ripdispl.301",CRC(47c87ad4) SHA1(eb372b9f17b28d0781c49a28cb850916ccec323d))
RBION_SND_SP
SE_ROMEND
#define input_ports_ripleysl input_ports_ripleys
#define init_ripleysl init_ripleys
CORE_CLONEDEFNV(ripleysl,ripleys,"Ripley's Believe It or Not! (3.20 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip310, "ripcpu.310",CRC(669f87cc) SHA1(0e07bbcf337bf7c289a9093d3db805da617cbfef))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
RBION_SND
SE_ROMEND
#define input_ports_rip310 input_ports_ripleys
#define init_rip310 init_ripleys
CORE_CLONEDEFNV(rip310,ripleys,"Ripley's Believe It or Not! (3.10)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip310f,"ripcpu.310",CRC(669f87cc) SHA1(0e07bbcf337bf7c289a9093d3db805da617cbfef))
DE_DMD32ROM8x(        "ripdispf.301",CRC(e5ae9d99) SHA1(74929b324b457d08a925c641430e6a7036c7039d))
RBION_SND_FR
SE_ROMEND
#define input_ports_rip310f input_ports_ripleys
#define init_rip310f init_ripleys
CORE_CLONEDEFNV(rip310f,ripleys,"Ripley's Believe It or Not! (3.10 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip310g,"ripcpu.310",CRC(669f87cc) SHA1(0e07bbcf337bf7c289a9093d3db805da617cbfef))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
RBION_SND_GR
SE_ROMEND
#define input_ports_rip310g input_ports_ripleys
#define init_rip310g init_ripleys
CORE_CLONEDEFNV(rip310g,ripleys,"Ripley's Believe It or Not! (3.10 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip310i,"ripcpu.310",CRC(669f87cc) SHA1(0e07bbcf337bf7c289a9093d3db805da617cbfef))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
RBION_SND_IT
SE_ROMEND
#define input_ports_rip310i input_ports_ripleys
#define init_rip310i init_ripleys
CORE_CLONEDEFNV(rip310i,ripleys,"Ripley's Believe It or Not! (3.10 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip310l,"ripcpu.310",CRC(669f87cc) SHA1(0e07bbcf337bf7c289a9093d3db805da617cbfef))
DE_DMD32ROM8x(        "ripdispl.301",CRC(47c87ad4) SHA1(eb372b9f17b28d0781c49a28cb850916ccec323d))
RBION_SND_SP
SE_ROMEND
#define input_ports_rip310l input_ports_ripleys
#define init_rip310l init_ripleys
CORE_CLONEDEFNV(rip310l,ripleys,"Ripley's Believe It or Not! (3.10 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip302, "ripcpu.302",CRC(ee79d9eb) SHA1(79b45ceac00ebd414a9fb1d97c05252d9f953872))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
RBION_SND
SE_ROMEND
#define input_ports_rip302 input_ports_ripleys
#define init_rip302 init_ripleys
CORE_CLONEDEFNV(rip302,ripleys,"Ripley's Believe It or Not! (3.02)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip302f,"ripcpu.302",CRC(ee79d9eb) SHA1(79b45ceac00ebd414a9fb1d97c05252d9f953872))
DE_DMD32ROM8x(        "ripdispf.301",CRC(e5ae9d99) SHA1(74929b324b457d08a925c641430e6a7036c7039d))
RBION_SND_FR
SE_ROMEND
#define input_ports_rip302f input_ports_ripleys
#define init_rip302f init_ripleys
CORE_CLONEDEFNV(rip302f,ripleys,"Ripley's Believe It or Not! (3.02 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip302g,"ripcpu.302",CRC(ee79d9eb) SHA1(79b45ceac00ebd414a9fb1d97c05252d9f953872))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
RBION_SND_GR
SE_ROMEND
#define input_ports_rip302g input_ports_ripleys
#define init_rip302g init_ripleys
CORE_CLONEDEFNV(rip302g,ripleys,"Ripley's Believe It or Not! (3.02 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip302i,"ripcpu.302",CRC(ee79d9eb) SHA1(79b45ceac00ebd414a9fb1d97c05252d9f953872))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
RBION_SND_IT
SE_ROMEND
#define input_ports_rip302i input_ports_ripleys
#define init_rip302i init_ripleys
CORE_CLONEDEFNV(rip302i,ripleys,"Ripley's Believe It or Not! (3.02 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip302l,"ripcpu.302",CRC(ee79d9eb) SHA1(79b45ceac00ebd414a9fb1d97c05252d9f953872))
DE_DMD32ROM8x(        "ripdispl.301",CRC(47c87ad4) SHA1(eb372b9f17b28d0781c49a28cb850916ccec323d))
RBION_SND_SP
SE_ROMEND
#define input_ports_rip302l input_ports_ripleys
#define init_rip302l init_ripleys
CORE_CLONEDEFNV(rip302l,ripleys,"Ripley's Believe It or Not! (3.02 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip301, "ripcpu.301",CRC(a867d1b4) SHA1(dca4ba5c981397d26cac016d8438704f7adea0f3))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
RBION_SND
SE_ROMEND
#define input_ports_rip301 input_ports_ripleys
#define init_rip301 init_ripleys
CORE_CLONEDEFNV(rip301,ripleys,"Ripley's Believe It or Not! (3.01)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip301f,"ripcpu.301",CRC(a867d1b4) SHA1(dca4ba5c981397d26cac016d8438704f7adea0f3))
DE_DMD32ROM8x(        "ripdispf.301",CRC(e5ae9d99) SHA1(74929b324b457d08a925c641430e6a7036c7039d))
RBION_SND_FR
SE_ROMEND
#define input_ports_rip301f input_ports_ripleys
#define init_rip301f init_ripleys
CORE_CLONEDEFNV(rip301f,ripleys,"Ripley's Believe It or Not! (3.01 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip301g,"ripcpu.301",CRC(a867d1b4) SHA1(dca4ba5c981397d26cac016d8438704f7adea0f3))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
RBION_SND_GR
SE_ROMEND
#define input_ports_rip301g input_ports_ripleys
#define init_rip301g init_ripleys
CORE_CLONEDEFNV(rip301g,ripleys,"Ripley's Believe It or Not! (3.01 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip301i,"ripcpu.301",CRC(a867d1b4) SHA1(dca4ba5c981397d26cac016d8438704f7adea0f3))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
RBION_SND_IT
SE_ROMEND
#define input_ports_rip301i input_ports_ripleys
#define init_rip301i init_ripleys
CORE_CLONEDEFNV(rip301i,ripleys,"Ripley's Believe It or Not! (3.01 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip301l,"ripcpu.301",CRC(a867d1b4) SHA1(dca4ba5c981397d26cac016d8438704f7adea0f3))
DE_DMD32ROM8x(        "ripdispl.301",CRC(47c87ad4) SHA1(eb372b9f17b28d0781c49a28cb850916ccec323d))
RBION_SND_SP
SE_ROMEND
#define input_ports_rip301l input_ports_ripleys
#define init_rip301l init_ripleys
CORE_CLONEDEFNV(rip301l,ripleys,"Ripley's Believe It or Not! (3.01 Spanish)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip300, "ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispa.300",CRC(016907c9) SHA1(d37f1ca5ebe089fca879339cdaffc3fabf09c15c))
RBION_SND
SE_ROMEND
#define input_ports_rip300 input_ports_ripleys
#define init_rip300 init_ripleys
CORE_CLONEDEFNV(rip300,ripleys,"Ripley's Believe It or Not! (3.00)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip300f,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispf.300",CRC(b9901941) SHA1(653997ff5d63e7ee0270db08cad952ac8293a8cd))
RBION_SND_FR
SE_ROMEND
#define input_ports_rip300f input_ports_ripleys
#define init_rip300f init_ripleys
CORE_CLONEDEFNV(rip300f,ripleys,"Ripley's Believe It or Not! (3.00 French)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip300g,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispg.300",CRC(1a75883b) SHA1(0ef2f4af72e435e5be9d3d8a6b69c66ae18271a1))
RBION_SND_GR
SE_ROMEND
#define input_ports_rip300g input_ports_ripleys
#define init_rip300g init_ripleys
CORE_CLONEDEFNV(rip300g,ripleys,"Ripley's Believe It or Not! (3.00 German)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip300i,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispi.300",CRC(c3541c04) SHA1(26256e8dee77bcfa96326d2e3f67b6fd3696c0c7))
RBION_SND_IT
SE_ROMEND
#define input_ports_rip300i input_ports_ripleys
#define init_rip300i init_ripleys
CORE_CLONEDEFNV(rip300i,ripleys,"Ripley's Believe It or Not! (3.00 Italian)",2004,"Stern",de_mSES3,0)

SE128_ROMSTART(rip300l,"ripcpu.300",CRC(8c4bf2a9) SHA1(260dd5a99a36de541b5f852047ae4166afc621cc))
DE_DMD32ROM8x(        "ripdispl.300",CRC(d2f496bb) SHA1(48622e25171030b83d8d1736735e97a13c5f47c6))
RBION_SND_SP
SE_ROMEND
#define input_ports_rip300l input_ports_ripleys
#define init_rip300l init_ripleys
CORE_CLONEDEFNV(rip300l,ripleys,"Ripley's Believe It or Not! (3.00 Spanish)",2004,"Stern",de_mSES3,0)

// Elvis moved to its own sim file (elvis)

/*-------------------------------------------------------------------
/ The Sopranos
/-------------------------------------------------------------------*/
#define SOP_SND \
DE3SC_SOUNDROM18888("sopsnda.u7",  CRC(47817240) SHA1(da2ff205fb5fe732514e7aa312ff552ecd4b31b7), \
                    "sopsnda.u17", CRC(21e0cfd2) SHA1(d2ff1242f1f4a206e0b2884c079ef2be5df143ac), \
                    "sopsnda.u21", CRC(10669399) SHA1(6ad31c91ba3c772f7a79e4408a4422332243c7d1), \
                    "sopsnda.u36", CRC(f3535025) SHA1(3fc22af5db0a8b744c0513e24a6331c9cf82e3ed), \
                    "sopsnda.u37", CRC(4b67fe8a) SHA1(b980b9705b4a41a0524b3b0095d6398bdbed609f))

#define SOP_SND_SP \
DE3SC_SOUNDROM18888("sopsndl.u7",  CRC(137110f2) SHA1(9bd911fc74b91e811ada4c66bec214d22506a646), \
                    "sopsndl.u17", CRC(3d5189e6) SHA1(7d846d0b18678ff7aa44029643571e237bc48d58), \
                    "sopsndl.u21", CRC(0ac4f407) SHA1(9473ab42c0f758901644256d7cd1cb47c8396433), \
                    "sopsndl.u36", CRC(147c4216) SHA1(ded2917188bea51cb03db72fe53fcd76a3e66ab9), \
                    "sopsndl.u37", CRC(cfe814fb) SHA1(51b6b10dda4640f8569e610b41c77e3657eabff2))

#define SOP_SND_GR \
DE3SC_SOUNDROM18888("sopsndg.u7",  CRC(bb615e03) SHA1(ce5ef766376c060fc071d54aa878d69b3aa30294), \
                    "sopsndg.u17", CRC(cfa7fca1) SHA1(2efbc8c3e8ad6dc39973908e37ecdc7b02be720a), \
                    "sopsndg.u21", CRC(7669769b) SHA1(adf0c0dbfbaa981fa90db4e54702a4dbaf474e82), \
                    "sopsndg.u36", CRC(08d715b5) SHA1(ddccd311ba2b608ab0845afb3ef63b8d3425d530), \
                    "sopsndg.u37", CRC(2405df73) SHA1(b8074610d9d87d3f1c0244ef0f450c766aac8a20))

#define SOP_SND_FR \
DE3SC_SOUNDROM18888("sopsndf.u7",  CRC(57426738) SHA1(393e1d654ef09172580ad9a2722f696b6e44ec0f), \
                    "sopsndf.u17", CRC(0e34f843) SHA1(2a026bda4c44803239a93e6603a4dfb25e3103b5), \
                    "sopsndf.u21", CRC(28726d20) SHA1(63c6bea953cc34b6a3c2c9688ab86641f94cd227), \
                    "sopsndf.u36", CRC(99549d4a) SHA1(15e3d35b9cefbc8825a7dee5309adc2526de3dec), \
                    "sopsndf.u37", CRC(2b4a9130) SHA1(eed9c84c932bb86954226b0d51461c5094ebe02e))

#define SOP_SND_IT \
DE3SC_SOUNDROM18888("sopsndi.u7",  CRC(afb9c474) SHA1(fd184e8cd6afff61fd2874b08f0e841934916ccb), \
                    "sopsndi.u17", CRC(ec8e4e36) SHA1(312f1d86bf6703b8ff6b807a3a2abea9fe0c20b8), \
                    "sopsndi.u21", CRC(37727b76) SHA1(8801091870a30222d5a99535bbe15ac97334e368), \
                    "sopsndi.u36", CRC(71568348) SHA1(516d5ea35f8323e247c25000cb223f3539796ea1), \
                    "sopsndi.u37", CRC(b34c0a5f) SHA1(b84979d6eef7d23e6dd5410993d83fba2121bc6a))

INITGAME(sopranos,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(sopranos, "sopcpua.500", CRC(e3430f28) SHA1(3b942922087cca41701ef70fbc84baaffe8da90d))
DE_DMD32ROM8x(      "sopdspa.500", CRC(170bd8d1) SHA1(df8d240425ac2c1aa4bea560ecdd3d64120faeb7))
SOP_SND
SE_ROMEND
#define input_ports_sopranos input_ports_se
CORE_GAMEDEFNV(sopranos,"Sopranos, The (5.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopranog, "sopcpug.500", CRC(e6317603) SHA1(2e7f49cd77f65d0af08ab503fc82ec56c8890926))
DE_DMD32ROM8x(      "sopdspg.500", CRC(d8f365e9) SHA1(395209169e023913bf0bf3c3837e9a1b6b636e75))
SOP_SND_GR
SE_ROMEND
#define input_ports_sopranog input_ports_sopranos
#define init_sopranog init_sopranos
CORE_CLONEDEFNV(sopranog,sopranos,"Sopranos, The (5.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopranof, "sopcpuf.500", CRC(d047d4bd) SHA1(588ba8f6c7af32f6aa94f29e59ad2bcd2bc64968))
DE_DMD32ROM8x(      "sopdspf.500", CRC(e4252fb5) SHA1(be54181af8f3650023f20cf1bf3b8b0310adb1bb))
SOP_SND_FR
SE_ROMEND
#define input_ports_sopranof input_ports_sopranos
#define init_sopranof init_sopranos
CORE_CLONEDEFNV(sopranof,sopranos,"Sopranos, The (5.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopranol, "sopcpul.500", CRC(ba978d00) SHA1(afe561cc439d9e51dee0697f423fce103448504c))
DE_DMD32ROM8x(      "sopdspl.500", CRC(a4100c9e) SHA1(08ea2424ff315f6174d56301c7a8164a32629367))
SOP_SND_SP
SE_ROMEND
#define input_ports_sopranol input_ports_sopranos
#define init_sopranol init_sopranos
CORE_CLONEDEFNV(sopranol,sopranos,"Sopranos, The (5.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopranoi, "sopcpui.500", CRC(c6b29efb) SHA1(1d7045c06bd8c6fc2304fba150c201e920ae92b4))
DE_DMD32ROM8x(      "sopdspi.500", CRC(5a3f479b) SHA1(43f36e27549259df172ed4340ae891eca634a594))
SOP_SND_IT
SE_ROMEND
#define input_ports_sopranoi input_ports_sopranos
#define init_sopranoi init_sopranos
CORE_CLONEDEFNV(sopranoi,sopranos,"Sopranos, The (5.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr400, "sopcpua.400", CRC(68efcf24) SHA1(9ef30808260f96fb19067ee473add0c43dd6180e))
DE_DMD32ROM8x(      "sopdspa.400", CRC(60d6b9d3) SHA1(925d2c84e486e4a71bd05b542429a0e22a99072f))
SOP_SND
SE_ROMEND
#define input_ports_sopr400 input_ports_sopranos
#define init_sopr400 init_sopranos
CORE_CLONEDEFNV(sopr400,sopranos,"Sopranos, The (4.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr400g, "sopcpug.400", CRC(44bd5b83) SHA1(edd11ee10a3bba9055363919f317421dee84cd85))
DE_DMD32ROM8x(      "sopdspg.400", CRC(2672ef2c) SHA1(8e042b6a98edd8d7b7682d01914d8d021f526b35))
SOP_SND_GR
SE_ROMEND
#define input_ports_sopr400g input_ports_sopranos
#define init_sopr400g init_sopranos
CORE_CLONEDEFNV(sopr400g,sopranos,"Sopranos, The (4.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr400f, "sopcpuf.400", CRC(d6f770cd) SHA1(35f35bb91c1444ba42e29542213745b7e90c9b27))
DE_DMD32ROM8x(      "sopdspf.400", CRC(df451810) SHA1(3c396cac89c57dbacde9b82681dd5600616d6d93))
SOP_SND_FR
SE_ROMEND
#define input_ports_sopr400f input_ports_sopranos
#define init_sopr400f init_sopranos
CORE_CLONEDEFNV(sopr400f,sopranos,"Sopranos, The (4.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr400l, "sopcpul.400", CRC(7fe54359) SHA1(cb00318484ad1e30ab86c3a239fd2ea322aa945e))
DE_DMD32ROM8x(      "sopdspl.400", CRC(4fbef543) SHA1(3ffb48031451d3b318f88bfab4d92d2903993492))
SOP_SND_SP
SE_ROMEND
#define input_ports_sopr400l input_ports_sopranos
#define init_sopr400l init_sopranos
CORE_CLONEDEFNV(sopr400l,sopranos,"Sopranos, The (4.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr400i, "sopcpui.400", CRC(9bd48a86) SHA1(16aa55892a295a1486ede60df104520362005155))
DE_DMD32ROM8x(      "sopdspi.400", CRC(7e58e364) SHA1(1fcf282ae68e3e725e16e43b85f57d1a18b43508))
SOP_SND_IT
SE_ROMEND
#define input_ports_sopr400i input_ports_sopranos
#define init_sopr400i init_sopranos
CORE_CLONEDEFNV(sopr400i,sopranos,"Sopranos, The (4.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr300, "sopcpua.300", CRC(cca0e551) SHA1(ef109b871605879b55abb966d6736d7deeca404f))
DE_DMD32ROM8x(      "sopdspa.300", CRC(aa6306ac) SHA1(80737bd2b93bfc64490d07d2cd482350ed3303b3))
SOP_SND
SE_ROMEND
#define input_ports_sopr300 input_ports_sopranos
#define init_sopr300 init_sopranos
CORE_CLONEDEFNV(sopr300,sopranos, "Sopranos, The (3.00)",2005,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ The Sopranos (3.00) alternative sound feat a unique U7
/-------------------------------------------------------------------*/
SE128_ROMSTART(soprano3, "sopcpua.300", CRC(cca0e551) SHA1(ef109b871605879b55abb966d6736d7deeca404f))
DE_DMD32ROM8x(      "sopdspa.300", CRC(aa6306ac) SHA1(80737bd2b93bfc64490d07d2cd482350ed3303b3))
DE3SC_SOUNDROM18888("sopsnd3.u7",  CRC(b22ba5aa) SHA1(8f69932e3b115ae7a6bcb9a15a8b5bf6579e94e0),
                    "sopsnda.u17", CRC(21e0cfd2) SHA1(d2ff1242f1f4a206e0b2884c079ef2be5df143ac),
                    "sopsnd1.u21", CRC(257ab09d) SHA1(1d18e279139b1658ce02160d9a37b4bf043393f0),
                    "sopsnd1.u36", CRC(db33b45c) SHA1(d3285008a3c770371389be470c1ec5ca49c1e568),
                    "sopsnd1.u37", CRC(06a2a6e1) SHA1(fdbe622223724ac2b4c5183c43d3e635654864bf))
SE_ROMEND
#define input_ports_soprano3 input_ports_sopranos
#define init_soprano3 init_sopranos
CORE_CLONEDEFNV(soprano3,sopranos, "Sopranos, The (3.00, alternative sound)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr300l, "sopcpul.300", CRC(2efd7a45) SHA1(d179605c385c3e3b269bb81295f79a52e0f3f627))
DE_DMD32ROM8x(      "sopdspl.300", CRC(d6f7a723) SHA1(462c8c82ffb6e386adfc411d3e70c4b25553dc7a))
SOP_SND_SP
SE_ROMEND
#define input_ports_sopr300l input_ports_sopranos
#define init_sopr300l init_sopranos
CORE_CLONEDEFNV(sopr300l,sopranos, "Sopranos, The (3.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr300g, "sopcpug.300", CRC(acfa9c83) SHA1(8a31b1008097ff86930b2c946c2777511efc8f9a))
DE_DMD32ROM8x(      "sopdspg.300", CRC(9fa4f9d6) SHA1(86af57435d3d33f8686a56ac59e411f2cb69f565))
SOP_SND_GR
SE_ROMEND
#define input_ports_sopr300g input_ports_sopranos
#define init_sopr300g init_sopranos
CORE_CLONEDEFNV(sopr300g,sopranos, "Sopranos, The (3.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr300f, "sopcpuf.300", CRC(0bd0f8c4) SHA1(df32dcf1db996203fc8b50c78e973b784ae58dbd))
DE_DMD32ROM8x(      "sopdspf.300", CRC(693bd940) SHA1(dd277da4e8239ae5ede3ded37efc8377ba85919a))
SOP_SND_FR
SE_ROMEND
#define input_ports_sopr300f input_ports_sopranos
#define init_sopr300f init_sopranos
CORE_CLONEDEFNV(sopr300f,sopranos, "Sopranos, The (3.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr300i, "sopcpui.300", CRC(1a2846e6) SHA1(10dd57d5f65f397d67105f7e1d0e8d920753893c))
DE_DMD32ROM8x(      "sopdspi.300", CRC(d7903ed2) SHA1(ae54952cd3e6f7fb0075e71d484701def764f0d6))
SOP_SND_IT
SE_ROMEND
#define input_ports_sopr300i input_ports_sopranos
#define init_sopr300i init_sopranos
CORE_CLONEDEFNV(sopr300i,sopranos, "Sopranos, The (3.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr204, "sopcpua.204", CRC(95251d1e) SHA1(c22754647afd07f42bb6b2d0944f696922e68feb))
DE_DMD32ROM8x(      "sopdspa.200", CRC(e5de9a5d) SHA1(6e18d4bdf2d35c9c0743fa6f91f540686d0a706b))
DE3SC_SOUNDROM18888("sopsnd1.u7",  CRC(4f6748b5) SHA1(63e953a1455dee2a44484fef951fa34cb2e55d7b),
                    "sopsnd1.u17", CRC(1ecc5ecc) SHA1(42897387b90df8da8ae556ccc46e281ca461c063),
                    "sopsnd1.u21", CRC(257ab09d) SHA1(1d18e279139b1658ce02160d9a37b4bf043393f0),
                    "sopsnd1.u36", CRC(db33b45c) SHA1(d3285008a3c770371389be470c1ec5ca49c1e568),
                    "sopsnd1.u37", CRC(06a2a6e1) SHA1(fdbe622223724ac2b4c5183c43d3e635654864bf))
SE_ROMEND
#define input_ports_sopr204 input_ports_sopranos
#define init_sopr204 init_sopranos
CORE_CLONEDEFNV(sopr204,sopranos, "Sopranos, The (2.04)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr107l, "sopcpul.107", CRC(a08311fe) SHA1(93e3ecc3e2c69f30d0fbb72c7426f3c2ba4b27b4))
DE_DMD32ROM8x(      "sopdspl.100", CRC(1f52723e) SHA1(c972252a139c871e4bbbf20382ceb738b84f9a18))
DE3SC_SOUNDROM18888("sopsndl.u7",  CRC(137110f2) SHA1(9bd911fc74b91e811ada4c66bec214d22506a646),
                    "sopsndl.u17", CRC(3d5189e6) SHA1(7d846d0b18678ff7aa44029643571e237bc48d58),
                    "sopsndl1.u21",CRC(66cdb90d) SHA1(d96e1b92e54a94b5e0ed9d62cff9220b9e215e85),
                    "sopsndl.u36", CRC(147c4216) SHA1(ded2917188bea51cb03db72fe53fcd76a3e66ab9),
                    "sopsndl.u37", CRC(cfe814fb) SHA1(51b6b10dda4640f8569e610b41c77e3657eabff2))
SE_ROMEND
#define input_ports_sopr107l input_ports_sopranos
#define init_sopr107l init_sopranos
CORE_CLONEDEFNV(sopr107l,sopranos, "Sopranos, The (1.07 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr107g, "sopcpug.107", CRC(e9c83725) SHA1(958155919cbb347b72784c7da112b188e06c908f))
DE_DMD32ROM8x(      "sopdspg.100", CRC(38625560) SHA1(c41a6808fe05cafe44ea5026689b2ea6eb195e41))
DE3SC_SOUNDROM18888("sopsndg.u7",  CRC(bb615e03) SHA1(ce5ef766376c060fc071d54aa878d69b3aa30294),
                    "sopsndg.u17", CRC(cfa7fca1) SHA1(2efbc8c3e8ad6dc39973908e37ecdc7b02be720a),
                    "sopsndg1.u21",CRC(caae114a) SHA1(84703649d7ba05d011348f84e4cac31a023146c0),
                    "sopsndg.u36", CRC(08d715b5) SHA1(ddccd311ba2b608ab0845afb3ef63b8d3425d530),
                    "sopsndg.u37", CRC(2405df73) SHA1(b8074610d9d87d3f1c0244ef0f450c766aac8a20))
SE_ROMEND
#define input_ports_sopr107g input_ports_sopranos
#define init_sopr107g init_sopranos
CORE_CLONEDEFNV(sopr107g,sopranos, "Sopranos, The (1.07 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr107f, "sopcpuf.107", CRC(1cc86040) SHA1(0b4322eca8a5be7ea92356adf65b6c6c6f4205ca))
DE_DMD32ROM8x(      "sopdspf.100", CRC(18c36c19) SHA1(2b2e5cb00b92d7c8875de2d2d19b82305d9fb27f))
DE3SC_SOUNDROM18888("sopsndf.u7",  CRC(57426738) SHA1(393e1d654ef09172580ad9a2722f696b6e44ec0f),
                    "sopsndf1.u17",CRC(9e0dd4a8) SHA1(82b772eb7081f22f1203ed113ec7b05f2e26258c),
                    "sopsndf.u21", CRC(28726d20) SHA1(63c6bea953cc34b6a3c2c9688ab86641f94cd227),
                    "sopsndf.u36", CRC(99549d4a) SHA1(15e3d35b9cefbc8825a7dee5309adc2526de3dec),
                    "sopsndf.u37", CRC(2b4a9130) SHA1(eed9c84c932bb86954226b0d51461c5094ebe02e))
SE_ROMEND
#define input_ports_sopr107f input_ports_sopranos
#define init_sopr107f init_sopranos
CORE_CLONEDEFNV(sopr107f,sopranos, "Sopranos, The (1.07 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(sopr107i, "sopcpui.107", CRC(d5cd6b07) SHA1(830de4af7f54c85feeae6fb7f630f84e48fdb98b))
DE_DMD32ROM8x(      "sopdspi.100", CRC(2a6320c1) SHA1(8cd25c53abb353cbbb88af3e7384c7275d836dbb))
DE3SC_SOUNDROM18888("sopsndi.u7",  CRC(afb9c474) SHA1(fd184e8cd6afff61fd2874b08f0e841934916ccb),
                    "sopsndi1.u17",CRC(7cb762dd) SHA1(84ec54d6495ccb02052c8d5b6b66c018a702bb4e),
                    "sopsndi.u21", CRC(37727b76) SHA1(8801091870a30222d5a99535bbe15ac97334e368),
                    "sopsndi.u36", CRC(71568348) SHA1(516d5ea35f8323e247c25000cb223f3539796ea1),
                    "sopsndi.u37", CRC(b34c0a5f) SHA1(b84979d6eef7d23e6dd5410993d83fba2121bc6a))
SE_ROMEND
#define input_ports_sopr107i input_ports_sopranos
#define init_sopr107i init_sopranos
CORE_CLONEDEFNV(sopr107i,sopranos, "Sopranos, The (1.07 Italian)",2005,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ NASCAR
/-------------------------------------------------------------------*/
#define NASCAR_SND \
DE3SC_SOUNDROM18888("nassnd.u7",  CRC(3a3c8203) SHA1(c64c424c01ec91e2578fd6ddc5d3596b8a485c22), \
                    "nassnd.u17", CRC(4dcf65fa) SHA1(bc745e16f1f4b92b97fd0536bea789909b9c0c67), \
                    "nassnd.u21", CRC(82ac1e4f) SHA1(8a6518885d89651df31afc8119d87a46fd802e16), \
                    "nassnd.u36", CRC(2385ada2) SHA1(d3b59beffe6817cc3ea1140698095886ec2f2324), \
                    "nassnd.u37", CRC(458ba148) SHA1(594fd9b48aa48ab7b3df921e689b1acba2b09d79))

#define NASCAR_SND_SP \
DE3SC_SOUNDROM18888("nascsp.u7",   CRC(03a34394) SHA1(d1e3a1a8e14525c40e9f8a5441a106df662608f1), \
                    "nassndl.u17", CRC(058c67ad) SHA1(70e22d8a1842108309f6c03dcc6ac23a822da3c3), \
                    "nassndl.u21", CRC(e34d3b6f) SHA1(63ef27ed5965d719215d0a469886d3852b6bffb6), \
                    "nassndl.u36", CRC(9e2658b1) SHA1(0d93a381a65f11022a1a6da5e5b0e4a0e779f336), \
                    "nassndl.u37", CRC(63f084ab) SHA1(519807bf6e868df6f756ad30af2f6636804f167c))


INITGAME(nascar,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(nascar, "nascpua.450", CRC(da902e01) SHA1(afc6ace2b31c8682fb4d05e1b472c2ec30e7559b))
DE_DMD32ROM8x(      "nasdspa.400", CRC(364878bf) SHA1(a1fb477a37459a3583d3767386f87aa620e31e34))
NASCAR_SND
SE_ROMEND
#define input_ports_nascar input_ports_se
CORE_GAMEDEFNV(nascar,"NASCAR (4.50)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(nascarl, "nascpul.450", CRC(3eebae3f) SHA1(654f0e44ce009450e66250423fcf0ff4727e5ee1))
DE_DMD32ROM8x(      "nasdspl.400", CRC(a4de490f) SHA1(bc1aa9fc0182045f5d10044b3e4fa083572be4ac))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nascarl input_ports_se
#define init_nascarl init_nascar
CORE_CLONEDEFNV(nascarl,nascar,"NASCAR (4.50 Spanish)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(nas400, "nascpua.400", CRC(24a72071) SHA1(5bfe473e85e12b30963b15dfc8732f2ef9c299c3))
DE_DMD32ROM8x(      "nasdspa.400", CRC(364878bf) SHA1(a1fb477a37459a3583d3767386f87aa620e31e34))
NASCAR_SND
SE_ROMEND
#define input_ports_nas400 input_ports_se
#define init_nas400 init_nascar
CORE_CLONEDEFNV(nas400,nascar,"NASCAR (4.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas400l, "nascpul.400", CRC(23ca7b4a) SHA1(9ea7afb283157a8e65106dc027cfd45eecc3f86a))
DE_DMD32ROM8x(      "nasdspl.400", CRC(a4de490f) SHA1(bc1aa9fc0182045f5d10044b3e4fa083572be4ac))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nas400l input_ports_se
#define init_nas400l init_nascar
CORE_CLONEDEFNV(nas400l,nascar,"NASCAR (4.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas352, "nascpua.352", CRC(65b8132e) SHA1(b42dca3e68d3eff158bae830f6c8cca00e0ed3e2))
DE_DMD32ROM8x(      "nasdspa.303", CRC(86e20410) SHA1(c499682713facc6b2923fdd0eff47b98f6a36d14))
NASCAR_SND
SE_ROMEND
#define input_ports_nas352 input_ports_se
#define init_nas352 init_nascar
CORE_CLONEDEFNV(nas352,nascar,"NASCAR (3.52)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas352l, "nascpul.352", CRC(c78549d8) SHA1(9796c0d413fd2ea7f616ad238b67311c8c29286d))
DE_DMD32ROM8x(      "nasdspl.303", CRC(868277f0) SHA1(9c058054e6dc3b838bfc3a91d37438afcd59aa4b))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nas352l input_ports_se
#define init_nas352l init_nascar
CORE_CLONEDEFNV(nas352l,nascar,"NASCAR (3.52 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas350, "nascpua.350", CRC(e5b4ed49) SHA1(0d38c0e08862a0a5a200225634c5bf0d0afe5afe))
DE_DMD32ROM8x(      "nasdspa.303", CRC(86e20410) SHA1(c499682713facc6b2923fdd0eff47b98f6a36d14))
NASCAR_SND
SE_ROMEND
#define input_ports_nas350 input_ports_se
#define init_nas350 init_nascar
CORE_CLONEDEFNV(nas350,nascar,"NASCAR (3.50)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas350l, "nascpul.350", CRC(ab749309) SHA1(95d35126bda75c68037010f001c28a860b6a6e0c))
DE_DMD32ROM8x(      "nasdspl.303", CRC(868277f0) SHA1(9c058054e6dc3b838bfc3a91d37438afcd59aa4b))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nas350l input_ports_se
#define init_nas350l init_nascar
CORE_CLONEDEFNV(nas350l,nascar,"NASCAR (3.50 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas340, "nascpua.340", CRC(120dc65a) SHA1(151c1604dacb1c1cf74449291d81629a05fb1b09))
DE_DMD32ROM8x(      "nasdspa.303", CRC(86e20410) SHA1(c499682713facc6b2923fdd0eff47b98f6a36d14))
NASCAR_SND
SE_ROMEND
#define input_ports_nas340 input_ports_se
#define init_nas340 init_nascar
CORE_CLONEDEFNV(nas340,nascar,"NASCAR (3.40)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas340l, "nascpul.340", CRC(d5827082) SHA1(660216472a1faa445701eb3735771568cdba7b24))
DE_DMD32ROM8x(      "nasdspl.303", CRC(868277f0) SHA1(9c058054e6dc3b838bfc3a91d37438afcd59aa4b))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nas340l input_ports_se
#define init_nas340l init_nascar
CORE_CLONEDEFNV(nas340l,nascar,"NASCAR (3.40 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas301, "nascpua.301", CRC(8ede60c2) SHA1(aa49da40f2ed858c5fa260ce5e7dd096b4217544))
DE_DMD32ROM8x(      "nasdspa.301", CRC(4de3c8d5) SHA1(c2c08ddd0ecc511cf34ba6a6cae9968e903b88ad))
NASCAR_SND
SE_ROMEND
#define input_ports_nas301 input_ports_se
#define init_nas301 init_nascar
CORE_CLONEDEFNV(nas301,nascar,"NASCAR (3.01)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(nas301l, "nascpul.301", CRC(6c8fc295) SHA1(2852afb38807a96907bc7357c08235de643dfb29))
DE_DMD32ROM8x(      "nasdspl.301", CRC(a6d4a9e7) SHA1(c87286bd173a50e17994474f98574c30cd6d8d39))
NASCAR_SND_SP
SE_ROMEND
#define input_ports_nas301l input_ports_se
#define init_nas301l init_nascar
CORE_CLONEDEFNV(nas301l,nascar,"NASCAR (3.01 Spanish)",2005,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Dale Jr. (NASCAR 5.00)
/-------------------------------------------------------------------*/
INITGAME(dalejr,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(dalejr, "dalecpu.500", CRC(b723b7db) SHA1(ee5f96599f8ccb0fda0695e5e8af438c3f559df3))
DE_DMD32ROM8x(      "daledisp.500", CRC(5dad91cd) SHA1(ef9ce5573f580abc720a184625c96672b5337191))
NASCAR_SND
SE_ROMEND
#define input_ports_dalejr input_ports_se
CORE_GAMEDEFNV(dalejr,"Dale Jr. (5.00)",2006,"Stern",de_mSES3,0)

/*-------------------------------------------------------------------
/ Grand Prix
/-------------------------------------------------------------------*/
#define GP_SND \
DE3SC_SOUNDROM18888("gpsnda.u7",  CRC(f784634f) SHA1(40847986003b01c9de5d9af4c66a0f1f9fb0cac8), \
                    "gpsnda.u17", CRC(43dca7e2) SHA1(30726897950b168ffa5e0e8a4ff12856fd50f132), \
                    "gpsnda.u21", CRC(77717142) SHA1(055f975c3d1cf6560908f5d8553f7e64580a2bba), \
                    "gpsnda.u36", CRC(6e414e19) SHA1(5b7c9da9c340ec3b55163f5674d72ab30ffbb866), \
                    "gpsnda.u37", CRC(caf4c3f3) SHA1(ebdbaccf951ef6525f0fafa7e23d8140ef6b84e5))

#define GP_SND_SP \
DE3SC_SOUNDROM18888("gpsndl.u7",  CRC(0640fe8f) SHA1(aa45bf89c4cae5b4c2143656cfe19fe8f1ec30a3), \
                    "gpsndl.u17", CRC(2581ef04) SHA1(2d85040e355ed410c7d8348ef64fc2c8e76ec0f0), \
                    "gpsndl.u21", CRC(f4c97c9e) SHA1(ae04f416a7582efee20469ec686d02727558d850), \
                    "gpsndl.u36", CRC(863de01a) SHA1(3f1fd157c2abacdab072146499b64b9e0853fb3e), \
                    "gpsndl.u37", CRC(db16b68a) SHA1(815fdcd4ae01c6264133389ce3194da572e1c232))

#define GP_SND_GR \
DE3SC_SOUNDROM18888("gpsndg.u7",  CRC(95129e03) SHA1(5fddd9d8213f9f1f68fe9e96c9e78dc6771fab21), \
                    "gpsndg.u17", CRC(27ef97ea) SHA1(8ba941d5d4f929b8ec3222f1c91452395e2f690f), \
                    "gpsndg.u21", CRC(71391d71) SHA1(690b280710c79d94fc271541066ae90e462bbce2), \
                    "gpsndg.u36", CRC(415baa1b) SHA1(cca21e0e5ef0cbe34c9514d72a06fc129990787a), \
                    "gpsndg.u37", CRC(e4a6ae7f) SHA1(4a4cd973f90c13ced07459c8f457314c8280dd6a))

#define GP_SND_FR \
DE3SC_SOUNDROM18888("gpsndf.u7",  CRC(9b34e55a) SHA1(670fe4e4b62c46266667f37c0341bb4266e55067), \
                    "gpsndf.u17", CRC(18beb699) SHA1(a3bbd7c9fc1165da5e502e09f68321bd56992e76), \
                    "gpsndf.u21", CRC(b64702dd) SHA1(8762fb00d5549649444f7f85c3f6d72f27c6ba41), \
                    "gpsndf.u36", CRC(4e41f0bb) SHA1(4a25b472a9435c77712559d7ded1649dffbc885c), \
                    "gpsndf.u37", CRC(e6d96767) SHA1(a471d51796edad71eb21aadc4a26bb1529a0b9cc))

#define GP_SND_IT \
DE3SC_SOUNDROM18888("gpsndi.u7",  CRC(37d66e66) SHA1(219fd734d3a19407d9d47de198429c770d7d8856), \
                    "gpsndi.u17", CRC(868b225d) SHA1(bc169cf5882002a1b58973a22a78d8dd4467bc51), \
                    "gpsndi.u21", CRC(b6692c39) SHA1(ac36ffb37ad945a857d5098547479c8cd62b6356), \
                    "gpsndi.u36", CRC(f8558b24) SHA1(ceb3880b026fb7fcc69eb8d94e33e30c56c24de8), \
                    "gpsndi.u37", CRC(a76c6682) SHA1(6d319a8f07c10fe392fc0b8e177cc6abbce0b536))

INITGAME(gprix,GEN_WS,se_dmd128x32,SE_LED)
SE128_ROMSTART(gprix, "gpcpua.450", CRC(3e5ae527) SHA1(27d50a0460b1733c6c857968b85da492fa2ad544))
DE_DMD32ROM8x(      "gpdspa.400", CRC(ce431306) SHA1(2573049b52b928052f196371dbc3a5236ce8cfc3))
GP_SND
SE_ROMEND
#define input_ports_gprix input_ports_se
CORE_GAMEDEFNV(gprix,"Grand Prix (4.50)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(gprixg, "gpcpug.450", CRC(d803128b) SHA1(f914f75f4ec38dcbd2e40818fe8cb0ad446c59bf))
DE_DMD32ROM8x(      "gpdspg.400", CRC(b3f64332) SHA1(84e1b094c74b2dfae8e3cd3ce3f1cd20dc400fd7))
GP_SND_GR
SE_ROMEND
#define input_ports_gprixg input_ports_se
#define init_gprixg init_gprix
CORE_CLONEDEFNV(gprixg,gprix,"Grand Prix (4.50 German)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(gprixl, "gpcpul.450", CRC(816bf4a4) SHA1(d5cca282e58d493be36400a7cd7dc4321d98f2f8))
DE_DMD32ROM8x(      "gpdspl.400", CRC(74d9aa40) SHA1(802c6fbe4248a516f18e4b69997254b3dcf27706))
GP_SND_SP
SE_ROMEND
#define input_ports_gprixl input_ports_se
#define init_gprixl init_gprix
CORE_CLONEDEFNV(gprixl,gprix,"Grand Prix (4.50 Spanish)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(gprixf, "gpcpuf.450", CRC(b14f7d20) SHA1(b91097490ee568e00be58f5dac184c8d47196adc))
DE_DMD32ROM8x(      "gpdspf.400", CRC(f9b1ef9a) SHA1(a7e3c0fc1526cf3632e6b1f22caf7f73749e77a6))
GP_SND_FR
SE_ROMEND
#define input_ports_gprixf input_ports_se
#define init_gprixf init_gprix
CORE_CLONEDEFNV(gprixf,gprix,"Grand Prix (4.50 French)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(gprixi, "gpcpui.450", CRC(f18d8375) SHA1(b7e4e311623babc7b3c5d744122c88d45d77a33b))
DE_DMD32ROM8x(      "gpdspi.400", CRC(88675cdf) SHA1(b305a683350d38b43f2e3c9277af14d5503b3219))
GP_SND_IT
SE_ROMEND
#define input_ports_gprixi input_ports_se
#define init_gprixi init_gprix
CORE_CLONEDEFNV(gprixi,gprix,"Grand Prix (4.50 Italian)",2006,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr400, "gpcpua.400", CRC(c6042517) SHA1(19729a86a3afafb516f000489c38d00379e4f85c))
DE_DMD32ROM8x(      "gpdspa.400", CRC(ce431306) SHA1(2573049b52b928052f196371dbc3a5236ce8cfc3))
GP_SND
SE_ROMEND
#define input_ports_gpr400 input_ports_se
#define init_gpr400 init_gprix
CORE_CLONEDEFNV(gpr400,gprix,"Grand Prix (4.00)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr400g, "gpcpug.400", CRC(98d69588) SHA1(9d248b00a1aa966e69816cd7aaf869737e7a1ca7))
DE_DMD32ROM8x(      "gpdspg.400", CRC(b3f64332) SHA1(84e1b094c74b2dfae8e3cd3ce3f1cd20dc400fd7))
GP_SND_GR
SE_ROMEND
#define input_ports_gpr400g input_ports_se
#define init_gpr400g init_gprix
CORE_CLONEDEFNV(gpr400g,gprix,"Grand Prix (4.00 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr400l, "gpcpul.400", CRC(90f506f0) SHA1(4642fa8bf15955a32b6ae8c6b859d94dcd40c542))
DE_DMD32ROM8x(      "gpdspl.400", CRC(74d9aa40) SHA1(802c6fbe4248a516f18e4b69997254b3dcf27706))
GP_SND_SP
SE_ROMEND
#define input_ports_gpr400l input_ports_se
#define init_gpr400l init_gprix
CORE_CLONEDEFNV(gpr400l,gprix,"Grand Prix (4.00 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr400f, "gpcpuf.400", CRC(0e7f8776) SHA1(0731c3a5350445f70dd8bdac68b2554942f12c8d))
DE_DMD32ROM8x(      "gpdspf.400", CRC(f9b1ef9a) SHA1(a7e3c0fc1526cf3632e6b1f22caf7f73749e77a6))
GP_SND_FR
SE_ROMEND
#define input_ports_gpr400f input_ports_se
#define init_gpr400f init_gprix
CORE_CLONEDEFNV(gpr400f,gprix,"Grand Prix (4.00 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr400i, "gpcpui.400", CRC(49e161b7) SHA1(855e35ddedf35055de384bade4b237810bc5ffec))
DE_DMD32ROM8x(      "gpdspi.400", CRC(88675cdf) SHA1(b305a683350d38b43f2e3c9277af14d5503b3219))
GP_SND_IT
SE_ROMEND
#define input_ports_gpr400i input_ports_se
#define init_gpr400i init_gprix
CORE_CLONEDEFNV(gpr400i,gprix,"Grand Prix (4.00 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr352, "gpcpua.352", CRC(da46b437) SHA1(07cf288d47e1447c015e5bb1fd85df654bde71ef))
DE_DMD32ROM8x(      "gpdspa.303", CRC(814f6a50) SHA1(727eac96c4beaafc3ddd9ccd9ef098bd557cbc74))
GP_SND
SE_ROMEND
#define input_ports_gpr352 input_ports_se
#define init_gpr352 init_gprix
CORE_CLONEDEFNV(gpr352,gprix,"Grand Prix (3.52)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr352g, "gpcpug.352", CRC(b6c15f62) SHA1(b8757e7ef7064d200c7965902b624c8ef947f23d))
DE_DMD32ROM8x(      "gpdspg.303", CRC(0be9eb1d) SHA1(78c402efcc818e4960ef5ca17e7fa43a028b5c9b))
GP_SND_GR
SE_ROMEND
#define input_ports_gpr352g input_ports_se
#define init_gpr352g init_gprix
CORE_CLONEDEFNV(gpr352g,gprix,"Grand Prix (3.52 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr352l, "gpcpul.352", CRC(cd296661) SHA1(e706525b07e1e1278ab65a896616e63be52e8e73))
DE_DMD32ROM8x(      "gpdspl.303", CRC(82f30b13) SHA1(2f15228dbd6f3957f657772725f3280adf778d72))
GP_SND_SP
SE_ROMEND
#define input_ports_gpr352l input_ports_se
#define init_gpr352l init_gprix
CORE_CLONEDEFNV(gpr352l,gprix,"Grand Prix (3.52 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr352f, "gpcpuf.352", CRC(78140cd3) SHA1(9c7257dede5c3bf78d9a9bbdf89bd01f12180c4f))
DE_DMD32ROM8x(      "gpdspf.303", CRC(f48f3a4b) SHA1(74c7d1670d6f1ed68d5aed5a755f27ffdb566cbd))
GP_SND_FR
SE_ROMEND
#define input_ports_gpr352f input_ports_se
#define init_gpr352f init_gprix
CORE_CLONEDEFNV(gpr352f,gprix,"Grand Prix (3.52 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr352i, "gpcpui.352", CRC(941bd2a9) SHA1(11402004c5b57de5ec28ea2f4128b1852c205dac))
DE_DMD32ROM8x(      "gpdspi.303", CRC(36418722) SHA1(66f04e3069c51004cb82961a7d82ac0a5f6a84dd))
GP_SND_IT
SE_ROMEND
#define input_ports_gpr352i input_ports_se
#define init_gpr352i init_gprix
CORE_CLONEDEFNV(gpr352i,gprix,"Grand Prix (3.52 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr350, "gpcpua.350", CRC(bd47be96) SHA1(31b7adc5cec10d18dd551fdba94fbdb8c6eac01b))
DE_DMD32ROM8x(      "gpdspa.303", CRC(814f6a50) SHA1(727eac96c4beaafc3ddd9ccd9ef098bd557cbc74))
GP_SND
SE_ROMEND
#define input_ports_gpr350 input_ports_se
#define init_gpr350 init_gprix
CORE_CLONEDEFNV(gpr350,gprix,"Grand Prix (3.50)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr350g, "gpcpug.350", CRC(8e77c953) SHA1(07d23ce2f0b0a2dc6284b71d1a4a8d1bb5dab6d0))
DE_DMD32ROM8x(      "gpdspg.303", CRC(0be9eb1d) SHA1(78c402efcc818e4960ef5ca17e7fa43a028b5c9b))
GP_SND_GR
SE_ROMEND
#define input_ports_gpr350g input_ports_se
#define init_gpr350g init_gprix
CORE_CLONEDEFNV(gpr350g,gprix,"Grand Prix (3.50 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr350l, "gpcpul.350", CRC(714f0641) SHA1(c09aa248fe04fc3c569c6786c0db8d396cbd2403))
DE_DMD32ROM8x(      "gpdspl.303", CRC(82f30b13) SHA1(2f15228dbd6f3957f657772725f3280adf778d72))
GP_SND_SP
SE_ROMEND
#define input_ports_gpr350l input_ports_se
#define init_gpr350l init_gprix
CORE_CLONEDEFNV(gpr350l,gprix,"Grand Prix (3.50 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr350f, "gpcpuf.350", CRC(ad4224e9) SHA1(781a76ef14e7abb2b57ae49cd8712ddace8a4fca))
DE_DMD32ROM8x(      "gpdspf.303", CRC(f48f3a4b) SHA1(74c7d1670d6f1ed68d5aed5a755f27ffdb566cbd))
GP_SND_FR
SE_ROMEND
#define input_ports_gpr350f input_ports_se
#define init_gpr350f init_gprix
CORE_CLONEDEFNV(gpr350f,gprix,"Grand Prix (3.50 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr350i, "gpcpui.350", CRC(64bc5f6b) SHA1(fb079323d3548f1915de93d724d3fb76b2e02f27))
DE_DMD32ROM8x(      "gpdspi.303", CRC(36418722) SHA1(66f04e3069c51004cb82961a7d82ac0a5f6a84dd))
GP_SND_IT
SE_ROMEND
#define input_ports_gpr350i input_ports_se
#define init_gpr350i init_gprix
CORE_CLONEDEFNV(gpr350i,gprix,"Grand Prix (3.50 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr340, "gpcpua.340", CRC(798f2ab3) SHA1(046cbbd0115511b2cbd7f132b0755d03edce1e7b))
DE_DMD32ROM8x(      "gpdspa.303", CRC(814f6a50) SHA1(727eac96c4beaafc3ddd9ccd9ef098bd557cbc74))
GP_SND
SE_ROMEND
#define input_ports_gpr340 input_ports_se
#define init_gpr340 init_gprix
CORE_CLONEDEFNV(gpr340,gprix,"Grand Prix (3.40)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr340g, "gpcpug.340", CRC(34afe4e4) SHA1(0655be39f309f32dffca22d7ab780263da5e8cb2))
DE_DMD32ROM8x(      "gpdspg.303", CRC(0be9eb1d) SHA1(78c402efcc818e4960ef5ca17e7fa43a028b5c9b))
GP_SND_GR
SE_ROMEND
#define input_ports_gpr340g input_ports_se
#define init_gpr340g init_gprix
CORE_CLONEDEFNV(gpr340g,gprix,"Grand Prix (3.40 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr340l, "gpcpul.340", CRC(2cd2f25f) SHA1(ee21a680cf56b6b415b2b9f5d89125062b24f8ae))
DE_DMD32ROM8x(      "gpdspl.303", CRC(82f30b13) SHA1(2f15228dbd6f3957f657772725f3280adf778d72))
GP_SND_SP
SE_ROMEND
#define input_ports_gpr340l input_ports_se
#define init_gpr340l init_gprix
CORE_CLONEDEFNV(gpr340l,gprix,"Grand Prix (3.40 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr340f, "gpcpuf.340", CRC(cefc30e4) SHA1(2ffdfd09ed8ba00a36a6bf12b79200c562b7dc0d))
DE_DMD32ROM8x(      "gpdspf.303", CRC(f48f3a4b) SHA1(74c7d1670d6f1ed68d5aed5a755f27ffdb566cbd))
GP_SND_FR
SE_ROMEND
#define input_ports_gpr340f input_ports_se
#define init_gpr340f init_gprix
CORE_CLONEDEFNV(gpr340f,gprix,"Grand Prix (3.40 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr340i, "gpcpui.340", CRC(35f4a870) SHA1(24293fafcec6180ab62ef3298e4b53910e05a937))
DE_DMD32ROM8x(      "gpdspi.303", CRC(36418722) SHA1(66f04e3069c51004cb82961a7d82ac0a5f6a84dd))
GP_SND_IT
SE_ROMEND
#define input_ports_gpr340i input_ports_se
#define init_gpr340i init_gprix
CORE_CLONEDEFNV(gpr340i,gprix,"Grand Prix (3.40 Italian)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr301, "gpcpua.301", CRC(07cdb3eb) SHA1(2246d253dca93ce8c5f6775352611a3145ab8776))
DE_DMD32ROM8x(      "gpdspa.301", CRC(b11d752d) SHA1(c0e6f5544a3061027bf9addef4363c744aaaf736))
GP_SND
SE_ROMEND
#define input_ports_gpr301 input_ports_se
#define init_gpr301 init_gprix
CORE_CLONEDEFNV(gpr301,gprix,"Grand Prix (3.01)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr301g, "gpcpug.301", CRC(23ac882e) SHA1(5194c3ea18a08f844f4ee293c9de44b62a956ee6))
DE_DMD32ROM8x(      "gpdspg.301", CRC(0d214a2a) SHA1(c7f9bbd56d7038931c8658bc586d29ad2b9ecac2))
GP_SND_GR
SE_ROMEND
#define input_ports_gpr301g input_ports_se
#define init_gpr301g init_gprix
CORE_CLONEDEFNV(gpr301g,gprix,"Grand Prix (3.01 German)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr301l, "gpcpul.301", CRC(ad739825) SHA1(c2fbc25985b83bacd4285b6608366de485f16982))
DE_DMD32ROM8x(      "gpdspl.301", CRC(2f483f0a) SHA1(42550741bee6af022bccd130626913edff6180a0))
GP_SND_SP
SE_ROMEND
#define input_ports_gpr301l input_ports_se
#define init_gpr301l init_gprix
CORE_CLONEDEFNV(gpr301l,gprix,"Grand Prix (3.01 Spanish)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr301f, "gpcpuf.301", CRC(6f9d34ee) SHA1(a8b60300cc1e07d3beb8d607e24285dbbd871e83))
DE_DMD32ROM8x(      "gpdspf.301", CRC(b19729cd) SHA1(491fdf356f5a24b9895d2feccfe29d0bf45f4e27))
GP_SND_FR
SE_ROMEND
#define input_ports_gpr301f input_ports_se
#define init_gpr301f init_gprix
CORE_CLONEDEFNV(gpr301f,gprix,"Grand Prix (3.01 French)",2005,"Stern",de_mSES3,0)

SE128_ROMSTART(gpr301i, "gpcpui.301", CRC(d9be9fd7) SHA1(3dfc997d8d17d153ee42df0adb7993293bfff7e8))
DE_DMD32ROM8x(      "gpdspi.301", CRC(1fc478da) SHA1(5307e9b302a7e49eb3460e8ba1e4c22525a1dcfe))
GP_SND_IT
SE_ROMEND
#define input_ports_gpr301i input_ports_se
#define init_gpr301i init_gprix
CORE_CLONEDEFNV(gpr301i,gprix,"Grand Prix (3.01 Italian)",2005,"Stern",de_mSES3,0)
