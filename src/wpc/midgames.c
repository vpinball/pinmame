#include "driver.h"
#include "sim.h"
#include "midway.h"

#define INITGAME(name, disptype, lamps, flippers, balls) \
	MIDWAY_INPUT_PORTS_START(name, balls) MIDWAY_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,lamps}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/*-------------------------------------------------------------------
/ Flicker (Prototype, 11/1974)
/-------------------------------------------------------------------*/
core_tLCDLayout flicker_disp[] = {
  {0, 0, 0, 5,CORE_SEG7}, {0,12, 7, 5,CORE_SEG7},
  {2, 3, 5, 2,CORE_SEG7}, {2, 9,12, 2,CORE_SEG7}, {2,15,14, 2,CORE_SEG7},
  {0}
};
INITGAME(flicker, flicker_disp, 0, FLIP_SW(FLIP_L), 1)
MIDWAY_1_ROMSTART(flicker,"flicker.rom", 0x831041cd)
MIDWAY_ROMEND
CORE_GAMEDEFNV(flicker,"Flicker (Prototype)",1974,"Nutting Associates",gl_mMIDWAYP,GAME_USES_CHIMES|GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Rotation VIII (09/1978)
/-------------------------------------------------------------------*/
core_tLCDLayout rot_disp[] = {
  {0, 0, 0, 6,CORE_SEG7}, {0,16, 7, 6,CORE_SEG7},
  {2, 0,14, 6,CORE_SEG7}, {2,16,21, 6,CORE_SEG7},
  {4, 0,28, 2,CORE_SEG7}, {4, 8,35, 2,CORE_SEG7},
  {4,16,32, 2,CORE_SEG7}, {4,24,39, 2,CORE_SEG7},
  {0}
};
INITGAME(rotation, rot_disp, -1, FLIP_SW(FLIP_L), 1)
MIDWAY_3_ROMSTART(rotation,	"rot-a117.dat",	0x7bb6beb3,
							"rot-b117.dat",	0x538e37b2,
							"rot-c117.dat",	0x3321ff08)
MIDWAY_ROMEND
CORE_GAMEDEFNV(rotation,"Rotation VIII",1978,"Midway",gl_mMIDWAY,0)
