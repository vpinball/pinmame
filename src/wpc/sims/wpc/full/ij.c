/*******************************************************************************
 Indiana Jones (Williams) Pinball Simulator

 by Tom Haukap (tom@rattle.de)
 Dec. 9, 2000

 091200  First version
 101200  Added help, some cosmetical fixes (VERY good driver, Tom!) - ML.
 111200  Thanx Marton - TH.
 111200  Corrected the right ramp logic; changed one text
 010101  Added sample support, forgotten sSubwayRelease sol (#28)
 140501  Corrected handlemech/getMech
 ******************************************************************************/

/*----------------------------------------------------------------------------
  Keys for Indiana Jones Simulator:
  ---------------------------------
    +R  L/R Ramp
    +O  L/R Outlane
     O  R Outlane (not draining)
    +I  L/R Inlane
    +L  L/R Loop
    +-  L/R Slingshot
    +=  L/R Top Switches PoA
    +[  L/R Top Middle Switches PoA
    +'  L/R Bottom Middle switches PoA
    +/  L/R Bottom Switches PoA
     I  PoA pit hole
     N  PoA extra ball hole
     B  Totem drop down / hit captured ball
     M  Mode start
     C  Center hole
     W  Left jet
     E  Right jet
     S  Bottom jet
     G  (I)NDY lane
     H  I(N)DY lane
     J  IN(D)Y lane
     K  IND(Y) lane
     7* (A)DVENTURE
     8* A(D)VENTURE
     9* AD(V)ENTURE
     4* ADV(E)NTURE
     5* ADVE(N)TURE
     6* ADVEN(T)URE
     1* ADVENT(U)RE
     2* ADVENTU(R)E
     3* ADVENTUR(E)
     0* Center target
     2  Buy-In
     Q  Drain (except for the captured ball)

* numbers on keypad
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

#define IJ_IDOLTICK      2     /* turn 2 degrees per simulator tick */
#define IJ_POA_MIDPOS    180
#define IJ_POA_LLIMIT    176
#define IJ_POA_RLIMIT    184

#define IJ_CAPTUREDBALL  1     /* the first ball will be used to simulate the captured ball  /
                               /  this is the marker for the ball                           */

/*------------------
/  Local functions
/-------------------*/
static int ij_handleBallState(sim_tBallStatus *ball, int *inports);
static int ij_getSol(int solNo);
static void ij_handleMech(int mech);
static void ij_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls);
static int ij_getMech(int mechNo);
static void ij_drawMech(BMTYPE **line);
static void ij_drawStatic(BMTYPE **line);
static void init_ij(void);

/*-----------------------
/ local static variables
/------------------------*/
static struct ij_tLocals {
  int idolPos;    		/* idol motor position (degree) */
  int PoAPos;     		/* PoA position (degree) */
  int hitCaptiveBall;	/* flag is set if captured ball should be hit */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(ij,7) /* 7 balls: 6 plus the captured one */
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001, "Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002, "Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004, "L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008, "L/R Outlane",     KEYCODE_O)
    COREPORT_BIT(   0x0010, "R Outlane NE",    KEYCODE_O)
    COREPORT_BITIMP(0x0020, "L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040, "L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080, "L/R Inlane",      KEYCODE_I)
    COREPORT_BITIMP(0x0100, "L/R PoA Top",     KEYCODE_EQUALS)
    COREPORT_BITIMP(0x0200, "L/R PoA M Top",   KEYCODE_OPENBRACE)
    COREPORT_BITIMP(0x0400, "L/R PoA M Bot",   KEYCODE_QUOTE)
    COREPORT_BITIMP(0x0800, "L/R PoA Bottom",  KEYCODE_SLASH)
    COREPORT_BITIMP(0x1000, "Totem Target/Ball",KEYCODE_B)
    COREPORT_BITIMP(0x2000, "Center Hole",     KEYCODE_C)
    COREPORT_BIT(   0x4000, "Center Target",   KEYCODE_0_PAD)
    COREPORT_BITIMP(0x8000, "Mode Start",      KEYCODE_M)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001, "Left Jet",        KEYCODE_W)
    COREPORT_BIT(   0x0002, "Right Jet",       KEYCODE_E)
    COREPORT_BIT(   0x0004, "Bottom Jet",      KEYCODE_S)
    COREPORT_BIT(   0x0008, "(I)NDY",   	      KEYCODE_G)
    COREPORT_BIT(   0x0010, "I(N)DY",   	      KEYCODE_H)
    COREPORT_BIT(   0x0020, "IN(D)Y",   	      KEYCODE_J)
    COREPORT_BIT(   0x0040, "IND(Y)",          KEYCODE_K)
    COREPORT_BIT(   0x0080, "(A)DVENTURE",     KEYCODE_7_PAD)
    COREPORT_BIT(   0x0100, "A(D)VENTURE",     KEYCODE_8_PAD)
    COREPORT_BIT(   0x0200, "AD(V)ENTURE",     KEYCODE_9_PAD)
    COREPORT_BITIMP(0x0400, "ADV(E)NTURE",     KEYCODE_4_PAD)
    COREPORT_BITIMP(0x0800, "ADVE(N)TURE",     KEYCODE_5_PAD)
    COREPORT_BITIMP(0x1000, "ADVEN(T)URE",     KEYCODE_6_PAD)
    COREPORT_BIT(   0x2000, "ADVENT(U)RE",     KEYCODE_1_PAD)
    COREPORT_BIT(   0x4000, "ADVENTU(R)E",     KEYCODE_2_PAD)
    COREPORT_BIT(   0x8000, "ADVENTUR(E)",     KEYCODE_3_PAD)
  PORT_START /* 2 */
    COREPORT_BITIMP(0x1000, "PoA Pit",         KEYCODE_I)
    COREPORT_BITIMP(0x2000, "PoA EB",          KEYCODE_N)
    COREPORT_BIT(   0x4000, "Buy-In",   	      KEYCODE_2)
    COREPORT_BITIMP(0x8000, "Drain",           KEYCODE_Q)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swSDropTop	11
