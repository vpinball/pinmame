#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "sim.h"

/* 121200  Added SIM_STSWON, SIM_STSWOFF. */
/*         changes state if switch is activated or deactivated */
/* 111100  Corrected error in spinner handling */
/* 161000  Corrected a timer error. Was sometimes counting VBLANKS instead */
/*         of 1/10s */
/* 231000  Now uses textOutf function */
/* 070401  Added firstrow argument to sim_draw */
#define SIM_SHOOTRELTIME   3 /* how long the fake solenoid is active VBLANKS */
#define SIM_TIMEFACTOR     6 /* multiplier for VBLANK to simulator timing */
#define SIM_MAXSPIN        2 /* spinners in parallell */

static void sim_startSpin(int swNo, int time);
static void sim_updateSpin(void);

/*---------------------
/  Global variables
/---------------------*/
static sim_tSimData *simData = NULL;  /* data about the game simulator */
static struct {
  sim_tBallStatus balls[SIM_MAXBALLS]; /* ball status etc */
  int  currBall;      /* currently select ball */
  int  shooterRel;    /* plunger released */
  int  shooterSpeed;  /* */
  struct {
    int swNo;         /* spinner switch */
    int spinCount;    /* time until next spin */
    int nextSpin;     /* timer for next spin */
  } spinner[SIM_MAXSPIN];
} locals;

