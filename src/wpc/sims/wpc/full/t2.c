/********************************************************************************************
 Williams Terminator 2: Judgement Day (1991) Pinball Simulator

 by Steve Ellenoff (sellenoff@hotmail.com)
 Nov.2, 2000

 Known Issues/Problems with Simulator:
 -Gun Position & Timing Totally guessed..
	but seems ok!! (Sure is hard aiming the gun when you can't see it!)
 -Skull Target does not open when it should,
    so a hack was used... For some reason, during
    game play, the knockdown solenoid *should* fire when the Drop Target is activated but it does
    not.. So I had to put in an extra check for the switch being active to pull down the drop target
    rather than just relying on the knockdown solenoid!

 Notes:

  This game was incredibly easy to simulate. There's very little stuff going on.. No diverters,
  nothing fancy.. The only real pain was the gun. That took 80% of the time to figure out. The
  rest of the stuff are straight on shots which simply trigger switches. The ramps are straightforward!


 Changes:

  12-05-2000: Modified custom playfield lamp layout to accomodate new structure.
              Added support for shared bulbs where necessary
  12-30-2000: Added Sample Support

 *********************************************************************************************************/


/*-------------------------------------------------------------------------------------------------
  Keys for Terminator 2 Simulator:
  ----------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +L  L/R Loop
    +O  L/R Outlane
    WER Kickback Drop Targets(1-3)
      T Skull Drop Target
      Q SDTM (Drain Ball)
    HJK Jet Bumpers
    XCV Middle Drop Targets(1-3)
  NM,./ Left Drop Targets(1-5)
    ASD Multiplier Rollovers(1-3)
	  Z Left Lock

-------------------------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "wpcsam.h"
#include "mech.h"
/*------------------
/  Local functions
/-------------------*/
/*-- local state/ball handling functions --*/
static int  t2_handleBallState(sim_tBallStatus *ball, int *inports);
static void t2_handleMech(int mech);
static void t2_drawMech(BMTYPE **line);
static void t2_drawStatic(BMTYPE **line);
static int t2_getMech(int mechNo);
static void init_t2(void);
static int GetGunPos(void);
#if 0
static char * GetGunDescr(void);
#endif

/*-----------------------
  local static variables
 ------------------------
   I used this locals structure for setting up variables to track the state of mechanical objects
   which needed to be displayed on screen
--------------------------------------------------------------------------------------------------*/
static struct {
  int droptargetPos;    /* Skull Drop Target Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------
   Here we define for PINMAME which Keyboard Presses it should look for, and process!
--------------------------------------------------------------------------------------------------*/
WPC_INPUT_PORTS_START(t2,3)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"L/R Inlane",      KEYCODE_I)
    COREPORT_BITIMP(0x0100,"Kickb Drop Targ. 1",     KEYCODE_W)
    COREPORT_BITIMP(0x0200,"Kickb Drop Targ. 2",     KEYCODE_E)
    COREPORT_BITIMP(0x0400,"Kickb Drop Targ. 3",     KEYCODE_R)
    COREPORT_BITIMP(0x0800,"Skull Drop Target",	KEYCODE_T)
    COREPORT_BIT(   0x1000,"Left Lock",			KEYCODE_Z)
    COREPORT_BIT(   0x2000,"Jet 1",				KEYCODE_H)
    COREPORT_BITIMP(0x4000,"Jet 2",				KEYCODE_J)
    COREPORT_BIT(   0x8000,"Jet 3",				KEYCODE_K)

  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"Mid. Drop Targ. 1",	KEYCODE_X)
    COREPORT_BIT(   0x0002,"Mid. Drop Targ. 2",	KEYCODE_C)
    COREPORT_BIT(   0x0004,"Mid. Drop Targ. 3",	KEYCODE_V)
    COREPORT_BIT(   0x0008,"Rollover 1'",        KEYCODE_A)
    COREPORT_BIT(   0x0010,"Rollover 2",			KEYCODE_S)
    COREPORT_BIT(   0x0020,"Rollover 3",			KEYCODE_D)
    COREPORT_BIT(   0x0040,"L. Drop Targ. 1",	KEYCODE_N)
    COREPORT_BIT(   0x0100,"L. Drop Targ. 2",	KEYCODE_M)
    COREPORT_BIT(   0x0200,"L. Drop Targ. 3",	KEYCODE_COMMA)
    COREPORT_BIT(   0x0400,"L. Drop Targ. 4",	KEYCODE_STOP)
    COREPORT_BIT(   0x0800,"L. Drop Targ. 5",	KEYCODE_SLASH)
    COREPORT_BIT(   0x1000,"Drain",				KEYCODE_Q)
