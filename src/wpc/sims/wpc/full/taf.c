#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/* 061200 Fixed loop timings so Stars can be awarded (ML)*/
/* 051200 Added Custom Lamp Matrix (SJE)		*/
/* 251100 Added key for the Front swamp hole (ML)	*/
/* 161000 TAFG is not a clone of TAF anymore            */
/*        Added TAFG H3 ROM and Sound in TAFG           */
/*        Added Buy-in button and cleaned up BITIMP use */
/*        Added swBookOpen condition to Vault           */
/*        Adjusted timing. Thing Flip now works         */
/* 181000 Corrected year and names */
/* 231000 Now uses textOutf */
/* 160401 Added GetMech */
/* 020801 Added sample support (SJE)*/
/*--------------
/ Keys
/   +R  L/R Ramp
/   +I  L/R Inlane
/   +-  L/R Slingshot
/   +L  L/R Loop
/   +O  L/R Outlane
/   V   Vault
/   S   Swamp
/   K   Lock Kickout Hole
/   C   Chair
/   T   Thing Lane
/   Q   SDTM
/   B   Bookcase
/   I   Left Inner Inlane
/   H   Thing Hole
/   W   Train Wreck
/  EYU  G-R-A (V-E)
/ NM,./ Jet Bumpers
/ DFG   Swamp Milions
/  Z    Cousin It
/---------------------------*/
#define TAF_BOOKTICKS  200
#define TAF_THINGTICKS 400

/*------------------
/  Local functions
/-------------------*/
/*-- local state/ball handling functions --*/
static int taf_handleBallState(sim_tBallStatus *ball, int *inports);
/*static int taf_getSol(int solNo);*/
static void taf_handleMech(int mech);
static int taf_getMech(int mechNo);
static void taf_drawMech(BMTYPE **line);
static void taf_drawStatic(BMTYPE **line);
static void init_taf(void);

