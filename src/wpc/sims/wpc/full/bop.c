/*******************************************************************************
 The Machine: Bride of Pinbot (Williams, 1991) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 23, 2000 (Updated Dec. 4, 2000)
 Jul. 18, 2001, added getMech() (Tom)

 Thanks a lot to The Doc for giving help with the head mechanism.

 Known Issues/Problems with this Simulator:

 If you're playing and you're in face position 3 for example, when you exit
 PINMAME the position is recorded into the NVRAM, so when you start PINMAME
 again, the face will not be set into the correct position, in this case press
 F3 to fix it to position 1.
 This is not due to a simulator bug, it's just that I can't retrieve the
 NVRAM's face position flag out from the simulator.

 The MiniPF exit is chosen by the Ball's speed, i.e. if you get an Skillshot
 of 50/75K, you'll exit by the Right (straight to the Shooter), if you get an
 Skillshot of 100K/200K, you'll land on LeftExit (to the Right Flipper) and for
 25K (And sometimes 200K) Skillshots the ball will fall into the top hole.
 So don't shoot the ball with maximum Strength! ;-)

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 022512 Added helmet lights (GV)
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Machine: Bride of Pinbot Simulator:
  ------------------------------------------------
    +R  Left Ramp (Fail/OK)
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
     Q  SDTM (Drain Ball)
     R  Right Loop
   TYU  Jet Bumpers (1-3)
     S  Small Wheel Hole
     J  Spinner
     L  Left Loop
     C  Center Ramp
   VBN  5K Multiplier Targets (1-3)
     M  Loop Bonus X (Right Loop Backwards)

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "machine/4094.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
 #include <strings.h>
 #define _stricmp strcasecmp
#endif

/*------------------
/  Local functions
/-------------------*/
static int  bop_handleBallState(sim_tBallStatus *ball, int *inports);
static void bop_handleMech(int mech);
static int bop_getMech(int mechNo);
static void bop_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls);
static void bop_drawMech(BMTYPE **line);
static void bop_drawStatic(BMTYPE **line);
static void init_bop(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int facePos;    /* Face Position */
  int headPos;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(bop,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0008,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0010,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0020,"Ramp Fail/OK",	        KEYCODE_R)
    COREPORT_BIT(0x0040,"RightLoop",		KEYCODE_R)
    COREPORT_BIT(0x0100,"Left Jet",		KEYCODE_T)
    COREPORT_BIT(0x0200,"Right Jet",		KEYCODE_Y)
    COREPORT_BIT(0x0400,"Bottom Jet",		KEYCODE_U)
    COREPORT_BIT(0x0800,"Small Wheel",		KEYCODE_S)
    COREPORT_BIT(0x1000,"Left Loop",		KEYCODE_L)
    COREPORT_BIT(0x2000,"Center Ramp",		KEYCODE_C)
    COREPORT_BIT(0x4000,"BonusX",		KEYCODE_X)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)


  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Right Top 5K",		KEYCODE_B)
    COREPORT_BIT(0x0002,"Right Bot 5K",		KEYCODE_N)
    COREPORT_BIT(0x0004,"Left 5K",		KEYCODE_V)
    COREPORT_BIT(0x0008,"Spinner",		KEYCODE_J)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swLeftOutlane	15
#define swLeftInlane	16
#define swRightInlane	17
#define swRightOutlane	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swRTrough	25
#define swCTrough	26
#define swLTrough	27
#define swLeftStandup	28

#define swSkill50K	31
#define swSkill75K	32
#define swSkill100K	33
#define swSkill200K	34
#define swSkill25K	35
#define swRTopStandup	36
#define swRBotStandup	37
#define swOutHole	38

#define swRRampMade	41
#define swLeftLoop	43
#define swRTopLoop	44
#define swRBotLoop     	45
#define swUPKicker    	46
#define swEnterHead	47

#define swSpinner	51
#define swShooter	52
#define swRightJet	53
#define swLeftJet	54
#define	swBottomJet	55
#define	swBumperSling	56
#define	swLeftSling	57
#define	swRightSling	58

#define	swHLeftEye	63
#define	swHRightEye	64
#define	swHMouth	65
#define	swFacePosition	67