#define swBuyIn		12
#define	swStart		13
#define swTilt		14
#define swLOut		15
#define swLIn		16
#define swRIn		17
#define swRTOut		18

#define swSlamTilt      21
#define swCoinDoor      22
#define swTicketOpto    23
#define swAlwaysClosed	24
#define swIndy		25
#define swiNdy		26
#define swinDy		27
#define	swindY		28

#define swLEject	31
#define swExitIdol	32
#define swLSling	33
#define swGunTrigger	34
#define swLJet		35
#define swRJet		36
#define swBJet		37
#define swCStandup	38

#define swLRampEnter	41
#define swRRampEnter	42
#define swIdolEnter	43
#define swRPopper	44
#define swCEnter	45
#define swTPost		46
#define swSLookup	47
#define swRSling	48

#define swadventUre	51
#define swadventuRe	52
#define swadventurE	53
#define swLTLoop	54
#define swLBLoop	55
#define swRTLoop	56
#define swRBLoop	57
#define swRBOut		58

#define swAdventure	61
#define swaDventure	62
#define swadVenture	63
#define swBCapBall	64
#define swTL_PoA	65
#define swMTL_PoA	66
#define swMBL_PoA	67
#define swBL_PoA	68

#define swFCapBall	71
#define swPitHole	72
#define swEBHole	73
#define swRRampMade	74
#define swTR_PoA	75
#define swMTR_PoA	76
#define swMBR_PoA	77
#define swBR_PoA	78

#define swTrough6	81
#define swTrough5	82
#define swTrough4	83
#define swTrough3	84
#define swTrough2	85
#define swTrough1	86
#define swTTrough	87
#define swShooter	88

#define swIdolPos1	CORE_CUSTSWNO(1,1)
#define swIdolPos2	CORE_CUSTSWNO(1,2)
#define swIdolPos3	CORE_CUSTSWNO(1,3)
#define swLL_PoA	CORE_CUSTSWNO(1,4)
#define swRL_PoA	CORE_CUSTSWNO(1,5)


#define swCLDrop       115
#define swCMDrop       116
#define swCRDrop       117
#define swLRampMade    118


/*----------------------
/ Solenoid definitions
/----------------------*/
#define sBallPopper		 1
#define sBallLaunch		 2
#define sTotemDropUp	 3
#define sBallRelease	 4
#define sCenterDropUp	 5
#define sIdolRelease	 6
#define sKnocker		 7
#define sLEject	 		 8
#define sLJet			 9
#define sRJet			10
#define sBJet			11
#define sRSling       	12
#define sLSling       	13
#define sLControlGate	14
#define sRControlGate	15
#define sTotemDropDown	16

#define sL_PoA			22
#define sR_PoA			23
#define sSubwayRelease	28
#define sDivPower		33
#define sDivHold		34
#define sTopPostPower	35
#define sTopPostHold	36

