/*******************************************************************************
 Preliminary Corvette (Bally, 1994) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 Mech Handling by Destruk (destruk@vpforums.com) and Steve Ellenoff(sellenoff@hotmail.com)
 Additional engine mech by Gaston (gaston@yomail.de) and WPCMame (wpcmame@hotmail.com)
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Corvette Simulator:
  --------------------------------
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
static int  corv_handleBallState(sim_tBallStatus *ball, int *inports);
static void corv_drawStatic(BMTYPE **line);
static void init_corv(void);
static void corv_drawMech(BMTYPE **line);
static int corv_getMech(int mechNo);
static void corv_handleMech(int mech);


//Convenience Macros
#define FORWARD			0
#define REVERSE			1
#define TICKSPERPULSE	2		//# of Ticks before we pulse the Encoder switches

/*-----------------------
  local static variables
 ------------------------*/
/* Uncomment if you wish to use locals. type variables */
static struct {
  int bluecarPos;		/* Left Player Car's Position */
  int redcarPos;        /* Right Computer Car's Position */
  int enginePos;		/* The absolute position of the engine */
  int direction;        /* Race Direction */
  int active_blue;		/* # of ticks the blue car has been active*/
  int active_red;		/* # of ticks the blue car has been active*/
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(corv,4)

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
#define swSlamTilt		21
#define swCoinDoor		22
#define swTicket     	23



/* Other switches */
#define swLeftOutlane	11
#define swRightOutlane	12
#define swShooter		15
#define swLeftInlane	16
#define swRightInlane	17
#define swSpinner       18
#define swBuyIn         23
#define swFirstGear		25
#define swSecondGear	26
#define swThirdGear		27
#define swFourthGear	28
#define swTrough1		31
#define swTrough2		32
#define swTrough3		33
#define swTrough4		34
#define swRt66Entry		35
#define swPitstopPopper	36
#define swTroughJam		37
#define swInnerLoop		38
#define swZR1EntryBot	41
#define swZR1EntryTop	42
#define swSkidPadEntry	43
#define swSkidPadExit	44
#define swRt66Exit		45
#define swLeftStandup3	46
#define swLeftStandup2	47
#define swLeftStandup1	48

#define swLTRaceStart	51
#define swRTRaceStart	52
#define swLREncoder		55
#define swRREncoder		56
#define swZR1FullLeft	71
#define swZR1FullRight	72
#define swZR1Exit		75

#define swLeftSling		61
#define swRightSling	62
#define swLeftJet		63
#define swBottomJet		64
#define swRightJet		65

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough			1
#define sZR1LowRevGate	2
#define sKickBack		3
#define sPitstopPopper	4
#define sZR1UpRevGate	5
#define sKnocker		7
#define sRt66Kickout	8
#define sLeftSling		9
#define sRightSling		10
#define sLeftJet		11
#define sBottomJet		12
#define sRightJet		13
#define sZR1Lockup		15
#define sLoopGate		16
#define sRaceDirection	17
#define sLTRaceEnable	18
#define sRTRaceEnable	19
#define sTenthCorvette	20
#define sZR1Underside	26

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,stPitstopPopper
	  };

