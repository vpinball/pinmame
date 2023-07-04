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
#include <bitset>
#include <mutex>
#include <vector>

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

// Structure to hold information about active streams
struct _stream_info {
	_stream_info() : hstream(0), hsync(0), stream_type(static_cast<AltsoundSampleType>(0)), \
		channel_idx(0), sample_path(), ducking(1.0f), stop_music(false), \
		loop(0), gain(1.0f) {}
	~_stream_info() {
		LOG(("Destroying HSTREAM: %u\n", hstream));
	}

	unsigned long hstream;
	unsigned long hsync;
	enum AltsoundSampleType stream_type;
	unsigned int channel_idx;
	std::string sample_path;
	float ducking;
	bool stop_music;
	bool loop;
	float gain;
};

enum AltsoundSampleType {
	UNDEFINED = 0,
	MUSIC,
	JINGLE,
	SFX,
	CALLOUT,
	SOLO,
	OVERLAY
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

// Structure for managing sample type behavior
typedef struct _behavior_info {
	enum class BehaviorBits {
		MUSIC = 0,
		CALLOUT,
		SFX,
		SOLO,
		OVERLAY
	};

	std::bitset<5> ducks = 0;
	std::bitset<5> stops = 0;
	std::bitset<5> pauses = 0;

	float music_duck_vol = 1.0f;
	float callout_duck_vol = 1.0f;
	float sfx_duck_vol = 1.0f;
	float solo_duck_vol = 1.0f;
	float overlay_duck_vol = 1.0f;
} BehaviorInfo;


// Structure for holding Altsound2 sample data
typedef struct _sample_info {
	unsigned int id;
	std::string type;
	float duck = 1.0f;
	float gain = 1.0f;
	std::string fname;
	bool loop = false;
} SampleInfo;

typedef std::vector<SampleInfo> Samples;

// ---------------------------------------------------------------------------
// Helper function prototypes
// ---------------------------------------------------------------------------

// translate BASS error codes to printable strings
const char* get_bass_err();  // function prototype

// translate AltsoundSampleType enum values to strings
std::string toString(AltsoundSampleType sampleType);

// tranlsate string representation of AltsoundSample to enum value
AltsoundSampleType toSampleType(const std::string& type_in);

// convert provided string to uppercase
std::string toUpper(const std::string& string_in);

// convert provided string to lowercase
std::string toLower(const std::string& string_in);

#endif // ALTSOUND_DATA_H