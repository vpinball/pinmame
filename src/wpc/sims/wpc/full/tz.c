#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "mech.h"

/* 12-05-2000: Modified custom playfield lamp layout to accomodate new structure.
               Added support for shared bulbs where necessary (SJE)
   12-01-2000: Adjusted layout a little, to better match my real TZ! (SJE)  */
/* 161000  Cleaned up BITIMP use */
/* 231000  Now uses textOutf */
/* 280101  Tom: changed tz_getSol() to report back all state on the external board,
                changed the definition of sClockFwd, sClockRev and sGumRel for the new situation */
#define TZ_GUMTIME      120 /* Time for Gumball machine to dispense a ball */
#define TZ_GUMRELTIME     3 /* Timer for gumball exit */
#define TZ_POWERBALL   0x01

/*------------------
/  Local functions
/-------------------*/
static int tz_handleBallState(sim_tBallStatus *ball, int *inports);
static int tz_getSol(int solNo);
static void tz_handleMech(int mech);
static int tz_getMech(int mechNo);
static void tz_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls);
static void tz_drawMech(BMTYPE **line);
static void tz_drawStatic(BMTYPE **line);
static void init_tz(void);

/*-----------------------
/ local static variables
/------------------------*/
static struct tz_tLocals {
  int gumPos;     /* gumball motor position */
  int gumRel;     /* gumball release (faked solenoid) */
  int clockTick;  /* clock position (from noon) */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(tz,6)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001, "Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002, "Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004, "L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008, "L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010, "L/R Spiral",      KEYCODE_S)
    COREPORT_BIT(   0x0020, "L/R Mini PF",     KEYCODE_M)
    COREPORT_BIT(   0x0040, "L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080, "L/R Inlane",      KEYCODE_I)
    COREPORT_BIT(   0x0100, "Power Payoff",    KEYCODE_Y)
    COREPORT_BIT(   0x0200, "Jet 1",           KEYCODE_V)
    COREPORT_BIT(   0x0400, "Jet 2",           KEYCODE_B)
    COREPORT_BIT(   0x0800, "Jet 3",           KEYCODE_N)
    COREPORT_BITIMP(0x1000, "Mini PF Top",     KEYCODE_T)
    COREPORT_BITIMP(0x2000, "Mini PF Exit",    KEYCODE_E)
    COREPORT_BITIMP(0x4000, "Clock Lane",      KEYCODE_U)
    COREPORT_BIT(   0x8000, "Buy-in",          KEYCODE_2)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001, "Greed 1",         KEYCODE_A)
    COREPORT_BIT(   0x0002, "Greed 2",         KEYCODE_F)
    COREPORT_BIT(   0x0004, "Greed 3",         KEYCODE_G)
    COREPORT_BIT(   0x0008, "Greed 4",         KEYCODE_J)
    COREPORT_BIT(   0x0010, "Greed 5",         KEYCODE_K)
    COREPORT_BIT(   0x0020, "Greed 6",         KEYCODE_COLON)
    COREPORT_BIT(   0x0040, "Greed 7",         KEYCODE_QUOTE)
    COREPORT_BIT(   0x0080, "Clock Target",    KEYCODE_W)
    COREPORT_BIT(   0x0100, "Left Inner Inlane",KEYCODE_I)
    COREPORT_BITIMP(0x0200, "Slot Machine",    KEYCODE_S)
    COREPORT_BITIMP(0x0400, "Camera",          KEYCODE_C)
    COREPORT_BITIMP(0x0800, "Piano",           KEYCODE_Z)
    COREPORT_BITIMP(0x1000, "Dead End",        KEYCODE_D)
    COREPORT_BITIMP(0x2000, "Lock",            KEYCODE_L)
    COREPORT_BITIMP(0x4000, "Rocket",          KEYCODE_R)
    COREPORT_BIT(   0x8000, "Hitchhiker",      KEYCODE_H)
  PORT_START /* 2 */
    COREPORT_BITIMP(0x8000, "Drain",           KEYCODE_Q)
    COREPORT_DIPNAME( 0x0007, 0x05, "Powerball")
      COREPORT_DIPSET(0x0000, "1" )
      COREPORT_DIPSET(0x0001, "2" )
      COREPORT_DIPSET(0x0002, "3" )
      COREPORT_DIPSET(0x0003, "4" )
      COREPORT_DIPSET(0x0004, "5" )
      COREPORT_DIPSET(0x0005, "6" )
      COREPORT_DIPSET(0x0006, "7" )
    COREPORT_DIPNAME( 0x0008, 0x00, "Third Magnet")
      COREPORT_DIPSET(0x0000, DEF_STR(No) )
      COREPORT_DIPSET(0x0008, DEF_STR(Yes) )
    COREPORT_DIPNAME( 0x0010, 0x00, "Big Kick")
      COREPORT_DIPSET(0x0000, DEF_STR(No) )
      COREPORT_DIPSET(0x0010, DEF_STR(Yes) )
    COREPORT_DIPNAME( 0x0020, 0x00, "Clock Lane")
      COREPORT_DIPSET(0x0000, DEF_STR(No) )
      COREPORT_DIPSET(0x0020, DEF_STR(Yes) )
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swRIn         11
#define swROut        12
#define swStart       13
#define swTilt        14
#define swRTrough     15
#define swCTrough     16
#define swLTrough     17
#define swOuthole     18

