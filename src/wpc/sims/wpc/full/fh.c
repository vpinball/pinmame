/********************************************************************************************
 Williams Funhouse (1990) Pinball Simulator

 by Steve Ellenoff (sellenoff@hotmail.com)
 Nov.2, 2000

 Known Issues/Problems with Simulator:
 -None at the moment.

 Notes:
 ------
 I've tried to be very generous with comments so you could learn how I
 created the logic for this simulator. I tried to use C code syntax that was
 easy to read, instead of using C code shortcuts, which make the code look
 better, but is harder to understand (especially if you didn't write it!)

 This is my first shot at writing a simulator for PINMAME. At first I was very
 intimidated. I thought it would be really hard, and I couldn't really follow the
 existing simulator code. What I did, however, was try to understand small pieces of
 it.. Eventually it all started to make perfect sense, and it became really easy to do
 after that!!

 Changes:
 --------
 12-05-2000: Modified custom playfield lamp layout to accomodate new structure.
             Added support for shared bulbs where necessary
 12-15-2000: Added Sound Sample Support

 Guide to Writing Your Own Sim:
 ------------------------------
 Luckily, Funhouse is not a complicated pinball machine, and this simulator is a good example
 to learn from, since there's not a lot of things to handle.

 Start by deciding what keys will cause which shots to happen.  You must also make sure you think
 of every possible shot in the game. Don't worry, there's usually not that many of them.. Each target
 you could possible hit would be a shot. Same for bumpers, ramps, in lanes, outlanes, etc...
 Each one is defined as a shot. So if there are two ramps in the game, then there's two shots, one for
 each ramp. Once you've figured out what keys will go to what shots, set it up in the code, following
 the C code used here.. When deciding shots, it helped me to look at a picture of the playfield to help
 visualize all possible shots that can be made.

 One thing to remember, when setting up shots. The shot can either do 2 things:
 #1) It can simply trigger a switch, and then let the ball roll free (aka a drop target)...
 #2) It can set the ball into a new state, which can cause any kind of chain reaction needed, such as
     following the path of a ball through a ramp and triggering switches along the way...

 Don't get hung up on fully defining all your states, just make sure you map one to each key as needed.

 Next map out the switches and their corresponding switch #. Then the solenoids. I read the #'s straight
 out of the manually, and in order, since it was easiest to do that way..

 You should also be thinking about what mechanical objects you want to show the user, so they know if
 a certain object is open or closed, or whatever..

 Once you've mapped all keys to switches and states, and have setup what mechanical objects to track,
 you're ready to define the ball states!!

 Defining the ball states is probably the hardest part, but think of it like this.. you are saying
 where the ball will go next. Ultimately, you signify what switch is triggered, and what's the next
 state the ball goes too.. Use stFree to tell the simulator, that the ball is just rolling around the
 playfield, not touching anything.

 Check out the comments near the ball states code for more details on this process. Remember,
 each ball state can only point to the next state. So if you have a ramp, that has
 4 switches on it you need 3 states to define them. State 1 is where the ball enters the ramp, it points
 to State 2 which points to 3, and then finally to 4.. For each state, you are telling the simulator
 what switch is activated for that state. It's hard to describe, but try and follow the examples found
 in this code.

 *******************************************************************************************************
 I hope you are able to learn from this example, and I look forward to seeing
 your own Simulators of pinball games you wish to simulate!!

 Thanks goes to the author of wpcmame for writing an excellent simulator engine!

 I will be happy to try and answer your questions on shiva's wpcmame forum if you are having
 trouble!
*********************************************************************************************************/


/*-------------------------------------------------------------------------------------------------
  Keys for Funhouse Simulator:
  ----------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +L  L/R Loop
    +O  L/R Outlane
     W  Wind Tunnel
     F  Rudy Hit
     T  Trap Door Loop
     Q  SDTM (Drain Ball)
     E  Steps Drop Hole
     U  Hidden Hall Way
   HJK  Jet Bumpers
  XCVB  STEP Drop Targets
  NM,.  Left Ramp Award Frenzy, Extra Ball, 500,000 Pts, Start ,Super Hot Dog
   ASD  Hot Dog Drop Targets
     Z  Rudy's Hideout
     G  Right Outer InLane
     Y  Jet Bumper Lane
-------------------------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "wpcsam.h"

/*------------------
/  Local functions
/-------------------*/
/*-- local state/ball handling functions --*/
static int  fh_handleBallState(sim_tBallStatus *ball, int *inports);
static void fh_handleMech(int mech);
static void fh_drawMech(BMTYPE **line);
static void fh_drawStatic(BMTYPE **line);
static void init_fh(void);
static const char* showeyepos(void);

