/*******************************************************************************
 Black Knight Pinball Simulator

******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for the Black Kniigh Simulator:
  -----------------------------------
      +I   L/R Inlane
      +O   L/R Outlane
      +-   L/R Slingshot
      +R   L/R Ramp
      B    Bumper
      T    Turnaround
      L    Lock
      Q    SDTM (Drain Ball)
      +ASD Upper L/R drop targets
      +ZXC Lower L/R drop targets
      E    Eject Hole
-----------------------------------------------------------------------------*/
#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s7.h"
#include "sim.h"

/*------------------
/  Local functions
/-------------------*/
static void init_bk(void);
//static int  bk_handleBallState(sim_tBallStatus *ball, int *inports);

/*--------------------------
/ Game specific input ports
/---------------------------*/
S7_INPUT_PORTS_START(bk,3)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",   KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",  KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"L/R Inlane",       KEYCODE_I)
    COREPORT_BIT(0x0008,"L/R Outlane",      KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",    KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Ramp",         KEYCODE_R)
    COREPORT_BIT(0x0040,"L/R Upper Drop 1", KEYCODE_A)
    COREPORT_BIT(0x0080,"L/R Upper Drop 2", KEYCODE_S)
    COREPORT_BIT(0x0100,"L/R Upper Drop 3", KEYCODE_D)
    COREPORT_BIT(0x0200,"L/R Lower Drop 1", KEYCODE_Z)
    COREPORT_BIT(0x0400,"L/R Lower Drop 2", KEYCODE_X)
    COREPORT_BIT(0x0800,"L/R Lower Drop 3", KEYCODE_C)
    COREPORT_BIT(0x1000,"Bumper",           KEYCODE_B)
    COREPORT_BIT(0x2000,"Turnaround",       KEYCODE_T)
    COREPORT_BIT(0x4000,"Drain",            KEYCODE_Q)
    COREPORT_BIT(0x8000,"Lock",             KEYCODE_L)
  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"Eject hole",       KEYCODE_E)
S7_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swTilt         1
#define swBallTilt     2
#define swStart        3
#define swRCoin        4
#define swCCoin        5
#define swLCoin        6
#define swSlamTilt     7
#define swHiscoreReset 8
#define swRMagnet      9
#define swLMagnet     10
#define swLOutlane    11
#define swROutlane    12
#define swSpinner     13
#define swRRamp       14
#define swRInlane     15
#define swLInlane     16
#define swRTrough     17
#define swCTrough     18
#define swLTrough     19
#define swOuthole     20
#define swLSling      21
#define swRSling      22
#define swTurnAround  23
#define swEject       24
#define swLL1Drop     25
#define swLL2Drop     26
#define swLL3Drop     27
#define swLR1Drop     29
#define swLR2Drop     30
#define swLR3Drop     31
#define swUL1Drop     33
#define swUL2Drop     34
#define swUL3Drop     35
#define swBumper      36
#define swUR1Drop     37
#define swUR2Drop     38
#define swUR3Drop     39
#define swLLock       41
#define swCLock       42
#define swULock       43
#define swLRamp       44
#define swShooter     45

/*---------------------
/ Solenoid definitions
/----------------------*/
#define sOuthole     1 // Ball release
#define sLLDropReset 2
#define sLRDropReset 3
#define sULDropReset 4
#define sURDropReset 5
#define sBallRel     6 // Ramp thrower
#define sLockRel     7 // Multiball release
#define sEjectRel    8
#define sRMagnet     9
#define sLMagnet    10
#define sKnocker    11
#define sBell       15 //??
#define sCoinLock   16
#define sLSling     17
#define sRSling     18
#define sBumper     19

/*---------------------
/  Ball state handling
/----------------------*/
enum {stOuthole=SIM_FIRSTSTATE, stLTrough, stCTrough, stRTrough, stShooter,
     stDrain, stROutlane, stLOutlane, stULock, stCLock, stLLock, stEject,
     stUL1Drop,stUL2Drop,stUL3Drop,
     stUR1Drop,stUR2Drop,stUR3Drop,
     stLL1Drop,stLL2Drop,stLL3Drop,
     stLR1Drop,stLR2Drop,stLR3Drop
};

