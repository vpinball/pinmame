#include "driver.h"
#include "sim.h"
#include "GTS3.h"

/* NO OUTPUT */
core_tLCDLayout GTS3_NoOutput[] = {{0}};

#define INITGAME(name, balls) \
static core_tGameData name##GameData = {0,0}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GTS3_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Cue Ball Wizard
/-------------------------------------------------------------------*/
INITGAME(cueball,3/*?*/)
GTS3ROMSTART(cueball,	"grom.rom",0x3437fdd8)
GTS3_DMD256_ROMSTART(	"dsprom.dsp",0x3cc7f470)
GTS3_ROMEND
CORE_GAMEDEFNV(cueball,"Cue Ball Wizard",1992,"Gottlieb",de_mGTS3,GAME_NOT_WORKING)
