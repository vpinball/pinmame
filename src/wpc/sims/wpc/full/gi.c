/*******************************************************************************
 Gilligan's Island (Bally, 1991) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 18, 2000 (Updated Dec. 5, 2000)

 Known Issues/Problems with this Simulator:

 If for some reason during attract mode the Jungle doesn't stop at position 1,
 press F3.
 Possible reasons:
 1) It is spinning and you make a Slam Tilt.
 2) You go to the "Motor Test" menu, leave it at other position than 5 and exit
 to attract mode.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Gilligan's Island's Simulator
  --------------------------------------------------
    +R  Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
      Q SDTM (Drain Ball)
    WER Jet Bumpers
      T Top Hole (Shells)
      Y Top Kickout Hole (Skill Shot)
      U Rope Loop
      I Get Multiplier
    ASD Pineapple Targets (1-3)
    FGH Coconuts Targets (1-3)
      J Shrunken Head Target
      K Bananas Target
      L Turtle Eggs Loop
      M Center Ball Popper (Formula)
    Z-N LAGOON Targets
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  gi_handleBallState(sim_tBallStatus *ball, int *inports);
static void gi_handleMech(int mech);
static void gi_drawMech(BMTYPE **line);
static void gi_drawStatic(BMTYPE **line);
static void init_gi(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int junglerampPos;    /* Jungle Ramp Position */
  int jungleTime;	/* Jungle Position Time */
  int junglePos;	/* Jungle Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(gi,2)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"Ramp",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_E)
    COREPORT_BIT(0x0200,"Right Jet",		KEYCODE_R)
    COREPORT_BIT(0x0400,"Top Hole",		KEYCODE_T)
    COREPORT_BIT(0x0800,"Top Kickout Hole",	KEYCODE_Y)
    COREPORT_BIT(0x1000,"Rope Loop",		KEYCODE_U)
    COREPORT_BIT(0x2000,"Pineapple Target 1",	KEYCODE_A)
    COREPORT_BIT(0x4000,"Pineapple Target 2",	KEYCODE_S)
    COREPORT_BIT(0x8000,"Pineapple Target 3",	KEYCODE_D)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Coconuts Target 1",	KEYCODE_F)
    COREPORT_BIT(0x0002,"Coconuts Target 2",	KEYCODE_G)
    COREPORT_BIT(0x0004,"Coconuts Target 3",	KEYCODE_H)
    COREPORT_BIT(0x0008,"Shrunken Head Target",	KEYCODE_J)
    COREPORT_BIT(0x0010,"Bananas Target",	KEYCODE_K)
    COREPORT_BIT(0x0020,"Turtle Eggs Loop",	KEYCODE_L)
    COREPORT_BIT(0x0040,"Lagoon",		KEYCODE_Z)
    COREPORT_BIT(0x0100,"lAgoon",		KEYCODE_X)
    COREPORT_BIT(0x0200,"laGoon",		KEYCODE_C)
    COREPORT_BIT(0x0400,"lagOon",		KEYCODE_V)
    COREPORT_BIT(0x0800,"lagoOn",		KEYCODE_B)
    COREPORT_BIT(0x1000,"lagooN",		KEYCODE_N)
    COREPORT_BIT(0x2000,"Center Ball Popper",	KEYCODE_M)
    COREPORT_BIT(0x4000,"Drain",			KEYCODE_Q)
    COREPORT_BIT(0x8000,"Get Multiplier",	KEYCODE_I)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swLTrough	16
#define swRTrough	17
#define swOutHole	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swLeftOutlane	25
#define swLeftInlane	26
#define swRightInlane	27
#define swRightOutlane	28

#define swPMidLeft	31
#define swRt10pt	32
#define swLeftLock	33
#define swShrunken	35
#define swLPineapple	36
#define swMPineapple	37
#define swRPineapple	38

#define swLeftJet	41
#define swRightJet	42
#define swBottomJet	43
#define swLeftSling    	44
#define swRightSling   	45
#define swLCoconuts	46
#define swMCoconuts	47
#define swRCoconuts	48

#define swlagooN	51
#define swlagoOn	52
#define swlagOon	53
#define swlaGoon	54
#define	swlAgoon	55
#define	swLagoon	56
#define	swBananas	57
#define	swJet10pts	58

#define	swEnterIsland	61
#define	swRampStatus	62
#define	swLeftLoop	63
#define	swRightLoop	64
#define	swSTurn		65
#define	swBallPopper	66
#define	swTopEject	67
#define	swTopRight	68

#define	swPTopLeft	71
#define	swPTopRite	72
#define	swPBotRite	73
#define	swPBotLeft	74
#define	swLockLane	75
#define	swWheelLock	76
#define	swWheelOpto	77
#define	swShooter	78

#define	swTopLeftLoop	83
#define	swTopRightLoop	84

#define swFakeIslandPos 88	/* Fake Switch */

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLeftLock	1
#define sIslandLock	2
#define sOutHole	3
#define sBallPopper	4
#define sRightSling    	5
#define sLeftSling    	6
#define sKnocker       	7
#define sKickBack      	8
#define sTrough		10
#define sIslandLight	12
#define sLeftJet	13
#define sRightJet	14
#define sBottomJet	15
#define sTopEject	16
#define sRampUp		23
#define sRampDown	24

