// license:BSD-3-Clause

/*******************************************************************************
 Domino's Spectacular Pinball Adventure (Spooky Pinball, 2016): game definition
 and playfield simulator.

 Simulated: trough and shooter lane (manual plunger), autolauncher, both scoops,
 the orbits with the up-post, the oven ramp with the magnet, the Noid and its
 target bank, and the drain. Coils read as "on during the last frame".
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys (L/R Ctrl selects the left or right one of a pair):
    +-  L/R Slingshot        +I  L/R Inlane         +O  L/R Outlane (drain)
    +S  L/R Scoop            +R  L/R Orbit          +N  L/R Noid Orbit
    +B  L/R Pop Bumper        B  Lower Pop Bumper   +L  Star/5 Lane
     C  Center Ramp           V  Oven Ramp           G  Spinner
     Z/X/M  Target Bank left/middle/right (the Noid when the bank is down)
     F/H/J/K/U  Pizza Tracker targets (Delivery .. Order Placed)
     Q  Drain between the flippers           Space  Plunger (hold, release)
  The keys move a ball on the playfield (autoBall); Up/Down still select one.
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "pinheck.h"

PINHECK_INPUT_PORTS_START(dominos, 3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001, "Left Qualifier",   KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002, "Right Qualifier",  KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004, "L/R Slingshot",    KEYCODE_MINUS)
    COREPORT_BIT(0x0008, "L/R Inlane",       KEYCODE_I)
    COREPORT_BIT(0x0010, "L/R Outlane",      KEYCODE_O)
    COREPORT_BIT(0x0020, "L/R Scoop",        KEYCODE_S)
    COREPORT_BIT(0x0040, "L/R Orbit",        KEYCODE_R)
    COREPORT_BIT(0x0080, "L/R Noid Orbit",   KEYCODE_N)
    COREPORT_BIT(0x0100, "L/R/Lower Pop",    KEYCODE_B)
    COREPORT_BIT(0x0200, "Star/5 Lane",      KEYCODE_L)
    COREPORT_BIT(0x0400, "Center Ramp",      KEYCODE_C)
    COREPORT_BIT(0x0800, "Oven Ramp",        KEYCODE_V)
    COREPORT_BIT(0x1000, "Spinner",          KEYCODE_G)
    COREPORT_BIT(0x2000, "Drain",            KEYCODE_Q)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001, "Bank Left",        KEYCODE_Z)
    COREPORT_BIT(0x0002, "Bank Middle",      KEYCODE_X)
    COREPORT_BIT(0x0004, "Bank Right",       KEYCODE_M)
    COREPORT_BIT(0x0008, "Delivery",         KEYCODE_F)
    COREPORT_BIT(0x0010, "Quality Check",    KEYCODE_H)
    COREPORT_BIT(0x0020, "Bake",             KEYCODE_J)
    COREPORT_BIT(0x0040, "Prepare",          KEYCODE_K)
    COREPORT_BIT(0x0080, "Order Placed",     KEYCODE_U)
PINHECK_INPUT_PORTS_END

/* switches: document switch n (0-63) is (n/8+1)*10 + n%8+1; the optos are cabinet inputs 13 and 14 */
#define swShooter   11
#define swTrough1   12
#define swTrough2   13
#define swTrough3   14
#define swRSling    16
#define swRInlane   17
#define swROutlane  18
#define swLSling    22
#define swLInlane   23
#define swLOutlane  24
#define swLScoop    25
#define swBankR     26
#define swBankM     27
#define swBankL     28
#define swDelivery  31
#define swQuality   32
#define swBake      33
#define swPrepare   34
#define swOrder     35
#define swROrbit    36
#define swRNoidOrb  37
#define swLNoidOrb  38
#define swStarLane  41
#define sw5Lane     42
#define swRPop      43
#define swLowerPop  44
#define swLPop      45
#define swLOrbit    46
#define swSpinner   47
#define swRScoop    48
#define swNoidHome  58
#define swRampOpto  95
#define swOvenOpto  96

/* solenoids: document coil n is PinMAME solenoid n+1 */
#define sMagnet     6
#define sPost       7
#define sLScoop     9
#define sLFlipHigh  10
#define sLFlipLow   12
#define sRScoop     13
#define sLaunch     17
#define sLoad       18
#define sRFlipLow   19
#define sRFlipHigh  21

#define NOID_TURN   120  /* frames per Noid revolution */
#define NOID_HOME   10   /* frames of it with the home switch closed */

static struct {
  int since[25];   /* frames since each coil was last on */
  int noid;        /* Noid angle, 0 .. NOID_TURN-1, 0 = home */
  int home;        /* Noid Home as last set, -1 = not yet */
  int bankDown;    /* target bank lowered: servo 1 last pulsed above 1.5 ms */
} locals;

