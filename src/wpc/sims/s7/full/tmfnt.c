/*******************************************************************************
 Time Fantasy (Williams, 1982) Pinball Simulator

 by Marton Larrosa (marton@mail.com)
 Feb. 28, 2001.

 Known Issues/Problems with this Simulator:

 None.

 Changes:

 280201 - First version.
 110704 - Added special solenoids to game definition (TomB)

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Time Fantasy Simulator:
  -----------------------------------
      +I  L/R Inlane
      +O  L/R Outlane
      +-  L/R Slingshot
      +"  L/R Lower 10Pts. Standups
      +[  L/R Middle 10Pts. Standups
      +=  L/R Top 10Pts. Standups
      +L  LR/RL Loop
       Q  SDTM (Drain Ball)
     WER  Jet Bumpers
   TYUIO  Top 1-5 Rollovers
 ASDFGHJ  F/A/N/T/A/S/Y Targets
       K  Ramp Target
       L  Left Lane
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s7.h"
#include "sim.h"

/*------------------
/  Local functions
/-------------------*/
static void init_tmfnt(void);
static void tmfnt_drawMech(BMTYPE **line);
static int  tmfnt_handleBallState(sim_tBallStatus *ball, int *inports);

/*--------------------------
/ Game specific input ports
/---------------------------*/
S7_INPUT_PORTS_START(tmfnt,1)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"Lower Standups",	KEYCODE_EQUALS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"Left Jet",		KEYCODE_W)
    COREPORT_BIT(0x0080,"Right Jet",		KEYCODE_E)
    COREPORT_BIT(0x0100,"Bottom Jet",		KEYCODE_R)
    COREPORT_BIT(0x0200,"Middle Standups",	KEYCODE_OPENBRACE)
    COREPORT_BIT(0x0400,"Top Standups",		KEYCODE_QUOTE)
    COREPORT_BIT(0x0800,"LR/RL Loop",		KEYCODE_L)
    COREPORT_BIT(0x1000,"Ramp Target",		KEYCODE_K)
    COREPORT_BIT(0x2000,"Left Lane",		KEYCODE_L)
    COREPORT_BIT(0x4000,"Drain",			KEYCODE_Q)

  PORT_START
    COREPORT_BIT(0x0001,"1",			KEYCODE_T)
    COREPORT_BIT(0x0002,"2",			KEYCODE_Y)
    COREPORT_BIT(0x0004,"3",			KEYCODE_U)
    COREPORT_BIT(0x0008,"4",			KEYCODE_I)
    COREPORT_BIT(0x0010,"5",			KEYCODE_O)
    COREPORT_BIT(0x0020,"F",			KEYCODE_A)
    COREPORT_BIT(0x0040,"A",			KEYCODE_S)
    COREPORT_BIT(0x0080,"N",			KEYCODE_D)
    COREPORT_BIT(0x0100,"T",			KEYCODE_F)
    COREPORT_BIT(0x0200,"A",			KEYCODE_G)
    COREPORT_BIT(0x0400,"S",			KEYCODE_H)
    COREPORT_BIT(0x0800,"Y",			KEYCODE_J)

S7_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt		 1
#define swStart		 3
#define swSlamTilt	 7
#define swFantasy	 9
#define swfAntasy	10
#define swfaNtasy	11
#define swfanTasy	12
#define swfantAsy	13
#define swfantaSy	14
#define swfantasY	15
#define swLeftLoop	16
#define swRightLoop	17
#define swLeftOutlane	18
#define swRightOutlane	19
#define swLeftInlane	20
#define swRightInlane	21
#define sw1		22
#define sw2		23
#define sw3		24
#define sw4		25
#define sw5		26
#define swLLStandup	27
#define swULStandup	28
#define swUTRStandup	29
#define swUMRStandup	30
#define swUBRStandup	31
#define swLRStandup	32
#define swLeftJet	33
#define swRightJet	34
#define swBottomJet	35
#define swRampTarget	36
#define swLeftSling	37
#define swRightSling	38
#define swOuthole	39
#define swLaneChange	40
#define swPFTilt	41
#define swLeftLane	42

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOuthole	1
#define sLeftJet	17
#define sRightJet	18
#define sLeftKicker	19
#define sRightKicker	20
#define sBottomJet	21

