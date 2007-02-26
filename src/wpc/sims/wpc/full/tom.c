/*******************************************************************************
 Theatre of Magic (Bally) Pinball Simulator

 by Tom Haukap (tom@rattle.de)
 Dec. 26, 2000

 Special thanks to Bowen Kerins (Rulesheet)

 261200  First version
 301200  Added keycode to put the ball into the rear trunk entrance (KEYCODE_R)
 010101  Added sample support

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Theatre of Magic Simulator:
  ------------------------------------
    +R  L/R Ramp
    +O  L/R Outlane
    +-  L/R Slingshot
    +I  L/R Inlane
   WES  Jet Bumpers
    +K  L/R Inner Loops
    +L  L/R Outer Loops
	 B  Hit Captured Ball (Tiger Saw)
     T	Hit Trunk
     G	Hokus Pokus Targets
	 H  Trap Door Targets
	 J  Poof Target
	 M	Left Bonus X Lane
	 N	Right Bonus X Lane
	 R	Rear Trunk Entrance
	 2  Buy-In
     Q  SDTM (Drain Ball)

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

#define TOM_TRUNKTICKS		2    /* turn 2 degrees per simulator tick */
#define TOM_SWCLOSEDDEGREE	20   /* degrees during a trunk switch should be closed */
#define TOM_MAGNETTICKSINIT	30   /* ticks a ball is in range of the trunk magnet */

#define TOM_CAPTUREDBALL	1    /* the first ball will be used to simulate the captured ball  /
                                 /  this is the marker for the ball                           */

/*------------------
/  Local functions
/-------------------*/
static int  tom_handleBallState(sim_tBallStatus *ball, int *inports);
static void tom_handleMech(int mech);
static void tom_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls);
static void init_tom(void);
static void tom_drawMech(BMTYPE **line);
static void tom_drawStatic(BMTYPE **line);
static int tom_getMech(int mechNo);
/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int trunkPos;
  int hitCapturedBall;
  int magnetTicks;
} locals;