#define swSlamTilt    21
#define swCoinDoor    22
#define swBuyIn       23
#define swFLTrough    25
#define swTProxi      26
#define swShooter     27
#define swRocket      28

#define swJet1        31
#define swJet2        32
#define swJet3        33
#define swLSling      34
#define swRSling      35
#define swLOut        36
#define swLIn1        37
#define swLIn2        38

#define swDeadEnd     41
#define swCamera      42
#define swPiano       43
#define swMPFEnter    44
#define swLMPF        45
#define swRMPF        46
#define swClockM      47
#define swGreed1      48

#define swGumLane     51
#define swHitchH      52
#define swLRampEnt    53
#define swLRamp       54
#define swGeneva      55
#define swGumExit     56
#define swSProxi      57
#define swSlot        58

#define swSkillR      61
#define swSkillO      62
#define swSkillY      63
#define swGreed4      64
#define swPowerPay    65
#define swGreed5      66
#define swGreed6      67
#define swGreed7      68

#define swBigKick     71
#define swAutoFire    72
#define swRRampEnt    73
#define swGumPop      74
#define swMPFTop      75
#define swMPFExit     76
#define swGreed2      77
#define swGreed3      78

#define swRLMag       81
#define swRUMag       82
#define swLMag        83
#define swCLock       84
#define swULock       85
#define swCLane       86
#define swGumEnter    87
#define swLLock       88

#define swClockM15   CORE_CUSTSWNO(1,1)
#define swClockM0    CORE_CUSTSWNO(1,2)
#define swClockM45   CORE_CUSTSWNO(1,3)
#define swClockM30   CORE_CUSTSWNO(1,4)
#define swClockH1    CORE_CUSTSWNO(1,5)
#define swClockH2    CORE_CUSTSWNO(1,6)
#define swClockH3    CORE_CUSTSWNO(1,7)
#define swClockH4    CORE_CUSTSWNO(1,8)

/*----------------------
/ Solenoid definitions
/----------------------*/
#define sSlot          1
#define sRocket        2
#define sAutoFire      3
#define sGumPop        4
#define sRRampDiv      5
#define sGumDiv        6
#define sKnocker       7
#define sOuthole       8
#define sBallRel       9
#define sRSling       10
#define sLSling       11
#define sJet1         12
#define sJet2         13
#define sJet3         14
#define sLockRel      15
#define sAutoDiv      16
#define sLMag         21
#define sRUMag        22
#define sRLMag        23
#define sGumMotor     24
#define sLMPFMag      25
#define sRMPFMag      26
#define sLRampDiv     27
#define sClockRev     CORE_CUSTSOLNO(7)
#define sClockFwd     CORE_CUSTSOLNO(6)
/*-- fake solenoids to simplify the state handling--*/
#define sGumRel       CORE_CUSTSOLNO(9)

