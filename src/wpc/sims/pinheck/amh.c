// license:BSD-3-Clause

/*******************************************************************************
 America's Most Haunted (Spooky Pinball, 2014): game definition and playfield
 simulator. PIC32 image in Intel HEX (AMH_V023.hex); raw 128x32 DMD (dmdHub).

 Simulated: 4-ball trough with drain kicker and loader, shooter lane (autoplunger
 or manual plunger), basement scoop, Spooky Door (servo 1) with the VUK behind it,
 the Hellevator (servo 0), the ghost loop with opto and magnet, and the flipper
 end-of-stroke switches. Coils read as "on during the last frame".
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys (L/R Ctrl selects the left or right one of a pair):
    +-  L/R Slingshot        +I  L/R Inlane         +O  L/R Outlane (drain)
    +B  L/R Pop Bumper (0/1)  B  Pop Bumper 2       +R  Upper/Lower Left Orbit
    +N  Basement Upper/Lower  S  Basement Scoop      D  Spooky Door (the VUK while the door is open)
     E  Hellevator car        G  Ghost Loop          H  Hotel Path
     V  Elevator Call Button  J  Balcony Jump        F  Balcony Jump that falls short (Pop Path)
     W/K/M  Wiki, Tech, Psychic
     Z/X/C  Ghost Targets 1-3                         T/Y/U  "O", "R", "B" rollovers
     Q  Drain between the flippers                    Space  Plunger (hold, release)
  The keys move a ball on the playfield (autoBall); Up/Down still select one.
------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "pinheck.h"

PINHECK_INPUT_PORTS_START(amh, 4)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001, "Left Qualifier",   KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002, "Right Qualifier",  KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004, "L/R Slingshot",    KEYCODE_MINUS)
    COREPORT_BIT(0x0008, "L/R Inlane",       KEYCODE_I)
    COREPORT_BIT(0x0010, "L/R Outlane",      KEYCODE_O)
    COREPORT_BIT(0x0020, "U/L Left Orbit",   KEYCODE_R)
    COREPORT_BIT(0x0040, "Pop Bumpers",      KEYCODE_B)
    COREPORT_BIT(0x0080, "Basement Scoop",   KEYCODE_S)
    COREPORT_BIT(0x0100, "Spooky Door",      KEYCODE_D)
    COREPORT_BIT(0x0200, "Hellevator",       KEYCODE_E)
    COREPORT_BIT(0x0400, "Ghost Loop",       KEYCODE_G)
    COREPORT_BIT(0x0800, "Hotel Path",       KEYCODE_H)
    COREPORT_BIT(0x1000, "Elevator Call",    KEYCODE_V)
    COREPORT_BIT(0x2000, "Drain",            KEYCODE_Q)
    COREPORT_BIT(0x4000, "U/L Basement",     KEYCODE_N)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001, "Balcony Jump",     KEYCODE_J)
    COREPORT_BIT(0x0002, "Balcony Fail",     KEYCODE_F)
    COREPORT_BIT(0x0008, "Wiki",             KEYCODE_W)
    COREPORT_BIT(0x0010, "Tech",             KEYCODE_K)
    COREPORT_BIT(0x0020, "Psychic",          KEYCODE_M)
    COREPORT_BIT(0x0040, "Ghost Target 1",   KEYCODE_Z)
    COREPORT_BIT(0x0080, "Ghost Target 2",   KEYCODE_X)
    COREPORT_BIT(0x0100, "Ghost Target 3",   KEYCODE_C)
    COREPORT_BIT(0x0200, "O Rollover",       KEYCODE_T)
    COREPORT_BIT(0x0400, "R Rollover",       KEYCODE_Y)
    COREPORT_BIT(0x0800, "B Rollover",       KEYCODE_U)
PINHECK_INPUT_PORTS_END

/* switches: document switch n (0-63) is (n/8+1)*10 + n%8+1; the optos are cabinet inputs 13 and 14 */
#define swWiki       31
#define swTech       32
#define swGhost1     33
#define swGhost2     34
#define swGhost3     35
#define swScoop      37
#define swVUK        38
#define swHotel      45
#define swCall       46
#define swPsychic    47
#define swJumpMade   51
#define swJumpApp    52
#define swPopPath    53
#define swBaseUpper  54
#define swBaseLower  55
#define swPop0       56
#define swULOrbit    57
#define swLLOrbit    58
#define swRollO      61
#define swRollR      62
#define swRollB      63
#define swCar        64
#define swPop2       66
#define swPop1       67
#define swLOutlane   71
#define swLInlane    72
#define swLSling     73
#define swLFlipEOS   74
#define swRFlipEOS   75
#define swRSling     76
#define swRInlane    77
#define swROutlane   78
#define swShooter    82
#define swTrough1    84
#define swTrough2    85
#define swTrough3    86
#define swTrough4    87
#define swDrain      88
#define swLoopOpto   95
#define swDoorOpto   96