/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(tom,5)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",		KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",		KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",			KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",			KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",			KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",			KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",				KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",				KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",			KEYCODE_S)
    COREPORT_BIT(0x0200,"Hit Trunk",				KEYCODE_T)
    COREPORT_BITIMP(0x0400,"L/R Inner Loop",		KEYCODE_K)
    COREPORT_BITIMP(0x0800,"L/R Outer Loop",		KEYCODE_L)
    COREPORT_BITIMP(0x1000,"Hit Captured Ball",	KEYCODE_B)
    COREPORT_BIT(0x2000,"Hokus Pokus Targets",	KEYCODE_G)
    COREPORT_BIT(0x4000,"Trap Door Targets",		KEYCODE_H)
    COREPORT_BIT(0x8000,"Poof Target",			KEYCODE_J)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Bonus X L",				KEYCODE_N)
    COREPORT_BIT(0x0002,"Bonus X R",				KEYCODE_M)
    COREPORT_BIT(0x0004,"Rear Trunk Entrance",	KEYCODE_R)
    COREPORT_BIT(0x0008,"Buy-In",   				KEYCODE_2)
    COREPORT_BIT(0x0010,"Drain",					KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swNotUsed0		11
#define swNotUsed1		12
#define swStart      	13
#define swTilt       	14
#define swShooter		15
#define swNotUsed2		16
#define swNotUsed3		17
#define swNotUsed4		18

#define swSlamTilt		21
#define swCoinDoor		22
#define swBuyIn     	23
#define swAlwysClosed0  24
#define swLeftOutlane	25
#define swLeftInlane	26
#define swRightInlane	27
#define swRightOutlane	28

#define swTroughJAM		31
#define swTrough1		32
#define swTrough2		33
#define swTrough3		34
#define swTrough4		35
#define swSubwayOpto	36
#define swSpinner		37
#define swRLTarget		38

#define swLock1			41
#define swLock2			42
#define swLock3			43
#define swPopper		44
#define swLDrain		45
#define swNotUsed5		46
#define swSubwayMicro	47
#define swRDrain		48

#define swLBankTargets	51
#define swCapRest		52
#define swRLaneEnter	53
#define swLLaneEnter	54
#define swCubePos4		55
#define swCubePos1		56
#define swCubePos2		57
#define swCubePos3		58

#define swLeftSling		61
#define swRightSling	62
#define swBottomJet		63
#define swMiddleJet		64
#define swTopJet		65
#define swTopLane1		66
#define swTopLane2		67
#define swNotUsed6		68

#define swCRampExit		71
#define swNotUsed7		72
#define swRRampExit		73
#define swRRampExit2	74
#define swCRampEnter	75
#define swRRampEnter	76
#define swCapTop		77
#define swLLoop			78

#define swRLoop			81
#define swCRampTargets	82
#define swVanishLock1	83
#define swVanishLock2	84
#define swTrunkHit		85
#define swRLaneExit		86
#define swLLaneExit		87
#define swNotUsed8		88

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sBallTrough				1
#define sMagnetDiverter			2
#define sTrapDoorUp				3
#define sSubwayPopper			4
#define sRightDrainMagnet		5
#define sCLoopPost				6
#define sKnocker				7
#define sTDiverterPost			8
#define sLeftSling				9
#define sRightSling				10
#define sBottomJet				11
#define sMiddleJet				12
#define sTopJet					13
#define sTrapDoorHold			14
#define sLUpDownGate			15
#define sRUpDownGate			16
#define sBoxClockwise			17
#define sBoxCounterClockwise	18
#define sTopKickout				21
#define sCubeMagnet				33
#define sSubBallRelease			34
#define sLeftDrainMagnet		35

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stTroughJAM, stDrain, stDraining,
      stShooter, stBallLane, stNotEnough,
	  stLeftOutlaneTop, stLeftOutlane, stOnLeftMagnet,
	  stRightOutlaneTop, stRightOutlane, stOnRightMagnet,
	  stLeftInlane, stRightInlane,
	  stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,

	  stTrunkHit, stGrabbedByTrunk,

	  stRearTrunkEntrance, stInTrunk, stLock3, stLock2, stLock1, stTrapDoorPopper,

	  stCRampEnter, stCRampExit, stLRamp,
	  stRRamp, stRRampEnter, stRRampMiddle, stRRampExit, stUnderSpiritRing, stSpiritRing, stCHabit,

	  stLInnerLoopEnter, stLInnerLoopExit, stLInnerLoopSpinner,
	  stRInnerLoopSpinner, stRInnerLoopEnter, stRInnerLoopExit,

	  stLOuterLoopEnter, stLOuterLoopTop, stLOuterLoopRight, stLOuterLoopExit, stRHabit,
	  stROuterLoopEnter, stROuterLoopTop, stROuterLoopExit,

	  stCapturedHit, stCapturedRest, stCapturedTop,

	  stVanishLook2, stVanishLook1,
	  stHokusPokus, stTrapDoorTargets, stPoofTarget, stBonusXL, stBonusXR
	  };

