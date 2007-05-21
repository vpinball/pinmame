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
#define DM_SOUND_P4 \
DCS_SOUNDROM6x("dmsndp4.u2",CRC(8581116b) SHA1(ab24fa4aadf27761c9013adb84cfef9bfda27d44), \
               "dmsndp4.u3",CRC(fe79fc89) SHA1(4ef1ef0d66d43fa66af1ecb17c14141760859084), \
               "dmsndp4.u4",CRC(18407309) SHA1(499d62e4b434d48870fe532bb85106868df17c9b), \
               "dmsndp4.u5",CRC(f2006c93) SHA1(16656ae6ff18aad0965c5a14882138508925313a), \
               "dmsndp4.u6",CRC(bc17ba11) SHA1(a794599bc334762ddb79e1d0219ad20383139728), \
               "dmsndp4.u7",CRC(8760ed90) SHA1(cf8808f7cd347c47fa12e73a6bb5a54303fb7c49))

#define DM_SOUND_L2 \
DCS_SOUNDROM6x("dm_u2_s.l2",CRC(85fb8bce) SHA1(f2e912113d08b230e32aeeb4143485f266574fa2), \
               "dm_u3_s.l2",CRC(2b65a66e) SHA1(7796082ecd7af29a240190aff654320375502a8b), \
               "dm_u4_s.l2",CRC(9d6815fe) SHA1(fb4be63dee54a883884f1600565011cb9740a866), \
               "dm_u5_s.l2",CRC(9f614c27) SHA1(f8f2f083b644517582a748bda0a3f69c14583f13), \
               "dm_u6_s.l2",CRC(3efc2c0e) SHA1(bc4efdee44ff635771629a2bde79e230b7643f31), \
               "dm_u7_s.l2",CRC(75066af1) SHA1(4d70bce8a96343afcf02c89240b11faf19e11f02))

#define DM_SOUND_H1 \
DCS_SOUNDROM8x("dm.2",CRC(03dae358) SHA1(e6ab35a0c530eda90bd2d65af7bff82af08c39f3), \
               "dm.3",CRC(3b924d3f) SHA1(5bd6126cc6a6c662de0bc311c047441bc29919b2), \
               "dm.4",CRC(ff8985da) SHA1(b382c301744ce208f4710b3dd2342457d02f0ce9), \
               "dm.5",CRC(76f09bd0) SHA1(1e4861ddc12069733f7e1d25192df97b0d9b09ee), \
               "dm.6",CRC(2897aca8) SHA1(d910289e10422e22b4a3e1e296a4a167da1eaa5b), \
               "dm.7",CRC(6b1b9137) SHA1(4064f4fc230ba17b68819ff889335d9b6d9bba3e), \
               "dm.8",CRC(5b333818) SHA1(007b8c117516b6023b376f95ff13831111f4dc20), \
               "dm.9",CRC(4c1a34e8) SHA1(3eacc3c63b2d9db57fc86447f1408635b987ef69))

WPC_ROMSTART(dm,pa2,"u6-pa2.rom",  0x80000,CRC(862be56a) SHA1(95e1f899963762cb1a9de4eb5d6d57183ed1da38)) DM_SOUND_P4 WPC_ROMEND
WPC_ROMSTART(dm,px5,"dman_px5.rom",0x80000,CRC(42673371) SHA1(77570902c1ca13956fa65214184bce79bcc67173)) DM_SOUND_P4 WPC_ROMEND
WPC_ROMSTART(dm,la1,"dman_la1.rom",0x80000,CRC(be7c1965) SHA1(ed3b1016febc819b8c9f34953067bf0cdf3f33e6))
DCS_SOUNDROM6x("dm_u2_s.l1",CRC(f72dc72e) SHA1(a1267c32f70b4bfe6058d7e28d82006541fe3d6c),
               "dm_u3_s.l2",CRC(2b65a66e) SHA1(7796082ecd7af29a240190aff654320375502a8b),
               "dm_u4_s.l2",CRC(9d6815fe) SHA1(fb4be63dee54a883884f1600565011cb9740a866),
               "dm_u5_s.l2",CRC(9f614c27) SHA1(f8f2f083b644517582a748bda0a3f69c14583f13),
               "dm_u6_s.l2",CRC(3efc2c0e) SHA1(bc4efdee44ff635771629a2bde79e230b7643f31),
               "dm_u7_s.l2",CRC(75066af1) SHA1(4d70bce8a96343afcf02c89240b11faf19e11f02))
WPC_ROMEND
WPC_ROMSTART(dm,lx3,"dman_lx3.rom",0x80000,CRC(5aa57674) SHA1(e02d91a705799866bd741b998d93413ec5bced25)) DM_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(dm,lx4,"dman_lx4.rom",0x80000,CRC(c2d0f493) SHA1(26ee970827dd96f3b3c56aa548cf7629ed6a16c1)) DM_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(dm,h5, "dman_h5.rom", 0x80000,CRC(bdcc62f7) SHA1(d6f3181970f3f71a876e9a2166156eb8fc405af0)) DM_SOUND_H1 WPC_ROMEND
WPC_ROMSTART(dm,h6, "dman_h6.rom", 0x80000,CRC(3a079b80) SHA1(94a7ee94819ec878ced5e07745bf52b6c65e06c9)) DM_SOUND_H1 WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (dm,lx4,    "Demolition Man (LX-4)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,pa2,lx4,"Demolition Man (PA-2)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,px5,lx4,"Demolition Man (PX-5)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,la1,lx4,"Demolition Man (LA-1)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,lx3,lx4,"Demolition Man (LX-3)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h5,lx4, "Demolition Man (H-5)", 1995,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(dm,h6,lx4, "Demolition Man (H-6)", 1995,"Williams",wpc_mDCSS,0)

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


