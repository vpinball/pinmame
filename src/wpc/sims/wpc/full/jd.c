/*******************************************************************************
 Judge Dredd

 by Steve Ellenoff (sellenoff@hotmail.com)
 Dec. 1, 2000

 Never played this game before, but it sounds really cool from the rulesheet! :)
 Much thanks goes to "ari 'apz' sovijärvi" <apz@nic.fi> for answering all my questions
 about the game, since he owns the machine!

 Known Issues-
 Well, I'm not sure if the arm and globe work 100% right, but they seem ok..
 Some timing issues maybe.. the diverter doesn't always send the ball to the planet on lock3!
 Drop Target issues.. not sure they are working 100% correctly.. need someone to confirm!

 If you want help on writing your own seem, see FH.C or PZ.C!

 ******************************************************************************/

/*-------------------------------------------------------------------------------------------------
  Keys for Judge Dredd Simulator:
  ----------------------------
     2  	Buy-In
     E  	Super Game
 L.ALT  	Left FIRE Button
 SPACE  	Right FIRE Button
    +R		L/R Ramp
     R		Middle Ramp
    +I		L/R Inlane
     I  	Inner Right Inlane
    +-		L/R Slingshot
    +L		L/R Loop
    +O		L/R Outlane
     Q		SDTM (Drain Ball)
     A		Air Raid Ramp
     T  	Left Shooter Lane
     Y		Small Loop
     U		Sniper Tower
     H		Advance Crime 3 Bank Target
     S		Subway
     D		EB Left Target
     F		EB Right Target
     G		"?" Target
 ZXCVB		JUDGE Targets
     W		Captive Ball

-------------------------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"


static void init_jd(void);
static void jd_drawMech(BMTYPE **line);
static void jd_drawStatic(BMTYPE **line);
static int  jd_handleBallState(sim_tBallStatus *ball, int *inports);
static void jd_handleMech(int mech);
static int jd_getMech(int mechNo);
static int jd_getSol(int solNo);
static int jd_keyCond(int cond, int ballState, int *inports);
static char * DispDT(void);
static char * DispPlanet(void);
static char * DispClaw(void);
/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int droptargetPos;     /* Drop Target Position */
  int armPos;		 /* Arm Position  */
  int globePos;		 /* Globe Position*/
  int jdtPos;		 /* "J"udge DT Position*/
  int udtPos;		 /* J"u"dge DT Position*/
  int ddtPos;		 /* Ju"d"ge DT Position*/
  int gdtPos;		 /* Jud"g"e DT Position*/
  int edtPos;		 /* Judg"e" DT Position*/
  } locals;

WPC_INPUT_PORTS_START(jd,6)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"L/R Inlane",      KEYCODE_I)
    COREPORT_BIT(   0x0100,"Drain",		     KEYCODE_Q)
    COREPORT_BIT(   0x0200,"Left Fire",	     KEYCODE_LALT)
    COREPORT_BIT(   0x0400,"Buy In",		     KEYCODE_2)
    COREPORT_BIT(   0x0800,"Super Game",	     KEYCODE_E)
    COREPORT_BIT(   0x1000,"Air Rade Ramp",   KEYCODE_A)
    COREPORT_BIT(   0x2000,"L.Shooter",       KEYCODE_T)
    COREPORT_BIT(   0x4000,"Small Loop",      KEYCODE_Y)
    COREPORT_BIT(   0x8000,"Sniper Tower",    KEYCODE_U)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Subway",			 KEYCODE_S)
    COREPORT_BIT(0x0002,"Adv Crime DT",	     KEYCODE_H)
    COREPORT_BIT(0x0004,"EB Left DT",	     KEYCODE_D)
    COREPORT_BIT(0x0008,"EB Right DT",	     KEYCODE_F)
    COREPORT_BIT(0x0010,"'?' Target",	     KEYCODE_G)
    COREPORT_BIT(0x0020,"Captive Ball",	     KEYCODE_W)
    COREPORT_BIT(0x0040,"'J'udge",			 KEYCODE_Z)
    COREPORT_BIT(0x0080,"J'u'dge",			 KEYCODE_X)
    COREPORT_BIT(0x0100,"Ju'd'ge",			 KEYCODE_C)
    COREPORT_BIT(0x0200,"Jud'g'e",			 KEYCODE_V)
    COREPORT_BIT(0x0400,"Judg'e'",			 KEYCODE_B)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swLFire			11