static sim_tState tom_stateDef[] = {

/* general stuff, free state, ball troughs, shooting and draining */
  {"Not Installed",		0,0,			 0,		stDraining,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",			0,0,			 0,		0,		0,	0,	0,	SIM_STNOTEXCL},
  {"Trough 1",			1,swTrough1,	 sBallTrough,	stTroughJAM,	1},
  {"Trough 2",			1,swTrough2,	 0,		stTrough1,	5},
  {"Trough 3",			1,swTrough3,	 0,		stTrough2,	5},
  {"Trough 4",			1,swTrough4,	 0,		stTrough3,	5},
  {"Trough JAM",		1,swTroughJAM,	 0,		stShooter,	1},
  {"Drain",             0,0,         	 0,     0,  		0},
  {"Draining",          0,0,         	 0,     stTrough4,  0,0,0, SIM_STNOTEXCL},
  {"Shooter",			1,swShooter,	 sShooterRel,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",			1,0,			 0,		0,			2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",		1,swShooter,	 0,		stShooter,	3},

/* Inlanes, Slingshots, Jets */
  {"Left Outlane Top",	1,swLDrain,		 0,		0,			0},
  {"Left Outlane",		1,swLeftOutlane, 0,		stDrain,	15},
  {"On Left Magnet",	1,0,			 0,		0,			0},
  {"Rt Outlane Top",	1,swRDrain,      0,		0,			0},
  {"Right Outlane",		1,swRightOutlane,0,		stDrain,	15},
  {"On Rt Magnet",	    1,0,			 0,		0,			0},
  {"Left Inlane",		1,swLeftInlane,	 0,		stFree,		5},
  {"Right Inlane",		1,swRightInlane, 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",		1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",		1,swTopJet,		 0,		stFree,		1},
  {"Right Bumper",		1,swMiddleJet,	 0,		stFree,		1},
  {"Bottom Bumper",		1,swBottomJet,	 0,		stFree,		1},

/* trunk */
  {"Trunk Hit",		    2,swTrunkHit,	 0,		0,		    0},
  {"Grabbed by Tr.",    0,0,			 0,		0,		    0},

/* subway, locking and trap door */
  {"Rear Trunk",		1,swSubwayOpto,	 0,		stInTrunk,	5},
  {"In Trunk",			1,swSubwayMicro, 0,		stLock3,	5},
  {"Lock 3",			1,swLock3,		 0,		stLock2,	2},
  {"Lock 2",			1,swLock2,		 0,		stLock1,	2},
  {"Lock 1",			1,swLock1,		 sSubBallRelease,		stTrapDoorPopper,	5},
  {"Trap Door Pop.",	1,swPopper,		 sSubwayPopper,		    stFree,				2},

/* ramps */
  {"C. Ramp Enter",		1,swCRampEnter,	 0,	stCRampExit,  5},
  {"C. Ramp Done",		1,swCRampExit,	 0,	stLRamp,	  5},
  {"Left Ramp",		    1,0,			 0,	stLeftInlane, 2},

  {"Right Ramp",		0,0,			 0,	stRRampEnter,       2,sTrapDoorHold,stTrapDoorPopper,0},
  {"R. Ramp Enter",		1,swRRampEnter,	 0,	stRRampMiddle,		5},
  {"R Ramp",		    1,swRRampExit,	 0,	stRRampExit,		5},
  {"R Ramp Done",		1,swRRampExit2,	 0,	stUnderSpiritRing,  2},
  {"Under Spirit",	    0,0,			 0,	0,					0},
  {"Spirit Ring",		0,0,			 0,	0,					0},
  {"L/R Habitrail",	    1,0,			 0,	stRightInlane,		10},

/* loops */
  {"Enter LI Loop",		1,swLLoop,		 0,	stLInnerLoopExit,	2,sCLoopPost,stRearTrunkEntrance,SIM_STNOTEXCL},
  {"Exit LI Loop",		1,swRLoop,		 0,	stLInnerLoopSpinner,2},
  {"Spinner",			1,swRLoop,		 0,	stFree,				2, 0,0,SIM_STSPINNER},

  {"Spinner",			1,swSpinner,	 0,	stRInnerLoopEnter,	2,0,0,SIM_STSPINNER},
  {"Enter RI Loop",		1,swRLoop,	     0,	stRInnerLoopExit,	2,sCLoopPost,stRearTrunkEntrance,SIM_STNOTEXCL},
  {"Exit RI Loop",		1,swLLoop,		 0,	stFree,				2},

  {"Enter LO Loop",		1,swLLaneEnter,	 0,	stLOuterLoopTop,	3},
  {"Top LO Loop",		1,swRLaneExit,	 0,	stLOuterLoopRight,	3,sTDiverterPost,stVanishLook2},
  {"Right LO Loop",		1,0,			 0,	stFree,				2,sRUpDownGate,stLOuterLoopExit,0},
  {"Exit LO Loop",		1,swLLaneExit,	 0,	stRHabit,			5},
  {"Rt Habitrail",	    1,0,			 0,	stRightInlane,	    5},

  {"Enter RO Loop",		1,swRLaneEnter,	 0,	stFree,				3,sLUpDownGate,stROuterLoopTop,0},
  {"Top RO Loop",		1,swRLaneExit,	 0,	stROuterLoopExit,	3},
  {"Exit RO Loop",		1,swLLaneEnter,	 0,	stFree,				3},

/* captured ball */
  {"Hit Cap. Ball",		1,0,			 0,				0,			    0},
  {"Captured Ball",		0,swCapRest,	 0,				0,			    0},
  {"Capt. Ball Top",	1,swCapTop,		 0,				stCapturedRest, 5},

/* all the rest */
  {"Vanish Lock 2",		1,swVanishLock2, 0,			    stVanishLook1,		3},
  {"Vanish Lock 1",		1,swVanishLock1, sTopKickout,	stROuterLoopExit,	5},
  {"Hokus Pokus T.",	1,swLBankTargets,0,				stFree,				1},
  {"Trap Door T.",		1,swCRampTargets,0,				stFree,				1},
  {"Poof Target",		1,swRLTarget,	 0,				stFree,				1},
  {"Bonus X Left",		1,swTopLane1,	 0,				stFree,				1},
  {"Bonus X Right",		1,swTopLane2,	 0,				stFree,				1},

  {0}
};

static int tom_handleBallState(sim_tBallStatus *ball, int *inports) {
	switch (ball->state) {

	case stBallLane:
		if(ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/

		return setState(stRHabit,35);

	case stTrunkHit:
		if (!core_getSw(swCubePos4)) {
			if (locals.magnetTicks)
				return setState(stGrabbedByTrunk, 1);
		}
		else if (!core_getSw(swCubePos2))
			return setState(stInTrunk,1);
		return setState(stFree,1);

	case stGrabbedByTrunk:
		if (!locals.magnetTicks)
			return setState(stFree,1);
		if (!core_getSw(swCubePos2))
			return setState(stRearTrunkEntrance,1);
		break;

	case stUnderSpiritRing:
		if (core_getSol(sMagnetDiverter))
			return setState(stSpiritRing, 1);
		else
			return setState(stCHabit, 5);
		break;

	case stSpiritRing:
		if(!core_getSol(sMagnetDiverter))
			return setState(stLRamp, 2);
		break;

	case stLeftOutlaneTop:
		if(core_getSol(sLeftDrainMagnet))
			return setState(stOnLeftMagnet,1);
		else
			return setState(stLeftOutlane,2);
		break;

	case stOnLeftMagnet:
		if(!core_getSol(sLeftDrainMagnet))
			return setState(stLeftInlane,4);
		break;

	case stRightOutlaneTop:
		if(core_getSol(sRightDrainMagnet))
			return setState(stOnRightMagnet,1);
		else
			return setState(stRightOutlane,2);
		break;

	case stOnRightMagnet:
		if (!core_getSol(sRightDrainMagnet))
			return setState(stRightInlane,4);
		break;

    case stCapturedHit:
        locals.hitCapturedBall = 1;
      	return setState(stFree,1);

	case stCapturedRest:
		if(locals.hitCapturedBall) {
			locals.hitCapturedBall = 0;
			return setState(stCapturedTop,2);
		}
		break;

    case stDrain:
      /* don't let drain the captured ball */
      if (ball->type)
        return setState(stCapturedRest,0);
      else
        return setState(stDraining,0);
	}
	return 0;
}

static void tom_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
    initBallType(0, TOM_CAPTUREDBALL);

    locals.trunkPos = 0;
	locals.hitCapturedBall = 0;
	locals.magnetTicks = 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData tom_inportData[] = {

/* Port 0 */
  {0, 0x0005, stCRampEnter},
  {0, 0x0006, stRRamp},
  {0, 0x0009, stLeftOutlaneTop},
  {0, 0x000a, stRightOutlaneTop},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0200, stTrunkHit},
  {0, 0x0401, stLInnerLoopEnter},
  {0, 0x0402, stRInnerLoopSpinner},
  {0, 0x0801, stLOuterLoopEnter},
  {0, 0x0802, stROuterLoopEnter},
  {0, 0x1000, stCapturedHit},
  {0, 0x2000, stHokusPokus},
  {0, 0x4000, stTrapDoorTargets},
  {0, 0x8000, stPoofTarget},

/* Port 1 */
  {1, 0x0001, stBonusXL},
  {1, 0x0002, stBonusXR},
  {1, 0x0004, stRearTrunkEntrance},
  {1, 0x0008, swBuyIn, SIM_STANY},
  {1, 0x0010, stDrain},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay tom_lampPos = {
{ 0,  0}, /* top left */
{43, 26}, /* size */
{
/* 11 */  {1,{{29, 7,RED   }}}, /* 12 */  {1,{{28, 9,RED   }}}, /* 13 */  {1,{{27,11,RED   }}}, /* 14 */  {1,{{27,13,RED   }}},
/* 15 */  {1,{{27,15,RED   }}}, /* 16 */  {1,{{28,17,RED   }}}, /* 17 */  {1,{{29,19,RED   }}}, /* 18 */  {1,{{25, 9,LBLUE }}},
/* 21 */  {1,{{23,22,WHITE }}}, /* 22 */  {1,{{24,21,RED   }}}, /* 23 */  {1,{{25,20,WHITE }}}, /* 24 */  {1,{{26,19,RED   }}},
/* 25 */  {1,{{24,23,WHITE }}}, /* 26 */  {2,{{23, 5,ORANGE},{27,21,ORANGE}}}, /* 27 */  {1,{{24,11,LBLUE  }}}, /* 28 */  {1,{{14,13,RED   }}},
/* 31 */  {1,{{23,13,LBLUE }}}, /* 32 */  {1,{{25,17,LBLUE }}}, /* 33 */  {1,{{15,15,RED   }}}, /* 34 */  {1,{{17,15,WHITE }}},
/* 35 */  {1,{{19,15,WHITE }}}, /* 36 */  {1,{{24,15,LBLUE }}}, /* 37 */  {1,{{ 1,18,YELLOW}}}, /* 38 */  {1,{{ 2,21,YELLOW}}},
/* 41 */  {1,{{17, 2,RED   }}}, /* 42 */  {1,{{19, 3,RED   }}}, /* 43 */  {1,{{21, 4,WHITE }}}, /* 44 */  {1,{{20,11,RED   }}},
/* 45 */  {2,{{13, 6,WHITE },{14,19,WHITE }}}, /* 46 */  {1,{{18, 5,YELLOW}}}, /* 47 */  {1,{{15, 6,RED   }}}, /* 48 */  {1,{{14, 9,RED   }}},
/* 51 */  {1,{{13,11,RED   }}}, /* 52 */  {1,{{19,11,RED   }}}, /* 53 */  {1,{{18,11,RED   }}}, /* 54 */  {2,{{17, 7,WHITE },{17,17,WHITE }}},
/* 55 */  {1,{{17,11,RED   }}}, /* 56 */  {1,{{15,11,RED   }}}, /* 57 */  {1,{{16,11,RED   }}}, /* 58 */  {1,{{21, 6,RED   }}},
/* 61 */  {1,{{30,11,YELLOW}}}, /* 62 */  {1,{{32,11,GREEN }}}, /* 63 */  {2,{{29,12,RED   },{29,14,RED   }}}, /* 64 */  {1,{{34,11,ORANGE}}},
/* 65 */  {1,{{36,11,RED   }}}, /* 66 */  {1,{{30,15,YELLOW}}}, /* 67 */  {1,{{32,15,RED   }}}, /* 68 */  {1,{{34,15,GREEN }}},
/* 71 */  {1,{{36,15,LBLUE }}}, /* 72 */  {1,{{12,19,ORANGE}}}, /* 73 */  {1,{{38,10,RED   }}}, /* 74 */  {1,{{39,12,RED   }}},
/* 75 */  {1,{{39,14,ORANGE}}}, /* 76 */  {1,{{38,16,RED   }}}, /* 77 */  {1,{{21,25,RED   }}}, /* 78 */  {1,{{26, 5,ORANGE}}},
/* 81 */  {2,{{38, 2,RED},{38,24,RED}}}, /* 82 */  {1,{{0,0,BLACK}}}, /* 83 */  {1,{{0,1,BLACK}}}, /* 84 */  {1,{{0,2,BLACK}}},
/* 85 */  {1,{{11,11,GREEN }}}, /* 86 */  {1,{{41,13,RED   }}}, /* 87 */  {1,{{43,26,RED   }}}, /* 88 */  {1,{{43, 1,YELLOW}}}
}
};

static wpc_tSamSolMap tom_SamSolMap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER},
 {sBallTrough,0,SAM_BALLREL},
 {sCLoopPost,0,SAM_SOLENOID_ON}, {sCLoopPost,0,SAM_SOLENOID_OFF, WPCSAM_F_ONOFF},

 /*Channel #1*/
 {sLeftSling,1,SAM_LSLING}, {sRightSling,1,SAM_RSLING},
 {sSubwayPopper, 1, SAM_POPPER},
 {sBottomJet,1,SAM_JET1},

 /*Channel #2*/
 {sTrapDoorUp,2,SAM_FLAPOPEN}, {sTrapDoorHold,2,SAM_FLAPCLOSE,WPCSAM_F_ONOFF},
 {sBoxClockwise,2,SAM_MOTOR_1, WPCSAM_F_CONT}, {sBoxCounterClockwise,2,SAM_MOTOR_1, WPCSAM_F_CONT},
 {sTDiverterPost,2,SAM_SOLENOID_ON}, {sTDiverterPost,2,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sMiddleJet,2,SAM_JET2},

 /*Channel #3*/
 {sSubBallRelease, 3, SAM_SOLENOID},
 {sTopKickout, 3, SAM_SOLENOID},
 {sTopJet,3,SAM_JET3},{-1}
};

