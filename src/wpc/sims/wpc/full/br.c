/*******************************************************************************
 Black Rose (Bally, 1992) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 27, 2000.

 Known Issues/Problems with this Simulator:

 None.

 Actually I've made the Cannon Firing a little easier than in the real pin :-)

 Changes:

 071200 - Now the Cannon's Position is shown on screen.
 081200 - Added key for the Right Single Standup Target.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Black Rose Simulator:
  --------------------------------------------------
     +R  L/R Ramp
     +I  L/R Inlane
     +-  L/R Slingshot
     +O  L/R Outlane
      Q  SDTM (Drain Ball)
      W  Jet Bumpers Loop
      E  Ball Lock
      R  Ship Ramp (Left)
      T  Top Ramp Loop
    YUI  Jet Bumpers (1-3)
    ASD  Bottom Bank Targets (1-3)
    FGH  Middle Bank Targets (1-3)
    JKL  Top Bank Targets (1-3)
      Z  Right Single Standup
      B  Broadside Hole
  ENTER  Fire Button
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
static int  br_handleBallState(sim_tBallStatus *ball, int *inports);
static void br_handleMech(int mech);
static void br_drawMech(BMTYPE **line);
static int br_getMech(int mechNo);
static void br_drawStatic(BMTYPE **line);
static void init_br(void);
static const char* showcannonpos(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int shiprampPos;	/* Ship Ramp Position */
  int cannonPos;	/* Cannon Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(br,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Jet Bumpers Loop",	KEYCODE_W)
    COREPORT_BIT(0x0100,"Lock",			KEYCODE_E)
    COREPORT_BIT(0x0200,"Ship Ramp",		KEYCODE_R)
    COREPORT_BIT(0x0400,"Top Ramp Loop",		KEYCODE_T)
    COREPORT_BIT(0x0800,"Left Jet",		KEYCODE_Y)
    COREPORT_BIT(0x1000,"Right Jet",		KEYCODE_U)
    COREPORT_BIT(0x2000,"Bottom Jet",		KEYCODE_I)
    COREPORT_BIT(0x4000,"Broadside",		KEYCODE_B)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"BotBot",		KEYCODE_A)
    COREPORT_BIT(0x0002,"BotMid",		KEYCODE_S)
    COREPORT_BIT(0x0004,"BotTop",		KEYCODE_D)
    COREPORT_BIT(0x0008,"MidBot",		KEYCODE_F)
    COREPORT_BIT(0x0010,"MidMid",		KEYCODE_G)
    COREPORT_BIT(0x0020,"MidTop",		KEYCODE_H)
    COREPORT_BIT(0x0040,"TopBot",		KEYCODE_J)
    COREPORT_BIT(0x0100,"TopMid",		KEYCODE_K)
    COREPORT_BIT(0x0200,"TopTop",		KEYCODE_L)
    COREPORT_BIT(0x0400,"FIRE",			KEYCODE_ENTER)
    COREPORT_BIT(0x0800,"RS Tgt",		KEYCODE_Z)

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
#define swShooter	25
#define swLeftOutlane	26
#define swLeftInlane	27
#define swLeftSling	28

#define swBotBot	31
#define swBotMid	32
#define swBotTop	33
#define swFireButton	34
#define swCannonKicker	35
#define swRightOutlane	36
#define swRightInlane	37
#define swRightSling	38

#define swMidTop	41
#define swMidMid	42
#define swMidBot	43
#define swEnterLRamp   	44
#define swTopLeftLoop  	45
#define swLeftJet	46
#define swRightJet	47
#define swBottomJet	48

#define swTopBot	51
#define swTopMid	52
#define swTopTop	53
#define swRampDown	54
#define	swBallPopper	55
#define	swRRampMade	56
#define	swJetsExit	57
#define	swEnterJets	58

#define	swSubwayTop	61
#define	swBackboardRamp	62
#define	swLockup1	63
#define	swLockup2	64
#define	swRSStandup	65
#define	swSubwayBottom	66

#define	swEnterLockup	71
#define	swMiddleRamp	72
#define	swEnterRRamp	76
#define swCannonDir	77 /* FAKE!, Not used in the Pin */

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sBallPopper	1
#define sOutHole	2
#define sCannonMotor	3
#define sTrough    	4
#define sRightSling    	5
#define sLeftSling     	6
#define sKnocker       	7
#define sCannonKicker	8
#define sBallLockup	9
#define sRampUp		10
#define sRampDown	11
#define sLeftJet	13
#define sRightJet	14
#define sBottomJet	15

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stBottomJet, stRightJet,
      stEnterLRamp, stLRampMade, stInRamp, stShipRamp, stEnterShipRamp, stSRampMade, stLoadCannon, stLoadCannon2, stCannonLoaded, stCannonFired,
      stEnterRRamp, stRRampMade, stInRamp2, stTopRampLoop, stTopLoopMade, stDownFLock,
      stBotBot, stBotMid, stBotTop, stMidBot, stMidMid, stMidTop, stTopBot, stTopMid, stTopTop,
      stJetsLoop, stLJet, stRJet, stBJet, stDownRight, stRSStandup,
      stEnterLock, stLockup2, stLockup1, stBroadside, stInReel
	  };

