#include "driver.h"
#include "core.h"
#include "mech.h"
#include "vpintf.h"
#include "snd_cmd.h"

static struct {
  UINT8  lastPhysicsOutput[CORE_MODOUT_MAX];
  UINT8  lastLampMatrix[CORE_MAXLAMPCOL];
  int    lastGI[CORE_MAXGI];
  UINT64 lastSol;
  UINT32 solMask[2];
  UINT8  dips[VP_MAXDIPBANKS];
  UINT16 lastSeg[CORE_SEGCOUNT];
  int    lastSoundCommandIndex;
  mech_tInitData md;
} locals;

INLINE UINT8 saturatedByte(float v) { return (UINT8)(255.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v)); }

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
  if (coreData && coreData->lamp2m) lampNo = coreData->lamp2m(lampNo) - 8;
  /*-- Physical output mode: return a physically meaningful value depending on the output type --*/
  if (coreGlobals.nLamps && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_LAMPS)))
    return (int)saturatedByte(coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + lampNo].value);
  return (coreGlobals.lampMatrix[lampNo/8]>>(lampNo%8)) & 0x01;
}

/*------------------------------------
/  get status of a solenoid (0=off, !0=on)
/-------------------------------------*/
int vp_getSolenoid(int solNo)
{
  return core_getSol(solNo);
}

/*-------------------------------------------
/  get status of a GIString (0=off, !0=on)
/ (WPC, Whitestar and SAM games only)
/-------------------------------------*/
int vp_getGI(int giNo)
{
  /*-- Physical output mode: return a physically meaningful value depending on the output type --*/
  if (coreGlobals.nGI && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_GI)))
    return (int)saturatedByte(coreGlobals.physicOutputState[CORE_MODOUT_GI0 + giNo].value);
  return coreGlobals.gi[giNo];
}

/*-------------------------------------------
/  get all lamps changed since last call
/  returns number of changed lamps
/-------------------------------------*/
int vp_getChangedLamps(vp_tChgLamps chgStat) {
  int idx = 0;
  /*-- fill in array --*/
  if (coreGlobals.nLamps && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_LAMPS)))
  {
    int ii;
    for (ii = 0; ii < coreGlobals.nLamps; ii++) {
      const UINT8 val = saturatedByte(coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + ii].value);
      if (val != locals.lastPhysicsOutput[CORE_MODOUT_LAMP0 + ii]) {
        chgStat[idx].lampNo = coreData && coreData->m2lamp ? coreData->m2lamp((ii / 8) + 1, ii & 7) : 0;
        chgStat[idx].currStat = val;
        idx++;
        locals.lastPhysicsOutput[CORE_MODOUT_LAMP0 + ii] = val;
      }
    }
  }
  else
  {
    UINT8 lampMatrix[CORE_MAXLAMPCOL];
    memcpy(lampMatrix, (void*)coreGlobals.lampMatrix, sizeof(lampMatrix));
    const int hasSAMModulatedLeds = (core_gameData->gen & GEN_SAM) && (core_gameData->hw.lampCol > 2);
    const int nCol = CORE_STDLAMPCOLS + (hasSAMModulatedLeds ? 2 : core_gameData->hw.lampCol);
    int ii;
    for (ii = 0; ii < nCol; ii++) {
      int chgLamp = lampMatrix[ii] ^ locals.lastLampMatrix[ii];
      if (chgLamp) {
        int tmpLamp = lampMatrix[ii];
        int jj;

        for (jj = 0; jj < 8; jj++) {
          if (chgLamp & 0x01) {
            chgStat[idx].lampNo = coreData && coreData->m2lamp ? coreData->m2lamp(ii+1, jj) : 0;
            chgStat[idx].currStat = tmpLamp & 0x01;
            idx++;
          }
          chgLamp >>= 1;
          tmpLamp >>= 1;
        }
      }
    }
    memcpy((void*)locals.lastLampMatrix, lampMatrix, sizeof(lampMatrix));
    // Backward compatibility for modulated LED & RGB LEDs of SAM hardware
    if (hasSAMModulatedLeds) {
      for (ii = 80; ii < coreGlobals.nLamps; ii++) {
        const UINT8 val = saturatedByte(coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + ii].value);
        if (val != locals.lastPhysicsOutput[CORE_MODOUT_LAMP0 + ii]) {
          chgStat[idx].lampNo = ii + 1;
          chgStat[idx].currStat = val;
          idx++;
          locals.lastPhysicsOutput[CORE_MODOUT_LAMP0 + ii] = val;
        }
      }
    }
  }
  return idx;
}

