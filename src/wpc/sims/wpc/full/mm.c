/*******************************************************************************
 Medieval Madness (Williams, 1997) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 31, 2000

 Known Issues/Problems with this Simulator:

 None.

 Changes:

 311200 - First version.
 010101 - Added lamp matrix.
 240200 - New solenoid numbering makes getsol obsolete
 Thanks goes to Tom for the Drawbridge motor handling.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Medieval Madness Simulator:
  ----------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
    +R  L/R Ramp
    +L  L/R Loop
    +T  L/R Troll Head
    +D  L/R Troll Targets
    +X  L/R Bonus X Rollover
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
     M  Merlin's Magic Hole
     C  Castle
     L  Lock
     Z  Catapult
   VBN  Merlin Drop Targets
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "wpcsam.h"

#define MM_DRAWBRIDGETICK	2     /* turn 2 degrees per simulator tick */

/*------------------
/  Local functions
/-------------------*/
static int  mm_handleBallState(sim_tBallStatus *ball, int *inports);
static void mm_handleMech(int mech);
static void mm_drawMech(BMTYPE **line);
static void mm_drawStatic(BMTYPE **line);
static int mm_getMech(int mechNo);
static void init_mm(void);
static const char *showdrawbridgePos(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int drawbridgePos;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(mm,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"L/R Loop",		KEYCODE_L)
    COREPORT_BIT(0x0400,"Eject",			KEYCODE_M)
    COREPORT_BIT(0x0800,"L/R Troll",		KEYCODE_T)
    COREPORT_BIT(0x1000,"L/R Troll Targets",	KEYCODE_D)
    COREPORT_BIT(0x2000,"L/R Bonus X",		KEYCODE_X)
    COREPORT_BIT(0x4000,"Lock",			KEYCODE_L)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Castle",		KEYCODE_C)
    COREPORT_BIT(0x0100,"Catapult",		KEYCODE_Z)
    COREPORT_BIT(0x1000,"Top Merlin Tgt",	KEYCODE_V)
    COREPORT_BIT(0x2000,"Mid Merlin Tgt",	KEYCODE_B)
    COREPORT_BIT(0x4000,"Bot Merlin Tgt",	KEYCODE_N)


WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swLaunch	11
#define swCatapultTgt	12
#define swStart      	13
#define swTilt       	14
#define swLTrollTgt	15
#define swLeftOutlane	16
#define swRightInlane	17
#define swShooter	18
#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swRTrollTgt	25
#define swLeftInlane	26
#define swRightOutlane	27
#define swRightEject	28
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLPopper	36
#define swCastleGate	37
#define swCatapult	38
#define swEnterMoat	41
#define swCastleLock	44
#define swLTrollUPF	45
#define swRTrollUPF	46
#define swLeftTopLane	47
#define swRightTopLane	48
#define swLeftSling	51
#define swRightSling	52
#define swLeftJet	53
#define swBottomJet	54
#define swRightJet	55
#define swDBUp		56
#define swDBDown	57
#define swTowerExit	58
#define swEnterLRamp	61
#define swLRampMade	62
#define swEnterRRamp	63
#define swRRampMade	64
#define swLLoopLo	65
#define swLLoopHi	66
#define swRLoopLo	67
#define swRLoopHi	68
#define swRBankTop	71
#define swRBankMid	72
#define swRBankBot	73
#define swLTrollUp	74
#define swRTrollUp	75

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sTrough		2
#define sLeftPopper	3
#define sCastle		4
#define sCastleGateP	5
#define sCastleGateH	6
#define sKnocker	7
#define sCatapult	8
#define sRightEject	9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sBottomJet	13
#define sRightJet	14
#define sTowerDivP	15
#define sTowerDivH	16
#define sTowerLockPost	26
#define sRightGate	27
#define sLeftGate	28
#define sLTrollP	33
#define sLTrollH	34
#define sRTrollP	35
#define sRTrollH	36
#define sDBMotor	41

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
      stEnterLRamp, stLRampMade, stLHabitrail, stEnterRRamp, stRRampMade, stRHabitrail, stTower, stTowerLock,
      stLeftLoop1, stLeftLoop2, stRLoop2, stRLoop1, stRightLoop1, stRightLoop2, stLLoop2, stLLoop1, stLTopLane, stLJet, stRJet, stBJet, stRLoopExit, stRTopLane, stLLoopExit,
      stEject, stOutFromHole, stCatapult, stCatapultHole, stFired, stRBankTop, stRBankMid, stRBankBot,
      stLTroll, stLTroll2, stRTroll, stRTroll2, stLTrollTgt, stRTrollTgt,
      stLeftTopLane, stRightTopLane,
      stLeftPopper, stOutFromH, stLockLane, stLockHole, stCastle, stCastleMoat, stHitCastle, stHitCastle2, stHitCastle3, stEnterCastle, stEnterCastle2
	  };

