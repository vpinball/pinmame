// license:BSD-3-Clause

/*******************************************************************************
 Dr. Dude (Bally,1990) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 1, 2000 (Converted to Sys11 on Feb. 22, 2001)

 Known Issues/Problems with this Simulator:

 None.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Simulator:
  --------------------------------------------------
    +R  MixMaster Ramp
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
     T  I Test Target (Top, Skill Shot)
  YUIO  Right Drop Targets
ASDFGH  Reflex Targets
     J  Heart Target
     K  Magnet Target
     L  Big Shot Target
     Z  Bag of Tricks Hole
     X  Gift of Gab Hole
     C  Right Loop
  VBNM  10 Pts Switches
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "s11.h"
#include "sim.h"
#include "sndbrd.h"

/*------------------
/  Local functions
/-------------------*/
static int  tm_handleBallState(sim_tBallStatus *ball, int *inports);
static void tm_handleMech(int mech);
static void tm_drawStatic(BMTYPE **line);
void init_tmac(void);

/*--------------------------
/ Game specific input ports
/---------------------------*/
DE_INPUT_PORTS_START(tmac,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Ramp",	    KEYCODE_Q)
    COREPORT_BIT(0x0002,"Right Ramp",	    KEYCODE_CLOSEBRACE)
    COREPORT_BIT(0x0004,"STARWARP Ramp",	KEYCODE_W)
    COREPORT_BIT(0x0008,"Left Outlane",		KEYCODE_Z)
    COREPORT_BIT(0x0010,"Right Outlane",	KEYCODE_SLASH)
    COREPORT_BIT(0x0020,"Left Inlane",		KEYCODE_X)
    COREPORT_BIT(0x0040,"Right Inlane",		KEYCODE_STOP)
    COREPORT_BIT(0x0100,"Right Pop",		  KEYCODE_U)
    COREPORT_BIT(0x0200,"Left Pop",		    KEYCODE_Y)
    COREPORT_BIT(0x0400,"Bottom Pop",		  KEYCODE_H)
    COREPORT_BIT(0x0800,"Left Sling",		  KEYCODE_S)
    COREPORT_BIT(0x1000,"Right Sling",		KEYCODE_COLON)
    COREPORT_BIT(0x2000,"Left Lane",		  KEYCODE_6)
    COREPORT_BIT(0x4000,"Center Lane",		KEYCODE_7)
    COREPORT_BIT(0x8000,"Right Lane",		  KEYCODE_8)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Left Bank 70s",		KEYCODE_A)
    COREPORT_BIT(0x0002,"Left Bank 60s",		KEYCODE_S)
    COREPORT_BIT(0x0004,"Left Bank 50s",		KEYCODE_D)
    COREPORT_BIT(0x0008,"Center Bank 70s",	KEYCODE_V)
    COREPORT_BIT(0x0010,"Center Bank 60s",	KEYCODE_B)
    COREPORT_BIT(0x0020,"Center Bank 50s",	KEYCODE_N)
    COREPORT_BIT(0x0040,"Right Bank 70s",		KEYCODE_J)
    COREPORT_BIT(0x0080,"Right Bank 60s",	  KEYCODE_K)
    COREPORT_BIT(0x0100,"Right Bank 50s",		KEYCODE_L)
    COREPORT_BIT(0x0200,"LPopper",		KEYCODE_Z)
    COREPORT_BIT(0x0400,"RPopper",		KEYCODE_X)
    COREPORT_BIT(0x0800,"Rt Loop",		KEYCODE_C)

DE_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt		1
#define swStart		3
#define swRightCoin 4
#define swCenterCoin 5
#define swLeftCoin 6
#define swSlamTilt 7
#define swOutHole 10
#define swRTrough 11
#define swCTrough 12
#define swLTrough 13
#define swShooterLane 14
#define swLTEOS 15
#define swRTEOS 16
#define swLTOutlane 17
#define swLTInlane 18
#define swRTOutlane 19
#define swRTInlane 20
#define swLTSling 21
#define swRTSling 22
#define swLane1 25
#define swLane2 26
#define swLane3 27
#define swLTRamp 28
#define swCTRamp 29
#define swRTRamp 30
#define swLTRollover 31
#define swRTRollover 32
#define swLTBank1 33
#define swLTBank2 34
#define swLTBank3 35
#define swLock1 36
#define swLock2 37
#define swLock3 38
#define swCTBank1 41
#define swCTBank2 42
#define swCTBank3 43
#define swSubway 44
#define swVUK 45
#define swLTPop 46
#define swCTPop 47
#define swRTPop 48
#define swRTBank1 49
#define swRTBank2 50
#define swRTBank3 51

  /*---------------------
  / Solenoid definitions
  /----------------------*/
#define sOutHole 1
#define sTrough 2
#define sLPopper 3
#define sRPopper 4
#define sKnocker 6
#define sRDrop 7
#define sMagnet 13
#define sBigShot 14
#define sMotor 16
#define sLeftJet 17
#define sLeftSling 18
#define sRightJet 19
#define sRightSling 20
#define sBottomJet 21

#define sVUK 0
#define sLock 0

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
      stLock1, stLock2, stLock3,
      stLTRamp, stRTRamp, stCTRamp, stSubway, stVUK,
      stLTLane, stRTLane, stCTLane
	  };

  static sim_tState tm_stateDef[] = {
      {"Not Installed", 0, 0, 0, stDrain, 0, 0, 0, SIM_STNOTEXCL},
      {"Moving"},
      {"Playfield", 0, 0, 0, 0, 0, 0, 0, SIM_STNOTEXCL},

      /*Line 1*/
      {"Right Trough", 1, swRTrough, sTrough, stShooter, 5},
      {"Center Trough", 1, swCTrough, 0, stRTrough, 1},
      {"Left Trough", 1, swLTrough, 0, stCTrough, 1},
      {"Outhole", 1, swOutHole, sOutHole, stLTrough, 5},
      {"Drain", 1, 0, 0, stOutHole, 0, 0, 0, SIM_STNOTEXCL},

      /*Line 2*/
      {"Shooter", 1, swShooterLane, sShooterRel, stBallLane, 0, 0, 0, SIM_STNOTEXCL | SIM_STSHOOT},
      {"Ball Lane", 1, swRTRollover, 0, stFree, 2, 0, 0, SIM_STNOTEXCL},
      {"No Strength", 1, 0, 0, stShooter, 3},
      {"Right Outlane", 1, swRTOutlane, 0, stDrain, 15},
      {"Left Outlane", 1, swLTOutlane, 0, stDrain, 15},
      {"Right Inlane", 1, swRTInlane, 0, stFree, 5},
      {"Left Inlane", 1, swLTInlane, 0, stFree, 5},
      {"Left Slingshot", 1, swLTSling, 0, stFree, 1},
      {"Rt Slingshot", 1, swRTSling, 0, stFree, 1},
      {"Left Bumper", 1, swLTPop, 0, stFree, 1},
      {"Right Bumper", 1, swRTPop, 0, stFree, 1},
      {"Bottom Bumper", 1, swCTPop, 0, stFree, 1},

      /*lock states*/
      //TODO determine the number for the lock solenoid when it fires
      {"Lock 1", 1, swLock1, sLock, stFree, 0},
      {"Lock 2", 1, swLock2, 0, stLock1, 0},
      {"Lock 3", 1, swLock3, 0, stLock2, 0},

      //TODO determine the number for the VUK solenoid
      {"Left Ramp", 1, swLTRamp, 0, stLock3, 2},
      {"Right Ramp", 1, swRTRamp, 0, stLock3, 2},
      {"Center Ramp", 1, swCTRamp, 0, stSubway, 2},
      {"Subway", 1, swSubway, 0, stVUK, 1},
      {"Super VUK", 1, swVUK, sVUK, stRightInlane, 1}

};

  static int tm_handleBallState(sim_tBallStatus *ball, int *inports)
  {
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData tm_inportData[] = {

/* Port 0 */
  {0, 0x0001, stLTRamp},
  {0, 0x0002, stRTRamp},
  {0, 0x0004, stCTRamp},
  {0, 0x0008, stLeftOutlane},
  {0, 0x0010, stRightOutlane},
  {0, 0x0020, stLeftInlane},
  {0, 0x0040, stRightInlane},
  {0, 0x0100, stRightJet},
  {0, 0x0200, stLeftJet},
  {0, 0x0400, stBottomJet},
  {0, 0x0800, stLeftSling},
  {0, 0x1000, stRightSling},
  
  // Switches may be directly activated, their numbers are < the state numbers
  {0, 0x2000, swLane1},
  {0, 0x4000, swLane2},
  {0, 0x8000, swLane3},


  {1, 0x0001, swLTBank1},
  {1, 0x0002, swLTBank2},
  {1, 0x0004, swLTBank3},

  {1, 0x0008, swCTBank1},
  {1, 0x0010, swCTBank2},
  {1, 0x0020, swCTBank3},

  {1, 0x0040, swRTBank1},
  {1, 0x0080, swRTBank2},
  {1, 0x0100, swRTBank3}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay tm_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{ 0,18,RED}}},{1,{{ 0,20,RED}}},{1,{{ 0,22,RED}}},{1,{{ 0,24,RED}}},
{1,{{ 0,26,RED}}},{1,{{13,26,WHITE}}},{1,{{11,27,WHITE}}},{1,{{ 9,28,WHITE}}},
{1,{{ 6,28,WHITE}}},{1,{{ 4,28,WHITE}}},{1,{{ 2,28,WHITE}}},{1,{{ 0,28,WHITE}}},
{1,{{15, 5,RED}}},{1,{{13, 4,YELLOW}}},{1,{{11, 3,WHITE}}},{1,{{ 9, 2,GREEN}}},
{1,{{ 2,22,RED}}},{1,{{ 4,24,RED}}},{1,{{ 6,22,RED}}},{1,{{ 4,20,RED}}},
{1,{{ 4,22,RED}}},{1,{{33,17,WHITE}}},{1,{{31,18,WHITE}}},{1,{{29,19,WHITE}}},
{1,{{13,16,WHITE}}},{1,{{11,15,WHITE}}},{1,{{ 9,14,WHITE}}},{1,{{ 7,13,WHITE}}},
{1,{{ 5,12,WHITE}}},{1,{{ 3,11,WHITE}}},{1,{{17,18,WHITE}}},{1,{{29,10,WHITE}}},
{1,{{ 4,18,YELLOW}}},{1,{{ 8,22,YELLOW}}},{1,{{ 4,26,YELLOW}}},{1,{{ 7,22,GREEN}}},
{1,{{ 8,18,GREEN}}},{1,{{ 8,26,GREEN}}},{1,{{33,11,YELLOW}}},{2,{{31, 1,WHITE},{31,28,WHITE}}},
{1,{{17,26,WHITE}}},{1,{{19,24,ORANGE}}},{1,{{21,23,RED}}},{1,{{23,24,YELLOW}}},
{1,{{25,26,WHITE}}},{1,{{17,11,RED}}},{1,{{15,11,RED}}},{1,{{13,11,RED}}},
{1,{{26, 7,WHITE}}},{1,{{23, 6,GREEN}}},{1,{{21, 7,GREEN}}},{1,{{19, 6,GREEN}}},
{1,{{20,20,WHITE}}},{1,{{18,21,YELLOW}}},{1,{{16,22,YELLOW}}},{1,{{14,23,YELLOW}}},
{1,{{12, 7,WHITE}}},{1,{{15, 8,ORANGE}}},{1,{{18, 9,RED}}},{1,{{30,14,RED}}},
{1,{{32,14,RED}}},{1,{{34,14,RED}}},{1,{{36,14,RED}}},{1,{{38,14,RED}}}
}
};


