/********************************************************************************************
 Williams Star Trek Next Generation (1994) Pinball Simulator

 by Steve Ellenoff (sellenoff@hotmail.com)
 Nov.3, 2000

 Known Issues/Problems with Simulator:
 -Spinner not being read enough times
 -Gun not 100% correct... fudge was used to make sure gun get's loaded before it starts moving!
 -Occasionally Gun fires by itself right after loading
 -Variable Ball Launching from Gun not implemented..
 -Music cuts out on Shoot Again..
 -Music cuts out on Buy In...

 Changes:
 --------
 12-05-2000: Modified custom playfield lamp layout to accomodate new structure.
             Added support for shared bulbs where necessary
 12-30-2000: Added Sample Support
 04-17-2001: Added GetMech support
 05-02-2001: Added Custom Solenoid Support for #41-#42

 Notes:
 ------
*********************************************************************************************************/


/*-------------------------------------------------------------------------------------------------
  Keys for Star Trek Next Generation Simulator:
  ----------------------------
    +R		L/R Ramp
     R		Middle Ramp
    +I		L/R Inlane
    +-		L/R Slingshot
    +L		L/R Loop
    +O		L/R Outlane
   ASD		Left Drop Targets (1-3)
   FGH		Right Drop Targets (1-3)
   JKL		Center Drop Targets (1-3)
     W		Q Drop Target
     /		Center Hole
     .		Left Hole
     E		Neutral Zone Hole
    ZX		Time Rift Targets (1-2)
   CVB		Rollovers (1-3)
   NM,		Jet Bumpers (1-3)
     Q		SDTM (Drain Ball)
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
static int  sttng_handleBallState(sim_tBallStatus *ball, int *inports);
static void sttng_handleMech(int mech);
static void init_sttng(void);
static void sttng_drawMech(BMTYPE **line);
static int sttng_getMech(int mechNo);
static void sttng_drawStatic(BMTYPE **line);
static int sttng_getSol(int solNo);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int TdroptargetPos;    /* Top Drop Target Position */
  int LgunPos;			 /* Left Gun Position  */
  int LgunDir;		     /* Left Current Direction of Gun Movement*/
  int LgunLDir;			 /* Left Last direction of Gun Movement*/
  int RgunPos;			 /* Right Gun Position  */
  int RgunDir;		     /* Right Current Direction of Gun Movement*/
  int RgunLDir;			 /* Right Last direction of Gun Movement*/
  int UTDivPos;			 /* Keep Track of Under Playfield Top Diverter*/
  int UBDivPos;			 /* Keep Track of Under Playfield Bott Diverter*/
  int TTDivPos;			/* Keep Track of Top Diverter*/
  } locals;

/*--------------------------
/ Game specific input ports
/---------------------------
   Here we define for PINMAME which Keyboard Presses it should look for, and process!
--------------------------------------------------------------------------------------------------*/
WPC_INPUT_PORTS_START(sttng,6)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",        KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"L/R Inlane",      KEYCODE_I)
    COREPORT_BITIMP(0x0100,"L.Drop Target 1", KEYCODE_A)
    COREPORT_BITIMP(0x0200,"L.Drop Target 2", KEYCODE_S)
    COREPORT_BITIMP(0x0400,"L.Drop Target 3", KEYCODE_D)
    COREPORT_BITIMP(0x0800,"R.Drop Target 1", KEYCODE_F)
    COREPORT_BIT(   0x1000,"R.Drop Target 2", KEYCODE_G)
    COREPORT_BIT(   0x2000,"R.Drop Target 3", KEYCODE_H)
    COREPORT_BITIMP(0x4000,"C.Drop Target 1", KEYCODE_J)
    COREPORT_BIT(   0x8000,"C.Drop Target 1", KEYCODE_K)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"C.Drop Target 1", KEYCODE_L)
    COREPORT_BIT(   0x0002,"Q Drop Target",   KEYCODE_W)
    COREPORT_BIT(   0x0004,"Center Hole",     KEYCODE_SLASH)
    COREPORT_BIT(   0x0008,"Left Hole",       KEYCODE_STOP)
    COREPORT_BIT(   0x0010,"Time Rift 1",     KEYCODE_Z)
    COREPORT_BIT(   0x0020,"Time Rift 2",     KEYCODE_X)
    COREPORT_BIT(   0x0040,"Rollover 1",      KEYCODE_C)
    COREPORT_BIT(   0x0100,"Rollover 2",      KEYCODE_V)
    COREPORT_BIT(   0x0200,"Rollover 3",      KEYCODE_B)
    COREPORT_BIT(   0x0400,"Jet 1",           KEYCODE_N)
    COREPORT_BIT(   0x0800,"Jet 2",           KEYCODE_M)
    COREPORT_BIT(   0x1000,"Jet 3",           KEYCODE_COMMA)
    COREPORT_BITIMP(0x2000,"Drain",           KEYCODE_Q)
    COREPORT_BIT(   0x4000,"Buy-In",          KEYCODE_2)
    COREPORT_BIT(   0x8000,"Neutral Zone Hole", KEYCODE_E)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/

