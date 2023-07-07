#ifndef ALTSOUND_INI_PROCESSOR_H
#define ALTSOUND_INI_PROCESSOR_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Std Library includes
#include <bitset>
#include <string>

// Local includes
#include "inipp.h"

class AltsoundIniProcessor
{
public:
	// syntactic candy
	typedef std::map<inipp::Ini<char>::String, inipp::Ini<char>::String> IniSection;

	// Parse the altsound ini file
	bool parse_altsound_ini(const std::string& path_in, std::string& format_out, bool& rom_ctrl_out);

private: // functions
	
	bool parseBehaviorValue(const IniSection& section, const std::string& key, std::bitset<5>& behavior);
	bool parseVolumeValue(const IniSection& section, const std::string& key, float& volume);

	// determine altsound format from installed data
	std::string get_altound_format(const std::string& path_in);

	// Create altsound.ini file
	bool create_altsound_ini(const std::string& path_in);

private: // data

};

#endif // ALTSOUND_INI_PROCESSOR_H