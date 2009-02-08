/*******************************************************************************
 No Good Gofers (Williams, 1997) Pinball Simulator

 by Dave Roberts (daverob@cwcom.net)
 Feb. 16, 2001

 Known Issues : - It is possible for a weak loop shot to fall into the Putt
                  Out hole, but we are not going to simulate these cases
                - Some functions have more comment than code, but I wrote the
                  comments first and couldn't bring myself to remove them ;-)
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the No Good Gofers Simulator:
  --------------------------------------
    +I  L1/L2/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   SDF  Jet Bumpers
     R  L/C/R Ramp
     L  L/R Loop
     T  Putt Out
     Y  Golf Cart
     .  Hole In One
     A  Jet Advance
  GHJK  K I C K
     Z  Advance Trap
     X  Sand Trap
     C  Captive Ball
     V  Advance Kick
   BNM  Skill Shot

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  ngg_handleBallState(sim_tBallStatus *ball, int *inports);
static void ngg_handleMech(int mech);
static int  ngg_getMech(int mechNo);
static void ngg_drawMech(BMTYPE **line);
static void ngg_drawStatic(BMTYPE **line);
static int ngg_getSol(int solNo);
static void init_ngg(void);
static const char* showramp(int lr);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int ramppos[2];       // status of left and right ramps
  int goferpos[2];      // status of left and right gofers
  int wheelpos;         // position of wheel
  int slampos;          // position of slam ramp
  int slamdelay;        // slam ramp smoothing counter
} locals;

/*Status of Ramps and Gofers*/
enum {DOWN=0,UP};

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(ngg,6)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",              KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L1/L2/R Inlane",        KEYCODE_I)
    COREPORT_BIT(0x0040,"L/R Loop",              KEYCODE_L)
    COREPORT_BIT(0x0080,"Putt Out",              KEYCODE_T)
    COREPORT_BIT(0x0100,"Centre Ramp",           KEYCODE_R)
    COREPORT_BIT(0x0200,"Golf Cart",             KEYCODE_Y)
    COREPORT_BIT(0x1000,"Hole In One",           KEYCODE_STOP)
    COREPORT_BIT(0x4000,"Jet Advance",           KEYCODE_A)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Top Jet",               KEYCODE_S)
    COREPORT_BIT(0x0002,"Middle Jet",            KEYCODE_D)
    COREPORT_BIT(0x0004,"Bottom Jet",            KEYCODE_F)
    COREPORT_BIT(0x0008,"(K)ick",                KEYCODE_G)
    COREPORT_BIT(0x0010,"K(i)ck",                KEYCODE_H)
    COREPORT_BIT(0x0020,"Ki(c)k",                KEYCODE_J)
    COREPORT_BIT(0x0040,"Kic(k)",                KEYCODE_K)
    COREPORT_BIT(0x0100,"Advance Trap",          KEYCODE_Z)
    COREPORT_BIT(0x0200,"Sand Trap",             KEYCODE_X)
    COREPORT_BIT(0x0400,"Captive Ball",          KEYCODE_C)
    COREPORT_BIT(0x0800,"Advance Kick",          KEYCODE_V)
    COREPORT_BIT(0x1000,"Top Skill",             KEYCODE_B)
    COREPORT_BIT(0x2000,"Middle Skill",          KEYCODE_N)
    COREPORT_BIT(0x4000,"Bottom Skill",          KEYCODE_M)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* Standard Switches */
// not used             11
#define swLRampMake     12
#define swStart      	13
#define swTilt       	14
#define swCRampMake     15
#define swLeftOutlane   16
#define swRightInlane	17
#define swShooter	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swJetAdvance    23
#define swUnderGPass    25
#define swLeftInlane    26
#define swRightOutlane	27
#define swKickback      28

#define swTroughJam     31 // Opto
#define swTrough1       32 // Opto
#define swTrough2       33 // Opto
#define swTrough3       34 // Opto
#define swTrough4       35 // Opto
#define swTrough5       36 // Opto
#define swTrough6       37 // Opto
#define swJetPopper     38 // Opto