/*-----------------------
/ local static variables
/------------------------*/
static struct {
  int bookPos;  /* bookcase position */
  int thingPos; /* thing position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(taf,3)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"L/R Inlane",      KEYCODE_I)
    COREPORT_BITIMP(0x0100,"Vault",           KEYCODE_V)
    COREPORT_BITIMP(0x0200,"Swamp",           KEYCODE_S)
    COREPORT_BITIMP(0x0400,"Chair",           KEYCODE_C)
    COREPORT_BITIMP(0x0800,"Thing",           KEYCODE_T)
    COREPORT_BIT(   0x1000,"Bookcase",        KEYCODE_B)
    COREPORT_BIT(   0x2000,"Left Inner Inlane",KEYCODE_I)
    COREPORT_BITIMP(0x4000,"Thing Hole",      KEYCODE_H)
    COREPORT_BIT(   0x8000,"Train Wreck",     KEYCODE_W)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"Grave",           KEYCODE_E)
    COREPORT_BIT(   0x0002,"gRave",           KEYCODE_Y)
    COREPORT_BIT(   0x0004,"grAve",           KEYCODE_U)
    COREPORT_BIT(   0x0008,"It",              KEYCODE_Z)
    COREPORT_BIT(   0x0010,"Upper Swamp",     KEYCODE_D)
    COREPORT_BIT(   0x0020,"Center Swamp",    KEYCODE_F)
    COREPORT_BIT(   0x0040,"Lower Swamp",     KEYCODE_G)
    COREPORT_BIT(   0x0100,"Jet 1",           KEYCODE_N)
    COREPORT_BIT(   0x0200,"Jet 2",           KEYCODE_M)
    COREPORT_BIT(   0x0400,"Jet 3",           KEYCODE_COMMA)
    COREPORT_BIT(   0x0800,"Jet 4",           KEYCODE_STOP)
    COREPORT_BIT(   0x1000,"Jet 5",           KEYCODE_SLASH)
    COREPORT_BITIMP(0x2000,"Drain",           KEYCODE_Q)
    COREPORT_BIT(   0x4000,"Buy-in",          KEYCODE_2)
    COREPORT_BIT(   0x8000,"Lock Kickout",    KEYCODE_K)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swBuyIn      11
#define swStart      13
#define swTilt       14
#define swLTrough    15
#define swCTrough    16
#define swRTrough    17
#define swOuthole    18

#define swSlamTilt   21
#define swCoinDoor   22
#define swTicket     23
#define swRIn        25
#define swROut       26
#define swShooter    27

#define swJet1       31
#define swJet2       32
#define swJet3       33
#define swJet4       34
#define swJet5       35
#define swLSling     36
#define swRSling     37
#define swLLoop      38

#define swGrave      41
#define swgRave      42
#define swChair      43
#define swIt         44
#define swLSwamp     45
#define swCSwamp     47
#define swUSwamp     48

#define swBallLane   51
#define swBook1      53
#define swBook2      54
#define swBook3      55
#define swBook4      56
#define swBumpLane   57
#define swRRampExit  58

#define swLRampEnt   61
#define swTrain      62
#define swThLane     63
#define swRRampEnt   64
#define swRRamp      65
#define swLRamp      66
#define swRLoop      67
#define swVault      68

#define swULock      71
#define swCLock      72
#define swLLock      73
#define swLKick      74
#define swLOut       75
#define swLIn2       76
#define swThKick     77
#define swLIn1       78

#define swBookOpen   81
#define swBookClose  82
#define swThingDn    84
#define swThingUp    85
#define swgrAve      86
#define swThHole     87

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sChair        1
#define sThKnock      2
#define sRampDiv      3
#define sBallRel      4
#define sOuthole      5
#define sThMag        6
#define sThKick       7
#define sLKick        8
#define sJet1         9
#define sJet2        10
#define sJet3        11
#define sJet4        12
#define sJet5        13
#define sLSling      14
#define sRSling      15
#define sLMag        16
#define sUMag        23
#define sRMag        24
#define sThMotor     25
#define sThHole      26
#define sBookMotor   27
#define sSwampRel    28

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOuthole, stRIn,
      stROut, stShooter, stLLoopUp, stLLoopDn, stChair, stBallLane, stBumpLane,
      stRRampExit, stLRampEnt, stThLane, stRRampEnt, stRRamp, stLRamp,
      stRLoopUp, stRLoopDn, stVault, stULock, stCLock, stLLock, stLKick,
      stLOut, stThKick, stThHole,
      stDrain, stRampDiv, stTFlip, stUSwamp, stCSwamp, stLSwamp, stBallLane2,
      stThing};

static sim_tState taf_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",               0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Right Trough",     1,swRTrough,   sBallRel, stShooter,   5},
  {"Center Trough",    1,swCTrough,   0,        stRTrough,   1},
  {"Left Trough",      1,swLTrough,   0,        stCTrough,   1},
  {"Outhole",          1,swOuthole,   sOuthole, stLTrough,   5},
  {"Right Inlane",     1,swRIn,       0,        stFree,      5},
  {"Right Outlane",    1,swROut,      0,        stDrain,    20},
#ifdef NEWSHOOT
  {"Ball Shooter",     1,swShooter,   0,        0,           0,0,0,SIM_STNOTEXCL|SIM_STSHOOT1},
#else
  {"Ball Shooter",     1,swShooter,   sShooterRel,stBallLane, 10,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
#endif
  {"Left Loop",        1,swLLoop,     0,        stRLoopDn,  10},
  {"Left Loop",        1,swLLoop,     0,        stFree,     5},
  {"Chair",            1,swChair,     sChair,   stFree,     10,0,0,SIM_STNOTEXCL},
  {"Ball Lane",        1,swBallLane,  0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Bumper Lane",      1,swBumpLane},
  {"Right Ramp Exit",  1,swRRampExit, 0,        stRIn,       5},
  {"Left Ramp Enter",  1,swLRampEnt,  0,        stLRamp,     5},
  {"Thing Lane",       1,swThLane,    0,        stThHole,   10},
  {"Right Ramp Enter", 1,swRRampEnt,  0,        stRRamp,     5},
  {"Right Ramp",       1,swRRamp,     0,        stRampDiv,   5},
  {"Left Ramp",        1,swLRamp,     0,        stRampDiv,   5},
  {"Right Loop",       1,swRLoop,     0,        stLLoopDn,  10},
  {"Right Loop",       1,swRLoop,     0,        stFree,     5},
  {"Vault",            1,swVault,     0,        stULock,    20},
  {"Upper Lock",       1,swULock,     0,        stCLock,     1},
  {"Center Lock",      1,swCLock,     0,        stLLock,     1},
  {"Lower Lock",       1,swLLock,     sSwampRel,stLKick,     5},
  {"Lock Kickout",     1,swLKick,     sLKick,   stFree,     10},
  {"Left Outlane",     1,swLOut,      0,        stDrain,    20},
  {"Thing Kickout",    1,swThKick,    0,        stULock,    20},
  {"Thing Hole",       1,swThHole,    sThHole,  0,           0,0,0,SIM_STIGNORESOL},
  {"Drain",            1,0,           0,        stOuthole,   0,0,0,SIM_STNOTEXCL},
  {"Ramp Diverter",    1,0,           0,        stRRampExit,15,sRampDiv,stBumpLane},
  {"Thing flip",       1,0,           0,        0,           0},
  {"Swamp Million",    1,swUSwamp,    0,        stFree,     10},
  {"Swamp Million",    1,swCSwamp,    0,        stFree,     10},
  {"Swamp Million",    1,swLSwamp,    0,        stFree,     10},
  {"Upper Ball Lane",  1},
  {"On Thing",         1},
  {0}
};

static int taf_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    case stBallLane:  if (ball->speed > 5)     return setState(stBallLane2,10);
                      else { ball->speed = -1; return setState(stULock,10); }
    case stBumpLane:  ball->custom = 0; return setState(stTFlip, 1);
    case stThHole:    if (core_getSw(swThingUp) && core_getSol(sThMag))
                        return setState(stThing,5);
                      if (core_getSol(sThHole))
                        return setFree(10);
                      break;
    case stTFlip:     if (core_getSol(sULFlip)) {
                        if (ball->custom < 1)  return setFree(5);
                        if (ball->custom < 2)  return setState(stUSwamp,10);
                        if (ball->custom < 3)  return setState(stULock, 15);
                        if (ball->custom < 4)  return setState(stCSwamp,10);
                        if (ball->custom < 5)  return setState(stLSwamp,10);
                      }
                      if (ball->custom >= 5)   return setFree(10);
                      ball->custom += 1;
                      break;
    case stBallLane2: if (ball->speed > 20) {
                        if (ball->speed < 25) return setState(stThHole,5);
                        else                  return setState(stRLoopDn,10);
                      }
                      else {ball->speed = -1; return setState(stBallLane,10); }
    case stThing:     if (!core_getSol(sThMag)) {
                        if (core_getSw(swThingUp)) return setState(stThHole, 1);
                        if (core_getSw(swThingDn)) return setState(stThKick,10);
                        /* ball released between switches ?? */
                      }
                      break;
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData taf_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, swLIn1},
  {0, 0x0082, stRIn},
  {0, 0x0100, stVault, swBookOpen},
  {0, 0x0200, stULock},
  {0, 0x0400, stChair},
  {0, 0x0800, stThLane},
  {0, 0x1000, swBook1},
  {0, 0x2000, swLIn2},
  {0, 0x4000, stThHole},
  {0, 0x8000, swTrain},
  {1, 0x0001, swGrave},
  {1, 0x0002, swgRave},
  {1, 0x0004, swgrAve},
  {1, 0x0008, swIt},
  {1, 0x0010, stUSwamp},
  {1, 0x0020, swCSwamp},
  {1, 0x0040, stLSwamp},
  {1, 0x0100, swJet1},
  {1, 0x0200, swJet2},
  {1, 0x0400, swJet3},
  {1, 0x0800, swJet4},
  {1, 0x1000, swJet5},
  {1, 0x2000, stDrain},
  {1, 0x4000, swBuyIn},
  {1, 0x8000, stLKick},
  {0}
};

