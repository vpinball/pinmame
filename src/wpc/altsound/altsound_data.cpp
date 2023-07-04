// ---------------------------------------------------------------------------
// altsound_data.cpp
// 06/23/23 - Dave Roscoe
//
// Holds global variables and structures in support of AltSound processing.
// This provides a cleaner separation between the legacy C code and the new C++
// code, and is necessary to ensure that only one instance of these exist
// across multiple compilation units
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "altsound_data.hpp"

// Standard Library includes
#include <cctype>
#include <map>
//#include <unordered_map>

// Local includes
//#include "..\ext\bass\bass.h"
#include "bass.h"

// Instance of global thread synchronization mutex
std::mutex io_mutex;

// Instance of global array of BASS channels 
StreamArray channel_stream;

float master_vol = 1.0f;
float global_vol = 1.0f;

// Data structure to hold sample behaviors
BehaviorInfo music_behavior;
BehaviorInfo callout_behavior;
BehaviorInfo sfx_behavior;
BehaviorInfo solo_behavior;
BehaviorInfo overlay_behavior;

// namespace resolution
using std::string;

// ---------------------------------------------------------------------------
// Helper function to translate AltsoundSample type constants to strings
// ---------------------------------------------------------------------------

std::string toString(AltsoundSampleType sampleType) {
	switch (sampleType) {
	case UNDEFINED: return "UNDEFINED";
	case MUSIC:     return "MUSIC";
	case JINGLE:    return "JINGLE";
	case SFX:       return "SFX";
	case CALLOUT:   return "CALLOUT";
	case SOLO:      return "SOLO";
	case OVERLAY:   return "OVERLAY";
	default:        return "UNKNOWN";
	}
}

// ---------------------------------------------------------------------------
// Helper function to translate string reprsentation of AltsoundSampleType to
// enum value
// ---------------------------------------------------------------------------

AltsoundSampleType toSampleType(const std::string& type_in)
{
	static const std::map<std::string, AltsoundSampleType> typeMap{
			  {"UNDEFINED", AltsoundSampleType::UNDEFINED},
			  {"MUSIC", AltsoundSampleType::MUSIC},
			  {"JINGLE", AltsoundSampleType::JINGLE},
			  {"SFX", AltsoundSampleType::SFX},
			  {"CALLOUT", AltsoundSampleType::CALLOUT},
			  {"SOLO", AltsoundSampleType::SOLO},
			  {"OVERLAY", AltsoundSampleType::OVERLAY}
	};

	string str = toUpper(type_in);

	auto it = typeMap.find(str);
	if (it != typeMap.end()) {
		return it->second;
	}
	else {
		return AltsoundSampleType::UNDEFINED; // Indicate an error case
	}
}

// ---------------------------------------------------------------------------
// Helper function to translate BASS error codes to printable strings
// ---------------------------------------------------------------------------

const char* get_bass_err()
{
	// BASS error code->string mapping
	static const char* bass_err_names[] = {
		"BASS_OK",
		"BASS_ERROR_MEM",
		"BASS_ERROR_FILEOPEN",
		"BASS_ERROR_DRIVER",
		"BASS_ERROR_BUFLOST",
		"BASS_ERROR_HANDLE",
		"BASS_ERROR_FORMAT",
		"BASS_ERROR_POSITION",
		"BASS_ERROR_INIT",
		"BASS_ERROR_START",
		"BASS_ERROR_SSL",
		"BASS_ERROR_REINIT",
		"BASS_ERROR_ALREADY",
		"BASS_ERROR_NOTAUDIO",
		"BASS_ERROR_NOCHAN",
		"BASS_ERROR_ILLTYPE",
		"BASS_ERROR_ILLPARAM",
		"BASS_ERROR_NO3D",
		"BASS_ERROR_NOEAX",
		"BASS_ERROR_DEVICE",
		"BASS_ERROR_NOPLAY",
		"BASS_ERROR_FREQ",
		"BASS_ERROR_NOTFILE",
		"BASS_ERROR_NOHW",
		"BASS_ERROR_EMPTY",
		"BASS_ERROR_NONET",
		"BASS_ERROR_CREATE",
		"BASS_ERROR_NOFX",
		"BASS_ERROR_NOTAVAIL",
		"BASS_ERROR_DECODE",
		"BASS_ERROR_DX",
		"BASS_ERROR_TIMEOUT",
		"BASS_ERROR_FILEFORM",
		"BASS_ERROR_SPEAKER",
		"BASS_ERROR_VERSION",
		"BASS_ERROR_CODEC",
		"BASS_ERROR_ENDED",
		"BASS_ERROR_BUSY",
		"BASS_ERROR_UNSTREAMABLE",
		"BASS_ERROR_PROTOCOL",
		"BASS_ERROR_DENIED"
		//   [BASS_ERROR_UNKNOWN]      = ""
	};

	// returns string representation of BASS error codes
	int err = BASS_ErrorGetCode();
	if (err < 0) {
		return "BASS_ERROR_UNKNOWN";
	}
	else {
		return bass_err_names[err];
	}
}

// ----------------------------------------------------------------------------
// Helper function to convert a string to uppercase
// ----------------------------------------------------------------------------

std::string toUpper(const std::string& string_in)
{
	std::string result = string_in;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
		return std::toupper(c);
	});

	return result;
}

// ----------------------------------------------------------------------------
// Helper function to convert a string to lowercase
// ----------------------------------------------------------------------------

std::string toLower(const std::string& string_in)
{
	std::string result = string_in;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
		return std::tolower(c);
	});

	return result;
}