#define swBuyIn		11
#define swFire		12
#define swStart      	13
#define swTilt       	14
#define swLOut	 	15
#define swLIn		16
#define swRIn		17
#define swROut		18

#define swSlamTilt   	21
#define swCoinDoor   	22
#define swCRampExit    	23
#define swNotUsed	24
#define swRRampEnt	25
#define	swCBankL	26
#define	swCBankC	27
#define swCBankR	28

/*All switchs in this column are optos*/
#define	swBorgLock	31
#define	swULGun2	32
#define	swURGun2	33
#define	swRGunShooter	34
#define	swULLock2	35
#define	swULGun1	36
#define	swURGun1	37
#define	swLGunShooter	38

/*All switchs in this column are optos*/
#define swULLock1     	41
#define swULLock3	42
#define swULLock4	43
#define swLOuterLoop	44
#define swUTopHole	45
#define swULeftHole	46
#define swUBorgHole  	47
#define swBorgEntry	48

#define swLBankTop	51
#define swLBankMid	52
#define swLBankBott    	53
#define swRBankTop	54
#define swRBankMid	55
#define swRBankBott	56
#define swTDropTarget	57
#define swROuterLoop	58

/*All switchs in this column are optos -except last switch!-*/
#define swTrough1	61
#define swTrough2    	62
#define swTrough3    	63
#define swTrough4	64
#define swTrough5	65
#define swTrough6	66
#define swTroughUp	67
#define swShooter      	68

#define swJet1		71
#define swJet2	    	72
#define swJet3	  	73
#define swRSling    	74
#define swLSling	75
#define swMultL		76
#define swMultC       	77
#define swMultR       	78

#define swTime		81
#define swRift	    	82
#define swLRampExit    	83
#define swQ	  	84
#define swLShuttle	85
#define swRShuttle	86
#define swRRampExit    	87
#define swLRampEnt     	88

/*Since normal switches end at 88, we need custom ones!*/

/*91 Not Used*/
#define swLGunMark    	CORE_CUSTSWNO(1,2)		//92
/*93 Not Used*/
#define swRGunHome	CORE_CUSTSWNO(1,5)		//95
#define swRGunMark    	CORE_CUSTSWNO(1,6)		//96
#define swLGunHome	CORE_CUSTSWNO(1,7)		//97
/*98 Not Used*/

/*Spinner = F7*/
#define swSpinner	WPC_swF7


/*---------------------
/ Solenoid definitions
/----------------------*/
#define sLGunKicker    	1
#define sRGunKicker    	2
#define sLGunPopper  	3
#define sRGunPopper		4
#define sLPopper		5
#define sPlunger		6
#define sKnocker       	7
#define sKickBack   	8
#define sLSling      	9
#define sRSling      	10
#define sTrough		11
#define sJet1         	12
#define sJet2        	13
#define sJet3        	14
#define sTopDiverter	15
#define sBorgKicker	16
#define sLGunMotor    	17
#define sRGunMotor    	18
/*19 Not Used*/

/*We need custom solenoids, since regular ones end at #36*/
#define sUDiverterTop  	CORE_CUSTSOLNO(1)	//37
#define sUDiverterBott 	CORE_CUSTSOLNO(2)	//38
#define sTopDropUp     	CORE_CUSTSOLNO(3)	//39
#define sTopDropDown   	CORE_CUSTSOLNO(4)	//40
#define sfRomulan		CORE_CUSTSOLNO(5)	//41
#define sfRRamp			CORE_CUSTSOLNO(6)	//42


#define GUN_HOME    (8*2)   /*End Position of Gun Home*/
#define GUN_MARK    (8*7)   /* Where is Mark Switch Triggered*/
#define GUN_END	    (8*20)  /*# of Times to increment rotation until we stop and turnaround!*/
#define GUN_LEFT    0
#define GUN_RIGHT   1
#define DT_UP		0
#define DT_DOWN		1
#define DIV_OPEN	0
#define DIV_CLOSED	1


/*---------------------
/  Ball state handling
/----------------------*/

/* ---------------------------------------------------------------------------------
   The following variables are used to refer to the state array!
   These vars *must* be in the *exact* *same* *order* as each stateDef array entry!
   -----------------------------------------------------------------------------------------*/