/*--------------------
/ Drawing information
/---------------------*/
static core_tLampDisplay taf_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{ 7,11,ORANGE}}},{1,{{ 9,11,RED}}},{1,{{11,10,RED}}},{1,{{10,13,LBLUE}}},
 {1,{{12, 7,ORANGE}}},{1,{{13, 9,RED}}},{1,{{14,13,LBLUE}}},{1,{{19,22,ORANGE}}},
 {1,{{14, 1,RED}}},{1,{{14, 5,WHITE}}},{1,{{17, 2,LBLUE}}},{1,{{17, 6,YELLOW}}},
 {1,{{20, 4,ORANGE}}},{1,{{16, 8,WHITE}}},{1,{{16,12,WHITE}}},{1,{{18,11,ORANGE}}},
 {1,{{13,17,WHITE}}},{1,{{14,18,WHITE}}},{1,{{15,19,WHITE}}},{1,{{16,20,WHITE}}},
 {1,{{17,21,WHITE}}},{1,{{20,20,RED}}},{1,{{21,24,ORANGE}}},{1,{{22,23,ORANGE}}},
 {1,{{ 0, 0,BLACK}}},{1,{{20, 6,LPURPLE}}},{1,{{23, 4,LBLUE}}},{1,{{22, 5,LBLUE}}},
 {1,{{34,17,ORANGE}}},{1,{{21,18,WHITE}}},{1,{{18, 7,RED}}},{1,{{24,22,LBLUE}}},
 {1,{{30,11,GREEN}}},{1,{{30,13,ORANGE}}},{1,{{30,15,RED}}},{1,{{32, 9,WHITE}}},
 {1,{{32,11,GREEN}}},{1,{{32,13,ORANGE}}},{1,{{32,15,ORANGE}}},{1,{{34,15,ORANGE}}},
 {1,{{30, 1,RED}}},{1,{{28, 3,ORANGE}}},{1,{{28, 5,WHITE}}},{1,{{18, 5,YELLOW}}},
 {1,{{28,13,RED}}},{1,{{30, 9,WHITE}}},{1,{{34,11,YELLOW}}},{1,{{34, 9,WHITE}}},
 {1,{{28,22,WHITE}}},{1,{{29,26,RED}}},{1,{{37,14,RED}}},{1,{{ 5,22,GREEN}}},
 {1,{{ 7,22,RED}}},{1,{{ 0, 1,BLACK}}},{1,{{ 4,10,YELLOW}}},{1,{{ 4,12,GREEN}}},
 {1,{{ 1, 5,YELLOW}}},{1,{{ 1, 8,YELLOW}}},{1,{{ 1,11,YELLOW}}},{1,{{ 1,14,YELLOW}}},
 {1,{{ 1,17,YELLOW}}},{1,{{ 1,20,YELLOW}}},{1,{{ 1,23,YELLOW}}},{1,{{39, 1,YELLOW}}}
}
};

