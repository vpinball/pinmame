// license:BSD-3-Clause

/*******************************************************************************
 Preliminary Congo (Williams, 1995) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Congo Simulator:
  -----------------------------
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
static int  congo_handleBallState(sim_tBallStatus *ball, int *inports);
static void congo_drawStatic(BMTYPE **line);
static void init_congo(void);

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
WPC_INPUT_PORTS_START(congo,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",			KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",	KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",KEYCODE_R)
    COREPORT_BIT(0x0200,"",			KEYCODE_T)
    COREPORT_BIT(0x0400,"",			KEYCODE_Y)
    COREPORT_BIT(0x0800,"",			KEYCODE_U)
    COREPORT_BIT(0x1000,"",			KEYCODE_I)
    COREPORT_BIT(0x2000,"",			KEYCODE_O)
    COREPORT_BIT(0x4000,"",			KEYCODE_A)
    COREPORT_BIT(0x8000,"Drain",	KEYCODE_Q)

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
#define swStart			13
#define swTilt			14
#define swSlamTilt		21
#define swCoinDoor		22
#define swTicket		23

/* Other switches */
#define swLeftOutlane	16
#define swRightInlane	17
#define swShooter		18
#define swLaunch		23
#define swLeftInlane	26
#define swRightOutlane	27
#define swTroughJam		31
#define swTrough1		32
#define swTrough2		33
#define swTrough3		34
#define swTrough4		35
#define swLeftSling		61
#define swRightSling	62
#define swLeftJet		63
#define swRightJet		64
#define swBottomJet		65

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKickBack	2
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

