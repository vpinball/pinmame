#include "driver.h"
#include "sim.h"
#include "atari.h"

#define FLIPSW1920 FLIP_SWNO(19,20)
#define FLIPSW6667 FLIP_SWNO(66,67)

#define INITGAME1(name, disptype, flippers, balls) \
	ATARI1_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME2(name, disptype, flippers, balls) \
	ATARI2_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define DISP_SEG_6(row,col,type) {4*row,16*col,row*20+col*8+2,6,type}

/* 4 X 6 Segments, 2 X 2 Segments */
core_tLCDLayout atari_disp[] = {
  DISP_SEG_6(0,0,CORE_SEG7), DISP_SEG_6(0,1,CORE_SEG7),
  DISP_SEG_6(1,0,CORE_SEG7), DISP_SEG_6(1,1,CORE_SEG7),
  DISP_SEG_CREDIT(42,43,CORE_SEG7), DISP_SEG_BALLS(46,47,CORE_SEG7), {0}
};

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ The Atarians (11/1976)
/-------------------------------------------------------------------*/
INITGAME1(atarians, atari_disp, FLIPSW1920, 1)
ATARI_2_ROMSTART(atarians,	"atarian.e0",	0x45cb0427,
							"atarian.e00",	0x6066bd63)
ATARI_ROMEND
CORE_GAMEDEFNV(atarians,"The Atarians",1976,"Atari",gl_mATARI1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Time 2000 (06/1977)
/-------------------------------------------------------------------*/
INITGAME1(time2000, atari_disp, FLIPSW1920, 1)
ATARI_2_ROMSTART(time2000,	"time.e0",	0x1e79c133,
							"time.e00",	0xe380f35c)
ATARI_ROMEND
CORE_GAMEDEFNV(time2000,"Time 2000",1977,"Atari",gl_mATARI1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Airborne Avenger (09/1977)
/-------------------------------------------------------------------*/
INITGAME1(aavenger, atari_disp, FLIPSW1920, 1)
ATARI_2_ROMSTART(aavenger,	"airborne.e0",	0x44e67c54,
							"airborne.e00",	0x05ac26b8)
ATARI_ROMEND
CORE_GAMEDEFNV(aavenger,"Airborne Avenger",1977,"Atari",gl_mATARI1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Middle Earth (02/1978)
/-------------------------------------------------------------------*/
INITGAME1(midearth, atari_disp, FLIPSW1920, 1)
ATARI_2_ROMSTART(midearth,	"608.bin",	0x28b92faf,
							"609.bin",	0x589df745)
ATARI_ROMEND
CORE_GAMEDEFNV(midearth,"Middle Earth",1978,"Atari",gl_mATARI1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Space Riders (09/1978)
/-------------------------------------------------------------------*/
INITGAME1(spcrider, atari_disp, FLIPSW1920, 1)
ATARI_2_ROMSTART(spcrider,	"spacel.bin",	0x66ffb04e,
							"spacer.bin",	0x3cf1cd73)
ATARI_ROMEND
CORE_GAMEDEFNV(spcrider,"Space Riders",1978,"Atari",gl_mATARI1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Superman (03/1979)
/-------------------------------------------------------------------*/
INITGAME2(superman, atari_disp, FLIPSW6667, 1)
ATARI_3_ROMSTART(superman,	"supmn_k.rom",	0xa28091c2,
							"supmn_m.rom",	0x1bb6b72c,
							"supmn_j.rom",	0x26521779)
ATARI_ROMEND
CORE_GAMEDEFNV(superman,"Superman",1979,"Atari",gl_mATARI2,0)

/*-------------------------------------------------------------------
/ Hercules (05/1979)
/-------------------------------------------------------------------*/
INITGAME2(hercules, atari_disp, FLIPSW6667, 1)
ATARI_3_ROMSTART(hercules,	"herc_k.rom",	0x65e099b1,
							"supmn_m.rom",	0x1bb6b72c,
							"supmn_j.rom",	0x26521779)
ATARI_ROMEND
CORE_CLONEDEFNV(hercules,superman,"Hercules",1979,"Atari",gl_mATARI2,0)

//Road Runner (1979)
//Monza (1980)
//Neutron Star (1981)
//4x4 (1983)
//Triangle (19??)
