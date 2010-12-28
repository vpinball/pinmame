#include "driver.h"
extern "C" {
#include <wpc/core.h>
}
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <wpc/wpc.h>
#include <wpc/se.h>
#include "p-roc.h"
#include "p-roc_drivers.h"

static int se_m2sw(int col, int row) { return col*8+(7-row)+1; }

bool ignoreCoils[80] = { FALSE };
std::vector<int> activeCoils;
CoilDriver coilDrivers [256];
extern PRMachineType procType;
extern YAML::Node yamlDoc;

void set_swState(int value, int type) {
  switch (type) {
    case kPREventTypeSwitchOpenDebounced:
    case kPREventTypeSwitchClosedDebounced:
    case kPREventTypeSwitchOpenNondebounced:
    case kPREventTypeSwitchClosedNondebounced:
      if (procType == kPRMachineSternWhitestar || procType == kPRMachineSternSAM) {
        // Flippers need to go to column 12 for some reason
        if (value < 12) { /* dedicated switches */
          int temp = (7-(value & 0x7));
            core_setSw(se_m2sw(10, temp), (type & kPREventTypeSwitchClosedDebounced));
        }
        // Service switches need to go to column -1 for some reason.
        else if (value < 16) 
          // Dedicated Switches
          core_setSw(se_m2sw(-1, 7-(value & 0x7)), (type & kPREventTypeSwitchClosedDebounced));
        else if (value >= 32) 
          // Matrix Switches
          core_setSw(se_m2sw(((value - 32) >> 4), value & 0x7), (type & kPREventTypeSwitchClosedDebounced));
      }
      else {
        if (value < 8) 
          // Flipper Switches
          core_setSw(wpc_m2sw(CORE_FLIPPERSWCOL, value), (type & kPREventTypeSwitchClosedDebounced));
        else if (value < 16)  
          // Dedicated Switches
          core_setSw(wpc_m2sw(0, value & 0x7), (type & kPREventTypeSwitchClosedDebounced));
        else if (value >= 32) 
          // Matrix Switches
          core_setSw(wpc_m2sw(((value - 16) >> 4), value & 0x7), (type & kPREventTypeSwitchClosedDebounced));
      }
      break;

    case kPREventTypeDMDFrameDisplayed:
      break;
  }
}

void ConfigureWPCFlipperSwitchRule (PRHandle proc, int swNum, int mainCoilNum, int holdCoilNum, int pulseTime)
{
    const int numDriverRules = 2;
    PRDriverState fldrivers[numDriverRules];
    PRSwitchRule sw;
    
    // Flipper on rules
    PRDriverGetState(proc, mainCoilNum, &fldrivers[0]);
    PRDriverStatePulse(&fldrivers[0],pulseTime); // Pulse coil for 34ms.
    PRDriverGetState(proc, holdCoilNum, &fldrivers[1]);
    PRDriverStatePulse(&fldrivers[1],0);  // Turn on indefintely (set pulse for 0ms)
    sw.notifyHost = false;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedNondebounced, &sw, fldrivers, numDriverRules);
    sw.notifyHost = true;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedDebounced, &sw, NULL, 0);
    
    // Flipper off rules
    PRDriverGetState(proc, mainCoilNum, &fldrivers[0]);
    PRDriverStateDisable(&fldrivers[0]); // Disable main coil
    PRDriverGetState(proc, holdCoilNum, &fldrivers[1]);
    PRDriverStateDisable(&fldrivers[1]); // Disable hold coil
    sw.notifyHost = false;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchOpenNondebounced, &sw, fldrivers, numDriverRules);
    sw.notifyHost = true;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchOpenDebounced, &sw, NULL, 0);
}

void ConfigureSternFlipperSwitchRule (PRHandle proc, int swNum, int mainCoilNum, int pulseTime, int patterOnTime, int patterOffTime)
{
    const int numDriverRules = 1;
    PRDriverState fldrivers[numDriverRules];
    PRSwitchRule sw;
    
    // Flipper on rules
    PRDriverGetState(proc, mainCoilNum, &fldrivers[0]);
    PRDriverStatePatter(&fldrivers[0],patterOnTime,patterOffTime,pulseTime); // Pulse coil for 34ms.
    sw.notifyHost = false;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedNondebounced, &sw, fldrivers, numDriverRules);
    sw.notifyHost = true;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedDebounced, &sw, NULL, 0);
    
    // Flipper off rules
    PRDriverGetState(proc, mainCoilNum, &fldrivers[0]);
    PRDriverStateDisable(&fldrivers[0]); // Disable main coil
    sw.notifyHost = false;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchOpenNondebounced, &sw, fldrivers, numDriverRules);
    sw.notifyHost = true;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchOpenDebounced, &sw, NULL, 0);
}

