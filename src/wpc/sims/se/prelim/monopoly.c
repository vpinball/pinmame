/*******************************************************************************
 Preliminary Monopoly (Stern, 2001) Pinball Simulator

 by Gerrit Volkenborn (gaston@yomail.de)
 Sep 22, 2004
 updated Dec 26, 2005 by Brian Smith (destruk@vpforums.com)

 Read PZ.c or FH.c if you like more help.

 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Monopoly Simulator:
  ------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
     Q  SDTM (Drain Ball)

   More to be added...

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "vpintf.h"
#include "se.h"
#include "dedmd.h"
#include "desound.h"

/*------------------
/  Local functions
/-------------------*/
static int  monopoly_handleBallState(sim_tBallStatus *ball, int *inports);
static void monopoly_drawStatic(BMTYPE **line);
static void init_monopoly(void);
static void monopoly_drawMech(BMTYPE **line);
static int  monopoly_getMech(int mech);
static void monopoly_handleMech(int mech);

// The last used selocals variable is "flipsolPulse", so we can forget about the rest.
extern struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int	 flipsol, flipsolPulse;
} selocals;

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int flipperPos;
  int flipperDir;
  int flipperSpeed;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
SE_INPUT_PORTS_START(monopoly,4)

  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"",		        KEYCODE_R)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",		KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"",			KEYCODE_W)
    COREPORT_BIT(0x0080,"",			KEYCODE_E)
    COREPORT_BIT(0x0100,"",			KEYCODE_R)
    COREPORT_BIT(0x0200,"",			KEYCODE_T)
    COREPORT_BIT(0x0400,"",			KEYCODE_Y)
    COREPORT_BIT(0x0800,"",			KEYCODE_U)
    COREPORT_BIT(0x1000,"",			KEYCODE_I)
    COREPORT_BIT(0x2000,"",			KEYCODE_O)
    COREPORT_BIT(0x4000,"",			KEYCODE_A)
    COREPORT_BIT(0x8000,"Drain",			KEYCODE_Q)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"",			KEYCODE_S)
    COREPORT_BIT(0x0002,"",			KEYCODE_D)
    COREPORT_BIT(0x0004,"",			KEYCODE_F)
    COREPORT_BIT(0x0008,"",			KEYCODE_G)
    COREPORT_BIT(0x0010,"",			KEYCODE_H)
    COREPORT_BIT(0x0020,"",			KEYCODE_J)
    COREPORT_BIT(0x0040,"",			KEYCODE_K)
    COREPORT_BIT(0x0080,"",			KEYCODE_L)
    COREPORT_BIT(0x0100,"",			KEYCODE_Z)
    COREPORT_BIT(0x0200,"",			KEYCODE_X)
    COREPORT_BIT(0x0400,"",			KEYCODE_C)
    COREPORT_BIT(0x0800,"",			KEYCODE_V)
    COREPORT_BIT(0x1000,"",			KEYCODE_B)
    COREPORT_BIT(0x2000,"",			KEYCODE_N)
    COREPORT_BIT(0x4000,"",			KEYCODE_M)
    COREPORT_BIT(0x8000,"",			KEYCODE_COMMA)

SE_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
/* Standard Switches */
#define swStart		54
#define swSlamTilt	55
#define swTilt		56

/* Other switches */
#define swTrough1	11
#define swTrough2	12
#define swTrough3	13
#define swTrough4	14
#define swTroughJam	15
#define swShooter	16
#define swLeftOutlane	57
#define swLeftInlane	58
#define swLeftSling		59
#define swRightOutlane	60
#define swRightInlane	61
#define swRightSling	62

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough		1
#define sLaunch		2
#define sLeftSling	17
#define sRightSling	18

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling
	  };

