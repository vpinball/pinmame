/*******************************************************************************
 Preliminary Demolition Man (Williams, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 031003 Tom: added support for flashers 37-44 (51-58)

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Demolition Man Simulator:
  --------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +J  Jet Bumpers
    +-  L/R Slingshot
     -  Top Slingshot
     Q  SDTM (Drain Ball)

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
static int  dm_handleBallState(sim_tBallStatus *ball, int *inports);
static void dm_drawStatic(BMTYPE **line);
static void init_dm(void);
static int dm_getSol(int solNo);

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
WPC_INPUT_PORTS_START(dm,5)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"L/R Jet",		KEYCODE_J)
    COREPORT_BIT(0x0080,"Top Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0100,"",			KEYCODE_T)
    COREPORT_BIT(0x0200,"",			KEYCODE_Y)
    COREPORT_BIT(0x0400,"",			KEYCODE_U)
    COREPORT_BIT(0x0800,"",			KEYCODE_I)
    COREPORT_BIT(0x1000,"",			KEYCODE_O)
    COREPORT_BIT(0x2000,"",			KEYCODE_A)
    COREPORT_BIT(0x4000,"",			KEYCODE_S)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"",			KEYCODE_D)
    COREPORT_BIT(0x0002,"",			KEYCODE_F)
    COREPORT_BIT(0x0004,"",			KEYCODE_G)
    COREPORT_BIT(0x0008,"",			KEYCODE_H)
    COREPORT_BIT(0x0010,"",			KEYCODE_J)
    COREPORT_BIT(0x0020,"",			KEYCODE_K)
    COREPORT_BIT(0x0040,"",			KEYCODE_L)
    COREPORT_BIT(0x0080,"",			KEYCODE_Z)
    COREPORT_BIT(0x0100,"",			KEYCODE_X)
    COREPORT_BIT(0x0200,"",			KEYCODE_C)
    COREPORT_BIT(0x0400,"",			KEYCODE_V)
    COREPORT_BIT(0x0800,"",			KEYCODE_B)
    COREPORT_BIT(0x1000,"",			KEYCODE_N)
    COREPORT_BIT(0x2000,"",			KEYCODE_M)
    COREPORT_BIT(0x4000,"",			KEYCODE_COMMA)

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
#define swShooter	27
#define swTrough1	31
#define swTrough2	32
#define swTrough3	33
#define swTrough4	34
#define swTrough5	35
#define swTroughJam	36
#define swLeftSling	41
#define swRightSling	42
#define swLeftJet	43
#define swTopSling	44
#define swRightJet	45

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sLaunch		3
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sLeftJet	11
#define sTopSling	12
#define sRightJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough5=SIM_FIRSTSTATE, stTrough4, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stTopSling, stRightJet
	  };

static sim_tState dm_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 5",		1,swTrough5,	0,		stTrough4,	1},
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,	stTrough,	1},
  {"Trough Jam",	1,swTroughJam,  0,		stShooter,	1},
  {"Drain",		1,0,		0,		stTrough5,	0,	0,	0,	SIM_STNOTEXCL},

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
  {"Top Slingshot",	1,swTopSling,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int dm_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData dm_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0041, stLeftJet},
  {0, 0x0042, stRightJet},
  {0, 0x0080, stTopSling},
//  {0, 0x0100, st},
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
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void dm_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+J = L/R Jet Bumper");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R/Top Slingshot");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,100,BLACK,"Q = Drain Ball");
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
WPC_ROMSTART(dm,lx4,"dman_lx4.rom",0x80000,0xc2d0f493)
DCS_SOUNDROM6x("dm_u2_s.l2",0x85fb8bce,
               "dm_u3_s.l2",0x2b65a66e,
               "dm_u4_s.l2",0x9d6815fe,
               "dm_u5_s.l2",0x9f614c27,
               "dm_u6_s.l2",0x3efc2c0e,
               "dm_u7_s.l2",0x75066af1)
WPC_ROMEND
WPC_ROMSTART(dm,px5,"dman_px5.rom",0x80000,0x42673371)
DCS_SOUNDROM6x("dm_u2_s.l2",0x85fb8bce,
               "dm_u3_s.l2",0x2b65a66e,
               "dm_u4_s.l2",0x9d6815fe,
               "dm_u5_s.l2",0x9f614c27,
               "dm_u6_s.l2",0x3efc2c0e,
               "dm_u7_s.l2",0x75066af1)
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(dm,lx4,     "Demolition Man (LX-4)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,px5,lx4,"Demolition Man (PX-5)",1994,"Williams",wpc_mDCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData dmSimData = {
  2,    				/* 2 game specific input ports */
  dm_stateDef,				/* Definition of all states */
  dm_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  dm_handleBallState,			/*Function to handle ball state changes*/
  dm_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData dmGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
	0,0,8,0,0,0,0,
	dm_getSol, NULL, NULL, NULL
  },
  &dmSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x30, 0x3f, 0x00, 0x00, 0x40, 0x2f, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_dm(void) {
  core_gameData = &dmGameData;
}

static int dm_getSol(int solNo) {
  if ((solNo >= CORE_CUSTSOLNO(1)) && (solNo <= CORE_CUSTSOLNO(8)))
    return ((wpc_data[WPC_EXTBOARD1]>>(solNo-CORE_CUSTSOLNO(1)))&0x01);
  return 0;
}