void ConfigureBumperRule (PRHandle proc, int swNum, int coilNum, int pulseTime)
{
    const int numDriverRules = 1;
    PRDriverState fldrivers[numDriverRules];
    PRSwitchRule sw;
    
    PRDriverGetState(proc, coilNum, &fldrivers[0]);
    PRDriverStatePulse(&fldrivers[0],pulseTime); // Pulse coil for 34ms.
    sw.reloadActive = true;
    sw.notifyHost = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedNondebounced, &sw, fldrivers, numDriverRules);
    sw.notifyHost = true;
    sw.reloadActive = false;
    PRSwitchUpdateRule(proc, swNum, kPREventTypeSwitchClosedDebounced, &sw, NULL, 0);
}

void AddIgnoreCoil(int num) {
    ignoreCoils[num] = TRUE;
}
  
void procConfigureDriverDefaults()
{
    if (yamlDoc.size() > 0) {

      // WPC  Flippers
      std::string numStr;
      const YAML::Node& coils = yamlDoc[kCoilsSection];
      for (YAML::Iterator coilsIt = coils.begin(); coilsIt != coils.end(); ++coilsIt)
      {

          std::string coilName;
          coilsIt.first() >> coilName;
          int coilNum, pulseTime, patterOnTime, patterOffTime;
          yamlDoc[kCoilsSection][coilName][kNumberField] >> numStr; coilNum = PRDecode(procType, numStr.c_str());

          // Look for yaml entries defining coil pulse times.
          if (yamlDoc[kCoilsSection][coilName].FindValue(kPulseTimeField)) {
             yamlDoc[kCoilsSection][coilName][kPulseTimeField] >> pulseTime;
             coilDrivers[coilNum].SetPulseTime(pulseTime);
          }

          // Look for yaml entries defining coil patter times.
          if (yamlDoc[kCoilsSection][coilName].FindValue(kPatterOnTimeField) &&  
              yamlDoc[kCoilsSection][coilName].FindValue(kPatterOnTimeField)) {
             yamlDoc[kCoilsSection][coilName][kPatterOnTimeField] >> patterOnTime;
             yamlDoc[kCoilsSection][coilName][kPatterOffTimeField] >> patterOffTime;
             coilDrivers[coilNum].SetPatterTimes(patterOnTime, patterOffTime);
          }


      }
    }
}


void procConfigureSwitchRules()
{
    if (yamlDoc.size() > 0) {

      // WPC  Flippers
      std::string numStr;
      const YAML::Node& flippers = yamlDoc[kFlippersSection];
      for (YAML::Iterator flippersIt = flippers.begin(); flippersIt != flippers.end(); ++flippersIt)
      {
          int swNum, coilMain, coilHold;
          std::string flipperName;
          *flippersIt >> flipperName;
          if (procType == kPRMachineWPC || procType == kPRMachineWPC95)
          {
              yamlDoc[kSwitchesSection][flipperName][kNumberField] >> numStr; swNum = PRDecode(procType, numStr.c_str());
              yamlDoc[kCoilsSection][flipperName + "Main"][kNumberField] >> numStr; coilMain = PRDecode(procType, numStr.c_str());
              yamlDoc[kCoilsSection][flipperName + "Hold"][kNumberField] >> numStr; coilHold = PRDecode(procType, numStr.c_str());
              ConfigureWPCFlipperSwitchRule (coreGlobals.proc, swNum, coilMain, coilHold, kFlipperPulseTime);
              AddIgnoreCoil(coilMain);
              AddIgnoreCoil(coilHold);
          }
          else if (procType == kPRMachineSternWhitestar || procType == kPRMachineSternSAM)
          {
              yamlDoc[kSwitchesSection][flipperName][kNumberField] >> numStr; swNum = PRDecode(procType, numStr.c_str());
              yamlDoc[kCoilsSection][flipperName + "Main"][kNumberField] >> numStr; coilMain = PRDecode(procType, numStr.c_str());
              ConfigureSternFlipperSwitchRule (coreGlobals.proc, swNum, coilMain, kFlipperPulseTime, kFlipperPatterOnTime, kFlipperPatterOffTime);
              AddIgnoreCoil(coilMain);
          }
      }
      
      const YAML::Node& bumpers = yamlDoc[kBumpersSection];
      for (YAML::Iterator bumpersIt = bumpers.begin(); bumpersIt != bumpers.end(); ++bumpersIt)
      {
          int swNum, coilNum;
          // WPC  Slingshots
          std::string bumperName;
          *bumpersIt >> bumperName;
          yamlDoc[kSwitchesSection][bumperName][kNumberField] >> numStr; swNum = PRDecode(procType, numStr.c_str());
          yamlDoc[kCoilsSection][bumperName][kNumberField] >> numStr; coilNum = PRDecode(procType, numStr.c_str());
          ConfigureBumperRule (coreGlobals.proc, swNum, coilNum, kBumperPulseTime);
          AddIgnoreCoil(coilNum);
      }
  }
}