/*---------------------
/  Ball state handling
/----------------------*/
enum {stMPFFree=SIM_FIRSTSTATE, stRTrough, stCTrough, stLTrough, stFLTrough, stShooterDiv,
      stLLock, stCLock, stULock, stRLMagDn, stRLMagUp, stRUMagDn, stRUMagUp, stLMagDn,
      stLMagUp, stSpiral, stGumLane, stGumPop, stGumEntry, stGum4, stGum3, stGum2,
      stGum1, stGumExit, stCamera, stPiano, stDeadEnd, stSProx, stSlot, stLRampEnt,
      stLRamp, stRRampEnt, stRRamp, stRRampDiv, stMPFEnter, stMPFTop, stMPFExit,
      stDrain, stOuthole, stRocket, stAutoFire, stShooter, stBallLane, stSkillR, stSkillO,
      stSkillY, stLIn2, stHitchH, stLOut, stROut, stBigKick,
      stJet1, stJet2, stJet3};

static sim_tState tz_stateDef[] = {
 /* Display         swTime switch     sol       next    timer alt flags*/
  {"Not Installed",      0,0,         0,        stDrain,    0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",                 0,0,         0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Mini PF",            0,0,         0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Right Trough",       5,swRTrough, sBallRel, 0,          0,0,0,SIM_STIGNORESOL},
  {"Center Trough",      1,swCTrough, 0,        stRTrough,  1},
  {"Left Trough",        1,swLTrough, 0,        stCTrough,  1},
  {"Far Left Trough",    1,swFLTrough,0,        stLTrough,  1},
  {"Auto Diverter",      1,0,         0,        stShooter,  2,sAutoDiv,stAutoFire},
  {"Lower Lock",         1,swLLock,   sLockRel, stRLMagDn, 10},
  {"Center Lock",        1,swCLock,   0,        stLLock,    1},
  {"Upper Lock",         1,swULock,   0,        stCLock,    1},
  {"Lower Right Magnet", 1,swRLMag,   0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Lower Right Magnet", 1,swRLMag,   0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Upper Right Magnet", 1,swRUMag,   0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Upper Right Magnet", 1,swRUMag,   0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Left Magnet",        1,swLMag,    0,        0,          0,0,0,SIM_STNOTEXCL},
  {"Left Magnet",        1,swLMag,    0,        0,          0,0,0,SIM_STNOTEXCL},
  {"In Spiral",          1,0,         0,        stLMagDn,   5,sGumDiv,stGumLane},
  {"Gumball Lane",       1,swGumLane, 0,        stGumPop,  20},
  {"Gumball Popper",     1,swGumPop,  sGumPop,  stGumEntry, 3},
  {"Gumball Entry",      5,swGumEnter,0,        stGum4,     1},
  {"Gumball 4",          1,0,         0,        stGum3,     1},
  {"Gumball 3",          1,0,         0,        stGum2,     1},
  {"Gumball 2",          1,0,         0,        stGum1,     1},
  {"Gumball 1",          1,0,         sGumRel,  stGumExit, 15},
  {"Gumball Exit",       1,swGumExit, 0,        stCamera,  15},
  {"Camera",             1,swCamera,  0,        stSProx,   10},
  {"Piano",              1,swPiano,   0,        stSProx,   10},
  {"DeadEnd",            1,swDeadEnd, 0,        stCamera,   5},
  {"Slot Prox",          5,swSProxi,  0,        stSlot,     5,0,0,SIM_STNOTEXCL|SIM_STNOTBALL(TZ_POWERBALL)},
  {"Slot Machine",       1,swSlot,    sSlot,    stFree,     5,0,0,SIM_STNOTEXCL},
  {"Left Ramp Enter",    1,swLRampEnt,0,        stLRamp,   10},
  {"Left Ramp",          1,swLRamp,   0,        stLIn2,    15,sLRampDiv,stShooterDiv},
  {"Right Ramp Enter",   1,swRRampEnt,0,        stRRamp,    4},
  {"Right Ramp Rail",    0,0,         0,        stRRampDiv, 1,sRRampDiv,stMPFEnter},
  {"Right Ramp Diverter",0,0,         sRRampDiv,stFree,    10},
  {"Mini PF Enter",      1,swMPFEnter,0,        stMPFFree,  5},
  {"Mini PF Top",        1,swMPFTop,  0,        stCamera,  10},
  {"Mini PF Exit",       1,swMPFExit, 0,        stFree,    10},
  {"Drain",              0,0,         0,        stOuthole,  1,0,0, SIM_STNOTEXCL},
  {"Outhole",            1,swOuthole, sOuthole, stFLTrough, 3},
  {"Rocket Kicker",      1,swRocket,  sRocket,  stHitchH,   5},
  {"Auto Fire",          1,swAutoFire,sAutoFire,stRLMagUp, 10,0,0, SIM_STNOTEXCL},
  {"Shooter",            1,swShooter, sShooterRel,stBallLane, 5,0,0, SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",          0,0,         0,        0,          0,0,0, SIM_STNOTEXCL},
  {"Red Skill"},
  {"Orange Skill",       1,swSkillO},
  {"Yellow Skill",       1,swSkillY},
  {"Left Inlane2",       1,swLIn2,    0,        stFree,     5},
  {"Hitchhiker",         1,swHitchH,  0,        stJet1,    10},
  {"Left Outlane",       1,swLOut,    0,        stDrain,   10},
  {"Right Outlane",      1,swROut,    0,        stDrain,   10},
  {"Big Kick",           1,swBigKick},
  {"Jet Bumper 1",       1,swJet1,    0,        stJet2,    2},
  {"Jet Bumper 2",       1,swJet2,    0,        stJet3,    2},
  {"Jet Bumper 3",       1,swJet3,    0,        stFree,    2}
};

#define setMPFFree(delay) setState(stMPFFree, delay)
static int tz_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    case stRTrough:  if (core_getSol(sBallRel)) { core_setSw(swTProxi, FALSE); return setState(stShooterDiv, 6); }
                     else                      { core_setSw(swTProxi, !(ball->type & TZ_POWERBALL)); break; }
    case stRLMagDn:  if ((ball->type & TZ_POWERBALL) || !core_getSol(sRLMag)) return setFree(10);
                     else                                                    break;
    case stRLMagUp:  if ((ball->type & TZ_POWERBALL) || !core_getSol(sRLMag)) {
                       if (inports[2] & 0x0008) return setState(stRUMagUp, 5);
                       else                     return setState(stSpiral, 10);
                     }
                     else { forceState(stRLMagDn); break; } /* change direction */
    case stRUMagDn:  if ((ball->type & TZ_POWERBALL) || !core_getSol(sRUMag)) return setState(stRLMagDn,10);
                     else                                                    break;
    case stRUMagUp:  if ((ball->type & TZ_POWERBALL) || !core_getSol(sRUMag)) return setState(stSpiral, 5);
                     else { forceState(stRLMagDn); break; } /* change direction */
    case stLMagDn:   if ((ball->type & TZ_POWERBALL) || !core_getSol(sLMag)) return setFree(10);
                     else                                                   break;
    case stLMagUp:   if ((ball->type & TZ_POWERBALL) || !core_getSol(sLMag)) {
                       if (inports[2] & 0x0008) return setState(stRUMagDn, 10);
                       else                     return setState(stRLMagDn, 10);
                     }
                     else { forceState(stLMagDn); break; } /* change direction */
    case stBallLane: if (ball->speed > 10) return setState(stSkillR,5);
                     else                  return setState(stShooter, 5);
    case stSkillR:   if ((ball->speed > 13) && !core_getSw(swSkillO)) return setState(stSkillO, 1);
                     else { ball->speed = -1; return setState(stRocket, 20); }
    case stSkillO:   if ((ball->speed > 17) && !core_getSw(swSkillY)) return setState(stSkillY, 1);
                     else { ball->speed = -1; return setState(stSkillR, 2); }
    case stSkillY:   if (ball->speed > 20) { ball->speed = -1; return setState(stSlot, 5); }
                     else { ball->speed = -1; return setState(stSkillO, 2); }
                     break;
    case stBigKick:  if (core_getSol(sAutoFire)) return setState(stRLMagUp, 10);
                     else if (!core_getSw(swAutoFire)) return setState(stAutoFire, 2);
                     break;
  }
  return 0;
}