/*-----------------------
  local static variables
 ------------------------
   I used this locals structure for setting up variables to track the state of mechanical objects
   which needed to be displayed on screen
--------------------------------------------------------------------------------------------------*/
static struct {
  int stepgatePos;    /* Steps Gate Position */
  int trapdoorPos;    /* Trap Door Position  */
  int rudymouthPos;   /* Rudy's Mouth Position */
  int rudyeyesOC;     /* Rudy's Eyes Open or Closed? */
  int rudyeyesLR;     /* Rudy's Eyes Left or Right?  */
  int diverterPos;    /* Position of Diverter Open or Closed? */
  int divertercount;  /* Count # of times diverter solenoid is inactive*/
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------
   Here we define for PINMAME which Keyboard Presses it should look for, and process!
--------------------------------------------------------------------------------------------------*/
WPC_INPUT_PORTS_START(fh,3)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"L/R Inlane",      KEYCODE_I)
    COREPORT_BITIMP(0x0100,"Wind Tunnel",     KEYCODE_W)
    COREPORT_BITIMP(0x0200,"Rudy Hit",        KEYCODE_F)
    COREPORT_BITIMP(0x0400,"Trap Door Loop",  KEYCODE_T)
    COREPORT_BITIMP(0x0800,"Steps Drop Hole", KEYCODE_E)
    COREPORT_BIT(   0x1000,"Hidden Hall Way", KEYCODE_U)
    COREPORT_BIT(   0x2000,"Jet 1",	     KEYCODE_H)
    COREPORT_BITIMP(0x4000,"Jet 2",           KEYCODE_J)
    COREPORT_BIT(   0x8000,"Jet 3",           KEYCODE_K)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"'S'TEP",          KEYCODE_X)
    COREPORT_BIT(   0x0002,"S'T'EP",          KEYCODE_C)
    COREPORT_BIT(   0x0004,"ST'E'P",          KEYCODE_V)
    COREPORT_BIT(   0x0008,"STE'P'",          KEYCODE_B)
    COREPORT_BIT(   0x0010,"Hot Dog Upper",   KEYCODE_A)
    COREPORT_BIT(   0x0020,"Hot Dog Middle",  KEYCODE_S)
    COREPORT_BIT(   0x0040,"Hot Dog Lower",   KEYCODE_D)
    COREPORT_BIT(   0x0100,"Award Frenzy",    KEYCODE_N)
    COREPORT_BIT(   0x0200,"Award EB",        KEYCODE_M)
    COREPORT_BIT(   0x0400,"Award 500,000",   KEYCODE_COMMA)
    COREPORT_BIT(   0x0800,"Award Super Dog", KEYCODE_STOP)
    COREPORT_BIT(   0x1000,"Rudy's Hideout",  KEYCODE_Z)
    COREPORT_BITIMP(0x2000,"Drain",           KEYCODE_Q)
    COREPORT_BIT(   0x4000,"Right Inlane Outer", KEYCODE_G)
    COREPORT_BIT(   0x8000,"Jet Bumper Lane", KEYCODE_Y)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swAwardFrenzy 	15
#define swMRamp		16
#define swStepS		17
#define swJet1		18

#define swSlamTilt   	21
#define swCoinDoor   	22
#define swTicket     	23
#define swNotUsed	24
#define swLockR		25
#define	swAwardEB	26
#define	swLockC		27
#define swLockL		28

#define	swStepP		31
#define	swHotDogU	32
#define	swGangWayL	33
#define	swHotDogL	34
#define	swURampExit	35
#define	swAwardPTS	36
#define	swHotDogM	37
#define	swURampEnt	38

#define swLSling     	41
#define swLIn		42
#define swLOut		43
#define swWindTunnel	44
#define swTrapOpen  	45
#define swRudyHideout	46
#define swLShooter  	47
#define swMRampExit	48

#define swRudyJaw	51	/*Opto*/
#define swROut		52
#define swRSling     	53
#define swStepT		54
#define swAwardDog	55	/*Opto*/
#define swMRampEnt	56
#define swJetLane	57
#define swDropKickout	58

#define swRIn		61
#define swRShooter    	62
#define swRTrough    	63
#define swStepE		64
#define swDummyEject	65
#define swGangWayR	66
#define swDropHole	67
#define swJet3       	68

#define swRIn2		71
#define swLTrough    	72
#define swOuthole    	73
#define swCTrough    	74
#define swTrapLoop	75
#define swTrapClosed	76
#define swJet2       	77


