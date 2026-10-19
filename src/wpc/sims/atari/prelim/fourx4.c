// license:BSD-3-Clause

/*******************************************************************************
 Preliminary Atari 4x4 Pinball Simulator

 4x4 is a 1982-83 Atari prototype; two machines were built.  Everything named
 here was recovered from the ROM and the surviving schematics rather than from
 a machine, so the playfield layout is only partly known -- hence preliminary.

 The solenoid names are the ones pencilled onto the schematic by whoever drew
 it.  The switch names were established by running the game and watching which
 solenoid each of the 56 switches fires: switch 0 works the right flipper,
 switches 1-3 the three pop bumpers, switch 7 the ball feed (so it is the
 outhole), and switches $17 and $1F -- eight apart -- reset the left and right
 drop target banks, which is the two four-bank sets IPDB lists.

 The displays are named by the schematics: the backbox sheet puts PLAYER 1 and
 LAP (HEAT) on SEG1 and PLAYER 2 and CREDIT on SEG2, and the playfield diagram
 marks the middle-of-playfield display as ANTE, SEG 3.

 The playfield diagram draws two four-by-four lamp grids, L11-L20 on the left
 and L21-L30 on the right, numbered in hex and running straight through.  Its
 lamp numbers are the ROM's own output numbers -- see the lamp comment in
 atari.c, which checks that six ways.

 Sweeping all 56 switches with a ball in play and watching the score, the ante
 countdown and the lamp columns sorts them further.  These score: $02-$04,
 $06-$08, $0D-$14, $18-$24.  These do not: $01, $05, $09-$0C and everything
 from $28 up, which is the cabinet column and the unused column 6.  Switches
 $15, $17, $1D and $1F score differently from their neighbours -- they jump a
 higher digit rather than stepping the low one -- and $17 and $1F are the two
 that fire the bank resets, so the four of them are the ends of the two drop
 target banks.

 The wiring sheets name some of them outright, and they use the same hex switch
 numbers the game dispatches on.  P9 pins 20, 19, 18 and 17 are SW $28 START,
 $29 TILT, $2A COIN LT and $2B COIN RT, and P10 pin 11 is SW $2C TEST -- the
 five the attract enable mask $1D had already pointed at.  P11 marks RT FLIPPER
 at pin 13, which is SW $00, matching the measurement, and LT FLIPPER at pin 5,
 which is SW $04.  The flipper buttons appear to be gated in hardware rather
 than pulsed by the CPU: holding SW $04 produces no coil write.

 Still unknown by name: the rollunder spinner, the inlanes and the outlanes.
 They are among the scoring switches above.

 The sound is two POKEYs at $2600 and $2700, driven through the same
 sixteen-slot ramp engine that fades the lamps.  They are emulated well
 enough to hear, not well enough to be right -- the high-pass filters and
 the two-tone mode are not modelled -- hence GAME_IMPERFECT_SOUND.  The
 82S130 PROM that circulates with this ROM set is not 4x4's: it was dumped
 from a Superman, as the note in atarigames.c says.
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the 4x4 Simulator:
  ---------------------------
    Q    Drain Ball
   WER   Upper / Mid / Lower pop bumper
   ASDF  Left drop target bank 1-4
   ZXCV  Right drop target bank 1-4
    H    Kick-out hole
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "atari.h"

/*------------------
/  Local functions
/-------------------*/
static void init_fourx4(void);
static void fourx4_drawStatic(BMTYPE **line);