//Solenoids not mapped
//sThMotor
//sBookMotor
//sSwampRel
static wpc_tSamSolMap taf_SamSolMap[] = {
 /*Channel #0*/
 //{sKnocker,0,SAM_KNOCKER},

{sBallRel,0,SAM_BALLREL},{sOuthole,0,SAM_OUTHOLE}, {sChair,0,SAM_POPPER},

/*Channel #1*/
{sLSling,1,SAM_LSLING}, {sRSling,1,SAM_RSLING},
{sJet1,1,SAM_JET1}, {sJet2,1,SAM_JET2},
{sJet3,1,SAM_JET3}, {sJet4,1,SAM_JET1}, {sJet5,1,SAM_JET2},

 /*Channel #2*/
{sLKick,2,SAM_POPPER},{sRampDiv,2,SAM_SOLENOID},

/*Channel #3*/
{sThHole,3,SAM_SOLENOID}, {sThKick,3,SAM_SOLENOID},
{-1}
};

static void taf_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"Book: %-6s", core_getSw(swBookOpen) ? "Open" : (core_getSw(swBookClose) ? "Closed" : ""));
  core_textOutf(50,10,BLACK,"Thing: %-4s", core_getSw(swThingUp) ? "Up" : (core_getSw(swThingDn) ? "Down" : ""));
}
  /* Help */

