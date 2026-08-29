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
INITGAME(supstarf1, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarf1,
  "m31-a-01187.ic19", CRC(ab8b1148) SHA1(496d3c9664386ae64e94462db2fdd36811a68a87),
  "2532.ic4",         CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_GAMEDEFNV(supstarf1, "Super Star (rev. 1)", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)

/* TRAP re-entrancy sentinel. A new NVRAM byte at C089, and the TRAP handler bails out if it fires while the previous pass is still running.
   That single inserted byte shifts every NVRAM variable from C08A to C373 up by one, which is why the raw byte diff looks like 1502 bytes for what is a 13-instruction change */
INITGAME(supstarf2, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarf2,
  "27128Prg.bin", CRC(77c43e87) SHA1(efdf60b53ac105985ca6d4eeb6ed48b893bb7ad8),
  "2532.ic4",     CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_CLONEDEFNV(supstarf2, supstarf1, "Super Star (rev. 2)", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)

/* The sound sender stops halting: the HLT waiting for the RST5.5 reply becomes a bounded ~50-iteration spin, so a lost wake no longer wedges the machine.
   The TRAP sentinel prologue moves out of 1800 to 197F, and an explicit LXI SP,C7FF is added on the game-over/attract entry */
INITGAME(supstarf3, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarf3,
  "super.dat", CRC(51697aff) SHA1(d10c6456716ca49cce590996e7271b8cd7026f38),
  "2532.ic4",  CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_CLONEDEFNV(supstarf3, supstarf1, "Super Star (rev. 3)", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)

/* Operator menu 9 zones -> 19, plus a coin-contact conditioner, a stuck-contact watchdog, ten new gameplay settings, a new code block at 3880-3B4D,
   a changed chime ladder, and a checksum byte at 3FFF. Stack base drops C7FF -> C7CF to free C7D0-C7FF for the new variables.
   Its sound ROM is the same 2532 image: the dump taken alongside this game ROM had data line D5 stuck high, and clearing that bit reproduces the good
   dump exactly across all 4096 bytes, so the physical part held this content */
INITGAME(supstarf4, rfrancoDisp, 5, 0)
RFRANCO_ROMSTART(supstarf4,
  "27c128.ic19", CRC(9a440461) SHA1(e2f8dcf95084f755d3a34d77ba2649602687a610),
  "2532.ic4",    CRC(d6d7eee2) SHA1(60e497c8845320eea01662d894d0b16349ebb7e4))
RFRANCO_ROMEND
CORE_CLONEDEFNV(supstarf4, supstarf1, "Super Star (rev. 4)", 1986, "Recreativos Franco (Spain)", gl_mRFRANCO, 0)
