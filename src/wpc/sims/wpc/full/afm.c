#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"
#include "wmssnd.h"
#include "machine/4094.h"

/* 150904 Added Saucer LEDs (GV) */
/* 231100 Added Sound Support (SJE) */
/* 161000 Cleaned up BITIMP use */
/* 181000 Corrected year to 1995 according to IPD */
/* 191000 Added Drop target status and corrected condition */
/* 231000 Now uses textOutf */
/*--------------
/ Keys
/   +R   L/R Ramp
/   +I   L/R Inlane
/   +-   L/R Slingshot
/   +L   L/R Loop
/   +O   L/R Outlane
/   +X   L/R Bonus X
/   +S   L/R Sauser
/   C    Center Ramp
/JK[]',: MARTIAN (7)
/  VBN   Jet (3)
/  FGH	 ForceField (3)
/   D    Drop Target
/   Z    Hole
/   L    Stroke of Luck
/---------------------------*/

#define AFM_BANKTIME  300
#define AFM_BANKSLACK  50

/*-- custom key conditions --*/
#define AFM_DROPCOND    1  /* Drop traget reachable */
#define AFM_CTCOND      2  /* Center trough reachable */

/*------------------
/  Local functions
/-------------------*/
static void init_afm(void);
static int  afm_getSol(int solNo);
static void afm_handleMech(int mech);
static int afm_getMech(int mechNo);
static int  afm_keyCond(int sw, int ballState, int *inports);
static void afm_drawMech(BMTYPE **line);
static void afm_drawStatic(BMTYPE **line);

/*-----------------------
/ local static variables
/------------------------*/
static struct afm_tLocals {
  int bankPos;     /* bank motor position */
} afm_locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
WPC_INPUT_PORTS_START(afm, 4)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001, "M",               KEYCODE_J)
    COREPORT_BIT(   0x0002, "A",               KEYCODE_K)
    COREPORT_BIT(   0x0004, "R",               KEYCODE_OPENBRACE)
    COREPORT_BIT(   0x0008, "T",               KEYCODE_CLOSEBRACE)
    COREPORT_BIT(   0x0010, "I",               KEYCODE_QUOTE)
    COREPORT_BIT(   0x0020, "A",               KEYCODE_COMMA)
    COREPORT_BIT(   0x0040, "N",               KEYCODE_COLON)
    COREPORT_BIT(   0x0080, "Jet 1",           KEYCODE_V)
    COREPORT_BIT(   0x0100, "Jet 2",           KEYCODE_B)
    COREPORT_BIT(   0x0200, "Jet 3",           KEYCODE_N)
    COREPORT_BIT(   0x0400, "Force 1",         KEYCODE_F)
    COREPORT_BIT(   0x0800, "Force 2",         KEYCODE_G)
    COREPORT_BIT(   0x1000, "Force 3",         KEYCODE_H)
    COREPORT_BITIMP(0x2000, "Drop Target",     KEYCODE_D)
    COREPORT_BITIMP(0x4000, "Hole",            KEYCODE_Z)
    COREPORT_BITIMP(0x8000, "Stroke of Luck",  KEYCODE_U)
  PORT_START /* 1 */
    COREPORT_BIT(   0x0001, "Left Qualifier",  KEYCODE_LCONTROL)
    COREPORT_BIT(   0x0002, "Right Qualifier", KEYCODE_RCONTROL)
    COREPORT_BITIMP(0x0004, "L/R Ramp",        KEYCODE_R)
    COREPORT_BITIMP(0x0008, "L/R Outlane",     KEYCODE_O)
    COREPORT_BITIMP(0x0010, "L/R Loop",        KEYCODE_L)
    COREPORT_BITIMP(0x0020, "L/R Bonus X",     KEYCODE_X)
    COREPORT_BIT(   0x0040, "L/R Slingshot",   KEYCODE_MINUS)
    COREPORT_BIT(   0x0080, "L/R Inlane",      KEYCODE_I)
    COREPORT_BIT(   0x0100, "L/R Saucer",      KEYCODE_S)
    COREPORT_BITIMP(0x0200, "Center Ramp",     KEYCODE_C)
    COREPORT_BITIMP(0x0400, "Drain",           KEYCODE_Q)