/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOuthole      	1
#define sRampDiv      	2
#define sRudyHideout  	3
#define sDropKickout	4
#define sTrapOpen	5
#define sTrapClosed	6
#define sKnocker       	7
#define sLockRelease   	8
#define sJet1         	9
#define sJet2        	10
#define sJet3        	11
#define sLSling      	12
#define sRSling      	13
#define sStepGate	14
#define sBallRel	15
#define sDummyEject    	16

#define sMouthMotor    	21
#define sUpDownDriver  	22

#define sEyesRight     	25
#define sEyesOpen      	26
#define sEyesClosed   	27
#define sEyesLeft    	28

/*Status of Eyes*/
enum {EYES_ST=0,EYES_LEFT,EYES_RIGHT,EYES_CLOSED};
#define OPEN   0
#define CLOSED 1

/*---------------------
/  Ball state handling
/----------------------*/

/* ---------------------------------------------------------------------------------
   The following variables are used to refer to the state array!
   These vars *must* be in the *exact* *same* *order* as each stateDef array entry!
   -----------------------------------------------------------------------------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOuthole, stDrain,
      stRShooter, stRBallLane, stRNotEnough, stROut, stLOut, stRIn, stRIn2, stLIn,
      stLLoopUp, stLLoopDn, stRLoopUp, stRLoopDn,
      stMRampEnt, stMRamp, stMRampExit, stRampDiv, stURampEnt, stURampExit,
      stLShooter, stLBallLane, stLNotEnough, stAwardFrenzy, stAwardEB, stAwardPTS, stAwardDog,
      stRudyHideout, stRudyHideout2, stWindTunnel,
      stDropHole, stDropKickout, stHiddenHallway, stLockL, stLockC, stLockR,
      stRudyHit, stTrapDoorLoop, stUpperLoop, stBallInTrap, stRudyGulp, stLOut2, stRudyJaw,
      stRudyJaw1, stJetLane, stJet1, stJet2, stJet3
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
	STNOTEXCL:       More than one ball can be in this sate
	STNOTBALL(type): Specified ball type does not affect switch
	STSPINNER        Switch is a spinner
*******************************************************************************************************/
static sim_tState fh_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",               0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  {"Right Trough",     1,swRTrough,   sBallRel, stRShooter,  5},
  {"Center Trough",    1,swCTrough,   0,        stRTrough,   1},
  {"Left Trough",      1,swLTrough,   0,        stCTrough,   1},
  {"Outhole",          1,swOuthole,   sOuthole, stLTrough,   5},
  {"Drain",            1,0,           0,        stOuthole,   0,0,0,SIM_STNOTEXCL},

  {"R. Shooter",       1,swRShooter,   sShooterRel, stRBallLane, 10,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
  {"R. Ball Lane",     1, 0,  	       0,       0,      	     0,0,0,SIM_STNOTEXCL},
  {"Not Enough",       1,swRShooter,   0,        stRShooter, 3},

  {"Right Outlane",    1,swROut,       0,        stDrain,   20},
  {"Left Outlane",     1,swLOut,       0,        stLOut2,   20},

  {"Right Inlane",     1,swRIn,       0,        stFree,      5},
  {"Right Inlane (Outer)",     1,swRIn2,       0,        stFree,      5},
  {"Left  Inlane",     1,swLIn,       0,       stFree,      5},

  {"Left Loop",        1,swGangWayL,    0,        stRLoopDn,  10},
  {"Left Loop",        1,swGangWayL,    0,        stFree,      1},

  {"Right Loop",       1,swGangWayR,     0,       stLLoopDn,  10},
  {"Right Loop",       1,swGangWayR,     0,       stFree,      1},

  {"Main Ramp Enter",  1,swMRampEnt,   0,       stMRamp,     2},
  {"Main Ramp"	    ,  1,swMRamp,      0,       stRampDiv,   5},
  {"Main Ramp Exit",   1,swMRampExit,  0,       stLIn,       3},
  {"Ramp Diverter",    1,0,            0,       stMRampExit, 2, sRampDiv, stURampEnt},

  {"Steps Track Enter",  1,swURampEnt,   0,     stURampExit, 5},
  {"Steps Track Exit" ,  1,swURampExit,  0,     stLOut,      2},

  {"L. Shooter",       1,swLShooter,   sShooterRel, stLBallLane, 5,0,0,SIM_STNOTEXCL|SIM_STSHOOT},
  {"L. Ball Lane"  ,   1, 0,  	       0,       0,      	     0,0,0,SIM_STNOTEXCL},
  {"Not Enough",       1,swLShooter,   0,        stLShooter, 2},
  {"Award Frenzy",     1,swAwardFrenzy, 0,       stFree, 1},
  {"Award Ex.Ball",    1,swAwardEB,     0,       stFree, 3},
  {"Award 500,000",    1,swAwardPTS,    0,       stFree, 6},
  {"Award SuperDog",   1,swAwardDog,    0,       stFree, 8},

  {"Rudy's Hideout",   1,swGangWayR, 0, stRudyHideout2, 5},
  {"Rudy's Hideout",   1,swRudyHideout, sRudyHideout, stLLoopDn, 1},

  {"Wind Tunnel",      1,swWindTunnel,  0,       stDropKickout, 25},
  {"Drop Hole",        1,swDropHole,    0,       stDropKickout, 5},
  {"Drop Kickout",     1,swDropKickout, sDropKickout,   stFree, 5},

  {"Hidden HallWay",   1,0,    0,       stLockR, 10},
  {"Left Lock",        1,swLockL,    sLockRelease,       stFree, 5},
  {"Center Lock",      1,swLockC,    0,       		 stLockL, 1},
  {"Right Lock",       1,swLockR,    0,       		 stLockC, 1},

  {"Rudy Hit",         1,0, 0,  0, 1},
  {"Upper Loop",       1,0,    0,       0, 0},
  {"Upper Loop Made",	1,swTrapLoop, 0, stFree, 5},
  {"Ball in Trap Door", 1,swTrapOpen, 0, stDropKickout, 25},
  {"Rudy's Mouth",      1,swDummyEject, sDummyEject, stFree, 5},
  {"Left Outlane",	1,0,0,0,0},
  {"Rudy Jaw",		1,swRudyJaw,0,stFree,5},
  {"Rudy Jaw",		1,swRudyJaw,0,stRudyGulp,5},
  {"Jet Bumper Lane",   1,swJetLane,0,stRIn2,5},
  {"Jet Bumper 1",        1,swJet1, 0, stJet2, 5},
  {"Jet Bumper 2",        1,swJet2, 0, stJet3, 5},
  {"Jet Bumper 3",        1,swJet3, 0, stFree, 5},
  {0}
};

