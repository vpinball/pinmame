/*******************************************************************************
 Preliminary Dirty Harry (Williams, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Dirty Harry Simulator:
  -----------------------------------
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
static int  dh_handleBallState(sim_tBallStatus *ball, int *inports);
static void dh_drawStatic(BMTYPE **line);
static void init_dh(void);

/*-----------------------
  local static variables
 ------------------------*/
/* Uncomment if you wish to use locals. type variables */
//static struct {
//  int
//} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(dh,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Middle Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Right Jet",		KEYCODE_R)
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
#define swShooter	15
#define swRightOutlane	16
#define swRightInlane	17
#define swLeftInlane	25
#define swLeftOutlane	26
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	61
#define swRightSling	62
#define swLeftJet	63
#define swMiddleJet	64
#define swRightJet	65

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sLaunch		2
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sLeftJet	11
#define sMiddleJet	12
#define sRightJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stMiddleJet, stRightJet
	  };

static sim_tState dh_stateDef[] = {
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
  {"Middle Bumper",	1,swMiddleJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int dh_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData dh_inportData[] = {

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
  {0, 0x0080, stMiddleJet},
  {0, 0x0100, stRightJet},
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
  static void dh_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(dh,lx2,"harr_lx2.rom",0x80000,CRC(d92c2d35) SHA1(68f08120fbc510db46b1fd0e68ec07fe536f77ca))
DCS_SOUNDROM5x(	"dh_snd.u2",CRC(dce5339a) SHA1(c89ec1c2f4f5201cbc40c7038cd1219b200066c7),
		"dh_snd.u3",CRC(27c30ada) SHA1(388c0e533d1d5c88ae020ef8d8b98db4c603c157),
		"dh_snd.u4",CRC(8bde0089) SHA1(8efdcc60daef06c65acf5cb805790d2b82d3c091),
		"dh_snd.u5",CRC(bfacfbdb) SHA1(aa443906a0945586ba5d2910972b333b5d316894),
		"dh_snd.u6",CRC(793dcfb8) SHA1(c9b35e0511962f9fc372f98e937ee5989109056d))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(dh,lx2,"Dirty Harry (LX-2)",1995,"Williams",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData dhSimData = {
  2,    				/* 2 game specific input ports */
  dh_stateDef,				/* Definition of all states */
  dh_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  dh_handleBallState,			/*Function to handle ball state changes*/
  dh_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData dhGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UR)
  },
  &dhSimData,
  {
    "530 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_dh(void) {
  core_gameData = &dhGameData;
}

