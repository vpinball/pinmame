/*******************************************************************************
 Bram Stoker's Dracula (Williams, 1993) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 29, 2000 (Updated Dec. 18, 2000)

 Known Issues/Problems with this Simulator:

 None.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Bram Stoker's Dracula Simulator:
  -----------------------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
     Q  SDTM (Drain Ball)
     T  Top Left Hole
   WER  Top Rollovers (L-M-R)
   YUI  Jet Bumpers (L-R-B)
   ASD  Left Bank Targets (B-M-T)
   FGH  Middle Bank Targets (L-M-R)
     J  Bottom Right Hole (Rats)
     L  Left Loop
     M  Mistery Hole
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
static int  drac_handleBallState(sim_tBallStatus *ball, int *inports);
static void drac_handleMech(int mech);
static void drac_drawMech(BMTYPE **line);
static void drac_drawStatic(BMTYPE **line);
static void init_drac(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int coffinrampPos;    /* Coffin Ramp Position */
  int magnetPos;	/* Magnet Position */
  int diverterTime;	/* Diverter's Firing Time */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(drac,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"TopL Rollover",		KEYCODE_W)
    COREPORT_BIT(0x0100,"TopM Rollover",		KEYCODE_E)
    COREPORT_BIT(0x0200,"TopR Rollover",		KEYCODE_R)
    COREPORT_BIT(0x0400,"TopL Popper",		KEYCODE_T)
    COREPORT_BIT(0x0800,"Left Jet",		KEYCODE_Y)
    COREPORT_BIT(0x1000,"Right Jet",		KEYCODE_U)
    COREPORT_BIT(0x2000,"Bottom Jet",		KEYCODE_I)
    COREPORT_BIT(0x4000,"BR Hole",		KEYCODE_J)
    COREPORT_BIT(0x8000,"LeftLoop",		KEYCODE_L)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"LBank B",		KEYCODE_A)
    COREPORT_BIT(0x0002,"LBank M",		KEYCODE_S)
    COREPORT_BIT(0x0004,"LBank T",		KEYCODE_D)
    COREPORT_BIT(0x0008,"MBank L",		KEYCODE_F)
    COREPORT_BIT(0x0010,"MBank M",		KEYCODE_G)
    COREPORT_BIT(0x0020,"MBank R",		KEYCODE_H)
    COREPORT_BIT(0x0040,"Mistery",		KEYCODE_M)
    COREPORT_BIT(0x0080,"ExitMagnet",		KEYCODE_ENTER)
    COREPORT_BIT(0x0100,"Drain",			KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swLDrop		15
#define swLDropScore	16
#define swShooter	17

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swTopLLane	25
#define swTopMLane	26
#define swTopRLane	27
#define swRRampMade	28

#define swUnderShootR	31
#define swLaunch	34
#define swLeftOutlane	35
#define swLeftInlane	36
#define swRightInlane	37
#define swRightOutlane	38

#define swTrough1	41
#define swTrough2	42
#define swTrough3	43
#define swTrough4     	44
#define swOutHole	48

/* 51-58 are Optos */

#define swTRLane	51
#define swMagLPocket	52
#define swCastle1	53
#define swCastle2	54
#define	swBLPopper	55
#define	swTLPopper	56
#define	swCastle3	57
#define	swMistery	58

#define	swLeftJet	61
#define	swRightJet	62
#define	swBottomJet	63
#define	swLeftSling	64
#define	swRightSling	65
#define	swLBankT	66
#define	swLBankM	67
#define	swLBankB	68

/* 71-73 are Optos */

#define	swCastlePop	71
#define	swCoffinPop	72
#define	swEnterLRamp	73
#define	swRRampUp	77

#define	swMagnetLeft	81
#define	swBallOnMagnet	82
#define	swMagnetRight	83
#define	swLRampMade	84
#define	swLRampDivMade	85
#define	swMBankL	86
#define	swMBankM	87
#define	swMBankR	88

#define swExitMagnet	74 /* Fake Switch */
#define swDiverter	78 /* Fake Switch */

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sShooter	1
#define sCoffinPopper	2
#define sCastlePopper	3
#define sRRampDown	4
#define sTLPopper    	5
#define sBLPopper    	6
#define sKnocker       	7
#define sShooterRamp   	8
#define sLeftSling	9
#define sRightSling	10
#define sLeftJet	11
#define sRightJet	12
#define sBottomJet	13
#define sRRampUp	14
#define sOutHole	15
#define sTrough		16
#define sLDrop		25
#define sMagnet		27
#define sMagnetMotor	28
#define sTopDiverter	33
#define sRGate		34
#define sCastleRelease	35
#define sLGate		36


/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stOutHole, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet, stLJet, stRJet, stBJet,
      stUnderShootRamp, stRGate, stMagnet, stLGate,
      stLeftLoop, stLDrop, stLDrop2, stLDropScore, stTLPopper, stTopLLane, stTopMLane, stTopRLane, stTMLane,
      stBRHole, stLBankB, stLBankM, stLBankT, stMBankL, stMBankM, stMBankR, stMistery, stBLPopper,
      stRRamp, stRRampMade, stInRRamp, stCoffinPopper, stOutFromHole,
      stEnterLRamp, stLRamp, stLRampMade, stInLRamp, stLRampDivMade, stCastle3, stCastle2, stCastle1
	  };

