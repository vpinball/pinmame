/*******************************************************************************
 Creature From The Black Lagoon (Bally, 1992) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 15, 2000 (Updated Nov. 20, 2000)

 Known Issues/Problems with this Simulator:

 None.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Creature From The Black Lagoon Simulator:
  --------------------------------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
      Q SDTM (Drain Ball)
   WERT P-A-I-D Rollovers (1-4)
   YUIO Snackbar Targets (1-4)
    ASD Jet Bumpers
      H Center Subway Hole (Snackbar)
      L Left Subway (K-I-S-S)
      Z Right Subway (f-i-l-M)
      M Move Your Car Lane
      C Creature Bowl

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
static int  cftbl_handleBallState(sim_tBallStatus *ball, int *inports);
static void cftbl_handleMech(int mech);
static void cftbl_drawMech(BMTYPE **line);
static void cftbl_drawStatic(BMTYPE **line);
static void init_cftbl(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int creaturerampPos;    /* Creature Ramp Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(cftbl,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Paid - P",		KEYCODE_W)
    COREPORT_BIT(0x0100,"Paid - A",		KEYCODE_E)
    COREPORT_BIT(0x0200,"Paid - I",		KEYCODE_R)
    COREPORT_BIT(0x0400,"Paid - D",		KEYCODE_T)
    COREPORT_BIT(0x0800,"Snackbar - Cola",	KEYCODE_Y)
    COREPORT_BIT(0x1000,"Snackbar - HotDog",	KEYCODE_U)
    COREPORT_BIT(0x2000,"Snackbar - PopCorn",	KEYCODE_I)
    COREPORT_BIT(0x4000,"Snackbar - IceCream",	KEYCODE_O)
    COREPORT_BIT(0x8000,"Snackbar Hole",		KEYCODE_H)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Left Jet Bumper",	KEYCODE_A)
    COREPORT_BIT(0x0002,"Right Jet Bumper",	KEYCODE_S)
    COREPORT_BIT(0x0004,"Bottom Jet Bumper",	KEYCODE_D)
    COREPORT_BIT(0x0008,"Left Subway",		KEYCODE_L)
    COREPORT_BIT(0x0010,"Right Subway",		KEYCODE_Z)
    COREPORT_BIT(0x0020,"Creature Bowl",		KEYCODE_C)
    COREPORT_BIT(0x0040,"Move Your Car Lane",	KEYCODE_M)
    COREPORT_BIT(0x0100,"Drain",			KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swTopLeftRO	15
#define swLeftSubway	16
#define swCenterSubway	17
#define swCenterShotRU	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swPaid		25
#define swpAid		26
#define swpaId		27
#define swpaiD		28

#define swBottomJet	33
#define swRightPopper	34
#define swEnterRRamp	35
#define swEnterLRamp	36
#define swLowRPopper	37
#define swRampUpDown	38

#define swCola		41
#define swHotDog	42
#define swPopCorn	43
#define swIceCream     	44
#define swLeftJet    	45
#define swRightJet	46
#define swLeftSling	47
#define swRightSling	48

#define swLeftOutlane	51
#define swLeftInlane	52
#define swRightInlane	53
#define swRightOutlane	54
#define	swOutHole	55
#define	swRTrough	56
#define	swCTrough	57
#define	swLTrough	58

#define	swRRampExit	61
#define	swLowLRampExit	62
#define	swCLaneExit	63
#define	swURampExit	64
#define	swBowl		65
#define	swShooter	66

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sRightPopper	1
#define sLowRPopper	3
#define sTrough		4
#define sRightSling    	5
#define sLeftSling    	6
#define sKnocker       	7
#define sOutHole       	12
#define sLeftJet	13
#define sRightJet	14
#define sBottomJet	15
#define sCurlyRampGI1	20
#define sHologramPush	21
#define sRampUp		23
#define sCurlyRampGI2	24
#define sRampDown	26
#define sMirrorMotor	27
#define sHologramLamp	28

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
      stLRamp, stEnterLeftRamp, stLeftRamp, stInLRamp, stCreatureRamp, stInCRamp1, stInCRamp2, stBowl1, stBowl2, stBowl3, stBowl4, stBowl,
      stpaiD, stpaId, stpAid, stPaid, stLJet, stRJet, stBJet, stDownRight, stPaid2, stpAid2, stpaId2, stpaiD2, stLeftJet2, stBottomJet2, stRightJet2,
      stCenterSubway, stLeftSubway, stLowRPopper, stOutFromHole, stRightSubway, stOutFromRPop, stDownFromLeft, stMoveYourCar, stMoveCar2,
      stRightRampEnter, stRightRamp, stCola, stHotDog, stPopCorn, stIceCream
	  };

static sim_tState cftbl_stateDef[] = {
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

  /*Line 3*/
  {"Enter Left Rmp",	1,0,		 0,		0,		0},
  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		stLeftRamp,	3},
  {"L. Ramp Sw #2",	1,swLowLRampExit,0,		stInLRamp,	5},
  {"In Ramp",		1,0,		 0,		stRightInlane,  8},
  {"Up Creat. Ramp",	1,swEnterLRamp,  0,		stInCRamp1,	8},
  {"Cr. Ramp Sw #1",	1,swURampExit,	 0,		stInCRamp2,	5},
  {"In Ramp",		1,0,		 0,		stBowl1,	30},
  {"In Bowl Loop 1",	1,swBowl,	 0,		stBowl2,	5},
  {"In Bowl Loop 2",	1,swBowl,	 0,		stBowl3,	5},
  {"In Bowl Loop 3",	1,swBowl,	 0,		stBowl4,	5},
  {"In Bowl Loop 4",	1,swBowl,	 0,		stFree,		5},
  {"Creature Bowl",	1,swBowl,	 0,		stFree,		2},

  /*Line 4*/
  {"Paid - D",		1,swpaiD,	 0,		stLJet,		5},
  {"Paid - I",		1,swpaId,	 0,		stLJet,		5},
  {"Paid - A",		1,swpAid,	 0,		stLJet,		5},
  {"Paid - P",		1,swPaid,	 0,		stLJet,		5},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		5},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		5},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stDownRight,	9},
  {"Down F. Right",	1,0,		 0,		stFree,		5},
  {"Paid - D",		1,swpaiD,	 0,		stFree,		2},
  {"Paid - I",		1,swpaId,	 0,		stFree,		2},
  {"Paid - A",		1,swpAid,	 0,		stFree,		2},
  {"Paid - P",		1,swPaid,	 0,		stFree,		2},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 5*/
  {"Center Subway",	1,swCenterSubway,0,		stLowRPopper,	10},
  {"Left Subway",	1,swLeftSubway,	 0,		stLowRPopper,	12},
  {"Lower Rt. Hole",	1,swLowRPopper,	 sLowRPopper,	stOutFromHole,	0},
  {"Out From Hole",	1,0,		 0,		stRightInlane,	5},
  {"Right Subway",	1,swRightPopper, sRightPopper,	stOutFromRPop,	0},
  {"Out From Hole",	1,0,		 0,		stDownFromLeft,	7},
  {"Down F. Left",	1,swTopLeftRO,	 0,		stFree,		12},
  {"Enter M.Y. Car",	1,swCenterShotRU,0,		stMoveCar2,	5},
  {"M.Y. Car Sw #2",	1,swCLaneExit,	 0,		stpaiD,		7},

  /*Line 6*/
  {"Enter Rt. Ramp",	1,swEnterRRamp,	 0,		stRightRamp,	5},
  {"R. Ramp Sw. #2",	1,swRRampExit,	 0,		stLeftInlane,	5},
  {"Cola Target",	1,swCola,	 0,		stFree,		1},
  {"Hotdog Target",	1,swHotDog,	 0,		stFree,		1},
  {"Popcorn Target",	1,swPopCorn,	 0,		stFree,		1},
  {"Ice Cream Tgt",	1,swIceCream,	 0,		stFree,		1},

  {0}
};