static int fh_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in RIGHT Shooter Lane */
        /* Note: Sim supports max of 50 speed for manual plunger */
    	case stRBallLane:
		if (ball->speed < 20)
			return setState(stRNotEnough,3);	/*Ball not plunged hard enough*/
		if (ball->speed < 25)
			return setState(stJetLane,10);		/*Ball rolled down Jet Lane*/
		if (ball->speed < 30)
			return setState(stDropHole,20);		/*Ball landed in Drop Hole*/
		if (ball->speed < 35)
                        return setState(stJet1,20);              /*Ball Hit Bumper!*/
		if (ball->speed < 40)
			return setState(stRudyHideout,35);	/*Skill Shot - Landed in Rudy Hideout*/
		if (ball->speed >= 40)
			return setState(stRLoopUp,30);		/*Shot missed hideout, but triggered Right Loop!*/
		break;

	/* Ball in LEFT Shooter Lane */
        /* Note: Sim supports max of 50 speed for manual plunger */
    	case stLBallLane:
		if (ball->speed < 25)
			return setState(stLNotEnough,3);	/*Ball not plunged hard enough*/
		if (ball->speed < 30)
			return setState(stAwardFrenzy,15);	/*Ball landed in Steps Award Frenzy*/
		if (ball->speed < 35)
			return setState(stAwardEB,20);		/*Ball landed in Steps Award Extra Ball*/
		if (ball->speed < 40)
			return setState(stAwardPTS,25);		/*Ball landed in Steps Award Points*/
		if (ball->speed >= 40)
			return setState(stAwardDog,30);		/*Shot Awards Super Dog*/
		break;

	/* Rudy Hit */
    	case stRudyHit:
		/*Is Rudy's Mouth Open?*/
		if (locals.rudymouthPos)
			return setState(stRudyJaw1,10);		/*Yes, ball goes into rudy's mouth*/
		else
			return setState(stRudyJaw,10);		/*Ball hits Rudy's Jaw!!*/
		break;

	/* Trap Door */
    	case stTrapDoorLoop:
		/*Is the Trap Door Open?*/
		if (locals.trapdoorPos)
			return setState(stBallInTrap,10);	/*Trap Door is Open, Ball Lands in Trap Door!*/
		else
			return setState(stUpperLoop,10);	/*Trap Door is Closed, Make Upper Loop Shot!*/
		break;

	/* Left Outlane - Drain or Go to Left Shooter? */
	case stLOut2:
		if (locals.stepgatePos)
			{
			ball->speed = -1;
			return setState(stLShooter,5);
			}
		else
			return setState(stDrain,15);
		break;
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

