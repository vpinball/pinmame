/*******************************************************************************
 Party Zone (1991, Bally) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Nov. 15, 2000 (Updated Dec. 5, 2000)

 Known Issues/Problems with this Simulator:

 None.

 Thanks goes to Steve Ellenoff for his lamp matrix designer program.
 VERY Nice work Steve!

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for Party Zone Simulator:
  ------------------------------
    +R  Ramp/Fail Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +O  L/R Outlane
      Q SDTM (Drain Ball)
    WER B-O-P Rollovers (1-3)
   TYUI WOOC Targets (1-4)
      O WOOC Lane
    ASD Jet Bumpers
     FH Request-Time Targets (1-2)
      G Request-Time Kicker Hole
      J SuperSonic Robotic Comic Hole
      K Dancing Dummy Lane
      Z End Zone Target
    XCV Ha-Ha-Ha Targets(1-3)
      B Cottage Shot
  NM,./ Eat-Drink-B.Merry Targets(1-5)

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"

/*------------------
/  Local functions
/-------------------*/
/*-- local state/ball handling functions --*/
static int  pz_handleBallState(sim_tBallStatus *ball, int *inports);
static void pz_handleMech(int mech);
static void pz_drawMech(BMTYPE **line);
static void pz_drawStatic(BMTYPE **line);
static void init_pz(void);
static int pz_getMech(int mechNo);

/*--------------------------
/ Game specific input ports
/---------------------------
In this part you define the keyboard, if the pinball you're trying to simulate
has more switches/possibilities than this, just add a port (remember to add one
to the first number in the simulation definitions section.
You will see that each port contains 15 definitions, from 0x0001 to 0x8000,
those numbers must match with the 0xxxxx's in the Keyboard definitions section.
Remember that for shifted entries you'll have to add 1 or 2 to the original
action, IE if the ramp shot is 0x0004 normal, for shifted action it's 0x0005
for Left Shift and 0x0006 for Right Shift. So you are supposed to do three
different things with the same key.
Don't make any sense to me between WPCPORT_BIT or WPCPORT_BITIMP, so I've used
the first because it is shorter :)
--------------------------------------------------------------------------------------------------*/
WPC_INPUT_PORTS_START(pz,3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"Ozone Ramp",		KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"BOP - B",		KEYCODE_W)
    COREPORT_BIT(0x0100,"BOP - O",		KEYCODE_E)
    COREPORT_BIT(0x0200,"BOP - P",		KEYCODE_R)
    COREPORT_BIT(0x0400,"WOOC - Way",		KEYCODE_T)
    COREPORT_BIT(0x0800,"WOOC - Out",		KEYCODE_Y)
    COREPORT_BIT(0x1000,"WOOC - Of",		KEYCODE_U)
    COREPORT_BIT(0x2000,"WOOC - Control",	KEYCODE_I)
    COREPORT_BIT(0x4000,"End Zone Target",	KEYCODE_Z)
    COREPORT_BIT(0x8000,"WOOC Lane",		KEYCODE_O)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Left Jet Bumper",	KEYCODE_A)
    COREPORT_BIT(0x0002,"Right Jet Bumper",	KEYCODE_S)
    COREPORT_BIT(0x0004,"Bottom Jet Bumper",	KEYCODE_D)
    COREPORT_BIT(0x0008,"Request Time - Request",KEYCODE_F)
    COREPORT_BIT(0x0010,"Request Time - Time",	KEYCODE_H)
    COREPORT_BIT(0x0020,"Request Time - Eject",	KEYCODE_G)
    COREPORT_BIT(0x0040,"EDM Target 1",		KEYCODE_N)
    COREPORT_BIT(0x0100,"EDM Target 2",		KEYCODE_M)
    COREPORT_BIT(0x0200,"EDM Target 3",		KEYCODE_COMMA)
    COREPORT_BIT(0x0400,"EDM Target 4",		KEYCODE_STOP)
    COREPORT_BIT(0x0800,"EDM Target 5",		KEYCODE_SLASH)
    COREPORT_BIT(0x1000,"Drain",			KEYCODE_Q)
    COREPORT_BIT(0x2000,"Comic Hole",		KEYCODE_J)
    COREPORT_BIT(0x4000,"Dancing Dummy Lane",	KEYCODE_K)
    COREPORT_BIT(0x8000,"Cosmic Cottage",	KEYCODE_B)

  PORT_START /* 2 */
    COREPORT_BIT(0x1000,"Ha Target 1",		KEYCODE_X)
    COREPORT_BIT(0x2000,"Ha Target 2",		KEYCODE_C)
    COREPORT_BIT(0x4000,"Ha Target 3",		KEYCODE_V)

WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions (Not used in PZ: 15,24,25,32,33,46,47,48,86,87,88)
For filling this section, you should enter in the "T. Tests" menu and
"T2. Switch Edges" submenu. When you see the matrix, manually trigger all the
possible switches, write down the number and enter it here.
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swHa1        	16
#define swHa2        	17
#define swHa3        	18

#define swSlamTilt   	21
#define swCoinDoor   	22
#define swTicket     	23
#define swBopB		26
#define swBopO		27
#define swBopP		28

#define swBackRamp	31
#define swEDM1		34
#define swEDM2		35
#define swEDM3		36
#define swEDM4		37
#define swEDM5		38

#define swBackPopper	41
#define swRightPopper	42
#define swLeftJet	43
#define swRightJet     	44
#define swBottomJet    	45

#define swHeadOpto1	51
#define swHeadOpto2	52
#define swHeadOpto3	53
#define swLeftInlane	54
#define	swLeftOutlane	55
#define	swEndZone	56
#define	swRightInlane	57
#define	swRightOutlane	58

#define	swShooter	61
#define	swDancerLane	62
#define	swWOOCLane	63
#define	swTopRebound	64
#define	swSkillShot	65
#define	swRequestR	66
#define	swRequestK	67
#define	swRequestT	68

#define	swCottage1	71
#define	swEnterRamp	72
#define	swLeftSling	73
#define	swRightSling	74
#define	swOutHole	75
#define	swTrough1	76
#define	swTrough2	77
#define swTrough3	78

#define	swWOOCWay	81
#define	swWOOCOut	82
#define	swWOOCOf	83
#define	swWOOCControl	84
#define	swCottage2	85

/*---------------------
/ Solenoid definitions (Not used in PZ: 8, 14)
Same as Switch Definitions but instead of going to "T. Tests"/"T2. Switch Edges",
you should go to "T4. Solenoid Test"
/----------------------*/

#define sBackPopper	1
#define sRightPopper	2
#define sDJMouth      	3
#define sDJEject	4
#define sDummy     	5
#define sComicMouth    	6
#define sKnocker       	7
#define sOutHole       	9
#define sBallRel	10
#define sLeftJet	11
#define sRightJet	12
#define sBottomJet	13
#define sLeftSling	15
#define sRightSling	16
#define sHeadOnOff	23
#define sHeadDirection	24

/*---------------------
/  Ball state handling
/----------------------*/

