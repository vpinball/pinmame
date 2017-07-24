/*******************************************************************************
 Pinball Simulator Start Template (PSST)

 by Marton Larrosa (marton@mail.com)
 Dec. 1, 2000

 o Start adding data from the pinball you wish to simulate here.
 o Don't forget to replace all the xxx strings by the driver's name.
 o Use the other drivers as examples.
 o In the file WPCMAME.MAK you'll find some lines like this:

--- Cut ---
$(OBJ)/wpcgames.a: \
	$(OBJ)/wpc/games.o \
	$(OBJ)/wpc/afm.o \
	$(OBJ)/wpc/taf.o \
	$(OBJ)/wpc/t2.o
--- Cut ---

 So if you like to get your code in, add a line with your driver's name. Else
 you will get some errors from Driver.c when compiling.
 BTW, remember to remove COMPLETELY any data of the game you're simulating from
 Games.c!

 This is far, far away from compiling. First add the switches & solenoids
 (Go to T1. Switch Edges/T4. Solenoid tests and trigger them writing down their
 numbers (also remove the unused ones from the code)), so you can comment out
 the Keyboard conversion table empty items and, if you have added all the
 ROM/GameData information (found in Games.c), you'll be able to make a fresh
 build with your driver in.

 Some switches are used in the state array and you MUST give them the same name
 you found there, i.e. swLeftInlane, swRightOutlane, swLeftJet, swRightSling,
 etc.

 As DCS and WPC have different Trough systems, I've added code for both, so
 uncomment the correct code and remove the rest.

 Important code has comments.

 Read PZ.c or FH.c if you like more help on starting your first simulator.

 Feel free to ask Steve Ellenoff (sellenoff@acedsl.com) or me if you get stuck
 working on your simulator.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Simulator:
  --------------------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers

   Add more yourself.

------------------------------------------------------------------------------*/

#include "driver.h"
#include "wpc.h"
#include "wpcsim.h"

/* Remove the "#include "wpcsound.h"" and uncomment the next line if the game
   has the DCS sound system */

#include "wpcsound.h"
//#include "dcs.h"

/*------------------
/  Local functions
/-------------------*/
static int  xxx_handleBallState(wpcsim_tBallStatus *ball, int *inports);
static void xxx_handleMech(int *inports);
static void xxx_drawMech(unsigned char **line);
static void init_xxx(void);

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
WPC_INPUT_PORTS_START(xxx,3)
/*                        ^ This is the number of balls installed on the game
(DCS normally uses 4 balls, and WPC 3) */

  PORT_START /* 0 */
    WPCPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    WPCPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    WPCPORT_BIT(0x0004,"",		        KEYCODE_R)
    WPCPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    WPCPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    WPCPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    WPCPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    WPCPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    WPCPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    WPCPORT_BIT(0x0200,"",			KEYCODE_T)
    WPCPORT_BIT(0x0400,"",			KEYCODE_Y)
    WPCPORT_BIT(0x0800,"",			KEYCODE_U)
    WPCPORT_BIT(0x1000,"",			KEYCODE_I)
    WPCPORT_BIT(0x2000,"",			KEYCODE_O)
    WPCPORT_BIT(0x4000,"",			KEYCODE_A)
    WPCPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    WPCPORT_BIT(0x0001,"",			KEYCODE_S)
    WPCPORT_BIT(0x0002,"",			KEYCODE_D)
    WPCPORT_BIT(0x0004,"",			KEYCODE_F)
    WPCPORT_BIT(0x0008,"",			KEYCODE_G)
    WPCPORT_BIT(0x0010,"",			KEYCODE_H)
    WPCPORT_BIT(0x0020,"",			KEYCODE_J)
    WPCPORT_BIT(0x0040,"",			KEYCODE_K)
    WPCPORT_BIT(0x0080,"",			KEYCODE_L)
    WPCPORT_BIT(0x0100,"",			KEYCODE_Z)
    WPCPORT_BIT(0x0200,"",			KEYCODE_X)
    WPCPORT_BIT(0x0400,"",			KEYCODE_C)
    WPCPORT_BIT(0x0800,"",			KEYCODE_V)
    WPCPORT_BIT(0x1000,"",			KEYCODE_B)
    WPCPORT_BIT(0x2000,"",			KEYCODE_N)
    WPCPORT_BIT(0x4000,"",			KEYCODE_M)
    WPCPORT_BIT(0x8000,"",			KEYCODE_COMMA)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define sw		11
#define sw		12
#define swStart      	13
#define swTilt       	14
#define sw		15
#define sw		16
#define sw		17
#define sw		18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define sw	     	24
#define sw		25
#define sw		26
#define sw		27
#define sw		28

#define sw		31
#define sw		32
#define sw		33
#define sw		34
#define sw		35
#define sw		36
#define sw		37
#define sw		38

#define sw		41
#define sw		42
#define sw		43
#define sw		44
#define sw		45
#define sw		46
#define sw		47
#define sw		48

#define sw		51
#define sw		52
#define sw		53
#define sw		54
#define sw		55
#define sw		56
#define sw		57
#define sw		58

