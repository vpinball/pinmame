// ---------------------------------------------------------------------------
// altsound_data.cpp
//
// Holds global variables and structures in support of AltSound processing.
// This provides a cleaner separation between the legacy C code and the new C++
// code, and is necessary to ensure that only one instance of these exist
// across multiple compilation units
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders:Dave Roscoe
// ---------------------------------------------------------------------------
#include "altsound_data.hpp"

// Standard Library includes
#include <cctype>
#include <map>
#include <algorithm>

// Local includes
#include "miniaudio_bass_compat.hpp"
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
	const std::string profile_key = "profile" + std::to_string(profile_num);

	const auto it = ducking_profiles.find(profile_key);
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

	const auto it = typeMap.find(str);
	if (it != typeMap.end()) {
		return it->second;
	}
	else {
		return AltsoundSampleType::UNDEFINED; // Indicate an error case
	}
}

// ---------------------------------------------------------------------------
// Helper function to translate miniaudio error codes to printable strings
// ---------------------------------------------------------------------------

const char* get_miniaudio_err()
{
	// miniaudio error code->string mapping
	static const char* ma_err_names[] = {
		"MA_SUCCESS",
		"MA_ERROR",
		"MA_INVALID_ARGS",
		"MA_INVALID_OPERATION",
		"MA_OUT_OF_MEMORY",
		"MA_OUT_OF_RANGE",
		"MA_ACCESS_DENIED",
		"MA_DOES_NOT_EXIST",
		"MA_ALREADY_EXISTS",
		"MA_TOO_MANY_OPEN_FILES",
		"MA_INVALID_FILE",
		"MA_TOO_BIG",
		"MA_PATH_TOO_LONG",
		"MA_NAME_TOO_LONG",
		"MA_NOT_DIRECTORY",
		"MA_IS_DIRECTORY",
		"MA_DIRECTORY_NOT_EMPTY",
		"MA_AT_END",
		"MA_NO_SPACE",
		"MA_BUSY",
		"MA_IO_ERROR",
		"MA_INTERRUPT",
		"MA_UNAVAILABLE",
		"MA_ALREADY_IN_USE",
		"MA_BAD_ADDRESS",
		"MA_BAD_SEEK",
		"MA_BAD_PIPE",
		"MA_DEADLOCK",
		"MA_TOO_MANY_LINKS",
		"MA_NOT_IMPLEMENTED",
		"MA_NO_MESSAGE",
		"MA_BAD_MESSAGE",
		"MA_NO_DATA_AVAILABLE",
		"MA_INVALID_DATA",
		"MA_TIMEOUT",
		"MA_NO_NETWORK",
		"MA_NOT_UNIQUE",
		"MA_NOT_SOCKET",
		"MA_NO_ADDRESS",
		"MA_BAD_PROTOCOL",
		"MA_PROTOCOL_UNAVAILABLE",
		"MA_PROTOCOL_NOT_SUPPORTED",
		"MA_PROTOCOL_FAMILY_NOT_SUPPORTED",
		"MA_ADDRESS_FAMILY_NOT_SUPPORTED",
		"MA_SOCKET_NOT_SUPPORTED",
		"MA_CONNECTION_RESET",
		"MA_ALREADY_CONNECTED",
		"MA_NOT_CONNECTED",
		"MA_CONNECTION_REFUSED",
		"MA_NO_HOST",
		"MA_IN_PROGRESS",
		"MA_CANCELLED",
		"MA_MEMORY_ALREADY_MAPPED",
		"MA_FORMAT_NOT_SUPPORTED",
		"MA_DEVICE_TYPE_NOT_SUPPORTED",
		"MA_SHARE_MODE_NOT_SUPPORTED",
		"MA_NO_BACKEND",
		"MA_NO_DEVICE",
		"MA_API_NOT_FOUND",
		"MA_INVALID_DEVICE_CONFIG",
		"MA_LOOP",
		"MA_BACKEND_NOT_ENABLED"
	};

	// returns string representation of miniaudio error codes
	const int err = MiniAudio_ErrorGetCode();
	if (err < 0 || err >= (int)(sizeof(ma_err_names) / sizeof(ma_err_names[0]))) {
		return "MA_UNKNOWN_ERROR";
	}
	else {
		return ma_err_names[err];
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
	const size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	const size_t last = str.find_last_not_of(' ');
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
