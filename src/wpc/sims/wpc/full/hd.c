/*******************************************************************************
 Harley Davidson (Bally, 1991) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Jan. 5, 2001

 Known Issues/Problems with this Simulator:

 None.

 Changes:

 050101 - First version

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Harley Davidson Simulator:
  ---------------------------------------
      +L  L/R Loop
      +O  L/R Outlane
      +-  L/R Slingshot
      +H  Top L/R Eject Hole
       Q  SDTM (Drain Ball)
     WER  Jet Bumpers
    TYUI  B/I/K/E Rollover
       O  Bottom Right Eject Hole
     ASD  U/S/A Rollover
  FGHJKL  H/A/R/L/E/Y Drop Targets
ZXCVBNM,  D/A/V/I/D/S/O/N Targets
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  hd_handleBallState(sim_tBallStatus *ball, int *inports);
static void hd_handleMech(int mech);
static void hd_drawStatic(BMTYPE **line);
static void init_hd(void);

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(hd,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"Top L/R Hole",		KEYCODE_H)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Loop",		KEYCODE_L)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"Bike",			KEYCODE_T)
    COREPORT_BIT(0x0400,"bIke",			KEYCODE_Y)
    COREPORT_BIT(0x0800,"biKe",			KEYCODE_U)
    COREPORT_BIT(0x1000,"bikE",			KEYCODE_I)
    COREPORT_BIT(0x2000,"Usa",			KEYCODE_A)
    COREPORT_BIT(0x4000,"uSa",			KEYCODE_S)
    COREPORT_BIT(0x8000,"usA",			KEYCODE_D)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Harley",		KEYCODE_F)
    COREPORT_BIT(0x0002,"hArley",		KEYCODE_G)
    COREPORT_BIT(0x0004,"haRley",		KEYCODE_H)
    COREPORT_BIT(0x0008,"harLey",		KEYCODE_J)
    COREPORT_BIT(0x0010,"harlEy",		KEYCODE_K)
    COREPORT_BIT(0x0020,"harleY",		KEYCODE_L)
    COREPORT_BIT(0x0040,"Davidson",		KEYCODE_Z)
    COREPORT_BIT(0x0080,"dAvidson",		KEYCODE_X)
    COREPORT_BIT(0x0100,"daVidson",		KEYCODE_C)
    COREPORT_BIT(0x0200,"davIdson",		KEYCODE_V)
    COREPORT_BIT(0x0400,"daviDson",		KEYCODE_B)
    COREPORT_BIT(0x0800,"davidSon",		KEYCODE_N)
    COREPORT_BIT(0x1000,"davidsOn",		KEYCODE_M)
    COREPORT_BIT(0x2000,"davidsoN",		KEYCODE_COMMA)
    COREPORT_BIT(0x4000,"LR Eject",		KEYCODE_O)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define	swOutHole	15
#define	swRTrough	16
#define	swCTrough	17
#define	swLTrough	18
#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swLeftJet	25
#define swRightJet	26
#define swBottomJet	27
#define swLREject	28
#define swBike		31
#define swbIke		32
#define swbiKe		33
#define swbikE		34
#define swTREject	35
#define swTLEject	36
#define swLeftSling	37
#define swRightSling	38
#define swUsa		41
#define swuSa		42
#define swusA		43
#define swHarley	51
#define swhArley	52
#define swhaRley	53
#define swharLey	54
#define swharlEy	55
#define swharleY	56
#define swDavidson	57
#define swdAvidson	58
#define swdaVidson	61
#define swdavIdson	62
#define swdaviDson	63
#define swdavidSon	64
#define swdavidsOn	65
#define swdavidsoN	66
#define swRSpinner	67
#define swLSpinner	68
#define swLLoop		71
#define swRLoop		72
#define swLeftOutlane	73
#define swRightOutlane	74
#define swShooter	75

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOutHole	1
#define sTrough		2
#define sLDrop		3
#define sCDrop		4
#define sLeftJet	5
#define sRightJet	6
#define sKnocker	7
#define sBottomJet	8
#define sLREject	9
#define sTREject	10
#define sTLEject	11
#define sRGate		12
#define sLGate		13
#define sLeftSling	15
#define sRightSling	16

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stBike, stbIke, stbiKe, stbikE, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
      stTLHole, stTRHole, stLRHole, stLLoop, stLLoop2, stRLoop, stRLoop2, stRLoop3, stRLoop4, stLLoop3, stLLoop4,
      stUsa, stuSa, stusA, stUsa2, stuSa2, stusA2, stLJet, stRJet, stBJet,
      stHarley, sthArley, sthaRley, stharLey, stharlEy, stharleY,
      stDavidson, stdAvidson, stdaVidson, stdavIdson, stdaviDson, stdavidSon, stdavidsOn, stdavidsoN
	  };

static sim_tState hd_stateDef[] = {
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
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		10,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"BIKE - B",		1,swBike,	 0,		stFree,		5},
  {"BIKE - I",		1,swbIke,	 0,		stFree,		5},
  {"BIKE - K",		1,swbiKe,	 0,		stFree,		5},
  {"BIKE - E",		1,swbikE,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 3*/
  {"Top Left Hole",	1,swTLEject,	 sTLEject,	stFree,		0},
  {"Top Right Hole",	1,swTREject,	 sTREject,	stFree,		0},
  {"Bot Right Hole",	1,swLREject,	 sLREject,	stFree,		0},
  {"Left Spinner",	1,swLSpinner,	 0,		stLLoop2,	4,0,0,SIM_STSPINNER},
  {"Left Loop",		1,swLLoop,	 0,		stuSa2,		5,sRGate,stRLoop3},
  {"Right Spinner",	1,swRSpinner,	 0,		stRLoop2,	4,0,0,SIM_STSPINNER},
  {"Right Loop",	1,swRLoop,	 0,		stuSa2,		5,sLGate,stLLoop3},
  {"Right Loop",	1,swRLoop,	 0,		stRLoop4,	4},
  {"Right Spinner",	1,swRSpinner,	 0,		stFree,		4,0,0,SIM_STSPINNER},
  {"Left Loop",		1,swLLoop,	 0,		stLLoop4,	4},
  {"Left Spinner",	1,swLSpinner,	 0,		stFree,		4,0,0,SIM_STSPINNER},

  /*Line 4*/
  {"USA - U",		1,swUsa,	 0,		stFree,		5},
  {"USA - S",		1,swuSa,	 0,		stFree,		5},
  {"USA - A",		1,swusA,	 0,		stFree,		5},
  {"USA - U",		1,swUsa,	 0,		stLJet,		5},
  {"USA - S",		1,swuSa,	 0,		stLJet,		5},
  {"USA - A",		1,swusA,	 0,		stLJet,		5},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		3},

  /*Line 5*/
  {"Harley - H",	1,swHarley,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Harley - A",	1,swhArley,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Harley - R",	1,swhaRley,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Harley - L",	1,swharLey,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Harley - E",	1,swharlEy,	 0,		stFree,		3,0,0,SIM_STSWKEEP},
  {"Harley - Y",	1,swharleY,	 0,		stFree,		3,0,0,SIM_STSWKEEP},

  /*Line 6*/
  {"DAVIDSON - D",	1,swDavidson,	 0,		stFree,		3},
  {"DAVIDSON - A",	1,swdAvidson,	 0,		stFree,		3},
  {"DAVIDSON - V",	1,swdaVidson,	 0,		stFree,		3},
  {"DAVIDSON - I",	1,swdavIdson,	 0,		stFree,		3},
  {"DAVIDSON - D",	1,swdaviDson,	 0,		stFree,		3},
  {"DAVIDSON - S",	1,swdavidSon,	 0,		stFree,		3},
  {"DAVIDSON - O",	1,swdavidsOn,	 0,		stFree,		3},
  {"DAVIDSON - N",	1,swdavidsoN,	 0,		stFree,		3},

  {0}
};

