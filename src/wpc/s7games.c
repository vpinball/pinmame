#include "driver.h"
#include "core.h"
#include "sim.h"
#include "s67s.h"
#include "s7.h"

core_tLCDLayout s7_dispS7[] = {
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_CREDIT(16,24,CORE_SEG7S),DISP_SEG_BALLS(0,8,CORE_SEG7S),{0}
};
#define INITGAME(name, flip, balls) \
	static core_tGameData name##GameData = { GEN_S7, s7_dispS7, {flip} }; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S7_INPUT_PORTS_START(name, balls) S7_INPUT_PORTS_END


/*---------------------------------
/ Black Knight - Sys.7 (Game #500)
/---------------------------------*/
INITGAME(bk,FLIP_SW(FLIP_L),3)
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
CORE_GAMEDEF(bk,l4,"Black Knight (L-4)",1980,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Cosmic Gunfight - Sys.7 (Game #502)
/-----------------------------------*/
INITGAME(csmic,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(csmic,l1, "ic14.716",   0xac66c0dc,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xa259eba0)
S67S_SOUNDROMS8(          "sound12.716",0xaf41737b)
S7_ROMEND
CORE_GAMEDEF(csmic,l1,"Cosmic Gunfight (L-1)",1980,"Williams",s7_mS7,GAME_NO_SOUND)


/*--------------------------------
/ Jungle Lord - Sys.7 (Game #503)
/--------------------------------*/
INITGAME(jngld,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(jngld,l2, "ic14.716",   0x6e5a6374,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x4714b1f1)
S67S_SOUNDROMS8(          "sound12.716",0x55a10d13)
S67S_SPEECHROMS000x(      "speech7.532",0x83ffb695,
			  "speech5.532",0x754bd474,
			  "speech6.532",0xf2ac6a52)
S7_ROMEND
CORE_GAMEDEF(jngld,l2,"Jungle Lord (L-2)",1981,"Williams",s7_mS7S,0)

/*--------------------------------
/ Pharaoh - Sys.7 (Game #504)
/--------------------------------*/
INITGAME(pharo,FLIP_SW(FLIP_L),3)
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
CORE_GAMEDEF(pharo,l2,"Pharaoh (L-2)",1981,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Solar Fire - Sys.7 (Game #507)
/-----------------------------------*/
INITGAME(solar,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(solar,l2, "ic14.716",   0xcec19a55,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xb667ee32)
S67S_SOUNDROMS8(          "sound12.716",0x05a2230c)
S7_ROMEND
CORE_GAMEDEF(solar,l2,"Solar Fire (L-2)",1981,"Williams",s7_mS7S,0)


/*-------------------------------
/ Hyperball - Sys.7 - (Game #509)
/-------------------------------*/
INITGAME(hypbl,FLIP_SW(FLIP_L),3)
S7_ROMSTART000x(hypbl,l4, "ic14.532",0x8090fe71,
                          "ic17.532",0x6f4c0c4c,
                          "ic20.532",0xd13962e8)
S7_ROMEND
CORE_GAMEDEF(hypbl,l4,"HyperBall (L-4)",1981,"Williams",s7_mS7,GAME_NOT_WORKING)

/*----------------------------
/ Barracora- Sys.7 (Game #510)
/----------------------------*/
INITGAME(barra,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(barra,l1, "ic14.716",   0x522e944e,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x2a0e0171)
S67S_SOUNDROMS8(          "sound12.716",0x67ea12e7)
S7_ROMEND
CORE_GAMEDEF(barra,l1,"Barracora (L-1)",1981,"Williams",s7_mS7S,0)

/*----------------------------
/ Varkon- Sys.7 (Game #512)
/----------------------------*/
INITGAME(vrkon,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(vrkon,l1, "ic14.716",   0x3baba324,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xdf20330c)
S67S_SOUNDROMS8(          "sound12.716",0xd13db2bb)
S7_ROMEND
CORE_GAMEDEF(vrkon,l1,"Varkon (L-1)",1982,"Williams",s7_mS7S,0)

/*----------------------------------
/ Time Fantasy- Sys.7 (Game #515)
/----------------------------------*/
// MOVED TO tmfnt.c

/*----------------------------
/ Warlok- Sys.7 (Game #516)
/----------------------------*/
//Emulation locks up with sound running, so we'll disable it for now! 
//(NOT SURE IF THIS STILL HAPPENS?)
INITGAME(wrlok,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(wrlok,l3, "ic14.716",   0x291be241,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x44f8b507)
S67S_SOUNDROMS8(          "sound12.716",0x5d8e46d6)
S7_ROMEND
CORE_GAMEDEF(wrlok,l3,"Warlok (L-3)",1982,"Williams",s7_mS7S,0)
//CORE_GAMEDEF(wrlok,l3,"Warlok (L-3)",1982,"Williams",s7_mS7,GAME_NO_SOUND)

/*----------------------------
/ Defender - Sys.7 (Game #517)
/----------------------------*/
INITGAME(dfndr,FLIP_SWNO(0,S7_SWNO(59)),3)
S7_ROMSTART000x(dfndr,l4, "ic14.532",   0x959ec419,
                          "ic17.532",   0xbb571a17,
                          "ic20.532",   0xe99e64a2)
S67S_SOUNDROMS8(          "sound12.716",0xcabaec58)
S7_ROMEND
CORE_GAMEDEF(dfndr,l4,"Defender (L-4)",1982,"Williams",s7_mS7S,0)

/*---------------------------
/ Joust - Sys.7 (Game #519)
/--------------------------*/
INITGAME(jst,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(jst,l2, "ic14.716",   0xc4cae4bf,
                        "ic17.532",   0xbb571a17,
                        "ic20.716",   0xdfb4b75a,
                        "ic26.716",   0x63eea5d8)
S67S_SOUNDROMS0(        "sound12.532",0x3bbc90bf)
S7_ROMEND
CORE_GAMEDEF(jst,l2,"Joust (L-2)",1983,"Williams",s7_mS7S,0)


/*---------------------------
/ Laser Cue - Sys.7 (Game #520)
/--------------------------*/
INITGAME(lsrcu,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(lsrcu,l2, "ic14.716",   0x39fc350d,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0xdb4a09e7)
S67S_SOUNDROMS8(          "sound12.716",0x1888c635)
S7_ROMEND
CORE_GAMEDEF(lsrcu,l2,"Laser Cue (L-2)",1983,"Williams",s7_mS7S,0)

/*--------------------------------
/ Firepower II- Sys.7 (Game #521)
/-------------------------------*/
INITGAME(fpwr2,FLIP_SW(FLIP_L),3)
S7_ROMSTART8088(fpwr2,l2, "ic14.716",   0xa29688dd,
                          "ic17.532",   0xbb571a17,
                          "ic20.716",   0xdfb4b75a,
                          "ic26.716",   0x1068939d)
S67S_SOUNDROMS8(          "sound12.716",0x55a10d13)
S7_ROMEND
CORE_GAMEDEF(fpwr2,l2,"Firepower II (L-2)",1983,"Williams",s7_mS7S,0)

/*-----------------------------
/ Starlight - Sys.7 (Game #530)
/-----------------------------*/
INITGAME(strlt,FLIP_SW(FLIP_L),3)
//S7_ROMSTART000x (strlt,l1,	"ic14.bin",0x0638e3bad,
//							"ic17.bin",0x0a43d8518,
//							"ic20.bin",0x0b750ddc0)
S7_ROMSTART8088(strlt,l1,"ic14.716",    0x00000000,
                         "ic17.532",    0x00000000,
                         "ic20.716",    0x00000000,
                         "ic26.716",    0x00000000)
S67S_SOUNDROMS8(         "sound12.532", 0x00000000)
S7_ROMEND
CORE_GAMEDEF(strlt,l1,"Starlight (L-1)",1984,"Williams",s7_mS7S,0)
