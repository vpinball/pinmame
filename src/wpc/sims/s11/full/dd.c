/*******************************************************************************
 Dr. Dude (Bally,1990) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 1, 2000 (Converted to Sys11 on Feb. 22, 2001)

 Known Issues/Problems with this Simulator:

 None.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Simulator:
  --------------------------------------------------
    +R  MixMaster Ramp
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
     T  I Test Target (Top, Skill Shot)
  YUIO  Right Drop Targets
ASDFGH  Reflex Targets
     J  Heart Target
     K  Magnet Target
     L  Big Shot Target
     Z  Bag of Tricks Hole
     X  Gift of Gab Hole
     C  Right Loop
  VBNM  10 Pts Switches
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "s11.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
static int  dd_handleBallState(sim_tBallStatus *ball, int *inports);
static void dd_handleMech(int mech);
static void dd_drawStatic(BMTYPE **line);
static void init_dd(void);

/*--------------------------
/ Game specific input ports
/---------------------------*/
S11_INPUT_PORTS_START(dd,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"MixMaster",	        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",	KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0100,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0200,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x0400,"Top Target",		KEYCODE_T)
    COREPORT_BIT(0x0800,"RDrop1",		KEYCODE_Y)
    COREPORT_BIT(0x1000,"RDrop2",		KEYCODE_U)
    COREPORT_BIT(0x2000,"RDrop3",		KEYCODE_I)
    COREPORT_BIT(0x4000,"RDrop4",		KEYCODE_O)
    COREPORT_BIT(0x8000,"Drain",		KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Reflex",		KEYCODE_A)
    COREPORT_BIT(0x0002,"rEflex",		KEYCODE_S)
    COREPORT_BIT(0x0004,"reFlex",		KEYCODE_D)
    COREPORT_BIT(0x0008,"refLex",		KEYCODE_F)
    COREPORT_BIT(0x0010,"reflEx",		KEYCODE_G)
    COREPORT_BIT(0x0020,"refleX",		KEYCODE_H)
    COREPORT_BIT(0x0040,"Heart Target",		KEYCODE_J)
    COREPORT_BIT(0x0080,"Magnet",		KEYCODE_K)
    COREPORT_BIT(0x0100,"Big Shot",		KEYCODE_L)
    COREPORT_BIT(0x0200,"LPopper",		KEYCODE_Z)
    COREPORT_BIT(0x0400,"RPopper",		KEYCODE_X)
    COREPORT_BIT(0x0800,"Rt Loop",		KEYCODE_C)

S11_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt		1
#define swStart		3
#define swSlamTilt	7
#define swShooter	9
#define swOutHole	10
#define swRTrough	11
#define swCTrough	12
#define swLTrough	13
#define swHeart		14
#define swEnterRamp	15
#define swRampMade	16
#define swLeftOutlane	17
#define swRightOutlane	18
#define swRightInlane	19
#define swLeftInlane	20
#define swRDrop1	21 /* Top */
#define swRDrop2	22
#define swRDrop3	23
#define swRDrop4	24 /* Bottom */
#define swrefleX	25
#define swreflEx	26
#define swrefLex	27
#define swreFlex	28
#define swrEflex	29
#define swReflex	30
#define swBigShot	31
#define swRPopper	32
#define swMixerGabT	33
#define swMixerGabM	34
#define swMixerGabB	35
#define swMixerHeartL	36
#define swMixerHeartM	37
#define swMixerHeartR	38
#define swTopLeft10	39
#define swMixerMagT	41
#define swMixerMagM	42
#define swMixerMagB	43
#define swMidMid10	46
#define swMidBot10	47
#define swMidTop10	48
#define swITest		49
#define swMagnet	50
#define swLPopper	51
#define swLeftJet	52
#define swRightJet	53
#define swBottomJet	54
#define swLeftSling	55
#define swRightSling	56
#define swRFlip		57
#define swLFlip		58
#define swRLoop		59

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOutHole	1
#define sTrough		2
#define sLPopper	3
#define sRPopper	4
#define sKnocker	6
#define sRDrop		7
#define sMagnet		13
#define sBigShot	14
#define sMotor		16
#define sLeftJet	17
#define sLeftSling	18
#define sRightJet	19
#define sRightSling	20
#define sBottomJet	21


/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet,
      stRamp, stRampMade, stGabT, stGabM, stGabB, stHeartL, stHeartM, stHeartR, stMagT, stMagM, stMagB, stInReel,
      stITest, stRDrop1, stRDrop2, stRDrop3, stRDrop4,
      stReflex, strEflex, streFlex, strefLex, streflEx, strefleX, stHeart, stBigShot, stMagnet, stInMagnet,
      stRLoop, stITest2, stLJet, stRJet, stBJet, stTopLeft10, stMidTop10, stMidMid10, stMidBot10, stRLoopFall,
      stLPopper, stInReel2, stRPopper
	  };

static sim_tState dd_stateDef[] = {
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
  {"Enter MixMastr",	1,swEnterRamp,   0,		stRampMade,	7},
  {"MixMaster Made",	1,swRampMade,	 0,		stGabT,		5},
  {"Gab Top Tgt",	1,swMixerGabT,	 0,		stGabM,		1},
  {"Gab Mid Tgt",	1,swMixerGabM,	 0,		stGabB,		1},
  {"Gab Bot Tgt",	1,swMixerGabB,	 0,		stHeartL,	1},
  {"Heart L. Tgt",	1,swMixerHeartL, 0,		stHeartM,	1},
  {"Heart M. Tgt",	1,swMixerHeartM, 0,		stHeartR,	1},
  {"Heart R. Tgt",	1,swMixerHeartR, 0,		stMagT,		1},
  {"Magnet Top Tgt",	1,swMixerMagT,	 0,		stMagM,		1},
  {"Magnet Mid Tgt",	1,swMixerMagM,	 0,		stMagB,		1},
  {"Magnet Bot Tgt",	1,swMixerMagB,	 0,		stInReel,	1},
  {"Habitrail",		1,0,		 0,		stRightInlane,	11},

  /*Line 4*/
  {"I Test Target",	1,swITest,	 0,		stFree,		8},
  {"Right Drop 1",	1,swRDrop1,	 0,		stFree,		2,0,0,SIM_STSWKEEP},
  {"Right Drop 2",	1,swRDrop2,	 0,		stFree,		2,0,0,SIM_STSWKEEP},
  {"Right Drop 3",	1,swRDrop3,	 0,		stFree,		2,0,0,SIM_STSWKEEP},
  {"Right Drop 4",	1,swRDrop4,	 0,		stFree,		2,0,0,SIM_STSWKEEP},

  /*Line 5*/
  {"Reflex - R",	1,swReflex,	 0,		stFree,		3},
  {"Reflex - E",	1,swrEflex,	 0,		stFree,		3},
  {"Reflex - F",	1,swreFlex,	 0,		stFree,		3},
  {"Reflex - L",	1,swrefLex,	 0,		stFree,		3},
  {"Reflex - E",	1,swreflEx,	 0,		stFree,		3},
  {"Reflex - X",	1,swrefleX,	 0,		stFree,		3},
  {"Heart Target",	1,swHeart,	 0,		stFree,		3},
  {"Big Shot Tgt",	1,swBigShot,	 0,		stFree,		3},
  {"Magnet Target",	1,swMagnet,	 0,		stInMagnet,	3},
  {"In Magnet",		1,0,		 0,		stFree,		12},

  /*Line 6*/
  {"Right Loop",	1,swRLoop,	 0,		stITest2,	5},
  {"I Test Target",	1,swITest,	 0,		stLJet,		3},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stTopLeft10,	1},
  {"Top Left 10Pts",	1,swTopLeft10,	 0,		stMidTop10,	1},
  {"Mid. Top 10Pts",	1,swMidTop10,	 0,		stMidMid10,	1},
  {"Mid. Mid 10Pts",	1,swMidMid10,	 0,		stMidBot10,	1},
  {"Mid. Bot 10Pts",	1,swMidBot10,	 0,		stFree,		2},
  {"Down F. Right",	1,0,		 0,		stFree,		5},

  /*Line 7*/
  {"Bag of T. Hole",	1,swLPopper,	 sLPopper,	stInReel2,	0},
  {"Habitrail",		1,0,		 0,		stLeftInlane,	18},
  {"Gift of Gab H.",	1,swRPopper,	 sRPopper,	stInReel,	0},

  {0}
};