static sim_tState br_stateDef[] = {
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
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	10,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/
  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		stLRampMade,	7},
  {"Left Ramp Made",	1,swBackboardRamp,0,		stInRamp,	5},
  {"In Ramp",		1,0,		 0,		stFree,		5},
  {"Enter Ship Rmp",	0,0,		 0,		0,		0},
  {"Enter Ship Rmp",	1,swMiddleRamp,	 0,		stSRampMade,	5},
  {"Ship Ramp Made",	1,swBackboardRamp,0,		stInRamp,	5},
  {"Load Cannon",	1,swSubwayTop,	 0,		stLoadCannon2,	7},
  {"Load Cannon",	1,swSubwayBottom,0,		stCannonLoaded,	7},
  {"Cannon Loaded",	1,swCannonKicker,sCannonKicker, stCannonFired,	0},
  {"Cannon Fired",	1,0,		 0,		0,		0},

  /*Line 4*/
  {"Enter Rt. Ramp",	1,swEnterRRamp,	 0,		stRRampMade,	6},
  {"Rt. Ramp Made",	1,swRRampMade,	 0,		stInRamp2,	5},
  {"Habitrail",		1,0,		 0,		stLeftInlane,	12},
  {"Enter Top Rmp",	1,0,		 0,		stTopLoopMade,	5},
  {"Top Ramp Loop",	1,swTopLeftLoop, 0,		stDownFLock,	10},
  {"Lock Exit",		1,swEnterLockup, 0,		stFree,		7},

  /*Line 5*/
  {"Bot.Bot.Target",	1,swBotBot,	 0,		stFree,		2},
  {"Bot.Mid.Target",	1,swBotMid,	 0,		stFree,		2},
  {"Bot.Top Target",	1,swBotTop,	 0,		stFree,		2},
  {"Mid.Bot.Target",	1,swMidBot,	 0,		stFree,		2},
  {"Mid.Mid.Target",	1,swMidMid,	 0,		stFree,		2},
  {"Mid.Top Target",	1,swMidTop,	 0,		stFree,		2},
  {"Top.Bot.Target",	1,swTopBot,	 0,		stFree,		2},
  {"Top.Mid.Target",	1,swTopMid,	 0,		stFree,		2},
  {"Top.Top Target",	1,swTopTop,	 0,		stFree,		2},

  /*Line 6*/
  {"Jet Bmps. Loop",	1,swEnterJets,	 0,		stLJet,		6},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		5},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		5},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stDownRight,	9},
  {"Down F. Right",	1,swJetsExit,	 0,		stFree,		5},
  {"Rt. Single Tgt",	1,swRSStandup,	 0,		stFree,		2},

  /*Line 7*/
  {"Enter Lock",	1,swEnterLockup, 0,		stLockup2,	7},
  {"Lock 2",		1,swLockup2,	 0,		stLockup1,	1},
  {"Lock 1",		1,swLockup1,	 sBallLockup,	stDownFLock,	5},
  {"Broadside Hole",	1,swBallPopper,	 sBallPopper,	stInReel,	0},
  {"Habitrail",		1,0,		 0,		stFree,		8},

  /*Line 6*/

  {0}
};