static sim_tState mm_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,	stTrough,	1},
  {"Trough Jam",	1,swTroughJam,  0,		stShooter,	1},
  {"Drain",		1,0,		0,		stTrough4,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sLaunch,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		stLTopLane,	12,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 3*/
  {"Enter L. Ramp",	1,swEnterLRamp,	 0,		stLRampMade,	7},
  {"L. Ramp Made",	1,swLRampMade,	 0,		stLHabitrail,	3},
  {"L. Habitrail",	1,0,		 0,		stLeftInlane,	7},
  {"Enter R. Ramp",	1,swEnterRRamp,	 0,		stRRampMade,	7, sTowerDivH, stTower},
  {"R. Ramp Made",	1,swRRampMade,	 0,		stRHabitrail,	3},
  {"R. Habitrail",	1,0,		 0,		stRightInlane,	5},
  {"Damsel Tower",	10,swTowerExit,	 0,		stTowerLock,	0},
  {"Damsel Tower",	1,0,	 	 0,		0,		0},

  /*Line 4*/
  {"Enter L. Loop",	1,swLLoopLo,	 0,		stLeftLoop2,	2},
  {"L. Loop Sw #2",	2,swLLoopHi,	 0,		stRTopLane,	4,sRightGate, stRLoop2},
  {"R. Loop Sw #2",	1,swRLoopHi,	 0,		stRLoop1,	2},
  {"R. Loop Sw #1",	1,swRLoopLo,	 0,		stFree,		3},
  {"Enter R. Loop",	1,swRLoopLo,	 0,		stRightLoop2,	2},
  {"R. Loop Sw #2",	2,swRLoopHi,	 0,		stLTopLane,	4,sLeftGate, stLLoop2},
  {"L. Loop Sw #2",	1,swLLoopHi,	 0,		stLLoop1,	2},
  {"L. Loop Sw #1",	1,swLLoopLo,	 0,		stFree,		3},
  {"Top Left Lane",	1,swLeftTopLane, 0,		stLJet,		5},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stRLoopExit,	5},
  {"Down F. Right",	1,swRLoopLo,	 0,		stFree,		5},
  {"Top Right Lane",	1,swRightTopLane,0,		stLJet,		5},
  {"Down From Left",	1,swLLoopLo,	 0,		stFree,		5},

  /*Line 5*/
  {"Merlin's Hole",	1,swRightEject,  sRightEject,	stOutFromHole,	0},
  {"Out From Hole",	1,0,		 0,		stRLoopExit,	5},
  {"Catapult Tgt.",	1,swCatapultTgt, 0,		stCatapultHole,	4},
  {"Catapult",		1,swCatapult,	 sCatapult,	stFired,	0},
  {"Ball Fired",	1,0,		 0,		stLRampMade,	3},
  {"R. Top Target",	1,swRBankTop,	 0,		stFree,		3},
  {"R. Mid Target",	1,swRBankMid,	 0,		stFree,		3},
  {"R. Bot Target",	1,swRBankBot,	 0,		stFree,		3},

  /*Line 6*/
  {"Left Troll",	0,0,		 0,		0,		0},
  {"Left Troll",	1,swLTrollUPF,	 0,		stFree,		5},
  {"Right Troll",	0,0,		 0,		0,		0},
  {"Right Troll",	1,swRTrollUPF,	 0,		stFree,		5},
  {"Left Troll T.",	1,swLTrollTgt,	 0,		stFree,		3},
  {"Right Troll T.",	1,swRTrollTgt,	 0,		stFree,		3},

  /*Line 7*/
  {"Top Left Lane",	1,swLeftTopLane, 0,		stFree,		3},
  {"Top Right Lane",	1,swRightTopLane,0,		stFree,		3},

  /*Line 8*/
  {"Left Hole",		1,swLPopper,	 sLeftPopper,	stOutFromH,	0},
  {"Out From Hole",	1,0,		 0,		stLLoopExit,	3},
  {"Lock Lane",		1,0,		 0,		stLockHole,	3},
  {"Castle Lock",	1,swCastleLock,	 0,		stLeftPopper,	7},
  {"",			0,0,		 0,		0,		0},
  {"Castle Moat",	1,swEnterMoat,	 0,		stLeftPopper,	7},
  {"Castle Hit",	1,swEnterMoat,   0,		stHitCastle2,	1},
  {"Castle Hit",	1,swCastleGate,	 0,		stHitCastle3,	1},
  {"Castle Hit",	1,swEnterMoat,	 0,		stFree,		4},
  {"Enter Castle",	1,swEnterMoat,	 0,		stEnterCastle2,	1},
  {"Enter Castle",	1,swCastleGate,  0,		stLockHole,	4},

  {0}
};

