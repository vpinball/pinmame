/*******************************************************************************
 Red & Ted's Road Show (Williams, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 24, 2000

 Known Issues/Problems with this Simulator:

 None.

 Read PZ.c or FH.c if you like more help.

031003 Tom: added support for flashers 37-44 (51-58)

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Red & Ted's Road Show Simulator:
  ----------------------------------------

    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
     Q  SDTM (Drain Ball)
     W  Radio Riot Targets
     E  Buy-In Button
     R  Red's Mouth
     U  Upper Right Loop
     I  Right Outer Inlane
     S  Start City Hole
  DFGN  White/Red/Yellow/Orange Standup Targets
     H  Blast Zone Hole (Super Blast Zone/Skill Shot)
    JK  Red Standup Lower/Upper
     L  Spinner+Lock Hole
     Z  Blast Zone Targets
   XCV  Right/Left/Top Jet Bumpers
     B  Bulldozer/Ted Hit (Depends on Bulldozer Status)
     M  Left MiniFlipper Loop

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  rs_handleBallState(sim_tBallStatus *ball, int *inports);
static int rs_getSol(int solNo);
static void rs_handleMech(int mech);
static void rs_drawMech(BMTYPE **line);
static void rs_drawStatic(BMTYPE **line);
static void init_rs(void);
static int rs_getMech(int mechNo);
static const char* showtedeyepos(void);
static const char* showredeyepos(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int tedmouthPos; 	/* Ted's Mouth Position */
  int redmouthPos; 	/* Red's Mouth Position */
  int bulldozerPos;	/* Bulldozer's Position */
  int frgatePos;	/* Flying Rocks Gate Position */
  int tedeyesOC;	/* Ted's Eyes Open or Closed? */
  int tedeyesLR;	/* Ted's Eyes Left or Right?  */
  int redeyesOC;	/* Red's Eyes Open or Closed? */
  int redeyesLR;	/* Red's Eyes Left or Right?  */

} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(rs,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Ramp",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Rt Outer Inlane",	KEYCODE_I)
    COREPORT_BIT(0x0100,"Red's Mouth",		KEYCODE_R)