static int br_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 7)
			return setState(stNotEnough,7);		/*Ball not plunged hard enough*/
		if (ball->speed < 20)
			return setState(stFree,20);		/*Ball doesn't touch anything*/
		if (ball->speed < 25)
			return setState(stBotBot,25);		/*Ball touched Bot.Bot. Target*/
		if (ball->speed < 35)
			return setState(stBotMid,35);		/*Ball touched Bot.Mid. Target*/
		if (ball->speed < 45)
			return setState(stBotTop,45);		/*Ball touched Bot.Top. Target*/
		if (ball->speed < 51)
			return setState(stShipRamp,51);		/*Ball gets the Ship Ramp*/
		break;

		/*-----------------
		    Ship's Ramp
		-------------------*/
		case stShipRamp:
		/* If the ramp is Down... Go up the Creature Ramp!*/
		/* Otherwise go normally through the Left Ramp. */
		if (core_getSw(swRampDown))
			return setState(stEnterShipRamp,5);
		else
			return setState(stLoadCannon,5);
		break;

		/*-----------------------------------
		   Cannon - Where will the ball land?
		-------------------------------------*/
		case stCannonFired:
		if (locals.cannonPos >= 0 && locals.cannonPos <= 50)
			return setState(stEnterLock,30);
		if (locals.cannonPos >= 51 && locals.cannonPos <= 100)
			return setState(stEnterLRamp,30);
		if (locals.cannonPos >= 101 && locals.cannonPos <= 150)
			return setState(stBroadside,30);
		if (locals.cannonPos >= 151 && locals.cannonPos <= 200)
			return setState(stEnterRRamp,30);
		if (locals.cannonPos >= 201 && locals.cannonPos <= 250)
			return setState(stJetsLoop,30);
		if (locals.cannonPos >= 251 && locals.cannonPos <= 300)
			return setState(stFree,30);

	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData br_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterLRamp},
  {0, 0x0006, stEnterRRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stJetsLoop},
  {0, 0x0100, stEnterLock},
  {0, 0x0200, stShipRamp},
  {0, 0x0400, stTopRampLoop},
  {0, 0x0800, stLeftJet},
  {0, 0x1000, stRightJet},
  {0, 0x2000, stBottomJet},
  {0, 0x4000, stBroadside},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stBotBot},
  {1, 0x0002, stBotMid},
  {1, 0x0004, stBotTop},
  {1, 0x0008, stMidBot},
  {1, 0x0010, stMidMid},
  {1, 0x0020, stMidTop},
  {1, 0x0040, stTopBot},
  {1, 0x0100, stTopMid},
  {1, 0x0200, stTopTop},
  {1, 0x0400, swFireButton, SIM_STANY},
  {1, 0x0800, stRSStandup},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void br_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Ship Ramp  : %-4s", core_getSw(swRampDown) ? "Down":"Up");
  core_textOutf(30, 20,BLACK,"Cannon Pointing to:");
  core_textOutf(30, 30,BLACK,"%-11s (%3d)", showcannonpos(), locals.cannonPos);
}
/* Help */
static void br_drawStatic(BMTYPE **line) {

  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W = Jet Bumpers Loop");
  core_textOutf(30,100,BLACK,"E = Ball Lock, R = Ship Ramp (Left)");
  core_textOutf(30,110,BLACK,"T = Top Ramp Loop, B = Broadside Hole");
  core_textOutf(30,120,BLACK,"Y/U/I = Jet Bumpers, Z = R. Single Tgt");
  core_textOutf(30,130,BLACK,"A/S/D = Bottom Bank Targets");
  core_textOutf(30,140,BLACK,"F/G/H = Middle Bank Targets");
  core_textOutf(30,150,BLACK,"J/K/L = Top Bank Targets");
  core_textOutf(30,160,BLACK,"ENTER = Fire Button");

  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap br_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sCannonMotor,1,SAM_MOTOR_1,WPCSAM_F_CONT},

 /*Channel #2*/
 {sCannonKicker,2,SAM_POPPER},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},  {sBallPopper,2,SAM_POPPER},

 /*Channel #3*/
 {sBallLockup,3,SAM_SOLENOID_ON}, {sBallLockup,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sRampUp,3,SAM_FLAPOPEN}, {sRampDown,3,SAM_FLAPCLOSE},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(br,l4,"brose_l4.rom",0x80000,CRC(e140dc46) SHA1(a64dd212dadac17c3424cd0a8ae617f55736c22d))
WPCS_SOUNDROM224("br_u18.l1",CRC(7d446e7d) SHA1(2b03235aecbea0f6f5efad5aff9da194b0981130),
                 "br_u15.l1",CRC(10250e96) SHA1(769c9a8c7e2bf4af312345d885afa000c4aedf3d),
                 "br_u14.l1",CRC(490bb87f) SHA1(2e4c0bf776b82e2b5fb60c651edd34ab65d6c5aa))
WPC_ROMEND

