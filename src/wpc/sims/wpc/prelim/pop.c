/*******************************************************************************
 Preliminary Popeye Saves the Earth (Bally, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Popeye Saves the Earth Simulator:
  ----------------------------------------------
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
static int  pop_handleBallState(sim_tBallStatus *ball, int *inports);
static void pop_drawStatic(BMTYPE **line);
static void init_pop(void);

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
WPC_INPUT_PORTS_START(pop,6)

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

/* Other switches */
#define swLeftJet	16
#define swRightJet	17
#define swCenterJet	18
#define swLaunch	23
#define swWheelExit	37
#define swTrough1	51
#define swTrough2	52
#define swTrough3	53
#define swTrough4	54
#define swTrough5	55
#define swTrough6	56
#define swTroughJam	57
#define swLeftOutlane	74
#define swLeftInlane	75
#define swLeftSling	76
#define swRightSling	77
#define swRightInlane	78
#define swRightOutlane	85
#define swShooter	86

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		3
#define sTrough		5
#define sKnocker	7
#define sLeftJet	9
#define sRightJet	10
#define sCenterJet	11
#define sLeftSling	12
#define sRightSling	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough6=SIM_FIRSTSTATE, stTrough5, stTrough4, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stCenterJet,
      stWheelExit
	  };

static sim_tState pop_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 6",		1,swTrough6,	0,		stTrough5,	1},
  {"Trough 5",		1,swTrough5,	0,		stTrough4,	1},
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,	stTrough,	1},
  {"Trough Jam",	1,swTroughJam,  0,		stShooter,	1},
  {"Drain",		1,0,		0,		stTrough6,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sLaunch,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Habitrail",		1,0,		 0,		stWheelExit,	12,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Center Bumper",	1,swCenterJet,	 0,		stFree,		1},
  {"Wheel Exit",	1,swWheelExit,	 0,		stFree,		5},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int pop_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData pop_inportData[] = {

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
static void pop_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 80,BLACK,"Help on this Simulator:");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30,100,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,110,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,120,BLACK,"");
  core_textOutf(30,130,BLACK,"");
  core_textOutf(30,140,BLACK,"");
  core_textOutf(30,150,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,160,BLACK,"");
}

/*-----------------
/  ROM definitions
/------------------*/
#define POP_SOUND_L2 \
DCS_SOUNDROM6x(	"popsndl2.u2",CRC(00590f2d) SHA1(540ad9825dbaace55bf36a6cee98bef06f240e15), \
  "popsndl2.u3",CRC(87032b27) SHA1(9488d177418b53ceb37686cf6f4f58800b306d85), \
  "popsndl2.u4",CRC(b0808aa8) SHA1(bebe6ec3c3e675e096084b6ed61065ad48dc5c3f), \
  "popsndl2.u5",CRC(3662206b) SHA1(c2714665db18e9ae540a8f922d7ebb3058638563), \
  "popsndl2.u6",CRC(84a5f317) SHA1(f1b9710d109e28fe3255e36dafa2be23656d0445), \
  "popsndl2.u7",CRC(b8fde2c7) SHA1(ee82a7b1ad32e1231356ce42c4ad3109150a9992))

#define POP_SOUND_P0 \
DCS_SOUNDROM6x(	"popsndp0.u2",CRC(1e3a98a4) SHA1(2f871f354df7684d0b4aa31e2d2bb4035072bb4a), \
  "popsndl2.u3",CRC(87032b27) SHA1(9488d177418b53ceb37686cf6f4f58800b306d85), \
  "popsndl2.u4",CRC(b0808aa8) SHA1(bebe6ec3c3e675e096084b6ed61065ad48dc5c3f), \
  "popsndl2.u5",CRC(3662206b) SHA1(c2714665db18e9ae540a8f922d7ebb3058638563), \
  "popsndl2.u6",CRC(84a5f317) SHA1(f1b9710d109e28fe3255e36dafa2be23656d0445), \
  "popsndl2.u7",CRC(b8fde2c7) SHA1(ee82a7b1ad32e1231356ce42c4ad3109150a9992))

WPC_ROMSTART(pop,lx5,"peye_lx5.rom",0x80000,CRC(ee1f7a67) SHA1(f02518546de93256b00bc1f5b92452a10f9e56dd)) POP_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(pop,dx5,"peye_dx5.rom",0x80000,CRC(e8d5a0f3) SHA1(ddff92f4840649a17ed570885becd197378d08ac)) POP_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(pop,lx4,"peye_lx4.rom",0x80000,CRC(6770bce3) SHA1(95cc574b1e3e27ff4dff3277da8241f1a89c85a8)) POP_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(pop,la4,"peye_la4.rom",0x80000,CRC(11cedcf7) SHA1(e0219060cf09a757edf19875a224801b3179664c)) POP_SOUND_L2 WPC_ROMEND
WPC_ROMSTART(pop,pa3,"peye_pa3.rom",0x80000,CRC(1cee3ae7) SHA1(e5b05fcf8aac98993940a2cda2552ff93ee3a518)) POP_SOUND_P0 WPC_ROMEND
WPC_ROMSTART(pop,pa4,"peye_pa4.rom",0x80000,CRC(0be7a0bc) SHA1(584ff8876114277d868a305f313bd13f1e90a129)) POP_SOUND_P0 WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(pop,lx5,"Popeye Saves The Earth (LX-5)",1994,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(pop,dx5,lx5,"Popeye Saves The Earth (DX-5 LED Ghost Fix)",1994,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(pop,lx4,lx5,"Popeye Saves The Earth (LX-4)",1994,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(pop,la4,lx5,"Popeye Saves The Earth (LA-4)",1994,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(pop,pa3,lx5,"Popeye Saves The Earth (PA-3 Prototype)",1993,"Bally",wpc_mDCSS,0)
CORE_CLONEDEF(pop,pa4,lx5,"Popeye Saves The Earth (PA-4 LED Ghost Fix)",1993,"Bally",wpc_mDCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData popSimData = {
  2,    				/* 2 game specific input ports */
  pop_stateDef,				/* Definition of all states */
  pop_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  pop_handleBallState,			/*Function to handle ball state changes*/
  pop_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData popGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U)
  },
  &popSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x7f, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_pop(void) {
  core_gameData = &popGameData;
}