static sim_tState drac_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 1",		1,swTrough1,	 sTrough,	stShooter,	5},
  {"Trough 2",		1,swTrough2,	 0,		stTrough1,	1},
  {"Trough 3",		1,swTrough3,	 0,		stTrough2,	1},
  {"Trough 4",		1,swTrough4,	 0,		stTrough3,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stTrough4,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooter,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		3,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		0},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		0},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		0},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		3},

  /*Line 3*/
  {"Under Shoot R.",	1,swUnderShootR, 0,		stRGate,	5},
  {"Right Gate",	1,swBallOnMagnet,sRGate,	stMagnet,	0},
  {"Mist Magnet",	1,0,		 0,		0,		0},
  {"Left Gate",		1,swMagLPocket,	 sLGate,	stMagnet,	0},

  /*Line 4*/
  {"Left Loop",		1,swTRLane,	 0,		stLDrop,	5},
  {"Drop Target",	0,0,		 0,		0,		0},
  {"Drop Target",	1,swLDrop,	 0,		stTMLane,	5,0,0,SIM_STSWKEEP},
  {"Pass Drop Tgt",	1,swLDropScore,	 0,		stTLPopper,	5,0,0,SIM_STSWKEEP},
  {"Top Left Hole",	1,swTLPopper,	 sTLPopper,	stFree,		0},
  {"Top Left Lane",	1,swTopLLane,	 0,		stFree,		2},
  {"Top Mid Lane",	1,swTopMLane,	 0,		stFree,		2},
  {"Top Right Lane",	1,swTopRLane,	 0,		stFree,		2},
  {"Top Mid Lane",	1,swTopMLane,	 0,		stLJet,		7},

  /*Line 5*/
  {"Bottom R. Hole",	1,swCastlePop,	 sCastlePopper, stFree,		0},
  {"L. Bottom Tgt.",	1,swLBankB,	 0,		stFree,		2},
  {"L. Middle Tgt.",	1,swLBankM,	 0,		stFree,		2},
  {"Left Top Tgt.",	1,swLBankT,	 0,		stFree,		2},
  {"M. Left Tgt.",	1,swMBankL,	 0,		stFree,		2},
  {"M. Middle Tgt.",	1,swMBankM,	 0,		stFree,		2},
  {"M. Right Tgt.",	1,swMBankR,	 0,		stFree,		2},
  {"Mistery Hole",	1,swMistery,	 0,		stBLPopper,	15},
  {"Bot. Left Hole",	1,swBLPopper,	 sBLPopper,	stLeftInlane,	7},

  /*Line 6*/
  {"Enter R. Ramp",	1,0,		 0,		0,		0},
  {"R. Ramp Made",	1,swRRampMade,	 0,		stInRRamp,	3},
  {"Habitrail",		1,0,		 0,		stLeftInlane,	10},
  {"Coffin Hole",	1,swCoffinPop,	 sCoffinPopper, stOutFromHole,	0},
  {"Out From Hole",	1,0,		 0,		stLJet,		10},

  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		stLRamp,	5},
  {"Enter Left Rmp",	1,0,		 0,		0,		0},
  {"L. Ramp Made",	1,swLRampMade,	 0,		stInLRamp,	3},
  {"In Ramp",		1,0,		 0,		stRightInlane,  8},
  {"L. Ramp Div.",	1,swLRampDivMade,0,		stCastle3,	8},
  {"Castle Lock 3",	1,swCastle3,	 0,		stCastle2,	1},
  {"Castle Lock 2",	1,swCastle2,	 0,		stCastle1,	1},
  {"Castle Lock 1",	1,swCastle1,	 sCastleRelease,stShooter,	8},
  {0}
};