static void tom_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Trunk Pos : %-3d", locals.trunkPos);
  if (!core_getSw(swCubePos1))
	core_textOutf(30,10,BLACK,"Trunk Face: Closed ");
  else if (!core_getSw(swCubePos4))
	core_textOutf(30,10,BLACK,"Trunk Face: Magnet ");
  else if (!core_getSw(swCubePos3))
	core_textOutf(30,10,BLACK,"Trunk Face: Lamp   ");
  else if (!core_getSw(swCubePos2))
	core_textOutf(30,10,BLACK,"Trunk Face: Open   ");
  else
	core_textOutf(30,10,BLACK,"Trunk Face: Turning");

  core_textOutf(30,20,BLACK,"Trunk Mag.: %-3s", core_getSol(sCubeMagnet) ? "On":"Off");
  core_textOutf(30,30,BLACK,"Trap Door : %-6s", core_getSol(sTrapDoorHold) ? "Open":"Closed");
  core_textOutf(30,40,BLACK,"L.Post: %-2s T.Post: %-2s", core_getSol(sCLoopPost) ? "Up":"Dn",core_getSol(sTDiverterPost) ? "Up":"Dn");
  core_textOutf(30,50,BLACK,"L.Gate: %-2s R.Gate: %-2s", core_getSol(sLUpDownGate) ? "Op":"Cl",core_getSol(sRUpDownGate) ? "Op":"Cl");
  core_textOutf(30,60,BLACK,"Coin Door : %-6s", core_getSw(swCoinDoor) ? "Closed":"Open");
}
/* Help */