#define swRFire			12
#define swStart      	13
#define swTilt       	14
#define swLShooter		15
#define swLOut			16
#define swLIn			17
#define sw3Bank			18		//3 Bank Target

#define swSlamTilt		21
#define swCoinDoor		22
#define swTicket     	23
#define swAlwaysClosed	24
#define swTRPost		25		//Right EB Target
#define swCapBall1		26
#define swLLTarget		27		// Mystery '?' Target
/*28 Not used*/

#define swBuyIn			31
/*32 Not Used */
#define swTCRollOver	33		//Left Orbit Shot..
#define swIRIn			34
#define swSLoopC		35		//Small Loop Switch!
#define swLPost			36		//Left EB Target
#define swSubway		37
#define swSubway2		38

#define swRShooter		41
#define swROut			42
#define swRIn			43
#define swSuperGame		44
/*45 - 48 Not used */

#define swLSling		51
#define swRSling		52
#define swCapBall2		53
#define sw_Judge		54
#define swJ_udge		55
#define swJu_dge		56
#define swJud_ge		57
#define swJudg_e		58

#define swGlobe1		61		//?
#define swGlobeExit		62		//Fires when ball is dropped from Magnet!
#define swLRampToLock	63		//Switch towards planet!
#define swLRampExit		64
/*65 Not Used*/
#define swCRampExit		66
#define swLRampEnt		67
#define swCapBall3		68

#define swArmFR			71		//Magnet Over Ring - set when magnet ready to pick up ball!
#define swTROpto		72		//Right Orbit Shot
#define swLPopper		73		//Subway Popper
#define swRPopper		74		//Sniper Target
#define swTRRampExit	75		//Air Rade Ramp
#define swRRampExit		76
#define swGlobe2		77		//Planet - Indicates ball can be lifted!
/*78 Not used*/

#define swTrough6		81
#define swTrough5		82
#define swTrough4		83
#define swTrough3		84
#define swTrough2		85
#define swTrough1		86
#define swTroughTop		87
/*88 Not Used*/

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sArmMagnet		1		//Keeps ball in arm
#define sLPopper		2		//Subway Popper
#define sRPopper		3		//Sniper Popper
#define sGlobeArm		4		//Arm Motor
#define sDropTarget		5		//Knock All Drop Targets Up!
#define sGlobeMotor		6		//Spins Planet
#define sKnocker		7
#define sRShooter		8
#define sLShooter		9
#define sTripDropTarget		10		//Pull "D" Drop Target Down
#define sDiverter		11		//Left Ramp Diverter to Planet
/*12 - Not Used*/
#define sTrough			13
/*14 - Not Used*/
#define sLSling			15
#define sRSling			16

/*Define fake solenoid to signal when ball should be droped from claw*/
#define sFakeSol1		CORE_CUSTSOLNO(1) /* 33 */

/*Define custom key condition*/
#define JD_DROPCOND 1

/*Drop Target Positions*/
#define DT_UP  0
#define DT_DOWN  1


enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stTrough,
      stRShooter, stBallLane, stLShooter, stDrain, stLRampEnt,stRRampEnt, stCRampEnt, stLRampExit,
      stLOut, stROut, stLIn, stRIn, stLLoopUp, stLLoopDn, stRLoopUp, stRLoopDn,
      stIRIn, stSmallLoop, stSniper, stSniper2, stSubway, stSubway2, stSubway3, stSubway4,
      stARampEnt, stAHabit, stLRamp2, stPlanet, stPlanet2, stPlanet3,
      stEBLeft, stEBRight, stCaptiveBall, stCBall2, stCBall3, stMystery,
      st_Judge, stJ_udge, stJu_dge, stJud_ge, stJudg_e,
      st_Judge2, stJ_udge2, stJu_dge2, stJud_ge2, stJudg_e2
      };

