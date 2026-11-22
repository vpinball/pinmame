// license:BSD-3-Clause

/*******************************************************************************
 Verne's World (Spinball, 1996) -- game definition and ball simulator.

 The simulator file owns its game: input ports, game data, ROM set and the
 CORE_GAMEDEF line live here rather than in spinbgames.c.

 Switch numbers are the manual's (30-97, spinb_sw2m).  Coil numbers are
 PinMAME's: the firmware keeps a coil image in RAM, $8006 = coils 1-8,
 $8008 = 9-16 and $8007 = 17-24, one bit per coil.  The coils below come from
 the CPU ROM (which routine sets the bit, and on which switch), checked by
 setting the bits from the debugger and by play-testing:
     4  trough eject (SALIDA BOLAS)   serve routine $42BE, waits for switch 74
     6  octopus ball release          third octopus lock, or ball search
     8  drop target bank reset        ball start, when any of 41-44 is down
     9  volcano entrance diverter     pulsed $80 ticks after switch 76
    10  catapult                      switch 67
    11  ball launcher (LANZADOR)      FIRE button with a ball on switch 74
    12  balloon entrance diverter     pulsed $80 ticks after switch 77
    13  volcano elevator              switch 50
    14  slingshot (EXPULSOR)          switch 40
    15  left bumper                   switch 46
    16  right bumper                  switch 56
    22  balloon motor, reverse        pulsed with 23 off to brake the motor ($2E08)
    23  balloon motor, forward        runs the screw lift (also at power-on)
 Ball search pulses 6, 7, 9-16 and 20 in turn; 7 and 20 are not modelled.
 The manual's connector tables (pages 25 and 62) do not give this order, so the
 table above is the ROM's view, not the wiring diagram's.

 The balloon (manual pages 35 and 47) is a motor-driven screw that lifts balls
 past LOWER (45), CENTRE (55) and UPPER (65) to the exit (57).  The model follows
 the firmware: a ball landing on LOWER starts the lock handler, which runs the
 motor until the next CLOSING edge of CENTRE (the edge flags at $8052-$8058 latch
 closings only) and then brakes it ($35A5/$35E4 -> $2E08); if CONTROL (85) is
 closed at that edge it keeps the motor running and empties the lift instead.  A
 ball landing on LOWER while CENTRE and UPPER (or CENTRE and CONTROL) are closed
 means a full balloon: the firmware empties the lift -- the 3-ball multiball
 ($34B9-$34E8).  So a lock parks the newest ball on CENTRE and pushes the older
 one up: two locks rest on CENTRE and UPPER, and the third releases all three.
 CONTROL is modelled as a ball past UPPER, about to leave (the second half of the
 UPPER stage).  LOWER, CENTRE and UPPER close for the first half of their stage,
 so consecutive balls give separate edges.  The manual's balloon test (page 35)
 lists LOWER where this model has CENTRE; the model follows the firmware.  The
 stage time, VW_BALLOON_STAGE, is an estimate: the manual gives no motor speed.

 VW_SERVE_TIME is how long an ejected ball takes from the trough to the shooter
 lane switch (74).  The firmware gives a normal serve $2FF ticks (3 s) and a
 replacement ball after a lock only $FF ticks (1 s) before it fires the eject again ($2D8D).

 The octopus counts a lock on each closing edge of CONTROL (84) ($3B35, count in
 $81A7) and releases all held balls with coil 6 on the third.

 PLAY-TESTED headless with scripted keys: game start, serve and FIRE launch,
 bumpers, slingshot, catapult, the drop bank and its reset, drain and the next
 serve; volcano lock (diverter, replacement ball, elevator kick); three balloon
 locks ending in the 3-ball release; three octopus locks ending in the release.
 Nothing is tested on a machine.

 Two faults of the original ROM show in this simulator:
 - Double serve: with VW_SERVE_TIME at 1 s or more, the replacement ball
   for a lock is ejected again before the first reaches the shooter lane ($2D8D
   waits $FF ticks); at 2.5 s the trough empties and three balls come out.
 - No-switch timeout: 16 s ($1000 ticks in $81FD) without a scoring switch start
   a ball search ($29F9) that runs the balloon motor and fires the octopus release,
   so locked balls come back into play, and then clears the lock counts.

 Keys for the Verne's World simulator:
      W/E  Left/Right bumper            S  Slingshot (Expulsor)
        T  Catapult (N)                 G  Volcano entrance (S)
        H  Balloon entrance (E)         J  Octopus platform
        K  Octopus lock target    Z/X/C/V  Drop bank targets 1-4
      B/N  Rear bank targets x10/x20    A  Outlane (drain)        Q  Centre drain
Y/U/I/O/P  Lanes W-O-R-L-D          M/,/.  Top lanes V-E-R
      D/F  Balloon lock targets green/yellow
     '/\\  Volcano lock targets green/yellow
        R  Lower special target         /  Lower right extra ball lane
      [/]  Special / Extra ball keys (teclas)
    Enter  FIRE button (launch)       End  Extra ball button (cabinet)
 ******************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "spinb.h"

/*------------------
/  Local functions
/-------------------*/
static int  vrnwrld_handleBallState(sim_tBallStatus *ball, int *inports);
static void vrnwrld_drawStatic(BMTYPE **line);
static void vrnwrld_initSim(sim_tBallStatus balls[], int *inports, int noOfBalls);
static void vrnwrld_handleMech(int mech);

