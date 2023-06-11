#ifndef SND_ALT_H
#define SND_ALT_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <stdio.h>

#include "altsound_data.h"
#include "..\ext\bass\bass.h"

#ifdef __cplusplus
  extern "C" {
#endif
  #include "driver.h"
#ifdef __cplusplus
  }
#endif

// For command parsing
#define ALT_MAX_CMDS 4

// ---------------------------------------------------------------------------
// Global data structures
// ---------------------------------------------------------------------------

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
// BASS stream management function prototypes
// ---------------------------------------------------------------------------

// BASS callback for cleaning up after JINGLE playback
void CALLBACK jingle_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

// BASS callback for cleaning up after SFX playback
void CALLBACK sfx_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

// BASS callback for cleaning up after MUSIC playback
void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

// Create BASS stream and optional stream sync
BOOL create_stream(const char* fname_in, const int loop_count_in, SYNCPROC* syncproc_in,\
	               unsigned int* ch_idx_out);
// Create BASS stream sync
HSYNC create_sync(HSTREAM stream_in, SYNCPROC* syncproc_in, unsigned int ch_idx_in);

// Stop BASS stream playback and free stream data
BOOL stop_stream(HSTREAM stream_in);

// Free BASS stream data
BOOL free_stream(HSTREAM stream_in);

// Set volume for stream playback
int set_volume(HSTREAM stream_in, const float vol_in);

// ---------------------------------------------------------------------------
// Helper function prototypes
// ---------------------------------------------------------------------------

// Function to initialize all altound variables and data structures
int alt_sound_init(CmdData* cmds_out, PinSamples* psd_out);

// Function to parse altsound data and populate interal sample storage
int load_samples(PinSamples* pinsamples_out);

// Function to initialize command structure to default conditions
void initialize_cmds(CmdData* cmds_out);

// Function to initialize pinsound sample data
void initialize_sample_data(PinSamples* psd_out);

// Function to pre-process commands based on ROM hardware platform
void preprocess_commands(CmdData* cmds_out, int cmd);

// Function to search for samples matching current command
int get_sample(CmdData* cmds_in, const PinSamples* psd_in, int cmd_in, \
	           unsigned int* cmd_combined_out);

// Return a free channel withing ALT_MAX_CHANNELS
int find_free_channel(void);

// Returns the lowest ducking value of all active streams
float get_min_ducking();

//DAR_TODO
// This is called after all other command processing.
// ? Does it matter?
// ? can it be called right after combining?
// ? If any of the 16-bit commands trigger channel 0 to stop, can the rest of
// the function be allowed to run?
// Function to process combined commands based on ROM hardware platform
void postprocess_commands(const unsigned int combined_cmd);

// Process command to play Jingle/Single
BOOL process_jingle(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out);

// Process command to play SFX/Voice
BOOL process_sfx(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out);

// Process command to play Music
BOOL process_music(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out);

// Helper function to parse BASS error codes to strings
const char* get_bass_err();

// Helper function to get short path for sample filenames
const char* get_short_path(const char* long_path_in);

#endif //SND_ALT_H
