#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "mech.h"

/* 120700  Upgraded the Lamp Matrix to the new format (ML) */
/* 271100  Added Trough Stack state & increased assist hole - Goal delay */
/* 201100  Corrected diverter status reading */
/* 171100  Added lamp layout */
/* 141100  Rearranged inputports (avoid double) */
/*--------------
/ Keys
/   +R   L/R Ramp
/   +I   L/R Inlane
/   +-   L/R Slingshot
/   +O   L/R Outlane
/   +X   L/R Buy Ticket lane
/   +F   L/R Free Kick saucer
/   []   Light Kickback / Light Magna Goalie
/   L    Left Loop
/   A    Assist saucer
/   W    Spinner
/   T    TV-Scoop
/   F    Free Kick target
/   E    Tackle
/   S    Striker's Hideout
/ YUIO   Rollover (4)
/  ZXC   Striker Targets (3)
/  VBN   Jet (3)
/   G    Goal
/   K    Keeper
/   H    Header Lane
/   Q    Drain
/   M    Magna Goalie
/   2    Extra Ball buy-in
/---------------------------*/

#define WCS_GOALIETIME  100
#define WCS_GOALIESLACK  20

/*------------------
/  Local functions
/-------------------*/
static void init_wcs(void);
static int  wcs_getSol(int solNo);
static void wcs_handleMech(int mech);
static int wcs_getMech(int mechNo);
static void wcs_drawMech(BMTYPE **line);
static void wcs_drawStatic(BMTYPE **line);