static int drac_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

		/*-----------------
		    Coffin Ramp
		-------------------*/
		case stRRamp:
		/* If the ramp is Down, go up the Coffin Ramp
		else fall down into the Coffin Hole */
		if (core_getSw(swRRampUp))
			return setState(stCoffinPopper,5);
		else
			return setState(stRRampMade,20);
		break;

		/*-----------------
		    Castle Ramp
		-------------------*/
		case stLRamp:
		/* If the ramp diverter is off, go through the Left Ramp
		otherwise send the ball into Castle Locks */
		if (core_getSw(swDiverter))
			return setState(stLRampDivMade,5);
		else
			return setState(stLRampMade,5);
		break;

		/*-----------------
		    Shooter Ramp
		-------------------*/
		case stBallLane:
		/* If the Shooter ramp is down, go to the Left Loop
		else go to the Right Gate */
		if (core_getSol(sShooterRamp))
			return setState(stUnderShootRamp,5);
		else
			return setState(stLeftLoop,5);
		break;

		/*-----------------
		    Drop Target
		-------------------*/
		case stLDrop:
		/* If the Drop Target is down, pass through it into VM hole,
		else lower it */
		if (core_getSw(swLDrop))
			return setState(stLDropScore,5);
		else
			return setState(stLDrop2,5);
		break;

		/*-------------
		   Mist Magnet
		 --------------*/
		case stMagnet:
		/* The BallOnMagnet switch must be always 1, except when there
		is no ball on the left/right gates */
			core_setSw(swBallOnMagnet,1);

		/* If the Magnet is on the right AND active AND the motor
		position counts to 0, go to the Left Gate */
		if (core_getSw(swMagnetRight) && core_getSol(sMagnet) && locals.magnetPos == 0)
			return setState(stLGate,0);

		/* If the Magnet is on the left AND active AND the motor
		position counts to 500, go to the Right Gate */
		if (core_getSw(swMagnetLeft)  && core_getSol(sMagnet) && locals.magnetPos == 500)
			return setState(stRGate,0);

		/* If the Magnet is off OR the fake switch activated by enter
		is on, get out of the magnet's area */
		if (!core_getSol(sMagnet) || core_getSw(swExitMagnet))
		{
			core_setSw(swBallOnMagnet,0);
			return setState(stFree,0);
		}
		break;
	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData drac_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterLRamp},
  {0, 0x0006, stRRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stTopLLane},
  {0, 0x0100, stTopMLane},
  {0, 0x0200, stTopRLane},
  {0, 0x0400, stTLPopper},
  {0, 0x0800, stLeftJet},
  {0, 0x1000, stBottomJet},
  {0, 0x2000, stRightJet},
  {0, 0x4000, stBRHole},
  {0, 0x8000, stLeftLoop},