//	COREPORT_BITIMP(0x2000,"Home",				KEYCODE_F)
//	COREPORT_BIT(   0x4000,"Mark",				KEYCODE_G)
//	COREPORT_BIT(   0x8000,"Jet Bumper Lane", KEYCODE_Y)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      	13
#define swTilt       	14
#define swLTrough    	15
#define swCTrough    	16
#define swRTrough    	17
#define swOuthole    	18

#define swSlamTilt   	21
#define swCoinDoor   	22
#define swTicket     	23
#define swNotUsed		24
#define swLOut			25
#define swLIn			26
#define swRIn			27
#define swROut			28

#define swGunLoaded		31
#define swGunMark		32
#define swGunHome		33
#define swTrigger		34
/*35 Not Used*/
#define swMidLeft		36
#define swMidCenter		37
#define swMidRight		38

#define swJet1			41
#define swJet2			42
#define swJet3			43
#define swLSling     	44
#define swRSling     	45
#define swTopRDT     	46
#define swMidRDT     	47
#define swBotRDT     	48

#define swLockL			51
/*52 Not Used*/
#define swEscapeL		53
#define swEscapeH		54
#define	swLockT			55
#define	swMultL			56
#define	swMultC			57
#define	swMultR			58

#define	swLRampEnt		61
#define	swLRampExit		62
#define	swRRampEnt		63
#define	swRRampExit		64
#define	swLChaseLoop	65
#define	swHChaseLoop	66
/*67 Not Used*/
/*68 Not Used*/

#define	swTarget1		71
#define	swTarget2		72
#define	swTarget3		73
#define	swTarget4		74
#define	swTarget5		75
#define	swBallPopper	76
#define	swDropTarget	77
#define swShooter  		78

/*81-88 Not Used*/

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sBallPopper		1
#define sGunKicker		2
#define sOuthole      	3
#define sBallRel		4
#define sRSling      	5
#define sLSling      	6
#define sKnocker       	7
#define sKickBack      	8
#define sPlunger       	9
#define sTopLock       10
#define sGunMotor      11
#define sKnockDown	   12
#define sJet1		   13
#define sJet2          14
#define sJet3          15
#define sLeftLock      16
#define sDropTarget	   28

#define DT_UP		0
#define DT_DOWN		1

/*Track What gun is pointing at*/
enum {GUNPOS_PF, GUNPOS_T1,GUNPOS_T2,GUNPOS_T3,GUNPOS_T4,GUNPOS_T5};

/*---------------------
/  Ball state handling
/----------------------*/

/* ---------------------------------------------------------------------------------
   The following variables are used to refer to the state array!
   These vars *must* be in the *exact* *same* *order* as each stateDef array entry!
   -----------------------------------------------------------------------------------------*/
