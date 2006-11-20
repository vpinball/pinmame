/********************************************************************************************
 Williams Hispeed II: The Getaway (1992) Pinball Simulator

 by Steve Ellenoff (sellenoff@hotmail.com)
 Nov.19, 2000

 Known Issues/Problems with Simulator:
  #1) I don't have access to this game, I guessed most of it from a Playfield picture
      and the rulesheet! I did love to play it though, but it's been years since I have!

 Notes-
 ------

 Thanks to Marton for helping out with this sim and making some fixes for me!!
 *********************************************************************************************************/


/*-------------------------------------------------------------------------------------------------
  Keys for Getaway Simulator:
  ----------------------------
    +R  L/R Ramp
    +I  L/R Inlane
    +-  L/R Slingshot
    +L  L/R Loop
    +O  L/R Outlane
     Q  Drain
     I  Upper Left Loop
   ASD  Green Light Targets (1-3)
   FGH  Yellow Light Targets (1-3)
   JKL  Red Light Targets (1-3)
   ,./  RBank Targets (1-3)
   BNM  LBank Targets (1-3)
   WER  Jet Bumpers (1-3)
     Y  Tunnel Shot
 L.ALT  Gear Up
 SPACE  Gear Down
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
static int  gw_handleBallState(sim_tBallStatus *ball, int *inports);
static void gw_handleMech(int mech);
static void gw_drawMech(BMTYPE **line);
static void gw_drawStatic(BMTYPE **line);
static void init_gw(void);

/*-----------------------
  local static variables
 ------------------------
   I used this locals structure for setting up variables to track the state of mechanical objects
   which needed to be displayed on screen
--------------------------------------------------------------------------------------------------*/
static struct {
  int rampPos;    	/* Ramp Position */
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------
   Here we define for PINMAME which Keyboard Presses it should look for, and process!
--------------------------------------------------------------------------------------------------*/

WPC_INPUT_PORTS_START(gw,3)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001,"Left Qualifier",  	KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002,"Right Qualifier", 	KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004,"L/R Ramp",        	KEYCODE_R)
    COREPORT_BITIMP(0x0008,"L/R Outlane",     	KEYCODE_O)
    COREPORT_BITIMP(0x0010,"L/R Loop",       	KEYCODE_L)
    COREPORT_BIT(   0x0040,"L/R Slingshot",   	KEYCODE_MINUS)
    COREPORT_BIT(   0x0080,"Jet Bumper 1", 	KEYCODE_W)
    COREPORT_BITIMP(0x0100,"Jet Bumper 2",    	KEYCODE_E)
    COREPORT_BITIMP(0x0200,"Jet Bumper 3",    	KEYCODE_R)
    COREPORT_BIT(   0x0400,"Gear Up",    	KEYCODE_LALT)
    COREPORT_BITIMP(0x0800,"Tunnel Shot",     	KEYCODE_T)
    COREPORT_BIT(   0x1000,"L/R Inlane",     	KEYCODE_I)
    COREPORT_BIT(   0x2000,"R. Bank DT 1",	KEYCODE_V)
    COREPORT_BITIMP(0x4000,"R. Bank DT 2",	KEYCODE_B)
    COREPORT_BIT(   0x8000,"R. Bank DT 3",	KEYCODE_N)

  PORT_START /* 1 */
    COREPORT_BIT(   0x0001,"L. Bank DT 1",       KEYCODE_Z)
    COREPORT_BIT(   0x0002,"L. Bank DT 2",	KEYCODE_X)
    COREPORT_BIT(   0x0004,"L. Bank DT 3",	KEYCODE_C)
    COREPORT_BIT(   0x0008,"Red DT 1",		KEYCODE_J)
    COREPORT_BIT(   0x0010,"Red DT 2",		KEYCODE_K)
    COREPORT_BIT(   0x0040,"Red DT 3",		KEYCODE_L)
    COREPORT_BIT(   0x0080,"Yellow DT 1",	KEYCODE_F)
    COREPORT_BIT(   0x0100,"Yellow DT 2",	KEYCODE_G)
    COREPORT_BIT(   0x0200,"Yellow DT 3",	KEYCODE_H)
    COREPORT_BITIMP(0x0400,"Green DT 1",		KEYCODE_A)
    COREPORT_BIT(   0x0800,"Green DT 2",		KEYCODE_S)
    COREPORT_BIT(   0x1000,"Green DT 3",		KEYCODE_D)
    COREPORT_BIT(   0x2000,"Drain",		KEYCODE_Q)
    COREPORT_BIT(   0x4000,"Upper Left Loop", 	KEYCODE_U)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swStart      		13
