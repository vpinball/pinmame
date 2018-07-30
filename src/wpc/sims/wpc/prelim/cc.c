/*******************************************************************************
 Preliminary Cactus Canyon (Bally, 1998) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 24, 2000

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Cactus Canyon Simulator:
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
static int  cc_handleBallState(sim_tBallStatus *ball, int *inports);
static void cc_drawStatic(BMTYPE **line);
static void init_cc(void);

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
WPC_INPUT_PORTS_START(cc,4)

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
#define sKnocker	7
#define sTrough		9
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sRightJet	13
#define sBottomJet	16

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet
	  };

static sim_tState cc_stateDef[] = {
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
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int cc_handleBallState(sim_tBallStatus *ball, int *inports) {
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

static sim_tInportData cc_inportData[] = {

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
static core_tLampDisplay cc_lampPos = {
{ 0, 0 }, /* top left */
{34, 17}, /* size */
{
{1,{{22,5,YELLOW}}},{1,{{21,7,YELLOW}}},{1,{{21,9,YELLOW}}},{1,{{21,11,YELLOW}}},
{1,{{22,13,YELLOW}}},{1,{{23,9,YELLOW}}},{1,{{1,2,WHITE}}},{1,{{1,4,WHITE}}},
{1,{{20,14,WHITE}}},{1,{{19,13,WHITE}}},{1,{{18,14,WHITE}}},{1,{{18,12,WHITE}}},
{1,{{3,11,WHITE}}},{1,{{5,11,YELLOW}}},{1,{{7,7,RED}}},{1,{{9,7,GREEN}}},
{1,{{10,11,YELLOW}}},{1,{{12,3,YELLOW}}},{1,{{21,2,YELLOW}}},{1,{{19,17,LPURPLE}}},
{1,{{17,17,LPURPLE}}},{1,{{15,17,LPURPLE}}},{1,{{13,17,RED}}},{1,{{11,17,WHITE}}},
{1,{{5,13,WHITE}}},{1,{{7,13,RED}}},{1,{{9,13,LBLUE}}},{1,{{11,13,LBLUE}}},
{1,{{13,13,LBLUE}}},{1,{{25,2,YELLOW}}},{1,{{31,4,WHITE}}},{1,{{27,1,RED}}},
{1,{{11,15,YELLOW}}},{1,{{20,16,YELLOW}}},{1,{{22,16,YELLOW}}},{1,{{12,9,GREEN}}},
{1,{{10,9,GREEN}}},{1,{{8,9,GREEN}}},{1,{{6,9,RED}}},{1,{{4,9,WHITE}}},
{1,{{18,5,LBLUE}}},{1,{{16,5,LBLUE}}},{1,{{14,5,LBLUE}}},{1,{{12,5,RED}}},
{1,{{10,5,WHITE}}},{1,{{25,16,YELLOW}}},{1,{{27,17,RED}}},{1,{{31,14,WHITE}}},
{1,{{25,6,YELLOW}}},{1,{{25,12,YELLOW}}},{1,{{29,11,YELLOW}}},{1,{{10,1,WHITE}}},
{1,{{12,1,RED}}},{1,{{14,1,LPURPLE}}},{1,{{16,1,LPURPLE}}},{1,{{18,1,LPURPLE}}},
{1,{{26,9,YELLOW}}},{1,{{34,9,RED}}},{1,{{29,7,YELLOW}}},{1,{{12,7,YELLOW}}},
{1,{{0,0,BLACK}}},{1,{{0,1,BLACK}}},{1,{{0,2,BLACK}}},{1,{{34, 1,YELLOW}}}
}
};