#define swLGoferDown    41 // Opto
#define swRGoferDown    42 // Opto
// not used             43
#define swPuttOut       44 // Opto
#define swRPopperJam    45 // Opto
#define swRPopper       46 // Opto
#define swLRampDown     47
#define swRRampDown     48

#define swLeftSling	51
#define swRightSling	52
#define swTopJet	53
#define swMiddleJet	54
#define swBottomJet	55
#define swTopSkill      56
#define swMiddleSkill   57
#define swBottomSkill   58

#define swLeftSpinner   61
#define swRightSpinner  62
#define swInnerWheel    63 // Opto
#define swOuterWheel    64 // Opto
#define swLeftGofer1    65
#define swLeftGofer2    66
#define swBehindLGofer  67
#define swHoleInOne     68

#define swLCartPath     71
#define swRCartPath     72
#define swRRampMake     73
#define swGolfCart      74
#define swRightGofer1   75
#define swRightGofer2   76
#define swAdvanceTrap   77
#define swSandTrap      78

#define swAdvanceKick   81
#define swKick          82
#define swkIck          83
#define swkiCk          84
#define swkicK          85
#define swCaptiveBall   86
// not used             87
// not used             88

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKickBack	2
#define sClubhouse      3
#define sLGoferUp       4
#define sRGoferUp       5
#define sJetPopper      6
#define sLeftEject      7
#define sRightEject     8
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sTopJet		12
#define sMiddleJet	13
#define sBottomJet      14
#define sLGoferDown     15
#define sRGoferDown     16
#define sUGroundPass    24
#define sLRampDown      27
#define sRRampDown      28
#define sSlamRamp       35 // Upper Left flipper power solenoid.

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough6=SIM_FIRSTSTATE, stTrough5, stTrough4, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeft1Inlane, stLeft2Inlane,
      stLeftSling, stRightSling, stTopJet, stMiddleJet, stBottomJet,
      stLeftRamp, stCentreRamp, stRightRamp, stLeftLoop, stRightLoop, stPuttOut,
      stGolfCart, stHoleInOneTest, stHoleInOne, stJetAdvance, stKick, stkIck, stkiCk, stkicK,
      stAdvanceTrap, stSandTrap, stCaptiveBall, stAdvanceKick, stTopSkill, stMiddleSkill, stBottomSkill,
      stLCartPath, stRCartPath, stLCartPath1, stRCartPath1, stLeftLoop1, stRightLoop1,
      stUnderLRamp, stUGroundPass, stJetPopper, stUnderRRamp, stRPopper,
      stCRampTest, stLGofer2L, stLGofer2R, stLGofer2H, stLGofer1L, stLGofer1R, stLGofer1H, stLGofer2a,
      stRRampTest, stRGofer2L, stRGofer2R, stRGofer2H, stRGofer1L, stRGofer1R, stRGofer1H, stRGofer2a
    };

