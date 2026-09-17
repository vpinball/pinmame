// license:BSD-3-Clause
/*******************************************************************************
 Io Moon (Sleic, 1996) -- game definition and ball simulator.

 PinMAME's convention is that a simulator file owns its game: the input ports,
 the game data, the ROM sets and the CORE_GAMEDEF lines live here rather than in
 sleicgames.c, which keeps Bike Race and Sleic Pin-Ball.

 Contact names and C-numbers are the firmware's own (F16); coil numbers are the
 service manual's, as the Z80 driver latches carry them (F17).  The two coils the
 simulator needs that no Z80 port drives -- ball serve and drop-bank reset, the
 manual's 17 and 18, on the driver expansion board -- are derived by the driver
 from the 80188 commands that fire them (iomoon_getSol in sleic.c).

 Keys for the Io Moon simulator ('+' = with Left or Right Ctrl):
      +-  L/R Shooter (Expulsor 1/2)    +R  Ramp 1 / Ramp 2
      +H  Hole 1 / Hole 2 (Tragabolas)  +B  Bull Eye 1 / 2 (Diana)
   WEDFG  Bumpers 1-5                    J  Jupiter entrance
       I  Inner Bank (Fondo Bancada)     O  Lane 10 (orbit)      K  Lane 11
   ZXCVN  Drop targets Bank A-E     YUASML,./  Lanes 1-9
       Q  Drain                        Del  switch to the matrix test keys
   Space  Plunge the served ball      Down  follow the next ball after a drain

 The lamp matrix (F18) carries two rules no coil can show here: Jupiter holds a
 ball only with every ORBITS lamp lit (3.3.7), and Ramp 1's diverter follows the
 Lagrange pair (3.3.2).  A held ball still leaves on coil 16; the wait is a safety
 net, not the mechanism.

 PLAY-TESTED headless, with scripted keys: all three sets through a whole
 three-ball game -- served ball, plunge, drop targets, an orbit shot, a drain,
 and the next ball's serve, which is where the bank reset raises the targets
 again -- plus Hole 1 leaving on its own coil, Jupiter passing a ball through with
 the ORBITS lights out and holding two with them lit to reach Multiball, and Ramp 1
 reaching MIDDLE only with Lagrange Orbit lit.  Nothing is tested on a machine.
 ******************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sleic.h"

/*------------------
/  Local functions
/-------------------*/
static void iomoon_drawStatic(BMTYPE **line);
static int  iomoon_handleBallState(sim_tBallStatus *ball, int *inports);