/*-----------------------------------------
/  Run the simulator (called every VBLANK)
/-----------------------------------------*/
void sim_run(int *inports, int firstGameInport, int useSimKeys, int noOfBalls) {
  int ii;

  for (ii = 0; ii < simData->inports; ii++)
    inports[firstGameInport+ii] = readinputport(firstGameInport+ii);

  /*----------------------
  /  Change active ball ?
  /-----------------------*/
  locals.balls[locals.currBall].current = FALSE;
    if (inports[CORE_SIMINPORT] & SIM_NEXTKEY) locals.currBall += 1;
    if (inports[CORE_SIMINPORT] & SIM_PREVKEY) locals.currBall -= 1;
    if (locals.currBall < 0)                     locals.currBall = noOfBalls -1;
    if (locals.currBall >= noOfBalls)            locals.currBall = 0;
  locals.balls[locals.currBall].current = TRUE;

  /* update spinners running */
  sim_updateSpin();

  /*-------------------
  /  Check user input
  /--------------------*/
  if (useSimKeys) {
    sim_tBallStatus *ball = &locals.balls[locals.currBall];
    sim_tInportData *iData = simData->inportData;
    int eventFound = FALSE;
    while (iData->mask) {
	if (!eventFound && (inports[iData->port + firstGameInport] & iData->mask) == iData->mask) {
   	   /*-- mask this event off --*/
	   eventFound = TRUE;
        /*-- check all conditions --*/
        if ((iData->condition == SIM_STANY) || (inports[CORE_SIMINPORT] & SIM_IGNOREKEY) ||
            ((iData->condition == 0) && (ball->state == stFree)) ||
            ((iData->condition > 0) && (iData->condition < SIM_STATES) &&
             (ball->state == stFree) && core_getSw(iData->condition)) ||
            ((iData->condition < 0) && simData->keyCond &&
             simData->keyCond(-iData->condition, ball->state, &inports[firstGameInport])) ||
            ((iData->condition >= SIM_STATES) && (iData->condition == ball->state))) {
          /*-- condition met --*/
          if (iData->action >= SIM_STATES) {
            /*-- change state --*/
            if ((ball->state == ball->nextState) || (ball->timer <= 0)) {
              ball->nextState = iData->action;
              ball->timer = ball->nextTimer = 1;
            }
          }
          else if (iData->action > 0) { /* activate switch */
            core_setSw(iData->action, TRUE);
          }
        }
      }
      else if ((iData->action > 0) && (iData->action < SIM_STATES)) {
        core_setSw(iData->action, FALSE); /* reset switch regardless of ball state */
      }
      iData += 1;
    }
  }
  /*----------------------------------------------
  /  update swithes depending playfield mechanics
  /-----------------------------------------------*/
  if (simData->manShooter) {
    /*--  ball shooter --*/
    if (inports[CORE_SIMINPORT] & SIM_SHOOTERKEY) {
      locals.shooterSpeed += 1;
      if (locals.shooterSpeed > 50)
        locals.shooterSpeed = 50;
    }
    else if (locals.shooterSpeed > 0) {
      /*-- Shooters been released --*/
      if (locals.shooterRel == 1) locals.shooterSpeed = 0;
      if (locals.shooterRel > 0)  locals.shooterRel -= 1;
      else                        locals.shooterRel = SIM_SHOOTRELTIME;
    }
  }

  /*------------------------
  /  update ball positions
  /-------------------------*/
  if (simData->stateData) {
    for (ii = 0; ii < SIM_MAXBALLS; ii++) {
      sim_tBallStatus *ball = &locals.balls[ii];
      if (ii >= noOfBalls) {
        /* this ball is not in the game */
        ball->nextState = stNotInst;
        ball->timer = 0;
      }
      if (ball->timer > 0)
        ball->timer -= 1;
      else if (ball->timer < 0) {
        ball->timer = ball->nextTimer;
        ball->state = stPending;
      }
      while (ball->timer == 0) {
        sim_tState *sd = &simData->stateData[SIM_STATENO(ball->nextState)];
        if (ball->state != ball->nextState) {
          /*-- try to move the ball --*/
          if ((sd->solSwNo) && !(sd->flags & (SIM_STSWON | SIM_STSWOFF)) &&
              core_getSol(sd->solSwNo)) {
            /*-- next state is blocked by a solenoid --*/
            break;
          }
          if (!(sd->flags & SIM_STNOTEXCL)) {
            int jj;
            /*-- next state is exclusive --*/
            /*-- check if any other ball is in that state --*/
            for (jj = 0; jj < SIM_MAXBALLS; jj++) {
              if ((jj != ii) &&
                  ((locals.balls[jj].state == ball->nextState) ||
                   ((ball->state != stPending) &&
                    (locals.balls[jj].nextState == ball->nextState) &&
                    (locals.balls[jj].state == stPending)))) {
                /*-- next state is occupied --*/
                goto nextStateBusy;
              }
            }
          }
          /*-- next state OK --*/
          if (ball->state == stPending) {
            /*-- go into the new state --*/
            ball->timer = sd->timer * SIM_TIMEFACTOR;
            if (sd->swNo) { /* if the new state includes a switch, activate it */
              if (sd->flags & SIM_STSPINNER)
                sim_startSpin(sd->swNo, SIM_SPINSEC(inports[CORE_SIMINPORT]));
              else
                core_setSw(sd->swNo, !((sd->flags & SIM_STNOTBALL(0)) && (sd->flags & ball->type)));
            }
            ball->state = ball->nextState;
          }
          else {
            /*-- leave the current state first --*/
            ball->timer = -1;
            /*-- if the old state includes a switch, deactivate it --*/
            sd = &simData->stateData[SIM_STATENO(ball->state)];
            if (!(sd->flags & (SIM_STSWKEEP | SIM_STSPINNER)) && sd->swNo)
              core_setSw(sd->swNo, FALSE);
            ball->state = stPending;
          }
        }
        else if ((sd->solSwNo == 0) || (sd->flags & SIM_STIGNORESOL) ||
                 ((sd->flags & SIM_STSWON)  && core_getSw(sd->solSwNo)) ||
                 ((sd->flags & SIM_STSWOFF) && !core_getSw(sd->solSwNo)) ||
                 (!(sd->flags & (SIM_STSWON|SIM_STSWOFF)) && core_getSol(sd->solSwNo))) {
          if (sd->flags & SIM_STSHOOT) ball->speed = locals.shooterSpeed;
          if (sd->nextState) {
            ball->nextState = ((sd->altSolSw) &&
                               (((sd->flags & SIM_STSWON) && core_getSw(sd->altSolSw)) ||
                                ((sd->flags & SIM_STSWOFF) && !core_getSw(sd->altSolSw)) ||
                                (!(sd->flags & (SIM_STSWON|SIM_STSWOFF)) && core_getSol(sd->altSolSw)))) ?
                              sd->altState : sd->nextState;
            ball->nextTimer = sd->nextTimer * SIM_TIMEFACTOR;
          }
          else if ((simData->handleBallState == NULL) ||
                   !simData->handleBallState(ball, &inports[firstGameInport])) {
            /*-- The ball stays in its current state --*/
            break;
          }
        }
        else
          break;
      }
nextStateBusy: ;
    }
  }
}