static sim_tState ngg_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 6",          1,swTrough6,    0,              stTrough5,      1},
  {"Trough 5",          1,swTrough5,    0,              stTrough4,      1},
  {"Trough 4",          1,swTrough4,    0,              stTrough3,      1},
  {"Trough 3",          1,swTrough3,    0,              stTrough2,      1},
  {"Trough 2",          1,swTrough2,    0,              stTrough1,      1},
  {"Trough 1",          1,swTrough1,    sTrough,        stTrough,       1},
  {"Trough Jam",        1,swTroughJam,  0,              stShooter,      1},
  {"Drain",             1,0,            0,              stTrough6,      0,      0,      0,      SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",           1,swShooter,     0,             0,              0,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",         1,0,             0,             0,              2,0,0,SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",      1,swKickback,    0,             stDrain,        15, sKickBack, stFree, 5},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",       1,swLeftOutlane, 0,             stFree,         5},
  {"Left I.Inlane",     1,swLeftInlane,  0,             stFree,         5},

  /*Line 3*/
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Top Bumper",	1,swTopJet,	 0,		stFree,		1},
  {"Middle Bumper",	1,swMiddleJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 4*/
  {"Left Ramp",         1,swLRampMake,   0,             stLeft2Inlane,  1},
  {"Centre Ramp (Bud)", 1,swCRampMake,   0,             stLeftRamp,     1},
  {"Right Ramp (Buzz)", 1,swRRampMake,   0,             stRightInlane,  1},
  {"Left Spinner",      1,swLeftSpinner, 0,             stLCartPath,    1,0,0,SIM_STSPINNER},
  {"Right Spinner",     1,swRightSpinner,0,             stRCartPath,    1,0,0,SIM_STSPINNER},
  {"Putt Out",          1,swPuttOut,     0,0,0},

  /*Line 5*/
  {"Golf Cart",         1,swGolfCart,    0,             stFree,         1},
  {"Playfield",         1,0,0,0,0},
  {"Hole In One",       1,swHoleInOne,   0,             stUnderLRamp,   1},
  {"Jet Advance",       1,swJetAdvance,  0,             stFree,         1},
  {"(K)ick",            1,swKick,        0,             stFree,         1},
  {"K(i)ck",            1,swkIck,        0,             stFree,         1},
  {"Ki(c)k",            1,swkiCk,        0,             stFree,         1},
  {"Kic(k)",            1,swkicK,        0,             stFree,         1},

  /*Line 6*/
  {"Advance Trap",      1,swAdvanceTrap, 0,             stFree,         1},
  {"Sand Trap",         1,swSandTrap,    sLeftEject,    stkiCk,         50}, /* Sandtrap always hits Ki(c)k target (well it does on my machine!) */
  {"Captive Ball",      1,swCaptiveBall, 0,             stFree,         1},
  {"Advance Kick",      1,swAdvanceKick, 0,             stFree,         1},
  {"Top Skill",         1,swTopSkill,    0,             stFree,         1},
  {"Middle Skill",      1,swMiddleSkill, 0,             stFree,         1},
  {"Bottom Skill",      1,swBottomSkill, 0,             stFree,         1},

  /*Line 7*/
  {"Left Cart Path",    1,swLCartPath,   0,             stRCartPath1,   5},
  {"Right Cart Path",   1,swRCartPath,   0,             stLCartPath1,   5},
  {"Left Cart Path",    1,swLCartPath,   0,             stLeftLoop1,    5},
  {"Right Cart Path",   1,swRCartPath,   0,             stRightLoop1,   5},
  {"Left Spinner",      1,swLeftSpinner, 0,             stFree,         1,0,0,SIM_STSPINNER},
  {"Right Spinner",     1,swRightSpinner,0,             stFree,         1,0,0,SIM_STSPINNER},

  /*Line 8*/
  {"Under Left Ramp",   1,swBehindLGofer,0,             stPuttOut,      1},
  {"Underground Pass",  1,swUnderGPass,  0,             stJetPopper,    1},
  {"Jet Popper",        1,swJetPopper,   sJetPopper,    stFree,         1},
  {"Right Popper Jam",  1,swRPopperJam,  0,             stRPopper,      1},
  {"Right Popper",      1,swRPopper,     sRightEject,   stFree,         1},

  /*Line 9*/
  {"Hit Bud 2",         1,0,0,0,0},
  {"Hit Bud 2",         1,swLeftGofer2,  0,             stLGofer1L,     1},
  {"Hit Bud 2",         1,swLeftGofer2,  0,             stLGofer1R,     1},
  {"Hit Bud 2",         1,swLeftGofer2,  0,             stLGofer1H,     1},
  {"Hit Bud 1",         1,swLeftGofer1,  0,             stUnderLRamp,   1},
  {"Hit Bud 1",         1,swLeftGofer1,  0,             stCentreRamp,   1},
  {"Hit Bud 1",         1,swLeftGofer1,  0,             stLGofer2a,     1},
  {"Hit Bud 2",         1,swLeftGofer2,  0,             stFree,         1},

  /*Line 10*/
  {"Hit Buzz 1",        1,0,0,0,0},
  {"Hit Buzz 2",        1,swRightGofer2, 0,             stRGofer1L,     1},
  {"Hit Buzz 2",        1,swRightGofer2, 0,             stRGofer1R,     1},
  {"Hit Buzz 2",        1,swRightGofer2, 0,             stRGofer1H,     1},
  {"Hit Buzz 1",        1,swRightGofer1, 0,             stUnderRRamp,   1},
  {"Hit Buzz 1",        1,swRightGofer1, 0,             stRightRamp,    1},
  {"Hit Buzz 1",        1,swRightGofer1, 0,             stRGofer2a,     1},
  {"Hit Buzz 2",        1,swRightGofer2, 0,             stFree,         1},


  {0}
};