/*-------------------
/ Switch definitions
/--------------------*/
#define swKicker        40  /* EXPULSOR (slingshot) */
#define swBank1         41  /* DIANA BANCADA 1 (bottom) */
#define swBank2         42
#define swBank3         43
#define swBank4         44  /* DIANA BANCADA 4 (top) */
#define swBalloonLower  45  /* GLOBO INFERIOR */
#define swLBumper       46
#define swOctoLock      47  /* DIANA LOCK PULPO */
#define swVolcanoLift   50  /* ELEVADOR VOLCAN */
#define swBalLockGreen  51
#define swBalLockYellow 52
#define swVolLockGreen  53
#define swVolLockYellow 54
#define swBalloonCentre 55  /* GLOBO CENTRO */
#define swRBumper       56
#define swBalloonExit   57  /* SALIDA GLOBO */
#define swLaneW         60
#define swLaneO         61
#define swLaneR         62
#define swLaneL         63
#define swLaneD         64
#define swBalloonUpper  65  /* GLOBO SUPERIOR */
#define swLowerSpecial  66
#define swCatapult      67  /* CATAPULTA LETRA N */
#define swTrough4       70  /* BOLA CUARTA -- the entry end */
#define swTrough3       71
#define swTrough2       72
#define swTrough1       73  /* BOLA PRIMERA -- the served end */
#define swShooter       74  /* BOLA EN SALIDA */
#define swEBButton      75  /* PULSADOR EXTRA BALL (cabinet) */
#define swVolcanoEnt    76  /* ENTRADA VOLCAN LETRA S */
#define swBalloonEnt    77  /* ENTRADA GLOBO LETRA E */
#define swTopV          80
#define swTopE          81
#define swTopR          82
#define swOctoPlatform  83  /* PLATAFORMA PULPO */
#define swOctoControl   84  /* CONTROL BOLA CAUTIVA EN PULPO */
#define swBalloonCtrl   85  /* CONTROL GLOBO */
#define swFire          86  /* PULSADOR FIRE (cabinet) */
#define swLowerEB       87  /* BOLA EXTRA INFERIOR DERECHA */
#define swRearX10       90
#define swRearX20       91
#define swKeySpecial    92  /* TECLA ESPECIAL */
#define swKeyEB         93  /* TECLA BOLA EXTRA */
#define swOutlanes      94  /* PASILLOS INFERIORES PERDIDA */

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sTrough          4
#define sOctoRelease     6
#define sBankReset       8
#define sVolcanoDiv      9
#define sCatapult       10
#define sLauncher       11
#define sBalloonDiv     12
#define sVolcanoLift    13
#define sKicker         14
#define sLBumper        15
#define sRBumper        16
#define sBalloonMotor   23

/* Motor-on frames (60 Hz VBLANK) the screw needs to lift a ball one stage:
   LOWER -> CENTRE -> UPPER -> exit.  An estimate; see the header */
#define VW_BALLOON_STAGE 90
/* Frames from leaving the trough to closing the shooter lane switch (0.2 s) */
#define VW_SERVE_TIME    12
/* Simulator ticks a ball takes from one trough switch to the next: a trough state's
   next-state timer (1) times sim.c's SIM_TIMEFACTOR (6) */