/* Thanks PSTM.c! :) */
#define sJungleMotor	9

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
      stRamp, stDownRamp, stMiniLoop, stUpRamp, stTopRampHole, stOutFromHole, stInRamp, stTopEject, stTopRightLane, stMultiplier, stMultiplier2,
      stLeftJet, stRightJet, stBottomJet, stJet10pts, stRt10pt, stDownRope, stLeftJet2, stRightJet2, stBottomJet2,
      stLeftLane, stLeftLock, stBallPopper, stOutFromHole2, stInRamp2,
      stRopeLoop, stRopeLoop2, stTurtleEggs, stTurtleEggs2,
      stLPineapple, stMPineapple, stRPineapple, stLCoconuts, stMCoconuts, stRCoconuts, stBananas, stShrunken,
      stLagoon, stlAgoon, stlaGoon, stlagOon, stlagoOn, stlagooN,
      stPTopRite, stPBotRite, stPTopLeft, stPBotLeft
	  };

static sim_tState gi_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",	1,swRTrough,	 sTrough,	stShooter,	5},
  {"Left Trough",	1,swLTrough,	 0,		stRTrough,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stLTrough,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	10,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",      1,swLeftOutlane, 0,             stDrain,        7, sKickBack, stBallPopper},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		5},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		5},

  /*Line 3*/
  {"Enter Ramp",	1,0,		 0,		0,		0},
  {"Going DownRamp",	1,swEnterIsland, 0,		stMiniLoop,	5},
  {"Upper Mini Loop",	1,swSTurn,	 0,		stTopEject,	12},
//  {"Up Jungle Ramp",	1,0,		 0,		stTopRampHole,	5},
  {"Up Jungle Ramp",	1,0,		 0,		0,		0},
  {"Top Mid Hole",	1,swPMidLeft,	 0,		stLeftLock,	3},
  {"Out From Hole",	1,0,		 0,		stInRamp,	5},
  {"In Ramp",		1,0,		 0,		stRightInlane,	15},
  {"Top Eject Hole",	1,swTopEject,	 sTopEject,	stLeftJet,	5},
  {"Top Right Lane",	1,swTopRight,	 0,		stLeftJet,	5},
  {"Going DownRamp",	1,swEnterIsland, 0,		stMultiplier2,	5},
  {"Upper Mini Loop",	1,swSTurn,	 0,		stTopRightLane,	12},

  /*Line 4*/
  {"Left Bumper",	1,swLeftJet,	 0,		stRightJet,	5},
  {"Right Bumper",	1,swRightJet,	 0,		stBottomJet,	5},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stJet10pts,	0},
  {"Bottom Bumper",	1,swJet10pts,	 0,		stRt10pt,	0},
  {"Bottom Bumper",	1,swRt10pt,	 0,		stDownRope,	5},
  {"Down From Rope",	1,swRightLoop,	 0,		stFree,		10},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 5*/
  {"Center Lane",	1,swLockLane,	 0,		stLeftLock,	5},
  {"In Left Hole",	1,swLeftLock,	 sLeftLock,	stOutFromHole,	0},
  {"Center Hole",	1,swBallPopper,	 sBallPopper,   stOutFromHole2,	0},
  {"Out From Hole",	1,0,		 0,		stInRamp2,	5},
  {"Habitrail",		1,0,		 0,		stRightInlane,	10},

  /*Line 6*/
  {"Rope Loop",		1,swRightLoop,	 0,		stRopeLoop2,	2},
  {"Rope Loop",		1,swTopRightLoop,0,		stFree,		7},
  {"Turtle Eggs Lp",	1,swLeftLoop,	 0,		stTurtleEggs2,	2},
  {"Turtle Eggs Lp",	1,swTopLeftLoop, 0,		stFree,		7},

  /*Line 7*/
  {"Left Pineapple",	1,swLPineapple,	 0,		stFree,		1},
  {"Mid Pineapple",	1,swMPineapple,	 0,		stFree,		1},
  {"Rt Pineapple",	1,swRPineapple,	 0,		stFree,		1},
  {"Left Coconuts",	1,swLCoconuts,	 0,		stFree,		1},
  {"Mid Coconuts",	1,swMCoconuts,	 0,		stFree,		1},
  {"Right Coconuts",	1,swRCoconuts,	 0,		stFree,		1},
  {"Bananas",		1,swBananas,	 0,		stFree,		1},
  {"Shrunken Head",	1,swShrunken,	 0,		stFree,		1},

  /*Line 8*/
  {"Lagoon - L",	1,swLagoon,	 0,		stFree,		1},
  {"Lagoon - A",	1,swlAgoon,	 0,		stFree,		1},
  {"Lagoon - G",	1,swlaGoon,	 0,		stFree,		1},
  {"Lagoon - O",	1,swlagOon,	 0,		stFree,		1},
  {"Lagoon - O",	1,swlagoOn,	 0,		stFree,		1},
  {"Lagoon - N",	1,swlagooN,	 0,		stFree,		1},

  {"Top Right Lane",	1,swPTopRite,	 0,		stInRamp,	5},
  {"Bot Right Lane",	1,swPBotRite,	 0,		stRightInlane,	8},
  {"Top Left Hole",	1,swPTopLeft,	 0,		stLeftLock,	5},
  {"Bot Left Lane",	1,swPBotLeft,	 0,		stLeftInlane,	7},
  {0}
};