/*-----------------------
/  Draw simulator data
/  (ball status etc.)
/------------------------*/
void sim_draw(int firstRow) {
  struct rectangle rec;
  int ii;

  /*-- draw ball status --*/
  if (simData->stateData) {
    for (ii = 0; ii < SIM_MAXBALLS; ii++)
      if (locals.balls[ii].state != stPending)
        core_textOutf(160, 10*ii+10, locals.balls[ii].current ? WHITE: BLACK,
                     "%c %-20s", locals.balls[ii].type ? '*' : ' ',
                      simData->stateData[SIM_STATENO(locals.balls[ii].state)].name);
  }
  if (simData->manShooter) {
    /*-- draw shooter position --*/
    rec.max_y = (rec.min_y = firstRow) + 5;
    rec.max_x = (rec.min_x = 130) + locals.shooterSpeed/2;
    fillbitmap(Machine->scrbitmap, Machine->pens[WHITE], &rec);
    rec.min_x = 131+locals.shooterSpeed/2; rec.max_x = 155;
    fillbitmap(Machine->scrbitmap, Machine->pens[BLACK], &rec);
  }
  if (simData->drawStatic)
    simData->drawStatic((void *)(&Machine->scrbitmap->line[firstRow]));
}

/*--------------------
/ Simulated solenoids
/---------------------*/
int sim_getSol(int solNo) {
  if (solNo == sShooterRel)
    return locals.shooterRel > 0;
  return 0;
}

/*--------------------------
/  initialize the simluator
/---------------------------*/
int sim_init(sim_tSimData *gameSimData, int *inports, int firstGameInport) {
  int ii;

  simData = gameSimData;

  memset(locals.balls, 0, sizeof(locals.balls));

  for (ii = 0; ii < simData->inports; ii++)
    inports[firstGameInport+ii] = readinputport(firstGameInport+ii);

  for (ii = 0; ii < SIM_BALLS(inports[CORE_SIMINPORT]); ii++) {
    locals.balls[ii].nextState = simData->ballStart[ii];
    locals.balls[ii].state = stPending;
  }
  if (simData->initSim)
    simData->initSim(locals.balls, &inports[firstGameInport], SIM_BALLS(inports[CORE_SIMINPORT]));
  return (simData->inportData != NULL);
}

/*------------------
/  Spinner handling
/-------------------*/
static void sim_startSpin(int swNo, int time) {
  static int spinStart[] = {0,251,219,191,172,143,114,87,65};
  int idx = -1;
  int ii;

  for (ii = 0; ii < SIM_MAXSPIN; ii++)
    if (locals.spinner[ii].swNo == swNo)
      { idx = ii; break; } /* already running */
    else if (locals.spinner[ii].swNo == 0)
      idx = ii;
  if (idx < 0)
    { logerror("Only %d spinners supported\n",SIM_MAXSPIN); return; }

  locals.spinner[idx].swNo = swNo;
  locals.spinner[idx].spinCount = 0;
  locals.spinner[idx].nextSpin = spinStart[time];
  core_setSw(swNo, TRUE);
}

static void sim_updateSpin(void) {
  int ii;

  for (ii = 0; ii < SIM_MAXSPIN; ii++)
    if (locals.spinner[ii].swNo) {
      DBGLOG(("spin: count=%3d next=%3d\n", locals.spinner[ii].spinCount,
             locals.spinner[ii].nextSpin));
      if (locals.spinner[ii].spinCount >= locals.spinner[ii].nextSpin) {
        /* activate spinner switch and start new timer */
        locals.spinner[ii].spinCount = 0;
        locals.spinner[ii].nextSpin = locals.spinner[ii].nextSpin * 20 / 19;
        if (locals.spinner[ii].nextSpin <= 275 )
          core_setSw(locals.spinner[ii].swNo, TRUE);
        else /* spinner has stopped */
          locals.spinner[ii].swNo = 0;
      }
      else if (locals.spinner[ii].spinCount == 30) /* activated time */
        core_setSw(locals.spinner[ii].swNo, FALSE);
      locals.spinner[ii].spinCount += 10;
    }
}

