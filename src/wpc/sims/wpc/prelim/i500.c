/*******************************************************************************
 Preliminary Indianapolis 500 (Bally, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Indianapolis 500 Simulator:
  ----------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers

   More to be added...

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  i500_handleBallState(sim_tBallStatus *ball, int *inports);
static void i500_drawStatic(BMTYPE **line);
static void init_i500(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int turboPos, speedCount, speed;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(i500,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Center Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"",			KEYCODE_T)
    COREPORT_BIT(0x0400,"",			KEYCODE_Y)
    COREPORT_BIT(0x0800,"",			KEYCODE_U)
    COREPORT_BIT(0x1000,"",			KEYCODE_I)
    COREPORT_BIT(0x2000,"",			KEYCODE_O)
    COREPORT_BIT(0x4000,"",			KEYCODE_A)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"",			KEYCODE_S)
    COREPORT_BIT(0x0002,"",			KEYCODE_D)
    COREPORT_BIT(0x0004,"",			KEYCODE_F)
    COREPORT_BIT(0x0008,"",			KEYCODE_G)
    COREPORT_BIT(0x0010,"",			KEYCODE_H)
    COREPORT_BIT(0x0020,"",			KEYCODE_J)
    COREPORT_BIT(0x0040,"",			KEYCODE_K)
    COREPORT_BIT(0x0080,"",			KEYCODE_L)
    COREPORT_BIT(0x0100,"",			KEYCODE_Z)
    COREPORT_BIT(0x0200,"",			KEYCODE_X)
    COREPORT_BIT(0x0400,"",			KEYCODE_C)
    COREPORT_BIT(0x0800,"",			KEYCODE_V)
    COREPORT_BIT(0x1000,"",			KEYCODE_B)
    COREPORT_BIT(0x2000,"",			KEYCODE_N)
    COREPORT_BIT(0x4000,"",			KEYCODE_M)
    COREPORT_BIT(0x8000,"",			KEYCODE_COMMA)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* Standard Switches */
#define swStart      	13
#define swTilt       	14
#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23

/* Other switches */
#define swLaunch	11
#define swLeftOutlane	15
#define swLeftInlane	16
#define swRightInlane	17
#define swRightOutlane	18
#define swShooter	25
#define swLeftSling	26
#define swRightSling	27
#define swTroughJam	41
#define swTrough1	42
#define swTrough2	43
#define swTrough3	44
#define swTrough4	45
#define swLeftJet	72
#define swRightJet	73
#define swCenterJet	74

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKnocker	7
#define sLeftJet	8
#define sRightJet	9
#define sCenterJet	10
#define sLeftSling	11
#define sRightSling	12
#define sTrough		13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stCenterJet
	  };

static sim_tState i500_stateDef[] = {
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
  {"Ball Lane",		1,0,		 0,		stFree,		7,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Center Bumper",	1,swCenterJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int i500_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }


static void i500_handleMech(int mech) {
  if (mech & 0x01) {
    locals.speed = locals.speedCount < 60; /* 1 second between pulses = off */
    if (core_getPulsedSol(17)) {
      locals.speed = (locals.speedCount < 2) + 1;
      locals.speedCount = 0;
      locals.turboPos = (locals.turboPos + 1) % 64;
    }
    else
      locals.speedCount += 1;
  }
  if ((mech & 0x03) == 0x03)
    core_setSw(66, (locals.turboPos & 0x0f) < 3);
}
static int i500_getMech(int mechNo) {
  return mechNo ? locals.turboPos : locals.speed;
}
static void i500_drawMech(BMTYPE **line) {
  static const char *speedText[] = {"Stopped","Slow","Fast"};
  core_textOutf(50, 0,BLACK,"Turbo Speed:%-8s", speedText[locals.speed]);
  core_textOutf(50,10,BLACK,"Turbo Pos:%2d", locals.turboPos);
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData i500_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stCenterJet},
//  {0, 0x0200, st},
//  {0, 0x0400, st},
//  {0, 0x0800, st},
//  {0, 0x1000, st},
//  {0, 0x2000, st},
//  {0, 0x4000, st},
  {0, 0x8000, stDrain},

/* Port 1 */
//  {1, 0x0001, st},
//  {1, 0x0002, st},
//  {1, 0x0004, st},
//  {1, 0x0008, st},
//  {1, 0x0010, st},
//  {1, 0x0020, st},
//  {1, 0x0040, st},
//  {1, 0x0080, st},
//  {1, 0x0100, st},
//  {1, 0x0200, st},
//  {1, 0x0400, st},
//  {1, 0x0800, st},
//  {1, 0x1000, st},
//  {1, 0x2000, st},
//  {1, 0x4000, st},
//  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void i500_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,100,BLACK,"");
  core_textOutf(30,110,BLACK,"");
  core_textOutf(30,120,BLACK,"");
  core_textOutf(30,130,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,140,BLACK,"");
  core_textOutf(30,150,BLACK,"");
  core_textOutf(30,160,BLACK,"");
  }

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(i500,11r,"indy1_1r.rom",0x80000,CRC(ec385bf5) SHA1(50d8f3e682e3a59f3df35f099e97858b2fd211ff))
DCS_SOUNDROM6x(	"su2",CRC(d2f9ab24) SHA1(ac96fdce6545d1017fa3ed7567061ae2e0653750),
		"su3",CRC(067f4df6) SHA1(0adc116bfebefb17f27718bdd2401c336b07078f),
		"su4",CRC(229b96c2) SHA1(77eda81fd011fc818c3fde5e1094cfb3f12372c6),
		"su5",CRC(f0c006a5) SHA1(ead07bb131bd581c41ab0833f6269de7e574017c),
		"su6",CRC(a2b60d31) SHA1(0e0ddb310ec78e0963794994edd0c6bbc4863f4f),
		"su7",CRC(94eea5a4) SHA1(afb00e799dbc01c67ed2c4aa399e8a7365ca3dd3))
WPC_ROMEND

WPC_ROMSTART(i500,11b,"indy1_1b.rom",0x80000,CRC(76a5de55) SHA1(858d9817b534fed470919fa5957709dd1e4216d8))
DCS_SOUNDROM6x(	"su2",CRC(d2f9ab24) SHA1(ac96fdce6545d1017fa3ed7567061ae2e0653750),
		"su3",CRC(067f4df6) SHA1(0adc116bfebefb17f27718bdd2401c336b07078f),
		"su4",CRC(229b96c2) SHA1(77eda81fd011fc818c3fde5e1094cfb3f12372c6),
		"su5",CRC(f0c006a5) SHA1(ead07bb131bd581c41ab0833f6269de7e574017c),
		"su6",CRC(a2b60d31) SHA1(0e0ddb310ec78e0963794994edd0c6bbc4863f4f),
		"su7",CRC(94eea5a4) SHA1(afb00e799dbc01c67ed2c4aa399e8a7365ca3dd3))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(i500,11r,"Indianapolis 500 (1.1R)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(i500,11b,11r,"Indianapolis 500 (1.1 Belgium)",1995,"Bally",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData i500SimData = {
  2,    				/* 2 game specific input ports */
  i500_stateDef,			/* Definition of all states */
  i500_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  i500_handleBallState,			/*Function to handle ball state changes*/
  i500_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData i500GameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,0,0,0,0,0,0,
    NULL, i500_handleMech, i500_getMech, i500_drawMech,
    NULL, NULL
  },
  &i500SimData,
  {
    "526 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x1f, 0xe0, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_i500(void) {
  core_gameData = &i500GameData;
}

