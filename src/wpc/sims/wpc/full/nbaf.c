// license:BSD-3-Clause

/*******************************************************************************
 Preliminary NBA Fastbreak (Bally, 1997) Pinball Simulator

 Preliminary version by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Full simulation by Tom Collins (tom@tomlogic.com), August 2024

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for NBA Fastbreak the Simulator:
  -------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
  SDFGH S-H-O-O-T shots (l. loop, l. ramp, center ramp, r. ramp, r.loop)
   XCV  3-P-T Standups
     B  Crazy Bob's
    NM  Kickback standups
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "mech.h"

/*------------------
/  Local functions
/-------------------*/
static int  nbaf_handleBallState(sim_tBallStatus *ball, int *inports);
static void nbaf_handleMech(int mech);
static void nbaf_drawMech(BMTYPE **line);
static int nbaf_getMech(int mechNo);
static void nbaf_drawStatic(BMTYPE **line);
static void init_nbaf(void);
static int  nbaf_getSol(int solNo);

/*-----------------------
  local static variables
 ------------------------*/
#define NBAF_BACKBOX_COUNTDOWN  100     // countdown time for backbox game
static struct {
  int count, lastBit7, lastBit6;
  int backboxPos;   /* Position of ball in backbox, 0=on flipper, otherwise countdown */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(nbaf,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",					KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",	KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",			KEYCODE_W)
    COREPORT_BIT(0x0080,"Middle Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Right Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"",					KEYCODE_T)
    COREPORT_BIT(0x0400,"",					KEYCODE_Y)
    COREPORT_BIT(0x0800,"",					KEYCODE_U)
    COREPORT_BIT(0x1000,"",					KEYCODE_I)
    COREPORT_BIT(0x2000,"",					KEYCODE_O)
    COREPORT_BIT(0x4000,"",					KEYCODE_A)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Left Loop (S)",	KEYCODE_S)
    COREPORT_BIT(0x0002,"Left Ramp (H)",	KEYCODE_D)
    COREPORT_BIT(0x0004,"Center Ramp (O)",	KEYCODE_F)
    COREPORT_BIT(0x0008,"Right Ramp (O)",	KEYCODE_G)
    COREPORT_BIT(0x0010,"Right Loop (T)",	KEYCODE_H)
    COREPORT_BIT(0x0020,"",					KEYCODE_J)
    COREPORT_BIT(0x0040,"",					KEYCODE_K)
    COREPORT_BIT(0x0080,"",					KEYCODE_L)
    COREPORT_BIT(0x0100,"",					KEYCODE_Z)
    COREPORT_BIT(0x0200,"(3)PT Standup",	KEYCODE_X)
    COREPORT_BIT(0x0400,"3(P)T Standup",	KEYCODE_C)
    COREPORT_BIT(0x0800,"3P(T) Standup",	KEYCODE_V)
    COREPORT_BIT(0x1000,"Crazy Bob's",		KEYCODE_B)
    COREPORT_BIT(0x2000,"Kickback 1",		KEYCODE_N)
    COREPORT_BIT(0x4000,"Kickback 2",		KEYCODE_M)
    COREPORT_BIT(0x8000,"",					KEYCODE_COMMA)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/

#define swLaunch        11
#define swBackboxBasket 12
#define swStart         13
#define swTilt          14
#define swShooter       15
#define swLeftInlane    16
#define swRightInlane   17
#define swStandupLow    18

#define swSlamTilt      21
#define swCoinDoor      22
#define swRightJet      23
#define swAlwaysClosed  24
#define swEjectHole     25
#define swLeftOutlane   26
#define swRightOutlane  27
#define swStandupHigh   28

#define swTroughJam     31
#define swTrough1       32
#define swTrough2       33
#define swTrough3       34
#define swTrough4       35
#define swCenterRamp    36
#define swRLoopEnter    37
#define swRLoopExit     38

#define swStandup3      41
#define swStandupP      42
#define swStandupT      43
#define swRRampEnter    44
#define swLRampEnter    45
#define swLRampExit     46
#define swLLoopEnter    47
#define swLLoopExit     48

#define swDefender4     51
#define swDefender3     52
#define swDefenderLock  53
#define swDefender2     54
#define swDefender1     55
#define swJetsDrain     56   /* ball leaving jets */
#define swLeftSling     57
#define swRightSling    58

#define swLeftJet       61
#define swMiddleJet     62
#define swLLoopRampExit 63
#define swRRampExit     64
#define swInThePaint4   65
#define swInThePaint3   66
#define swInThePaint2   67
#define swInThePaint1   68

/* Switches 71 to 88 not used. */

/* Special switches */
#define swBasketMade    WPC_swF5
#define swBasketHold    WPC_swF7

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch         1
/* Solenoid 2 unused */
#define sLRampDiverter  3
#define sRLoopDiverter  4
#define sEject          5
#define sLoopGate       6
#define sBackboxFlipper 7
#define sBallCatchMag   8
#define sTrough         9
#define sLeftSling      10
#define sRightSling     11
#define sLeftJet        12
#define sMiddleJet      13
#define sRightJet       14
#define sPassRight2     15
#define sPassLeft2      16
/* 17-24 are flashers */
#define sPassRight1     25
#define sPassLeft3      26
#define sPassRight3     27
#define sPassLeft4      28

/* 29-32 are lower right/left flippers */
#define sShoot1         33
#define sShoot2         34
#define sShoot3         35
#define sShoot4         36

#define sMotorEnable    37
#define sMotorDirection 38
#define sShotClockEn    39
#define sShotClockCount 40

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane,
      stLeftSling, stRightSling, stLeftJet, stMiddleJet, stRightJet,
      stCrazyBob, stShooter1, stShooter2, stShooter3, stShooter4,
      stBlockedShot, stBasketMade, stBasketHold, stBallCatch,
      stBumpers, stBumpers2, stBumpers3, stBumpersExit,
      stLeftLoopEnter, stLeftLoopMade, stLLoopRampExit, stLeftRampEnter, stLeftRampMade,
      stCenterRamp, stRightRampEnter, stRightRampMade, stRightLoopEnter, stRightLoopMade,
     };

static sim_tState nbaf_stateDef[] = {
  {"Not Installed",	0,0,		 0,	stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,stTrough,	1},
  {"Trough Jam",	1,swTroughJam,	0,		stShooter,	1},
  {"Drain",			1,0,			0,		stTrough4,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sLaunch,stBallLane,0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,			 0,		stFree,		7,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},

  /*Line 3*/
  {"Left Slingshot",1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Middle Bumper",	1,swMiddleJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 4*/
  {"Crazy Bob's",   1,swEjectHole,   sEject,        stFree,     1},
  // In The Paint handled entirely in nbaf_handleBallState()
  {"Shooter 1",     1,swInThePaint1},
  {"Shooter 2",     1,swInThePaint2},
  {"Shooter 3",     1,swInThePaint3},
  {"Shooter 4",     1,swInThePaint4},

  /*Line 5*/
  {"Blocked Shot",  1,0,             0,                 stBumpers,      5},
  {"Basket Made",   1,swBasketMade,  0,                 stBasketHold,   10},
  {"Basket Hold",   1,swBasketHold,  swDefenderLock,    stBallCatch,    5, 0, 0, SIM_STSWOFF},
  // nbaf_handleBallState will advance out of stBallCatch when magnet off
  {"Ball Catch"},

  /*Line 6*/
  {"Pop Bumpers",   1, swLeftJet,    0,     stBumpers2,     3},
  {"Pop Bumpers",   1, swRightJet,   0,     stBumpers3,     3},
  {"Pop Bumpers",   1, swMiddleJet,  0,     stBumpersExit,  3},
  {"Bumper Exit",   1, swJetsDrain,  0,     stFree,         1},

  /*Line 7*/
  {"Left Loop Enter",   1, swLLoopEnter,    0,  stLeftLoopMade,     10},
  {"Left Loop Made",    1, swLLoopExit},    // hand off to handleBallState

  // left loop sent to ramp instead of "in the paint"
  {"L. Loop Ramp Exit", 1, swLLoopRampExit, 0, stLeftInlane,        10},

  {"Left Ramp Enter",   1, swLRampEnter},   // hand off to handleBallState
  {"Left Ramp Made",    1, swLRampExit,     0,  stLeftInlane,       10},

  /*Line 8*/
  {"Center Ramp",       1, swCenterRamp,    0,  stBasketMade,       10},

  {"Right Ramp Enter",  1, swRRampEnter,    0,  stRightRampMade,    10},
  {"Right Ramp Made",   1, swRRampExit,     0,  stRightInlane,      10},

  {"Right Loop Enter",  1, swRLoopEnter},   // hand off to handleBallState
  {"Right Loop Made",   1, swRLoopExit,     0,  stShooter4,         10},

  {0}
};

static int nbaf_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
  case stFree:
      // Automatically drain any balls on the playfield (and not held somewhere)
      // when the flippers are turned off.
      if (!core_getSol(31)) {
          return setState(stDrain, 5);
      }
      break;

  case stShooter1:
      if (core_getSol(sPassRight1)) {
          return setState(stShooter2, 5);
      }
      else if (core_getSol(sShoot1)) {
          return setState(core_getSw(swDefender1) ? stBlockedShot : stBasketMade, 10);
      }
      break;

  case stShooter2:
      if (core_getSol(sPassRight2)) {
          return setState(core_getSol(sBallCatchMag) ? stBallCatch : stShooter3, 5);
      }
      else if (core_getSol(sPassLeft2)) {
          return setState(stShooter1, 5);
      }
      else if (core_getSol(sShoot2)) {
          return setState(core_getSw(swDefender2) ? stBlockedShot : stBasketMade, 10);
      }
      break;

  case stShooter3:
      if (core_getSol(sPassRight3)) {
          return setState(stShooter4, 5);
      }
      else if (core_getSol(sPassLeft3)) {
          return setState(core_getSol(sBallCatchMag) ? stBallCatch : stShooter2, 5);
      }
      else if (core_getSol(sShoot3)) {
          return setState(core_getSw(swDefender3) ? stBlockedShot : stBasketMade, 10);
      }
      break;

  case stShooter4:
      if (core_getSol(sPassLeft4)) {
          return setState(stShooter3, 5);
      }
      else if (core_getSol(sShoot4)) {
          return setState(core_getSw(swDefender4) ? stBlockedShot : stBasketMade, 10);
      }
      break;

  case stBallCatch:
      // release ball if magnet is off
      if (!core_getSol(sBallCatchMag)) {
          return setState(stBumpers, 5);
      }
      break;

  case stLeftLoopMade:
      // if gate up, complete loop to ramp, otherwise feed In The Paint
      return setState(core_getSol(sLoopGate) ? stLLoopRampExit : stShooter2, 10);
      break;

  case stLeftRampEnter:
      return setState(core_getSol(sLRampDiverter) ? stBasketMade : stLeftRampMade, 10);
      break;

  case stRightLoopEnter:
      // if diverter up, feed right loop exit then in the paint, otherwise feed to pops
      return setState(core_getSol(sRLoopDiverter) ? stBumpers : stRightLoopMade, 10);
      break;
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData nbaf_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stMiddleJet},
  {0, 0x0100, stRightJet},
//  {0, 0x0200, st},
//  {0, 0x0400, st},
//  {0, 0x0800, st},
//  {0, 0x1000, st},
//  {0, 0x2000, st},
//  {0, 0x4000, st},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stLeftLoopEnter},
  {1, 0x0002, stLeftRampEnter},
  {1, 0x0004, stCenterRamp},
  {1, 0x0008, stRightRampEnter},
  {1, 0x0010, stRightLoopEnter},
//  {1, 0x0020, st},
//  {1, 0x0040, st},
//  {1, 0x0080, st},
//  {1, 0x0100, st},
  {1, 0x0200, swStandup3},
  {1, 0x0400, swStandupP},
  {1, 0x0800, swStandupT},
  {1, 0x1000, stCrazyBob},
  {1, 0x2000, swStandupHigh},
  {1, 0x4000, swStandupLow},
//  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/

/* macro for a single lamp */
#define LMP1(x, y, c)               {1, {{y, x, c}}}

/* macro for two lamps */
#define LMP2(x1, y1, x2, y2, c)     {2, {{y1, x1, c}, {y2, x2, c}}}

static core_tLampDisplay nbaf_lampPos = {
{ 0,  0}, /* top left */
{29, 25}, /* size */
{
    /* Lamp 11 to 18 */
    LMP1(15, 21, WHITE),    LMP1(14, 24, LPURPLE),  LMP1(12, 25, LPURPLE),  LMP1(10, 24, LPURPLE),
    LMP1(12, 23, LPURPLE),  LMP1(9, 21, LBLUE),     LMP1(8, 23, LBLUE),     LMP1(7, 25, LBLUE),

    /* Lamp 21 to 28 */
    LMP1(15, 18, GREEN),    LMP1(15, 15, RED),      LMP1(13, 14, RED),      LMP1(11, 14, RED),
    LMP1(12, 16, RED), LMP2(12, 19, 12, 20, YELLOW),LMP1(9, 15, RED),       LMP1(9, 18, YELLOW),

    /* Lamp 31 to 38 */
    LMP1(16, 16, GREEN),    LMP1(17, 14, GREEN),    LMP1(18, 12, GREEN),    LMP1(19, 10, GREEN),
    LMP1(20, 8, RED),       LMP1(21, 6, WHITE),     LMP1(22, 4, ORANGE),    LMP1(2, 17, LBLUE),

    /* Lamp 41 to 48 */
    LMP1(21, 12, RED),      LMP1(21, 14, LBLUE),    LMP1(22, 17, LBLUE),    LMP1(23, 15, WHITE),
    LMP1(23, 13, GREEN),    LMP1(24, 9, YELLOW),    LMP1(23, 7, YELLOW),    LMP1(0, 21, YELLOW),

    /* Lamp 51 to 58 */
    LMP1(7, 16, YELLOW),    LMP1(6, 14, YELLOW),    LMP1(5, 12, RED),       LMP1(4, 10, YELLOW),
    LMP1(3, 8, ORANGE),     LMP1(2, 6, RED),  LMP2(24, 20, 24, 21, GREEN),  LMP1(12, 27, ORANGE),

    /* Lamp 61 to 68 */
    LMP2(11, 9, 12, 12, LPURPLE), LMP1(12, 10, RED), LMP1(12, 8, RED),      LMP1(12, 6, RED),
    LMP1(12, 4, LPURPLE),   LMP1(12, 2, ORANGE),    LMP1(18, 0, YELLOW),    LMP1(14, 0, YELLOW),

    /* Lamp 71 to 78 */
    LMP1(10, 7, RED),       LMP1(9, 5, RED),        LMP1(8, 3, RED),        LMP1(15, 7, RED),
    LMP1(16, 5, RED),       LMP1(17, 3, ORANGE),    LMP1(6, 0, YELLOW),     LMP1(10, 0, YELLOW),

    /* Lamp 81 to 88 */
    LMP1(6, 8, RED),        LMP1(5, 6, WHITE),      LMP1(4, 4, ORANGE),     LMP1(10, 2, LPURPLE),
    LMP1(14, 2, LPURPLE),   LMP1(19, 4, LPURPLE),   LMP1(19, 27, ORANGE),   LMP1(2, 27, YELLOW)
}
};

static void nbaf_drawStatic(BMTYPE **line) {

/* Help */
  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,100,BLACK,"S/D/F/G/H = S-H-O-O-T Loops/Ramps");
  core_textOutf(30,110,BLACK,"B = Crazy Bob's, X/C/V = 3PT Targets");
  core_textOutf(30,120,BLACK,"N/M = Kickback Targets");
}

#define NBAF_SOUND1 \
DCS_SOUNDROM5xm("fb-s2.1_0",CRC(32f42a82) SHA1(387636c8e9f8525e7442ccdced735392db113044), \
                "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648), \
                "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9), \
                "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d), \
                "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))