static sim_tState corv_stateDef[] = {
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
  {"Left Outlane",      1,swLeftOutlane, 0,             stDrain,        15, sKickBack, stFree, 5},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"PitStop Popper",	1,swPitstopPopper,	0,	stPitstopPopper,	1},
  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int corv_handleBallState(sim_tBallStatus *ball, int *inports) {
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

static sim_tInportData corv_inportData[] = {

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
  static void corv_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(corv,21,"corv_2_1.rom",0x80000,CRC(4fe64c6d) SHA1(f68bca3c216b7b99575fce44bd257325dbcc4f47))
DCS_SOUNDROM6x("corvsnd2",CRC(630d20a3) SHA1(c7b6cbc7f23c1f9c149a3ef32e84ca8797ff8026),
               "corvsnd3",CRC(6ace0353) SHA1(dec5b6f129ee6b7c0d03c1677d6b71672dd25a5a),
               "corvsnd4",CRC(87807278) SHA1(ba01b44c0ad6d10163a8aed2211539d541e69449),
               "corvsnd5",CRC(35f82c21) SHA1(ee14489e5629e9cd5622a56849fab65b94ff9b59),
               "corvsnd6",CRC(61e56d90) SHA1(41388523fca4839132d3f7e117bdac9ea9f4020c),
               "corvsnd7",CRC(1417b547) SHA1(851acf77159a1ef99fc2934353eb887065568004))
WPC_ROMEND

WPC_ROMSTART(corv,lx1,"u6-lx1.rom",0x80000,CRC(0e762e27) SHA1(830d9ccb00a7884e2c6d3bdf7aedac6f58af2397))
DCS_SOUNDROM6x("su2-sl1.rom",CRC(141d280e) SHA1(ab1e8e38b9fa0e693837c93616f0821e25b31588),
               "corvsnd3",CRC(6ace0353) SHA1(dec5b6f129ee6b7c0d03c1677d6b71672dd25a5a),
               "corvsnd4",CRC(87807278) SHA1(ba01b44c0ad6d10163a8aed2211539d541e69449),
               "corvsnd5",CRC(35f82c21) SHA1(ee14489e5629e9cd5622a56849fab65b94ff9b59),
               "corvsnd6",CRC(61e56d90) SHA1(41388523fca4839132d3f7e117bdac9ea9f4020c),
               "corvsnd7",CRC(1417b547) SHA1(851acf77159a1ef99fc2934353eb887065568004))
WPC_ROMEND

WPC_ROMSTART(corv,px4,"u6-px4.rom",0x80000,CRC(a5f22149) SHA1(e0b0bce31b1e66e6b74930c3184f87ebec400f80))
DCS_SOUNDROM6x("corvsnd2",CRC(630d20a3) SHA1(c7b6cbc7f23c1f9c149a3ef32e84ca8797ff8026),
               "corvsnd3",CRC(6ace0353) SHA1(dec5b6f129ee6b7c0d03c1677d6b71672dd25a5a),
               "corvsnd4",CRC(87807278) SHA1(ba01b44c0ad6d10163a8aed2211539d541e69449),
               "corvsnd5",CRC(35f82c21) SHA1(ee14489e5629e9cd5622a56849fab65b94ff9b59),
               "corvsnd6",CRC(61e56d90) SHA1(41388523fca4839132d3f7e117bdac9ea9f4020c),
               "corvsnd7",CRC(1417b547) SHA1(851acf77159a1ef99fc2934353eb887065568004))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(corv,21,"Corvette (2.1)",1994,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(corv,px4,21,"Corvette (PX4)",1994,"Bally",wpc_mSecurityS,0)
CORE_CLONEDEF(corv,lx1,21,"Corvette (LX1)",1994,"Bally",wpc_mSecurityS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData corvSimData = {
  2,    				/* 2 game specific input ports */
  corv_stateDef,				/* Definition of all states */
  corv_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  corv_handleBallState,			/*Function to handle ball state changes*/
  corv_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/* 3 extra solenoids for magnet at engine, or lights inside the engine maybe? */
static int corv_getSol(int solNo) {
  if (solNo <= CORE_CUSTSOLNO(3))
    return wpc_data[WPC_EXTBOARD3] & (1<<(solNo - CORE_CUSTSOLNO(1)));
  return 0;
}

/*----------------------
/ Game Data Information
/----------------------*/
/*--------------
/ Corvette
/---------------*/
static core_tGameData corvGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
    0,0,3,0,0,1,0, // 3 extra solenoids
    corv_getSol, corv_handleMech, corv_getMech, corv_drawMech,
    NULL, NULL
  },
  &corvSimData,
  {
  "536 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
  { 0x00, 0x00, 0x00, 0x7f, 0x07, 0x33, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
  { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_corv(void) {
  core_gameData = &corvGameData;
  locals.direction = FORWARD;
}

static void corv_drawMech(BMTYPE **line) {
  core_textOutf(30,  0,BLACK,"Blue Car: %4d", corv_getMech(0));
  core_textOutf(30, 10,BLACK,"Red Car:  %4d", corv_getMech(1));
  core_textOutf(30, 20,BLACK,"Engine:   %4d", corv_getMech(2));
}


static void corv_handleMech(int mech) {
  /* -------------------------------------
     --	Left Car Routine             --
     ------------------------------------- */

	/*If Direction Solenoid firing, cars move FORWARD (toward finish)*/
	if (core_getSol(sRaceDirection))
		locals.direction = FORWARD;
	else
		locals.direction = REVERSE;

	/* BLUE CAR*/
	if (mech & 0x01) {
		/*swLTRaceStart set while it's at home!*/
		if (locals.bluecarPos <= 0)
			core_setSw(swLTRaceStart,1);
		else
			core_setSw(swLTRaceStart,0);

		/*swLTRaceEncoder is pulsed when specified # of ticks reached!*/
		if (locals.active_blue >=TICKSPERPULSE) {
			core_setSw(swLREncoder,1);
			locals.active_blue = 0;
		}
		else
			core_setSw(swLREncoder,0);

		/*--Is Car Moving?--*/
		if (core_getSol(sLTRaceEnable)){
			locals.active_blue++;
			locals.bluecarPos += (locals.direction==FORWARD)?1:-1;
		}
	}

	/* RED CAR*/
	if (mech & 0x02) {
		/*swLTRaceStart set while it's at home!*/
		if (locals.redcarPos <= 0)
			core_setSw(swRTRaceStart,1);
		else
			core_setSw(swRTRaceStart,0);

		/*swLTRaceEncoder is pulsed when specified # of ticks reached!*/
		if (locals.active_red >=TICKSPERPULSE) {
			core_setSw(swRREncoder,1);
			locals.active_red = 0;
		}
		else
			core_setSw(swRREncoder,0);

		/*--Is Car Moving?--*/
		if (core_getSol(sRTRaceEnable)){
			locals.active_red++;
			locals.redcarPos += (locals.direction==FORWARD)?1:-1;
		}
	}

	/*Make sure positions never go negative!*/
	if(locals.bluecarPos < 0) locals.bluecarPos = 0;
	if(locals.redcarPos < 0) locals.redcarPos = 0;

	/* ZR-1 engine */
	if (mech & 0x04) {
		locals.enginePos = (int)wpc_data[WPC_EXTBOARD2];
		core_setSw(71, locals.enginePos < 64 ? 1 : 0);
		core_setSw(72, locals.enginePos > 191 ? 1 : 0);
	}
}

static int corv_getMech(int mechNo){
  switch (mechNo) {
    case 0: return locals.bluecarPos;
    case 1: return locals.redcarPos;
    case 2: return locals.enginePos;
  }
  return 0;
}