#define VW_TROUGH_ROLL   6

/*---------------------
/  Ball state handling
/----------------------*/
enum {
  stTrough1=SIM_FIRSTSTATE, stTrough2, stTrough3, stTrough4, stServe, stShooter, stLaunch, stDrain, stOutlane,
  stKicker, stLBumper, stRBumper, stCatapult,
  stVolcanoEnt, stVolcano, stVolcanoUp,
  stBalloonEnt, stBalloon, stBalloonExit,
  stOctoPlatform, stOctoControl, stOcto,
  stOctoLock, stBalLockG, stBalLockY, stVolLockG, stVolLockY,
  stBank1, stBank2, stBank3, stBank4, stRearX10, stRearX20,
  stLaneW, stLaneO, stLaneR, stLaneL, stLaneD, stTopV, stTopE, stTopR,
  stLowerSpecial, stLowerEB, stKeySpecial, stKeyEB
};

static sim_tState vrnwrld_stateDef[] = {
  {"Not Installed",    0,0,              0,             stDrain,     0,0,0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",        0,0,              0,             0,           0,0,0, SIM_STNOTEXCL},

  /* States left on a coil have no minimum switch time (0): the firmware may fire the
     coil before a minimum would have run out, and a missed pulse strands the ball.
     The trough: balls drain in at contact 70 and roll towards 73, the served end */
  {"Trough 1 (served)",0,0,              sTrough,       stServe,     1},
  /* The trough switches are set by vrnwrld_handleMech, not by these states: the
     stack rolls forward as one, with no moment in which every trough switch is
     open.  Such a gap matters, because the replacement-ball serve ($2DB7) takes an
     empty trough as "nothing to serve" and stops waiting for the shooter lane */
  {"Trough 2",         1,0,              0,             stTrough1,   1},
  {"Trough 3",         1,0,              0,             stTrough2,   1},
  {"Trough 4 (entry)", 1,0,              0,             stTrough3,   1},
  {"Serving",          1,0,              0,             0,           0,0,0, SIM_STNOTEXCL},
  {"Shooter Lane",     0,swShooter,      sLauncher,     stLaunch,    1},
  {"Launched",         1,0,              0,             stFree,      7,0,0, SIM_STNOTEXCL},
  {"Drain",            1,0,              0,             stTrough4,   1,0,0, SIM_STNOTEXCL},
  {"Outlane",          1,swOutlanes,     0,             stDrain,     2,0,0, SIM_STNOTEXCL},

  {"Slingshot",        1,swKicker,       0,             stFree,      1},
  {"Left Bumper",      1,swLBumper,      0,             stFree,      1},
  {"Right Bumper",     1,swRBumper,      0,             stFree,      1},
  {"Catapult",         0,swCatapult,     sCatapult,     stFree,      3},

  /* Volcano: the entrance switch is passed first; the diverter then decides,
     see vrnwrld_handleBallState.  The elevator kicks the ball back out */
  {"Volcano Entrance", 1,swVolcanoEnt,   0,             0,           0},
  {"Volcano",          0,swVolcanoLift,  sVolcanoLift,  stVolcanoUp, 3},
  {"Volcano Kick",     1,0,              0,             stFree,      5,0,0, SIM_STNOTEXCL},

  /* Balloon: entrance, then the screw lift (handler-driven, several balls) */
  {"Balloon Entrance", 1,swBalloonEnt,   0,             0,           0},
  {"Balloon Lift",     1,0,              0,             0,           0,0,0, SIM_STNOTEXCL},
  {"Balloon Exit",     1,swBalloonExit,  0,             stFree,      3},

  /* Octopus: the platform leads over CONTROL (84) into the lock, where the balls wait
     behind the release post.  The firmware counts a lock on each closing edge of 84
     ($3B35, count in $81A7), so 84 is passed, not held */
  {"Octopus Platform", 1,swOctoPlatform, 0,             stOctoControl,2},
  {"Octopus Control",  1,swOctoControl,  0,             stOcto,      2},
  {"Octopus Lock",     1,0,              sOctoRelease,  stFree,      5,0,0, SIM_STNOTEXCL},

  {"Octopus Lock Tgt", 1,swOctoLock,     0,             stFree,      2},
  {"Balloon Lock Grn", 1,swBalLockGreen, 0,             stFree,      2},
  {"Balloon Lock Yel", 1,swBalLockYellow,0,             stFree,      2},
  {"Volcano Lock Grn", 1,swVolLockGreen, 0,             stFree,      2},
  {"Volcano Lock Yel", 1,swVolLockYellow,0,             stFree,      2},

  /* The drop bank: a target stays down until the reset coil (vrnwrld_handleMech) */
  {"Bank Target 1",    1,swBank1,        0,             stFree,      2,0,0, SIM_STSWKEEP},
  {"Bank Target 2",    1,swBank2,        0,             stFree,      2,0,0, SIM_STSWKEEP},
  {"Bank Target 3",    1,swBank3,        0,             stFree,      2,0,0, SIM_STSWKEEP},
  {"Bank Target 4",    1,swBank4,        0,             stFree,      2,0,0, SIM_STSWKEEP},
  {"Rear Target x10",  1,swRearX10,      0,             stFree,      2},
  {"Rear Target x20",  1,swRearX20,      0,             stFree,      2},

  {"Lane W",           1,swLaneW,        0,             stFree,      3},
  {"Lane O",           1,swLaneO,        0,             stFree,      3},
  {"Lane R",           1,swLaneR,        0,             stFree,      3},
  {"Lane L",           1,swLaneL,        0,             stFree,      3},
  {"Lane D",           1,swLaneD,        0,             stFree,      3},
  {"Top Lane V",       1,swTopV,         0,             stFree,      3},
  {"Top Lane E",       1,swTopE,         0,             stFree,      3},
  {"Top Lane R",       1,swTopR,         0,             stFree,      3},
  {"Lower Special",    1,swLowerSpecial, 0,             stFree,      2},
  {"Lower Extra Ball", 1,swLowerEB,      0,             stFree,      3},
  {"Key Special",      1,swKeySpecial,   0,             stFree,      2},
  {"Key Extra Ball",   1,swKeyEB,        0,             stFree,      2},

  {0}
};

/* The game's diverters pulse only after it has seen the entrance switch, so the
   route is decided when the ball leaves the entrance: diverter on -> into the toy */
static int vrnwrld_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    /* on its way from the trough to the shooter lane */
    case stServe:
      if (++ball->custom < VW_SERVE_TIME) return 0;
      ball->custom = 0;
      return setState(stShooter, 1);

    case stVolcanoEnt:
      if (++ball->custom < 10) return 0; /* give the firmware time to fire */
      ball->custom = 0;
      return core_getSol(sVolcanoDiv) ? setState(stVolcano, 3) : setState(stFree, 3);

    case stBalloonEnt:
      if (++ball->custom < 10) return 0;
      ball->custom = 0;
      return core_getSol(sBalloonDiv) ? setState(stBalloon, 3) : setState(stFree, 3);

    /* ball->custom is the height in motor frames, moved in vrnwrld_handleMech */
    case stBalloon:
      if (ball->custom >= 3*VW_BALLOON_STAGE) {
        ball->custom = 0;
        return setState(stBalloonExit, 2);
      }
      return 0;
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData vrnwrld_inportData[] = {
  {0, 0x0001, stLBumper},
  {0, 0x0002, stRBumper},
  {0, 0x0004, stKicker},
  {0, 0x0008, stCatapult},
  {0, 0x0010, stVolcanoEnt},
  {0, 0x0020, stBalloonEnt},
  {0, 0x0040, stOctoPlatform},
  {0, 0x0080, stOctoLock},
  {0, 0x0100, stBank1},
  {0, 0x0200, stBank2},
  {0, 0x0400, stBank3},
  {0, 0x0800, stBank4},
  {0, 0x1000, stRearX10},
  {0, 0x2000, stRearX20},
  {0, 0x4000, stOutlane},
  {0, 0x8000, stDrain},

  {1, 0x0001, stLaneW},
  {1, 0x0002, stLaneO},
  {1, 0x0004, stLaneR},
  {1, 0x0008, stLaneL},
  {1, 0x0010, stLaneD},
  {1, 0x0020, stTopV},
  {1, 0x0040, stTopE},
  {1, 0x0080, stTopR},
  {1, 0x0100, stBalLockG},
  {1, 0x0200, stBalLockY},
  {1, 0x0400, stVolLockG},
  {1, 0x0800, stVolLockY},
  {1, 0x1000, stLowerSpecial},
  {1, 0x2000, stLowerEB},
  {1, 0x4000, stKeySpecial},
  {1, 0x8000, stKeyEB},

  /* cabinet buttons: plain switches, whatever the ball does */
  {2, 0x0001, swFire,     SIM_STANY},
  {2, 0x0002, swEBButton, SIM_STANY},
  {0}
};

/*----------------------
/  Playfield mechanics
/-----------------------*/
static sim_tBallStatus *vrnwrld_balls;
static int vrnwrld_noOfBalls;

static void vrnwrld_initSim(sim_tBallStatus balls[], int *inports, int noOfBalls) {
  vrnwrld_balls = balls;
  vrnwrld_noOfBalls = noOfBalls;
}

static void vrnwrld_handleMech(int mech) {
  const int stageTime = VW_BALLOON_STAGE;
  int ii, lower = 0, centre = 0, upper = 0, control = 0, trough = 0, moving = 0;

  /* the drop bank rises on its reset coil, and only then */
  if (core_getSol(sBankReset)) {
    core_setSw(swBank1, FALSE); core_setSw(swBank2, FALSE);
    core_setSw(swBank3, FALSE); core_setSw(swBank4, FALSE);
  }
  if (!vrnwrld_balls) return;

  /* When the stack rolls forward after a serve, the balls touch: a ball rolling towards
     a trough slot already holds that slot's switch, so no moment shows an empty trough.
     A ball rolling on its own (a drained ball joining the stack) opens the switch it
     leaves and closes the next one only half a roll later.  Without that gap the
     firmware, which takes a closing sooner than an opening, sees the old and the new
     switch closed at once -- one ball too many in the trough, which ends a multiball
     ball while a ball is still in play ($2AB0) */
  for (ii = 0; ii < vrnwrld_noOfBalls; ii++) {
    const sim_tBallStatus *ball = &vrnwrld_balls[ii];
    if (ball->state == stServe ||
        (ball->state == stPending && ball->nextState >= stTrough1 && ball->nextState <= stTrough3))
      moving += 1;
  }
  for (ii = 0; ii < vrnwrld_noOfBalls; ii++) {
    const sim_tBallStatus *ball = &vrnwrld_balls[ii];
    const int st = (ball->state == stPending) ? ball->nextState : ball->state;
    if (st < stTrough1 || st > stTrough4) continue;
    if (ball->state == stPending && st != stTrough4 && moving < 2 &&
        (ball->timer < 0 || ball->timer > VW_TROUGH_ROLL/2)) /* < 0: just left (sim.c) */
      continue;                         /* rolling alone: between two switches */
    trough |= 1 << (st - stTrough1);
  }
  core_setSw(swTrough1, trough & 1); core_setSw(swTrough2, (trough >> 1) & 1);
  core_setSw(swTrough3, (trough >> 2) & 1); core_setSw(swTrough4, (trough >> 3) & 1);

  /* the balloon micro-switches follow the balls in the lift; the
     screw lifts every ball in the balloon while the forward motor coil is on */
  for (ii = 0; ii < vrnwrld_noOfBalls; ii++) {
    sim_tBallStatus *ball = &vrnwrld_balls[ii];
    if (ball->state == stBalloon) {
      const int stage = ball->custom / stageTime;
      const int onSwitch = (ball->custom % stageTime) < stageTime/2;
      if (core_getSol(sBalloonMotor) && ball->custom < 3*stageTime)
        ball->custom += 1;
      if (stage == 2 && !onSwitch) control = 1;
      if (onSwitch) {
        if      (stage == 0) lower  = 1;
        else if (stage == 1) centre = 1;
        else if (stage == 2) upper  = 1;
      }
    }
  }
  core_setSw(swBalloonLower,  lower);
  core_setSw(swBalloonCentre, centre);
  core_setSw(swBalloonUpper,  upper);
  core_setSw(swBalloonCtrl,   control);
}

/*--------------------
  Drawing information
  --------------------*/
static void vrnwrld_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"W/E = Bumpers, S = Slingshot");
  core_textOutf(30, 60,BLACK,"T = Catapult, G = Volcano");
  core_textOutf(30, 70,BLACK,"H = Balloon, J = Octopus");
  core_textOutf(30, 80,BLACK,"K = Octopus lock target");
  core_textOutf(30, 90,BLACK,"D/F, '/\\ = Balloon/Volcano locks");
  core_textOutf(30,100,BLACK,"Z/X/C/V = Drop bank, B/N = Rear");
  core_textOutf(30,110,BLACK,"Y/U/I/O/P = WORLD, M/,/. = VER");
  core_textOutf(30,120,BLACK,"R = Lower special, / = Lower EB");
  core_textOutf(30,130,BLACK,"[/] = Special / Extra Ball keys");
  core_textOutf(30,140,BLACK,"A = Outlane, Q = Drain");
  core_textOutf(30,150,BLACK,"Enter = Fire, End = EB button");
}

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData vrnwrldSimData = {
  3,                        /* 3 game specific input ports */
  vrnwrld_stateDef,         /* Definition of all states */
  vrnwrld_inportData,       /* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },
  vrnwrld_initSim,          /* Simulator Init */
  vrnwrld_handleBallState,  /* Function to handle ball state changes */
  vrnwrld_drawStatic,       /* Function to draw the on-screen help */
  FALSE,                    /* No manual shooter: the FIRE button fires the launcher */
  NULL,                     /* Custom key conditions */
  TRUE                      /* Keys act on the ball on the playfield: the locks keep balls */
};