// German speech included in Sound ROM S2 3.0, no separate German speech ROM is necessary anymore
#define NBAF_SOUND3 \
DCS_SOUNDROM5m("fb-s2.3_0",CRC(4594abd3) SHA1(d14654f0c2d29c28cae604e2dbcc9adf361b28a9), \
               "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648), \
               "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9), \
               "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d), \
               "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(nbaf,31,"fb_g11.3_1",0x80000,CRC(acd84ec2) SHA1(bd641b26e7a577be9f8705b21de4a694400945ce)) NBAF_SOUND3 WPC_ROMEND

//Version 1.0 is the production release.
// This version requires version 1.0 Sound ROMs or higher.
// German-jumpered games require version 2.0 of Sound ROM S2.
WPC_ROMSTART(nbaf,11,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99)) NBAF_SOUND1 WPC_ROMEND
WPC_ROMSTART(nbaf,11a,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99))
DCS_SOUNDROM5m("fb-s2.2_0",CRC(f950f481) SHA1(8d7c54c5f27a85889179ee690512fa69b1357bb6),
               "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648),
               "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9),
               "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d),
               "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))
WPC_ROMEND
WPC_ROMSTART(nbaf,11s,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99))
DCS_SOUNDROM5xm("fb-s2.0_4",CRC(6a96f42b) SHA1(b6019bccdf62c9cf044a88d35019ebf0593b24d7),
                "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648),
                "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9),
                "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d),
                "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))
WPC_ROMEND
WPC_ROMSTART(nbaf,115,"g11-115",0x80000,CRC(c0ed9848) SHA1(196d13cf93fe61db36d3bd936549210875a88948)) NBAF_SOUND1 WPC_ROMEND

//Version 2.1
// Linked play requires Sound ROM S2 Version 3.0 and the NBA Fastbreak Linking Kit
WPC_ROMSTART(nbaf,21,"g11-21.rom",0x80000,CRC(598d33d0) SHA1(98c2bfcca573a6e790a4d3ba306953ff0fb3b042)) NBAF_SOUND3 WPC_ROMEND
WPC_ROMSTART(nbaf,22,"g11-22.rom",0x80000,CRC(2e7a9685) SHA1(2af250a947089469c942cf2c570063bdebd4abe4)) NBAF_SOUND3 WPC_ROMEND
WPC_ROMSTART(nbaf,23,"g11-23.rom",0x80000,CRC(a6ceb6de) SHA1(055387ee7da57e1a8fbce803a0dd9e67d6dbb1bd)) NBAF_SOUND3 WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(nbaf,31,"NBA Fastbreak (3.1 / S3.0 English/German)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,11s,31,"NBA Fastbreak (1.1 / S0.4)",1997,"Bally",wpc_m95S,0) //S0.4 only parked here, should only be used with proto game code
CORE_CLONEDEF(nbaf,11,31,"NBA Fastbreak (1.1 / S1.0)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,11a,31,"NBA Fastbreak (1.1 / S2.0 German)",1997,"Bally",wpc_m95S,0) //basically S1.0, but german speech
CORE_CLONEDEF(nbaf,115,31,"NBA Fastbreak (1.15 / S1.0)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,21,31,"NBA Fastbreak (2.1 / S3.0 English/German)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,22,31,"NBA Fastbreak (2.2 / S3.0 English/German)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,23,31,"NBA Fastbreak (2.3 / S3.0 English/German)",1997,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData nbafSimData = {
  2,    				/* 2 game specific input ports */
  nbaf_stateDef,		/* Definition of all states */
  nbaf_inportData,		/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  nbaf_handleBallState,	/*Function to handle ball state changes*/
  nbaf_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

extern PINMAME_VIDEO_UPDATE(wpcdmd_update32);
static PINMAME_VIDEO_UPDATE(led_update) {
  return 1;
}
static struct core_dispLayout nbaf_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(genf *)wpcdmd_update32,NULL},
  {7,0, 0,  2,CORE_SEG7 | CORE_NODISP,(genf *)led_update,NULL},
  {0}
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData nbafGameData = {
  GEN_WPC95, nbaf_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),     // other switches/solenoids used for non-flippers
    0,0,0,  /* custom switch columns, lamp columns, solenoids */
    0,0,    /* sound board, display */
    0,0,    /* gameSpecific1, gameSpecific2 */
    nbaf_getSol, nbaf_handleMech, nbaf_getMech, nbaf_drawMech,
    &nbaf_lampPos,          /* lampData */
  },
  &nbafSimData,
  {
    "553 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

static WRITE_HANDLER(nbaf_wpc_w) {
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {    
    /* solenoids sShotClockCount (40/BIT7) and sShotClockEn (39/BIT6) */
	if (GET_BIT7 && !locals.lastBit7 && locals.count) {
	  locals.count--;
	}
	if (GET_BIT6) {
	  coreGlobals.segments[0].w = core_bcd2seg7[locals.count / 10];
	  coreGlobals.segments[1].w = core_bcd2seg7[locals.count % 10];
	}
	else if (locals.lastBit6) {
	  locals.count = 24;
	}
	if (!(GET_BIT6 || GET_BIT7 || locals.lastBit6 || locals.lastBit7)) {
	  coreGlobals.segments[0].w = coreGlobals.segments[1].w = 0;
	}
	locals.lastBit6 = GET_BIT6;
	locals.lastBit7 = GET_BIT7;
  }
}

#define NBAF_DEFENDER_MAX       80      // rightmost position of defender
#define NBAF_DEFENDER_SW_WIDTH  4       // positions that register for each defender location
#define DEFEND_POS(x)   (x) - (NBAF_DEFENDER_SW_WIDTH / 2), (x) + (NBAF_DEFENDER_SW_WIDTH / 2)
// Defender uses an enable and direction solenoid output to sweep from
// shooter 1 to 4 (100%, 66%, 33%, 0%) with an extra switch for the "lock"
// position (50%).  Starts in the lock position.
static mech_tInitData nbaf_defender_mech = {
    sMotorEnable, sMotorDirection,
    MECH_LINEAR | MECH_STOPEND | MECH_ONEDIRSOL,
    NBAF_DEFENDER_MAX, NBAF_DEFENDER_MAX,
    {
        { swDefender4, 0, NBAF_DEFENDER_SW_WIDTH },
        { swDefender3, DEFEND_POS(NBAF_DEFENDER_MAX / 3) },
        { swDefenderLock, DEFEND_POS(NBAF_DEFENDER_MAX / 2) },
        { swDefender2, DEFEND_POS(NBAF_DEFENDER_MAX * 2 / 3) },
        { swDefender1, NBAF_DEFENDER_MAX - NBAF_DEFENDER_SW_WIDTH, NBAF_DEFENDER_MAX - 1 },
    }, NBAF_DEFENDER_MAX / 2
};

/*---------------
/  Game handling
/----------------*/
static void init_nbaf(void) {
  core_gameData = &nbafGameData;
  memset(&locals, 0, sizeof(locals));

  mech_add(0, &nbaf_defender_mech);

  locals.count = 24;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, nbaf_wpc_w);
  wpc_set_fastflip_addr(0x7b);
}

static int nbaf_getSol(int solNo) {
    if (solNo >= 33 && solNo <= 40) {
        return (wpc_data[WPC_FLIPPERCOIL95] >> (solNo - 33)) & 0x1;
    }
    return 0;
}

static void nbaf_drawMech(BMTYPE **line) {
    if (coreGlobals.simAvail) {
        // Defender Position
        core_textOutf(30, 0, BLACK, "Defender: %3u %c",
                      mech_getPos(0),
                      core_getSw(swDefender1) ? '1' :
                      core_getSw(swDefender2) ? '2' :
                      core_getSw(swDefenderLock) ? '*' :
                      core_getSw(swDefender3) ? '3' :
                      core_getSw(swDefender4) ? '4' : ' ');
        // Backbox basketball position
        core_textOutf(30, 10, BLACK, "Backbox: %3u %c",
                      locals.backboxPos,
                      core_getSw(swBackboxBasket) ? '*' : ' ');
    }
}

#define BACKBOX_AT(pos)  (abs(locals.backboxPos - pos) <= 2)
static void nbaf_handleMech(int mech) {
    // Simulate the backbox game by tracking ball position
    // and scoring a basket when appropriate.
    if (mech & 0x01) {
        if (core_getSol(sBackboxFlipper)) {
            if (locals.backboxPos == 0) {
                locals.backboxPos = NBAF_BACKBOX_COUNTDOWN;
            }
            else if (locals.backboxPos < 10) {
                // flipped too soon
                locals.backboxPos = NBAF_BACKBOX_COUNTDOWN / 2 - 5;
            }
            else {
                // flipped when the ball wasn't close to the flipper
            }
        }
        // update the ball position for the backbox game
        if (locals.backboxPos) {
            --locals.backboxPos;
            core_setSw(swBackboxBasket, BACKBOX_AT(NBAF_BACKBOX_COUNTDOWN / 2));
        }
    }
}

static int nbaf_getMech(int mechNo)
{
    return mech_getPos(mechNo);
}
