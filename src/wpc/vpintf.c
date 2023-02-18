#include "driver.h"
#include "core.h"
#include "mech.h"
#include "vpintf.h"
#include "snd_cmd.h"

static struct {
  UINT8  lastLampMatrix[CORE_MAXLAMPCOL];
  UINT8 lastRGBLamps[CORE_MAXRGBLAMPS];
  UINT64 lastSol;
  UINT8  lastModSol[CORE_MODSOL_MAX];
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
#ifndef LIBPINMAME
  return (coreGlobals.lampMatrix[lampNo/8]>>(lampNo%8)) & 0x01;
#else
  return (coreGlobals.lampMatrix[lampNo/8]>>(lampNo%8)) & 0x01 ? 255 : 0;
#endif
}

/*-------------------------------------------
/  get all lamps changed since last call
/  returns number of changed lamps
/-------------------------------------*/
int vp_getChangedLamps(vp_tChgLamps chgStat) {
  UINT8 lampMatrix[CORE_MAXLAMPCOL];
  UINT8 RGBlamps[CORE_MAXRGBLAMPS];
  int idx = 0;
  int ii;

  /*-- get current status --*/
  memcpy(lampMatrix, coreGlobals.lampMatrix, sizeof(lampMatrix));
  memcpy(RGBlamps, coreGlobals.RGBlamps, sizeof(RGBlamps));

  /*-- fill in array --*/
  for (ii = 0; ii < CORE_STDLAMPCOLS+core_gameData->hw.lampCol; ii++) {
    int chgLamp = lampMatrix[ii] ^ locals.lastLampMatrix[ii];
    if (chgLamp) {
      int tmpLamp = lampMatrix[ii];
      int jj;

      for (jj = 0; jj < 8; jj++) {
        if (chgLamp & 0x01) {
          chgStat[idx].lampNo = coreData->m2lamp ? coreData->m2lamp(ii+1, jj) : 0;
#ifndef LIBPINMAME
          chgStat[idx].currStat = tmpLamp & 0x01;
#else
          chgStat[idx].currStat = tmpLamp & 0x01 ? 255 : 0;
#endif
          idx += 1;
        }
        chgLamp >>= 1;
        tmpLamp >>= 1;
      }
    }
  }

  for (ii = 0; ii < CORE_MAXRGBLAMPS; ii++) {
	  int chgLamp = RGBlamps[ii] ^ locals.lastRGBLamps[ii];
	  if (chgLamp) {
		  // With this mapping 1-80 are "legacy" 
		  // 8 bit lamps, and 81+ are modern intensity-level
		  // RGB capable LEDs.  
		  chgStat[idx].lampNo = ii+81;  
		  chgStat[idx].currStat = RGBlamps[ii]; 
		  idx += 1;
	  }
  }

  memcpy(locals.lastLampMatrix, lampMatrix, sizeof(lampMatrix));
  memcpy(locals.lastRGBLamps, RGBlamps, sizeof(RGBlamps));
  return idx;
}