/*--------------------------
/ Game specific input ports
/---------------------------*/
/* With MDRV_DIPS(24) the game ports start after the two DIP ports: inports 5-7 */
/* One playfield, one set of ports; INPUT_PORTS_START is per game, hence the macro */
#define VRNWRLD_SIM_PORTS(name) \
  SPINB_INPUT_PORTS_DIPS_START(name, 4, SPINB_DIPS_VRNWRLD) \
    PORT_START /* 5 */ \
      COREPORT_BIT(0x0001,"Left Bumper",        KEYCODE_W) \
      COREPORT_BIT(0x0002,"Right Bumper",       KEYCODE_E) \
      COREPORT_BIT(0x0004,"Slingshot",          KEYCODE_S) \
      COREPORT_BIT(0x0008,"Catapult",           KEYCODE_T) \
      COREPORT_BIT(0x0010,"Volcano",            KEYCODE_G) \
      COREPORT_BIT(0x0020,"Balloon",            KEYCODE_H) \
      COREPORT_BIT(0x0040,"Octopus",            KEYCODE_J) \
      COREPORT_BIT(0x0080,"Octopus Lock Target",KEYCODE_K) \
      COREPORT_BIT(0x0100,"Bank Target 1",      KEYCODE_Z) \
      COREPORT_BIT(0x0200,"Bank Target 2",      KEYCODE_X) \
      COREPORT_BIT(0x0400,"Bank Target 3",      KEYCODE_C) \
      COREPORT_BIT(0x0800,"Bank Target 4",      KEYCODE_V) \
      COREPORT_BIT(0x1000,"Rear Target x10",    KEYCODE_B) \
      COREPORT_BIT(0x2000,"Rear Target x20",    KEYCODE_N) \
      COREPORT_BIT(0x4000,"Outlane",            KEYCODE_A) \
      COREPORT_BIT(0x8000,"Drain",              KEYCODE_Q) \
    PORT_START /* 6 */ \
      COREPORT_BIT(0x0001,"Lane W",             KEYCODE_Y) \
      COREPORT_BIT(0x0002,"Lane O",             KEYCODE_U) \
      COREPORT_BIT(0x0004,"Lane R",             KEYCODE_I) \
      COREPORT_BIT(0x0008,"Lane L",             KEYCODE_O) \
      COREPORT_BIT(0x0010,"Lane D",             KEYCODE_P) \
      COREPORT_BIT(0x0020,"Top Lane V",         KEYCODE_M) \
      COREPORT_BIT(0x0040,"Top Lane E",         KEYCODE_COMMA) \
      COREPORT_BIT(0x0080,"Top Lane R",         KEYCODE_STOP) \
      COREPORT_BIT(0x0100,"Balloon Lock Green", KEYCODE_D) \
      COREPORT_BIT(0x0200,"Balloon Lock Yellow",KEYCODE_F) \
      COREPORT_BIT(0x0400,"Volcano Lock Green", KEYCODE_QUOTE) \
      COREPORT_BIT(0x0800,"Volcano Lock Yellow",KEYCODE_BACKSLASH) \
      COREPORT_BIT(0x1000,"Lower Special",      KEYCODE_R) \
      COREPORT_BIT(0x2000,"Lower Extra Ball",   KEYCODE_SLASH) \
      COREPORT_BIT(0x4000,"Key Special",        KEYCODE_OPENBRACE) \
      COREPORT_BIT(0x8000,"Key Extra Ball",     KEYCODE_CLOSEBRACE) \
    PORT_START /* 7 */ \
      COREPORT_BIT(0x0001,"Fire Button",        KEYCODE_ENTER) \
      COREPORT_BIT(0x0002,"Extra Ball Button",  KEYCODE_END) \
  SPINB_INPUT_PORTS_END

