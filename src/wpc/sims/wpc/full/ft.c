/********************************************************************************************
 Williams Fishtales (1992) Pinball Simulator

 by Steve Ellenoff (sellenoff@hotmail.com)
 Nov.19, 2000

 Known Issues/Problems with Simulator:
  #1) I don't have access to this game, I guessed most of it from a Playfield picture
      and the rulesheet! I did love to play it though, but it's been years since I have!
  #2) I'm guessing on the reel optos, with help from "ari 'apz' sovijärvi"

Changes:
 --------
  05-1-2001: Added GetMech support & Added all 3 fake solenoids to gamedata structure

 *********************************************************************************************************/

/*-------------------------------------------------------------------------------------------------
  Keys for Fish Tales Simulator:
  -----------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +L  L/R Loop
    +O  L/R Outlane
     Q  Drain
   ZXC  LIE Rollovers (1-3)
    NM  RBank Targets (1-2)
    VB  LBank Targets (1-2)
   HJK  Jet Bumpers (1-3)
     E  Extra Ball
     W  Captive Ball
     T  Caster's Club
-------------------------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
/*-- local state/ball handling functions --*/
static int  ft_handleBallState(sim_tBallStatus *ball, int *inports);
static void ft_handleMech(int mech);
static void ft_drawMech(BMTYPE **line);
static void ft_drawStatic(BMTYPE **line);
static void init_ft(void);
static int  ft_getSol(int solNo);
static const char * showReelPos(void);
static int ft_getMech(int mechNo);

/*-----------------------
  local static variables
 ------------------------
   I used this locals structure for setting up variables to track the state of mechanical objects
   which needed to be displayed on screen
--------------------------------------------------------------------------------------------------*/
static struct {
  int dtPos;    	/* Drop Target Position */
  int gatePos;		/* Gate Position */
  int reelPos;		/* Reel Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------
   Here we define for PINMAME which Keyboard Presses it should look for, and process!
--------------------------------------------------------------------------------------------------*/

WPC_INPUT_PORTS_START(ft,3)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  	KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", 	KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        	KEYCODE_R)
    COREPORT_BIT(   0x0008,"L/R Inlane",		KEYCODE_I)
    COREPORT_BITIMP(0x0010,"L/R Outlane",     	KEYCODE_O)
    COREPORT_BITIMP(0x0040,"L/R Loop",       	KEYCODE_L)
    COREPORT_BIT(   0x0080,"L/R Slingshot",   	KEYCODE_MINUS)
    COREPORT_BIT(   0x0100,"Jet Bumper 1", 	KEYCODE_H)
    COREPORT_BITIMP(0x0200,"Jet Bumper 2",    	KEYCODE_J)
    COREPORT_BITIMP(0x0400,"Jet Bumper 3",    	KEYCODE_K)
    COREPORT_BITIMP(0x0800,"LIE 'L'",    	KEYCODE_Z)
    COREPORT_BITIMP(0x1000,"LIE 'I'",     	KEYCODE_X)
    COREPORT_BIT(   0x2000,"LIE 'E'",	     	KEYCODE_C)
    COREPORT_BIT(   0x4000,"R. Bank DT 1",	KEYCODE_N)
    COREPORT_BITIMP(0x8000,"R. Bank DT 2",	KEYCODE_M)

  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"L. Bank DT 1",       KEYCODE_V)
    COREPORT_BIT(   0x0002,"L. Bank DT 2",	KEYCODE_B)
    COREPORT_BIT(   0x0004,"Drain",		KEYCODE_Q)
    COREPORT_BIT(   0x0008,"Caster's Club",	KEYCODE_T)
    COREPORT_BIT(   0x0010,"Extra Ball",		KEYCODE_E)
    COREPORT_BIT(   0x0040,"Captive Ball",	KEYCODE_W)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      		13
#define swTilt       		14
#define swOuthole    		15
#define swLTrough    		16
#define swCTrough    		17
#define swRTrough   		18

#define swSlamTilt   		21
#define swCoinDoor   		22
#define swTicket     		23
#define swNotUsed		24
#define swLOut			25
#define swLIn			26
#define swLDT1			27
#define swLDT2			28

#define swCast			31
#define swLBoatExit		32
#define swRBoatExit		33
#define swSpinner		34
#define swReelEntry		35
#define swCatapult		36
#define swReel1Opto		37
#define swReel2Opto		38

