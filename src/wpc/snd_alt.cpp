// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

#include "snd_alt.h"

#if defined(_WIN32)
  #include <direct.h>
  #define CURRENT_DIRECTORY _getcwd
#else
  #include <unistd.h>
  #define CURRENT_DIRECTORY getcwd
#endif

#include <algorithm>
#include <time.h>
#include <string.h>
#include <fstream>
#include <ostream>
#include <sys/stat.h>

#ifdef __cplusplus
  extern "C" {
#endif
  #include "core.h"
#ifdef __cplusplus
  }
#endif

#include "osdepend.h"
#include "gen.h"

//#include "altsound2_processor.hpp"
#include "altsound_processor_base.hpp"
#include "altsound_processor.hpp"
#include "altsound_file_parser.hpp"
#include "inipp.h"
#include "altsound_data.hpp"

// namespace scope resolution
using std::string;
using std::endl;

// windef.h "min" conflicts with std::min
#ifdef min
  #undef min
#endif

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define FILTERED_INCOMPLETE 999

// ---------------------------------------------------------------------------
// Externals
// ---------------------------------------------------------------------------

extern "C" char g_szGameName[256];

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Command storage and workspace
CmdData cmds;

// Initialization support
static BOOL is_initialized = FALSE;
static BOOL run_once = true;
static BOOL altsound_stable = TRUE;

// Processing instance to specialize command handling
static AltsoundProcessor *processor = NULL;

// Use ROM control commands to control master volume
bool use_rom_ctrl = true;

// ---------------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------------

// get path to VPinMAME
std::string get_vpinmame_path();

// Helper function to check if supplied path exists
bool dir_exists(const string& path_in);

// Create altsound.ini file if it doesn't exist
bool create_altsound_ini(const string& path_in);

// DAR@20230623
// This is in support of altsound.ini file creation for packages that don't
// already have one (legacy).  It works in order of precedence:
//
//  1. presence of altsound2.csv
//  2. presence of altsound.csv
//	3. presense of PinSound directory structure
//
// Once the .ini file is created, it can be modified to adjust preference.
//
// determine altsound format from installed data
string get_altound_format(const string& path_in);

// Parse the altsound ini file
bool parse_altsound_ini(const string& path_in, string& format_out, bool& rom_ctrl_out);

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