static void tz_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls) {

  initBallType(inports[2] & 0x0007, TZ_POWERBALL);

  /*-- set hardware status --*/
  core_updInvSw(swRUMag,   (inports[2] & 0x0008));
  core_updInvSw(swBigKick, (inports[2] & 0x0010));
  core_updInvSw(swCLane,   (inports[2] & 0x0020));
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData tz_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0011, stLMagUp},
  {0, 0x0012, stRLMagUp},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0021, swLMPF,   stMPFFree},
  {0, 0x0022, swRMPF,   stMPFFree},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, swLIn1},
  {0, 0x0082, swRIn},
  {0, 0x0100, swPowerPay},
  {0, 0x0200, swJet1},
  {0, 0x0400, swJet2},
  {0, 0x0800, swJet3},
  {0, 0x1000, stMPFTop,  stMPFFree},
  {0, 0x2000, stMPFExit, stMPFFree},
  {0, 0x4000, swCLane},
  {0, 0x8000, swBuyIn, SIM_STANY},
  {1, 0x0001, swGreed1},
  {1, 0x0002, swGreed2},
  {1, 0x0004, swGreed3},
  {1, 0x0008, swGreed4},
  {1, 0x0010, swGreed5},
  {1, 0x0020, swGreed6},
  {1, 0x0040, swGreed7},
  {1, 0x0080, swClockM},
  {1, 0x0100, swLIn2},
  {1, 0x0200, stSlot},
  {1, 0x0400, stCamera},
  {1, 0x0800, stPiano},
  {1, 0x1000, stDeadEnd},
  {1, 0x2000, stULock},
  {1, 0x4000, stRocket},
  {1, 0x8000, stHitchH},
  {2, 0x8000, stDrain, SIM_STANY},
  {0}
};