static void ngg_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
    locals.wheelpos = 0;
    core_setSw(swOuterWheel,0);
    core_setSw(swInnerWheel,0);
}


static int ngg_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
        // NGG has both, manual and auto plunger
	case stShooter:
                if (core_getSol(sLaunch)) {
			ball->speed = 50;
			return setState(stBallLane,1);
		}
		if (sim_getSol(sShooterRel)) {
			return setState(stBallLane,1);
		}
		break;

	/* Ball in Shooter Lane */
    	case stBallLane:
                if (ball->speed < 10)
                        return setState(stNotEnough,25);        /*Ball not plunged hard enough*/
                if (ball->speed < 25)
			return setState(stFree,51);		/*Ball goes to stFree*/
                if (ball->speed < 30)
                        return setState(stBottomSkill,25);      /*Ball hits bottom skill shot target*/
                if (ball->speed < 35)
                        return setState(stMiddleSkill,25);      /*Ball hits middle skill shot target*/
                if (ball->speed < 40)
                        return setState(stTopSkill,25);         /*Ball hits top skill shot target*/
                if (ball->speed < 45)
			return setState(stFree,51);		/*Ball goes to stFree*/
                if (ball->speed < 51)
                        return setState(stLeftRamp,25);         /*Ball goes up left ramp*/
		break;

        // Centre ramp, Gofer Hit or Lock
        // We have to test for this before we trigger any switches as the
        // software will drop the gofers for a lock, as soon as the first
        // switch is made (relying on the mechanical delay of the mechanism).
        // Without this test first we would lock the ball on every gofer hit!
        case stCRampTest:
                if (locals.ramppos[0] == DOWN)
                  return setState(stLGofer2R,1);
                else
                  if (locals.goferpos[0] == DOWN)
                        return setState(stLGofer2L,1);
                return setState(stLGofer2H,1);
                break;

        // Right ramp, Gofer Hit or Lock (Same as Centre Ramp!)
        case stRRampTest:
                if (locals.ramppos[1] == DOWN)
                  return setState(stRGofer2R,1);
                else
                  if (locals.goferpos[1] == DOWN)
                        return setState(stRGofer2L,1);
                return setState(stRGofer2H,1);
                break;

        // Ball can leave Putt Out by one of two solenoids
        case stPuttOut:
                if (core_getSol(sClubhouse))
                        return setState(stFree,1);
                if (core_getSol(sUGroundPass))
                        return setState(stUGroundPass,1);
                break;

        // We can only hit a hole in one if the slam ramp is down
        case stHoleInOneTest:
                if (locals.slampos==UP)
                        return setState(stFree,1);
                else
                        return setState(stHoleInOne,1);
                break;
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData ngg_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLeftRamp},
  {0, 0x0006, stRRampTest},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0020, stLeft2Inlane},
  {0, 0x0021, stLeft1Inlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0041, stLeftLoop},
  {0, 0x0042, stRightLoop},
  {0, 0x0080, stPuttOut},
  {0, 0x0100, stCRampTest},
  {0, 0x0200, stGolfCart},
  {0, 0x1000, stHoleInOneTest},
  {0, 0x4000, stJetAdvance},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stTopJet},
  {1, 0x0002, stMiddleJet},
  {1, 0x0004, stBottomJet},
  {1, 0x0008, stKick},
  {1, 0x0010, stkIck},
  {1, 0x0020, stkiCk},
  {1, 0x0040, stkicK},
  {1, 0x0100, stAdvanceTrap},
  {1, 0x0200, stSandTrap},
  {1, 0x0400, stCaptiveBall},
  {1, 0x0800, stAdvanceKick},
  {1, 0x1000, stTopSkill},
  {1, 0x2000, stMiddleSkill},
  {1, 0x4000, stBottomSkill},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/