static sim_tState jd_stateDef[] =  {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",               0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 1",          1,swTrough1,    sTrough,	stRShooter,  1},
  {"Trough 2",			1,swTrough2,	0,			stTrough1,   1},
  {"Trough 3",			1,swTrough3,	0,			stTrough2,   1},
  {"Trough 4",			1,swTrough4,	0,			stTrough3,   1},
  {"Trough 5",			1,swTrough5,	0,			stTrough4,   1},
  {"Trough 6",			1,swTrough6,	0,			stTrough5,   1},
  {"Trough Up",			1,swTroughTop,  0,          stRShooter,  1},

  /*Line 2*/
  {"R.Shooter",	    	1,swRShooter,   sRShooter,  stBallLane,  1},
  {"Ball Lane",			1,0,			0,			stLLoopDn,	10},
  {"L. Shooter",		1,swLShooter,	sLShooter,  stJu_dge,    10}, //Assume it hits the Middle of the Judge targets everytime!
  {"Drain",				1,0,            0,          stTrough6,   0,0,0,SIM_STNOTEXCL},
  {"Left Ramp",         1,swLRampEnt,   0,          stLRampExit, 5, sDiverter, stLRamp2},
  {"Right Ramp",		1,swRRampExit,	0,			stIRIn,		10},
  {"Center Ramp",		1,swCRampExit,		0,			stFree,		10},
  {"Left Ramp Exit",	1,swLRampExit,  0,			stRIn,	    10},

  /*Line 3*/
  {"Left Outlane",      1,swLOut,       0,			stDrain,   10},
  {"Right Outlane",     1,swROut,       0,			stDrain,   10},
  {"Left  Inlane",		1,swLIn,        0,			stFree,     5},
  {"Right Inlane",		1,swRIn,        0,			stFree,     5},
  {"Left Orbit Loop",	1,swTCRollOver,	0,			stRLoopDn,	5}, /*Loop Going Up*/
  {"Left Orbit Loop",	1,swTCRollOver,	0,			stFree,		5}, /*Loop Coming Down*/
  {"Right Orbit Loop",	1,swTROpto,		0,			stLLoopDn,	5}, /*Loop Going Up*/
  {"Right Orbit Loop",	1,swTROpto,		0,			stFree,		5}, /*Loop Coming Down*/

  /*Line 4*/
  {"I. Right Inlane",	1,swIRIn,		0,			stFree,		5},
  {"Small Loop",		1,swSLoopC,		0,			stRLoopDn,	5},
  {"Sniper",			1,swRPopper,	sRPopper,	stSniper2,	1},
  {"Sniper Habitrail",  1,0,			0,			stLIn,		10},
  {"Subway",			1,swSubway,		0,			stSubway2,	5},
  {"Subway",			1,swSubway2,		0,			stSubway3,	5},
  {"Subway Popper",		1,swLPopper,	sLPopper,	stSubway4,	1},
  {"Subway Habitrail",	1,0,			0,			stFree,		10},

  /*Line 5*/
  {"Air Raid Ramp",		1,swTRRampExit,	0,			stAHabit,	3},
  {"Air Raid Habitrail",1,0,			0,			stLShooter, 15},
  {"Towards Planet",	1,swLRampToLock,	0,			stPlanet,	5},
  {"Planet",		1,swGlobe1,		sArmMagnet,		stPlanet2,	5},	//Keep ball here, till Arm Magnet picks it up!
  {"In Claw",		1,0,			sFakeSol1,		stPlanet3,	1},
  {"Dropped",		1,swGlobeExit,  0,			stFree,		5},	//Dropped by Claw!

  /*Line 6*/
  {"EB Left Target",	1,swLPost,		0,			stFree,		5},
  {"EB Right Target",	1,swTRPost,		0,			stFree,		5},
  {"Captive Ball -",	1,swCapBall1,	0,			stCBall2,	5},
  {"Captive Ball /",	1,swCapBall2,	0,			stCBall3,	5},
  {"Captive Ball |",	1,swCapBall3,	0,			stFree,		5},
  {"'?' Target",		1,swLLTarget,	0,			stFree,		5},

  /*Line 7*/
  {"'J'udge Target",	1,0,			0,			0,		5},
  {"J'u'dge Target",	1,0,			0,			0,		5},
  {"Ju'd'ge Target",    1,0,                    0,                      0,		5},
  {"Jud'g'e Target",	1,0,			0,			0,		5},
  {"Judg'e' Target",	1,0,			0,			0,		5},

  /*Line 8*/
  {"'J'udge Target",	1,sw_Judge,		0,			stFree,		5, 0, 0, SIM_STSWKEEP},
  {"J'u'dge Target",	1,swJ_udge,		0,			stFree,		5, 0, 0, SIM_STSWKEEP},
  {"Ju'd'ge Target",    1,swJu_dge,             0,                      stFree,		5, 0, 0, SIM_STSWKEEP},
  {"Jud'g'e Target",	1,swJud_ge,		0,			stFree,		5, 0, 0, SIM_STSWKEEP},
  {"Judg'e' Target",	1,swJudg_e,		0,			stFree,		5, 0, 0, SIM_STSWKEEP},

  {0}
};