/*--------------------
/ Drawing information
/---------------------*/
static core_tLampDisplay tz_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{28,13,ORANGE}}},{1,{{28,10,ORANGE}}},{1,{{26,10,ORANGE}}},{1,{{24,10,ORANGE}}},
 {1,{{22,10,ORANGE}}},{1,{{20,10,ORANGE}}},{1,{{18,10,ORANGE}}},{1,{{18,13,ORANGE}}},
 {1,{{25,14,RED}}},{1,{{28,16,ORANGE}}},{1,{{26,16,ORANGE}}},{1,{{24,16,ORANGE}}},
 {1,{{22,16,ORANGE}}},{1,{{20,16,ORANGE}}},{1,{{18,16,ORANGE}}},{1,{{21,14,RED}}},
 {1,{{27, 1,RED}}},{1,{{25,12,RED}}},{1,{{26, 3,WHITE}}},{1,{{23,12,WHITE}}},
 {1,{{26, 5,WHITE}}},{1,{{21,12,RED}}},{1,{{23, 4,YELLOW}}},{1,{{20, 5,WHITE}}},
 {1,{{32, 8,YELLOW}}},{1,{{31,10,YELLOW}}},{1,{{30,12,YELLOW}}},{1,{{30,14,YELLOW}}},
 {1,{{31,16,YELLOW}}},{1,{{32,18,RED}}},{1,{{36,13,RED}}},{1,{{26,21,WHITE}}},
 {1,{{ 5,11,WHITE}}},{1,{{ 7,12,WHITE}}},{1,{{ 9,13,ORANGE}}},{1,{{ 6, 9,WHITE}}},
 {1,{{14,11,WHITE}}},{1,{{ 6,17,ORANGE}}},{1,{{ 3,19,RED}}},{1,{{ 1,20,ORANGE}}},
 {1,{{17, 1,RED}}},{1,{{22, 3,ORANGE}}},{1,{{17, 6,YELLOW}}},{1,{{19, 7,YELLOW}}},
 {1,{{17, 8,YELLOW}}},{1,{{27,23,RED}}},{1,{{14,22,WHITE}}},{1,{{12,23,ORANGE}}},
 {1,{{16,17,YELLOW}}},{1,{{14,16,YELLOW}}},{1,{{12,16,YELLOW}}},{1,{{10,18,WHITE}}},
 {1,{{ 5,20,YELLOW}}},{1,{{ 9, 3,YELLOW}}},{1,{{ 7, 2,RED}}},{1,{{ 7, 4,ORANGE}}},
 {1,{{ 5, 6,YELLOW}}},{1,{{ 5,15,WHITE}}},{1,{{ 8,21,YELLOW}}},{1,{{ 7,21,RED}}},
 {1,{{14,19,YELLOW}}},{1,{{15,23,RED}}},{1,{{38,27,YELLOW}}},{1,{{38, 1,YELLOW}}}
}
};