/*-------------------------------------------
/  get all solenoids changed since last call
/  returns number of changed solenoids
/-------------------------------------*/
int vp_getChangedSolenoids(vp_tChgSols chgStat) 
{
	UINT64 allSol = core_getAllSol();
	UINT64 chgSol = (allSol ^ locals.lastSol) & vp_getSolMask64();
	int idx = 0;
	int ii;
	int start = 0, end = CORE_FIRSTCUSTSOL+core_gameData->hw.custSol-1;

	locals.lastSol = allSol;

	// If activated, treat the PWM integrated solenoids
	if (coreGlobals.nModulatedOutputs > 0)
	{
		core_perform_pwm_integration();
		for (ii = 0; ii < CORE_MODOUT_SOL_MAX; ii++)
		{
			UINT8 activeValue = coreGlobals.modulatedOutputs[ii].value;
			if (locals.lastModSol[ii] != activeValue)
			{
				locals.lastModSol[ii] = activeValue;
				chgStat[idx].solNo = ii + 1; // Solenoid number
				chgStat[idx].currStat = activeValue;
				idx += 1;
			}
		}
		start = CORE_MODOUT_SOL_MAX;
		chgSol >>= start;
		allSol >>= start;
	}

	// If activated, treat the solenoid modulation performed by hardware driver (GTS3 & WPC)
	if (options.usemodsol)
	{
		for(ii = start; ii<CORE_MODSOL_MAX; ii++)
		{
			// Skip the VPM reserved solenoids, they will be handled after.  Need to include
			// "flipper" solenoids as WPC may sneak flashers there when upper flippers aren't present.
			// WPC will put unsmoothed 0/1 values on actual flippers so this shouldn't harm anything.
			if (ii==40)
				ii=CORE_FIRSTCUSTSOL-1;
			UINT8 activeValue = coreGlobals.modulatedSolenoids[CORE_MODSOL_CUR][ii];
			if (locals.lastModSol[ii] != activeValue)
			{
				locals.lastModSol[ii] = activeValue;
				chgStat[idx].solNo = ii + 1; // Solenoid number
				chgStat[idx].currStat = activeValue;
				idx += 1;
			}
		}
		// Treat the VPM reserved solenoids the old way. 
		chgSol >>= (40 - start);
		allSol >>= (40 - start);
		start = 40;
		end = CORE_FIRSTCUSTSOL-1;
	}

	// Treat the remaining solenoids as binary state
	for (ii = start; ii < end; ii++) 
	{
		if (chgSol & 0x01) {
			chgStat[idx].solNo = ii+1; // Solenoid number
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

  if (coreGlobals.nModulatedOutputs > 0)
  {
	 core_perform_pwm_integration();
	 for (ii = 0; ii < CORE_MAXGI; ii++) {
		allGI[ii] = coreGlobals.modulatedOutputs[CORE_MODOUT_SOL_MAX + ii].value;
	 }
  }
  else
  {
	  memcpy(allGI, coreGlobals.gi, sizeof(allGI));
  }

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
	// TODO This is a bit of a B2S compatibility hack - B2S precludes us from adding a proper new setting,
	// Therefore we use this setting for modulated and PWM settings, also see VPinMame Controller.put_SolMask()
	if (1001 <= no && no <= 1200)
		// Map to solenoid output PWM settings (first solenoid is #1 to ...)
		vp_setModOutputType(VP_OUT_SOLENOID, no - 1000, mask);
	else if (1201 <= no && no <= 1300)
		// Map to GI output PWM settings (first GI is #1 to #5)
		vp_setModOutputType(VP_OUT_GI, no - 1200, mask);
	else if (1301 <= no && no <= 2000)
		// Map to lamp output PWM settings (first lamp is #1 to ...)
		vp_setModOutputType(VP_OUT_LAMP, no - 1300, mask);
	else if (no == 2)
		// Use index 2 to turn on/off modulated solenoids
		options.usemodsol = mask;
	else
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

/*-----------
/  set Output Modulation Type ('no' starts at 1 upward, for example 1-5 for WPC GI)
/-----------*/
void vp_setModOutputType(int output, int no, int type) {
	// For the time being, the only supported output type is solenoid, but the API is designed to be 
	// extended to lamp matrix (needed by Whitestar for Lord of the ring) and GI (needed by all WPC).
	int pos;
	if (output == VP_OUT_SOLENOID && 1 <= no && no <= CORE_MODOUT_SOL_MAX)
		pos = no - 1;
	else if (output == VP_OUT_GI && 1 <= no && no <= CORE_MODOUT_GI_MAX)
		pos = CORE_MODOUT_SOL_MAX + no - 1;
	else if (output == VP_OUT_LAMP && 1 <= no && no <= CORE_MODOUT_LAMP_MAX)
		pos = CORE_MODOUT_SOL_MAX + CORE_MODOUT_GI_MAX + no - 1;
	else
		return;
	if (coreGlobals.modulatedOutputs[pos].type != type)
	{
		int prev_type = coreGlobals.modulatedOutputs[pos].type;
		coreGlobals.modulatedOutputs[pos].type = type;
		if (type == CORE_MODOUT_DEFAULT && prev_type != CORE_MODOUT_DEFAULT)
			coreGlobals.nModulatedOutputs--;
		else if (type != CORE_MODOUT_DEFAULT && prev_type == CORE_MODOUT_DEFAULT)
			coreGlobals.nModulatedOutputs++;
	}
}

int vp_getModOutputType(int output, int no) {
	int pos;
	if (output == VP_OUT_SOLENOID && 1 <= no && no <= CORE_MODOUT_SOL_MAX)
		pos = no - 1;
	else if (output == VP_OUT_GI && 1 <= no && no <= CORE_MODOUT_GI_MAX)
		pos = CORE_MODOUT_SOL_MAX + no - 1;
	else if (output == VP_OUT_LAMP && 1 <= no && no <= CORE_MODOUT_LAMP_MAX)
		pos = CORE_MODOUT_SOL_MAX + CORE_MODOUT_GI_MAX + no - 1;
	else
		return CORE_MODOUT_DEFAULT;
	return coreGlobals.modulatedOutputs[pos].type;
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
      idx += 1;
    }
  }
  memcpy(locals.lastSeg, coreGlobals.drawSeg, sizeof(locals.lastSeg));
  return idx;
}