static core_tLampDisplay ngg_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{28, 2,RED   }}},                  // 11 Outlane Extra Ball
 {1,{{31, 2,WHITE }}},                  // 12 Kickback
 {1,{{27, 5,WHITE }}},                  // 13 Lower Driving Range
 {1,{{37,14,RED   }}},                  // 14 Shoot Again
 {1,{{28,24,RED   }}},                  // 15 Special
 {1,{{23,16,YELLOW}}},                  // 16 Wheel Value
 {1,{{20, 8,ORANGE}}},                  // 17 Jet Lightning
 {1,{{29,18,RED   }}},                  // 18 Hole 8
 {1,{{27,14,YELLOW}}},                  // 21 Hole 5
 {1,{{27,12,YELLOW}}},                  // 22 Hole 4
 {1,{{28,11,WHITE }}},                  // 23 Hole 3
 {1,{{30,13,YELLOW}}},                  // 24 Hit Bud
 {1,{{31,10,WHITE }}},                  // 25 Hole 1
 {1,{{33,10,WHITE }}},                  // 26 2x
 {1,{{33,12,WHITE }}},                  // 27 Cart Path 2
 {2,{{35,13,ORANGE},{35,15,ORANGE}}},   // 28 5x Cart Path (2)
 {1,{{27,16,YELLOW}}},                  // 31 Hole 6
 {1,{{28,17,ORANGE}}},                  // 32 Hole 7
 {1,{{29,10,WHITE }}},                  // 33 Hole 2
 {1,{{30,15,YELLOW}}},                  // 34 Hit Buzz
 {1,{{31,18,RED   }}},                  // 35 Hole 9
 {1,{{33,18,RED   }}},                  // 36 4x
 {1,{{33,18,ORANGE}}},                  // 37 Cart Path 4
 {1,{{33,14,YELLOW}}},                  // 38 3x
 {1,{{22,13,ORANGE}}},                  // 41 Driving Range
 {1,{{21,13,YELLOW}}},                  // 42 Increase Golf Cart
 {1,{{20,12,YELLOW}}},                  // 43 Increase Buzz Value
 {1,{{19,12,WHITE }}},                  // 44 Increase Bud Value
 {1,{{18,11,WHITE }}},                  // 45 Newton Drive
 {1,{{18,10,ORANGE}}},                  // 46 Collect
 {1,{{17,11,YELLOW}}},                  // 47 Rip Off
 {1,{{17, 9,WHITE }}},                  // 48 Left Loop Drive
 {1,{{19,23,ORANGE}}},                  // 51 (K)ick
 {1,{{20,22,ORANGE}}},                  // 52 K(i)ck
 {1,{{21,22,ORANGE}}},                  // 53 Ki(c)k
 {1,{{22,21,ORANGE}}},                  // 54 Kic(k)
 {2,{{15,18,ORANGE},{16,22,ORANGE}}},   // 55 Skill Shot (2)
 {1,{{12,19,YELLOW}}},                  // 56 Relight Jackpot
 {1,{{13,19,ORANGE}}},                  // 57 Right Ramp Lock
 {1,{{14,19,WHITE }}},                  // 58 Right Ramp Drive
 {1,{{23,11,YELLOW}}},                  // 61 4 Strokes
 {1,{{24,12,YELLOW}}},                  // 62 3 Strokes
 {1,{{25,10,WHITE }}},                  // 63 2 Strokes
 {1,{{23, 9,YELLOW}}},                  // 64 5 Strokes
 {1,{{26, 8,ORANGE}}},                  // 65 7 Strokes
 {1,{{24, 8,ORANGE}}},                  // 66 6 Strokes
 {1,{{15, 8,WHITE }}},                  // 67 Left Spinner
 {2,{{23, 5,YELLOW},{13,13,YELLOW}}},   // 68 Trap Ready (2)
 {1,{{22, 7,YELLOW}}},                  // 71 Advance Trap (2)
 {1,{{13,16,WHITE }}},                  // 72 Center Drive
 {1,{{12,16,ORANGE}}},                  // 73 Center Lock
 {1,{{11,15,WHITE }}},                  // 74 Get T.N.T.
 {1,{{11,17,YELLOW}}},                  // 75 Center Raise Gofer
 {1,{{ 9,23,WHITE }}},                  // 76 Right Spinner
 {1,{{11,22,WHITE }}},                  // 77 Right Loop Drive
 {1,{{20, 3,LBLUE }}},                  // 78 Bottom Jet Bumper
 {1,{{10,12,WHITE }}},                  // 81 Side Ramp Drive
 {1,{{ 8,13,RED   }}},                  // 82 Extra Ball
 {1,{{ 9,13,ORANGE}}},                  // 83 Multiball
 {1,{{11,13,YELLOW}}},                  // 84 Jackpot
 {1,{{ 7,13,WHITE }}},                  // 85 Putt Out
 {1,{{14, 3,WHITE }}},                  // 86 Top Jet Bumper
 {1,{{17, 6,RED   }}},                  // 87 Middle Jet Bumper
 {1,{{39, 1,YELLOW}}}                   // 88 Start Button
}};