/* Help */

static void tm_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this bingus bongus:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = MixMaster Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,100,BLACK,"T/J/K = I Test/Heart/Magnet Targets");
  core_textOutf(30,110,BLACK,"Y/U/I/O = Right Drop Targets");
  core_textOutf(30,120,BLACK,"A/S/D/F/G/H = R/E/F/L/E/X Targets");
  core_textOutf(30,130,BLACK,"L = Big Shot Target, C = Right Loop");
  core_textOutf(30,140,BLACK,"Z/X = Bag of Tricks/Gift of Gab Hole");
}

/* Solenoid-to-sample handling */
#ifdef ENABLE_MECHANICAL_SAMPLES
static wpc_tSamSolMap tm_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sBigShot,1,SAM_SOLENOID},

 /*Channel #2*/
 {sRDrop,2,SAM_SOLENOID},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},

 /*Channel #3*/
 {sLPopper,3,SAM_POPPER},  {sRPopper,3,SAM_POPPER},{-1}
};
#endif

/*--------------
/  Game drivers
/---------------*/
static MACHINE_DRIVER_START(s11c_one)
    MDRV_IMPORT_FROM(s11_s11cS)
        MDRV_SCREEN_SIZE(640, 400)
            MDRV_VISIBLE_AREA(0, 639, 0, 399)
                MACHINE_DRIVER_END