static void taf_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W = Train Wreck");
  core_textOutf(30,100,BLACK,"E/Y/U = G/R/A (V-E) Targets");
  core_textOutf(30,110,BLACK,"T = Thing Lane, I = Left Inner Inlane");
  core_textOutf(30,120,BLACK,"S = Swamp, D/F/G = Swamp Millons");
  core_textOutf(30,130,BLACK,"H = Thing Hole, Z = Cousin It");
  core_textOutf(30,140,BLACK,"C = Chair, V = Vault");
  core_textOutf(30,150,BLACK,"B = Bookcase, K = Lock Kickout");
  core_textOutf(30,160,BLACK,"N/M/,/.// = Jet Bumpers");
}
/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(taf,p2,  "addam_p2.rom", 0x40000, CRC(eabf0e72) SHA1(5b84d0315702b39b90beb6a92fb7ad9aba7e620c))
WPCS_SOUNDROM8xx("afsnd_p2.rom" , CRC(73d19698) SHA1(d14a6ea36a93db185a599a7810dfbef2deb0adc0))
WPC_ROMEND
WPC_ROMSTART(taf,l1,  "addam_l1.rom", 0x40000, CRC(db287bf7) SHA1(51574c7c04d85aa816a0bc6e9db74f2d2b407525))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,l2,  "addam_l2.rom", 0x40000, CRC(952bfc92) SHA1(d95b4b9e6c496a9ce4ceb1aa368c862b2beeffd9))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,l3,  "addam_l3.rom", 0x40000, CRC(d428a760) SHA1(29afee7b1ae64d7a41faf813cdfa1ab7cef1f247))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,l4,  "addam_l4.rom", 0x40000, CRC(ea29935f) SHA1(9f711396728026546c8bd1f69a0833d15e02c192))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,l7,  "addam_l7.rom", 0x80000, CRC(4401b43a) SHA1(64e9678334cc900d1f44b95d25bb90c1fff566f8))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,l5,  "addam_l5.rom",0x80000,CRC(4c071564) SHA1(d643506db1b3ba1ea20f34ddb38837df379fb5ab))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND

WPC_ROMSTART(taf,l6,  "taf_l6.u6",0x80000,CRC(06b37e65) SHA1(ce6f9cc45df08f50f5ece2a4c9376ecf67b0466a))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND
WPC_ROMSTART(taf,h4,  "addam_h4.rom",0x80000, CRC(d0bbd679) SHA1(ebd8c4981dd68a4f8e2dea90144486cb3cbd6b84))
WPCS_SOUNDROM8xx("tafu18l1.rom",CRC(131ae471) SHA1(5ed03b521dfef56cbb99814539d4c74da4216f67))
WPC_ROMEND

WPC_ROMSTART(tafg,lx3,"afgldlx3.rom",0x80000,CRC(0cc62fa5) SHA1(295bd8c483132c8fe7c38646847067041fc98b2f))
WPCS_SOUNDROM84x("ag_u18_s.l1",CRC(02e824a9) SHA1(ed8aa5161ea6c12cc9e646939290d848408a59a3),
                 "ag_u15_s.l1",CRC(b8c88c75) SHA1(b2b88e5192eb817ae60ab1f306e932d8bae3fbba))
WPC_ROMEND
WPC_ROMSTART(tafg,h3,"cpu-u6h3.rom",0x80000,CRC(2fe97098) SHA1(59767fce189385af16064abf0d640ef74a3450f8))
WPCS_SOUNDROM84x("ag_u18_s.l1",CRC(02e824a9) SHA1(ed8aa5161ea6c12cc9e646939290d848408a59a3),
                 "ag_u15_s.l1",CRC(b8c88c75) SHA1(b2b88e5192eb817ae60ab1f306e932d8bae3fbba))
WPC_ROMEND
WPC_ROMSTART(tafg,la2,"u6-la2.rom",0x80000,CRC(a9a42bff) SHA1(40bb8e2767219582e7e532d2154213748808c62b))
WPCS_SOUNDROM84x("ag_u18_s.l1",CRC(02e824a9) SHA1(ed8aa5161ea6c12cc9e646939290d848408a59a3),
                 "ag_u15_s.l1",CRC(b8c88c75) SHA1(b2b88e5192eb817ae60ab1f306e932d8bae3fbba))