#define swCaptiveBall		41
#define swRBoatEntry		42
#define swLBoatEntry		43
#define swLIE_E		     	44
#define swLIE_I		     	45
#define swLIE_L		     	46
#define swBallPopper		47
#define swDropTarget		48

#define swJet1			51
#define swJet2			52
#define swJet3			53
#define swRDT1			54
#define	swRDT2			55
#define	swBallShooter		56
#define	swLSling		57
#define	swRSling		58

#define	swExtraBall		61
#define	swTRLoop		62
#define	swTopEject		63
#define	swTLLoop		64
#define	swRIn			65
#define	swROut			66
/*67 Not Used*/
/*68 Not Used*/

/*71-88 NOT USED*/

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sBallShooter		1
#define sCatapult		2
#define sBallPopper	      	3
#define sLSling			4
#define sRSling      		5
#define sLGate      		6
#define sKnocker       		7
#define sBackBoxFish   		8
#define sOuthole		9
#define sBallRelease		10
#define sEjectHole		11
#define sDropTargetUp		12
#define sDropTargetDown	   	13
#define sJet1          		14
#define sJet2          		15
#define sJet3			16
#define sReelMotor	   	28

/*Define fake solenoids used to move the ball from the reel to the ball catapult!*/
/*We use 1 solenoid for each ball*/
#define sFakeReel1		CORE_CUSTSOLNO(1) /* 33 */
#define sFakeReel2		CORE_CUSTSOLNO(2) /* 34 */
#define sFakeReel3		CORE_CUSTSOLNO(3) /* 35 */

#define UP			0
#define DOWN			1
#define OPEN 			0
#define CLOSED			1

#define ERROR_RANGE		10
#define BALL1UP			0
#define BALL1DOWN		75
#define BALL2UP			50
#define BALL2DOWN		125
#define BALL3UP			100
#define BALL3DOWN		25

/*---------------------
/  Ball state handling
/----------------------*/



/* ---------------------------------------------------------------------------------
   The following variables are used to refer to the state array!
   These vars *must* be in the *exact* *same* *order* as each stateDef array entry!
   -----------------------------------------------------------------------------------------*/
enum {	stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOuthole, stDrain,
      	stShooter, stBallLane, stROut, stLOut, stRIn, stLIn, stLSling, stRSling, stJetB1, stJetB2, stJetB3,
      	stLLoopUp, stLLoopDn, stSpinnerRL, stSpinnerLR,
	stEjectHole, stGateRL, stGateLR,
	stRLoopUp, stRLoopDn, stLRampEnt, stLRampMade, stRRampEnt, stRRampMade,
	stDropTarget, stCaster, stCatapult, stCatapult2, stHabitTrail, stReelEntry,
	stJet1, stJet2, stJet3, stLIE_L,
	stReel, stReel1, stReel2, stReel3,
	stLie, stlIe, stliE, stRDT1, stRDT2, stLDT1, stLDT2, stExtraBall, stCaptiveBall
      };

/********************************************************************************************************
   The following is a list of all possible game states.....
   We start defining our own states at element #3, since 'Not Installed', 'Moving', and 'Playfield'
   are always defined the same!!

   Any state with all zeros, like the following example must be handled in the Handle Ball State function!
   {"State Name",    1,0,    0,       0, 0},

   Fields for each array element
   -----------------------------
   #1) Name to display on screen
   #2) Switch down time (minimum)
   #3) Switch to be triggered while in this state
   #4) Solenoid used to get out of state (0=leave ASAP) also: Balls can not enter state while solenoid is active
   #5) State following this one
   #6) Time between this state and next
   #7) If this solenoid is active go to altstate (below)
   #8) alternative state
   #9) State flags
	STSWKEEP:        Leave switch on (e.g. drop targets)
	STSHOOT:         Set ballspeed to plunger speed
	STIGNORESOL:     Always call statehandler regardless of solenoid state
	STNOTEXCL:       More than one ball can be in this state
	STNOTBALL(type): Specified ball type does not affect switch
	STSPINNER        Switch is a spinner
*******************************************************************************************************/


