// ---------------------------------------------------------------------------
// altsound_ini_processor.cpp
//
// Parser for AltSound configuration file
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#include "altsound_ini_processor.hpp"

// Std Library includes
#include <algorithm>
#include <fstream>
#include <string>

// Local includes
#include "altsound_logger.hpp"

// namespace resolution
using std::string;

// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------

// reference to global AltSound logger
extern AltsoundLogger alog;

// references to global G-Sound behaviors
extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

// ----------------------------------------------------------------------------
// Functional code
// ----------------------------------------------------------------------------

bool AltsoundIniProcessor::parse_altsound_ini(const string& path_in)
{
	ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::parse_altsound_ini()");
	INDENT;

	// if altsound.ini does not exist, create it
	string ini_path = path_in;
	ini_path.append("\\altsound.ini");

	std::ifstream file_in(ini_path);
	if (!file_in.good()) {
		ALT_INFO(0, "\"altsound.ini\" not found. Creating it.");
		if (!create_altsound_ini(path_in)) {
			ALT_ERROR(0, "FAILED AltsoundIniProcessor::create_ini_file()");

			OUTDENT;
			ALT_DEBUG(0, "END AltsoundIniProcessor::parse_altsound_ini()");
			return false;
		}
		ALT_INFO(0, "SUCCESS AltsoundIniProcessor::create_ini_file()");

		// .ini file is created, open it
		file_in.open(ini_path);
	}

	if (!file_in.good()) {
		ALT_ERROR(0, "Failed to open \"altsound.ini\"");

		OUTDENT;
		ALT_DEBUG(0, "END parse_altsound_ini()");
		return false;
	}

	// parse ini file
	inipp::Ini<char> ini;
	ini.parse(file_in);

	// ------------------------------------------------------------------------
	// System parsing
	// ------------------------------------------------------------------------

	// get sound command playback flag
	string record_sound_cmds;
	inipp::get_value(ini.sections["system"], "record_sound_cmds", record_sound_cmds);
	record_sound_commands = (record_sound_cmds == "1");
	ALT_INFO(0, "Parsed \"record_sound_cmds\": %s", (record_sound_commands ? "true" : "false"));

	// get ROM volume control flag
	string rom_control;
	inipp::get_value(ini.sections["system"], "rom_volume_ctrl", rom_control);
	rom_volume_control = (rom_control == "1");
	ALT_INFO(0, "Parsed \"rom_volume_ctrl\": %s", rom_volume_control ? "true" : "false");

	// get skip count
	string skip_count_str;
	inipp::get_value(ini.sections["system"], "cmd_skip_count", skip_count_str);
	try {
		if (!skip_count_str.empty()) {
			const int val = std::stoi(skip_count_str);
			skip_count = val;
			ALT_INFO(0, "Parsed \"cmd_skip_count\": %d", skip_count);
		}
	}
	catch (const std::invalid_argument& /*e*/) {
		ALT_ERROR(0, "Invalid number format while parsing cmd_skip_count value: %s\n", skip_count_str.c_str());
		return false;
	}
	catch (const std::out_of_range& /*e*/) {
		ALT_ERROR(0, "Number out of range while parsing cmd_skip_count value: %s\n", skip_count_str.c_str());
		return false;
	}

	// get AltSound format type
	inipp::get_value(ini.sections["format"], "format", altsound_format);
	altsound_format = normalizeString(altsound_format);
	ALT_INFO(0, "Parsed \"format\": %s", altsound_format.c_str());


	// ------------------------------------------------------------------------
	// Logging parsing
	// ------------------------------------------------------------------------

	// parse LOGGING_LEVEL
	string logging;
	inipp::get_value(ini.sections["logging"], "logging_level", logging);
	ALT_INFO(0, "Parsed \"logging_level\": %s", logging.c_str());
	const AltsoundLogger::Level level = alog.toLogLevel(logging);
	if (level == AltsoundLogger::UNDEFINED) {
		ALT_ERROR(0, "Unknown log level: %s. Defaulting to Error logging", logging.c_str());
		alog.setLogLevel(AltsoundLogger::Level::Error);
	}
	else {
		alog.setLogLevel(level);
	}

	// ------------------------------------------------------------------------
	// Behavior parsing
	// ------------------------------------------------------------------------

	using BB = BehaviorInfo::BehaviorBits;
	bool success = true;

	// ------------------------------------------------------------------------
	// MUSIC behavior parsing
	// ------------------------------------------------------------------------

	const auto& music_section = ini.sections["music"];

	// parse MUSIC "STOP" behavior
	// MUSIC streams only stop themselves
	music_behavior.stops.set(static_cast<int>(BB::MUSIC), true); // MUSIC stops other MUSIC streams
	success &= parseBehaviorValue(music_section, "stops", music_behavior.stops);

	// DAR@20230823 Leaving this commented code for now in case it becomes necessary
	// to activate this in the near future.  Will remove later, if not needed
	// parse MUSIC "DUCKS" behavior
	// MUSIC streams do not duck other streams
	//success &= parseBehaviorValue(music_section, "ducks", music_behavior.ducks);

	// parse MUSIC "MUSIC_DUCKING_PROFILES"
	// MUSIC streams do not duck other streams
	//auto& music_ducking_section = ini.sections["music_ducking_profiles"];
	//
	//if (!music_ducking_section.empty()) {
	//	success &= parseDuckingProfile(music_ducking_section, music_behavior.ducking_profiles);
	//}
	//else if (music_behavior.ducks != 0) {
	//	// MUSIC behavior specifies stream ducking but no profile defined.
	//	ALT_ERROR(1, "No ducking profiles defined for MUSIC behavior");
	//	success &= false;
	//}

	// parse MUSIC "PAUSES" behavior
	// MUSIC streams do not pause other streams
	// success &= parseBehaviorValue(music_section, "pauses", music_behavior.pauses);

	// parse MUSIC "GROUP_VOL" behavior
	success &= parseVolumeValue(music_section, "group_vol", music_behavior.group_vol);

	// ------------------------------------------------------------------------
	// CALLOUT behavior parsing
	// ------------------------------------------------------------------------

	auto& callout_section = ini.sections["callout"];

	// Parse CALLOUT "STOPS" behavior
	callout_behavior.stops.set(static_cast<int>(BB::CALLOUT), true); // CALLOUT stops other CALLOUT streams
	success &= parseBehaviorValue(callout_section, "stops", callout_behavior.stops);

	// Parse CALLOUT "DUCKS" behavior	
	success &= parseBehaviorValue(callout_section, "ducks", callout_behavior.ducks);

	// parse CALLOUT "CALLOUT_DUCKING_PROFILES"
	auto& callout_ducking_section = ini.sections["callout_ducking_profiles"];

	if (!callout_ducking_section.empty()) {
		success &= parseDuckingProfile(callout_ducking_section, callout_behavior.ducking_profiles);
	}
	else if (callout_behavior.ducks != 0) {
		// CALLOUT behavior specifies stream ducking but no profile defined.
		ALT_ERROR(1, "No ducking profiles defined for CALLOUT behavior");
		success &= false;
	}

	// Parse CALLOUT "PAUSES"  behavior
	success &= parseBehaviorValue(callout_section, "pauses", callout_behavior.pauses);

	// Parse CALLOUT "GROUP_VOL" behavior
	success &= parseVolumeValue(callout_section, "group_vol", callout_behavior.group_vol);

	// ------------------------------------------------------------------------
	// SFX behavior parsing
	// ------------------------------------------------------------------------

	const auto& sfx_section = ini.sections["sfx"];

	// Parse SFX "STOPS" behavior
	// SFX streams do not stop other streams
	//success &= parseBehaviorValue(sfx_section, "stops", sfx_behavior.stops);

	// Parse SFX "DUCKS" behavior
	success &= parseBehaviorValue(sfx_section, "ducks", sfx_behavior.ducks);

	// Parse SFX "SFX_DUCKING_PROFILES"
	const auto& sfx_ducking_section = ini.sections["sfx_ducking_profiles"];

	if (!sfx_ducking_section.empty()) {
		success &= parseDuckingProfile(sfx_ducking_section, sfx_behavior.ducking_profiles);
	}
	else if (sfx_behavior.ducks != 0) {
		// SFX behavior specifies stream ducking but no profile defined.
		ALT_ERROR(1, "No ducking profiles defined for SFX behavior");
		success &= false;
	}

	// Parse SFX "PAUSES" behavior
	// SFX streams do not pause other streams
	//success &= parseBehaviorValue(sfx_section, "pauses", sfx_behavior.pauses);

	// Parse SFX "GROUP_VOL" behavior
	success &= parseVolumeValue(sfx_section, "group_vol", sfx_behavior.group_vol);

	// ------------------------------------------------------------------------
	// SOLO behavior parsing
	// ------------------------------------------------------------------------

	const auto& solo_section = ini.sections["solo"];

	// Parse SOLO "STOPS" behavior
	solo_behavior.stops.set(static_cast<int>(BB::SOLO), true); // SOLO stops other SOLO streams
	success &= parseBehaviorValue(solo_section, "stops", solo_behavior.stops);

	// DAR@20230823 Leaving this commented code for now in case it becomes necessary
	// to activate this in the near future.  Will remove later, if not needed
	// Parse SOLO "DUCKS" behavior
	// SOLO streams do not duck other streams
	//success &= parseBehaviorValue(solo_section, "ducks", solo_behavior.ducks);

	// parse SOLO "SOLO_DUCKING_PROFILES"
	// SOLO streams do not duck other streams
	//auto& solo_ducking_section = ini.sections["solo_ducking_profiles"];
	//
	//if (!solo_ducking_section.empty()) {
	//  success &= parseDuckingProfile(solo_ducking_section, solo_behavior.ducking_profiles);
	//}
	//else if (solo_behavior.ducks != 0) {
	//	// SOLO behavior specifies stream ducking but no profile defined.
	//	ALT_ERROR(1, "No ducking profiles defined for SOLO behavior");
	//	success &= false;
	//}

	// Parse SOLO "PAUSES" behavior
	// SOLO streams do not pause other streams
	//success &= parseBehaviorValue(solo_section, "pauses", solo_behavior.pauses);

	// Parse SOLO "GROUP_VOL" behavior
	success &= parseVolumeValue(solo_section, "group_vol", solo_behavior.group_vol);

	// ------------------------------------------------------------------------
	// OVERLAY behavior parsing
	// ------------------------------------------------------------------------

	const auto& overlay_section = ini.sections["overlay"];

	// Parse OVERLAY "STOPS" behavior
	// OVERLAY streams only stop other OVERLAY streams
	overlay_behavior.stops.set(static_cast<int>(BB::OVERLAY), true); // OVERLAY stops other OVERLAY streams
	//success &= parseBehaviorValue(overlay_section, "stops", overlay_behavior.stops);

	// Parse OVERLAY "DUCKS" behavior
	success &= parseBehaviorValue(overlay_section, "ducks", overlay_behavior.ducks);

	// parse OVERLAY "OVERLAY_DUCKING_PROFILES"
	const auto& overlay_ducking_section = ini.sections["overlay_ducking_profiles"];

	if (!overlay_ducking_section.empty()) {
		success &= parseDuckingProfile(overlay_ducking_section, overlay_behavior.ducking_profiles);
	}
	else if (overlay_behavior.ducks != 0) {
			// OVERLAY behavior specifies stream ducking but no profile defined.
			ALT_ERROR(1, "No ducking profiles defined for OVERLAY behavior");
			success &= false;
	}

	// Parse OVERLAY "PAUSES" behavior
	// OVERLAY streams do not pause other streams
	//success &= parseBehaviorValue(overlay_section, "pauses", overlay_behavior.pauses);

	// Parse OVERLAY "GROUP_VOL" behavior
	success &= parseVolumeValue(overlay_section, "group_vol", overlay_behavior.group_vol);

	// ------------------------------------------------------------------------

	OUTDENT;
	ALT_DEBUG(0, "END parse_altsound_ini()");
	return success;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound behavior values
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseBehaviorValue(const IniSection& section,
	                                          const std::string& key,
	                                          std::bitset<5>& behavior)
{
	std::string token;
	std::string parsed_value;
	inipp::get_value(section, key, parsed_value);

	std::stringstream ss(parsed_value);
	while (std::getline(ss, token, ',')) {
		token = normalizeString(token);

		if (token == "music") {
			behavior.set(0, true);
		}
		else if (token == "callout") {
			behavior.set(1, true);
		}
		else if (token == "sfx") {
			behavior.set(2, true);
		}
		else if (token == "solo") {
			behavior.set(3, true);
		}
		else if (token == "overlay") {
			behavior.set(4, true);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound behavior volume values
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseVolumeValue(const IniSection& section, const std::string& key, float& volume)
{
	std::string parsed_value;
	if (!inipp::get_value(section, key, parsed_value)) {
		return true;
	}

	try {
		const int val = std::stoi(parsed_value);
		volume = (float)clamp(val, 0, 100) / 100.f;
	}
	catch (const std::invalid_argument& /*e*/) {
		ALT_ERROR(0, "Invalid number format while parsing volume value: %s\n", parsed_value.c_str());
		return false;
	}
	catch (const std::out_of_range& /*e*/) {
		ALT_ERROR(0, "Number out of range while parsing volume value: %s\n", parsed_value.c_str());
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound ducking profiles
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseDuckingProfile(const IniSection& ducking_section, ProfileMap& profiles)
{
	ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::parseDuckingProfile()");
	INDENT;

	// Iterate over the key-value pairs in the ducking_section
	for (const auto& pair : ducking_section) {
		const inipp::Ini<char>::String& key = pair.first; // profile name
		const inipp::Ini<char>::String& value = pair.second; // profile value string

		// Check if the key starts with "ducking_profile" to identify the profiles
		std::string subkey_test = "ducking_profile";
		std::string subkey = normalizeString(key.substr(0, subkey_test.size()));

		if (subkey != subkey_test) {
			ALT_ERROR(1, "Failed to parse ini file - unexpected key: %s", key.c_str());

			OUTDENT;
			ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
			return false;
		}

		// Extract the profile number from the key
		// Exception handling added to catch invalid stoi conversion
		int profile_number = 0;
		try {
			profile_number = std::stoi(key.substr(subkey.size()));
		}
		catch (std::invalid_argument& /*e*/) {
			ALT_ERROR(1, "Invalid profile number: %s", key.substr(subkey.size()).c_str());

			OUTDENT;
			ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
			return false;
		}

		// Parse the value to extract the individual tokens and volume values
		std::istringstream valueStream(value);
		std::string token;
		DuckingProfile profile;

		while (std::getline(valueStream, token, ',')) {
			// Extract the label and volume value
			std::istringstream tokenStream(token);
			std::string label;
			int val;

			// Split the token at the ':' delimiter
			if (!std::getline(tokenStream, label, ':')) {
				ALT_ERROR(1, "Failed to parse label: %s", token.c_str());

				OUTDENT;
				ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				return false;
			}

			label = normalizeString(label);
			if (!(tokenStream >> val)) {
				ALT_ERROR(1, "Failed to parse value: %s", token.c_str());

				OUTDENT;
				ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				return false;
			}

			const float volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : static_cast<float>(val) / 100.f;

			// Set the corresponding volume based on the label
			if (label == "music") {
				profile.music_duck_vol = volume;
			}
			else if (label == "callout") {
				profile.callout_duck_vol = volume;
			}
			else if (label == "sfx") {
				profile.sfx_duck_vol = volume;
			}
			else if (label == "solo") {
				profile.solo_duck_vol = volume;
			}
			else if (label == "overlay") {
				profile.overlay_duck_vol = volume;
			}
			else {
				ALT_ERROR(1, "Unknown sample type label: %s", label.c_str());

				OUTDENT;
				ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				return false;
			}
		}

		// Store the parsed profile in the ducking_profiles map
		std::string profileKey = "profile" + std::to_string(profile_number);
		profiles[profileKey] = profile;
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
	return true;
}


// ---------------------------------------------------------------------------
// Helper function to determine AltSound format
// ---------------------------------------------------------------------------

// DAR@20230623
// This is in support of altsound.ini file creation for packages that don't
// already have one (legacy).  It works in order of precedence:
//
//  1. presence of g-sound.csv
//  2. presence of altsound.csv
//	3. presence of PinSound directory structure
//
// Once the .ini file is created, it can be modified to adjust preference.
//
std::string AltsoundIniProcessor::get_altsound_format(const std::string& path_in)
{
	ALT_DEBUG(0, "BEGIN get_altsound_format()");
	INDENT;

	const std::vector<std::pair<std::string, std::string>> filesAndFormats{
		{ "\\g-sound.csv", "g-sound" },
		{ "\\altsound.csv", "altsound" },
	};

	for (const auto& fileAndFormat : filesAndFormats) {
		std::ifstream ini(path_in + fileAndFormat.first);
		if (ini) {
			ALT_INFO(0, ("Using " + fileAndFormat.second + " format").c_str());
			OUTDENT;
			ALT_DEBUG(0, "END get_altsound_format()");
			return fileAndFormat.second;
		}
	}

	const std::vector<std::string> directories{
		"\\jingle",
		"\\music",
		"\\sfx",
		"\\voice"
	};

	if (std::any_of(directories.begin(), directories.end(), [&](const auto& directory) {
		return dir_exists(path_in + directory);
	}))
	{
		ALT_INFO(0, "Using Legacy (PinSound) format");
		OUTDENT;
		ALT_DEBUG(0, "END get_altsound_format()");
		return "legacy";
	}


	OUTDENT;
	ALT_DEBUG(0, "END get_altsound_format()");
	return string();
}

// ----------------------------------------------------------------------------
// Helper function to trim whitespace and convert from string and convert to
// lowercase
// ---------------------------------------------------------------------------

std::string AltsoundIniProcessor::normalizeString(std::string str) {
	str = trim(str); // remove whitespace
	std::transform(str.begin(), str.end(), str.begin(), ::tolower); // convert to lowercase
	return str;
}

// Helper function to create the altsound.ini file
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::create_altsound_ini(const std::string& path_in)
{
	ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::create_altsound_ini()");
	INDENT;

	const std::string format = get_altsound_format(path_in);

	if (format.empty()) {
		ALT_ERROR(0, "FAILED AltsoundIniProcessor::get_altsound_format()");

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
		return false;
	}
	ALT_INFO(0, "SUCCESS AltsoundIniProcessor::get_altsound_format(): %s", format.c_str());

	const std::string config_str =
		"; ----------------------------------------------------------------------------\n"
		"; altsound.ini - Configuration file for all AltSound formats\n"
		";\n"
		"; If this file does not already exist, it is created automatically the first\n"
		"; time a table configured for AltSound is launched\n"
		"; ----------------------------------------------------------------------------\n"
		"; record_sound_cmds : records all received sound commands and relative\n"
		";                     playback times. This can be useful for testing altsound\n"
		";                     sample combinations without having to recreate them on\n"
		";                     the table. The file can also be edited or created by hand\n"
		";                     to create custom testing scenarios. The\n"
		";                     \"cmdlog.txt\" file is created in the \"tables\"\n"
		";                     folder. This feature is turned off by default\n"
		";\n"
		"; rom_volume_ctrl   : the AltSound processor attempts to recreate original\n"
		";                     playback behavior using commands sent from the ROM.\n"
		";                     This does not work in all cases, resulting in undesirable\n"
		";                     muting of the playback volume. Setting this variable\n"
		";                     to 0 turns this feature off.\n"
		";                     NOTE: This option works for all AltSound formats\n"
		";\n"
		"; cmd_skip_count    : some ROMs send out spurious commands during initialization\n"
		";                     which match valid runtime commands.  In this case, it is not\n"
		";                     desirable to output sound, but want to allow later instances\n"
		";                     to play normally.  This variable allows AltSound authors to\n"
		";                     specify how many initial commands to ignore at startup.\n"
		";                     NOTE:  If the record_sound_cmds flag is set, the skipped\n"
		";                     commands will be included in the recording file.\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[system]\n"
		"record_sound_cmds = 0\n"
		"rom_volume_ctrl = 1\n"
		"cmd_skip_count = 0\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; There are three supported AltSound formats:\n"
		";  1. Legacy\n"
		";  2. AltSound\n"
		";  3. G-Sound\n"
		";\n"
		"; Legacy   : the original AltSound format that parses a file/folder structure\n"
		";            similar to the PinSound system. It is no longer used for new\n"
		";            AltSound packages\n"
		";\n"
		"; AltSound : a CSV-based format designed as a replacement for the PinSound\n"
		";            format. This format defines samples according to \"channels\" with\n"
		";            loosely defined behaviors controlled by the associated metadata\n"
		";            \"gain\", \"ducking\", \"stop\", and \"loop\" fields. This is the format\n"
		";            currently used by most AltSound authors\n"
		";\n"
		"; G-Sound  : a new CSV-based format that defines samples according to\n"
		";            contextual types, allowing for more intuitively designed AltSound\n"
		";            packages. General playback behavior is dictated by the assigned\n"
		";            type. Behaviors can evolve without the need for adding\n"
		";            additional CSV fields, and the combinatorial complexity that\n"
		";            comes with it.\n"
		";            NOTE: This option requires the new g-sound.csv format\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[format]\n"
		"format = " + format + "\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; To facilitate troubleshooting, the AltSound processor logs events to the\n"
		"; \"altsound.log\" file, which can be found in the \"tables\" folder. There\n"
		"; are 4 logging levels:\n"
		"; 1. Info\n"
		"; 2. Error\n"
		"; 3. Warning\n"
		"; 4. Debug\n"
		";\n"
		"; Setting a logging level will also include lower-ordered logging as well.  For\n"
		"; example, setting logging level to \"Warning\" will also enable \"Info\" and\n"
		"; \"Error\" logging.  By default, logging is set to \"Error\" which will also\n"
		"; include \"Info\" log messages.  The log file will be overwritten each time a\n"
		"; new table is loaded.\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[logging]\n"
		"logging_level = Error\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; The section below allows for tailoring of the G-Sound behaviors.\n"
		";\n"
		"; The G-Sound format supports the following sample types:\n"
		";\n"
		"; - MUSIC   : background music. Only one can play at a time\n"
		"; - CALLOUT : voice interludes and callouts. Only one can play at a time\n"
		"; - SFX     : short sounds to supplement table sounds. Multiple can play at a time\n"
		"; - SOLO    : sound played at end-of-ball/game, or tilt. Only one can play at a time\n"
		"; - OVERLAY : sounds played over music/sfx. Only one can play at a time\n"
		";\n"
		"; G - Sound sample types have built-in behaviors. Some can be user-modified.\n"
		"; Where available, the adjustable variables are:\n"
		";\n"
		"; ducks           : specify which sample types are ducked\n"
		"; pauses          : specify which sample types are paused\n"
		"; stops           : specify which sample types are stopped\n"
		"; group_vol       : relative group volume for sample type\n"
		"; ducking_profile : relative ducking volumes for specified sample types\n"
		";\n"
		"; NOTES\n"
		"; - a sample type cannot duck/pause another sample of the same type\n"
		"; - a stopped sample cannot be resumed/restarted\n"
		"; - ducking values are specified as a percentage of the gain of the\n"
		";   affected sample type(s). Values range from 0 to 100 where\n"
		";   0 completely mutes the sample, and 100 effectively negates ducking\n"
		"; - if multiple ducking values apply to a single sample, the lowest\n"
		";   ducking value is used. When the sample with the lowest duck value ends,\n"
		";   the next lowest duck value is used, and so on, until all affecting samples\n"
		";   have ended.\n"
		"; - ducking/pausing ends when the last affecting sample that set it has ended\n"
		"; - If \"ducks\" variable is set, there must be at least one ducking_profile\n"
		";   defined\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[music]\n"
		"group_vol = 100\n"
		"\n"
		"[callout]\n"
		"ducks = sfx, music, overlay\n"
		"pauses =\n"
		"stops =\n"
		"group_vol = 100\n"
		"\n"
		"[callout_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:50, overlay:50\n"
		"\n"
		"[sfx]\n"
		"ducks = music\n"
		"group_vol = 100\n"
		"\n"
		"[sfx_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = music:50\n"
		"\n"
		"[solo]\n"
		"stops = music, overlay, callout\n"
		"group_vol = 100\n"
		"\n"
		"[overlay]\n"
		"ducks = music, sfx\n"
		"group_vol = 100\n"
		"\n"
		"[overlay_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:65\n"
		"ducking_profile2 = sfx:80, music:50\n";

	const std::string ini_path = path_in + "\\altsound.ini";
	std::ofstream file_out(ini_path);

	if (file_out.is_open()) {
		file_out << config_str;
		file_out.close();
	}
	else {
		ALT_ERROR(1, "Unable to open file: %s", ini_path.c_str());

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
	return true;
}