/*-------------------------------------------------------------------
/ Verne's World (1996)
/-------------------------------------------------------------------*/
static core_tLCDLayout vrnwrld_disp[] = {
  {0,0,32,128,CORE_DMD,NULL,NULL}, {0}
};
// IRQ: NE556 with a 100k trimmer; the game code counts 256 ticks per second, see spinb.c
static core_tGameData vrnwrldGameData = {GEN_SPINB,vrnwrld_disp,{FLIP_SWNO(0,0),0,4,0,
  SNDBRD_SPINB,SPINB_DSP3/*?*/,SPINB_GS1(0xff,0xff,0xff,TRUE),256, NULL,vrnwrld_handleMech},&vrnwrldSimData};
static void init_vrnwrld(void) {
  core_gameData = &vrnwrldGameData;
}
VRNWRLD_SIM_PORTS(vrnwrld)
SPINB_ROMSTART2(vrnwrld,"vwcpu0.rom",  CRC(89c980e7) SHA1(09eeed0232255875cf119d59143d839ff40e30dd),
						"vwcpu1.rom",  CRC(a4db4e64) SHA1(fc55781295fc723741de24ad60311b7e33551830))
SPINB_DMDROM3(			"vwdmd0.rom",  CRC(40600060) SHA1(7ad619bcb5e5e50325360f4e946b5bfa072caead),
						"vwdmd1.rom",  CRC(de4a1060) SHA1(6b848dfd8aafdbcf7e1593f98bd1c3d69306aa11),
						"vwdmd2.rom",  CRC(29fc8da7) SHA1(2704f14a3338a63abda3bcbc56e9f984a679eb38))
