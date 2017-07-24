/*
 * The MIT License
 * Copyright (c) 2009 Gerry Stellenberg, Adam Preble
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
/** @file pinproc.h
 * @brief libpinproc, P-ROC Layer 1 API (Preliminary)
 *
 */
#ifndef PINPROC_PINPROC_H
#define PINPROC_PINPROC_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

/*
 * 3rd party "stdint.h" replacement available for Visual C++ before version 2010.
 * http://en.wikipedia.org/wiki/Stdint.h#External_links
 * -> http://msinttypes.googlecode.com/svn/trunk/stdint.h
 * Place inside the major include dir (e.g. %ProgramFiles%\Microsoft Visual Studio\VC98\INCLUDE)
 */
#include <stdint.h>

/** @cond */
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the pinproc_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// PINPROC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#undef PINPROC_API

#ifdef PINPROC_DLL	// Using or Building PinPROC DLL (definition defined manually)
    #ifdef pinproc_EXPORTS	// Building PinPROC DLL (definition created by CMake or defined manually)
        #define PINPROC_API __declspec(dllexport)
    #else
        #define PINPROC_API __declspec(dllimport)
    #endif
#endif
// fallback for non-DLL usage and builds
#ifndef PINPROC_API
    #define PINPROC_API
#endif

#if defined(__cplusplus)
    #define PINPROC_EXTERN_C_BEGIN extern "C" {
    #define PINPROC_EXTERN_C_END   }
#else
    #define PINPROC_EXTERN_C_BEGIN
    #define PINPROC_EXTERN_C_END
#endif

PINPROC_EXTERN_C_BEGIN
/** @endcond */

// Types

typedef int32_t bool_t; // FIXME: This needs better platform independence.

typedef int32_t PRResult; /**< See: #kPRSuccess and #kPRFailure. */
#define kPRSuccess (1)    /**< Success value for #PRResult. */
#define kPRFailure (0)    /**< Failure value for #PRResult. */

typedef void * PRHandle;     /**< Opaque type used to reference an individual P-ROC device.  Created with PRCreate() and destroyed with PRDelete().  This value is used as the first parameter to all P-ROC API function calls. */
#define kPRHandleInvalid (0) /**< Value returned by PRCreate() on failure.  Indicates an invalid #PRHandle. */

typedef enum PRLogLevel {
    kPRLogVerbose,
    kPRLogInfo,
    kPRLogWarning,
    kPRLogError
} PRLogLevel;

typedef void (*PRLogCallback)(PRLogLevel level, const char *text); /**< Function pointer type for a custom logging callback.  See: PRLogSetCallback(). */
PINPROC_API void PRLogSetCallback(PRLogCallback callback); /**< Replaces the default logging handler with the given callback function. */

PINPROC_API void PRLogSetLevel(PRLogLevel level);

PINPROC_API const char *PRGetLastErrorText();

/**
 * @defgroup device Device Creation & Deletion
 * @{
 */

typedef enum PRMachineType {
    kPRMachineInvalid = 0,
    kPRMachineCustom = 1,
    kPRMachineWPCAlphanumeric = 2,
    kPRMachineWPC = 3,
    kPRMachineWPC95 = 4,
    kPRMachineSternWhitestar = 5,
    kPRMachineSternSAM = 6,
    kPRMachinePDB = 7,
} PRMachineType;

// PRHandle Creation and Deletion

PINPROC_API PRHandle PRCreate(PRMachineType machineType); /**< Create a new P-ROC device handle.  Only one handle per device may be created. This handle must be destroyed with PRDelete() when it is no longer needed.  Returns #kPRHandleInvalid if an error occurred. */
PINPROC_API void PRDelete(PRHandle handle);               /**< Destroys an existing P-ROC device handle. */

#define kPRResetFlagDefault (0) /**< Only resets state in memory and does not write changes to the device. */
#define kPRResetFlagUpdateDevice (1) /**< Instructs PRReset() to update the device once it has reset the configuration to its defaults. */

/**
 * @brief Resets internally maintained driver and switch rule structures.
 * @param resetFlags Specify #kPRResetFlagDefault to only reset the configuration in host memory.  #kPRResetFlagUpdateDevice will write the default configuration to the device, effectively disabling all drivers and switch rules.
 */
