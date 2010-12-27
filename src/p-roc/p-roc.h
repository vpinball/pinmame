#ifndef INC_PROC
#define INC_PROC

#include <p-roc/pinproc.h>
#include "time.h"

#define PROC_NUM_DMD_FRAMES 4
// 255 is the longest pulse time supported by the P-ROC.  PinMAME should turn
// most coils off well before this expires.
#define PROC_COIL_DRIVE_TIME 0

#define PROC_LAMP_DRIVE_TIME 0 // Turn on indefinitely.  Let PinMAME turn off.

#define kFlippersSection "PRFlippers"
#define kBumpersSection "PRBumpers"
#define kCoilsSection "PRCoils"
#define kSwitchesSection "PRSwitches"
#define kNumberField "number"
#define kPulseTimeField "pulseTime"
#define kPatterOnTimeField "patterOnTime"
#define kPatterOffTimeField "patterOffTime"

#define kFlipperPulseTime (34) // 34 ms
#define kFlipperPatterOnTime (1) // 2 ms
#define kFlipperPatterOffTime (20) // 2 ms
#define kBumperPulseTime (25) // 25 ms

#ifdef __cplusplus
 extern "C" {
#endif

  // Display functions
  void procDMDInit(void);
  void procClearDMD (void);
  void procDrawDot (int x, int y, int color);
  void procDrawSegment (int x, int y, int seg);
  void procFillDMDSubFrame( int frameIndex, UINT8 * dotData, int length );
  void procUpdateDMD(void);
  void procUpdateAlphaDisplay (UINT16 *top, UINT16 *bottom); 

  // Gameitem functions
  void procSetSwitchStates (void);
  void procDriveLamp(int num, int state);
  void procDriveCoil(int num, int state); 
  void procGetSwitchEvents(void);
  void procInitializeCoilDrivers();
  void procCheckActiveCoils(void);
  void procConfigureDefaultSwitchRules(); 
  void procConfigureDriverDefaults();
  void procConfigureSwitchRules();

  // Generic P-ROC functions.
  int procInitialize(char * yaml_filename);
  void procTickleWatchdog(void);
  void procFlush(void);
#ifdef __cplusplus
 }
#endif

#endif /* INC_PROC */