static int  jd_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	case st_Judge:
		/*If "J" target is down, go to Immediately, no switch is triggered!*/
		if(locals.jdtPos == DT_UP)
			return setState(st_Judge2,5);
		else
			return setState(stFree,5);
		break;
	case stJ_udge:
		/*If "U" target is down, go to Immediately, no switch is triggered!*/
		if(locals.udtPos == DT_UP)
			return setState(stJ_udge2,5);
		else
			return setState(stFree,5);
		break;
	//*SPECIAL CASE*//
	case stJu_dge:
		/*If "D" target is down, go to subway, otherwise, stay free*/
		if(locals.ddtPos == DT_UP)
			return setState(stJu_dge2,5);
		else
			return setState(stSubway,5);
		break;
	case stJud_ge:
		/*If "J" target is down, go to Immediately, no switch is triggered!*/
		if(locals.gdtPos == DT_UP)
			return setState(stJud_ge2,5);
		else
			return setState(stFree,5);
		break;
	case stJudg_e:
		/*If "E" target is down, go to Immediately, no switch is triggered!*/
		if(locals.edtPos == DT_UP)
			return setState(stJudg_e2,5);
		else
			return setState(stFree,5);
		break;
  	}
  return 0;
}

/* The Subway can only be reached if "D" drop target down! */
static int jd_keyCond(int cond, int ballState, int *inports) {
  switch (cond) {
    case JD_DROPCOND:
      /* ball on PF & bank down & drop up */
      return (ballState == stFree) && (locals.ddtPos == DT_DOWN);
  }
  return FALSE;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------
--------------------------------------------------------------------------------------------------
  Layout of inportData array
   ----------------------------
   #1) Inport number
   #2) Mask for this switch (all bits must be set for switch to activate
       Only first match will be used. Placed qualified keys first
   #3) Switch to activate or new ball state
   #4) Condition for the action to occur
                 0                  : ball in stFree state
                 switch             : ball in stFree state and switch active
                 SIM_STANY       : ball can be in any state
                 SIM_CUSTCOND(n) : call custom condition handler (keyCond)
--------------------------------------------------------------------------------------------------*/

static sim_tInportData jd_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0004, stCRampEnt},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0100, stDrain},
  {0, 0x0200, swLFire, SIM_STANY},
  {0, 0x0400, swBuyIn, SIM_STANY},
  {0, 0x0800, swSuperGame, SIM_STANY},
  {0, 0x1000, stARampEnt},
  {0, 0x2000, stLShooter},
  {0, 0x4000, stSmallLoop},
  {0, 0x8000, stSniper},

  {1, 0x0001, stSubway,SIM_CUSTCOND(JD_DROPCOND)},
  {1, 0x0002, sw3Bank},
  {1, 0x0004, stEBLeft},
  {1, 0x0008, stEBRight},
  {1, 0x0010, stMystery},
  {1, 0x0020, stCaptiveBall},
  {1, 0x0040, st_Judge},
  {1, 0x0080, stJ_udge},
  {1, 0x0100, stJu_dge},
  {1, 0x0200, stJud_ge},
  {1, 0x0400, stJudg_e},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void jd_drawMech(BMTYPE **line) {
  core_textOutf(30,  0,BLACK,"Claw:%-25s",DispClaw());
  core_textOutf(30, 10,BLACK,"Planet:%-14s",DispPlanet());
  core_textOutf(30, 20,BLACK,"Drop Targets:%-5s",DispDT());
  }
  /* Help */
  static void jd_drawStatic(BMTYPE **line) {
  core_textOutf(30, 30, BLACK,"Help on this Simulator:");
  core_textOutf(30, 40, BLACK,"2/E=Buy-In,Super Game");
  core_textOutf(30, 50,BLACK,"L.Alt/Space=L/R FIRE");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+- =L/R Sling");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+R,R = L/R/C Ramps");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+L = L/R Orbit Loops");
  core_textOutf(30, 100,BLACK,"Q = Drain Ball, I = Inner R.Inlane");
  core_textOutf(30, 110,BLACK,"A = Air Raid Ramp, S = Subway");
  core_textOutf(30, 120,BLACK,"T = L.Shooter Lane, Y = Small Loop");
  core_textOutf(30, 130,BLACK,"H = Avd Crim DT, U = Sniper Tower");
  core_textOutf(30, 140,BLACK,"D/F = Extra Ball L/R Target");
  core_textOutf(30, 150,BLACK,"G = '?' Target, W = Captive Ball");
  core_textOutf(30, 160,BLACK,"Z/X/C/V/B = JUDGE Targets");
  }