PINPROC_API PRResult PRReset(PRHandle handle, uint32_t resetFlags);

/** @} */ // End of Device Creation & Deletion

// I/O

/** Flush all pending write data out to the P-ROC. */
PINPROC_API PRResult PRFlushWriteData(PRHandle handle);

/** Write data out to the P-ROC immediately (does not require a call to PRFlushWriteData). */
PINPROC_API PRResult PRWriteData(PRHandle handle, uint32_t moduleSelect, uint32_t startingAddr, int32_t numWriteWords, uint32_t * writeBuffer);

/** Read data from the P-ROC. */
PINPROC_API PRResult PRReadData(PRHandle handle, uint32_t moduleSelect, uint32_t startingAddr, int32_t numReadWords, uint32_t * readBuffer);

// Manager
/** @defgroup Manager
 * @{
 */

typedef struct PRManagerConfig {
    bool_t reuse_dmd_data_for_aux;
    bool_t invert_dipswitch_1;
} PRManagerConfig;

/** Update Manager configuration */
PINPROC_API PRResult PRManagerUpdateConfig(PRHandle handle, PRManagerConfig *managerConfig);

// Drivers
/** @defgroup drivers Driver Manipulation
 * @{
 */

#define kPRDriverGroupsMax (26)   /**< Number of available driver groups. */
#define kPRDriverCount (256)          /**< Total number of drivers */

#define kPRDriverAuxCmdOutput (2)
#define kPRDriverAuxCmdDelay  (1)
#define kPRDriverAuxCmdJump   (0)

typedef struct PRDriverGlobalConfig {
    bool_t enableOutputs; // Formerly enable_direct_outputs
    bool_t globalPolarity;
    bool_t useClear;
    bool_t strobeStartSelect;
    uint8_t startStrobeTime;
    uint8_t matrixRowEnableIndex1;
    uint8_t matrixRowEnableIndex0;
    bool_t activeLowMatrixRows;
    bool_t encodeEnables;
    bool_t tickleSternWatchdog;
    bool_t watchdogExpired;
    bool_t watchdogEnable;
    uint16_t watchdogResetTime;
} PRDriverGlobalConfig;

typedef struct PRDriverGroupConfig {
    uint8_t groupNum;
    uint16_t slowTime;
    uint8_t enableIndex;
    uint8_t rowActivateIndex;
    uint8_t rowEnableSelect;
    bool_t matrixed;
    bool_t polarity;
    bool_t active;
    bool_t disableStrobeAfter;
} PRDriverGroupConfig;

typedef struct PRDriverState {
    uint16_t driverNum;
    uint8_t outputDriveTime;
    bool_t polarity;
    bool_t state;
    bool_t waitForFirstTimeSlot;
    uint32_t timeslots;
    uint8_t patterOnTime;
    uint8_t patterOffTime;
    bool_t patterEnable;
    bool_t futureEnable;
} PRDriverState;

typedef struct PRDriverAuxCommand {
    bool_t active;
    bool_t muxEnables;
    uint8_t command;
    uint8_t enables;
    uint8_t extraData;
    uint8_t data;
    uint16_t delayTime;
    uint8_t jumpAddr;
} PRDriverAuxCommand;

/** Update registers for the global driver configuration. */
PINPROC_API PRResult PRDriverUpdateGlobalConfig(PRHandle handle, PRDriverGlobalConfig *driverGlobalConfig);

PINPROC_API PRResult PRDriverGetGroupConfig(PRHandle handle, uint8_t groupNum, PRDriverGroupConfig *driverGroupConfig);
/** Update registers for the given driver group configuration. */
PINPROC_API PRResult PRDriverUpdateGroupConfig(PRHandle handle, PRDriverGroupConfig *driverGroupConfig);

PINPROC_API PRResult PRDriverGetState(PRHandle handle, uint8_t driverNum, PRDriverState *driverState);
/**
 * @brief Sets the state of the given driver (lamp or coil).
 */
