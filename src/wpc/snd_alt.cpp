// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

#include "snd_alt.h"
#include "altsound_data.h"

//#include <mutex>
#include <time.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
  extern "C" {
#endif
  #include <dirent.h>
#ifdef __cplusplus
  }
#endif

#ifdef __cplusplus
  extern "C" {
#endif
  #include "core.h"
#ifdef __cplusplus
  }
#endif

#include "osdepend.h"
#include "gen.h"

#include "altsound_processor.hpp"

// Processing instance to specialize command handling
static AltsoundProcessor *processor = NULL;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define FILTERED_INCOMPLETE 999

// ---------------------------------------------------------------------------
// Game(ROM) globals
// ---------------------------------------------------------------------------

extern "C" char g_szGameName[256];
static char* cached_machine_name = 0;

// ---------------------------------------------------------------------------
// Sound volume globals
// ---------------------------------------------------------------------------

//static float music_vol = 1.0f;
static float master_vol = 1.0f;
static float global_vol = 1.0f;

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

extern "C" void alt_sound_handle(int boardNo, int cmd)
{
	LOG(("\nBEGIN: ALT_SOUND_HANDLE\n")); //DAR_DEBUG
	

	static CmdData cmds;
	static BOOL is_initialized = FALSE;
	static BOOL run_once = true;
	static BOOL altsound_stable = TRUE; // future use

	if (cached_machine_name != 0 && strstr(g_szGameName, cached_machine_name) == 0)
	{// A new game has been loaded, clear out old memory and re-initialize
		LOG(("NEW GAME LOADED: %s\n", g_szGameName)); //DAR_DEBUG
		delete processor;
		run_once = true;

		// Save current machine name
		cached_machine_name = (char*)malloc(strlen(g_szGameName) + 1);
		strcpy(cached_machine_name, g_szGameName);
	}

	if (run_once) {
		is_initialized = alt_sound_init(&cmds);
		run_once = false;
	}

	if (!is_initialized || !altsound_stable) {
		// Intialization failed, or a downstream error has made continuing
		// unsafe... exit
		
		LOG(("END: ALT_SOUND_HANDLE\n")); //DAR_DEBUG
		return;
	}

	//DAR@20230519 not sure what this does
	int	attenuation = osd_get_mastervolume();

	while (attenuation++ < 0) {
		master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB
	}
	LOG(("Master Volume (Post Attenuation): %f\n ", master_vol)); //DAR_DEBUG

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
			LOG(("COMMAND FILTERED: %04X\n", cmd)); //DAR_DEBUG
		}

		if ((cmds.cmd_counter & 1) != 0) {
			LOG(("COMMAND INCOMPLETE: %04X\n", cmd)); //DAR_DEBUG
		}

		
		LOG(("END: ALT_SOUND_HANDLE\n")); //DAR_DEBUG
		return;
	}
	LOG(("COMMAND COMPLETE. PROCESSING...\n")); //DAR_DEBUG

	// combine stored command with the current
	unsigned int cmd_combined = (cmds.stored_command << 8) | cmd;

	// Handle the resulting command
	if (!processor->process_cmd(cmd_combined)) {
		LOG(("FAILED: processor::handle_cmd()\n")); //DAR_DEBUG

		postprocess_commands(cmd_combined);

		
		LOG(("END: ALT_SOUND_HANDLE\n")); //DAR_DEBUG
		return;
	}
	LOG(("SUCCESS: processor::handle_cmd()\n")); //DAR_DEBUG

	//DAR@20230522 I don't know what this does yet
	postprocess_commands(cmd_combined);

	
	LOG(("END: ALT_SOUND_HANDLE\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

BOOL alt_sound_init(CmdData* cmds_out) {
	LOG(("BEGIN: ALT_SOUND_INIT\n"));
	

	//DAR_TODO figure out which processor to use and create new
	if (!processor) {
		processor = new AltsoundProcessor(g_szGameName, global_vol, master_vol);
	}

	global_vol = 1.0f;
//	music_vol = 1.0f;
	master_vol = 1.0f;

	// intialize the command bookkeeping structure
	cmds_out->cmd_counter = 0;
	cmds_out->stored_command = -1;
	cmds_out->cmd_filter = 0;
	for (int i = 0; i < ALT_MAX_CMDS; ++i)
		cmds_out->cmd_buffer[i] = ~0;

	int DSidx = -1; // BASS default device
	//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
	if (DSidx != -1)
		DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
	if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
	{
		LOG(("BASS initialization error: %s\n", get_bass_err()));
		//sprintf_s(bla, "BASS music/sound library initialization error %d", BASS_ErrorGetCode());
	}

	// disable the global mixer to mute ROM sounds in favor of altsound
	mixer_sound_enable_global_w(0);

	//DAR@20230520
	// This code does not appear to be necessary.  The call above which sets the
	// global mixer status seems to do the trick.  If it's really
	// required for WPC89 generation, there should be an explicit check for it and
	// this code should only be executed in that case only
	//	int ch;
	//	// force internal PinMAME volume mixer to 0 to mute emulated sounds & musics
	//	// required for WPC89 sound board
	//	LOG(("MUTING INTERNAL PINMAME MIXER\n")); //DAR_DEBUG
	//	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++) {
	//		const char* mixer_name = mixer_get_name(ch);
	//		if (mixer_name != NULL)	{
	//			//LOG(("MIXER_NAME (Channel %d) IS NOT NULL\n", ch)); //DAR_DEBUG
	////DAR_DEBUG this crashed the program
	////				LOG(("MIXER_NAME (Channel %d): %s\n"), ch, mixer_name); //DAR_DEBUG
	//			mixer_set_volume(ch, 0);
	//		}
	//		else {
	//			//LOG(("MIXER_NAME (Channel %d) IS NULL\n", ch)); //DAR_DEBUG
	//		}
	//	}

	
	LOG(("END: ALT_SOUND_INIT\n"));
	return TRUE;
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_exit() {
	//DAR_TODO clean up internal storage?
	LOG(("BEGIN: ALT_SOUND_EXIT\n"));
	

	if (cached_machine_name != 0) // better free everything like above!
	{
		cached_machine_name[0] = '#';
		if (BASS_Free() == FALSE) {
			LOG(("FAILED: BASS_Free(): %s\n", get_bass_err()));
		}
		else {
			LOG(("SUCCESS: BASS_Free()\n"));
		}
	}

	
	LOG(("END: ALT_SOUND_EXIT\n"));
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_pause(BOOL pause) {
	LOG(("BEGIN: ALT_SOUND_PAUSE\n")); //DAR_DEBUG
	

	if (pause) {
		LOG(("PAUSING STREAM PLAYBACK (ALL)\n")); //DAR_DEBUG
		// NOTE: channel_steam[0] is reserved for uninitialized streams
	    // Pause all channels
		for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
			HSTREAM stream = processor->getStreams()[i].handle;
			if (stream != BASS_NO_STREAM
				&& BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING) {
				if (!BASS_ChannelPause(stream)) {
					LOG(("FAILED: BASS_ChannelPause(%u): %s\n", stream, get_bass_err())); //DAR_DEBUG
				}
				else {
					LOG(("SUCCESS: BASS_ChannelPause(%u)\n", stream)); //DAR_DEBUG
				}
			}
		}
	}
	else {
		LOG(("RESUMING STREAM PLAYBACK (ALL)\n")); //DAR_DEBUG
		// NOTE: channel_steam[0] is reserved for uninitialized streams
	   // Resume all channels
		for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
			HSTREAM stream = processor->getStreams()[i].handle;
			if (stream != BASS_NO_STREAM
				&& BASS_ChannelIsActive(stream) == BASS_ACTIVE_PAUSED) {
				if (!BASS_ChannelPlay(stream, 0)) {
					LOG(("FAILED: BASS_ChannelPlay(%u): %s\n", stream, get_bass_err())); //DAR_DEBUG
				}
				else {
					LOG(("SUCCESS: BASS_ChannelPlay(%u)\n", stream)); //DAR_DEBUG
				}
			}
		}
	}

	
	LOG(("END: ALT_SOUND_PAUSE\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void preprocess_commands(CmdData* cmds_out, int cmd_in) {
	LOG(("BEGIN: PREPROCESS_COMMANDS\n"));
	

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("MAME_GEN: %d\n", hardware_gen));

	// syntactic candy
	unsigned int* cmd_buffer = cmds_out->cmd_buffer;
	unsigned int* cmd_counter = &cmds_out->cmd_counter;
	unsigned int* cmd_filter = &cmds_out->cmd_filter;
	int* stored_command = &cmds_out->stored_command;

	// DAR_TODO what does this do?
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
				//DAR@20230518 I don't know why this is nerfing the
				//             volume.  In any case, going to wait until
				//             the end to apply it, so we can account for
				//             ducking without multiple volume changes
				//global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
				//					if (channel_0_stream != 0)
				//						BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

				//LOG(("Change volume %.2f\n", global_vol));
			}
			else
				LOG(("filtered command %02X %02X %02X %02X\n", cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]));

			for (int i = 0; i < ALT_MAX_CMDS; ++i) {
				cmd_buffer[i] = ~0;
			}

			*cmd_counter = 0;
			*cmd_filter = 1;
		}
		else
			*cmd_filter = 0;
	}

	//DAR_TODO what does this do?
	if ((hardware_gen == GEN_WPCALPHA_2) || //!! ?? test this gen actually
		(hardware_gen == GEN_WPCDMD) || // remaps everything to 16bit, a bit stupid maybe
		(hardware_gen == GEN_WPCFLIPTRON))
	{
		LOG(("Hardware Generation: GEN_WPCALPHA_2, GEN_WPCDMD, GEN_WPCFLIPTRON\n")); //DAR_DEBUG

		*cmd_filter = 0;
		if ((cmd_buffer[2] == 0x79) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
		{
			//DAR@20230518 I don't know why we are nerfing volume here.
			// In any case, I'm going to delay setting it until we know
			// the complete picture including ducking
			//global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
			//				if (channel_0_stream != 0)
			//					BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

			//LOG(("Change volume %.2f\n", global_vol));

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

	//DAR_TODO what do these do?
	if ((hardware_gen == GEN_WPCALPHA_1) || // remaps everything to 16bit, a bit stupid maybe //!! test all these generations!
		(hardware_gen == GEN_S11) ||
		(hardware_gen == GEN_S11X) ||
		(hardware_gen == GEN_S11B2) ||
		(hardware_gen == GEN_S11C))
	{
		LOG(("Hardware Generation: GEN_WPCALPHA_1, GEN_S11, GEN_S11X, GEN_S11B2, GEN_S11C\n")); //DAR_DEBUG

		if (cmd_in != cmd_buffer[1]) //!! some stuff is doubled or tripled -> filter out?
		{
			*stored_command = 0; // 8 bit command //!! 7F & 7E opcodes?
			*cmd_counter = 0;
		}
		else // ignore
			*cmd_counter = 1;
	}

	// DAR_TODO what does this do?
	if ((hardware_gen == GEN_DEDMD16) || // remaps everything to 16bit, a bit stupid maybe
		(hardware_gen == GEN_DEDMD32) ||
		(hardware_gen == GEN_DEDMD64) ||
		(hardware_gen == GEN_DE))        // this one just tested with BTTF so far
	{
		LOG(("Hardware Generation: GEN_DEDMD16, GEN_DEDMD32, GEN_DEDMD64, GEN_DE\n")); //DAR_DEBUG

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

	// DAR_TODO what do these do?
	if ((hardware_gen == GEN_WS) ||
		(hardware_gen == GEN_WS_1) ||
		(hardware_gen == GEN_WS_2))
	{
		LOG(("Hardware Generation: GEN_WS, GEN_WS_1, GEN_WS_2\n")); //DAR_DEBUG

		*cmd_filter = 0;
		if (cmd_buffer[1] == 0xFE)
		{
			if (cmd_in >= 0x10 && cmd_in <= 0x2F)
			{
				//DAR@20230518 I don't know why we are nerfing volume here.
				// In any case, I'm going to delay setting it until we know
				// the complete picture including ducking
				//global_vol = (float)(0x2F - cmd_in) / 31.f;
				//					if (channel_0_stream != 0)
				//						BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

				//LOG(("change volume %.2f\n", global_vol));

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

	
	LOG(("END: PREPROCESS_COMMANDS\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void postprocess_commands(const unsigned int combined_cmd) {
	LOG(("BEGIN: POSTPROCESS_COMMANDS\n")); //DAR_DEBUG
	

//	HSTREAM* cur_mus_stream = &channel_stream[mus_stream_idx];

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("MAME_GEN: %d\n", hardware_gen)); //DAR_DEBUG

		// DAR_TODO what does this do?
	if (hardware_gen == GEN_WPCDCS
		|| hardware_gen == GEN_WPCSECURITY
		|| hardware_gen == GEN_WPC95DCS
		|| hardware_gen == GEN_WPC95)
	{
		if (combined_cmd == 0x03E3/* && *cur_mus_stream != BASS_NO_STREAM*/) // stop music
		{
			LOG(("STOP MUSIC (2)\n")); //DAR_DEBUG
			processor->stop_music_stream();
		}
	}

	//!! old WPC machines music stop? -> 0x00 for SYS11?

	if (hardware_gen == GEN_DEDMD32)
	{
		if ((combined_cmd == 0x0018 || combined_cmd == 0x0023)
			/*&& *cur_mus_stream != BASS_NO_STREAM*/) // stop music //!! ???? 0x0019??
		{
			LOG(("STOP MUSIC (3)\n")); //DAR_DEBUG
			processor->stop_music_stream();
		}
	}

	if (hardware_gen == GEN_WS
		|| hardware_gen == GEN_WS_1
		|| hardware_gen == GEN_WS_2)
	{
		if (((combined_cmd == 0x0000 || (combined_cmd & 0xf0ff) == 0xf000))
			/*&& *cur_mus_stream != BASS_NO_STREAM*/) // stop music
		{
			LOG(("STOP MUSIC (4)\n")); //DAR_DEBUG
			processor->stop_music_stream();
		}
	}

	
	LOG(("END: POSTPROCESS_COMMANDS\n")); //DAR_DEBUG
}
