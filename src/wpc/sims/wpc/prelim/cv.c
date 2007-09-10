/*******************************************************************************
 Preliminary Cirqus Voltaire (Bally, 1997) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Cirqus Voltaire Simulator:
  ---------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
    WE  Jet Bumpers

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
static int  cv_handleBallState(sim_tBallStatus *ball, int *inports);
static void cv_drawStatic(BMTYPE **line);
static void init_cv(void);

/*-----------------------
  local static variables
 ------------------------*/
/* Uncomment if you wish to use locals. type variables */
static struct {
  int sol35, sol36;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(cv,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Upper Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Lower Jet",		KEYCODE_E)
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
#define swRightInlane	17
#define swShooter	18
#define swLeftInlane	26
#define swLeftOutlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	51
#define swRightSling	52
#define swUpperJet	53
#define swLowerJet	55
#define swRightOutlane	57

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKnocker	7
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sUpperJet	12
#define sLowerJet	13

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stUpperJet, stLowerJet
	  };

static sim_tState cv_stateDef[] = {
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
  {"Lower Bumper",	1,swLowerJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int cv_handleBallState(sim_tBallStatus *ball, int *inports) {
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

static sim_tInportData cv_inportData[] = {

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
  {0, 0x0080, stLowerJet},
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
static void cv_drawStatic(BMTYPE **line) {
  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E = Jet Bumpers");
  core_textOutf(30,100,BLACK,"");
  core_textOutf(30,110,BLACK,"");
  core_textOutf(30,120,BLACK,"");
  core_textOutf(30,130,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,140,BLACK,"");
  core_textOutf(30,150,BLACK,"");
  core_textOutf(30,160,BLACK,"");
}

static mech_tInitData cv_ringMech = {
  22, 43, MECH_LINEAR|MECH_REVERSE|MECH_ONEDIRSOL, 128, 128,
  {{42, 0, 4},{43, 33, 38},{44,123,127}}
};

static void cv_handleMech(int mech) {
//  if (mech & 0x01) mech_update(0);
  if (mech & 0x02) {
    if (core_getSol(35)) locals.sol35 = 10; else if (locals.sol35 > 0) locals.sol35 -= 1;
    if (core_getSol(36)) locals.sol36 = 10; else if (locals.sol36 > 0) locals.sol36 -= 1;
  }
}

static int cv_getSol(int solNo) {
  if (solNo == CORE_CUSTSOLNO(1)) return locals.sol35;
  if (solNo == CORE_CUSTSOLNO(2)) return locals.sol36;
  return 0;
}

static int cv_getMech(int mechNo) {
  return mech_getPos(mechNo);
}
static void cv_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"CV Pos: %3d", mech_getPos(0));
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(cv,14,"cirq_14.rom",0x100000,CRC(7a8bf999) SHA1(b33baabf4f6cbf8615cc00eb1286238c5aea386a))
DCS_SOUNDROM5xm("s2v1_0.rom",CRC(79dbb8ee) SHA1(f76c0db93b89beaf1e90c5f2199262e296fb1b78),
                "s3v0_4.rom",CRC(8c6c0c56) SHA1(792431cc5b06c3d5028168297614f5eb7e8af34f),
                "s4v0_4.rom",CRC(a9014b78) SHA1(abffe32ab729fb39ab2360d850c8b5476094fd92),
                "s5v0_4.rom",CRC(7e07a2fc) SHA1(f908363c968c15c0dc62e32695e5e2d0ca869391),
                "s6v0_4.rom",CRC(36ca43d3) SHA1(b599f88649c220143aa44cd5213e725e62afb0bc))
WPC_ROMEND

WPC_ROMSTART(cv, 20h, "cv200h.rom", 0x100000, CRC(138a0c3c) SHA1(dd6d4b5519ca161bd6779ed60cc7f52542a10147))
DCS_SOUNDROM5xm("s2v1_0.rom",CRC(79dbb8ee) SHA1(f76c0db93b89beaf1e90c5f2199262e296fb1b78),
                "s3v0_4.rom",CRC(8c6c0c56) SHA1(792431cc5b06c3d5028168297614f5eb7e8af34f),
                "s4v0_4.rom",CRC(a9014b78) SHA1(abffe32ab729fb39ab2360d850c8b5476094fd92),
                "s5v0_4.rom",CRC(7e07a2fc) SHA1(f908363c968c15c0dc62e32695e5e2d0ca869391),
                "s6v0_4.rom",CRC(36ca43d3) SHA1(b599f88649c220143aa44cd5213e725e62afb0bc))
WPC_ROMEND

WPC_ROMSTART(cv,10, "g11_100.rom", 0x100000, CRC(00028589) SHA1(46639c45abbdc59ca0f861824eca3efa10547123))
DCS_SOUNDROM5xm("s2v1_0.rom",CRC(79dbb8ee) SHA1(f76c0db93b89beaf1e90c5f2199262e296fb1b78),
                "s3v0_4.rom",CRC(8c6c0c56) SHA1(792431cc5b06c3d5028168297614f5eb7e8af34f),
                "s4v0_4.rom",CRC(a9014b78) SHA1(abffe32ab729fb39ab2360d850c8b5476094fd92),
                "s5v0_4.rom",CRC(7e07a2fc) SHA1(f908363c968c15c0dc62e32695e5e2d0ca869391),
                "s6v0_4.rom",CRC(36ca43d3) SHA1(b599f88649c220143aa44cd5213e725e62afb0bc))
WPC_ROMEND

WPC_ROMSTART(cv,11, "g11_110.rom", 0x100000, CRC(c7a4c104) SHA1(a96d34b2cf94591879de5b7838db0c98c9abfad8))
DCS_SOUNDROM5xm("s2v1_0.rom",CRC(79dbb8ee) SHA1(f76c0db93b89beaf1e90c5f2199262e296fb1b78),
                "s3v0_4.rom",CRC(8c6c0c56) SHA1(792431cc5b06c3d5028168297614f5eb7e8af34f),
                "s4v0_4.rom",CRC(a9014b78) SHA1(abffe32ab729fb39ab2360d850c8b5476094fd92),
                "s5v0_4.rom",CRC(7e07a2fc) SHA1(f908363c968c15c0dc62e32695e5e2d0ca869391),
                "s6v0_4.rom",CRC(36ca43d3) SHA1(b599f88649c220143aa44cd5213e725e62afb0bc))
WPC_ROMEND

WPC_ROMSTART(cv,13, "cv_g11.1_3", 0x100000, CRC(58b3bea0) SHA1(243f15c6b383921faf735caece2073cb6f88601a))
DCS_SOUNDROM5xm("s2v1_0.rom",CRC(79dbb8ee) SHA1(f76c0db93b89beaf1e90c5f2199262e296fb1b78),
                "s3v0_4.rom",CRC(8c6c0c56) SHA1(792431cc5b06c3d5028168297614f5eb7e8af34f),
                "s4v0_4.rom",CRC(a9014b78) SHA1(abffe32ab729fb39ab2360d850c8b5476094fd92),
                "s5v0_4.rom",CRC(7e07a2fc) SHA1(f908363c968c15c0dc62e32695e5e2d0ca869391),
                "s6v0_4.rom",CRC(36ca43d3) SHA1(b599f88649c220143aa44cd5213e725e62afb0bc))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(cv,14,"Cirqus Voltaire (1.4)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cv,20h, 14, "Cirqus Voltaire (2.0H)", 1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cv,10, 14, "Cirqus Voltaire (1.0)", 1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cv,11, 14, "Cirqus Voltaire (1.1)", 1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cv,13, 14, "Cirqus Voltaire (1.3)", 1997,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData cvSimData = {
  2,    				/* 2 game specific input ports */
  cv_stateDef,				/* Definition of all states */
  cv_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  cv_handleBallState,			/*Function to handle ball state changes*/
  cv_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData cvGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,2,0,0,1,0,
    cv_getSol, cv_handleMech, cv_getMech, cv_drawMech,
    NULL, NULL
  },
  &cvSimData,
  {
    "562 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_cv(void) {
  core_gameData = &cvGameData;
  mech_add(0, &cv_ringMech);
}

