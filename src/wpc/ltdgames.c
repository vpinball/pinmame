#include "driver.h"
#include "sim.h"
#include "ltd.h"

#define INITGAME(name, disptype, balls) \
	LTD_INPUT_PORTS_START(name, balls) LTD_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{FLIP_SW(FLIP_L),0,-1}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME2(name, disptype, balls) \
	LTD_INPUT_PORTS_START(name, balls) LTD_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{FLIP_SW(FLIP_L),0,6}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

core_tLCDLayout ltd_disp[] = {
  { 0, 0, 0,10,CORE_SEG7 },
  { 2, 0,10,10,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Atlantis
/-------------------------------------------------------------------*/
core_tLCDLayout atla_disp[] = {
  { 0, 0,14,2,CORE_SEG7 }, { 0, 4,18,2,CORE_SEG7 }, { 0, 8,10,1,CORE_SEG7 },
  { 0,12,12,2,CORE_SEG7 }, { 0,16,16,2,CORE_SEG7 }, { 0,20,11,1,CORE_SEG7 },
  { 3, 8, 2,1,CORE_SEG7 }, { 3,12, 3,1,CORE_SEG7 },
  { 3,17, 8,1,CORE_SEG7 }, { 3,20, 9,1,CORE_SEG7 },
  {0}
};
INITGAME(atla_ltd, atla_disp, 1)
LTD_2_ROMSTART(atla_ltd, "atlantis.bin", CRC(c61be043) SHA1(e6c4463f59a5743fa34aa55beeb6f536ad9f1b56))
LTD_ROMEND
CORE_GAMEDEFNV(atla_ltd,"Atlantis (LTD)",19??,"LTD",gl_mLTD,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Black Hole
/-------------------------------------------------------------------*/
core_tLCDLayout bhol_disp[] = {
  { 0, 0,10,2,CORE_SEG7 }, { 0, 4,14,2,CORE_SEG7 }, { 0, 8,18,2,CORE_SEG7 },
  { 0,14, 8,2,CORE_SEG7 }, { 0,18,12,2,CORE_SEG7 }, { 0,22,16,2,CORE_SEG7 },
  { 3,10, 2,1,CORE_SEG7 }, { 3,14, 3,1,CORE_SEG7 },
  {0}
};
INITGAME(bhol_ltd, bhol_disp, 1)
LTD_2_ROMSTART(bhol_ltd, "blackhol.bin", CRC(9f6ae35e) SHA1(c17bf08a41c6cf93550671b0724c58e8ac302c33))
LTD_ROMEND
CORE_GAMEDEFNV(bhol_ltd,"Black Hole (LTD)",19??,"LTD",gl_mLTD,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Pec Man
/-------------------------------------------------------------------*/
INITGAME(pecman, ltd_disp, 1)
LTD_2_ROMSTART(pecman, "pecman.bin", NO_DUMP)
LTD_ROMEND
CORE_GAMEDEFNV(pecman,"Pec Man",19??,"LTD",gl_mLTD,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Zephy
/-------------------------------------------------------------------*/
core_tLCDLayout zephy_disp[] = {
  { 0, 0,14,6,CORE_SEG7 }, { 0,14, 8,6,CORE_SEG7 }, { 3, 0, 2,6,CORE_SEG7 },
  { 3,14, 1,1,CORE_SEG7 }, { 3,18, 0,1,CORE_SEG7 },
  {0}
};
INITGAME2(zephy, zephy_disp, 1)
LTD_4_ROMSTART(zephy, "zephy.l2", CRC(8dd11287) SHA1(8133d0c797eb0fdb56d83fc55da91bfc3cddc9e3))
LTD_ROMEND
CORE_GAMEDEFNV(zephy,"Zephy",19??,"LTD",gl_mLTD,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Cowboy
/-------------------------------------------------------------------*/
INITGAME(cowboy, ltd_disp, 1)
LTD_44_ROMSTART(cowboy, "cauboy.l",     CRC(87befe2a) SHA1(93fdf40b10e53d7d95e5dc72923b6be887411fc0),
                        "cauboy.h",     CRC(105e5d7b) SHA1(75edeab8c8ba19f334479133802acbc25f405763))
LTD_ROMEND
CORE_GAMEDEFNV(cowboy,"Cowboy",19??,"LTD",gl_mLTD4,GAME_NOT_WORKING|GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Columbia
/-------------------------------------------------------------------*/
INITGAME(columbia, ltd_disp, 1)
LTD_44_ROMSTART(columbia, "columb-d.bin", NO_DUMP,
                          "columb-e.bin", CRC(013abca0) SHA1(269376af92368d214c3d09ec6d3eb653841666f3))
LTD_ROMEND
CORE_GAMEDEFNV(columbia,"Columbia",19??,"LTD",gl_mLTD4,GAME_NOT_WORKING|GAME_NO_SOUND)