enum {stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOuthole, stDrain,
      stShooter, stBallLane, stROut, stLOut,stRIn, stLIn,
      stLLoopUp, stLLoopUp2, stTopLock, stLLoopDn, stLLoopDn2,
	  stRLoopUp, stRLoopUp2, stRLoopDn, stRLoopDn2,
	  stLRampEnt, stLRamp, stLRampExit, stRRampEnt, stRRamp, stRRampExit,
	  stLockLeft, stDropTarget, stDropTarget2, stBallPopper, stGunLoaded, stGunFire,
	  stTarget1, stTarget2, stTarget3, stTarget4, stTarget5, stMultL, stJet1, stJet2, stJet3
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

static sim_tState t2_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",               0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",     1,swRTrough,   sBallRel, stShooter,  5},
  {"Center Trough",    1,swCTrough,   0,        stRTrough,   1},
  {"Left Trough",      1,swLTrough,   0,        stCTrough,   1},
  {"Outhole",          5,swOuthole,   sOuthole, stLTrough,   5},
  {"Drain",            1,0,           0,        stOuthole,   0,0,0,SIM_STNOTEXCL},
  /*Line 2*/
  {"Shooter",		   1,swShooter,   sPlunger, stBallLane,  1,0,0,SIM_STNOTEXCL},
  {"Ball Lane",		   1, 0,  	      0,        stTarget2,   5,0,0,SIM_STNOTEXCL},
  {"Right Outlane",    1,swROut,      0,        stDrain,    20},
  {"Left Outlane",     1,swLOut,      0,        stDrain,    20, sKickBack, stFree},
  {"Right Inlane",     1,swRIn,       0,        stFree,      5},
  {"Left  Inlane",     1,swLIn,       0,        stFree,      5},
  /*Line 3*/
  {"Left Loop",        1,swEscapeL,    0,       stLLoopUp2,   1},	/* Left Loop Going Up     */
  {"Left Loop",        1,swEscapeH,    0,       stTopLock,   15},   /* Left Loop Going Up #2  */
  {"Top Lock",         5,swLockT,      sTopLock,stMultL,     25},
  {"Left Loop",        1,swEscapeL,    0,       stFree,       1},	/* Left Loop Coming Down */
  {"Left Loop",        1,swEscapeH,    0,       stLLoopDn,    1},   /* Left Loop Coming Down2  */
  /*Line 4*/
  {"Right Loop",       1,swLChaseLoop, 0,       stRLoopUp2,   1},	/* Right Loop Going Up     */
  {"Right Loop",       1,swHChaseLoop, 0,       stLLoopDn2,  10},	/* Right Loop Going Up #2  */
  {"Right Loop",       1,swLChaseLoop, 0,       stFree,       1},	/* Right Loop Coming Down  */
  {"Right Loop",       1,swHChaseLoop, 0,       stRLoopDn,   10},	/* Right Loop Coming Down2 */
  /*Line 5*/
  {"L. Ramp Enter",	   1,swLRampEnt,   0,       stLRamp,	 5},
  {"L. Ramp",		   1,swLRampExit,  0,       stLRampExit,15},	/*Here strickly to fix timing issues*/
  {"L. Ramp Exit",     1,0,        0,           stLIn,       2},
  {"R. Ramp Enter",	   1,swRRampEnt,   0,       stRRamp,	 5},
  {"R. Ramp",		   1,swRRampExit,  0,       stRRampExit,15},	/*Here strickly to fix timing issues*/
  {"R. Ramp Exit",     1,0,		   0,           stRIn,       2},
  /*Line 6*/
  {"Left Lock",        1,swLockL,  sLeftLock,stFree,      10},
  {"Skull Drop Target",1,0,  0,  0,  0},
  {"Skull Drop Target",1,swDropTarget, 0, stFree, 1, 0,0, SIM_STSWKEEP},
  {"Ball Popper",	   5,swBallPopper, sBallPopper, stGunLoaded, 5},
  {"Gun Loaded" ,	   1,swGunLoaded,  sGunKicker, stGunFire, 1},
  {"Gun Fired",		   1,0,0,0,0},
  /*Line 7*/
  {"Orbit Target 1",   1,swTarget1, 0, stFree, 1},
  {"Orbit Target 2",   1,swTarget2, 0, stFree, 1},
  {"Orbit Target 3",   1,swTarget3, 0, stFree, 1},
  {"Orbit Target 4",   1,swTarget4, 0, stFree, 1},
  {"Orbit Target 5",   1,swTarget5, 0, stFree, 1},
  {"Left Rollover",    1,swMultL,   0, stJet1, 10},
  {"Jet Bumper 1",       1,swJet1,    0, stJet2, 2},
  {"Jet Bumper 2",       1,swJet2,    0, stJet3, 2},
  {"Jet Bumper 3",       1,swJet3,    0, stFree, 3},
  {0}
};