enum {stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stTrough,
	  stShooter, stLaunchWire, stDrain,
	  stLRampEnt,stLRampExit, stCRampExit, stRRampEnt, stRRampExit,
	  stLOut, stROut, stLIn, stRIn, stLLoopUp1, stSpinner, stLLoopUp2, stRLoopDn, stRLoopUp,
	  stTDropTarget, stTDropTarget2, stMultL, stMultC, stMultR,
      stJet1, stJet2, stJet3,
      stTopHole, stCenterHole, stLeftHole,  stTopDiverter, stBottDiverter, stDiverter,
	  stTLPopper4,stTLPopper3,stTLPopper2, stTLPopper1,
	  stBLPopper, stBLPopper2,stRPopper2, stRPopper, stLGunLoaded, stRGunLoaded,
	  stBorgEntry, stBorgLock, stNZone
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
static sim_tState sttng_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",               0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  {"Trough 1",			1,swTrough1,	0,	  stTrough2,   1},
  {"Trough 2",			1,swTrough2,	0,        stTrough3,   1},
  {"Trough 3",			1,swTrough3,	0,        stTrough4,   1},
  {"Trough 4",			1,swTrough4,	0,        stTrough5,   1},
  {"Trough 5",			1,swTrough5,	0,        stTrough6,   1},
  {"Trough 6",			1,swTrough6,	sTrough,  stTrough,    1},
  {"Trough Up",			1,swTroughUp,   0,        stShooter,   1},

  {"Shooter",		    	1,swShooter,   sPlunger,  stLaunchWire, 1},
  {"Launch Wire",		1,0,	       0,	 stRLoopUp,   10},
  {"Drain",			1,0,           0,        stTrough1,   0,0,0,SIM_STNOTEXCL},

  {"L. Ramp Enter",		1,swLRampEnt,   0,       stDiverter,    5},
  {"L. Ramp Exit",		1,swLRampExit,  0,       stRIn,        15},
  {"C. Ramp Exit",		1,swCRampExit,  0,       stLIn,        10},
  {"R. Ramp Enter",		1,swRRampEnt,   0,       stRRampExit,   5},
  {"R. Ramp Exit",		1,swRRampExit,  0,       stRIn,        10},

  {"Left Outlane",      1,swLOut,       0,        stDrain,   10, sKickBack, stFree},
  {"Right Outlane",     1,swROut,       0,        stDrain,   10},
  {"Left  Inlane",     1,swLIn,         0,        stFree,      5},
  {"Right Inlane",     1,swRIn,         0,        stFree,      5},

  {"Left Loop",        1,0,		0,        stSpinner,	3},
  {"Spinner",          1,swSpinner,	0,	  stLLoopUp2,	5, 0,0,SIM_STSPINNER},
  {"Left Loop",        1,swLOuterLoop,	0,	  stRLoopDn,	10},
  {"Right Loop",       1,swROuterLoop,	0,        stFree,	10},	/*Right Loop Coming Down*/
  {"Right Loop",       1,swROuterLoop,	0,        stTDropTarget,10},	/*Right Loop Going Up */

  {"Top Drop Target",	1,0, 0, 0, 0},
  {"Top Drop Target",	1,swTDropTarget,0,	stMultL,	5},
  {"Rollover 1",	1,swMultL,	0,	stJet2,		5},
  {"Rollover 2",	1,swMultC,	0,	stJet3,		5},
  {"Rollover 3",	1,swMultR,	0,	stJet1,		5},
  {"Jet Bumper 1",	1,swJet1, 	0,	stFree, 	5},
  {"Jet Bumper 2",	1,swJet1, 	0, 	stFree, 	5},
  {"Jet Bumper 3",	1,swJet1, 	0, 	stFree, 	5},

		/*Under Bott Diverter firing, Goes to L. Gun, otherwise L.Popper
		  Under Top Diverter firing, Goes to R. Gun
		*/

  {"Top Hole",		1,swUTopHole,	0,			stLeftHole,	 2},	/*Top Holes Next passes Left hole Switch*/
  {"Center Hole",	1,swUBorgHole,	0,			stTopDiverter,   2},	/*Center Next goes to Top Diverter*/
  {"Left Hole",		1,swULeftHole,	0,			stTopDiverter,   2},	/*Left Next goes to Top Diverter*/
  {"U.Top Diverter",	1,0,		0,			0,	 0},
  {"U.Bott Diverter",	1,0,		0,			0,	 0},
  {"Top Diverter",	1,0,		0,			0,	 0},

  {"L. Popper 4",	1, swULLock4,	0,			stTLPopper3,	 1},	/*TLPopper4*/
  {"L. Popper 3",	1, swULLock3,	0,			stTLPopper2,	 1},	/*TLPopper3*/
  {"L. Popper 2",	1, swULLock2,	0,			stTLPopper1,	 1},	/*TLPopper2*/
  {"L. Popper 1",	1, swULLock1,   sLPopper,		stRIn,			 1},	/*TLPopper1*/