static sim_tState ft_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",        0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",     1,swRTrough,   sBallRelease,  stShooter,  5},
  {"Center Trough",    1,swCTrough,   0,        stRTrough,   1},
  {"Left Trough",      1,swLTrough,   0,        stCTrough,   1},
  {"Outhole",          1,swOuthole,   sOuthole, stLTrough,   5},
  {"Drain",            1,0,           0,        stOuthole,   0,0,0,SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swBallShooter,   sBallShooter, stBallLane,  1,0,0,SIM_STNOTEXCL},
  {"Ball Lane",		1, 0,  	       0,        stRLoopUp,    10,0,0,SIM_STNOTEXCL},	/*Normally ball should enter Right Loop*/
  {"Right Outlane",	1,swROut,      0,        stDrain,    20},
  {"Left Outlane",	1,swLOut,      0,        stDrain,    20},
  {"Right Inlane",	1,swRIn,       0,        stFree,      5},
  {"Left Inlane",	1,swLIn,       0,        stFree,      5},
  {"Left Slingshot",	1,swLSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRSling,	 0,		stFree,		1},
  {"Jet Bumper 1",	1,swJet1,	 0,		stFree,		1},
  {"Jet Bumper 2",	1,swJet2,	 0,		stFree,		1},
  {"Jet Bumper 3",	1,swJet3,	 0,		stFree,		1},

  /*Line 3*/
  {"Left Loop",		1,swTLLoop,      0,       stSpinnerLR,   2}, /* Left Loop Going Up     */
  {"Left Loop",		1,swTLLoop,      0,       stFree,        3}, /* Left Loop Going Down   */

  /*Spinner-> Ball moving from Right->Left: Do NOT Trigger Spinner - simply go to Left Loop Down!*/
  {"Spinner",           1,0,             0,       stLLoopDn,     5},
  /*Spinner-> Ball moving from Left->Right: Trigger Spinner - Then to GateLR! */
  {"Spinner",           1,swSpinner,     0,       stGateLR,      5,0,0,SIM_STSPINNER},

  /*Line 4*/
  {"Eject Hole",	1,swTopEject,    sEjectHole,       stLIE_L,    10},
  {"Gate",		1,0,		 0,	  0},
  {"Gate",		1,0,		 0,	  0},

  /*Line 5*/
  {"Right Loop",       1,swTRLoop, 	 0,       stGateRL,   1}, /* Right Loop Going Up     */
  {"Right Loop",       1,swTRLoop, 	 0,       stFree,   10},  /* Right Loop Coming Down  */

  /*Line 5*/
  {"L. Ramp Enter",	1,swLBoatEntry,  0,       stLRampMade,	 5},
  {"L. Ramp Exit",	1,swLBoatExit,   0,	  stLIn, 	 10},
  {"R. Ramp Enter",	1,swRBoatEntry,  0,       stRRampMade,	 5},
  {"R. Ramp Exit",	1,swRBoatExit,   0,	  stRIn, 	 10},

  /*Line 6*/
  {"Drop Target",	1,		  0,  	  0, 	 0},
  {"Caster's Club",     1, swBallPopper,  sBallPopper,   stHabitTrail, 0},
  {"Catapult",		1, swCatapult,    sCatapult,     stCatapult2,  1},
  {"Catapult Trail",	1, 0,		  0,      	 stFree,       10}, //,0,0,SIM_STNOTEXCL},
  {"Habit Trail",       1, 0,             0,      stReelEntry,    1},
  {"Reel Entry",	1, swReelEntry,   0,	  stReel,	  1},

  /*Line 7*/
  {"Jet Bumper 1",	1, swJet1,	 0,	  stJet2,	  1},
  {"Jet Bumper 2",	1, swJet2,	 0,	  stJet3,	  1},
  {"Jet Bumper 3",	1, swJet3,	 0,	  stFree,	  1},
  {"LIE 'L'",		1, swLIE_L,	 0,	  stJet1,	  1},

  /*Line 8*/
  {"Reel",		1, 0,		0,	0,		1},
  {"Reel Lock 1", 	1, 0,		sFakeReel1,	stCatapult,	1},
  {"Reel Lock 2", 	1, 0,		sFakeReel2,	stCatapult,	1},
  {"Reel Lock 3", 	1, 0,		sFakeReel3,	stCatapult,	1},

  /*Line 9*/
  {"Lie - L",		1,swLIE_L,	0,	  stFree,		2},
  {"Lie - I",		1,swLIE_I,	0,	  stFree,		2},
  {"Lie - E",		1,swLIE_E,	0,	  stFree,		2},
  {"Rt. Fish Tgt 1",	1,swRDT1,	0,	  stFree,		2},
  {"Rt. Fish Tgt 2",	1,swRDT2,	0,	  stFree,		2},
  {"Lt. Fish Tgt 1",	1,swLDT1,	0,	  stFree,		2},
  {"Lt. Fish Tgt 2",	1,swLDT2,	0,	  stFree,		2},
  {"Extra Ball Tgt",	1,swExtraBall,  0,	  stFree,		2},
  {"Captive Ball",	1,swCaptiveBall,0,	  stFree,		4},

  {0}
};