static void tz_drawMech(BMTYPE **line) {
//  core_textOutf(50, 0,BLACK,"%2d:%02d",locals.clockTick/(4*),
//                          (locals.clockTick*15/TZ_CLOCKTICKS) % 60);
  core_textOutf(50,0,BLACK,"%2d:%02d", mech_getPos(0)/60,mech_getPos(0) % 60);
  core_textOutf(50,10,BLACK,"Gum:%3d",locals.gumPos);
}
  /* Help */

static void tz_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+S = L/R Spiral");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+M = L/R MiniPF Switches");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W = Clock Target");
  core_textOutf(30,110,BLACK,"R = Rocket Kicker, Y = Power Payoff");
  core_textOutf(30,120,BLACK,"T/E= MiniPF Top/Exit U = Clock Lane");
  core_textOutf(30,130,BLACK,"A/F/G/J/K/:/\" = Greed Targets");
  core_textOutf(30,140,BLACK,"S = Slot Machine, D = Dead End");
  core_textOutf(30,150,BLACK,"H = Hitchhiker, L = Lock, I = LInn.Inl");
  core_textOutf(30,160,BLACK,"Z = Piano, C = Camera, V/B/N = Jet B.");

}

/*-----------------
/  ROM definitions
/------------------*/
#define TZ_SOUND \
WPCS_SOUNDROM882("tzu18_l2.rom", CRC(66575ec2) SHA1(deceb56324ee9785946f5771f8cfbaf1b1d2c8bc), \
                 "tzu15_l2.rom", CRC(389d2442) SHA1(58a4bc7cc7a28b47c75d5c9bbf14abf34bd7a9e3), \
                 "tzu14_l2.rom", CRC(5a67bd56) SHA1(98669fbfdc5793bcf09fe72c231e2b4fa2524cc5))

WPC_ROMSTART(tz,92,  "tzone9_2.rom",0x80000,CRC(ec3e61c8) SHA1(378c33add72c934aa2ee32e71830297ad1f08ce5)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,94h, "tz_94h.rom",  0x80000,CRC(5032e8c6) SHA1(d7481612b1c3040823e1f7b9e53ebbaa83de0532)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,94ch, "tz_94ch.rom",0x80000,CRC(e3d3f3ef) SHA1(c2a856ffef84718382d840d9aa7ae41706fef1fd)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,l2,  "tz_l2.u6",    0x80000,CRC(1f0f5611) SHA1(e8860b1c288039682e56bbf8dd0c263b2632c4b7)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,l3,  "tz_l3.u6",    0x80000,CRC(c35fe03c) SHA1(44d2267ab278d092385e7ba21da8e6cbc2b69bf4)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,l4,  "tz_l4.u6",    0x80000,CRC(4baf5acd) SHA1(1edef7de6c3d24ef61e59b688d7b6871d88fd3b5)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,ifpa,"u6-ifpa.040", 0x80000,CRC(57f4c514) SHA1(17064a76e1037d439639ebbc9e64ca4fd1e5e62d)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,h7,  "u6-h7.040",   0x80000,CRC(84f29e46) SHA1(744c7e64418c8d95972470e4aafe42e96d2ea9cf)) TZ_SOUND WPC_ROMEND
WPC_ROMSTART(tz,h8,  "tz_h8.u6",    0x80000,CRC(f1b2d60c) SHA1(295fc07ae83f2cfbc84caf581915f99080fa397d)) TZ_SOUND WPC_ROMEND