#define	swWireformTop	71
#define	swWireformBot	72
#define	swEnterMiniPF	73
#define	swMiniPFLeft	74
#define	swMiniPFRight	75
#define	swEnterLRamp	76
#define	swEnterRRamp	77

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sOutHole	1
#define sTrough		2
#define sUPKicker	3
#define sControlledGate	4
#define sSkillShot	5
#define sWireBallHolder	6
#define sKnocker       	7
#define sHeadMouth	8
#define sLeftJet	9
#define sLeftSling	10
#define sRightJet	11
#define sRightSling	12
#define sBottomJet	13
#define sBumperSling	14
#define sHeadLeftEye	15
#define sHeadRightEye	16
#define sHeadDirection	27
#define sHeadMotor	28

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
      stSkill25K, stSkill200K, stSkill100K, stSkill75K, stSkill50K, stBallFired,
      stLJet, stRJet, stBJet, stJetSling, stSpinner2, stLeftJet, stBottomJet, stRightJet,
      stRampFail, stRampFail2,
      stLRamp, stPinbot, stMiniPFExit, stMiniPFLeft, stMiniPFRight, stEnterHead, stWireformTop, stWireformBot, stInReel, stDropHole,
      stRightLoop, stRightLoopMade, stRightLoopE, stMultiplier, stLeftLoop, stLeftLoopMade,
      stLeftPassage, stSmallWheel, stOutFromHole, stCenterRamp, stCRamp2,
      stRightTop5K, stRightBot5K, stLeft5K, stSpinner, stWireformTop2, stHMouth, stHLeftEye, stHRightEye
	  };

static sim_tState bop_stateDef[] = {
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
  {"SkillShot 25K",	1,swSkill25K,	 0,		stSkill200K,	2},
  {"SkillShot 200K",	1,swSkill200K,	 0,		stSkill100K,	2},
  {"SkillShot 100K",	1,swSkill100K,	 0,		stSkill75K,	2},
  {"SkillShot 75K",	1,swSkill75K,	 0,		stSkill50K,	2},
  {"SkillShot 50K",	1,swSkill50K,	 sSkillShot,	stBallFired,	0},
  {"Ball Fired",	1,0,	 	 0,		stLJet,		10},

  /*Line 4*/
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		2},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		2},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stJetSling,	2},
  {"Bumper Sling",	1,swBumperSling, 0,		stSpinner2,	3},
  {"Spinner",		1,swSpinner,	 0,		stFree,		5,0,0,SIM_STSPINNER},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 5*/
  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		stRampFail2,	17},
  {"Fall Down",		1,swEnterLRamp,	 0,		stFree,		5},

  /*Line 6*/
  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		0,		0},
  {"Pinbot MiniPF",	1,swEnterMiniPF, 0,		stMiniPFExit,	4},
  {"MiniPF Exit",	0,		 0,		0,		0},
  {"MiniPF-OutLeft",	1,swMiniPFLeft,	 0,		stFree,		15},
  {"MiniPF-OutRite",	1,swMiniPFRight, 0,		stShooter,	15},
  {"Enter Head",	1,swEnterHead,	 0,		stWireformTop,	4,0,0,SIM_STNOTEXCL},
  {"Wireform Top",	0,0,		 0,		0,		0},
  {"Wireform Bot",	1,swWireformBot, sWireBallHolder,stInReel,	0},
  {"Habitrail",		1,0,		 0,		stLeftInlane,	12},
  {"MiniPF Hole",	1,0,		 0,		stLJet,		7},

  /*Line 7*/
  {"Enter Rt Loop",	1,0,		 0,		stRightLoopMade,2},
  {"Rt Loop Made",	1,swRTopLoop,	 0,		stRightLoopE,	3},
  {"Exit Rt Loop",	1,swRBotLoop,	 0,		stFree,		5},
  {"Loop Bonus X",	1,swRBotLoop,	 0,		stFree,		6},
  {"Enter Lt Loop",	1,0,		 0,		stLeftLoopMade, 2},
  {"Lt. Loop Made",	1,swLeftLoop,	 0,		stFree,		4},

  /*Line 8*/
  {"Left Passage",	1,0,		 0,		stSmallWheel,	7},
  {"Small Wheel",	1,swUPKicker,	 sUPKicker,	stOutFromHole,	0},
  {"Out From Hole",	1,0,		 0,		stFree,		7},
  {"Enter C. Ramp",	1,swEnterRRamp,	 0,		stCRamp2,	4,0,0,SIM_STNOTEXCL},
  {"Center R. Made",	1,swRRampMade,	 0,		stFree,		13,0,0,SIM_STNOTEXCL},

  /*Line 9*/
  {"Rt. Top 5K Tgt",	1,swRTopStandup, 0,		stFree,		3},
  {"Rt. Bot 5K Tgt",	1,swRBotStandup, 0,		stFree,		3},
  {"Left 5K Target",	1,swLeftStandup, 0,		stFree,		3},
  {"Spinner",		1,swSpinner,	 0,		stFree,		5,0,0,SIM_STSPINNER},
  {"Wireform Top",	1,swWireformTop, 0,		stWireformBot,	1},
  {"Face Mouth",	1,swHMouth,	 sHeadMouth,	stWireformTop2, 2},
  {"Face Left Eye",	1,swHLeftEye,	 sHeadLeftEye,	stWireformTop2,	2},
  {"Face Right Eye",	1,swHRightEye,	 sHeadRightEye,	stWireformTop2,	2},

  {0}
};