/*-----------------------
/ local static variables
/------------------------*/
static struct {
  int goaliePos;     /* bank motor position */
  int ball;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(wcs, 5)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001, "Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002, "Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004, "L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008, "L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010, "L/R Light Free Kick",KEYCODE_F)
    COREPORT_BITIMP(0x0020, "L/R Buy Ticket lanes",  KEYCODE_X)
    COREPORT_BIT(   0x0040, "L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080, "L/R Inlane",      KEYCODE_I)
    COREPORT_BIT(   0x0100, "Free Kick",       KEYCODE_F)
    COREPORT_BIT(   0x0200, "Rollover 1",      KEYCODE_Y)
    COREPORT_BIT(   0x0400, "Rollover 2",      KEYCODE_U)
    COREPORT_BIT(   0x0800, "Rollover 3",      KEYCODE_I)
    COREPORT_BIT(   0x1000, "Rollover 4",      KEYCODE_O)
    COREPORT_BIT(   0x2000, "Striker 1",       KEYCODE_Z)
    COREPORT_BIT(   0x4000, "Striker 2",       KEYCODE_X)
    COREPORT_BIT(   0x8000, "Striker 3",       KEYCODE_C)
  PORT_START /* 1 */
    COREPORT_BITIMP(0x0001, "Left Loop",       KEYCODE_L)
    COREPORT_BITIMP(0x0002, "Assist",          KEYCODE_A)
    COREPORT_BITIMP(0x0004, "Spinner",         KEYCODE_W)
    COREPORT_BIT(   0x0008, "Light Kickback",  KEYCODE_OPENBRACE)
    COREPORT_BIT(   0x0010, "Light Mag Goalie",KEYCODE_CLOSEBRACE)
    COREPORT_BITIMP(0x0020, "TV Scoop",        KEYCODE_T)
    COREPORT_BIT(   0x0040, "Tackle",          KEYCODE_E)
    COREPORT_BITIMP(0x0080, "Striker's Hideout", KEYCODE_S)
    COREPORT_BIT(   0x0100, "Jet 1",           KEYCODE_V)
    COREPORT_BIT(   0x0200, "Jet 2",           KEYCODE_B)
    COREPORT_BIT(   0x0400, "Jet 3",           KEYCODE_N)
    COREPORT_BITIMP(0x0800, "Goal",            KEYCODE_G)
    COREPORT_BIT(   0x1000, "Keeper",          KEYCODE_K)
    COREPORT_BITIMP(0x2000, "Header Save",     KEYCODE_H)
    COREPORT_BITIMP(0x4000, "Drain",           KEYCODE_Q)
    COREPORT_BIT(   0x8000, "Magna Goalie",    KEYCODE_M)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swMagGoalie 12
#define swStart     13
#define swTilt      14
#define swLIn       15
#define swStriker3  16
#define swRIn       17
#define swROut      18

#define swSlamTilt  21
#define swCoinDoor  22
#define swBuyIn     23
#define swFreeKick  25
#define swKickBackU 26
#define swSpinner   27
#define swLiteKick  28

#define swTrough1   31
#define swTrough2   32
#define swTrough3   33
#define swTrough4   34
#define swTrough5   35
#define swTroughS   36
#define swLiteMag   37
#define swBallShoot 38

#define swGoalTrough 41
#define swGoalPop    42
#define swGoalieL    43
#define swGoalieR    44
#define swTVPop      45
#define swTravel     47
#define swGoalie     48
#define swRTopRoll   48

#define swSkillF    51
#define swSkillC    52
#define swSkillR    53
#define swRHole     54
#define swUHole     55
#define swLHole     56

#define swRoll1     61
#define swRoll2     62
#define swRoll3     63
#define swRoll4     64
#define swTackle    65
#define swStriker1  66
#define swStriker2  67

#define swLRampDiv  71
#define swLRampEnt  72
#define swLRampEx   74
#define swRRampEnt  75
#define swLockL     76
#define swLockU     77
#define swRRampEx   78

#define swJet1      81
#define swJet2      82
#define swJet3      83
#define swLSling    84
#define swRSling    85
#define swLOut      86
#define swLLane     87
#define swRLane     88

/*----------------------
/ Solenoid definitions
/----------------------*/
#define sGoalPop    1
#define sTVPop      2
#define sKickBack   3
#define sLock       4
#define sUHole      5
#define sTrough     6
#define sKnocker    7
#define sRampDiv    8
#define sJet1       9
#define sJet2      10
#define sJet3      11
#define sLSling    12
#define sRSling    13
#define sRHole     14
#define sLHole     15
#define sDivHold   16
#define sGoalieMot 21
#define sBallMot   22
#define sBallCW    23
#define sBallCCW   24
#define sMagGoalie 33
#define sLoopGate  34
#define sMagLock   35
#define sLRampDiv  CORE_CUSTSOLNO(1) /* rampdiv + divhold */

/*---------------------
/  Ball state handling
/----------------------*/
enum { stTicketArea=SIM_FIRSTSTATE, stDrain,
       stLRampEnt, stLRampEx, stLRampDiv,
       stRRampEnt, stRRampEx, stMagLock,
       stULock, stLLock,
       stLIn, stLOut, stUKickBack, stROut,
       stLLane, stRLane,
       stLHole, stRHole,
       stLoopSpin, stTravel,
       stAssist, stGoal, stGoalPop, stRIn,
       stGoalie,
       stSpinner, stTVPop, stHeader,
       stOuthole, stTrough1, stTrough2, stTrough3, stTrough4, stTroughS,
       stSLane, stSkillF, stSkillC, stSkillR
};

static sim_tState wcs_stateDef[] = {
  {"Not Installed",      0,0,         0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving",},
  {"Playfield",          0,0,         0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Buy Ticket Area",    0,0,         0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Drain",              1,0,         0,        stOuthole,  10,0,0,SIM_STNOTEXCL},
  {"Left Ramp Ent",      1,swLRampEnt,0,        stLRampEx,  15,sLRampDiv,stLRampDiv},
  {"Left Ramp Exit",     1,swLRampEx, 0,        stFree,      5,0,0},
  {"Left Ramp Divirted", 1,swLRampDiv,0,        stRRampEx,  15,0,0},
  {"Right Ramp Ent",     1,swRRampEnt,0,        stRRampEx,  15,0,0},
  {"Right Ramp Exit",    5,swRRampEx, 0,        stLIn,       5,sMagLock,stMagLock},
  {"Lock Magnet",        1,0,         0,        0,           0,0,0},
  {"Lock Upper",         1,swLockU,   0,        stLLock,     5,0,0},
  {"Lock Lower",         1,swLockL,   sLock,    stLIn,      15,0,0},
  {"Left Inlane",        1,swLIn,     0,        stFree,      5,0,0},
  {"Left Outlane",       3,swLOut,    0,        stDrain,     3,sKickBack,stUKickBack},
  {"Kickback Upper",     1,swKickBackU,0,       stFree,      5,0,0},
  {"Right Outlane",      1,swROut,    0,        stDrain,     5,0,0},
  {"Buy Lane",           1,swLLane,   0,        stFree,      5,0,0},
  {"Ticket Lane",        1,swRLane,   0,        stFree,      5,0,0},
  {"Left Free Kick Hole",1,swLHole,   sLHole,   stFree,      5,0,0},
  {"Right Free Kick Hole",1,swRHole,  sRHole,   stFree,      5,0,0},
  {"Loop Spinner",       1,swSpinner, 0,        stTravel,    5,0,0,SIM_STSPINNER},
  {"Travel Lane",        1,swTravel,  0,        stTicketArea,10,sLoopGate,stFree},
  {"Assist Hole",        1,swUHole,   sUHole,   0,           0,0,0},
  {"Goal",               2,swGoalTrough,0,      stGoalPop,   5,0,0},
  {"Goal Popper",        1,swGoalPop, sGoalPop, stRIn,      15,0,0},
  {"Right Inlane",       1,swRIn,     0,        stFree,      5,0,0},
  {"Goalie",             1,swGoalie,  0,        stFree,      5,0,0},
  {"Spinner",            1,swSpinner, 0,        stFree,     10,0,0,SIM_STSPINNER},
  {"TV Popper",          1,swTVPop,   sTVPop,   stFree,      5,0,0},
  {"Header Lane",        1,swKickBackU,0,       stLOut,     10,0,0},
  {"Trough 5",           1,swTrough5, 0,        stTrough4,   5},
  {"Trough 1",           1,swTrough1, sTrough,  stTroughS,   2},
  {"Trough 2",           1,swTrough2, 0,        stTrough1,   5},
  {"Trough 3",           1,swTrough3, 0,        stTrough2,   5},
  {"Trough 4",           1,swTrough4, 0,        stTrough3,   5},
  {"Trough stack",       1,swTroughS, 0,        stSLane,     1},
  {"Shooter Lane",       1,swBallShoot,sShooterRel,0,        0,0,0,SIM_STSHOOT},
  {"Skill Front",        1,swSkillF,  0,        stRIn,       15},
  {"Skill Center",       1,swSkillC,  0,        stRIn,       15},
  {"Skill Rear",         1,swSkillR,  0,        stRIn,       15},
  {0}
};

static int wcs_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    case stAssist:   if (locals.goaliePos > WCS_GOALIESLACK) return setState(stGoal,15);
                     else                                    return setState(stGoalie,15);
    case stSLane:    if (ball->speed > 40) return setState(stSkillF,5);
                     if (ball->speed > 30) return setState(stSkillC,5);
                     if (ball->speed > 20) return setState(stSkillR,5);
                     break;
    case stMagLock:  if (!core_getSol(sMagLock)) return setState(stULock,10);
                     break;
  }
  return 0;
}
/*-- return status of custom solenoids --*/
static int wcs_getSol(int solNo) {
  if (solNo == sLRampDiv) return (core_getSol(sDivHold) || core_getSol(sRampDiv));
  return 0;
}

