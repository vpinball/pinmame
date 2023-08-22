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

// Local includes
#include "bass.h"
#include "altsound_logger.hpp"

// namespace resolution
using std::string;

// ----------------------------------------------------------------------------
// Logging support
// ----------------------------------------------------------------------------

extern AltsoundLogger alog;  // external global logger instance

// ----------------------------------------------------------------------------
// Helper function to print the ducking profiles in a _behavior_info structure
// ----------------------------------------------------------------------------

void _behavior_info::printDuckingProfiles() const {
	for (const auto& profile : ducking_profiles) {
		const DuckingProfile& dp = profile.second;
		ALT_DEBUG(0, "Ducking %s, music_duck_vol: %f, callout_duck_vol: %f, sfx_duck_vol: %f, solo_duck_vol: %f, overlay_duck_vol: %f",
			profile.first.c_str(), dp.music_duck_vol, dp.callout_duck_vol, dp.sfx_duck_vol, dp.solo_duck_vol, dp.overlay_duck_vol);
	}
}

// ---------------------------------------------------------------------------
// Helper function to find ducking_profiles stored in the _behavior_info
// structure
// ---------------------------------------------------------------------------

float _behavior_info::getDuckVolume(unsigned int profile_num, AltsoundSampleType type) const
{
	std::string profile_key = "profile" + std::to_string(profile_num);

	auto it = ducking_profiles.find(profile_key);
	if (it != ducking_profiles.end()) {
		const _ducking_profile& profile = it->second;

		switch (type) {
		case AltsoundSampleType::MUSIC:
			return profile.music_duck_vol;
		case AltsoundSampleType::CALLOUT:
			return profile.callout_duck_vol;
		case AltsoundSampleType::SFX:
			return profile.sfx_duck_vol;
		case AltsoundSampleType::SOLO:
			return profile.solo_duck_vol;
		case AltsoundSampleType::OVERLAY:
			return profile.overlay_duck_vol;
		default:
			return 1.0f;
		}
	}
	else {
		ALT_ERROR(0, "Ducking Profile %s not found.  Using default", profile_key.c_str());
	}

	return 1.0f;
}

// ---------------------------------------------------------------------------
// Helper function to translate AltsoundSample type constants to strings
// ---------------------------------------------------------------------------

const char* toString(AltsoundSampleType sampleType) {
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

	string str = type_in;
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

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

// ---------------------------------------------------------------------------
// Helper function to check if a directory exists
// ---------------------------------------------------------------------------

bool dir_exists(const std::string& path_in)
{
	ALT_DEBUG(0, "BEGIN dir_exists()");
	INDENT;

	struct stat info;

	if (stat(path_in.c_str(), &info) != 0) {
		ALT_INFO(0, "Directory: %s does not exist", path_in.c_str());
		ALT_DEBUG(0, "END dir_exists()");
		return false;
	}
	ALT_INFO(0, "Directory: %s exists", path_in.c_str());

	OUTDENT;
	ALT_INFO(0, "END dir_exists()");
	return (info.st_mode & S_IFDIR) != 0;
}

// ----------------------------------------------------------------------------
// Helper function to trim whitespace from parsed tokens
// ----------------------------------------------------------------------------

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

// ----------------------------------------------------------------------------
// Helper function to convert strings to lowercase
// ----------------------------------------------------------------------------

std::string toLowerCase(const std::string& str)
{
	std::string lowerCaseStr;
	std::transform(str.begin(), str.end(), std::back_inserter(lowerCaseStr), ::tolower);
	return lowerCaseStr;
}