//    COREPORT_BIT(0x0200,"Ted's Mouth",		KEYCODE_T)
    COREPORT_BIT(0x0400,"Blast Zone Tgts",	KEYCODE_Z)
    COREPORT_BIT(0x0800,"Right Jet",		KEYCODE_X)
    COREPORT_BIT(0x1000,"Left Jet",		KEYCODE_C)
    COREPORT_BIT(0x2000,"Top Jet",		KEYCODE_V)
    COREPORT_BIT(0x4000,"Red Standup L",		KEYCODE_J)
    COREPORT_BIT(0x8000,"Red Standup U",		KEYCODE_K)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Upper Right Loop",	KEYCODE_U)
    COREPORT_BIT(0x0002,"Start City",		KEYCODE_S)
    COREPORT_BIT(0x0004,"White Standup",		KEYCODE_D)
    COREPORT_BIT(0x0008,"Red Standup",		KEYCODE_F)
    COREPORT_BIT(0x0010,"Yellow Standup",	KEYCODE_G)
    COREPORT_BIT(0x0020,"Orange Standup",	KEYCODE_N)
    COREPORT_BIT(0x0040,"Blast Zone Hole",	KEYCODE_H)
    COREPORT_BIT(0x0100,"Lock Hole",		KEYCODE_L)
    COREPORT_BIT(0x0200,"Bulldozer",		KEYCODE_B)
    COREPORT_BIT(0x0400,"Left Loop",		KEYCODE_M)
    COREPORT_BIT(0x0800,"Radio Riot Tgts",	KEYCODE_W)
    COREPORT_BIT(0x1000,"Buy-In Button",		KEYCODE_E)
    COREPORT_BIT(0x2000,"Drain",			KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTedMouth     	11
#define swDozerDown    	12
#define swStart      	13
#define swTilt       	14
#define swDozerUp	15
#define swRightOutlane	16
#define swRightInlane2	17
#define swRightInlane1	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swBuyIn     	23
#define swRedMouthH	25
#define swLeftOutlane	26
#define swLeftInlane	27
#define swBlastZoneT	28

#define swSkillShotL	31
#define swSkillShotU	32
#define swShooter	33
#define swRadioT	34
#define swRedStandupU	35
#define swRedStandupL	36
#define swHitRed	37
#define swRtLoopExit	38

#define swTroughJam	41
#define swTrough1	42
#define swTrough2	43
#define swTrough3	44
#define swTrough4     	45
#define swRtLoopEnter  	46
#define swBulldozer    	47
#define swHitTed     	48

#define swSpinner    	51
#define swLockup1	52
#define swLockup2	53
#define swLockKickout	54
#define swRRampExitL	55
#define swLRampExit	56
#define swLRampEnter	57
#define swLShooter	58

#define swLeftSling	61
#define swRightSling	62
#define swLeftJet	63
#define	swTopJet	64
#define	swRightJet	65

#define swRRampEnter   	71
#define swRRampExitC	72
#define swFRocks5XB	73
#define swFRocksRR	74
#define swFRocksEB	75
#define swFRocksTop	76
#define swUBZone	77
#define swStartCity	78

#define swWStandup   	81
#define swRStandup	82
#define swYStandup	83
#define swOStandup	84
#define swLFlipTop	85
#define swLFlipBot	86

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sTrough		1
#define sLLDiverter	2
#define sLockupPin	3
#define sULDiverter	4
#define sURDiverter  	5
#define sStartCity     	6
#define sKnocker       	7
#define sLockKickout  	8
#define sTedEyesLeft  	9
#define sTedLidsDown	10
#define sTedLidsUp	11
#define sTedEyesRight	12
#define sRedLidsDown	13
#define sRedEyesLeft	14
#define sRedLidsUp	15
#define sRedEyesRight	16
#define sRedMouthMotor	17
#define sRedMotorDrv	18
#define sTedMotorDrv	19
#define sTedMouthMotor	20
#define sLeftSling	21
#define sRightSling	22
#define sBulldozer	23
#define sRedEject	24
#define sTopJet		25
#define sLeftJet	26
#define sRightJet	27
#define sShakerMotor	28

/* Bulldozer Open/Close timer */
#define BULLDOZER_TICKS 250
/*Status of Eyes*/
enum {REYES_ST=0,REYES_LEFT,REYES_RIGHT,REYES_CLOSED,
      TEYES_ST=0,TEYES_LEFT,TEYES_RIGHT,TEYES_CLOSED};

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane1, stLeftInlane, stLeftSling, stRightSling,
      stEnterLRamp, stLRampMade, stRightReel, stLeftReel,
      stEnterRRamp, stRRampMade, stCenterReel, stToMiniFlip, stBlastLit,
      stWStandup, stRStandup, stYStandup, stOStandup, stRedStandupU, stRedStandupL, stBulldozer, stRightInlane2, stRadioT, stBlastZoneT, stLeftJet, stRightJet, stTopJet,
      stURightLoop, stURLoopExit, stLJet, stTJet, stRJet, stSkillUp, stSkillDown, stLeftLoop, stLeftLoop2,
      stSpinner, stLockup2, stLockup1, stLockKickout, stSpinner2,
      stBlastZoneHole, stStartCity, stSkillShot, stRightLane2, stRightLane3,
      stHitRed, stRedMouth, stRedGulp, stHitTed, stTedMouth, stTedGulp, stDozerHit,
      stLShooter, stLBallLane, stNoStrength, st5XBlast, stRRiot, stEB, stHelmet, stTopLReel, stLeftInlane2
	  };

