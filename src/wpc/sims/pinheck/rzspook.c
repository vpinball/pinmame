// license:BSD-3-Clause

/*******************************************************************************
 Rob Zombie's Spookshow International (Spooky Pinball, 2016): game definition
 and playfield simulator.

 Simulated: 7-ball trough, shooter lane (manual plunger and autoplunger), VUK,
 the drop target and the rail lock behind it, the Spaulding gate (servo 0) and
 upper playfield exit, the robot (servo 1), flipper end-of-stroke switches, pops,
 slings and the drain. Coils read as "on during the last frame".
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys (L/R Ctrl selects the left or right one of a pair):
    +-  L/R Lower Slingshot   +=  L/R Upper Slingshot   +I  L/R Inlane
    +O  L/R Outlane (drain)   +R  L/R Orbit             +N  L/R Inner Orbit
    +B  Upper Left/Right Pop   B  Lower Left Pop        +L  L/R Living Dead Girl
     C  Ramp                   V  VUK                    S  Secret Passage
     G  Spaulding (gate)       H  Chicken                J  Gasoline
     T  Ramp Target            E  Extra Ball             A  Rattle
     X  Pop Target             U  Upper Playfield Exit
     Q  Drain between the flippers           Space  Plunger (hold, release)
  The right inner orbit knocks the drop target down while it is up and reaches
  the lock behind it while it is down: the first ball rests on Rail Lower, the
  next on Rail Upper, a third on the right inner orbit switch.
  The keys move a ball on the playfield (autoBall); Up/Down still select one.
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "pinheck.h"

PINHECK_INPUT_PORTS_START(rzspook, 7)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001, "Left Qualifier",   KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002, "Right Qualifier",  KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004, "L/R Slingshot",    KEYCODE_MINUS)
    COREPORT_BIT(0x0008, "L/R Upper Sling",  KEYCODE_EQUALS)
    COREPORT_BIT(0x0010, "L/R Inlane",       KEYCODE_I)
    COREPORT_BIT(0x0020, "L/R Outlane",      KEYCODE_O)
    COREPORT_BIT(0x0040, "L/R Orbit",        KEYCODE_R)
    COREPORT_BIT(0x0080, "L/R Inner Orbit",  KEYCODE_N)
    COREPORT_BIT(0x0100, "Pop Bumpers",      KEYCODE_B)
    COREPORT_BIT(0x0200, "L/R LDG Target",   KEYCODE_L)
    COREPORT_BIT(0x0400, "Ramp",             KEYCODE_C)
    COREPORT_BIT(0x0800, "VUK",              KEYCODE_V)
    COREPORT_BIT(0x1000, "Secret Passage",   KEYCODE_S)
    COREPORT_BIT(0x2000, "Drain",            KEYCODE_Q)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001, "Spaulding",        KEYCODE_G)
    COREPORT_BIT(0x0002, "Chicken",          KEYCODE_H)
    COREPORT_BIT(0x0004, "Gasoline",         KEYCODE_J)
    COREPORT_BIT(0x0008, "Ramp Target",      KEYCODE_T)
    COREPORT_BIT(0x0010, "Extra Ball",       KEYCODE_E)
    COREPORT_BIT(0x0020, "Rattle",           KEYCODE_A)
    COREPORT_BIT(0x0040, "Pop Target",       KEYCODE_X)
    COREPORT_BIT(0x0080, "Upper PF Exit",    KEYCODE_U)
PINHECK_INPUT_PORTS_END

/* switches: document switch n (0-63) is (n/8+1)*10 + n%8+1; the optos are cabinet inputs 13 and 14 */
#define swShooter   11
#define swTrough1   12
#define swTrough2   13
#define swTrough3   14
#define swTrough4   15
#define swTrough5   16
#define swTrough6   17
#define swTrough7   18
#define swROutlane  21
#define swRInlane   22
#define swRSling    23
#define swRFlipEOS  24
#define swLFlipEOS  25
#define swLSling    26
#define swLInlane   27
#define swLOutlane  28
#define swExtraBall 31
#define swRUSling   32
#define swRailLow   33
#define swRailUp    34
#define swRInner    35
#define swRPop      36
#define swChicken   37
#define swGasoline  38
#define swUFlipEOS  41
#define swROrbit    42
#define swVUK       43
#define swSecret    44
#define swLInner    45
#define swRLDG      46
#define swLLDG      47
#define swDrop      48
#define swRampTgt   51
#define swRattle    52
#define swLLPop     54
#define swULPop     55
#define swPopTarget 56
#define swLUSling   57
#define swLOrbit    58
#define swRamp      61
#define swGateOpto  95
#define swExitOpto  96

