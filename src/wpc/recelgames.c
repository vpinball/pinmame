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
   own results land on player 2 as the manual says they do. But the "six
   digits" are the counter unit's own six 7448-driven positions (×1 .. ×100
   000, system3-operation-maintenance.md §5.4); the GPKD only multiplexes
   five of them (×10 .. ×100 000, docs/gpkd-protocol.md §11.4) plus a
   *separate* status-LED nibble on the same scan column as the counter's
   decimal point/LEDs, latched raw rather than run through a 7448 (§6). That
   nibble is not the counter's missing ×1 digit -- treating it as one is a
   real bug, not a rendering choice, and is what used to make the ball/tilt/
   game-over latch (see below) render as a flashing numeral. So each score
   field below is 5 digits, and the status nibble is exposed as a lamp
   instead (gpkd_refresh() in recel.c). The rest of the allocation is
   docs/gpkd-protocol.md §8's, and one entry of it is measured: inserting
   coins one at a time moves scan time B1 and nothing else in all 32
   positions, which puts credit on row 4's indicator pair and so player 4,
   not player 3, on B7..B2. Which digit of that pair is the tens is still
   open (§11.1). A9/A8 (match number and ball/tilt/game-over) are likewise
   latched, not digits (§6), and are exposed as lamps rather than laid out
   here; which of the two is which is still open (§11.6). B9/B8 drive no
   indicator on a real machine (§4) and are not modelled at all. */
#define RECEL_D(row, col, pos) {row, col, pos, 1, CORE_SEG7},
/* 5 GPKD-multiplexed score digits, MSD first; base = the ×10 digit's GPKD
   position (docs/gpkd-protocol.md §11.4 -- the ×1 digit is not multiplexed
   and is not modelled). */
#define RECEL_COUNTER(row, col, base) \
  RECEL_D(row, col,    (base)+4) RECEL_D(row, (col)+2,  (base)+3) \
  RECEL_D(row, (col)+4,(base)+2) RECEL_D(row, (col)+6,  (base)+1) \
  RECEL_D(row, (col)+8,(base))

static core_tLCDLayout recel_disp[] = {
  /* row 1: player 1 (A7..A3), then extra games / extra balls (A1, A0).
     A2 (status LEDs) is a lamp, not a digit -- see the header comment. */
  RECEL_COUNTER(0, 0, 3)  RECEL_D(0, 12, 1) RECEL_D(0, 14, 0)
  /* row 2: player 2 (AF..AB) -- the self-check display. AA (status) is a
     lamp; A9/A8 (match, ball/tilt/game-over) are lamps, not laid out here. */
  RECEL_COUNTER(2, 0, 11)
  /* row 3: player 3 (BF..BB). BA (status) is a lamp; B9/B8 have no
     indicator on a real machine (§4) and are not modelled. */
  RECEL_COUNTER(4, 0, 27)
  /* row 4: player 4 (B7..B3), then the two credit digits (B1, B0).
     B2 (status) is a lamp. */
  RECEL_COUNTER(6, 0, 19) RECEL_D(6, 12, 17) RECEL_D(6, 14, 16)
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
RECEL_BIOS_ROMSTART(recel)
RECEL_ROMEND
GAMEX(1978,recel,0,RECEL,recel,recel,ROT0,"Recel","System III",NOT_A_DRIVER)

/*-------------------------------------------------------------------
/ Fair Fight (1978) - model 1.053, doc 035-623
/-------------------------------------------------------------------*/
INIT_RECEL(r_fairfght, recel_disp, 1)
RECEL_ROMSTART(r_fairfght, "fa.c5", 0x0100, CRC(5d3694da) SHA1(4d0a8033acb6ef2e2af107f76540fd19b4a39b12))
RECEL_ROMEND
CORE_CLONEDEFNV(r_fairfght,recel,"Fair Fight",1978,"Recel",gl_mRECEL,GAME_IMPERFECT_SOUND)