#define sWheelMotor		CORE_CUSTSOLNO(6)

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stTTrough,
      stDrain, stDraining, stShooter, stBallLane,

      stRIn, stLIn, stLSling, stRSling, stLJet, stRJet, stBJet,

      stLBLoop, stLTLoop, stLTLoopD, stLBLoopD,

      stRBLoop, stRTLoop, stRTLoopD, stRBLoopD,

      stLRampEnter, stLRamp, stRRampEnter, stRRamp, stRopeBridge, stBypassPoA,

      stLOut, stROut, stROut2, stRTOut,

      stTopPost,
      stPoAT, stPoATL, stPoATR,
      stPoAMT, stPoAMTL, stPoAMTR, stPitHole,
      stPoAMB, stPoAMBL, stPoAMBR,
      stPoAB, stPoABL, stPoABR, stEBHole,

      stCenter, stSubway, stIdolPopper, stEnterIdol,
      stIdol1, stIdol2, stIdol3, stIdol4, stIdol5, stIdol6,
      stIdolExit,

      stEDropDown, stNDropDown, stTDropDown,

      stTotem, stTotemDropDown, stCapturedIdle, stCapturedF, stCapturedB,

      stStartMode, stCStandup, stAdventure, staDventure, stadVenture, stadventUre, stadventuRe, stadventurE, stIndy, stiNdy, stinDy, stindY
     };