/* solenoids: document coil n is PinMAME solenoid n+1 */
#define sUFlipHigh  3
#define sVUK        4
#define sPost       5
#define sDrop       7
#define sUFlipLow   8
#define sLFlipLow   13
#define sLFlipHigh  14
#define sLaunch     17
#define sLoad       18
#define sRFlipLow   19
#define sRFlipHigh  20

#define GATE_OPEN_US 1700 /* servo 0: 1,227 us closed, 2,222 us open (servo test, factory settings) */

static struct {
  int drop;       /* drop target down, -1 = not yet set */
  int gateUs;     /* servo 0 (Spaulding gate): last pulse width in us, 0 = none yet */
  int robotUs;    /* servo 1 (robot): last pulse width in us, 0 = none yet */
  int eos[3];     /* upper, right, left flipper end-of-stroke switches as last set, -1 = not yet */
} locals;

static int sol(int n) { return (coreGlobals.solenoids >> (n - 1)) & 1; }

enum { stTrough7 = SIM_FIRSTSTATE, stTrough6, stTrough5, stTrough4, stTrough3, stTrough2, stTrough1,
       stShooter, stLaunched, stDrain,
       stLOutlane, stROutlane, stLInlane, stRInlane, stLSling, stRSling, stLUSling, stRUSling,
       stLOrbit, stROrbit, stLOrbitExit, stROrbitExit, stLInner, stRInnerShot, stRInner,
       stRailUp, stRailLow, stRamp, stVUK, stSecret, stGate, stExit,
       stULPop, stRPop, stLLPop, stLLDG, stRLDG,
       stChicken, stGasoline, stRampTgt, stExtraBall, stRattle, stPopTarget };

