/*******************************************************************************
 Hurricane (Williams, 1991) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 21, 2000 (Updated Dec. 5, 2000)

 Known Issues/Problems with this Simulator:

 None.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Hurricane Simulator:
  --------------------------------------------------
    +R  Comet/Hurricane Ramps
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
     D  Dunk The Dummy Target
   ZXC  Left Drop Targets (1-3)
  VBNM  Right Targets (Cats) (1-4)
     F  Ferris Wheels
     J  Juggler
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "wpcsam.h"

/*------------------
/  Local functions
/-------------------*/
static int  hurr_handleBallState(sim_tBallStatus *ball, int *inports);
static void hurr_handleMech(int mech);
static void hurr_drawStatic(BMTYPE **line);
static void init_hurr(void);

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(hurr,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"C/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Jet1",			KEYCODE_W)
    COREPORT_BIT(0x0100,"Jet2",			KEYCODE_E)
    COREPORT_BIT(0x0200,"Jet3",			KEYCODE_R)
    COREPORT_BIT(0x0400,"LDropT",		KEYCODE_Z)
    COREPORT_BIT(0x0800,"LDropM",		KEYCODE_X)
    COREPORT_BIT(0x1000,"LDropB",		KEYCODE_C)
    COREPORT_BIT(0x2000,"Ferris",		KEYCODE_F)
    COREPORT_BIT(0x4000,"Juggler",		KEYCODE_J)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Cat1",			KEYCODE_V)
    COREPORT_BIT(0x0002,"Cat2",			KEYCODE_B)
    COREPORT_BIT(0x0004,"Cat3",			KEYCODE_N)
    COREPORT_BIT(0x0008,"Cat4",			KEYCODE_M)
    COREPORT_BIT(0x0010,"Dummy",			KEYCODE_D)


WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swOutHole	15
#define swRTrough	16
#define swCTrough	17
#define swLTrough	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swRightSling	25
#define swRightInlane	26
#define swRightOutlane	27
#define swShooter	28

#define swFerrisWheel	31
#define swLDropT	33
#define swLDropM	34
#define swLDropB	35
#define swLeftSling	36
#define swLeftInlane	37
#define swLeftOutlane	38

#define swRTarget1	42
#define swRTarget2	43
#define swRTarget3	44
#define swRTarget4     	45

#define swLeftJet    	51
#define swRightJet	52
#define swBottomJet	53
#define swDunkTheDummy	55
#define swLeftJuggler	56
#define swRightJuggler	57

#define swEnterHurr	61
#define swHurrMade	62
#define swEnterComet	63
#define	swCometMade	64

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sBackBoxMotor	1
#define sLeftBank	2
#define sLeftJuggler	4
#define sRightJuggler	5
#define sFerrisWheels  	6
#define sKnocker       	7
#define sOutHole       	9
#define sTrough		10
#define sLeftSling	11
#define sRightSling	12
#define sLeftJet	13
#define sRightJet	14
#define sBottomJet	15

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
      stEnterHurr, stHurrMade, stInHRamp, stEnterComet, stCometMade, stInCRamp,
      stFerris1, stFerris2, stLJuggler, stRJuggler, stLJet, stRJet, stBJet, stDownFCenter, stDownFRamp,
      stLDropT, stLDropM, stLDropB, stLeftJet, stRightJet, stBottomJet, stRTarget1, stRTarget2, stRTarget3, stRTarget4, stDunkTheDummy
	  };