void procSetSwitchStates() {

  int i;
  PREventType procSwitchStates[kPRSwitchPhysicalLast + 1];

  // Get all of the switch states from the P-ROC.
  if (PRSwitchGetStates( coreGlobals.proc, procSwitchStates, kPRSwitchPhysicalLast + 1 ) == kPRFailure)
  {
    fprintf(stderr, "Error: Unable to retrieve switch states from P-ROC.\n");
  }
  else
  {
    // Copy the returning states into the local switches array.
    fprintf(stderr, "\nInitial switch states:");
    for (i = 0; i <= kPRSwitchPhysicalLast; i++)
    {
      if (i%16==0) fprintf(stderr, "\n");
      fprintf(stderr, "%d ", procSwitchStates[i]);

      if (procType == kPRMachineSternSAM) {
        set_swState(i, procSwitchStates[i]);
      }
      else if ((i < 32) || ((i%16) < 8)) set_swState(i, procSwitchStates[i]);
    }
    fprintf(stderr, "\n\n");
  }
}

void procConfigureDefaultSwitchRules() {
  int ii;
  PRSwitchConfig switchConfig;

  // Configure switch controller registers
  switchConfig.clear = FALSE;
  switchConfig.use_column_8 = (procType == kPRMachineWPC);
  switchConfig.use_column_9 = FALSE; // No WPC machines actually use this.
  switchConfig.hostEventsEnable = TRUE;
  switchConfig.directMatrixScanLoopTime = 2; // milliseconds
  switchConfig.pulsesBeforeCheckingRX = 10;
  switchConfig.inactivePulsesAfterBurst = 12;
  switchConfig.pulsesPerBurst = 6;
  switchConfig.pulseHalfPeriodTime = 13; // milliseconds
  PRSwitchUpdateConfig(coreGlobals.proc, &switchConfig);

  //fprintf(stderr, "\nConfiguring P-ROC switch rules");
  // Go through the switches array and reset the current status of each switch
  for (ii = 0; ii <= kPRSwitchPhysicalLast; ii++)
  {
    PRSwitchRule swRule;
    if ((procType == kPRMachineWPC || procType == kPRMachineWPC95) && 
        !((ii < 32) || ((ii % 16) < 8)))
      swRule.notifyHost = 0;
    else swRule.notifyHost = 1;
    PRSwitchUpdateRule(coreGlobals.proc, ii, kPREventTypeSwitchClosedNondebounced, &swRule, NULL, 0);
    PRSwitchUpdateRule(coreGlobals.proc, ii, kPREventTypeSwitchOpenNondebounced, &swRule, NULL, 0);
  }
}

void procDriveLamp(int num, int state) {
  PRDriverState lampState;
  memset(&lampState, 0, sizeof(lampState));
  PRDriverGetState(coreGlobals.proc, num, &lampState);
  lampState.state = state;
  lampState.outputDriveTime = PROC_LAMP_DRIVE_TIME;
  lampState.waitForFirstTimeSlot = FALSE;
  lampState.timeslots = 0;
  lampState.patterOnTime = 0;
  lampState.patterOffTime = 0;
  lampState.patterEnable = FALSE;

  PRDriverUpdateState(coreGlobals.proc, &lampState);
}

void procGetSwitchEvents() {
  int i;
  PREvent eventArray[16];

  int numEvents = PRGetEvents(coreGlobals.proc, eventArray, 16);
  for (i = 0; i < numEvents; i++) {
    PREvent *pEvent = &eventArray[i];
    set_swState(pEvent->value,pEvent->type);
    //fprintf(stderr, "\nP-ROC switch event: value: %d, type: %d",pEvent->value, pEvent->type);
  }
}

CoilDriver::CoilDriver() {
  num = 0;
  timeLastChanged = 0;
  numPatterOn = 0;
  numPatterOff = 0;
  avgOnTime = 0;
  avgOffTime = 0;
  patterActive = 0;
  useDefaultPulseTime = 1;
  useDefaultPatterTimes = 1;
}