static int hd_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 15)
			return setState(stNotEnough,15);	/*Ball not plunged hard enough*/
		if (ball->speed < 30)
			return setState(stLRHole,70);		/*Ball goes to Bottom Right Hole*/
		if (ball->speed < 40)
			return setState(stusA2,100);		/*Ball goes to usA*/
		if (ball->speed < 45)
			return setState(stuSa2,100);		/*Ball goes to uSa*/
		if (ball->speed < 51)
			return setState(stUsa2,100);		/*Ball goes to Usa*/
		break;
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData hd_inportData[] = {

/* Port 0 */
  {0, 0x0005, stTLHole},
  {0, 0x0006, stTRHole},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLLoop},
  {0, 0x0022, stRLoop},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0200, stBike},
  {0, 0x0400, stbIke},
  {0, 0x0800, stbiKe},
  {0, 0x1000, stbikE},
  {0, 0x2000, stUsa},
  {0, 0x4000, stuSa},
  {0, 0x8000, stusA},

/* Port 1 */
  {1, 0x0001, stHarley},
  {1, 0x0002, sthArley},
  {1, 0x0004, sthaRley},
  {1, 0x0008, stharLey},
  {1, 0x0010, stharlEy},
  {1, 0x0020, stharleY},
  {1, 0x0040, stDavidson},
  {1, 0x0080, stdAvidson},
  {1, 0x0100, stdaVidson},
  {1, 0x0200, stdavIdson},
  {1, 0x0400, stdaviDson},
  {1, 0x0800, stdavidSon},
  {1, 0x1000, stdavidsOn},
  {1, 0x2000, stdavidsoN},
  {1, 0x4000, stLRHole},
  {1, 0x8000, stDrain},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
