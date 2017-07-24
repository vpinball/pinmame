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
#include "wmssnd.h"
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
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
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
#define swTilt      1
#define swRelay     2
#define swStart     3
#define swSlamTilt  7
#define swOuthole   9
#define swTrough1  11
#define swTrough2  12
#define swTrough3  13
#define swRStand1  15
#define swRStand2  16
#define swLOut     17
#define swLIn      18
#define swRIn      19
#define swROut     20
#define swShooter  21
#define swPizzaTop 22
#define swPizzaBot 23
#define swDHwake   25
#define swDHthee   26
#define swDHdead   27
#define swDHheads  28
#define swLockEnt  29
#define swLRampEnt 30
#define swLRampEnd 31
#define swPopper   32
#define swLSling   33
#define swRSling   34
#define swLJet     35
#define swRJet     36
#define swBJet     37
#define swLDrop    41
#define swCDrop    42
#define swRDrop    43
#define swRRampEnt 44
#define swbAT      45
#define swBaT      46
#define swBAt      47
#define swEject    48
#define swLock1    49
#define swLock2    50
#define swLock3    51
#define swLockSafe 52
#define swFlip1Tar 53
#define swFlip2Tar 54
#define swFlip1Op  55
#define swFlip2Op  56
#define swRFlip    57
#define swLFlip    58

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
S11_ROMSTART48(eatpm,l4,"elvi_u26.l4", CRC(24e09bf6) SHA1(0ff686c671e8cb2b2c8a9669bf44c3b0ba32ed4d),
                        "elvi_u27.l4", CRC(3614f3e2) SHA1(3143fef8ab91ad357803d1e98b8ee953e6a194ef))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

S11_ROMSTART48(eatpm,l1,"u26-la1.rom", CRC(7a4873e6) SHA1(8e37ba2e428d83f6a84447761d99af12f5677c1d),
                        "u27-la1.rom", CRC(d1c80549) SHA1(ab7dd88c460102e7db095a2df58c567ba43d81af))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

S11_ROMSTART48(eatpm,l2,"u26-la2.rom", CRC(c4dc967d) SHA1(e12d06282176d231ffa0e2895499ebd8dd8e6e4f),
                        "u27-la2.rom", CRC(01e7aef5) SHA1(82c07635285ff9efb584043601ff5d811a1ab28b))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

S11_ROMSTART48(eatpm,4g,"u26-lg4.rom", CRC(5e196382) SHA1(e948993ae100ab3d7e1b771f4ce22e3faaad84b4),
                        "elvi_u27.l4", CRC(3614f3e2) SHA1(3143fef8ab91ad357803d1e98b8ee953e6a194ef))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

S11_ROMSTART48(eatpm,4u,"u26-lu4.rom", CRC(504366c8) SHA1(1ca667208d4dcc8a09e35cad5f57798902611d7e),
                        "elvi_u27.l4", CRC(3614f3e2) SHA1(3143fef8ab91ad357803d1e98b8ee953e6a194ef))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

S11_ROMSTART48(eatpm,p7,"u26-pa7.rom", CRC(0bcc6639) SHA1(016685f6f0ed144e673846c5d44c81baa273c949),
                        "u27-pa7.rom", CRC(c9c2bbf0) SHA1(9d23ccd26dc103edee303759f10b11ce0381223b))
S11XS_SOUNDROM88(       "elvi_u21.l1", CRC(68d44545) SHA1(8c3ea8521a44b1539cd148f142cca14184174ba7),
                        "elvi_u22.l1", CRC(e525b4fe) SHA1(be728ec33a00b93c3346428a9248b588460af945))
S11CS_SOUNDROM888(      "elvi_u4.l1",  CRC(b5afa4db) SHA1(59b72dac5301a4befa01b93da5162478682e6021),
                        "elvi_u19.l1", CRC(806bc350) SHA1(d170aef11001096da9f2f7240726662009e26f5f),
                        "elvi_u20.l1", CRC(3d92d5fd) SHA1(834d40a59be57057103d1d8ab48fdaaf7dc5eda2))
S11_ROMEND

/*--------------
/  Game drivers
/---------------*/
static MACHINE_DRIVER_START(elvira)
  MDRV_IMPORT_FROM(s11_s11aS)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END

CORE_GAMEDEF(eatpm, l4, "Elvira and the Party Monsters (LA-4)", 1989, "Bally", elvira, 0)
CORE_CLONEDEF(eatpm,l1,l4, "Elvira and the Party Monsters (LA-1)", 1989, "Bally", elvira, 0)
CORE_CLONEDEF(eatpm,l2,l4, "Elvira and the Party Monsters (LA-2)", 1989, "Bally", elvira, 0)
CORE_CLONEDEF(eatpm,4g,l4, "Elvira and the Party Monsters (LG-4)", 1989, "Bally", elvira, 0)
CORE_CLONEDEF(eatpm,4u,l4, "Elvira and the Party Monsters (LU-4)", 1989, "Bally", elvira, 0)
CORE_CLONEDEF(eatpm,p7,l4, "Elvira and the Party Monsters (PA-7)", 1989, "Bally", elvira, 0)

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
  {0,0,0,16,CORE_SEG16},{0,33,20,16,CORE_SEG16}, {0}
};

static core_tGameData eatpmGameData = {
  GEN_S11B, dispeatpm,
  { FLIP_SWNO(swLFlip,swRFlip),0,0,0,0,S11_LOWALPHA|S11_DISPINV,S11_MUXSW2},
  &eatpmSimData, {{0}}, {12}
};

/*---------------
/  Game handling
/----------------*/
static void init_eatpm(void) {
  core_gameData = &eatpmGameData;
}