void CoilDriver::SetNum(int number) {
  num = number;
}

void CoilDriver::SetPulseTime(int ms) {
  pulseTime = ms;
  useDefaultPulseTime = 0;
}

void CoilDriver::SetPatterTimes(int msOn, int msOff) {
  patterOnTime = msOn;
  patterOffTime = msOff;
  useDefaultPatterTimes = 0;
  fprintf(stderr, "\nSetting patter for coil: %d, on:%d, off:%d",num, msOn, msOff);
}

void CoilDriver::ResetPatter() {
  numPatterOn = 0;
  numPatterOff = 0;
  avgOnTime = 0;
  avgOffTime = 0;
  patterActive = 0;
  reqPatterState = 0;
}

void CoilDriver::CheckEndPatter() {
  if (patterActive) {
    // If too much time has passed since the last change, disable the patter.
    if ( ((clock()/CLOCKS_PER_MS) - timeLastChanged) > PROC_MAX_PATTER_INTERVAL_MS) {
      long int msTime = clock() / CLOCKS_PER_MS;
      fprintf (stderr, "\nt time: %ld: Ending Patter for Coil %d.", msTime, num);
      Drive(reqPatterState);
      ResetPatter();
    }
  }
}

void CoilDriver::Drive(int state) {
    PRDriverState coilState;
    PRDriverGetState(coreGlobals.proc, num, &coilState);
    if (state) {
      if (useDefaultPulseTime) PRDriverStatePulse(&coilState, PROC_COIL_DRIVE_TIME);
      else PRDriverStatePulse(&coilState, pulseTime);
    }
    else PRDriverStateDisable(&coilState);
    PRDriverUpdateState(coreGlobals.proc, &coilState);
#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 4.5)
    long int msTime = clock() / CLOCKS_PER_MS;
#endif
}

void CoilDriver::Patter(int msOn, int msOff) {
    PRDriverState coilState;
    PRDriverGetState(coreGlobals.proc, num, &coilState);
    if (useDefaultPatterTimes) PRDriverStatePatter(&coilState, msOn, msOff, 0);
    else PRDriverStatePatter(&coilState, patterOnTime, patterOffTime, 0);
    PRDriverUpdateState(coreGlobals.proc, &coilState);
    patterActive = 1;
}

void CoilDriver::RequestDrive(int state) {
    long int msSinceChanged = (clock() / CLOCKS_PER_MS) - timeLastChanged;
    timeLastChanged = clock() / CLOCKS_PER_MS;

    if ( msSinceChanged > PROC_MAX_PATTER_INTERVAL_MS) {
      ResetPatter();
      Drive(state);
    }
    else if (patterActive) {
      reqPatterState = state;
    }
    else {
      if (state) {
        avgOffTime = ((avgOffTime * numPatterOff) + msSinceChanged) / ++numPatterOff;
        // If Enough transitions have occurred to indicate a patter, enable patter.
        if (numPatterOff > 2) Patter(avgOnTime, avgOffTime);
        // Otherwise drive as requested.
        else Drive(state);
      }
      else {
        avgOnTime = ((avgOnTime * numPatterOn) + msSinceChanged) / ++numPatterOn;
        // Update patter stats.
        Drive(state);
      }
    }
}

void procDriveCoil(int num, int state) {
  if (!ignoreCoils[num]) coilDrivers[num].RequestDrive(state);
}

#define PROC_PATTER_END_TIME 50

void procCheckActiveCoils() {
  int i;
  for (i=0; i<256; i++) coilDrivers[i].CheckEndPatter();
}

void procCheckActiveCoilsUnused() {
  std::vector<int> eraseList;

  PRDriverState coilState;
  int timeSinceChanged = 60;

  for (std::vector<int>::iterator it = activeCoils.begin(); it!=activeCoils.end(); ++it) {
    PRDriverGetState(coreGlobals.proc, *it, &coilState);
    if (timeSinceChanged > PROC_PATTER_END_TIME) {
      procDriveCoil(*it, coilState.state);
      // Don't erase yet because it will change the vector and affect the iterator.
      eraseList.push_back(*it);
      fprintf(stderr, "\nEnding patter for coil %d", *it);
    }
  }

  for (std::vector<int>::iterator it = eraseList.begin(); it!=eraseList.end(); ++it) {
    activeCoils.erase(activeCoils.begin() + *it);
  }
}

void procInitializeCoilDrivers() {
      int i;
      for (i=0; i<256; i++) coilDrivers[i].SetNum(i);
}