WPC_ROMSTART(tz,l1,"u6-l1.040",0x80000,CRC(6db6ae06) SHA1(a0e15c5f5e94391c3f0e77155307c3aacba1aff9))
WPCS_SOUNDROM882("tzu18_l1.rom", CRC(a021a494) SHA1(27bbb60fce2892b1b0b611687a1ac59c7c668f9d),
                 "tzu15_l2.rom", CRC(389d2442) SHA1(58a4bc7cc7a28b47c75d5c9bbf14abf34bd7a9e3),
                 "tzu14_l2.rom", CRC(5a67bd56) SHA1(98669fbfdc5793bcf09fe72c231e2b4fa2524cc5))
WPC_ROMEND

WPC_ROMSTART(tz,pa1,"u6-pa1.040",  0x80000,CRC(c34e06a6) SHA1(09c8097a54fdc15de7ee3e3eb5b12952c0bf5eaf))
WPCS_SOUNDROM888("u18-sp1.040", CRC(1632951e) SHA1(041396411dc5343fe7e5147d26e038de79982464),
                 "u15-sp1.040", CRC(0f17c9e9) SHA1(fcaa6f87ebd03222e3a40be08eb5aa6a5e002a8b),
                 "u14-sp1.040", CRC(ad7cb98b) SHA1(a84bf157cb535acaf811e93ad22a505e1dd08dad))
WPC_ROMEND

WPC_ROMSTART(tz,p4,"tz_p4.rom", 0x80000,CRC(5a662df5) SHA1(0f609ff59549225d56b913c3bf928b58f7bf1ca5))
WPCS_SOUNDROM888("tzu18_p3.rom",CRC(1f750672) SHA1(033c6e261201a17667110069b7570fe90490286b),
                 "u15-sp1.040", CRC(0f17c9e9) SHA1(fcaa6f87ebd03222e3a40be08eb5aa6a5e002a8b),
                 "u14-sp1.040", CRC(ad7cb98b) SHA1(a84bf157cb535acaf811e93ad22a505e1dd08dad))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (tz,92,     "Twilight Zone (9.2)", 1995,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,94h, 92,"Twilight Zone (9.4H)",1998,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,94ch, 92,"Twilight Zone (9.4CH)",1998,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,pa1, 92,"Twilight Zone (PA-1)",1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,p4,  92,"Twilight Zone (P-4)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,l1,  92,"Twilight Zone (L-1)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,l2,  92,"Twilight Zone (L-2)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,ifpa,92,"Twilight Zone (IFPA rules)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,l3,  92,"Twilight Zone (L-3)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,l4,  92,"Twilight Zone (L-4)", 1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,h7,  92,"Twilight Zone (H-7)", 1994,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tz,h8,  92,"Twilight Zone (H-8)", 1994,"Bally",wpc_mFliptronS,0)

/*----------
/ Game Data
/-----------*/
static sim_tSimData tzSimData = {
  3,        /* 5 game specific input ports */
  tz_stateDef,
  tz_inportData,
  { stRTrough, stCTrough, stLTrough, stGum1, stGum2, stGum3, stFLTrough },
  tz_initSim,
  tz_handleBallState,
  tz_drawStatic,
  TRUE, /* simulate manual Shooter */
  NULL, /* no custom conditions */
};

static core_tGameData tzGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,/* generation */
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U),
    1,0,9,0,0,0,0, // 1 switch column, 9 solenoids
    tz_getSol, tz_handleMech, tz_getMech, tz_drawMech,
    &tz_lampPos, NULL
  },
  &tzSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x7f, 0x00, 0x00, 0x00, 0xff}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