PINPROC_API PRResult PRDriverUpdateState(PRHandle handle, PRDriverState *driverState);
/**
 * @brief Loads the driver defaults for the given machine type.
 *
 * PRReset() calls this function internally; this function is useful for basing custom driver settings off of the defaults for a particular machine.
 * @note This function does not update the P-ROC hardware, only the internal data structures.  Use PRDriverGetGlobalConfig() and PRDriverGetGroupConfig() to retrieve the settings.
 */
PINPROC_API PRResult PRDriverLoadMachineTypeDefaults(PRHandle handle, PRMachineType machineType);

// Driver Group Helper functions:

/**
 * Disables (turns off) the given driver group.
 * This function is provided for convenience.  See PRDriverGroupDisable() for a full description.
 */
PINPROC_API PRResult PRDriverGroupDisable(PRHandle handle, uint8_t groupNum);
// Driver Helper functions:

/**
 * Disables (turns off) the given driver.
 * This function is provided for convenience.  See PRDriverStateDisable() for a full description.
 */
PINPROC_API PRResult PRDriverDisable(PRHandle handle, uint8_t driverNum);
/**
 * Pulses the given driver for a number of milliseconds.
 * This function is provided for convenience.  See PRDriverStatePulse() for a full description.
 */
PINPROC_API PRResult PRDriverPulse(PRHandle handle, uint8_t driverNum, uint8_t milliseconds);
/**
 * Pulses the given driver for a number of milliseconds when the hardware reaches a specific timestamp.
 * This function is provided for convenience.  See PRDriverStatePulse() for a full description.
 */
PINPROC_API PRResult PRDriverFuturePulse(PRHandle handle, uint8_t driverNum, uint8_t milliseconds, uint32_t futureTime);
/**
 * Assigns a repeating schedule to the given driver.
 * This function is provided for convenience.  See PRDriverStateSchedule() for a full description.
 */
PINPROC_API PRResult PRDriverSchedule(PRHandle handle, uint8_t driverNum, uint32_t schedule, uint8_t cycleSeconds, bool_t now);
/**
 * Assigns a pitter-patter schedule (repeating on/off) to the given driver.
 * This function is provided for convenience.  See PRDriverStatePatter() for a full description.
 */
PINPROC_API PRResult PRDriverPatter(PRHandle handle, uint8_t driverNum, uint8_t millisecondsOn, uint8_t millisecondsOff, uint8_t originalOnTime, bool_t now);
/**
 * Assigns a pitter-patter schedule (repeating on/off) to the given driver on for the given duration.
 * This function is provided for convenience.  See PRDriverStatePulsedPatter() for a full description.
 */
PINPROC_API PRResult PRDriverPulsedPatter(PRHandle handle, uint8_t driverNum, uint8_t millisecondsOn, uint8_t millisecondsOff, uint8_t originalOnTime, bool_t now);
/**
 * Prepares an Aux Command to drive the Aux bus.
 * This function is provided for convenience.
 */
PINPROC_API void PRDriverAuxPrepareOutput(PRDriverAuxCommand *auxCommand, uint8_t data, uint8_t extraData, uint8_t enables, bool_t muxEnables, uint16_t delayTime);
/**
 * Prepares an Aux Command to delay the Aux logic.
 * This function is provided for convenience.
 */
PINPROC_API void PRDriverAuxPrepareDelay(PRDriverAuxCommand *auxCommand, uint16_t delayTime);
/**
 * Prepares an Aux Command to have the Aux memory pointer jump to a new address.
 * This function is provided for convenience.
 */
PINPROC_API void PRDriverAuxPrepareJump(PRDriverAuxCommand *auxCommand, uint8_t jumpAddr);
/**
 * Prepares a disabled Aux Command.
 * This function is provided for convenience.
 */
PINPROC_API void PRDriverAuxPrepareDisable(PRDriverAuxCommand *auxCommand);

/** Tickle the watchdog timer. */
PINPROC_API PRResult PRDriverWatchdogTickle(PRHandle handle);

/**
 * Changes the given #PRDriverGroupConfig to reflect a disabled group.
 * @note The driver group config structure must be applied using PRDriverUpdateGroupConfig() to have any effect.
 */
PINPROC_API void PRDriverGroupStateDisable(PRDriverGroupConfig *driverGroup);
/**
 * Changes the given #PRDriverState to reflect a disabled state.
 * @note The driver state structure must be applied using PRDriverUpdateState() or linked to a switch rule using PRSwitchUpdateRule() to have any effect.
 */