/* ---------------------------------------------------------------------------------
OK, here comes the hard part... the following variables enumerates the ball
states, OK, so take the line 3 as an example, the Cosmic Cottage entrance has
three switches, "Cottage Entrance", "Cottage Switch 2" and ends in the
RightPopper, after that, the ball comes out through a ramp and ends in the
RightInlane. Now go and look forward in the state list array, that looks scary
at the first sight... but it is pretty clear when you understand it...
Go down till you find the comment "Line 3", you will see this:
  {"Cottage Enter",	1,swCottage1,	 0,		stCottage2,	2},
  {"Cottage Sw #2",	1,swCottage2,	 0,		stRightPopper,	5},
  {"Cottage Hole",	1,swRightPopper, sRightPopper,	stHoleExit1,	0},
  {"Hole Exit",		1,0,		 0,		stRightInlane,	15},

OK... what is this? Well here we say first that the name that should be
displayed onplay is "Cottage Enter" and that this switch must be down for about
1ms (ms?), duh, the minimum time down, 0 is don't push it, totally useless.
Now we have swCottage1, "sw" means "press this switch" (it has to be in the
switches definitions), now a zero, this zero means that it doesn't has to wait
for a solenoid to fire the ball, now a stCottage2, "st" means "state", OK, it
should follow the Cottage2 state, and the 2 says that the ball must wait 2ms
or so before going to the next state. In the third line we have this:
      stCottage1, stCottage2, stRightPopper, stHoleExit1,
If you look, the next thing that follows is stCottage2, OK, so back to the
array... what do we have? Ah... display "Cottage Sw #2", be down for 1ms or so,
trigger the Cottage2 switch, 0 solenoids fired, follows stRightPopper in 5ms.
Now we have to enter the "Cottage Hole", be down for at least 1ms, trigger the
RightPopper switch and WAIT till the 's'olenoid "RightPopper" fires then go
instantly to HoleExit1, this HoleExit1 is used as a timing to show that the
ball is coming down through the reel. And it ends on the state "RightInlane"
after 15ms, this state triggers the RightInlane switch and leaves the ball free.
Hope this helps. Steve Ellenoff wrote a description of the array that is below.
   -----------------------------------------------------------------------------------------*/
enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stOutHole, stDrain,
      stShooter, stBallLane, stNotEnough, stSkillShot, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
      stCottage1, stCottage2, stRightPopper, stHoleExit1, stInRRamp,
	  stWOOCLane, stLeftJet, stRightJet, stBottomJet, stTopRebound, stBopO, stCottageDown, stLeftJet2, stBottomJet2, stRightJet2,
	  stEnterRamp, stBackRamp, stBackPopper, stHoleExit2, stInLRamp,
	  stDancerLane, stComicHole, stRequestK,
	  stRampFail1, stRampFail2,
          stEDM1, stEDM2, stEDM3, stEDM4, stEDM5, stWOOCW, stWOOCO, stWOOCOf, stWOOCC, stRequestR, stRequestT, stHa1, stHa2, stHa3, stEndZone,
          stBopB, stBopO2, stBopP
	  };

