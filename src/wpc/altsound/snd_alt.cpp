// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

#include "snd_alt.h"

//#if defined(_WIN32)
//  #include <direct.h>
//  #define CURRENT_DIRECTORY _getcwd
//#else
//  #include <unistd.h>
//  #define CURRENT_DIRECTORY getcwd
//#endif

// Std Library includes
#include <algorithm>
#include <time.h>
#include <string.h>
#include <ostream>
#include <sys/stat.h>

#ifdef __cplusplus
  extern "C" {
#endif
  #include "core.h"
#include "osdepend.h"
#ifdef __cplusplus
  }
#endif

// local includes
#include "osdepend.h"
#include "gen.h"
#include "altsound_processor_base.hpp"
#include "altsound_processor.hpp"
#include "gsound_processor.hpp"
#include "altsound_file_parser.hpp"
#include "altsound_ini_processor.hpp"
#include "inipp.h"
#include "altsound_data.hpp"
#include "altsound_logger.hpp"

// namespace scope resolution
using std::string;
using std::endl;

AltsoundLogger alog("altsound.log");

// Data structure to hold sample behaviors
BehaviorInfo music_behavior;
BehaviorInfo callout_behavior;
BehaviorInfo sfx_behavior;
BehaviorInfo solo_behavior;
BehaviorInfo overlay_behavior;

// Instance of global thread synchronization mutex
std::mutex io_mutex;

// Instance of global array of BASS channels 
StreamArray channel_stream;

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
BOOL is_initialized = FALSE;
BOOL run_once = true;
BOOL altsound_stable = TRUE;

// Processing instance to specialize command handling
AltsoundProcessorBase *processor = NULL;

// Use ROM control commands to control master volume
bool use_rom_ctrl = true;

// ---------------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------------

// get path to VPinMAME
std::string get_vpinmame_path();

//std::string trim(const std::string& str);

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

extern "C" void alt_sound_handle(int boardNo, int cmd)
{
	ALT_DEBUG(0, "");
	ALT_DEBUG(0, "BEGIN alt_sound_handle()");
	INDENT;

	if (!is_initialized && altsound_stable) {
		is_initialized = alt_sound_init(&cmds);
		altsound_stable = is_initialized;
	}

	if (!is_initialized || !altsound_stable) {
		ALT_ERROR(0, "Altsound unstable. Processing skipped.");

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_handle()");
		return;
	}

	//DAR@20230519 not sure what this does
	int	attenuation = osd_get_mastervolume();

	float master_vol = processor->getMasterVol();
	while (attenuation++ < 0) {
		master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB
	}
	processor->setMasterVol(master_vol);
	ALT_DEBUG(0, "Master Volume (Post Attenuation): %.02f", master_vol);

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
			ALT_DEBUG(0, "Command filtered: %04X", cmd);
		}

		if ((cmds.cmd_counter & 1) != 0) {
			ALT_DEBUG(0, "Command incomplete: %04X", cmd);
		}
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_handle");
		return;
	}
	ALT_DEBUG(0, "Command complete. Processing...");

	// combine stored command with the current
	unsigned int cmd_combined = (cmds.stored_command << 8) | cmd;

	// Handle the resulting command
	if (!ALT_CALL(processor->handleCmd(cmd_combined))) {
		ALT_WARNING(0, "FAILED processor::handleCmd()");

		postprocess_commands(cmd_combined);
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_handle()");
		return;
	}
	ALT_INFO(0, "SUCCESS processor::handleCmd()");

	//DAR@20230522 I don't know what this does yet
	postprocess_commands(cmd_combined);
	
	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_handle()");
	ALT_DEBUG(0, "");
}

// ---------------------------------------------------------------------------

