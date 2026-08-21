// license:BSD-3-Clause

#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "rfranco.h"
#include "sndbrd.h"

#define GEN_RFRANCO 0

#define INITGAME(name, disptype, balls, lamps) \
  RFRANCO_INPUT_PORTS_START(name, balls) RFRANCO_INPUT_PORTS_END \
  static core_tGameData name##GameData = \
    {GEN_RFRANCO, disptype, {FLIP_SW(FLIP_L), 0, lamps, 0, SNDBRD_NONE}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
    /* DRIVER_INIT runs once per game start, after the ROM regions are loaded \
       and before the machine is reset. That is the only correct place for the \
       sound ROM descramble: the regions are reloaded on every start, so a \
       once-per-process guard would leave a second game inside the same \
       VPinMAME/libpinmame process running a scrambled image, and MACHINE_INIT \
       also runs on soft reset and would reverse it back. */ \
    rfranco_unscramble_sound_rom(); \
  }

/* Display board 53/3307: 30 HDSP-3400 digits behind an 8279, a 74159 digit
   select and two 7447 segment decoders. Sixteen scan positions each drive an
   anode pair - one digit from D1..D14 (7447 IC5, the display byte's low
   nibble) and one from D15..D30 (IC7, high nibble). Players 1 and 3 take the
   low nibble, 2 and 4 the high one; players 1-2 use even RAM addresses and 3-4
   odd ones. Address 0/1 is the least significant digit and is the fixed
   trailing zero, since the smallest playfield award is 10 points.

   Segment indices: 0-6 player 1, 8-14 player 2, 16-22 player 3, 24-30 player 4,
   32/33 credits tens/units. Row and column positions are cosmetic, taken from
   the component placement drawing. There is no ball-in-play display on this
   board - 4x7 score digits plus 2 credit digits is exactly 30. */
/* `left` is in HALF-digit columns: core.c:1462 halves the position while
   core.c:1525 advances a whole cols+1 per digit, so a 7-digit score occupies 14
   columns, not 7 - and a 2-digit credit display occupies 4.

   The right-hand group used to sit at 18, which is correct for the 8-digit
   layouts this was adapted from but leaves only columns 14-17 free, and credits
   was placed at 15 as if digits were one column wide. Its second digit landed on
   player 4's first, and player 4 - drawn later - erased the overlapping strokes.

   Moving the right-hand group out to 20 opens a six-column gap and lets credits
   sit at 15 with clear air on both sides, which is what the machine looks like:
   the credits are a separate two-digit display, not part of a score. */
static core_tLCDLayout rfrancoDisp[] = {
  { 0, 0,  0, 7, CORE_SEG7},   /* player 1  - top left     */
  { 0,20, 16, 7, CORE_SEG7},   /* player 3  - top right    */
  { 3, 0,  8, 7, CORE_SEG7},   /* player 2  - bottom left  */
  { 3,15, 32, 2, CORE_SEG7},   /* credits   - centre       */
  { 3,20, 24, 7, CORE_SEG7},   /* player 4  - bottom right */
  {0}
};

/*-------------------------------------------------------------------
/ Super Star (1986)
/-------------------------------------------------------------------*/
INITGAME(supstarf, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarf,
  "m31-a-01187.ic19", CRC(ab8b1148) SHA1(496d3c9664386ae64e94462db2fdd36811a68a87),
  "2532.ic4",         CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_GAMEDEFNV(supstarf, "Super Star", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)

/* Rev. 2 - the newer firmware, and named so: it is a true revision, not a
   variant, carrying real fixes (the sender's HLT wake race replaced by a
   bounded spin, the stuck-contact watchdog) on top of rev. 1. It extends the operator menu from 9 adjustment zones to
   25. Its sound ROM is the same 2532 image: the dump taken alongside this game
   ROM had data line D5 stuck high, and clearing that bit reproduces the good
   dump exactly across all 4096 bytes, so the physical part held this content.
   MAME still carries its own copy flagged BAD_DUMP. */
INITGAME(supstarfa, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarfa,
  "27c128.ic19", CRC(9a440461) SHA1(e2f8dcf95084f755d3a34d77ba2649602687a610),
  "2532.ic4",    CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_CLONEDEFNV(supstarfa, supstarf, "Super Star (rev. 2)", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)