PINPROC_API void PRDriverStateDisable(PRDriverState *driverState);
/**
 * Changes the given #PRDriverState to reflect a pulse state.
 * @param milliseconds Number of milliseconds to pulse the driver for.
 * @note The driver state structure must be applied using PRDriverUpdateState() or linked to a switch rule using PRSwitchUpdateRule() to have any effect.
 */
PINPROC_API void PRDriverStatePulse(PRDriverState *driverState, uint8_t milliseconds);
/**
 * Changes the given #PRDriverState to reflect a future scheduled pulse state.
 * @param milliseconds Number of milliseconds to pulse the driver for.
 * @param futureTime Value indicating at which HW timestamp the pulse should occur.  Currently only the low 10-bits are used.
 * @note The driver state structure must be applied using PRDriverUpdateState() or linked to a switch rule using PRSwitchUpdateRule() to have any effect.
 */
PINPROC_API void PRDriverStateFuturePulse(PRDriverState *driverState, uint8_t milliseconds, uint32_t futureTime);
/**
 * Changes the given #PRDriverState to reflect a scheduled state.
 * Assigns a repeating schedule to the given driver.
 * @note The driver state structure must be applied using PRDriverUpdateState() or linked to a switch rule using PRSwitchUpdateRule() to have any effect.
 */
PINPROC_API void PRDriverStateSchedule(PRDriverState *driverState, uint32_t schedule, uint8_t cycleSeconds, bool_t now);
/**
 * @brief Changes the given #PRDriverState to reflect a pitter-patter schedule state.
 * Assigns a pitter-patter schedule (repeating on/off) to the given driver.
 * @note The driver state structure must be applied using PRDriverUpdateState() or linked to a switch rule using PRSwitchUpdateRule() to have any effect.
 *
 * Use originalOnTime to pulse the driver for a number of milliseconds before the pitter-patter schedule begins.
 */
PINPROC_API void PRDriverStatePatter(PRDriverState *driverState, uint8_t millisecondsOn, uint8_t millisecondsOff, uint8_t originalOnTime, bool_t now);

/**
 * @brief Changes the given #PRDriverState to reflect a pitter-patter schedule state.
 * Just like the regular Patter above, but PulsePatter only drives the patter
 * scheduled for the given number of milliseconds before disabling the driver.
 */
PINPROC_API void PRDriverStatePulsedPatter(PRDriverState *driverState, uint8_t millisecondsOn, uint8_t millisecondsOff, uint8_t patterTime, bool_t now);

/**
 * Write Aux Port commands into the Aux Port command memory.
 */

PINPROC_API PRResult PRDriverAuxSendCommands(PRHandle handle, PRDriverAuxCommand * commands, uint8_t numCommands, uint8_t startingAddr);

/**
 * @brief Converts a coil, lamp, switch, or GI string into a P-ROC driver number.
 * The following formats are accepted: Cxx (coil), Lxx (lamp), Sxx (matrix switch), SFx (flipper grounded switch), or SDx (dedicated grounded switch).
 * If the string does not match this format it will be converted into an integer using atoi().
 */
PINPROC_API uint16_t PRDecode(PRMachineType machineType, const char *str);

/** @} */ // End of Drivers

// Switches

/** @defgroup switches Switches and Events
 * @{
 */

// Events
// Closed == 0, Open == 1
typedef enum PREventType {
    kPREventTypeInvalid = 0,
    kPREventTypeSwitchClosedDebounced    = 1, /**< The switch has gone from open to closed and the signal has been debounced. */
    kPREventTypeSwitchOpenDebounced      = 2, /**< The switch has gone from closed to open and the signal has been debounced. */
    kPREventTypeSwitchClosedNondebounced = 3, /**< The switch has gone from open to closed and the signal has not been debounced. */
    kPREventTypeSwitchOpenNondebounced   = 4, /**< The switch has gone from closed to open and the signal has not been debounced. */
    kPREventTypeDMDFrameDisplayed        = 5, /**< A DMD frame has been displayed. */
    kPREventTypetLast = kPREventTypeSwitchOpenNondebounced
} PREventType;