static sim_tState ij_stateDef[] = {
 /* Display         swTime switch	sol         	next        	timer alt flags*/

/* general stuff, free state, ball troughs, shooting and draining */
  {"Not Installed",	 0,0,		0,		stDrain,	0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		 0,0,		0,		0,          	0,0,0,SIM_STNOTEXCL},
  {"Trough 1",           1,swTrough1, 	sBallRelease,   stTTrough,  	1},
  {"Trough 2",           1,swTrough2, 	0,          	stTrough1,  	5},
  {"Trough 3",           1,swTrough3, 	0,          	stTrough2,  	5},
  {"Trough 4",           1,swTrough4, 	0,          	stTrough3,  	5},
  {"Trough 5",           1,swTrough5, 	0,          	stTrough4,  	5},
  {"Trough 6",           1,swTrough6, 	0,          	stTrough5,  	5},
  {"Trough Top",         1,swTTrough, 	0,	  	stShooter,  	1},
  {"Drain",              0,0,         	0,          	0,  		0},
  {"Draining",           0,0,         	0,          	stTrough6,  	2,0,0, SIM_STNOTEXCL},
  {"Shooter",            1,swShooter, 	sBallLaunch, 	stBallLane, 	0,0,0, SIM_STNOTEXCL},
  {"Ball Lane",          1,0,         	0,          	stRTLoop,   	5},

/* Inlanes, Slingshots, Jets */
  {"Right Inlane",	1,swRIn,	 0,		stFree,		5},
  {"Left Inlane",	1,swLIn,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBJet,	 0,		stFree,		1},

/* loops */
  {"Bottom Left Loop",   1,swLBLoop,    0,              stLTLoop,       2,0,0, SIM_STNOTEXCL},
  {"Top Left Loop",      1,swLTLoop,  	0,          	stFree,     	2,sRControlGate,stRTLoopD, SIM_STNOTEXCL},
  {"Top Left Loop",      1,swLTLoop,  	0,          	stLBLoopD,     	2,0,0, SIM_STNOTEXCL},
  {"Bottom Left Loop",   1,swLBLoop,    0,              stFree,         2,0,0, SIM_STNOTEXCL},
  {"Bottom Right Loop",  1,swRBLoop,    0,              stRTLoop,       2,0,0, SIM_STNOTEXCL},
  {"Top Right Loop",     1,swRTLoop,  	0,          	stFree,     	2,sLControlGate,stLTLoopD, SIM_STNOTEXCL},
  {"Top Right Loop",     1,swRTLoop,  	0,          	stRBLoopD,     	2,0,0, SIM_STNOTEXCL},
  {"Bottom Right Loop",  1,swRBLoop,    0,              stFree,         2,0,0, SIM_STNOTEXCL},

/* ramps */
  {"L. Ramp Enter",	 1,swLRampEnter,0,		stLRamp,    	5},
  {"Left Ramp",    	 1,swLRampMade, 0,		stLIn,  	8},
  {"R. Ramp Enter",   	 1,swRRampEnter,0,		stRRamp,    	2,sDivHold,stRopeBridge},
  {"Right Ramp",    	 1,swRRampMade, 0,		stFree,    	5},
  {"Rope Bridge",    	 1,0,		0,		stTopPost,      2,sTopPostHold,stBypassPoA, SIM_STNOTEXCL},
  {"Bypass PoA",    	 1,swTPost,	0,		stFree,         6,0,0, SIM_STNOTEXCL},

/* outlanes */
  {"Left Outlane",    	 1,swLOut, 	0,		stDrain,    	5,0,0, SIM_STNOTEXCL},
  {"Right Outlane",    	 1,swRTOut, 	0,		stROut2,    	5,0,0, SIM_STNOTEXCL},
  {"Right Outlane",    	 1,swRBOut, 	0,		stDrain,    	5,0,0, SIM_STNOTEXCL},
  {"Right Outlane",	 1,swRTOut,	0,		stFree,		15},

/* Path of Adventure (PoA, a mini playfield) */
  {"Top Post",    	 1,swTPost,     sTopPostPower,	stPoAT,    	5,0,0, SIM_STNOTEXCL},
  {"PoA Top", 	         0,0,     	0,		0,    		0,0,0, SIM_STNOTEXCL},
  {"PoA Top L",          1,swTL_PoA,    0,		stPoAMT,        1,0,0, SIM_STNOTEXCL},
  {"PoA Top R",          1,swTR_PoA,    0,		stPoAMT,        1,0,0, SIM_STNOTEXCL},
  {"PoA T Middle",       0,0,     	0,		0,    		0,0,0, SIM_STNOTEXCL},
  {"PoA T Middle L",     1,swMTL_PoA, 	0,		stPoAMB,        0,0,0, SIM_STNOTEXCL},
  {"PoA T Middle R",     1,swMTR_PoA,   0,		stPoAMB,        0,0,0, SIM_STNOTEXCL},
  {"PoA Pithole",        1,swPitHole,   0,		stFree,         5,0,0, SIM_STNOTEXCL},
  {"PoA B Middle",       0,0,     	0,		0,    		0,0,0, SIM_STNOTEXCL},
  {"PoA B Middle L",     1,swMBL_PoA, 	0,		stPoAB,         0,0,0, SIM_STNOTEXCL},
  {"PoA B Middle R",     1,swMBR_PoA,   0,		stPoAB,         0,0,0, SIM_STNOTEXCL},
  {"PoA Bottom",         0,0,     	0,		0,    		0,0,0, SIM_STNOTEXCL},
  {"PoA Bottom L",       1,swBL_PoA, 	0,		stFree,         3,0,0, SIM_STNOTEXCL},
  {"PoA Bottom R",       1,swBR_PoA,    0,		stFree,         3,0,0, SIM_STNOTEXCL},
  {"PoA EB Hole",        1,swEBHole,    0,		stFree,         5,0,0, SIM_STNOTEXCL},

/* center lock and idol */
  {"Center Lock",    	 1,swCEnter,   	0,				stSubway, 5},
  {"Subway Lookup",    	 1,swSLookup,  	sSubwayRelease,	stIdolPopper,   5},
  {"Idol Popper",    	 1,swRPopper,  	sBallPopper,	stEnterIdol,    2},
  {"Entering Idol",    	 1,swIdolEnter,	0,		0,	    	0},
  {"Idol Pos 1",    	 0,0,		0,		0,	    	0},
  {"Idol Pos 2",    	 0,0,		0,		0,	    	0},
  {"Idol Pos 3",    	 0,0,		0,		0,	    	0},
  {"Idol Pos 4",    	 0,0,		0,		0,	    	0},
  {"Idol Pos 5",    	 0,0,		0,		0,	    	0},
  {"Idol Pos 6",    	 0,0,		0,		0,	    	0},
  {"Exit Idol",    	 2,swExitIdol,	0,		stFree,	    	5},

/* center drop down targets */
  {"Adventure - E", 	 1,swCLDrop,    0,		stFree,		2,0,0, SIM_STSWKEEP},
  {"Adventure - N", 	 1,swCMDrop,    0,		stFree,		2,0,0, SIM_STSWKEEP},
  {"Adventure - T", 	 1,swCRDrop,    0,		stFree,		2,0,0, SIM_STSWKEEP},

/* totem drop down and captured ball */
  {"Totem", 	 	 0,0,    	0,		0,		0},
  {"Totem Drop Down", 	 1,swSDropTop,  0,		stFree,		1,0,0, SIM_STSWKEEP},
  {"Captured Ball",      0,0,           0,		0,    		0},
  {"Capt. Ball F",       1,swFCapBall,  0,		stCapturedB,    1},
  {"Capt. Ball B",       1,swBCapBall,  0,		stCapturedIdle, 1},

/* all the rest */
  {"Start Mode",    	 1,swLEject,    sLEject,	stFree,    	5},
  {"Center Target",	 1,swCStandup,	0,		stFree,		3},
  {"Adventure - A",	 1,swAdventure,	0,		stFree,		2},
  {"Adventure - D",	 1,swaDventure,	0,		stFree,		2},
  {"Adventure - V",	 1,swadVenture,	0,		stFree,		2},
  {"Adventure - U",	 1,swadventUre,	0,		stFree,		2},
  {"Adventure - R",	 1,swadventuRe,	0,		stFree,		2},
  {"Adventure - E",	 1,swadventurE,	0,		stFree,		2},
  {"Indy - I",		 1,swIndy,	0,		stFree,		2},
  {"Indy - N",		 1,swiNdy,	0,		stFree,		2},
  {"Indy - D",		 1,swinDy,	0,		stFree,		2},
  {"Indy - Y",		 1,swindY,	0,		stFree,		2},

};

