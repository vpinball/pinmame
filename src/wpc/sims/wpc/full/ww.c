/*******************************************************************************
 White Water (Williams, 1993) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Dec. 2, 2000

 Known Issues/Problems with this Simulator:

 Feb 8, 2006: Added backboard lamps (GV)

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for White Water Simulator:
  --------------------------------------------------
    +R  Rapids/Whirlpool Ramps
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
    +L  L/R Loop
     Q  SDTM (Drain Ball)
   WER  Jet Bumpers
   TYU  3 Bank Top/Mid/Bottom Targets
    IO  Light/Lock Targets
 ASDFG  R/I/V/E/R Targets
    HJ  HotFoot Targets
     K  Extra Ball Target
     L  Lock Ball
     Z  Disaster Drop Ramp
     X  Left Ramp
     C  Whirlpool Hole
     V  Secret Passage
------------------------------------------------------------------------------*/
#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "machine/4094.h"

/*------------------
/  Local functions
/-------------------*/
static int  ww_handleBallState(sim_tBallStatus *ball, int *inports);
static void ww_handleMech(int mech);
static int ww_getMech(int mechNo);
static void ww_drawMech(BMTYPE **line);
static void ww_drawStatic(BMTYPE **line);
static void init_ww(void);
static const char* showbigfootpos(void);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
 int bigfootPos;	/* Bigfoot's Head Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(ww,3)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"Rpd/Wpool Ramp",        KEYCODE_T)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"L/R Loop",		KEYCODE_L)
    COREPORT_BIT(0x0080,"L/DDrop Ramp",		KEYCODE_R)
    COREPORT_BIT(0x0100,"LJet",			KEYCODE_W)
    COREPORT_BIT(0x0200,"RJet",			KEYCODE_E)
    COREPORT_BIT(0x0400,"CJet",			KEYCODE_R)
    COREPORT_BIT(0x0800,"3BankT",		KEYCODE_T)
    COREPORT_BIT(0x1000,"3BankM",		KEYCODE_Y)
    COREPORT_BIT(0x2000,"3BankB",		KEYCODE_U)
    COREPORT_BIT(0x4000,"Light",			KEYCODE_I)
    COREPORT_BIT(0x8000,"Lock",			KEYCODE_O)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"River",			KEYCODE_A)
    COREPORT_BIT(0x0002,"rIver",			KEYCODE_S)
    COREPORT_BIT(0x0004,"riVer",			KEYCODE_D)
    COREPORT_BIT(0x0008,"rivEr",			KEYCODE_F)
    COREPORT_BIT(0x0010,"riveR",			KEYCODE_G)
    COREPORT_BIT(0x0020,"Drain",			KEYCODE_Q)
    COREPORT_BIT(0x0040,"EB",			KEYCODE_K)
    COREPORT_BIT(0x0080,"Lockup",		KEYCODE_L)
    COREPORT_BIT(0x0100,"LHotFoot",		KEYCODE_Z)
    COREPORT_BIT(0x0200,"UHotFoot",		KEYCODE_X)
    COREPORT_BIT(0x0400,"WpoolPopper",		KEYCODE_J)
    COREPORT_BIT(0x0800,"Hidden",		KEYCODE_H)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swOutHole	15
#define swLeftJet	16
#define swRightJet	17
#define swCenterJet	18

#define swSlamTilt	21
#define swCoinDoor	22
#define swTicket     	23
#define swLeftOutlane	25
#define swLeftInlane	26
#define swRightInlane	27
#define swRightOutlane	28

#define swriveR		31
#define swrivEr		32
#define swriVer		33
#define swrIver		34
#define swRiver		35
#define sw3BankT	36
#define sw3BankM	37
#define sw3BankB	38

#define swLite		41
#define swLock		42
#define swLeftLoop	43
#define swRightLoop	44
#define swSecretPassage	45
#define swEnterLRamp	46
#define swEnterRapids	47
#define swEnterCanyon	48

#define swLeftSling	51
#define swRightSling	52
#define swShooter	53
#define swLArena	54
#define swRArena	55
#define swExtraBall	56
#define swCanyonMade	57
#define swBigFootCave	58

#define swWpoolPopper	61
#define swWpoolMade	62
#define swLockupR	63
#define swLockupC	64
#define swLockupL	65
#define swLRampMade	66
#define swEnterDDrop	68

#define swRapidsMade	71
#define swUHotFoot	73
#define swLHotFoot	74
#define swDDropMade	75
#define swRTrough	76
#define swCTrough	77
#define swLTrough	78

#define swBigFoot1	86
#define swBigFoot2	87

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOutHole	1
#define sTrough		2
#define sWpoolPopper	3
#define sLockupPopper	4
#define sKickBack	5
#define sDiverter	6
#define sKnocker	7
#define sLeftSling	10
#define sRightSling	11
#define sLeftJet	12
#define sRightJet	13
#define sCenterJet	14
#define sMotor		25
#define sMotorDriver	26

/*---------------------
/  Ball state handling
/----------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling, stLeftJet, stRightJet, stCenterJet,
      stEnterLRamp, stLRampMade, stMiniPF, stEnterDDrop, stDDropMade, stEnterRapids, stRapidsMade, stInRamp, stEnterCanyon, stCanyonMade, stCanyonDiv, stInCRamp1, stInCRamp2, stBigFootCave, stWhirlpool, stWpoolMade, stWpoolPopper, stDownFRamp,
      st3BankT, st3BankM, st3BankB, stLight, stLock, stLArena, stRArena, stLHotFoot, stUHotFoot, stEB,
      stRiver, strIver, striVer, strivEr, striveR,
      stEnterLockup, stLockupL, stLockupC, stLockupR,
      stLeftLoop, stLeftLoop2, stLJet, stRJet, stCJet, stRightLoop, stLeftLoop3, stRightLoop2, stSecret
	  };

static sim_tState ww_stateDef[] = {
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
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15,sKickBack,stFree,2},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Center Bumper",	1,swCenterJet,	 0,		stFree,		1},

  /*Line 3*/
  {"Enter Left Rmp",	1,swEnterLRamp,	 0,		stLRampMade,	7},
  {"Left Ramp Made",	1,swLRampMade,	 0,		stMiniPF,	5},
  {"Mini PF",		1,0,		 0,		stRightInlane,	15},
  {"Enter Dis.Drop",	1,swEnterDDrop,	 0,		stDDropMade,	4},
  {"Dis. Drop Made",	1,swDDropMade,	 0,		stFree,		5},
  {"Enter Rapids",	1,swEnterRapids, 0,		stRapidsMade,	7},
  {"Rapids Made",	1,swRapidsMade,	 0,		stInRamp,	5},
  {"In Ramp",		1,0,		 0,		stLeftInlane,	20},
  {"Enter Canyon",	1,swEnterCanyon, 0,		stCanyonMade,	5},
  {"Canyon Made",	1,swCanyonMade,	 0,		stCanyonDiv,	3},
  {"Canyon Made",	1,0,		 0,		0,		0},
  {"In Canyon Ramp",	1,0,		 0,		stWhirlpool,	15},
  {"In Canyon Ramp",	1,0,		 0,		stBigFootCave,	7},
  {"BigFoot's Cave",	1,swBigFootCave, 0,		stRightInlane,	7},
  {"Whirlpool Bowl",	1,0,		 0,		stWpoolMade,	18},
  {"Whirlpool Hole",	1,swWpoolMade,	 0,		stWpoolPopper,	8},
  {"Whirlpool Exit",	1,swWpoolPopper, sWpoolPopper,	stFree,		0},
  {"Down From Ramp",	1,swEnterLRamp,	 0,		stFree,		4},

  /*Line 4*/
  {"3 Bank Top Tgt",	1,sw3BankT,	 0,		stFree,		2},
  {"3 Bank Mid Tgt",	1,sw3BankM,	 0,		stFree,		2},
  {"3 Bank Bot Tgt",	1,sw3BankB,	 0,		stFree,		2},
  {"Lite Target",	1,swLite,	 0,		stFree,		2},
  {"Lock Target",	1,swLock,	 0,		stFree,		2},
  {"Left Arena",	1,swLArena,	 0,		stRArena,	2},
  {"Right Arena",	1,swRArena,	 0,		stFree,		2},
  {"Lower Hot Foot",	1,swLHotFoot,	 0,		stFree,		2},
  {"Upper Hot Foot",	1,swUHotFoot,	 0,		stFree,		2},
  {"Extra Ball Tgt",	1,swExtraBall,	 0,		stFree,		2},

  /*Line 5*/
  {"River - R",		1,swRiver,	 0,		stFree,		2},
  {"River - I",		1,swrIver,	 0,		stFree,		2},
  {"River - V",		1,swriVer,	 0,		stFree,		2},
  {"River - E",		1,swrivEr,	 0,		stFree,		2},
  {"River - R",		1,swriveR,	 0,		stFree,		2},

  /*Line 6*/
  {"Enter Lock",	1,0,		 0,		stLockupL,	5},
  {"Left Lock",		1,swLockupL,	 0,		stLockupC,	1},
  {"Center Lock",	1,swLockupC,	 0,		stLockupR,	1},
  {"Right Lock",	1,swLockupR,	 sLockupPopper,	stMiniPF,	0},

  /*Line 7*/
  {"Left Loop",		1,swLeftLoop,	 0,		stLeftLoop2,	5},
  {"Left Loop Made",	1,swRightLoop,	 0,		stLJet,		3},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stCJet,		3},
  {"Center Bumper",	1,swCenterJet,	 0,		stLArena,	3},
  {"Right Loop",	1,swRightLoop,	 0,		stLeftLoop3,	5},
  {"Rt. Loop Made",	1,swLeftLoop,	 0,		stFree,		3},
  {"Right Loop",	1,swRightLoop,	 0,		stSecret,	10},
  {"Secret Passage",	1,swSecretPassage,0,		stFree,		5},

  {0}
};