static int t2_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
		/*-----------------
		  Skull Drop Target
		-------------------*/
		case stDropTarget:
		/* If Drop Target Is Down.. Shoot to Ball Popper*/
		/* Otherwise simply trigger Drop Target Switch  */
		if (locals.droptargetPos==DT_DOWN)
			return setState(stBallPopper,5);
		else
			return setState(stDropTarget2,5);
		break;

		/* Gun Fired   */
		/* - Gun Hits Targets based on Position
			 Position (Mark -> Mark + 14)  = No Target Hit!
					  (Mark+15  ->  Mark+29)  = Target 1
					  (Mark+30  ->  Mark+44)  = Target 2
					  (Mark+45  ->  Mark+59)  = Target 3
					  (Mark+60  ->  Mark+74)  = Target 4
					  (Mark+75  ->  Mark+89)  = Target 5
					  (Mark+90  ->  Mark+104) = No Target Hit!
		*/

		case stGunFire:
                        {
                        int gp=GetGunPos();
                        if(gp==GUNPOS_PF)
                           return setState(stFree,2);
                        if(gp==GUNPOS_T1)
                           return setState(stTarget1,2);
                        if(gp==GUNPOS_T2)
                           return setState(stTarget2,2);
                        if(gp==GUNPOS_T3)
                           return setState(stTarget3,2);
                        if(gp==GUNPOS_T4)
                           return setState(stTarget4,2);
                        if(gp==GUNPOS_T5)
                           return setState(stTarget5,2);
                        return setState(stFree,2);
                        break;
                        }
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

static sim_tInportData t2_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0100, swTopRDT},
  {0, 0x0200, swMidRDT},
  {0, 0x0400, swBotRDT},
  {0, 0x0800, stDropTarget},
  {0, 0x1000, stLockLeft},
  {0, 0x2000, swJet1},
  {0, 0x4000, swJet2},
  {0, 0x8000, swJet3},
  {1, 0x0001, swMidLeft},
  {1, 0x0002, swMidCenter},
  {1, 0x0004, swMidRight},
  {1, 0x0008, swMultL},
  {1, 0x0010, swMultC},
  {1, 0x0020, swMultR},
  {1, 0x0040, swTarget1},
  {1, 0x0100, swTarget2},
  {1, 0x0200, swTarget3},
  {1, 0x0400, swTarget4},
  {1, 0x0800, swTarget5},
  {1, 0x1000, stDrain},
#if 0
  {1, 0x2000, swGunHome},
  {1, 0x4000, swGunMark},
#endif
  {0}
};

/*--------------------
  Drawing information
 ---------------------*/