static int sol(int n) { return (coreGlobals.solenoids >> (n - 1)) & 1; }

enum { stTrough3 = SIM_FIRSTSTATE, stTrough2, stTrough1, stShooter, stLaunched, stDrain,
       stLOutlane, stROutlane, stLInlane, stRInlane, stLSling, stRSling,
       stLScoop, stRScoop, stLOrbit, stROrbit, stLOrbitOut, stROrbitOut, stLOrbitBack, stROrbitBack,
       stLNoidOrb, stRNoidOrb, stLNoidOut, stRNoidOut, stCRamp, stOvenRamp, stMagnet,
       stLPop, stRPop, stLowerPop, stStarLane, st5Lane, stSpinner,
       stBankL, stBankM, stBankR, stDelivery, stQuality, stBake, stPrepare, stOrder };

static sim_tState dominos_stateDef[] = {
  {"Not Installed", 0, 0,          0, stDrain,   0, 0, 0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0, 0,          0, 0,         0, 0, 0, SIM_STNOTEXCL},

  {"Trough 3",      1, swTrough3,  0, stTrough2, 3},
  {"Trough 2",      1, swTrough2,  0, stTrough1, 3},
  {"Trough 1",      1, swTrough1,  0, 0,         0},
  {"Shooter Lane",  1, swShooter,  0, 0,         0},
  {"Launched",      1, 0,          0, stFree,   10},
  {"Drain",         1, 0,          0, stTrough3, 1, 0, 0, SIM_STNOTEXCL},

  {"Left Outlane",  1, swLOutlane, 0, stDrain,  10},
  {"Right Outlane", 1, swROutlane, 0, stDrain,  10},
  {"Left Inlane",   1, swLInlane,  0, stFree,    5},
  {"Right Inlane",  1, swRInlane,  0, stFree,    5},
  {"Left Sling",    1, swLSling,   0, stFree,    2},
  {"Right Sling",   1, swRSling,   0, stFree,    2},

  {"Left Scoop",    1, swLScoop,   0, 0,         0},
  {"Right Scoop",   1, swRScoop,   0, 0,         0},
  {"Left Orbit",    1, swLOrbit,   0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Right Orbit",   1, swROrbit,   0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"L Orbit Exit",  1, swROrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Orbit Exit",  1, swLOrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"L Orbit Post",  1, swLOrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Orbit Post",  1, swROrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},

  {"L Noid Orbit",  1, swLNoidOrb, 0, stLNoidOut, 3, 0, 0, SIM_STNOTEXCL},
  {"R Noid Orbit",  1, swRNoidOrb, 0, stRNoidOut, 3, 0, 0, SIM_STNOTEXCL},
  {"L Noid Exit",   1, swRNoidOrb, 0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Noid Exit",   1, swLNoidOrb, 0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Center Ramp",   1, swRampOpto, 0, stFree,   10, 0, 0, SIM_STNOTEXCL},
  {"Oven Ramp",     1, swOvenOpto, 0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Magnet",        1, 0,          0, 0,         0},

  {"Left Pop",      1, swLPop,     0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Right Pop",     1, swRPop,     0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Lower Pop",     1, swLowerPop, 0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Star Lane",     1, swStarLane, 0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"5 Lane",        1, sw5Lane,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Spinner",       1, swSpinner,  0, stFree,    5, 0, 0, SIM_STNOTEXCL | SIM_STSPINNER},

  {"Bank Left",     1, swBankL,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Bank Middle",   1, swBankM,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Bank Right",    1, swBankR,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Delivery",      1, swDelivery, 0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Quality Check", 1, swQuality,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Bake",          1, swBake,     0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Prepare",       1, swPrepare,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Order Placed",  1, swOrder,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {0}
};

/* frames: up-post reached 9 after the orbit switch; magnet reached 6 after the oven opto, catching if on within 12 */
#define ORBIT_POST   9
#define MAGNET_CATCH 12
#define MAGNET_DROP  6   /* the firmware pulses the held magnet every 11 ms: 6 frames off is a release */

static int dominos_handleBallState(sim_tBallStatus *ball, int *inports) {
  (void)inports;
  switch (ball->state) {
    case stTrough1:   if (sol(sLoad) && !core_getSw(swShooter)) return setState(stShooter, 5); break; /* a full lane loses the pulse */
    case stShooter:   if (sol(sLaunch) || sim_getSol(sShooterRel)) return setState(stLaunched, 2); break;
    case stLScoop:    if (sol(sLScoop)) return setState(stFree, 5); break;
    case stRScoop:    if (sol(sRScoop)) return setState(stFree, 5); break;
    case stLOrbit:    return setState(locals.since[sPost] < ORBIT_POST ? stLOrbitBack : stLOrbitOut, 3);
    case stROrbit:    return setState(locals.since[sPost] < ORBIT_POST ? stROrbitBack : stROrbitOut, 3);
    case stOvenRamp:  return setState(locals.since[sMagnet] < MAGNET_CATCH ? stMagnet : stFree, 5);
    case stMagnet:    if (locals.since[sMagnet] > MAGNET_DROP) return setState(stFree, 5); break;
  }
  return 0;
}

/* mech bit 0: the Noid (continuous servo, turns away from 1.5 ms); bit 1: target bank, down above 1.5 ms */
static void dominos_handleMech(int mech) {
  int i, us = pinheck_servo(0), bank = pinheck_servo(1);
  for (i = 1; i <= 24; i++)
    locals.since[i] = sol(i) ? 0 : locals.since[i] < 10000 ? locals.since[i] + 1 : 10000;
  if ((mech & 0x02) && bank) locals.bankDown = bank > 1500;
  if (!(mech & 0x01)) return;
  if (us > 1550) locals.noid = (locals.noid + 1) % NOID_TURN;
  else if (us && us < 1450) locals.noid = (locals.noid + NOID_TURN - 1) % NOID_TURN;
  if ((locals.noid < NOID_HOME) != locals.home) /* only on change: the switch test may toggle it by hand */
    core_setSw(swNoidHome, locals.home = locals.noid < NOID_HOME);
}

static int dominos_getMech(int mechNo) {
  return mechNo == 0 ? locals.noid : mechNo == 1 ? locals.bankDown : 0;
}

static sim_tInportData dominos_inportData[] = {
  {0, 0x0005, stLSling},   {0, 0x0006, stRSling},
  {0, 0x0009, stLInlane},  {0, 0x000a, stRInlane},
  {0, 0x0011, stLOutlane}, {0, 0x0012, stROutlane},
  {0, 0x0021, stLScoop},   {0, 0x0022, stRScoop},
  {0, 0x0041, stLOrbit},   {0, 0x0042, stROrbit},
  {0, 0x0081, stLNoidOrb}, {0, 0x0082, stRNoidOrb},
  {0, 0x0101, stLPop},     {0, 0x0102, stRPop},     {0, 0x0100, stLowerPop},
  {0, 0x0201, stStarLane}, {0, 0x0202, st5Lane},
  {0, 0x0400, stCRamp},    {0, 0x0800, stOvenRamp}, {0, 0x1000, stSpinner},
  {0, 0x2000, stDrain},
  {1, 0x0001, stBankL},    {1, 0x0002, stBankM},    {1, 0x0004, stBankR},
  {1, 0x0008, stDelivery}, {1, 0x0010, stQuality},  {1, 0x0020, stBake},
  {1, 0x0040, stPrepare},  {1, 0x0080, stOrder},
  {0}
};

static sim_tSimData dominosSimData = {
  2,                    /* 2 game specific input ports */
  dominos_stateDef,
  dominos_inportData,
  { stTrough1, stTrough2, stTrough3, stDrain, stDrain, stDrain, stDrain },
  NULL,                 /* no init */
  dominos_handleBallState,
  NULL,                 /* no static drawing */
  TRUE,                 /* manual plunger (Space) next to the autolauncher */
  NULL,                 /* no custom key conditions */
  TRUE                  /* autoBall */
};

static core_tLCDLayout dominos_disp[] = {
  {0, 0, PINHECK_VIDEO_H, PINHECK_VIDEO_W, CORE_VIDEO, (genf *)pinheck_video, NULL}, {0}
};

static pinheck_tGameData dominosGameData = {
  { GEN_PINHECK, dominos_disp,
    { FLIP_SWNO(PINHECK_SWLFLIP, PINHECK_SWRFLIP) | FLIP_SOL(FLIP_L), 0, 1, PINHECK_CUSTSOLS, SNDBRD_NONE, 0, 6, 0,
      pinheck_getsol, dominos_handleMech, dominos_getMech },
    &dominosSimData },
  PINHECK_DOMINOS_DATA,
  { sRFlipHigh, sRFlipLow, sLFlipHigh, sLFlipLow }
};

static void init_dominos(void) {
  int i;
  core_gameData = &dominosGameData.core;
  memset(&locals, 0, sizeof(locals));
  for (i = 0; i <= 24; i++) locals.since[i] = 10000;
  locals.home = -1;
}

PINHECK_ROMSTART(dominos_006, "DOM_V006.PRG", 0x31990, CRC(750e27a4) SHA1(3fbebce7f885563e61dd2e4f5d7f0a8a54c53b23),
                 "PRP_V008.BIN", CRC(a51ee28d) SHA1(0556d88b6f0c7cb15e648e1f76f4d47890ddb858))
PINHECK_ROMEND
PINHECK_GAMEDEF(dominos, 006, "Domino's Spectacular Pinball Adventure", 2016, "Spooky Pinball", gl_mPINHECK, 0)