static int ij_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch(ball->state) {
    case stEnterIdol:
      /* optimized, look and this:
          i = locals.idolPos/60;
          if ((i==0) || (i==2) || (i==4))
      */
      if ( !(locals.idolPos/60%2) )
        return setState(stIdol1, 1);

    case stIdol4:
      if ( core_getSol(sIdolRelease) )
      	return setState(stIdolExit,2);
      /* fall through */

    case stIdol1:
    case stIdol2:
    case stIdol3:
    case stIdol5:
    case stIdol6:
      /* look to the future (for the idol pos if motor is running) */
      if ( core_getSol(sWheelMotor) && ((locals.idolPos/60)!=((locals.idolPos+IJ_IDOLTICK)/60)) )
        return setState(stIdol1 + (ball->state+1-stIdol1)%6, 0);
      break;

    case stTotem:
      if (core_getSw(swSDropTop)) {
        locals.hitCaptiveBall = 1;
      	return setState(stFree,0);
      }
      else
        return setState(stTotemDropDown,0);

    case stCapturedIdle:
      if (locals.hitCaptiveBall) {
        locals.hitCaptiveBall = 0;
        return setState(stCapturedF,0);
      }
      return 0;

    case stDrain:
      /* don't let drain the captured ball */
      if ( ball->type )
        return setState(stCapturedIdle,0);
      else
        return setState(stDraining,0);
  }

  return 0;
}

static void ij_initSim(sim_tBallStatus *balls, int *inports, int noOfBalls)
{
    initBallType(0, IJ_CAPTUREDBALL);

    locals.idolPos = 0;
    locals.PoAPos = IJ_POA_MIDPOS;
    locals.hitCaptiveBall = 0;

    /* set switch for idol pos 1*/
    core_setSw(swIdolPos1, 1);
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData ij_inportData[] = {
  {0, 0x0005, stLRampEnter},
  {0, 0x0006, stRRampEnter},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0010, stRTOut},
  {0, 0x0021, stLBLoop},
  {0, 0x0022, stRBLoop},
  {0, 0x0041, stLSling},
  {0, 0x0042, stRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0101, stPoATL, stPoAT},
  {0, 0x0102, stPoATR, stPoAT},
  {0, 0x0201, stPoAMTL, stPoAMT},
  {0, 0x0202, stPoAMTR, stPoAMT},
  {0, 0x0401, stPoAMBL, stPoAMB},
  {0, 0x0402, stPoAMBR, stPoAMB},
  {0, 0x0801, stPoABL, stPoAB},
  {0, 0x0802, stPoABR, stPoAB},
  {0, 0x1000, stTotem},
  {0, 0x2000, stCenter},
  {0, 0x4000, stCStandup},
  {0, 0x8000, stStartMode},

  {1, 0x0001, stLJet},
  {1, 0x0002, stRJet},
  {1, 0x0004, stBJet},
  {1, 0x0008, stIndy},
  {1, 0x0010, stiNdy},
  {1, 0x0020, stinDy},
  {1, 0x0040, stindY},
  {1, 0x0080, stAdventure},
  {1, 0x0100, staDventure},
  {1, 0x0200, stadVenture},
  {1, 0x0400, stEDropDown},
  {1, 0x0800, stNDropDown},
  {1, 0x1000, stTDropDown},
  {1, 0x2000, stadventUre},
  {1, 0x4000, stadventuRe},
  {1, 0x8000, stadventurE},

  {2, 0x1000, stPitHole, stPoAMT},
  {2, 0x2000, stEBHole, stPoAB},
  {2, 0x4000, swBuyIn, SIM_STANY},
  {2, 0x8000, stDrain},
  {0}
};