/* Port 1 */
  {1, 0x0001, stLBankB},
  {1, 0x0002, stLBankM},
  {1, 0x0004, stLBankT},
  {1, 0x0008, stMBankL},
  {1, 0x0010, stMBankM},
  {1, 0x0020, stMBankR},
  {1, 0x0040, stMistery},
  {1, 0x0080, swExitMagnet, SIM_STANY},
  {1, 0x0100, stDrain},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay drac_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{ 0, 0,BLACK}}},{1,{{ 0, 1,BLACK}}},{1,{{ 0, 2,BLACK}}},{1,{{ 0, 3,BLACK}}},
{1,{{ 0, 4,BLACK}}},{1,{{ 5,23,YELLOW}}},{1,{{20,20,WHITE}}},{1,{{11,23,ORANGE}}},
{1,{{ 3,22,GREEN}}},{1,{{ 3,24,GREEN}}},{1,{{26,20,ORANGE}}},{1,{{13,21,YELLOW}}},
{1,{{11,19,ORANGE}}},{1,{{11,21,RED}}},{1,{{ 7,22,RED}}},{1,{{ 9,21,YELLOW}}},
{1,{{ 1,29,ORANGE}}},{1,{{ 3,28,ORANGE}}},{1,{{ 5,27,ORANGE}}},{1,{{ 7,26,ORANGE}}},
{1,{{ 9,25,ORANGE}}},{1,{{25,10,ORANGE}}},{1,{{28, 1,RED}}},{1,{{28, 4,YELLOW}}},
{1,{{28,24,YELLOW}}},{1,{{28,27,ORANGE}}},{1,{{32,11,LPURPLE}}},{1,{{34,13,GREEN}}},
{1,{{32,14,LPURPLE}}},{1,{{34,15,GREEN}}},{1,{{32,17,LPURPLE}}},{1,{{26, 8,ORANGE}}},
{1,{{ 0,22,RED}}},{1,{{ 0,24,RED}}},{1,{{19,14,WHITE}}},{1,{{36,14,ORANGE}}},
{1,{{38,14,YELLOW}}},{1,{{ 1,23,RED}}},{1,{{13, 4,RED}}},{1,{{ 1,19,WHITE}}},
{1,{{ 0, 9,ORANGE}}},{1,{{ 0,12,ORANGE}}},{1,{{ 0,15,ORANGE}}},{1,{{ 2, 9,RED}}},
{1,{{ 2,12,RED}}},{1,{{ 2,15,RED}}},{1,{{24,16,ORANGE}}},{1,{{ 7,15,WHITE}}},
{1,{{23,14,ORANGE}}},{1,{{25,18,ORANGE}}},{1,{{16, 6,RED}}},{1,{{18, 6,RED}}},
{1,{{20, 6,RED}}},{1,{{13,13,RED}}},{1,{{13,15,RED}}},{1,{{13,17,RED}}},
{1,{{21,24,WHITE}}},{1,{{24,12,ORANGE}}},{1,{{ 4, 5,WHITE}}},{1,{{ 7, 6,YELLOW}}},
{1,{{10, 7,ORANGE}}},{1,{{13, 8,LBLUE}}},{1,{{39,29,RED}}},{1,{{39, 0,YELLOW}}}
}
};

  static void drac_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Shooter Ramp : %-6s", core_getSol(sShooterRamp) ? "Up":"Down");
  core_textOutf(30, 20,BLACK,"Coffin Ramp  : %-6s", core_getSw(swRRampUp) ? "Up":"Down");
  core_textOutf(30, 30,BLACK,"Drop Target  : %-6s", core_getSw(swLDrop) ? "Down":"Up");
//  core_textOutf(30, 40,BLACK,"Ramp Diverter: %-6s", core_getSw(swDiverter) ? "On":"Off");
}
/* Help */
  static void drac_drawStatic(BMTYPE **line) {

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, Y/U/I = Jet Bumpers");
  core_textOutf(30,110,BLACK,"W/E/R = Multiplier Rollovers (L-M-R)");
  core_textOutf(30,120,BLACK,"A/S/D = Left Bank Targets (B-M-T)");
  core_textOutf(30,130,BLACK,"F/G/H = Middle Bank Targets (L-M-R)");
  core_textOutf(30,140,BLACK,"J = Bottom Right Hole, L = Left Loop");
  core_textOutf(30,150,BLACK,"T = Top Left Hole, M = Mistery Hole");
  core_textOutf(30,160,BLACK,"ENTER = Drop Ball from Mist Magnet");

}

/* Solenoid-to-sample handling */
static wpc_tSamSolMap drac_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE}, {sShooterRamp,0,SAM_FLAPOPEN},
 {sShooterRamp,0,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3},  {sTLPopper,1,SAM_POPPER},
 {sBLPopper,1,SAM_POPPER},
