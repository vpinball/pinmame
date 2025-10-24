#include <math.h>
#include "driver.h"
#include "core.h"
#include "mech.h"

#ifndef M_PI
#  define M_PI 3.1415926535897932384626433832795
#endif /* M_PI */
#define MECH_STEP       60
#define MECH_FASTPULSES  8

static struct {
  mech_tMechData mechData[MECH_MAXMECH];
  void *mechTimer;
  int mechCounter;
  int emuRunning;
} locals;
static void mech_updateAll(int param);
static void mech_update(int mechNo);

#ifdef LIBPINMAME
  extern void libpinmame_update_mech(const int mechNo, mech_tMechData* mechData);
#endif

void mech_init(void) {
  memset(&locals,0,sizeof(locals));
}
void mech_emuInit(void) {
  if (locals.mechData[0].sol1) {
    locals.mechTimer = timer_alloc(mech_updateAll);
    timer_adjust(locals.mechTimer,0,0, TIME_IN_HZ(60*MECH_FASTPULSES));
  }
  locals.emuRunning = TRUE;
}
void mech_emuExit(void) {
  if (locals.mechTimer) { timer_remove(locals.mechTimer); locals.mechTimer = NULL; }
  locals.emuRunning = FALSE;
}
int mech_getPos(int mechNo) {
  return locals.mechData[mechNo].pos;
}
int mech_getSpeed(int mechNo) {
  return (locals.mechData[mechNo].ret != 0) ? locals.mechData[mechNo].speed / locals.mechData[mechNo].ret : 0;
}
float mech_getFloatPos(int mechNo) {
  return locals.mechData[mechNo].floatPos;
}
float mech_getFloatSpeed(int mechNo) {
  return (float)mech_getSpeed(mechNo);
}

void mech_addLong(int mechNo, int sol1, int sol2, int type, int length, int steps, mech_tSwData sw[], int initialpos) {
  if ((locals.mechTimer == NULL) && locals.emuRunning) {
    locals.mechTimer = timer_alloc(mech_updateAll);
    timer_adjust(locals.mechTimer,0,0, TIME_IN_HZ(60*MECH_FASTPULSES));
  }
  if ((mechNo >= 0) && (mechNo < MECH_MAXMECH)) {
    ptMechData md = &locals.mechData[mechNo];
    int ii = 0;
    md->solinv = 0;
    if (sol1 < 0) { md->solinv |= 1; sol1 = -sol1; }
    if (sol2 < 0) { md->solinv |= 2; sol2 = -sol2; }
    md->sol1 = sol1; md->sol2 = sol2;
    md->length = length; md->steps = steps;
    md->type = type & 0x1ff;
    md->ret = ((type & 0xff000000) ? ((type>>24) & 0x00ff) : 1);
    md->acc = ((type & 0x00fffe00) ? ((type>>9)  & 0x7fff) : 1);
    md->fast = (type & MECH_FAST) > 0;
    do {
      md->swPos[ii] = sw[ii];
      /* backward compatible */
      if (sw[ii].startPos < 0) {
        md->swPos[ii].pulse = -sw[ii].startPos;
        md->swPos[ii].startPos = 0;
        md->swPos[ii].endPos -= 1;
      }
    } while (sw[ii++].swNo);
    md->pos = -1; /* not initialized */
    md->floatPos = -1.f;
	if (initialpos > 0)
		md->anglePos = initialpos-1;
  }
}
void mech_add(int mechNo, mech_ptInitData id) {
  mech_addLong(mechNo, id->sol1, id->sol2, id->type, id->length, id->steps, &id->sw[0], id->initialpos);
}

static void mech_updateAll(int param) {
  int mech = -1, ii;
  locals.mechCounter = (locals.mechCounter + 1) % MECH_FASTPULSES;
  //printf("updateAll %d\n",locals.mechCounter);
#if defined(VPINMAME) || defined(LIBPINMAME)
  { extern int g_fHandleMechanics; mech = g_fHandleMechanics; }
  //printf("updateAll %d\n",locals.mechCounter);
  if (mech == 0) {
    for (ii = MECH_MAXMECH/2; ii < MECH_MAXMECH; ii++) {
      if (locals.mechData[ii].sol1 && ((locals.mechCounter == 0) || locals.mechData[ii].fast))
        mech_update(ii);
  }}
  else
#endif /* VPINMAME */
    for (ii = 0; ii < MECH_MAXMECH/2; ii++) {
      //printf("ii=%d,mech=%x,sol1=%d\n",ii,mech,locals.mechData[ii].sol1);
      if ((mech & 0x01) && locals.mechData[ii].sol1 &&
          ((locals.mechCounter == 0) || locals.mechData[ii].fast))
        { mech_update(ii); /* printf("updateMech %d\n",ii);*/}
      mech >>= 1;
    }
}