static int dd_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 15)
			return setState(stNotEnough,40);	/*Ball not plunged hard enough*/
		if (ball->speed < 30)
			return setState(stRLoopFall,60);	/*Ball falls from Right Loop*/
		if (ball->speed < 51)
			return setState(stRLoop,40);		/*Ball Hits I Test Target*/
			break;
       }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData dd_inportData[] = {

/* Port 0 */
  {0, 0x0005, stRamp},
  {0, 0x0006, stRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0200, stRightJet},
  {0, 0x0400, stITest},
  {0, 0x0800, stRDrop1},
  {0, 0x1000, stRDrop2},
  {0, 0x2000, stRDrop3},
  {0, 0x4000, stRDrop4},
  {0, 0x8000, stDrain},

/* Port 1 */
  {1, 0x0001, stReflex},
  {1, 0x0002, strEflex},
  {1, 0x0004, streFlex},
  {1, 0x0008, strefLex},
  {1, 0x0010, streflEx},
  {1, 0x0020, strefleX},
  {1, 0x0040, stHeart},
  {1, 0x0080, stMagnet},
  {1, 0x0100, stBigShot},
  {1, 0x0200, stLPopper},
  {1, 0x0400, stRPopper},
  {1, 0x0800, stRLoop},

/* Port 2 */

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static core_tLampDisplay dd_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{ 0,18,RED}}},{1,{{ 0,20,RED}}},{1,{{ 0,22,RED}}},{1,{{ 0,24,RED}}},
{1,{{ 0,26,RED}}},{1,{{13,26,WHITE}}},{1,{{11,27,WHITE}}},{1,{{ 9,28,WHITE}}},
{1,{{ 6,28,WHITE}}},{1,{{ 4,28,WHITE}}},{1,{{ 2,28,WHITE}}},{1,{{ 0,28,WHITE}}},
{1,{{15, 5,RED}}},{1,{{13, 4,YELLOW}}},{1,{{11, 3,WHITE}}},{1,{{ 9, 2,GREEN}}},
{1,{{ 2,22,RED}}},{1,{{ 4,24,RED}}},{1,{{ 6,22,RED}}},{1,{{ 4,20,RED}}},
{1,{{ 4,22,RED}}},{1,{{33,17,WHITE}}},{1,{{31,18,WHITE}}},{1,{{29,19,WHITE}}},
{1,{{13,16,WHITE}}},{1,{{11,15,WHITE}}},{1,{{ 9,14,WHITE}}},{1,{{ 7,13,WHITE}}},
{1,{{ 5,12,WHITE}}},{1,{{ 3,11,WHITE}}},{1,{{17,18,WHITE}}},{1,{{29,10,WHITE}}},
{1,{{ 4,18,YELLOW}}},{1,{{ 8,22,YELLOW}}},{1,{{ 4,26,YELLOW}}},{1,{{ 7,22,GREEN}}},
{1,{{ 8,18,GREEN}}},{1,{{ 8,26,GREEN}}},{1,{{33,11,YELLOW}}},{2,{{31, 1,WHITE},{31,28,WHITE}}},
{1,{{17,26,WHITE}}},{1,{{19,24,ORANGE}}},{1,{{21,23,RED}}},{1,{{23,24,YELLOW}}},
{1,{{25,26,WHITE}}},{1,{{17,11,RED}}},{1,{{15,11,RED}}},{1,{{13,11,RED}}},
{1,{{26, 7,WHITE}}},{1,{{23, 6,GREEN}}},{1,{{21, 7,GREEN}}},{1,{{19, 6,GREEN}}},
{1,{{20,20,WHITE}}},{1,{{18,21,YELLOW}}},{1,{{16,22,YELLOW}}},{1,{{14,23,YELLOW}}},
{1,{{12, 7,WHITE}}},{1,{{15, 8,ORANGE}}},{1,{{18, 9,RED}}},{1,{{30,14,RED}}},
{1,{{32,14,RED}}},{1,{{34,14,RED}}},{1,{{36,14,RED}}},{1,{{38,14,RED}}}
}
};


