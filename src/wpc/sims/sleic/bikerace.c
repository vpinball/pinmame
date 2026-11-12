// license:BSD-3-Clause
/*******************************************************************************
 Bike Race (Sleic, 1992) -- ball simulator and the V4.1 game definitions.

 The simulator follows the V4.1 Z80 ROM (bk07); the parent and bikerac2 stay in
 sleicgames.c without one.  Contact names are the firmware's (chip 04, F000:370B),
 coil numbers the service manual's (2.3.1).

 The trough holds two balls.  Coil 6 lifts the outhole ball (C6) to the waiting
 position (C7), and coil 7 releases that ball into the shooter lane (C8).  A ball
 reaching C6 in play is the drain.

 Keys, one per shot:
       R  Ramp 1 (C20 -> C44 -> Lane 12, one lap)   H  Expulsor 1 (C22)
       B  Captive ball (C19)        W/E  Bumpers 1/2        D  Slingshots (C25)
 F/G/J/K  Targets 1-4 (Dianas)        I  Bank back target (C34)
   ZXCVN  Drop targets A-E      YUAS  Lanes 1-4   M  Spinner (Lane 5)  L,./ Lanes 6-9
       O  Lane 10 -- the left outlane: the kickback returns the ball, or it drains
     [ ]  Lanes 11/12          ; '  Lanes 13/14          \  Lane 15
       Q  Drain              Space  plunge            Del  matrix test keys
 ******************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sleic.h"

/*------------------
/  Local functions
/-------------------*/
static void bikerace_drawStatic(BMTYPE **line);
static int  bikerace_handleBallState(sim_tBallStatus *ball, int *inports);

/*-------------------
/ Switch definitions
/--------------------
/  Z80 code 0x0A + 8c + r is PinMAME switch 50 + 10c + r (SLEIC_sw2m) */
#define swLane1         50  /* 0x0A C35 PASILLO 1 */
#define swLane2         51  /* 0x0B C36 PASILLO 2 */
#define swLane3         52  /* 0x0C C37 PASILLO 3 */
#define swLane4         53  /* 0x0D C38 PASILLO 4 */
#define swLane6         54  /* 0x0E C40 PASILLO 6 */
#define swLane7         55  /* 0x0F C41 PASILLO 7 */
#define swLane8         56  /* 0x10 C42 PASILLO 8 */
#define swLane9         57  /* 0x11 C43 PASILLO 9 */
#define swBallLeaving   60  /* 0x12 C26 BOLA SALIENDO -- up the shooter lane */
#define swBankBack      61  /* 0x13 C34 FONDO BANCADA */
#define swSpinner       62  /* 0x14 C39 VELETA (Pasillo 5) */
#define swBankA         63  /* 0x15 C29 DIANA BANCADA A */
#define swBankB         64  /* 0x16 C30 */
#define swBankC         65  /* 0x17 C31 */
#define swBankD         66  /* 0x18 C32 */
#define swBankE         67  /* 0x19 C33 */
#define swBumper1       71  /* 0x1B C27 */
#define swBumper2       72  /* 0x1C C28 */
#define swTarget2       73  /* 0x1D C21 DIANA 2 */
#define swRamp1Ent      74  /* 0x1E C20 ENTRADA RAMPA 1 */
#define swTarget1       76  /* 0x20 C18 DIANA 1 */
#define swCaptive       77  /* 0x21 C19 BOLA CAUTIVA */
#define swLane10        80  /* 0x22 C9  PASILLO 10 -- left outlane, kickback */
#define swLane11        81  /* 0x23 C12 PASILLO 11 */
#define swLane12        82  /* 0x24 C13 PASILLO 12 -- the end of a lap */
#define swLane13        85  /* 0x27 C14 PASILLO 13 */
#define swLane14        86  /* 0x28 C15 PASILLO 14 */
#define swLane15        87  /* 0x29 C16 PASILLO 15 */
#define swRamp1Exit     90  /* 0x2A C44 SALIDA RAMPA 1 */
#define swSlings        91  /* 0x2B C25 BANDAS */
#define swKicker1       92  /* 0x2C C22 EXPULSOR 1 */
#define swTarget3       93  /* 0x2D C23 DIANA 3 */
#define swTarget4       94  /* 0x2E C24 DIANA 4 */
#define swOuthole       95  /* 0x2F C6  SALIDA BOLAS */
#define swBallWaiting   96  /* 0x30 C7  BOLA EN ESPERA */
#define swShooter       97  /* 0x31 C8  BOLA FUERA */