/*-------------------
/ Switch definitions
/--------------------
/  Names and C-numbers are the firmware's own (F16).  PinMAME switch numbers come
/  from SLEIC_sw2m: swMatrix[m] bit b is switch (m+4)*10 + b, and the driver puts
/  Z80 column c in swMatrix[1+c], so column 0 is 50-57 and column 5 is 100-103.  */
#define swOutholeC0     50  /* code 0x0A  C6  OUTHOLE 1 -- trough entry, ball-over */
#define swOutholeC1     51  /* code 0x0B  C7  OUTHOLE 2 */
#define swOutholeC2     52  /* code 0x0C  C8  OUTHOLE 3 -- the served end */
#define swBallOut       53  /* code 0x0D  C9  BALL OUT */
#define swLane5         54  /* code 0x0E  C18 */
#define swLane4         55  /* code 0x0F  C17 */
#define swLFlipEOS      56  /* code 0x10  C11 L.C.FLIPPER */
#define swRFlipEOS      57  /* code 0x11  C10 R.C.FLIPPER */
#define swLane11        60  /* code 0x12  C22 */
#define swRamp1Exit     61  /* code 0x13  C21 -- not fitted (2.1.1); no state closes it */
#define swUFlipEOS      62  /* code 0x14  C19 U.C.FLIPPER */
#define swRightShooter  63  /* code 0x15  C16 RIGHT SHOOTER (Expulsor 2) */
#define swLeftShooter   64  /* code 0x16  C15 LEFT SHOOTER  (Expulsor 1) */
#define swLane3         65  /* code 0x17  C14 */
#define swLane2         66  /* code 0x18  C13 */
#define swLane1         67  /* code 0x19  C12 */
#define swLane6         70  /* code 0x1A  C24 */
#define swBankA         71  /* code 0x1B  C25 */
#define swBankB         72  /* code 0x1C  C26 */
#define swBankC         73  /* code 0x1D  C27 */
#define swBankD         74  /* code 0x1E  C28 */
#define swBankE         75  /* code 0x1F  C29 */
#define swInnerBank     76  /* code 0x20  C30 FONDO BANCADA */
#define swHole2         77  /* code 0x21  C31 TRAGABOLAS 2 */
#define swHole1         80  /* code 0x22  C23 TRAGABOLAS 1 */
#define swBumper1       81  /* code 0x23  C33 */
#define swBullEye1      82  /* code 0x24  C32 DIANA 1 */
#define swBumper3       83  /* code 0x25  C35 */
#define swBumper2       84  /* code 0x26  C34 */
#define swBumper5       85  /* code 0x27  C37 */
#define swBumper4       86  /* code 0x28  C36 */
#define swRamp1Ent      87  /* code 0x29  C40 */
#define swJup1          90  /* code 0x2A  C44 */
#define swJup2          91  /* code 0x2B  C45 */
#define swJup3          92  /* code 0x2C  C46 */
#define swRamp2Ent      93  /* code 0x2D  C39 */
#define swBullEye2      94  /* code 0x2E  C38 DIANA 2 */
#define swLane10        95  /* code 0x2F  C48 -- the orbit shot */
#define swRamp1Mid      96  /* code 0x30  C49 MEDIA RAMPA 1 */
#define swJupEnt        97  /* code 0x31  C50 ENTRADA JUPITER */
#define swRamp2Exit    100  /* code 0x34  C47 */
#define swLane9        101  /* code 0x35  C43 */
#define swLane8        102  /* code 0x36  C42 */
#define swLane7        103  /* code 0x37  C41 */

/*---------------------
/ Solenoid definitions
/----------------------
/  1-16 are the two Z80 driver latches, bit b of 0x85 = coil b+1 and of 0x86 =
/  coil b+9 (F17).  51 and 52 are DERIVED: the coils they stand for are on the
/  expansion board, which no Z80 port drives, so the driver reports them from the
/  firmware's own commands (iomoon_getSol in sleic.c) */
#define sLFlipPower      1
#define sLFlipHold       2
#define sRFlipPower      3
#define sRFlipHold       4
#define sUFlipPower      5
#define sUFlipHold       6
#define sBumper1         7
#define sHole1           8   /* Tragabolas 1 */
#define sBumper2         9
#define sBumper3        10
#define sBumper4        11
#define sBumper5        12
#define sTaca           13   /* Hole 1 fires this, then coil 8 (F17 addendum) */
#define sLeftShooter    14   /* Expulsor 1 */
#define sRightShooter   15   /* Expulsor 2 */
#define sJupRelease     16   /* Sueltabolas de Jupiter */
#define sBallServe      CORE_CUSTSOLNO(1)  /* derived from Z80 command 0xE9 */
#define sBankReset      CORE_CUSTSOLNO(2)  /* derived from 0xF3 = ball start   */

/*---------------------
/  Ball state handling
/----------------------*/
enum {
  stTroughC2=SIM_FIRSTSTATE, stTroughC1, stTroughC0, stBallOut, stBallLane,
  stNoStrength, stDrain,
  stLane1, stLane2, stLane3, stLane4, stLane5, stLane6, stLane7, stLane8,
  stLane9, stLane10, stLane11,
  stBumper1, stBumper2, stBumper3, stBumper4, stBumper5,
  stBullEye1, stBullEye2, stLeftShooter, stRightShooter,
  stBankA, stBankB, stBankC, stBankD, stBankE, stInnerBank,
  stHole1, stHole2,
  stRamp1Ent, stRamp1Mid, stRamp2Ent, stRamp2Exit,
  stJupEnt, stJupPass, stJupC44, stJupC45
};