  {"L. Gun 2",		1, swULGun2,	0,			stBLPopper2,	1}, /*BLPopper*/
  {"L. Gun 1",		1, swULGun1,	sLGunPopper,		stLGunLoaded, 	1}, /*BLPopper2*/
  {"R. Gun 2",		1, swURGun2,	0,			stRPopper,	1}, /*RPopper2*/
  {"R. Gun 1",		1, swURGun1,	sRGunPopper,		stRGunLoaded,	1}, /*RPopper*/
  {"L. Gun Loaded",	1, swLGunShooter, sLGunKicker,  	stFree,		2},
  {"R. Gun Loaded",	1, swRGunShooter, sRGunKicker,		stFree,		2},
  {"Borg Entry",	1, swBorgEntry,   0, 			stBorgLock,	1},
  {"Borg Lock",		1, swBorgLock,	  sBorgKicker,		stFree,		2},
  {"Neutral Zone Hole", 1, swCBankC,	0,			stLeftHole,	1}, /*Neutral Zone fires middle bank target, then fires left hole switch*/
  {0}
};

static int sttng_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{
		/*-----------------
		  Top Drop Target
		-------------------*/
		case stTDropTarget:
		/* If Drop Target Is Down.. Ball goes into Top Hole!*/
		/* Otherwise simply trigger Drop Target Switch  */
		if (locals.TdroptargetPos==DT_DOWN)
			return setState(stTopHole,25);
		else
			return setState(stTDropTarget2,25);
		break;

		/*-------------------------
		  Underfield Top Diverter
		-------------------------*/
		/*If Underside Top Diverter firing(CLOSED), send to Right Popper/Gun, otherwise, bottom diverted!*/
		case stTopDiverter:
		if (locals.UTDivPos==DIV_CLOSED)
			return setState(stRPopper2,1);
		else
			return setState(stBottDiverter,1);
		break;

		/*--------------------------
		  Underfield Bottom Diverter
		---------------------------*/

		/*If Underside Bott Diverter firing(OPEN), send to Left Popper/Gun, otherwise, top left popper!*/
		case stBottDiverter:
		if (locals.UBDivPos==DIV_OPEN)
			return setState(stBLPopper,1);
		else
			return setState(stTLPopper4,1);
		break;

		/*-------------------------
		  Top Diverter
		-------------------------*/
		/*If Top Diverter firing(CLOSED), send to Completed Left Ramp Shot, otherwise to Borg Entry!*/
		case stDiverter:
		if (locals.TTDivPos==DIV_CLOSED)
			return setState(stLRampExit,3);
		else
			return setState(stBorgEntry,3);
		break;
  	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------
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

static sim_tInportData sttng_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stRRampEnt},
  {0, 0x0004, stCRampExit},     /*For this to work, C Ramp shot must come before L&R..  because 0x0004 is set for both left & right ramp shots*/
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp1},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, swLSling},
  {0, 0x0042, swRSling},
  {0, 0x0081, stLIn},
  {0, 0x0082, stRIn},
  {0, 0x0100, swLBankTop},
  {0, 0x0200, swLBankMid},
  {0, 0x0400, swLBankBott},
  {0, 0x0800, swRBankTop},
  {0, 0x1000, swRBankMid},
  {0, 0x2000, swRBankBott},
  {0, 0x4000, swCBankL},
  {0, 0x8000, swCBankC},
  {1, 0x0001, swCBankR},
  {1, 0x0002, swQ},
  {1, 0x0004, stCenterHole},
  {1, 0x0008, stLeftHole},
  {1, 0x0010, swTime},
  {1, 0x0020, swRift},
  {1, 0x0040, swMultL},
  {1, 0x0100, swMultC},
  {1, 0x0200, swMultR},
  {1, 0x0400, swJet1},
  {1, 0x0800, swJet2},
  {1, 0x1000, swJet3},
  {1, 0x2000, stDrain},
  {1, 0x4000, swBuyIn, SIM_STANY},
  {1, 0x8000, stNZone},
  {0}
};

/*--------------------
  Drawing information
 ---------------------*/