typedef struct PREvent {
    PREventType type;  /**< The type of event that has occurred.  Usually a switch event at this point. */
    uint32_t value;    /**< For switch events, the switch number that has changed. For DMD events, the frame buffer that was just displayed. */
    uint32_t time;     /**< Time (in milliseconds) that this event occurred. */
} PREvent;

/** Get all of the available events that have been received.
 * \return Number of events returned; -1 if an error occurred.
 */
PINPROC_API int PRGetEvents(PRHandle handle, PREvent *eventsOut, int maxEvents);


#define kPRSwitchPhysicalFirst (0)   /**< Switch number of the first physical switch. */
#define kPRSwitchPhysicalLast (255)  /**< Switch number of the last physical switch.  */
#define kPRSwitchNeverDebounceFirst (192)  /**< Switch number of the first switch that doesn't need to debounced.  */
#define kPRSwitchNeverDebounceLast (255)   /**< Switch number of the last switch that doesn't need to be debounce.   */
#define kPRSwitchCount (256)
#define kPRSwitchRulesCount (kPRSwitchCount << 2) /**< Total number of available switch rules. */

typedef struct PRSwitchConfig {
    bool_t clear; // Drive the clear output
    bool_t hostEventsEnable; // Drive the clear output
    bool_t use_column_9; // Use switch matrix column 9
    bool_t use_column_8; // Use switch matrix column 8
    uint8_t directMatrixScanLoopTime; // milliseconds
    uint8_t pulsesBeforeCheckingRX;
    uint8_t inactivePulsesAfterBurst;
    uint8_t pulsesPerBurst;
    uint8_t pulseHalfPeriodTime; // milliseconds
} PRSwitchConfig;

typedef struct PRSwitchRule {
    bool_t reloadActive; /**< If true, any associated driver changes resulting from this rule will only happen at most once every 256ms. */
    bool_t notifyHost; /**< If true this switch change event will provided to the user via PRGetEvents(). */
} PRSwitchRule;

/** Update the switch controller configurion registers */
PINPROC_API PRResult PRSwitchUpdateConfig(PRHandle handle, PRSwitchConfig *switchConfig);

/**
 * @brief Configures the handling of switch rules within P-ROC.
 *
 * P-ROC's switch rule system allows the user to decide which switch events are returned to software,
 * as well as optionally linking one or more driver state changes to rules to create immediate feedback (such as in pop bumpers).
 *
 * For instance, P-ROC can provide debounced switch events for a flipper button so software can apply lange change behavior.
 * This is accomplished by configuring the P-ROC with a switch rule for the flipper button and then receiving the events via the PRGetEvents() call.
 * The same switch can also be configured with a non-debounced rule to fire a flipper coil.
 * Multiple driver changes can be tied to a single switch state transition to create more complicated effects: a slingshot
 * switch that fires the slingshot coil, a flash lamp, and a score event.
 *
 * P-ROC holds four different switch rules for each switch: closed to open and open to closed, each with a debounced and non-debounced versions:
 *  - #kPREventTypeSwitchOpenDebounced
 *  - #kPREventTypeSwitchClosedDebounced
 *  - #kPREventTypeSwitchOpenNondebounced
 *  - #kPREventTypeSwitchClosedNondebounced
 *
 * @section Examples
 *
 * Configuring a basic switch rule to simply notify software via PRGetEvents() without affecting any coil/lamp drivers:
 * @code
 * PRSwitchRule rule;
 * rule.notifyHost = true;
 * PRSwitchUpdateRule(handle, switchNum, kPREventTypeSwitchOpenDebounced, &rule, NULL, 0);
 * @endcode
 *
 * Configuring a pop bumper switch to pulse the coil and a flash lamp for 50ms each:
 * @code
 * // Configure a switch rule to fire the coil and flash lamp:
 * PRSwitchRule rule;
 * rule.notifyHost = false;
 * PRDriverState drivers[2];
 * PRDriverGetState(handle, drvCoilPopBumper1, &drivers[0]);
 * PRDriverGetState(handle, drvFlashLamp1, &drivers[1]);
 * PRDriverStatePulse(&drivers[0], 50);
 * PRDriverStatePulse(&drivers[1], 50);
 * PRSwitchUpdateRule(handle, swPopBumper1, kPREventTypeSwitchClosedNondebounced,
 *                      &rule, drivers, 2);
 * // Now configure a switch rule to process scoring in software:
 * rule.notifyHost = true;
 * PRSwitchUpdateRule(handle, swPopBumper1, kPREventTypeSwitchClosedDebounced,
 *                      &rule, NULL, 0);
 * @endcode
 *
 * @param handle The P-ROC device handle.
 * @param switchNum The index of the switch this configuration affects.
 * @param eventType The switch rule for the specified switchNum to be configured.
 * @param rule A pointer to the #PRSwitchRule structure describing how this state change should be handled.  May not be NULL.
 * @param linkedDrivers An array of #PRDriverState structures describing the driver state changes to be made when this switch rule is triggered.  May be NULL if numDrivers is 0.
 * @param numDrivers Number of elements in the linkedDrivers array.  May be zero or more.
 */
