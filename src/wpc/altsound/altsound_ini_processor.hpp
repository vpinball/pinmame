// ---------------------------------------------------------------------------
// altsound_ini_processor.hpp
//
// Parser for AltSound configuration file
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#ifndef ALTSOUND_INI_PROCESSOR_H
#define ALTSOUND_INI_PROCESSOR_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

// Std Library includes
#include <assert.h>
#include <bitset>
#include <string>

// Local includes
#include "altsound_data.hpp"
#include "inipp.h"

class AltsoundIniProcessor
{
public:

	// syntactic candy
	using IniSection = std::map<inipp::Ini<char>::String, inipp::Ini<char>::String>;
	using ProfileMap = std::unordered_map<std::string, DuckingProfile>;

	// Default constructor
	AltsoundIniProcessor() = default;

	// Default destructor
	~AltsoundIniProcessor() = default;

	// Copy constructor - NOT USED
	AltsoundIniProcessor(AltsoundIniProcessor&) = delete;

	// Parse the altsound ini file
	bool parse_altsound_ini(const std::string& path_in);

	// Return parsed flag indicating whether to enable sound command recording
	const bool recordSoundCmds() const;

	// Return parsed AltsoundFormat type
	const std::string& getAltsoundFormat() const;

	// Return parsed ROM volume control flag
	const bool usingRomVolumeControl() const;

	// Return parsed skip count value
	const unsigned int getSkipCount() const;

private: // functions

	// helper function to parse behavior variable values
	bool parseBehaviorValue(const IniSection& section, const std::string& key, std::bitset<5>& behavior);

	// helper function to parse behavior volume values
	bool parseVolumeValue(const IniSection& section, const std::string& key, float& volume);

	// helper function to parse ducking profiles
	bool parseDuckingProfile(const IniSection& ducking_section, ProfileMap& profiles);

	// determine altsound format from installed data
	std::string get_altsound_format(const std::string& path_in);

	// Create altsound.ini file
	bool create_altsound_ini(const std::string& path_in);

	// Helper function to trim whitespace from strings and conver to lowercase
	std::string normalizeString(std::string str);

	// Helper template class to replicate C++17 function
	template<class T>
	const T& clamp(const T& v, const T& lo, const T& hi)
	{
		assert(!(hi < lo));
		return (v < lo) ? lo : (hi < v) ? hi : v;
	}

private: // data

	bool record_sound_commands = false;
	bool rom_volume_control = true;
	std::string altsound_format;
	unsigned int skip_count = 0;
};

// ----------------------------------------------------------------------------
// Inline functions
// ----------------------------------------------------------------------------

inline const bool AltsoundIniProcessor::recordSoundCmds() const {
	return record_sound_commands;
}

// ----------------------------------------------------------------------------

inline const std::string& AltsoundIniProcessor::getAltsoundFormat() const {
	return altsound_format;
}


// ----------------------------------------------------------------------------

inline const bool AltsoundIniProcessor::usingRomVolumeControl() const {
	return rom_volume_control;
}

// ----------------------------------------------------------------------------

inline const unsigned int AltsoundIniProcessor::getSkipCount() const {
	return skip_count;
}

#endif // ALTSOUND_INI_PROCESSOR_H