static int ft_handleBallState(sim_tBallStatus *ball, int *inports) {
	  switch (ball->state)
		{
		/* If Drop Target Down go to Caster State...
		   register a hit to the drop target, and knock it down, but roll free*/
		case stDropTarget:
			if(locals.dtPos == DOWN)
				return setState(stCaster,5);
			else
				{
				/*Knock Drop Target Down*/
				locals.dtPos = DOWN;
				return setState(stFree,1);
				}


  		/*Gate ->  Ball moving from Right->Left: If Gate Open, Continue to SpinnerRL, otherwise, to LIE_L Target*/
		case stGateRL:
			if(locals.gatePos == OPEN)
				return setState(stSpinnerRL,5);
			else
				return setState(stLIE_L,15);

		/*Gate ->  Ball moving from Left->Right: */
		/*IGNORE GATE POSITION.. IT'S ALWAYS ALLOWED THROUGH!*/
		case stGateLR:
			return setState(stEjectHole,10);

		case stReel:
			if(locals.reelPos>=BALL1UP && locals.reelPos<=BALL1UP+ERROR_RANGE)
				return setState(stReel1,5);
			if(locals.reelPos>=BALL2UP && locals.reelPos<=BALL2UP+ERROR_RANGE)
				return setState(stReel2,5);
			if(locals.reelPos>=BALL3UP && locals.reelPos<=BALL3UP+ERROR_RANGE)
				return setState(stReel3,5);
			return setState(stFree,5);
		}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------
  Each entry maps a keypress to either begin a state or trigger a switch!
  The Inport #, and Bit Mask define which keypresses should be used - See code from
  WPC_INPUT_PORTS_START to determine # and Mask for each key press
--------------------------------------------------------------------------------------------------
  Layout of inportData array
   ----------------------------
   #1) Inport number
   #2) Mask for this switch (all bits must be set for switch to activate
       Only first match will be used. Placed qualified keys first
   #3) Switch to activate or new ball state
   #4) Condition for the action to occur
                 0                  : ball in stFree state
                 switch             : ball in stFree state and switch active
                 SIM_STANY       : ball can be in any state
                 SIM_CUSTCOND(n) : call custom condition handler (keyCond)
--------------------------------------------------------------------------------------------------*/

static sim_tInportData ft_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0009, stLIn},
  {0, 0x000a, stRIn},
  {0, 0x0011, stLOut},
  {0, 0x0012, stROut},
  {0, 0x0041, stLLoopUp},
  {0, 0x0042, stRLoopUp},
  {0, 0x0081, stLSling},
  {0, 0x0082, stRSling},
  {0, 0x0100, stJetB1},
  {0, 0x0200, stJetB2},
  {0, 0x0400, stJetB3},
  {0, 0x0800, stLie},
  {0, 0x1000, stlIe},
  {0, 0x2000, stliE},
  {0, 0x4000, stRDT1},
  {0, 0x8000, stRDT2},
  {1, 0x0001, stLDT1},
  {1, 0x0002, stLDT2},
  {1, 0x0004, stDrain},
  {1, 0x0008, stDropTarget},
  {1, 0x0010, stExtraBall},
  {1, 0x0040, stCaptiveBall},
  {0}
};


/*--------------------
  Drawing information
 ---------------------*/
/*--------------------------------------------------------
  Code to draw the mechanical objects, and their states!
---------------------------------------------------------*/

static void ft_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Gate: %-6s", (locals.gatePos==OPEN)?"Open":"Closed");
  core_textOutf(30, 10,BLACK,"Drop Target: %-6s", (locals.dtPos==UP)?"Up":"Down");
  core_textOutf(30, 20,BLACK,"Reel: %-15s",showReelPos());
//  core_textOutf(30, 20,BLACK,"Reel: %6d", locals.reelPos);
}
static void ft_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W = Captive Ball");
  core_textOutf(30,110,BLACK,"E = Extra Ball Tgt, T = Caster's Club");
  core_textOutf(30,120,BLACK,"H/J/K = Jet Bumpers");
  core_textOutf(30,130,BLACK,"Z/X/C = L/I/E Rollovers");
  core_textOutf(30,140,BLACK,"V/B,N/M = Left,Right Bank Targets");

}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(ft,l5,"fshtl_5.rom",0x80000,CRC(88847775) SHA1(ab323980b914678e1e3c9e2e4d92956e97dc32fa))
WPCS_SOUNDROM8xx("ft_u18.l1",CRC(48d2760a) SHA1(701b0bbb68f99332493ee1276e5a1cef5c85d499))
WPC_ROMEND