/**********************************
 Wheel Position
 *********************************/
static const char* WheelText[] =
{" 0 Lite Xtra Ball", " 1 Cart Attack   ", " 2 Gofer Attack! ", " 3 Lite Q Jackpot",
 " 4 Warp          ", " 5 Pop-A-Gofer   ", " 6 Lite Outlanes ", " 7 Players Choice",
 " 8 Free Lock     ", " 9 Speed Golf    ", "10 Lite Ripoff   ", "11 Bad Shot      ",
 "12 Hole In One   ", "13 Lite Kickback ", "14 Big Spinners  ", "15 Gofer's Choice", " 0 Lite Xtra Ball"};

static const char* SlamText[] = {"Down","Up"};

  static void ngg_drawMech(BMTYPE **line) {

/* Help */

  core_textOutf(30, 30,BLACK,"Centre Ramp: %-10s", showramp(0));
  core_textOutf(30, 40,BLACK,"Right Ramp: %-10s", showramp(1));
  core_textOutf(30, 50,BLACK,"Wheel:%s", WheelText[(locals.wheelpos+1)/4]);
  core_textOutf(30, 60,BLACK,"Slam Ramp: %s  ", SlamText[locals.slampos]);
}
  static void ngg_drawStatic(BMTYPE **line) {
  core_textOutf(30, 80,BLACK,"Help on this Simulator:");
  core_textOutf(30, 89,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 98,BLACK,"L/R Ctrl+L/O = L/R Loops/Outlane");
  core_textOutf(30,107,BLACK,"L/R Ctrl+I/R =L/C/R Inlanes/Ramp");
  core_textOutf(30,116,BLACK,"Q = Drain Ball, S/D/F = Jet Bumpers");
  core_textOutf(30,125,BLACK,"G/H/J/K = Kick B/N/M = Skill Targets");
  core_textOutf(30,134,BLACK,"T = Putt Out A = Jet Advance");
  core_textOutf(30,143,BLACK,"Z = Advance Trap X = Sand Trap");
  core_textOutf(30,152,BLACK,"C = Captive Ball V = Advance Kick");
  core_textOutf(30,161,BLACK,"Y = Golf Cart Hit . = Hole in One");
  }

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(ngg,13,"go_g11.1_3",0x80000,CRC(64e73117) SHA1(ce7ba5a6d309677e51dcbc9e3058f98e69d1e917))
DCS_SOUNDROM5xm("nggsndl1.s2",  CRC(6263866d) SHA1(c72a2f176aa24e91ecafe1704affd16b86d671e2),
                "nggsndl1.s3",	CRC(6871b20d) SHA1(0109c02282806016a6b22f7dfe3ac964931ba609),
                "nggsndl1.s4",	CRC(86ed8f5a) SHA1(231f6313adff89ef4cec0d9f25b13e69ea96213d),
                "nggsndl1.s5",	CRC(ea2062f0) SHA1(f8e45c1fcc6b8677a0745a5d83ca93b77fbde752),
                "nggsndl1.s6",	CRC(b1b8b514) SHA1(e16651bcb2eae747987dc3c13a5dc20a33c0a1f8))
