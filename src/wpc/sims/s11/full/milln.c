/*******************************************************************************
 Millionaire (Williams, 1987) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Feb. 21, 2001.

 Known Issues/Problems with this Simulator:

 The roulette doesn't work.
 The left gate doesn't want to get open during gameplay, although it works on
 test mode. Strange.

 Changes:

 210201 - First version.
 220201 - Modified on-screen info, fixed non-working "C"(ash Held) key, added
          sample-to-solenoid handling.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Millionaire Simulator:
  -----------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
  +-/-  L/R Slingshot/10 Points Rubber
    +S  L/R Spin Eject Holes
    +L  L/R Ball Lock
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
 TYUIO  MONEY Targets
     S  Silver/Gold Target
  DFGH  BANK Targets
     C  Cash Held Target
     M  Multiplier Lane

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "s11.h"
#include "sim.h"
#include "wmssnd.h"
/*------------------
/  Local functions
/-------------------*/
static void init_milln(void);
static void milln_handleMech(int mech);
static void milln_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls);
static void milln_drawMech(BMTYPE **line);
static void milln_drawStatic(BMTYPE **line);
static int  milln_handleBallState(sim_tBallStatus *ball, int *inports);

/*--------------------------
/ Game specific input ports
/---------------------------*/
S11_INPUT_PORTS_START(milln,2)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"10 Points",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"L/R Spin",		KEYCODE_S)
    COREPORT_BIT(0x0400,"L/R Lock",		KEYCODE_L)
    COREPORT_BIT(0x0800,"Bank",			KEYCODE_D)
    COREPORT_BIT(0x1000,"bAnk",			KEYCODE_F)
    COREPORT_BIT(0x2000,"baNk",			KEYCODE_G)
    COREPORT_BIT(0x4000,"banK",			KEYCODE_H)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START
    COREPORT_BIT(0x0001,"Money",			KEYCODE_T)
    COREPORT_BIT(0x0002,"mOney",			KEYCODE_Y)
    COREPORT_BIT(0x0004,"moNey",			KEYCODE_U)
    COREPORT_BIT(0x0008,"monEy",			KEYCODE_I)
    COREPORT_BIT(0x0010,"moneY",			KEYCODE_O)
    COREPORT_BIT(0x0020,"SilverGold",		KEYCODE_S)
    COREPORT_BIT(0x0040,"Multiplier",		KEYCODE_M)
    COREPORT_BIT(0x0080,"Cash Held",		KEYCODE_C)

S11_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt		 1
#define swStart		 3
#define swSlamTilt	 7
#define swPFTilt	 9
#define swLiteCashHeld	10
#define swLeftOutlane	11
#define swRightOutlane	12
#define swLeftInlane	15
#define swRightInlane	16
#define swBank		17
#define swbAnk		18
#define swbaNk		19
#define swbanK		20
#define swSilverGold	22
#define swShooter	24
#define swMoney		25
#define swmOney		26
#define swmoNey  	27
#define swmonEy		28
#define swmoneY  	29
#define swTopMultTgt	30
#define swLeftEject	31
#define swRightEject	32
#define swRTrough	33
#define swLTrough	34
#define swCKickBig2B	35
#define swOuthole	36
#define swLLaneChange	37
#define swRLaneChange	38
#define swTopDrop	39
#define swBotDrop	40
#define swRollUnder	41
#define swTopKickbig	42
#define swCKickbig	43
#define swEnterCKickbig 44
#define sw40K		45
#define swLeftMB	46
#define swLeftEB	47
#define sw50K		48
#define swRightLock	49
#define swLeftJet	50
#define swRightJet	51
#define swBottomJet	52
#define sw100K		53
#define swTSpecial	54
#define sw10K		55
#define swRightMB	56
#define swLeftSling	57
#define swRightSling	58
#define sw10Points	59
#define swRKickbig	60
#define swRightEB	61
#define sw20K		62
#define sw30K		63
#define swBSpecial	64

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOuthole	1
#define sTrough		2
#define sLeftEject	3
#define sRightEject	4
#define sTopDrop	5
#define sBottomDrop	6
#define sRightKickBig	7
#define sKnocker	8
#define sMiddleKicker	9
#define sRotaryBeacon	10
#define sRightGate	13
#define sMovingBGuide	14
#define sCBSpinDetent	15
#define sCBSpinMotor	16
#define sLeftGate	17
#define sLeftJet	18
#define sRightJet	19
#define sBottomJet	20
#define sLeftKicker	21
#define sRightKicker	22
#define sTopKickbig	31

/*---------------------
/  Ball state handling
/----------------------*/
enum {stLTrough=SIM_FIRSTSTATE, stRTrough, stShooter, stBallLane, stNotEnough,
     stDrain, stOuthole,stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane,
     stLeftSling, stRightSling, st10Points, stLeftJet, stRightJet, stBottomJet,
     stLeftEject, stRightEject, stLLock, stLLock2, stHabitrail, stLLock3, stRLock, stRLock2, stRLock3,
     stBank, stbAnk, stbaNk, stbanK, stMoney, stmOney, stmoNey, stmonEy, stmoneY, stSilverGold, stCashHeld,
     stMultiplier, stBonus2X, stBonus3X, stBonus5X
};