/********************************************************************************************************
   The following is a list of all possible game states.....
   We start defining our own states at element #3, since 'Not Installed', 'Moving', and 'PF'
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

static sim_tState pz_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",	1,swTrough1,	 sBallRel,	stShooter,	5},
  {"Center Trough",	1,swTrough2,	 0,		stTrough1,	1},
  {"Left Trough",	1,swTrough3,	 0,		stTrough2,	1},
  {"Outhole",		1,swOutHole,	 sOutHole,	stTrough3,	5},
  {"Drain",		1,0,		 0,		stOutHole,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sShooterRel,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		10,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},
  {"Skill Shot",	1,swSkillShot,	 0,		stLeftJet,	5},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},

  /*Line 3*/
  {"Cottage Enter",	1,swCottage1,	 0,		stCottage2,	2},
  {"Cottage Sw #2",	1,swCottage2,	 0,		stRightPopper,	5},
  {"Cottage Hole",	1,swRightPopper, sRightPopper,	stHoleExit1,	0},
  {"Exit From Hole",	1,0,		 0,		stInRRamp,	3},
  {"In Ramp",		1,0,		 0,		stRightInlane,	12},

  /*Line 4*/
  {"WOOC Lane",		1,swWOOCLane,	 0,		stLeftJet,	5},
  {"Left Bumper",	1,swLeftJet,	 0,		stRightJet,	3},
  {"Right Bumper",	1,swRightJet,	 0,		stBottomJet,	3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stTopRebound,	3},
  {"Top Rebound",	1,swTopRebound,	 0,		stFree,		3},
  {"Bop - O",		1,swBopO,	 0,		stLeftJet,	5},
  {"Down F.Cottage",	1,swCottage1,	 0,		stFree,		5},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},

  /*Line 5*/
  {"Enter Ramp",	1,swEnterRamp,	 0,		stBackRamp,	5},
  {"Ozone Switch",	1,swBackRamp,	 0,		stBackPopper,	2},
  {"Ozone Hole",	1,swBackPopper,	 sBackPopper,	stHoleExit2,	0},
  {"Exit From Hole",	1,0,		 0,		stInLRamp,	3},
  {"In Ramp",		1,0,		 0,		stLeftInlane,	15},

  /*Line 6*/
  {"Dancer Lane",	1,swDancerLane,	 0,		stBopO,		10},
  {"Comic Hole",	1,swRightPopper, sRightPopper,	stHoleExit1,	0},
  {"DJ Eject Hole",	1,swRequestK,	 sDJEject,	stFree,		0},

  /*Line 7*/
  {"Enter Ramp",	1,swEnterRamp,	0,		stRampFail2,	10},
  {"Fall Down",		1,swEnterRamp,	0,		stFree,		5},

  /*Line 8*/
  {"EDM Target 1",	1,swEDM1,	0,		stFree,		1},
  {"EDM Target 2",	1,swEDM2,	0,		stFree,		1},
  {"EDM Target 3",	1,swEDM3,	0,		stFree,		1},
  {"EDM Target 4",	1,swEDM4,	0,		stFree,		1},
  {"EDM Target 5",	1,swEDM5,	0,		stFree,		1},
  {"WOOC - Way",	1,swWOOCWay,	0,		stFree,		1},
  {"WOOC - Out",	1,swWOOCOut,	0,		stFree,		1},
  {"WOOC - Of",		1,swWOOCOf,	0,		stFree,		1},
  {"WOOC - Control",	1,swWOOCControl,0,		stFree,		1},
  {"Request Target",	1,swRequestR,	0,		stFree,		1},
  {"Time Target",	1,swRequestT,	0,		stFree,		1},
  {"Top Ha Target",	1,swHa1,	0,		stFree,		1},
  {"Center Ha Tgt",	1,swHa2,	0,		stFree,		1},
  {"Bottom Ha Tgt",	1,swHa3,	0,		stFree,		1},
  {"End Zone Tgt",	1,swEndZone,	0,		stFree,		1},

  /*Line 9*/
  {"BOP - B",		1,swBopB,	0,		stFree,		2},
  {"BOP - O",		1,swBopO,	0,		stFree,		2},
  {"BOP - P",		1,swBopP,	0,		stFree,		2},

  {0}
};

static int pz_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

        /* This is pretty straightforward */
	/* Ball in Shooter Lane */
        /* Note: Sim supports max of 50 speed for manual plunger */
    	case stBallLane:
		if (ball->speed < 20)
			return setState(stNotEnough,20);	/*Ball not plunged hard enough*/
		if (ball->speed < 30)
			return setState(stCottageDown,30);	/*Ball rolled down Cottage Entrance*/
		if (ball->speed < 40)
			return setState(stSkillShot,40);	/*Ball rolled down Skill Shot Lane*/
		if (ball->speed < 51)
			return setState(stBopO,51);		/*Ball rolled down O letter in BOP*/
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

static sim_tInportData pz_inportData[] = {

/* Port 0 */
  {0, 0x0005, stRampFail1},
  {0, 0x0006, stEnterRamp},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stBopB},
  {0, 0x0100, stBopO2},
  {0, 0x0200, stBopP},
  {0, 0x0400, stWOOCW},
  {0, 0x0800, stWOOCO},
  {0, 0x1000, stWOOCOf},
  {0, 0x2000, stWOOCC},
  {0, 0x4000, stEndZone},
  {0, 0x8000, stWOOCLane},

/* Port 1 */
  {1, 0x0001, stLeftJet2},
  {1, 0x0002, stBottomJet2},
  {1, 0x0004, stRightJet2},
  {1, 0x0008, stRequestR},
  {1, 0x0010, stRequestT},
  {1, 0x0020, stRequestK},
  {1, 0x0040, stEDM1},
  {1, 0x0100, stEDM2},
  {1, 0x0200, stEDM3},
  {1, 0x0400, stEDM4},
  {1, 0x0800, stEDM5},
  {1, 0x1000, stDrain},
  {1, 0x2000, stComicHole},
  {1, 0x4000, stDancerLane},
  {1, 0x8000, stCottage1},