WPC_ROMSTART(ft,l3,"ft-l3.u6",0x80000,CRC(db1d3b4d) SHA1(a38afa42014140b566bc8923db9025ebdee16068))
WPCS_SOUNDROM8xx("ft_u18.l1",CRC(48d2760a) SHA1(701b0bbb68f99332493ee1276e5a1cef5c85d499))
WPC_ROMEND

WPC_ROMSTART(ft,l4,"fshtl_4.rom",0x80000,CRC(beb147b6) SHA1(543eea2e14283485221c1a28c3fdf3c87df8acc7))
WPCS_SOUNDROM8xx("ft_u18.l1",CRC(48d2760a) SHA1(701b0bbb68f99332493ee1276e5a1cef5c85d499))
WPC_ROMEND

WPC_ROMSTART(ft,p4,"ft_p4.u6",0x80000,CRC(386cbe45) SHA1(5cb4a32591121c4ed3da292fb50cec0d8d5dd44f))
WPCS_SOUNDROM8xx("ft_u18.l1",CRC(48d2760a) SHA1(701b0bbb68f99332493ee1276e5a1cef5c85d499))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(ft,l5,"Fish Tales (L-5)",1992, "Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ft,l3,l5,"Fish Tales (L-3)",1992, "Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ft,l4,l5,"Fish Tales (L-4)",1992, "Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(ft,p4,l5,"Fish Tales (P-4)",1992, "Williams",wpc_mFliptronS,0)


/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData ftSimData = {
  2,    			/* # game specific input ports */
  ft_stateDef,                  /* Definition of all states */
  ft_inportData,                /* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 			/* no init */
  ft_handleBallState,           /*Function to handle ball state changes*/
  ft_drawStatic,                /*Function to handle mechanical state changes*/
  FALSE, 			/* Do Not Simulate manual shooter */
  NULL  			/* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData ftGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD, /* generation */
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),	/* Which switches are the flippers */
    0,0,3,0,0,0,0,
    ft_getSol, ft_handleMech, ft_getMech, ft_drawMech,
    NULL, NULL
  },
  &ftSimData,
  {
    {0}, 			/* no serial number */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* Inverted Switches*/
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swCast},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_ft(void) {
  core_gameData = &ftGameData;
  /*Gate starts as closed!*/
  locals.gatePos = CLOSED;
}

/*-- return status of custom solenoids --*/
static int ft_getSol(int solNo) {
 /* Motor Must be STOPPED for ball to drop to catapult! */
 if (core_getSol(sReelMotor)==0)
 {
  if (solNo == sFakeReel1)
    return (locals.reelPos>=BALL1DOWN && locals.reelPos<=BALL1DOWN+ERROR_RANGE);
  if (solNo == sFakeReel2)
    return (locals.reelPos>=BALL2DOWN && locals.reelPos<=BALL2DOWN+ERROR_RANGE);
  if (solNo == sFakeReel3)
    return (locals.reelPos>=BALL3DOWN && locals.reelPos<=BALL3DOWN+ERROR_RANGE);
 }
 return 0;
}