WPC_INPUT_PORTS_END

/*-------------------
/ Switch definitions
/--------------------*/
#define swLaunch    11
#define swStart     13
#define swTilt      14
#define swLOut      16
#define swRIn       17
#define swSLane     18

#define swSlamTilt  21
#define swCoinDoor  22
#define swLIn       26
#define swROut      27

#define swTEject    31
#define swTrough1   32
#define swTrough2   33
#define swTrough3   34
#define swTrough4   35
#define swLPop      36
#define swRPop      37
#define swLTopRoll  38

#define swMARTIaN   41
#define swMARTIAn   42
#define swMARtIAN   43
#define swMARTiAN   44
#define swLMotor    45
#define swCMotor    46
#define swRMotor    47
#define swRTopRoll  48

#define swLSling    51
#define swRSling    52
#define swJet1      53
#define swJet2      54
#define swJet3      55
#define swmARTIAN   56
#define swMaRTIAN   57
#define swMArTIAN   58
#define swLRampEnt  61
#define swCRampEnt  62
#define swRRampEnt  63
#define swLRampTop  64
#define swRRampEx   65
#define swMBankDn   66
#define swMBankUp   67

#define swRLoopHi   71
#define swRLoopLo   72
#define swLLoopHi   73
#define swLLoopLo   74
#define swLSaucer   75
#define swRSaucer   76
#define swDrop      77
#define swCTrough   78

/*----------------------
/ Solenoid definitions
/----------------------*/
#define sAutoFire   1
#define sBallRel    2
#define sLPop       3
#define sRPop       4
#define sLAlienLo   5
#define sLAlienHi   6
#define sKnocker    7
#define sRAlienHi   8
#define sLSling     9
#define sRSling    10
#define sJet1      11
#define sJet2      12
#define sJet3      13
#define sRAlienLo  14
#define sSaucer    15
#define sDrop      16
#define sBankMotor 24
#define sRGate     CORE_CUSTSOLNO(1) /* 33 */
#define sLGate     CORE_CUSTSOLNO(2) /* 34 */
#define sDiverter  CORE_CUSTSOLNO(3) /* 35 (power) & 36 (hold) */

/*---------------------
/  Ball state handling
/----------------------*/
enum { stBonusX=SIM_FIRSTSTATE, stDrain, stDrop,
       stLOut, stRIn, stSLane, stLIn, stROut,
       stOuthole, stTEject, stTrough1, stTrough2, stTrough3, stTrough4,
       stLPop, stRPop, stLTopRoll, stRTopRoll, stLRampEnt,
       stCRampEnt, stRRampEnt, stLRampTop, stRRampEx,
       stRLoopHiUp, stRLoopLoUp, stLLoopHiUp, stLLoopLoUp,
       stRLoopHiDn, stRLoopLoDn, stLLoopHiDn, stLLoopLoDn, stCTrough
};

