/*******************************************************************************
 Preliminary Who Dunnit (Bally, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Simulator:
  --------------------------------------------------
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
#include "mech.h"

/*------------------
/  Local functions
/-------------------*/
static int  wd_handleBallState(sim_tBallStatus *ball, int *inports);
static void wd_handleMech(int mech);
static void wd_drawStatic(BMTYPE **line);
static void init_wd(void);

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
WPC_INPUT_PORTS_START(wd,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_E)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_R)
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
#define swShooter	15
#define swRightOutlane	16
#define swRightInlane	17
#define swLeftInlane	26
#define swLeftOutlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	61
#define swRightSling	62
#define swLeftJet	63
#define swBottomJet	64
#define swRightJet	65

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sLeftJet	11
#define sBottomJet	12
#define sRightJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stBottomJet, stRightJet
	  };

static sim_tState wd_stateDef[] = {
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
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int wd_handleBallState(sim_tBallStatus *ball, int *inports) {
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

static sim_tInportData wd_inportData[] = {

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
  {0, 0x0080, stBottomJet},
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

static mech_tInitData wd_reelMech[] = {{
  22,24, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL,50, 50,
  {{22, 0, 2}}
},{
  25,26, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL,50, 50,
  {{25, 0, 2}}
},{
  27,28, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL,50, 50,
  {{12, 0, 2}}
}};

static void wd_handleMech(int mech) {
//  if (mech & 0x01) mech_update(0);
//  if (mech & 0x02) mech_update(1);
//  if (mech & 0x04) mech_update(2);
}

static int wd_getMech(int mechNo) {
  return mech_getPos(mechNo);
}

/*--------------------
  Drawing information
  --------------------*/
static void wd_drawStatic(BMTYPE **line) {
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
static void wd_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"Reel1:%3d", mech_getPos(0));
  core_textOutf(50,10,BLACK,"Reel2:%3d", mech_getPos(1));
  core_textOutf(50,20,BLACK,"Reel3:%3d", mech_getPos(2));
}


/*-----------------
/  ROM definitions
/------------------*/
#define WD_SOUND \
DCS_SOUNDROM6x("wdu2_10.rom",CRC(2fd534be) SHA1(9fcbcfb9bc6fc410398413dea71a6fcbe69f761f), \
               "wdu3_10.rom",CRC(be9b312f) SHA1(53038a8a264da4e62455240f2016309462c28275), \
               "wdu4_10.rom",CRC(46965682) SHA1(b12c21a17090480c0960aec808908f2d37c4b498), \
               "wdu5_10.rom",CRC(0a787015) SHA1(e01a19ac0a1b674e2b348d77e584275ef1359cd7), \
               "wdu6_10.rom",CRC(d2e05659) SHA1(3f926dac710adadc38afd70618a84c9f049ebfd0), \
               "wdu7_10.rom",CRC(36285ca2) SHA1(d42f04aa62b9859ce2452fa05da2049fe39e9411))

WPC_ROMSTART(wd,12,"whod1_2.rom",0x80000,CRC(d49be363) SHA1(a265110170e1debf4a566d91c12e0e4c93838d08)) WD_SOUND WPC_ROMEND

WPC_ROMSTART(wd,12g,"whod1_2.rom",0x80000,CRC(d49be363) SHA1(a265110170e1debf4a566d91c12e0e4c93838d08))
DCS_SOUNDROM6x("wdu2_20g.rom",CRC(2fe0ce7e) SHA1(ae148809b8f8925376bc6b6b0478176cae490a2b),
               "wdu3_20g.rom",CRC(f01142ab) SHA1(ee2620b6238df0069c9b10d1fee3ea0607b022da),
               "wdu4_10.rom",CRC(46965682) SHA1(b12c21a17090480c0960aec808908f2d37c4b498),
               "wdu5_10.rom",CRC(0a787015) SHA1(e01a19ac0a1b674e2b348d77e584275ef1359cd7),
               "wdu6_10.rom",CRC(d2e05659) SHA1(3f926dac710adadc38afd70618a84c9f049ebfd0),
               "wdu7_10.rom",CRC(36285ca2) SHA1(d42f04aa62b9859ce2452fa05da2049fe39e9411))
WPC_ROMEND

WPC_ROMSTART(wd,10r,"whod1_0.rom",0x80000,CRC(85c29cfe) SHA1(5156d3699f16ac366c063149113ec78232ba787b)) WD_SOUND WPC_ROMEND
WPC_ROMSTART(wd,11, "whod1_1.rom",0x80000,CRC(85cab586) SHA1(3940bff8dfa240f8c0ed96c96f58ab66effbdea5)) WD_SOUND WPC_ROMEND
WPC_ROMSTART(wd,10f,"u6_10f.rom", 0x80000,CRC(86ca3749) SHA1(fa011a39c260f9c3fd8c6f5d18f803f6f0bfe7a0)) WD_SOUND WPC_ROMEND

WPC_ROMSTART(wd,10g,"u6_10g.rom",0x80000,CRC(fbc17e3f) SHA1(7d9a8c7dda06bb4353517417fdc65d87b6c94167))
DCS_SOUNDROM6x("wdu2_20g.rom",CRC(2fe0ce7e) SHA1(ae148809b8f8925376bc6b6b0478176cae490a2b),
               "wdu3_20g.rom",CRC(f01142ab) SHA1(ee2620b6238df0069c9b10d1fee3ea0607b022da),
               "wdu4_10.rom",CRC(46965682) SHA1(b12c21a17090480c0960aec808908f2d37c4b498),
               "wdu5_10.rom",CRC(0a787015) SHA1(e01a19ac0a1b674e2b348d77e584275ef1359cd7),
               "wdu6_10.rom",CRC(d2e05659) SHA1(3f926dac710adadc38afd70618a84c9f049ebfd0),
               "wdu7_10.rom",CRC(36285ca2) SHA1(d42f04aa62b9859ce2452fa05da2049fe39e9411))
WPC_ROMEND

WPC_ROMSTART(wd,03r,"u6_03r.rom",0x80000,CRC(8901868a) SHA1(35d8173865208a08a819275b4d76db3f050f61f1))
DCS_SOUNDROM6x("u2-s031.rom",CRC(a265cd93) SHA1(2cebb00119f8fc2022959be2107669c9b4b65bfb),
               "u3-s031.rom",CRC(16105ac9) SHA1(7b40cc9a30dd4f675fda979f41a22364aa9ea452),
               "u4-s031.rom",CRC(07d52ef3) SHA1(4bd7dd97316c7244b556b4bd0a8475e282abaa25),
               "u5-s031.rom",CRC(14fea24c) SHA1(a63e2a7796b89d5a6bca419ceaa14888ae22b7a6),
               "u6-s031.rom",CRC(d15d073e) SHA1(063412a51de3b6c2bdbde0c3f84132d70c935fb4),
               "u7-s031.rom",CRC(d252f599) SHA1(bdce67187c027b713b6ef88f6cd4f025de469929))
WPC_ROMEND

WPC_ROMSTART(wd,048r,"wd_048r.rom",0x80000,CRC(45653baa) SHA1(788d5195e61605e151796f5fff9ca8d00820c7a3))
DCS_SOUNDROM6x("u2-s031.rom",CRC(a265cd93) SHA1(2cebb00119f8fc2022959be2107669c9b4b65bfb),
               "u3-s031.rom",CRC(16105ac9) SHA1(7b40cc9a30dd4f675fda979f41a22364aa9ea452),
               "u4-s031.rom",CRC(07d52ef3) SHA1(4bd7dd97316c7244b556b4bd0a8475e282abaa25),
               "u5-s031.rom",CRC(14fea24c) SHA1(a63e2a7796b89d5a6bca419ceaa14888ae22b7a6),
               "u6-s031.rom",CRC(d15d073e) SHA1(063412a51de3b6c2bdbde0c3f84132d70c935fb4),
               "u7-s031.rom",CRC(d252f599) SHA1(bdce67187c027b713b6ef88f6cd4f025de469929))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (wd,12,    "Who Dunnit (1.2)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,12g,12,"Who Dunnit (1.2 Germany)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,11,12, "Who Dunnit (1.1)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,10r,12,"Who Dunnit (1.0 R)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,10g,12,"Who Dunnit (1.0 Germany)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,10f,12,"Who Dunnit (1.0 French)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,03r,12,"Who Dunnit (0.3 R)",1995,"Bally",wpc_m95DCSS,0)
CORE_CLONEDEF(wd,048r,12,"Who Dunnit (0.48 R)",1995,"Bally",wpc_m95DCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData wdSimData = {
  2,    				/* 2 game specific input ports */
  wd_stateDef,				/* Definition of all states */
  wd_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  wd_handleBallState,			/*Function to handle ball state changes*/
  wd_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData wdGameData = {
  GEN_WPC95DCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L|FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    NULL, wd_handleMech, wd_getMech, wd_drawMech,
    NULL, NULL
  },
  &wdSimData,
  {
    "544 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x02, 0x10, 0x7f, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_wd(void) {
  core_gameData = &wdGameData;
  mech_add(0, &wd_reelMech[0]);
  mech_add(1, &wd_reelMech[1]);
  mech_add(2, &wd_reelMech[2]);
}


