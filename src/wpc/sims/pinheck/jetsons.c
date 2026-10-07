// license:BSD-3-Clause

/*******************************************************************************
 The Jetsons (Spooky Pinball, 2017): game definition and playfield simulator.

 Simulated: 3-ball trough with optos, shooter lane and autolauncher (no manual
 plunger), left scoop, kickout hole, orbits with the up-post, Elroy loop, flipper
 end-of-stroke switches, pops, slings and the drain; the Orbitty topper (servo 0)
 is tracked. Coils read as "on during the last frame".

 Outputs beyond the coils (see pinheck.c for the numbering):
   solenoids 37/38  ramp / scoop flasher (GI strings 8/9, by the wiring list);
                    seen at run time: 38 flashes while a ball sits in the scoop,
                    both flash in the show after a captive ball hit
   solenoids 39/40  left / right GI; 41-44 dim along with them
   solenoid 57      the Orbitty topper (servo 0), sweeping as a game starts;
                    58 (servo 1) rests centred
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys (L/R Ctrl selects the left or right one of a pair):
    +-  L/R Slingshot        +I  L/R Inlane         +O  L/R Outlane (drain)
    +R  L/R Orbit            +B  L/R Pop Bumper      B  Lower Pop Bumper
    +L  L/R Lane              L  Middle Lane         S  Scoop
     K  Kickout Hole          E  Elroy Loop          C  Captive Ball
     X  Extra Ball            G/H/J  Center Targets G, E, O
     T/Y/U  Right Targets R, G, E                    Q  Drain between the flippers
  The Launch Button (9) fires the autolauncher.
  The keys move a ball on the playfield (autoBall); Up/Down still select one.
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "pinheck.h"

PINHECK_INPUT_PORTS_START(jetsons, 3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001, "Left Qualifier",   KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002, "Right Qualifier",  KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004, "L/R Slingshot",    KEYCODE_MINUS)
    COREPORT_BIT(0x0008, "L/R Inlane",       KEYCODE_I)
    COREPORT_BIT(0x0010, "L/R Outlane",      KEYCODE_O)
    COREPORT_BIT(0x0020, "L/R Orbit",        KEYCODE_R)
    COREPORT_BIT(0x0040, "L/R/Lower Pop",    KEYCODE_B)
    COREPORT_BIT(0x0080, "L/R/Middle Lane",  KEYCODE_L)
    COREPORT_BIT(0x0100, "Scoop",            KEYCODE_S)
    COREPORT_BIT(0x0200, "Kickout Hole",     KEYCODE_K)
    COREPORT_BIT(0x0400, "Elroy Loop",       KEYCODE_E)
    COREPORT_BIT(0x0800, "Captive Ball",     KEYCODE_C)
    COREPORT_BIT(0x1000, "Extra Ball",       KEYCODE_X)
    COREPORT_BIT(0x2000, "Drain",            KEYCODE_Q)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001, "Center Target G",  KEYCODE_G)
    COREPORT_BIT(0x0002, "Center Target E",  KEYCODE_H)
    COREPORT_BIT(0x0004, "Center Target O",  KEYCODE_J)
    COREPORT_BIT(0x0008, "Right Target R",   KEYCODE_T)
    COREPORT_BIT(0x0010, "Right Target G",   KEYCODE_Y)
    COREPORT_BIT(0x0020, "Right Target E",   KEYCODE_U)
PINHECK_INPUT_PORTS_END

/* switches: document switch n (0-63) is (n/8+1)*10 + n%8+1; the optos are cabinet inputs 10, 11 and 14 */
#define swROrbit    11
#define swCaptive   12
#define swRLane     13
#define swMLane     14
#define swLLane     15
#define swElroy     16
#define swKickout   18
#define swExtraBall 22
#define swLSling    23
#define swLInlane   24
#define swLOutlane  25
#define swLFlipEOS  26
#define swCTargetG  31
#define swCTargetE  32
#define swCTargetO  33
#define swLowerPop  34
#define swLPop      35
#define swRPop      36
#define swRTargetR  41
#define swRTargetG  42
#define swRTargetE  43
#define swRSling    44
#define swRInlane   45
#define swROutlane  46
#define swShooter   51
#define swTrough2   53
#define swTrough3   54
#define swRFlipEOS  55
#define swLOrbit    56
#define swTrough1   92
#define swScoop     96