/*---------------------
/ Solenoid definitions
/----------------------
/  Port 0x85 bit b is solenoid b+1, port 0x86 bit b solenoid b+9 */
#define sBankReset       9  /* coil 12 Bancada de Dianas */
#define sBallRelease    10  /* coil 7  Multibola: C7 -> shooter lane */
#define sKicker1        13  /* coil 9  Expulsor 1 */
#define sKickback       14  /* coil 5  Expulsor 2 de Cañón, Pasillo 10 */
#define sOuthole        15  /* coil 6  Salida de Bolas: outhole -> C7 */

/*---------------------
/  Ball state handling
/----------------------*/
enum {
  stOuthole=SIM_FIRSTSTATE, stBallWaiting, stShooter, stBallLane, stNoStrength,
  stBallLeaving, stDrain,
  stLane1, stLane2, stLane3, stLane4, stSpinner, stLane6, stLane7, stLane8, stLane9,
  stLane10, stLane11, stLane12, stLane13, stLane14, stLane15,
  stBankA, stBankB, stBankC, stBankD, stBankE, stBankBack,
  stBumper1, stBumper2, stSlings, stTarget1, stTarget2, stTarget3, stTarget4,
  stCaptive, stKicker1, stRamp1Ent, stRamp1Exit
};

static sim_tState bikerace_stateDef[] = {
  {"Not Installed",    0,0,             0,           stDrain,     0,0,0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",        0,0,             0,           0,           0,0,0, SIM_STNOTEXCL},

  /* Coil 6 lifts the outhole ball only while C7 is empty (bikerace_handleBallState) */
  {"Outhole (C6)",     1,swOuthole,     0,           0,           0},
  {"Ball Waiting (C7)",1,swBallWaiting, sBallRelease,stShooter,   3},
  {"Shooter (C8)",     1,swShooter,     sShooterRel, stBallLane,  0,0,0, SIM_STSHOOT},
  {"Ball Lane",        1,0,             0,           0,           0},
  {"No Strength",      1,0,             0,           stShooter,   3},
  {"Ball Leaving (C26)",1,swBallLeaving,0,           stFree,      5},
  {"Drain",            1,0,             0,           stOuthole,   3,0,0, SIM_STNOTEXCL},

  {"Lane 1",           1,swLane1,       0,           stFree,      3},
  {"Lane 2",           1,swLane2,       0,           stFree,      3},
  {"Lane 3",           1,swLane3,       0,           stFree,      3},
  {"Lane 4",           1,swLane4,       0,           stFree,      3},
  {"Spinner (Lane 5)", 1,swSpinner,     0,           stFree,      3},
  {"Lane 6",           1,swLane6,       0,           stFree,      3},
  {"Lane 7",           1,swLane7,       0,           stFree,      3},
  {"Lane 8",           1,swLane8,       0,           stFree,      3},
  {"Lane 9",           1,swLane9,       0,           stFree,      3},
  /* The left outlane waits for the kickback (coil 5), which fires only while LC19 is lit
     (manual 3.2.10); without it the ball drains */
  {"Lane 10 (kickback)",0,swLane10,     0,           0,           0},
  {"Lane 11",          1,swLane11,      0,           stFree,      3},
  {"Lane 12",          1,swLane12,      0,           stFree,      3},
  {"Lane 13",          1,swLane13,      0,           stFree,      3},
  {"Lane 14",          1,swLane14,      0,           stFree,      3},
  {"Lane 15",          1,swLane15,      0,           stFree,      3},

  /* Drop targets stay down until the bank's reset coil raises them (bikerace_handleMech) */
  {"Drop Target A",    1,swBankA,       0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Drop Target B",    1,swBankB,       0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Drop Target C",    1,swBankC,       0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Drop Target D",    1,swBankD,       0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Drop Target E",    1,swBankE,       0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Bank Back Target", 1,swBankBack,    0,           stFree,      3},

  {"Bumper 1",         1,swBumper1,     0,           stFree,      1},
  {"Bumper 2",         1,swBumper2,     0,           stFree,      1},
  {"Slingshots",       1,swSlings,      0,           stFree,      1},
  {"Target 1",         1,swTarget1,     0,           stFree,      3},
  {"Target 2",         1,swTarget2,     0,           stFree,      3},
  {"Target 3",         1,swTarget3,     0,           stFree,      3},
  {"Target 4",         1,swTarget4,     0,           stFree,      3},
  {"Captive Ball",     1,swCaptive,     0,           stFree,      3},

  /* Held until coil 9 throws the ball back towards the bumpers (manual 3.6) */
  {"Expulsor 1 (C22)", 1,swKicker1,     sKicker1,    stBumper1,   3},

  /* One lap: the ramp's entrance and exit, then Lane 12, which ends the lap (3.1.2) */
  {"Ramp 1 Entrance",  1,swRamp1Ent,    0,           stRamp1Exit, 6},
  {"Ramp 1 Exit",      1,swRamp1Exit,   0,           stLane12,    8},

  {0}
};

/* Frames the left outlane waits for the kickback before the ball drains */
#define BIKERACE_KICKBACK_WAIT 30

static int bikerace_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    case stOuthole:
      if (core_getSol(sOuthole) && !core_getSw(swBallWaiting)) return setState(stBallWaiting, 3);
      return 0;

    case stBallLane:
      if (ball->speed < 8) return setState(stNoStrength, 5);
      return setState(stBallLeaving, 3);

    case stLane10:
      if (core_getSol(sKickback)) { ball->custom = 0; return setState(stFree, 5); }
      if (++ball->custom > BIKERACE_KICKBACK_WAIT) { ball->custom = 0; return setState(stDrain, 3); }
      return 0;
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData bikerace_inportData[] = {
  {0, 0x0001, stRamp1Ent},
  {0, 0x0002, stKicker1},
  {0, 0x0004, stCaptive},
  {0, 0x0008, stBumper1},
  {0, 0x0010, stBumper2},
  {0, 0x0020, stSlings},
  {0, 0x0040, stTarget1},
  {0, 0x0080, stTarget2},
  {0, 0x0100, stTarget3},
  {0, 0x0200, stTarget4},
  {0, 0x0400, stBankBack},
  {0, 0x0800, stLane10},
  {0, 0x1000, stLane11},
  {0, 0x2000, stLane12},
  {0, 0x4000, stLane15},
  {0, 0x8000, stDrain},

  {1, 0x0001, stBankA},
  {1, 0x0002, stBankB},
  {1, 0x0004, stBankC},
  {1, 0x0008, stBankD},
  {1, 0x0010, stBankE},
  {1, 0x0020, stLane1},
  {1, 0x0040, stLane2},
  {1, 0x0080, stLane3},
  {1, 0x0100, stLane4},
  {1, 0x0200, stSpinner},
  {1, 0x0400, stLane6},
  {1, 0x0800, stLane7},
  {1, 0x1000, stLane8},
  {1, 0x2000, stLane9},
  {1, 0x4000, stLane13},
  {1, 0x8000, stLane14},
  {0}
};

/*----------------------
/  Playfield mechanics
/-----------------------*/
/* sim.c never clears a SIM_STSWKEEP switch itself, so the bank's reset coil does it here */
void bikerace_handleMech(int mech) {
  if (core_getSol(sBankReset)) {
    core_setSw(swBankA, FALSE); core_setSw(swBankB, FALSE); core_setSw(swBankC, FALSE);
    core_setSw(swBankD, FALSE); core_setSw(swBankE, FALSE);
  }
}

/*--------------------
  Drawing information
  --------------------*/
static void bikerace_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"R = Ramp 1 (a lap), H = Expulsor 1");
  core_textOutf(30, 60,BLACK,"B = Captive ball, D = Slingshots");
  core_textOutf(30, 70,BLACK,"W/E = Bumpers 1/2");
  core_textOutf(30, 80,BLACK,"F/G/J/K = Targets 1-4, I = Bank back");
  core_textOutf(30, 90,BLACK,"Z/X/C/V/N = Drop targets A-E");
  core_textOutf(30,100,BLACK,"Y/U/A/S = Lanes 1-4, M = Spinner");
  core_textOutf(30,110,BLACK,"L/,/./ / = Lanes 6-9");
  core_textOutf(30,120,BLACK,"O = Lane 10 (left outlane, kickback)");
  core_textOutf(30,130,BLACK,"[/] = Lanes 11/12, ;/' = Lanes 13/14");
  core_textOutf(30,140,BLACK,"\\ = Lane 15, Q = Drain");
  core_textOutf(30,150,BLACK,"Space = plunge, Del = matrix test keys");
}

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData bikeraceSimData = {
  2,                        /* 2 game specific input ports */
  bikerace_stateDef,        /* Definition of all states */
  bikerace_inportData,      /* Keyboard Entries */
  { stBallWaiting, stOuthole, stDrain, stDrain, stDrain, stDrain, stDrain },
  NULL,                     /* Simulator Init */
  bikerace_handleBallState, /* Function to handle ball state changes */
  bikerace_drawStatic,      /* Function to draw the on-screen help */
  TRUE,                     /* Simulate manual shooter */
  NULL,                     /* Custom key conditions */
  TRUE                      /* the shot keys move the ball on the playfield */
};