/*--------------------------
/ Game specific input ports
/---------------------------*/
ATARI2_INPUT_PORTS_START(fourx4,3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Upper Pop Bumper",   KEYCODE_W)
    COREPORT_BIT(0x0002,"Mid Pop Bumper",     KEYCODE_E)
    COREPORT_BIT(0x0004,"Lower Pop Bumper",   KEYCODE_R)
    COREPORT_BIT(0x0008,"Left Drop 1",        KEYCODE_A)
    COREPORT_BIT(0x0010,"Left Drop 2",        KEYCODE_S)
    COREPORT_BIT(0x0020,"Left Drop 3",        KEYCODE_D)
    COREPORT_BIT(0x0040,"Left Drop 4",        KEYCODE_F)
    COREPORT_BIT(0x0080,"Right Drop 1",       KEYCODE_Z)
    COREPORT_BIT(0x0100,"Right Drop 2",       KEYCODE_X)
    COREPORT_BIT(0x0200,"Right Drop 3",       KEYCODE_C)
    COREPORT_BIT(0x0400,"Right Drop 4",       KEYCODE_V)
    COREPORT_BIT(0x0800,"Kick-out Hole",      KEYCODE_H)
    COREPORT_BIT(0x1000,"Outhole",            KEYCODE_O)
    COREPORT_BIT(0x4000,"Drain",              KEYCODE_Q)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Messenger 1",        KEYCODE_1_PAD)
    COREPORT_BIT(0x0002,"Messenger 2",        KEYCODE_2_PAD)
    COREPORT_BIT(0x0004,"Messenger 3",        KEYCODE_3_PAD)
    COREPORT_BIT(0x0008,"Messenger 4",        KEYCODE_4_PAD)
    COREPORT_BIT(0x0010,"Messenger 5",        KEYCODE_5_PAD)
    COREPORT_BIT(0x0020,"Messenger 6",        KEYCODE_6_PAD)
    COREPORT_BIT(0x0040,"Messenger 7",        KEYCODE_7_PAD)
    COREPORT_BIT(0x0080,"Messenger 8",        KEYCODE_8_PAD)
    COREPORT_BIT(0x0100,"Lane 1",             KEYCODE_T)
    COREPORT_BIT(0x0200,"Lane 2",             KEYCODE_Y)
    COREPORT_BIT(0x0400,"Lane 3",             KEYCODE_U)
    COREPORT_BIT(0x0800,"Lane 4",             KEYCODE_I)
    COREPORT_BIT(0x1000,"Left Standing Tgts", KEYCODE_G)
    COREPORT_BIT(0x2000,"Right Standing Tgts",KEYCODE_J)
ATARI_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* The game dispatches on col*8+bit, and ATARI_sw2m maps a switch number
   straight onto that, so these numbers are the game's own event codes -- and
   the same hex numbers the wiring sheets print against the connector pins.

   The handler table at $8B06 gives one thread per switch, and its shape sorts
   them.  Nine switches point at the do-nothing stub $8A9C and so have no
   handler at all: $00, $04, $27, $2D, $2E, $2F and $34-$37.  The first two are
   the flipper buttons, which is why holding them produces no coil write -- the
   flippers are gated in hardware, not pulsed by the CPU.

   The rest fall into groups whose handlers are the same size as each other:

     $01-$03   three pop bumpers (measured: upper, mid, lower)
     $05-$07   three of a kind
     $0C-$13   eight of a kind -- IPDB lists eight messenger balls
     $14-$17   four of a kind, and $17 fires reset-left-bank
     $18-$1B   four of a kind
     $1C-$1F   four of a kind, and $1F fires reset-right-bank
     $20-$23   four of a kind, same sizes as $18-$1B
     $28-$2C   the cabinet, named outright by P9 and P10
     $30-$33   four on column 6
*/
#define swRFlipper    0   /* P11 pin 13, RT FLIPPER; no handler */
#define swUpperPop    1
#define swMidPop      2
#define swLowerPop    3
#define swLFlipper    4   /* P11 pin 5, LT FLIPPER; no handler */
#define swOuthole     7   /* works the ball feed */
#define swMsgr1      12   /* $0C-$13, the group of eight */
#define swMsgr2      13
#define swMsgr3      14
#define swMsgr4      15
#define swMsgr5      16
#define swMsgr6      17
#define swMsgr7      18
#define swMsgr8      19
#define swLDrop1     20   /* $14-$17; the diagram draws these four along the
                           top edge of the left four-by-four grid, and each
                           lights one lamp of the grid's bottom row ($14, $15
                           and $16 light $1D, $1E and $1F) -- four lanes by
                           four levels is what the machine is named after */