static sim_tInportData fh_inportData[] = {
  {0, 0x0005, stMRampEnt},
  {0, 0x0006, stMRampEnt},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0100, stWindTunnel},
  {0, 0x0200, stRudyHit},
  {0, 0x0400, stTrapDoorLoop},
  {0, 0x0800, stDropHole},
  {0, 0x1000, stHiddenHallway},
  {0, 0x2000, swJet1},
  {0, 0x4000, swJet2},
  {0, 0x8000, swJet3},
  {1, 0x0001, swStepS},
  {1, 0x0002, swStepT},
  {1, 0x0004, swStepE},
  {1, 0x0008, swStepP},
  {1, 0x0010, swHotDogU},
  {1, 0x0020, swHotDogM},
  {1, 0x0040, swHotDogL},
  {1, 0x0100, stAwardFrenzy},
  {1, 0x0200, stAwardEB},
  {1, 0x0400, stAwardPTS},
  {1, 0x0800, stAwardDog},
  {1, 0x1000, stRudyHideout},
  {1, 0x2000, stDrain},
  {1, 0x4000, swRIn2},
  {1, 0x8000, stJetLane},
  {0}
};

/*--------------------
  Drawing information
  --------------------------------------------------------
  Code to draw the mechanical objects, and their states!
---------------------------------------------------------*/
static core_tLampDisplay fh_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{32, 9,ORANGE}}},{1,{{33,11,ORANGE}}},{1,{{33,13,ORANGE}}},{1,{{33,15,ORANGE}}},
 {1,{{33,17,ORANGE}}},{1,{{32,19,RED}}},{1,{{36,14,RED}}},{1,{{33, 3,LBLUE}}},
 {1,{{25, 9,ORANGE}}},{1,{{27,10,YELLOW}}},{1,{{29,14,YELLOW}}},{1,{{30,17,ORANGE}}},
 {1,{{25,19,ORANGE}}},{1,{{22,19,ORANGE}}},{1,{{20,14,RED}}},{1,{{18,14,RED}}},
 {1,{{28, 9,ORANGE}}},{1,{{30,11,ORANGE}}},{1,{{30,14,ORANGE}}},{1,{{28,19,ORANGE}}},
 {1,{{25,18,YELLOW}}},{1,{{21,16,YELLOW}}},{1,{{21,12,YELLOW}}},{1,{{22, 9,ORANGE}}},
 {1,{{25,10,YELLOW}}},{1,{{29,12,YELLOW}}},{1,{{29,16,YELLOW}}},{1,{{27,18,YELLOW}}},
 {1,{{23,18,YELLOW}}},{1,{{20,17,ORANGE}}},{1,{{20,11,ORANGE}}},{1,{{23,10,YELLOW}}},
 {1,{{16,24,LBLUE}}},{1,{{15,21,RED}}},

 /*Lamp 35 - Matrix # 53 - Splits into 3 bulbs*/
 {3,{{16,17,RED},{15,17,RED},{14,17,RED}}},

 {1,{{10, 3,ORANGE}}},{1,{{ 8, 3,ORANGE}}},{1,{{ 6, 3,ORANGE}}},
 {1,{{10, 6,RED}}},{1,{{17,22,YELLOW}}},

 /*Lamp 41 - Matrix # 61 - Splits into 2 bulbs*/
 {2,{{27,25,YELLOW},{27, 4,YELLOW}}},

 {1,{{22, 5,LBLUE}}},{1,{{13,14,RED}}},{1,{{12, 9,ORANGE}}},
 {1,{{12, 7,LBLUE}}},{1,{{14, 3,YELLOW}}},{1,{{ 8,16,ORANGE}}},{1,{{ 6,17,YELLOW}}},
 {1,{{ 1,13,RED}}},{1,{{13,24,WHITE}}},{1,{{18,19,LBLUE}}},{1,{{ 2,13,YELLOW}}},
 {1,{{ 3,13,YELLOW}}},{1,{{ 4,13,YELLOW}}},{1,{{ 5,13,YELLOW}}},{1,{{ 6,13,GREEN}}},
 {1,{{15, 8,RED}}},

 /*Lamp 58 - Matrix # 82 - Splits into 2 bulbs*/
 {2,{{27, 3,RED},{29,26,RED}}},

 {1,{{14,11,YELLOW}}},{1,{{13,10,LBLUE}}},
 {1,{{11,13,WHITE}}},{1,{{11,10,LBLUE}}},{1,{{11,16,YELLOW}}},{1,{{39, 1,YELLOW}}}
}
};


