/*******************************************************************************
 Preliminary The Flintstones (Williams, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Flintstones Simulator:
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
static int  fs_handleBallState(sim_tBallStatus *ball, int *inports);
static void fs_drawStatic(BMTYPE **line);
static void init_fs(void);

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
WPC_INPUT_PORTS_START(fs,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
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
#define swTicket     	12

/* Other switches */
#define swLaunch	11
#define swShooter	15
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	61
#define swRightSling	62
#define swLeftJet	63
#define swRightJet	64
#define swBottomJet	65
#define swLeftOutlane	71
#define swLeftInlane	72
#define swRightInlane	73
#define swRightOutlane	74

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sLaunch		2
#define sKnocker	7
#define sRightSling	9
#define sLeftSling	10
#define sLeftJet	11
#define sBottomJet	12
#define sRightJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet
	  };

static sim_tState fs_stateDef[] = {
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
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int fs_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData fs_inportData[] = {

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
  {0, 0x0100, stBottomJet},
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
  static void fs_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(fs,lx5,"flin_lx5.rom",0x80000,CRC(06707244) SHA1(d86d4564fb27a81e8ab896e2efaf05f4f4a4a152))
DCS_SOUNDROM8x("fs_u2_s.l1",CRC(aa3da768) SHA1(b9ab9d716f03c3fa4dc7352993477c021a07138a),
               "fs_u3_s.l1",CRC(e8a0b2d1) SHA1(5fd7ff4a194f845db53573a1a44efbfffed292f9),
               "fs_u4_s.l1",CRC(a5de69f4) SHA1(a7e7f35964ec8b40a971920c2c6cf2ecb730bc60),
               "fs_u5_s.l1",CRC(74b4d495) SHA1(98a145c07694db7b56f5c6ba84bc631fb5c18bae),
               "fs_u6_s.l1",CRC(3c7f7a04) SHA1(45e017dc36922ad2ff420724f912e109a75a15a3),
               "fs_u7_s.l1",CRC(f32b9271) SHA1(19308cb54ae6fc6343ab7411546b251ba66b0905),
               "fs_u8_s.l1",CRC(a7aafa3e) SHA1(54dca32dc2bec5432cd3664bb5aa45d367560b96),
               "fs_u9_s.l1",CRC(0a6664fb) SHA1(751a726e3ea6a808bb137f3563d54acd1580836d))
WPC_ROMEND

WPC_ROMSTART(fs,lx2,"flin_lx2.rom",0x80000,CRC(cbab53cd) SHA1(e58ac50326f7acae4d732c2db92e86dd8162e760))
DCS_SOUNDROM8x("fs_u2_s.l1",CRC(aa3da768) SHA1(b9ab9d716f03c3fa4dc7352993477c021a07138a),
               "fs_u3_s.l1",CRC(e8a0b2d1) SHA1(5fd7ff4a194f845db53573a1a44efbfffed292f9),
               "fs_u4_s.l1",CRC(a5de69f4) SHA1(a7e7f35964ec8b40a971920c2c6cf2ecb730bc60),
               "fs_u5_s.l1",CRC(74b4d495) SHA1(98a145c07694db7b56f5c6ba84bc631fb5c18bae),
               "fs_u6_s.l1",CRC(3c7f7a04) SHA1(45e017dc36922ad2ff420724f912e109a75a15a3),
               "fs_u7_s.l1",CRC(f32b9271) SHA1(19308cb54ae6fc6343ab7411546b251ba66b0905),
               "fs_u8_s.l1",CRC(a7aafa3e) SHA1(54dca32dc2bec5432cd3664bb5aa45d367560b96),
               "fs_u9_s.l1",CRC(0a6664fb) SHA1(751a726e3ea6a808bb137f3563d54acd1580836d))
WPC_ROMEND

WPC_ROMSTART(fs,sp2,"flin_lx5.rom",0x80000,CRC(06707244) SHA1(d86d4564fb27a81e8ab896e2efaf05f4f4a4a152))
DCS_SOUNDROM8x("su2-sp2.rom",CRC(8c627583) SHA1(ddbd5bd06ee83b126025b487d94998da9106ff3f),
               "fs_u3_s.l1",CRC(e8a0b2d1) SHA1(5fd7ff4a194f845db53573a1a44efbfffed292f9),
               "fs_u4_s.l1",CRC(a5de69f4) SHA1(a7e7f35964ec8b40a971920c2c6cf2ecb730bc60),
               "fs_u5_s.l1",CRC(74b4d495) SHA1(98a145c07694db7b56f5c6ba84bc631fb5c18bae),
               "fs_u6_s.l1",CRC(3c7f7a04) SHA1(45e017dc36922ad2ff420724f912e109a75a15a3),
               "fs_u7_s.l1",CRC(f32b9271) SHA1(19308cb54ae6fc6343ab7411546b251ba66b0905),
               "fs_u8_s.l1",CRC(a7aafa3e) SHA1(54dca32dc2bec5432cd3664bb5aa45d367560b96),
               "fs_u9_s.l1",CRC(0a6664fb) SHA1(751a726e3ea6a808bb137f3563d54acd1580836d))
WPC_ROMEND

WPC_ROMSTART(fs,lx4,"flin_lx4.rom",0x80000,CRC(fca5634c) SHA1(8d713c0ba94cfc446fef823d45e268bccb5c6fcc))
DCS_SOUNDROM8x("fs_u2_s.l1",CRC(aa3da768) SHA1(b9ab9d716f03c3fa4dc7352993477c021a07138a),
               "fs_u3_s.l1",CRC(e8a0b2d1) SHA1(5fd7ff4a194f845db53573a1a44efbfffed292f9),
               "fs_u4_s.l1",CRC(a5de69f4) SHA1(a7e7f35964ec8b40a971920c2c6cf2ecb730bc60),
               "fs_u5_s.l1",CRC(74b4d495) SHA1(98a145c07694db7b56f5c6ba84bc631fb5c18bae),
               "fs_u6_s.l1",CRC(3c7f7a04) SHA1(45e017dc36922ad2ff420724f912e109a75a15a3),
               "fs_u7_s.l1",CRC(f32b9271) SHA1(19308cb54ae6fc6343ab7411546b251ba66b0905),
               "fs_u8_s.l1",CRC(a7aafa3e) SHA1(54dca32dc2bec5432cd3664bb5aa45d367560b96),
               "fs_u9_s.l1",CRC(0a6664fb) SHA1(751a726e3ea6a808bb137f3563d54acd1580836d))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(fs,lx5,"The Flintstones (LX-5)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(fs,lx2,lx5,"The Flintstones (LX-2)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(fs,sp2,lx5,"The Flintstones (SP-2)",1994,"Williams",wpc_mSecurityS,0)
CORE_CLONEDEF(fs,lx4,lx5,"The Flintstones (LX-4)",1994,"Williams",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData fsSimData = {
  2,    				/* 2 game specific input ports */
  fs_stateDef,				/* Definition of all states */
  fs_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  fs_handleBallState,			/*Function to handle ball state changes*/
  fs_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData fsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR)
  },
  &fsSimData,
  {
    "529 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_fs(void) {
  core_gameData = &fsGameData;
}