static sim_tState monopoly_stateDef[] = {
  {"Not Installed",	0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",		0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 4",		1,swTrough4,	0,		stTrough3,	1},
  {"Trough 3",		1,swTrough3,	0,		stTrough2,	1},
  {"Trough 2",		1,swTrough2,	0,		stTrough1,	1},
  {"Trough 1",		1,swTrough1,	sTrough,	stTrough,	1},
  {"Trough Jam",	1,swTroughJam,  0,		stShooter,	1},
  {"Drain",		1,0,		0,		stTrough4,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",		1,swShooter,	 sLaunch,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",		1,0,		 0,		stFree,		7,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",	1,swRightOutlane,0,		stDrain,	15},
  {"Left Outlane",	1,swLeftOutlane, 0,		stDrain,	15},
  {"Right Inlane",	1,swRightInlane, 0,		stFree,		5},
  {"Left Inlane",	1,swLeftInlane,	 0,		stFree,		5},
  {"Left Slingshot",	1,swLeftSling,	 0,		stFree,		1},
  {"Rt Slingshot",	1,swRightSling,	 0,		stFree,		1},

  /*Line 3*/

  /*Line 4*/

  /*Line 5*/

  /*Line 6*/

  {0}
};

static int monopoly_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
  {
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData monopoly_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
//  {0, 0x0040, },
//  {0, 0x0080, },
//  {0, 0x0100, },
//  {0, 0x0200, st},
//  {0, 0x0400, st},
//  {0, 0x0800, st},
//  {0, 0x1000, st},
//  {0, 0x2000, st},
//  {0, 0x4000, st},
  {0, 0x8000, stDrain},

/* Port 1 */
//  {1, 0x0001, st},
//  {1, 0x0002, st},
//  {1, 0x0004, st},
//  {1, 0x0008, st},
//  {1, 0x0010, st},
//  {1, 0x0020, st},
//  {1, 0x0040, st},
//  {1, 0x0080, st},
//  {1, 0x0100, st},
//  {1, 0x0200, st},
//  {1, 0x0400, st},
//  {1, 0x0800, st},
//  {1, 0x1000, st},
//  {1, 0x2000, st},
//  {1, 0x4000, st},
//  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void monopoly_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"Q = Drain Ball");
  core_textOutf(30,100,BLACK,"");
  core_textOutf(30,110,BLACK,"");
  core_textOutf(30,120,BLACK,"");
  core_textOutf(30,130,BLACK,"      *** PRELIMINARY ***");
  core_textOutf(30,140,BLACK,"");
  core_textOutf(30,150,BLACK,"");
  core_textOutf(30,160,BLACK,"");
}

/*-----------------
/  ROM definitions
/------------------*/

/*-------------------------------------------------------------------
/ Monopoly (3.20)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopoly,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(        "mondsp-a.301",CRC(c4e2e032) SHA1(691f7b6ed0616338683f7e3f316d64a70db58dd4))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND

/*-------------------------------------------------------------------
/ Monopoly (3.03)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopole,"moncpu.303",CRC(4a66c9e4) SHA1(a368b0ced32f1017e781a59108670b979b50c9d7))
DE_DMD32ROM8x(        "mondsp-a.301",CRC(c4e2e032) SHA1(691f7b6ed0616338683f7e3f316d64a70db58dd4))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_monopole input_ports_monopoly
#define init_monopole init_monopoly

#ifdef TEST_NEW_SOUND
/*-------------------------------------------------------------------
/ Monopoly (3.20) (ARM7 Sound Board)
/-------------------------------------------------------------------*/
SE128_ROMSTART(mononew,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(         "mondsp-a.301",CRC(c4e2e032) SHA1(691f7b6ed0616338683f7e3f316d64a70db58dd4))
DE3S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                       "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                       "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                       "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_mononew input_ports_monopoly
#define init_mononew init_monopoly
CORE_CLONEDEFNV(mononew,monopoly,"Monopoly (ARM7 Sound Board)",2002,"Stern",de_mSES3,GAME_NOCRC)
#endif


/*-------------------------------------------------------------------
/ Monopoly (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopolf,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(        "mondsp-f.301",CRC(e78b1998) SHA1(bd022dc90b55374baed17360fad7bf0f89e2ee33))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_monopolf input_ports_monopoly
#define init_monopolf init_monopoly

/*-------------------------------------------------------------------
/ Monopoly (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopolg,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(        "mondsp-g.301",CRC(aab48728) SHA1(b9ed8574ac463a5fc21dc5f41d090cf0ad3f8362))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_monopolg input_ports_monopoly
#define init_monopolg init_monopoly

/*-------------------------------------------------------------------
/ Monopoly (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopoli,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(        "mondsp-i.301",CRC(32431b3c) SHA1(6266e17e705bd50d2358d9f7c0168de51aa13750))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_monopoli input_ports_monopoly
#define init_monopoli init_monopoly

/*-------------------------------------------------------------------
/ Monopoly (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(monopoll,"moncpu.320",CRC(6c107c8b) SHA1(236d85b971c70a30e663787d643e6d589591d582))
DE_DMD32ROM8x(        "mondsp-s.301",CRC(9f70dad6) SHA1(bf4b1c579b4bdead51e6b34de81fe65c45b6596a))
DE2S_SOUNDROM1888(     "mnsndu7.100",CRC(400442e7) SHA1(d6c075dc439d5366b7ae71b5a523b86543b1ecd6),
                      "mnsndu17.100",CRC(f9bc55e8) SHA1(7dc41521305021961927ebde4dcf22611e3d622d),
                      "mnsndu21.100",CRC(e0727e1f) SHA1(2093dba6e2f59cd1d1fc49c8d995b603ea0913ba),
                      "mnsndu36.100",CRC(c845aa97) SHA1(2632aa8c5576b7afcb96693fa524c7d0350ac9a8))
SE_ROMEND
#define input_ports_monopoll input_ports_monopoly
#define init_monopoll init_monopoly

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEFNV(monopoly,"Monopoly (3.20)",2001,"Stern",de_mSES1,GAME_NOCRC)
CORE_CLONEDEFNV(monopole,monopoly,"Monopoly (3.03)",2002,"Stern",de_mSES1,GAME_NOCRC)
CORE_CLONEDEFNV(monopolf,monopoly,"Monopoly (France)",2002,"Stern",de_mSES1,GAME_NOCRC)
CORE_CLONEDEFNV(monopolg,monopoly,"Monopoly (Germany)",2002,"Stern",de_mSES1,GAME_NOCRC)
CORE_CLONEDEFNV(monopoli,monopoly,"Monopoly (Italy)",2002,"Stern",de_mSES1,GAME_NOCRC)
CORE_CLONEDEFNV(monopoll,monopoly,"Monopoly (Spain)",2002,"Stern",de_mSES1,GAME_NOCRC)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData monopolySimData = {
  2,    				/* 2 game specific input ports */
  monopoly_stateDef,	/* Definition of all states */
  monopoly_inportData,	/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed */
  NULL, 				/* no init */
  monopoly_handleBallState,	/*Function to handle ball state changes */
  monopoly_drawStatic,	/* Function to handle mechanical state changes */
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static struct core_dispLayout dispMonopoly[] = {
  { 0, 0,32,128,CORE_DMD, (void *)dedmd32_update},
  {34,10, 7, 15,CORE_DMD|CORE_DMDNOAA, (void *)seminidmd2_update},
  {0}
};
static core_tGameData monopolyGameData = {
  GEN_WS, dispMonopoly,
  {
    FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    0, 2, 0, 0, SE_MINIDMD, 0, 0,
    0, 0, monopoly_getMech, monopoly_drawMech
  },
  &monopolySimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* inverted switches */
    /*Start    Tilt    SlamTilt */
    { swStart, swTilt, swSlamTilt },
  }

};

/*-- Solenoids --*/
static WRITE_HANDLER(monopoly_w) {
  static const int solmaskno[] = { 8, 0, 16, 24 };
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];
  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    selocals.flipsol |= selocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  selocals.solenoids |= sols;
  if (offset == 3) {
    locals.flipperDir = ((data & 0x04) >> 1) - 1; // so +1 for cw, -1 for ccw
	if (data & 0x01) { // increase flipper speed if set
      if (locals.flipperSpeed < 4) locals.flipperSpeed++;
    } else { // decrease flipper speed if not set
      if (locals.flipperSpeed) locals.flipperSpeed--;
    }
    locals.flipperPos += locals.flipperDir * locals.flipperSpeed;
    if (locals.flipperPos < 0) locals.flipperPos += 5000;
    if (locals.flipperPos > 4999) locals.flipperPos -= 5000;
#ifndef VPINMAME // must be disabled, as the switch is set by script in VPM
    core_setSw(30, locals.flipperPos < 100 || locals.flipperPos > 4900);
#endif
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_monopoly(void) {
  core_gameData = &monopolyGameData;
  install_mem_write_handler(0, 0x2000, 0x2003, monopoly_w);
}

static void monopoly_drawMech(BMTYPE **line) {
  core_textOutf(30,  0,BLACK,"WaterWorks Flipper");
  core_textOutf(30, 10,BLACK,"pos:%4d, speed:%4d", monopoly_getMech(0), monopoly_getMech(1));
}

static void monopoly_handleMech(int mech) {
}

static int monopoly_getMech(int mechNo){
  static int speedCnt;
  static int oldSpeed[8];
  static int oldFlipperPos;
  int speed, dist;
  switch (mechNo) {
    case 0: return locals.flipperPos /5;
    case 1:
      dist = locals.flipperPos - oldFlipperPos;
      if (dist < 0) dist = - dist;
      if (dist > 2500) dist -= 5000;
      if (dist < 0) dist = - dist;
      oldFlipperPos = locals.flipperPos;
      oldSpeed[speedCnt = (speedCnt + 1) % 8] = locals.flipperDir * dist;
      speed = (oldSpeed[0] + oldSpeed[1] + oldSpeed[2] + oldSpeed[3] + oldSpeed[4] + oldSpeed[5] + oldSpeed[6] + oldSpeed[7]) / 8;
      return speed / 3;
  }
  return 0;
}