WPC_ROMEND
// No Good Gofers prototype roms
// s2 v0.2 rom is identical to l1 rom except for version string
// even the checksum (non CRC32) is the same!
WPC_ROMSTART(ngg,p06,"ngg0_6.rom",0x80000,CRC(e0e0d331) SHA1(e1b91eccec6034bcd2029c15596aa0b129c9e53f))
DCS_SOUNDROM5xm("ngg_s2.0_2",  CRC(dde128d5) SHA1(214ee807d2323ecb407a3d116b038e15c60e5580),
                "nggsndl1.s3",	CRC(6871b20d) SHA1(0109c02282806016a6b22f7dfe3ac964931ba609),
                "nggsndl1.s4",	CRC(86ed8f5a) SHA1(231f6313adff89ef4cec0d9f25b13e69ea96213d),
                "nggsndl1.s5",	CRC(ea2062f0) SHA1(f8e45c1fcc6b8677a0745a5d83ca93b77fbde752),
                "nggsndl1.s6",	CRC(b1b8b514) SHA1(e16651bcb2eae747987dc3c13a5dc20a33c0a1f8))
WPC_ROMEND


/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(ngg,13,"No Good Gofers (1.3)",1997,"Williams",wpc_m95S,0)
CORE_CLONEDEF(ngg,p06,13,"No Good Gofers (p0.6)",1997,"Williams",wpc_m95S,0)