/* Help */
  static void cc_drawStatic(BMTYPE **line) {

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
WPC_ROMSTART(cc,10,"cc_g11.1_0",0x100000,CRC(c4e2e838) SHA1(3223dd03353dead0f41626b04c9f019d6fe1528c))
DCS_SOUNDROM6m("sav2_8.rom",CRC(94928841) SHA1(953586d6abe8222a6cd6b74e417fa4ce078efa43),
               "sav3_8.rom",CRC(a22b13f0) SHA1(5df6ea9d5059cd04bdb369c1c7255b09d64b3c65),
	       "sav4_8.rom",CRC(fe8324e2) SHA1(72c56d094cb4185a083a7da81fd527a908ce9de0),
	       "sav5_8.rom",CRC(1b2a1ff3) SHA1(2d9a5952c7ac000c47d87d198ff7ca62913ec73f),
	       "sav6_8.rom",CRC(2cccf10e) SHA1(3b9b9c87ab3c0d74eaacde416d18f3357f8302bd),
	       "sav7_8.rom",CRC(90fb1277) SHA1(502c920e1d54d285a4d4af401e574f785149da47))
WPC_ROMEND

WPC_ROMSTART(cc,12,"cc_g11.1_2",0x100000,CRC(17ad9266) SHA1(b18c4e2cc9f4269904c05e5e414675a94f96e955))
DCS_SOUNDROM6m("sav2_8.rom",CRC(94928841) SHA1(953586d6abe8222a6cd6b74e417fa4ce078efa43),
               "sav3_8.rom",CRC(a22b13f0) SHA1(5df6ea9d5059cd04bdb369c1c7255b09d64b3c65),
               "sav4_8.rom",CRC(fe8324e2) SHA1(72c56d094cb4185a083a7da81fd527a908ce9de0),
               "sav5_8.rom",CRC(1b2a1ff3) SHA1(2d9a5952c7ac000c47d87d198ff7ca62913ec73f),
	       "sav6_8.rom",CRC(2cccf10e) SHA1(3b9b9c87ab3c0d74eaacde416d18f3357f8302bd),
	       "sav7_8.rom",CRC(90fb1277) SHA1(502c920e1d54d285a4d4af401e574f785149da47))
WPC_ROMEND

WPC_ROMSTART(cc,13,"cc_g11.1_3",0x100000,CRC(7741fa4e) SHA1(adaf6b07d2f2714e87e367db28d15ae0145b6ae6))
DCS_SOUNDROM6m("sav2_8.rom",CRC(94928841) SHA1(953586d6abe8222a6cd6b74e417fa4ce078efa43),
               "sav3_8.rom",CRC(a22b13f0) SHA1(5df6ea9d5059cd04bdb369c1c7255b09d64b3c65),
	       "sav4_8.rom",CRC(fe8324e2) SHA1(72c56d094cb4185a083a7da81fd527a908ce9de0),
	       "sav5_8.rom",CRC(1b2a1ff3) SHA1(2d9a5952c7ac000c47d87d198ff7ca62913ec73f),
	       "sav6_8.rom",CRC(2cccf10e) SHA1(3b9b9c87ab3c0d74eaacde416d18f3357f8302bd),
	       "sav7_8.rom",CRC(90fb1277) SHA1(502c920e1d54d285a4d4af401e574f785149da47))
WPC_ROMEND

WPC_ROMSTART(cc,13k,"cc_g11k.1_3",0x100000,CRC(68a903b1) SHA1(e97e464c92d91cb8868ad905f218c99c57eca398))
DCS_SOUNDROM6m("sav2_8.rom",CRC(94928841) SHA1(953586d6abe8222a6cd6b74e417fa4ce078efa43),
               "sav3_8.rom",CRC(a22b13f0) SHA1(5df6ea9d5059cd04bdb369c1c7255b09d64b3c65),
	       "sav4_8.rom",CRC(fe8324e2) SHA1(72c56d094cb4185a083a7da81fd527a908ce9de0),
	       "sav5_8.rom",CRC(1b2a1ff3) SHA1(2d9a5952c7ac000c47d87d198ff7ca62913ec73f),
	       "sav6_8.rom",CRC(2cccf10e) SHA1(3b9b9c87ab3c0d74eaacde416d18f3357f8302bd),
	       "sav7_8.rom",CRC(90fb1277) SHA1(502c920e1d54d285a4d4af401e574f785149da47))
WPC_ROMEND

WPC_ROMSTART(cc,104,"cc_g11.104",0x100000,CRC(21a7b816) SHA1(0e67da694b8713e15e04bf0c49a48e14f057a737))
DCS_SOUNDROM6m("sav2_8.rom",CRC(94928841) SHA1(953586d6abe8222a6cd6b74e417fa4ce078efa43),
               "sav3_8.rom",CRC(a22b13f0) SHA1(5df6ea9d5059cd04bdb369c1c7255b09d64b3c65),
	       "sav4_8.rom",CRC(fe8324e2) SHA1(72c56d094cb4185a083a7da81fd527a908ce9de0),
	       "sav5_8.rom",CRC(1b2a1ff3) SHA1(2d9a5952c7ac000c47d87d198ff7ca62913ec73f),
	       "sav6_8.rom",CRC(2cccf10e) SHA1(3b9b9c87ab3c0d74eaacde416d18f3357f8302bd),
	       "sav7_8.rom",CRC(90fb1277) SHA1(502c920e1d54d285a4d4af401e574f785149da47))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (cc,13,   "Cactus Canyon (1.3)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cc,12,13,"Cactus Canyon (1.2)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cc,10,13,"Cactus Canyon (1.0)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cc,13k,13,"Cactus Canyon (1.3 Real Knocker patch)",1998,"Bally",wpc_m95S,0)
CORE_CLONEDEF(cc,104,13,"Cactus Canyon (1.04 Test 0.2)",1998,"Bally / The Pinball Factory",wpc_m95S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData ccSimData = {
  2,    				/* 2 game specific input ports */
  cc_stateDef,				/* Definition of all states */
  cc_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  cc_handleBallState,			/*Function to handle ball state changes*/
  cc_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData ccGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,1,0,
    NULL,NULL,NULL,NULL,&cc_lampPos,NULL
  },
  &ccSimData,
  {
    "566 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x03, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_cc(void) {
  core_gameData = &ccGameData;
  wpc_set_fastflip_addr(0x87);
}