static int gi_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
		if (ball->speed < 40)
			return setState(stTopEject,40);		/*Ball fall into Top-KickOut*/
		if (ball->speed < 51)
			return setState(stTopRightLane,51);	/*Ball rolled down Right Top Lane*/

		/*-----------------
		   Island Ramp
		-------------------*/
		case stRamp:
		/* If the ramp is Down, go up the Jungle*/
		/* Otherwise go normally down-ramp */
		if (core_getSw(swRampStatus))
			return setState(stUpRamp,5);
		else
			return setState(stDownRamp,5);
		break;

		/*-----------------
		  Where in Jungle?
		-------------------*/
		case stUpRamp:
		if (locals.junglePos == 1)
			return setState(stTopRampHole,5);
		if (locals.junglePos == 2)
			return setState(stPBotRite,5);
		if (locals.junglePos == 3)
			return setState(stPTopRite,5);
		if (locals.junglePos == 4)
			return setState(stPTopLeft,5);
		if (locals.junglePos == 5)
			return setState(stPBotLeft,5);
	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData gi_inportData[] = {

/* Port 0 */
  {0, 0x0005, stRamp},
  {0, 0x0006, stRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet2},
  {0, 0x0100, stBottomJet2},
  {0, 0x0200, stRightJet2},
  {0, 0x0400, stLeftLane},
  {0, 0x0800, stTopEject},
  {0, 0x1000, stRopeLoop},
  {0, 0x2000, stLPineapple},
  {0, 0x4000, stMPineapple},
  {0, 0x8000, stRPineapple},

/* Port 1 */
  {1, 0x0001, stLCoconuts},
  {1, 0x0002, stMCoconuts},
  {1, 0x0004, stRCoconuts},
  {1, 0x0008, stShrunken},
  {1, 0x0010, stBananas},
  {1, 0x0020, stTurtleEggs},
  {1, 0x0040, stLagoon},
  {1, 0x0100, stlAgoon},
  {1, 0x0200, stlaGoon},
  {1, 0x0400, stlagOon},
  {1, 0x0800, stlagoOn},
  {1, 0x1000, stlagooN},
  {1, 0x2000, stBallPopper},
  {1, 0x4000, stDrain},
  {1, 0x8000, stMultiplier},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay gi_lampPos = {
{ 0, 0 }, /* top left */
{39, 28}, /* size */
{
{1,{{26, 3,LPURPLE}}},{1,{{28, 2,YELLOW}}},{1,{{30, 1,YELLOW}}},{1,{{32, 1,ORANGE}}},
{1,{{34, 1,ORANGE}}},{1,{{32, 3,WHITE}}},{1,{{32,24,WHITE}}},{1,{{32,27,RED}}},
{1,{{14,14,LBLUE}}},{1,{{12,14,LBLUE}}},{1,{{10,14,LBLUE}}},{1,{{ 8,14,LBLUE}}},
{1,{{ 6,14,LBLUE}}},{1,{{ 4,14,LBLUE}}},{1,{{ 0,13,WHITE}}},{1,{{ 2,14,WHITE}}},
{1,{{23, 4,YELLOW}}},{1,{{21, 4,YELLOW}}},{1,{{19, 4,YELLOW}}},{1,{{15,20,WHITE}}},
{1,{{17,21,WHITE}}},{1,{{19,22,WHITE}}},{1,{{14,17,RED}}},{1,{{16,16,RED}}},
{1,{{30,13,WHITE}}},{1,{{28,11,WHITE}}},{1,{{26,12,WHITE}}},{1,{{24,11,WHITE}}},
{1,{{25,14,WHITE}}},{1,{{27,14,WHITE}}},{1,{{26,17,WHITE}}},{1,{{38,14,WHITE}}},
{1,{{34, 9,GREEN}}},{1,{{32,11,GREEN}}},{1,{{33,14,GREEN}}},{1,{{32,17,GREEN}}},
{1,{{34,19,GREEN}}},{1,{{36,14,YELLOW}}},{1,{{28, 5,ORANGE}}},{1,{{16, 7,GREEN}}},
{1,{{10, 2,WHITE}}},{1,{{12, 3,WHITE}}},{1,{{14, 4,WHITE}}},{1,{{16, 5,WHITE}}},
{1,{{17,10,WHITE}}},{1,{{19,11,WHITE}}},{1,{{11, 7,YELLOW}}},{1,{{13, 8,YELLOW}}},
{1,{{ 2,19,LPURPLE}}},{1,{{ 1,21,WHITE}}},{1,{{ 1,23,RED}}},{1,{{ 2,25,YELLOW}}},
{1,{{ 0, 0,BLACK}}},{1,{{21,25,WHITE}}},{1,{{23,24,WHITE}}},{1,{{ 0, 1,BLACK}}},
{1,{{ 0, 2,BLACK}}},{1,{{ 0, 3,BLACK}}},{1,{{ 0, 4,BLACK}}},{1,{{ 0, 5,BLACK}}},
{1,{{ 0, 6,BLACK}}},{1,{{ 0, 7,BLACK}}},{1,{{ 0, 8,BLACK}}},{1,{{39, 0,YELLOW}}}
}
};

  static void gi_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Ramp Status    : %-4s", core_getSw(swRampStatus) ? "Down":"Up");
  core_textOutf(30, 20,BLACK,"Jungle Position: %d", locals.junglePos);

/* Help */
}
  static void gi_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+R = Ramp");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,100,BLACK,"T = Top Hole, Y = Top Kickout Hole");
  core_textOutf(30,110,BLACK,"U = Rope Loop, A/S/D = Pineapple Tgts");
  core_textOutf(30,120,BLACK,"F/G/H = Coconuts Targets");
  core_textOutf(30,130,BLACK,"J/K = Shrunken Head/Banana Targets");
  core_textOutf(30,140,BLACK,"L = Turtle Eggs Loop");
  core_textOutf(30,150,BLACK,"M = Center Hole, I = Get Multiplier");
  core_textOutf(30,160,BLACK,"Z-N = Lagoon Targets");

  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap gi_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sBallPopper,2,SAM_POPPER},

 /*Channel #2*/
 {sLeftLock,2,SAM_POPPER},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},  {sKickBack,2,SAM_SOLENOID},

 /*Channel #3*/
 {sRampUp,3,SAM_FLAPOPEN}, {sRampDown,3,SAM_FLAPCLOSE},
 {sTopEject,3,SAM_SOLENOID_ON},

 /*Channel #4*/
 {sJungleMotor,4,SAM_MOTOR_1,WPCSAM_F_CONT},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