static int mm_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Hit Trolls */
	case stLTroll:
		if(core_getSw(swLTrollUp))
			return setState(stLTroll2,0);
		else
			return setState(stFree,0);
	case stRTroll:
		if(core_getSw(swRTrollUp))
			return setState(stRTroll2,0);
		else
			return setState(stFree,0);

	/* Hit the Castle */
	case stCastle:
		if(core_getSw(swDBUp))
			return setState(stCastleMoat,0);
		else
			if (core_getSol(sCastleGateH))
				return setState(stEnterCastle,0);
			else
      			return setState(stHitCastle,0);

	/* Tower Lock (has to be here, works inverted) */
	case stTowerLock:
		if(!core_getSol(sTowerLockPost))
			return setState(stFree,0);

	}
    return 0;
  }

static void mm_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
    locals.drawbridgePos = 0;
	core_setSw(swDBUp, TRUE);
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData mm_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterLRamp},
  {0, 0x0006, stEnterRRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0201, stLeftLoop1},
  {0, 0x0202, stRightLoop1},
  {0, 0x0400, stEject},
  {0, 0x0801, stLTroll},
  {0, 0x0802, stRTroll},
  {0, 0x1001, stLTrollTgt},
  {0, 0x1002, stRTrollTgt},
  {0, 0x2001, stLeftTopLane},
  {0, 0x2002, stRightTopLane},
  {0, 0x4000, stLockLane},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stCastle},
  {1, 0x0100, stCatapult},
  {1, 0x1000, stRBankTop},
  {1, 0x2000, stRBankMid},
  {1, 0x4000, stRBankBot},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay mm_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{22,26,WHITE}}},{1,{{24,26,WHITE}}},{1,{{26,26,WHITE}}},{1,{{18,23,RED}}},
{1,{{20,22,YELLOW}}},{1,{{22,21,YELLOW}}},{1,{{24,20,YELLOW}}},{1,{{26,19,YELLOW}}},
{1,{{16,27,RED}}},{1,{{18,26,YELLOW}}},{1,{{20,25,YELLOW}}},{1,{{22,24,YELLOW}}},
{1,{{11,14,LBLUE}}},{1,{{13,14,LBLUE}}},{1,{{15,13,LBLUE}}},{1,{{17,12,LBLUE}}},
{1,{{ 7,21,YELLOW}}},{1,{{ 9,20,ORANGE}}},{1,{{11,19,WHITE}}},{1,{{15,17,YELLOW}}},
{1,{{17,16,ORANGE}}},{1,{{19,15,YELLOW}}},{1,{{21,14,YELLOW}}},{1,{{23,13,YELLOW}}},
{1,{{11, 3,RED}}},{1,{{13, 4,YELLOW}}},{1,{{15, 5,YELLOW}}},{1,{{17, 6,YELLOW}}},
{1,{{21, 3,RED}}},{1,{{23, 4,WHITE}}},{1,{{25, 5,WHITE}}},{1,{{27, 6,WHITE}}},
{1,{{ 3,14,RED}}},{1,{{ 5,14,WHITE}}},{1,{{ 7,14,LBLUE}}},{1,{{ 9,14,LBLUE}}},
{1,{{ 1,23,YELLOW}}},{1,{{ 1,26,YELLOW}}},{1,{{ 5,11,YELLOW}}},{1,{{ 7,17,YELLOW}}},
{1,{{31,19,WHITE}}},{1,{{30,14,WHITE}}},{1,{{31, 9,WHITE}}},{1,{{13, 7,RED}}},
{1,{{15, 8,YELLOW}}},{1,{{17, 9,YELLOW}}},{1,{{19,10,YELLOW}}},{1,{{21,11,YELLOW}}},
{1,{{35,18,WHITE}}},{1,{{33,14,YELLOW}}},{1,{{35,10,WHITE}}},{1,{{37,14,WHITE}}},
{1,{{ 6, 8,GREEN}}},{1,{{ 8, 9,GREEN}}},{1,{{10,10,YELLOW}}},{1,{{ 5,28,YELLOW}}},
{1,{{31,28,RED}}},{1,{{31,25,RED}}},{1,{{31, 3,RED}}},{1,{{31, 0,RED}}},
{1,{{ 4, 7,GREEN}}},{1,{{39,14,YELLOW}}},{1,{{39,29,RED}}},{1,{{39, 0,YELLOW}}}
}
};