static void tom_drawStatic(BMTYPE **line) {
  core_textOutf(30, 80,BLACK,"Help on this Simulator:");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30,100,BLACK,"L/R Ctrl+R = L/R Ramps (Trap Door)");
  core_textOutf(30,110,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,120,BLACK,"L/R Ctrl+K/L = L/R Inner/Outer Loops");
  core_textOutf(30,130,BLACK,"Q = Drain Ball, W/E/S = Jet Bumpers");
  core_textOutf(30,140,BLACK,"G,H,J = Hokus Pokus, Trap Door, Poof");
  core_textOutf(30,150,BLACK,"N,M = L/R Bonux X, R = Rear Trunk");
  core_textOutf(30,160,BLACK,"T = Trunk, B = Tiger Saw Cap. Ball");
  }

#define TOM_SOUND \
DCS_SOUNDROM6x("tm_u2_s.l2",CRC(b128fbba) SHA1(59101f9f4f43c240630dfbdc7fb432a9939f122d), \
               "tm_u3_s.l2",CRC(128c7d3c) SHA1(1bd5b56d3f9c8485498746ae6c4d65a1e053161a), \
               "tm_u4_s.l2",CRC(3d9b2354) SHA1(a39917c0cceda33288594652c47fd0385a85b8b1), \
               "tm_u5_s.l2",CRC(44247b60) SHA1(519b9d6eab4fe05676382c5f99ea87d4f7a12c5e), \
               "tm_u6_s.l2",CRC(f366bbe5) SHA1(aca23649a54521748e90aa9a182b9bbdde126409), \
               "tm_u7_s.l2",CRC(f98e9e38) SHA1(bf8c204cfbbf5f9d59b7ad03d1784d37c638712c))