static int ww_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 20)
			return setState(stNotEnough,25);	/*Ball not plunged hard enough*/
		if (ball->speed < 40)
			return setState(stMiniPF,40);		/*Ball goes to MiniPF*/
		if (ball->speed < 51)
			return setState(stDownFRamp,60);	/*Ball gets down from Ramp*/
		break;

	/* Canyon Ramp Diverter */
	case stCanyonDiv:
		if (core_getSol(sDiverter))
			return setState(stInCRamp1,5);
		else
			return setState(stInCRamp2,5);

       }
    return 0;
  }

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData ww_inportData[] = {

/* Port 0 */
  {0, 0x0005, stEnterRapids},
  {0, 0x0006, stEnterCanyon},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0041, stLeftLoop},
  {0, 0x0042, stRightLoop},
  {0, 0x0081, stEnterLRamp},
  {0, 0x0082, stEnterDDrop},
  {0, 0x0100, stLeftJet},
  {0, 0x0200, stRightJet},
  {0, 0x0400, stCenterJet},
  {0, 0x0800, st3BankT},
  {0, 0x1000, st3BankM},
  {0, 0x2000, st3BankB},
  {0, 0x4000, stLight},
  {0, 0x8000, stLock},

/* Port 1 */
  {1, 0x0001, stRiver},
  {1, 0x0002, strIver},
  {1, 0x0004, striVer},
  {1, 0x0008, strivEr},
  {1, 0x0010, striveR},
  {1, 0x0020, stDrain},
  {1, 0x0040, stEB},
  {1, 0x0080, stEnterLockup},
  {1, 0x0100, stLHotFoot},
  {1, 0x0200, stUHotFoot},
  {1, 0x0400, stWpoolPopper},
  {1, 0x0800, stRightLoop2},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
  static void ww_drawMech(BMTYPE **line) {
  core_textOutf(30, 10,BLACK,"    BigFoot's Head:");
  core_textOutf(30, 20,BLACK,"       %-8s", showbigfootpos());
  core_textOutf(30, 30,BLACK,"%3d", locals.bigfootPos);
//  core_textOutf(30, 10,BLACK,"BigFoot Pos: %-4d", locals.bigfootPos);
//  core_textOutf(30, 20,BLACK,"Diverter   : %-4s", core_getSol(sDiverter) ? "On":"Off");
}
/* Help */

  static void ww_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+T = Rapids/Canyon Ramp");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+R = Left/Disaster Drop Ramp");
  core_textOutf(30,100,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,110,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,120,BLACK,"T/Y/U = 3 Bank Tgts, I/O = Light/Lock");
  core_textOutf(30,130,BLACK,"A/S/D/F/G = R/I/V/E/R Targets");
  core_textOutf(30,140,BLACK,"H = Secret Passage, K = Extra Ball Tgt");
  core_textOutf(30,150,BLACK,"J = Whirlpool Exit Hole");
  core_textOutf(30,160,BLACK,"L = Lock Hole, Z/X = HotFoot Targets");
  }