static int bop_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 10)
			return setState(stNotEnough,10);	/*Ball not plunged hard enough*/
		if (ball->speed < 15)
			return setState(stSkill50K,15);		/*Skill Shot 50K*/
		if (ball->speed < 25)
			return setState(stSkill75K,25);		/*Skill Shot 75K*/
		if (ball->speed < 35)
			return setState(stSkill100K,35);	/*Skill Shot 100K*/
		if (ball->speed < 45)
			return setState(stSkill200K,45);	/*Skill Shot 200K*/
		if (ball->speed < 51)
			return setState(stSkill25K,51);		/*Skill Shot 25K*/
		break;

		/*-----------------
		     Left Ramp
		-------------------*/
		case stLRamp:
		/* If the gate is open, go to the Pinbot MiniPF */
		/* Otherwise Enter into the head */
		if (core_getSol(sControlledGate))
			return setState(stPinbot,50);
		else
			return setState(stEnterHead,75);
		break;

		/*--------------------------------
		  Head - Where the ball should go?
		----------------------------------*/
		case stWireformTop:
		if (locals.headPos == 1)
			return setState(stHMouth,5);
		if (locals.headPos == 2 && !core_getSw(swHLeftEye))
			return setState(stHLeftEye,5);
		if (locals.headPos == 2 && core_getSw(swHLeftEye))
			return setState(stHRightEye,5);
		if (locals.headPos == 3 || locals.headPos == 4)
			return setState(stWireformTop2,5);
		break;

		/*-----------------
		    MiniPF Exit
		-------------------*/
    	case stMiniPFExit:
		if (ball->speed < 25)
			return setState(stMiniPFRight,150);	/* Out MiniPF From Right */
		if (ball->speed < 35)
			return setState(stMiniPFLeft,150);	/* Out MiniPF From  Left */
		if (ball->speed < 51)
			return setState(stDropHole,150);	/* Out MiniPF From Hole */
		break;

	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

  static sim_tInportData bop_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLeftOutlane},
  {0, 0x0006, stRightOutlane},
  {0, 0x0009, stLeftSling},
  {0, 0x000a, stRightSling},
  {0, 0x0011, stLeftInlane},
  {0, 0x0012, stRightInlane},
  {0, 0x0021, stRampFail},
  {0, 0x0022, stLRamp},
  {0, 0x0040, stRightLoop},
  {0, 0x0100, stLeftJet},
  {0, 0x0200, stRightJet},
  {0, 0x0400, stBottomJet},
  {0, 0x0800, stLeftPassage},
  {0, 0x1000, stLeftLoop},
  {0, 0x2000, stCenterRamp},
  {0, 0x4000, stMultiplier},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stRightTop5K},
  {1, 0x0002, stRightBot5K},
  {1, 0x0004, stLeft5K},
  {1, 0x0008, stSpinner},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay bop_lampPos = {
{ 0, 0 }, /* top left */
{41, 31}, /* size */
{
{1,{{32, 0,ORANGE}}},{1,{{32, 3,ORANGE}}},{1,{{32,26,ORANGE}}},{1,{{32,29,ORANGE}}},
{1,{{29, 3,GREEN}}}, {1,{{26,23,GREEN}}}, {1,{{28,24,GREEN}}}, {1,{{40,14,ORANGE}}},
{1,{{27,14,RED}}},   {1,{{25,14,RED}}},   {1,{{33,18,ORANGE}}},{1,{{35,14,LBLUE}}},
{1,{{33,10,YELLOW}}},{1,{{29,10,WHITE}}}, {1,{{31,14,RED}}},   {1,{{21, 1,YELLOW}}},
{1,{{29,18,GREEN}}}, {1,{{27,20,GREEN}}}, {1,{{35,20,ORANGE}}},{1,{{37,14,LBLUE}}},
{1,{{35, 8,YELLOW}}},{1,{{27, 8,WHITE}}}, {1,{{21, 4,WHITE}}}, {1,{{23, 5,ORANGE}}},
{1,{{23,27,LBLUE}}}, {1,{{20,27,RED}}},   {1,{{17,27,ORANGE}}},{1,{{14,27,YELLOW}}},
{1,{{11,27,LBLUE}}}, {1,{{ 4, 9,RED}}},   {1,{{ 4,13,RED}}},   {1,{{ 7,11,RED}}},
{1,{{15, 9,RED}}},   {1,{{17, 9,ORANGE}}},{1,{{19, 9,YELLOW}}},{1,{{21, 9,WHITE}}},
{1,{{16,23,GREEN}}}, {1,{{18,22,ORANGE}}},{1,{{20,21,YELLOW}}},{1,{{22,20,WHITE}}},
{1,{{18,14,LBLUE}}}, {1,{{16,14,GREEN}}}, {1,{{14,14,YELLOW}}},{1,{{12,10,WHITE}}},
{1,{{16,20,RED}}},   {1,{{18,19,ORANGE}}},{1,{{20,18,YELLOW}}},{1,{{22,17,WHITE}}},
{1,{{ 2, 0,YELLOW}}},{1,{{ 2, 2,YELLOW}}},{1,{{ 2, 4,YELLOW}}},{1,{{ 2, 6,YELLOW}}},
{1,{{ 2, 8,YELLOW}}},{1,{{ 2,10,YELLOW}}},{1,{{ 2,12,YELLOW}}},{1,{{ 2,14,YELLOW}}},
{1,{{ 2,21,RED}}},   {1,{{ 2,23,RED}}},   {1,{{ 2,25,RED}}},   {1,{{ 2,27,RED}}},
{1,{{ 2,29,RED}}},   {1,{{ 4,22,ORANGE}}},{1,{{ 4,25,YELLOW}}},{1,{{ 4,28,WHITE}}},
// 16 helmet lights below (see test #15)
{1,{{ 0, 0,WHITE}}}, {1,{{ 0, 2,WHITE}}}, {1,{{ 0, 4,WHITE}}}, {1,{{ 0, 6,WHITE}}},
{1,{{ 0, 8,WHITE}}}, {1,{{ 0,10,WHITE}}}, {1,{{ 0,12,WHITE}}}, {1,{{ 0,14,WHITE}}},
{1,{{ 0,16,WHITE}}}, {1,{{ 0,18,WHITE}}}, {1,{{ 0,20,WHITE}}}, {1,{{ 0,22,WHITE}}},
{1,{{ 0,24,WHITE}}}, {1,{{ 0,26,WHITE}}}, {1,{{ 0,28,WHITE}}}, {1,{{ 0,30,WHITE}}}
}
};

  /* On-Screen stuff */