static sim_tState hurr_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",	1,swRTrough,	 sTrough,	stShooter,	5},
  {"Center Trough",	1,swCTrough,	 0,		stRTrough,	1},
  {"Left Trough",	1,swLTrough,	 0,		stCTrough,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stLTrough,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	2,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Hurricane Ramp",	5,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},

  /*Line 3*/
  {"Enter Hurricne",	1,swEnterHurr,	 0,		stHurrMade,	10},
  {"Hurricane Made",	1,swHurrMade,	 0,		stInHRamp,	5},
  {"In Ramp",		1,0,		 0,		stLeftInlane,	20},
  {"Enter Comet",	1,swEnterComet,  0,		stCometMade,	7},
  {"Comet Made",	1,swCometMade,	 0,		stInCRamp,	3},
  {"Habitrail",		1,0,		 0,		stRightInlane,	6},

  /*Line 4*/
  {"Ferris Wheel 1",	1,swFerrisWheel, 0,		stFerris2,	40},
  {"Ferris Wheel 2",	1,0,		 0,		stInHRamp,	40},
  {"Left Juggler",	1,swLeftJuggler, sLeftJuggler,	stRJuggler,	0},
  {"Right Juggler",	1,swRightJuggler,sRightJuggler, stLJet,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stDownFCenter,	7},
  {"Down F. Center",	1,0,		 0,		stFree,		5},
  {"Down From Ramp",	1,swEnterHurr,	 0,		stFree,		5},

  /*Line 5*/
  {"Top Drop Tgt",	1,swLDropT,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Mid Drop Tgt",	1,swLDropM,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Bot Drop Tgt",	1,swLDropB,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		2},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		2},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		2},
  {"Cat Target 1",	1,swRTarget1,	 0,		stFree,		2},
  {"Cat Target 2",	1,swRTarget2,	 0,		stFree,		2},
  {"Cat Target 3",	1,swRTarget3,	 0,		stFree,		2},
  {"Cat Target 4",	1,swRTarget4,	 0,		stFree,		2},
  {"Dunk The Dummy",	1,swDunkTheDummy,0,		stFree,		2},

  {0}
};

static int hurr_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
		if (ball->speed < 35)
			return setState(stDownFRamp,35);	/*Ball rolled down Hurricane entrance*/
		if (ball->speed < 51)
			return setState(stHurrMade,60);		/*Ball fired up normally*/
		break;
       }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData hurr_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterComet},
  {0, 0x0006, stEnterHurr},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0200, stRightJet},
  {0, 0x0400, stLDropT},
  {0, 0x0800, stLDropM},
  {0, 0x1000, stLDropB},
  {0, 0x2000, stFerris1},
  {0, 0x4000, stLJuggler},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stRTarget1},
  {1, 0x0002, stRTarget2},
  {1, 0x0004, stRTarget3},
  {1, 0x0008, stRTarget4},
  {1, 0x0010, stDunkTheDummy},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay hurr_lampPos = {
{ 0, 0 }, /* top left */
{39, 28}, /* size */
{
{1,{{30,12,YELLOW}}},{1,{{30,16,YELLOW}}},{1,{{33,10,ORANGE}}},{1,{{32,14,RED}}},
{1,{{33,18,ORANGE}}},{1,{{35,14,RED}}},{1,{{27, 2,ORANGE}}},{1,{{27, 5,ORANGE}}},
{1,{{28,11,LBLUE}}},{1,{{26, 9,LBLUE}}},{1,{{24, 7,LBLUE}}},{1,{{22, 5,LBLUE}}},
{1,{{20, 3,LBLUE}}},{1,{{12, 2,GREEN}}},{1,{{14, 2,GREEN}}},{1,{{16, 2,GREEN}}},
{1,{{24,10,WHITE}}},{1,{{26,12,WHITE}}},{1,{{27,14,WHITE}}},{1,{{27,16,WHITE}}},
{1,{{26,18,WHITE}}},{1,{{24,20,WHITE}}},{1,{{27,26,ORANGE}}},{1,{{27,23,ORANGE}}},
{1,{{24,17,RED}}},{1,{{22,18,ORANGE}}},{1,{{20,19,WHITE}}},{1,{{18,20,YELLOW}}},
{1,{{16,21,WHITE}}},{1,{{10,22,YELLOW}}},{1,{{11,23,YELLOW}}},{1,{{12,24,YELLOW}}},
{1,{{12, 9,GREEN}}},{1,{{10, 8,GREEN}}},{1,{{ 8, 7,WHITE}}},{1,{{ 6, 6,YELLOW}}},
{1,{{38,14,ORANGE}}},{1,{{ 9, 3,WHITE}}},{1,{{ 7, 2,YELLOW}}},{1,{{14,14,WHITE}}},
{1,{{11,13,GREEN}}},{1,{{11,15,RED}}},{1,{{ 9,13,ORANGE}}},{1,{{ 9,15,WHITE}}},
{1,{{ 0, 0,BLACK}}},{1,{{ 0, 2,BLACK}}},{1,{{ 0, 4,BLACK}}},{1,{{14,18,LBLUE}}},
{1,{{21,14,YELLOW}}},{1,{{19,14,YELLOW}}},{1,{{17,14,YELLOW}}},{1,{{16,11,WHITE}}},
{1,{{22,25,GREEN}}},{1,{{20,25,GREEN}}},{1,{{18,25,GREEN}}},{1,{{16,25,GREEN}}},
{1,{{ 2, 7,WHITE}}},{1,{{ 0, 9,WHITE}}},{1,{{ 0,12,WHITE}}},{1,{{ 2,14,WHITE}}},
{2,{{ 34, 6,WHITE},{34,22,WHITE}}},{1,{{39, 0,YELLOW}}},{1,{{31, 7,YELLOW}}},{1,{{31,21,YELLOW}}}
}
};