/***************************************************************
  Solenoid to Sample Mapping -

  This list links a solenoid to a sampled (.wav) sound file.
  Each Sample must play in a certain channel #. If two sounds
  can play at the same time, specify different channels for each,
  since if a channel is playing a sound, any new sounds will not be
  played. Max # of Channels is 6!

  The code specifies: SOLENOID, CHANNEL #, and SAMPLE NAME
  *************************************************************/
static wpc_tSamSolMap fh_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sBallRel,0,SAM_BALLREL},
 {sOuthole,0,SAM_OUTHOLE},
 {sLockRelease,0,SAM_SOLENOID}, {sDummyEject,0,SAM_POPPER},

//Ramp Diverter needs special checking due to solenoid smoothing!
// {sRampDiv,0,SAM_DIVERTER},

 /*Channel #1*/
 {sLSling,1,SAM_LSLING}, {sRSling,1,SAM_RSLING},
 {sJet1,1,SAM_JET1}, {sJet2,1,SAM_JET2},
 {sJet3,1,SAM_JET3}, {sDropKickout,1,SAM_POPPER},
 {sRudyHideout,1,SAM_SOLENOID},

 /*Channel #2*/
 {sTrapOpen,2,SAM_FLAPOPEN}, {sTrapClosed,2,SAM_FLAPCLOSE},

 /*Channel #3*/
 {sEyesOpen,3,SAM_SOLENOID}, {sEyesClosed,3,SAM_SOLENOID},

 /*Channel #4*/
 {sMouthMotor,4,SAM_MOTOR_1, WPCSAM_F_CONT},{-1}

// Eyes need special checking due to solenoid smoothing!
// {sEyesRight,3,SAM_DIVERTER},  {sEyesLeft,3,SAM_DIVERTER}

};

static void fh_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Trap Door: %-6s", locals.trapdoorPos?"Open":"Closed");
  core_textOutf(30, 10,BLACK,"Step Gate: %-6s", locals.stepgatePos?"Open":"Closed");
  core_textOutf(30, 20,BLACK,"Rudy Jaw : %-6s", locals.rudymouthPos?"Open":"Closed");
  core_textOutf(30, 30,BLACK,"Rudy Eyes: %-10s", showeyepos());
}
  /* Help */

static void fh_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+R = L/R Ramp Shot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W = Wind Tunnel");
  core_textOutf(30,100,BLACK,"E = Steps Hole, T = Trap Door Loop");
  core_textOutf(30,110,BLACK,"Y = Jet Bumper Lane, U = Hidden Hallw.");
  core_textOutf(30,120,BLACK,"A/S/D = Hot Dog Targets, F = Rudy Hit");
  core_textOutf(30,130,BLACK,"G = Rt Outer Inl., H/J/K = Jet Bumpers");
  core_textOutf(30,140,BLACK,"X/C/V/B = S/T/E/P Drop Targets");
  core_textOutf(30,150,BLACK,"N/M/,/. = Left Ramp Awd Frenzy/ExBall/");
  core_textOutf(30,160,BLACK,"500,000 Pts./Start Super HotDog");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(fh,l9,"funh_l9.rom",0x040000,CRC(c8f90ff8) SHA1(8d200ea30a68f5e3ba1ac9232a516c44b765eb45))
WPCS_SOUNDROM222("fh_u18.l2m",CRC(7f6c7045) SHA1(8c8d601e8e6598507d75b4955ccc51623124e8ab),
                 "fh_u15.l2", CRC(0744b9f5) SHA1(b626601d82e6b1cf25f7fdcca31e623fc14a3f92),
                 "fh_u14.l2", CRC(3394b69b) SHA1(34690688f00106b725b27a6975cdbf1e077e3bb3))
