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

 The playfield diagram also shows the lamps as a four-by-four grid, L1 through
 L10, which is what the machine is named after.

 Sweeping all 56 switches with a ball in play and watching the score, the ante
 countdown and the lamp columns sorts them further.  These score: $02-$04,
 $06-$08, $0D-$14, $18-$24.  These do not: $01, $05, $09-$0C and everything
 from $28 up, which is the cabinet column and the unused column 6.  Switches
 $15, $17, $1D and $1F score differently from their neighbours -- they jump a
 higher digit rather than stepping the low one -- and $17 and $1F are the two
 that fire the bank resets, so the four of them are the ends of the two drop
 target banks.

 Still unknown by name: the left flipper button, the rollunder spinner, the
 inlanes and the outlanes.  They are among the scoring switches above; telling
 which is which needs a playfield with the switch numbers on it.

 There is no sound.  The 82S130 PROM that circulates with this ROM set was
 dumped from a Superman (see the note in atarigames.c) and is not 4x4's, and
 nothing in the ROM writes a port with the shape of the gen-2 sound board's
 control and data registers.  Hence GAME_NO_SOUND.
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
    COREPORT_BIT(0x4000,"Drain",              KEYCODE_Q)
ATARI_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* The game dispatches on col*8+bit, and ATARI_sw2m maps a switch number
   straight onto that, so these numbers are the game's own event codes. */
#define swRFlipper    0   /* works the right flipper */
#define swUpperPop    1
#define swMidPop      2
#define swLowerPop    3
#define swOuthole     7   /* works the ball feed */
#define swLDrop1     20   /* $14 */
#define swLDrop2     21
#define swLDrop3     22
#define swLDrop4     23   /* $17 - resets the left bank */
#define swRDrop1     28   /* $1C */
#define swRDrop2     29
#define swRDrop3     30
#define swRDrop4     31   /* $1F - resets the right bank */
#define swHole       35   /* $23 */
#define swStart      40   /* $28 - column 5, in the attract enable mask $1D */
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

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough=SIM_FIRSTSTATE, stShooter, stDrain, stOuthole,
      stUpperPop, stMidPop, stLowerPop,
      stLDrop1, stLDrop2, stLDrop3, stLDrop4,
      stRDrop1, stRDrop2, stRDrop3, stRDrop4, stHole
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
  {0, 0x4000, stDrain},
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
  core_textOutf(30,100,BLACK,"H = Kick-out hole, Q = Drain ball");
  core_textOutf(30,110,BLACK,"");
  core_textOutf(30,120,BLACK,"Prototype: two machines were built, and");
  core_textOutf(30,130,BLACK,"the playfield is only partly known.");
  core_textOutf(30,140,BLACK,"");
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
  1,                    /* 1 game specific input port */
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
/   a heat of 1 while the SEG2 bus reads a blank score -- player 2 is not
/   playing -- plus the credit count. --*/
static core_tLCDLayout fourx4_disp[] = {
  { 0, 0, 0, 6, CORE_SEG7 }, { 0,16, 6, 2, CORE_SEG7 },   /* SEG1: player 1, lap (heat) */
  { 2, 0, 8, 6, CORE_SEG7 }, { 2,16,14, 2, CORE_SEG7 },   /* SEG2: player 2, credit */
  { 5, 0,24, 6, CORE_SEG7 },                              /* SEG3: ante, on the playfield */
  { 0 }
};

static core_tGameData fourx4GameData = {
  0, fourx4_disp,
  { FLIP_SWNO(0,swRFlipper), 0, 0, 0, SNDBRD_NONE, 0 },
  &fourx4SimData
};

/*---------------
/  Game handling
/----------------*/
static void init_fourx4(void) {
  core_gameData = &fourx4GameData;
}

CORE_GAMEDEFNV(fourx4,"4x4 (Prototype)",1982,"Atari",gl_mATARI4,GAME_IMPERFECT_SOUND)