static sim_tState rs_stateDef[] = {
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
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		1,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane1,0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},

  /*Line 3*/
  {"Enter L. Ramp",	1,swLRampEnter,	 0,		stLRampMade,	5},
  {"L. Ramp Made",	1,swLRampExit,	 0,		stRightReel,	3,sULDiverter,stLeftReel,2},
  {"R. Habitrail",	1,0,		 0,		stRightInlane1,	18},
  {"L. Habitrail",	1,0,		 0,		stLeftInlane2, 	12},

  /*Line 4*/
  {"Enter R. Ramp",	1,swRRampEnter,	 0,		stRRampMade,	5},
  {"R. Ramp Made",	1,0,		 0,		stCenterReel,	2,sURDiverter,stToMiniFlip,10},
  {"C. Habitrail",	1,swRRampExitC,	 0,		stRightInlane1,	14},
  {"To Mini Flip",	1,swRRampExitL,	 0,		stBlastLit, 	10},
  {"S. Blast Lit",	1,swLFlipBot,	 0,		stFree,		5},

  /*Line 5*/
  {"White Target",	1,swWStandup,	 0,		stFree,		2},
  {"Red Target",	1,swRStandup,	 0,		stFree,		2},
  {"Yellow Target",	1,swYStandup,	 0,		stFree,		2},
  {"Orange Target",	1,swOStandup,	 0,		stFree,		2},
  {"Red's Upp Tgt",	1,swRedStandupU, 0,		stFree,		2},
  {"Red's Low Tgt",	1,swRedStandupL, 0,		stFree,		2},
  {"Bulldozer",		0,0,		 0,		0,		0},
  {"R.Outer Inlane",	1,swRightInlane2,0,		stFree,		2},
  {"Radio Riot Tgt",	1,swRadioT,	 0,		stFree,		2},
  {"Blast Zone Tgt",	1,swBlastZoneT,	 0,		stFree,		2},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		2},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		2},
  {"Top Bumper",	1,swTopJet,	 0,		stFree,		2},

  /*Line 6*/
  {"Upper Rt. Loop",	1,swRtLoopEnter, 0,		stURLoopExit,	6},
  {"U.R. Loop Exit",	1,swRtLoopExit,	 0,		stLJet,		3},
  {"Left Jet",		1,swLeftJet,	 0,		stTJet,		2},
  {"Top Jet",		1,swTopJet,	 0,		stRJet,		2},
  {"Right Jet",		1,swRightJet,	 0,		stFree,		2},
  {"Right Lane",	1,swSkillShotU,	 0,		stSkillDown,	2},
  {"Down Rt. Lane",	1,swSkillShotL,	 0,		stRightInlane2, 3},
  {"Left Mini Loop",	1,swLFlipTop,	 0,		stLeftLoop2,	3},
  {"Lt. Mini Sw #2",	1,swLFlipBot,	 0,		stFree,		3},

  /*Line 7*/
  {"Spinner",		1,swSpinner,	 0,		stLockup2,	3,0,0,SIM_STSPINNER},
  {"Lock 2",		1,swLockup2,	 0,		stLockup1,	1},
  {"Lock 1",		1,swLockup1,	 sLockupPin,	stLockKickout,	1},
  {"Lock Kickout",	1,swLockKickout, sLockKickout,   stSpinner2,	3},
  {"Spinner",		1,swSpinner,	 0,		stFree,		3},

  /*Line 8*/
  {"B.Zone/Sk.Shot",	1,swUBZone,	 0,		stStartCity,	3},
  {"Start City H.",	1,swStartCity,	 sStartCity,	stFree,		0},
  {"Right Lane",	1,swSkillShotU,	 0,		stBlastZoneHole,5},
  {"Right Lane",	1,swSkillShotU,	 0,		stLJet,		3},
  {"Right Lane",	1,swSkillShotU,	 0,		stBulldozer,	1},

  /*Line 9*/
  {"Hit Red",		0,0,		 0,		0,		0},
  {"Red's Mouth",	1,swHitRed,	 0,		stFree,		5},
  {"Red's Gulp",	1,swRedMouthH,	 sRedEject,	stFree,		0},
  {"Hit Ted",		0,0,		 0,		0,		0},
  {"Ted's Mouth",	1,swHitTed,	 0,		stFree,		5},
  {"Ted's Gulp",	1,swTedMouth,	 0,		stLockup2,	7},
  {"Bulldozer Hit",	1,swBulldozer,	 0,		stFree,		2},

  /*Line10*/
  {"L. Shooter",	1,swLShooter,	 sShooterRel,	stLBallLane,	0,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
  {"L. Ball Lane",	1,0,		 0,		0,		0,0,0,SIM_STNOTEXCL},
  {"No Strength",	1,swLShooter,	 0,		stLShooter,	3},
  {"Award 5X Blast",	1,swFRocks5XB,	 0,		stBlastLit,	5},
  {"Award R. Riot",	1,swFRocksRR,	 0,		stLeftReel,	5},
  {"Ex. Ball Lane",	1,swFRocksEB,	 0,		stTopLReel,	5},
  {"Rudy's Helmet",	1,swFRocksTop,	 0,		stTopLReel,	8},
  {"T.L. Habitrail",	1,0,		 0,		stRightReel,	7},
  {"Left Inlane",	1,0,		 0,		0,		0},

  {0}
};