/* solenoids: document coil n is PinMAME solenoid n+1 */
#define sRFlipLow   3
#define sRFlipHigh  4
#define sLoad       5
#define sLaunch     6
#define sLFlipHigh  7
#define sLFlipLow   8
#define sScoop      9
#define sPost       17
#define sKickout    20

static struct {
  int since[25];  /* frames since each coil was last on */
  int orbittyUs;  /* servo 0 (the Orbitty topper): last pulse width in us, 0 = none yet */
  int eos[2];     /* left and right flipper end-of-stroke switches as last set, -1 = not yet */
} locals;

static int sol(int n) { return (coreGlobals.solenoids >> (n - 1)) & 1; }

enum { stTrough3 = SIM_FIRSTSTATE, stTrough2, stTrough1, stShooter, stLaunched, stDrain,
       stLOutlane, stROutlane, stLInlane, stRInlane, stLSling, stRSling,
       stScoop, stKickout, stLOrbit, stROrbit, stLOrbitOut, stROrbitOut, stLOrbitBack, stROrbitBack,
       stElroy, stCaptive, stExtraBall, stLLane, stMLane, stRLane, stLPop, stRPop, stLowerPop,
       stCTargetG, stCTargetE, stCTargetO, stRTargetR, stRTargetG, stRTargetE };