/*--------------------
/ Drawing information
/---------------------*/
static core_tLampDisplay t2_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{	/*Start of lamps array*/
 {1,{{31, 8,GREEN}}},{1,{{30,10,GREEN}}},{1,{{30,12,ORANGE}}},
 {1,{{30,14,GREEN}}},{1,{{31,16,GREEN}}},{1,{{36,12,YELLOW}}},

 /*Lamp 07 - Matrix # 17 - Splits into two bulbs*/
 {2,{{28,12,WHITE},{28,12,WHITE}}},

 {1,{{39, 3,BLACK}}},{1,{{32, 2,ORANGE}}},

 /*Lamp 10 - Matrix # 22 - Splits into two bulbs*/
 {2,{{30, 2,RED},{30,23,RED}}},

 {1,{{29, 4,WHITE}}},{1,{{29,21,ORANGE}}},
 {1,{{15,10,YELLOW}}},{1,{{13, 9,GREEN}}},{1,{{11, 8,ORANGE}}},
 {1,{{ 9, 7,LPURPLE}}},{1,{{13, 5,YELLOW}}},{1,{{14, 6,YELLOW}}},
 {1,{{15, 7,YELLOW}}},{1,{{16, 8,YELLOW}}},{1,{{17, 9,YELLOW}}},
 {1,{{14,12,RED}}},{1,{{15,13,RED}}},{1,{{16,14,RED}}},
 {1,{{14, 3,GREEN}}},{1,{{15, 4,ORANGE}}},{1,{{17, 4,RED}}},
 {1,{{19, 5,RED}}},{1,{{21, 6,RED}}},{1,{{23, 7,RED}}},
 {1,{{25, 8,RED}}},{1,{{27, 9,RED}}},

 /*Lamp 33 - Matrix # 51 - Splits into two bulbs*/
 {2,{{26,11,RED},{26,13,RED}}},
 /*Lamp 34 - Matrix # 52 - Splits into two bulbs*/
 {2,{{ 4, 5,RED},{ 4, 7,RED}}},

 {1,{{17,21,RED}}},{1,{{19,20,RED}}},
 {1,{{21,19,RED}}},{1,{{23,18,RED}}},{1,{{25,17,RED}}},
 {1,{{27,16,RED}}},{1,{{19, 8,ORANGE}}},{1,{{20, 9,ORANGE}}},
 {1,{{21,10,ORANGE}}},{1,{{22,11,ORANGE}}},{1,{{23,12,ORANGE}}},
 {1,{{23, 3,GREEN}}},{1,{{24, 4,ORANGE}}},{1,{{11, 6,RED}}},
 {1,{{19,18,ORANGE}}},{1,{{20,17,ORANGE}}},{1,{{21,16,ORANGE}}},
 {1,{{22,15,ORANGE}}},{1,{{23,14,ORANGE}}},{1,{{21,21,YELLOW}}},
 {1,{{23,21,YELLOW}}},{1,{{25,21,YELLOW}}},{1,{{14,23,RED}}},
 {1,{{17,19,RED}}},{1,{{15,22,WHITE}}},{1,{{39, 1,YELLOW}}},
 {1,{{ 7, 6,WHITE}}},{1,{{ 2,13,GREEN}}},{1,{{ 2,15,GREEN}}},
 {1,{{ 2,17,GREEN}}}
}	/*End of lamps array*/
};

static wpc_tSamSolMap t2_SamSolMap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sBallRel,0,SAM_BALLREL},
 {sOuthole,0,SAM_OUTHOLE}, {sBallPopper,0,SAM_POPPER},

 /*Channel #1*/
 {sLSling,1,SAM_LSLING}, {sRSling,1,SAM_RSLING},
 {sJet1,1,SAM_JET1}, {sJet2,1,SAM_JET2},
 {sJet3,1,SAM_JET3},

 /*Channel #2*/
 {sGunKicker,2,SAM_SOLENOID}, {sKickBack,2,SAM_SOLENOID},
 {sPlunger,2,SAM_SOLENOID}, {sTopLock,2,SAM_SOLENOID},

 /*Channel #3*/
 {sLeftLock,3,SAM_SOLENOID}, {sKnockDown,3,SAM_FLAPCLOSE},
 {sDropTarget,3,SAM_SOLENOID},
 {-1}
};

/*--------------------------------------------------------
  Code to draw the mechanical objects, and their states!
---------------------------------------------------------*/

  /* Help */