static sim_tState afm_stateDef[] = {
  {"Not Installed",      0,0,         0,        stDrain,     0,0,0,SIM_STNOTEXCL},
  {"Moving",},
  {"Playfield",                 0,0,         0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Bonus X Area",       0,0,         0,        0,           0,0,0,SIM_STNOTEXCL},
  {"Drain",              1,0,         0,        stOuthole,   5,0,0,SIM_STNOTEXCL}, /* stDrain */
  {"Drop Target",        5,swDrop,    0,        stFree,      5,0,0,SIM_STSWKEEP},
  {"Left Outlane",       1,swLOut,    0,        stDrain,     5}, /* stLOut */
  {"Right Inlane",       1,swRIn,     0,        stFree,      5}, /* stRin */
  {"Shooter Lane",       1,swSLane,   sAutoFire,stRLoopHiUp,10}, /* stSLane */
  {"Left Inlane",        1,swLIn,     0,        stFree,      5}, /* stLin */
  {"Right Outlane",      1,swROut,    0,        stDrain,     5}, /* stRout */
  {"Outhole",            1,0,         0,        stTrough4,   5}, /* stOuthole */
/* I have no idea what this solenoid does. the simulator seems to work fine without it */
  {"Trough Eject",       1,swTEject,  0,        stFree,      5}, /* stTEject */
  {"Trough 1",           1,swTrough1, sBallRel, stSLane,     5}, /* stTrough1 */
  {"Trough 2",           1,swTrough2, 0,        stTrough1,   5}, /* stTrough2 */
  {"Trough 3",           1,swTrough3, 0,        stTrough2,   5}, /* stTrough3 */
  {"Trough 4",           1,swTrough4, 0,        stTrough3,   5}, /* stTrough4 */
  {"Left Popper",        1,swLPop,    sLPop,    stLIn,      10}, /* stLPop */ /* up to left ramp */
  {"Right Popper",       1,swRPop,    sRPop,    stFree,      5}, /* stRPop */ /* to playfield */
  {"Left Top Roll",      1,swLTopRoll,0,        stFree,      5}, /* stLTopRoll */
  {"Right Top Roll",     1,swRTopRoll,0,        stFree,      5}, /* stRTopRoll */
  {"Left Ramp Ent",      1,swLRampEnt,0,        stLRampTop,  5}, /* stLRampEnt */
  {"Center Ramp Ent",    1,swCRampEnt,0,        stRRampEx,   5,sDiverter,stLPop}, /* stCRampEnt */
  {"Right Ramp Ent",     1,swRRampEnt,0,        stRRampEx,   5}, /* stRRampEnt */
  {"Left Ramp Top",      1,swLRampTop,0,        stLIn,       5}, /* stLRampTop */
  {"Right Ramp Exit",    1,swRRampEx, 0,        stRIn,       5}, /* stRRampEx */
  {"Right Loop Hi",      3,swRLoopHi, 0,        stBonusX,    5,sRGate,stLLoopHiDn}, /* stRLoopHiUp */
  {"Right Loop Lo",      3,swRLoopLo, 0,        stRLoopHiUp, 3}, /* stRLoopLoUp */
  {"Left Loop Hi",       3,swLLoopHi, 0,        stBonusX,    3,sLGate,stRLoopHiDn}, /* stLLoopHiUp */
  {"Left Loop Lo",       3,swLLoopLo, 0,        stLLoopHiUp, 3}, /* stLLoopLoUp */
  {"Right Loop Hi",      3,swRLoopHi, 0,        stRLoopLoDn, 3}, /* stRLoopHiDn */
  {"Right Loop Lo",      3,swRLoopLo, 0,        stFree,      3}, /* stRLoopLoDn */
  {"Left Loop Hi",       3,swLLoopHi, 0,        stLLoopLoDn, 3}, /* stLLoopHiDn */
  {"Left Loop Lo",       3,swLLoopLo, 0,        stFree,      3}, /* stLLoopLoDn */
  {"Center Trough",      3,swCTrough, 0,        stLPop,      3},
  {0}
};

/*-- return status of custom solenoids --*/
static int afm_getSol(int solNo) {
  if (solNo == sLGate)    return (wpc_data[WPC_FLIPPERCOIL95] & 0x10) > 0;
  if (solNo == sRGate)    return (wpc_data[WPC_FLIPPERCOIL95] & 0x20) > 0;
  if (solNo == sDiverter) return (wpc_data[WPC_FLIPPERCOIL95] & 0xc0) > 0;
  return 0;
}

static void afm_handleMech(int mech) {
  /*---------------
  /  handle bank
  /----------------*/
  if (mech & 0x01) {
    if (core_getSol(sBankMotor)) afm_locals.bankPos = (afm_locals.bankPos + 1) % AFM_BANKTIME;
    core_setSw(swMBankDn, afm_locals.bankPos < AFM_BANKSLACK);
    core_setSw(swMBankUp, (afm_locals.bankPos >= AFM_BANKTIME/2) &&
                         (afm_locals.bankPos <  AFM_BANKTIME/2 + AFM_BANKSLACK));
  }
  /*-------------
  /  Drop target
  /--------------*/
  if ((mech & 0x02) && core_getSol(sDrop)) {
    core_setSw(swDrop, FALSE);
  }
}

static int afm_getMech(int mechNo) {
  return afm_locals.bankPos;
}
/* The drop target and hole behind can only be reached if the bank is down */
static int afm_keyCond(int cond, int ballState, int *inports) {
  switch (cond) {
    case AFM_DROPCOND:
      /* ball on PF & bank down & drop up */
      return (ballState == stFree) && core_getSw(swMBankDn) && !core_getSw(swDrop);
    case AFM_CTCOND:
      /* ball on PF & bank down & drop down */
      return (ballState == stFree) && core_getSw(swMBankDn) && core_getSw(swDrop);
  }
  return FALSE;
}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData afm_inportData[] = {
  {0, 0x0001, swmARTIAN},
  {0, 0x0002, swMaRTIAN},
  {0, 0x0004, swMArTIAN},
  {0, 0x0008, swMARtIAN},
  {0, 0x0010, swMARTiAN},
  {0, 0x0020, swMARTIaN},
  {0, 0x0040, swMARTIAn},
  {0, 0x0080, swJet1},
  {0, 0x0100, swJet2},
  {0, 0x0200, swJet3},
  {0, 0x0400, swLMotor, swMBankUp},
  {0, 0x0800, swCMotor, swMBankUp},
  {0, 0x1000, swRMotor, swMBankUp},
  {0, 0x2000, stDrop,   SIM_CUSTCOND(AFM_DROPCOND)},
  {0, 0x4000, stCTrough,SIM_CUSTCOND(AFM_CTCOND)},
  {0, 0x8000, stRPop},
  {1, 0x0005, stLRampEnt},
  {1, 0x0006, stRRampEnt},
  {1, 0x0009, stLOut},
  {1, 0x000a, stROut},
  {1, 0x0011, stLLoopLoUp},
  {1, 0x0012, stRLoopLoUp},
  {1, 0x0021, stLTopRoll, stBonusX},
  {1, 0x0022, stRTopRoll, stBonusX},
  {1, 0x0041, swLSling},
  {1, 0x0042, swRSling},
  {1, 0x0081, stLIn},
  {1, 0x0082, stRIn},
  {1, 0x0101, swLSaucer, swMBankDn},
  {1, 0x0102, swRSaucer, swMBankDn},
  {1, 0x0200, stCRampEnt},
  {1, 0x0400, stDrain, SIM_STANY},
  {0}
};

/*--------------------
/ Drawing information
/---------------------*/
static void afm_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"Bank: %-4s", core_getSw(swMBankUp) ? "Up" : (core_getSw(swMBankDn) ? "Down" : ""));
  core_textOutf(50,10,BLACK,"Drop: %-4s", core_getSw(swDrop) ? "Down" : "Up");
}