/*--------------------------
/ Game specific input ports
/---------------------------*/
/* Inports 4 and 5, the last two, where sim_run looks with MDRV_DIPS(8) */
#define BIKERACE_SIM_PORTS(name) \
  SLEIC_INPUT_PORTS_START(name,2) \
    PORT_START /* 4 */ \
      COREPORT_BIT(0x0001,"Ramp 1",            KEYCODE_R) \
      COREPORT_BIT(0x0002,"Expulsor 1",        KEYCODE_H) \
      COREPORT_BIT(0x0004,"Captive Ball",      KEYCODE_B) \
      COREPORT_BIT(0x0008,"Bumper 1",          KEYCODE_W) \
      COREPORT_BIT(0x0010,"Bumper 2",          KEYCODE_E) \
      COREPORT_BIT(0x0020,"Slingshots",        KEYCODE_D) \
      COREPORT_BIT(0x0040,"Target 1",          KEYCODE_F) \
      COREPORT_BIT(0x0080,"Target 2",          KEYCODE_G) \
      COREPORT_BIT(0x0100,"Target 3",          KEYCODE_J) \
      COREPORT_BIT(0x0200,"Target 4",          KEYCODE_K) \
      COREPORT_BIT(0x0400,"Bank Back Target",  KEYCODE_I) \
      COREPORT_BIT(0x0800,"Lane 10 (kickback)",KEYCODE_O) \
      COREPORT_BIT(0x1000,"Lane 11",           KEYCODE_OPENBRACE) \
      COREPORT_BIT(0x2000,"Lane 12",           KEYCODE_CLOSEBRACE) \
      COREPORT_BIT(0x4000,"Lane 15",           KEYCODE_BACKSLASH) \
      COREPORT_BIT(0x8000,"Drain",             KEYCODE_Q) \
    PORT_START /* 5 */ \
      COREPORT_BIT(0x0001,"Drop Target A",     KEYCODE_Z) \
      COREPORT_BIT(0x0002,"Drop Target B",     KEYCODE_X) \
      COREPORT_BIT(0x0004,"Drop Target C",     KEYCODE_C) \
      COREPORT_BIT(0x0008,"Drop Target D",     KEYCODE_V) \
      COREPORT_BIT(0x0010,"Drop Target E",     KEYCODE_N) \
      COREPORT_BIT(0x0020,"Lane 1",            KEYCODE_Y) \
      COREPORT_BIT(0x0040,"Lane 2",            KEYCODE_U) \
      /* A and L are also CORE_PORTS' upper-flipper keys, inert under FLIP_SW(FLIP_L) */ \
      COREPORT_BIT(0x0080,"Lane 3",            KEYCODE_A) \
      COREPORT_BIT(0x0100,"Lane 4",            KEYCODE_S) \
      COREPORT_BIT(0x0200,"Spinner (Lane 5)",  KEYCODE_M) \
      COREPORT_BIT(0x0400,"Lane 6",            KEYCODE_L) \
      COREPORT_BIT(0x0800,"Lane 7",            KEYCODE_COMMA) \
      COREPORT_BIT(0x1000,"Lane 8",            KEYCODE_STOP) \
      COREPORT_BIT(0x2000,"Lane 9",            KEYCODE_SLASH) \
      COREPORT_BIT(0x4000,"Lane 13",           KEYCODE_COLON) \
      COREPORT_BIT(0x8000,"Lane 14",           KEYCODE_QUOTE) \
  SLEIC_INPUT_PORTS_END