/* Port 2 */
  {2, 0x1000, stHa1},
  {2, 0x2000, stHa2},
  {2, 0x4000, stHa3},
  {0}
};

/*-----------
     Lamps
  ----------- */
static core_tLampDisplay pz_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
{1,{{14, 7,ORANGE}}},{1,{{16, 5,YELLOW}}},{1,{{38,14,YELLOW}}},{1,{{14, 4,GREEN}}},
{1,{{35, 9,WHITE}}},{1,{{ 5,22,WHITE}}},{1,{{ 7,23,WHITE}}},{1,{{ 9,24,WHITE}}},
{1,{{21,26,ORANGE}}},{1,{{23,24,WHITE}}},{1,{{25,22,GREEN}}},{1,{{27,20,RED}}},
{1,{{ 3,14,WHITE}}},{1,{{ 1,10,WHITE}}},{1,{{ 1,14,WHITE}}},{1,{{ 1,18,WHITE}}},
{1,{{12,25,YELLOW}}},{1,{{14,23,YELLOW}}},{1,{{16,21,YELLOW}}},{1,{{17, 2,GREEN}}},
{1,{{19, 2,GREEN}}},{1,{{21, 2,GREEN}}},{1,{{23, 2,GREEN}}},{1,{{25, 2,GREEN}}},
{1,{{19, 9,WHITE}}},{1,{{36,16,RED}}},{1,{{34,15,RED}}},{1,{{32,14,RED}}},
{1,{{30,13,RED}}},{1,{{28,12,RED}}},{1,{{26,11,RED}}},{1,{{24,10,RED}}},
{1,{{ 5, 7,YELLOW}}},{1,{{ 9, 9,RED}}},{1,{{11,10,WHITE}}},{1,{{13,11,WHITE}}},
{1,{{18,21,ORANGE}}},{1,{{19,19,GREEN}}},{1,{{20,21,YELLOW}}},{1,{{21,19,LBLUE}}},
{1,{{15,12,YELLOW}}},{1,{{15,16,YELLOW}}},{1,{{17,14,YELLOW}}},{1,{{19,14,GREEN}}},
{1,{{32,18,WHITE}}},{1,{{30,17,WHITE}}},{1,{{28,16,WHITE}}},{1,{{26,15,WHITE}}},
{1,{{ 9,19,RED}}},{1,{{11,18,WHITE}}},{1,{{13,17,WHITE}}},{1,{{ 3,21,GREEN}}},
{1,{{ 5,12,YELLOW}}},{1,{{ 5,16,YELLOW}}},{1,{{10,14,YELLOW}}},{1,{{39, 0,YELLOW}}},
{1,{{30, 1,RED}}},{1,{{30,28,RED}}},{1,{{ 0,27,RED}}},{1,{{ 1,25,YELLOW}}},
{1,{{ 1,29,YELLOW}}},{1,{{15,14,RED}}}
}
};

/*--------------------------------------------------------
  Code to draw the mechanical objects, and their states!
This part is pretty easy, core_getSol tells the emulator the state of a Solenoid,
you can use getSw too if you like to know the state of a switch.
---------------------------------------------------------*/
  static void pz_drawMech(BMTYPE **line) {
  core_textOutf(30,  0,BLACK,"Dancin'Dummy: %-6s", core_getSol(sDummy) ? "Down":"Up");
  core_textOutf(30, 10,BLACK,"Comic Mouth : %-6s", core_getSol(sComicMouth) ? "Open" : "Closed");
  core_textOutf(30, 20,BLACK,"B.Zarr Mouth: %-6s", core_getSol(sDJMouth) ? "Open":"Closed");
}
/* I've added help because I was tired of having to print/exit PINMAME to have
all the keys handy :) */

  static void pz_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+R = Ramp Shot (Fail/OK)");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W/E/R = B/O/P Rollover");
  core_textOutf(30,100,BLACK,"A/S/D = Jet Bumpers, K = Dummy Lane");
  core_textOutf(30,110,BLACK,"G = DJ Eject, J = Supersonic R. Comic");
  core_textOutf(30,120,BLACK,"Z = End Zone Tgt, X/C/V = HaHaHa Tgts");
  core_textOutf(30,130,BLACK,"B = Cosmic Cottage, O = WOOC Lane");
  core_textOutf(30,140,BLACK,"F/H = Request/Time Targets");
  core_textOutf(30,150,BLACK,"T/Y/U/I = Way/Out/Of/Control Targets");
  core_textOutf(30,160,BLACK,"N/M/,/.// = Eat-Drink-B.Merry Targets");
}