#define swLDrop2     21
#define swLDrop3     22
#define swLDrop4     23   /* $17 - resets the left bank */
#define swLane1      24   /* $18-$1B, the four the diagram draws along the
                           bottom edge of the left grid; they light lamps
                           $35-$38, which the diagram draws as one run */
#define swLane2      25
#define swLane3      26
#define swLane4      27
#define swRDrop1     28   /* $1C-$1F, the matching set on the right; they
                           light lamps $45-$48 */
#define swRDrop2     29
#define swRDrop3     30
#define swRDrop4     31   /* $1F - resets the right bank */
#define swHole       35   /* $23 */
#define swLStand     37   /* $25, "LEFT SIDE STANDING TARGETS" in the diagram's
                           own legend; it lights lamp $56 */
#define swRStand     38   /* $26, "RIGHT SIDE STANDING TARGETS"; lamp $57 */
#define swVolume     39   /* $27, a cabinet volume control - no game handler */
/* The playfield diagram's own legend names these, and P9 pins 20, 19, 18, 17
   and P10 pin 11 agree:
       SW 25 = left side swinging switches   SW 26 = right side
       SW 27 = volume   SW 28 = start   SW 29 = tilt
       SW 2A = left coin   SW 2B = right coin   SW 2C = test */
#define swStart      40   /* $28 */
#define swTilt       41   /* $29 */
#define swLCoin      42   /* $2A */
#define swRCoin      43   /* $2B */
#define swTest       44   /* $2C */

/*---------------------
/ Solenoid definitions
/----------------------*/
/* $200A and $200B, low nibble to slice 13E and high nibble to 12E, each into
   a UDN2981.  The names are the schematic's own. */
#define sRFlipper     1   /* $200A bit 0 */
#define sUpperPop     2
#define sMidPop       3
#define sLowerPop     4
#define sLFlipper     5   /* $200A bit 4 */
#define sSol5         6
#define sSol6         7
#define sSol7         8
#define sHoleKick     9   /* $200B bit 0 */
#define sBallFeed    10
#define sResetLBank  11
#define sResetRBank  12
#define sExtraGame   13   /* knocker */
#define sSolD        14
#define sSolE        15
#define sSolF        16

/*-------------------
/ Lamps
/--------------------*/
/* Lamp numbers here are the ROM's own output numbers, blitIndex*8 + bit --
   the same scheme as the solenoids.  atari.c carries the derivation and the
   port-to-column table; the short version is that the lamp column is the
   port's slot in the blit list at $8844, and the playfield diagram's L**
   labels land on those numbers with no offset.

   What the diagram names:

     L11-L20   the left four-by-four grid, the feature the machine is named
               after, with switches $14-$17 along its top edge and $18-$1B
               along its bottom
     L21-L30   the right four-by-four grid
     L35-L38   a run of lane lamps, lit by switches $18-$1B
     L45-L48   lit by switches $1C-$1F
     L53-L55   the three pop bumper lamps; the diagram writes each as a
               triplet with its solenoid and switch, L53/SOL 3/SW 3 and so on,
               and those solenoid and switch numbers match this file
     L55-L58   the cluster above the pops, lit by switches $24-$27

   One loose end: L11-L17 would be image byte 2 bits 1-7, and bits 4-6 of that
   byte are the task bank, so three lamps of the left grid have nowhere to sit.
   Everything else checks out, so it is recorded rather than worked around. */

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough=SIM_FIRSTSTATE, stShooter, stDrain, stOuthole,
      stUpperPop, stMidPop, stLowerPop,
      stLDrop1, stLDrop2, stLDrop3, stLDrop4,
      stRDrop1, stRDrop2, stRDrop3, stRDrop4, stHole,
      stLane1, stLane2, stLane3, stLane4, stLStand, stRStand,
      stMsgr1, stMsgr2, stMsgr3, stMsgr4, stMsgr5, stMsgr6, stMsgr7, stMsgr8
};

