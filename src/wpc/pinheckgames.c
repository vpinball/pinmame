// license:BSD-3-Clause

#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "pinheck.h"

static core_tLCDLayout pinheck_disp[] = {
  {0, 0, PINHECK_VIDEO_H, PINHECK_VIDEO_W, CORE_VIDEO, (genf *)pinheck_video, NULL}, {0}
};

/* data: the game's width, height, aligned, servoMin, servoMax, rgbInverted, inService, dmdHub, bootHold (pinheck_tGameData) */
#define INIT_PINHECK(name, balls, version, data) \
PINHECK_INPUT_PORTS_START(name, balls) PINHECK_INPUT_PORTS_END \
static pinheck_tGameData name##GameData = { { GEN_PINHECK, pinheck_disp, {FLIP_SWNO(PINHECK_SWLFLIP, PINHECK_SWRFLIP), 0, 1, PINHECK_CUSTSOLS, SNDBRD_NONE, 0, version, 0, pinheck_getsol} }, data }; \
static void init_##name(void) { core_gameData = &name##GameData.core; }

/*-------------------------------------------------------------------
/ pinHeck system: the Parallax Propeller mask ROM (not a game)
/-------------------------------------------------------------------*/
INIT_PINHECK(pinheck, 3, 0, PINHECK_DOMINOS_DATA)
PINHECK_BIOS_ROMSTART(pinheck)
PINHECK_ROMEND
GAMEX(2014,pinheck,0,PINHECK,pinheck,pinheck,ROT0,"Spooky Pinball","pinHeck System",NOT_A_DRIVER)

/* Domino's Spectacular Pinball Adventure: sims/pinheck/dominos.c */
/* Rob Zombie's Spookshow International: sims/pinheck/rzspook.c */
/* The Jetsons: sims/pinheck/jetsons.c */
/* America's Most Haunted: sims/pinheck/amh.c */
