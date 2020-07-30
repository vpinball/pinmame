/*******************************************************************************
 Preliminary Demolition Man (Williams, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 031003 Tom: added support for flashers 37-44 (51-58)
 2020-07: Tom Collins: complete shots, lamp layout and elevator/claw simulation
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Demolition Man Simulator:
  --------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
    +R  L/R Ramp
    +L  L/R Loop
     Q  SDTM (Drain Ball)
     W  Car Crash
     E  Eyeball/Retina Scan
     R  Retina Eject Saucer
     T  Side Ramp
     Y  Center Ramp
     U  Underground (Computer)
   HJK  Jet (3)
 ZXCVB  Freeze Standup (5)
   M,.  M-T-L lanes (Bonus X)
    []  Left/Right Handle Thumb Buttons
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  dm_handleBallState(sim_tBallStatus *ball, int *inports);
static void dm_handleMech(int mech);
static void dm_drawMech(BMTYPE **line);
static void dm_drawStatic(BMTYPE **line);
static void init_dm(void);
static int dm_getSol(int solNo);

/*-----------------------
  local static variables
 ------------------------*/
#define DM_ELEVATOR_TIME      80
#define DM_CLAW_SLOT          18
#define DM_CLAW_MAX           (6 * DM_CLAW_SLOT - 1)
// value from 0 to 6 indicating where the ball will drop
#define DM_CLAW_DROP_INDEX    (locals.clawPos / DM_CLAW_SLOT)
const char *dm_claw_drop[] = {
  "Elevator", "Simon", "Jets", "Prison", "Freeze", "Acmag"
};
static struct {
  int elevatorPos;    /* Elevator Position, 0 for lowest, DM_ELEVATOR_TIME for highest */
  int clawPos;        /* Claw Position, 0 for rightmost, DM_CLAW_MAX for leftmost */
  int clawDirection;  /* Claw Direction +1 for left, -1 for right, 0 for stopped */
  enum { NORMAL = 0, CRYOCLAW } diverterPos;
  int ballInElevator;
  int diverterCount;  /* cycles of dm_handleMech where sDiverterHold not asserted */
  int magnetCount;    /* if 0, magnet not asserted */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(dm,5)

  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BITIMP(0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BITIMP(0x0080,"L/R Inlane",      KEYCODE_I)

    COREPORT_BITIMP(0x0100,"Standup 1",       KEYCODE_Z)
    COREPORT_BITIMP(0x0200,"Standup 2",       KEYCODE_X)
    COREPORT_BITIMP(0x0400,"Standup 3",       KEYCODE_C)
    COREPORT_BITIMP(0x0800,"Standup 4",       KEYCODE_V)
    COREPORT_BITIMP(0x1000,"Standup 5",       KEYCODE_B)
    COREPORT_BITIMP(0x2000,"Jet 1",           KEYCODE_H)
    COREPORT_BITIMP(0x4000,"Jet 2",           KEYCODE_J)
    COREPORT_BITIMP(0x8000,"Jet 3",           KEYCODE_K)

  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"Eyeball",         KEYCODE_E)
    COREPORT_BIT(   0x0002,"Retina Eject",    KEYCODE_R)
    COREPORT_BIT(   0x0004,"Car Crash",       KEYCODE_W)
    COREPORT_BIT(   0x0008,"Center Ramp",     KEYCODE_Y)
    COREPORT_BIT(   0x0010,"Underground",     KEYCODE_U)
    COREPORT_BIT(   0x0020,"Side Ramp",       KEYCODE_T)
    COREPORT_BIT(   0x0100,"Left Thumb",      KEYCODE_OPENBRACE)
    /* map both CLOSEBRACE and SPACE to ball launch button */
    COREPORT_BIT(   0x0200,"Right Thumb",     KEYCODE_CLOSEBRACE)
    COREPORT_BIT(   0x0200,"Ball Launch",     KEYCODE_SPACE)
    COREPORT_BIT(   0x0800,"Buy-In",          KEYCODE_2)
    COREPORT_BITIMP(0x1000,"M Lane",          KEYCODE_M)
    COREPORT_BITIMP(0x2000,"T Lane",          KEYCODE_COMMA)
    COREPORT_BITIMP(0x4000,"L Lane",          KEYCODE_STOP)
    COREPORT_BITIMP(0x8000,"Drain",           KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* Standard Switches */
#define swLaunch        11
#define swLeftHandle    12
#define swStart         13
#define swTilt          14
#define swLeftOutlane   15
#define swLeftInlane    16
#define swRightInlane   17
#define swRightOutlane  18

#define swSlamTilt      21
#define swCoinDoor      22
#define swBuyIn         23
#define swAlwaysClosed  24
#define swClawPosRight  25  /* Opto */
#define swClawPosLeft   26  /* Opto */
#define swShooter       27
/* Switch 28 not used */

#define swTrough1       31  /* Opto */
#define swTrough2       32  /* Opto */
#define swTrough3       33  /* Opto */
#define swTrough4       34  /* Opto */
#define swTrough5       35  /* Opto */
#define swTroughJam     36  /* Opto */
/* Switch 37 not used */
#define swStandup5      38

#define swLeftSling     41
#define swRightSling    42
#define swLeftJet       43
#define swTopSling      44
#define swRightJet      45
#define swRRampEnter    46
#define swRRampExit     47
#define swLoopRight     48

#define swLRampEnter    51
#define swLRampExit     52
#define swCenterRamp    53
#define swUpperRebound  54
#define swLoopCenter    55  /* in manual as Left Loop */
#define swStandup2      56
#define swStandup3      57
#define swStandup4      58

#define swSRampEnter    61
#define swSRampExit     62
#define swRolloverM     63
#define swRolloverT     64
#define swRolloverL     65
#define swRetinaEject   66
#define swElevatorIndex 67  /* Opto */
/* Switch 68 not used */

#define swCarChase1     71  /* Opto */
#define swCarChase2     72  /* Opto */
#define swTopPopper     73  /* Opto */
#define swElevatorHold  74  /* Opto */
#define swElevatorRamp  75
#define swBottomPopper  76  /* Opto */
#define swEyeball       77
#define swStandup1      78

#define swClawSimon     81
#define swClawJets      82
#define swClawPrison    83
#define swClawFreeze    84
#define swClawAcmag     85
#define swLoopLeft      86  /* in manual as Upper Left Flipper Gate */
#define swCarChase3     87
#define swLowerRebound  88

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough         1
#define sBottomPopper   2
#define sLaunch         3
#define sTopPopper      4
#define sDiverterPower  5
#define sKnocker        7
#define sLeftSling      9
#define sRightSling     10
#define sLeftJet        11
#define sTopSling       12
#define sRightJet       13
#define sRetinaEject    14
#define sDiverterHold   15
#define sElevatorMotor  18
#define sClawLeft       19
#define sClawRight      20
#define sClawMagnet     33

/*---------------------
/  Ball state handling
/----------------------*/
enum {
  stTrough5=SIM_FIRSTSTATE, stTrough4, stTrough3, stTrough2, stTrough1, stTrough, stShooter, stDrain,
  stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane,
  stLeftSling, stRightSling, stLeftJet, stTopSling, stRightJet, stBumpers, stBumpers2, stBumpers3,
  stRolloverM, stRolloverT, stRolloverL, stCarCrash, stCarCrash2, stCarCrash3,
  stLLoopUp, stLLoopDn, stCLoopRight, stCLoopLeft, stRLoopUp, stRLoopDn,
  stTopPopper, stBottomPopper, stEyeball, stRetinaEject,
  stLRampEnter, stLRampExit, stCRamp, stSRampEnter, stSRampExit, stRRampEnter, stRRampDivert, stRRampExit,
  stElevatorWait, stOnClaw, stInElevator, stClawSimon, stClawJets, stClawPrison, stClawFreeze, stClawAcmag,
};

static sim_tState dm_stateDef[] = {
  {"Not Installed", 0, 0,               0,              stDrain,        0, 0, 0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0, 0,               0,              0,              0, 0, 0, SIM_STNOTEXCL},

  /* Line 1 */
  {"Trough 5",      1, swTrough5,       0,              stTrough4,      1},
  {"Trough 4",      1, swTrough4,       0,              stTrough3,      1},
  {"Trough 3",      1, swTrough3,       0,              stTrough2,      1},
  {"Trough 2",      1, swTrough2,       0,              stTrough1,      1},
  {"Trough 1",      1, swTrough1,       sTrough,        stTrough,       1},
  {"Trough Jam",    1, swTroughJam,     0,              stShooter,      1},
  {"Shooter",       1, swShooter,       sLaunch,        stBallLane,     0, 0, 0, SIM_STNOTEXCL | SIM_STSHOOT},
  {"Drain",         1, 0,               0,              stTrough5,      0, 0, 0, SIM_STNOTEXCL},

  /* Line 2 */
  /* auto-plunge feeds to upper-left flipper via right loop */
  {"Ball Lane",     1, 0,               0,              stRLoopUp,      7, 0, 0, SIM_STNOTEXCL},
  {"Right Outlane", 1, swRightOutlane,  0,              stDrain,        15},
  {"Left Outlane",  1, swLeftOutlane,   0,              stDrain,        15},
  {"Right Inlane",  1, swRightInlane,   0,              stFree,         5},
  {"Left Inlane",   1, swLeftInlane,    0,              stFree,         5},

  /* Line 3 */
  {"Left Sling",    1, swLeftSling,     0,              stFree,         1},
  {"Right Sling",   1, swRightSling,    0,              stFree,         1},
  {"Left Bumper",   1, swLeftJet,       0,              stFree,         1},
  {"Top Sling",     1, swTopSling,      0,              stFree,         1},
  {"Right Bumper",  1, swRightJet,      0,              stFree,         1},
  /* TODO: randomize bumper action? */
  {"Bumpers",       1, swLeftJet,       0,              stBumpers2,     3},
  {"Bumpers",       1, swRightJet,      0,              stBumpers3,     3},
  {"Bumpers",       1, swTopSling,      0,              stFree,         3},

  /* Line 4 */
  {"M-t-l",         1, swRolloverM,     0,              stBumpers,      1},
  {"m-T-l",         1, swRolloverT,     0,              stBumpers,      1},
  {"m-t-L",         1, swRolloverL,     0,              stBumpers,      1},
  {"Car Crash 1",   1, swCarChase1,     0,              stCarCrash2,    2},
  {"Car Crash 2",   1, swCarChase2,     0,              stCarCrash3,    2},
  {"Car Crash 3",   1, swCarChase3,     0,              stFree,         1},

  /* Line 5: loops */
  {"L. Loop Enter", 1, swLoopLeft,      0,              stCLoopRight,   5},
  {"L. Loop Exit",  1, swLoopLeft,      0,              stFree,         1},
  {"Looping Right", 1, swLoopCenter,    0,              stTopPopper,    5},
  {"Looping Left",  1, swLoopCenter,    0,              stLLoopDn,      5},
  {"R. Loop Enter", 1, swLoopRight,     0,              stCLoopLeft,    5},
  {"R. Loop Exit",  1, swLoopRight,     0,              stFree,         1},

  /* Line 6 */
  /* TODO: Top Popper should go to any lane (stRollover{M,T,L}) or stRLoopDn */
  {"Top Popper",    1, swTopPopper,     sTopPopper,     stRolloverT,    1},
  {"Bottom Popper", 1, swBottomPopper,  sBottomPopper,  stRightInlane,  1},
  {"Eyeball",       1, swEyeball,       0,              stRetinaEject,  3},
  {"Retina Eject",  1, swRetinaEject,   sRetinaEject,   stLeftInlane,   1},

  /* Line 7 */
  {"L. Ramp Enter",   1, swLRampEnter,  0,              stLRampExit,    5},
  {"L. Ramp Exit",    1, swLRampExit,   0,              stLeftInlane,   5},
  /* TODO: Center Ramp should randomly do stTopPopper (15%), stCLoopLeft (15%) or stRolloverM (70%) */
  {"Center Ramp",     1, swCenterRamp,  0,              stRolloverM,    5},
  {"Side Ramp Enter", 1, swSRampEnter,  0,              stSRampExit,    5},
  {"Side Ramp Exit",  1, swSRampExit,   0,              stRetinaEject,  5},
  /* once claw emulated, update R. Ramp to reference sDiverterHold */
  {"R. Ramp Enter",   1, swRRampEnter,  0,              stRRampDivert,  3},
  {"R. Ramp Divert",  1, 0,             0,              0,              0},
  {"R. Ramp Exit",    1, swRRampExit,   0,              stRightInlane,  5},

  /* Line 8 */
  {"Wait f/Elevator", 1, 0,              0,             0,              0},
  {"On Claw",         1, 0,              0,             0,              0},
  {"In Elevator",     1, 0,              0,             0,              0},
  {"Claw Simon",      1, swClawSimon,    0,             stBottomPopper, 5},
  {"Claw Jets",       1, swClawJets,     0,             stBumpers,      5},
  {"Claw Prison",     1, swClawPrison,   0,             stLLoopDn,      5},
  {"Claw Freeze",     1, swClawFreeze,   0,             stRetinaEject,  5},
  {"Claw Acmag",      1, swClawAcmag,    0,             stLeftInlane,   5},

  {0}
};

static int dm_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
  case stRRampDivert:
    return setState(locals.diverterPos == NORMAL ? stRRampExit : stElevatorWait, 2);

  case stElevatorWait:
    if (core_getSw(swElevatorIndex)) {
      return setState(stInElevator, 3);
    }
    break;

  case stInElevator:
    locals.ballInElevator = TRUE;
    /* pick up ball if magnet on and elevator at 45% to 55% of range */
    if (locals.magnetCount
        && abs(locals.elevatorPos - DM_ELEVATOR_TIME / 2) <= (DM_ELEVATOR_TIME / 20))
    {
      locals.ballInElevator = FALSE;
      return setState(stOnClaw, 3);
    }
    break;

  case stOnClaw:
    if (!locals.magnetCount) {
      /* release ball depending on position */
      return setState(stInElevator + DM_CLAW_DROP_INDEX, 3);
    }
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData dm_inportData[] = {
/* Port 0 */
  {0, 0x0005, stLRampEnter},
  {0, 0x0006, stRRampEnter},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLeftSling},
  {0, 0x0042, swRightSling},
  {0, 0x0081, stLeftInlane},
  {0, 0x0082, stRightInlane},
  {0, 0x0100, swStandup1},
  {0, 0x0200, swStandup2},
  {0, 0x0400, swStandup3},
  {0, 0x0800, swStandup4},
  {0, 0x1000, swStandup5},
  {0, 0x2000, swLeftJet},
  {0, 0x4000, swRightJet},
  {0, 0x8000, swTopSling},

/* Port 1 */
  {1, 0x0001, stEyeball},
  {1, 0x0002, stRetinaEject},
  {1, 0x0004, stCarCrash},
  {1, 0x0008, stCRamp},
  {1, 0x0010, stBottomPopper},  /* Computer/Underground */
  {1, 0x0020, stSRampEnter},

  /* Thumb buttons on handles valid in any state.  Necessary for secret
     awards in regular ROMs and video modes in Demolition Time. */
  {1, 0x0100, swLeftHandle, SIM_STANY},
  {1, 0x0200, swLaunch, SIM_STANY},

  {1, 0x0800, swBuyIn, SIM_STANY},

  {1, 0x1000, stRolloverM},
  {1, 0x2000, stRolloverT},
  {1, 0x4000, stRolloverL},
  {1, 0x8000, stDrain},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay dm_lampPos = {
  { 0,  0}, /* top left */
  {25, 25}, /* size */
  {
  /* Lamp 11; Ball Save */
  {2, {{22, 10, RED}, {22, 14, RED}}},
  /* Lamps 12 to 15; Fortress/Museum/Cryoprison/Wasteland multiball */
  {1, {{23, 12, WHITE}}}, {1, {{22, 12, WHITE}}}, {1, {{20, 12, WHITE}}}, {1, {{21, 12, WHITE}}},
  /* Lamps 16 to 18; Shoot Again, Access Claw, Left Ramp Explode */
  {1, {{25, 12, ORANGE}}}, {1, {{19, 4, RED}}}, {1, {{14, 12, ORANGE}}},

  /* Column 2 */
  {1, {{13, 18, RED}}}, {1, {{14, 22, ORANGE}}}, {1, {{19, 22, RED}}}, {1, {{17, 18, LBLUE}}},
  {1, {{16, 16, ORANGE}}}, {1, {{19, 15, LBLUE}}}, {1, {{19, 9, LBLUE}}}, {1, {{17, 6, LBLUE}}},

  /* Column 3 */
  {1, {{13, 23, RED}}}, {1, {{12, 22, LBLUE}}}, {1, {{12, 20, RED}}}, {1, {{13, 11, RED}}},
  {1, {{13, 8, RED}}}, {1, {{13, 5, YELLOW}}}, {1, {{13, 7, LBLUE}}}, {1, {{14, 6, YELLOW}}},

  /* Column 4 */
  {1, {{14, 18, ORANGE}}}, {1, {{15, 17, ORANGE}}}, {1, {{16, 14, ORANGE}}}, {1, {{15, 13, ORANGE}}},
  {1, {{16, 11, ORANGE}}}, {1, {{15, 10, ORANGE}}}, {1, {{15, 7, YELLOW}}}, {1, {{14, 9, ORANGE}}},

  /* Column 5 */
  {1, {{6, 15, RED}}}, {1, {{7, 14, RED}}}, {1, {{7, 12, LBLUE}}}, {1, {{7, 10, RED}}},
  {1, {{9, 14, RED}}}, {1, {{8, 14, RED}}}, {1, {{7, 7, RED}}}, {1, {{5, 14, RED}}},

  /* Column 6 */
  {1, {{1, 7, RED}}}, {1, {{3, 7, RED}}}, {1, {{4, 5, RED}}}, {1, {{5, 3, RED}}},
  {1, {{5, 1, RED}}}, {1, {{1, 19, GREEN}}}, {1, {{1, 21, GREEN}}}, {1, {{1, 23, GREEN}}},

  /* Column 7 */
  {1, {{10, 21, RED}}}, {1, {{7, 18, YELLOW}}}, {1, {{6, 18, LBLUE}}}, { 0 },
  { 0 }, {1, {{11, 17, LBLUE}}}, {1, {{10, 16, LBLUE}}}, {1, {{15, 4, WHITE}}},

  /* Column 8; Lamps 81-83 center (ACMAG) ramp */
  {1, {{2, 14, RED}}}, {2, {{2, 12, RED}, {2, 16, RED}}}, {2, {{2, 13, RED}, {2, 15, RED}}},
  {1, {{4, 14, RED}}}, {1, {{10, 25, RED}}}, {1, {{25, 20, YELLOW}}}, {1, {{25, 23, RED}}},
  {1, {{25, 3, YELLOW}}}
  }
};

static void dm_drawStatic(BMTYPE **line) {
  /* Help */
  core_textOutf(30, 60, BLACK, "Help on this Simulator:");
  core_textOutf(30, 70, BLACK, "L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 80, BLACK, "L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 90, BLACK, "L/R Ctrl+R/L = L/R Ramp/Loop Shot");
  core_textOutf(30, 100, BLACK, "Q = Drain Ball  U = Underground");
  core_textOutf(30, 110, BLACK, "T/Y = Side Ramp/Center Ramp");
  core_textOutf(18, 120, BLACK, "W/E/R = Car Crash/Eyeball/Retina Eject");
  core_textOutf(18, 130, BLACK, "H/J/K = Jet Bumpers  M/,/. = MTL Lanes");
  core_textOutf(18, 140, BLACK, "Z/X/C/V/B = Freeze Standups");
  core_textOutf(18, 150, BLACK, "[/] = L/R Handle Thumb Buttons");
}

/*-----------------
/  ROM definitions
/------------------*/
#define DM_SOUND_P4 \
DCS_SOUNDROM6x("dmsndp4.u2",CRC(8581116b) SHA1(ab24fa4aadf27761c9013adb84cfef9bfda27d44), \
               "dmsndp4.u3",CRC(fe79fc89) SHA1(4ef1ef0d66d43fa66af1ecb17c14141760859084), \
               "dmsndp4.u4",CRC(18407309) SHA1(499d62e4b434d48870fe532bb85106868df17c9b), \
               "dmsndp4.u5",CRC(f2006c93) SHA1(16656ae6ff18aad0965c5a14882138508925313a), \
               "dmsndp4.u6",CRC(bc17ba11) SHA1(a794599bc334762ddb79e1d0219ad20383139728), \
               "dmsndp4.u7",CRC(8760ed90) SHA1(cf8808f7cd347c47fa12e73a6bb5a54303fb7c49))

#define DM_SOUND_L2 \
DCS_SOUNDROM6x("dm_u2_s.l2",CRC(85fb8bce) SHA1(f2e912113d08b230e32aeeb4143485f266574fa2), \
               "dm_u3_s.l2",CRC(2b65a66e) SHA1(7796082ecd7af29a240190aff654320375502a8b), \
               "dm_u4_s.l2",CRC(9d6815fe) SHA1(fb4be63dee54a883884f1600565011cb9740a866), \
               "dm_u5_s.l2",CRC(9f614c27) SHA1(f8f2f083b644517582a748bda0a3f69c14583f13), \
               "dm_u6_s.l2",CRC(3efc2c0e) SHA1(bc4efdee44ff635771629a2bde79e230b7643f31), \
               "dm_u7_s.l2",CRC(75066af1) SHA1(4d70bce8a96343afcf02c89240b11faf19e11f02))

#define DM_SOUND_H1 \
DCS_SOUNDROM8x("dm.2",CRC(03dae358) SHA1(e6ab35a0c530eda90bd2d65af7bff82af08c39f3), \
               "dm.3",CRC(3b924d3f) SHA1(5bd6126cc6a6c662de0bc311c047441bc29919b2), \
               "dm.4",CRC(ff8985da) SHA1(b382c301744ce208f4710b3dd2342457d02f0ce9), \
               "dm.5",CRC(76f09bd0) SHA1(1e4861ddc12069733f7e1d25192df97b0d9b09ee), \
               "dm.6",CRC(2897aca8) SHA1(d910289e10422e22b4a3e1e296a4a167da1eaa5b), \
               "dm.7",CRC(6b1b9137) SHA1(4064f4fc230ba17b68819ff889335d9b6d9bba3e), \
               "dm.8",CRC(5b333818) SHA1(007b8c117516b6023b376f95ff13831111f4dc20), \
               "dm.9",CRC(4c1a34e8) SHA1(3eacc3c63b2d9db57fc86447f1408635b987ef69))

WPC_ROMSTART(dm,pa2,"u6-pa2.rom",  0x80000,CRC(862be56a) SHA1(95e1f899963762cb1a9de4eb5d6d57183ed1da38)) DM_SOUND_P4 WPC_ROMEND
WPC_ROMSTART(dm,pa3,"u6-pa3.rom",  0x80000,CRC(deaa034e) SHA1(ebb13368e76c3ff9b1a0cad5502fa3db8e7cc864)) DM_SOUND_P4 WPC_ROMEND
WPC_ROMSTART(dm,px5,"dman_px5.rom",0x80000,CRC(42673371) SHA1(77570902c1ca13956fa65214184bce79bcc67173)) DM_SOUND_P4 WPC_ROMEND
WPC_ROMSTART(dm,px6,"dman_px6.rom",0x80000,CRC(d9c31096) SHA1(7146a3693287f3f2d6bc9bbf5c8f5270d2aa10e6)) DM_SOUND_P4 WPC_ROMEND

WPC_ROMSTART(dm,la1,"dman_la1.rom",0x80000,CRC(be7c1965) SHA1(ed3b1016febc819b8c9f34953067bf0cdf3f33e6))
DCS_SOUNDROM6x("dm_u2_s.l1",CRC(f72dc72e) SHA1(a1267c32f70b4bfe6058d7e28d82006541fe3d6c),
               "dm_u3_s.l2",CRC(2b65a66e) SHA1(7796082ecd7af29a240190aff654320375502a8b),
               "dm_u4_s.l2",CRC(9d6815fe) SHA1(fb4be63dee54a883884f1600565011cb9740a866),
               "dm_u5_s.l2",CRC(9f614c27) SHA1(f8f2f083b644517582a748bda0a3f69c14583f13),
               "dm_u6_s.l2",CRC(3efc2c0e) SHA1(bc4efdee44ff635771629a2bde79e230b7643f31),
               "dm_u7_s.l2",CRC(75066af1) SHA1(4d70bce8a96343afcf02c89240b11faf19e11f02))
WPC_ROMEND
WPC_ROMSTART(dm,da1,"dman_da1.rom",0x80000,CRC(365487ef) SHA1(e1be907cb96697f4ec43d6db58d67e6cf87f23e4))
DCS_SOUNDROM6x("dm_u2_s.l1",CRC(f72dc72e) SHA1(a1267c32f70b4bfe6058d7e28d82006541fe3d6c),
               "dm_u3_s.l2",CRC(2b65a66e) SHA1(7796082ecd7af29a240190aff654320375502a8b),
               "dm_u4_s.l2",CRC(9d6815fe) SHA1(fb4be63dee54a883884f1600565011cb9740a866),
               "dm_u5_s.l2",CRC(9f614c27) SHA1(f8f2f083b644517582a748bda0a3f69c14583f13),
               "dm_u6_s.l2",CRC(3efc2c0e) SHA1(bc4efdee44ff635771629a2bde79e230b7643f31),
               "dm_u7_s.l2",CRC(75066af1) SHA1(4d70bce8a96343afcf02c89240b11faf19e11f02))
WPC_ROMEND

WPC_ROMSTART(dm,lx3,  "dman_lx3.rom",0x80000, CRC(5aa57674) SHA1(e02d91a705799866bd741b998d93413ec5bced25)) DM_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(dm,dx3,  "dman_dx3.rom",0x80000, CRC(aede7ca6) SHA1(874aca191ca7bbee108f2f0204a060966b66623e)) DM_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(dm,lx4,  "dman_lx4.rom",0x80000, CRC(c2d0f493) SHA1(26ee970827dd96f3b3c56aa548cf7629ed6a16c1)) DM_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(dm,dx4,  "dman_dx4.rom",0x80000, CRC(6974789d) SHA1(9cf2c49f879fa1f2547e1e7c853e12dc4a35b97d)) DM_SOUND_L2 WPC_ROMEND
//WPC_ROMSTART(dm,lx4c, "dman_lx4c.rom",0x80000,CRC(d05d3bd6) SHA1(eb99dfbc43e299b7ecfe3208905c70d89886be55)) DM_SOUND_L2 WPC_ROMEND //L-4X patch be55
//WPC_ROMSTART(dm,lx4c, "Demolition Man U6 game ROM rev LX-4 patch c53f.rom",0x80000,CRC(b4df0ac3) SHA1(5f8498b066d02fbd64a01ae3da352f976b67059a)) DM_SOUND_L2 WPC_ROMEND //L-4X patch c53f
WPC_ROMSTART(dm,lx4c, "Demolition Man U6 game ROM rev LX-4 patch 00c6.rom",0x80000,CRC(f669f92d) SHA1(0dc1c746c8a6ebcbcbc3866bbdcd5471a1d626ba)) DM_SOUND_L2 WPC_ROMEND //L-4X patch 00c6
WPC_ROMSTART(dm, h5,  "dman_h5.rom", 0x80000, CRC(bdcc62f7) SHA1(d6f3181970f3f71a876e9a2166156eb8fc405af0)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,dh5,  "dman_dh5.rom",0x80000, CRC(91388e13) SHA1(b09a628afd921e34b99d0db3005ab12bbec189b5)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,h5b,  "dman_h5b.rom",0x80000, CRC(b7d0e138) SHA1(5533f36f9eab8626bc1240bdea11ae7757db5244)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,dh5b, "dmandh5b.rom",0x80000, CRC(e2e77372) SHA1(df1384f57339c077d98bda5e6ad8ca1d38e3e6ce)) DM_SOUND_H1 WPC_ROMEND
//WPC_ROMSTART(dm,h5c,  "dman_h5c.rom",0x80000, CRC(de77e678) SHA1(886da8b79baff2403fb66c2e5b4278fbc39d634e)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,h6,   "dman_h6.rom", 0x80000, CRC(3a079b80) SHA1(94a7ee94819ec878ced5e07745bf52b6c65e06c9)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,h6b,  "dman_h6b.rom",0x80000, CRC(047711de) SHA1(7a62b5862f7b5414e3eabe75f117866e6921efaf)) DM_SOUND_H1 WPC_ROMEND
//WPC_ROMSTART(dm,h6c,  "dman_h6c.rom",0x80000, CRC(57de32fd) SHA1(b3ec3b3b2e1136989d92347a22ba38a44bf29e24)) DM_SOUND_H1 WPC_ROMEND //L-6H patch 9e24
WPC_ROMSTART(dm,h6c,  "Demolition Man U6 game ROM rev LH-6 patch 4d2b.rom",0x80000,CRC(19e764a9) SHA1(333e422d2f641952a3e4fddf2a70fdba7268b4e0)) DM_SOUND_H1 WPC_ROMEND //L-6H patch 4d2b
WPC_ROMSTART(dm,dt099,"dm_dt099.rom",0x100000,CRC(64428b78) SHA1(ac0b0174dcecd1b1cd467c4369cb78c2430e9e4b)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,dt101,"dm_dt101.rom",0x100000,CRC(ce36910f) SHA1(45902edd6d8ac2bff95d0c7f65e0f80c4c17b8f2)) DM_SOUND_H1 WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (dm,lx4,       "Demolition Man (LX-4)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,dx4,  lx4, "Demolition Man (DX-4 LED Ghost Fix)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,lx4c, lx4, "Demolition Man (LX-4C Competition + LED Ghost MOD)",2020,"Williams",wpc_mDCSS,0) //L-4X patch 00c6
CORE_CLONEDEF(dm,pa2,  lx4, "Demolition Man (PA-2 Prototype)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,pa3,  lx4, "Demolition Man (PA-3 LED Ghost Fix)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,px5,  lx4, "Demolition Man (PX-5 Prototype)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,px6,  lx4, "Demolition Man (PX-6 LED Ghost Fix)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,la1,  lx4, "Demolition Man (LA-1)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,da1,  lx4, "Demolition Man (DA-1 LED Ghost Fix)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,lx3,  lx4, "Demolition Man (LX-3)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,dx3,  lx4, "Demolition Man (DX-3 LED Ghost Fix)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h5,   lx4, "Demolition Man (H-5)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,dh5,  lx4, "Demolition Man (DH-5 LED Ghost Fix)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h5b,  lx4, "Demolition Man (H-5B Coin Play)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,dh5b, lx4, "Demolition Man (DH-5B Coin Play LED Ghost Fix)", 1995,"Williams",wpc_mDCSS,0)
//CORE_CLONEDEF(dm,h5c,  lx4, "Demolition Man (H-5C Competition MOD)",2017,"Williams",wpc_mDCSS,0) //L-5H patch 634e // as there was no newer, updated version released for this variant again, disable it to avoid confusion
CORE_CLONEDEF(dm,h6,   lx4, "Demolition Man (H-6)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h6b,  lx4, "Demolition Man (H-6B Coin Play)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h6c,  lx4, "Demolition Man (H-6C Competition MOD)",2019,"Williams",wpc_mDCSS,0) //L-6H patch 4d2b
CORE_CLONEDEF(dm,dt099,lx4, "Demolition Man (FreeWPC/Demolition Time 0.99)", 2014,"FreeWPC",wpc_mDCSS,0)
CORE_CLONEDEF(dm,dt101,lx4, "Demolition Man (FreeWPC/Demolition Time 1.01)", 2014,"FreeWPC",wpc_mDCSS,0)


static void dm_drawMech(BMTYPE** line) {
  if (coreGlobals.simAvail) {
    // Claw position, 'M' if magnet enabled, description of position, '*' if
    // right ramp diverter feeding claw.
      core_textOutf(30, 0, BLACK, "Claw: %3u%c %-9s%c", locals.clawPos,
                    locals.magnetCount > 0 ? 'M' : ' ', dm_claw_drop[DM_CLAW_DROP_INDEX],
                    locals.diverterPos == NORMAL ? ' ' : '*');
      // Elevator position and switch indicators (INDEX and HOLD).
      core_textOutf(30, 10, BLACK, "Elev: %3u %-5s %-5s",
                    locals.elevatorPos,
                    core_getSw(swElevatorIndex) ? "INDEX" : "",
                    core_getSw(swElevatorHold) ? "HOLD" : "");
  }
}

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData dmSimData = {
  2,    				/* 2 game specific input ports */
  dm_stateDef,			/* Definition of all states */
  dm_inportData,		/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  dm_handleBallState,	/*Function to handle ball state changes*/
  dm_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData dmGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
    0,0,8,0,0,0,0,
    dm_getSol, dm_handleMech, NULL, dm_drawMech,
    &dm_lampPos
  },
  &dmSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x30, 0x3f, 0x00, 0x00, 0x40, 0x2f, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

#ifdef PROC_SUPPORT
  #include "p-roc/p-roc.h"
  /*
    Note that dm_dt101 doesn't use solenoids 28/30, so we override the default
    function to configure switch rules so that procFullTroughDisablesFlippers()
    will correctly enable both cabinet and handle switches.
  */
  void procConfigureFlipperSwitchRules_dm(int enabled)
  {
    static const char* coil_name[] = { "FLRM", "FLRH", "FLLM", "FLLH", "FULM", "FULH" };
    static const char* sw_right[] = { "SF2", "SF6" };
    static const char* sw_left[] = { "SF4", "SF8" };
    PRCoilList coils[6];
    int i;

    for (i = 0; i < 6; ++i) {
      coils[i].coilNum = PRDecode(kPRMachineWPC, coil_name[i]);
      coils[i].pulseTime = (i & 1) ? 0 : kFlipperPulseTime;
      AddIgnoreCoil(coils[i].coilNum);
    }

    for (i = 0; i < 2; ++i) {
      ConfigureWPCFlipperSwitchRule(PRDecode(kPRMachineWPC, sw_right[i]),
        &coils[0], enabled ? 2 : 0);
      ConfigureWPCFlipperSwitchRule(PRDecode(kPRMachineWPC, sw_left[i]),
        &coils[2], enabled ? 4 : 0);
    }

    if (!enabled) {
      // make sure none of the coils are being driven
      for (i = 0; i < 6; ++i) {
        procDriveCoilDirect(coils[i].coilNum, FALSE);
      }
    }
  }

  /*
    Include special handling for the claw motor.  We ignore states where both
    motors are enabled in order to get smooth movement.
  */
  #define CLAW_MOTOR_OFF    (0)
  #define CLAW_MOTOR_LEFT   (1<<18)
  #define CLAW_MOTOR_RIGHT  (1<<19)
  void dm_wpc_proc_solenoid_handler(int solNum, int enabled, int smoothed) {
    static int motor_pinmame = 0;
    static int motor_proc = 0;

    // Only process immediate changes to claw motor solenoids (C19 and C20)
    if (solNum == 18 || solNum == 19) {
      if (!smoothed) {
        if (enabled)
          motor_pinmame |= (1 << solNum);
        else
          motor_pinmame &= ~(1 << solNum);
        // ignore states where both motors are enabled
        if (motor_pinmame != (CLAW_MOTOR_LEFT | CLAW_MOTOR_RIGHT)) {
          // motor_pinmame and motor_proc are either _OFF, _LEFT or _RIGHT
          // Turn off the motor if it's _LEFT or _RIGHT, then turn it on a
          // direction if necessary.
          if (motor_pinmame != motor_proc) {
            if (motor_proc == CLAW_MOTOR_LEFT) {     // _LEFT to _OFF or _RIGHT
              default_wpc_proc_solenoid_handler(18, FALSE, TRUE);
            }
            if (motor_proc == CLAW_MOTOR_RIGHT) {    // _RIGHT to _OFF or _LEFT
              default_wpc_proc_solenoid_handler(19, FALSE, TRUE);
            }
            if (motor_pinmame == CLAW_MOTOR_LEFT) {  // _OFF or _RIGHT to _LEFT
              default_wpc_proc_solenoid_handler(18, TRUE, TRUE);
            }
            if (motor_pinmame == CLAW_MOTOR_RIGHT) { // _OFF or _LEFT to _RIGHT
              default_wpc_proc_solenoid_handler(19, TRUE, TRUE);
            }

            motor_proc = motor_pinmame;
          }
        }
      }
      return;
    }

    default_wpc_proc_solenoid_handler(solNum, enabled, smoothed);
  }
#endif

/*---------------
/  Game handling
/----------------*/
static void init_dm(void) {
  core_gameData = &dmGameData;
  memset(&locals, 0, sizeof locals);
  wpc_set_modsol_aux_board(1);
#ifdef PROC_SUPPORT
  wpc_proc_solenoid_handler = dm_wpc_proc_solenoid_handler;
  procConfigureFlipperSwitchRules = procConfigureFlipperSwitchRules_dm;
#endif
}

static int dm_getSol(int solNo) {
  if ((solNo >= CORE_CUSTSOLNO(1)) && (solNo <= CORE_CUSTSOLNO(8)))
    return ((wpc_data[WPC_EXTBOARD1]>>(solNo-CORE_CUSTSOLNO(1)))&0x01);
  return 0;
}

/*
  Data logged from a Demolition Man game with P-ROC emulating original ROMs.
  Time for claw to complete a full swing from right-to-left (or vice versa): 1.15s
  Time between Elevator Hold clearing and Elevator Index setting (no ball): 0.1s

  General sequence:
  - Game drives Elevator Motor until Elevator Index set (elevator in lowest position).
  - Looks for Elevator Hold switch to be enabled, indicating ball on elevator.  (Note
    that Elevator Hold is also active when Elevator moves away from home position).
  - Uses claw motor signals to drive claw right until Claw Right switch enabled.
  - Note that game pulses the drive by enabling both Claw Right and Claw Left instead
    of just pulsing one or the other between on and off.
  - Enables Magnet driver.
  - Runs Elevator Motor and expects Elevator Hold to disable shortly before Elevator
    Index enabled, which indicates ball is now on Claw Magnet.
  - Moves claw to the left and instructs player to drop ball.
  - On startup, game will cycle elevator, claw and magnet to drop any ball in the
    elevator into the Acmag position (furthest left).
*/
static void dm_handleMech(int mech) {
  /* cryoclaw diverter */
  if (mech & 0x01) {
    // Either diverter coil held will send the ball to the cryoclaw
    if (core_getSol(sDiverterHold) || core_getSol(sDiverterPower)) {
      locals.diverterCount = 0;
      if (locals.diverterPos != CRYOCLAW) {
        locals.diverterPos = CRYOCLAW;
        wpc_play_sample(0, SAM_DIVERTER);
      }
    }
    else if (locals.diverterPos == CRYOCLAW && locals.diverterCount++ > 25) {
      // release the diverter if not powered for 25 cycles
      locals.diverterPos = NORMAL;
    }
  }

  /* cryoclaw elevator */
  if (mech & 0x02) {
    if (core_getSol(sElevatorMotor)) {
      if (++locals.elevatorPos == DM_ELEVATOR_TIME) {
        locals.elevatorPos = 0;
      }
    }

    /* Claw Pickup Zone:                      |*********|
                0% ----- 10% ----- 20% ----- 45% ----- 55% ----- 90% -----
       Elev. Sw |<-INDEX->|         |<------ HOLD w/o ball ------>|
    */

    /* elevator at bottom in lower 10% of range */
    core_setSw(swElevatorIndex, locals.elevatorPos < DM_ELEVATOR_TIME / 10);
    /* swElevatorHold triggered by ball or elevator rising; note that game
       expects Elevator Hold to be clear for some amount of time before
       Elevator Index is set, when lowering an empty elevator. */
    core_setSw(swElevatorHold, locals.ballInElevator ||
               (locals.elevatorPos > DM_ELEVATOR_TIME / 5
                && locals.elevatorPos < DM_ELEVATOR_TIME * 9 / 10));
  }

  /* cryoclaw magnet */
  if (mech & 0x04) {
    // Either diverter coil held will send the ball to the cryoclaw
    if (core_getSol(sClawMagnet)) {
      locals.magnetCount = 25;
    }
    else if (locals.magnetCount > 0) {
      locals.magnetCount--;
    }
  }

  /* cryoclaw */
  if (mech & 0x08) {
    // need to read pulsed solenoid values instead of smoothed values
    int left = core_getPulsedSol(sClawLeft);
    int right = core_getPulsedSol(sClawRight);
    if (!(left && right)) {
      /* only update when one or none of the solenoids are active */
      locals.clawDirection = left ? +1 : right ? -1 : 0;
    }

    if (locals.clawDirection != 0) {
      locals.clawPos += locals.clawDirection;
      if (locals.clawPos > DM_CLAW_MAX)
        locals.clawPos = DM_CLAW_MAX;
      if (locals.clawPos < 0)
        locals.clawPos = 0;
      core_setSw(swClawPosLeft, locals.clawPos == DM_CLAW_MAX);
      core_setSw(swClawPosRight, locals.clawPos == 0);
    }
  }
}