static sim_tState rzspook_stateDef[] = {
  {"Not Installed", 0, 0,           0, stDrain,   0, 0, 0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0, 0,           0, 0,         0, 0, 0, SIM_STNOTEXCL},

  {"Trough 7",      1, swTrough7,   0, stTrough6, 3},
  {"Trough 6",      1, swTrough6,   0, stTrough5, 3},
  {"Trough 5",      1, swTrough5,   0, stTrough4, 3},
  {"Trough 4",      1, swTrough4,   0, stTrough3, 3},
  {"Trough 3",      1, swTrough3,   0, stTrough2, 3},
  {"Trough 2",      1, swTrough2,   0, stTrough1, 3},
  {"Trough 1",      1, swTrough1,   0, 0,         0},
  {"Shooter Lane",  1, swShooter,   0, 0,         0},
  {"Launched",      1, 0,           0, stFree,   10},
  {"Drain",         1, 0,           0, stTrough7, 1, 0, 0, SIM_STNOTEXCL},

  {"Left Outlane",  1, swLOutlane,  0, stDrain,  10},
  {"Right Outlane", 1, swROutlane,  0, stDrain,  10},
  {"Left Inlane",   1, swLInlane,   0, stFree,    5},
  {"Right Inlane",  1, swRInlane,   0, stFree,    5},
  {"Left Sling",    1, swLSling,    0, stFree,    2},
  {"Right Sling",   1, swRSling,    0, stFree,    2},
  {"L Upper Sling", 1, swLUSling,   0, stFree,    2},
  {"R Upper Sling", 1, swRUSling,   0, stFree,    2},

  {"Left Orbit",    1, swLOrbit,    0, stLOrbitExit, 5, 0, 0, SIM_STNOTEXCL},
  {"Right Orbit",   1, swROrbit,    0, stROrbitExit, 5, 0, 0, SIM_STNOTEXCL},
  {"L Orbit Exit",  1, swROrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Orbit Exit",  1, swLOrbit,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"L Inner Orbit", 1, swLInner,    0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Inner Shot",  1, 0,           0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"R Inner Orbit", 1, swRInner,    0, stRailUp,  2},
  {"Rail Upper",    1, swRailUp,    0, stRailLow, 2},
  {"Rail Lower",    1, swRailLow,   0, 0,         0},
  {"Ramp",          1, swRamp,      0, stFree,   10, 0, 0, SIM_STNOTEXCL},
  {"VUK",           1, swVUK,       0, 0,         0},
  {"Secret Passage",1, swSecret,    0, stVUK,     5},
  {"Spaulding",     2, swGateOpto,  0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Upper PF Exit", 2, swExitOpto,  0, stFree,   10, 0, 0, SIM_STNOTEXCL},

  {"U Left Pop",    1, swULPop,     0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Right Pop",     1, swRPop,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"L Left Pop",    1, swLLPop,     0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Left LDG",      1, swLLDG,      0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Right LDG",     1, swRLDG,      0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Chicken",       1, swChicken,   0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Gasoline",      1, swGasoline,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Ramp Target",   1, swRampTgt,   0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Extra Ball",    1, swExtraBall, 0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Rattle",        1, swRattle,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Pop Target",    1, swPopTarget, 0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {0}
};

static void rzspook_setDrop(int down) {
  if (down != locals.drop) core_setSw(swDrop, locals.drop = down);
}

static int rzspook_handleBallState(sim_tBallStatus *ball, int *inports) {
  (void)inports;
  switch (ball->state) {
    case stTrough1:  if (sol(sLoad) && !core_getSw(swShooter)) return setState(stShooter, 5); break; /* a full lane loses the pulse */
    case stShooter:  if (sol(sLaunch) || sim_getSol(sShooterRel)) return setState(stLaunched, 2); break;
    case stVUK:      if (sol(sVUK)) return setState(stFree, 5); break;
    case stRInnerShot:
      if (locals.drop == 1) return setState(stRInner, 2);
      rzspook_setDrop(1);    /* the ball knocks the target down and comes back */
      return setState(stFree, 3);
    case stRailLow:  if (sol(sPost)) return setState(stRInlane, 5); break; /* the post drops: one ball rolls on */
    case stGate:     return setState(locals.gateUs > GATE_OPEN_US ? stExit : stFree, 5);
  }
  return 0;
}

/* mech bit 0: drop target; bit 1: flipper EOS switches, closed while a flipper coil is on */
static void rzspook_handleMech(int mech) {
  static const int coils[3][3] = { { sUFlipHigh, sUFlipLow, swUFlipEOS }, { sRFlipHigh, sRFlipLow, swRFlipEOS }, { sLFlipHigh, sLFlipLow, swLFlipEOS } };
  int i;
  if (pinheck_servo(0)) locals.gateUs = pinheck_servo(0);
  if (pinheck_servo(1)) locals.robotUs = pinheck_servo(1);
  if ((mech & 0x01) && (sol(sDrop) || locals.drop < 0)) rzspook_setDrop(0);
  if (mech & 0x02)
    for (i = 0; i < 3; i++) {
      const int on = sol(coils[i][0]) || sol(coils[i][1]);
      if (on != locals.eos[i]) core_setSw(coils[i][2], locals.eos[i] = on); /* only on change: the switch test may toggle it */
    }
}

static int rzspook_getMech(int mechNo) {
  return mechNo == 0 ? locals.gateUs > GATE_OPEN_US : mechNo == 1 ? locals.robotUs : mechNo == 2 ? locals.drop == 1 : 0;
}

static sim_tInportData rzspook_inportData[] = {
  {0, 0x0005, stLSling},   {0, 0x0006, stRSling},
  {0, 0x0009, stLUSling},  {0, 0x000a, stRUSling},
  {0, 0x0011, stLInlane},  {0, 0x0012, stRInlane},
  {0, 0x0021, stLOutlane}, {0, 0x0022, stROutlane},
  {0, 0x0041, stLOrbit},   {0, 0x0042, stROrbit},
  {0, 0x0081, stLInner},   {0, 0x0082, stRInnerShot},
  {0, 0x0101, stULPop},    {0, 0x0102, stRPop},     {0, 0x0100, stLLPop},
  {0, 0x0201, stLLDG},     {0, 0x0202, stRLDG},
  {0, 0x0400, stRamp},     {0, 0x0800, stVUK},      {0, 0x1000, stSecret},
  {0, 0x2000, stDrain},
  {1, 0x0001, stGate},     {1, 0x0002, stChicken},  {1, 0x0004, stGasoline},
  {1, 0x0008, stRampTgt},  {1, 0x0010, stExtraBall},{1, 0x0020, stRattle},
  {1, 0x0040, stPopTarget},{1, 0x0080, stExit},
  {0}
};

static sim_tSimData rzspookSimData = {
  2,                    /* 2 game specific input ports */
  rzspook_stateDef,
  rzspook_inportData,
  { stTrough1, stTrough2, stTrough3, stTrough4, stTrough5, stTrough6, stTrough7 },
  NULL,                 /* no init */
  rzspook_handleBallState,
  NULL,                 /* no static drawing */
  TRUE,                 /* manual plunger (Space) next to the autoplunger */
  NULL,                 /* no custom key conditions */
  TRUE                  /* autoBall */
};

static core_tLCDLayout rzspook_disp[] = {
  {0, 0, PINHECK_VIDEO_H, PINHECK_VIDEO_W, CORE_VIDEO, (genf *)pinheck_video, NULL}, {0}
};

/* factory POSITION 460 (the Propeller's defaults); servo levels 0-255 = 0-180 degrees (0.544-2.4 ms).
   The upper flipper (coils 3/8) is PinMAME's upper right flipper (33/34), confirmed with the coil test (serial command
   [MXXzzz]). Not confirmed yet: that the ROM fires it from the right button, as no test game reached the upper playfield */
static pinheck_tGameData rzspookGameData = {
  { GEN_PINHECK, rzspook_disp,
    { FLIP_SWNO(PINHECK_SWLFLIP, PINHECK_SWRFLIP) | FLIP_SOL(FLIP_L) | FLIP_SOL(FLIP_UR), 0, 1, PINHECK_CUSTSOLS, SNDBRD_NONE, 0, 26, 0,
      pinheck_getsol, rzspook_handleMech, rzspook_getMech },
    &rzspookSimData },
  128, 32, 460, 544, 2400, 0, 1, 0, 3000, 0,
  { sRFlipHigh, sRFlipLow, sLFlipHigh, sLFlipLow, sUFlipHigh, sUFlipLow }
};

static void init_rzspook(void) {
  int i;
  core_gameData = &rzspookGameData.core;
  memset(&locals, 0, sizeof(locals));
  locals.drop = -1;
  for (i = 0; i < 3; i++) locals.eos[i] = -1;
}

PINHECK_ROMSTART(rzspook_026, "RZO_V026.PRG", 0x4B350, CRC(db1ee6f9) SHA1(9bf575f033cfdb17dd5abad5e05735ea24f0ef24),
                 "PRP_V008.BIN", CRC(caeb2c41) SHA1(0393c4bbb6902cf18ef921fb19451e5e1064b6d6))
PINHECK_ROMEND
PINHECK_GAMEDEF(rzspook, 026, "Rob Zombie's Spookshow International", 2016, "Spooky Pinball", gl_mPINHECK, 0)