/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData nggSimData = {
  2,    				/* 2 game specific input ports */
  ngg_stateDef,				/* Definition of all states */
  ngg_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  ngg_initSim,                          /* init */
  ngg_handleBallState,                  /* Function to handle ball state changes*/
  ngg_drawStatic,                       /* Function to handle mechanical state changes*/
  TRUE,                                 /* Simulate manual shooter */
  NULL                                  /* No Custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData nggGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,0,8,0,0,1,0,
    ngg_getSol, ngg_handleMech, ngg_getMech, ngg_drawMech,
    &ngg_lampPos, NULL
  },
  &nggSimData,
  {
    "561 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0xff, 0x3b, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

static WRITE_HANDLER(ngg_wpc_w) {
  static int lastFlip, lastWheel = 0;
// writes to the flippers have to be delayed 1 cycle to not interfere with the flashers.
// also, an intermittent 0 byte write has to be avoided.
  if (!(data & 0xc0) && offset == WPC_FLIPPERCOIL95) {
    if (lastFlip & data & 0x3f) wpc_w(WPC_FLIPPERCOIL95, data);
    if (!lastFlip & !data) wpc_w(WPC_FLIPPERCOIL95, 0);
    lastFlip = data;
  } else
    wpc_w(offset, data);

// Wheel state handling
//
// Wheel motor is driven by two solenoid outputs
// The solenoid drivers are fed to the input of a motor driver chip with
// active low inputs. So the inputs are active when the solenoid outputs
// are _not_ energised.
// solenoid 38 drives wheel clockwise (state decreases by one)
// solenoid 37 drives wheel counter clockwise (state increases by one)
//
// there are 64 states and 16 positions on the wheel
// Wheel position sensing is provided by two optos as shown in table
//
// Wheel 0000 0000 0011 1111 1111 2222 2222 2233 3333 3333 4444 4444 4455 5555 5555 6666
// State 0123 4567 8901 2345 6789 0123 4567 8901 2345 6789 0123 4567 8901 2345 6789 0123
//
// Inner 0111 1111 1111 1111 1111 1111 1111 1111 1110 0000 0000 0000 0000 0000 0000 0000
// Outer 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011 0011
//
// Pos.    0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
  if (offset == WPC_SOLENOID1) {
    if (lastWheel & ~data & 0x20) {
      locals.wheelpos++;
      if (locals.wheelpos > 63)
        locals.wheelpos = 0;
    }
    if (lastWheel & ~data & 0x10) {
      locals.wheelpos--;
      if (locals.wheelpos < 0)
        locals.wheelpos = 63;
    }
    core_setSw(swInnerWheel, locals.wheelpos > 0);
    core_setSw(swOuterWheel, locals.wheelpos % 4 == 0 || locals.wheelpos % 4 == 3);
    lastWheel = data;
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_ngg(void) {
  core_gameData = &nggGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, ngg_wpc_w);
  locals.slampos = 0;
  locals.slamdelay = 0;
}

static void ngg_handleMech(int mech) {
// NGG Gofers and ramps are controlled by three solenoids
// GoferUp raises the ramp and the Gofer
// when GoferDown solenoid is energised gofer drops
// when RampDown solenoid is energised ramp drops
  if (mech & 0x02) {
    /* -- If Raise solenoid Fired, Raise the Gofer (and ramp) --*/
    if (core_getSol(sLGoferUp)) {
      locals.ramppos[0] = UP;
      locals.goferpos[0] = UP;
      core_setSw(swLGoferDown,0);
      core_setSw(swLRampDown,0);
    }
    if (core_getSol(sRGoferUp)) {
      locals.ramppos[1] = UP;
      locals.goferpos[1] = UP;
      core_setSw(swRGoferDown,0);
      core_setSw(swRRampDown,0);
    }
    /* -- If gofer drop solenoid fires lower gofer --*/
    if (core_getSol(sLGoferDown)) {
      locals.goferpos[0] = DOWN;
      core_setSw(swLGoferDown,1);
    }
    if (core_getSol(sRGoferDown)) {
      locals.goferpos[1] = DOWN;
      core_setSw(swRGoferDown,1);
    }
  }
  /* -- If ramp drop solenoid fires lower ramp --*/
  if (mech & 0x04) {
    if (locals.goferpos[0] == DOWN && core_getSol(sLRampDown)) {
      locals.ramppos[0] = DOWN;
      core_setSw(swLRampDown,1);
    }
    if (locals.goferpos[1] == DOWN && core_getSol(sRRampDown)) {
      locals.ramppos[1] = DOWN;
      core_setSw(swRRampDown,1);
    }
  }
// Slam Ramp position handling
// Slam ramp is driven by a high power flipper coil and is pulsed
// with a much lower duty cycle than regular solenoids so we need
// some extra smoothing on this coil
  if (mech & 0x08) {
    if (core_getSol(sSlamRamp)) {       // if the solenoid is energised
      locals.slampos = DOWN;            // the ramp needs to be down
      locals.slamdelay = 0;             // and we reset the delay counter
    }
    else if (locals.slamdelay < 16)    // the solenoid is not energised
      locals.slamdelay++;              // and the delay has not expired
    else                               // once the delay has expired
      locals.slampos = UP;    // the ramp goes back up
  }
}


static int ngg_getSol(int solNo) {
  return wpc_data[WPC_EXTBOARD2] & (1<<(solNo - CORE_CUSTSOLNO(1)));
}

static int ngg_getMech (int mechNo) {
  switch (mechNo) {
    case 0: return locals.wheelpos;
    case 1: return locals.goferpos[0] | (locals.goferpos[1] << 1);
    case 2: return locals.ramppos[0] | (locals.ramppos[1] << 1);
    case 3: return locals.slampos;
  }
  return 0;
}

/**********************************
 Display Status of Ramp Entries
 *********************************/
static const char* showramp(int lr)
{
  if (locals.ramppos[lr]==UP )
    if (locals.goferpos[lr]==UP)
      return (lr?"Buzz":"Bud");
    else
      return "Lock";
  else
    return "Ramp";
}

