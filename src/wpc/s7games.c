#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s7.h"

const core_tLCDLayout s7_dispS7[] = {
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_BALLS(0,8,CORE_SEG7S),DISP_SEG_CREDIT(20,28,CORE_SEG7S),{0}
};
#define INITGAME(name, disp) \
static core_tGameData name##GameData = { GEN_S7, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S7, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S7_INPUT_PORTS_START(s7, 1) S7_INPUT_PORTS_END


/*---------------------------------
/ Black Knight - Sys.7 (Game #500)
/---------------------------------*/
INITGAMEFULL(bk,s7_dispS7,0,0,21,22,36,0,0,0)
S7_ROMSTART8088(bk,l4,"ic14.716",   0xfcbe3d44,
                      "ic17.532",   0xbb571a17,
                      "ic20.716",   0xdfb4b75a,
                      "ic26.716",   0x104b78da)
S67S_SOUNDROMS8(      "sound12.716",0x6d454c0e)
S67S_SPEECHROMS0000(  "speech7.532",0xc7e229bf,
                      "speech5.532",0x411bc92f,
                      "speech6.532",0xfc985005,
                      "speech4.532",0xf36f12e5)
S7_ROMEND
#define input_ports_bk input_ports_s7
CORE_GAMEDEF(bk,l4,"Black Knight (L-4)",1980,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Cosmic Gunfight - Sys.7 (Game #502)
/-----------------------------------*/
INITGAMEFULL(csmic,s7_dispS7,52,49,36,37,21,22,24,23)
S7_ROMSTART8088(csmic,l1, "ic14.716",   0xac66c0dc,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xa259eba0)
S67S_SOUNDROMS8(          "sound12.716",0xaf41737b)
S7_ROMEND
#define input_ports_csmic input_ports_s7
CORE_GAMEDEF(csmic,l1,"Cosmic Gunfight (L-1)",1980,"Williams",s7_mS7S,0)


/*--------------------------------
/ Jungle Lord - Sys.7 (Game #503)
/--------------------------------*/
INITGAMEFULL(jngld,s7_dispS7,0,0,12,28,40,0,0,0)
S7_ROMSTART8088(jngld,l2, "ic14.716",   0x6e5a6374,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x4714b1f1)
S67S_SOUNDROMS8(          "sound12.716",0x55a10d13)
S67S_SPEECHROMS000x(      "speech7.532",0x83ffb695,
			  "speech5.532",0x754bd474,
			  "speech6.532",0xf2ac6a52)
S7_ROMEND
#define input_ports_jngld input_ports_s7
CORE_GAMEDEF(jngld,l2,"Jungle Lord (L-2)",1981,"Williams",s7_mS7S,0)

/*--------------------------------
/ Pharaoh - Sys.7 (Game #504)
/--------------------------------*/
INITGAMEFULL(pharo,s7_dispS7,0,0,20,24,44,0,0,0)
S7_ROMSTART8088(pharo,l2, "ic14.716",   0xcef00088,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x2afbcd1f)
S67S_SOUNDROMS8(          "sound12.716",0xb0e3a04b)
S67S_SPEECHROMS0000(      "speech7.532",0xe087f8a1,
                          "speech5.532",0xd72863dc,
                          "speech6.532",0xd29830bd,
                          "speech4.532",0x9ecc23fd)
S7_ROMEND
#define input_ports_pharo input_ports_s7
CORE_GAMEDEF(pharo,l2,"Pharaoh (L-2)",1981,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Solar Fire - Sys.7 (Game #507)
/-----------------------------------*/
INITGAMEFULL(solar,s7_dispS7,0,0,11,12,0,0,0,0)
S7_ROMSTART8088(solar,l2, "ic14.716",   0xcec19a55,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xb667ee32)
S67S_SOUNDROMS8(          "sound12.716",0x05a2230c)
S7_ROMEND
#define input_ports_solar input_ports_s7
CORE_GAMEDEF(solar,l2,"Solar Fire (L-2)",1981,"Williams",s7_mS7S,0)


/*-------------------------------
/ Hyperball - Sys.7 - (Game #509)
/-------------------------------*/
static const core_tLCDLayout dispHypbl[] = {
  { 0, 0, 1, 7,CORE_SEG87 }, { 0,16, 9, 7,CORE_SEG87 },
  { 2,10,20, 1,CORE_SEG87 }, { 2,12,28, 1,CORE_SEG87 },
  { 2,16, 0, 1,CORE_SEG87 }, { 2,18, 8, 1,CORE_SEG87 },
  { 4, 3,43, 5,CORE_SEG16 }, { 4,13,49, 7,CORE_SEG16 }, {0}
};
static core_tGameData hypblGameData = { GEN_S7, dispHypbl, {FLIP_SWNO(33,34)}, NULL, {"", {0,0,0,0xf8,0x0f}, {0} } };
static void init_hypbl(void) { core_gameData = &hypblGameData; }
#define input_ports_hypbl input_ports_s7

S7_ROMSTART000x(hypbl,l4, "ic14.532",    0x8090fe71,
                          "ic17.532",    0x6f4c0c4c,
                          "ic20.532",    0xd13962e8)
S67S_SOUNDROMS0(          "sound12.532", 0x06051e5e)
S7_ROMEND
CORE_GAMEDEF(hypbl,l4,"HyperBall (L-4)",1981,"Williams",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l5, "ic14.532",    0x8090fe71,
                          "ic17.532",    0x6f4c0c4c,
                          "ic20_fix.532",    0x48958d77)
S67S_SOUNDROMS0(          "sound12.532", 0x06051e5e)
S7_ROMEND
CORE_CLONEDEF(hypbl,l5,l4,"HyperBall (L-5)",1998,"Williams (High score bootleg)",s7_mS7S,0)


/*----------------------------
/ Barracora- Sys.7 (Game #510)
/----------------------------*/
INITGAMEFULL(barra,s7_dispS7,34,35,0,20,21,22,29,30)
S7_ROMSTART8088(barra,l1, "ic14.716",   0x522e944e,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x2a0e0171)
S67S_SOUNDROMS8(          "sound12.716",0x67ea12e7)
S7_ROMEND
#define input_ports_barra input_ports_s7
CORE_GAMEDEF(barra,l1,"Barracora (L-1)",1981,"Williams",s7_mS7S,0)

/*----------------------------
/ Varkon- Sys.7 (Game #512)
/----------------------------*/
INITGAMEFULL(vrkon,s7_dispS7,40,39,33,41,0,0,0,0)
S7_ROMSTART8088(vrkon,l1, "ic14.716",   0x3baba324,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xdf20330c)
S67S_SOUNDROMS8(          "sound12.716",0xd13db2bb)
S7_ROMEND
#define input_ports_vrkon input_ports_s7
CORE_GAMEDEF(vrkon,l1,"Varkon (L-1)",1982,"Williams",s7_mS7S,0)

/*----------------------------------
/Time Fantasy- Sys.7 (Game #515)
/----------------------------------*/
// MOVED TO tmfnt.c

/*----------------------------
/ Warlok- Sys.7 (Game #516)
/----------------------------*/
INITGAMEFULL(wrlok,s7_dispS7,0,36,13,14,15,20,21,0)
S7_ROMSTART8088(wrlok,l3, "ic14.716",   0x291be241,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x44f8b507)
S67S_SOUNDROMS8(          "sound12.716",0x5d8e46d6)
S7_ROMEND
#define input_ports_wrlok input_ports_s7
CORE_GAMEDEF(wrlok,l3,"Warlok (L-3)",1982,"Williams",s7_mS7S,0)

/*----------------------------
/ Defender - Sys.7 (Game #517)
/----------------------------*/
INITGAMEFULL(dfndr,s7_dispS7,0,59,55,56,57,58,0,0)
S7_ROMSTART000x(dfndr,l4, "ic14.532",   0x959ec419,
                          "ic17.532",   0xbb571a17,
                          "ic20.532",   0xe99e64a2)
S67S_SOUNDROMS8(          "sound12.716",0xcabaec58)
S7_ROMEND
#define input_ports_dfndr input_ports_s7
CORE_GAMEDEF(dfndr,l4,"Defender (L-4)",1982,"Williams",s7_mS7S,0)

/*---------------------------
/ Joust - Sys.7 (Game #519)
/--------------------------*/
INITGAMEFULL(jst,s7_dispS7,0,0,0,0,29,53,30,54) // 4 flippers,buttons need special
S7_ROMSTART8088(jst,l2, "ic14.716",   0xc4cae4bf,
                        "ic17.532",   0xbb571a17,
                        "ic20.716",   0xdfb4b75a,
                        "ic26.716",   0x63eea5d8)
S67S_SOUNDROMS0(        "sound12.532",0x3bbc90bf)
S7_ROMEND
#define input_ports_jst input_ports_s7
CORE_GAMEDEF(jst,l2,"Joust (L-2)",1983,"Williams",s7_mS7S,0)


/*---------------------------
/ Laser Cue - Sys.7 (Game #520)
/--------------------------*/
INITGAMEFULL(lsrcu,s7_dispS7,0,43,41,29,18,17,0,0)
S7_ROMSTART8088(lsrcu,l2, "ic14.716",   0x39fc350d,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xdb4a09e7)
S67S_SOUNDROMS8(          "sound12.716",0x1888c635)
S7_ROMEND
#define input_ports_lsrcu input_ports_s7
CORE_GAMEDEF(lsrcu,l2,"Laser Cue (L-2)",1983,"Williams",s7_mS7S,0)

/*--------------------------------
/ Firepower II- Sys.7 (Game #521)
/-------------------------------*/
INITGAMEFULL(fpwr2,s7_dispS7,0,13,47,48,41,42,43,44)
S7_ROMSTART8088(fpwr2,l2, "ic14.716",   0xa29688dd,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x1068939d)
S67S_SOUNDROMS8(          "sound12.716",0x55a10d13)
S7_ROMEND
#define input_ports_fpwr2 input_ports_s7
CORE_GAMEDEF(fpwr2,l2,"Firepower II (L-2)",1983,"Williams",s7_mS7S,0)

/*-----------------------------
/ Starlight - Sys.7 (Game #530)
/-----------------------------*/
INITGAMEFULL(strlt,s7_dispS7,0,54,50,51,41,42,43,0)
S7_ROMSTART000x(strlt,l1,"ic14.532",   0x638e3bad,
                         "ic17.532",   0xa43d8518,
                         "ic20.532",   0xb750ddc0)
S67S_SOUNDROMS8(         "sound12.716",0x00000000)
S7_ROMEND
#define input_ports_strlt input_ports_s7
CORE_GAMEDEF(strlt,l1,"Starlight (L-1)",1984,"Williams",s7_mS7S,GAME_NOT_WORKING)