/*-----------------
/  ROM definitions  (Take them from Games.c)
/------------------*/
WPC_ROMSTART(tom,14h,"1_40h.u6",  0x80000,CRC(4181db9b) SHA1(027ada8518207d5a841ec3cc8c7842c7b3841f70)) TOM_SOUND WPC_ROMEND
WPC_ROMSTART(tom,13,"tom1_3x.rom",0x80000,CRC(aff4d14c) SHA1(9896f3034bb7a59c9e241d16bf231fefc0ae1fd0)) TOM_SOUND WPC_ROMEND
WPC_ROMSTART(tom,12,"tom1_2x.rom",0x80000,CRC(bd8dd884) SHA1(2cb74ae5082d8ceaf89b8ef4df86f78cb5ba6463)) TOM_SOUND WPC_ROMEND
WPC_ROMSTART(tom,06,"u6-06a.rom", 0x80000,CRC(dc1d6681) SHA1(7e60e9fd6e953e3c2899ae2fb2900982f078a4ba)) TOM_SOUND WPC_ROMEND

/*--------------
/  Game drivers    (Found in Games.c too)
/---------------*/
CORE_GAMEDEF(tom,13,"Theatre Of Magic (1.3X)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(tom,14h,13,"Theatre Of Magic (1.4H)",2005,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(tom,12,13,"Theatre Of Magic (1.2X)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(tom,06,13,"Theatre Of Magic (0.6a)",1995,"Bally",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData tomSimData = {
  2,    				/* 2 game specific input ports */
  tom_stateDef,			/* Definition of all states */
  tom_inportData,		/* Keyboard Entries */
  { stCapturedRest, stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain },
  tom_initSim,
  tom_handleBallState,	/* Function to handle ball state changes */
  tom_drawStatic,		/* Function to handle mechanical state changes */
  TRUE, 				/* Simulate manual shooter */
  NULL  				/* Custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/

static core_tGameData tomGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    NULL, tom_handleMech, tom_getMech, tom_drawMech,
    &tom_lampPos, tom_SamSolMap
  },
  &tomSimData,
  {
    "539 123456 12345 124",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_tom(void) {
  core_gameData = &tomGameData;
}

static void tom_handleMech(int mech) {
  if (mech & 0x01) {
    if (core_getSol(sBoxClockwise) && (locals.trunkPos < (360-TOM_TRUNKTICKS)))
      locals.trunkPos += TOM_TRUNKTICKS;
    else if (core_getSol(sBoxCounterClockwise) && (locals.trunkPos >= TOM_TRUNKTICKS))
      locals.trunkPos -= TOM_TRUNKTICKS;

    core_setSw(swCubePos1, !((locals.trunkPos>=  0) && (locals.trunkPos<(  0+TOM_SWCLOSEDDEGREE))));
    core_setSw(swCubePos4, !((locals.trunkPos>= 90) && (locals.trunkPos<( 90+TOM_SWCLOSEDDEGREE))));
    core_setSw(swCubePos3, !((locals.trunkPos>=180) && (locals.trunkPos<(180+TOM_SWCLOSEDDEGREE))));
    core_setSw(swCubePos2, !((locals.trunkPos>=270) && (locals.trunkPos<(270+TOM_SWCLOSEDDEGREE))));
  }

  if (mech & 0x02) {
    if (core_getSol(sCubeMagnet))
      locals.magnetTicks = TOM_MAGNETTICKSINIT;
    else if (locals.magnetTicks)
      locals.magnetTicks--;
  }
}
static int tom_getMech(int mechNo) {
  return mechNo ? locals.magnetTicks : locals.trunkPos;
}
