// ---------------------------------------------------------------------------
// altsound_data.hpp
// 06/23/23 - Dave Roscoe
//
// Holds global variables and structures in support of AltSound processing.
// This provides a cleaner separation between the legacy C code and the new C++
// code, and is necessary to ensure that only one instance of these exist
// across multiple compilation units
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND_DATA_H
#define ALTSOUND_DATA_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Library includes
#include <array>
#include <mutex>

#define BASS_NO_STREAM 0
#define ALT_MAX_CHANNELS 16

// ----------------------------------------------------------------------------
// Logging support
// ----------------------------------------------------------------------------

#if (defined(_DEBUG) && defined(VPINMAME))
  #ifdef __cplusplus
    extern "C" {
  #endif
	extern void logerror(const char *text, ...);
  #ifdef __cplusplus
  }
  #endif

#ifdef LOG
  #undef LOG
#endif
  #define LOG(x) logerror x;
#else
  #ifdef LOG
    #undef LOG
  #endif
  #define LOG(x)
#endif // VPinMAME DEBUG

// ----------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------
struct _stream_info;  // forward declaration for clarity
typedef _stream_info AltsoundStreamInfo;
typedef std::array<AltsoundStreamInfo*, ALT_MAX_CHANNELS> StreamArray;

// DAR@20230623
// This is necessary because BASS SYNCPROC callbacks run in a separate thread
// from the main AltSound processor, and both modify shared memory
//
extern std::mutex io_mutex;  // thread synchronization mutex

extern const char* bass_err_names[];  // debug output support 
extern StreamArray channel_stream;  // channel stream array
extern float master_vol;
extern float global_vol;

// ----------------------------------------------------------------------------
// Global Data Structures
// ----------------------------------------------------------------------------

struct _stream_info {
	_stream_info() : hstream(0), hsync(0), stream_type(static_cast<AltsoundSampleType>(0)), \
		channel_idx(0), sample_path(0), ducking(1.0f), stop_music(false), \
		loop(0), gain(1.0f) {}
	~_stream_info() {
		LOG(("Destroying HSTREAM: %u\n", hstream));
	}

	unsigned long hstream;
	unsigned long hsync;
	enum AltsoundSampleType stream_type;
	unsigned int channel_idx;
	char* sample_path;
	float ducking;
	bool stop_music;
	char loop;
	float gain;
};

enum AltsoundSampleType {
	UNDEFINED,
	MUSIC,
	JINGLE,
	SFX,
};

// Structure for holding traditional AltSound sample data
typedef struct _pin_samples {
	int * ID;
	char ** files_with_subpath;
	signed char * channel;
	float * gain;
	float * ducking;
	unsigned char * loop;
	unsigned char * stop;
	unsigned int num_files;
} PinSamples;

// ---------------------------------------------------------------------------
// Helper function to translate BASS error codes to printable strings
// ---------------------------------------------------------------------------

const char* get_bass_err();  // function prototype

#endif // ALTSOUND_DATA_H