static void bop_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"Pinbot Gate  : %-6s", core_getSol(sControlledGate) ? "Open":"Closed");
  core_textOutf(30, 20,BLACK,"Face Position: %-3d", locals.headPos);
}
/* Help */
static void bop_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+-   = L/R Slingshot");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+R   = Left Ramp (Fail/OK)");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, R = Right Loop");
  core_textOutf(30,100,BLACK,"T/Y/U = Jet Bumpers");
  core_textOutf(30,110,BLACK,"S = Small Wheel Hole, J = Spinner");
  core_textOutf(30,120,BLACK,"L = Left Loop, C = Center Ramp");
  core_textOutf(30,130,BLACK,"X = Loop Bonus X (RLoop Backwards)");
  core_textOutf(30,140,BLACK,"V/B/N = 5K Multiplier Targets");

  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap bop_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sHeadMotor,1,SAM_MOTOR_1,WPCSAM_F_CONT},

 /*Channel #2*/
 {sWireBallHolder,2,SAM_SOLENOID_ON}, {sWireBallHolder,2,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sLeftSling,2,SAM_LSLING}, {sRightSling,2,SAM_RSLING},
 {sBumperSling,2,SAM_LSLING},

 /*Channel #3*/
 {sSkillShot,3,SAM_SOLENOID}, {sUPKicker,3,SAM_POPPER},
 {sControlledGate,3,SAM_SOLENOID_ON}, {sControlledGate,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {-1}
};