WPC_ROMSTART(br,p17,"u6-p17.rom",0x40000,CRC(c5629f68) SHA1(580b38d57c4297525c5a96719fb886b2b6cfa772))
WPCS_SOUNDROM224("u18-sp1.rom",CRC(01fb319d) SHA1(881180fbf524823cc3a89efe6dd6a444b40552ee),
                 "br_u15.l1",CRC(10250e96) SHA1(769c9a8c7e2bf4af312345d885afa000c4aedf3d),
                 "br_u14.l1",CRC(490bb87f) SHA1(2e4c0bf776b82e2b5fb60c651edd34ab65d6c5aa))
WPC_ROMEND

WPC_ROMSTART(br,l1,"u6-l1.rom",0x80000,CRC(87ad7f78) SHA1(5978fb753574bfbfcf5164bc098c5764df8c7403))
WPCS_SOUNDROM224("br_u18.l1",CRC(7d446e7d) SHA1(2b03235aecbea0f6f5efad5aff9da194b0981130),
                 "br_u15.l1",CRC(10250e96) SHA1(769c9a8c7e2bf4af312345d885afa000c4aedf3d),
                 "br_u14.l1",CRC(490bb87f) SHA1(2e4c0bf776b82e2b5fb60c651edd34ab65d6c5aa))
WPC_ROMEND

WPC_ROMSTART(br,l3,"u6-l3.rom",0x80000,CRC(d9a47ca7) SHA1(1ef407d06a9bda1f1273289273283b9bd31001c8))
WPCS_SOUNDROM224("br_u18.l1",CRC(7d446e7d) SHA1(2b03235aecbea0f6f5efad5aff9da194b0981130),
                 "br_u15.l1",CRC(10250e96) SHA1(769c9a8c7e2bf4af312345d885afa000c4aedf3d),
                 "br_u14.l1",CRC(490bb87f) SHA1(2e4c0bf776b82e2b5fb60c651edd34ab65d6c5aa))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (br,l4,"Black Rose (L-4)",1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF (br,p17,l4,"Black Rose (SP-1)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF (br,l1,l4,"Black Rose (L-1)",1992,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF (br,l3,l4,"Black Rose (L-3)",1993,"Bally",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData brSimData = {
  2,    				/* 2 game specific input ports */
  br_stateDef,				/* Definition of all states */
  br_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  br_handleBallState,			/*Function to handle ball state changes*/
  br_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData brGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,0,0,0,0,0,0,
    NULL, br_handleMech, br_getMech, br_drawMech,
    NULL, br_samsolmap
  },
  &brSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
  { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_br(void) {
  core_gameData = &brGameData;
}

static void br_handleMech(int mech) {

  /* --------------------------------------
     --	Get Up and Down the Ship's Ramp  --
     -------------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is Up and the RampDown Solenoid is firing, Lower it --*/
    if (core_getSol(sRampDown))
      locals.shiprampPos = 1 ;
    /*-- if Ramp is Down and the RampUp Solenoid is firing, raise it --*/
    if (core_getSol(sRampUp))
      locals.shiprampPos = 0 ;
    /*-- Make sure RampUpDown Switch is on, when ramp is Up --*/
    core_setSw(swRampDown,locals.shiprampPos);
  }
  /* --------------------------------------------------------------------
     --	Handle the Cannon to change its position and go back and forth --
     -------------------------------------------------------------------- */
  if (mech & 0x02) {
    if (locals.cannonPos == 0)
      core_setSw(swCannonDir,0);
    if (locals.cannonPos == 300)
      core_setSw(swCannonDir,1);
    if (core_getSol(sCannonMotor) && !core_getSw(swCannonDir))
      locals.cannonPos++;
    if (core_getSol(sCannonMotor) && core_getSw(swCannonDir))
      locals.cannonPos--;
  }
}
static int br_getMech(int mechNo) {
  return mechNo?locals.cannonPos:locals.shiprampPos;
}
/*---------------------------------
    Display the Cannon's Position
  ---------------------------------*/
static const char* showcannonpos(void)
{
  if (locals.cannonPos >= 0 && locals.cannonPos <= 50)
	return "Lock Lane";
  if (locals.cannonPos >= 51 && locals.cannonPos <= 100)
	return "Left Ramp";
  if (locals.cannonPos >= 101 && locals.cannonPos <= 150)
	return "Broadside";
  if (locals.cannonPos >= 151 && locals.cannonPos <= 200)
	return "Right Ramp";
  if (locals.cannonPos >= 201 && locals.cannonPos <= 250)
	return "Jet B. Loop";
  if (locals.cannonPos >= 251 && locals.cannonPos <= 300)
	return "Nothing";
  return "Nothing";
}