/*--------------------
/ Drawing information
/---------------------*/
static core_tLampDisplay ij_lampPos = {
{ 0, 0 }, /* top left */
{43, 26}, /* size */
{
/* 11 */  {1,{{20, 6,YELLOW}}}, /* 12 */  {1,{{21, 7,WHITE}}},  /* 13 */  {1,{{22, 8,ORANGE}}}, /* 14 */  {1,{{25, 5,ORANGE}}},
/* 15 */  {1,{{27, 5,ORANGE}}}, /* 16 */  {1,{{29, 5,ORANGE}}}, /* 17 */  {1,{{31, 6,RED   }}}, /* 18 */  {1,{{32, 7,RED   }}},
/* 21 */  {1,{{24,10,RED   }}}, /* 22 */  {1,{{21,13,ORANGE}}}, /* 23 */  {1,{{19,11,ORANGE}}}, /* 24 */  {1,{{20,12,ORANGE}}},
/* 25 */  {1,{{27,10,RED   }}}, /* 26 */  {1,{{25,12,GREEN }}}, /* 27 */  {1,{{31,10,RED   }}}, /* 28 */  {1,{{29,12,RED   }}},
/* 31 */  {1,{{21, 9,YELLOW}}}, /* 32 */  {1,{{25, 7,RED   }}}, /* 33 */  {1,{{27, 7,GREEN }}}, /* 34 */  {1,{{28, 7,RED   }}},
/* 35 */  {1,{{31, 7,GREEN }}}, /* 36 */  {1,{{35,12,GREEN }}}, /* 37 */  {1,{{35,10,YELLOW}}}, /* 38 */  {1,{{35, 7,GREEN }}},
/* 41 */  {1,{{27,14,RED   }}}, /* 42 */  {1,{{33,12,RED   }}}, /* 43 */  {1,{{32,17,RED   }}}, /* 44 */  {1,{{31,17,GREEN }}},
/* 45 */  {1,{{35,14,YELLOW}}}, /* 46 */  {1,{{35,17,GREEN }}}, /* 47 */  {1,{{31,14,RED   }}}, /* 48 */  {1,{{21, 4,YELLOW}}},
/* 51 */  {1,{{25,17,RED   }}}, /* 52 */  {1,{{27,17,GREEN }}}, /* 53 */  {1,{{28,17,RED   }}}, /* 54 */  {1,{{27,20,ORANGE}}},
/* 55 */  {1,{{29,20,ORANGE}}}, /* 56 */  {1,{{31,20,ORANGE}}}, /* 57 */  {1,{{24,14,RED   }}}, /* 58 */  {1,{{22,21,YELLOW}}},
/* 61 */  {1,{{ 4,10,ORANGE}}}, /* 62 */  {1,{{ 3,12,ORANGE}}}, /* 63 */  {1,{{ 2,14,ORANGE}}}, /* 64 */  {1,{{ 1,16,ORANGE}}},
/* 65 */  {1,{{37, 9,GREEN }}}, /* 66 */  {1,{{39, 8,YELLOW}}}, /* 67 */  {1,{{41, 8,GREEN }}}, /* 68 */  {1,{{21,19,YELLOW}}},
/* 71 */  {1,{{ 3, 4,YELLOW}}}, /* 72 */  {1,{{ 3, 6,YELLOW}}}, /* 73 */  {1,{{ 7, 4,YELLOW}}}, /* 74 */  {1,{{ 7, 6,YELLOW}}},
/* 75 */  {1,{{ 8, 5,GREEN }}}, /* 76 */  {1,{{37,15,GREEN }}}, /* 77 */  {1,{{39,16,YELLOW}}}, /* 78 */  {1,{{41,16,GREEN }}},
/* 81 */  {1,{{11, 4,YELLOW}}}, /* 82 */  {1,{{11, 6,YELLOW}}}, /* 83 */  {1,{{14, 4,YELLOW}}}, /* 84 */  {1,{{14, 6,YELLOW}}},
/* 85 */  {1,{{15, 5,GREEN }}}, /* 86 */  {1,{{ 9,19,YELLOW}}}, /* 87 */  {1,{{18,14,GREEN }}}, /* 88 */  {1,{{43, 1,YELLOW}}}
}
};

static wpc_tSamSolMap ij_SamSolMap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER},
 {sBallRelease,0,SAM_BALLREL},
 {sBallLaunch,0,SAM_SOLENOID},

 /*Channel #1*/
 {sDivPower,1,SAM_SOLENOID_ON}, {sDivHold,1,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sLSling,1,SAM_LSLING}, {sRSling,1,SAM_RSLING},
 {sBallPopper, 1, SAM_POPPER},
 {sLJet,1,SAM_JET1},

 /*Channel #2*/
 {sTotemDropDown,2,SAM_SOLENOID}, {sTotemDropUp,2,SAM_SOLENOID},
 {sL_PoA,2,SAM_MOTOR_1, WPCSAM_F_CONT}, {sR_PoA,2,SAM_MOTOR_2, WPCSAM_F_CONT},
 {sSubwayRelease,2,SAM_SOLENOID},
 {sRJet,2,SAM_JET2},

 /*Channel #3*/
 {sWheelMotor, 3, SAM_MOTOR_2, WPCSAM_F_CONT},
 {sLEject, 3, SAM_SOLENOID},
 {sBJet,3,SAM_JET3},

 /*Channel #4*/
 {sTopPostPower,4,SAM_SOLENOID_ON}, {sTopPostHold,4,SAM_SOLENOID_OFF, WPCSAM_F_ONOFF},
 {sCenterDropUp,4,SAM_SOLENOID},

 {sIdolRelease, 4, SAM_SOLENOID_ON}, {sIdolRelease, 4, SAM_SOLENOID_OFF, WPCSAM_F_ONOFF},{-1}
};