/* Help */
  static void hurr_drawStatic(BMTYPE **line) {

  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+R = Comet/Hurricane Ramp");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,110,BLACK,"D = Dunk The Dummy Target");
  core_textOutf(30,120,BLACK,"Z/X/C = Left Drop Targets");
  core_textOutf(30,130,BLACK,"V/B/N/M = Right Targets (Cats)");
  core_textOutf(30,140,BLACK,"F = Ferris Wheels");
  core_textOutf(30,150,BLACK,"J = Juggler");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap hurr_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3},

 /*Channel #2*/
 {sLeftSling,2,SAM_LSLING}, {sRightSling,2,SAM_RSLING},
 {sLeftBank,2,SAM_SOLENOID},

 /*Channel #3*/
 {sLeftJuggler,3,SAM_POPPER}, {sRightJuggler,3,SAM_SOLENOID},{-1}
};


/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(hurr,l2,"hurcnl_2.rom",0x40000,CRC(fda6155f) SHA1(0088155a2582524d8720d71cd3ff82e8733ef434))
WPCS_SOUNDROM222("u18.pp",CRC(63944b37) SHA1(045f8046ba5bf1c88b65a80737e2d3d017271c04),
                 "u15.pp",CRC(93d02c62) SHA1(203cd6b933822d6d3f70c63e051237e3587568f1),
                 "u14.pp",CRC(51c82899) SHA1(aa6c3d9e7efa3708727b06fb3372638d5245a510))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(hurr,l2,"Hurricane (L-2)",1991, "Williams",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData hurrSimData = {
  2,    				/* 2 game specific input ports */
  hurr_stateDef,			/* Definition of all states */
  hurr_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  hurr_handleBallState,		/*Function to handle ball state changes*/
  hurr_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData hurrGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SWNO(12,11),
    0,0,0,0,0,0,0,
    NULL, hurr_handleMech, NULL, NULL,
    &hurr_lampPos, hurr_samsolmap
  },
  &hurrSimData,
  {
    {0}, /* sNO */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_hurr(void) {
  core_gameData = &hurrGameData;
}

static void hurr_handleMech(int mech) {
  /* ---------------------------------------
     --	Get Up the Left Drop Target Bank  --
     --------------------------------------- */
  /*-- if the UpTargets (sLeftBank) Solenoid is firing, Up the targets --*/
  if ((mech & 0x01) && core_getSol(sLeftBank))
    { core_setSw(swLDropT,0) ; core_setSw(swLDropM,0) ; core_setSw(swLDropB,0); }
}