static int cftbl_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 15)
			return setState(stNotEnough,15);	/*Ball not plunged hard enough*/
		if (ball->speed < 22)
			return setState(stpaiD,22);		/*Ball rolled down 'D' Letter*/
		if (ball->speed < 29)
			return setState(stpaId,29);		/*Ball rolled down 'I' Letter*/
		if (ball->speed < 36)
			return setState(stpAid,36);		/*Ball rolled down 'A' Letter*/
		if (ball->speed < 43)
			return setState(stPaid,43);		/*Ball rolled down 'P' Letter*/
		if (ball->speed < 51)
			return setState(stDownFromLeft,51);	/*Ball rolled down from left*/
		break;

		/*-----------------
		   Creature Ramp
		-------------------*/
/*
This time I'm gonna give a little help on this part...
If you look at the state array you'll find that this state has all 0s, you
must create this state for using it as a hook. Jump your Keyboard Definitions
here in case you want to decide something, in this case the RampUp/Down state
is decided. If you jump to a normal state (ie without 0s), this part will not
be used so be careful :)
*/
		case stLRamp:
		/* If the ramp is Down... Go up the Creature Ramp!*/
		/* Otherwise go normally through the Left Ramp. */
		if (core_getSw(swRampUpDown))
			return setState(stCreatureRamp,5);
		else
			return setState(stEnterLeftRamp,5);
		break;
	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData cftbl_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLRamp},
  {0, 0x0006, stRightRampEnter},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stpaiD2},
  {0, 0x0100, stpaId2},
  {0, 0x0200, stpAid2},
  {0, 0x0400, stPaid2},
  {0, 0x0800, stCola},
  {0, 0x1000, stHotDog},
  {0, 0x2000, stPopCorn},
  {0, 0x4000, stIceCream},
  {0, 0x8000, stCenterSubway},

