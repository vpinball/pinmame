#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "recel.h"

/* Lite box layout. coreGlobals.segments keeps the 10788's own index -- scan
   time, group A at 0-15 and group B at 16-31 (docs/gpkd-protocol.md §4) --
   but the *display* runs the other way: the pointer starts at time 15 and
   walks down, so time 15 is a field's leftmost digit. Two independent
   readings of the same ROM say so. The self-check writes its result digits
   at times 15, 13 and 11 and the manual reads that back as "X.Y.Z", e.g.
   "9.8.7" for a RAM fault at address 87 (system3-operation-maintenance.md
   §3.2). And a score written from time 2 upward only reads correctly in
   that direction: three hits on Fair Fight's 500-point target walk group A
   times 7..3 through 00050, 00100, 00150. PinMAME layouts only run left to
   right, so each field is spelled out a digit at a time, descending.

   §7.4's lite box is four rows of a six-digit counter plus a pair of small
   indicators, which is exactly the 8 + 8 nibbles of one group: group A
   carries players 1 and 2, group B players 3 and 4, and the self-check's
   own results land on player 2 as the manual says they do. Which of each
   indicator pair is which is not established -- §7.4 names them but not
   their order. */
#define RECEL_D(row, col, pos) {row, col, pos, 1, CORE_SEG7},
#define RECEL_COUNTER(row, col, base) \
  RECEL_D(row, col,    (base)+5) RECEL_D(row, (col)+2,  (base)+4) \
  RECEL_D(row, (col)+4,(base)+3) RECEL_D(row, (col)+6,  (base)+2) \
  RECEL_D(row, (col)+8,(base)+1) RECEL_D(row, (col)+10, (base))

static core_tLCDLayout recel_disp[] = {
  /* row 1: player 1 (A7..A2), then extra games / extra balls (A1, A0) */
  RECEL_COUNTER(0, 0, 2)  RECEL_D(0, 14, 1) RECEL_D(0, 16, 0)
  /* row 2: player 2 (AF..AA) -- the self-check display -- then the A9/A8 pair */
  RECEL_COUNTER(2, 0, 10) RECEL_D(2, 14, 9) RECEL_D(2, 16, 8)
  /* row 3: player 3 (B7..B2), then B1/B0 (no indicator fitted, §7.4) */
  RECEL_COUNTER(4, 0, 18) RECEL_D(4, 14, 17) RECEL_D(4, 16, 16)
  /* row 4: player 4 (BF..BA), then the two credit digits (B9, B8) */
  RECEL_COUNTER(6, 0, 26) RECEL_D(6, 14, 25) RECEL_D(6, 16, 24)
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
CORE_CLONEDEFNV(r_fairfght,recel,"Fair Fight",1978,"Recel",gl_mRECEL,GAME_IMPERFECT_SOUND)