/*-------------------------------------------
/  get all solenoids changed since last call
/  returns number of changed solenoids
/-------------------------------------*/
int vp_getChangedSolenoids(vp_tChgSols chgStat) 
{
  int ii, idx = 0;
  // The backward compatibility is not perfect here: mod sol was only available for a bunch of generations, and would limit modulation to the first 32 solenoids
  if (coreGlobals.nSolenoids && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)))
  {
    float state[CORE_MODOUT_SOL_MAX];
    core_getAllPhysicSols(state);
    for (ii = 0; ii < coreGlobals.nSolenoids; ii++) {
      if ((options.usemodsol & CORE_MODOUT_ENABLE_MODSOL) && (coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + ii].type == CORE_MODOUT_BULB_44_6_3V_AC_REV))
        state[ii] = 1.0f - state[ii];
      UINT8 v = saturatedByte(state[ii]);
      if (v != locals.lastPhysicsOutput[CORE_MODOUT_SOL0 + ii]) {
        chgStat[idx].solNo = ii + 1;
        chgStat[idx].currStat = v;
        idx++;
        locals.lastPhysicsOutput[CORE_MODOUT_SOL0 + ii] = v;
      }
    }
  }
  else {
	 UINT64 allSol = core_getAllSol();
	 UINT64 chgSol = (allSol ^ locals.lastSol) & vp_getSolMask64();
	 locals.lastSol = allSol;
	 for (ii = 0; ii < CORE_FIRSTCUSTSOL + core_gameData->hw.custSol - 1; ii++)
	 {
		if (chgSol & 0x01) {
		  chgStat[idx].solNo = ii+1; // Solenoid number
		  chgStat[idx].currStat = (allSol & 0x01);
		  idx++;
		}
		chgSol >>= 1;
		allSol >>= 1;
	 }
  }
  return idx;
}