/* Port 1 */
  {1, 0x0001, stLeftJet2},
  {1, 0x0002, stBottomJet2},
  {1, 0x0004, stRightJet2},
  {1, 0x0008, stLeftSubway},
  {1, 0x0010, stRightSubway},
  {1, 0x0020, stBowl},
  {1, 0x0040, stMoveYourCar},
  {1, 0x0100, stDrain},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void cftbl_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Creature Ramp: %-6s", core_getSw(swRampUpDown) ? "Down":"Up");
}
/* Help */
  static void cftbl_drawStatic(BMTYPE **line) {

  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, A/S/D = Jet Bumpers");
  core_textOutf(30,100,BLACK,"W/E/R/T = P/A/I/D Rollovers");
  core_textOutf(30,110,BLACK,"Y/U/I/O = Snackbar Targets");
  core_textOutf(30,120,BLACK,"L = Left Subway Hole (K-I-S-S)");
  core_textOutf(30,130,BLACK,"H = Center Subway Hole (Snackbar)");
  core_textOutf(30,140,BLACK,"Z = Right Subway (f-i-l-M)");
  core_textOutf(30,150,BLACK,"C = Creature Bowl Switch");
  core_textOutf(30,160,BLACK,"M = Move Your Car Lane");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap cftbl_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sMirrorMotor,1,SAM_MOTOR_1,WPCSAM_F_CONT},

 /*Channel #2*/
 {sLeftSling,2,SAM_LSLING}, {sRightSling,3,SAM_RSLING},

 /*Channel #3*/
 {sRampUp,3,SAM_FLAPOPEN}, {sRampDown,3,SAM_FLAPCLOSE},
 {sLowRPopper,3,SAM_POPPER}, {sRightPopper,2,SAM_POPPER},
 {-1}
};

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(cftbl,l4,"creat_l4.rom",0x80000,CRC(b8778cb6) SHA1(a5dcc1ebedbd62d81e2e56fb8aebdc33fa6ba70c))
WPCS_SOUNDROM288("bl_u18.l1",CRC(87267bcc) SHA1(3e733437bce3491c216a8627810897f6123f0679),
                 "bl_u15.l1",CRC(15477d6f) SHA1(3ed7942828630bc9111d2e602fee931ef67db2ce),
                 "bl_u14.l1",CRC(6b02bee4) SHA1(4f852b897dbf0ec2d5b17eed2ff70d9360b12213))
WPC_ROMEND

WPC_ROMSTART(cftbl,l3,"cftbl_l3.u6",0x80000,CRC(11280230) SHA1(98ce0777cd7dc91d1d2b7016b2e44bdf60ec2c08))
WPCS_SOUNDROM288("u18-sp1.rom",CRC(07198d93) SHA1(d91eb7ae7bd11340b0daf4edd2cd2e87acadeda4),
                 "bl_u15.l1",CRC(15477d6f) SHA1(3ed7942828630bc9111d2e602fee931ef67db2ce),
                 "bl_u14.l1",CRC(6b02bee4) SHA1(4f852b897dbf0ec2d5b17eed2ff70d9360b12213))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (cftbl,l4,"Creature from the Black Lagoon (L-4)",1993,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF (cftbl,l3,l4,"Creature from the Black Lagoon (L-3,SP-1)",1993,"Bally",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData cftblSimData = {
  2,    				/* 2 game specific input ports */
  cftbl_stateDef,			/* Definition of all states */
  cftbl_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  cftbl_handleBallState,		/*Function to handle ball state changes*/
  cftbl_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData cftblGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    NULL, cftbl_handleMech, NULL, cftbl_drawMech,
    NULL, cftbl_samsolmap
  },
  &cftblSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x10, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_cftbl(void) {
  core_gameData = &cftblGameData;
}

static void cftbl_handleMech(int mech) {

  /* ----------------------------------------
     --	Get Up and Down the Creature Ramp  --
     ---------------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is Up and the RampDown Solenoid is firing, Lower it --*/
    if (core_getSol(sRampDown))
      locals.creaturerampPos = 1 ;
    /*-- if Ramp is Down and the RampUp Solenoid is firing, raise it --*/
    if (core_getSol(sRampUp))
      locals.creaturerampPos = 0 ;
    /*-- Make sure RampUpDown Switch is on, when ramp is Up --*/
    core_setSw(swRampUpDown,locals.creaturerampPos);
  }
}
