#include <math.h>
#include "driver.h"
#include "core.h"
#include "mech.h"

#ifndef M_PI
#  define M_PI PI
#endif /* M_PI */
#define MECH_STEP 60

typedef struct {
  int sol1, sol2; /* Controlling solenoids */
  int length;     /* Length to move from one end to the other in VBLANKS 1/60s */
  int steps;      /* steps returned */
  int type;       /* type */
  int acc;        /* acceleration */
  int ret;
  mech_tSwData swPos[10]; /* switches activated */
  int pos;      /* current position */
  int speed;    /* current speed -acc -> acc */
  int anglePos;
  int last;
} tMechData, *ptMechData;
static tMechData mechData[MECH_MAXMECH];

void mech_init(void) { memset(mechData,0,sizeof(mechData)); }
int mech_getPos(int mechNo)   { return mechData[mechNo].pos; }
int mech_getSpeed(int mechNo) { return mechData[mechNo].speed / mechData[mechNo].ret; }
void mech_addLong(int mechNo, int sol1, int sol2, int type, int length, int steps, mech_tSwData sw[]) {
  if ((mechNo >= 0) && (mechNo < MECH_MAXMECH)) {
    ptMechData md = &mechData[mechNo];
    int ii = 0;

    md->sol1 = sol1; md->sol2 = sol2;
    md->length = length; md->steps = steps;
    md->type = type & 0xff;
    md->ret = ((type & 0xff000000) ? ((type>>24) & 0x00ff) : 1);
    md->acc = ((type & 0x00ffff00) ? ((type>>8)  & 0xffff) : 1);
    do md->swPos[ii] = sw[ii]; while (sw[ii++].swNo);
    md->pos = -1; /* not initialized */
  }
}
void mech_add(int mechNo, mech_ptInitData id) {
  mech_addLong(mechNo, id->sol1, id->sol2, id->type, id->length, id->steps, &id->sw[0]);
}

void mech_update(int mechNo) {
  ptMechData md = &mechData[mechNo];
  if (md->sol1) {
    int speed = md->speed;
    int dir = 0;
    int ii;

    { /*-- check power direction -1, 0, 1 --*/
      int sol = (core_getPulsedSol(md->sol1) != 0) +
                2*(md->sol2 ? (core_getPulsedSol(md->sol2) != 0) : 0);
      if (md->type & MECH_TWOSTEPSOL)
        dir = (sol != md->last);
      else if (md->type & MECH_TWODIRSOL)
        dir = (sol == 1) - (sol == 2);
      else /* MECH_ONEDIRSOL | MECH_ONESOL */
        dir = (sol == 1) - (sol == 3);
      md->last = sol;
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
        md->pos = md->length * MECH_STEP * (1-cos(anglePos*M_PI/md->length/MECH_STEP)) / 2;
      else /* MECH_LINEAR */
        md->pos = (anglePos >= md->length*MECH_STEP) ? md->length*2*MECH_STEP-anglePos : anglePos;
      md->anglePos = anglePos;
      md->pos = md->pos * md->steps / md->length / MECH_STEP;
    }
    /*-- update switches --*/
    for (ii = 0; md->swPos[ii].swNo > 0; ii++)
      core_setSw(md->swPos[ii].swNo, FALSE);
    for (ii = 0; md->swPos[ii].swNo > 0; ii++)
      if (((md->swPos[ii].startPos < 0) &&
           ((md->pos % (-md->swPos[ii].startPos)) < md->swPos[ii].endPos)) ||
          ((md->pos >= md->swPos[ii].startPos) &&
           (md->pos <= md->swPos[ii].endPos)))
        core_setSw(md->swPos[ii].swNo, TRUE);
  }
}

void mech_nv(void *file, int write) {
  int ii;
  for (ii = 0; ii < MECH_MAXMECH; ii++) {
    if (write)     osd_fwrite(file, &mechData[ii].anglePos, sizeof(int)); /* Save */
    else if (file) osd_fread (file, &mechData[ii].anglePos, sizeof(int)); /* Load */
    else mechData[ii].anglePos = 0; /* First time */
    mechData[ii].pos = -1;
  }
}