static core_tLampDisplay sttng_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{23, 5,YELLOW}}},{1,{{25, 4,YELLOW}}},{1,{{26,11,WHITE}}},{1,{{28,12,WHITE}}},
 {1,{{27, 3,YELLOW}}},{1,{{29,14,RED}}},{1,{{37,14,ORANGE}}},{1,{{28,16,WHITE}}},
 {1,{{24,12,WHITE}}},{1,{{23,14,WHITE}}},{1,{{24,16,WHITE}}},{1,{{23,23,YELLOW}}},
 {1,{{25,24,YELLOW}}},{1,{{26,14,WHITE}}},{1,{{26,17,WHITE}}},{1,{{27,25,YELLOW}}},
 {1,{{ 3,18,GREEN}}},{1,{{ 3,20,GREEN}}},{1,{{ 3,22,GREEN}}},{1,{{34,10,GREEN}}},
 {1,{{33,12,GREEN}}},{1,{{32,14,GREEN}}},{1,{{33,16,GREEN}}},{1,{{34,18,GREEN}}},
 {1,{{12,19,RED}}},{1,{{13,28,WHITE}}},{1,{{17,26,GREEN}}},{1,{{15,27,ORANGE}}},
 {1,{{16,24,WHITE}}},{1,{{16,22,WHITE}}},{1,{{17,21,YELLOW}}},{1,{{14,22,WHITE}}},
 {1,{{34, 5,WHITE}}},{1,{{31, 3,RED}}},{1,{{ 6, 5,YELLOW}}},{1,{{ 9,16,WHITE}}},
 {1,{{12,16,YELLOW}}},{1,{{14,16,RED}}},{1,{{16,16,ORANGE}}},{1,{{18,16,WHITE}}},
 {1,{{16, 4,WHITE}}},{1,{{18, 5,ORANGE}}},{1,{{20, 6,RED}}},{1,{{ 8,14,WHITE}}},
 {1,{{10,14,YELLOW}}},{1,{{12,14,ORANGE}}},{1,{{16, 9,RED}}},{1,{{17, 8,RED}}},
 {1,{{12, 7,WHITE}}},{1,{{ 9, 8,YELLOW}}},{1,{{ 8,10,YELLOW}}},{1,{{ 7,12,YELLOW}}},
 {1,{{10,11,GREEN}}},{1,{{13,10,WHITE}}},{1,{{14,11,LPURPLE}}},{1,{{ 3,12,RED}}},
 {1,{{34,23,WHITE}}},{1,{{31,25,RED}}},{1,{{10,22,YELLOW}}},{1,{{31, 1,ORANGE}}},
 {1,{{10, 5,GREEN}}},{1,{{11, 5,RED}}},{1,{{39,28,GREEN}}},{1,{{39, 1,YELLOW}}}
}
};

static wpc_tSamSolMap sttng_SamSolMap[] = {
 /*Channel #0*/
 {sKnocker,0,SAM_KNOCKER}, {sPlunger,0,SAM_SOLENOID},
 {sTrough,0,SAM_POPPER}, {sLPopper,3,SAM_POPPER},

 /*Channel #1*/
 {sLSling,1,SAM_LSLING}, {sRSling,1,SAM_RSLING},
 {sJet1,1,SAM_JET1}, {sJet2,1,SAM_JET2},
 {sJet3,1,SAM_JET3},

 /*Channel #2*/
 {sLGunKicker,2,SAM_SOLENOID}, {sLGunPopper,2,SAM_POPPER},
 {sKickBack,2,SAM_SOLENOID}, {sBorgKicker,2,SAM_SOLENOID},

 /*Channel #3*/
 {sRGunKicker,3,SAM_SOLENOID}, {sRGunPopper,3,SAM_POPPER},
// {sTopDiverter,3,SAM_DIVERTER}, {sUDiverterTop,3,SAM_DIVERTER},
 {sTopDiverter,3,SAM_SOLENOID_ON}, {sTopDiverter,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sUDiverterTop,3,SAM_SOLENOID_ON}, {sUDiverterTop,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},

 /*Channel #4*/
// {sUDiverterBott,4,SAM_DIVERTER},
 {sUDiverterBott,3,SAM_SOLENOID_ON}, {sUDiverterBott,3,SAM_SOLENOID_OFF,WPCSAM_F_ONOFF},
 {sTopDropUp,4,SAM_FLAPOPEN}, {sTopDropDown,4,SAM_FLAPCLOSE},

 /*Channel #5*/
 {sLGunMotor,5,SAM_MOTOR_1, WPCSAM_F_CONT}, {sRGunMotor,5,SAM_MOTOR_1, WPCSAM_F_CONT},{-1}
};

/*--------------------------------------------------------
  Code to draw the mechanical objects, and their states!
---------------------------------------------------------*/
static void sttng_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Top Drop Target: %-6s", locals.TdroptargetPos==DT_UP?"Up":"Down");
  core_textOutf(30, 10,BLACK,"Left Gun: %3d", locals.LgunPos);
  core_textOutf(30, 20,BLACK,"Right Gun: %3d", locals.RgunPos);
}
  /* Help */

