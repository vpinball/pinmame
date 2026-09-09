#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "recel.h"

/* GPKD position layout: canonical position = 16*group + scan time (group A =
   0-15, group B = 16-31). docs/gpkd-protocol.md §4, §8.
   Group A (upper line): free play/extra ball, player 1, ball/match, player 2.
   Group B (lower line): credit, player 4, unused, player 3. */
static core_tLCDLayout recel_disp[] = {
  {0, 0,  0,2,CORE_SEG7}, /* A0-A1: free play, extra ball */
  {0, 4,  2,6,CORE_SEG7}, /* A2-A7: player 1, status nibble + score */
  {0,12,  8,1,CORE_SEG7}, /* A8: ball in play / game over / tilt */
  {0,14,  9,1,CORE_SEG7}, /* A9: match number */
  {0,16, 10,6,CORE_SEG7}, /* AA-AF: player 2, status nibble + score */
  {2, 0, 16,2,CORE_SEG7}, /* B0-B1: credit (digit order unverified, §11.1) */
  {2, 4, 18,6,CORE_SEG7}, /* B2-B7: player 4, status nibble + score */
  {2,12, 24,2,CORE_SEG7}, /* B8-B9: no equivalent on a real machine, §4 */
  {2,16, 26,6,CORE_SEG7}, /* BA-BF: player 3, status nibble + score */
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
