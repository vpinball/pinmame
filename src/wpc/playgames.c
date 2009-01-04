#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "play.h"

#define GEN_PLAYMATIC 0

#ifdef MAME_DEBUG
#define GAME_STATUS 0
#else
#define GAME_STATUS GAME_NOT_WORKING|GAME_NO_SOUND
#endif

#define INITGAME1(name, disptype, balls) \
    PLAYMATIC1_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,-1}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME(name, disptype, balls) \
    PLAYMATIC_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,8}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME3(name, disptype, balls) \
	PLAYMATIC3_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,8}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME4(name, disptype, balls) \
	PLAYMATIC4_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,8}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

core_tLCDLayout play_dispOld[] = {
  { 0, 0, 0,6,CORE_SEG7 }, { 0,14, 6,4,CORE_SEG7 }, { 0,22, 4,2,CORE_SEG7 },
  { 2, 0,10,4,CORE_SEG7 }, { 2, 8, 4,2,CORE_SEG7 }, { 2,14,14,4,CORE_SEG7 }, { 2,22, 4,2,CORE_SEG7 },
  { 4, 8,18,2,CORE_SEG7 }, { 4,14,20,3,CORE_SEG7 },
  {0}
};

core_tLCDLayout play_disp6[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 },
  { 3, 0,29,1,CORE_SEG7 }, { 3, 2,24,5,CORE_SEG7 },
  { 6, 0,21,1,CORE_SEG7 }, { 6, 2,16,5,CORE_SEG7 },
  { 9, 0,13,1,CORE_SEG7 }, { 9, 2, 8,5,CORE_SEG7 },
  {12, 0, 5,1,CORE_SEG7 }, {12, 2, 0,2,CORE_SEG7 }, {12, 8, 3,2,CORE_SEG7 },
  {0}
};

core_tLCDLayout play_disp7[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 }, { 0,12,52,1,CORE_SEG7 },
  { 6, 0,29,1,CORE_SEG7 }, { 6, 2,24,5,CORE_SEG7 }, { 6,12,51,1,CORE_SEG7 },
  { 0,20,21,1,CORE_SEG7 }, { 0,22,16,5,CORE_SEG7 }, { 0,32,50,1,CORE_SEG7 },
  { 6,20,13,1,CORE_SEG7 }, { 6,22, 8,5,CORE_SEG7 }, { 6,32,49,1,CORE_SEG7 },
  { 3,10, 5,1,CORE_SEG7 }, { 3,12, 0,1,CORE_SEG7 }, { 3,16, 2,1,CORE_SEG7 }, { 3,20, 4,1,CORE_SEG7 }, { 3,22,48,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Last Lap (1978)
/-------------------------------------------------------------------*/
INITGAME1(lastlap, play_dispOld, 1)
PLAYMATIC_ROMSTART88(lastlap,	"lastlapa.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
						"lastlapb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(lastlap,"Last Lap",1978,"Playmatic",gl_mPLAYMATIC,0)

/*-------------------------------------------------------------------
/ Antar (1979)
/-------------------------------------------------------------------*/
INITGAME(antar, play_disp6, 1)
PLAYMATIC_ROMSTART8888(antar,	"antar08.bin", CRC(f6207f77) SHA1(f68ce967c6189457bd0ce8638e9c477f16e65763),
						"antar09.bin", CRC(2c954f1a) SHA1(fa83a5f1c269ea28d4eeff181f493cbb4dc9bc47),
						"antar10.bin", CRC(a6ce5667) SHA1(85ecd4fce94dc419e4c210262f867310b0889cd3),
						"antar11.bin", CRC(6474b17f) SHA1(e4325ceff820393b06eb2e8e4a85412b0d01a385))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(antar,"Antar",1979,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

/*-------------------------------------------------------------------
/ Evil Fight (1980)
/-------------------------------------------------------------------*/
INITGAME(evlfight, play_disp6, 1)
PLAYMATIC_ROMSTART8888(evlfight,	"evfg08.bin", CRC(2cc2e79a) SHA1(17440512c419b3bb2012539666a5f052f3cd8c1d),
						"evfg09.bin", CRC(5232dc4c) SHA1(6f95a578e9f09688e6ce8b0a622bcee887936c82),
						"evfg10.bin", CRC(de2f754d) SHA1(0287a9975095bcbf03ddb2b374ff25c080c8020f),
						"evfg11.bin", CRC(5eb8ac02) SHA1(31c80e74a4272becf7014aa96eaf7de555e26cd6))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(evlfight,"Evil Fight",1980,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

/*-------------------------------------------------------------------
/ Mad Race (1982)
/-------------------------------------------------------------------*/
INITGAME(madrace, play_disp6, 1)
PLAYMATIC_ROMSTART000(madrace,	"madrace.2a0", CRC(ab487c79) SHA1(a5df29b2af4c9d94d8bf54c5c91d1e9b5ca4d065),
								"madrace.2b0", CRC(dcb54b39) SHA1(8e2ca7180f5ea3a28feb34b01f3387b523dbfa3b),
								"madrace.2c0", CRC(b24ea245) SHA1(3f868ccbc4bfb77c40c4cc05dcd8eeca85ecd76f))
PLAYMATIC_SOUNDROM6416(	"madrace1.snd", CRC(49e956a5) SHA1(8790cc27a0fda7b8e07bee65109874140b4018a2),
						"madrace2.snd", CRC(c19283d3) SHA1(42f9770c46030ef20a80cc94fdbe6548772aa525))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(madrace,"Mad Race",1982,"Playmatic",gl_mPLAYMATIC2S,GAME_STATUS)

/*-------------------------------------------------------------------
/ Meg-Aaton (1983)
/-------------------------------------------------------------------*/
INITGAME3(megaaton, play_disp7, 1)
PLAYMATIC_ROMSTART64(megaaton,	"cpumegat.bin", CRC(7e7a4ede) SHA1(3194b367cbbf6e0cb2629cd5d82ddee6fe36985a))
PLAYMATIC_SOUNDROM6432(	"smogot.bin", CRC(92fa0742) SHA1(ef3100a53323fd67e23b47fc3e72fdb4671e9b0a),
						"smegat.bin", CRC(910ab7fe) SHA1(0ddfd15c9c25f43b8fcfc4e11bc8ea180f6bd761))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(megaaton,"Meg-Aaton",1983,"Playmatic",gl_mPLAYMATIC3S,GAME_STATUS)

/*-------------------------------------------------------------------
/ KZ-26 (1984)
/-------------------------------------------------------------------*/
INITGAME4(kz26, play_disp7, 1)
PLAYMATIC_ROMSTART64(kz26,	"kz26.cpu", CRC(8030a699) SHA1(4f86b325801d8ce16011f7b6ba2f3633e2f2af35))
PLAYMATIC_SOUNDROM6432(	"sound1.su3", CRC(f9550ab4) SHA1(7186158f515fd9fbe5a7a09c6b7d2e8dfc3b4bb2),
						"sound2.su4", CRC(b66100d3) SHA1(85f5a319715f99d1b7afeca0d01c81aa615d416a))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(kz26,"KZ-26",1984,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)