/*---------------------
/  Ball state handling
/----------------------*/
enum {stOuthole=SIM_FIRSTSTATE, stShooter, stBallLane, stNotEnough,
     stDrain, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane,
     stLeftSling, stRightSling, stLeftJet, stRightJet, stBottomJet, stLLStandup, stLRStandup, stULStandup, stUTRStandup, stUMRStandup, stUBRStandup,
     stLeftLoop, stLeftLoop2, stRightLoop, stRightLoop2,
     stFantasy, stfAntasy, stfaNtasy, stfanTasy, stfantAsy, stfantaSy, stfantasY, stRampTarget, stLeftLane,
     st1, st2, st3, st4, st5, stLJet, stRJet, stBJet
};

static sim_tState tmfnt_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Outhole",		1,swOuthole,	 sOuthole,	stShooter,	1},
  {"Shooter",		1,0,		 sShooterRel,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		0,		2,	0,	0,	SIM_STNOTEXCL},
  {"No Strength",	1,0,		 0,		stShooter,	3},

  /*Line 2*/
  {"Drain",		1,0,		 0,		stOuthole,	0,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},

  /*Line 3*/
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},
  {"Left Bumper",	1,swLeftJet,	 0,		stFree,		1},
  {"Right Bumper",	1,swRightJet,	 0,		stFree,		1},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		1},
  {"L. Left 10 Pts",	1,swLLStandup,	 0,		stFree,		1},
  {"L. Rt. 10 Pts.",	1,swLRStandup,	 0,		stFree,		1},
  {"U. Left 10 Pts",	1,swULStandup,	 0,		stFree,		1},
  {"U. Rt. 10 Pts.",	1,swUTRStandup,	 0,		stFree,		1},
  {"U.M.Rt. 10 Pts",	1,swUMRStandup,	 0,		stFree,		1},
  {"U.B.Rt. 10 Pts",	1,swUBRStandup,	 0,		stFree,		1},

  /*Line 4*/
  {"Left Loop",		1,swLeftLoop,	 0,		stLeftLoop2,	3},
  {"Right Loop",	1,swRightLoop,	 0,		stFree,		5},
  {"Right Loop",	1,swRightLoop,	 0,		stRightLoop2,	3},
  {"Left Loop",		1,swLeftLoop,	 0,		stFree,		5},

  /*Line 5*/
  {"Fantasy - F",	1,swFantasy, 	 0,		stFree,		3},
  {"Fantasy - A",	1,swfAntasy, 	 0,		stFree,		3},
  {"Fantasy - N",	1,swfaNtasy, 	 0,		stFree,		3},
  {"Fantasy - T",	1,swfanTasy, 	 0,		stFree,		3},
  {"Fantasy - A",	1,swfantAsy, 	 0,		stFree,		3},
  {"Fantasy - S",	1,swfantaSy, 	 0,		stFree,		3},
  {"Fantasy - Y",	1,swfantasY, 	 0,		stFree,		3},
  {"Ramp Target",	1,swRampTarget,	 0,		stFree,		7},
  {"Left Lane",		1,swLeftLane,	 0,		st3,		8},

  /*Line 6*/
  {"Top 1 Rollover",	1,sw1,		 0,		stLJet, 	5},
  {"Top 2 Rollover",	1,sw2,		 0,		stLJet, 	5},
  {"Top 3 Rollover",	1,sw3,		 0,		stLJet, 	5},
  {"Top 4 Rollover",	1,sw4,		 0,		stLJet, 	5},
  {"Top 5 Rollover",	1,sw5,		 0,		stLJet, 	5},
  {"Left Bumper",	1,swLeftJet,	 0,		stRJet,		3},
  {"Right Bumper",	1,swRightJet,	 0,		stBJet,		3},
  {"Bottom Bumper",	1,swBottomJet,	 0,		stFree,		3},

  {0}
};