/* Help */

  static void dd_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = MixMaster Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,100,BLACK,"T/J/K = I Test/Heart/Magnet Targets");
  core_textOutf(30,110,BLACK,"Y/U/I/O = Right Drop Targets");
  core_textOutf(30,120,BLACK,"A/S/D/F/G/H = R/E/F/L/E/X Targets");
  core_textOutf(30,130,BLACK,"L = Big Shot Target, C = Right Loop");
  core_textOutf(30,140,BLACK,"Z/X = Bag of Tricks/Gift of Gab Hole");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap dd_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sBigShot,1,SAM_SOLENOID},

 /*Channel #2*/
 {sRDrop,2,SAM_SOLENOID},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},

 /*Channel #3*/
 {sLPopper,3,SAM_POPPER},  {sRPopper,3,SAM_POPPER},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
S11_ROMSTART48(dd,l2,"dude_u26.l2", CRC(d1e19fc2) SHA1(800329b5fd563fcd27add14da4522082c01eb86e),
                     "dude_u27.l2", CRC(654b5d4c) SHA1(e73834dbb35cf78eab68a5966e4049640e16dddf))
S11CS_SOUNDROM000(   "dude_u4.l1",  CRC(3eeef714) SHA1(74dcc83958cb62819e0ac36ca83001694faafec7),
                     "dude_u19.l1", CRC(dc7b985b) SHA1(f672d1f1fe1d1d887113ea6ccd745a78f7760526),
                     "dude_u20.l1", CRC(a83d53dd) SHA1(92a81069c42c7760888201fb0787fa7ddfbf1658))