#define sw		61
#define sw		62
#define sw		63
#define sw		64
#define sw		65
#define sw		66
#define sw		67
#define sw		68

#define sw		71
#define sw		72
#define sw		73
#define sw		74
#define sw		75
#define sw		76
#define sw		77
#define sw		78

#define sw		81
#define sw		82
#define sw		83
#define sw		84
#define sw		85
#define sw		86
#define sw		87
#define sw		88

/*---------------------
/ Solenoid definitions
/----------------------*/
#define s		1
#define s		2
#define s		3
#define s		4
#define s		5
#define s		6
#define s		7
#define s		8
#define s		9
#define s		10
#define s		11
#define s		12
#define s		13
#define s		14
#define s		15
#define s		16
#define s		17
#define s		18
#define s		19
#define s		20

/*---------------------
/  Ball state handling
/----------------------*/
/* Uncomment the correct Trough system */
//enum {stRTrough=WPCSIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
//enum {stTrough4=WPCSIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
	  };

static wpcsim_tState xxx_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	WPCSIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	WPCSIM_STNOTEXCL},

  /*Line 1*/
/* Uncomment the correct Trough system again */

/* WPC
  {"Right Trough",	1,swRTrough,	 sTrough,	stShooter,	5},
  {"Center Trough",	1,swCTrough,	 0,		stRTrough,	1},
  {"Left Trough",	1,swLTrough,	 0,		stCTrough,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stLTrough,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	WPCSIM_STNOTEXCL},
*/
/* DCS
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,	stTrough,	1},
  {"Trough Jam",	1,swTroughJam,  0,		stShooter,	1},
  {"Drain",		1,0,		0,		stTrough4,	0,	0,	0,	WPCSIM_STNOTEXCL},
*/
  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	0,	0,	0,	WPCSIM_STNOTEXCL|WPCSIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	WPCSIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
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

static int xxx_handleBallState(wpcsim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 25)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
//		if (ball->speed < 35)
//			return setState(st,35);			/*Ball */
//		if (ball->speed < 51)
//			return setState(st,60);			/*Ball */
		break;
       }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static wpcsim_tInportData xxx_inportData[] = {

/* Port 0 */
  {0, 0x0005, st},
  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0200, st},
  {0, 0x0400, st},
  {0, 0x0800, st},
  {0, 0x1000, st},
  {0, 0x2000, st},
  {0, 0x4000, st},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, st},
  {1, 0x0002, st},
  {1, 0x0004, st},
  {1, 0x0008, st},
  {1, 0x0010, st},
  {1, 0x0020, st},
  {1, 0x0040, st},
  {1, 0x0080, st},
  {1, 0x0100, st},
  {1, 0x0200, st},
  {1, 0x0400, st},
  {1, 0x0800, st},
  {1, 0x1000, st},
  {1, 0x2000, st},
  {1, 0x4000, st},
  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void xxx_drawMech(unsigned char **line) {

/* Help */

  wpc_textOutf(30, 40,BLACK,"Help on this Simulator:");
  wpc_textOutf(30, 50,BLACK,"L/R Ctrl+- = L/R Slingshot");
  wpc_textOutf(30, 60,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  wpc_textOutf(30, 70,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  wpc_textOutf(30, 80,BLACK,"");
  wpc_textOutf(30, 90,BLACK,"");
  wpc_textOutf(30,100,BLACK,"");
  wpc_textOutf(30,110,BLACK,"");
  wpc_textOutf(30,120,BLACK,"");
  wpc_textOutf(30,130,BLACK,"");
  wpc_textOutf(30,140,BLACK,"");
  wpc_textOutf(30,150,BLACK,"");
  wpc_textOutf(30,160,BLACK,"");
  }

/*-----------------
/  ROM definitions  (Take them from Games.c)
/------------------*/
WPC_ROMSTART()
WPCS_SOUNDROMXXX(,
                 ,
                 )
WPC_ROMEND

/*--------------
/  Game drivers    (Found in Games.c too)
/---------------*/
WPC_GAMEDEF()

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static wpcsim_tSimData xxxSimData = {
  2,    				/* 2 game specific input ports */
  xxx_stateDef,				/* Definition of all states */
  xxx_inportData,			/* Keyboard Entries */
/* Uncomment the correct Trough system */
//  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
//  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  xxx_handleBallState,			/*Function to handle ball state changes*/
  xxx_handleMech,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/

/* Game Data from Games.c here */

/* ... */
  ,NULL, 			/* no custom solenoid handler needed */
  NULL, NULL, NULL, 		/* switch, lamp & solenoid positions */
  xxx_drawMech,			/* Function to Draw the Status of Mechanical Objects on Screen */
  &xxxSimData			/* Definition of Simulator Data */
};

/*---------------
/  Game handling
/----------------*/
static void init_xxx(void) {
  wpc_gameData = &xxxGameData;
}

static void xxx_handleMech(int *inports) {

/*
   If you have to handle motors, get up drop targets, move diverters and etc,
   add your code here. Don't forget that there are lots of sims made so you can
   get code examples from them.
   Good luck!
*/

}