#define swTilt       		14
#define swLFreeBot    		15
#define swLFreeTop    		16
#define swRFreeBot    		17
#define swRFreeTop   		18

#define swSlamTilt   		21
#define swCoinDoor   		22
#define swTicket     		23
#define swNotUsed		24
#define swLOut			25
#define swLIn			26
#define swRIn			27
#define swROut			28

#define swLSling		31
#define swRSling		32
#define swGearLo		33
#define swGearHi		34
/*35 Not Used*/
#define swTopRed		36
#define swMidRed		37
#define swBotRed		38

#define swTopYellow		41
#define swMidYellow		42
#define swBotYellow		43
#define swRBankBot	     	44
#define swRBankMid	     	45
#define swRBankTop	     	46
/*47 Not Used*/
/*48 Not Used*/

#define swTopGreen		51
#define swMidGreen		52
#define swBotGreen		53
#define swRampDown		54
#define	swOuthole		55
#define	swLTrough		56
#define	swCTrough		57
#define	swRTrough		58

#define	swJet1			61
#define	swJet2			62
#define	swJet3			63
/*64 Not Used*/
#define	swUDRampMade		65
/*66 Not Used*/
#define	swLRampMade		67
/*68 Not Used*/

#define	swTopLoop		71
#define	swMidLoop		72
#define	swBotLoop		73
#define	swTopLock		74
#define	swMidLock		75
#define	swBotLock		76
#define	swEjectHole		77
#define swShooter  		78

#define	swOpto1			81
#define	swOpto2			82
#define	swOpto3			83
#define	swLRampEnt		84
#define	swOptoLoopMade		85
#define	swLBankBot		86
#define	swLBankMid		87
#define swLBankTop		88

/*---------------------
/ Solenoid definitions
/----------------------*/

#define sDiverterHi		1
#define sRampUp			2
#define sRampDown	      	3
#define sLocker			4
#define sRSling      		5
#define sLSling      		6
#define sKnocker       		7
#define sKickBack      		8
#define sEjectHole		9
#define sDiverterLo		10
#define sTrough			11
#define sPlunger		12
#define sJet1		   	13
#define sJet2          		14
#define sJet3          		15
#define sOuthole		16
#define sEnable1	   	25
#define sEnable2	   	26
#define sMarsLamps	   	27
#define sEnable3	   	28

#define UP			0
#define DOWN			1
/*---------------------
/  Ball state handling
/----------------------*/

/* ---------------------------------------------------------------------------------
   The following variables are used to refer to the state array!
   These vars *must* be in the *exact* *same* *order* as each stateDef array entry!
   -----------------------------------------------------------------------------------------*/
