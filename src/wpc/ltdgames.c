#include "driver.h"
#include "sim.h"
#include "ltd.h"

#define INITGAME(name, disptype, balls) \
	LTD_INPUT_PORTS_START(name, balls) LTD_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* 4 X 6 Segments, 2 X 2 Segments */
core_tLCDLayout ltd_disp[] = {
  { 0, 0, 0,16, CORE_SEG7 },
  { 2, 0,16,16, CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Black Hole
/-------------------------------------------------------------------*/
INITGAME(bhol_ltd, ltd_disp, 1)
LTD_2_ROMSTART(bhol_ltd, "blackhol.bin", CRC(9f6ae35e) SHA1(c17bf08a41c6cf93550671b0724c58e8ac302c33))
LTD_ROMEND
CORE_GAMEDEFNV(bhol_ltd,"Black Hole (LTD)",19??,"LTD",gl_mLTD,0)

/*-------------------------------------------------------------------
/ Vulcano
/-------------------------------------------------------------------*/
INITGAME(vulcano, ltd_disp, 1)
LTD_2_ROMSTART(vulcano, "vulcano.bin", CRC(5931c6f7) SHA1(e104a6c3ca2175bb49199e06963e26185dd563d2))
LTD_ROMEND
CORE_GAMEDEFNV(vulcano,"Vulcano",19??,"LTD",gl_mLTD,0)

/*-------------------------------------------------------------------
/ Zephy
/-------------------------------------------------------------------*/
INITGAME(zephy, ltd_disp, 1)
LTD_4_ROMSTART(zephy, "zephy.l2", CRC(8dd11287) SHA1(8133d0c797eb0fdb56d83fc55da91bfc3cddc9e3))
LTD_ROMEND
CORE_GAMEDEFNV(zephy,"Zephy",19??,"LTD",gl_mLTD,0)

/*-------------------------------------------------------------------
/ Cauboy
/-------------------------------------------------------------------*/
INITGAME(cauboy, ltd_disp, 1)
LTD_44_ROMSTART(cauboy, "cauboy.l", CRC(87befe2a) SHA1(93fdf40b10e53d7d95e5dc72923b6be887411fc0),
                        "cauboy.h", CRC(105e5d7b) SHA1(75edeab8c8ba19f334479133802acbc25f405763))
LTD_ROMEND
CORE_GAMEDEFNV(cauboy,"Cauboy",19??,"LTD",gl_mLTD,0)