static sim_tState milln_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Left Trough",	1,swLTrough,	0,		stRTrough,	1},
  {"Right Trough",	1,swRTrough,	sTrough,	stShooter,	1},
  {"Shooter",		1,swShooter,	sShooterRel,	stBallLane,     0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},

  /*Line 2*/
  {"Drain",		1,0,		 0,		stOuthole,	0,	0,	0,	SIM_STNOTEXCL},
  {"Outhole",	        1,swOuthole,	 sOuthole,	stLTrough,	1},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15, sRightGate, stShooter, 5},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15, sLeftGate, stLeftInlane, 5},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},

  /*Line 3*/
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"10 Points",		1,sw10Points,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 4*/
  {"Left Spin Hole",	1,swLeftEject,	 sLeftEject,	stFree,		0},
  {"Rt. Spin Hole",	1,swRightEject,	 sRightEject,	stFree,		0},
  {"Enter L. Lock",	1,swRightLock,	 0,		stLLock2,	5},
  {"L. Big Kicker",	1,swTopKickbig,	 sTopKickbig,	stHabitrail,	0},
  {"Habitrail",		1,0,		 0,		stLLock3,	20},
  {"Left Lock",		1,swRKickbig,	 sRightKickBig,	stFree,		0},
  {"Enter R. Lock",	1,swRollUnder,0,		stRLock2,	4},
  {"R. Lock Lane",	1,swEnterCKickbig,0,		stRLock3,	3},
  {"Right Lock",	1,swCKickbig,	 sMiddleKicker,	stFree,		0},

  /*Line 5*/
  {"Bank - B",		1,swBank,	 0,		stFree,		3},
  {"Bank - A",		1,swbAnk,	 0,		stFree,		3},
  {"Bank - N",		1,swbaNk,	 0,		stFree,		3},
  {"Bank - K",		1,swbanK,	 0,		stFree,		3},
  {"Money - M",		1,swMoney,	 0,		stFree,		3},
  {"Money - O",		1,swmOney,	 0,		stFree,		3},
  {"Money - N",		1,swmoNey,	 0,		stFree,		3},
  {"Money - E",		1,swmonEy,	 0,		stFree,		3},
  {"Money - Y",		1,swmoneY,	 0,		stFree,		3},
  {"Silver/Gold T.",	1,swSilverGold,	 0,		stFree,		3},
  {"Cash Held Tgt.",	1,swLiteCashHeld,0,		stFree,		3},

  /*Line 6*/
  {"Multiplier L.",	1,0,		 0,		0,		0},
  {"2X Target",		1,swBotDrop,	 0,		stFree,		6,0,0,SIM_STSWKEEP},
  {"3X Target",		1,swTopDrop,	 0,		stFree,		6,0,0,SIM_STSWKEEP},
  {"5X Target",		1,swTopMultTgt,	 0,		stFree,		6},

  /*Line 7*/
  {0}
};

static int milln_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 7)
			return setState(stNotEnough,7);
		if (ball->speed < 15)
			return setState(stMoney,15);
		if (ball->speed < 20)
			return setState(stmOney,20);
		if (ball->speed < 25)
			return setState(stmoNey,25);
		if (ball->speed < 35)
			return setState(stMultiplier,35);
		if (ball->speed < 45)
			return setState(stmonEy,45);
		if (ball->speed < 51)
			return setState(stmoneY,51);
		break;

		/*-----------------
		     Multiplier
		-------------------*/
		case stMultiplier:
		if (!core_getSw(swBotDrop) && !core_getSw(swTopDrop))
			return setState(stBonus2X,10);
		if (core_getSw(swBotDrop) && !core_getSw(swTopDrop))
			return setState(stBonus3X,10);
		if (core_getSw(swTopDrop))
			return setState(stBonus5X,10);

		break;
	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData milln_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLeftSling},
  {0, 0x0006, stRightSling},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0010, st10Points},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0201, stLeftEject},
  {0, 0x0202, stRightEject},
  {0, 0x0401, stLLock},
  {0, 0x0402, stRLock},
  {0, 0x0800, stBank},
  {0, 0x1000, stbAnk},
  {0, 0x2000, stbaNk},
  {0, 0x4000, stbanK},
  {0, 0x8000, stDrain},

  {1, 0x0001, stMoney},
  {1, 0x0002, stmOney},
  {1, 0x0004, stmoNey},
  {1, 0x0008, stmonEy},
  {1, 0x0010, stmoneY},
  {1, 0x0020, stSilverGold},
  {1, 0x0040, stMultiplier},
  {1, 0x0080, stCashHeld},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void milln_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Drop T.:  2X / 3X");
  core_textOutf(30, 20,BLACK,"         %-4s/%-4s", core_getSw(swBotDrop) ? "Down":" Up", core_getSw(swTopDrop) ? "Down":" Up");
  core_textOutf(30, 30,BLACK,"Gates:  Left /Right");
  core_textOutf(30, 40,BLACK,"       %-6s/%-6s", core_getSol(sLeftGate) ? " Open":"Closed", core_getSol(sRightGate) ? "Open":"Closed");
}
/* Help */
static void milln_drawStatic(BMTYPE **line) {

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+L = L/R Lock");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+S = L/R Spin Hole");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+- = L/R Slingshot/10 Pts.");
  core_textOutf(30,100,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,110,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,120,BLACK,"T/Y/U/I/O = M/O/N/E/Y Target");
  core_textOutf(30,130,BLACK,"S = Silver/Gold Target");
  core_textOutf(30,140,BLACK,"D/F/G/H = B/A/N/K Target");
  core_textOutf(30,150,BLACK,"C = Cash Held Target");
  core_textOutf(30,160,BLACK,"M = Multiplier Lane");
}