static int tmfnt_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
	{

	/* Ball in Shooter Lane */
    	case stBallLane:
		if (ball->speed < 7)
			return setState(stNotEnough,7);
		if (ball->speed < 15)
			return setState(st5,15);
		if (ball->speed < 25)
			return setState(st4,25);
		if (ball->speed < 35)
			return setState(st3,35);
		if (ball->speed < 45)
			return setState(st2,45);
		if (ball->speed < 51)
			return setState(st1,51);
		break;

	}
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData tmfnt_inportData[] = {

/* Port 0 */
  {0, 0x0005, stLeftSling},
  {0, 0x0006, stRightSling},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLLStandup},
  {0, 0x0012, stLRStandup},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0040, stLeftJet},
  {0, 0x0080, stRightJet},
  {0, 0x0100, stBottomJet},
  {0, 0x0201, stUBRStandup},
  {0, 0x0202, stULStandup},
  {0, 0x0401, stUTRStandup},
  {0, 0x0402, stUMRStandup},
  {0, 0x0801, stLeftLoop},
  {0, 0x0802, stRightLoop},
  {0, 0x1000, stRampTarget},
  {0, 0x2000, stLeftLane},
  {0, 0x4000, stDrain},

  {1, 0x0001, st1},
  {1, 0x0002, st2},
  {1, 0x0004, st3},
  {1, 0x0008, st4},
  {1, 0x0010, st5},
  {1, 0x0020, stFantasy},
  {1, 0x0040, stfAntasy},
  {1, 0x0080, stfaNtasy},
  {1, 0x0100, stfanTasy},
  {1, 0x0200, stfantAsy},
  {1, 0x0400, stfantaSy},
  {1, 0x0800, stfantasY},

  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void tmfnt_drawMech(BMTYPE **line) {

/* Help */

  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+L = LR/RL Loop");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+' = L/R Lower 10Pt. Standup");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+] = L/R Mid. 10Pt. Standup");
  core_textOutf(30,100,BLACK,"L/R Ctrl+= = L/R Upper 10Pt. Standup");
  core_textOutf(30,110,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30,120,BLACK,"Q = Drain Ball, W/E/R = Jet Bumpers");
  core_textOutf(30,130,BLACK,"T/Y/U/I/O = Top 1-5 Rollover");
  core_textOutf(30,140,BLACK,"A/S/D/F/G/H/J = F/A/N/T/A/S/Y Target");
  core_textOutf(30,150,BLACK,"K = Ramp Target");
  core_textOutf(30,160,BLACK,"L = Left Lane");
}

/*-----------------
/  ROM definitions
/------------------*/
S7_ROMSTART8088(tmfnt,l5, "ic14.716",   CRC(56b8e5ad) SHA1(84d6ab59032282cdccb3bdce0365c1fc766d0e5b),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(0f86947c) SHA1(e775f44b4ca5dae5ec2626fa84fae83c4f0c5c33))
S67S_SOUNDROMS8(          "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S7_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(tmfnt,l5,"Time Fantasy (L-5)",1982,"Williams",s7_mS7S,0)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData tmfntSimData = {
  2,   			/* 2 game specific input ports */
  tmfnt_stateDef,	/* Definition of all states */
  tmfnt_inportData,	/* Keyboard Entries */
  { stOuthole, stDrain, stDrain, stDrain, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed*/
  NULL,			/* Simulator Init */
  tmfnt_handleBallState,/*Function to handle ball state changes*/
  NULL,			/* Function to handle mechanical state changes*/
  TRUE, 		/* Simulate manual shooter? */
  NULL  		/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData tmfntGameData = {
  GEN_S7, s7_dispS7,
  {
    0,
    0,0,0,0,0,0,0,
    NULL, NULL, NULL, tmfnt_drawMech,
    NULL, NULL
  },
  &tmfntSimData,
  {{ 0 }},
  {0,{33,34,37,38,35,0,0,0}}
};


/*---------------
/  Game handling
/----------------*/
static void init_tmfnt(void) {
  core_gameData = &tmfntGameData;
}