static int rs_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Right Shooter Lane */
    	case stBallLane:
		if (ball->speed < 10)
			return setState(stNotEnough,10);	/*Ball not plunged hard enough*/
		if (ball->speed < 20)
			return setState(stSkillUp,20);		/*Ball comes down from the right*/
		if (ball->speed < 30)
			return setState(stSkillShot,25);	/*Ball entered Skill Shot hole*/
		if (ball->speed < 40)
			return setState(stRightLane2,35);	/*Ball goes to the Jets*/
		if (ball->speed < 51)
			return setState(stRightLane3,51);	/*Ball hits Bulldozer*/
		break;

	/* Ball in Left Shooter Lane */
    	case stLBallLane:
		if (ball->speed < 15)
			return setState(stNoStrength,30);	/*Ball not plunged hard enough*/
		if (ball->speed < 25)
			return setState(st5XBlast,50);		/*Ball landed in 5X Blast Hole*/
		if (ball->speed < 35)
			return setState(stRRiot,70);		/*Ball landed in Radio Riot Hole*/
		if (ball->speed < 45)
			return setState(stEB,70);		/*Ball landed in Extra Ball Lit Hole*/
		if (ball->speed < 51)
			return setState(stHelmet,70);		/*Ball Hits the Ted's Helmet*/
		break;

	/* Red Hit */
    	case stHitRed:
		/*Is Red's Mouth Open?*/
		if (locals.redmouthPos)
			return setState(stRedMouth,10);		/*Ball hits Red's Jaw*/
		else
			return setState(stRedGulp,10);		/*Ball goes into Red's mouth*/
		break;


	/* Bulldozer */
	case stBulldozer:
		/*Is the Bulldozer Open?*/
		if (core_getSw(swDozerDown))
			return setState(stDozerHit,10);
		else
			return setState(stHitTed,10);

	/* Ted Hit */
    	case stHitTed:
		/*Is Ted's Mouth Open?*/
		if (!locals.tedmouthPos)
			return setState(stTedMouth,10);		/*Ball hits Ted's Jaw*/
		else
			return setState(stTedGulp,10);		/*Ball goes into Ted's mouth*/
		break;

	/* Left Inlane - StFree or Go to Left Shooter? */
	case stLeftInlane2:
		if (locals.frgatePos)
			{
			ball->speed = -1;
			return setState(stLShooter,5);
			}
		else
			return setState(stLeftInlane,5);
		break;

       }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData rs_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterLRamp},
  {0, 0x0006, stEnterRRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane1},
  {0, 0x0040, stRightInlane2},
  {0, 0x0100, stHitRed},
//  {0, 0x0200, stHitTed},
  {0, 0x0400, stBlastZoneT},
  {0, 0x0800, stRightJet},
  {0, 0x1000, stLeftJet},
  {0, 0x2000, stTopJet},
  {0, 0x4000, stRedStandupL},
  {0, 0x8000, stRedStandupU},