/*
F*CKING diverter... it flicks so the sample gets played a lot of times!
 {sTopDiverter,1,SAM_SOLENOID_ON}, {sTopDiverter,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
*/

 /*Channel #2*/
 {sShooter,2,SAM_SOLENOID},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},  {sCoffinPopper,2,SAM_POPPER},

 /*Channel #3*/
 {sCastlePopper,3,SAM_POPPER}, {sRRampUp,3,SAM_FLAPOPEN},
 {sRRampDown,3,SAM_FLAPCLOSE}, {sLDrop,3,SAM_SOLENOID_ON},
 {sCastleRelease,3,SAM_SOLENOID_ON}, {sCastleRelease,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(drac,l1,"dracu_l1.rom",0x80000,CRC(b8679686) SHA1(fdd25368f6134523ff3d68df9399a99784a00b7d))
WPCS_SOUNDROM888("dracsnd.u18",CRC(372ffb90) SHA1(89979670869c565d3ab86abbce462e2f935a566b),
                 "dracsnd.u15",CRC(77b5abe2) SHA1(e5622ae9ae0c1a0be886a1e5dc25b5a42c00c2ae),
                 "dracsnd.u14",CRC(5137aaf5) SHA1(e6ee924e7e4718db0f7f315f2a6843e6f90afb41))
WPC_ROMEND

WPC_ROMSTART(drac,p11,"u6-p11.rom",0x80000,CRC(6eb2fc06) SHA1(62591adf5a3c3c9016462961e8f3d7c6f5125e45))
WPCS_SOUNDROM888("dracsnd.u18",CRC(372ffb90) SHA1(89979670869c565d3ab86abbce462e2f935a566b),
                 "dracsnd.u15",CRC(77b5abe2) SHA1(e5622ae9ae0c1a0be886a1e5dc25b5a42c00c2ae),
                 "dracsnd.u14",CRC(5137aaf5) SHA1(e6ee924e7e4718db0f7f315f2a6843e6f90afb41))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(drac,l1,"Bram Stoker's Dracula (L-1)",1993,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(drac,p11,l1,"Bram Stoker's Dracula (P-11)",1993,"Williams",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData dracSimData = {
  2,    				/* 2 game specific input ports */
  drac_stateDef,			/* Definition of all states */
  drac_inportData,			/* Keyboard Entries */
  { stTrough2, stTrough3, stTrough4, stTrough1, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  drac_handleBallState,			/*Function to handle ball state changes*/
  drac_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData dracGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    NULL, drac_handleMech, NULL, drac_drawMech,
    &drac_lampPos, drac_samsolmap
  },
  &dracSimData,
  {
    {0}, /* sNO */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x07, 0x02, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_drac(void) {
  core_gameData = &dracGameData;
}

static void drac_handleMech(int mech) {
  /* ---------------------------------
     --	Raise/Lower the Coffin Ramp --
     --------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is up and the RampDown Solenoid is firing, lower it --*/
    if (core_getSol(sRRampDown))
      locals.coffinrampPos = 0 ;
    /*-- if Ramp is down and the RampUp Solenoid is firing, raise it --*/
    if (core_getSol(sRRampUp))
      locals.coffinrampPos = 1 ;
    /*-- Make sure RampUp Switch is on, when the ramp is up --*/
    core_setSw(swRRampUp,locals.coffinrampPos);
  }
  /* ---------------------------
     --	Raise the Drop Target --
     --------------------------- */
  if ((mech & 0x02) && core_getSol(sLDrop))
    core_setSw(swLDrop,0);

  /* -----------------
     --	Mist Magnet --
     ----------------- */
  if (mech & 0x04) {
    /* Set switches according to position */
    if (locals.magnetPos == 0) {
      core_setSw(swMagnetLeft,1);
      core_setSw(swMagnetRight,0);
    }
    if (locals.magnetPos == 500) {
      core_setSw(swMagnetRight,1);
      core_setSw(swMagnetLeft,0);
    }

    /* If the magnet is on the left and the motor is on, increase position */
    if (core_getSw(swMagnetLeft) && core_getSol(sMagnetMotor))
      locals.magnetPos++;

    /* If the magnet is on the right and the motor is on, decrease position */
    if (core_getSw(swMagnetRight) && core_getSol(sMagnetMotor))
      locals.magnetPos--;
  }
  /* ----------------------------------------------------------------------
     --	Ramp Diverter (Needs to be tricked because it doesn't like to be --
     -- pressed for the necessary time needed to complete the ramp shot) --
     ---------------------------------------------------------------------- */
  if (mech & 0x08) {
    /* If the diverter fires, increase diverter's time, when it is < 20 set a
       fake diverter switch to 1 */
    if (core_getSol(sTopDiverter)) {
      locals.diverterTime++;
      if (locals.diverterTime < 20)
        core_setSw(swDiverter,1);
    }

    /* If any of these switches are pressed, reset the diverter's timer and set
       the fake diverter switch to 0 */
    if (core_getSw(swLRampDivMade) || core_getSw(swCastle3) ||
        core_getSw(swCastle2) || core_getSw(swCastle3) ||
        core_getSw(swShooter)) {
      locals.diverterTime = 0;
      core_setSw(swDiverter,0);
    }
  }
}