/* Solenoid-to-sample handling */
static wpc_tSamSolMap pz_samsolmap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sBallRel,0,SAM_BALLREL},
 {sOutHole,0,SAM_OUTHOLE},

 /*Channel #1*/
 {sLeftJet,1,SAM_JET1}, {sRightJet,1,SAM_JET2},
 {sBottomJet,1,SAM_JET3}, {sHeadOnOff,1,SAM_MOTOR_1,WPCSAM_F_CONT},

 /*Channel #2*/
 {sBackPopper,2,SAM_POPPER},  {sLeftSling,2,SAM_LSLING},
 {sRightSling,2,SAM_RSLING},  {sDummy,2,SAM_SOLENOID_ON},
 {sDummy,2,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #3*/
 {sDJEject,3,SAM_SOLENOID_ON}, {sComicMouth,3,SAM_SOLENOID_ON},
 {sComicMouth,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF}, {sRightPopper,3,SAM_POPPER},

 /*Channel #4*/
 {sDJMouth,4,SAM_SOLENOID_ON}, {sDJMouth,4,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},{-1}
};

/*-----------------
/  ROM definitions
/------------------*/
/* (Name,Revision,"FileName.Rom",Size,CRC (0 means that No Good Dump Exist)) */
WPC_ROMSTART(pz,f4,"pzonef_4.rom",0x40000,CRC(041d7d15) SHA1(d40e7010caa3bc664dc985c748309fe84ae17dac))
/* ("FileName.Rom",CRC (0 means that No Good Dump Exist)) */
WPCS_SOUNDROM224("pz_u18.l1",CRC(b7fbba98) SHA1(6533a1474dd335419331d37d4a4447951171412b),
                 "pz_u15.l1",CRC(168bcc52) SHA1(0bae89278cd24950b2e247bba48eaa636f7b566c),
                 "pz_u14.l1",CRC(4d8897ce) SHA1(7a4ac9e849dae93078ddd60adbd34f3930e4cd46))
WPC_ROMEND

WPC_ROMSTART(pz,l1,"u6-l1.rom",0x40000,CRC(48023444) SHA1(0c14f5902c6c0b3466fb4265a2e1fc6a1050f8d7))
WPCS_SOUNDROM224("pz_u18.l1",CRC(b7fbba98) SHA1(6533a1474dd335419331d37d4a4447951171412b),
                 "pz_u15.l1",CRC(168bcc52) SHA1(0bae89278cd24950b2e247bba48eaa636f7b566c),
                 "pz_u14.l1",CRC(4d8897ce) SHA1(7a4ac9e849dae93078ddd60adbd34f3930e4cd46))
WPC_ROMEND

WPC_ROMSTART(pz,l2,"pz_u6.l2",0x40000,CRC(200455a9) SHA1(d0f9a2227c67ddc73111a120a6a19dc5ac218baa))
WPCS_SOUNDROM224("pz_u18.l1",CRC(b7fbba98) SHA1(6533a1474dd335419331d37d4a4447951171412b),
                 "pz_u15.l1",CRC(168bcc52) SHA1(0bae89278cd24950b2e247bba48eaa636f7b566c),
                 "pz_u14.l1",CRC(4d8897ce) SHA1(7a4ac9e849dae93078ddd60adbd34f3930e4cd46))