/* solenoids: document coil n is PinMAME solenoid n+1 */
#define sMagnet      1
#define sScoop       11
#define sVUK         12
#define sRFlipHigh   17
#define sRFlipHold   18
#define sLFlipHigh   19
#define sLFlipHold   20
#define sLoad        21
#define sDrainKick   22
#define sLaunch      23

/* servos (pulse widths in us as the firmware's servo test sends them): see amh_handleMech */
#define DOOR_OPEN_US  1000  /* DOOR OPEN 5 degrees (593 us), DOOR CLOSE 90 (1,476 us) */
#define CAR_DOWN_US   1400  /* HELL DOWN 10 degrees (647 us), HELL UP 160 (2,200 us) */

static struct {
  int since[25];  /* frames since each coil was last on */
  int us[5];      /* each servo's last pulse width in us, 0 = none yet */
  int ride;       /* frames the Hellevator car has been up with a ball in it */
  int eos[2];     /* left and right flipper end-of-stroke switches as last set, -1 = not yet */
} locals;

static int sol(int n) { return (coreGlobals.solenoids >> (n - 1)) & 1; }
static int doorOpen(void) { return locals.us[1] && locals.us[1] < DOOR_OPEN_US; }
static int carDown(void) { return !locals.us[0] || locals.us[0] < CAR_DOWN_US; }

enum { stTrough4 = SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stShooter, stLaunched, stDrain, stDrainHole,
       stLOutlane, stROutlane, stLInlane, stRInlane, stLSling, stRSling,
       stScoop, stDoor, stDoorHit, stVUK, stCar, stLoop, stMagnet,
       stULOrbit, stLLOrbit, stHotel, stCall, stJumpApp, stJumpMade, stJumpShort, stJumpFail, stBaseUpper, stBaseLower,
       stPop0, stPop1, stPop2, stWiki, stTech, stPsychic, stGhost1, stGhost2, stGhost3, stRollO, stRollR, stRollB };