/*-----------------
/  ROM definitions
/------------------*/
#define BOP_SOUND \
WPCS_SOUNDROM222("mach_u18.l1",CRC(f3f53896) SHA1(4be5a8a27c5ac4718713c05ff2ddf51658a1be27), \
                 "mach_u15.l1", CRC(fb49513b) SHA1(01f5243ff258adce3a28b24859eba3f465444bdf), \
                 "mach_u14.l1", CRC(be2a736a) SHA1(ebf7b26a86d3ffcc35eaa1da8e4f432bd281fe15)) \

WPC_ROMSTART(bop,l7,"tmbopl_7.rom",0x40000,CRC(773e1488) SHA1(36e8957b3903b99844a76bf15ba393b17db0db59)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d7,"tmbopd_7.rom",0x40000,CRC(43e69417) SHA1(3881a04e409b18319ad33874d742a611cc783feb)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l8,"tmbopl_8.rom",0x40000,CRC(ffeb8991) SHA1(2d9af6274cd0f60c951456198492c0bc5ae433bf)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d8,"tmbopd_8.rom",0x40000,CRC(166473a1) SHA1(5fd24c4a579652eafac5274bb55e7502f1519371)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l6,"tmbopl_6.rom",0x20000,CRC(96b844d6) SHA1(981194c249a8fc2534e24ef672380d751a5dc5fd)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d6,"tmbopd_6.rom",0x20000,CRC(6d5d3df8) SHA1(37fe8bc12321d97e4f368392c8c29c539d8aaedf)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l5,"tmbopl_5.rom",0x20000,CRC(fd5c426d) SHA1(e006f8e39cf382249db0b969cf966fd8deaa344a)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d5,"tmbopd_5.rom",0x20000,CRC(cfe43a8c) SHA1(6f5c1ddfc4f5ec40851612653924c77abc8309ee)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l4,"tmbopl_4.rom",0x20000,CRC(eea14ecd) SHA1(afd670bdc3680f12360561a1a5e5854718c099f7)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d4,"tmbopd_4.rom",0x20000,CRC(20295f41) SHA1(20f98b15bb15b6f534b2ea6fa13cbadc4501ff1f)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l3,"bop_l3.u6",   0x20000,CRC(cd4d219d) SHA1(4e73dca186867ebee07682deab058a45cee53be1)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d3,"bop_d3.u6",   0x20000,CRC(8e00fcba) SHA1(b8f9cb7f9715e45d43f47654ef553868c856235a)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,l2,"bop_l2.u6",   0x20000,CRC(17ee1f56) SHA1(bee68ed5680455f23dc33e889acec83cba68b1dc)) BOP_SOUND WPC_ROMEND
WPC_ROMSTART(bop,d2,"bop_d2.u6",   0x20000,CRC(c362eb30) SHA1(93704d1634f170e10c7c6850e7f24be69aefd89e)) BOP_SOUND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(bop,l7,"Machine: Bride of Pinbot, The (L-7)",1992,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d7,l7,"Machine: Bride of Pinbot, The (D-7 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l8,l7,"Machine: Bride of Pinbot, The (L-8 billionaire multiplayer patch)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d8,l7,"Machine: Bride of Pinbot, The (D-8 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l6,l7,"Machine: Bride of Pinbot, The (L-6)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d6,l7,"Machine: Bride of Pinbot, The (D-6 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l5,l7,"Machine: Bride of Pinbot, The (L-5)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d5,l7,"Machine: Bride of Pinbot, The (D-5 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l4,l7,"Machine: Bride of Pinbot, The (L-4)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d4,l7,"Machine: Bride of Pinbot, The (D-4 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l3,l7,"Machine: Bride of Pinbot, The (L-3)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d3,l7,"Machine: Bride of Pinbot, The (D-3 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,l2,l7,"Machine: Bride of Pinbot, The (L-2)",1991,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(bop,d2,l7,"Machine: Bride of Pinbot, The (D-2 LED Ghost Fix)",1991,"Williams",wpc_mAlpha2S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData bopSimData = {
  2,    				/* 2 game specific input ports */
  bop_stateDef,				/* Definition of all states */
  bop_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  bop_initSim, 				/* Simulator Init */
  bop_handleBallState,			/*Function to handle ball state changes*/
  bop_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData bopGameData = {
  GEN_WPCALPHA_2, wpc_dispAlpha,
  {
    FLIP_SWNO(12,11),
    0, 2, // 2 extra lamp columns for the helmet lights
    0, 0, 0, 0, 0,
    NULL, bop_handleMech, bop_getMech, bop_drawMech,
    &bop_lampPos, bop_samsolmap
  },
  &bopSimData,
  {
    { 0 },
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*----------------
/  Init Simulator
/-----------------*/
static void bop_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
  /* The Face position must be 1 at start */
  locals.headPos = 1;
}

/*------------------
/  Handle Mechanics
/-------------------*/

extern int g_fHandleMechanics;

static void bop_handleMech(int mech) {
	// This is a bit of a hack, but seems to be the best way to keep this localized
	// to bop.c and keep things fully backwards compatible..   We need a way for a table 
	// to let VPinmame know that we want to reset the internal NVRAM head position value to 0, 
	// at table startup.
	//
	// We cannot do this in init_bop, NVRAM hasn't loaded yet. 
	// However we may NOT want to use the built in mech handling (the below code would have 
	// been simpler as an actual mech and doesn't relay the motion).   
	//
	// So if g_fHandleMechanics is -1, reset the head position to 0,
	// and then disable the internal mech handling.   If it is -2, then continue using the mech handling
	// after reset. 
	if (g_fHandleMechanics < 0)
	{
		if (_stricmp(Machine->gamedrv->name, "bop_l7") == 0)
		{
			// Reset Bride of Pinbot face to 0. 
			wpc_ram[0x1fc9] = 0x01;
		}
		if (g_fHandleMechanics == -1)
		{
			g_fHandleMechanics = 0;
			return;
		}
		g_fHandleMechanics = 1;
	}

  /* ----------------------------------------------
     --	Head Position - SH*T, this was a PAIN!!! --
     --  BTW: Thanks to The Doc for giving help  --
     ---------------------------------------------- */
  /* If the Face is at Position 1, the motor moves and the Direction driver is
     off, move the head to position 2 */
  if (mech & 0x01) {
    if (locals.headPos == 1 && core_getSol(sHeadMotor) && !core_getSol(sHeadDirection)) {
      locals.facePos++;
      if (locals.facePos < 100)
        core_setSw(swFacePosition,1);
      if (locals.facePos > 101 && locals.facePos < 149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == 150) {
	locals.headPos = 2;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 2, the motor moves and the Direction driver is
       off, move the head to position 3 */
    if (locals.headPos == 2 && core_getSol(sHeadMotor) && !core_getSol(sHeadDirection)) {
      locals.facePos++;
      if (locals.facePos < 100)
	core_setSw(swFacePosition,1);
      if (locals.facePos > 101 && locals.facePos < 149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == 150) {
	locals.headPos = 3;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 3, the motor moves and the Direction driver is
       off, move the head to position 4
       Notice that the FacePosition switch doesn't end pressed here */
    if (locals.headPos == 3 && core_getSol(sHeadMotor) && !core_getSol(sHeadDirection)) {
      locals.facePos++;
      if (locals.facePos < 125)
	core_setSw(swFacePosition,1);
      if (locals.facePos > 126 && locals.facePos < 149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == 150) {
	locals.headPos = 4;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 4, the motor moves and the Direction driver is
       off, move the head to position 1 and restart */
    if (locals.headPos == 4 && core_getSol(sHeadMotor) && !core_getSol(sHeadDirection)) {
      locals.facePos++;
      if (locals.facePos < 100)
	core_setSw(swFacePosition,0);
      if (locals.facePos > 101 && locals.facePos < 169)
	core_setSw(swFacePosition,1);
      if (locals.facePos == 170) {
	locals.headPos = 1;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 4, the motor moves and the Direction driver is
       on, move the head back to position 3 */
    if (locals.headPos == 4 && core_getSol(sHeadMotor) && core_getSol(sHeadDirection)) {
      locals.facePos--;
      if (locals.facePos > -100)
 	core_setSw(swFacePosition,1);
      if (locals.facePos < -101 && locals.facePos > -149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == -150) {
	locals.headPos = 3;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 3, the motor moves and the Direction driver is
       on, move the head back to position 2 */
    if (locals.headPos == 3 && core_getSol(sHeadMotor) && core_getSol(sHeadDirection)) {
      locals.facePos--;
      if (locals.facePos > -100)
  	core_setSw(swFacePosition,1);
      if (locals.facePos < -101 && locals.facePos > -149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == -150) {
	locals.headPos = 2;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 2, the motor moves and the Direction driver is
       on, move the head back to position 1 */
    if (locals.headPos == 2 && core_getSol(sHeadMotor) && core_getSol(sHeadDirection)) {
      locals.facePos--;
      if (locals.facePos > -100)
	core_setSw(swFacePosition,1);
      if (locals.facePos < -101 && locals.facePos > -149)
	core_setSw(swFacePosition,0);
      if (locals.facePos == -150) {
	locals.headPos = 1;
	locals.facePos = 0;
      }
    }

    /* If the Face is at Position 1, the motor moves and the Direction driver is
       on, move the head back to position 4
       Notice that the FacePosition switch doesn't end pressed here */
    if (locals.headPos == 1 && core_getSol(sHeadMotor) && core_getSol(sHeadDirection)) {
      locals.facePos--;
      if (locals.facePos > -100)
	core_setSw(swFacePosition,1);
      if (locals.facePos < -101 && locals.facePos > -169)
	core_setSw(swFacePosition,0);
      if (locals.facePos == -170) {
	locals.headPos = 4;
	locals.facePos = 0;
      }
    }
    /* Those were all the possible movements that the head can make */
  }
}

static int bop_getMech(int mechNo) {
  return locals.headPos;
}

static WRITE_HANDLER(parallel_0_out) {
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8] = data ^ 0xff;
}
static WRITE_HANDLER(parallel_1_out) {
  coreGlobals.lampMatrix[9] = coreGlobals.tmpLampMatrix[9] = data ^ 0xff;
}
static WRITE_HANDLER(qspin_0_out) {
  HC4094_data_w(1, data);
}

static HC4094interface hc4094bop = {
  2, // 2 chips
  { parallel_0_out, parallel_1_out },
  { qspin_0_out }
};

static WRITE_HANDLER(bop_wpc_w) {
  static UINT8 lastVal;
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {
    HC4094_strobe_w(0, lastVal == (data & 3));
    HC4094_strobe_w(1, lastVal == (data & 3));
    HC4094_data_w  (0, GET_BIT0);
    HC4094_clock_w (0, GET_BIT1);
    HC4094_clock_w (1, GET_BIT1);
    lastVal = data & 3;
  }
}

#ifdef PROC_SUPPORT
  #include "p-roc/p-roc.h"
  #include "p-roc/proc_shift_reg.h"

  /*
    LEDs of helmet controlled by a serial shift register.  Game sets DATA and
    then pulses clock HIGH for less than 1ms, about every 100ms.  Except for
    when it's clearing the LEDs or pulsing a specific pattern and shifts with
    sub-millisecond timing.  We use a 64-bit queue to track shift requests,
    and a 12ms timer to shift them out at a rate the P-ROC can handle.
  */
  void bop_wpc_proc_solenoid_handler(int solNum, int enabled, int smoothed) {
    static int helmet_data = 0;

    // coils to process immediately, and not smoothed
    switch (solNum) {
      case 16:  // C17 to C24 flashers
      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
      case 22:
      case 23:
      
      case 26:  // C27, head direction
      case 27:  // C28, head motor
        if (!smoothed)
          default_wpc_proc_solenoid_handler(solNum, enabled, TRUE);
        return;

      case 24:  // C25 data for helmet LEDs
        if (!smoothed)
          helmet_data = enabled;
        return;

      case 25:  // C26 clock for helmet LEDs
        if (!smoothed && enabled) {
          proc_shiftRegEnqueue(helmet_data);
        }
        return;
    }

    default_wpc_proc_solenoid_handler(solNum, enabled, smoothed);
  }
#endif

/*---------------
/  Game handling
/----------------*/
static void init_bop(void) {
  core_gameData = &bopGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, bop_wpc_w);
  HC4094_init(&hc4094bop);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
#ifdef PROC_SUPPORT
  wpc_proc_solenoid_handler = bop_wpc_proc_solenoid_handler;
  if (coreGlobals.p_rocEn) {
    // clock on C26, data on C25
    proc_shiftRegInit(25 + 40, 24 + 40);
  }
#endif
}