/* Port 1 */
  {1, 0x0001, stURightLoop},
  {1, 0x0002, stStartCity},
  {1, 0x0004, stWStandup},
  {1, 0x0008, stRStandup},
  {1, 0x0010, stYStandup},
  {1, 0x0020, stOStandup},
  {1, 0x0040, stBlastZoneHole},
  {1, 0x0100, stSpinner},
  {1, 0x0200, stBulldozer},
  {1, 0x0400, stLeftLoop},
  {1, 0x0800, stRadioT},
  {1, 0x1000, swBuyIn,SIM_STANY},
  {1, 0x2000, stDrain},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay rs_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{25,12,YELLOW}}},{1,{{26,13,YELLOW}}},{1,{{24,13,WHITE}}},{1,{{24,15,WHITE}}},
{1,{{26,15,YELLOW}}},{1,{{26,21,WHITE}}},{1,{{26,18,WHITE}}},{1,{{25,16,WHITE}}},
{1,{{28, 9,ORANGE}}},{1,{{27,11,ORANGE}}},{1,{{28,13,ORANGE}}},{1,{{29,15,ORANGE}}},
{1,{{30,17,ORANGE}}},{1,{{27,16,YELLOW}}},{1,{{28,18,YELLOW}}},{1,{{31,20,ORANGE}}},
{1,{{26, 8,YELLOW}}},{1,{{23, 9,WHITE}}},{1,{{30, 1,RED}}},{1,{{22, 4,WHITE}}},
{1,{{39,14,ORANGE}}},{1,{{30,28,RED}}},{3,{{23,24,WHITE},{24,25,WHITE},
{25,26,WHITE}}},{1,{{17,25,WHITE}}},{1,{{13,14,LBLUE}}},{1,{{12,16,YELLOW}}},
{2,{{14,18,WHITE},{15,17,WHITE}}},{1,{{11,17,WHITE}}},{1,{{ 5,21,RED}}},
{1,{{ 9,18,ORANGE}}},{1,{{ 7,17,YELLOW}}},{1,{{ 6,16,WHITE}}},{1,{{15,10,YELLOW}}},
{1,{{12,12,WHITE}}},{1,{{11,11,YELLOW}}},{1,{{ 9,10,ORANGE}}},{1,{{ 6,12,YELLOW}}},
{1,{{ 7,11,WHITE}}},{1,{{ 6, 8,WHITE}}},{1,{{ 5,14,RED}}},{1,{{17,14,YELLOW}}},
{1,{{16,12,YELLOW}}},{1,{{18,16,YELLOW}}},{1,{{19,18,ORANGE}}},{1,{{ 6, 3,YELLOW}}},
{1,{{ 8, 4,ORANGE}}},{1,{{ 1,20,WHITE}}},{1,{{ 3,19,YELLOW}}},{1,{{34,18,ORANGE}}},
{1,{{35,16,ORANGE}}},{1,{{36,14,ORANGE}}},{1,{{35,12,ORANGE}}},{1,{{34,10,ORANGE}}},
{1,{{34,14,RED}}},{1,{{21,21,YELLOW}}},{1,{{19,24,YELLOW}}},{1,{{25, 0,YELLOW}}},
{1,{{16, 0,YELLOW}}},{1,{{ 6, 0,RED}}},{1,{{ 0, 1,RED}}},{1,{{ 1, 3,YELLOW}}},
{1,{{ 0,25,RED}}},{1,{{39,26,ORANGE}}},{1,{{39, 0,YELLOW}}}
}
};

  static void rs_drawMech(BMTYPE **line) {
//  core_textOutf(30,  0,BLACK,"Ted's /Red's Mouth:");
//  core_textOutf(30, 10,BLACK,"%-6s/%-6s", locals.tedmouthPos?"Open":"Closed", locals.redmouthPos?"Closed":"Open");
//  core_textOutf(30, 20,BLACK,"Ted's   /Red's Eyes:");
//  core_textOutf(30, 30,BLACK,"%-8s/%-8s", showtedeyepos(), showredeyepos());
//  core_textOutf(30, 40,BLACK,"Bulldozer: %-6s", core_getSw(swDozerDown)?"Closed":"Open");
  core_textOutf(30,  0,BLACK,"Ted/Red's Mouth, Eyes:");
  core_textOutf(30, 10,BLACK,"%-8s/%-6s,", locals.tedmouthPos?"Open":"Closed", locals.redmouthPos?"Closed":"Open");
  core_textOutf(30, 20,BLACK,"%-8s/%-8s", showtedeyepos(), showredeyepos());
  core_textOutf(30, 30,BLACK,"Bulldozer: %-6s", core_getSw(swDozerDown)?"Closed":"Open");
}
/* Help */

  static void rs_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W = Radio Riot Tgts.");
  core_textOutf(30,100,BLACK,"E = Buy-In Button, R = Red's Mouth");
  core_textOutf(30,110,BLACK,"I = R.Outer Inlane, U = Upper R.Loop");
  core_textOutf(30,120,BLACK,"S = Start City Hole, H = Blast Zone H.");
  core_textOutf(30,130,BLACK,"D/F/G/N = White/Red/Yellow/Orange Tgt.");
  core_textOutf(30,140,BLACK,"J/K/Z = Red's Lower/Upper/B.Zone Tgts");
  core_textOutf(30,150,BLACK,"L = Spinner+Lock, B = Bulldozer/Ted");
  core_textOutf(30,160,BLACK,"X/C/V = Jet Bumpers, M = Lt. MiniLoop");
 }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap rs_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sTopJet,1,SAM_JET3}, {sLLDiverter,1,SAM_DIVERTER},
 {sLLDiverter,1,SAM_SOLENOID_ON,WPCSAM_F_ONOFF},

 /*Channel #2*/
 {sLeftSling,2,SAM_LSLING}, {sRightSling,2,SAM_RSLING},
 {sULDiverter,2,SAM_DIVERTER}, {sULDiverter,2,SAM_SOLENOID_ON,WPCSAM_F_ONOFF},
 {sURDiverter,2,SAM_DIVERTER}, {sURDiverter,2,SAM_SOLENOID_ON,WPCSAM_F_ONOFF},

 /*Channel #3*/
 {sStartCity,3,SAM_POPPER}, {sLockKickout,3,SAM_POPPER},
 {sRedEject,3,SAM_SOLENOID_ON},

 /*Channel #4*/
 {sBulldozer,4,SAM_MOTOR_1,WPCSAM_F_CONT},{-1}
