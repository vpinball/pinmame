#ifndef INC_SIM
#define INC_SIM

/* 121200  Added SIM_STSWON, SIM_STSWOFF */
/* 201100  Added new types for manual shooter */
/* 111100  Updated solenoid numbering */
/* 161000  Updated input port to new BITIMP macro */
/* 070401  Added firstrow argument to sim_draw */
/*----------------------
/ WPC driver constants
/-----------------------*/
#define sShooterRel       (CORE_FIRSTSIMSOL+0)  /* fake solenoid for ball shooter */

/*-- Simulator inputport --*/
#define SIM_PORTS(balls) \
    /* 0x000f occupied */ \
    COREPORT_BIT(     0x8000, "Shoot Ball",       KEYCODE_SPACE) \
    COREPORT_BIT(     0x0100, "Ignore Location",  KEYCODE_LALT) \
    COREPORT_BITTOG(  0x0200, "Switch/Simulator", KEYCODE_DEL) \
    COREPORT_BITIMP(  0x0400, "Next Ball",        KEYCODE_DOWN) \
    COREPORT_BITIMP(  0x0800, "Prev Ball",        KEYCODE_UP) \
    COREPORT_DIPNAME( 0x7000, (0x1000*(balls)), "Balls") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
      COREPORT_DIPSET(0x2000, "2" ) \
      COREPORT_DIPSET(0x3000, "3" ) \
      COREPORT_DIPSET(0x4000, "4" ) \
      COREPORT_DIPSET(0x5000, "5" ) \
      COREPORT_DIPSET(0x6000, "6" ) \
      COREPORT_DIPSET(0x7000, "7" ) \
    COREPORT_DIPNAME( 0x0030, 0x0010, "Spinner time") \
      COREPORT_DIPSET(0x0000, "2s") \
      COREPORT_DIPSET(0x0010, "4s") \
      COREPORT_DIPSET(0x0020, "6s") \
      COREPORT_DIPSET(0x0030, "8s")

#define SIM_SHOOTERKEY  0x8000
#define SIM_IGNOREKEY   0x0100
#define SIM_SWITCHKEY   0x0200
#define SIM_NEXTKEY     0x0400
#define SIM_PREVKEY     0x0800
#define SIM_BALLS(x)    (((x)>>12)&0x07)
#define SIM_SPINSEC(x)  (((x)&0x0030)>>4)
/*-----------------------
/ Simulator constants
/------------------------*/
#define SIM_MAXBALLS      7  /* balls */

/*-----------------------------------
/  Status data for each ball
/
/  not needed by the driver.
/  use macros below
/------------------------------------*/
typedef struct {
  int state;     /* current state of the ball */
  int timer;     /* time until next state change */
                 /* if state==ePending timer means "between switches" */
                 /* in any other state the timer is the time between switch on to switch off */
  int nextState; /* the next state for the ball  */
  int nextTimer;
  int type;      /* >0 -> this is a special ball that does not trigger some switches e.g. Powerball */
  int current;   /* this ball is the currently selected */
  int speed;     /* ball speed from shooter or other places */
  int custom;    /* spare field to be used for game specific data */
} sim_tBallStatus;

/*-- use these macros in the statehandler --*/

/*-- set next ball state --*/
#define setState(state,delay)   ((ball->nextState=state), (ball->nextTimer=delay),1)
/*-- set free state --*/
#define setFree(delay)          setState(stFree,delay)
/*-- force ball into new state immediatly --*/
/*-- warning! Switches will not be triggered or relased --*/
#define forceState(s)           (ball->nextState=ball->state=(s))

/*-- use this to init ball type --*/
#define initBallType(no, ballType) (balls[(no)].type=(ballType))

/*------------------
/ The ball states
/------------------- */
#define SIM_STATES     256
#define SIM_FIRSTSTATE (SIM_STATES+3)
#define SIM_STATENO(x) (x & 0xff)

/*-- common for all games --*/
#define stNotInst (0)                 /* ball is not installed in the game */
#define stPending (SIM_STATES+1)   /* ball is travelling towards a switch */
#define stFree    (SIM_STATES+2)   /* ball is on the playfield */
/*-- game specific states start on SIM_FIRSTSTATE --*/