static mech_tInitData wcs_ballMech = {
  sBallCW, sBallCCW, MECH_LINEAR|MECH_CIRCLE|MECH_TWODIRSOL|MECH_ACC(120)|MECH_RET(3), 20, 4,
  {{0}}
};

static void wcs_handleMech(int mech) {
  /*---------------
  /  handle bank
  /----------------*/
  if ((mech & 0x01) && core_getSol(sGoalieMot)) {
    locals.goaliePos = (locals.goaliePos + 1) % WCS_GOALIETIME;
    core_setSw(swGoalieL, locals.goaliePos < WCS_GOALIESLACK);
    core_setSw(swGoalieR, (locals.goaliePos >= WCS_GOALIETIME/2) &&
                          (locals.goaliePos <  WCS_GOALIETIME/2 + WCS_GOALIESLACK));
  }
  //if (mech & 0x02) mech_update(1);
}

/* 0 = goaliePos */
/* 1 = ball */
static int wcs_getMech(int mechNo) {
  return mechNo ? mech_getPos(1) : locals.goaliePos;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData wcs_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLHole},
  {0, 0x0012, stRHole},
  {0, 0x0021, stLLane, stTicketArea},
  {0, 0x0022, stRLane, stTicketArea},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0100, swFreeKick},
  {0, 0x0200, swRoll1},
  {0, 0x0400, swRoll2},
  {0, 0x0800, swRoll3},
  {0, 0x1000, swRoll4},
  {0, 0x2000, swStriker1},
  {0, 0x4000, swStriker2},
  {0, 0x8000, swStriker3},
  {1, 0x0001, stLoopSpin},
  {1, 0x0002, stAssist},
  {1, 0x0004, stSpinner},
  {1, 0x0008, swLiteKick},
  {1, 0x0010, swLiteMag},
  {1, 0x0020, stTVPop},
  {1, 0x0040, swTackle},
  {1, 0x0080, stGoalPop},
  {1, 0x0100, swJet1},
  {1, 0x0200, swJet2},
  {1, 0x0400, swJet3},
  {1, 0x0800, stGoal},
  {1, 0x1000, stGoalie},
  {1, 0x2000, stHeader},
  {1, 0x4000, stDrain, SIM_STANY},
  {1, 0x8000, swMagGoalie},
  {0}
};