/***************************************************************************************************
 Functions here must manually adjust the state of mechanicla objects, usually checking for solenoids
 which control the state of those objects. Opening & Closing Doors, Diverter Switchs, etc..
****************************************************************************************************/
static void ft_handleMech(int mech) {
  /* -------------------------------------
     --	Raise & Lower Drop Target Ramp  --
     ------------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is up, & Ramp Down Solenoid firing lower it! --*/
    if (locals.dtPos == UP && core_getSol(sDropTargetDown))
      locals.dtPos = DOWN;
    /*-- if Ramp is down, & Ramp Up Solenoid firing raise it! --*/
    if (locals.dtPos == DOWN && core_getSol(sDropTargetUp))
      locals.dtPos = UP;
    /*-- Set DropTarget Switch --*/
    core_setSw(swDropTarget,(locals.dtPos == DOWN)); /*If Ramp Down, Switch Must be Active*/
  }

  /* -- If Gate Solenoid Firing, Gate is...! --*/
  /* WPCMAME: This one seems unnecessary. Why not use getSol directly? */
  if (core_getSol(sLGate))
    locals.gatePos = OPEN;
  else
    locals.gatePos = CLOSED;

  /*------ REEL LOGIC ----------------------
     Totally Guessed - with help from Ari
  ------------------------------------------
   	Position: 00-10: Ball 1 Up: Opto #1: Closed: Opto #2: Closed
	Position: 11-24: In Between: Opto #1: Open: Opto #2: Open
   	Position: 25-35: Ball 3 Down: Opto #1: Closed: Opto #2: Open
	Position: 36-49: In Between: Opto #1: Open: Opto #2: Open
   	Position: 50-60: Ball 2 Up: Opto #1: Closed: Opto #2: Open
	Position: 61-74: In Between: Opto #1: Open: Opto #2: Open
   	Position: 75-85: Ball 1 Down: Opto #1: Closed: Opto #2: Open
	Position: 86-99: In Between: Opto #1: Open: Opto #2: Open
   	Position: 100-110: Ball 3 Up: Opto #1: Closed: Opto #2: Open
	Position: 111-124: In Between: Opto #1: Open: Opto #2: Open
   	Position: 125-135: Ball 2 Down: Opto #1: Closed: Opto #2: Open
	Position: 136-149: In Between: Opto #1: Open: Opto #2: Open
	Back to 00!
  ----------------------------------------------------------------*/

  if (mech & 0x02) {
    /* Flip Reel back to 0 after Position 150 */
    if (locals.reelPos >=150)
      locals.reelPos = 0;

    /* Opto #2 Only set when position = Ball1Up! */
    core_setSw(swReel2Opto,(locals.reelPos>=BALL1UP && locals.reelPos<=BALL1UP+ERROR_RANGE));

    /* Opto #1 Closed during Ball Positions */
    if ((locals.reelPos>=BALL1UP && locals.reelPos<=BALL1UP+ERROR_RANGE) ||
        (locals.reelPos>=BALL2UP && locals.reelPos<=BALL2UP+ERROR_RANGE) ||
        (locals.reelPos>=BALL3UP && locals.reelPos<=BALL3UP+ERROR_RANGE) ||
        (locals.reelPos>=BALL1DOWN && locals.reelPos<=BALL1DOWN+ERROR_RANGE) ||
        (locals.reelPos>=BALL2DOWN && locals.reelPos<=BALL2DOWN+ERROR_RANGE) ||
        (locals.reelPos>=BALL3DOWN && locals.reelPos<=BALL3DOWN+ERROR_RANGE)      )
      core_setSw(swReel1Opto,1);
    else
      core_setSw(swReel1Opto,0);

    /* -- If Reel Motor Turning Rotate Position! --*/
    if (core_getSol(sReelMotor))
      locals.reelPos++;
  }

}

static const char * showReelPos(void)
{
  if(locals.reelPos>=BALL1UP && locals.reelPos<=BALL1UP+ERROR_RANGE)
	return "Ball 1 Up";
  if(locals.reelPos>=BALL2UP && locals.reelPos<=BALL2UP+ERROR_RANGE)
	return "Ball 2 Up";
  if(locals.reelPos>=BALL3UP && locals.reelPos<=BALL3UP+ERROR_RANGE)
	return "Ball 3 Up";
  if(locals.reelPos>=BALL1DOWN && locals.reelPos<=BALL1DOWN+ERROR_RANGE)
	return "Ball 1 Down";
  if(locals.reelPos>=BALL2DOWN && locals.reelPos<=BALL2DOWN+ERROR_RANGE)
	return "Ball 2 Down";
  if(locals.reelPos>=BALL3DOWN && locals.reelPos<=BALL3DOWN+ERROR_RANGE)
	return "Ball 3 Down";
  return "Spinning";
}

static int ft_getMech(int mechNo) {
  switch (mechNo) {
	//Return Reel Position
    case 0: return locals.reelPos;
	//Return Which Ball Lock is UP in Reel Position!
	case 1:
		if(locals.reelPos>=BALL1UP && locals.reelPos<=BALL1UP+ERROR_RANGE)
			return 1;
		if(locals.reelPos>=BALL2UP && locals.reelPos<=BALL2UP+ERROR_RANGE)
			return 2;
		if(locals.reelPos>=BALL3UP && locals.reelPos<=BALL3UP+ERROR_RANGE)
			return 3;
		return 0;
    }
  return 0;
}