static sim_tState fourx4_stateDef[] = {
  {"Not Installed",   0,0,         0,          stDrain,   0,0,0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",       0,0,         0,          0,         0,0,0, SIM_STNOTEXCL},

  {"Trough",          1,0,         sBallFeed,  stShooter, 1},
  {"Shooter",         1,0,         0,          stFree,    0,0,0, SIM_STNOTEXCL|SIM_STSHOOT},

  {"Drain",           1,0,         0,          stOuthole, 0,0,0, SIM_STNOTEXCL},
  {"Outhole",         1,swOuthole, sBallFeed,  stTrough,  1},

  {"Upper Bumper",    1,swUpperPop,0,          stFree,    1},
  {"Mid Bumper",      1,swMidPop,  0,          stFree,    1},
  {"Lower Bumper",    1,swLowerPop,0,          stFree,    1},

  {"Left Drop 1",     1,swLDrop1,  0,          stFree,    1},
  {"Left Drop 2",     1,swLDrop2,  0,          stFree,    1},
  {"Left Drop 3",     1,swLDrop3,  0,          stFree,    1},
  {"Left Drop 4",     1,swLDrop4,  0,          stFree,    1},

  {"Right Drop 1",    1,swRDrop1,  0,          stFree,    1},
  {"Right Drop 2",    1,swRDrop2,  0,          stFree,    1},
  {"Right Drop 3",    1,swRDrop3,  0,          stFree,    1},
  {"Right Drop 4",    1,swRDrop4,  0,          stFree,    1},

  {"Kick-out Hole",   1,swHole,    sHoleKick,  stFree,    3},

  {"Lane 1",          1,swLane1,   0,          stFree,    1},
  {"Lane 2",          1,swLane2,   0,          stFree,    1},
  {"Lane 3",          1,swLane3,   0,          stFree,    1},
  {"Lane 4",          1,swLane4,   0,          stFree,    1},

  {"L Standing Tgt",  1,swLStand,  0,          stFree,    1},
  {"R Standing Tgt",  1,swRStand,  0,          stFree,    1},

  {"Messenger 1",     1,swMsgr1,   0,          stFree,    1},
  {"Messenger 2",     1,swMsgr2,   0,          stFree,    1},
  {"Messenger 3",     1,swMsgr3,   0,          stFree,    1},
  {"Messenger 4",     1,swMsgr4,   0,          stFree,    1},
  {"Messenger 5",     1,swMsgr5,   0,          stFree,    1},
  {"Messenger 6",     1,swMsgr6,   0,          stFree,    1},
  {"Messenger 7",     1,swMsgr7,   0,          stFree,    1},
  {"Messenger 8",     1,swMsgr8,   0,          stFree,    1},
  {0}
};

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData fourx4_inportData[] = {
  {0, 0x0001, stUpperPop},
  {0, 0x0002, stMidPop},
  {0, 0x0004, stLowerPop},
  {0, 0x0008, stLDrop1},
  {0, 0x0010, stLDrop2},
  {0, 0x0020, stLDrop3},
  {0, 0x0040, stLDrop4},
  {0, 0x0080, stRDrop1},
  {0, 0x0100, stRDrop2},
  {0, 0x0200, stRDrop3},
  {0, 0x0400, stRDrop4},
  {0, 0x0800, stHole},
  {0, 0x1000, stOuthole},
  {0, 0x4000, stDrain},
  {1, 0x0001, stMsgr1},
  {1, 0x0002, stMsgr2},
  {1, 0x0004, stMsgr3},
  {1, 0x0008, stMsgr4},
  {1, 0x0010, stMsgr5},
  {1, 0x0020, stMsgr6},
  {1, 0x0040, stMsgr7},
  {1, 0x0080, stMsgr8},
  {1, 0x0100, stLane1},
  {1, 0x0200, stLane2},
  {1, 0x0400, stLane3},
  {1, 0x0800, stLane4},
  {1, 0x1000, stLStand},
  {1, 0x2000, stRStand},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void fourx4_drawStatic(BMTYPE **line) {
  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"W/E/R = Upper/Mid/Lower pop bumper");
  core_textOutf(30, 80,BLACK,"A/S/D/F = Left drop target bank");
  core_textOutf(30, 90,BLACK,"Z/X/C/V = Right drop target bank");
  core_textOutf(30,100,BLACK,"H = Kick-out hole, O = Outhole, Q = Drain");
  core_textOutf(30,110,BLACK,"Keypad 1-8 = the eight-switch group");
  core_textOutf(30,120,BLACK,"T/Y/U/I = Lanes 1-4, G/J = Standing tgts");
  core_textOutf(30,130,BLACK,"Prototype: two machines were built, and");
  core_textOutf(30,140,BLACK,"the playfield is only partly known.");
  core_textOutf(30,150,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,160,BLACK,"");
}

/*-----------------
/  ROM definitions
/------------------*/
ATARI_4x4_ROMSTART(fourx4, "8000ce65.bin", CRC(27341155) SHA1(c0da1fbf64f93ab163b2ea6bfbfc7b778cea819f),
                           "a0004c37.bin", CRC(6f93102f) SHA1(d6520987ed5805b0e6b5da5653fc7cb063e86dda),
                           "c000a70c.bin", CRC(c31ca8d3) SHA1(53f20eff0084771dc61d19db7ddae52e4423e75e))
ATARI_ROMEND

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData fourx4SimData = {
  2,                    /* 2 game specific input ports */
  fourx4_stateDef,      /* Definition of all states */
  fourx4_inportData,    /* Keyboard Entries */
  { stTrough, stTrough, stTrough, stDrain, stDrain, stDrain, stDrain },
  NULL,                 /* no init */
  NULL,                 /* no ball state handler */
  fourx4_drawStatic,
  TRUE,                 /* Simulate manual shooter? */
  NULL                  /* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
/*-- The schematics name every bus.  On the backbox sheet PLAYER 1 and LAP
/   (HEAT) both hang off segments 1A-1G and PLAYER 2 and CREDIT both off
/   2A-2G, so each bus carries a six-digit score and a two-character display --
/   eight digits, which is exactly the eight registers.  The playfield diagram
/   marks the display in the middle of the playfield as ANTE, SEG 3.
/
/   The slices are nibble-paired: 12E takes BD4-BD7 and drives SEG1 (sheet
/   4936), 13E takes BD0-BD3 and drives SEG2 (sheet 4933).  So the high nibble
/   of each register byte is SEG1 and the low nibble SEG2, and the same for
/   SEG3/SEG4 on the second device.
/
/   The emulation agrees: in a one-player game the SEG1 bus reads a score plus
/   a heat of 01 while the SEG2 bus reads a blank score -- player 2 is not
/   playing -- plus a credit count of 02.
/
/   SEG4 is not unused, as was assumed here before.  The wiring sheet draws
/   three displays on the playfield, one of one digit, one of six and one of
/   two, and device 1 feeds both buses: its high nibble reads 000000 through
/   a game, which is the six-digit one, and its low nibble carries a separate
/   two-digit value.  SEG3 was pointed at the low nibble, so the six-digit
/   display was not shown at all and the two-digit one was being read as a
/   six-digit number.  Which of the two remaining displays is the one-digit
/   one is not established. --*/
static core_tLCDLayout fourx4_disp[] = {
  { 0, 0, 0, 6, CORE_SEG7 }, { 0,16, 6, 2, CORE_SEG7 },   /* SEG1: player 1, lap (heat) */
  { 2, 0, 8, 6, CORE_SEG7 }, { 2,16,14, 2, CORE_SEG7 },   /* SEG2: player 2, credit */
  { 5, 0,16, 6, CORE_SEG7 },                              /* SEG3: six-digit playfield display */
  { 5,16,24, 2, CORE_SEG7 },                              /* SEG4: two-digit playfield display */
  { 0 }
};

static core_tGameData fourx4GameData = {
  0, fourx4_disp,
  { FLIP_SWNO(swLFlipper,swRFlipper), 0, 0, 0, SNDBRD_NONE, 0 },
  &fourx4SimData
};

/*---------------
/  Game handling
/----------------*/
static void init_fourx4(void) {
  core_gameData = &fourx4GameData;
}

CORE_GAMEDEFNV(fourx4,"4x4 (Prototype)",1982,"Atari",gl_mATARI4,GAME_IMPERFECT_SOUND)