#define GI_SOUND \
WPCS_SOUNDROM222("gi_u18.l2",CRC(ea53e196) SHA1(5dcf3f44d2d658f6a7b130fa9e48d3cd616b4300), \
                 "gi_u15.l2",CRC(f8241dc9) SHA1(118a65555b9fff6f94e5e8324ed97d6ddec3d82b), \
                 "gi_u14.l2",CRC(0e7a4140) SHA1(c6408794120b5e45a48b35c380333879e1f0be78))

WPC_ROMSTART(gi,l9,"gilli_l9.rom",0x40000,CRC(af07a757) SHA1(29c4f4ac2aed5b36e1d22490d656b1c4acba7f4c)) GI_SOUND WPC_ROMEND
WPC_ROMSTART(gi,l3,"gi_l3.u6",    0x40000,CRC(d4e26140) SHA1(c2a9f02217071768ec1ef9169d2922c0e1585bee)) GI_SOUND WPC_ROMEND
WPC_ROMSTART(gi,l4,"gi_l4.u6",    0x40000,CRC(2313986d) SHA1(6e0dd293b869ea986ac9cb65b020463a86d955d4)) GI_SOUND WPC_ROMEND
WPC_ROMSTART(gi,l6,"gi_l6.u6",    0x40000,CRC(7b73eef2) SHA1(fade23019600d84492d5a0fc6f4f5be52ec319be)) GI_SOUND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(gi,l9,"Gilligan's Island (L-9)",1991,"Bally", wpc_mDMDS,0)
CORE_CLONEDEF(gi,l3,l9,"Gilligan's Island (L-3)",1991,"Bally", wpc_mDMDS,0)
CORE_CLONEDEF(gi,l4,l9,"Gilligan's Island (L-4)",1991,"Bally", wpc_mDMDS,0)
CORE_CLONEDEF(gi,l6,l9,"Gilligan's Island (L-6)",1991,"Bally", wpc_mDMDS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData giSimData = {
  2,    				/* 2 game specific input ports */
  gi_stateDef,				/* Definition of all states */
  gi_inportData,			/* Keyboard Entries */
  { stRTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  gi_handleBallState,			/*Function to handle ball state changes*/
  gi_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData giGameData = {
  GEN_WPCDMD, wpc_dispDMD,
  {
    FLIP_SWNO(12,11),
    0,0,0,0,0,0,0,
    NULL, gi_handleMech, NULL, gi_drawMech,
    &gi_lampPos, gi_samsolmap
  },
  &giSimData,
  {
    {0}, /* sNO */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x70 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_gi(void) {
  core_gameData = &giGameData;
}

static void gi_handleMech(int mech) {
  /* --------------------------------------
     --	Get Up and Down the Jungle Ramp  --
     -------------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is Up and the RampDown Solenoid is firing, Lower it --*/
    if (core_getSol(sRampDown))
      locals.junglerampPos = 1 ;
    /*-- if Ramp is Down and the RampUp Solenoid is firing, raise it --*/
    if (core_getSol(sRampUp))
      locals.junglerampPos = 0 ;
    /*-- Make sure RampStatus Switch is off, when ramp is Up --*/
    core_setSw(swRampStatus,locals.junglerampPos);
  }

  /* Now here we handle the Jungle */
  if (mech & 0x02) {
    /* If the Jungle Motor is spinning, increase Jungle Time */
    if (core_getSol(sJungleMotor) && !core_getSol(sIslandLock)) {
      core_setSw(swFakeIslandPos,1);
      locals.jungleTime++;
    }

    /* Set switches according to position */
    if (locals.jungleTime >= 0 && locals.jungleTime <= 12) {
      core_setSw(swWheelOpto,0);
      core_setSw(swWheelLock,0);
      locals.junglePos = 5;
    }
    else if (locals.jungleTime >= 12 && locals.jungleTime <= 49) {
      core_setSw(swWheelOpto,1);
      core_setSw(swWheelLock,1);
    }

    /* Same that before from now on, but on different positions */
    if (locals.jungleTime >= 50 && locals.jungleTime <= 62) {
      core_setSw(swWheelOpto,0);
      core_setSw(swWheelLock,0);
      locals.junglePos = 1;
    }
    else if (locals.jungleTime >= 63 && locals.jungleTime <= 99) {
      core_setSw(swWheelOpto,1);
      core_setSw(swWheelLock,1);
    }

    if (locals.jungleTime >= 100 && locals.jungleTime <= 112) {
      core_setSw(swWheelOpto,0);
      core_setSw(swWheelLock,0);
      locals.junglePos = 2;
    }
    else if (locals.jungleTime >= 113 && locals.jungleTime <= 149) {
      core_setSw(swWheelOpto,1);
      core_setSw(swWheelLock,1);
    }

    if (locals.jungleTime >= 150 && locals.jungleTime <= 162) {
      core_setSw(swWheelOpto,0);
      core_setSw(swWheelLock,0);
      locals.junglePos = 3;
    }
    else if (locals.jungleTime >= 163 && locals.jungleTime <= 199) {
      core_setSw(swWheelOpto,1);
      core_setSw(swWheelLock,1);
    }

    if (locals.jungleTime >= 200 && locals.jungleTime <= 212) {
      core_setSw(swWheelOpto,0);
      core_setSw(swWheelLock,0);
      locals.junglePos = 4;
    }
    else if (locals.jungleTime >= 213 && locals.jungleTime <= 249) {
      core_setSw(swWheelOpto,1);
      core_setSw(swWheelLock,1);
    }

    if (locals.jungleTime == 250)
      locals.jungleTime = 0;

    /* If the emulation resets (F3), sync the island position */
    if (!core_getSw(swFakeIslandPos)) {
      locals.junglePos = 5;
      locals.jungleTime = 0;
    }
  }
}