/*-----------------
/  ROM definitions
/------------------*/
#define JD_SOUND \
DCS_SOUNDROM8x( "jdsnd_u2.bin",CRC(d8f453c6) SHA1(5dd677fde46436dbf2d2e9058f06dd3048600234), \
                "jdsnd_u3.bin",CRC(0a11f673) SHA1(ab556477a25e3493555b8a281ca86677caec8947), \
                "jdsnd_u4.bin",CRC(93f6ebc1) SHA1(5cb306afa693e60887069745588dfd5b930c5951), \
                "jdsnd_u5.bin",CRC(c9f28ba6) SHA1(8447372428e3b9fc86a98286c05f95a13abe26b0), \
                "jdsnd_u6.bin",CRC(ef0bf094) SHA1(c0860cecd436d352fe2c2208533ff6dc71bfced1), \
                "jdsnd_u7.bin",CRC(aebab88b) SHA1(d3f1be60a6840d9d085e22b43aafea1354771980), \
                "jdsnd_u8.bin",CRC(77604893) SHA1(a9a4a66412096edd88ee7adfd960eef6f5d16476), \
                "jdsnd_u9.bin",CRC(885b7c70) SHA1(be3bb42aeda3020a72c527f52c5330d0bafa9966))

WPC_ROMSTART(jd,l7,"jdrd_l7.rom",0x80000,CRC(87b2a5c3) SHA1(e487e9ff78353ee96d5fb5f036b1a6cef586f5b4)) JD_SOUND WPC_ROMEND
WPC_ROMSTART(jd,l1,"jd_l1.u6",   0x80000,CRC(09a4b1d8) SHA1(9f941bbeb6e58d918d374694c7ff2a67f1084cc0)) JD_SOUND WPC_ROMEND
WPC_ROMSTART(jd,l6,"jd_l6.u6",   0x80000,CRC(0a74cba4) SHA1(1872fd86bbfa772eac9cc2ef2634a90b72b3d5e2)) JD_SOUND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(jd,l7,"Judge Dredd (L-7)",1993,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(jd,l1,l7,"Judge Dredd (L-1)",1993,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(jd,l6,l7,"Judge Dredd (L-6)",1993,"Bally",wpc_mDCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData jdSimData = {
  2,	    				/* 2 game specific input ports */
  jd_stateDef,				/* Definition of all states */
  jd_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stDrain },
  NULL,					/* no init */
  jd_handleBallState,			/*Function to handle ball state changes*/
  jd_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE,				/* Do not simulate manual shooter */
  jd_keyCond				/* advanced key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData jdGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U),
    0,0,1,0,0,0,0,
    jd_getSol, jd_handleMech, jd_getMech, jd_drawMech,
    NULL, NULL
  },
  &jdSimData,
  {
    {0}, /* sNO */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x02, 0x00, 0xf8, 0x6e, 0x3e, 0x7f, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swRFire},
  }
};