/* Help */

  static void hd_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+H = Top L/R Eject Hole");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,110,BLACK,"T/Y/U/I = B/I/K/E Rollover");
  core_textOutf(30,120,BLACK,"O = Bottom Right Hole");
  core_textOutf(30,130,BLACK,"A/S/D = U/S/A Rollover");
  core_textOutf(30,140,BLACK,"F/G/H/J/K/L = H/A/R/L/E/Y Target");
  core_textOutf(30,150,BLACK,"Z/X/C/V/B/N/M/, = D/A/V/I/D/S/O/N Tgt.");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap hd_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sLGate,1,SAM_SOLENOID_ON},
 {sLGate,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sRGate,1,SAM_SOLENOID_ON},
 {sRGate,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #2*/
 {sLeftSling,2,SAM_LSLING}, {sRightSling,2,SAM_RSLING},
 {sLDrop,2,SAM_SOLENOID}, {sCDrop,2,SAM_SOLENOID},

 /*Channel #3*/
 {sTLEject,3,SAM_POPPER}, {sTREject,3,SAM_POPPER},
 {sLREject,3,SAM_POPPER},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(hd,l3,"harly_l3.rom",0x020000,CRC(65f2e0b4) SHA1(a44216c13b9f9adf4161ff6f9eeceba28ef37963))
WPCS_SOUNDROM22x("hd_u18.rom",CRC(810d98c0) SHA1(8080cbbe0f346020b2b2b8e97015dbb615dbadb3),
		 "hd_u15.rom",CRC(e7870938) SHA1(b4f28146a5e7baa8522db65b41311afaf49604c6))
WPC_ROMEND

WPC_ROMSTART(hd,l1,"u6-l1.rom",0x020000,CRC(a0bdcfbf) SHA1(f906ffa2d4d04e87225bf711a07dd3bee1655a40))
WPCS_SOUNDROM22x("u18-sp1.rom",CRC(708aa419) SHA1(cfc2692fb3bcbacceb85021e282bfbc8dcdf8fcc),
		 "hd_u15.rom",CRC(e7870938) SHA1(b4f28146a5e7baa8522db65b41311afaf49604c6))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
static MACHINE_DRIVER_START(hd)
  MDRV_IMPORT_FROM(wpc_alpha2S)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END

CORE_GAMEDEF(hd,l3,"Harley Davidson (L-3)",1991,"Bally", hd, 0)
CORE_CLONEDEF(hd,l1,l3,"Harley Davidson (L-1)",1991,"Bally", hd, 0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData hdSimData = {
  2,    				/* 2 game specific input ports */
  hd_stateDef,				/* Definition of all states */
  hd_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  hd_handleBallState,			/*Function to handle ball state changes*/
  hd_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tLCDLayout disphd[] = { \
	{0,0,0,16,CORE_SEG16R},{0,33,20,16,CORE_SEG16R}, {0}
};

static core_tGameData hdGameData = {
  GEN_WPCALPHA_2, disphd,
  {
    FLIP_SWNO(12,11),
    0,0,0,0,0,0,0,
    NULL, hd_handleMech, NULL, NULL,
    NULL, hd_samsolmap
  },
  &hdSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_hd(void) {
  core_gameData = &hdGameData;
}

static void hd_handleMech(int mech) {
  /* --------------------------------------
     -- Raise the Left Drop Target Bank  --
     -------------------------------------- */
  if ((mech & 0x01) && core_getSol(sLDrop))
    { core_setSw(swHarley,0) ; core_setSw(swhArley,0) ; core_setSw(swhaRley,0); }

  /* ----------------------------------------
     -- Raise the Center Drop Target Bank  --
     ---------------------------------------- */
  if ((mech & 0x02) && core_getSol(sCDrop))
    { core_setSw(swharLey,0) ; core_setSw(swharlEy,0) ; core_setSw(swharleY,0); }

}

