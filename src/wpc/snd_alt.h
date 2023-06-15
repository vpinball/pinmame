#ifndef SND_ALT_H
#define SND_ALT_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <stdio.h>

#include "altsound_data.h"

#ifdef __cplusplus
  extern "C" {
#endif
  #include "driver.h"
#ifdef __cplusplus
  }
#endif
#define ALT_MAX_CMDS 4

// Structure for command data
typedef struct _cmd_data {
unsigned int cmd_counter;
int stored_command;
unsigned int cmd_filter;
unsigned int cmd_buffer[ALT_MAX_CMDS];
} CmdData;

// ---------------------------------------------------------------------------
// snd_cmd function prototypes
// ---------------------------------------------------------------------------

#ifdef __cplusplus
  extern "C" {
#endif
  // Main entrypoint for AltSound handling
  void alt_sound_handle(int boardNo, int cmd);
  
  // Pause/Resume all streams
  void alt_sound_pause(BOOL pause);
  
  // Exit AltSound processing and clean up
  void alt_sound_exit();
#ifdef __cplusplus
  }
#endif

// ---------------------------------------------------------------------------
// Helper function prototypes
// ---------------------------------------------------------------------------

// Function to initialize all altound variables and data structures
BOOL alt_sound_init(CmdData* cmds_out);

// Function to pre-process commands based on ROM hardware platform
void preprocess_commands(CmdData* cmds_out, int cmd);

//DAR_TODO
// This is called after all other command processing.
// ? Does it matter?
// ? can it be called right after combining?
// ? If any of the 16-bit commands trigger channel 0 to stop, can the rest of
// the function be allowed to run?
// Function to process combined commands based on ROM hardware platform
void postprocess_commands(const unsigned int combined_cmd);

// Helper function to get short path for sample filenames
//const char* get_short_path(const char* long_path_in);

#endif //SND_ALT_H
