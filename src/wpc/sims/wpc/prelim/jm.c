/*******************************************************************************
 Preliminary Johnny Mnemonic (Bally/Williams, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000
 Cyber Glove Hand mech updated by Gerrit Volkenborn (gaston@yomail.de) June 2004

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Johnny Mnemonic Simulator:
  ---------------------------------------
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
static int  jm_handleBallState(sim_tBallStatus *ball, int *inports);
static void jm_drawStatic(BMTYPE **line);
static void init_jm(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int xPos;			/* Hand X Position */
  int yPos;			/* Hand Y Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(jm,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Bottom Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Right Jet",		KEYCODE_R)
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
#define swLeftOutlane	15
#define swLeftInlane	16
#define swRightInlane	17
#define swRightOutlane	18
#define swLeftSling	25
#define swRightSling	26
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftJet	44
#define swBottomJet	45
#define swRightJet	46
#define swShooter	78

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sLaunch		2
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
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stBottomJet, stRightJet
	  };

static sim_tState jm_stateDef[] = {
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
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int jm_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData jm_inportData[] = {

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

/*--------------------
  Drawing information
  --------------------*/
  static void jm_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(jm,12r,"john1_2r.rom",0x80000,CRC(fff07398) SHA1(3b9a51414498ef4c4a9d59ebd35348bca1cc7dfb))
DCS_SOUNDROM7x("jm_u2_s.1_0",CRC(4aeeff3d) SHA1(861b65b97715182385e2fe076af1fb2eb2ccc298),
               "jm_u3_s.1_0",CRC(9bf7bc43) SHA1(94fa83be84940a4db0143acc330aacded1d0d9ca),
               "jm_u4_s.1_0",CRC(2e044582) SHA1(0de30f6c223338a67f9332de038baf1398d9043e),
               "jm_u5_s.1_0",CRC(50cc06a7) SHA1(fa3072a8bc9be72fe974413094f0944d98cf3857),
               "jm_u6_s.1_0",CRC(bfc94707) SHA1(a1f4d35a4b1d80c8160e937458a8e5181f295f28),
               "jm_u7_s.1_0",CRC(9d4d9e9d) SHA1(d6e074806eed6fedc169c4849a9dd9ac2beed07e),
               "jm_u8_s.1_0",CRC(fc7af6c0) SHA1(a70dadf86d1af2122b58fdd85e938d50d113305f))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(jm,12r,"Johnny Mnemonic (1.2R)",1995,"Williams",wpc_mSecurityS,0)

static void jm_handleMech(int mech) {
	static UINT8 twobits_x, twobits_y;
	UINT8 sols = wpc_data[WPC_SOLENOID3] >> 4; /* The normal solenoids are smoothed too much */

	core_setSw(68, core_getSw(118));
	core_setSw(67, core_getSw(116));

	/* Hand X-axis */
	if (mech & 0x01) {
		if (sols & 0x02) { /* enable */
			locals.xPos += (sols & 0x01) ? 1 : -1; /* direction */
			twobits_x = locals.xPos & 0x03;
		}
		core_setSw(12, locals.xPos < 1 ? 1 : 0); /* zero-pos switch */
		core_setSw(74, twobits_x == 1 || twobits_x == 2); /* direction encoder A */
		core_setSw(75, twobits_x < 2); /* direction encoder B */
	}
	/* Hand Y-axis */
	if (mech & 0x02) {
		if (sols & 0x08) { /* enable */
			locals.yPos += (sols & 0x04) ? 1 : -1; /* direction */
			twobits_y = locals.yPos & 0x03;
		}
		core_setSw(37, locals.yPos < 1 ? 1 : 0); /* zero-pos switch */
		core_setSw(76, twobits_y < 2); /* direction encoder B */
		core_setSw(77, twobits_y == 1 || twobits_y == 2); /* direction encoder A */
	}
}

static int jm_getMech(int mechNo){
  switch (mechNo) {
    case 0: return locals.xPos + 64;
    case 1: return locals.yPos + 300;
  }
  return 0;
}

static void jm_drawMech(BMTYPE **line) {
  core_textOutf(30, 0, BLACK,"Hand X: %04d", jm_getMech(0) - 64);
  core_textOutf(30,10, BLACK,"Hand Y: %04d", jm_getMech(1) - 300);
}

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData jmSimData = {
  2,    				/* 2 game specific input ports */
  jm_stateDef,				/* Definition of all states */
  jm_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  jm_handleBallState,			/*Function to handle ball state changes*/
  jm_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData jmGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L) | FLIP_BUT(FLIP_L | FLIP_U),
    0,0,0,0,0,0,0,
    0, jm_handleMech,jm_getMech, jm_drawMech,
    0, NULL
  },
  &jmSimData,
  {
    "542 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch },
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_jm(void) {
  core_gameData = &jmGameData;
  locals.xPos = -64;
  locals.yPos = -300;
}

