/*******************************************************************************
 Preliminary Elvira and the Party Monsters Pinball Simulator

 by WPCMAME

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the PinBot
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
#include "s11.h"
#include "sim.h"
#include "s11csoun.h"
/*------------------
/  Local functions
/-------------------*/
static void init_eatpm(void);
static void eatpm_drawStatic(BMTYPE **line);

/*--------------------------
/ Game specific input ports
/---------------------------*/
S11_INPUT_PORTS_START(eatpm,3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LSHIFT)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RSHIFT)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)
S11_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt     S11_SWNO( 1)
#define swRelay    S11_SWNO( 2)
#define swStart    S11_SWNO( 3)
#define swSlamTilt S11_SWNO( 7)
#define swOuthole  S11_SWNO( 9)
#define swTrough1  S11_SWNO(11)
#define swTrough2  S11_SWNO(12)
#define swTrough3  S11_SWNO(13)
#define swRStand1  S11_SWNO(15)
#define swRStand2  S11_SWNO(16)
#define swLOut     S11_SWNO(17)
#define swLIn      S11_SWNO(18)
#define swRIn      S11_SWNO(19)
#define swROut     S11_SWNO(20)
#define swShooter  S11_SWNO(21)
#define swPizzaTop S11_SWNO(22)
#define swPizzaBot S11_SWNO(23)
#define swDHwake   S11_SWNO(25)
#define swDHthee   S11_SWNO(26)
#define swDHdead   S11_SWNO(27)
#define swDHheads  S11_SWNO(28)
#define swLockEnt  S11_SWNO(29)
#define swLRampEnt S11_SWNO(30)
#define swLRampEnd S11_SWNO(31)
#define swPopper   S11_SWNO(32)
#define swLSling   S11_SWNO(33)
#define swRSling   S11_SWNO(34)
#define swLJet     S11_SWNO(35)
#define swRJet     S11_SWNO(36)
#define swBJet     S11_SWNO(37)
#define swLDrop    S11_SWNO(41)
#define swCDrop    S11_SWNO(42)
#define swRDrop    S11_SWNO(43)
#define swRRampEnt S11_SWNO(44)
#define swbAT      S11_SWNO(45)
#define swBaT      S11_SWNO(46)
#define swBAt      S11_SWNO(47)
#define swEject    S11_SWNO(48)
#define swLock1    S11_SWNO(49)
#define swLock2    S11_SWNO(50)
#define swLock3    S11_SWNO(51)
#define swLockSafe S11_SWNO(52)
#define swFlip1Tar S11_SWNO(53)
#define swFlip2Tar S11_SWNO(54)
#define swFlip1Op  S11_SWNO(55)
#define swFlip2Op  S11_SWNO(56)
#define swRFlip    S11_SWNO(57)
#define swLFlip    S11_SWNO(58)

/* Solenoids, The rest are flashes I think */
#define sOuthole 1
#define sDummy   S11_CSOL(1) /* this is just an example of a C line solenoid */
#define sRelease 2
#define sDropRel 3
#define sEject   5
#define sPopper  6
#define sKnocker 7
#define sLockRel 8
#define sLJet    17
#define sLSling  18
#define sRJet    19


/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough3=SIM_FIRSTSTATE, stTrough2, stTrough1, stShooter,
     stDrain, stOuthole,stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane,
      stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet
};

static sim_tState eatpm_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sRelease,	stShooter,	1},
  {"Shooter",		1,swShooter,	sShooterRel,	stFree,	        0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  /*Line 2*/
  {"Drain",		1,0,		0,		stOuthole,	0,	0,	0,	SIM_STNOTEXCL},
  {"Outhole",	        1,swOuthole,    sOuthole,	stTrough3,	1},
  {"Right Outlane",	1,swROut,       0,		stDrain,	15},
  {"Left Outlane",	1,swLOut,       0,		stDrain,	15},
  {"Right Inlane",	1,swRIn,        0,		stFree,		5},
  {"Left Inlane",	1,swLIn,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBJet,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData eatpm_inportData[] = {

/* Port 0 */
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x8000, stDrain},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/

/* Help */

static void eatpm_drawStatic(BMTYPE **line) {
  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Shift+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Shift+I/O = L/R Inlane/Outlane");
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
S11_ROMSTART48(eatpm,l4,"elvi_u26.l4", 0x24e09bf6,
                        "elvi_u27.l4", 0x3614f3e2)
S11S_SOUNDROM88(        "elvi_u21.l1", 0x68d44545,
                        "elvi_u22.l1", 0xe525b4fe)
S11CS_SOUNDROM888(      "elvi_u4.l1",  0xb5afa4db,
                        "elvi_u19.l1", 0x806bc350,
                        "elvi_u20.l1", 0x3d92d5fd)

S11_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(eatpm, l4, "Elvira and the Party Monsters (L-4)", 1989, "Bally", s11_mS11B_2S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData eatpmSimData = {
  1,    				/* 1 game specific input ports */
  eatpm_stateDef,				/* Definition of all states */
  eatpm_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  NULL,			/*Function to handle ball state changes*/
  eatpm_drawStatic,	/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tLCDLayout dispeatpm[] = { \
	{0,0,0,16,CORE_SEG16},{0,33,16,16,CORE_SEG16}, {0}
};

static core_tGameData eatpmGameData = {
  GEN_S11B_2, dispeatpm,
  {
    FLIP_SWNO(swLFlip,swRFlip),
    0,0,0,
    NULL, NULL, NULL, NULL,
    NULL, NULL
  },
  &eatpmSimData,
  {{0}},
  {12}
};

/*---------------
/  Game handling
/----------------*/
static void init_eatpm(void) {
  core_gameData = &eatpmGameData;
}

