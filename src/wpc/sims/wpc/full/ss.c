/*******************************************************************************
 Scared Stiff (Bally, 1996) Pinball Simulator

 by Tom Haukap (Tom.Haukap@t-online.de)

 301200 First version
 050301 Removed unused getsol function (propably copy/paste error)
 Known issues:
 - the spider wheel turns, but the calculated result is in most cases not equal to
   the result the machine gets. There is a +/-1 step difference all the time...

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Scared Stiff Simulator:
  ------------------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     -  Upper Slingshot
   WED  Jet Bumpers
     C  Crate
	 S  Spider Hole
   JKL  Skull Lanes
     A  Secret Passage
	 V  Extra Ball Lane
   FGH  Leaper Targets
   BNM  Spell Targets
     R  Missed Right Ramp Target
	 T  Telepathetic Ratget
	 Q  SDTM (Drain Ball)
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
static int  ss_handleBallState(sim_tBallStatus *ball, int *inports);
static void ss_handleMech(int mech);
static int  ss_getMech(int mechNo);
static void ss_drawMech(BMTYPE **line);
static void ss_drawStatic(BMTYPE **line);
static int ss_getMech(int mechNo);
static void init_ss(void);

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(ss,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",			KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"L/R Loop",			KEYCODE_L)
    COREPORT_BIT(0x0080,"Upper Jet",			KEYCODE_W)
    COREPORT_BIT(0x0100,"Center Jet",		KEYCODE_E)
    COREPORT_BIT(0x0200,"Lower Jet",			KEYCODE_D)
    COREPORT_BIT(0x0400,"Upper Slingshot",	KEYCODE_MINUS)
    COREPORT_BIT(0x0800,"Hit Crate",			KEYCODE_C)
    COREPORT_BIT(0x1000,"Spider Hole",		KEYCODE_S)
    COREPORT_BIT(0x2000,"L. Skull Lane",		KEYCODE_J)
    COREPORT_BIT(0x4000,"C. Skull Lane",		KEYCODE_K)
    COREPORT_BIT(0x8000,"R. Skull Lane",		KEYCODE_L)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Secret Passage",	KEYCODE_A)
    COREPORT_BIT(0x0002,"Ex. Ball Lane",		KEYCODE_V)
    COREPORT_BIT(0x0008,"Left Leaper",		KEYCODE_F)
    COREPORT_BIT(0x0004,"Center Leaper",		KEYCODE_G)
    COREPORT_BIT(0x0010,"Right Leaper",		KEYCODE_H)
    COREPORT_BIT(0x0020,"Lower Spell",		KEYCODE_B)
    COREPORT_BIT(0x0040,"Middle Spell",		KEYCODE_N)
    COREPORT_BIT(0x0080,"Upper Spell",		KEYCODE_M)
    COREPORT_BIT(0x0100,"Missed Right Ramp",	KEYCODE_R)
    COREPORT_BIT(0x0200,"Telepathetic",		KEYCODE_T)
    COREPORT_BIT(0x0400,"Drain",				KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swNotUsed0		11
#define swWheelIndex	12
#define swStart      	13
#define swTilt       	14
#define swNotUsed1		15
#define swLeftOutlane	16
#define swRightInlane	17
#define swShooter		18

#define swSlamTilt		21
#define swCoinDoor		22
#define swNotUsed2     	23
#define swAlwaysClosed	24
#define swExtraBallLane	25
#define swLeftInlane	26
#define swRightOutlane	27
#define swSingleStandup	28

#define swTroughJam		31
#define swTrough1		32
#define swTrough2		33
#define swTrough3		34
#define swTrough4		35
#define swRightPopper	36
#define swLeftKickout	37
#define swCrateEntrance	38

#define swCoffinLeft	41
#define swCoffinCenter	42
#define swCoffinRight	43
#define swLRampEnter	44
#define swRRampEnter	45
#define swLRampMade		46
#define swRRampMade		47
#define swCoffinEntrance 48

#define swLeftSling		51
#define swRightSling	52
#define swUpperJet		53
#define swCenterJet		54
#define swLowerJet		55
#define swUpperSling	56
#define swDoorSensor	57
#define swLLoop			58

#define sw3BankUp		61
#define sw3BankMid		62
#define sw3BankDown		63
#define	swLLeaper		64
#define swCLeaper		65
#define swRLeaper		66
#define swRRamp10Pt		67
#define swRLoop			68

#define	swLSkullLane	71
#define	swCSkullLane	72
#define	swRSkullLane	73
#define swSecretPassage	74
#define swNotUsed3		75
#define swNotUsed4		76
#define swNotUsed5		77
#define swNotUsed6		78


/*---------------------
/ Solenoid definitions
/----------------------*/
#define sAutoPlunger	 1
#define sLoopGate		 2
#define sRightPopper	 3
#define sCoffinPopper	 4
#define sCoffinDoor		 5
#define sCrateKickout	 6
#define sKnocker		 7
#define sCratePostPower	 8
#define sTrough			 9
#define sLeftSling		10
#define sRightSling		11
#define sCenterJet		12
#define sUpperJet		13
#define sLowerJet		14
#define sUpperSling		15
#define sCratePostHold	16
#define sLDiverterPower	33
#define sLDiverterHold	34

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stUpperJet, stCenterJet, stLowerJet, stUpperSling,

	  stLRampEnter, stLRampDone,
	  stRRampEnter, stRRampDone,

	  stLLoopEnter, stLLoopMiddle, stLLoopExit,
	  stRLoopEnter,

	  stHitCrate, stInsideCrate, stSkillHole,

	  stEnterCoffin, stCoffinRight, stCoffinCenter, stCoffinLeft,

	  stSkulls, stJetBumpers,

	  stSpiderHole, stLSkullLane, stCSkullLane, stRSkullLane,
	  stSecretPassage1, stSecretPassage2, stExtraBallLane,
	  stLowerSpell, stMiddleSpell, stUpperSpell,
          stLeftLeaper, stCenterLeaper, stRightLeaper, stRRamp10pt, stTelepathetic
	  };

