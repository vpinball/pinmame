/*******************************************************************************
 Preliminary Jack*Bot (Williams, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Jack*Bot Simulator:
  --------------------------------
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
static int  jb_handleBallState(sim_tBallStatus *ball, int *inports);
static void jb_drawStatic(BMTYPE **line);
static void init_jb(void);

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
WPC_INPUT_PORTS_START(jb,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Upper Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Left Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Lower Jet",		KEYCODE_R)
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
#define swLeftOutlane	25
#define swLeftInlane	26
#define swRightInlane	27
#define swRightOutlane	28
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swUpperJet	61
#define swLeftJet	62
#define swLowerJet	63
#define swRightSling	64
#define swLeftSling	65
#define swShooter	68

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sLowerJet	11
#define sLeftJet	12
#define sUpperJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stUpperJet, stLeftJet, stLowerJet
	  };

static sim_tState jb_stateDef[] = {
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
  {"Upper Bumper",	1,swUpperJet,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Lower Bumper",	1,swLowerJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int jb_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
		if (ball->speed < 51)
			return setState(stFree,51);		/*Ball goes to stFree*/
		break;
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData jb_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stUpperJet},
  {0, 0x0080, stLeftJet},
  {0, 0x0100, stLowerJet},
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
  static void jb_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(jb,10r,"jack1_0r.rom",0x80000,CRC(0e1a900a) SHA1(894f642611d29ce11e13ef9dd68dba7dfc602a3a))
DCS_SOUNDROM5x( "jbsnd_u2.rom",CRC(b116d59f) SHA1(c87c82d7fa4ca40d337fb59476b0b1c90dff86f0),
                "jbsnd_u3.rom",CRC(76ad3aad) SHA1(012b44c48d1cbb282eb763e40db40b141397f426),
                "jbsnd_u4.rom",CRC(038b1309) SHA1(a6e337476902ed9ec5123fe4e088a0608c0d5f48),
                "jbsnd_u5.rom",CRC(0957e2ad) SHA1(0fb4e3fdb949b0979721064162a41cfba84d0013),
				"jbsnd_u6.rom",CRC(7a1e2c3d) SHA1(0c6ccb937328509cb0a87e4c557a64c13bbed2db))
WPC_ROMEND
WPC_ROMSTART(jb,10b,"jack1_0b.rom",0x80000,CRC(da3b2735) SHA1(f895b1548107052f469d8e3fa205bce6113962d9))
DCS_SOUNDROM5x( "jbsnd_u2.rom",CRC(b116d59f) SHA1(c87c82d7fa4ca40d337fb59476b0b1c90dff86f0),
                "jbsnd_u3.rom",CRC(76ad3aad) SHA1(012b44c48d1cbb282eb763e40db40b141397f426),
                "jbsnd_u4.rom",CRC(038b1309) SHA1(a6e337476902ed9ec5123fe4e088a0608c0d5f48),
                "jbsnd_u5.rom",CRC(0957e2ad) SHA1(0fb4e3fdb949b0979721064162a41cfba84d0013),
				"jbsnd_u6.rom",CRC(7a1e2c3d) SHA1(0c6ccb937328509cb0a87e4c557a64c13bbed2db))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(jb,10r,"Jack*Bot (1.0R)",1995,"Williams",wpc_m95DCSS,0)
CORE_CLONEDEF(jb,10b,10r,"Jack*Bot (1.0B) (Belgium/Canada)",1995,"Williams",wpc_m95DCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData jbSimData = {
  2,    				/* 2 game specific input ports */
  jb_stateDef,				/* Definition of all states */
  jb_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  jb_handleBallState,			/*Function to handle ball state changes*/
  jb_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData jbGameData = {
  GEN_WPC95DCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L)
  },
  &jbSimData,
  {
    "551 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0xe0, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_jb(void) {
  core_gameData = &jbGameData;
}

