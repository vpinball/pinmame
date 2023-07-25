#include "altsound_ini_processor.hpp"

// Std Library includes
#include <algorithm>
#include <fstream>
#include <string>

// Local includes
//#include "altsound_data.hpp"
#include "altsound_logger.hpp"

// namespace resolution
using std::string;

extern AltsoundLogger alog;
extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

bool AltsoundIniProcessor::parse_altsound_ini(const string& path_in, string& format_out,
	                                           bool& rec_cmds_out, bool& rom_ctrl_out)
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

	// get sound command playback
	string record_sound_cmds;
	inipp::get_value(ini.sections["system"], "record_sound_cmds", record_sound_cmds);
	ALT_INFO(0, "Parsed \"record_sound_cmds\": %s", record_sound_cmds.c_str());
	rec_cmds_out = (record_sound_cmds == "1");

	// get format
	string format;
	inipp::get_value(ini.sections["format"], "format", format);
	std::transform(format.begin(), format.end(), format.begin(), ::tolower);
	ALT_INFO(0, "Parsed \"format\": %s", format.c_str());
	format_out = format;

	// get ROM volume control
	string rom_control;
	inipp::get_value(ini.sections["volume"], "rom_ctrl", rom_control);
	ALT_INFO(0, "Parsed \"rom_ctrl\": %s", rom_control.c_str());
	rom_ctrl_out = (rom_control == "1");

	// parse LOGGING_LEVEL
	string logging;
	inipp::get_value(ini.sections["logging"], "logging_level", logging);
	ALT_INFO(0, "Parsed \"logging_level\": %s", logging.c_str());
	AltsoundLogger::Level level = alog.toLogLevel(logging);
	if (level == AltsoundLogger::UNDEFINED) {
		ALT_ERROR(0, "Unknown log level: %s. Defaulting to Error logging", logging);
		alog.setLogLevel(AltsoundLogger::Level::Error);
	}
	else {
		alog.setLogLevel(level);
	}

	using BB = BehaviorInfo::BehaviorBits;

	// parse MUSIC behavior
	auto& music_section = ini.sections["music"];

	bool success = true;

	// DAR@20230715
	// MUSIC behaviors are fixed. It does not duck / pause / stop other streams
	//
	// parse MUSIC behavior
	music_behavior.stops.set(static_cast<int>(BB::MUSIC), true); // MUSIC stops other MUSIC streams
	success &= parseVolumeValue(music_section, "group_vol", music_behavior.group_vol);

	// parse MUSIC ducking profiles
	auto& music_ducking_section = ini.sections["music_ducking_profiles"];

	if (!music_ducking_section.empty()) {
		success &= parseDuckingProfile(music_ducking_section, music_behavior.ducking_profiles);
	}

	// parse CALLOUT behavior
	auto& callout_section = ini.sections["callout"];

		// CALLOUT streams stop other CALLOUT streams	
		callout_behavior.stops.set(static_cast<int>(BB::CALLOUT), true); // CALLOUT stops other CALLOUT streams
	
		success &= parseBehaviorValue(callout_section, "ducks", callout_behavior.ducks);
		success &= parseBehaviorValue(callout_section, "pauses", callout_behavior.pauses);
		success &= parseBehaviorValue(callout_section, "stops", callout_behavior.stops);
		success &= parseVolumeValue(callout_section, "group_vol", callout_behavior.group_vol);

	// parse CALLOUT ducking profiles
	auto& callout_ducking_section = ini.sections["callout_ducking_profiles"];
	if (!callout_ducking_section.empty()) {
		success &= parseDuckingProfile(callout_ducking_section, callout_behavior.ducking_profiles);
	}

	// parse SFX behavior
	auto& sfx_section = ini.sections["sfx"];

		success &= parseBehaviorValue(sfx_section, "ducks", sfx_behavior.ducks);
		success &= parseVolumeValue(sfx_section, "group_vol", sfx_behavior.group_vol);

	// parse SFX ducking profiles
	auto& sfx_ducking_section = ini.sections["sfx_ducking_profiles"];
	if (!sfx_ducking_section.empty()) {
		success &= parseDuckingProfile(sfx_ducking_section, sfx_behavior.ducking_profiles);
	}

	// parse SOLO behavior
	auto& solo_section = ini.sections["solo"];

		// SOLO streams stop other SOLO streams
		solo_behavior.stops.set(static_cast<int>(BB::SOLO), true); // SOLO stops other SOLO streams
		
		success &= parseBehaviorValue(solo_section, "stops", solo_behavior.stops);
		success &= parseVolumeValue(solo_section, "group_vol", solo_behavior.group_vol);

	// parse SOLO ducking profiles
	auto& solo_ducking_section = ini.sections["solo_ducking_profiles"];
	if (!solo_ducking_section.empty()) {
		success &= parseDuckingProfile(solo_ducking_section, solo_behavior.ducking_profiles);
	}

	// parse OVERLAY behavior
	auto& overlay_section = ini.sections["overlay"];

		// OVERLAY streams stop other OVERLAY streams
		overlay_behavior.stops.set(static_cast<int>(BB::OVERLAY), true); // OVERLAY stops other OVERLAY streams

		success &= parseBehaviorValue(overlay_section, "ducks", overlay_behavior.ducks);
		success &= parseVolumeValue(overlay_section, "group_vol", overlay_behavior.group_vol);

	// parse OVERLAY ducking profiles
	auto& overlay_ducking_section = ini.sections["overlay_ducking_profiles"];
	if (!overlay_ducking_section.empty()) {
		success &= parseDuckingProfile(overlay_ducking_section, overlay_behavior.ducking_profiles);
	}

	OUTDENT;
	ALT_DEBUG(0, "END parse_altsound_ini()");
	return success;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound behavior values
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseBehaviorValue(const IniSection& section, const std::string& key, std::bitset<5>& behavior)
{
	std::string token;
	std::string parsed_value;
	inipp::get_value(section, key, parsed_value);

	std::stringstream ss(parsed_value);
	while (std::getline(ss, token, ',')) {
		token = trim(token);
		std::transform(token.begin(), token.end(), token.begin(), ::tolower);

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
	inipp::get_value(section, key, parsed_value);

	if (!parsed_value.empty()) {
		try {
			int val = std::stoul(parsed_value);
			volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : (float)val / 100.f;
		}
		catch (const std::exception& e) {
			ALT_ERROR(0, "Exception while parsing volume value: %s\n", e.what());
			return false;
		}
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

	// Iterate over the key-value pairs in the calloutDuckingSection
	for (const auto& pair : ducking_section) {
		const inipp::Ini<char>::String& key = pair.first; // profile name
		const inipp::Ini<char>::String& value = pair.second; // profile value string

		// Check if the key starts with "profile" to identify the profiles
		std::string subkey_test = "DUCKING_PROFILE";
		std::string subkey = key.substr(0, subkey_test.size());
		std::transform(subkey.begin(), subkey.end(), subkey.begin(), ::toupper);

		if (subkey == subkey_test) {
			// Extract the profile number from the key
			//DAR_TODO catch exception
			int profile_number = std::stoi(key.substr(subkey.size()));

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
				//DAR_TODO what if there's no number, or whitespace after ':'?
				std::getline(tokenStream, label, ':');
				label = trim(label); // remove whitespace
				std::transform(label.begin(), label.end(), label.begin(), ::tolower); // convert to lowercase
				tokenStream >> val;

				float volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : (float)val / 100.f;

				/*float music_duck_vol = 1.0f;
				float callout_duck_vol = 1.0f;
				float sfx_duck_vol = 1.0f;
				float solo_duck_vol = 1.0f;
				float overlay_duck_vol = 1.0f;*/
				// Set the corresponding volume based on the label
				// DAR_TODO make case-insensitive
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
					ALT_ERROR(1, "Unknown sample type label: %s", label);

					OUTDENT;
					ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				}

				// Store the parsed profile in the ducking_profiles map
				std::string profileKey = "profile" + std::to_string(profile_number);
				profiles[profileKey] = profile;
			}
		}
		else {
			ALT_ERROR(1, "Failed to parse ini file");

			OUTDENT;
			ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
			return false;
		}
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
string AltsoundIniProcessor::get_altound_format(const string& path_in)
{
	ALT_DEBUG(0, "BEGIN get_altsound_format()");
	INDENT;

	bool using_altsound = false;
	bool using_gsound = false;
	bool using_pinsound = false;

	// check for gsound
	string path1 = path_in;
	path1.append("\\g-sound.csv");

	std::ifstream ini(path1.c_str());
	if (ini.good()) {
		ALT_INFO(0, "Using G-Sound format");
		using_gsound = true;
		ini.close();
	}
	else {
		// checl for traditional altsound
		string path2 = path_in;
		path2.append("\\altsound.csv");

		std::ifstream ini(path2.c_str());
		if (ini.good()) {
			ALT_INFO(0, "Using traditional AltSound format");
			using_altsound = true;
			ini.close();
		}
	}

	if (!using_altsound && !using_gsound) {
		// check for PinSound
		string path3 = path_in;
		string path4 = path_in;
		string path5 = path_in;
		string path6 = path_in;

		path3.append("\\jingle");
		path4.append("\\music");
		path5.append("\\sfx");
		path6.append("\\voice");

		// DAR@20230617
		// I don't know if all of these will be included in every altsound
		// package, so if even one is found, it will assume that the PinSound
		// format is to be used
		using_pinsound = dir_exists(path3) ? true : dir_exists(path4) ? true : \
			dir_exists(path5) ? true : dir_exists(path6) ? true : false;

		if (using_pinsound)
			ALT_INFO(0, "Using PinSound format");
	}

	return using_gsound ? "g-sound" : using_altsound ? "altsound" : using_pinsound ? "pinsound" : "";

	OUTDENT;
	ALT_DEBUG(0, "END get_altsound_format()");
}

// ---------------------------------------------------------------------------
// Helper function to create the altsound.ini file
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::create_altsound_ini(const std::string& path_in)
{
	ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::create_altsound_ini()");
	INDENT;

	std::string format = get_altound_format(path_in);

	if (format.empty()) {
		ALT_ERROR(0, "FAILED AltsoundIniProcessor::get_altsound_format()");

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
		return false;
	}
	ALT_INFO(0, "SUCCESS AltsoundIniProcessor::get_altsound_format(): %s", format.c_str());

	std::string config_str =
		";-----------------------------------------------------------------------------\n"
		"; altsound.ini - Configuration file for all AltSound formats\n"
		";\n"
		"; If this file does not already exist, it is created automatically the first\n"
		"; time a table configured for AltSound is launched\n"
		"; ----------------------------------------------------------------------------\n"
		"; record_sound_cmds : records all received sound commands and relative\n"
		";                     playback times.  This can be useful for testing altsound\n"
		";                     sample combinations without having to recreate them on\n"
		";                     the table.  The file can also be edited or created by hand\n"
		";                     to create custom testing scenarios. The\n"
		";                     \"<gamename>_cmdlog.txt\" file is created in the \"tables\"\n"
		";                     folder.  This feature is turned off by default\n"
		"; ----------------------------------------------------------------------------\n"
		"[system]\n"
		"record_sound_cmds = 0\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; There are three supported AltSound formats :\n"
		";  1. Legacy\n"
		";  2. AltSound\n"
		";  3. G-Sound\n"
		";\n"
		"; Legacy  : the original AltSound format that parses a file / folder structure\n"
		";           similar to the PinSound system. It is no longer used for new\n"
		";           AltSound packages\n"
		";\n"
		"; AltSound: a CSV-based format designed as a replacement for the PinSound\n"
		";           format. This format defines samples according to \"channels\" with\n"
		";           loosely defined behaviors controlled by the associated metadata\n"
		";           \"gain\", \"ducking\", and \"stop\" fields. This is the format\n"
		";           currently in use by most AltSound authors\n"
		";\n"
		"; G-Sound : a new CSV-based format that defines samples according to\n"
		";           contextual types, allowing for more intuitively designed AltSound\n"
		";           packages. General playback behavior is dictated by the assigned\n"
		";           type. Behaviors can evolve without the need for adding\n"
		";           additional CSV fields, and the combinatorial complexity that\n"
		";           comes with it.\n"
		";           NOTE: This option requires the new g-sound.csv format\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[format]\n"
		"format = " + format + "\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; The AltSound processor attempts to recreate original playback behavior based\n"
		"; on commands sent from the ROM. This does not appear to be working in all\n"
		"; cases, resulting in undesirable muting of the sample playback volume.\n"
		"; Until this can be addressed, this feature can be disabled by setting\n"
		"; the \"rom_control\" setting to false.\n"
		";\n"
		"; Turning this feature off will use only the defined \"gain\" and \"ducking\"\n"
		"; values defined in the CSV file, and will not attempt to replicate ROM\n"
		"; behavior for sample playback.\n"
		";\n"
		"; To preserve current functionality, this feature is enabled by default\n"
		"; NOTE: This option works with all AltSound formats listed above\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[volume]\n"
		"rom_ctrl = 0\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; To facilitate troubleshooting, the AltSound processor logs events to the\n"
		"; \"altsound.log\" file, which can be found in the \"tables\" folder.  There\n"
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
		"; ----------------------------------------------------------------------------\n\n"

		"[logging]\n"
		"logging_level = Error\n\n"

		"; ----------------------------------------------------------------------------\n"
		"; The section below allows for tailoring of the G-Sound behaviors. They are\n"
		"; not used for traditional AltSound or Legacy Altsound formats\n"
		";\n"
		"; The G-Sound format supports the following sample types:\n"
		";\n"
		"; - MUSIC   : background music.  Only one can play at a time\n"
		"; - CALLOUT : voice interludes and callouts.  Only one can play at a time\n"
		"; - SFX     : short sounds to supplement table sounds.  Multiple can play at a time\n"
		"; - SOLO    : sound played at end-of-ball/game, or tilt.  Only one can play at a time\n"
		"; - OVERLAY : sounds played over music/sfx.  Only one can play at a time\n"
		";\n"
		"; G-Sound sample types have built-in behaviors.  Some can be user-modified.\n"
		"; Where available, the adjustable variables are:\n"
		";\n"
		"; ducks            : specify which sample types are ducked\n"
		"; pauses           : specify which sample types are paused\n"
		"; stops            : specify which sample types are stopped\n"
		"; group_vol        : relative group volume for sample type\n"
		"; ducking_profile  : relative ducking volumes for specified sample types\n"
		";\n"
		"; NOTES\n"
		"; - a sample type cannot duck/pause another sample of the same type\n"
		"; - a stopped sample cannot be resumed/restarted\n"
		"; - ducking values are specified as a percentage of the gain of the\n"
		";   affected sample type(s). Values range from 0 to 100 where\n"
		";   0 completely mutes the sample, and 100 effectively negates ducking\n"
		"; - if multiple ducking values apply to a single sample, the lowest\n"
		";   ducking value is used\n"
		"; - ducking/pausing ends when the sample that set it has ended\n"
		"; - If multiple sample types duck/pause another type, playback will remain\n"
		";   ducked/paused until the last affecting sample has ended\n"
		"; ----------------------------------------------------------------------------\n\n"

		"[music]\n"
		"group_vol = 100\n\n"

		"[callout]\n"
		"ducks = sfx, music, overlay\n"
		"pauses =\n"
		"stops =\n"
		"group_vol = 100\n\n"

		"[sfx]\n"
		"ducks =\n"
		"group_vol = 100\n\n"

		"[solo]\n"
		"stops = music, overlay, callout\n"
		"group_vol = 100\n\n"

		"[overlay]\n"
		"ducks = music, sfx\n"
		"group_vol = 100\n\n"

		"[callout_ducking_profiles]\n"
		"; profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:50, overlay:50\n\n"

		"[overlay_ducking_profiles]\n"
		"; profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:50\n"
		"ducking_profile2 = sfx:80, music:65";

	std::string ini_path = path_in + "\\altsound.ini";
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