PINPROC_API PRResult PRSwitchUpdateRule(PRHandle handle, uint8_t switchNum, PREventType eventType, PRSwitchRule *rule, PRDriverState *linkedDrivers, int numDrivers, bool_t drive_outputs_now);

/** Returns a list of PREventTypes describing the states of the requested number of switches  */
PINPROC_API PRResult PRSwitchGetStates(PRHandle handle, PREventType * switchStates, uint16_t numSwitches);

/** @} */ // End of Switches & Events

// DMD

/**
 * @defgroup dmd DMD Control
 * @{
 */
typedef struct PRDMDConfig {
    uint8_t numRows;
    uint16_t numColumns;
    uint8_t numSubFrames;
    uint8_t numFrameBuffers;
    bool_t autoIncBufferWrPtr;
    bool_t enableFrameEvents;
    bool_t enable;
    uint8_t rclkLowCycles[8];
    uint8_t latchHighCycles[8];
    uint16_t deHighCycles[8];
    uint8_t dotclkHalfPeriod[8];
} PRDMDConfig;

/** Sets the configuration registers for the DMD driver. */
PINPROC_API int32_t PRDMDUpdateConfig(PRHandle handle, PRDMDConfig *dmdConfig);
/** Updates the DMD frame buffer with the given data. */
PINPROC_API PRResult PRDMDDraw(PRHandle handle, uint8_t * dots);

/** @} */ // End of DMD


// JTAG

/**
 * @defgroup jtag JTAG interface control
 * @{
 */

typedef struct PRJTAGStatus {
    bool_t commandComplete;
    bool_t tdi;
} PRJTAGStatus;

typedef struct PRJTAGOutputs {
    bool_t tckMask;
    bool_t tmsMask;
    bool_t tdoMask;
    bool_t tck;
    bool_t tms;
    bool_t tdo;
} PRJTAGOutputs;

/** Force JTAG outputs (TCK, TDO, TMS) to specific values. Optionally toggle the clock when driving only TDO and/or TMS.*/
PINPROC_API PRResult PRJTAGDriveOutputs(PRHandle handle, PRJTAGOutputs * jtagOutputs, bool_t toggleClk);
/** Store data to be shifted out on TDO */
PINPROC_API PRResult PRJTAGWriteTDOMemory(PRHandle handle, uint16_t tableOffset, uint16_t numWords, uint32_t * tdoData);
/** Shift stored TDO data onto the TDO pin, toggling TCK on every bit. */
PINPROC_API PRResult PRJTAGShiftTDOData(PRHandle handle, uint16_t numBits, bool_t dataBlockComplete);
/** Get the contents of the TDI memory. */
PINPROC_API PRResult PRJTAGReadTDIMemory(PRHandle handle, uint16_t tableOffset, uint16_t numWords, uint32_t * tdiData);
/** Read the JTAG status register for the command complete bit and JTAG pin states. */
PINPROC_API PRResult PRJTAGGetStatus(PRHandle handle, PRJTAGStatus * status);

/** @} */ // End of DMD

/** @cond */
PINPROC_EXTERN_C_END
/** @endcond */

/**
 * @mainpage libpinproc API Documentation
 *
 * This is the documentation for libpinproc, the P-ROC Layer 1 API.
 */

#endif /* PINPROC_PINPROC_H */