/*--------------------
/ Drawing information
/---------------------*/
static void wcs_drawMech(BMTYPE **line) {
  static const char *goalie[] = {" * ", "*  ", "  *"};
  static const char ball[] = {'|','/','-','\\'};
  core_textOutf(50, 0,BLACK,"Goalie: [%s]",
               goalie[core_getSw(swGoalieL) ? 1 : (core_getSw(swGoalieR) ? 2 : 0)]);
  core_textOutf(50,10,BLACK,"Ball: %c %3d", ball[mech_getPos(1)], mech_getSpeed(1));
}
  /* Help */
static void wcs_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+X = L/R Buy Ticket");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+F = Free Kick Saucer");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W = Spinner");
  core_textOutf(30,110,BLACK,"E = Tackle, T = TV-Scoop, K = Keeper");
  core_textOutf(30,120,BLACK,"Y/U/I/O = Rollovers, A = Assist Saucer");
  core_textOutf(30,130,BLACK,"[/] = Light Kickback/Magna Goalie");
  core_textOutf(30,140,BLACK,"S = Striker Scoop, F = Free Kick Targt");
  core_textOutf(30,150,BLACK,"G = Goal, H = HeaderSv, L = Left Loop");
  core_textOutf(30,160,BLACK,"Z/X/C = Striker Tgts, V/B/N = Jet Bmp.");
}

