/*******************************************************************************
 Preliminary NBA Fastbreak (Bally, 1997) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for NBA Fastbreak the Simulator:
  -------------------------------------
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
static int  nbaf_handleBallState(sim_tBallStatus *ball, int *inports);
static void nbaf_drawStatic(BMTYPE **line);
static void init_nbaf(void);

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
WPC_INPUT_PORTS_START(nbaf,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Middle Jet",		KEYCODE_E)
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
#define swShooter	15
#define swLeftInlane	16
#define swRightInlane	17
#define swRightJet	23
#define swLeftOutlane	26
#define swRightOutlane	27
#define swTroughJam	31
#define swTrough1	32
#define swTrough2	33
#define swTrough3	34
#define swTrough4	35
#define swLeftSling	57
#define swRightSling	58
#define swLeftJet	61
#define swMiddleJet	62

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLaunch		1
#define sKnocker	7
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sMiddleJet	13
#define sRightJet	14

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stMiddleJet, stRightJet
	  };

static sim_tState nbaf_stateDef[] = {
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
  {"Middle Bumper",	1,swMiddleJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int nbaf_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
	}
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData nbaf_inportData[] = {

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
  {0, 0x0080, stMiddleJet},
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
  static void nbaf_drawStatic(BMTYPE **line) {

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

#define NBAF_SOUND \
DCS_SOUNDROM5xm("fb-s2.1_0",CRC(32f42a82) SHA1(387636c8e9f8525e7442ccdced735392db113044), \
                "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648), \
                "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9), \
                "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d), \
                "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(nbaf,31,"fb_g11.3_1",0x80000,CRC(acd84ec2) SHA1(bd641b26e7a577be9f8705b21de4a694400945ce))
DCS_SOUNDROM5m("fb-s2.3_0",CRC(4594abd3) SHA1(d14654f0c2d29c28cae604e2dbcc9adf361b28a9),
               "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648),
               "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9),
               "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d),
               "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))
WPC_ROMEND
WPC_ROMSTART(nbaf,31a,"fb_g11.3_1",0x80000,CRC(acd84ec2) SHA1(bd641b26e7a577be9f8705b21de4a694400945ce)) NBAF_SOUND WPC_ROMEND

WPC_ROMSTART(nbaf,11,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99)) NBAF_SOUND WPC_ROMEND
WPC_ROMSTART(nbaf,11a,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99))
DCS_SOUNDROM5m("fb-s2.2_0",CRC(f950f481) SHA1(8d7c54c5f27a85889179ee690512fa69b1357bb6),
               "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648),
               "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9),
               "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d),
               "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))
WPC_ROMEND
WPC_ROMSTART(nbaf,11s,"g11-11.rom",0x80000,CRC(debfb64a) SHA1(7f50246f5fde1e7fc295be6b6bbd455e244e4c99))
DCS_SOUNDROM5xm("fb-s2.0_4",CRC(6a96f42b) SHA1(b6019bccdf62c9cf044a88d35019ebf0593b24d7),
                "fb-s3.1_0",CRC(033aa54a) SHA1(9221f3013f204a9a857aced5d774c606a7e48648),
                "fb-s4.1_0",CRC(6965a7c5) SHA1(7e72bbd3bad9accc8da1754c57c24ebdf13e57b9),
                "fb-s5.1_0",CRC(db50b79a) SHA1(9753d599cd822b55ed64bcf64955f625dc51997d),
                "fb-s6.1_0",CRC(f1633371) SHA1(a707748d3298ffb6d10d8308f4dae7982b540fa0))
WPC_ROMEND
WPC_ROMSTART(nbaf,115,"g11-115",0x80000,CRC(c0ed9848) SHA1(196d13cf93fe61db36d3bd936549210875a88948)) NBAF_SOUND WPC_ROMEND

WPC_ROMSTART(nbaf,21,"g11-21.rom",0x80000,CRC(598d33d0) SHA1(98c2bfcca573a6e790a4d3ba306953ff0fb3b042)) NBAF_SOUND WPC_ROMEND
WPC_ROMSTART(nbaf,22,"g11-22.rom",0x80000,CRC(2e7a9685) SHA1(2af250a947089469c942cf2c570063bdebd4abe4)) NBAF_SOUND WPC_ROMEND
WPC_ROMSTART(nbaf,23,"g11-23.rom",0x80000,CRC(a6ceb6de) SHA1(055387ee7da57e1a8fbce803a0dd9e67d6dbb1bd)) NBAF_SOUND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(nbaf,31,"NBA Fastbreak (3.1 - S3.0)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,31a,31,"NBA Fastbreak (3.1 - S1.0)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,11s,31,"NBA Fastbreak (1.1 - S0.4)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,11,31,"NBA Fastbreak (1.1)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,11a,31,"NBA Fastbreak (1.1 - S2.0)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,115,31,"NBA Fastbreak (1.15)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,21,31,"NBA Fastbreak (2.1)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,22,31,"NBA Fastbreak (2.2)",1997,"Bally",wpc_m95S,0)
CORE_CLONEDEF(nbaf,23,31,"NBA Fastbreak (2.3)",1997,"Bally",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData nbafSimData = {
  2,    				/* 2 game specific input ports */
  nbaf_stateDef,			/* Definition of all states */
  nbaf_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  nbaf_handleBallState,			/*Function to handle ball state changes*/
  nbaf_drawStatic,			/*Function to handle mechanical state changes*/
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

extern PINMAME_VIDEO_UPDATE(wpcdmd_update);
static PINMAME_VIDEO_UPDATE(led_update) {
  return 1;
}
static struct core_dispLayout nbaf_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)wpcdmd_update},
  {7,0, 0,  2,CORE_SEG7,(void*)led_update},
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData nbafGameData = {
  GEN_WPC95, nbaf_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1
  },
  &nbafSimData,
  {
    "553 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

static int count = 24, lastBit7, lastBit6;

static WRITE_HANDLER(nbaf_wpc_w) {
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {
	if (GET_BIT7 && !lastBit7) {
	  if (count > 0) count--;
	}
	if (GET_BIT6) {
	  coreGlobals.segments[0].w = core_bcd2seg7[count / 10];
	  coreGlobals.segments[1].w = core_bcd2seg7[count % 10];
	}
	if (!(GET_BIT6) && lastBit6) {
	  count = 24;
	}
	if (!(GET_BIT6) && !(GET_BIT7) && !lastBit6 && !lastBit7) {
	  coreGlobals.segments[0].w = coreGlobals.segments[1].w = 0;
	}
	lastBit6 = GET_BIT6;
	lastBit7 = GET_BIT7;
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_nbaf(void) {
  core_gameData = &nbafGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, nbaf_wpc_w);
}