static sim_tState congo_stateDef[] = {
  {"Not Installed",	0,0,			0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,			0,		0,			0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,stTrough,	1},
  {"Trough Jam",	1,swTroughJam,	0,		stShooter,	1},
  {"Drain",			1,0,			0,		stTrough4,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sLaunch,stBallLane,0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,			 0,		stFree,		7,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15, sKickBack, stFree, 5},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",1,swLeftSling,	 0,		stFree,		1},
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

static int congo_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData congo_inportData[] = {

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
  static void congo_drawStatic(BMTYPE **line) {

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

// If the revision log is somehow correct, only Game ROM 2.1 should use the Sound ROM 1.1
// (all other game revisions are too old for it)
WPC_ROMSTART(congo,21,"cg_g11.2_1",0x80000,CRC(5d8435bf) SHA1(1356758fd788bbb3c7ab29abaaea7d2baac75f55))
DCS_SOUNDROM3m("cgs2v1_1.rom",CRC(2b7637ae) SHA1(5b5d7214c632a506b986c892b39b1356b2909598),
               "cgs3v1_0.rom",CRC(6cfd9fe0) SHA1(a76267f865c645648c8cb27aec2d05062a4a20b5),
               "cgs4v1_0.rom",CRC(2a1980e7) SHA1(0badf27c2b8bc7b0074dc5e606d64490470bc108))
WPC_ROMEND

// Thus the other DCS95 based game revisions should use Sound ROM 1.0,
// BUT we only have the 1.0-kit version so far (which is only intended for WPC-S based machines)
WPC_ROMSTART(congo,20,"cong2_00.rom",0x80000,CRC(e1a256ac) SHA1(f1f7a1865b5a0220e2f2ef492059df158451ca5b))
DCS_SOUNDROM3m("cgs2v1_1.rom",CRC(2b7637ae) SHA1(5b5d7214c632a506b986c892b39b1356b2909598),
               "cgs3v1_0.rom",CRC(6cfd9fe0) SHA1(a76267f865c645648c8cb27aec2d05062a4a20b5),
               "cgs4v1_0.rom",CRC(2a1980e7) SHA1(0badf27c2b8bc7b0074dc5e606d64490470bc108))
WPC_ROMEND

WPC_ROMSTART(congo,13,"cong1_30.rom",0x80000,CRC(e68c0404) SHA1(e851f42e6bd0e910fc87b9500cbacac3c088b488))
DCS_SOUNDROM3m("cgs2v1_1.rom",CRC(2b7637ae) SHA1(5b5d7214c632a506b986c892b39b1356b2909598),
               "cgs3v1_0.rom",CRC(6cfd9fe0) SHA1(a76267f865c645648c8cb27aec2d05062a4a20b5),
               "cgs4v1_0.rom",CRC(2a1980e7) SHA1(0badf27c2b8bc7b0074dc5e606d64490470bc108))
WPC_ROMEND

WPC_ROMSTART(congo,11,"cong1_10.rom",0x80000,CRC(b0b0ffd9) SHA1(26343f3bfbacf85b3f4db5aa3dad39216311a2da))
DCS_SOUNDROM3m("cgs2v1_1.rom",CRC(2b7637ae) SHA1(5b5d7214c632a506b986c892b39b1356b2909598),
               "cgs3v1_0.rom",CRC(6cfd9fe0) SHA1(a76267f865c645648c8cb27aec2d05062a4a20b5),
               "cgs4v1_0.rom",CRC(2a1980e7) SHA1(0badf27c2b8bc7b0074dc5e606d64490470bc108))
WPC_ROMEND

// Game ROM 2.0 was the first to support both WPC-S and DCS95 (the WPC-S variant was intended to be used with the playfield conversion kit)
// Sound is broken during gameplay and inc/decreasing volume, so maybe the
//  'This ROM automatically detects the WPC or WPC-95 hardware and will work with either system.' part is not working correctly due to emulation?
WPC_ROMSTART(congo,20s10k,"cong2_00.rom",0x80000,CRC(e1a256ac) SHA1(f1f7a1865b5a0220e2f2ef492059df158451ca5b))
DCS_SOUNDROM6x("su2-100.rom", CRC(c4b59ac9) SHA1(a0bc5150120777c771a181496ced71bd3f92a311),
               "su3-100.rom", CRC(1d4dbc9a) SHA1(3fac6ffb1af806d1dfcf71d85b0be21e7ea4b8d2),
               "su4-100.rom", CRC(a3e9fd93) SHA1(7d767ddf22080f9886621a5130929d7afce90472),
               "su5-100.rom", CRC(c397b3f6) SHA1(ef4cc5a08a55ae941f42d2b02213cc5c85d67b43),
               "su6-100.rom", CRC(f89a29a2) SHA1(63f69ae6a886d9eac44627edd5ee561bdb3dd418),
               "su7-100.rom", CRC(d1244d35) SHA1(7c5b3fcf8a35c417c778cd9bc741b92aaffeb444))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (congo,21,"Congo (2.1, DCS95 S1.1)",1995,"Williams",wpc_m95S,0)
CORE_CLONEDEF(congo,20,21,"Congo (2.0, DCS95 S1.1)",1995,"Williams",wpc_m95S,0)
CORE_CLONEDEF(congo,20s10k,21,"Congo (2.0, WPC-S S1.0-kit)",1995,"Williams",wpc_m95DCSS,GAME_IMPERFECT_SOUND)
CORE_CLONEDEF(congo,13,21,"Congo (1.3, DCS95 S1.1)",1995,"Williams",wpc_m95S,0)
CORE_CLONEDEF(congo,11,21,"Congo (1.1, DCS95 S1.1)",1995,"Williams",wpc_m95S,0)
// 1.0 and 0.2 (and maybe 0.3) exist, too (0.2/0.3 also with 0.4 sound ROMs)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData congoSimData = {
  2,					/* 2 game specific input ports */
  congo_stateDef,		/* Definition of all states */
  congo_inportData,		/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  congo_handleBallState,/*Function to handle ball state changes*/
  congo_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData congoGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
    0,0,0,0,0,0,0
  },
  &congoSimData,
  {
  "550 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
  { 0x00, 0x00, 0x00, 0x3f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
  { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

static core_tGameData congoDCSGameData = {
  GEN_WPC95DCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UL),
    0,0,0,0,0,1
  },
  &congoSimData,
  {
    "550 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_congo(void) {
  if (strncasecmp(Machine->gamedrv->name, "congo_20s10k", 11)) {
    core_gameData = &congoGameData;
  } else {
    core_gameData = &congoDCSGameData;
  }
  wpc_set_fastflip_addr(0x80);
}
