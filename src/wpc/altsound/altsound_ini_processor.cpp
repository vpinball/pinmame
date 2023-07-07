#include "altsound_ini_processor.hpp"

// Std Library includes
#include <algorithm>
#include <fstream>
#include <string>

// Local includes
#include "altsound_data.hpp"
#include "altsound_logger.hpp"

// namespace resolution
using std::string;

extern AltsoundLogger alog;
extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

bool AltsoundIniProcessor::parse_altsound_ini(const string& path_in, string& format_out, bool& rom_ctrl_out)
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
	rom_ctrl_out = rom_control == "1";

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

	// parse MUSIC behavior
	auto& music_section = ini.sections["music"];

	bool success = true;

	success &= parseBehaviorValue(music_section, "ducks", music_behavior.ducks);
	success &= parseBehaviorValue(music_section, "pauses", music_behavior.pauses);
	success &= parseBehaviorValue(music_section, "stops", music_behavior.stops);

	success &= parseVolumeValue(music_section, "music_duck_vol", music_behavior.music_duck_vol);
	success &= parseVolumeValue(music_section, "callout_duck_vol", music_behavior.callout_duck_vol);
	success &= parseVolumeValue(music_section, "sfx_duck_vol", music_behavior.sfx_duck_vol);
	success &= parseVolumeValue(music_section, "solo_duck_vol", music_behavior.solo_duck_vol);
	success &= parseVolumeValue(music_section, "overlay_duck_vol", music_behavior.overlay_duck_vol);

	// parse CALLOUT behavior
	auto& callout_section = ini.sections["callout"];

	success &= parseBehaviorValue(callout_section, "ducks", callout_behavior.ducks);
	success &= parseBehaviorValue(callout_section, "pauses", callout_behavior.pauses);
	success &= parseBehaviorValue(callout_section, "stops", callout_behavior.stops);

	success &= parseVolumeValue(callout_section, "music_duck_vol", callout_behavior.music_duck_vol);
	success &= parseVolumeValue(callout_section, "callout_duck_vol", callout_behavior.callout_duck_vol);
	success &= parseVolumeValue(callout_section, "sfx_duck_vol", callout_behavior.sfx_duck_vol);
	success &= parseVolumeValue(callout_section, "solo_duck_vol", callout_behavior.solo_duck_vol);
	success &= parseVolumeValue(callout_section, "overlay_duck_vol", callout_behavior.overlay_duck_vol);

	// parse SFX behavior
	auto& sfx_section = ini.sections["sfx"];

	success &= parseBehaviorValue(sfx_section, "ducks", sfx_behavior.ducks);
	success &= parseBehaviorValue(sfx_section, "pauses", sfx_behavior.pauses);
	success &= parseBehaviorValue(sfx_section, "stops", sfx_behavior.stops);

	success &= parseVolumeValue(sfx_section, "music_duck_vol", sfx_behavior.music_duck_vol);
	success &= parseVolumeValue(sfx_section, "callout_duck_vol", sfx_behavior.callout_duck_vol);
	success &= parseVolumeValue(sfx_section, "sfx_duck_vol", sfx_behavior.sfx_duck_vol);
	success &= parseVolumeValue(sfx_section, "solo_duck_vol", sfx_behavior.solo_duck_vol);
	success &= parseVolumeValue(sfx_section, "overlay_duck_vol", sfx_behavior.overlay_duck_vol);

	// parse SOLO behavior
	auto& solo_section = ini.sections["solo"];

	success &= parseBehaviorValue(solo_section, "ducks", solo_behavior.ducks);
	success &= parseBehaviorValue(solo_section, "pauses", solo_behavior.pauses);
	success &= parseBehaviorValue(solo_section, "stops", solo_behavior.stops);

	success &= parseVolumeValue(solo_section, "music_duck_vol", solo_behavior.music_duck_vol);
	success &= parseVolumeValue(solo_section, "callout_duck_vol", solo_behavior.callout_duck_vol);
	success &= parseVolumeValue(solo_section, "sfx_duck_vol", solo_behavior.sfx_duck_vol);
	success &= parseVolumeValue(solo_section, "solo_duck_vol", solo_behavior.solo_duck_vol);
	success &= parseVolumeValue(solo_section, "overlay_duck_vol", solo_behavior.overlay_duck_vol);

	// parse OVERLAY behavior
	auto& overlay_section = ini.sections["overlay"];

	success &= parseBehaviorValue(overlay_section, "ducks", overlay_behavior.ducks);
	success &= parseBehaviorValue(overlay_section, "pauses", overlay_behavior.pauses);
	success &= parseBehaviorValue(overlay_section, "stops", overlay_behavior.stops);

	success &= parseVolumeValue(overlay_section, "music_duck_vol", overlay_behavior.music_duck_vol);
	success &= parseVolumeValue(overlay_section, "callout_duck_vol", overlay_behavior.callout_duck_vol);
	success &= parseVolumeValue(overlay_section, "sfx_duck_vol", overlay_behavior.sfx_duck_vol);
	success &= parseVolumeValue(overlay_section, "solo_duck_vol", overlay_behavior.solo_duck_vol);
	success &= parseVolumeValue(overlay_section, "overlay_duck_vol", overlay_behavior.overlay_duck_vol);

	OUTDENT;
	ALT_DEBUG(0, "END parse_altsound_ini()");
	return success;
}