/* Help */
static void afm_drawStatic(BMTYPE **line) {
  core_textOutf(30, 50,BLACK,"Help on this Simulator:");
  core_textOutf(30, 60,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+L = L/R Loop");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+R = L/R Ramp");
  core_textOutf(30,100,BLACK,"L/R Ctrl+X = L/R Bonus X");
  core_textOutf(30,110,BLACK,"L/R Ctrl+S = L/R Saucer");
  core_textOutf(30,120,BLACK,"Q = Drain Ball, C = Center Ramp");
  core_textOutf(30,130,BLACK,"J/K/[/]/'/,/: = M/A/R/T/I/A/N Targets");
  core_textOutf(30,140,BLACK,"V/B/N = Left/Right/Bottom Jet Bumpers");
  core_textOutf(30,150,BLACK,"F/G/H = Forcefield Targets, Z = Hole");
  core_textOutf(30,160,BLACK,"D = Drop Target, L = Stroke of Luck");
}

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART(afm,11, "mars1_1.rom",  0x080000,CRC(13b174d9) SHA1(57952f3184496b0316e4cf301e0181cb9de3519a))
DCS_SOUNDROM3m(	"afm_s2.l1",CRC(6e39d96e) SHA1(b34e31bb1734c86614f153f7201163aaa9943cec),
		"afm_s3.l1",CRC(1cbce9b1) SHA1(7f258bfe1904a879a2cb007419483f4fee91e072),
		"afm_s4.l1",CRC(5ff7fbb7) SHA1(ebaf825d3b90b6acee1920e6703801a4bcddfc5b))
WPC_ROMEND