SPINB_SNDROM13(			"vws2ic9.rom", CRC(ab8cb4c5) SHA1(92a702c11e2cef703992244529ba86079d5ab9b0),
						"vws3ic15.rom",CRC(d62c9443) SHA1(7c6b8662d88ba6592da8b83af11087647105e8dd), //Sound Effects 1
						"vws4ic30.rom",CRC(ecd18a19) SHA1(558e687e0429d31fafe8db05954d9a8ad90d6aeb),
						"vws5ic25.rom",CRC(56d349f0) SHA1(e71d2d03c3e978c552e272de8850cc265255fbd1), //Music 1
						"vws6ic26.rom",CRC(bee399c1) SHA1(b2c6e4830641ed32b9643dc8c1fa08a2da5a7e9b), //Music 2
						"vws7ic27.rom",CRC(7335b29c) SHA1(4de6de09f069feecbad2e5ef50032e8d381ff9b1)) //Music 3
SPINB_ROMEND
CORE_GAMEDEFNV(vrnwrld,"Verne's World",1996,"Spinball (Spain)",mSPINBSNMI2,0)

/*-------------------------------------------------------------------
/ Verne's World, operator MOD of the CPU ROMs (vwmcpu0/1); display and
/ sound ROMs are the parent's.  Built by build_mod.py from vw_mod.asm (kept
/ with the Verne's World notes, not in this repository):
/ - Serve fix: both serve routines (ball start $42BE, lock replacement $2D8D)
/   fired the trough eject again when the ball was slow to reach the shooter
/   lane, putting an extra ball into play.  They now fire again only if the
/   trough still holds as many balls as before the eject (drains landing on
/   switch 70 in meanwhile counted), and otherwise keep waiting.  The lock
/   replacement waits 3 s per check instead of 1 s.
/ - No-switch timeout ($81FD, ball search, then ball lost on the third)
/   60 s instead of 16 s, 76 s instead of 32 s at game start.
/ - Last game's scores in attract: before every second high score page the
/   attract shows the scores of the last game, with the game's own score
/   screen (one player: the one-player screen; more: the four-quadrant
/   screen).  The players of the last game are kept in battery RAM ($85F2,
/   unused by the firmware), so the page survives a power cycle, and are
/   forgotten when a new game starts and overwrites the scores.
/ - Final scores held until START: when the game-over sequence ends, the
/   attract mode does not start.  The final scores stay on the display with
/   PRESS START between them (one player: medium digits with PRESS START
/   underneath; more: two scores per row, PRESS START in the middle row),
/   drawn after the game's own score screen ($5018) has been sent.  START
/   only dismisses the screen -- it never starts a game, credits or not --
/   and the attract mode carries on; the next START starts a game.  The
/   releasing press cannot start a game as well: START stays ignored until it
/   has read open for 1/4 s, so contact bounce on press or release gets no
/   second edge through.  Coins are accepted while the scores are held.  The
/   hold is cleared at power-on ($0272), so a machine that was switched off
/   comes up in attract mode.
/   Hooks: the attract loop's idle test at $068D, the IRQ's START handling
/   at $585A and its timer updates at $52C8, game over at $4A08.
/ - Cold start restarts by itself: with empty NVRAM the original writes the
/   factory defaults ($040F) and then loops at $047C with a dark display until
/   the operator resets the machine.  The jump to that loop ($045C) now
/   restarts the boot at $012A, which finds the defaults and comes up in
/   attract mode (about 11 s instead of 6 s).  It restarts after the I/O
/   setup at $00E3-$0129, which holds most coils on for about 0.15 s and so
/   would kick a ball out of the trough a second time.
/ Code cave at $5B50 (vwmcpu1.rom $1B50, erased in the original); variables at
/ $85F0-$85F9, battery RAM the firmware clears at cold boot and never uses.
/ Free play needs no change: the Exhibition DIP (SL1-8) already keeps one
/ credit standing and does not spend it
/-------------------------------------------------------------------*/
static void init_vrnwrldm(void) {
  core_gameData = &vrnwrldGameData;
}
VRNWRLD_SIM_PORTS(vrnwrldm)
SPINB_ROMSTART2(vrnwrldm,"vwmcpu0.rom",CRC(5030b04e) SHA1(fe5b2c29b76428d4e49a76879bf913af685053bf),
						"vwmcpu1.rom", CRC(ddc402b2) SHA1(8ae77436ff36c14e356016ad629369617e7e2273))