static sim_tState bk_stateDef[] = {
  {"Not Installed", 0,0,         0,           stDrain,    0, 0,  0,  SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",     0,0,         0,           0,          0, 0,  0,  SIM_STNOTEXCL},

  {"Outhole",       1,swOuthole, sOuthole,    stLTrough, 1},
  {"Trough L",      1,swLTrough, 0,           stCTrough, 5},
  {"Trough C",      1,swCTrough, 0,           stRTrough, 5},
  {"Trough R",      1,swRTrough, sBallRel,    stShooter, 5},
  {"Shooter",       1,0,         sShooterRel, stFree,    0,  0,  0,  SIM_STNOTEXCL|SIM_STSHOOT},

  {"Drain",         1,0,         0,           stOuthole, 5,  0,  0,  SIM_STNOTEXCL},
  {"Right Outlane", 1,swROutlane,0,           stDrain,  15},
  {"Left Outlane",  1,swLOutlane,0,           stDrain,  15},

  {"Lock Top",      1,swULock,   0,           stCLock,   5},
  {"Lock Center",   1,swCLock,   0,           stLLock,   5},
  {"Lock Bottom",   1,swLLock,   sLockRel,    stFree,    5},

  {"Eject hole",    1,swEject,   sEjectRel,   stFree,    5},

  {"",              5,swUL1Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swUL2Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swUL3Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swUR1Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swUR2Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swUR3Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLL1Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLL2Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLL3Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLR1Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLR2Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {"",              5,swLR3Drop, 0,           stFree,      5, 0,  0, SIM_STSWKEEP},
  {0}
};
static void bk_handleMech(int mech) {
  if (core_getSol(sLLDropReset)) {
    core_setSw(swLL1Drop, FALSE); core_setSw(swLL2Drop, FALSE); core_setSw(swLL3Drop, FALSE);
  }
  if (core_getSol(sLRDropReset)) {
    core_setSw(swLR1Drop, FALSE); core_setSw(swLR2Drop, FALSE); core_setSw(swLR3Drop, FALSE);
  }
  if (core_getSol(sULDropReset)) {
    core_setSw(swUL1Drop, FALSE); core_setSw(swUL2Drop, FALSE); core_setSw(swUL3Drop, FALSE);
  }
  if (core_getSol(sURDropReset)) {
    core_setSw(swUR1Drop, FALSE); core_setSw(swUR2Drop, FALSE); core_setSw(swUR3Drop, FALSE);
  }
}
/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData bk_inportData[] = {
  /* Port 0 */
  {0, 0x0005, swLInlane},
  {0, 0x0006, swRInlane},
  {0, 0x0009, stLOutlane},
  {0, 0x000a, stROutlane},
  {0, 0x0011, swLSling},
  {0, 0x0012, swRSling},
  {0, 0x0021, swLRamp},
  {0, 0x0022, swRRamp},
  {0, 0x0041, stUL1Drop},
  {0, 0x0081, stUL2Drop},
  {0, 0x0101, stUL3Drop},
  {0, 0x0042, stUR1Drop},
  {0, 0x0082, stUR2Drop},
  {0, 0x0102, stUR3Drop},
  {0, 0x0201, stLL1Drop},
  {0, 0x0401, stLL2Drop},
  {0, 0x0801, stLL3Drop},
  {0, 0x0202, stLR1Drop},
  {0, 0x0402, stLR2Drop},
  {0, 0x0802, stLR3Drop},
  {0, 0x1000, swBumper},
  {0, 0x2000, swTurnAround},
  {0, 0x4000, stDrain},
  {0, 0x8000, stULock},
  {1, 0x0001, stEject},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void bk_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+ASD = L/R Upper Drop targets");
  core_textOutf(30,100,BLACK,"L/R Ctrl+ZXC = L/R Upper Drop targets");
  core_textOutf(30,110,BLACK,"Q = Drain Ball, B = Jet Bumper");
  core_textOutf(30,120,BLACK,"T = Turnaround, L = Lock");
  core_textOutf(30,130,BLACK,"E = Eject hole");
}

/*-----------------
/  ROM definitions
/------------------*/
S7_ROMSTART8088(bk,l4,"ic14.716",   CRC(fcbe3d44) SHA1(92ec4d41beea205ba29530624b68dd1139053535),
                      "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                      "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                      "ic26.716",   CRC(104b78da) SHA1(c3af2563b3b380fe0e154b737799f6beacf8998c))
S67S_SOUNDROMS8(      "sound12.716",CRC(6d454c0e) SHA1(21640b9ed3bdbae8bf27629891f355304e467c64))
S67S_SPEECHROMS0000(  "speech7.532",CRC(c7e229bf) SHA1(3b2ab41031f507963af828639f1690dc350737af),
                      "speech5.532",CRC(411bc92f) SHA1(6c8d26fd13ed5eeba5cc40886d39c65a64beb377),
                      "speech6.532",CRC(fc985005) SHA1(9df4ad12cf98a5a92b8f933e6b6788a292c8776b),
                      "speech4.532",CRC(f36f12e5) SHA1(24fb192ad029cd35c08f4899b76d527776a4895b))
S7_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF(bk,l4,"Black Knight (L-4)",1980,"Williams",s7_mS7S,0)
/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData bkSimData = {
  2,                /* 2 game specific input ports */
  bk_stateDef,   /* Definition of all states */
  bk_inportData, /* Keyboard Entries */
  { stOuthole, stDrain, stDrain },  /*Position where balls start.. Max 7 Balls Allowed*/
  NULL,         /* Simulator Init */
  NULL,         /*Function to handle ball state changes*/
  bk_drawStatic,/* Function to handle mechanical state changes*/
  TRUE,         /* Simulate manual shooter? */
  NULL          /* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/
static core_tGameData bkGameData = {
  GEN_S7, s7_dispS7,
  {
    0,
    0,0,0,0,0,0,0,
    NULL, bk_handleMech, NULL, NULL,
    NULL, NULL
  },
  &bkSimData,
  {{ 0 }},
  {0,{21,22,36,0,0,0}}
};


/*---------------
/  Game handling
/----------------*/
static void init_bk(void) { core_gameData = &bkGameData; }