extern "C" void alt_sound_handle(int boardNo, int cmd)
{
	LOG(("\nBEGIN: alt_sound_handle()\n")); //DAR_DEBUG

	if (!is_initialized && altsound_stable) {
		is_initialized = alt_sound_init(&cmds);
		altsound_stable = is_initialized;
	}

	if (!is_initialized || !altsound_stable) {
		LOG(("ERROR: Altsound unstable. Processing skipped.\n"));
		
		LOG(("END: alt_sound_handle()\n")); //DAR_DEBUG
		return;
	}

	//DAR@20230519 not sure what this does
	int	attenuation = osd_get_mastervolume();

	while (attenuation++ < 0) {
		master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB
	}
	LOG(("- Master Volume (Post Attenuation): %f\n ", master_vol)); //DAR_DEBUG

	cmds.cmd_counter++;

	//Shift all commands up to free up slot 0
	for (int i = ALT_MAX_CMDS - 1; i > 0; --i)
		cmds.cmd_buffer[i] = cmds.cmd_buffer[i - 1];

	cmds.cmd_buffer[0] = cmd; //add command to slot 0

	// pre-process commands based on ROM hardware platform
	preprocess_commands(&cmds, cmd);

	if (cmds.cmd_filter || (cmds.cmd_counter & 1) != 0) {
		// Some commands are 16-bits collected from two 8-bit commands.  If
		// the command is filtered or we have not received enough data yet,
		// try again on the next command
		//
		// NOTE:
		// Command size and filter requirements are ROM hardware platform
		// dependent.  The command preprocessor will take care of the
		// bookkeeping

		// Store the command for accumulation
		cmds.stored_command = cmd;

		if (cmds.cmd_filter) {
			LOG(("- Command filtered: %04X\n", cmd)); //DAR_DEBUG
		}

		if ((cmds.cmd_counter & 1) != 0) {
			LOG(("- Command incomplete: %04X\n", cmd)); //DAR_DEBUG
		}
		
		LOG(("END: alt_sound_handle\n")); //DAR_DEBUG
		return;
	}
	LOG(("- Command complete. Processing...\n")); //DAR_DEBUG

	// combine stored command with the current
	unsigned int cmd_combined = (cmds.stored_command << 8) | cmd;

	// Handle the resulting command
	if (!processor->handleCmd(cmd_combined)) {
		LOG(("- FAILED: processor::handleCmd()\n")); //DAR_DEBUG

		postprocess_commands(cmd_combined);

		
		LOG(("END: alt_sound_handle()\n")); //DAR_DEBUG
		return;
	}
	LOG(("- SUCCESS: processor::handleCmd()\n")); //DAR_DEBUG

	//DAR@20230522 I don't know what this does yet
	postprocess_commands(cmd_combined);

	
	LOG(("END: alt_sound_handle()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

BOOL alt_sound_init(CmdData* cmds_out)
{
	LOG(("BEGIN: alt_sound_init()\n"));

	// DAR@20230616
	// This shouldn't happen, so if it does, log it
	if (processor) {
		LOG(("ERROR: processor already defined\n"));
		delete processor;
		processor = NULL;
	}

	// initialize channel_stream storage
	std::fill(channel_stream.begin(), channel_stream.end(), nullptr);

	// Seed random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// get path to altsound folder for the current game
	string altsound_path = get_vpinmame_path();
	if (!altsound_path.empty()) {
		altsound_path += "\\altsound\\";
		altsound_path += g_szGameName;
		LOG(("- Path to altsound: %s\n", altsound_path.c_str()));
	}
	else {
		LOG(("FAILED: get_vpinmame_path()\n"));
		return false;
	}

	// parse .ini file
	string format;
	bool rom_ctrl = true;

	if (!parse_altsound_ini(altsound_path, format, rom_ctrl)) {
		LOG(("FAILED: parse_altsound_ini(%s)", altsound_path.c_str()));
		return FALSE;
	}

	if (format == "altsound2") {
		// AltSound2 only supports new CSV format. No need to specify format
		// in the constructor
		// DAR_TODO
	}
	else if (format == "altsound" || format == "pinsound") {
		// Traditional altsound processor handles existing CSV and legacy
		// PinSound format so it must be specified in the constructor
		processor = new AltsoundProcessor(g_szGameName, format);
	}
	else {
		LOG(("ERROR: Unknown AltSound format: %s\n", format.c_str()));
		return FALSE;
	}

	if (!processor) {
		LOG(("FAILED: Unable to create AltSound processor\n"));
		LOG(("END: alt_sound_init()\n"));
		return FALSE;
	}
	LOG(("SUCCESS: %s processor created\n", format.c_str()));

	global_vol = 1.0f;
	master_vol = 1.0f;
	use_rom_ctrl = rom_ctrl;

	// intialize the command bookkeeping structure
	cmds_out->cmd_counter = 0;
	cmds_out->stored_command = -1;
	cmds_out->cmd_filter = 0;
	std::fill_n(cmds_out->cmd_buffer, ALT_MAX_CMDS, ~0);

	int DSidx = -1; // BASS default device
	//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
	if (DSidx != -1)
		DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
	if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
	{
		LOG(("- BASS initialization error: %s\n", get_bass_err()));
		//sprintf_s(bla, "BASS music/sound library initialization error %d", BASS_ErrorGetCode());
	}

	// disable the global mixer to mute ROM sounds in favor of altsound
	mixer_sound_enable_global_w(0);

	//DAR@20230520
	// This code does not appear to be necessary. The call above which sets the
	// global mixer status seems to do the trick. If it's really
	// required for WPC89 generation, there should be an explicit check for it and
	// this code should be executed in that case only
	//	int ch;
	//	// force internal PinMAME volume mixer to 0 to mute emulated sounds & musics
	//	// required for WPC89 sound board
	//	LOG(("MUTING INTERNAL PINMAME MIXER\n")); //DAR_DEBUG
	//	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++) {
	//		const char* mixer_name = mixer_get_name(ch);
	//		if (mixer_name != NULL)	{
	//			mixer_set_volume(ch, 0);
	//		}
	//		else {
	//			LOG(("MIXER_NAME (Channel %d) IS NULL\n", ch)); //DAR_DEBUG
	//		}
	//	}

	LOG(("END: alt_sound_init()\n"));
	return TRUE;
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_exit() {
	//DAR_TODO clean up internal storage?
	LOG(("BEGIN: alt_sound_exit()\n"));

	// reset static variables
	static CmdData cmds;

	// Initialization support
	is_initialized = FALSE;
	run_once = TRUE;
	altsound_stable = TRUE;
	use_rom_ctrl = true;
	master_vol = 1.0f;
	global_vol = 1.0f;

	// clean up processor
	delete processor;
	processor = nullptr;

	// DAR@20230618
	// This needs to happen AFTER the processor is deleted.  BASS_Free() cleans
	// up resources that are still held by the processor.  If this is called
	// first, it will cause an error to be logged when shutting down
	if (BASS_Free() == FALSE) {
		LOG(("- FAILED: BASS_Free(): %s\n", get_bass_err()));
	}
	else {
		LOG(("- SUCCESS: BASS_Free()\n"));
	}

	LOG(("END: alt_sound_exit()\n"));
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_pause(BOOL pause) {
	LOG(("BEGIN: alt_sound_pause()\n")); // DAR_DEBUG

	if (pause) {
		LOG(("- Pausing stream playback (ALL)\n")); // DAR_DEBUG

		// Pause all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING) {
				if (!BASS_ChannelPause(stream)) {
					LOG(("- FAILED: BASS_ChannelPause(%u): %s\n", stream, get_bass_err())); // DAR_DEBUG
				}
				else {
					LOG(("- SUCCESS: BASS_ChannelPause(%u)\n", stream)); // DAR_DEBUG
				}
			}
		}
	}
	else {
		LOG(("- Resuming stream playback (ALL)\n")); // DAR_DEBUG

		// Resume all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PAUSED) {
				if (!BASS_ChannelPlay(stream, 0)) {
					LOG(("- FAILED: BASS_ChannelPlay(%u): %s\n", stream, get_bass_err())); // DAR_DEBUG
				}
				else {
					LOG(("- SUCCESS: BASS_ChannelPlay(%u)\n", stream)); // DAR_DEBUG
				}
			}
		}
	}

	LOG(("END: alt_sound_pause()\n")); // DAR_DEBUG
}

// ---------------------------------------------------------------------------

void preprocess_commands(CmdData* cmds_out, int cmd_in)
{
	LOG(("BEGIN: preprocess_commands()\n"));

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("- MAME_GEN: %d\n", hardware_gen));

	// syntactic candy
	unsigned int* cmd_buffer = cmds_out->cmd_buffer;
	unsigned int* cmd_counter = &cmds_out->cmd_counter;
	unsigned int* cmd_filter = &cmds_out->cmd_filter;
	int* stored_command = &cmds_out->stored_command;

	if ((hardware_gen == GEN_WPCDCS) ||
		(hardware_gen == GEN_WPCSECURITY) ||
		(hardware_gen == GEN_WPC95DCS) ||
		(hardware_gen == GEN_WPC95))
	{
		LOG(("Hardware Generation: GEN_WPCDCS, GEN_WPCSECURITY, GEN_WPC95DCS, GEN_WPC95\n")); //DAR_DEBUG

		if (((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA)) // change volume?
			||
			((cmd_buffer[2] == 0x00) && (cmd_buffer[1] == 0x00) && (cmd_buffer[0] == 0x00))) // glitch in command buffer?
		{
			if ((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) { // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
				// DAR@20230518
				// I don't know why this is nerfing the volume.  It does not
				// appear to work correctly in all cases. 
				if (use_rom_ctrl) {
					global_vol = std::min((float)cmd_buffer[1] / 127.f, 1.0f);
					LOG(("- Change volume %.2f\n", global_vol));
				}
			}
			else
				LOG(("- filtered command %02X %02X %02X %02X\n", cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]));

			for (int i = 0; i < ALT_MAX_CMDS; ++i) {
				cmd_buffer[i] = ~0;
			}

			*cmd_counter = 0;
			*cmd_filter = 1;
		}
		else
			*cmd_filter = 0;
	}

	if ((hardware_gen == GEN_WPCALPHA_2) || //!! ?? test this gen actually
		(hardware_gen == GEN_WPCDMD) || // remaps everything to 16bit, a bit stupid maybe
		(hardware_gen == GEN_WPCFLIPTRON))
	{
		LOG(("- Hardware Generation: GEN_WPCALPHA_2, GEN_WPCDMD, GEN_WPCFLIPTRON\n")); //DAR_DEBUG

		*cmd_filter = 0;
		if ((cmd_buffer[2] == 0x79) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
		{
			// DAR@20230518
			// I don't know why this is nerfing the volume.  It does not
			// appear to work correctly in all cases. 
			if (use_rom_ctrl) {
				global_vol = std::min((float)cmd_buffer[1] / 127.f, 1.0f);
				LOG(("- Change volume %.2f\n", global_vol));
			}

			for (int i = 0; i < ALT_MAX_CMDS; ++i)
				cmd_buffer[i] = ~0;

			*cmd_counter = 0;
			*cmd_filter = 1;
		}
		else if (cmd_buffer[1] == 0x7A) // 16bit command second part //!! TZ triggers a 0xFF in the beginning -> check sequence and filter?
		{
			*stored_command = cmd_buffer[1];
			*cmd_counter = 0;
		}
		else if (cmd_in != 0x7A) // 8 bit command
		{
			*stored_command = 0;
			*cmd_counter = 0;
		}
		else // 16bit command first part
			*cmd_counter = 1;
	}

	if ((hardware_gen == GEN_WPCALPHA_1) || // remaps everything to 16bit, a bit stupid maybe //!! test all these generations!
		(hardware_gen == GEN_S11) ||
		(hardware_gen == GEN_S11X) ||
		(hardware_gen == GEN_S11B2) ||
		(hardware_gen == GEN_S11C))
	{
		LOG(("- Hardware Generation: GEN_WPCALPHA_1, GEN_S11, GEN_S11X, GEN_S11B2, GEN_S11C\n")); //DAR_DEBUG

		if (cmd_in != cmd_buffer[1]) //!! some stuff is doubled or tripled -> filter out?
		{
			*stored_command = 0; // 8 bit command //!! 7F & 7E opcodes?
			*cmd_counter = 0;
		}
		else // ignore
			*cmd_counter = 1;
	}

	if ((hardware_gen == GEN_DEDMD16) || // remaps everything to 16bit, a bit stupid maybe
		(hardware_gen == GEN_DEDMD32) ||
		(hardware_gen == GEN_DEDMD64) ||
		(hardware_gen == GEN_DE))        // this one just tested with BTTF so far
	{
		LOG(("- Hardware Generation: GEN_DEDMD16, GEN_DEDMD32, GEN_DEDMD64, GEN_DE\n")); //DAR_DEBUG

		if (cmd_in != 0xFF && cmd_in != 0x00) // 8 bit command
		{
			*stored_command = 0;
			*cmd_counter = 0;
		}
		else // ignore
			*cmd_counter = 1;

		if (cmd_buffer[1] == 0x00 && cmd_in == 0x00) // handle 0x0000 special //!! meh?
		{
			*stored_command = 0;
			*cmd_counter = 0;
		}
	}

	if ((hardware_gen == GEN_WS) ||
		(hardware_gen == GEN_WS_1) ||
		(hardware_gen == GEN_WS_2))
	{
		LOG(("- Hardware Generation: GEN_WS, GEN_WS_1, GEN_WS_2\n")); //DAR_DEBUG

		*cmd_filter = 0;
		if (cmd_buffer[1] == 0xFE)
		{
			if (cmd_in >= 0x10 && cmd_in <= 0x2F)
			{
				// DAR@20230518
				// I don't know why this is nerfing the volume.  It does not
				// appear to work correctly in all cases. 
				if (use_rom_ctrl) {
					global_vol = std::min((float)cmd_buffer[1] / 127.f, 1.0f);
					LOG(("- Change volume %.2f\n", global_vol));
				}

				for (int i = 0; i < ALT_MAX_CMDS; ++i)
					cmd_buffer[i] = ~0;

				*cmd_counter = 0;
				*cmd_filter = 1;
			}
			else if (cmd_in >= 0x01 && cmd_in <= 0x0F)	// ignore FE 01 ... FE 0F
			{
				*stored_command = 0;
				*cmd_counter = 0;
				*cmd_filter = 1;
			}
		}

		if ((cmd_in & 0xFC) == 0xFC) // start byte of a command will ALWAYS be FF, FE, FD, FC, and never the second byte!
			*cmd_counter = 1;
	}
	
	LOG(("END: preprocess_commands()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void postprocess_commands(const unsigned int combined_cmd) {
	LOG(("BEGIN: postprocess_commands()\n")); //DAR_DEBUG

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("- MAME_GEN: %d\n", hardware_gen)); //DAR_DEBUG

	if (hardware_gen == GEN_WPCDCS
		|| hardware_gen == GEN_WPCSECURITY
		|| hardware_gen == GEN_WPC95DCS
		|| hardware_gen == GEN_WPC95)
	{
		if (combined_cmd == 0x03E3) // stop music
		{
			LOG(("- stopping MUSIC(2)\n")); //DAR_DEBUG
			processor->stopMusicStream();
		}
	}

	//!! old WPC machines music stop? -> 0x00 for SYS11?

	if (hardware_gen == GEN_DEDMD32)
	{
		if ((combined_cmd == 0x0018 || combined_cmd == 0x0023)) // stop music //!! ???? 0x0019??
		{
			LOG(("- stopping MUSIC(3)\n")); //DAR_DEBUG
			processor->stopMusicStream();
		}
	}

	if (hardware_gen == GEN_WS
		|| hardware_gen == GEN_WS_1
		|| hardware_gen == GEN_WS_2)
	{
		if (((combined_cmd == 0x0000 || (combined_cmd & 0xf0ff) == 0xf000))) // stop music
		{
			LOG(("- stopping MUSIC(4)\n")); //DAR_DEBUG
			processor->stopMusicStream();
		}
	}
	
	LOG(("END: postprocess_commands()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------
// Helper function to determine AltSound format
// ---------------------------------------------------------------------------

string get_altound_format(const string& path_in)
{
	LOG(("BEGIN: get_altsound_format()\n"));

	bool using_altsound = false;
	bool using_altsound2 = false;
	bool using_pinsound = false;

	// check for altsound2
	string path1 = path_in;
	path1.append("\\altsound2.csv");

	std::ifstream ini(path1.c_str());
	if (ini.good()) {
		LOG(("- Using AltSound2 format\n"));
		using_altsound2 = true;
		ini.close();
	}
	else {
		// checl for traditional altsound
		string path2 = path_in;
		path2.append("\\altsound.csv");

		std::ifstream ini(path2.c_str());
		if (ini.good()) {
			LOG(("- Using traditional AltSound format\n"));
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
			LOG(("- Using PinSound format\n"));
	}

	return using_altsound2 ? "altsound2" : using_altsound ? "altsound" : 
		   using_pinsound ? "pinsound" : "";

	LOG(("END: get_altsound_format()\n"));
}

// ---------------------------------------------------------------------------
// Helper function to get path to VpinMAME
// ---------------------------------------------------------------------------

std::string get_vpinmame_path()
{
	LOG(("BEGIN: get_vpinmame_path\n"));

	char cvpmd[MAX_PATH];
	HMODULE hModule = nullptr;

#ifdef _WIN64
	hModule = GetModuleHandleA("VPinMAME64.dll");
#else
	hModule = GetModuleHandleA("VPinMAME.dll");
#endif

	if (hModule != nullptr)
	{
		GetModuleFileNameA(hModule, cvpmd, sizeof(cvpmd));
		std::string vpm_path = cvpmd;
		LOG(("END: get_vpinmame_path\n"));
		return vpm_path.substr(0, vpm_path.rfind("\\"));
	}
	else
	{
		LOG(("Module not found: VPinMAME.dll or VPinMAME64.dll\n"));
	}

	LOG(("END: get_vpinmame_path\n"));
	return "";
}

// ---------------------------------------------------------------------------
// Helper function to check if a directory exists
// ---------------------------------------------------------------------------

bool dir_exists(const std::string& path_in)
{
	LOG(("BEGIN: dir_exists()\n"));

	struct stat info;

	if (stat(path_in.c_str(), &info) != 0) {
		LOG(("- Directory: %s does not exist\n", path_in.c_str()));
		
		LOG(("END: dir_exists()\n"));
		return false;
	}
	LOG(("- Directory: %s exists\n", path_in.c_str()));

	LOG(("END: dir_exists()\n"));
	return (info.st_mode & S_IFDIR) != 0;
}

// ---------------------------------------------------------------------------
// Helper function to parse the altsound.ini file
// ---------------------------------------------------------------------------

bool parse_altsound_ini(const string& path_in, string& format_out, bool& rom_ctrl_out)
{
	LOG(("BEGIN: parse_altsound_ini()\n"));

	// if altsound.ini does not exist, create it
	string ini_path = path_in;
	ini_path.append("\\altsound.ini");

	std::ifstream file_in(ini_path);
	if (!file_in.good()) {
		LOG(("- \"altsound.ini\" not found. Creating it.\n"));
		if (!create_altsound_ini(path_in)) {
			LOG(("- FAILED: create_ini_file()\n"));

			LOG(("END: parse_altsound_ini()\n"));
			return false;
		}
		LOG(("- SUCCESS: create_ini_file()\n"));
		file_in.open(ini_path);
	}

	if (!file_in.good()) {
		LOG(("- failed to open \"altsound.ini\"\n"));

		LOG(("END: parse_altsound_ini()\n"));
		return false;
	}

	// parse ini file
	inipp::Ini<char> ini;
	ini.parse(file_in);

	// get format
	string format;
	inipp::get_value(ini.sections["format"], "format", format);
	LOG(("- Parsed \"format\": %s\n", format.c_str()));
	format_out = format;

	// get ROM volume control
	string rom_control;
	inipp::get_value(ini.sections["volume"], "rom_ctrl", rom_control);
	LOG(("- Parsed \"rom_ctrl\": %s\n", rom_control.c_str()));
	rom_ctrl_out = rom_control == "1";

	LOG(("END: parse_altsound_ini()\n"));
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to create the altsound.ini file
// ---------------------------------------------------------------------------

bool create_altsound_ini(const string& path_in)
{
	LOG(("BEGIN: create_altsound_ini()\n"));

	string format = get_altound_format(path_in);

	if (format.empty()) {
		LOG(("- FAILED: get_altsound_format()\n"));

		LOG(("END: create_altsound_ini()\n"));
		return false;
	}
	LOG(("- SUCCESS: get_altsound_format(): %s\n", format.c_str()));

	string ini_path = path_in;
	ini_path.append("\\altsound.ini");

	std::ofstream file_out;
	file_out.open(ini_path);

	file_out << "; ----------------------------------------------------------------------------" << endl;
	file_out << "; There are three supported formats :" << endl;
	file_out << ";  1. Legacy" << endl;
	file_out << ";  2. AltSound" << endl;
	file_out << ";  3. AltSound2" << endl;
	file_out << ";" << endl;
	file_out << "; Legacy: the original AltSound format that parses a file / folder structure" << endl;
	file_out << ";         similar to the PinSound system. It is no longer used for new" << endl;
	file_out << ";         AltSound packages" << endl;
	file_out << ";" << endl;
	file_out << "; AltSound: a CSV-based format designed as a replacement for the PinSound" << endl;
	file_out << ";           format. This format defines samples according to \"channels\" with" << endl;
	file_out << ";           loosely defined behaviors controlled by the associated metadata" << endl;
	file_out << ";           \"gain\", \"ducking\", and \"stop\" fields. This is the format" << endl;
	file_out << ";           currently in use by most AltSound authors" << endl;
	file_out << ";" << endl;
	file_out << "; Altsound2: a new CSV-based format that defines samples according to" << endl;
	file_out << ";            contextual types, allowing for more intuitively designed AltSound" << endl;
	file_out << ";            packages. General playback behavior is dictated by the assigned" << endl;
	file_out << ";            type. Behaviors can evolve without the need for adding" << endl;
	file_out << ";            additional CSV fields, and the combinatorial complexity that" << endl;
	file_out << ";            comes with it." << endl;
    file_out << ";            NOTE: This option requires the new altsound2.csv" << endl;
	file_out << ";" << endl;
	file_out << "; ----------------------------------------------------------------------------" << endl;

	// create [format] section
	file_out << "[format]" << endl;
	file_out << "format = " << format << endl;

	file_out << endl;
	file_out << "; ----------------------------------------------------------------------------" << endl;
	file_out << "; The AltSound processor attempts to recreate certain sample behaviors based" << endl;
	file_out << "; on commands sent from the ROM.This does not appear to be working in all" << endl;
	file_out << "; cases, resulting in undesirable muting of the sample playback volume." << endl;
	file_out << "; Until this can be addressed, this feature can be disabled by setting" << endl;
	file_out << "; the \"rom_control\" setting to false." << endl;
	file_out << ";" << endl;
	file_out << "; Turning this feature off will use only the defined \"gain\" and \"ducking\"" << endl;
	file_out << "; values defined in the CSV file, and will not attempt to replicate ROM" << endl;
	file_out << "; behavior for sample playback." << endl;
	file_out << ";" << endl;
	file_out << "; To preserve current functionality, this feature is enabled by default" << endl;
	file_out << "; NOTE: This option works with all AltSound formats listed above" << endl;
	file_out << "; ----------------------------------------------------------------------------" << endl;

	// create [volume] section
	file_out << "[volume]" << endl;
	file_out << "rom_ctrl = 1" << endl;
	file_out.close();
	LOG(("- \"altsound.ini\" created\n"));

	LOG(("END: create_altsound_ini()\n"));
	return true;
}