/* V4.1: chips 03, 04 and 07 differ from the parent.  02 and 05 are the parent's, verified
/  against a complete pull off a V4.1 machine, and 06 by a re-dump (the first read, CRC
/  ad48a30a, was bad; archived at sleic-iomoon/roms/related-machines/bike-race/v4.1/) */
BIKERACE_SIM_PORTS(bikerac3)
INITGAME3(bikerac3, sleic_dispDMD, &bikeraceSimData)
SLEIC_ROMSTART7(bikerac3,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04.bin",    CRC(33fd212e) SHA1(9471e34fc4280741816d65f88590febc9e8629a7),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac3,bikerace,"Bike Race (V4.1)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* MOD of V4.1, chip 04 only.  FREE PLAY: a credit always stands (hook at E059F, the
/  triplicated-credit read).  PRESS START: the finished game's scores stay on the panel
/  until START is pressed (hooks at F000:10F2 and E8C5D).  Generated by
/  sleic-iomoon/scripts/bike_race_free_play_patch.py and bike_race_press_start_patch.py */
BIKERACE_SIM_PORTS(bikerc3f)
INITGAME3(bikerc3f, sleic_dispDMD, &bikeraceSimData)
SLEIC_ROMSTART7(bikerc3f,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04f.bin",   CRC(7626564e) SHA1(d69c5dd794c55d91d4dde5c6c07009bb1ebe2c41),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerc3f,bikerace,"Bike Race (V4.1, press start MOD, free play)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* bikerc3f in English: chips 04, 05 and 06 change, and the attract's credit screen
/  shows Freeplay.  Generated by sleic-iomoon/scripts/bike_race_english_patch.py */
BIKERACE_SIM_PORTS(bikerc3e)
INITGAME3(bikerc3e, sleic_dispDMD, &bikeraceSimData)
SLEIC_ROMSTART7(bikerc3e,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04e.bin",   CRC(99006ab6) SHA1(41e2604d3a6a75b86b1c43776d4c815629bc5ffe),
						 "bk05e.bin",   CRC(590aced9) SHA1(1f81cae3e6efc150fd88488a25e615b488401a22),
						 "bk06e.bin",   CRC(47f41c51) SHA1(7508cb5476c3881c465c6303c7776fac9df9b0a9),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerc3e,bikerace,"Bike Race (V4.1, press start MOD, free play, English)",1992,"Sleic (Spain)",gl_mSLEIC3,0)