// CORE_GAMEDEF (tmac,    a24, "Time Machine (R02-4)", 1988, "Data East", s11c_one,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData tmSimData = {
        2,                                         /* 2 game specific input ports */
        tm_stateDef,                               /* Definition of all states */
        tm_inportData,                             /* Keyboard Entries */
        {stRTrough, stCTrough, stLTrough, 0, 0, 0, 0}, /*Position where balls start.. Max 7 Balls Allowed*/
        NULL,                                      /* no init */
        tm_handleBallState,                        /*Function to handle ball state changes*/
        tm_drawStatic,                             /*Function to handle mechanical state changes*/
        TRUE,                                      /* Simulate manual shooter? */
        NULL                                       /* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
#define FLIP1516 FLIP_SWNO(15, 16)

static struct core_dispLayout de_dispAlpha2[] = { /* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows */
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG8),  DISP_SEG_7(1,1, CORE_SEG8), {0}
};

static core_tGameData tmacGameData = {
    GEN_DE, de_dispAlpha2, {FLIP1516, 0, 0, 0, SNDBRD_DE1S, 0, 0}, &tmSimData, {{0}}, {10}};

/*---------------
/  Game handling
/----------------*/
void init_tmac(void) {
  printf("initializing Time Machine");
  core_gameData = &tmacGameData;
}

static void tm_handleMech(int mech) {


}