/*--------------------
/  State definitions
/---------------------*/
typedef struct {
  const char *name;   /* Name to display on screen */
  int   timer;        /* Switch down time (minimum) */
  int   swNo;         /* Switch to be triggered while in this state */
  int   solSwNo;      /* Solenoid/switch used to get out of state (0=leave ASAP)*/
                      /* also: Balls can not enter state while solenoid is active */
  int   nextState;    /* State following this one */
  int   nextTimer;    /* Time between this state and next */
  int   altSolSw;     /* If this switch/solenoid is active go to altstate (below) */
  int   altState;     /* alternative state */
  int   flags;        /* see below */
} sim_tState;
/*-- State flags --*/
/* STSWKEEP:        Leave switch on (e.g. drop targets) */
/* STSHOOT:         Set ballspeed to plunger speed */
/* STIGNORESOL:     Always call statehandler regardless of solenoid state */
/* STNOTEXCL:       More than one ball can be in this sate */
/* STNOTBALL(type): Specified ball type does not affect switch */
/* STSPINNER:       Switch is a spinner */
/* STSWON:          solSwNo and altSolSw fields refers to a active switch */
/* STSWOFF:         solSwNo and altSolSw fields refers to a inactive switch */
/*-------------------------------------------------------------*/
#define SIM_STSHOOT         0x200
#define SIM_STSPINNER       0x100
#define SIM_STSWKEEP        0x080
#define SIM_STIGNORESOL     0x040
#define SIM_STNOTEXCL       0x020
#define SIM_STSWON          0x400
#define SIM_STSWOFF         0x800
#define SIM_STNOBALLMASK    0x00f
#define SIM_STNOTBALL(type) (0x010|(type))

/*------------------------------------------------
/  Convert keypresses to switch or state changes
/-------------------------------------------------*/
typedef struct  {
  int port;      /* Inport number */
  int mask;      /* Mask for this switch (all bits must be set for switch to activate */
		 /* Only first match will be used. Placed qualified keys first */
  int action;    /* Switch to activate or new ball state */
  int condition; /* Condition for the action to occur */
                 /* 0                  : ball in stFree state */
                 /* switch             : ball in stFree state and switch active */
                 /* SIM_STANY       : ball can be in any state */
                 /* SIM_CUSTCOND(n) : call custom condition handler (keyCond) */
} sim_tInportData;

#define SIM_STANY SIM_STATES
#define SIM_CUSTCOND(n) (-(n))

/*-- Example
sim_tInportData example[] = {
{3, 0x0001, swTarget, stFree},             switch swTarget will be activated
                                           if ball is in stFree
{1, 0x1000, stLRamp,  swRampOpen},         ball will change state to stLRamp
                                           if ball is in stFree and swrampOpen active
{3, 0x1000, stLRamp,  SIM_STANY},       ball will change state to stLRamp
{2, 0x4000, stRRamp,  SIM_CUSTCOND(2)}, custom condition handler called with cond=2
{0}  must end with a zero
--*/
typedef struct {
  /*-- simulator data --*/
  int  inports;               /* no of inports */
  sim_tState *stateData;      /* state descriptions */
  sim_tInportData *inportData;   /* keypress to switch/state conversion table */
  int ballStart[SIM_MAXBALLS];   /* start positions of balls */
  void (*initSim)(sim_tBallStatus balls[], int *inports, int noOfBalls);
  int  (*handleBallState)(sim_tBallStatus *ball, int *inports);  /* ball state handler */
  void (*drawStatic)(BMTYPE **line);     /* draw game specific static data */
  int manShooter; /* true if a manual shooter should be simulated */
  int (*keyCond)(int cond, int ballState, int *inports); /* advanced key conditions */
} sim_tSimData;

/*-----------------------------
/ exported simulator functions
/------------------------------*/
void sim_draw(int firstRow);
void sim_run(int *inports, int firstGameInport, int useSimKeys, int noOfBalls);
int sim_getSol(int solNo);
int sim_init(sim_tSimData *gameSimData, int *inports, int firstGameInport);
#endif /* INC_SIM */