// ---------------------------------------------------------------------------
// Helper function to parse Altsound2 behavior values
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseBehaviorValue(const IniSection& section, const std::string& key, std::bitset<5>& behavior)
{
	std::string token;
	std::string parsed_value;
	inipp::get_value(section, key, parsed_value);

	std::stringstream ss(parsed_value);
	while (std::getline(ss, token, ',')) {
		token = trim(token);
		//token = toLower(token);
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
// Helper function to parse Altsound2 behavior volume values
// ---------------------------------------------------------------------------

bool AltsoundIniProcessor::parseVolumeValue(const IniSection& section, const std::string& key, float& volume)
{
	std::string parsed_value;
	inipp::get_value(section, key, parsed_value);

	if (!parsed_value.empty()) {
		try {
			int val = std::stoul(parsed_value);
			volume = val > 100 ? 1.0f : val < 0 ? 0.0f : (float)val / 100.f;
		}
		catch (const std::exception& e) {
			ALT_ERROR(0, "Exception while parsing volume value: %s\n", e.what());
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to determine AltSound format
// ---------------------------------------------------------------------------

// DAR@20230623
// This is in support of altsound.ini file creation for packages that don't
// already have one (legacy).  It works in order of precedence:
//
//  1. presence of altsound2.csv
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
	bool using_altsound2 = false;
	bool using_pinsound = false;

	// check for altsound2
	string path1 = path_in;
	path1.append("\\altsound2.csv");

	std::ifstream ini(path1.c_str());
	if (ini.good()) {
		ALT_INFO(0, "Using AltSound2 format");
		using_altsound2 = true;
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

	if (!using_altsound && !using_altsound2) {
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

	return using_altsound2 ? "altsound2" : using_altsound ? "altsound" :
		using_pinsound ? "pinsound" : "";

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

	std::string ini_path = path_in + "\\altsound.ini";
	std::ofstream file_out(ini_path);

	file_out << "; ----------------------------------------------------------------------------\n";
	file_out << "; There are three supported AltSound formats :\n";
	file_out << ";  1. Legacy\n";
	file_out << ";  2. AltSound\n";
	file_out << ";  3. AltSound2\n";
	file_out << ";\n";
	file_out << "; Legacy: the original AltSound format that parses a file / folder structure\n";
	file_out << ";         similar to the PinSound system. It is no longer used for new\n";
	file_out << ";         AltSound packages\n";
	file_out << ";\n";
	file_out << "; AltSound: a CSV-based format designed as a replacement for the PinSound\n";
	file_out << ";           format. This format defines samples according to \"channels\" with\n";
	file_out << ";           loosely defined behaviors controlled by the associated metadata\n";
	file_out << ";           \"gain\", \"ducking\", and \"stop\" fields. This is the format\n";
	file_out << ";           currently in use by most AltSound authors\n";
	file_out << ";\n";
	file_out << "; Altsound2: a new CSV-based format that defines samples according to\n";
	file_out << ";            contextual types, allowing for more intuitively designed AltSound\n";
	file_out << ";            packages. General playback behavior is dictated by the assigned\n";
	file_out << ";            type. Behaviors can evolve without the need for adding\n";
	file_out << ";            additional CSV fields, and the combinatorial complexity that\n";
	file_out << ";            comes with it.\n";
	file_out << ";            NOTE: This option requires the new altsound2.csv format\n";
	file_out << ";\n";
	file_out << "; ----------------------------------------------------------------------------\n\n";

	// create [format] section
	file_out << "[format]\n";
	file_out << "format = " << format << "\n\n";

	file_out << "; ----------------------------------------------------------------------------\n";
	file_out << "; The AltSound processor attempts to recreate original playback behavior based\n";
	file_out << "; on commands sent from the ROM. This does not appear to be working in all\n";
	file_out << "; cases, resulting in undesirable muting of the sample playback volume.\n";
	file_out << "; Until this can be addressed, this feature can be disabled by setting\n";
	file_out << "; the \"rom_control\" setting to false.\n";
	file_out << ";\n";
	file_out << "; Turning this feature off will use only the defined \"gain\" and \"ducking\"\n";
	file_out << "; values defined in the CSV file, and will not attempt to replicate ROM\n";
	file_out << "; behavior for sample playback.\n";
	file_out << ";\n";
	file_out << "; To preserve current functionality, this feature is enabled by default\n";
	file_out << "; NOTE: This option works with all AltSound formats listed above\n";
	file_out << "; ----------------------------------------------------------------------------\n\n";

	// create [volume] section
	file_out << "[volume]\n";
	file_out << "rom_ctrl = 1\n\n";

	file_out << "; ----------------------------------------------------------------------------\n";
	file_out << "; To facilitate troubleshooting, the AltSound processor logs events to the\n";
	file_out << "; \"altsound.log\" file, which can be found in the \"tables\" folder.  There\n";
	file_out << "; are 4 logging levels:\n";
	file_out << "; 1. Info\n";
	file_out << "; 2. Error\n";
	file_out << "; 3. Warning\n";
	file_out << "; 4. Debug\n";
	file_out << ";\n";
	file_out << "; Setting a logging level will also include lower-ordered logging as well.  For\n";
	file_out << "; example, setting logging level to \"Warning\" will also enable \"Info\" and\n";
	file_out << "; \"Error\" logging.  By default, logging is set to \"Error\" which will also\n";
	file_out << "; include \"Info\" log messages.  The log file will be overwritten each time a\n";
	file_out << "; new table is loaded.\n";
	file_out << "; ----------------------------------------------------------------------------\n\n";

	// create [logging] section
	file_out << "[logging]\n";
	file_out << "logging_level = Error\n\n";

	file_out << "; ----------------------------------------------------------------------------\n";
	file_out << "; The section below allows for tailoring of the AltSound2 behaviors. They are\n";
	file_out << "; not used for traditional Altound or Legacy Altsound formats\n";
	file_out << ";\n";
	file_out << "; The AltSound2 format supports the following sample types :\n";
	file_out << ";\n";
	file_out << "; -MUSIC   : background music\n";
	file_out << "; -CALLOUT : voice interludes and callouts\n";
	file_out << "; -SFX     : short sounds to supplement table sounds\n";
	file_out << "; -SOLO    : sound played at end-of-ball/game, or tilt\n";
	file_out << "; -OVERLAY : sounds played over music/sfx\n";
	file_out << ";\n";
	file_out << "; Each sample type supports the following variables to tailor playback behavior\n";
	file_out << "; with respect to other sample types:\n";
	file_out << ";\n";
	file_out << "; \"ducks\"            : specify which sample types are ducked\n";
	file_out << "; \"pauses\"           : specify which sample types are paused\n";
	file_out << "; \"stops\"            : specify which sample types are stopped\n";
	file_out << "; \"music_duck_vol\"   : specify the ducked volume for music samples\n";
	file_out << "; \"callout_duck_vol\" : specify the ducked volume for callout samples\n";
	file_out << "; \"sfx_duck_vol\"     : specify the ducked volume for sfx samples\n";
	file_out << "; \"solo_duck_vol\"    : specify the ducked volume for solo samples\n";
	file_out << "; \"overlay_duck_vol\" : specify the ducked volume for overlay samples\n";
	file_out << ";\n";
	file_out << "; NOTES\n";
	file_out << "; - a sample type cannot duck / pause another sample of the same type\n";
	file_out << "; - stopping a sample of the same type essentially means that only one sample\n";
	file_out << ";   of that type can be played at the same time\n";
	file_out << "; - a stopped sample cannot be resumed / restarted\n";
	file_out << "; - ducking values are specified as a percentage of the gain of the\n";
	file_out << ";   affected sample type(s). Valid values range from 0 to 100 where\n";
	file_out << ";   0 completely mutes the sample, and 100 effectively negates ducking\n";
	file_out << "; - if multiple ducking values apply to a single sample, the lowest\n";
	file_out << ";   ducking value is used\n";
	file_out << "; - ducking / pausing ends when the sample that set it has ended.If\n";
	file_out << ";   multiple sample types duck / pause another type, playback will remain\n";
	file_out << ";   ducked / paused until the last affecting sample has ended\n";
	file_out << "; ----------------------------------------------------------------------------\n\n";

	// create behavior variable section
	file_out << "[music]\n";
	file_out << "ducks =\n";
	file_out << "pauses =\n";
	file_out << "stops = music\n";
	file_out << "; music_duck_vol =\n";
	file_out << "; callout_duck_vol =\n";
	file_out << "; sfx_duck_vol =\n";
	file_out << "; solo_duck_vol =\n";
	file_out << "; overlay_duck_vol =\n\n";

	file_out << "[callout]\n";
	file_out << "ducks = sfx\n";
	file_out << "pauses = music\n";
	file_out << "stops = callout\n";
	file_out << "; music_duck_vol =\n";
	file_out << "; callout_duck_vol =\n";
	file_out << "sfx_duck_vol =\n";
	file_out << "; solo_duck_vol =\n";
	file_out << "; overlay_duck_vol =\n\n";

	file_out << "[sfx]\n";
	file_out << "ducks =\n";
	file_out << "pauses =\n";
	file_out << "stops =\n";
	file_out << "; music_duck_vol =\n";
	file_out << "; callout_duck_vol =\n";
	file_out << "; sfx_duck_vol =\n";
	file_out << "; solo_duck_vol =\n";
	file_out << "; overlay_duck_vol =\n\n";

	file_out << "[solo]\n";
	file_out << "ducks =\n";
	file_out << "pauses =\n";
	file_out << "stops = music, solo, overlay, callout\n";
	file_out << "; music_duck_vol =\n";
	file_out << "; callout_duck_vol =\n";
	file_out << "; sfx_duck_vol =\n";
	file_out << "; solo_duck_vol =\n";
	file_out << "; overlay_duck_vol =\n\n";

	file_out << "[overlay]\n";
	file_out << "ducks = music, sfx\n";
	file_out << "pauses =\n";
	file_out << "stops =\n";
	file_out << "music_duck_vol =\n";
	file_out << "; callout_duck_vol =\n";
	file_out << "sfx_duck_vol =\n";
	file_out << "; solo_duck_vol =\n";
	file_out << "; overlay_duck_vol =\n\n";

	file_out.close();

	ALT_INFO(0, "\"altsound.ini\" created");
	
	OUTDENT;
	ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
	return true;
}