WPC_ROMEND
WPC_ROMSTART(tafg,la3,"u6-la3.rom",0x80000,CRC(dbe4d791) SHA1(6906e9c2158e86ab05b6783567b42a245239fed9))
WPCS_SOUNDROM84x("ag_u18_s.l1",CRC(02e824a9) SHA1(ed8aa5161ea6c12cc9e646939290d848408a59a3),
                 "ag_u15_s.l1",CRC(b8c88c75) SHA1(b2b88e5192eb817ae60ab1f306e932d8bae3fbba))
WPC_ROMEND
/*--------------
/  Game drivers
/---------------*/
/*-- TAFG uses TAF input ports and init functions --*/
#define input_ports_tafg input_ports_taf
#define init_tafg        init_taf

CORE_GAMEDEF(taf,l5,        "The Addams Family (L-5)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,p2,l5,    "The Addams Family (Prototype) (P-2)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l1,l5,    "The Addams Family (L-1)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l2,l5,    "The Addams Family (L-2)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l3,l5,    "The Addams Family (L-3)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l4,l5,    "The Addams Family (L-4)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l7,l5,    "The Addams Family (Prototype L-5) (L-7)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,l6,l5,    "The Addams Family (L-6)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(taf,h4,l5,    "The Addams Family (H-4)",1992,"Bally",wpc_mFliptronS,0)
CORE_GAMEDEF(tafg,lx3,     "The Addams Family Special Collectors Edition Gold (LX-3)", 1994,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tafg,h3,lx3,"The Addams Family Special Collectors Edition (H-3)",1994,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tafg,la2,lx3,"The Addams Family Special Collectors Edition (LA-2)",1994,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(tafg,la3,lx3,"The Addams Family Special Collectors Edition (LA-3)",1994,"Bally",wpc_mFliptronS,0)

/*----------
/ Game Data
/-----------*/
static sim_tSimData tafSimData = {
  2,    /* 2 game specific input ports */
  taf_stateDef,
  taf_inportData,
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },
  NULL, /* no init */
  taf_handleBallState,
  taf_drawStatic,
#ifdef NEWSHOOT
  {{{10, "Swamp",      stULock,  10},
    {20, "Thing Hole", stThHole, 15},
    {25, "Loop",       stRLoopDn,20)
  }}
#else
  TRUE, /* simulate manual shooter */
#endif
  NULL  /* no custom key conditions */
};

static core_tGameData tafGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,/* generation */
  {
    FLIP_BUT(FLIP_L) | FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L|FLIP_U),
    0,0,0,0,0,0,0,
    NULL, taf_handleMech, taf_getMech, taf_drawMech,
    &taf_lampPos, taf_SamSolMap
  },
  &tafSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};


/*---------------
/  Game handling
/----------------*/
static void init_taf(void) {
  core_gameData = &tafGameData;
}

static void taf_handleMech(int mech) {
  /*-- update bookcase switches --*/
  if (mech & 0x01) {
    if (core_getSol(sBookMotor)) locals.bookPos = (locals.bookPos + 1) % TAF_BOOKTICKS;
    core_setSw(swBookOpen, locals.bookPos < 15);
    core_setSw(swBookClose, (locals.bookPos >= (TAF_BOOKTICKS/2)) &&
                            (locals.bookPos <  (TAF_BOOKTICKS/2+15)));
  }
  /*-- update thing switches --*/
  if (mech & 0x02) {
    if (core_getSol(sThMotor))   locals.thingPos = (locals.thingPos + 1) % TAF_THINGTICKS;
    core_setSw(swThingDn, locals.thingPos < 15);
    core_setSw(swThingUp, (locals.thingPos >= (TAF_THINGTICKS/2)) &&
                          (locals.thingPos <  (TAF_THINGTICKS/2+15)));
  }
}

static int taf_getMech(int mechNo) {
  return mechNo ? locals.thingPos : locals.bookPos;
}
