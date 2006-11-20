/*******************************************************************************
 Preliminary Doctor Who (Williams, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Doctor Who Simulator:
  ----------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  dw_handleBallState(sim_tBallStatus *ball, int *inports);
static void dw_handleMech(int mech);
static void dw_drawStatic(BMTYPE **line);
static void init_dw(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int minipf;		/* Mini Playfield Position */
  int minipfPos;	/* Mini Playfield Motor Position */
} locals;


/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(dw,3)

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
#define swStart      	13
#define swTilt       	14
#define swLeftSling	15
#define swRightSling	16
#define swShooter	17
#define swExitJets	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swRTrough	25
#define swCTrough	26
#define swLTrough	27
#define swOutHole	28

#define swPopper	31
#define swMPFHome	32
#define swEnterLRamp	33
#define swLaunch	34
#define swLRampMade	35
#define swEnterRRamp	36
#define swRRampMade	37
#define swMPFMidDoor	38

#define swEscape	41
#define sweScape	42
#define swesCape	43
#define swescApe	44
#define swescaPe	45
#define swescapE	46
#define swHangOnMade	47
#define swSelectDoctor	48

#define swRepair	51
#define swrEpair	52
#define swrePair	53
#define swrepAir	54
#define swrepaIr	55
#define swrepaiR	56
#define swTrapDoorDown	57
#define swActTransmat	58

#define swLeftJet	61
#define swRightJet	62
#define swBottomJet	63
#define swLeftOutlane	64
#define swLeftInlane	65
#define swRightInlane	66
#define swRightOutlane	67
#define swMPFLeftDoor	68

#define swMPFTargetR	71
#define swMPFTargetR2	72
#define swMPFTargetM	73
#define swMPFTargetL2	74
#define swMPFTargetL	75
#define swLeftLock	76
#define swRightLock	77
#define swLightLock	78

#define swPFGlass	82
#define swMPFRightDoor	88

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrapDoor	1
#define sShooter	2
#define sPopper		3
#define sLeftLock	4
#define sRightLock	5
#define sKnocker	7
#define sLeftSling	9
#define sRightSling	10
#define sLeftJet	11
#define sRightJet	12
#define sBottomJet	13
#define sOutHole	15
#define sTrough		16
#define sMPFDir		27
#define sMPFMotor	28

#define MINIPFHOME	0
#define MINIPFCENTER	1
#define MINIPFTOP	2

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
	stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
        stPopper, stOutFromHole
	  };

static sim_tState dw_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",	1,swRTrough,	 sTrough,	stShooter,	5},
  {"Center Trough",	1,swCTrough,	 0,		stRTrough,	1},
  {"Left Trough",	1,swLTrough,	 0,		stCTrough,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stLTrough,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooter,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		stPopper,	10,	0,	0,	SIM_STNOTEXCL},
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
  {"Right Hole",	1,swPopper,	 sPopper,	stOutFromHole,	0},
  {"Out From Hole",	1,0,		 0,		stRightInlane,	5},

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int dw_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
        }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData dw_inportData[] = {

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
//  core_textOutf(30, 0,BLACK,"MiniPF Position: %-3d",locals.minipfPos);

/* Help */

  static void dw_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 70,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30, 80,BLACK,"");
  core_textOutf(30, 90,BLACK,"");
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
WPC_ROMSTART(dw,l2,"drwho_l2.rom",0x80000,CRC(988c2deb) SHA1(e1f703a03f8fc594ae324c46ca77a54ead956094))
WPCS_SOUNDROM288("dw_u18.l1",CRC(1ca78024) SHA1(e652b07e21b59c8c497a20dfddcb9ddd8cad228d),
                 "dw_u15.l1",CRC(9b87ff26) SHA1(483206ca0abdad4557843f83f245a1c20234af64),
                 "dw_u14.l1",CRC(71f7d55b) SHA1(c3a22cfce2d10fe94a9749af7018e17e5e3bf4b3))
WPC_ROMEND

WPC_ROMSTART(dw,l1,"dw_l1.u6",0x80000,CRC(af8279af) SHA1(89651689156c0bf4abe35a538dc35c69f6575d4a))
WPCS_SOUNDROM288("dw_u18.l1",CRC(1ca78024) SHA1(e652b07e21b59c8c497a20dfddcb9ddd8cad228d),
                 "dw_u15.l1",CRC(9b87ff26) SHA1(483206ca0abdad4557843f83f245a1c20234af64),
                 "dw_u14.l1",CRC(71f7d55b) SHA1(c3a22cfce2d10fe94a9749af7018e17e5e3bf4b3))
WPC_ROMEND

WPC_ROMSTART(dw,p5,"dw_p5.u6",0x80000,CRC(2756a3ed) SHA1(ffeb2d303804c0a4ca4cbc2eb20b552529b5aabf))
WPCS_SOUNDROM288("dw_u18.l1",CRC(1ca78024) SHA1(e652b07e21b59c8c497a20dfddcb9ddd8cad228d),
                 "dw_u15.l1",CRC(9b87ff26) SHA1(483206ca0abdad4557843f83f245a1c20234af64),
                 "dw_u14.l1",CRC(71f7d55b) SHA1(c3a22cfce2d10fe94a9749af7018e17e5e3bf4b3))
WPC_ROMEND
/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(dw,l2,"Dr. Who (L-2)",1992, "Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(dw,l1,l2,"Dr. Who (L-1)",1992, "Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(dw,p5,l2,"Dr. Who (P-5)",1992, "Bally",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData dwSimData = {
  2,    				/* 2 game specific input ports */
  dw_stateDef,				/* Definition of all states */
  dw_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  dw_handleBallState,			/*Function to handle ball state changes*/
  dw_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData dwGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
    0,0,0,0,0,0,0,
    NULL, dw_handleMech, NULL, NULL,
    NULL, NULL
  },
  &dwSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x7f, 0x01, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_dw(void) {
  core_gameData = &dwGameData;
}

static void dw_handleMech(int mech) {

  /* Fool the Playfield glass switch */
  core_setSw(swPFGlass,1);

  /* This doesn't work! */
  if (mech & 0x01) {
    if (core_getSol(sMPFMotor) && core_getSol(sMPFDir))
      locals.minipfPos++;
    if (core_getSol(sMPFMotor) && !core_getSol(sMPFDir))
      locals.minipfPos--;
    if (locals.minipfPos >= 0 && locals.minipfPos < 25) {
      core_setSw(swMPFHome,1);
      locals.minipf = MINIPFHOME;
    }
    else if (locals.minipfPos >= 150 && locals.minipfPos < 175) {
      core_setSw(swMPFHome,1);
      locals.minipf = MINIPFCENTER;
    }
    else if (locals.minipfPos >= 300 && locals.minipfPos < 325) {
      core_setSw(swMPFHome,1);
      locals.minipf = MINIPFTOP;
    }
    else
      core_setSw(swMPFHome,0);

    if (locals.minipfPos < 0)
      locals.minipfPos = 350;
    if (locals.minipfPos > 350)
      locals.minipfPos = 0;
  }
}