WPC_ROMEND
// Found this on the stormaster site
// It is an updated L9 rom where the German text translation is corrected
// Author is unknown but it is definetly not from WMS
// (author didn't know how to calculate the checksum so he
//  altered some blanks in the credits to match the original checksum)
WPC_ROMSTART(fh,l9b,"fh_l9ger.rom",0x040000,CRC(e9b32a8f) SHA1(deb77f0d025001ddcc3045b4e49176c54896da3f))
WPCS_SOUNDROM222("fh_u18.l2m",CRC(7f6c7045) SHA1(8c8d601e8e6598507d75b4955ccc51623124e8ab),
                 "fh_u15.l2", CRC(0744b9f5) SHA1(b626601d82e6b1cf25f7fdcca31e623fc14a3f92),
                 "fh_u14.l2", CRC(3394b69b) SHA1(34690688f00106b725b27a6975cdbf1e077e3bb3))
WPC_ROMEND

WPC_ROMSTART(fh,l3,"u6-l3.rom",0x020000,CRC(7a74d702) SHA1(91540cdc62c855b4139b202aa6ad5440b2dee141))
WPCS_SOUNDROM222("fh_u18.l2m",CRC(7f6c7045) SHA1(8c8d601e8e6598507d75b4955ccc51623124e8ab),
                 "fh_u15.l2", CRC(0744b9f5) SHA1(b626601d82e6b1cf25f7fdcca31e623fc14a3f92),
                 "fh_u14.l2", CRC(3394b69b) SHA1(34690688f00106b725b27a6975cdbf1e077e3bb3))
WPC_ROMEND

WPC_ROMSTART(fh,l4,"u6-l4.rom",0x020000,CRC(f438aaca) SHA1(42bf75325a0e85a4334a5a710c2eddf99160ffbf))
WPCS_SOUNDROM222("fh_u18.l2m",CRC(7f6c7045) SHA1(8c8d601e8e6598507d75b4955ccc51623124e8ab),
                 "fh_u15.l2", CRC(0744b9f5) SHA1(b626601d82e6b1cf25f7fdcca31e623fc14a3f92),
                 "fh_u14.l2", CRC(3394b69b) SHA1(34690688f00106b725b27a6975cdbf1e077e3bb3))
WPC_ROMEND

WPC_ROMSTART(fh,l5,"u6-l5.rom",0x020000,CRC(e2b25da4) SHA1(87129e18c60a65035ade2f4766c154d5d333696b))
WPCS_SOUNDROM222("fh_u18.l2m",CRC(7f6c7045) SHA1(8c8d601e8e6598507d75b4955ccc51623124e8ab),
                 "fh_u15.l2", CRC(0744b9f5) SHA1(b626601d82e6b1cf25f7fdcca31e623fc14a3f92),
                 "fh_u14.l2", CRC(3394b69b) SHA1(34690688f00106b725b27a6975cdbf1e077e3bb3))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/