/*---------------
/  Game handling
/----------------*/
static void init_jd(void){
  core_gameData = &jdGameData;
}

/*-- return status of custom solenoids --*/
static int jd_getSol(int solNo) {
 /* Arm Magnet Must be Off for solenoid to fire! */
 if (solNo == sFakeSol1)
  return (core_getSol(sArmMagnet)==0);
 return 0;
}

static void jd_handleMech(int mech)
{
  /*Spin Globe*/
  if (mech & 0x01) {
    if (core_getSol(sGlobeMotor))
      locals.globePos++;
    if (locals.globePos>100)
      locals.globePos=0;
    /*Ball ok to lift switch set when in desired position*/
    core_setSw(swGlobe2,(locals.globePos>=0 && locals.globePos<=10));
  }

  /*Move Arm*/
  if (mech & 0x02) {
    if (core_getSol(sGlobeArm))
      locals.armPos++;
    if (locals.armPos>100)
      locals.armPos=0;
    /*ARM FAR RIGHT is set when at desired position*/
    core_setSw(swArmFR,(locals.armPos>=0 && locals.armPos<=10));
  }

  /*Handle Drop Targets*/
  /*WPCMAME: I don't get this. Why do we need all this drop target handling? */
  //Drop Target Solenoid Raises ALL 5 DTS*/
  if (mech & 0x04) {
    if (core_getSol(sDropTarget))
      locals.jdtPos = locals.udtPos = locals.ddtPos = locals.gdtPos = locals.edtPos = DT_UP;
    else {
      /*Switch hit pops the target down!*/
      if (locals.jdtPos == DT_UP && core_getSw(sw_Judge))
     	locals.jdtPos = DT_DOWN;
      if (locals.udtPos == DT_UP && core_getSw(swJ_udge))
     	locals.udtPos = DT_DOWN;
      if (locals.ddtPos == DT_UP && core_getSw(swJu_dge))
     	locals.ddtPos = DT_DOWN;
      if (locals.gdtPos == DT_UP && core_getSw(swJud_ge))
     	locals.gdtPos = DT_DOWN;
      if (locals.edtPos == DT_UP && core_getSw(swJudg_e))
     	locals.edtPos = DT_DOWN;
    }
    /*If TripDrop Pull D Back Down! */
    if (core_getSol(sTripDropTarget))
      locals.ddtPos = DT_DOWN;

    /*All drop target switches must be on, while target is down!*/
    core_setSw(sw_Judge,locals.jdtPos == DT_DOWN);
    core_setSw(swJ_udge,locals.udtPos == DT_DOWN);
    core_setSw(swJu_dge,locals.ddtPos == DT_DOWN);
    core_setSw(swJud_ge,locals.gdtPos == DT_DOWN);
    core_setSw(swJudg_e,locals.edtPos == DT_DOWN);
  }
}
static int jd_getMech(int mechNo) {
  if (mechNo == 0) return locals.globePos;
  if (mechNo == 1) return locals.armPos;
  return 0;
}
static char * DispDT()
{
static char buf[100];
sprintf(buf,"%c%c%c%c%c",
		     (locals.jdtPos==DT_UP)?'J':'_',
		     (locals.udtPos==DT_UP)?'U':'_',
		     (locals.ddtPos==DT_UP)?'D':'_',
		     (locals.gdtPos==DT_UP)?'G':'_',
		     (locals.edtPos==DT_UP)?'E':'_'
       );
return buf;
}

static char * DispPlanet()
{
static char buf[100];
sprintf(buf,"%s%-3s",(core_getSol(sGlobeMotor))?"Spinning":"Not Moving",
		   ((locals.globePos>=0 && locals.globePos<=10))?"(*)":"   "
       );
return buf;
}

static char * DispClaw()
{
static char buf[100];
sprintf(buf,"%s%-13s",(core_getSol(sGlobeArm))?"Moving":"Not Moving",
		   ((locals.armPos>=0 && locals.armPos<=10))?"(Over Planet)":"(Over PF)"
       );
return buf;
}