S11_ROMEND

S11_ROMSTART48(dd,p6,"u26-pa6.11c", CRC(6f6a6e22) SHA1(2d8a1b472eb06a9f7aeea4b2f9a82f83eb4ee08a),
                     "u27-pa6.11c", CRC(26022273) SHA1(ca66139c3bd0c313d41a396c484d2c1b8f4ae536))
S11CS_SOUNDROM000(   "dude_u4.l1",  CRC(3eeef714) SHA1(74dcc83958cb62819e0ac36ca83001694faafec7),
                     "dude_u19.l1", CRC(dc7b985b) SHA1(f672d1f1fe1d1d887113ea6ccd745a78f7760526),
                     "dude_u20.l1", CRC(a83d53dd) SHA1(92a81069c42c7760888201fb0787fa7ddfbf1658))
S11_ROMEND

/*--------------
/  Game drivers
/---------------*/
static MACHINE_DRIVER_START(s11c_one)
  MDRV_IMPORT_FROM(s11_s11cS)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END
CORE_GAMEDEF(dd, l2, "Dr. Dude (LA-2)", 1990, "Bally", s11c_one,0)
CORE_CLONEDEF(dd,p6,l2, "Dr. Dude (PA-6)", 1990, "Bally", s11c_one,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData ddSimData = {
  2,    				/* 2 game specific input ports */
  dd_stateDef,				/* Definition of all states */
  dd_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  dd_handleBallState,		        /*Function to handle ball state changes*/
  dd_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tLCDLayout disp16oneline[] = { \
  {0,0,0,16,CORE_SEG16},{0,33,20,16,CORE_SEG16}, {0}
};
static core_tGameData ddGameData = {
  GEN_S11C, disp16oneline,
  {
    FLIP_SWNO(swLFlip,swRFlip),
    0,0,0,0,S11_LOWALPHA|S11_DISPINV,S11_MUXSW2,0,
    NULL, dd_handleMech, NULL, NULL,
    &dd_lampPos, dd_samsolmap
  },
  &ddSimData,
  {{ 0 }},
  {12}
};

/*---------------
/  Game handling
/----------------*/
static void init_dd(void) {
  core_gameData = &ddGameData;
}

static void dd_handleMech(int mech) {

  /* ----------------------------------------
     --	Get Up the Right Drop Target Bank  --
     ---------------------------------------- */
  /*-- if the RaiseTargets (sRDrop) Solenoid is firing, raise the targets --*/
  if ((mech & 0x01) && core_getSol(sRDrop)) {
    core_setSw(swRDrop1,0);
    core_setSw(swRDrop2,0);
    core_setSw(swRDrop3,0);
    core_setSw(swRDrop4,0);
  }

}