static void ij_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Idol Position: %3d",locals.idolPos);
  core_textOutf(30,10,BLACK,"PoA Degrees  : %3d",locals.PoAPos);
  core_textOutf(30,20,BLACK,"PoA Post     : %-4s", core_getSol(sTopPostHold) ? "Down":"Up");
  core_textOutf(30,30,BLACK,"Totem Drop T.: %-4s", core_getSw(swSDropTop) ? "Down":"Up");
}
/* Help */
static void ij_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Sling");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+=/[ = L/R Top/TopMid PoA Sw");
  core_textOutf(30,100,BLACK,"L/R Ctrl+//' = L/R Bot/BotMid PoA Sw");
  core_textOutf(30,110,BLACK,"I/N = PoA Pit/EB Hole, B = Totem");
  core_textOutf(30,120,BLACK,"M = Mode Start, C = Center Hole");
  core_textOutf(30,130,BLACK,"W/E/S = Right/Left/Bottom Jet Bumpers");
  core_textOutf(30,140,BLACK,"G/H/J/K = Indy Rollovers, 2 = Buy-In");
  core_textOutf(30,150,BLACK,"NUMPAD 1-9 = Adventure Targets");
  core_textOutf(30,160,BLACK,"NUMPAD 0 = Center Target, Q = Drain");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(ij,l7,"ijone_l7.rom",0x80000,CRC(4658c877) SHA1(b47ab064ff954bd182919f714ed8930cf0bed896))