static sim_tState ss_stateDef[] = {
  {"Not Installed",		0,0,				0,				stDrain,		0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",			0,0,				0,				0,				0,0,0,SIM_STNOTEXCL},

  {"Trough 4",			1,swTrough4,		0,				stTrough3,		1},
  {"Trough 3",			1,swTrough3,		0,				stTrough2,		1},
  {"Trough 2",			1,swTrough2,		0,				stTrough1,		1},
  {"Trough 1",			1,swTrough1,		sTrough,		stTrough,		1},
  {"Trough Jam",		1,swTroughJam,		0,				stShooter,		1},
  {"Drain",				1,0,				0,				stTrough4,		0,0,0,SIM_STNOTEXCL},

  {"Shooter",			1,swShooter,		0,				0,				0,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",			1,0,				0,				0,				2,0,0,SIM_STNOTEXCL},
  {"No Strength",		1,0,				0,				stShooter,		3},
  {"Right Outlane",		1,swRightOutlane,	0,				stDrain,		15},
  {"Left Outlane",		1,swLeftOutlane,	0,				stDrain,		15},
  {"Right Inlane",		1,swRightInlane,	0,				stFree,			5},
  {"Left Inlane",		1,swLeftInlane,		0,				stFree,			5},
  {"Left Slingshot",	1,swLeftSling,		0,				stFree,			1},
  {"Rt Slingshot",		1,swRightSling,		0,				stFree,			1},
  {"Upper Jet",			1,swUpperJet,		0,				stJetBumpers,	1},
  {"Center Jet",		1,swCenterJet,		0,				stJetBumpers,	1},
  {"Lower Jet",			1,swLowerJet,		0,				stFree,			1},
  {"Upper Slingsht",	1,swUpperSling,		0,				stFree,			1},

  /* ramps */
  {"Left Ramp",			1,swLRampEnter,		0,				stLRampDone,	5},
  {"Left Ramp Done",    1,swLRampMade,		0,				stRightInlane,	10},
  {"Right Ramp",		1,swRRampEnter,		0,				stRRampDone,	5},
  {"Rt. Ramp Done",		1,swRRampMade,		0,				stLeftInlane,	10},

  /* loops */
  {"Left Loop",			1,swLLoop,			0,				stLLoopMiddle,	5,sLDiverterHold,stEnterCoffin,SIM_STNOTEXCL},
  {"On Left Loop",		1,0,				0,				stSkulls,		3,sLoopGate,stLLoopExit},
  {"Left Loop Exit",	1,swRLoop,			0,				stJetBumpers,	3},
  {"Right Loop",		1,0,				0,				stSkillHole,	5},

  /* crate */
  {"Hit Crate",			1,swDoorSensor,		0,				stFree,			1,sCratePostHold,stInsideCrate,SIM_STNOTEXCL},
  {"Inside Crate",		1,swCrateEntrance,	0,				stSkillHole,	5},
  {"Skill Shot H.",		1,swLeftKickout,	sCrateKickout,	stFree,			3},

  /* coffin */
  {"Enter Coffin",		1,swCoffinEntrance,	0,				stCoffinRight,	15},
  {"Coffin Right",		1,swCoffinRight,	0,				stCoffinCenter,	2},
  {"Coffin Center",		1,swCoffinCenter,	0,				stCoffinLeft,	2},
  {"Coffin Left",		1,swCoffinLeft,		sCoffinPopper,	stLeftInlane,	5},

  /* two state, if the ball is in the Skull Lane or Bumber region */
  {"Skull Lanes",		1,0,				0,				0,				0},
  {"Jet Bumpers",		1,0,				0,				0,				0},

  /* all the rest */
  {"Spider Hole",		1,swRightPopper,	sRightPopper,	stFree,			5},
  {"L. Skull Lane",		1,swLSkullLane,		0,				stSkillHole,	5},
  {"C. Skull Lane",		1,swCSkullLane,		0,				stSkillHole,	5},
  {"R. Skull Lane",		1,swRSkullLane,		0,				stSkillHole,	5},
  {"Secret Passage",	1,swLLoop,			0,				stSecretPassage2,5},
  {"Secret Passage",	1,swSecretPassage,	0,				stSkillHole,	3},
  {"Ex. Ball Lane",		1,swExtraBallLane,	0,				stSkulls,		3},
  {"Lower Spell",		1,sw3BankUp,		0,				stFree,			1},
  {"Middle Spell",		1,sw3BankMid,		0,				stFree,			1},
  {"Upper Spell",		1,sw3BankDown,		0,				stFree,			1},
  {"Left Leaper",		1,swLLeaper,		0,				stFree,			1},
  {"Center Leaper",		1,swCLeaper,		0,				stFree,			1},
  {"Right Leaper",		1,swRLeaper,		0,				stFree,			1},
  {"Missed R Ramp",		1,swRRamp10Pt,		0,				stFree,			1},
  {"Telepathetic",		1,swSingleStandup,	0,				stFree,			1},

  {0}
};

static void ss_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
	core_setSw(swWheelIndex, TRUE);
}

static int ss_handleBallState(sim_tBallStatus *ball, int *inports) {
	switch (ball->state) {
	case stShooter:
		if (core_getSol(sAutoPlunger)) {		/* SS has both, manual and auto plunger */
			ball->speed = 50;
			return setState(stBallLane,1);
		}
		if (sim_getSol(sShooterRel)) {
			return setState(stBallLane,1);
		}
		break;

	case stBallLane:
		if (ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
		if (ball->speed < 40)
			return setState(stSkillHole,10);
		if (ball->speed < 51)
			return setState(stFree,51);		/*Ball goes to stFree*/
		break;
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData ss_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLRampEnter},
  {0, 0x0006, stRRampEnter},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0041, stLLoopEnter},
  {0, 0x0042, stRLoopEnter},
  {0, 0x0080, stUpperJet, stJetBumpers},
  {0, 0x0100, stCenterJet, stJetBumpers},
  {0, 0x0200, stLowerJet, stJetBumpers},
  {0, 0x0400, stUpperSling},
  {0, 0x0800, stHitCrate},
  {0, 0x1000, stSpiderHole},
  {0, 0x2000, stLSkullLane, stSkulls},
  {0, 0x4000, stCSkullLane, stSkulls},
  {0, 0x8000, stRSkullLane, stSkulls},

/* Port 1 */
  {1, 0x0001, stSecretPassage1},
  {1, 0x0002, stExtraBallLane},
  {1, 0x0004, stLowerSpell},
  {1, 0x0008, stMiddleSpell},
  {1, 0x0010, stUpperSpell},
  {1, 0x0020, stLeftLeaper},
  {1, 0x0040, stCenterLeaper},
  {1, 0x0080, stRightLeaper},
  {1, 0x0100, stRRamp10pt},
  {1, 0x0200, stTelepathetic},
  {1, 0x0400, stDrain},
  {0}
};
	/* Help */
static void ss_drawStatic(BMTYPE **line) {
	core_textOutf(30, 70,BLACK,"Help on this Simulator:");
	core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R/Upper Slingshot");
	core_textOutf(30, 90,BLACK,"L/R Ctrl+R = L/R Ramp");
	core_textOutf(30,100,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
	core_textOutf(30,110,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
	core_textOutf(30,120,BLACK,"C=Crate, S=Spider Hole, R=Missed Ramp");
	core_textOutf(30,130,BLACK,"B,N,M=Leapers, F,G,H=Spell Targets");
	core_textOutf(30,140,BLACK,"V=Extra Ball Lane, J,K,L=Skull Lanes");
	core_textOutf(30,150,BLACK,"A=Secret Passage, W,E,D=Jet Bumpers");
	core_textOutf(30,160,BLACK,"T=Telepathetic, Q=Drain");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(ss,15,"ss_g11.1_5",0x80000,CRC(5de8d0a0) SHA1(91cdd5f4e1654fd4dbde8b9cb03db935cba5d876))
DCS_SOUNDROM3m("sssnd_11.s2",CRC(1b080a72) SHA1(be80e99e6bcc482379fe99519a4122f3b225be30),
               "sssnd_11.s3",CRC(c4f2e08a) SHA1(e20ff622a3f475db11f1f44d36a6669e160437a3),
               "sssnd_11.s4",CRC(258b0a27) SHA1(83763b98907cf38e6f7b9fe4f26ce93a54ba3568))
WPC_ROMEND
WPC_ROMSTART(ss,14,"stiffg11.1_4",0x80000,CRC(17359ed6) SHA1(2ae549064a3666ea8b0b09aff9f5551db906d1d2))
DCS_SOUNDROM3m("sssnd_11.s2",CRC(1b080a72) SHA1(be80e99e6bcc482379fe99519a4122f3b225be30),
               "sssnd_11.s3",CRC(c4f2e08a) SHA1(e20ff622a3f475db11f1f44d36a6669e160437a3),
               "sssnd_11.s4",CRC(258b0a27) SHA1(83763b98907cf38e6f7b9fe4f26ce93a54ba3568))
WPC_ROMEND
WPC_ROMSTART(ss,12,"stiffg11.1_2",0x80000,CRC(70eca59c) SHA1(07d50a32a4fb287780c4e6c1cb6fbeba97480219))
DCS_SOUNDROM3m("sssnd_11.s2",CRC(1b080a72) SHA1(be80e99e6bcc482379fe99519a4122f3b225be30),
               "sssnd_11.s3",CRC(c4f2e08a) SHA1(e20ff622a3f475db11f1f44d36a6669e160437a3),
               "sssnd_11.s4",CRC(258b0a27) SHA1(83763b98907cf38e6f7b9fe4f26ce93a54ba3568))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(ss,15,"Scared Stiff (1.5)",1996,"Bally",wpc_m95S,0)
CORE_CLONEDEF(ss,14,15,"Scared Stiff (1.4)",1996,"Bally",wpc_m95S,0)
CORE_CLONEDEF(ss,12,15,"Scared Stiff (1.2)",1996,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData ssSimData = {
  2,    					/* 2 game specific input ports */
  ss_stateDef,				/* Definition of all states */
  ss_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  ss_initSim, 				/* init */
  ss_handleBallState,		/* Function to handle ball state changes*/
  ss_drawStatic,			/* Function to handle mechanical state changes*/
  TRUE, 					/* Simulate manual shooter */
  NULL  					/* no key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData ssGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1,0,
    NULL, ss_handleMech, ss_getMech, ss_drawMech,
    NULL, NULL
  },
  &ssSimData,
  {
    "548 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x02, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

static mech_tInitData ss_wheelMech = {
  39,40, MECH_LINEAR|MECH_CIRCLE|MECH_TWOSTEPSOL|MECH_FAST, 200, 200,
  {{swWheelIndex, 25, 199}}
};
/*---------------
/  Game handling
/----------------*/
static void init_ss(void) {
  core_gameData = &ssGameData;
  mech_add(0,&ss_wheelMech);
}

static void ss_handleMech(int mech) {
//  if (mech & 0x01) mech_update(0);
}
static int ss_getMech(int mechNo) {
  return mech_getPos(mechNo);
}
static const char *spiderWheelText[] =
  {"Collect Deadhead  ", "Jackpot Is Lit    ", "Double Trouble    ", "Collect Eyeball   ",
   "Beat The Crate    ", "Coffin Multiball  ", "Telepathetic Power", "Laboratory        ",
   "Telepathetic Power", "Boogie Man Boogie ", "Crate Multiball   ", "Collect Eyeball   ",
   "Leaper Mania      ", "Beast Hurry Up    ", "Collect Deadhead  ", "Collect Eyeball   ", "Collect Deadhead  "};
/*--------------------
  Drawing information
  --------------------*/
static void ss_drawMech(BMTYPE **line) {
        core_textOutf(30,10,BLACK,"Wheel Pos   : %3d",  ss_getMech(0));
        core_textOutf(30,20,BLACK,"Coffin Door : %-6s", core_getSol(sCoffinDoor)?"Open":"Closed");
        core_textOutf(30,30,BLACK,"Loop Gate   : %-6s", core_getSol(sLoopGate) ? "Open":"Closed");
        core_textOutf(30,40,BLACK,"Coin Door   : %-6s", core_getSw(swCoinDoor) ? "Closed":"Open");
        core_textOutf(30,50,BLACK,"Spider Wheel: %s",   spiderWheelText[(mech_getPos(0)+6)*2/25]);
}