/* We need a Shaker motor sample!
 {sShakerMotor,4,SAM_SHAKER_MOTOR,WPCSAM_F_CONT},{-1}
*/

};

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(rs,l6,"rshw_l6.rom",0x80000,CRC(3986d402) SHA1(1a67e5bafb7a6aa1d42b2e631e2294a3c1403038))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND

WPC_ROMSTART(rs,la5,"u6_la5.rom",0x80000,CRC(61e63268) SHA1(79e32f489c51e7e79e892d36f586af14ab9aa2a5))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND
WPC_ROMSTART(rs,lx5,"u6_lx5.rom",0x80000,CRC(a2de6ee3) SHA1(90fea1100d5f79c885e693d713b9a113d43131bb))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND

WPC_ROMSTART(rs,la4,"u6_la4.rom",0x80000,CRC(d957a038) SHA1(bd78b62eda2046a72eaaee2fff973fe3589f7d88))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND

WPC_ROMSTART(rs,lx4,"rshw_lx4.rom",0x80000,CRC(866f16a5) SHA1(09180ca87b1b4a9f8f81d81fc2d08092f357205a))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND
WPC_ROMSTART(rs,lx3,"u6-lx3.rom",0x80000,CRC(5df17d02) SHA1(94b262c91f906d68d2a6ee9432042a202bf04d35))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND
WPC_ROMSTART(rs,lx2,"rshw_lx2.rom",0x80000,CRC(317210d0) SHA1(38adcf9c72552bd371b096080b172c63d0f843d3))
DCS_SOUNDROM7x("rs_u2_s.l1",CRC(5a2db20c) SHA1(34ce236cc874b820db2d2268cc77815ed7ca061b),
               "rs_u3_s.l1",CRC(719be036) SHA1(fa975a6a93fcaefddcbd1c0b97c49bd9f9608ad4),
               "rs_u4_s.l1",CRC(d452d007) SHA1(b850bc8e17d8940f93c1e7b6a0ab786b092694b3),
               "rs_u5_s.l1",CRC(1faa04c9) SHA1(817bbd7fc0781d84af6c40cb477adf83cef07ab2),
               "rs_u6_s.l1",CRC(eee00add) SHA1(96d664ca73ac896e918d7011c1cda3e55e3731b7),
               "rs_u7_s.l1",CRC(3a222a54) SHA1(2a788e4ac573bf1d128e5bef9357e62c805014b9),
               "rs_u8_s.l1",CRC(c70f2210) SHA1(9be9f271d81d15a4eb123f1377b0c077eef97774))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(rs,l6,"Red and Ted's Road Show (L-6)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,la5,l6,"Red and Ted's Road Show (La-5)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,lx5,l6,"Red and Ted's Road Show (Lx-5)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,la4,l6,"Red and Ted's Road Show (La-4)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,lx4,l6,"Red and Ted's Road Show (Lx-4)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,lx3,l6,"Red and Ted's Road Show (Lx-3)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(rs,lx2,l6,"Red and Ted's Road Show (Lx-2)",1994,"Williams",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData rsSimData = {
  2,    				/* 2 game specific input ports */
  rs_stateDef,				/* Definition of all states */
  rs_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  rs_handleBallState,			/*Function to handle ball state changes*/
  rs_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData rsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U), /* actually 3 left flippers */
    0,0,8,0,0,0,0,
    rs_getSol, rs_handleMech, rs_getMech, rs_drawMech,
    &rs_lampPos, rs_samsolmap
  },
  &rsSimData,
  {
    "524 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x12, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_rs(void) {
  core_gameData = &rsGameData;
}

static int rs_getSol(int solNo) {
  if ((solNo >= CORE_CUSTSOLNO(1)) && (solNo <= CORE_CUSTSOLNO(8)))
    return ((wpc_data[WPC_EXTBOARD1]>>(solNo-CORE_CUSTSOLNO(1)))&0x01);
  return 0;
}

static void rs_handleMech(int mech) {
  /* ------------------------------
     --	Open & Close Ted's Mouth --
     ------------------------------ */
  if (mech & 0x01) {
    /* -- If open, and Mouth Solenoid fired and !UpDown fired - Close it! -- */
    if (locals.tedmouthPos && core_getSol(sTedMouthMotor) && !core_getSol(sTedMotorDrv))
      locals.tedmouthPos = 0;
    /* -- If closed, and Mouth Solenoid fired and UpDown fired - Open it! -- */
    if (!locals.tedmouthPos && core_getSol(sTedMouthMotor) && core_getSol(sTedMotorDrv))
      locals.tedmouthPos = 1;
  }
    /* ------------------------------
       --	Open & Close Red's Mouth --
       ------------------------------ */
  if (mech & 0x02) {
    /* -- If open, and Mouth Solenoid fired and !UpDown fired - Close it! -- */
    if (locals.redmouthPos && core_getSol(sRedMouthMotor) && !core_getSol(sRedMotorDrv))
      locals.redmouthPos = 0;
    /* -- If closed, and Mouth Solenoid fired and UpDown fired - Open it! -- */
    if (!locals.redmouthPos && core_getSol(sRedMouthMotor) && core_getSol(sRedMotorDrv))
      locals.redmouthPos = 1;
  }
  /* -------------------------------
     --	Open & Close Bulldozer    --
     ------------------------------- */
  if (mech & 0x04) {
    if (core_getSol(sBulldozer))
      locals.bulldozerPos = (locals.bulldozerPos + 1) % BULLDOZER_TICKS;
    core_setSw(swDozerUp, locals.bulldozerPos < 15);
    core_setSw(swDozerDown, (locals.bulldozerPos >= (BULLDOZER_TICKS/2)) &&
                            (locals.bulldozerPos <  (BULLDOZER_TICKS/2+15)));
  }

  /* ------------------------------------
     --	Open & Close Flying Rocks Gate --
     ------------------------------------ */
  /* -- If the Lower Left Solenoid fires, open it till the Left Shooter Switch is pressed --*/
  if (mech & 0x08) {
    if (core_getSol(sLLDiverter))
      locals.frgatePos = 1;
    if (core_getSw(swLShooter))
      locals.frgatePos = 0;
  }
  /* ----------------------------------
     -- Update status of Ted's Eyes --
     ---------------------------------- */
  if (mech & 0x10) {
    /* -- If Eyes are Open, and Closed Solenoid Fired, close them! --*/
    if (locals.tedeyesOC == 0 && core_getSol(sTedLidsDown))
      locals.tedeyesOC = 1;
    /* - If Eyes are Closed, and Open Solenoid Fired, open them! -*/
    if (locals.tedeyesOC == 1 && core_getSol(sTedLidsUp))
      locals.tedeyesOC = 0;
  }
  /* -- Now check position of Eyes -- */
  if (mech & 0x20) {
    if (core_getSol(sTedEyesLeft))
      locals.tedeyesLR = TEYES_LEFT;
    else
      if (core_getSol(sTedEyesRight))
        locals.tedeyesLR = TEYES_RIGHT;
      else
        locals.tedeyesLR = TEYES_ST;
  }
  /* ----------------------------------
     -- Update status of Red's Eyes --
     ---------------------------------- */
  if (mech & 0x40) {
    /* -- If Eyes are Open, and Closed Solenoid Fired, close them! --*/
    if (locals.redeyesOC == 0 && core_getSol(sRedLidsDown))
      locals.redeyesOC = 1;
    /* -- If Eyes are Closed, and Open Solenoid Fired, open them! -- */
    if (locals.redeyesOC == 1 && core_getSol(sRedLidsUp))
      locals.redeyesOC = 0;
  }
  if (mech & 0x80) {
    /* -- Now check position of Eyes  -- */
    if (core_getSol(sRedEyesLeft))
      locals.redeyesLR = REYES_LEFT;
    else
      if (core_getSol(sRedEyesRight))
        locals.redeyesLR = REYES_RIGHT;
      else
        locals.redeyesLR = REYES_ST;
  }
}
static int rs_getMech(int mechNo) {
  switch (mechNo) {
    case 0: return locals.tedmouthPos;
    case 1: return locals.redmouthPos;
    case 2: return locals.bulldozerPos;
    case 3: return locals.frgatePos;
    case 4: return locals.tedeyesOC;
    case 5: return locals.tedeyesLR;
    case 6: return locals.redeyesOC;
    case 7: return locals.redeyesLR;
  }
  return 0;
}
/*--------------------------------
    Display Status of Ted's Eyes
  --------------------------------*/
static const char* showtedeyepos(void)
{
if(locals.tedeyesOC)
	return "Closed";
else
	{
	switch(locals.tedeyesLR)
		{
		case TEYES_ST: return "Straight";
		case TEYES_LEFT: return "Left";
		case TEYES_RIGHT: return "Right";
		default: return "?";
		}
	}
}

/*---------------------------------
    Display Status of Red's Eyes
  ---------------------------------*/
static const char* showredeyepos(void)
{
if(locals.redeyesOC)
	return "Closed";
else
	{
	switch(locals.redeyesLR)
		{
		case REYES_ST: return "Straight";
		case REYES_LEFT: return "Left";
		case REYES_RIGHT: return "Right";
		default: return "?";
		}
	}
}