static sim_tState iomoon_stateDef[] = {
  {"Not Installed",   0,0,             0,           stDrain,     0,0,0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",       0,0,             0,           0,           0,0,0, SIM_STNOTEXCL},

  /* The trough, in enum order, so the SERVED end comes first.  Contact 0 is the
     ENTRY and doubles as the ball-over sensor (its Z80 routine sends 0x43, not its
     own code, and only while armed), so a drained ball closes it and a ball in play
     leaves it open.  Balls settle away from the entry -- drain -> C0 -> C1 -> C2 --
     which makes contact 2 the served end, and the complement the firmware sees for
     n balls home the top n bits (F15) */
  {"Outhole 3 (served)",1,swOutholeC2, sBallServe,  stBallOut,   1},
  {"Outhole 2",        1,swOutholeC1,  0,           stTroughC2,  1},
  {"Outhole 1 (entry)",1,swOutholeC0,  0,           stTroughC1,  1},
  {"Ball Out",         1,swBallOut,    sShooterRel, stBallLane,  0,0,0, SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",        1,0,            0,           0,           2,0,0, SIM_STNOTEXCL},
  {"No Strength",      1,0,            0,           stBallOut,   3},
  {"Drain",            1,0,            0,           stTroughC0,  0,0,0, SIM_STNOTEXCL},

  {"Lane 1",           1,swLane1,      0,           stFree,      3},
  {"Lane 2",           1,swLane2,      0,           stFree,      3},
  {"Lane 3",           1,swLane3,      0,           stFree,      3},
  {"Lane 4",           1,swLane4,      0,           stFree,      3},
  {"Lane 5",           1,swLane5,      0,           stFree,      3},
  {"Lane 6 (skill)",   1,swLane6,      0,           stFree,      3},
  {"Lane 7",           1,swLane7,      0,           stFree,      3},
  {"Lane 8",           1,swLane8,      0,           stFree,      3},
  {"Lane 9",           1,swLane9,      0,           stFree,      3},
  {"Lane 10 (orbit)",  1,swLane10,     0,           stFree,      3},
  {"Lane 11",          1,swLane11,     0,           stFree,      3},

  {"Bumper 1",         1,swBumper1,    0,           stFree,      1},
  {"Bumper 2",         1,swBumper2,    0,           stFree,      1},
  {"Bumper 3",         1,swBumper3,    0,           stFree,      1},
  {"Bumper 4",         1,swBumper4,    0,           stFree,      1},
  {"Bumper 5",         1,swBumper5,    0,           stFree,      1},

  {"Bull Eye 1",       1,swBullEye1,   0,           stFree,      3},
  {"Bull Eye 2",       1,swBullEye2,   0,           stFree,      3},
  {"Left Shooter",     1,swLeftShooter,0,           stFree,      1},
  {"Right Shooter",    1,swRightShooter,0,          stFree,      1},

  /* Drop targets: SIM_STSWKEEP, so a hit target stays down after the ball has gone.
     Nothing raises one but iomoon_handleMech below, on the derived bank reset --
     modelled from the manual's rules, not from either ROM (F17) -- so a mode
     needing the bank cannot be completed if targets were already down */
  {"Bank A",           1,swBankA,      0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Bank B",           1,swBankB,      0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Bank C",           1,swBankC,      0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Bank D",           1,swBankD,      0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Bank E",           1,swBankE,      0,           stFree,      3,0,0, SIM_STSWKEEP},
  {"Inner Bank",       1,swInnerBank,  0,           stFree,      3},

  {"Hole 1",           1,swHole1,      sHole1,      stFree,      0},
  /* Hole 2 commands NO coil, so it has no release to wait on: both fields are 0, which
     leaves iomoon_handleBallState to time it out (F17 addendum) */
  {"Hole 2",           1,swHole2,      0,           0,           0},

  /* Ramp 1 is sensed at its entrance and its middle only -- C21 is not fitted.  The
     entrance takes nextState 0 so iomoon_handleBallState can route the diverter */
  {"Ramp 1 Entrance",  1,swRamp1Ent,   0,           0,           3},
  {"Ramp 1 Middle",    1,swRamp1Mid,   0,           stFree,      3},
  {"Ramp 2 Entrance",  1,swRamp2Ent,   0,           stRamp2Exit, 4},
  {"Ramp 2 Exit",      1,swRamp2Exit,  0,           stFree,      3},

  /* Jupiter holds up to two balls (manual 3.3.7), and the two CPUs use different
     contacts for it: a ball ROLLS OVER C46, the only one the Z80 reports (remapped code
     0x44, which the 80188 counts in [4134:0030]), and comes to REST at the far end on
     C44, which is the contact the Z80's own release keys on -- command 0xEE, sub_2B86,
     returns without firing coil 16 unless C44 reads closed.  The second ball rests on
     C45 and rolls down when C44 empties */
  {"Jupiter Entrance", 1,swJupEnt,     0,           stJupPass,   3},
  {"Jupiter (C46)",    1,swJup3,       0,           0,           3},
  {"Jupiter 1 (C44)",  1,swJup1,       sJupRelease, 0,           0,0,0, SIM_STIGNORESOL},
  {"Jupiter 2 (C45)",  1,swJup2,       0,           0,           0},

  {0}
};

/*------------------------------------
/  The two lamp reads the coils cannot
/-------------------------------------*/
/* F18's matrix: the six ORBITS letters are column 2 bits 0-5, the Lagrange pair
   LPA11/LPA12 column 4 bits 2 and 3.  The letters can blink, so they are OR-ed over a
   short window; the pair is steady and never shows both, so it is read as it stands */
#define IOMOON_ORBITS_ALL  0x3f
#define IOMOON_LAMP_WINDOW 8

static struct { UINT8 now, prev; int frame; } iomoon_orbits;

static void iomoon_sampleLamps(void) {
  iomoon_orbits.now |= coreGlobals.lampMatrix[2];
  if (++iomoon_orbits.frame >= IOMOON_LAMP_WINDOW) {
    iomoon_orbits.prev  = iomoon_orbits.now;
    iomoon_orbits.now   = 0;
    iomoon_orbits.frame = 0;
  }
}

/* 3.3.7: Jupiter retains nothing until every ORBITS lamp is lit */
static int iomoon_orbitsLit(void) {
  return ((iomoon_orbits.now | iomoon_orbits.prev) & IOMOON_ORBITS_ALL) == IOMOON_ORBITS_ALL;
}

/* 3.3.2: LPA11 (Lagrange Scape) fires Ramp 1's diverter, LPA12 (Lagrange Orbit) does not */
static int iomoon_diverterFires(void) {
  return (coreGlobals.lampMatrix[4] & 0x0c) == 0x04;
}

/* Frames a held ball waits for its release coil before freeing itself: 15 s at the 60 Hz
   VBLANK.  A safety net for a release that never comes, not a mechanism */
#define IOMOON_HELD_FREE 900

/* Free a held ball on its release coil, and otherwise once the wait runs out.  <sol> 0
   means the device commands no coil at all, so the wait is its only exit */
static int iomoon_releaseHeld(sim_tBallStatus *ball, int sol) {
  if ((sol && core_getSol(sol)) || ++ball->custom > IOMOON_HELD_FREE) {
    ball->custom = 0;
    return setState(stFree, 0);
  }
  return 0;
}

/* The plunger: the cabinet has a manual shooter rod (service manual 6.10, "tirador
   con guias largo"), so plunger strength picks where the ball arrives.  The
   manual's Pasillo 6 entry -- "si no viene de Skill Orbit 100.000 puntos" -- says
   a skill shot feeds Lane 6, which is the medium band here */
static int iomoon_handleBallState(sim_tBallStatus *ball, int *inports) {
  switch (ball->state) {
    case stBallLane:
      if (ball->speed < 8)  return setState(stNoStrength, 7);
      if (ball->speed < 30) return setState(stLane6, 15);
      return setState(stFree, 20);

    /* Hole 2 has no coil: the manual's coil list names one for Tragabolas 1 only, and the
       firmware commands none for this contact, so the ball leaves when the wait ends */
    case stHole2:
      return iomoon_releaseHeld(ball, 0);

    /* Diverted, the ball leaves before RAMP 1 MIDDLE -- which is where the orbit is
       banked (sub_D8E3F -> sub_D8EE1), not at the entrance */
    case stRamp1Ent:
      return iomoon_diverterFires() ? setState(stFree, 4) : setState(stRamp1Mid, 3);

    /* Without the ORBITS lights the ball rolls over C46 and leaves (3.3.7); with them
       it settles at the far end, on the contact the Z80's release keys on */
    case stJupPass:
      if (!iomoon_orbitsLit()) return setState(stFree, 3);
      return core_getSw(swJup1) ? setState(stJupC45, 3) : setState(stJupC44, 3);

    case stJupC44:
      return iomoon_releaseHeld(ball, sJupRelease);

    /* The second ball rolls down as soon as C44 is free */
    case stJupC45:
      ball->custom = 0;
      return core_getSw(swJup1) ? 0 : setState(stJupC44, 3);
  }
  return 0;
}

/*---------------------------
/  Keyboard conversion table
/  Qualified entries first: sim.c takes the first match
/----------------------------*/
static sim_tInportData iomoon_inportData[] = {
  {0, 0x0005, stLeftShooter},
  {0, 0x0006, stRightShooter},
  {0, 0x0009, stRamp1Ent},
  {0, 0x000a, stRamp2Ent},
  {0, 0x0011, stHole1},
  {0, 0x0012, stHole2},
  {0, 0x0021, stBullEye1},
  {0, 0x0022, stBullEye2},
  {0, 0x0040, stBumper1},
  {0, 0x0080, stBumper2},
  {0, 0x0100, stBumper3},
  {0, 0x0200, stBumper4},
  {0, 0x0400, stBumper5},
  {0, 0x0800, stJupEnt},
  {0, 0x1000, stInnerBank},
  {0, 0x2000, stLane10},
  {0, 0x4000, stLane11},
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
  {1, 0x0200, stLane5},
  {1, 0x0400, stLane6},
  {1, 0x0800, stLane7},
  {1, 0x1000, stLane8},
  {1, 0x2000, stLane9},

  {0}
};

/*----------------------
/  Playfield mechanics
/-----------------------*/
/* The drop bank has one reset coil, on the expansion board, which no Z80 port is
   seen driving -- the real drive path is open (F17).  The sim models the manual's
   rule that it pulses at ball start and nowhere else, so this is the ONLY thing
   that raises a target, and the bank cannot be rebuilt mid-ball.  sim.c never
   clears a SIM_STSWKEEP switch itself, which is why the raise lives here rather
   than in the state table */
void iomoon_handleMech(int mech) {
  iomoon_sampleLamps();
  if (core_getSol(sBankReset)) {
    core_setSw(swBankA, FALSE); core_setSw(swBankB, FALSE); core_setSw(swBankC, FALSE);
    core_setSw(swBankD, FALSE); core_setSw(swBankE, FALSE);
  }
}

/*--------------------
  Drawing information
  --------------------*/
static void iomoon_drawStatic(BMTYPE **line) {
  core_textOutf(30, 40,BLACK,"Help on this Simulator:");
  core_textOutf(30, 50,BLACK,"L/R Ctrl+- = L/R Shooter (Expulsor)");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+R = Ramp 1 / Ramp 2");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+H = Hole 1 / Hole 2");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+B = Bull Eye 1 / 2 (Diana)");
  core_textOutf(30, 90,BLACK,"W/E/D/F/G = Bumpers 1-5");
  core_textOutf(30,100,BLACK,"J = Jupiter, I = Inner Bank");
  core_textOutf(30,110,BLACK,"O = Lane 10 (orbit), K = Lane 11");
  core_textOutf(30,120,BLACK,"Z/X/C/V/N = Drop targets A-E");
  core_textOutf(30,130,BLACK,"Y/U/A/S/M/L/,/./ = Lanes 1-9");
  core_textOutf(30,140,BLACK,"Q = Drain,  Del = matrix test keys");
  core_textOutf(30,150,BLACK,"Space = plunge, Down = follow next ball");
  core_textOutf(30,160,BLACK,"Drops rise on ball start (or on Del)");
}

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData iomoonSimData = {
  2,                      /* 2 game specific input ports */
  iomoon_stateDef,        /* Definition of all states */
  iomoon_inportData,      /* Keyboard Entries */
  { stTroughC2, stTroughC1, stTroughC0, stDrain, stDrain, stDrain, stDrain },
  NULL,                   /* Simulator Init */
  iomoon_handleBallState, /* Function to handle ball state changes */
  iomoon_drawStatic,      /* Function to draw the on-screen help */
  TRUE,                   /* Simulate manual shooter -- the cabinet has one */
  NULL                    /* Custom key conditions */
};

/*--------------------------
/ Game specific input ports
/---------------------------*/
/* One playfield, so one set of game ports for all three sets; INPUT_PORTS_START is
   per game, hence the macro.  These must be inports 4 and 5, which is where sim_run
   looks with MDRV_DIPS(8) -- i.e. the last two PORT_STARTs of the block */
#define IOMOON_SIM_PORTS(name) \
  SLEIC2_SIM_INPUT_PORTS_START(name,3) \
    PORT_START /* 4 */ \
      COREPORT_BIT(0x0001,"Left Qualifier",   KEYCODE_LCONTROL) \
      COREPORT_BIT(0x0002,"Right Qualifier",  KEYCODE_RCONTROL) \
      COREPORT_BIT(0x0004,"L/R Shooter",      KEYCODE_MINUS) \
      COREPORT_BIT(0x0008,"L/R Ramp",         KEYCODE_R) \
      COREPORT_BIT(0x0010,"L/R Hole",         KEYCODE_H) \
      COREPORT_BIT(0x0020,"L/R Bull Eye",     KEYCODE_B) \
      COREPORT_BIT(0x0040,"Bumper 1",         KEYCODE_W) \
      COREPORT_BIT(0x0080,"Bumper 2",         KEYCODE_E) \
      COREPORT_BIT(0x0100,"Bumper 3",         KEYCODE_D) \
      COREPORT_BIT(0x0200,"Bumper 4",         KEYCODE_F) \
      COREPORT_BIT(0x0400,"Bumper 5",         KEYCODE_G) \
      COREPORT_BIT(0x0800,"Jupiter Entrance", KEYCODE_J) \
      COREPORT_BIT(0x1000,"Inner Bank",       KEYCODE_I) \
      COREPORT_BIT(0x2000,"Lane 10 (Orbit)",  KEYCODE_O) \
      COREPORT_BIT(0x4000,"Lane 11",          KEYCODE_K) \
      COREPORT_BIT(0x8000,"Drain",            KEYCODE_Q) \
    PORT_START /* 5 */ \
      COREPORT_BIT(0x0001,"Bank A",           KEYCODE_Z) \
      COREPORT_BIT(0x0002,"Bank B",           KEYCODE_X) \
      COREPORT_BIT(0x0004,"Bank C",           KEYCODE_C) \
      COREPORT_BIT(0x0008,"Bank D",           KEYCODE_V) \
      COREPORT_BIT(0x0010,"Bank E",           KEYCODE_N) \
      COREPORT_BIT(0x0020,"Lane 1",           KEYCODE_Y) \
      COREPORT_BIT(0x0040,"Lane 2",           KEYCODE_U) \
      /* A and L are also CORE_PORTS' upper-flipper keys; inert only while this game \
       * declares FLIP_SW(FLIP_L), so adding FLIP_SW(FLIP_UL) would collide here */ \
      COREPORT_BIT(0x0080,"Lane 3",           KEYCODE_A) \
      COREPORT_BIT(0x0100,"Lane 4",           KEYCODE_S) \
      COREPORT_BIT(0x0200,"Lane 5",           KEYCODE_M) \
      COREPORT_BIT(0x0400,"Lane 6",           KEYCODE_L) \
      COREPORT_BIT(0x0800,"Lane 7",           KEYCODE_COMMA) \
      COREPORT_BIT(0x1000,"Lane 8",           KEYCODE_STOP) \
      COREPORT_BIT(0x2000,"Lane 9",           KEYCODE_SLASH) \
  SLEIC_INPUT_PORTS_END

/*-------------------------------------------------------------------
/ Io Moon (1996)
/-------------------------------------------------------------------*/
/* Three balls, which is the firmware's own number rather than a guess: the 80188's
   ball-start path stores 3 into its trough counter [413C:00F9] after a successful ball
   search (DC514, DC587), its 0xEA reply table does the same for the "trough full" answer
   0x3A (DC14D), and the Z80's trough test 2C1F only ever clears with three adjacent
   contacts closed.  With the simulator registered "Balls" is its ball complement and the
   driver's own opt-in trough model stands down (SWITCH_UPDATE(SLEIC2) in sleic.c) */
IOMOON_SIM_PORTS(iomoon)
INITGAME2(iomoon, sleic_dispDMD, 3, &iomoonSimData)
SLEIC_ROMSTART5(iomoon, "v1_3_01.bin", CRC(df80bf4f) SHA1(29547b444cad116c9dc925d6b3112f584df37250),
						"v1_3_02.bin", CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin", CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin", CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin", CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_GAMEDEFNV(iomoon,"Io Moon",1996,"Sleic (Spain)",gl_mSLEIC2,0)

/* An earlier dump of the same 1.3 set.  The stickers on both sets read V1.3 -- the number
   after the dash is the chip position, not a sub-revision -- so the two are told apart by
   content: chip 01 (80188 code + upper graphics) and chip 05 (Z80 I/O code) differ, and
   the graphics and OKI sample ROMs 02/03/04 are byte-identical to the parent set and are
   listed here under the parent's names.  The suffixed file names are this driver's, for
   the same reason bikerac2 renames the two chips it changes: the chips themselves carry
   no label that separates them.

   TESTED to the same depth as the parent and no further: it boots, runs, seeds a blank
   non-volatile store and renders the DMD, and the parts of chip 01 that differ do not
   reach the sound, non-volatile-store or country/pricing code, which is byte-identical
   to the parent's.  What is NOT tested is where the two revisions actually diverge --
   the service-menu dispatch around DD480 in chip 01, and the Z80 trough and port-0x04
   handlers 2BC7/2C1F/2D9D in chip 05.  Those want an interactive play-test */
IOMOON_SIM_PORTS(iomoona)
INITGAME2(iomoona, sleic_dispDMD, 3, &iomoonSimData)
SLEIC_ROMSTART5(iomoona,"v1_3_01e.bin", CRC(00a75790) SHA1(3af7a5c10a8c1687a212a01393cc9195a04a73c9),
						"v1_3_02.bin",  CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin",  CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin",  CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05e.bin", CRC(dd5145f5) SHA1(7de0b9582e5130cd1eafb1c0038ee7c9ce7b3ec2))
SLEIC_ROMEND
CORE_CLONEDEFNV(iomoona,iomoon,"Io Moon (earlier ROM revision)",1996,"Sleic (Spain)",gl_mSLEIC2,0)

/* Tournament MOD of the parent set: chip 01 patched so the end of a game asks for PRESS
   START instead of dropping straight back to attract, which is what a tournament wants
   between players.  Only chip 01 changes; 02-05 are the parent's.  The patch is 186
   bytes in four regions -- a 168-byte and an 11-byte block of new code at C0010-C00B7
   and C00D0-C00DA, reached by two four-byte hooks planted at D5077 and D5123 */
IOMOON_SIM_PORTS(iomoont)
INITGAME2(iomoont, sleic_dispDMD, 3, &iomoonSimData)
SLEIC_ROMSTART5(iomoont,"v1_3_01t.bin", CRC(42cafcda) SHA1(0ac3dd882748bc86a3b66aff2d286eecd8d24a4b),
						"v1_3_02.bin",  CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin",  CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin",  CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin",  CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_CLONEDEFNV(iomoont,iomoon,"Io Moon (PRESS START tournament MOD)",1996,"Sleic (Spain)",gl_mSLEIC2,0)