/* Solenoid-to-sample handling */
static wpc_tSamSolMap milln_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOuthole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sLeftKicker,1,SAM_LSLING},
 {sRightKicker,1,SAM_RSLING},

 /*Channel #2*/
 {sTopKickbig,2,SAM_SOLENOID}, {sRightKickBig,2,SAM_SOLENOID},
 {sMiddleKicker,2,SAM_SOLENOID},

 /*Channel #3*/
 {sLeftGate,3,SAM_SOLENOID_ON}, {sLeftGate,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sRightGate,3,SAM_SOLENOID_ON}, {sRightGate,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #4*/
 {sLeftEject,4,SAM_SOLENOID_ON}, {sRightEject,4,SAM_SOLENOID_ON},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
S11_ROMSTART48(milln,l3,"mill_u26.l3", CRC(07bc9fff) SHA1(b16082fb51df3e4d2fb786cb8894b1c232521ef3),
                        "mill_u27.l3", CRC(ba789c43) SHA1(c066a304882bea4cba1e215642416fcb22585aa4))
S11XS_SOUNDROM88(       "mill_u21.l1", CRC(4cd1ee90) SHA1(4e24b96138ced16eff9036303ca6347e3423dbfc),
                        "mill_u22.l1", CRC(73735cfc) SHA1(f74c873a20990263e0d6b35609fc51c08c9f8e31))
S11CS_SOUNDROM88(       "mill_u4.l1",  CRC(cf766506) SHA1(a6e4df19a513102abbce2653d4f72245f54407b1),
                        "mill_u19.l1", CRC(e073245a) SHA1(cbaddde6bb19292ace574a8329e18c97c2ee9763))
S11_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(milln, l3, "Millionaire (L-3)", 1987, "Williams", s11_mS11AS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData millnSimData = {
  2,   			/* 2 game specific input ports */
  milln_stateDef,	/* Definition of all states */
  milln_inportData,	/* Keyboard Entries */
  { stRTrough, stLTrough, stDrain, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  milln_initSim,	/* Simulator Init */
  milln_handleBallState,/*Function to handle ball state changes*/
  milln_drawStatic,	/* Function to handle mechanical state changes*/
  TRUE, 		/* Simulate manual shooter? */
  NULL  		/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData millnGameData = {
  GEN_S11A, s11_dispS11,
  {
    FLIP_SWNO(swLLaneChange,swRLaneChange),
    0,0,0,0,0,0,0,
    NULL, milln_handleMech, NULL, milln_drawMech,
    NULL, milln_samsolmap
  },
  &millnSimData,
  {{ 0 }},
  {12,{0,swLeftJet,swRightJet,swBottomJet,swLeftSling,swRightSling}}
};

/*---------------
/  Game handling
/----------------*/
static void init_milln(void) {
  core_gameData = &millnGameData;
}

/*----------------
/  Init Simulator
/-----------------*/
static void milln_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
	/* Set the Roulette's switches (Don't know if they're are optos, but
	   they act inverted */
	core_setSw(sw40K,1); core_setSw(swLeftMB,1); core_setSw(swLeftEB,1); core_setSw(sw50K,1);
	core_setSw(sw100K,1); core_setSw(swTSpecial,1); core_setSw(sw10K,1); core_setSw(swRightMB,1);
	core_setSw(swRightEB,1); core_setSw(sw20K,1); core_setSw(sw30K,1); core_setSw(swBSpecial,1);
}

static void milln_handleMech(int mech) {

  /* -----------------------------
     --	Raise the Drop Targets  --
     ----------------------------- */
  if ((mech & 0x01) && core_getSol(sBottomDrop))
          core_setSw(swBotDrop,0);
  if ((mech & 0x02) && core_getSol(sTopDrop))
	  core_setSw(swTopDrop,0);

}