static sim_tState amh_stateDef[] = {
  {"Not Installed", 0, 0,           0, stDrain,   0, 0, 0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0, 0,           0, 0,         0, 0, 0, SIM_STNOTEXCL},

  {"Trough 4",      1, swTrough4,   0, stTrough3, 3},
  {"Trough 3",      1, swTrough3,   0, stTrough2, 3},
  {"Trough 2",      1, swTrough2,   0, stTrough1, 3},
  {"Trough 1",      1, swTrough1,   0, 0,         0},
  {"Shooter Lane",  1, swShooter,   0, 0,         0},
  {"Launched",      1, 0,           0, stFree,   10},
  {"Drain",         1, 0,           0, stDrainHole, 1, 0, 0, SIM_STNOTEXCL},
  {"Drain Hole",    1, swDrain,     0, 0,         0},

  {"Left Outlane",  1, swLOutlane,  0, stDrain,  10},
  {"Right Outlane", 1, swROutlane,  0, stDrain,  10},
  {"Left Inlane",   1, swLInlane,   0, stFree,    5},
  {"Right Inlane",  1, swRInlane,   0, stFree,    5},
  {"Left Sling",    1, swLSling,    0, stFree,    2},
  {"Right Sling",   1, swRSling,    0, stFree,    2},

  {"Basement Scoop",1, swScoop,     0, 0,         0},
  {"Spooky Door",   1, 0,           0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Door Hit",      1, swDoorOpto,  0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"VUK",           1, swVUK,       0, 0,         0},
  {"Hellevator",    1, swCar,       0, 0,         0},
  {"Ghost Loop",    1, swLoopOpto,  0, 0,         0, 0, 0, SIM_STNOTEXCL},
  {"Magnet",        1, 0,           0, 0,         0},
  {"Upper L Orbit", 1, swULOrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Lower L Orbit", 1, swLLOrbit,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Hotel Path",    1, swHotel,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Elevator Call", 1, swCall,      0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Jump Approach", 1, swJumpApp,   0, stJumpMade,5, 0, 0, SIM_STNOTEXCL},
  {"Jump Made",     1, swJumpMade,  0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Jump Short",    1, swJumpApp,   0, stJumpFail,5, 0, 0, SIM_STNOTEXCL},
  {"Jump Fail",     1, swPopPath,   0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Basement Upper",1, swBaseUpper, 0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Basement Lower",1, swBaseLower, 0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"Pop Bumper 0",  1, swPop0,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Pop Bumper 1",  1, swPop1,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Pop Bumper 2",  1, swPop2,      0, stFree,    2, 0, 0, SIM_STNOTEXCL},
  {"Wiki",          1, swWiki,      0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Tech",          1, swTech,      0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Psychic",       1, swPsychic,   0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Ghost Target 1",1, swGhost1,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Ghost Target 2",1, swGhost2,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"Ghost Target 3",1, swGhost3,    0, stFree,    3, 0, 0, SIM_STNOTEXCL},
  {"O Rollover",    1, swRollO,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"R Rollover",    1, swRollR,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {"B Rollover",    1, swRollB,     0, stFree,    5, 0, 0, SIM_STNOTEXCL},
  {0}
};

/* frames: the ball reaches the magnet 6 frames after the loop opto and is caught if the magnet came on within 12 */
#define MAGNET_CATCH 12
#define MAGNET_DROP  6
#define CAR_RIDE     30  /* frames the car takes from the bottom to the top */

static int amh_handleBallState(sim_tBallStatus *ball, int *inports) {
  (void)inports;
  switch (ball->state) {
    case stTrough1:  if (sol(sLoad) && !core_getSw(swShooter)) return setState(stShooter, 5); break; /* a full lane loses the pulse */
    case stShooter:  if (sol(sLaunch) || sim_getSol(sShooterRel)) return setState(stLaunched, 2); break;
    case stDrainHole:if (sol(sDrainKick)) return setState(stTrough4, 3); break;
    case stScoop:    if (sol(sScoop)) return setState(stFree, 5); break;
    case stDoor:     return setState(doorOpen() ? stVUK : stDoorHit, 3);
    case stVUK:      if (sol(sVUK)) return setState(stFree, 10); break;
    case stCar:      /* the ball rides up in the car and rolls out at the top */
      if (carDown()) locals.ride = 0;
      else if (++locals.ride >= CAR_RIDE) return setState(stFree, 5);
      break;
    case stLoop:     return setState(locals.since[sMagnet] < MAGNET_CATCH ? stMagnet : stFree, 5);
    case stMagnet:   if (locals.since[sMagnet] > MAGNET_DROP) return setState(stFree, 5); break;
  }
  return 0;
}

/* mech bit 0: flipper EOS switches, closed while a flipper coil is on */
static void amh_handleMech(int mech) {
  static const int coils[2][3] = { { sLFlipHigh, sLFlipHold, swLFlipEOS }, { sRFlipHigh, sRFlipHold, swRFlipEOS } };
  int i;
  for (i = 1; i <= 24; i++)
    locals.since[i] = sol(i) ? 0 : locals.since[i] < 10000 ? locals.since[i] + 1 : 10000;
  for (i = 0; i < 5; i++)
    if (pinheck_servo(i)) locals.us[i] = pinheck_servo(i);
  if (mech & 0x01)
    for (i = 0; i < 2; i++) {
      const int on = sol(coils[i][0]) || sol(coils[i][1]);
      if (on != locals.eos[i]) core_setSw(coils[i][2], locals.eos[i] = on); /* only on change: the switch test may toggle it */
    }
}

/* getMech(n): servo n's last pulse width in us, 0 before its first pulse */
static int amh_getMech(int mechNo) {
  return mechNo >= 0 && mechNo < 5 ? locals.us[mechNo] : 0;
}

static sim_tInportData amh_inportData[] = {
  {0, 0x0005, stLSling},   {0, 0x0006, stRSling},
  {0, 0x0009, stLInlane},  {0, 0x000a, stRInlane},
  {0, 0x0011, stLOutlane}, {0, 0x0012, stROutlane},
  {0, 0x0021, stULOrbit},  {0, 0x0022, stLLOrbit},
  {0, 0x0041, stPop0},     {0, 0x0042, stPop1},     {0, 0x0040, stPop2},
  {0, 0x0080, stScoop},    {0, 0x0100, stDoor},     {0, 0x0200, stCar},
  {0, 0x0400, stLoop},     {0, 0x0800, stHotel},    {0, 0x1000, stCall},
  {0, 0x2000, stDrain},
  {0, 0x4001, stBaseUpper},{0, 0x4002, stBaseLower},
  {1, 0x0001, stJumpApp},  {1, 0x0002, stJumpShort},
  {1, 0x0008, stWiki},     {1, 0x0010, stTech},     {1, 0x0020, stPsychic},
  {1, 0x0040, stGhost1},   {1, 0x0080, stGhost2},   {1, 0x0100, stGhost3},
  {1, 0x0200, stRollO},    {1, 0x0400, stRollR},    {1, 0x0800, stRollB},
  {0}
};

static sim_tSimData amhSimData = {
  2,                    /* 2 game specific input ports */
  amh_stateDef,
  amh_inportData,
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },
  NULL,                 /* no init */
  amh_handleBallState,
  NULL,                 /* no static drawing */
  TRUE,                 /* manual plunger (Space) next to the autoplunger */
  NULL,                 /* no custom key conditions */
  TRUE                  /* autoBall */
};

static core_tLCDLayout amh_disp[] = {
  {0, 0, 32, 128, CORE_DMD, NULL, NULL}, {0}
};

/* raw DMD frame at hub $5B0C, servos 0.544-2.4 ms, ghost on on-board LED 2, no in-service record; the 5 s
   boot hold covers the Propeller's main loop starting 3.9 s after power-on.
   TODO: $5B0C is verified for V23 only; for V22 it lies in PROP_022.BIN's VAR area ($3A78-$78D4) but is unconfirmed.
   Only PINHECK_DMD_PROOF and PINHECK_FRAME_LOG read the hub frame; the display is decoded from the scan pins */
static pinheck_tGameData amhGameData = {
  { GEN_PINHECK, amh_disp,
    { FLIP_SWNO(PINHECK_SWLFLIP, PINHECK_SWRFLIP) | FLIP_SOL(FLIP_L), 0, 1, PINHECK_CUSTSOLS, SNDBRD_NONE, 0, 23, 0,
      pinheck_getsol, amh_handleMech, amh_getMech },
    &amhSimData },
  128, 32, 0, 544, 2400, 0, 0, 0x5B0C, 5000, 1,
  { sRFlipHigh, sRFlipHold, sLFlipHigh, sLFlipHold }
};

static void init_amh(void) {
  int i;
  core_gameData = &amhGameData.core;
  memset(&locals, 0, sizeof(locals));
  for (i = 0; i <= 24; i++) locals.since[i] = 10000;
  for (i = 0; i < 2; i++) locals.eos[i] = -1;
  pinheck_flash_hex();
}

PINHECK_HEX_ROMSTART(amh_023, "AMH_V023.hex", 705004, CRC(d5147386) SHA1(adbf469841e4aa5063fe7d7e8fb1d756d932d64d),
                     "PROP_023.BIN", CRC(bd5a99e8) SHA1(763e1e01c663cc8eace3dbe0689da894dc4cadec))
PINHECK_ROMEND
PINHECK_GAMEDEF(amh, 023, "America's Most Haunted (V23)", 2014, "Spooky Pinball", gl_mPINHECKDMD, 0)

PINHECK_HEX_ROMSTART(amh_022, "AMH_V022.hex", 612459, CRC(B74F2A7B) SHA1(4a36e71ba9fcfcd5779645e849babeed5a842c0b),
                     "PROP_022.BIN", CRC(53A6B98B) SHA1(6427841d9f3ac6a744bf86856dfd3faf58e43828))
PINHECK_ROMEND
PINHECK_GAMEDEF(amh, 022, "America's Most Haunted (V22)", 2014, "Spooky Pinball", gl_mPINHECKDMD, 0)