static void mm_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"DrawBridge Pos.: %-17s",showdrawbridgePos());
  core_textOutf(30,10,BLACK,"Castle Gate: %-6s",core_getSol(sCastleGateH) ? "Open":"Closed");
  core_textOutf(30,20,BLACK,"L/R Troll  : %-4s/%-4s",core_getSol(sLTrollH) ? " Up ":"Down", core_getSol(sRTrollH) ? " Up ":"Down");
}
/* Help */

static void mm_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help Keys:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30,100,BLACK,"L/R Ctrl+T = L/R Troll Head");
  core_textOutf(30,110,BLACK,"L/R Ctrl+D = L/R Troll Target");
  core_textOutf(30,120,BLACK,"L/R Ctrl+X = L/R Bonus X Rollover");
  core_textOutf(30,130,BLACK,"M = Merlin's Magic Hole");
  core_textOutf(30,140,BLACK,"V/B/N = Merlin's Drop Targets");
  core_textOutf(30,150,BLACK,"C = Castle, L = Lock, Z = Catapult");
  core_textOutf(30,160,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap mm_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sRightEject,1,SAM_SOLENOID_ON},
 {sTowerDivP,1,SAM_DIVERTER}, {sTowerDivH,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #2*/
 {sLaunch,2,SAM_SOLENOID},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},  {sCatapult,2,SAM_POPPER},

 /*Channel #3*/
 {sLeftPopper,3,SAM_POPPER}, {sCastle,3,SAM_SOLENOID},
 {sCastleGateP,3,SAM_FLAPOPEN}, {sCastleGateH,3,SAM_FLAPCLOSE,WPCSAM_F_ONOFF},

 /*Channel #4*/
 {sLTrollP,4,SAM_SOLENOID}, {sLTrollH,4,SAM_FLAPCLOSE,WPCSAM_F_ONOFF},
 {sRTrollP,4,SAM_SOLENOID}, {sRTrollH,4,SAM_FLAPCLOSE,WPCSAM_F_ONOFF},
 {sTowerLockPost,4,SAM_SOLENOID_ON}, {sTowerLockPost,4,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #5*/
 {sLeftGate,5,SAM_SOLENOID_ON}, {sLeftGate,5,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sRightGate,5,SAM_SOLENOID_ON}, {sRightGate,5,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sDBMotor,5,SAM_MOTOR_1,WPCSAM_F_CONT},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
#define MM_SOUND \
DCS_SOUNDROM5xm("mm_s2.1_0",  CRC(c55c3b71) SHA1(95febbf16645dd897bdd459ccad9501d2457d1f1), \
                "mm_sav3.rom",CRC(ed1be570) SHA1(ead4c4f89d63ee0b46d8a8bcd8650d506542d1ee), \
                "mm_sav4.rom",CRC(9c89eacf) SHA1(594a2aa81e34658862a9b7f0a83cf514182f2a2d), \
                "mm_sav5.rom",CRC(45089e30) SHA1(e83492109c59e8a2f1ba9e1f793788b97d150a9b), \
                "mm_sav6.rom",CRC(439d55f2) SHA1(d80e7268223157d864674261d140322634fb3bc2))

WPC_ROMSTART(mm,109, "mm_1_09.bin", 0x100000,CRC(9bac4d0c) SHA1(92cbe21802e1a77feff77b78f4dbbdbffb7b14bc)) MM_SOUND WPC_ROMEND
WPC_ROMSTART(mm,109b,"mm_109b.bin",0x100000,CRC(4eaab86a) SHA1(694cbb1154e7374275becfbe4f743fb8d31df8fb)) MM_SOUND WPC_ROMEND
WPC_ROMSTART(mm,109c,"mm_1_09c.bin",0x100000,CRC(d9e5189f) SHA1(fc01855c139d408559605fe9932236250cd566a8)) MM_SOUND WPC_ROMEND
WPC_ROMSTART(mm,10,  "mm_g11.1_0",  0x080000,CRC(6bd735c6) SHA1(3922df00e785610837230d5d9c24b9e082aa6fb6)) MM_SOUND WPC_ROMEND

WPC_ROMSTART(mm,05, "g11-050.rom", 0x080000,CRC(d211ad16) SHA1(539fb0c4ca6fe19ac6140f5792c5b7cd51f737ce))
DCS_SOUNDROM5xm("s2-020.rom",  CRC(ee009ce4) SHA1(36843b2f1a07cf1e23bdff9b7347ceeca7e915bc),
                "mm_sav3.rom",CRC(ed1be570) SHA1(ead4c4f89d63ee0b46d8a8bcd8650d506542d1ee),
                "mm_sav4.rom",CRC(9c89eacf) SHA1(594a2aa81e34658862a9b7f0a83cf514182f2a2d),
                "mm_sav5.rom",CRC(45089e30) SHA1(e83492109c59e8a2f1ba9e1f793788b97d150a9b),
                "mm_sav6.rom",CRC(439d55f2) SHA1(d80e7268223157d864674261d140322634fb3bc2))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (mm,10,   "Medieval Madness (1.0)",1997,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mm,109,10,"Medieval Madness (1.09)", 1999,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mm,109b,10,"Medieval Madness (1.09B)", 1999,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mm,109c,10,"Medieval Madness (1.09C, Profanity)", 1999,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mm,05,10,"Medieval Madness (0.50)", 1997,"Williams",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData mmSimData = {
  2,    			/* 2 game specific input ports */
  mm_stateDef,			/* Definition of all states */
  mm_inportData,		/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  mm_initSim, 			/* no init */
  mm_handleBallState,	/*Function to handle ball state changes*/
  mm_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 			/* Simulate manual shooter? */
  NULL  			/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData mmGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1,0,
    NULL, mm_handleMech, mm_getMech, mm_drawMech,
    &mm_lampPos, mm_samsolmap
  },
  &mmSimData,
  {
    "559 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_mm(void) {
  core_gameData = &mmGameData;
}

static void mm_handleMech(int mech) {
  /* Handle the DrawBridge - Thanx Tom! */
  if (mech & 0x01) {
    if (core_getSol(sDBMotor) > 0)
      locals.drawbridgePos = (locals.drawbridgePos+MM_DRAWBRIDGETICK) % 500;
    core_setSw(swDBUp, (locals.drawbridgePos>=490) || (locals.drawbridgePos<=10));
    core_setSw(swDBDown, (locals.drawbridgePos>=240) && (locals.drawbridgePos<=260));
  }
  /* Handle the Trolls */
  if (mech & 0x02) {
    /* Left Troll */
    if (core_getSol(sLTrollH))
      core_setSw(swLTrollUp,1);
    else
      core_setSw(swLTrollUp,0);

    /* Right Troll */
    if (core_getSol(sRTrollH))
      core_setSw(swRTrollUp,1);
    else
      core_setSw(swRTrollUp,0);
  }
}

static int mm_getMech(int mechNo) {
  return locals.drawbridgePos;
}
/*---------------------------------
  Display Status of the DrawBridge
 ----------------------------------*/
static const char* showdrawbridgePos(void)
{
  if(core_getSw(swDBUp) && !core_getSol(sDBMotor))
	return "Up              ";
  if(core_getSw(swDBDown) && !core_getSol(sDBMotor))
	return "Down            ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 50)
	return "Moving Down.....";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 100)
	return "Moving Down.... ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 150)
	return "Moving Down...  ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 200)
	return "Moving Down..   ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 250)
	return "Moving Down.    ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 300)
	return "Moving Up.      ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 350)
	return "Moving Up..     ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 400)
	return "Moving Up...    ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 450)
	return "Moving Up....   ";
  if(core_getSol(sDBMotor) && locals.drawbridgePos < 500)
	return "Moving Up.....  ";
  return "Unknown         ";
}