CORE_GAMEDEF(fh,l9,"Funhouse L-9 (SL-2m)",1990,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(fh,l9b,l9,"Funhouse L-9 (SL-2m) Bootleg Improved German translation",1990,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(fh,l3,l9,"Funhouse L-3",1990,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(fh,l4,l9,"Funhouse L-4",1990,"Williams",wpc_mAlpha2S,0)
CORE_CLONEDEF(fh,l5,l9,"Funhouse L-5",1990,"Williams",wpc_mAlpha2S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData fhSimData = {
  2,    		/* 2 game specific input ports */
  fh_stateDef,		/* Definition of all states */
  fh_inportData,	/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 		/* no init */
  fh_handleBallState,	/*Function to handle ball state changes*/
  fh_drawStatic,	/*Function to handle mechanical state changes*/
  TRUE, 		/* simulate manual shooter */
  NULL  		/* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData fhGameData = {
  GEN_WPCALPHA_2, wpc_dispAlpha, /* generation */
  {
    FLIP_SWNO(12,11), 	/* Which switches are the flippers */
    0,0,0,0,0,0,0,
    NULL, fh_handleMech, NULL, fh_drawMech,
    &fh_lampPos, fh_samsolmap
  },
  &fhSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* Dummy Jaw (Col 5, Row 1) & Superdog opto (Col 5, Row 5) = 0x11) */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/* -----------------------------------------------------------------------------------------
   Inverted Switch Values -
   -----------------------------------------------------------------------------------------
   How to handle inverted switches in the game...

   Each Entry represents a Column in the Switch Matrix! (see below)

   Since there are 8 Rows per column, we use an 8-bit # to represent which of the switches
   are inverted. Row 1 = Bit 1 ... Row 8 = Bit 8!

   So, if Column 3 has 2 inverted switches (optos), on rows, 1, 3, and 8, it would work out
   like this:

   Bit       8  7  6  5 4 3 2 1
   ----------------------------
   Value   128 64 32 16 8 4 2 1
   ----------------------------
             1  0  0  0 0 1 0 1 = (128+4+1) = 133 = 0x85

   Therefore, our inverted data array, looks like:
      Coin     1     2     3  .....
    { 0x00, 0x00, 0x00, 0x85, .....

---------------------------------------------------------------------------------------------*/


/*---------------
/  Game handling
/----------------*/
static void init_fh(void) {
  core_gameData = &fhGameData;
  locals.divertercount=0;
}


/***************************************************************************************************
 Functions here must manually adjust the state of mechanicla objects, usually checking for solenoids
 which control the state of those objects. Opening & Closing Doors, Diverter Switchs, etc..
****************************************************************************************************/
static void fh_handleMech(int mech) {
  int eyesleft, eyesright;
  /* ----------------
     Track Diverter
     ---------------*/
  if (mech & 0x01) {
    if (core_getSol(sRampDiv) && locals.diverterPos != OPEN) {
      locals.diverterPos = OPEN;
      wpc_play_sample(0,SAM_DIVERTER);
    }
    else
      locals.divertercount++;
    if (locals.divertercount > 25) {
      locals.divertercount = 0;
      locals.diverterPos = CLOSED;
    }
  }

  /* ----------------------------------
     --	Open & Close Step Gate       --
     ---------------------------------- */
  if (mech & 0x02) {
    /*-- if Step Gate Solenoid fires, keep it open for a timer of 50 seconds! --*/
    if (core_getSol(sStepGate))
      locals.stepgatePos = 50;
    else
      locals.stepgatePos-= (locals.stepgatePos>0)?1:0; /* -- Count down time till it closes --*/
  }
  /* ----------------------------------
     --	Open & Close Trap Door       --
     ---------------------------------- */
  if (mech & 0x04) {
    /*-- if Trap Door Open Solenoid firing, and TD is closed, open it! --*/
    if (core_getSol(sTrapOpen) && !locals.trapdoorPos)
      locals.trapdoorPos = 1 ;
    /*-- if Trap Door Closed Solenoid firing, and TD is open, close it! --*/
    if (core_getSol(sTrapClosed) && locals.trapdoorPos)
      locals.trapdoorPos = 0 ;
    /*-- Make sure Trap Door Closed Switch is on, when door is closed! --*/
    core_setSw(swTrapClosed,!locals.trapdoorPos);
  }

  /* ----------------------------------
     --	Open & Close Rudy's Mouth    --
     ---------------------------------- */
  /* -- If open, and Mouth Solenoid fired and !UpDown fired - Close it! -- */
  if (mech & 0x08) {
    if (locals.rudymouthPos && core_getSol(sMouthMotor) && !core_getSol(sUpDownDriver))
      locals.rudymouthPos = 0;
    /* -- If closed, and Mouth Solenoid fired and UpDown fired - Open it! -- */
    if (!locals.rudymouthPos && core_getSol(sMouthMotor) && core_getSol(sUpDownDriver))
      locals.rudymouthPos = 1;

    /* ----------------------------------
       -- Update status of Rudy's Eyes --
       ---------------------------------- */
    /* -- If Eyes are Open, and Closed Solenoid Fired, close them! --*/
    if (locals.rudyeyesOC == 0 && core_getSol(sEyesClosed))
      locals.rudyeyesOC = 1;
    /* -- If Eyes are Closed, and Open Solenoid Fired, open them! --*/
    if (locals.rudyeyesOC == 1 && core_getSol(sEyesOpen))
      locals.rudyeyesOC = 0;
    /*-- Now check position of Eyes  -- */
    eyesleft = core_getSol(sEyesLeft);
    eyesright = core_getSol(sEyesRight);
    if (eyesleft && !(locals.rudyeyesLR == EYES_LEFT)) {
      locals.rudyeyesLR = EYES_LEFT;
      wpc_play_sample(2,SAM_DIVERTER);
    }
    if (eyesright && !(locals.rudyeyesLR == EYES_RIGHT)) {
      locals.rudyeyesLR = EYES_RIGHT;
      wpc_play_sample(2,SAM_DIVERTER);
    }
    if (!eyesleft && !eyesright)
      locals.rudyeyesLR = EYES_ST;
  }
}

/**********************************
 Display Status of Rudy's Eyes
 *********************************/
static const char* showeyepos(void)
{
if(locals.rudyeyesOC)
	return "Closed";
else
	{
	switch(locals.rudyeyesLR)
		{
		case EYES_ST: return "Straight";
		case EYES_LEFT: return "Left";
		case EYES_RIGHT: return "Right";
		default: return "?";
		}
	}
}
