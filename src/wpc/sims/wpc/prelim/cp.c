/*******************************************************************************
 Preliminary The Champion Pub (Bally, 1998) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Champion Pub Simulator:
  ------------------------------------
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
#include "machine/4094.h"

/*------------------
/  Local functions
/-------------------*/
static int  cp_handleBallState(sim_tBallStatus *ball, int *inports);
static void cp_drawStatic(BMTYPE **line);
static void init_cp(void);

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
WPC_INPUT_PORTS_START(cp,4)

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
#define swLeftOutlane	16
#define swRightInlane	17
#define swShooter	18
#define swLaunch	23
#define swLeftInlane	26
#define swRightOutlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	51
#define swRightSling	52

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sTrough		2
#define sKnocker	7
#define sLeftSling	15
#define sRightSling	16

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling
	  };

static sim_tState cp_stateDef[] = {
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

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int cp_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData cp_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
//  {0, 0x0040, },
//  {0, 0x0080, },
//  {0, 0x0100, },
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
  static void cp_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball");
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
WPC_ROMSTART(cp,16,"cp_g11.1_6",0x80000,CRC(d6d0b921) SHA1(6784bd5116d239f307310d4a1ddac1068292dd60))
DCS_SOUNDROM6xm("cp_s2.bin",CRC(e0b67f6f) SHA1(48fbf01eca4fd6250df18bb5f35959100f40f6e0),
				"cp_s3.bin",CRC(68accf24) SHA1(9f86ac84ef8130592e471f1da0e05ba811dbc38b),
				"cp_s4.bin",CRC(50d1c920) SHA1(00b247853ef1f91c6245746c9311f8463b9335d1),
				"cp_s5.bin",CRC(69af347a) SHA1(d15683e6297603104e4ba777224331c24565be7c),
				"cp_s6.bin",CRC(76ca4fed) SHA1(8995e518c8dafbdd8bf994533b71f42172057b27),
				"cp_s7.bin",CRC(be619157) SHA1(b18acde4f683b5f8b2248b46bb3dc7c3e0ab1c26))
WPC_ROMEND

WPC_ROMSTART(cp,15,"cp_g11.1_5",0x80000,CRC(4255bfcb) SHA1(4ec17e6c0e07fd8d52af9d33776007930d8422c6))
DCS_SOUNDROM6xm("cp_s2.bin",CRC(e0b67f6f) SHA1(48fbf01eca4fd6250df18bb5f35959100f40f6e0),
				"cp_s3.bin",CRC(68accf24) SHA1(9f86ac84ef8130592e471f1da0e05ba811dbc38b),
				"cp_s4.bin",CRC(50d1c920) SHA1(00b247853ef1f91c6245746c9311f8463b9335d1),
				"cp_s5.bin",CRC(69af347a) SHA1(d15683e6297603104e4ba777224331c24565be7c),
				"cp_s6.bin",CRC(76ca4fed) SHA1(8995e518c8dafbdd8bf994533b71f42172057b27),
				"cp_s7.bin",CRC(be619157) SHA1(b18acde4f683b5f8b2248b46bb3dc7c3e0ab1c26))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(cp,16,"The Champion Pub (1.6)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cp,15,16,"The Champion Pub (1.5)",1998,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData cpSimData = {
  2,    				/* 2 game specific input ports */
  cp_stateDef,				/* Definition of all states */
  cp_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  cp_handleBallState,			/*Function to handle ball state changes*/
  cp_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData cpGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,4,0,0,0,1,0, // 4 extra lamp columns for the LEDs
  },
  &cpSimData,
  {
  "563 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
  { 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
  { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

static WRITE_HANDLER(parallel_0_out) {
  coreGlobals.lampMatrix[10] = coreGlobals.tmpLampMatrix[10] = data;
}
static WRITE_HANDLER(parallel_1_out) {
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8] = data;
}
static WRITE_HANDLER(parallel_2_out) {
  coreGlobals.lampMatrix[11] = coreGlobals.tmpLampMatrix[11] = data;
}
static WRITE_HANDLER(parallel_3_out) {
  coreGlobals.lampMatrix[9] = coreGlobals.tmpLampMatrix[9] = data;
}
static WRITE_HANDLER(qspin_0_out) {
  HC4094_data_w(2, data);
}
static WRITE_HANDLER(qspin_1_out) {
  HC4094_data_w(3, data);
}

static HC4094interface hc4094cp = {
  4, // 4 chips
  { parallel_0_out, parallel_1_out, parallel_2_out, parallel_3_out },
  { qspin_0_out, qspin_1_out }
};

/*
J2 - connected to the serial driver board

Pin 1 - CLK    - Sol 37
Pin 2 - DB     - Sol 40  (Strobe J5 1-24 Leds)
Pin 3 - DA     - Sol 39  (Strobe J4 1-24 Leds)
Pin 4 - Enable - Sol 38
Pin 5 - Key
Pin 6 - Gnd
Pin 7 - 12V
*/
static WRITE_HANDLER(cp_wpc_w) {
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {
    HC4094_data_w (0, GET_BIT6);
    HC4094_data_w (1, GET_BIT7);
    HC4094_strobe_w(0, GET_BIT5);
    HC4094_strobe_w(1, GET_BIT5);
    HC4094_strobe_w(2, GET_BIT5);
    HC4094_strobe_w(3, GET_BIT5);
    HC4094_clock_w(0, GET_BIT4);
    HC4094_clock_w(1, GET_BIT4);
    HC4094_clock_w(2, GET_BIT4);
    HC4094_clock_w(3, GET_BIT4);
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_cp(void) {
  core_gameData = &cpGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, cp_wpc_w);
  HC4094_init(&hc4094cp);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_oe_w(2, 1);
  HC4094_oe_w(3, 1);
}

