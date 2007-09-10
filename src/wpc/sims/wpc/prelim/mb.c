/*******************************************************************************
 Preliminary Monster Bash (Williams, 1998) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/
/* 160401 Added mechanics simulator */
/*------------------------------------------------------------------------------
  Keys for the Monster Bash Simulator:
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
#include "mech.h"
/*------------------
/  Local functions
/-------------------*/
static int  mb_handleBallState(sim_tBallStatus *ball, int *inports);
static void mb_drawStatic(BMTYPE **line);
static void init_mb(void);
static void mb_handleMech(int mech);
static void mb_drawMech(BMTYPE **line);
static int mb_getMech(int mechNo);


/*-----------------------
  local static variables
 ------------------------*/
#if 0
/* Uncomment if you wish to use locals. type variables */
static struct {
  int bankPos;
  int frankPos;
  int dracPos;
} locals;
#endif

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(mb,4)

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
#define swTicket     	23

/* Other switches */
#define swLaunch	11
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
#define swBottomJet	55

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKnocker	7
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sRightJet	13
#define sBottomJet	14

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet
	  };

static sim_tState mb_stateDef[] = {
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

static int mb_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
        }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData mb_inportData[] = {

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
  static void mb_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(mb,106,"mb_1_06.bin",0x100000,CRC(381a8822) SHA1(b0b5bf58accff24a4023c102952c89c1f116a174))
DCS_SOUNDROM6xm("mb_s2.rom",CRC(9152f559) SHA1(68d455d8b875101caedafd21b59c447f326566dd),
                "mb_s3.rom",CRC(c3cd6e81) SHA1(b041979c8955907090b30960780cecb19258bd5e),
                "mb_s4.rom",CRC(00b88352) SHA1(5da75e0b400eb71583681e06088eb97fc12e7f17),
                "mb_s5.rom",CRC(dae16105) SHA1(15878ef8685f3e9fc8eb2a2401581d30fe706e89),
		"mb_s6.rom",CRC(3975d5da) SHA1(6dbb34a827c0956e6aef1401c12cba88ae370e1f),
		"mb_s7.rom",CRC(c242fb78) SHA1(c5a2a37ff3414d1e946cddb69b5e8f067b50bcc6))
WPC_ROMEND

WPC_ROMSTART(mb,106b,"mb_106b.bin",0x100000,CRC(c7c5d855) SHA1(96a43a955c0abaef8d6af1b64eaf50a7eeb69fe0))
DCS_SOUNDROM6xm("mb_s2.rom",CRC(9152f559) SHA1(68d455d8b875101caedafd21b59c447f326566dd),
                "mb_s3.rom",CRC(c3cd6e81) SHA1(b041979c8955907090b30960780cecb19258bd5e),
                "mb_s4.rom",CRC(00b88352) SHA1(5da75e0b400eb71583681e06088eb97fc12e7f17),
                "mb_s5.rom",CRC(dae16105) SHA1(15878ef8685f3e9fc8eb2a2401581d30fe706e89),
		"mb_s6.rom",CRC(3975d5da) SHA1(6dbb34a827c0956e6aef1401c12cba88ae370e1f),
		"mb_s7.rom",CRC(c242fb78) SHA1(c5a2a37ff3414d1e946cddb69b5e8f067b50bcc6))
WPC_ROMEND

WPC_ROMSTART(mb,10, "mb_g11.1_0", 0x100000,CRC(6b8db967) SHA1(e24d801ed9d326b9d4ddb26100c85cfd8e697d17))
DCS_SOUNDROM6xm("mb_s2.rom",CRC(9152f559) SHA1(68d455d8b875101caedafd21b59c447f326566dd),
                "mb_s3.rom",CRC(c3cd6e81) SHA1(b041979c8955907090b30960780cecb19258bd5e),
                "mb_s4.rom",CRC(00b88352) SHA1(5da75e0b400eb71583681e06088eb97fc12e7f17),
                "mb_s5.rom",CRC(dae16105) SHA1(15878ef8685f3e9fc8eb2a2401581d30fe706e89),
		"mb_s6.rom",CRC(3975d5da) SHA1(6dbb34a827c0956e6aef1401c12cba88ae370e1f),
		"mb_s7.rom",CRC(c242fb78) SHA1(c5a2a37ff3414d1e946cddb69b5e8f067b50bcc6))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (mb,10,   "Monster Bash (1.0)",1998,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mb,106,10,"Monster Bash (1.06)", 1998,"Williams",wpc_m95S,0)
CORE_CLONEDEF(mb,106b,10,"Monster Bash (1.06b)", 1998,"Williams",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData mbSimData = {
  2,    				/* 2 game specific input ports */
  mb_stateDef,				/* Definition of all states */
  mb_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  mb_handleBallState,			/*Function to handle ball state changes*/
  mb_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData mbGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1,0,
    NULL, mb_handleMech, mb_getMech, mb_drawMech,
    NULL, NULL
  },
  &mbSimData,
  {
    "565 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
#define FRANKTIME 120
#define BANKTIME  120
#define DRACTIME   90

static mech_tInitData mb_mech[] = {{
  28,0, MECH_LINEAR | MECH_REVERSE | MECH_ONESOL, BANKTIME, BANKTIME,
  {{81, 0, 10},{82, BANKTIME-10, BANKTIME-1}}
},{
  27,0,MECH_LINEAR | MECH_REVERSE | MECH_ONESOL, FRANKTIME, FRANKTIME,
  {{83, 0, 10},{84, FRANKTIME-10, FRANKTIME-1}}
},{
  41,42,MECH_LINEAR | MECH_STOPEND | MECH_TWODIRSOL, DRACTIME, DRACTIME,
  {{78, 0, 5},{77, 18, 23},{76,36,51},{75,64,69},{74,85,89}}
}};

static void mb_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"Bank: %3d", mech_getPos(0));
  core_textOutf(50,10,BLACK,"Frank: %3d",mech_getPos(1));
  core_textOutf(50,20,BLACK,"Drac: %3d", mech_getPos(2));
}
static void mb_handleMech(int mech) {
  /*-- up/dn bank --*/
  //if (mech & 0x01) mech_update(0);
  /*-- Frankenstein Table --*/
  //if (mech & 0x02) mech_update(1);
  //if (mech & 0x04) mech_update(2);
}

static int mb_getMech(int mechNo) {
  return mech_getPos(mechNo);
}

static void init_mb(void) {
  core_gameData = &mbGameData;
  mech_add(0, &mb_mech[0]);
  mech_add(1, &mb_mech[1]);
  mech_add(2, &mb_mech[2]);
}