SPINB_DMDROM3(			"vwdmd0.rom",  CRC(40600060) SHA1(7ad619bcb5e5e50325360f4e946b5bfa072caead),
						"vwdmd1.rom",  CRC(de4a1060) SHA1(6b848dfd8aafdbcf7e1593f98bd1c3d69306aa11),
						"vwdmd2.rom",  CRC(29fc8da7) SHA1(2704f14a3338a63abda3bcbc56e9f984a679eb38))
SPINB_SNDROM13(			"vws2ic9.rom", CRC(ab8cb4c5) SHA1(92a702c11e2cef703992244529ba86079d5ab9b0),
						"vws3ic15.rom",CRC(d62c9443) SHA1(7c6b8662d88ba6592da8b83af11087647105e8dd),
						"vws4ic30.rom",CRC(ecd18a19) SHA1(558e687e0429d31fafe8db05954d9a8ad90d6aeb),
						"vws5ic25.rom",CRC(56d349f0) SHA1(e71d2d03c3e978c552e272de8850cc265255fbd1),
						"vws6ic26.rom",CRC(bee399c1) SHA1(b2c6e4830641ed32b9643dc8c1fa08a2da5a7e9b),
						"vws7ic27.rom",CRC(7335b29c) SHA1(4de6de09f069feecbad2e5ef50032e8d381ff9b1))
SPINB_ROMEND
CORE_CLONEDEFNV(vrnwrldm,vrnwrld,"Verne's World (MOD serve fix, 60s timeout, last game scores, PRESS START)",2026,"Spinball (Spain)",mSPINBSNMI2,0)