static void t2_drawStatic(BMTYPE **line) {

  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball");
  core_textOutf(30,100,BLACK,"W/E/R = Kickback Targets");
  core_textOutf(30,110,BLACK,"T = Skull Drop Target");
  core_textOutf(30,120,BLACK,"H/J/K = Jet Bumpers");
  core_textOutf(30,130,BLACK,"X/C/V = Middle Drop Targets");
  core_textOutf(30,140,BLACK,"A/S/D = Multiplier Rollovers");
  core_textOutf(30,150,BLACK,"N/M/,/.// = Left Drop Targets");
  core_textOutf(30,160,BLACK,"Z = Left Lock");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(t2,l8,"t2_l8.rom",0x80000,CRC(c00e52e9) SHA1(830c1a7eabf3c8e4fa6242421587b398e21449e8))
WPCS_SOUNDROM222("t2_u18.l3",CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                 "t2_u15.l3",CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                 "t2_u14.l3",CRC(9addc9dc) SHA1(847bb027f6b9167cbbaa13f1af50d61e0c69f01f))
WPC_ROMEND

WPC_ROMSTART(t2,l6,"t2_l6.u6",0x40000,CRC(0d714b35) SHA1(050fd2b3afbecbbd03d58ab206ff6cfac8780a2b))
WPCS_SOUNDROM222("t2_u18.l3",CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                 "t2_u15.l3",CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                 "t2_u14.l3",CRC(9addc9dc) SHA1(847bb027f6b9167cbbaa13f1af50d61e0c69f01f))
WPC_ROMEND

/* Profanity speech ROM. Don't know if the sound rom works with L8 */
WPC_ROMSTART(t2,p2f,"u6-nasty.rom",0x40000,CRC(add685a4) SHA1(d1ee7eb620864b017495e52ea8fe8db18508c3eb))
WPCS_SOUNDROM222(   "t2_u18.l3",   CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                    "t2_u15.l3",   CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                    "u14-nsty.rom",CRC(b4d64152) SHA1(03a828cef8b067d4da058fd3a1e972265a72f10a))
WPC_ROMEND

WPC_ROMSTART(t2,l4,"u6-l4.rom",0x40000,CRC(4d8b894d) SHA1(218b3628e7709c329c2030a5391ded60301aad26))
WPCS_SOUNDROM222("t2_u18.l3",CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                 "t2_u15.l3",CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                 "t2_u14.l3",CRC(9addc9dc) SHA1(847bb027f6b9167cbbaa13f1af50d61e0c69f01f))
WPC_ROMEND
WPC_ROMSTART(t2,l3,"u6-l3.rom",0x40000,CRC(7520398a) SHA1(862881481dc7b617f3b14bbb35d48cffb0ce950e))
WPCS_SOUNDROM222("t2_u18.l3",CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                 "t2_u15.l3",CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                 "t2_u14.l3",CRC(9addc9dc) SHA1(847bb027f6b9167cbbaa13f1af50d61e0c69f01f))
WPC_ROMEND
WPC_ROMSTART(t2,l2,"u6-l2.rom",0x40000,CRC(efe49c18) SHA1(9f91081c384990eac6e3c57f318a2639626929f9))
WPCS_SOUNDROM222("t2_u18.l3",CRC(2280bdd0) SHA1(ea94265cb8291ee427e0a2119d901ba1eb50d8ee),
                 "t2_u15.l3",CRC(dad03ad1) SHA1(7c200f9a6564d751e5aa9b1ba84363b221502770),
                 "t2_u14.l3",CRC(9addc9dc) SHA1(847bb027f6b9167cbbaa13f1af50d61e0c69f01f))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(t2,l8,"Terminator 2: Judgement Day (L-8)",1991,"Williams",wpc_mDMDS,0)
CORE_CLONEDEF(t2,l6,l8,"Terminator 2: Judgement Day (L-6)",1991,"Williams",wpc_mDMDS,0)
CORE_CLONEDEF(t2,p2f,l8,"Terminator 2: Judgement Day (P-2F) Profanity",1991,"Williams",wpc_mDMDS,0)
CORE_CLONEDEF(t2,l4,l8,"Terminator 2: Judgement Day (L-4)",1991,"Williams",wpc_mDMDS,0)
CORE_CLONEDEF(t2,l3,l8,"Terminator 2: Judgement Day (L-3)",1991,"Williams",wpc_mDMDS,0)
CORE_CLONEDEF(t2,l2,l8,"Terminator 2: Judgement Day (L-2)",1991,"Williams",wpc_mDMDS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData t2SimData = {
  2,    				/* 2 game specific input ports */
  t2_stateDef,			/* Definition of all states */
  t2_inportData,		/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 				/* no init */
  t2_handleBallState,	/*Function to handle ball state changes*/
  t2_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 				/* Do Not Simulate manual shooter */
  NULL  				/* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData t2GameData = {
  GEN_WPCDMD, wpc_dispDMD,
  {
    FLIP_SWNO(12,11),
    0,0,0,0,0,0,0,
    NULL, t2_handleMech, t2_getMech, t2_drawMech,
    &t2_lampPos, t2_SamSolMap
  },
  &t2SimData,
  {
    {0},{ 0x00,0x00,0x00,0x02 }, /* No inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swTrigger}
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_t2(void) {
  static mech_tSwData gunSw[] = {{swGunHome, 0, 3},{swGunMark, 75, 85},{0}};
  core_gameData = &t2GameData;
  mech_addLong(0, sGunMotor, 0, MECH_NONLINEAR|MECH_REVERSE|MECH_ONESOL,
               200,200, gunSw);
}

/***************************************************************************************************
 Functions here must manually adjust the state of mechanicla objects, usually checking for solenoids
 which control the state of those objects. Opening & Closing Doors, Diverter Switchs, etc..
****************************************************************************************************/
#if 0
static mech_motorData t2_gunMech = {
  200, 200, MECH_NONLINEAR|MECH_REVERSE|MECH_ONESOL,
  {{swGunHome, 0, 3},{swGunMark, 75, 85}},sGunMotor
};
#endif
static void t2_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Skull Target: %-6s", (locals.droptargetPos==DT_UP)?"Up":"Down");
  core_textOutf(30, 10,BLACK,"Gun Position:%3d", mech_getPos(0));
}
static void t2_handleMech(int mech) {
  /* -------------------------------------
     --	Open & Close Skull Drop Target  --
     ------------------------------------- */
  if (mech & 0x01) {
    /*-- if DT is up, & Kick Down Solenoid firing lower it! --*/
    if (locals.droptargetPos == DT_UP && core_getSol(sKnockDown)) {
      locals.droptargetPos = DT_DOWN;
      core_setSw(swDropTarget,1);		/*If Drop Target Down, Switch Must be Active*/
    }
    /*-- if DT is up, & Drop Target Switch Active, lower it!! --*/
    /*-- NOTE: Here because for somereason, the sKnockDown solenoid does not fire during game play --*/
    if (locals.droptargetPos == DT_UP && core_getSw(swDropTarget)) {
      locals.droptargetPos = DT_DOWN;
      wpc_play_sample(2,SAM_FLAPCLOSE);
    }
    /*-- if DT is down, & Drop Target Solenoid firing raise it! --*/
    if (locals.droptargetPos == DT_DOWN && core_getSol(sDropTarget)) {
      locals.droptargetPos = DT_UP;
      core_setSw(swDropTarget,0);		/*If Drop Target Down, Switch Must be InActive*/
    }
  }
  /* ----------------------------------
     --	Rotate the Gun?              --
     ---------------------------------- */
//  if (mech & 0x02) mech_update(&t2_gunMech,core_getPulsedSol(sGunMotor),0);
//  if (mech & 0x02) mech_update(0);
}
static int t2_getMech(int mechNo) {
  return mechNo ? mech_getPos(0) : locals.droptargetPos;
}
#define GUN_MARK 80
static int GetGunPos(void) {
  int gunPos = mech_getPos(0);
 if(gunPos>= GUN_MARK && gunPos<= GUN_MARK+14)
   return GUNPOS_PF;
 if(gunPos>= GUN_MARK+15 && gunPos<= GUN_MARK+29)
   return GUNPOS_T1;
 if(gunPos>= GUN_MARK+30 && gunPos<= GUN_MARK+44)
   return GUNPOS_T2;
 if(gunPos>= GUN_MARK+45 && gunPos<= GUN_MARK+59)
   return GUNPOS_T3;
 if(gunPos>= GUN_MARK+60 && gunPos<= GUN_MARK+74)
   return GUNPOS_T4;
 if(gunPos>= GUN_MARK+75 && gunPos<= GUN_MARK+89)
   return GUNPOS_T5;
 if(gunPos>= GUN_MARK+90 && gunPos<= GUN_MARK+104)
   return GUNPOS_PF;
 return GUNPOS_PF;
}


#if 0
/*Return A string depciting what the gun is pointing at*/
static char * GetGunDescr()
{
switch (GetGunPos())
        {
        case GUNPOS_PF:
             return "PF";
        case GUNPOS_T1:
             return "T1";
        case GUNPOS_T2:
             return "T2";
        case GUNPOS_T3:
             return "T3";
        case GUNPOS_T4:
             return "T4";
        case GUNPOS_T5:
             return "T5";
	default:
	     return "PF";
        }
}
#endif