static sim_tState jetsons_stateDef[] = {
  {"Not Installed", 0, 0,           0, stDrain,   0, 0, 0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0, 0,           0, 0,         0, 0, 0, SIM_STNOTEXCL},

  {"Trough 3",      1, swTrough3,   0, stTrough2, 3},
  {"Trough 2",      1, swTrough2,   0, stTrough1, 3},
  {"Trough 1",      1, swTrough1,   0, 0,         0},
  {"Shooter Lane",  1, swShooter,   0, 0,         0},
  {"Launched",      1, 0,           0, stFree,   10},
  {"Drain",         1, 0,           0, stTrough3, 1, 0, 0, SIM_STNOTEXCL},

  {"Left Outlane",  1, swLOutlane,  0, stDrain,  10},
  {"Right Outlane", 1, swROutlane,  0, stDrain,  10},
  {"Left Inlane",   1, swLInlane,   0, stFree,    5},
  {"Right Inlane",  1, swRInlane,   0, stFree,    5},
  {"Left Sling",    1, swLSling,    0, stFree,    2},
  {"Right Sling",   1, swRSling,    0, stFree,    2},

  {"Scoop",         1, swScoop,     0, 0,         0},
  {"Kickout Hole",  1, swKickout,   0, 0,         0},
  {"Left Orbit",    1, swLOrbit,    0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Right Orbit",   1, swROrbit,    0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"L Orbit Exit",  1, swROrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Orbit Exit",  1, swLOrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"L Orbit Post",  1, swLOrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Orbit Post",  1, swROrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Elroy Loop",    1, swElroy,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Captive Ball",  1, swCaptive,   0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Extra Ball",    1, swExtraBall, 0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Left Lane",     1, swLLane,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Middle Lane",   1, swMLane,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Right Lane",    1, swRLane,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Left Pop",      1, swLPop,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Right Pop",     1, swRPop,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Lower Pop",     1, swLowerPop,  0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Center G",      1, swCTargetG,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Center E",      1, swCTargetE,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Center O",      1, swCTargetO,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Right R",       1, swRTargetR,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Right G",       1, swRTargetG,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Right E",       1, swRTargetE,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {0}
};

/* frames: the ball meets the up-post 9 frames after the orbit switch and is stopped if the post rose since */
#define ORBIT_POST 9

static int jetsons_handleBallState(sim_tBallStatus *ball, int *inports) {
  (void)inports;
  switch (ball->state) {
    case stTrough1:  if (sol(sLoad) && !core_getSw(swShooter)) return setState(stShooter, 5); break; /* a full lane loses the pulse */
    case stShooter:  if (sol(sLaunch)) return setState(stLaunched, 2); break;
    case stScoop:    if (sol(sScoop)) return setState(stFree, 5); break;
    case stKickout:  if (sol(sKickout)) return setState(stFree, 5); break;
    case stLOrbit:   return setState(locals.since[sPost] < ORBIT_POST ? stLOrbitBack : stLOrbitOut, 3);
    case stROrbit:   return setState(locals.since[sPost] < ORBIT_POST ? stROrbitBack : stROrbitOut, 3);
  }
  return 0;
}

/* mech bit 0: flipper EOS switches, closed while a flipper coil is on */
static void jetsons_handleMech(int mech) {
  static const int coils[2][3] = { { sLFlipHigh, sLFlipLow, swLFlipEOS }, { sRFlipHigh, sRFlipLow, swRFlipEOS } };
  int i;
  for (i = 1; i <= 24; i++)
    locals.since[i] = sol(i) ? 0 : locals.since[i] < 10000 ? locals.since[i] + 1 : 10000;
  if (pinheck_servo(0)) locals.orbittyUs = pinheck_servo(0);
  if (mech & 0x01)
    for (i = 0; i < 2; i++) {
      const int on = sol(coils[i][0]) || sol(coils[i][1]);
      if (on != locals.eos[i]) core_setSw(coils[i][2], locals.eos[i] = on); /* only on change: the switch test may toggle it */
    }
}

/* getMech(0): the Orbitty's angle in degrees (544-2,400 us = 0-180), -1 before its first pulse */
static int jetsons_getMech(int mechNo) {
  if (mechNo == 0) return locals.orbittyUs ? (locals.orbittyUs - 544) * 180 / (2400 - 544) : -1;
  return 0;
}

static sim_tInportData jetsons_inportData[] = {
  {0, 0x0005, stLSling},   {0, 0x0006, stRSling},
  {0, 0x0009, stLInlane},  {0, 0x000a, stRInlane},
  {0, 0x0011, stLOutlane}, {0, 0x0012, stROutlane},
  {0, 0x0021, stLOrbit},   {0, 0x0022, stROrbit},
  {0, 0x0041, stLPop},     {0, 0x0042, stRPop},     {0, 0x0040, stLowerPop},
  {0, 0x0081, stLLane},    {0, 0x0082, stRLane},    {0, 0x0080, stMLane},
  {0, 0x0100, stScoop},    {0, 0x0200, stKickout},  {0, 0x0400, stElroy},
  {0, 0x0800, stCaptive},  {0, 0x1000, stExtraBall},
  {0, 0x2000, stDrain},
  {1, 0x0001, stCTargetG}, {1, 0x0002, stCTargetE}, {1, 0x0004, stCTargetO},
  {1, 0x0008, stRTargetR}, {1, 0x0010, stRTargetG}, {1, 0x0020, stRTargetE},
  {0}
};

static sim_tSimData jetsonsSimData = {
  2,                    /* 2 game specific input ports */
  jetsons_stateDef,
  jetsons_inportData,
  { stTrough1, stTrough2, stTrough3, stDrain, stDrain, stDrain, stDrain },
  NULL,                 /* no init */
  jetsons_handleBallState,
  NULL,                 /* no static drawing */
  FALSE,                /* no manual plunger: the Launch Button fires the autolauncher */
  NULL,                 /* no custom key conditions */
  TRUE                  /* autoBall */
};

static core_tLCDLayout jetsons_disp[] = {
  {0, 0, PINHECK_VIDEO_H64, PINHECK_VIDEO_W, CORE_VIDEO, (genf *)pinheck_video, NULL}, {0}
};

/* the 128x64 module, factory POSITION 55; servo levels 0-255 = 0-180 degrees (0.544-2.4 ms) */
static pinheck_tGameData jetsonsGameData = {
  { GEN_PINHECK, jetsons_disp,
    { FLIP_SWNO(PINHECK_SWLFLIP, PINHECK_SWRFLIP) | FLIP_SOL(FLIP_L), 0, 1, PINHECK_CUSTSOLS, SNDBRD_NONE, 0, 4, 0,
      pinheck_getsol, jetsons_handleMech, jetsons_getMech },
    &jetsonsSimData },
  128, 64, 55, 544, 2400, 0, 1, 0, 3000, 0,
  { sRFlipHigh, sRFlipLow, sLFlipHigh, sLFlipLow }
};

static void init_jetsons(void) {
  int i;
  core_gameData = &jetsonsGameData.core;
  memset(&locals, 0, sizeof(locals));
  for (i = 0; i <= 24; i++) locals.since[i] = 10000;
  for (i = 0; i < 2; i++) locals.eos[i] = -1;
}

PINHECK_ROMSTART(jetsons_004, "JET_V004.PRG", 0x2FF60, CRC(c778cb10) SHA1(a70445cae2e013ac8c14dba7507657e6cf1583ca),
                 "PRP_V002.BIN", CRC(91725a9a) SHA1(c9d4335c0c6872e7d019a58fcdf000a08c93123d))
PINHECK_ROMEND
PINHECK_GAMEDEF(jetsons, 004, "Jetsons, The", 2017, "Spooky Pinball", gl_mPINHECK, 0)