/*-------------------------------------------
/  get all GIstrings changed since last call
/  returns number of canged GIstrings
/-------------------------------------*/
int vp_getChangedGI(vp_tChgGIs chgStat) {
  /*-- Physical output mode: return a physically meaningful value depending on the output type --*/
  if (coreGlobals.nGI && (options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_GI))
  {
	  int idx = 0;
	  for (int i = 0; i < coreGlobals.nGI; i++) {
		  UINT8 val = saturatedByte(coreGlobals.physicOutputState[CORE_MODOUT_GI0 + i].value);
		  if (val != locals.lastPhysicsOutput[CORE_MODOUT_GI0 + i]) {
			  chgStat[idx].giNo = i;
			  chgStat[idx].currStat = val;
			  locals.lastPhysicsOutput[CORE_MODOUT_GI0 + i] = val;
			  idx++;
		  }
	  }
	  return idx;
  }

  /*-- add changed to array --*/
  int allGI[CORE_MAXGI];
  int idx = 0;
  int ii;
  memcpy(allGI, (void*)coreGlobals.gi, sizeof(allGI));
  for (ii = 0; ii < CORE_MAXGI; ii++) {
    if (allGI[ii] != locals.lastGI[ii]) {
      chgStat[idx].giNo     = ii;
      chgStat[idx].currStat = allGI[ii];
      idx++;
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
	// TODO This is a horrible B2S compatibility hack - B2S precludes us from adding a proper new setting,
	// Therefore we use this setting for modulated and PWM settings, also see VPinMame Controller.put_SolMask()
	if (1001 <= no && no <= 1200)
		// Map to solenoid output PWM settings (first solenoid is #1 to #64)
		vp_setModOutputType(VP_OUT_SOLENOID, no - 1000, mask);
	else if (1201 <= no && no <= 1300)
		// Map to GI output PWM settings (first GI is #1 to #5)
		vp_setModOutputType(VP_OUT_GI, no - 1200, mask);
	else if (1301 <= no && no <= 2000)
		// Map to lamp output PWM settings (first lamp is #1 to #336)
		vp_setModOutputType(VP_OUT_LAMP, no - 1300, mask);
	else if (2001 <= no)
		// Map to alpha segment output PWM settings (first alpha segment is #1 to ...)
		vp_setModOutputType(VP_OUT_ALPHASEG, no - 2000, mask);
	else if (no == 2)
	{
		// Use index 2 to turn on/off modulated solenoids, using a bit mask:
		// 1 enable legacy "modulated solenoid"
		// 2 enable physical outputs (solenoids/lamps/GI/alphanum segments)
		options.usemodsol = (options.usemodsol & CORE_MODOUT_FORCE_ON) | (mask==2 ? CORE_MODOUT_ENABLE_PHYSOUT_ALL : mask);
	}
	else if (no == 0 || no == 1)
	{
		// Binary mask for solenoid output
		locals.solMask[no] = mask;
	}
}

/*-----------
/  get Solenoid Mask
/-----------*/
int vp_getSolMask(int no) {
	// TODO This is a horrible B2S compatibility hack - B2S precludes us from adding a proper new setting,
	// Therefore we use this setting for modulated and PWM settings, also see VPinMame Controller.put_SolMask()
	if (1001 <= no && no <= 1200)
		return vp_getModOutputType(VP_OUT_SOLENOID, no - 1000);
	else if (1201 <= no && no <= 1300)
		return vp_getModOutputType(VP_OUT_GI, no - 1200);
	else if (1301 <= no && no <= 2000)
		return vp_getModOutputType(VP_OUT_LAMP, no - 1300);
	else if (2001 <= no)
		return vp_getModOutputType(VP_OUT_ALPHASEG, no - 2000);
	else if (no == 2)
		return options.usemodsol & ~CORE_MODOUT_FORCE_ON;
	else if (no == 0 || no == 1)
		return locals.solMask[no];
	else
		return -1;
}

UINT64 vp_getSolMask64(void) {
  return (((UINT64)locals.solMask[1])<<32) | locals.solMask[0];
}

/*-----------
/  set Output Modulation Type ('no' starts at 1 upward, for example 1-5 for WPC GI)
/-----------*/
void vp_setModOutputType(int output, int no, int type) {
	int pos = -1;
	if (output == VP_OUT_SOLENOID && 1 <= no && no <= coreGlobals.nSolenoids)
		pos = CORE_MODOUT_SOL0 + no - 1;
	else if (output == VP_OUT_GI && 1 <= no && no <= coreGlobals.nGI)
		pos = CORE_MODOUT_GI0 + no - 1;
	else if (output == VP_OUT_LAMP && 1 <= no && no <= coreGlobals.nLamps)
		pos = CORE_MODOUT_LAMP0 + no - 1;
	else if (output == VP_OUT_ALPHASEG && 1 <= no && no <= coreGlobals.nAlphaSegs)
		pos = CORE_MODOUT_SEG0 + no - 1;
	if (pos != -1)
		core_set_pwm_output_type(pos, 1, type);
}

int vp_getModOutputType(int output, int no) {
	int pos;
	if (output == VP_OUT_SOLENOID && 1 <= no && no <= coreGlobals.nSolenoids)
		pos = CORE_MODOUT_SOL0 + no - 1;
	else if (output == VP_OUT_GI && 1 <= no && no <= coreGlobals.nGI)
		pos = CORE_MODOUT_GI0 + no - 1;
	else if (output == VP_OUT_LAMP && 1 <= no && no <= coreGlobals.nLamps)
		pos = CORE_MODOUT_LAMP0 + no - 1;
	else if (output == VP_OUT_ALPHASEG && 1 <= no && no <= coreGlobals.nAlphaSegs)
		pos = CORE_MODOUT_SEG0 + no - 1;
	else
		return -1; // Undefined behavior
	return coreGlobals.physicOutputState[pos].type;
}

extern void time_fence_post(); // in cpuexec.c
extern volatile double time_fence_global_offset;
void vp_setTimeFence(double timeInS)
{
	if (options.time_fence != timeInS)
	{
		if (options.time_fence == 0.0)
			time_fence_global_offset = -timeInS;
		options.time_fence = timeInS;
		time_fence_post();
	}
}

int vp_getMech(int mechNo) {
#if defined(VPINMAME) || defined(LIBPINMAME)
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
  else if (para == 8) locals.md.initialpos = data + 1;
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
int vp_getChangedLEDs(vp_tChgLED chgStat, UINT64 mask, UINT64 mask2) {
  int idx = 0;
  int ii;

  for (ii = 0; ii < CORE_SEGCOUNT; ii++, mask >>= 1) {
    UINT16 chgSeg = coreGlobals.drawSeg[ii] ^ locals.lastSeg[ii];
    if (ii == 64)
      mask = mask2;
    if ((mask & 0x01) && chgSeg) {
      chgStat[idx].ledNo = ii;
      chgStat[idx].chgSeg = chgSeg;
      chgStat[idx].currStat = coreGlobals.drawSeg[ii];
      idx++;
    }
  }
  memcpy(locals.lastSeg, coreGlobals.drawSeg, sizeof(locals.lastSeg));
  return idx;
}