BOOL alt_sound_init(CmdData* cmds_out)
{
	ALT_DEBUG(0, "BEGIN alt_sound_init()");
	INDENT;

	// DAR@20230616
	// This shouldn't happen, so if it does, log it
	if (processor) {
		ALT_ERROR(0, "Processor already defined");
		delete processor;
		processor = NULL;
	}

	// initialize channel_stream storage
	std::fill(channel_stream.begin(), channel_stream.end(), nullptr);

	// Seed random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// get path to altsound folder for the current game
	string altsound_path;
	string vpm_path = get_vpinmame_path();
	if (!vpm_path.empty()) {
		altsound_path = vpm_path + "/altsound/" + g_szGameName;
		ALT_INFO(0, "Path to altsound: %s", altsound_path.c_str());
	}
	else {
		ALT_ERROR(0, "VPinMAME not found");

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return false;
	}

	// parse .ini file
	AltsoundIniProcessor ini_proc;

	if (!ini_proc.parse_altsound_ini(altsound_path)) {
		ALT_ERROR(0, "Failed to parse_altsound_ini(%s)", altsound_path.c_str());
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}

	string format = ini_proc.getAltsoundFormat();

	std::string game_name(g_szGameName);

	if (format == "g-sound") {
		// G-Sound only supports new CSV format. No need to specify format
		// in the constructor
		processor = new GSoundProcessor(game_name, vpm_path);
	}
	else if (format == "altsound" || format == "legacy") {
		// Traditional altsound processor handles existing CSV and legacy
		// PinSound format so it must be specified in the constructor
		processor = new AltsoundProcessor(game_name, vpm_path, format);
	}
	else {
		ALT_ERROR(0, "Unknown AltSound format: %s", format.c_str());

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}

	if (!processor) {
		ALT_ERROR(0, "FAILED: Unable to create AltSound processor");
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}
	ALT_INFO(0, "%s processor created", format.c_str());

	processor->setMasterVol(1.0f);
	processor->setGlobalVol(1.0f);
	processor->romControlsVol(ini_proc.usingRomVolumeControl());
	processor->recordSoundCmds(ini_proc.recordSoundCmds());

	// perform processor initialization (load samples, etc)
	processor->init();

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
		ALT_ERROR(0, "BASS initialization error: %s", get_bass_err());
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
	//	LOG(("MUTING INTERNAL PINMAME MIXER"));
	//	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++) {
	//		const char* mixer_name = mixer_get_name(ch);
	//		if (mixer_name != NULL)	{
	//			mixer_set_volume(ch, 0);
	//		}
	//		else {
	//			LOG(("MIXER_NAME (Channel %d) IS NULL", ch));
	//		}
	//	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_init()");
	return TRUE;
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_exit() {
	//DAR_TODO clean up internal storage?
	ALT_DEBUG(0, "BEGIN alt_sound_exit()");
	INDENT;

	// reset static variables
	static CmdData cmds;

	// Initialization support
	is_initialized = FALSE;
	run_once = TRUE;
	altsound_stable = TRUE;
	use_rom_ctrl = true;

	// clean up processor
	delete processor;
	processor = nullptr;

	// DAR@20230618
	// This needs to happen AFTER the processor is deleted.  BASS_Free() cleans
	// up resources that are still held by the processor.  If this is called
	// first, it will cause an error to be logged when shutting down
	if (BASS_Free() == FALSE) {
		ALT_ERROR(0, "FAILED BASS_Free(): %s", get_bass_err());
	}
	else {
		ALT_INFO(0, "SUCCESS BASS_Free()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_exit()");
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_pause(BOOL pause) {
	ALT_DEBUG(0, "BEGIN alt_sound_pause()");
	INDENT;

	if (pause) {
		ALT_INFO(0, "Pausing stream playback (ALL)");

		// Pause all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING) {
				if (!BASS_ChannelPause(stream)) {
					ALT_WARNING(0, "FAILED BASS_ChannelPause(%u): %s", stream, get_bass_err());
				}
				else {
					ALT_INFO(0, "SUCCESS BASS_ChannelPause(%u)", stream);
				}
			}
		}
	}
	else {
		ALT_INFO(0, "Resuming stream playback (ALL)");

		// Resume all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PAUSED) {
				if (!BASS_ChannelPlay(stream, 0)) {
					ALT_WARNING(0, "FAILED BASS_ChannelPlay(%u): %s", stream, get_bass_err());
				}
				else {
					ALT_INFO(0, "SUCCESS BASS_ChannelPlay(%u)", stream);
				}
			}
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_pause()");
}

// ---------------------------------------------------------------------------

void preprocess_commands(CmdData* cmds_out, int cmd_in)
{
	ALT_DEBUG(0, "BEGIN preprocess_commands()");
	INDENT;

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	ALT_DEBUG(0, "MAME_GEN: %d", hardware_gen);

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
		ALT_DEBUG(0, "Hardware Generation: GEN_WPCDCS, GEN_WPCSECURITY, GEN_WPC95DCS, GEN_WPC95");

		if (((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA)) // change volume?
			||
			((cmd_buffer[2] == 0x00) && (cmd_buffer[1] == 0x00) && (cmd_buffer[0] == 0x00))) // glitch in command buffer?
		{
			if ((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) { // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
				// DAR@20230518
				// I don't know why this is nerfing the volume.  It does not
				// appear to work correctly in all cases. 
				if (processor->romControlsVol()) {
					processor->setGlobalVol(std::min((float)cmd_buffer[1] / 127.f, 1.0f));
					ALT_INFO(0, "Change volume %.02f", processor->getGlobalVol());
				}
			}
			else
				ALT_DEBUG(0, "filtered command %02X %02X %02X %02X", cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]);

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
		ALT_DEBUG(0, "Hardware Generation: GEN_WPCALPHA_2, GEN_WPCDMD, GEN_WPCFLIPTRON");

		*cmd_filter = 0;
		if ((cmd_buffer[2] == 0x79) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
		{
			// DAR@20230518
			// I don't know why this is nerfing the volume.  It does not
			// appear to work correctly in all cases. 
			if (processor->romControlsVol()) {
				processor->setGlobalVol(std::min((float)cmd_buffer[1] / 127.f, 1.0f));
				ALT_INFO(0, "Change volume %.02f", processor->getGlobalVol());
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
		ALT_DEBUG(0, "Hardware Generation: GEN_WPCALPHA_1, GEN_S11, GEN_S11X, GEN_S11B2, GEN_S11C");

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
		ALT_DEBUG(0, "Hardware Generation: GEN_DEDMD16, GEN_DEDMD32, GEN_DEDMD64, GEN_DE");

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
		ALT_DEBUG(0, "Hardware Generation: GEN_WS, GEN_WS_1, GEN_WS_2");

		*cmd_filter = 0;
		if (cmd_buffer[1] == 0xFE)
		{
			if (cmd_in >= 0x10 && cmd_in <= 0x2F)
			{
				// DAR@20230518
				// I don't know why this is nerfing the volume.  It does not
				// appear to work correctly in all cases. 
				if (processor->romControlsVol()) {
					processor->setGlobalVol(std::min((float)cmd_buffer[1] / 127.f, 1.0f));
					ALT_INFO(0, "Change volume %.02f", processor->getGlobalVol());
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
	
	OUTDENT;
	ALT_DEBUG(0, "END preprocess_commands()");
}

// ---------------------------------------------------------------------------

void postprocess_commands(const unsigned int combined_cmd) {
	ALT_DEBUG(0, "BEGIN postprocess_commands()");
	INDENT;

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	ALT_DEBUG(0, "MAME_GEN: %d", hardware_gen);

	if (hardware_gen == GEN_WPCDCS
		|| hardware_gen == GEN_WPCSECURITY
		|| hardware_gen == GEN_WPC95DCS
		|| hardware_gen == GEN_WPC95)
	{
		if (combined_cmd == 0x03E3) // stop music
		{
			ALT_INFO(0, "Stopping MUSIC(2)");
			processor->stopMusic();
		}
	}

	//!! old WPC machines music stop? -> 0x00 for SYS11?

	if (hardware_gen == GEN_DEDMD32)
	{
		if ((combined_cmd == 0x0018 || combined_cmd == 0x0023)) // stop music //!! ???? 0x0019??
		{
			ALT_INFO(0, "Stopping MUSIC(3)");
			processor->stopMusic();
		}
	}

	if (hardware_gen == GEN_WS
		|| hardware_gen == GEN_WS_1
		|| hardware_gen == GEN_WS_2)
	{
		if (((combined_cmd == 0x0000 || (combined_cmd & 0xf0ff) == 0xf000))) // stop music
		{
			ALT_INFO(0, "Stopping MUSIC(4)");
			processor->stopMusic();
		}
	}
	
	OUTDENT;
	ALT_DEBUG(0, "END postprocess_commands()");
}

// ---------------------------------------------------------------------------
// Helper function to get path to VpinMAME
// ---------------------------------------------------------------------------

std::string get_vpinmame_path()
{
	ALT_DEBUG(0, "BEGIN get_vpinmame_path");
	INDENT;

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

		// Normalize slashes
		std::replace(vpm_path.begin(), vpm_path.end(), '\\', '/');

		OUTDENT;
		ALT_DEBUG(0, "END get_vpinmame_path()");
		return vpm_path.substr(0, vpm_path.rfind("/"));
	}
	else
	{
		ALT_ERROR(0, "Module not found: VPinMAME.dll or VPinMAME64.dll");
	}

	OUTDENT;
	ALT_DEBUG(0, "END get_vpinmame_path()");
	return "";
}