static void sttng_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball, W = Q's Drop Target");
  core_textOutf(30,100,BLACK,"E = Neutral Zone, R = Center Ramp");
  core_textOutf(30,110,BLACK,"A/S/D = Left Drop Targets");
  core_textOutf(30,120,BLACK,"J/K/L = Center Drop Targets");
  core_textOutf(30,130,BLACK,"F/G/H = Right Drop Targets");
  core_textOutf(30,140,BLACK,"Z/X = Time Rift Tgts, N/M/, Jet Bumper");
  core_textOutf(30,150,BLACK,". = Left Hole, / = Center Hole");
  core_textOutf(30,160,BLACK,"C/V/B = Top Rollovers");

}

/*-----------------
/  ROM definitions
/------------------*/
#define ST_SND \
DCS_SOUNDROM7x("ng_u2_s.l1",CRC(c3bd7bf5) SHA1(2476ff90232a52d667a407fac81ee4db028b94e5), \
               "ng_u3_s.l1",CRC(9456cac7) SHA1(83e415e0f21bb5418f3677dbc13433e056c523ab), \
               "ng_u4_s.l1",CRC(179d22a4) SHA1(456b7189e23d4e2bd7e2a6249fa2a73bf0e12194), \
               "ng_u5_s.l1",CRC(231a3e72) SHA1(081b1a042e62ccb723788059d6c1e00b9b32c778), \
               "ng_u6_s.l1",CRC(bb21377d) SHA1(229fb42a1f8b22727a809e5d63f26f045a2adda5), \
               "ng_u7_s.l1",CRC(d81b39f0) SHA1(3443e7327c755b85a5b390f7fcd0e9923890425a), \
               "ng_u8_s.l1",CRC(c9fb065e) SHA1(c148178ee0ea787acc88078db01d17073e75fdc7))

WPC_ROMSTART(sttng,l7,"trek_lx7.rom",0x80000,CRC(d439fdbb) SHA1(12d1c72cd6cc18db53e51ebb4c1e55ca9bcf9908)) ST_SND WPC_ROMEND

WPC_ROMSTART(sttng,x7,"trek_x7.rom",0x80000,CRC(4e71c9c7) SHA1(8a7ec42dfb4a6902ba745548b40e84de5305c295))
DCS_SOUNDROM7x("ngs_u2.rom",CRC(e9fe68fe) SHA1(3d7631aa5ddd52f7c3c00cd091e212430faea249),
               "ngs_u3.rom",CRC(368cfd89) SHA1(40ddc12b2cabbcf73ababf753f3a2fd4bcc10737),
               "ngs_u4.rom",CRC(8e79a513) SHA1(4b763d7445acd921a0a6d64d18b5df8ff9e3257e),
               "ngs_u5.rom",CRC(46049eb0) SHA1(02991bf1d33ac1df91f459b2d37cf7e07e347b04),
               "ngs_u6.rom",CRC(e0124da0) SHA1(bfdba059d084c93122ad291aa8def61f43c26d47),
               "ngs_u7.rom",CRC(dc1c74d0) SHA1(21b6b4d2cdd5086bcbbc7ee7a2abdc550a45d2e3),
               "ng_u8_s.l1",CRC(c9fb065e) SHA1(c148178ee0ea787acc88078db01d17073e75fdc7))
WPC_ROMEND

WPC_ROMSTART(sttng,s7,"trek_lx7.rom",0x80000,CRC(d439fdbb) SHA1(12d1c72cd6cc18db53e51ebb4c1e55ca9bcf9908))
DCS_SOUNDROM7x("su2-sp1.rom",CRC(bdef8b2c) SHA1(188d8d2a652844e9885bd9e9ad4143927ddc6fee),
               "ng_u3_s.l1",CRC(9456cac7) SHA1(83e415e0f21bb5418f3677dbc13433e056c523ab),
               "ng_u4_s.l1",CRC(179d22a4) SHA1(456b7189e23d4e2bd7e2a6249fa2a73bf0e12194),
               "ng_u5_s.l1",CRC(231a3e72) SHA1(081b1a042e62ccb723788059d6c1e00b9b32c778),
               "ng_u6_s.l1",CRC(bb21377d) SHA1(229fb42a1f8b22727a809e5d63f26f045a2adda5),
               "ng_u7_s.l1",CRC(d81b39f0) SHA1(3443e7327c755b85a5b390f7fcd0e9923890425a),
               "ng_u8_s.l1",CRC(c9fb065e) SHA1(c148178ee0ea787acc88078db01d17073e75fdc7))
WPC_ROMEND

