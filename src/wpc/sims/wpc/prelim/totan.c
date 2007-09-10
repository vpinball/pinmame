/*******************************************************************************
 Preliminary Tales of the Arabian Nights (Williams, 1996) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Tales of the Arabian Nights Simulator:
  ---------------------------------------------------
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
static int  totan_handleBallState(sim_tBallStatus *ball, int *inports);
static void totan_drawStatic(BMTYPE **line);
static void init_totan(void);

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
WPC_INPUT_PORTS_START(totan,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Middle Jet",		KEYCODE_R)
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
#define swLeftOutlane	16
#define swRightInlane	17
#define swShooter	18
#define swLeftInlane	26
#define swRightOutlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	51
#define swRightSling	52
#define swLeftJet	53
#define swRightJet	54
#define swMiddleJet	55
#define swTopSkill	63
#define swMidSkill	64
#define swBotSkill	65

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sKnocker	7
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sRightJet	13
#define sMiddleJet	14

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stMiddleJet,
      stTopSkill, stMidSkill, stBotSkill
	  };

static sim_tState totan_stateDef[] = {
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
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Middle Bumper",	1,swMiddleJet,	 0,		stFree,		1},
  {"Top Skill Shot",	1,swTopSkill,	 0,		stFree,		12},
  {"Mid Skill Shot",	1,swMidSkill,	 0,		stFree,		12},
  {"Bot Skill Shot",	1,swBotSkill,	 0,		stFree,		12},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int totan_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 15)
			return setState(stNotEnough,15);	/*Ball not plunged hard enough*/
		if (ball->speed < 30)
			return setState(stBotSkill,30);		/*Ball goes to the Bottom Skill Shot Target*/
		if (ball->speed < 45)
			return setState(stMidSkill,45);		/*Ball goes to the Middle Skill Shot Target*/
		if (ball->speed < 51)
			return setState(stTopSkill,51);		/*Ball goes to the Top Skill Shot Target*/
		break;
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData totan_inportData[] = {

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
  {0, 0x0100, stMiddleJet},
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
  static void totan_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(totan,14,"an_g11.1_4",0x80000,CRC(54db749e) SHA1(8f8b44febf3b672107e7715ec16e39dd91ee4cbb))
DCS_SOUNDROM4mx("ans2v1_1.rom",CRC(0d023f90) SHA1(e411f7824df89374cf3385a2660d5bc91e0e9ef0),
                "ans3v1_0.rom",CRC(3f677813) SHA1(b1e67c74b927c0c8cb76be8794a04a53fdf643d4),
                "ans4v1_0.rom",CRC(c26dff5f) SHA1(d86323f0df15cf7abd4480d173e6b217ef715396),
                "ans5v1_0.rom",CRC(32ca1602) SHA1(e4c7235b5d387bdde16ebef4d3aeeb7276c69d6d))
WPC_ROMEND
WPC_ROMSTART(totan,13,"arab1_3.rom",0x80000,CRC(2e4b9439) SHA1(ba564c5984d3b68eaeba27d06f3acd95d26073ee))
DCS_SOUNDROM4mx("ans2v1_1.rom",CRC(0d023f90) SHA1(e411f7824df89374cf3385a2660d5bc91e0e9ef0),
                "ans3v1_0.rom",CRC(3f677813) SHA1(b1e67c74b927c0c8cb76be8794a04a53fdf643d4),
                "ans4v1_0.rom",CRC(c26dff5f) SHA1(d86323f0df15cf7abd4480d173e6b217ef715396),
                "ans5v1_0.rom",CRC(32ca1602) SHA1(e4c7235b5d387bdde16ebef4d3aeeb7276c69d6d))
WPC_ROMEND
WPC_ROMSTART(totan,12,"arab1_2.rom",0x80000,CRC(f9ae3796) SHA1(06e4ce89cab2e0fe5039de4261f7b5ebd4c11c0b))
DCS_SOUNDROM4mx("ans2v1_1.rom",CRC(0d023f90) SHA1(e411f7824df89374cf3385a2660d5bc91e0e9ef0),
                "ans3v1_0.rom",CRC(3f677813) SHA1(b1e67c74b927c0c8cb76be8794a04a53fdf643d4),
                "ans4v1_0.rom",CRC(c26dff5f) SHA1(d86323f0df15cf7abd4480d173e6b217ef715396),
                "ans5v1_0.rom",CRC(32ca1602) SHA1(e4c7235b5d387bdde16ebef4d3aeeb7276c69d6d))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(totan,14,"Tales Of The Arabian Nights (1.4)",1996,"Williams",wpc_m95S,0)
CORE_CLONEDEF(totan,13,14,"Tales Of The Arabian Nights (1.3)",1996,"Williams",wpc_m95S,0)
CORE_CLONEDEF(totan,12,14,"Tales Of The Arabian Nights (1.2)",1996,"Williams",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData totanSimData = {
  2,    				/* 2 game specific input ports */
  totan_stateDef,			/* Definition of all states */
  totan_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  totan_handleBallState,		/*Function to handle ball state changes*/
  totan_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData totanGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1
  },
  &totanSimData,
  {
    "547 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_totan(void) {
  core_gameData = &totanGameData;
}