/* Solenoid-to-sample handling */
static wpc_tSamSolMap ww_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sTrough,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sCenterJet,1,SAM_JET3}, {sDiverter,1,SAM_SOLENOID_ON},
 {sDiverter,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #2*/
 {sLeftSling,2,SAM_LSLING}, {sRightSling,2,SAM_RSLING},
 {sKickBack,2,SAM_SOLENOID},

 /*Channel #3*/
 {sLockupPopper,3,SAM_POPPER}, {sWpoolPopper,3,SAM_POPPER},

 /*Channel #4*/
 {sMotor,4,SAM_MOTOR_1,WPCSAM_F_CONT},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
#define WW_SOUND \
WPCS_SOUNDROM248("ww_u18.l1",CRC(6f483215) SHA1(03053a16c106ccc7aa5a1206eb1da3f5f05ed38f), \
                 "ww_u15.l1",CRC(fe1ae71b) SHA1(8898a56866448728e7f81338ce8ad2e8cc6c7370), \
                 "ww_u14.l1",CRC(f3faa427) SHA1(fb0a266b80571b4717caa69f078b7e73e2866b6b))

WPC_ROMSTART(ww,l5,"wwatr_l5.rom",0x80000,CRC(4eb1d233) SHA1(b4eda04221e11697a7c5924c37622221fe4a47d0)) WW_SOUND WPC_ROMEND
WPC_ROMSTART(ww,lh5,"ww_lh5.rom",0x100000,CRC(5e03a182) SHA1(0a988d8e1b2b9ed9d6b012634f2b3eede5673600)) WW_SOUND WPC_ROMEND
WPC_ROMSTART(ww,lh6,"ww_lh6.rom",0x100000,CRC(bb1465cf) SHA1(2e554b0d3ceca46c6eb8852534c140916a16069d)) WW_SOUND WPC_ROMEND
WPC_ROMSTART(ww,l4,"u6-l4.rom",   0x80000,CRC(59c2def3) SHA1(99fe53f228d3e4047958ec263e92926891ea7594)) WW_SOUND WPC_ROMEND
WPC_ROMSTART(ww,l3,"u6-l3.rom",   0x80000,CRC(b8ff04d9) SHA1(a7b16306bf050ee961490abfaf904b1800bfbc3e)) WW_SOUND WPC_ROMEND
WPC_ROMSTART(ww,l2,"ww_l2.u6",    0x80000,CRC(2738acf8) SHA1(1554dd497d6aae53934e2e4a2e42bda1f87aaa02)) WW_SOUND WPC_ROMEND

WPC_ROMSTART(ww,p8,"ww_p8.u6",    0x80000,CRC(251a7f14) SHA1(8e36efc9a14d3cd31967f072bfc185461022864d))
WPCS_SOUNDROM248("ww_u18.p2",CRC(7a9ace30) SHA1(996cb73504ef73675c596e6f811047f16fbff0dd), \
                 "ww_u15.p2",CRC(8eb9033f) SHA1(9a4e269733d6ae58bf30b8b293ecb0b10dbcc8b3), \
                 "ww_u14.l1",CRC(f3faa427) SHA1(fb0a266b80571b4717caa69f078b7e73e2866b6b))
WPC_ROMEND

WPC_ROMSTART(ww,p1,"ww_p8.u6",    0x80000,CRC(251a7f14) SHA1(8e36efc9a14d3cd31967f072bfc185461022864d))
WPCS_SOUNDROM248("ww_u18.p1",CRC(e0e51ea6) SHA1(819133d55a48ea84b6d8d0dfd1316f28919361cc), \
                 "ww_u15.p1",CRC(a2a8e005) SHA1(bdbfc3f6c403d1ebef822a6381574f4a7bd19897), \
                 "ww_u14.l1",CRC(f3faa427) SHA1(fb0a266b80571b4717caa69f078b7e73e2866b6b))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF (ww,l5,   "White Water (L-5)",1993,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,lh6,l5,"White Water (LH-6)",2000,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,lh5,l5,"White Water (LH-5)",2000,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,l4,l5,"White Water (L-4)",1993,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,l3,l5,"White Water (L-3)",1993,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,l2,l5,"White Water (L-2)",1992,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,p8,l5,"White Water (P-8, P-2 sound)",1992,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ww,p1,l5,"White Water (P-8, P-1 sound)",1992,"Williams",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData wwSimData = {
  2,    				/* 2 game specific input ports */
  ww_stateDef,				/* Definition of all states */
  ww_inportData,			/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  ww_handleBallState,			/*Function to handle ball state changes*/
  ww_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData wwGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,2,0,0,0,0,0,
    NULL, ww_handleMech, ww_getMech, ww_drawMech,
    NULL, ww_samsolmap
  },
  &wwSimData,
  {     /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    {0},{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x60, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};

static core_tGameData lh5GameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),
    0,2,0,0,0,1,0,
    NULL, ww_handleMech, ww_getMech, ww_drawMech,
    NULL, ww_samsolmap
  },
  &wwSimData,
  {     /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    {0},{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x60, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};

static WRITE_HANDLER(parallel_0_out) {
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8] = data ^ 0xff;
}
static WRITE_HANDLER(parallel_1_out) {
  coreGlobals.lampMatrix[9] = coreGlobals.tmpLampMatrix[9] = data ^ 0xff;
}
static WRITE_HANDLER(qspin_0_out) {
  HC4094_data_w(1, data);
}

static HC4094interface hc4094ww = {
  2, // 2 chips
  { parallel_0_out, parallel_1_out },
  { qspin_0_out }
};

static WRITE_HANDLER(ww_wpc_w) {
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {
    HC4094_data_w (0, GET_BIT3);
    HC4094_clock_w(0, GET_BIT2);
    HC4094_clock_w(1, GET_BIT2);
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_ww(void) {
  // LH-5 version needs a longer GI smoothing delay, so we just check for years 2000 and up!
  core_gameData = Machine->gamedrv->year[0] == '2' ? &lh5GameData : &wwGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, ww_wpc_w);
  HC4094_init(&hc4094ww);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_strobe_w(0, 1);
  HC4094_strobe_w(1, 1);
}

static void ww_handleMech(int mech) {
  /* ------------------------------------------
     --	Trick BigFoot's Head into working :) --
     ------------------------------------------ */
  if (mech & 0x01) {
    if (core_getPulsedSol(sMotor)) {
      if (core_getPulsedSol(sMotorDriver)) locals.bigfootPos += 1;
      else                                 locals.bigfootPos -= 1;
      locals.bigfootPos &= 0x7f;
    }
    core_setSw(swBigFoot1, ((locals.bigfootPos/32+1) & 0x02));
    core_setSw(swBigFoot2, ((locals.bigfootPos/32+1) & 0x01));
  }
}
static int ww_getMech(int mechNo) {
  return locals.bigfootPos;
}

/*-----------------------------------
 Display Status of the BigFoot's Head
 ------------------------------------*/
static const char* showbigfootpos(void)
{
  if(locals.bigfootPos == 56 && !core_getSol(sMotor))
	return "   Up   ";
  if(locals.bigfootPos == 60 && !core_getSol(sMotor))
	return "Diverter";
  if(locals.bigfootPos == 64 && !core_getSol(sMotor))
	return " Player ";
  if(core_getSol(sMotor) && locals.bigfootPos < 33)
	return "Moving.  ";
  if(core_getSol(sMotor) && locals.bigfootPos < 66)
	return "Moving.. ";
  if(core_getSol(sMotor) && locals.bigfootPos < 100)
	return "Moving...";

/*// Alternate Effect:
  if(core_getSol(sMotor) && locals.bigfootPos < 12)
	return "Moving \\";
  if(core_getSol(sMotor) && locals.bigfootPos < 25)
	return "Moving |";
  if(core_getSol(sMotor) && locals.bigfootPos < 37)
	return "Moving /";
  if(core_getSol(sMotor) && locals.bigfootPos < 50)
	return "Moving -";
  if(core_getSol(sMotor) && locals.bigfootPos < 62)
	return "Moving \\";
  if(core_getSol(sMotor) && locals.bigfootPos < 75)
	return "Moving |";
  if(core_getSol(sMotor) && locals.bigfootPos < 87)
	return "Moving /";
  if(core_getSol(sMotor) && locals.bigfootPos < 100)
	return "Moving -";
*/
  if(locals.bigfootPos != 56 && locals.bigfootPos != 60 &&
     locals.bigfootPos != 64 && !core_getSol(sMotor))
	return " Unknown ";
	return " Unknown ";
}