static core_tLampDisplay wcs_lampPos = {
{ 0, 0 }, /* top left */
{45, 25}, /* size */
{
/* 11 */  {1,{{39,12,RED}}},    /* 12 */  {1,{{37,11,RED}}},    /* 13 */  {1,{{35,10,RED}}},    /* 14 */  {1,{{33, 9,RED   }}},
/* 15 */  {1,{{31, 8,RED   }}}, /* 16 */  {1,{{29, 7,RED   }}}, /* 17 */  {1,{{27, 6,RED   }}}, /* 18 */  {1,{{25, 5,RED   }}},
/* 21 */  {1,{{36,13,YELLOW}}}, /* 22 */  {1,{{34,13,YELLOW}}}, /* 23 */  {1,{{32,13,YELLOW}}}, /* 24 */  {1,{{30,13,YELLOW}}},
/* 25 */  {1,{{27,12,WHITE }}}, /* 26 */  {1,{{25,11,WHITE }}}, /* 27 */  {1,{{23,12,WHITE }}}, /* 28 */  {1,{{21,11,ORANGE}}},
/* 31 */  {1,{{16,16,YELLOW}}}, /* 32 */  {1,{{17,18,WHITE}}},  /* 33 */  {1,{{31,16,WHITE}}},  /* 34 */  {1,{{29,17,WHITE}}},
/* 35 */  {1,{{26,14,WHITE}}},  /* 36 */  {1,{{24,14,WHITE}}},  /* 37 */  {1,{{21,13,ORANGE}}}, /* 38 */  {1,{{14, 9,GREEN}}},
/* 41 */  {1,{{38, 1,GREEN }}}, /* 42 */  {1,{{36, 1,GREEN }}}, /* 43 */  {1,{{34, 1,GREEN }}}, /* 44 */  {1,{{19,20,WHITE }}},
/* 45 */  {1,{{21,19,YELLOW}}}, /* 46 */  {1,{{25,18,WHITE}}},  /* 47 */  {1,{{27,18,WHITE}}},  /* 48 */  {1,{{ 3,15,RED  }}},
/* 51 */  {1,{{ 8,12,RED   }}}, /* 52 */  {1,{{ 7,14,ORANGE}}}, /* 53 */  {1,{{ 6,11,YELLOW}}}, /* 54 */  {1,{{ 5,15,WHITE}}},
/* 55 */  {1,{{29,21,YELLOW}}}, /* 56 */  {1,{{35,22,GREEN }}}, /* 57 */  {1,{{43,17,ORANGE}}}, /* 58 */  {1,{{38,24,RED   }}},
/* 61 */  {1,{{19, 6,WHITE}}},  /* 62 */  {1,{{19, 2,WHITE}}},  /* 63 */  {1,{{21, 3,ORANGE}}}, /* 64 */  {1,{{23, 4,RED   }}},
/* 65 */  {1,{{21, 7,YELLOW}}}, /* 66 */  {1,{{ 1,17,GREEN}}},  /* 67 */  {1,{{ 1,19,GREEN}}},  /* 68 */  {1,{{30,25,YELLOW}}},
/* 71 */  {1,{{16, 4,RED   }}}, /* 72 */  {1,{{19,17,WHITE }}}, /* 73 */  {1,{{43, 7,ORANGE}}}, /* 74 */  {1,{{35, 3,ORANGE}}},
/* 75 */  {1,{{30, 4,GREEN }}}, /* 76 */  {1,{{16, 6,YELLOW}}}, /* 77 */  {1,{{16,20,YELLOW}}}, /* 78 */  {1,{{16, 5,YELLOW}}},
/* 81 */  {1,{{10,13,RED   }}}, /* 82 */  {1,{{13,12,RED  }}},  /* 83 */  {1,{{16,13,RED   }}}, /* 84 */  {1,{{19,14,RED}}},
/* 85 */  {1,{{26,25,YELLOW}}}, /* 86 */  {1,{{28,25,YELLOW}}}, /* 87 */  {1,{{45,21,GREEN }}}, /* 88 */  {1,{{45, 3,YELLOW}}}
}
};

WPC_ROMSTART(wcs,l2,"wcup_lx2.rom",0x80000,CRC(0e4514e8) SHA1(4ef8b78777b8caf1a1ab8f63383c8a7a74d5189a))
DCS_SOUNDROM7x("wcup_u2.rom",CRC(92252f28) SHA1(962a58ea910bcb90c82c81456a888d45f23fcd9a),
               "wcup_u3.rom",CRC(83f541ad) SHA1(2d81d89e43f350caba60d5bec8a66560f8556ad8),
               "wcup_u4.rom",CRC(1540c505) SHA1(aca5a421a0fd067f5411fae2fc3c7c3bcfa1b12f),
               "wcup_u5.rom",CRC(bddad8d4) SHA1(ae6bb1ca3d97a56d1ba984060a1c1ef6c7a00159),
               "wcup_u6.rom",CRC(00f46c12) SHA1(64e99eb32908dbb7b90ee8fa92a20aacf800aeac),
               "wcup_u7.rom",CRC(fff01703) SHA1(fb8d7212fe562e9933941b7bfc707aed1eb74e79),
               "wcup_u8.rom",CRC(670cd382) SHA1(89548420c3b6b8a3d7621b10c538ee1dc6a7be62))
