// ---------------------------------------------------------------------------
// altsound_data.hpp
//
// Holds global variables and structures in support of AltSound processing.
// This provides a cleaner separation between the legacy C code and the new C++
// code, and is necessary to ensure that only one instance of these exist
// across multiple compilation units
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders:Dave Roscoe
// ---------------------------------------------------------------------------
#ifndef ALTSOUND_DATA_H
#define ALTSOUND_DATA_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Std Library includes
#include <array>
#include <bitset>
#include <mutex>
#include <unordered_map>
#include <vector>

#define BASS_NO_STREAM 0
#define ALT_MAX_CHANNELS 16

#define LOG // DAR_TODO remove when logging converted

// ----------------------------------------------------------------------------
// Global Data Structures
// ----------------------------------------------------------------------------

struct _stream_info;  // forward declaration for clarity
typedef _stream_info AltsoundStreamInfo;
typedef std::array<AltsoundStreamInfo*, ALT_MAX_CHANNELS> StreamArray;

// Structure to hold information about active streams
struct _stream_info {
	unsigned int hstream = 0;
	unsigned int hsync = 0;
	enum AltsoundSampleType stream_type = static_cast<AltsoundSampleType>(0);
	unsigned int channel_idx = 0;
	std::string sample_path;
	unsigned int ducking_profile = 0;
	float ducking = 1.0f;
	bool stop_music = false;
	bool loop = false;
	float gain = 1.0f;
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

// Structure for storing G-Sound ducking profiles
typedef struct _ducking_profile {
	float music_duck_vol = 1.0f;
	float callout_duck_vol = 1.0f;
	float sfx_duck_vol = 1.0f;
	float solo_duck_vol = 1.0f;
	float overlay_duck_vol = 1.0f;
} DuckingProfile;

// Structure for managing G-Sound sample type behavior
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

	float group_vol = 1.0f;

	std::unordered_map<std::string, _ducking_profile> ducking_profiles;

	// For ducking volume for supplied sample type in supplied profile ID
	float getDuckVolume(unsigned int profile_num, AltsoundSampleType type) const;

	// Debug helper to print contents of stored ducking profiles
	void printDuckingProfiles() const;

} BehaviorInfo;

// Structure for holding traditional AltSound sample data
typedef struct _altsound_sample_info {
	unsigned int id;
	int channel;
	float gain;
	float ducking;
	bool loop;
	bool stop;
	std::string name;
	std::string fname;
} AltsoundSampleInfo;

// DAR_TODO do we need "duck" here?
// Structure for holding G-Sound sample data
typedef struct _gsound_sample_info {
	unsigned int id = 0;
	std::string type = "";
	float duck = 1.0f;
	float gain = 1.0f;
	std::string fname = "";
	bool loop = false;
	unsigned int ducking_profile = 0;
} GSoundSampleInfo;

// ---------------------------------------------------------------------------
// Helper function prototypes
// ---------------------------------------------------------------------------

// translate BASS error codes to printable strings
const char* get_bass_err();  // function prototype

// translate AltsoundSampleType enum values to strings
const char* toString(AltsoundSampleType sampleType);

// tranlsate string representation of AltsoundSample to enum value
AltsoundSampleType toSampleType(const std::string& type_in);

// determine if the given path exists
bool dir_exists(const std::string& path_in);

// trim leading and trailing whitespace from string
std::string trim(const std::string& str);

// convert string to lowercase
std::string toLowerCase(const std::string& str);

#endif // ALTSOUND_DATA_H