enum {	stRTrough=SIM_FIRSTSTATE, stCTrough, stLTrough, stOuthole, stDrain,
      	stGearChange, stShooter, stBallLane, stROut, stLOut, stRIn, stLIn, stLSling, stRSling,
      	stLLoopUp, stLLoopUp2, stLLoopDn, stLLoopDn2,
	stRLoopUp, stRLoopUp2, stRLoopDn, stRLoopDn2,
	stLRampEnt, stLRampDiv, stLRampExit, stRRampEnt, stRRamp,
	stTunnel, stTopLock, stMidLock, stBotLock,
	stSuperCh1, stSuperCh2, stSuperCh3, stSuperCh4,
	stULoop1, stULoop2, stULoop3,
        stTopRed, stMidRed, stBotRed, stTopYellow, stMidYellow, stBotYellow, stTopGreen, stMidGreen, stBotGreen,
        stJet1, stJet2, stJet3,
        stRBankTop, stRBankMid, stRBankBot, stLBankTop, stLBankMid, stLBankBot
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

static sim_tState gw_stateDef[] = {
  {"Not Installed",    0,0,           0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",        0,0,           0,        0,           0,0,0,SIM_STNOTEXCL},

  /*Line 1*/
  {"Right Trough",     1,swRTrough,   sTrough,  stShooter,  5},
  {"Center Trough",    1,swCTrough,   0,        stRTrough,   1},
  {"Left Trough",      1,swLTrough,   0,        stCTrough,   1},
  {"Outhole",          1,swOuthole,   sOuthole, stLTrough,   5},
  {"Drain",            1,0,           0,        stOuthole,   0,0,0,SIM_STNOTEXCL},

  /*Line 2*/
  {"Gear Change",	1,0,0,0},
  {"Shooter",		1,swShooter,   sPlunger, stBallLane,  1,0,0,SIM_STNOTEXCL},
  {"Ball Lane",		1, 0,  	       0,        stULoop1,    10,0,0,SIM_STNOTEXCL},	/*Normally ball should enter upper loop*/
  {"Right Outlane",	1,swROut,      0,        stDrain,      20},
  {"Left Outlane",	1,swLOut,      0,        stDrain,      20, sKickBack, stTunnel},	/*Left Lane has a kickback*/
  {"Right Inlane",	1,swRIn,       0,        stFree,        5},
  {"Left  Inlane",	1,swLIn,       0,        stFree,        5},
  {"Left Slingshot",	1,swLSling,    0,	 stFree,	5},
  {"Rt Slingshot",	1,swRSling,    0,	 stFree,	5},

  /*Line 3*/
  {"Left Loop",        1,swLFreeBot,    0,       stLLoopUp2,    1},	/* Left Loop Going Up     */
  {"Left Loop",        1,swLFreeTop,    0,       stRRampEnt,    3},   	/* Left Loop Going Up #2 - Leads to Right Ramp Entrance! */
  {"Left Loop",        1,swLFreeTop,    0,       stLLoopDn2,    1},	/* Left Loop Coming Down */
  {"Left Loop",        1,swLFreeBot,    0,       stFree,    	3},   	/* Left Loop Coming Down2  */

  /*Line 4*/
  {"Right Loop",       1,swRFreeTop, 0,       stRLoopUp2,   1},	/* Right Loop Going Up     */
  {"Right Loop",       1,swRFreeBot, 0,       stLLoopDn,    3},	/* Right Loop Going Up #2  */
  {"Right Loop",       1,swRFreeTop, 0,       stRLoopDn2,   1},	/* Right Loop Coming Down  */
  {"Right Loop",       1,swRFreeBot, 0,       stFree,	    3},	/* Right Loop Coming Down2 */

  /*Line 5*/
  {"L. Ramp Enter",	1,swLRampEnt,   0,       stLRampDiv,	 2,0,0,SIM_STNOTEXCL},
  {"Ramp Diverter",	1,0,   		0,       stLRampExit,	 2, sDiverterLo, stSuperCh1,SIM_STNOTEXCL}, /*If Diverter firing, keep ball in Supercharger Loop, otherwise to Left Ramp Made!*/
  {"L. Ramp Exit",	1,swLRampMade,  0,	 stLIn, 	 10},

  {"Right Loop",   	1,0,  0,       0,	 0},
  {"R. Ramp",	   	1,swUDRampMade,  0,       stTopLock,	 5},

  /*Line 6*/
  {"Tunnel Shot",	1,swEjectHole,  	sEjectHole, 	stFree,	 0},
  {"Top Lock",        	1,swTopLock,  		0,	 	stMidLock,       1},
  {"Middle Lock",      	1,swMidLock,  		0,	 	stBotLock,       1},
  {"Bottom Lock",      	1,swBotLock,  		sLocker,	stRIn,		 1},
  {"SuperCharger \\",	1,swOpto1,		0,		stSuperCh2,	 0},
  {"SuperCharger |",	1,swOpto2,		0,		stSuperCh3,	 0},
  {"SuperCharger /",	1,swOpto3,		0,		stSuperCh4,	 0},
  {"SuperCharger -",	1,swOptoLoopMade,	0,		stLRampDiv,	 0,  sDiverterLo,  stSuperCh1,1}, /*If Lo Diverter is Firing.. Keep Ball in Supercharger Loop*/

  /*Line 7*/
  {"Upper Loop",	1,swBotLoop,		0,		stULoop2,	 1},
  {"Upper Loop",	1,swMidLoop,		0,		stULoop3,	 1},
  {"Upper Loop",	1,swTopLoop,		0,		stRRampEnt,	 1}, /*Leads to Right Ramp Entrance*/

  /*Line 8*/
  {"Top Green Tgt",	1,swTopGreen,		0,		stFree,		 1},
  {"Mid Green Tgt",	1,swMidGreen,		0,		stFree,		 1},
  {"Bot Green Tgt",	1,swBotGreen,		0,		stFree,		 1},
  {"Top Yellow Tgt",	1,swTopYellow,		0,		stFree,		 1},
  {"Mid Yellow Tgt",	1,swMidYellow,		0,		stFree,		 1},
  {"Bot Yellow Tgt",	1,swBotYellow,		0,		stFree,		 1},
  {"Top Red Target",	1,swTopRed,		0,		stFree,		 1},
  {"Mid Red Target",	1,swMidRed,		0,		stFree,		 1},
  {"Bot Red Target",	1,swBotRed,		0,		stFree,		 1},

  /*Line 9*/
  {"Left Bumper",	1,swJet1,	        0,		stFree,		1},
  {"Right Bumper",	1,swJet2,	        0,		stFree,		1},
  {"Bottom Bumper",	1,swJet3,	        0,		stFree,		1},

  /*Line10*/
  {"Rt Bank Top Tg",	1,swRBankTop,	        0,		stFree,		1},
  {"Rt Bank Mid Tg",	1,swRBankMid,	        0,		stFree,		1},
  {"Rt Bank Bot Tg",	1,swRBankBot,	        0,		stFree,		1},
  {"Lt Bank Top Tg",	1,swLBankTop,	        0,		stFree,		1},
  {"Lt Bank Mid Tg",	1,swLBankMid,	        0,		stFree,		1},
  {"Lt Bank Bot Tg",	1,swLBankBot,	        0,		stFree,		1},

  {0}
};

static int gw_handleBallState(sim_tBallStatus *ball, int *inports) {
	  switch (ball->state)
		{
		/*As Ball Approaches Right Ramp, if ramp is up.. it loops down playfield
		  otherwise, into the ramp!*/
		case stRRampEnt:
			if(locals.rampPos == UP)
				return setState(stRLoopDn,5);
			else
				return setState(stRRamp,5);
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

static sim_tInportData gw_inportData[] = {
  {0, 0x0005, stLRampEnt},
  {0, 0x0006, stLLoopUp},
  {0, 0x0009, stLOut},
  {0, 0x000a, stROut},
  {0, 0x0011, stLLoopUp},
  {0, 0x0012, stRLoopUp},
  {0, 0x0041, stLSling},
  {0, 0x0042, stRSling},
  {0, 0x0080, stJet1},
  {0, 0x0100, stJet3},
  {0, 0x0200, stJet2},
  {0, 0x0400, swGearHi, SIM_STANY},
  {0, 0x0800, stTunnel},
  {0, 0x1001, stLIn},
  {0, 0x1002, stRIn},
  {0, 0x2000, stRBankTop},
  {0, 0x4000, stRBankMid},
  {0, 0x8000, stRBankBot},
  {1, 0x0001, stLBankTop},
  {1, 0x0002, stLBankMid},
  {1, 0x0004, stLBankBot},
  {1, 0x0008, stTopGreen},
  {1, 0x0010, stMidGreen},
  {1, 0x0040, stBotGreen},
  {1, 0x0080, stTopYellow},
  {1, 0x0100, stMidYellow},
  {1, 0x0200, stBotYellow},
  {1, 0x0400, stTopRed},
  {1, 0x0800, stMidRed},
  {1, 0x1000, stBotRed},
  {1, 0x2000, stDrain},
  {1, 0x4000, stULoop1},
  {0}
};


/*--------------------
  Drawing information
 ---------------------*/
static core_tLampDisplay gw_lampPos = {
{ 0, 0 }, /* top left */
{39, 29}, /* size */
{
 {1,{{34,10,ORANGE}}},{1,{{35,12,ORANGE}}},{1,{{35,14,ORANGE}}},{1,{{35,16,ORANGE}}},
 {1,{{34,18,ORANGE}}},

 /*Lamp 6 - Matrix # 16 - Splits into 2 bulbs*/
 {2,{{10, 1,RED},{ 8,26,RED}}},

 {1,{{12, 2,ORANGE}}},

 /*Lamp 8 - Matrix # 18 - Splits into 2 bulbs*/
 {2,{{14, 3,GREEN},{15,17,GREEN}}},

 {1,{{31,10,GREEN}}},{1,{{32,12,GREEN}}},{1,{{32,14,ORANGE}}},{1,{{32,16,GREEN}}},
 {1,{{31,18,GREEN}}},{1,{{13,13,RED}}},{1,{{14,15,ORANGE}}},{1,{{16,19,WHITE}}},
 {1,{{ 3,15,RED}}},{1,{{ 4,16,YELLOW}}},{1,{{ 5,17,GREEN}}},{1,{{10,25,ORANGE}}},

 /*Lamp 21 - Matrix # 35 - Splits into 2 bulbs*/
 {2,{{28, 1,RED},{28,27,RED}}},

 {1,{{ 6,20,RED}}},{1,{{ 8,19,ORANGE}}},{1,{{10,18,RED}}},
 {1,{{30, 8,WHITE}}},{1,{{28, 7,WHITE}}},{1,{{26, 7,WHITE}}},{1,{{24, 7,WHITE}}},
 {1,{{22, 8,WHITE}}},{1,{{13,10,RED}}},{1,{{14,11,YELLOW}}},{1,{{15,12,GREEN}}},
 {1,{{37,14,RED}}},{1,{{30, 1,ORANGE}}},{1,{{22,20,ORANGE}}},{1,{{24,21,RED}}},
 {1,{{26,21,RED}}},{1,{{28,21,RED}}},{1,{{30,20,RED}}},{1,{{22,14,ORANGE}}},
 {1,{{26, 3,WHITE}}},{1,{{26,25,WHITE}}},

 /*Lamp 43 - Matrix # 63 - Splits into 2 bulbs
        44 - Matrix # 64
        45 - Matrix # 65                       */
 {2,{{23, 3,ORANGE},{23,25,ORANGE}}},
 {2,{{22, 4,ORANGE},{22,24,ORANGE}}},
 {2,{{21, 5,ORANGE},{21,23,ORANGE}}},

 {1,{{13, 7,YELLOW}}},{1,{{13, 6,RED}}},{1,{{39, 0,YELLOW}}},
 {1,{{17,16,RED}}},{1,{{18,18,RED}}},{1,{{ 8,22,RED}}},{1,{{ 9,22,YELLOW}}},
 {1,{{10,22,GREEN}}},{1,{{18,10,RED}}},{1,{{17,12,RED}}},{1,{{17,14,RED}}},
 {1,{{19,16,WHITE}}},{1,{{20,18,ORANGE}}},{1,{{ 9,10,RED}}},{1,{{ 9,11,YELLOW}}},
 {1,{{ 9,12,GREEN}}},{1,{{20,10,WHITE}}},{1,{{19,12,WHITE}}},{1,{{19,14,WHITE}}}
}
};
/*--------------------------------------------------------
  Code to draw the mechanical objects, and their states!
---------------------------------------------------------*/

static void gw_drawMech(BMTYPE **line) {
  core_textOutf(30, 0,BLACK,"Gear Shifter: %-6s", core_getSw(swGearLo) ? "Down  " : (core_getSw(swGearHi) ? "Up    " : "Middle"));
  core_textOutf(30, 10,BLACK,"Up/Down Ramp: %-6s", (locals.rampPos==UP)?"Up":"Down");
}
static void gw_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+I = L/R Inlane");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+O = L/R Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+L = L/R Freeway Loop");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+R = Lock/SuperCharger Ramp");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,110,BLACK,"T = Tunnel Hole, U = Upper Left Loop");
  core_textOutf(30,120,BLACK,"A/S/D = Top/Mid/Bottom Green Targets");
  core_textOutf(30,130,BLACK,"F/G/H = Top/Mid/Bottom Yellow Targets");
  core_textOutf(30,140,BLACK,"J/K/L = Top/Mid/Bottom Red Targets");
  core_textOutf(30,150,BLACK,"Z/X/C/V/B/N = Left/Right Bank Targets");
  core_textOutf(30,160,BLACK,"Space = Gear Down, LeftAlt = Gear Up");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(gw,l5,"getaw_l5.rom",0x80000,CRC(b97f3d62) SHA1(c87c36a2327561c50b37d587d3bd0782acf8860d))
WPCS_SOUNDROM8xx("u18snd",CRC(37bbe485) SHA1(e6b7ccef250db0c801e3dd8ebf93522b466ca1ec))
WPC_ROMEND

WPC_ROMSTART(gw,pc,"u6-p-c.rom",0x80000,CRC(2bd887e6) SHA1(fe06307f5c9b19be9a889be7027a4b0f399b505f))
WPCS_SOUNDROM8xx("u18-sp1.rom",CRC(fc5a5ff6) SHA1(bbe810135e05f81d1399ee0cb490ee93d6f9bb03))
WPC_ROMEND

WPC_ROMSTART(gw,l1,"gw_l1.u6",0x80000,CRC(cecf66cf) SHA1(f8286ee08402ce65b034e64fc777ead7bfe2fe13))
WPCS_SOUNDROM8xx("u18snd",CRC(37bbe485) SHA1(e6b7ccef250db0c801e3dd8ebf93522b466ca1ec))
WPC_ROMEND

WPC_ROMSTART(gw,l2,"get_l2.u6",0x80000,CRC(9efc0005) SHA1(037ede538fb2c6f18980484528680fe90ccbb355))
WPCS_SOUNDROM8xx("u18snd",CRC(37bbe485) SHA1(e6b7ccef250db0c801e3dd8ebf93522b466ca1ec))
WPC_ROMEND

WPC_ROMSTART(gw,l3,"get_l3.u6",0x80000,CRC(d10aaa44) SHA1(39ba198f9c4ce2419f8dbb8d90508e1beb524b5d))
WPCS_SOUNDROM8xx("u18snd",CRC(37bbe485) SHA1(e6b7ccef250db0c801e3dd8ebf93522b466ca1ec))
WPC_ROMEND
/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(gw,l5,"The Getaway: High Speed II (L-5)",1992, "Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(gw,pc,l5,"The Getaway: High Speed II (P-C)",1992,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(gw,l1,l5,"The Getaway: High Speed II (L-1)",1992,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(gw,l2,l5,"The Getaway: High Speed II (L-2)",1992,"Williams",wpc_mFliptronS,0)
CORE_CLONEDEF(gw,l3,l5,"The Getaway: High Speed II (L-3)",1992,"Williams",wpc_mFliptronS,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData gwSimData = {
  2,    			/* # game specific input ports */
  gw_stateDef,			/* Definition of all states */
  gw_inportData,		/* Keyboard Entries */
  { stRTrough, stCTrough, stLTrough, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL, 			/* no init */
  gw_handleBallState,		/*Function to handle ball state changes*/
  gw_drawStatic,		/*Function to handle mechanical state changes*/
  FALSE, 			/* Do Not Simulate manual shooter */
  NULL  			/* no custom key conditions */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData gwGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,	/* generation */
  {
    FLIP_SW(FLIP_L | FLIP_UR) | FLIP_SOL(FLIP_L | FLIP_UR),	/* Which switches are the flippers */
    0,0,0,0,0,0,0,
    NULL, gw_handleMech, NULL, gw_drawMech,
    &gw_lampPos, NULL
  },
  &gwSimData,
  {
    {0}, 			/* no serial number */
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}, /* Inverted Switches*/
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swGearLo},
  }
};

/*---------------
/  Game handling
/----------------*/
static void init_gw(void) {
  core_gameData = &gwGameData;
}

/***************************************************************************************************
 Functions here must manually adjust the state of mechanicla objects, usually checking for solenoids
 which control the state of those objects. Opening & Closing Doors, Diverter Switchs, etc..
****************************************************************************************************/
static void gw_handleMech(int mech) {
  /* -------------------------------------
     --	Raise & Lower Ramp  --
     ------------------------------------- */
  if (mech & 0x01) {
    /*-- if Ramp is up, & Ramp Down Solenoid firing lower it! --*/
    if (locals.rampPos == UP && core_getSol(sRampDown)) {
      locals.rampPos = DOWN;
      core_setSw(swRampDown,1); /*If Ramp Down, Switch Must be Active*/
    }
    /*-- if Ramp is down, & Ramp Up Solenoid firing raise it! --*/
    if (locals.rampPos == DOWN && core_getSol(sRampUp)) {
      locals.rampPos = UP;
      core_setSw(swRampDown,0); /*If Ramp Up, Switch Must be InActive*/
    }
  }
}
