#include "driver.h"
#include "sim.h"
#include "gts1.h"

#define INITGAME(name, disptype, flippers, balls) \
	GTS1_INPUT_PORTS_START(name, balls) GTS1_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/*-------------------------------------------------------------------
/ Cleopatra (11/1977)
/-------------------------------------------------------------------*/
core_tLCDLayout sys1_disp[] = {
  {0, 0, 0,16,CORE_SEG9},
  {2, 0,16,16,CORE_SEG9},
  {4, 0,32,16,CORE_SEG9},
  {0}
};
INITGAME(cleoptra, sys1_disp, FLIP_SW(FLIP_L), 1)
GTS1_1_ROMSTART(cleoptra,"409.cpu", 0)
GTS1_ROMEND
CORE_GAMEDEFNV(cleoptra,"Cleopatra",1977,"Gottlieb",gl_mGTS1,0)