WPC_ROMSTART(sttng,p5,"sttng_p5.u6",0x80000,CRC(c1b80a8e) SHA1(90dd99efd41ec5405c631ad374a369f9fcb7217e)) ST_SND WPC_ROMEND
WPC_ROMSTART(sttng,g7,"trek_lg7.rom",0x80000,CRC(e723b8a1) SHA1(77c3f8ea378772ce45bb8de818069fc08cbc4574)) ST_SND WPC_ROMEND
WPC_ROMSTART(sttng,l1,"trek_lx1.rom",0x80000,CRC(390befc0) SHA1(2059891e3fc3034d600274c3915371123c964d28)) ST_SND WPC_ROMEND
WPC_ROMSTART(sttng,l2,"trek_lx2.rom",0x80000,CRC(e2557554) SHA1(7d8502ab9df340d60fd72e6964740bc7a2da2065)) ST_SND WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(sttng,l7,"Star Trek: The Next Generation (LX-7)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,x7,l7,"Star Trek: The Next Generation (LX-7 Special)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,p5,l7,"Star Trek: The Next Generation (P-5)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,s7,l7,"Star Trek: The Next Generation (LX-7) SP1",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,g7,l7,"Star Trek: The Next Generation (LG-7)",1994,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,l1,l7,"Star Trek: The Next Generation (LX-1)",1993,"Williams",wpc_mDCSS,0)
CORE_CLONEDEF(sttng,l2,l7,"Star Trek: The Next Generation (LX-2)",1993,"Williams",wpc_mDCSS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData sttngSimData = {
  2,    		/* 2 game specific input ports */
  sttng_stateDef,	/* Definition of all states */
  sttng_inportData,	/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stDrain }, /*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 		/* no init */
  sttng_handleBallState,/*Function to handle ball state changes*/
  sttng_drawStatic,	/*Function to handle mechanical state changes*/
  FALSE, 		/* Do not simulate manual shooter */
  NULL  		/* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData sttngGameData = {
  GEN_WPCDCS, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_UR),
    1,0,6,0,0,0,0,
    sttng_getSol, sttng_handleMech, sttng_getMech, sttng_drawMech,
    &sttng_lampPos, sttng_SamSolMap
  },
  &sttngSimData,
  {     /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    {0},{ 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swFire},
  }
};

/*---------------
/  Game handling
/----------------*/
/*-- patched memory read function to handle the 9th switch column --*/
READ_HANDLER(sttng_swRowRead) {
//  if ((wpc_data[WPC_EXTBOARD1] && 0x80) > 0) /* 9th column enabled */
  if (wpc_data[WPC_EXTBOARD1] == 0x80) /* 9th column enabled */
    return coreGlobals.swMatrix[CORE_CUSTSWCOL];
  else
    return wpc_r(WPC_SWROWREAD);
}

static void init_sttng(void) {
  core_gameData = &sttngGameData;
  install_mem_read_handler(WPC_CPUNO, WPC_SWROWREAD+WPC_BASE, WPC_SWROWREAD+WPC_BASE,
                         sttng_swRowRead);
}

static int sttng_getSol(int solNo) {
  if (solNo == sUDiverterTop)				//37
    return (wpc_data[WPC_EXTBOARD1] & 0x01) > 0;
  if (solNo == sUDiverterBott)				//38
    return (wpc_data[WPC_EXTBOARD1] & 0x02) > 0;
  if (solNo == sTopDropUp)					//39
    return (wpc_data[WPC_EXTBOARD1] & 0x04) > 0;
  if (solNo == sTopDropDown)				//40
    return (wpc_data[WPC_EXTBOARD1] & 0x08) > 0;
  if (solNo == sfRomulan)					//41
    return (wpc_data[WPC_EXTBOARD1] & 0x10) > 0;
  if (solNo == sfRRamp)						//42
    return (wpc_data[WPC_EXTBOARD1] & 0x20) > 0;
  return 0;
}

static int utdivcnt=0;
static int ubdivcnt=0;
static int ttdivcnt=0;
static int lbuff=0;
static int rbuff=0;
#define CHECK_SOL	50	/*Number of times to check if the solenoid is still firing*/

static void sttng_handleMech(int mech) {
  /*-----------------------------
    Set the Status of Diverters
   ------------------------------*/
  if (mech & 0x01) {
    /*If Under-Top-Diverter Solenoid Fires - Set Position to Closed!
      Reset Counter for opening it back!*/
    if (core_getSol(sUDiverterTop)) {
      locals.UTDivPos = DIV_CLOSED;
      utdivcnt = 0;
    }
    else {
      /*Solenoid not firing, if after CHECK_SOL # of checks, it hasn't fired, we flag it as open!*/
      utdivcnt++;
      if (utdivcnt > CHECK_SOL) {
	utdivcnt=0;
	locals.UTDivPos = DIV_OPEN;
      }
    }
  }
  if (mech & 0x02) {
    /*If Under-Bott-Diverter Solenoid Fires - Set Position to Open!
      Reset Counter for closing it back!*/
    if (core_getSol(sUDiverterBott)) {
      locals.UBDivPos = DIV_OPEN;
      ubdivcnt = 0;
    }
    else {
      /*Solenoid not firing, if after CHECK_SOL # of checks, it hasn't fired, we flag it as closed!*/
      ubdivcnt++;
      if (ubdivcnt > CHECK_SOL) {
        ubdivcnt=0;
        locals.UBDivPos = DIV_CLOSED;
      }
    }
  }
  if (mech & 0x04) {
    /*If Top-Diverter Solenoid Fires - Set Position to Closed!
    Reset Counter for opening it back!*/
    if (core_getSol(sTopDiverter)) {
      locals.TTDivPos = DIV_CLOSED;
      ttdivcnt = 0;
    }
    else {
      /*Solenoid not firing, if after CHECK_SOL # of checks, it hasn't fired, we flag it as open!*/
      ttdivcnt++;
      if (ttdivcnt > CHECK_SOL) {
	ttdivcnt=0;
 	locals.TTDivPos = DIV_OPEN;
      }
    }
  }
  /*Raise & Lower Drop Target*/
  if (mech & 0x08) {
    if (core_getSol(sTopDropUp))
      locals.TdroptargetPos=DT_UP;
    if (core_getSol(sTopDropDown))
      locals.TdroptargetPos=DT_DOWN;
    /* If Drop Target is Down, switch should be active!*/
    core_setSw(swTDropTarget,locals.TdroptargetPos==DT_DOWN);
  }

  /*-----------------------------------------------------
                      Gun Handling
   ------------------------------------------------------*/

  /* -------------- */
  /* -- LEFT GUN -- */
  /* -------------- */
  /*--Make sure Gun Home & Mark switches set!--*/

  /*Gun Home Switch set while it's at home!*/
  if (mech & 0x10) {
    if (locals.LgunPos >= 0 && locals.LgunPos <= GUN_HOME)
      core_setSw(swLGunHome,1);
    else
      core_setSw(swLGunHome,0);

    /*Gun Mark Switch set unless it's at mark!*/
    if (locals.LgunPos >= GUN_MARK)
      core_setSw(swLGunMark,0);
    else
      core_setSw(swLGunMark,1);

    /*-- Is Gun Motor Turning? --*/
    if (core_getSol(sLGunMotor)) {
      /*Buffer movement.. Even if motor is firing, delay actual movement*/
      /*Here to prevent a simulator bug which moves the guns before loading them*/
      if (lbuff++ > CHECK_SOL+40) {
	/*Do we need to reverse direction?*/
	if (locals.LgunLDir == GUN_LEFT && locals.LgunPos >= GUN_END)
    	  locals.LgunDir = GUN_RIGHT;

	if (locals.LgunLDir == GUN_RIGHT && locals.LgunPos <= 0)
  	  locals.LgunDir = GUN_LEFT;

	/*Capture direction*/
	locals.LgunLDir = locals.LgunDir;

	/*Change it's position*/
	locals.LgunPos += (locals.LgunDir==GUN_LEFT)?1:-1;
      }
    }	/*Gun Motor*/
    else
      lbuff = 0;
  }

/* --------------- */
/* -- RIGHT GUN -- */
/* --------------- */
/*--Make sure Gun Home & Mark switches set!--*/

/*Gun Home Switch set while it's at home!*/
  if (mech & 0x20) {
    if (locals.RgunPos >= 0 && locals.RgunPos <= GUN_HOME)
      core_setSw(swRGunHome,1);
    else
      core_setSw(swRGunHome,0);

    /*Gun Mark Switch set unless it's at mark!*/
    if (locals.RgunPos >= GUN_MARK)
      core_setSw(swRGunMark,0);
    else
      core_setSw(swRGunMark,1);

    /*-- Is Gun Motor Turning? --*/
    if (core_getSol(sRGunMotor)) {
      /*Buffer movement.. Even if motor is firing, delay actual movement*/
      /*Here to prevent a simulator bug which moves the guns before loading them*/
      if (rbuff++ > CHECK_SOL+40) {
	/*Do we need to reverse direction?*/
	if (locals.RgunLDir == GUN_LEFT && locals.RgunPos >= GUN_END)
  	  locals.RgunDir = GUN_RIGHT;
	if (locals.RgunLDir == GUN_RIGHT && locals.RgunPos <= 0)
	  locals.RgunDir = GUN_LEFT;

	/*Capture direction*/
	locals.RgunLDir = locals.RgunDir;

	/*Change it's position*/
	locals.RgunPos += (locals.RgunDir==GUN_LEFT)?1:-1;
      }	/*Gun Motor*/
    }
    else
      rbuff=0;
  }
}

static int sttng_getMech(int mechNo) {
  switch (mechNo) {
    case 0: return locals.UTDivPos;
    case 1: return locals.UBDivPos;
    case 2: return locals.TTDivPos;
    case 3: return locals.TdroptargetPos;
    case 4: return locals.LgunPos;
    case 5: return locals.RgunPos;
  }
  return 0;
}