/*-- patched memory read function to handle the 9th switch column --*/
READ_HANDLER(tz_swRowRead) {
  if ((wpc_data[WPC_EXTBOARD1] & 0x80) > 0) /* 9th column enabled */
    return coreGlobals.swMatrix[CORE_CUSTSWCOL];
  return wpc_r(WPC_SWROWREAD);
}
/*
   2 3 4 5 6 7 8 9 a b c 1
   c 1 2 3 4 5 6 7 8 9 a b
   012345678901234567890123
      xxxx          xxxx
    xxxxxxxxxxxx
            xxxxxxxxxx
        xxxxxx    xxxxxxxx
*/
#define TZ_CLOCKTICKS   10 /* Time for clock to move 15 minutes */
static mech_tInitData mechClock = {
  sClockRev, sClockFwd, MECH_TWODIRSOL|MECH_FAST,TZ_CLOCKTICKS*48*8,720,
  {{swClockH4,210,329}, //  3:30 -  5:30
   {swClockH4,630,719}, // 10:30 - 12:00
   {swClockH4,  0, 29}, // 12:00 - 12:30
   {swClockH3,150,509}, //  2:30 -  8:30
   {swClockH2,390,689}, //  6:30 - 11:30
   {swClockH1,270,450}, //  4:30 -  7:30
   {swClockH1,570,719}, //  9:30 - 12:00
   {swClockH1,  0, 89}, // 12:00 -  1:30
   {swClockM0,  0,  2,60}, //  0 min
   {swClockM15,15, 17,60}, // 15 min
   {swClockM30,30, 32,60}, // 30 min
   {swClockM45,45, 47,60}} // 45 min
};

static void init_tz(void) {
  core_gameData = &tzGameData;
  install_mem_read_handler(WPC_CPUNO, WPC_SWROWREAD+WPC_BASE, WPC_SWROWREAD+WPC_BASE,
                           tz_swRowRead);
  mech_add(0,&mechClock);
}

static int tz_getSol(int solNo) {
  if (solNo == sGumRel)  return locals.gumRel;

  if ((solNo >= CORE_CUSTSOLNO(1)) && (solNo <= CORE_CUSTSOLNO(8)))
    return ((wpc_data[WPC_EXTBOARD1]>>(solNo-CORE_CUSTSOLNO(1)))&0x01);
  return 0;
}
static void tz_handleMech(int mech) {
  /*---------------
  /  handle clock
  /----------------*/
#if 0
  if (mech & 0x01) {
    switch (2*(core_getSol(sClockFwd)>0)+(core_getSol(sClockRev)>0)) {
      case 0:
      case 3: break;
      case 1: locals.clockTick += 1; if (locals.clockTick == TZ_CLOCKTICKS*48) locals.clockTick = 0; break;
      case 2: locals.clockTick -= 1; if (locals.clockTick == -1) locals.clockTick = TZ_CLOCKTICKS*48-1; break;
    }
    { /* convert clockTick to optos */
      static int hour2sw[12] = {0x09,0x01,0x00,0x04,0x0c,0x0d,0x05,0x07,0x06,0x02,0x03,0x0b};
      int tickM = locals.clockTick % (TZ_CLOCKTICKS * 4);
      int hour = hour2sw[((locals.clockTick+2*TZ_CLOCKTICKS)/(4*TZ_CLOCKTICKS)+12) % 12];

      core_setSw(swClockM15, (tickM == TZ_CLOCKTICKS));
      core_setSw(swClockM0,  (tickM == 0));
      core_setSw(swClockM30, (tickM == TZ_CLOCKTICKS*2));
      core_setSw(swClockM45, (tickM == TZ_CLOCKTICKS*3));
      core_setSw(swClockH4, hour & 0x08);
      core_setSw(swClockH3, hour & 0x04);
      core_setSw(swClockH2, hour & 0x02);
      core_setSw(swClockH1, hour & 0x01);
    }
  }
#endif
  /*-------------------------
  /  handle gumball machine
  /--------------------------*/
  if (mech & 0x02) {
    if (locals.gumRel > 0)
      locals.gumRel -= 1;
    if (core_getSol(sGumMotor)) {
      locals.gumPos = (locals.gumPos + 1) % TZ_GUMTIME;
      if (locals.gumPos == 0)
        locals.gumRel = TZ_GUMRELTIME;
    }
    /* dont' know how much "slack" there are in the geneva switch */
    core_setSw(swGeneva, locals.gumPos < TZ_GUMTIME/5); /* 20% slack */
  }
}

/* get Status of mechanics */
/* 0 = time min after 12:00 */
/* 1 = Gumball motor position (0-119) */
static int tz_getMech(int mechNo) {
  return mechNo ? locals.gumPos : mech_getPos(0);
}
