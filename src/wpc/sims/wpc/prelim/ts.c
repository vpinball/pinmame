/*******************************************************************************
 Preliminary The Shadow (Bally, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Shadow Simulator:
  ------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)

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
static int  ts_handleBallState(sim_tBallStatus *ball, int *inports);
static void ts_handleMech(int mech);
static void ts_drawMech(BMTYPE **line);
static void ts_drawStatic(BMTYPE **line);
static int ts_getSol(int solNo);
static int ts_getMech(int mechNo);
static void init_ts(void);

/*-----------------------
  local static variables
 ------------------------*/
/* Uncomment if you wish to use locals. type variables */
static struct {
  int magnetCnt;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(ts,5)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"",			KEYCODE_W)
    COREPORT_BIT(0x0080,"",			KEYCODE_E)
    COREPORT_BIT(0x0100,"",			KEYCODE_R)
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
#define swRightOutlane	15
#define swRightInlane	16
#define swLeftInlane	17
#define swLeftOutlane	18
#define swTrough1	41
#define swTrough2	42
#define swTrough3	43
#define swTrough4	44
#define swTrough5	45
#define swTroughJam	46
#define swShooter	48
#define swLeftSling	61
#define swRightSling	62

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sTrough		13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough5=SIM_FIRSTSTATE, stTrough4, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling
	  };

static sim_tState ts_stateDef[] = {
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
  {"Habitrail",		1,0,		 0,		stRightInlane,	10,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int ts_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
        }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData ts_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
//  {0, 0x0040, st},
//  {0, 0x0080, st},
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
//  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void ts_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 70,BLACK,"Help on this Simulator:");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,100,BLACK,"Q = Drain Ball");
  core_textOutf(30,110,BLACK,"");
  core_textOutf(30,120,BLACK,"");
  core_textOutf(30,130,BLACK,"");
  core_textOutf(30,140,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,150,BLACK,"");
  core_textOutf(30,160,BLACK,"");
  }

/*-----------------
/  ROM definitions
/------------------*/
#define SHADOW_SOUND \
DCS_SOUNDROM6x("ts_u2_s.l1",CRC(f1486cfb) SHA1(a916917cb4e46b5d1e04eb4dd52b4193e48d4da8), \
               "ts_u3_s.l1",CRC(b9e39c3f) SHA1(183730dcaa84f8b83b6d26521e90fdb0fc558b4c), \
               "ts_u4_s.l1",CRC(a1d1ab66) SHA1(5380f347cb3970bac4aab5917a51d2d64fbca541), \
               "ts_u5_s.l1",CRC(ab8cf435) SHA1(86d7f9eca3e49e184700a0ac0f672349fc1241bb), \
               "ts_u6_s.l1",CRC(63b8d2db) SHA1(a662a3280a377ac91fdf55d98d2204e024668706), \
               "ts_u7_s.l1",CRC(62b5db14) SHA1(13832c8573623f9d541de8b814aa10cfb527be99))

WPC_ROMSTART(ts,lh6,"shad_h6.rom", 0x080000,CRC(0a72268d) SHA1(97836afc23c4160bca462f14c115b17e58fe5a48)) SHADOW_SOUND WPC_ROMEND
WPC_ROMSTART(ts,la2,"cpu-u6l2.rom",0x080000,CRC(e4cff76a) SHA1(37c01f8c6e88186f3b88808bbfee75005ca4008d)) SHADOW_SOUND WPC_ROMEND
WPC_ROMSTART(ts,la4,"u6-la4.rom",  0x080000,CRC(5915cf6d) SHA1(1957988c51b791f76130b8960e9ee61ce17b2088)) SHADOW_SOUND WPC_ROMEND
WPC_ROMSTART(ts,lx4,"u6-lx4.rom",  0x080000,CRC(1d908d38) SHA1(9dbc770ea7b22e27439399f92d81f736a12ddf9f)) SHADOW_SOUND WPC_ROMEND
WPC_ROMSTART(ts,lx5,"shad_x5.rom", 0x080000,CRC(bb545f83) SHA1(c2851f7169ca3d28399468967c04e69835f61536)) SHADOW_SOUND WPC_ROMEND

WPC_ROMSTART(ts,pa1,"cpu-u6p1.rom",0x080000,CRC(835b8167) SHA1(70c00dbe7a7c1a188ef9fe303558e248fdf7230a))
DCS_SOUNDROM6x("su2-sp2.rom",CRC(ba17f74b) SHA1(9c1f00ea27986d025bcaa6b2ffe8c7c4d2216893),
               "ts_u3_s.l1", CRC(b9e39c3f) SHA1(183730dcaa84f8b83b6d26521e90fdb0fc558b4c),
               "ts_u4_s.l1", CRC(a1d1ab66) SHA1(5380f347cb3970bac4aab5917a51d2d64fbca541),
               "ts_u5_s.l1", CRC(ab8cf435) SHA1(86d7f9eca3e49e184700a0ac0f672349fc1241bb),
               "ts_u6_s.l1", CRC(63b8d2db) SHA1(a662a3280a377ac91fdf55d98d2204e024668706),
               "ts_u7_s.l1", CRC(62b5db14) SHA1(13832c8573623f9d541de8b814aa10cfb527be99))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (ts,lx5,    "The Shadow (LX-5)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(ts,lh6,lx5,"The Shadow (LH-6)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(ts,lx4,lx5,"The Shadow (LX-4)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(ts,la4,lx5,"The Shadow (LA-4)",1995,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(ts,la2,lx5,"The Shadow (LA-2)",1994,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(ts,pa1,lx5,"The Shadow (PA-1)",1994,"Bally",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData tsSimData = {
  2,    				/* 2 game specific input ports */
  ts_stateDef,				/* Definition of all states */
  ts_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  ts_handleBallState,			/*Function to handle ball state changes*/
  ts_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData tsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,0,1,0,0,0,0,
    ts_getSol, ts_handleMech, ts_getMech, ts_drawMech,
    NULL, NULL
  },
  &tsSimData,
  {
    "532 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0xe7, 0x7f, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
#define TS_MINIPFPOS 20
static mech_tInitData ts_paddleMech = {
  19, 20, MECH_LINEAR|MECH_STOPEND|MECH_TWODIRSOL, TS_MINIPFPOS+1, TS_MINIPFPOS-1,
  {{37, 0, 0},{38, TS_MINIPFPOS-2, TS_MINIPFPOS-2}}
};

static void ts_handleMech(int mech) {
//  if (mech & 0x01) mech_update(0);
  if (mech & 0x02) {
    if (core_getSol(35)) locals.magnetCnt = 8;
    else if (locals.magnetCnt > 0) locals.magnetCnt -= 1;
  }
}
static void ts_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"MiniPF:%3d", mech_getPos(0));
}
static int ts_getSol(int solNo) {
  return locals.magnetCnt > 0;
}

static int ts_getMech(int mechNo) {
  return mechNo ? (locals.magnetCnt > 0) : mech_getPos(0);
}
static void init_ts(void) {
  core_gameData = &tsGameData;
  mech_add(0,&ts_paddleMech);
}

