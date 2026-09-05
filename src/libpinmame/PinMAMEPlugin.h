// license:BSD-3-Clause

#pragma once

///////////////////////////////////////////////////////////////////////////////
// PinMAME controller plugin
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
// This interface is part of a work in progress and will evolve likely a lot
// before being considered stable. Do not use it, or if you do, use it knowing
// that you're plugin will be broken by the upcoming updates.
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//

#define PMPI_NAMESPACE                       "PinMAME"

// PinMAME identify emulated machines by a game name which can be either a rom name
// or an alias name. They are reported in the controller enumeration using the unique
// id "pinmame::game".
//
// An emulator implementing PinMAME emulation may report these unique ids only if it 
// implements the state description and messages defined in this header.
#define PMPI_GAMEID_PREFIX                   "pinmame::"


// Message to gather machine information, message must be a PinMAMEMachineStateMsg
#define PMPI_GET_MACHINE_STATE               "GetMachineState:1"

typedef struct PinMAMEMachineStateMsg
{
   int version; // Always 1 (to allow upgrading this message in later revisions)
   const char* game; // Game, that is to say ROM or alias name
   const char* rom; // ROM id, usually the same as the game unless an alias is used
   uint64_t hardwareGen;
} PinMAMEMachineStateMsg;

#define PMPI_READ_MEMORY                     "ReadMemory:1"

typedef struct PinMAMEReadMemoryMsg
{
   int version;         // Always 1 (to allow upgrading this message in later revisions)
   uint32_t address;    // Request: start address in the main CPU address space
   uint32_t size;       // Request: number of bytes to read
   uint8_t* data;       // Request: caller-owned buffer, at least 'size' bytes
   uint32_t read;       // Response: bytes actually read, 0 if unavailable
} PinMAMEReadMemoryMsg;


// State groups
// 
// The groups correspond to the hardware of WPC as PinMAME started as a WPC 
// emulator and other hardwares were added along the way, keeping the original
// layout (1 switch matrix, 1 lamp matrix, 1 high current output group, some GIs
// and dip switches).
// 
#define PMPI_GROUP_SOLENOID        1 // 'Solenoids', index is 1 based
#define PMPI_GROUP_GI              2 // 'Global Illumination', index is 0 based
#define PMPI_GROUP_LAMP            3 // 'Lamps', indexing depends on each driver
#define PMPI_GROUP_MECH            4 // Position & speed of each mech (core or user defined)
#define PMPI_GROUP_SWITCH          5 // Playfield and cabinet switches, index depends on each driver and can be negative (for cabinet switches)
#define PMPI_GROUP_DIPSWITCH       6 // DIP switches
#define PMPI_GROUP_GAMESTATE       7 // Game states derived from live memory
#define PMPI_GROUP_VPM_SOLENOID    8 // VPinMAME compatible solenoids (the result used to depend on the request path, also exposing internal storage layout)
#define PMPI_GROUP_VPM_GI          9 // VPinMAME compatible GI
#define PMPI_GROUP_VPM_LAMP       10 // VPinMAME compatible lamps
#define PMPI_GROUP_VPM_MECH       11 // VPinMAME compatible mechs

// Game events
//
// Some PinMAME driver exposes communication between the main controller board (CPU node) and the child nodes (Sound & DMD board)
// and SAM driver exposes console data through these event messages
#define PMPI_EVT_ON_AUDIO_CMD                "OnAudioCmd:1"
#define PMPI_EVT_ON_DISPLAY_CMD              "OnDisplayCmd:1"
#define PMPI_EVT_ON_CONSOLE_DATA             "OnConsoleData:1"

typedef struct PinMAMEChildBoardEventMsg
{
   uint32_t boardNo;
   uint32_t cmd;
} PinMAMEChildBoardEventMsg;

typedef struct PinMAMEConsoleDataMsg
{
   uint32_t size;
   uint8_t* data;
} PinMAMEConsoleDataMsg;