DCS_SOUNDROM7x("ijsnd_l3.u2",CRC(fbd91a0d) SHA1(8d9a74f04f6088f18dfbb578893410abc21a0e42),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

WPC_ROMSTART(ij,lg7,"u6-lg7.rom",0x80000,CRC(c168a9f7) SHA1(732cc0863da06bce3d9793d57d67ba03c4c2f4d7))
DCS_SOUNDROM7x("ijsnd_l3.u2",CRC(fbd91a0d) SHA1(8d9a74f04f6088f18dfbb578893410abc21a0e42),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

WPC_ROMSTART(ij,l6,"ijone_l6.rom",0x80000,CRC(8c44b880) SHA1(9bc2cd91ea4d98e6509d6c1e2e34622e83c5a4d7))
DCS_SOUNDROM7x("ijsnd_l3.u2",CRC(fbd91a0d) SHA1(8d9a74f04f6088f18dfbb578893410abc21a0e42),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

WPC_ROMSTART(ij,l5,"ijone_l5.rom",0x80000,CRC(bf46ff92) SHA1(1afb1aadf115ae7d7f54bfea1fcca71a9de6ebb0))
DCS_SOUNDROM7x("ijsnd_l2.u2",CRC(508d27c5) SHA1(da9787905c6f11d16e9a62047f15c5780017b551),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

WPC_ROMSTART(ij,l4,"ij_l4.u6",0x80000,CRC(5f2c3130) SHA1(b748932a1c0ac622e00744314fafef857f59026d))
DCS_SOUNDROM7x("ijsnd_l2.u2",CRC(508d27c5) SHA1(da9787905c6f11d16e9a62047f15c5780017b551),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

WPC_ROMSTART(ij,l3,"ijone_l3.rom",0x80000,CRC(0555c593) SHA1(1a73946fff9ae40e5499fcfa2d9f8330a25b8bae))
DCS_SOUNDROM7x("ijsnd_l1.u2",CRC(89061ade) SHA1(0bd5ec961c780c4d46296aee7f2cb63b72e990f5),
               "ijsnd_l3.u3",CRC(3f12a996) SHA1(5f5d2853e671d13fafdb2972f52a823e18f27643),
               "ijsnd_l3.u4",CRC(05a92937) SHA1(e4e53e2899a7e7cbcd6ce7e3331bb8aa13321aa6),
               "ijsnd_l3.u5",CRC(e6fe417c) SHA1(d990ed218fe296ad9a015d77519b8d954d252035),
               "ijsnd_l3.u6",CRC(975f3e48) SHA1(16c56500b18e551bcd2e0c7e4c55ddab4791ac84),
               "ijsnd_l3.u7",CRC(2d9cd098) SHA1(8d26c84cbd4ab2a5c9f4be3ea95a79fd125248e3),
               "ijsnd_l3.u8",CRC(45e35bd7) SHA1(782b406be341d55d22a96acb8c2459f3058940df))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(ij,l7,"Indiana Jones (L-7)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(ij,lg7,l7,"Indiana Jones (LG-7)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(ij,l6,l7,"Indiana Jones (L-6)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(ij,l5,l7,"Indiana Jones (L-5)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(ij,l4,l7,"Indiana Jones (L-4)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(ij,l3,l7,"Indiana Jones (L-3)",1993,"Williams",wpc_mDCSS,0)

/*----------
/ Game Data
/-----------*/
static sim_tSimData ijSimData = {
  3,        /* 3 game specific input ports */
  ij_stateDef,
  ij_inportData,
  { stCapturedIdle, stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6 },
  ij_initSim,
  ij_handleBallState,
  ij_drawStatic,
  FALSE, /* simulate manual shooter */
  NULL,  /* no custom conditions */
};

static core_tGameData ijGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    1,0,7,0,0,0,0,
    ij_getSol, ij_handleMech, ij_getMech, ij_drawMech,
    &ij_lampPos, ij_SamSolMap
  },
  &ijSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x06, 0x7F, 0x00, 0x00, 0x70, 0x18 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swGunTrigger },
  }
};


/*---------------
/  Game handling
/----------------*/
/*-- patched memory read function to handle the 9th switch column --*/
READ_HANDLER(ij_ijRowRead) {
  if ((wpc_data[WPC_EXTBOARD1] & 0x80) > 0) /* 9th column enabled */
    return coreGlobals.swMatrix[CORE_CUSTSWCOL];
  return wpc_r(WPC_SWROWREAD);
}

static void init_ij(void) {
  core_gameData = &ijGameData;
  install_mem_read_handler(WPC_CPUNO, WPC_SWROWREAD+WPC_BASE, WPC_SWROWREAD+WPC_BASE,
                           ij_ijRowRead);
}

static int ij_getSol(int solNo) {
  if ((solNo >= CORE_CUSTSOLNO(1)) && (solNo <= CORE_CUSTSOLNO(8)))
    return ((wpc_data[WPC_EXTBOARD1]>>(solNo-CORE_CUSTSOLNO(1)))&0x01);
  return 0;
}

static void ij_handleMech(int mech) {
  /*---------------------------------
  /  handle Adv(ent)ure Drop Targets
  /----------------------------------*/
  if ((mech & 0x01) && core_getSol(sCenterDropUp))
    { core_setSw(swCLDrop, 0); core_setSw(swCMDrop, 0); core_setSw(swCRDrop, 0); }

  /*--------------------------
  /  handle Totem Drop Target
  / --------------------------*/
  if (mech & 0x02) {
    if (core_getSol(sTotemDropUp))
      core_setSw(swSDropTop, 0);
    if (core_getSol(sTotemDropDown))
      core_setSw(swSDropTop, 1);
  }

  /*-------------
  /   handle PoA
  /--------------*/
  if (mech & 0x04) {
    if ((locals.PoAPos > 0) && core_getPulsedSol(sL_PoA))   locals.PoAPos--;
    if ((locals.PoAPos < 359) && core_getPulsedSol(sR_PoA)) locals.PoAPos++;
    core_setSw(swLL_PoA, !(locals.PoAPos>IJ_POA_LLIMIT));
    core_setSw(swRL_PoA, !(locals.PoAPos<IJ_POA_RLIMIT));
  }

  /*-------------
  /  handle idol
  /--------------*/
  if (mech & 0x08) {
    if (core_getSol(sWheelMotor)) locals.idolPos = (locals.idolPos+IJ_IDOLTICK)%360;
    switch (locals.idolPos/60) {
      case 0: core_setSw(swIdolPos2, FALSE);  break;
      case 1: core_setSw(swIdolPos3, TRUE);   break;
      case 2: core_setSw(swIdolPos1, FALSE);  break;
      case 3: core_setSw(swIdolPos2, TRUE);   break;
      case 4: core_setSw(swIdolPos3, FALSE);  break;
      case 5: core_setSw(swIdolPos1, TRUE);   break;
    }
  }
}
static int ij_getMech(int mechNo) {
  if (mechNo == 2) return locals.PoAPos;
  return locals.idolPos;
}
