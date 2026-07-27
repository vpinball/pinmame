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
#define PMPI_GET_MACHINE_STATE               "GetMachineState"

typedef struct PinMAMEMachineStateMsg
{
   int version; // Always 1 (to allow upgrading this message in later revisions)
   const char* game; // Game, that is to say ROM or alias name
   const char* rom; // ROM id, usually the same as the game unless an alias is used
   uint64_t hardwareGen;
} PinMAMEMachineStateMsg;


// State groups
// 
// The groups correspond to the hardware of WPC as PinMAME started as a WPC 
// emulator and other hardwares were added along the way, keeping the original
// layout (1 switch matrix, 1 lamp matrix, 1 high current output group, some GIs
// and dip switches).
// 
// Solenoid/GI/Lamp states can be requested as byte or float:
// . bytes hold similar value as VPinMAME (not exactly as the result used to depend on the request path, and solenoids used to expose internal storage layout)
// . floats are normalized values (0..1)
//
#define PMPI_GROUP_MASK      0xFF00
#define PMPI_GROUP_SOLENOID  0x0000 // 'Solenoids', index is 1 based
#define PMPI_GROUP_GI        0x0100 // 'Global Illumination', index is 0 based
#define PMPI_GROUP_LAMP      0x0200 // 'Lamps', indexing depends on each driver
#define PMPI_GROUP_MECH      0x0300 // float or int32 values for position & speed
#define PMPI_GROUP_SWITCH    0x0400 // boolean states as byte values, index depends on each driver and can be negative (for cabinet switches)
#define PMPI_GROUP_DIPSWITCH 0x0500 // boolean states as byte values
#define PMPI_GROUP_GAMESTATE 0x0600 // game states derived from live memory

// Game events
//
// Some PinMAME driver exposes communication between the main controller board (CPU node) and the child nodes (Sound and/or DMD board) through these messages
#define PMPI_EVT_ON_AUDIO_CMD                "OnAudioCmd"
#define PMPI_EVT_ON_DISPLAY_CMD              "OnDisplayCmd"

typedef struct PinMAMEChildBoardEventMsg
{
   uint32_t boardNo;
   uint32_t cmd;
} PinMAMEChildBoardEventMsg;
