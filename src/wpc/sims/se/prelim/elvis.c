/*******************************************************************************
 Preliminary Elvis (Stern, 2004) Pinball Simulator

 by Gerrit Volkenborn (gaston@yomail.de)
 Aug 19, 2007

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
static int  elvis_handleBallState(sim_tBallStatus *ball, int *inports);
static void elvis_drawStatic(BMTYPE **line);
static void init_elvis(void);
static void elvis_drawMech(BMTYPE **line);
static int  elvis_getMech(int mech);
static void elvis_handleMech(int mech);

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int lastPos;
  int direction, position, arms, legs;
} locals;

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

/*--------------------------
/ Game specific input ports
/---------------------------*/
SE_INPUT_PORTS_START(elvis,4)

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

static sim_tState elvis_stateDef[] = {
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

static int elvis_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state)
  {
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData elvis_inportData[] = {

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
static void elvis_drawStatic(BMTYPE **line) {

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
/ Elvis (5.00)
/-------------------------------------------------------------------*/
SE128_ROMSTART(elvis, "elvscpua.500", CRC(aae9d65d) SHA1(d6e789f7257448e97697c406561c14a0abc45187))
DE_DMD32ROM8x(        "elvsdspa.500", CRC(76a672cb) SHA1(8a9d4ac9538f0f91f2e95800147478cbaeb152a5))
DE3SA_SOUNDROM18888(  "elvis.u7", CRC(1df6c1b5) SHA1(7a9ebfc555e54ce92ad140ac6fcb82d9848ad8a6),
                      "elvis.u17",CRC(ff032897) SHA1(bf347c26a450bc07cdc94fc582dedf3a0cdc2a1b),
                      "elvis.u21",CRC(c3c19a40) SHA1(97f7f36eed62ca389c770bf5d746721724e17250),
                      "elvis.u36",CRC(e98f0dc9) SHA1(6dbab09435e993fef97d6a80a73675723bea7c1d),
                      "elvis.u37",CRC(88ba0966) SHA1(43ea198c9fcdc1c396d4180308042c6c08311829))
SE_ROMEND

/*-------------------------------------------------------------------
/ Elvis (Spain)
/-------------------------------------------------------------------*/
SE128_ROMSTART(elvisl, "elvscpul.500", CRC(d6813d67) SHA1(863a1b5bc62eca218f64d9bae24b205e1a8e2b6c))
DE_DMD32ROM8x(         "elvsdspl.500", CRC(68946b3b) SHA1(5764a5f6779097acfcf82eb176f6d966f6bb6988))
DE3SA_SOUNDROM18888(   "elvisl.u7", CRC(f0d70ee6) SHA1(9fa2c9d7b3690ec0c17645be066496d6833da5d1),
                       "elvisl.u17",CRC(2f86bcda) SHA1(73972fd30e84a2f97478f076cc8771c501440be5),
                       "elvisl.u21",CRC(400c7174) SHA1(a4fa0d51b7c11e70f6b93068a6bf859cdf3359c3),
                       "elvisl.u36",CRC(01ebbdbe) SHA1(286fa471b20b6ffcb0114d66239ab6aebe9bca9d),
                       "elvisl.u37",CRC(bed26746) SHA1(385cb77ec7599b12a4b021c53b42b8e9b9fb08a8))
SE_ROMEND
#define input_ports_elvisl input_ports_elvis
#define init_elvisl init_elvis

/*-------------------------------------------------------------------
/ Elvis (Germany)
/-------------------------------------------------------------------*/
SE128_ROMSTART(elvisg, "elvscpug.500", CRC(1582dd3a) SHA1(9a5d044dbad03e3ec2358ef16d983195761cb17b))
DE_DMD32ROM8x(         "elvsdspg.500", CRC(4b6e7d37) SHA1(259a5d0d11392f05504d4477cf03f2a270db670c))
DE3SA_SOUNDROM18888(   "elvisg.u7", CRC(1085bd7c) SHA1(2c34ee7d7c44906b0894c0c01b0fad74cb0d2a32),
                       "elvisg.u17",CRC(8b888d75) SHA1(b8c654d0fb558c205c338be2b458cbf931b23bac),
                       "elvisg.u21",CRC(79955b60) SHA1(36ad9e487408c9fd26641d484490b1b3bc8e1194),
                       "elvisg.u36",CRC(25ba1ad4) SHA1(1e1a846af4ff43bb47f081b0cc179cd732c0bbea),
                       "elvisg.u37",CRC(f6d7a2a0) SHA1(54c160a298c7ead1fe0404bce51bc16211da82cf))
SE_ROMEND
#define input_ports_elvisg input_ports_elvis
#define init_elvisg init_elvis

/*-------------------------------------------------------------------
/ Elvis (France)
/-------------------------------------------------------------------*/
SE128_ROMSTART(elvisf, "elvscpuf.500", CRC(e977fdb0) SHA1(aa24b4e6c461188b330bbc01204af8bfb03a9abf))
DE_DMD32ROM8x(         "elvsdspf.500", CRC(e4ce2da7) SHA1(030d9200844fc47d5ea6c4afeab0851de5b42b23))
DE3SA_SOUNDROM18888(   "elvisf.u7", CRC(84a057cd) SHA1(70e626f13a164df184dc5b0c79e8d320eeafb13b),
                       "elvisf.u17",CRC(9b13e40d) SHA1(7e7eac1be5cbc7bde4296d168a1cc0716bcb293a),
                       "elvisf.u21",CRC(5b668b6d) SHA1(9b104af5df5cc21c2504760b119f3e6584a1871b),
                       "elvisf.u36",CRC(03ee0c04) SHA1(45a994589e3d9e6fe971db8722848b5f7432b675),
                       "elvisf.u37",CRC(aa265440) SHA1(36b13ef0be4203936d9816e521098e72d6b4e4c1))
SE_ROMEND
#define input_ports_elvisf input_ports_elvis
#define init_elvisf init_elvis

/*-------------------------------------------------------------------
/ Elvis (Italy)
/-------------------------------------------------------------------*/
SE128_ROMSTART(elvisi, "elvscpui.500", CRC(11bafdbb) SHA1(51036cdf4beda20f2680fff7cd8cae25219406bd))
DE_DMD32ROM8x(         "elvsdspi.500", CRC(3ecbadb2) SHA1(589cf6de348359944585b718b2289dd70676807a))
DE3SA_SOUNDROM18888(   "elvisi.u7", CRC(8c270da4) SHA1(6a21332fdd1f2714aa78a1730e0f90159022ad1c),
                       "elvisi.u17",CRC(bd2e6580) SHA1(dc8c974860498d5766dbb0881cc9d6866c9a98a1),
                       "elvisi.u21",CRC(1932a22b) SHA1(864d6bc2c60e763431fd19511dc1a946cf131d63),
                       "elvisi.u36",CRC(df6772d7) SHA1(96e98ff4e93fc0c6fb2d9924da99b97f0c436c44),
                       "elvisi.u37",CRC(990fd624) SHA1(d5e104485dc8dd7386d8f3e7d99dc6cf7bf91568))
SE_ROMEND
#define input_ports_elvisi input_ports_elvis
#define init_elvisi init_elvis

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEFNV(elvis,"Elvis (5.00)",2004,"Stern",de_mSES3,GAME_IMPERFECT_SOUND | GAME_NOCRC)
CORE_CLONEDEFNV(elvisl,elvis,"Elvis (Spain)",2004,"Stern",de_mSES3,GAME_IMPERFECT_SOUND | GAME_NOCRC)
CORE_CLONEDEFNV(elvisg,elvis,"Elvis (Germany)",2004,"Stern",de_mSES3,GAME_IMPERFECT_SOUND | GAME_NOCRC)
CORE_CLONEDEFNV(elvisf,elvis,"Elvis (France)",2004,"Stern",de_mSES3,GAME_IMPERFECT_SOUND | GAME_NOCRC)
CORE_CLONEDEFNV(elvisi,elvis,"Elvis (Italy)",2004,"Stern",de_mSES3,GAME_IMPERFECT_SOUND | GAME_NOCRC)

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData elvisSimData = {
  2,    				/* 2 game specific input ports */
  elvis_stateDef,	/* Definition of all states */
  elvis_inportData,	/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed */
  NULL, 				/* no init */
  elvis_handleBallState,	/*Function to handle ball state changes */
  elvis_drawStatic,	/* Function to handle mechanical state changes */
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static struct core_dispLayout dispElvis[] = {
  { 0, 0,32,128,CORE_DMD, (void *)dedmd32_update},
};
static core_tGameData elvisGameData = {
  GEN_WS, dispElvis,
  {
    FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    0, 2, 0, 0, 0, 0, 0,
    0, 0, elvis_getMech, elvis_drawMech
  },
  &elvisSimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* inverted switches */
    /*Start    Tilt    SlamTilt */
    { swStart, swTilt, swSlamTilt },
  }

};

/*-- Solenoids --*/
static WRITE_HANDLER(elvis_w) {
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
    int pos = data & 0x0f;
	locals.legs = (data & 0x10) ? 1 : 0;
	locals.arms = (data & 0x20) ? 1 : 0;
    if (pos) {
      if (locals.lastPos != pos) {
        if ((locals.lastPos == 0x0c && pos == 0x06) || (locals.lastPos == 0x06 && pos == 0x03) || (locals.lastPos == 0x03 && pos == 0x09) || (locals.lastPos == 0x09 && pos == 0x0c)) {
          locals.position++;
		  locals.direction = 1;
        } else if ((locals.lastPos == 0x06 && pos == 0x0c) || (locals.lastPos == 0x0c && pos == 0x09) || (locals.lastPos == 0x09 && pos == 0x03) || (locals.lastPos == 0x03 && pos == 0x06)) {
          locals.direction = -1;
          if (locals.position) locals.position--;
        }
        locals.lastPos = pos;
      }
#ifndef VPINMAME // must be disabled, as the switch is set by script in VPM
      core_setSw(33, !locals.position);
#endif
    } else {
      locals.direction = 0;
    }
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_elvis(void) {
  core_gameData = &elvisGameData;
  install_mem_write_handler(0, 0x2000, 0x2003, elvis_w);
}

static void elvis_drawMech(BMTYPE **line) {
  core_textOutf(30,  0,BLACK,"Dancing Elvis");
  core_textOutf(30, 10,BLACK,"direction:%3d", elvis_getMech(0));
  core_textOutf(30, 20,BLACK,"arms:    %s",   elvis_getMech(1) ? "  Up" : "Down");
  core_textOutf(30, 30,BLACK,"legs:    %s",   elvis_getMech(2) ? "  Up" : "Down");
  core_textOutf(30, 40,BLACK,"position: %3d", elvis_getMech(3));
}

static void elvis_handleMech(int mech) {
}

static int elvis_getMech(int mechNo){
  if (0 == mechNo) return locals.direction;
  if (1 == mechNo) return locals.arms;
  if (2 == mechNo) return locals.legs;
  if (3 == mechNo) return locals.position;
  return 0;
}
