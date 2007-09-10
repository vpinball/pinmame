/*******************************************************************************
 Preliminary Safe Cracker (Bally, 1996) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Safe Cracker Simulator:
  ------------------------------------
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
static int  sc_handleBallState(sim_tBallStatus *ball, int *inports);
static void sc_drawStatic(BMTYPE **line);
static void init_sc(void);

/*-----------------------
  local static variables
 ------------------------*/
/* Uncomment if you wish to use locals. type variables */
static struct {
  int auxLast, auxClk;
} sclocals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(sc,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Top Jet",		KEYCODE_R)
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
#define swRightOutlane	17
#define swShooter	18
#define swLeftInlane	26
#define swRightInlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftJet	44
#define swRightJet	45
#define swTopJet	46
#define swLeftSling	47
#define swRightSling	48

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sRightJet	13
#define sTopJet		16

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stTopJet
	  };

static sim_tState sc_stateDef[] = {
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
  {"Top Bumper",	1,swTopJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int sc_handleBallState(sim_tBallStatus *ball, int *inports) {
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

static sim_tInportData sc_inportData[] = {

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
  {0, 0x0100, stTopJet},
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
  static void sc_drawStatic(BMTYPE **line) {

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
#define SC_SOUND \
DCS_SOUNDROM3m(	"safsnds2.rom",CRC(20e14c63) SHA1(61b1c000a7afe5d0e9c31093e3fa963d6a594d54), \
				"safsnds3.rom",CRC(99e318e7) SHA1(918f9013da82b29a559cb474bce93fb4ce88b731), \
				"safsnds4.rom",CRC(9c8a23eb) SHA1(a0ee1174c8af0f262f9bec950da588cc9eb8747d))

WPC_ROMSTART(sc,18,  "safe_18g.rom",0x80000,CRC(aeb4b669) SHA1(2925eb11133526ddff8ae92bb53f9b45c6ed8134)) SC_SOUND WPC_ROMEND
WPC_ROMSTART(sc,18n, "safe_18n.rom",0x80000,CRC(4d5d5626) SHA1(2d6f201d47f24df2195f10267ec1426cf0a087c9)) SC_SOUND WPC_ROMEND

WPC_ROMSTART(sc,18s2,"safe_18g.rom",0x80000,CRC(aeb4b669) SHA1(2925eb11133526ddff8ae92bb53f9b45c6ed8134))
DCS_SOUNDROM3m(	"su2-24g.rom", CRC(712ce42e) SHA1(5d3b8e3eccdd1bc09a92de610161dd51293181b1),
				"safsnds3.rom",CRC(99e318e7) SHA1(918f9013da82b29a559cb474bce93fb4ce88b731),
				"safsnds4.rom",CRC(9c8a23eb) SHA1(a0ee1174c8af0f262f9bec950da588cc9eb8747d))
WPC_ROMEND

WPC_ROMSTART(sc,17,  "g11-17g.rom", 0x80000,CRC(f3d64156) SHA1(9226664b59c7b65ac39e2f32597efc45672cf505)) SC_SOUND WPC_ROMEND
WPC_ROMSTART(sc,17n, "g11-17n.rom", 0x80000,CRC(97628907) SHA1(3435f496e1850bf433add1bc403e3148de05c13a)) SC_SOUND WPC_ROMEND
WPC_ROMSTART(sc,14,  "g11-14.rom",  0x80000,CRC(1103f976) SHA1(6d6d23af1cd03f63b94a0ceb9711be51dce202f8)) SC_SOUND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(sc,18,"Safe Cracker (1.8)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(sc,18n,18,"Safe Cracker (1.8N)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(sc,18s2,18,"Safe Cracker (1.8, alternate sound)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(sc,17,18,"Safe Cracker (1.7)",1996,"Bally",wpc_m95S,0)
CORE_CLONEDEF(sc,17n,18,"Safe Cracker (1.7N)",1996,"Bally",wpc_m95S,0)
CORE_CLONEDEF(sc,14,18,"Safe Cracker (1.4)",1996,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData scSimData = {
  2,    				/* 2 game specific input ports */
  sc_stateDef,				/* Definition of all states */
  sc_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  sc_handleBallState,			/*Function to handle ball state changes*/
  sc_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData scGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,6,0,0,0,1
  },
  &scSimData,
  {
    "903 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x06, 0xe0, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};
static WRITE_HANDLER(sc_auxLamp) {
  wpc_data[WPC_SOLENOID1] |= data;
  if (~sclocals.auxLast & data & 0x10) sclocals.auxClk = 0;
  if (~sclocals.auxLast & data & 0x20) {
    if (data & 0x40) coreGlobals.tmpLampMatrix[8+sclocals.auxClk/8] |= (1<<(sclocals.auxClk%8));
    if (data & 0x80) coreGlobals.tmpLampMatrix[11+sclocals.auxClk/8] |= (1<<(sclocals.auxClk%8));
    sclocals.auxClk += 1;
  }
  sclocals.auxLast = data;
}
/*---------------
/  Game handling
/----------------*/
static void init_sc(void) {
  core_gameData = &scGameData;
  install_mem_write_handler(WPC_CPUNO, WPC_SOLENOID1+WPC_BASE, WPC_SOLENOID1+WPC_BASE,sc_auxLamp);
}