WPC_ROMEND
WPC_ROMSTART(wcs,p3,"wcup_px3.rom",0x80000,CRC(617ea2bc) SHA1(f8e025b62d509126fb4ba425ac4a025dcf13ad99))
DCS_SOUNDROM7x("wcup_u2.rom",CRC(92252f28) SHA1(962a58ea910bcb90c82c81456a888d45f23fcd9a),
               "wcup_u3.rom",CRC(83f541ad) SHA1(2d81d89e43f350caba60d5bec8a66560f8556ad8),
               "wcup_u4.rom",CRC(1540c505) SHA1(aca5a421a0fd067f5411fae2fc3c7c3bcfa1b12f),
               "wcup_u5.rom",CRC(bddad8d4) SHA1(ae6bb1ca3d97a56d1ba984060a1c1ef6c7a00159),
               "wcup_u6.rom",CRC(00f46c12) SHA1(64e99eb32908dbb7b90ee8fa92a20aacf800aeac),
               "wcup_u7.rom",CRC(fff01703) SHA1(fb8d7212fe562e9933941b7bfc707aed1eb74e79),
               "wcup_u8.rom",CRC(670cd382) SHA1(89548420c3b6b8a3d7621b10c538ee1dc6a7be62))
WPC_ROMEND

WPC_ROMSTART(wcs,p2,"u6-pa2.rom",0x80000,CRC(8fcb11b3) SHA1(b8549db3dc096b8b3f684bee35bf5dea3d966957))
DCS_SOUNDROM7x("wcup_u2.rom",CRC(92252f28) SHA1(962a58ea910bcb90c82c81456a888d45f23fcd9a),
               "wcup_u3.rom",CRC(83f541ad) SHA1(2d81d89e43f350caba60d5bec8a66560f8556ad8),
               "wcup_u4.rom",CRC(1540c505) SHA1(aca5a421a0fd067f5411fae2fc3c7c3bcfa1b12f),
               "wcup_u5.rom",CRC(bddad8d4) SHA1(ae6bb1ca3d97a56d1ba984060a1c1ef6c7a00159),
               "wcup_u6.rom",CRC(00f46c12) SHA1(64e99eb32908dbb7b90ee8fa92a20aacf800aeac),
               "wcup_u7.rom",CRC(fff01703) SHA1(fb8d7212fe562e9933941b7bfc707aed1eb74e79),
               "wcup_u8.rom",CRC(670cd382) SHA1(89548420c3b6b8a3d7621b10c538ee1dc6a7be62))
WPC_ROMEND

CORE_GAMEDEF(wcs,l2,"World Cup Soccer (Lx-2)",1994,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(wcs,p2,l2,"World Cup Soccer (Pa-2)",1994,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(wcs,p3,l2,"World Cup Soccer (Px-3)",1994,"Bally",wpc_mSecurityS,0)

/*----------
/ Game Data
/-----------*/
static sim_tSimData wcsSimData = {
  2,                   /* 2 input ports */
  wcs_stateDef,        /* stateData*/
  wcs_inportData,      /* inportData*/
  { stTrough1, stTrough2, stTrough3, stTrough4, stOuthole, stDrain, stDrain },
  NULL,                /* initSim */
  wcs_handleBallState, /* handleBallState*/
  wcs_drawStatic,      /* handleMech*/
  TRUE,                /* automatic plunger */
  NULL                 /* advanced key conditions */
};
static core_tGameData wcsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    wcs_getSol, wcs_handleMech, wcs_getMech, wcs_drawMech,
    &wcs_lampPos, NULL
  },
  &wcsSimData,
  {
    "531 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x1f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_wcs(void) {
  core_gameData = &wcsGameData;
  mech_add(1, &wcs_ballMech);
}

