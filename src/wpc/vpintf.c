#include "driver.h"
#include "core.h"
#include "mech.h"
#include "vpintf.h"
#include "snd_cmd.h"

static struct {
  UINT8  lastLampMatrix[CORE_MAXLAMPCOL];
  UINT64 lastSol;
  UINT32 solMask[2];
  int    lastGI[CORE_MAXGI];
  UINT8  dips[VP_MAXDIPBANKS];
  UINT16 lastSeg[CORE_SEGCOUNT];
  int    lastSoundCommandIndex;
  mech_tInitData md;
} locals;

/*-------------------------------
/  Initialise/reset the VP interface
/--------------------------------*/
void vp_init(void) {
  memset(&locals, 0, sizeof(locals));
  locals.solMask[0] = locals.solMask[1] = 0xffffffff;
  mech_init();
}

/*------------------------------------
/  get status of a lamp (0=off, 1=on)
/-------------------------------------*/
int vp_getLamp(int lampNo) {
  if (coreData->lamp2m) lampNo = coreData->lamp2m(lampNo)-8;
  return (coreGlobals.lampMatrix[lampNo/8]>>(lampNo%8)) & 0x01;
}

/*-------------------------------------------
/  get all lamps changed since last call
/  returns number of canged lamps
/-------------------------------------*/
int vp_getChangedLamps(vp_tChgLamps chgStat) {
  UINT8 lampMatrix[CORE_MAXLAMPCOL];
  int idx = 0;
  int ii;

  /*-- get current status --*/
  memcpy(lampMatrix, coreGlobals.lampMatrix, sizeof(lampMatrix));

  /*-- fill in array --*/
  for (ii = 0; ii < CORE_STDLAMPCOLS+core_gameData->hw.lampCol; ii++) {
    int chgLamp = lampMatrix[ii] ^ locals.lastLampMatrix[ii];
    if (chgLamp) {
      int tmpLamp = lampMatrix[ii];
      int jj;

      for (jj = 0; jj < 8; jj++) {
        if (chgLamp & 0x01) {
          chgStat[idx].lampNo = coreData->m2lamp ? coreData->m2lamp(ii+1, jj) : 0;
          chgStat[idx].currStat = tmpLamp & 0x01;
          idx += 1;
        }
        chgLamp >>= 1;
        tmpLamp >>= 1;
      }
    }
  }
  memcpy(locals.lastLampMatrix, lampMatrix, sizeof(lampMatrix));
  return idx;
}

/*-------------------------------------------
/  get all solenoids changed since last call
/  returns number of canged solenoids
/-------------------------------------*/
int vp_getChangedSolenoids(vp_tChgSols chgStat) {
  UINT64 allSol = core_getAllSol();
  UINT64 chgSol = (allSol ^ locals.lastSol) & vp_getSolMask64();
  int idx = 0;
  int ii;

  locals.lastSol = allSol;

  /*-- add changed solenoids to the array --*/
  for (ii = 1; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
    if (chgSol & 0x01) {
      chgStat[idx].solNo = ii; // Solenoid number
      chgStat[idx].currStat = (allSol & 0x01);
      idx += 1;
    }
    chgSol >>= 1;
    allSol >>= 1;
  }
  return idx;
}

/*-------------------------------------------
/  get all GIstrings changed since last call
/  returns number of canged GIstrings
/-------------------------------------*/
int vp_getChangedGI(vp_tChgGIs chgStat) {
  int allGI[CORE_MAXGI];
  int idx = 0;
  int ii;

  memcpy(allGI, coreGlobals.gi, sizeof(allGI));

  /*-- add changed to array --*/
  for (ii = 0; ii < CORE_MAXGI; ii++) {
    if (allGI[ii] != locals.lastGI[ii]) {
      chgStat[idx].giNo     = ii;
      chgStat[idx].currStat = allGI[ii];
      idx += 1;
    }
  }
  memcpy(locals.lastGI, allGI, sizeof(allGI));

  return idx;
}
/*-----------
/  set DIPs
/-----------*/
void vp_setDIP(int dipBank, int value) {
  locals.dips[dipBank] = value;
}

/*-----------
/  get DIPs
/-----------*/
int vp_getDIP(int dipBank) {
  return locals.dips[dipBank];
}

/*-----------
/  set Solenoid Mask
/-----------*/
void vp_setSolMask(int no, int mask) {
  locals.solMask[no] = mask;
}

/*-----------
/  get Solenoid Mask
/-----------*/
int vp_getSolMask(int no) {
  return locals.solMask[no];
}

UINT64 vp_getSolMask64(void) {
  return (((UINT64)locals.solMask[1])<<32) | locals.solMask[0];
}

int vp_getMech(int mechNo) {
#ifdef VPINMAME
  extern int g_fHandleMechanics;
  if (g_fHandleMechanics == 0)
    return (mechNo < 0) ? mech_getSpeed(MECH_MAXMECH/2-1-mechNo) : mech_getPos(MECH_MAXMECH/2-1+mechNo);
  else
#endif
    return core_gameData->hw.getMech ? core_gameData->hw.getMech(mechNo) : 0;
}

void vp_setMechData(int para, int data) {
  if      (para == 0) { mech_add(MECH_MAXMECH/2+data-1, &locals.md); memset(&locals.md, 0, sizeof(locals.md)); }
  else if (para == 1) locals.md.sol1   = data;
  else if (para == 2) locals.md.sol2   = data;
  else if (para == 3) locals.md.length = data;
  else if (para == 4) locals.md.steps  = data;
  else if (para == 5) locals.md.type   = (locals.md.type & 0xfffffe00) | data;
  else if (para == 6) locals.md.type   = (locals.md.type & 0xff0001ff) | (data<<9);
  else if (para == 7) locals.md.type   = (locals.md.type & 0x00ffffff) | (data<<24);
  else if (para % 10 == 0) locals.md.sw[para/10-1].swNo     = data;
  else if (para % 10 == 1) locals.md.sw[para/10-1].startPos = data;
  else if (para % 10 == 2) locals.md.sw[para/10-1].endPos   = data;
  else if (para % 10 == 3) locals.md.sw[para/10-1].pulse    = data;
}

/*-------------------------------------------------
/  get all sound commands issued since last call
/------------------------------------------------*/
int vp_getNewSoundCommands(vp_tChgSound chgSound) {
  int cmds[MAX_CMD_LOG];
  int numcmd = snd_get_cmd_log(&locals.lastSoundCommandIndex, cmds);
  int ii;

  // this would have been even easier if vp_tChgSound was a plain array
  for (ii = 0; ii < numcmd; ii++)
    chgSound[ii].sndNo = cmds[ii];
  return numcmd;
}
/*-------------------------------------------------
/  get all changed segments since last call
/------------------------------------------------*/
int vp_getChangedLEDs(vp_tChgLED chgStat, UINT64 mask) {
  int idx = 0;
  int ii;

  for (ii = 0; ii < CORE_SEGCOUNT; ii++, mask >>= 1) {
    UINT16 chgSeg = coreGlobals.drawSeg[ii] ^ locals.lastSeg[ii];
    if ((mask & 0x01) && chgSeg) {
      chgStat[idx].ledNo = ii;
      chgStat[idx].chgSeg = chgSeg;
      chgStat[idx].currStat = coreGlobals.drawSeg[ii];
      idx += 1;
    }
  }
  memcpy(locals.lastSeg, coreGlobals.drawSeg, sizeof(locals.lastSeg));
  return idx;
}