WPC_ROMSTART(afm,10, "afm_1_00.g11",  0x080000,CRC(1a30fe95) SHA1(218674e63ce4efeecb266f35f0f315758f7c72fc))
DCS_SOUNDROM3m(	"afm_1_00.s2",CRC(610ff107) SHA1(9590f809a05cb2bda4979fa16f165e2e994719a0),
		"afm_s3.l1",CRC(1cbce9b1) SHA1(7f258bfe1904a879a2cb007419483f4fee91e072),
		"afm_s4.l1",CRC(5ff7fbb7) SHA1(ebaf825d3b90b6acee1920e6703801a4bcddfc5b))
WPC_ROMEND

WPC_ROMSTART(afm,113,"afm_1_13.bin", 0x100000,CRC(e1fbd81b) SHA1(0ff35253d8eac7b75abb3e4db84cdcca458182cd))
DCS_SOUNDROM3m(	"afm_s2.l1",CRC(6e39d96e) SHA1(b34e31bb1734c86614f153f7201163aaa9943cec),
		"afm_s3.l1",CRC(1cbce9b1) SHA1(7f258bfe1904a879a2cb007419483f4fee91e072),
		"afm_s4.l1",CRC(5ff7fbb7) SHA1(ebaf825d3b90b6acee1920e6703801a4bcddfc5b))
WPC_ROMEND

WPC_ROMSTART(afm,113b,"afm_113b.bin", 0x100000,CRC(34fd2d7d) SHA1(57a41bd686286429880e63696d7d9d3990ca5d05))
DCS_SOUNDROM3m(	"afm_s2.l1",CRC(6e39d96e) SHA1(b34e31bb1734c86614f153f7201163aaa9943cec),
		"afm_s3.l1",CRC(1cbce9b1) SHA1(7f258bfe1904a879a2cb007419483f4fee91e072),
		"afm_s4.l1",CRC(5ff7fbb7) SHA1(ebaf825d3b90b6acee1920e6703801a4bcddfc5b))
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
CORE_GAMEDEF( afm,113,   "Attack From Mars (1.13)",1995, "Bally",wpc_m95S,0)
CORE_CLONEDEF(afm,113b,113,"Attack From Mars (1.13b)",1995, "Bally",wpc_m95S,0)
CORE_CLONEDEF(afm,11,113,"Attack From Mars (1.1)", 1995, "Bally",wpc_m95S,0)
CORE_CLONEDEF(afm,10,113,"Attack From Mars (1.0)", 1995, "Bally",wpc_m95S,0)

/*----------
/ Game Data
/-----------*/
static sim_tSimData afmSimData = {
  2,                   /* 2 input ports */
  afm_stateDef,        /* stateData*/
  afm_inportData,      /* inportData*/
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },
  NULL,                /* initSim */
  NULL,                /* handleBallState*/
  afm_drawStatic,      /* help */
  FALSE,               /* automatic plunger */
  afm_keyCond          /* advanced key conditions */
};

static core_tGameData afmGameData = {
  GEN_WPC95, wpc_dispDMD,
  {
    FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L),
    0,2,3,0,0,1,0, // 2 extra lamp columns for the LEDs
    afm_getSol, afm_handleMech, afm_getMech, afm_drawMech,
    NULL, NULL
  },
  &afmSimData,
  {
    "541 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* inverted switches */
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, swLaunch},
  }
};

static WRITE_HANDLER(parallel_0_out) {
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8] = data;
}
static WRITE_HANDLER(parallel_1_out) {
  coreGlobals.lampMatrix[9] = coreGlobals.tmpLampMatrix[9] = data;
}
static WRITE_HANDLER(qspin_0_out) {
  HC4094_data_w(1, data);
}

static HC4094interface hc4094afm = {
  2, // 2 chips
  { parallel_0_out, parallel_1_out },
  { qspin_0_out }
};

static WRITE_HANDLER(afm_wpc_w) {
  wpc_w(offset, data);
  if (offset == WPC_SOLENOID1) {
    HC4094_data_w (0, GET_BIT5);
    HC4094_clock_w(0, GET_BIT4);
    HC4094_clock_w(1, GET_BIT4);
  }
}

/*---------------
/  Game handling
/----------------*/
static void init_afm(void) {
  core_gameData = &afmGameData;
  install_mem_write_handler(0, 0x3fb0, 0x3fff, afm_wpc_w);
  HC4094_init(&hc4094afm);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_strobe_w(0, 1);
  HC4094_strobe_w(1, 1);
}