WPC_ROMEND

WPC_ROMSTART(pz,l3,"pzonel_3.rom",0x40000,CRC(156f158f) SHA1(73a31deee6b299e5f5479b43210a822009e116d0))
WPCS_SOUNDROM224("pz_u18.l1",CRC(b7fbba98) SHA1(6533a1474dd335419331d37d4a4447951171412b),
                 "pz_u15.l1",CRC(168bcc52) SHA1(0bae89278cd24950b2e247bba48eaa636f7b566c),
                 "pz_u14.l1",CRC(4d8897ce) SHA1(7a4ac9e849dae93078ddd60adbd34f3930e4cd46))
WPC_ROMEND



/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(pz,f4,"Party Zone (F-4)",1991,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(pz,l1,f4,"Party Zone (L-1)",1991,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(pz,l2,f4,"Party Zone (L-2)",1991,"Bally",wpc_mFliptronS,0)
CORE_CLONEDEF(pz,l3,f4,"Party Zone (L-3)",1991,"Bally",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData pzSimData = {
  3,    				/* 3 game specific input ports */
  pz_stateDef,				/* Definition of all states */
  pz_inportData,			/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  pz_handleBallState,	/*Function to handle ball state changes*/
  pz_drawStatic,			/*Function to handle mechanical state changes*/
  TRUE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData pzGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  {
    FLIP_SWNO(12,11) | FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    0,0,0,0,0,0,0,
    NULL, pz_handleMech, pz_getMech, pz_drawMech,
    &pz_lampPos, pz_samsolmap
  },
  &pzSimData,
  {
    {0}, /* sNO */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0},
  }
};

/*---------------
   Game handling
 ----------------*/
static void init_pz(void) {
  core_gameData = &pzGameData;
}

static int trick=0;

static void pz_handleMech(int mech) {
  /* ---------------------------------------
     --	 Handle the Moving Head (Tricky)  --
     WPCMAME: All this ++ stuff doesn't seem right but I don't want to change it
     --------------------------------------- */
  if (mech & 0x01) {
    /* Move to the Left */
    if (core_getSol(sHeadOnOff) && core_getSol(sHeadDirection)) {
      core_setSw(swHeadOpto1,1); core_setSw(swHeadOpto2,1); core_setSw(swHeadOpto3,1); /* 111 */
      if (trick++ > 20)
        core_setSw(swHeadOpto2,0); /* 101 */
      if (trick++ > 40)
        core_setSw(swHeadOpto3,0); /* 100 */
      if (trick++ > 60)
        core_setSw(swHeadOpto1,0); /* 000 */
      if (trick++ > 80)
        core_setSw(swHeadOpto3,1); /* 001 */
      if (trick++ > 100)
        core_setSw(swHeadOpto2,1); /* 011 */
      if (trick++ > 120)
        core_setSw(swHeadOpto3,0); /* 010 */
    }
    if (trick++>120)
      trick = 0;
    /* Move to the Right */
    if (core_getSol(sHeadOnOff) && !core_getSol(sHeadDirection)) {
      core_setSw(swHeadOpto1,0);core_setSw(swHeadOpto2,1);core_setSw(swHeadOpto3,0); /* 010 */
      if (trick++>20)
        core_setSw(swHeadOpto3,1); /* 011 */
      if (trick++>40)
        core_setSw(swHeadOpto2,0); /* 001 */
      if (trick++>60)
        core_setSw(swHeadOpto3,0); /* 000 */
      if (trick++>80)
        core_setSw(swHeadOpto1,1); /* 100 */
      if (trick++>100)
        core_setSw(swHeadOpto3,1); /* 101 */
      if (trick++>120)
        core_setSw(swHeadOpto2,1);
      core_setSw(swHeadOpto3,0); /* 110 */
    }
    if (trick++>120)
      trick = 0;
  }
}

static int pz_getMech(int mechNo) {
  return trick;
}
