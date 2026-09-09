#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "recel.h"

/* 4 players x 6 digits, plus credit/ball. Positions are provisional until the
   GPKD scan mapping is derived in Task 4. */
static core_tLCDLayout recel_disp[] = {
  {0, 0, 0,6,CORE_SEG7}, {0,16, 6,6,CORE_SEG7},
  {2, 0,12,6,CORE_SEG7}, {2,16,18,6,CORE_SEG7},
  {4, 8,24,2,CORE_SEG7}, {4,16,26,2,CORE_SEG7},
  {0}
};

#define INIT_RECEL(name, dsp, hwver) \
RECEL_INPUT_PORTS_START(name, 1) RECEL_INPUT_PORTS_END \
static core_tGameData name##GameData = { \
  GEN_RECEL, dsp, {FLIP_SW(FLIP_L),0,0,0,SNDBRD_NONE,0,hwver}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*-------------------------------------------------------------------
/ Recel System III shared program (not a game)
/-------------------------------------------------------------------*/
INIT_RECEL(recel, recel_disp, 1)
RECEL_ROMSTART(recel, "fa.c5", 0x0100, CRC(5d3694da) SHA1(4d0a8033acb6ef2e2af107f76540fd19b4a39b12))
RECEL_ROMEND
GAMEX(1978,recel,0,RECEL,recel,recel,ROT0,"Recel","System III",NOT_A_DRIVER)

/*-------------------------------------------------------------------
/ Fair Fight (1978) - model 1.053, doc 035-623
/-------------------------------------------------------------------*/
INIT_RECEL(r_fairfght, recel_disp, 1)
RECEL_ROMSTART(r_fairfght, "fa.c5", 0x0100, CRC(5d3694da) SHA1(4d0a8033acb6ef2e2af107f76540fd19b4a39b12))
RECEL_ROMEND
CORE_CLONEDEFNV(r_fairfght,recel,"Fair Fight",1978,"Recel",gl_mRECEL,GAME_NOT_WORKING)