static void mech_update(int mechNo) {
  ptMechData md = &locals.mechData[mechNo];
  int speed = md->speed;
  int dir = 0;
  int currPos, ii;

  { /*-- check power direction -1, 0, 1 --*/
     if ((md->type & 0x70) == MECH_FOURSTEPSOL)
     {
        int sol = 0;
        for (ii = 0; ii < 4; ii++)
        {
           sol = (sol << 1) | ((core_getPulsedSol(md->sol1 + ii) != 0) ^ md->solinv);
        }

        if (sol)
        {
           int lsol = (sol << 1 & 0x0F) | (sol & 0x08) >> 3;
           int rsol = (sol >> 1) | (sol & 0x1) << 3;

           if (md->last == rsol)
              dir = 1;
           else if (md->last == lsol)
              dir = -1;
           md->last = sol;
        }
     } 
     else
     {
        int sol = ((core_getPulsedSol(md->sol1) != 0) +
           (md->sol2 ? 2 * (core_getPulsedSol(md->sol2) != 0) : 0)) ^ md->solinv;
        if (md->type & MECH_TWOSTEPSOL)
           dir = (sol != md->last);
        else if (md->type & MECH_TWODIRSOL)
           dir = (sol == 1) - (sol == 2);
        else /* MECH_ONEDIRSOL | MECH_ONESOL */
           dir = (sol == 1) - (sol == 3);
        md->last = sol;
     }
  }
  
  { /*-- update speed --*/
    if (dir == 0) {
      if (speed > 0)      { speed -= 1; }
      else if (speed < 0) { speed += 1; }
    }
    else if (dir < 0)
      { speed -= (speed > 0) + md->ret; if (speed < -md->acc*md->ret) speed = -md->acc*md->ret; }
    else /* if (dir > 0) */
      { speed += (speed < 0) + md->ret; if (speed >  md->acc*md->ret) speed =  md->acc*md->ret; }
    md->speed = speed;
  }

  /*-- update position --*/
  if (speed || (md->pos < 0)) {
    int anglePos = md->anglePos + speed * MECH_STEP / md->acc / md->ret;
    if (md->type & MECH_STOPEND) {
      if (anglePos >= md->length*MECH_STEP) anglePos = (md->length - 1)*MECH_STEP;
      else if (anglePos < 0)                anglePos = 0;
    }
    else if (md->type & MECH_REVERSE) {
      if (anglePos >= md->length*2*MECH_STEP) anglePos -= md->length*2*MECH_STEP;
      else if (anglePos < 0)                  anglePos += md->length*2*MECH_STEP;
    }
    else { /* MECH_CIRCLE */
      if (anglePos >= md->length*MECH_STEP) anglePos -= md->length*MECH_STEP;
      else if (anglePos < 0)                anglePos += md->length*MECH_STEP;
    }
    if (md->type & MECH_NONLINEAR)
      currPos = (int)(md->length * MECH_STEP * (1.-cos(anglePos*M_PI/md->length/MECH_STEP)) / 2.);
    else /* MECH_LINEAR */
      currPos = (anglePos >= md->length*MECH_STEP) ? md->length*2*MECH_STEP-anglePos : anglePos;
    md->anglePos = anglePos;
    md->pos = currPos * md->steps / md->length / MECH_STEP;
    md->floatPos = currPos * md->steps / (float) md->length / (float) MECH_STEP;
    /*-- update switches --*/
    currPos = (md->type & MECH_LENGTHSW) ? currPos / MECH_STEP : md->pos;

    for (ii = 0; ii < MECH_MAXMECHSW && md->swPos[ii].swNo > 0; ii++)
      core_setSw(md->swPos[ii].swNo, FALSE);
    for (ii = 0; ii < MECH_MAXMECHSW && md->swPos[ii].swNo > 0; ii++) {
      if (md->swPos[ii].pulse) currPos %= md->swPos[ii].pulse;
      if ((currPos >= md->swPos[ii].startPos) &&
          (currPos <= md->swPos[ii].endPos))
        core_setSw(md->swPos[ii].swNo, TRUE);
    }
  }

#ifdef LIBPINMAME
    libpinmame_update_mech(mechNo, md);
#endif
}

void mech_nv(void *file, int write) {
  int ii;
  for (ii = 0; ii < MECH_MAXMECH; ii++) {
	if (write)
	{
		mame_fwrite(file, &locals.mechData[ii].anglePos, sizeof(int)); /* Save */
		if (file && ((mame_file*)file)->type == RAM_FILE) // if writing out nvram to ram file then do nothing to pos
			continue;
	}
	else
	{
		if (file)
			mame_fread(file, &locals.mechData[ii].anglePos, sizeof(int)); /* Load */
		else
			locals.mechData[ii].anglePos = 0; /* First time */
	}

    locals.mechData[ii].pos = -1;
  }
}
