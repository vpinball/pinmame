// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

#include "snd_alt.h"

#include <mutex>
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
#include "altsound_csv_parser.hpp"
#include "altsound_file_parser.hpp"

static std::mutex io_mutex;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define ALT_MAX_CHANNELS 16
#define BASS_NO_STREAM 0
#define FILTERED_INCOMPLETE 999

// ---------------------------------------------------------------------------
// Game(ROM) globals
// ---------------------------------------------------------------------------

extern "C" char g_szGameName[256];
static char* cached_machine_name = 0;

// ---------------------------------------------------------------------------
// Sound stream globals
// ---------------------------------------------------------------------------

// NOTE:
// - channel_stream[0] is reserved for uninitialized streams
// - SFX streams don't require index tracking since multiple can play
//   simultaneously

// Music channel stream index
static unsigned int mus_stream_idx = 0;

// Jingle/Single channel stream index
static unsigned int jin_stream_idx = 0;

// Channel stream storage (all channels)
static HSTREAM channel_stream[ALT_MAX_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Storage for all channel duck values applied to music
static float channel_ducking[ALT_MAX_CHANNELS] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// ---------------------------------------------------------------------------
// Sound volume globals
// ---------------------------------------------------------------------------

static float music_vol = 1.0f;
static float master_vol = 1.0f;
static float global_vol = 1.0f;

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

extern "C" void alt_sound_handle(int boardNo, int cmd)
{
	LOG(("%s\nBEGIN: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
	
	LOG(("%sACQUIRING MUTEX\n", indent)); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("%sMUTEX ACQUIRED\n", indent)); //DAR_DEBUG
	INDENT;

	static CmdData cmds;
	static PinSamples psd;
	static BOOL init_once = TRUE;
	static BOOL init_successful = TRUE;
	static BOOL altsound_stable = TRUE; // future use

	//DAR@20230519 not sure what this does
	int	attenuation = osd_get_mastervolume();

	while (attenuation++ < 0) {
		master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB
	}
	LOG(("%sMaster Volume (Post Attenuation): %f\n ", indent, master_vol)); //DAR_DEBUG

	if (cached_machine_name != 0 && strstr(g_szGameName, cached_machine_name) == 0)
	{// A new game has been loaded, clear out old memory and re-initialize
		LOG(("%sNEW GAME LOADED: %s\n", indent, g_szGameName)); //DAR_DEBUG

		free(cached_machine_name);
		cached_machine_name = 0;

		init_once = TRUE; // allow initialization to occur
	}

	if (init_once) {
		init_once = FALSE; // prevent re-entry

		LOG(("%sCURRENT GAME: %s\n", indent, g_szGameName)); //DAR_DEBUG

		if (alt_sound_init(&cmds, &psd) == 0) {
			LOG(("%sSUCCESS: ALT_SOUND_INIT\n", indent));

			// Save current machine name
			cached_machine_name = (char*)malloc(strlen(g_szGameName) + 1);
			strcpy(cached_machine_name, g_szGameName);
		}
		else {
			LOG(("%sFAILED: ALT_SOUND_INIT\n", indent));
			init_successful = FALSE;
		}
	}

	if (!init_successful || !altsound_stable) {
		// Intialization failed, or a downstream error has made continuing
		// unsafe... exit
		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}

	cmds.cmd_counter++;

	//Shift all commands up to free up slot 0
	for (int i = ALT_MAX_CMDS - 1; i > 0; --i)
		cmds.cmd_buffer[i] = cmds.cmd_buffer[i - 1];

	cmds.cmd_buffer[0] = cmd; //add command to slot 0

	// pre-process commands based on ROM hardware platform
	preprocess_commands(&cmds, cmd);

	// storage for accumulated commands (8-bit -> 16-bit)
	unsigned int cmd_combined;

	// get sample for playback
	int sample_idx = get_sample(&cmds, &psd, cmd, &cmd_combined);
	if (sample_idx == FILTERED_INCOMPLETE) {
		LOG(("%sCOMMAND FILTERED/INCOMPLETE\n", indent));

		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}
	
	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		LOG(("%sFAILED: GET_SAMPLE(%04X): UNKNOWN, BOARD: %u\n", indent, cmd_combined, boardNo)); //DAR_DEBUG

		// DAR@20230520
		// Not sure why this is needed, but to preserve original functionality,
		// this needs to be called in this case, before we exit
		postprocess_commands(cmd_combined);

		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}
	LOG(("%sSUCCESS: GET_SAMPLE(%04X): BOARD: %u\n", indent, cmd_combined, boardNo));

	BOOL play_music  = FALSE;
	BOOL play_jingle = FALSE;
	BOOL play_sfx    = FALSE;

	HSTREAM stream = BASS_NO_STREAM;
	unsigned int ch_idx = 0;

	unsigned int sample_channel = psd.channel[sample_idx];
	if (sample_channel == 1) {
		// Command is for playing Jingle/Single
		if(process_jingle(&psd, sample_idx, &ch_idx)) {
			LOG(("%sSUCCESS: process_jingle()\n", indent)); //DAR_DEBUG
			
		    // Defer playback until the end
			play_jingle = TRUE;
			jin_stream_idx = ch_idx;
			stream = channel_stream[jin_stream_idx];
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: process_jingle()\n", indent)); //DAR_DEBUG
		}
	}

	if (sample_channel == 0) {
		// Command is for playing music
		if(process_music(&psd, sample_idx, &ch_idx)) {
			LOG(("%sSUCCESS: process_music()\n", indent)); //DAR_DEBUG
			
			// Defer playback until the end
			play_music = TRUE;
			mus_stream_idx = ch_idx;
			stream = channel_stream[mus_stream_idx];
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: process_music()\n", indent)); //DAR_DEBUG
		}
	}

	if (sample_channel == -1) {
		// Command is for playing voice/sfx
		if(process_sfx(&psd, sample_idx, &ch_idx)) {
			LOG(("%sSUCCESS: process_sfx()\n", indent)); //DAR_DEBUG
			
		    // Defer playback until the end
			play_sfx = TRUE;
			stream = channel_stream[ch_idx];
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: process_sfx()\n", indent)); //DAR_DEBUG
		}
	}

	if (!play_music && !play_jingle && !play_sfx) {
		LOG(("%sFAILED: alt_sound_handle()\n", indent)); //DAR_DEBUG

		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}

	// Get lowest ducking value from all channels (including music, jingle)
	float min_ducking = get_min_ducking();
	LOG(("%sMIN DUCKING VALUE: %.2f\n", indent, min_ducking)); //DAR_DEBUG

	HSTREAM cur_mus_stream = channel_stream[mus_stream_idx];
	HSTREAM cur_jin_stream = channel_stream[jin_stream_idx];

	// set new music volume	
	if (cur_mus_stream != BASS_NO_STREAM) {
		// calculate ducked volume for music
		float adj_mus_vol = music_vol * min_ducking;
	
		set_volume(cur_mus_stream, adj_mus_vol);
	}
	else {
		// No music is currently playing.  If a music file starts later,
		// the code above will ensure the volume will be set correctly
		LOG(("%sNO MUSIC STREAM. VOLUME NOT SET\n", indent)); //DAR_DEBUG
	}

	// Play pending sound determined above, if any
	if (stream != BASS_NO_STREAM) {
		if (!BASS_ChannelPlay(stream, 0)) {
			// Sound playback failed
			LOG(("%sFAILED: BASS_ChannelPlay(%u): %s\n", indent, stream, get_bass_err())); //DAR_DEBUG
		}
		else {
			LOG(("%sSUCCESS: BASS_ChannelPlay(%u): CH(%d) CMD(%04X) SAMPLE(%s)\n", \
				indent, stream, ch_idx, cmd_combined, \
				get_short_path(psd.files_with_subpath[sample_idx])));
		}
	}

	//DAR@20230522 I don't know what this does yet
	postprocess_commands(cmd_combined);

	OUTDENT;
	LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

int alt_sound_init(CmdData* cmds_out, PinSamples* psd_out) {
	LOG(("%sBEGIN: ALT_SOUND_INIT\n", indent));
	INDENT;

	int result = 0; // assume success at start

	global_vol = 1.0f;
	music_vol = 1.0f;
	master_vol = 1.0f;

	// intialize the command and sample data bookkeeping structures
	initialize_cmds(cmds_out);
	initialize_sample_data(psd_out);

	mus_stream_idx = 0;
	jin_stream_idx = 0;

	// It's OK to reinitialize channel_stream[0] and channel_ducking[0] here
	// initialize static channel structures
	for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
		channel_stream[i] = BASS_NO_STREAM;
		channel_ducking[i] = 1.f;
	}

	// Seed random number generator
	srand((unsigned int)time(NULL));

	int DSidx = -1; // BASS default device
	//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
	if (DSidx != -1)
		DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
	if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
	{
		LOG(("%sBASS initialization error: %s\n", indent, get_bass_err()));
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

	// Load sound samples
	if (load_samples(psd_out) == 0) {
		LOG(("%sSUCCESS: LOAD_SAMPLES\n", indent));
	}
	else {
		LOG(("%sFAILED: LOAD_SAMPLES\n", indent));
		psd_out->num_files = 0;
		result = 1;
	}

	OUTDENT;
	LOG(("%sEND: ALT_SOUND_INIT\n", indent));
	return result;
}

// ---------------------------------------------------------------------------

void initialize_cmds(CmdData* cmds_out) {
	LOG(("%sBEGIN: INITIALIZE_CMDS\n", indent));
	INDENT;

	cmds_out->cmd_counter = 0;
	cmds_out->stored_command = -1;
	cmds_out->cmd_filter = 0;
	for (int i = 0; i < ALT_MAX_CMDS; ++i)
		cmds_out->cmd_buffer[i] = ~0;

	OUTDENT;
	LOG(("%sEND: INITIALIZE_CMDS\n", indent));
}

// ---------------------------------------------------------------------------

void initialize_sample_data(PinSamples* psd_out) {
	LOG(("%sBEGIN: INITIALIZE_SAMPLE_DATA\n", indent));
	INDENT;

	for (int i = 0; i < psd_out->num_files; ++i)
	{
		free(psd_out->files_with_subpath[i]);
		psd_out->files_with_subpath[i] = 0;
	}

	free(psd_out->ID);
	free(psd_out->files_with_subpath);
	free(psd_out->gain);
	free(psd_out->ducking);
	free(psd_out->channel);
	free(psd_out->loop);
	free(psd_out->stop);

	psd_out->ID = 0;
	psd_out->files_with_subpath = 0;
	psd_out->gain = 0;
	psd_out->ducking = 0;
	psd_out->channel = 0;
	psd_out->loop = 0;
	psd_out->stop = 0;

	psd_out->num_files = 0;

	OUTDENT;
	LOG(("%sEND: INITIALIZE_SAMPLE_DATA\n", indent));
}

// ---------------------------------------------------------------------------

int get_sample(CmdData* cmds_in, const PinSamples* psd_in, int cmd_in,\
	                    unsigned int* cmd_combined_out) {
	LOG(("%sBEGIN: GET_SAMPLE\n", indent)); //DAR_DEBUG
	INDENT;

	//const unsigned int* cmd_buffer = cmds_in->cmd_buffer;
	const unsigned int cmd_counter = cmds_in->cmd_counter;
	const unsigned int cmd_filter = cmds_in->cmd_filter;
	int* stored_command = &cmds_in->stored_command;

	if (cmd_filter || (cmd_counter & 1) != 0) {
		// Some commands are 16-bits collected from two 8-bit commands.  If
		// the command is filtered or we have not received enough data yet,
		// try again on the next command
		//
		// NOTE:
		// Command size and filter requirements are ROM hardware platform
		// dependent.  The command preprocessor will take care of the
		// bookkeeping

		// Store the command for accumulation
		*stored_command = cmd_in;

		if (cmd_filter) {
			LOG(("%sCOMMAND FILTERED: %04X\n", indent, cmd_in)); //DAR_DEBUG
		}

		if ((cmd_counter & 1) != 0) {
			LOG(("%sCOMMAND INCOMPLETE: %04X\n", indent, cmd_in)); //DAR_DEBUG
		}

		OUTDENT;
		LOG(("%sEND: GET_SAMPLE\n", indent)); //DAR_DEBUG
		return FILTERED_INCOMPLETE;
	}
	LOG(("%sCOMMAND COMPLETE. FINDING SAMPLE(S)\n", indent)); //DAR_DEBUG

	// combine stored command with the current
	*cmd_combined_out = (*stored_command << 8) | cmd_in;

	unsigned int sample_idx = -1;

	// Look for sample that matches the current command
	for (int i = 0; i < psd_in->num_files; ++i) {
		if (psd_in->ID[i] == *cmd_combined_out) {
			// Current ID matches the command. Check if there are more samples for this
			// command and randomly pick one
			unsigned int num_samples = 0;
			do {
				num_samples++;

				if (i + num_samples >= psd_in->num_files)
					// sample index exceeds the number of samples
					break;

				// Loop while the next sample ID matches the command
			} while (psd_in->ID[i + num_samples] == *cmd_combined_out);
			LOG(("%sSUCCESS: FOUND %d SAMPLE(S) FOR ID: %04X\n", indent, num_samples, *cmd_combined_out)); //DAR_DEBUG

			// num_samples now contains the number of samples with the same ID
			// pick one to play at random
			sample_idx = i + rand() % num_samples;
			break;
		}
	}

	if (sample_idx == -1) {
		LOG(("%sFAILED: NO SAMPLE(S) FOUND FOR ID: %04X\n", indent, *cmd_combined_out)); //DAR_DEBUG
	}

	OUTDENT;
	LOG(("%sEND: GET_SAMPLE\n", indent)); //DAR_DEBUG
	return sample_idx;
}
// ----------------------------------------------------------------------------

BOOL process_music(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out) {
	LOG(("%sBEGIN: PROCESS_MUSIC\n", indent));
	INDENT;

	HSTREAM* cur_mus_stream = &channel_stream[mus_stream_idx];
	LOG(("%sCURRENT MUSIC STREAM: %u  CH: %02d\n", indent, *cur_mus_stream, mus_stream_idx)); //DAR_DEBUG

	if (*cur_mus_stream != BASS_NO_STREAM) {
		LOG(("%s STOPPING MUSIC STREAM: %u\n", indent, *cur_mus_stream)); //DAR_DEBUG

		// Music stream defined.  Stop it if still playing
		stop_stream(*cur_mus_stream);
		//DAR_TODO trap error?

		*cur_mus_stream = BASS_NO_STREAM;
		channel_ducking[mus_stream_idx] = 1.0f;
		mus_stream_idx = 0;
		music_vol = 1.0f;
	}

	char* fname = psd_in->files_with_subpath[sample_idx_in];
	int loops = psd_in->loop[sample_idx_in];
	unsigned int ch_idx = 0;

	// Create playback stream
	if (!create_stream(fname, loops, &music_callback, &ch_idx)) {
		// Failed to create stream
		LOG(("%sFAILED: create_stream(): %s\n", indent, get_bass_err()));

		OUTDENT;
		LOG(("%sEND: PROCESS_MUSIC\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	LOG(("%sSUCCESS: create_stream()\n", indent)); //DAR_DEBUG

	// Everything is good.  Update output
	*ch_idx_out = ch_idx;

	// Grab newly created stream
	HSTREAM mus_stream = channel_stream[*ch_idx_out];

	// set ducking for music
	//channel_ducking[mus_stream_idx] = psd_in->ducking[index_in];
	channel_ducking[*ch_idx_out] = 1.f; // no ducking
	LOG(("%sMUSIC DUCKING: %.2f\n", indent, channel_ducking[*ch_idx_out])); //DAR_DEBUG

	// Set music volume.  This will get ducked as needed in the main handler
	LOG(("%sSETTING MUSIC VOLUME\n", indent)); //DAR_DEBUG
	music_vol = psd_in->gain[sample_idx_in]; // updates global storage
	set_volume(mus_stream, music_vol);

	OUTDENT;
	LOG(("%sEND: PROCESS_MUSIC\n", indent));
	return TRUE;
}

// ---------------------------------------------------------------------------

BOOL process_jingle(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out) {
	LOG(("%sBEGIN: PROCESS_JINGLE\n", indent)); //DAR_DEBUG
	INDENT;

	HSTREAM* cur_jin_stream = &channel_stream[jin_stream_idx];
	HSTREAM* cur_mus_stream = &channel_stream[mus_stream_idx];
	LOG(("%sCURRENT JINGLE STREAM: %u  CH: %02d\n", indent, *cur_jin_stream, jin_stream_idx)); //DAR_DEBUG
	LOG(("%sCURRENT MUSIC STREAM: %u  CH: %02d\n", indent, *cur_mus_stream, mus_stream_idx)); //DAR_DEBUG

	if (*cur_jin_stream != BASS_NO_STREAM) {
		LOG(("%sSTOPPING JINGLE STREAM: %u\n", indent, *cur_jin_stream)); //DAR_DEBUG

		// Jingle/Single already playing.  Stop it
		if (!stop_stream(*cur_jin_stream)) {
			LOG(("&sFAILED: stop_stream(%u)\n", indent, *cur_jin_stream));

			OUTDENT;
			LOG(("%sEND: PROCESS_JINGLE\n", indent)); //DAR_DEBUG
			return FALSE;
		}
		LOG(("%sSUCCESS: stop_stream(%u)\n", indent, *cur_jin_stream));

		*cur_jin_stream = BASS_NO_STREAM;
		channel_ducking[jin_stream_idx] = 1.0f;
		jin_stream_idx = 0;
	}

	if (*cur_mus_stream != BASS_NO_STREAM) {
		// Music stream playing.  Pause/stop it
		if (psd_in->stop[sample_idx_in] != 0) {
			// STOP field is set, stop music
			LOG(("%sSTOP FIELD SET\n", indent)); //DAR_DEBUG
			LOG(("%s STOPPING MUSIC STREAM: %u\n", indent, *cur_mus_stream)); //DAR_DEBUG
			
			stop_stream(*cur_mus_stream);
			//DAR_TODO trap error?

			*cur_mus_stream = BASS_NO_STREAM;
			channel_ducking[mus_stream_idx] = 1.0f;
			mus_stream_idx = 0;
			music_vol = 1.0f;
		}
		else if (psd_in->ducking[sample_idx_in] < 0.f) {
			// Ducking is less than zero.  Pause music
			LOG(("%sPAUSING MUSIC STREAM\n", indent)); //DAR_DEBUG
			
			if (!BASS_ChannelPause(*cur_mus_stream)) {
				LOG(("%sFAILED: BASS_ChannelPause(%u): %s\n", indent, *cur_mus_stream, get_bass_err())); //DAR_DEBUG
			}
			else {
				LOG(("%sSUCCESS: BASS_ChannelPause(%u)\n", indent, *cur_mus_stream)); //DAR_DEBUG
			}
		}
		else {
			// not stopping or pausing.  Process ducking as usual, which is
			// handled in alt_sound_handle()
		}
	}

	char* fname = psd_in->files_with_subpath[sample_idx_in];
	int loops = psd_in->loop[sample_idx_in];
	unsigned int ch_idx = 0;

	// Create playback stream
	if (!create_stream(fname, loops, &jingle_callback, &ch_idx)) {
		LOG(("%sFAILED: create_stream(): %s\n", indent, get_bass_err()));

		OUTDENT;
		LOG(("%sEND: PROCESS_JINGLE\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	LOG(("%sSUCCESS: create_stream()\n", indent));

	// Everything is good.  Update output
	*ch_idx_out = ch_idx;

	// Grab newly created stream
	HSTREAM jin_stream = channel_stream[*ch_idx_out];

	// set ducking for channel
	channel_ducking[*ch_idx_out] = psd_in->ducking[sample_idx_in];
	LOG(("%sJINGLE DUCKING: %.2f\n", indent, channel_ducking[*ch_idx_out])); //DAR_DEBUG

	// Set volume for jingle/single
	LOG(("%sSETTING JINGLE VOLUME\n", indent)); //DAR_DEBUG
	set_volume(jin_stream, psd_in->gain[sample_idx_in]);

	OUTDENT;
	LOG(("%sEND: PROCESS_JINGLE\n", indent)); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

BOOL process_sfx(PinSamples* psd_in, int sample_idx_in, unsigned int* ch_idx_out) {
	LOG(("%sBEGIN: PROCESS_SFX\n", indent)); //DAR_DEBUG
	INDENT;

	char* fname = psd_in->files_with_subpath[sample_idx_in];
	int loops = psd_in->loop[sample_idx_in];
	unsigned int ch_idx = 0;

	// create playback stream
	if(!create_stream(fname, loops, &sfx_callback, &ch_idx)) {
		LOG(("%sFAILED: create_stream(): %s\n", indent, get_bass_err()));

		OUTDENT;
		LOG(("%sEND: PROCESS_SFX\n", indent)); //DAR_DEBUG
		return FALSE;
	}
    LOG(("%sSUCCESS: create_stream()\n", indent)); //DAR_DEBUG

	// Everything is good.  Update output
	*ch_idx_out = ch_idx;

	// Grab newly created stream
	HSTREAM sfx_stream = channel_stream[*ch_idx_out];
    
	// set ducking for sfx/voice
	channel_ducking[*ch_idx_out] = psd_in->ducking[sample_idx_in];
	LOG(("%sSFX/VOICE DUCKING: %.2f\n", indent, channel_ducking[*ch_idx_out])); //DAR_DEBUG

	// Set volume for sfx/voice
	LOG(("%sSETTING SFX VOLUME\n", indent)); //DAR_DEBUG
	set_volume(sfx_stream, psd_in->gain[sample_idx_in]);

	OUTDENT;
	LOG(("%sEND: PROCESS_SFX\n", indent)); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

BOOL create_stream(const char* fname_in, const int loop_count_in, SYNCPROC* syncproc_in,\
                   unsigned int* ch_idx_out)
{
	LOG(("%sBEGIN: CREATE STREAM\n", indent)); //DAR_DEBUG
	INDENT;

	// Find an available channel
	unsigned int ch_idx = find_free_channel();

	if (!ch_idx) {
		LOG(("%sFAILED: find_free_channel()\n", indent)); //DAR_DEBUG

		OUTDENT;
		LOG(("%sEND: CREATE_STREAM\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	LOG(("%sSUCCESS: find_free_channel(): %d\n", indent, ch_idx)); //DAR_DEBUG

	const char* short_fname = get_short_path(fname_in);

	//  DAR@20230517
	// This seems to suggest that either we loop BASS_SAMPLE_LOOP times ONLY
	// if the LOOP field is set to 100, otherwise don't loop at all.  Why?
	//
	// Create playback stream
	HSTREAM ch_stream = BASS_StreamCreateFile(FALSE, fname_in, 0, 0,\
		                                      (loop_count_in == 100) ? BASS_SAMPLE_LOOP : 0);
	if (ch_stream == BASS_NO_STREAM) {
		// Failed to create stream
		LOG(("%sFAILED: BASS_StreamCreateFile(%s): %s\n", indent,\
			 get_short_path(fname_in), get_bass_err()));

		// clean up
		free_stream(ch_stream);

		OUTDENT;
		LOG(("%sEND: CREATE STREAM\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	LOG(("%sSUCCESS: BASS_StreamCreateFile(%s): %u\n", indent, get_short_path(fname_in),\
		 ch_stream)); //DAR_DEBUG

	if (syncproc_in) {
		// Set callback to execute when sample playback ends
		HSYNC new_hsync = create_sync(ch_stream, syncproc_in, ch_idx);

		if (new_hsync == 0) {
			LOG(("%sFAILED: create_sync()\n", indent));

			// clean up
			free_stream(ch_stream);

			OUTDENT;
			LOG(("%sEND: CREATE STREAM\n", indent)); //DAR_DEBUG
			return FALSE;
		}
		else {
			LOG(("%sSUCCESS: create_sync()\n", indent)); //DAR_DEBUG
		}
	}

	// Everything is good.  Update external data
	*ch_idx_out = ch_idx;
	channel_stream[*ch_idx_out] = ch_stream;

	OUTDENT;
	LOG(("%sEND: CREATE STREAM\n", indent)); //DAR_DEBUG
	return TRUE;
}

// ----------------------------------------------------------------------------

HSYNC create_sync(HSTREAM stream_in, SYNCPROC* syncproc_in, \
	              unsigned int stream_idx_in) {
	LOG(("%sBEGIN: CREATE_SYNC\n", indent)); //DAR_DEBUG
	INDENT;

	// Set callback to execute when sample playback ends
	HSYNC new_hsync = BASS_ChannelSetSync(stream_in, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, \
		                                  syncproc_in, (void*)stream_idx_in);
	if (new_hsync == 0) {
		LOG(("%sFAILED: BASS_ChannelSetSync(): STREAM: %u ERROR: %s\n", indent, stream_in,\
			 get_bass_err()));

		OUTDENT;
		LOG(("%sEND: CREATE_SYNC\n", indent)); //DAR_DEBUG
		return 0;
	}
	else {
		LOG(("%sSUCCESS: BASS_ChannelSetSync(): STREAM: %u  HSYNC: %u\n", indent, stream_in, new_hsync)); //DAR_DEBUG
	}

	OUTDENT;
	LOG(("%sEND: CREATE_SYNC\n", indent)); //DAR_DEBUG
	return new_hsync;
}

// ---------------------------------------------------------------------------

BOOL stop_stream(HSTREAM stream_in) {
	LOG(("%sBEGIN: STOP_STREAM\n", indent)); //DAR_DEBUG
	INDENT;

	if (!BASS_ChannelStop(stream_in)) {
		LOG(("%sFAILED: BASS_ChannelStop(%u): %s\n", indent, stream_in, get_bass_err())); //DAR_DEBUG

		OUTDENT;
		LOG(("%sEND: STOP_STREAM\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	else {
		LOG(("%sSUCCESS: BASS_ChannelStop(%u)\n", indent, stream_in)); //DAR_DEBUG
	}

	// Free BASS stream
	// Note: this call also removes any predefined BASS sync
	if (!free_stream(stream_in)) {
		LOG(("%sFAILED: free_stream(%u)\n", indent, stream_in));

		OUTDENT;
		LOG(("%sEND: STOP_STREAM\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	LOG(("%sSUCCESS: free_stream(%u)\n", indent, stream_in));

	OUTDENT;
	LOG(("%sEND: STOP_STREAM\n", indent)); //DAR_DEBUG

	return TRUE;
}

// ---------------------------------------------------------------------------

BOOL free_stream(HSTREAM stream_in) {
	LOG(("%sBEGIN: FREE_STREAM\n", indent)); //DAR_DEBUG
	INDENT;

	if (!BASS_StreamFree(stream_in)) {
		LOG(("%sFAILED: BASS_StreamFree(%u): %s\n", indent, stream_in, get_bass_err())); //DAR_DEBUG

		OUTDENT;
		LOG(("%sEND: FREE_STREAM\n", indent)); //DAR_DEBUG
		return FALSE;
	}
	else {
		LOG(("%sSUCCESS: BASS_StreamFree(%u)\n", indent, stream_in)); //DAR_DEBUG
	}

	OUTDENT;
	LOG(("%sEND: FREE_STREAM\n", indent)); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

int set_volume(HSTREAM stream_in, const float vol_in) {
	LOG(("%sBEGIN: SET_VOLUME\n", indent)); //DAR_DEBUG
	INDENT;

	float new_vol = vol_in * global_vol * master_vol;
	LOG(("%sVOL: %.2f (GAIN: %.2f  GLOBAL_VOL: %.2f  MASTER_VOL %.2f)\n", indent, new_vol, vol_in, global_vol, master_vol)); //DAR_DEBUG

	if (!BASS_ChannelSetAttribute(stream_in, BASS_ATTRIB_VOL, new_vol)) {
		LOG(("%sFAILED: BASS_ChannelSetAttribute(%u, BASS_ATTRIB_VOL, %.2f): %s\n", indent, stream_in, new_vol, get_bass_err()));
		//DAR_TODO trap error?
	}
	else {
		LOG(("%sSUCCESS: BASS_ChannelSetAttribute(%u, BASS_ATTRIB_VOL, %.2f)\n", indent, stream_in, new_vol)); //DAR_DEBUG
	}

	OUTDENT;
	LOG(("%sEND: SET_VOLUME\n", indent)); //DAR_DEBUG
	return 0;
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_exit() {
	//DAR_TODO clean up internal storage?
	LOG(("%sBEGIN: ALT_SOUND_EXIT\n", indent));
	INDENT;

	if (cached_machine_name != 0) // better free everything like above!
	{
		free(cached_machine_name);
		cached_machine_name[0] = '#';
		if (BASS_Free() == FALSE) {
			LOG(("%sFAILED: BASS_Free(): %s\n", indent, get_bass_err()));
		}
		else {
			LOG(("%sSUCCESS: BASS_Free()\n", indent));
		}
	}

	OUTDENT;
	LOG(("%sEND: ALT_SOUND_EXIT\n", indent));
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_pause(BOOL pause) {
	LOG(("%sBEGIN: ALT_SOUND_PAUSE\n", indent)); //DAR_DEBUG
	INDENT;

	if (pause) {
		LOG(("%sPAUSING STREAM PLAYBACK (ALL)\n", indent)); //DAR_DEBUG
		// NOTE: channel_steam[0] is reserved for uninitialized streams
	    // Pause all channels
		for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
			HSTREAM stream = channel_stream[i];
			if (stream != BASS_NO_STREAM
				&& BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING) {
				if (!BASS_ChannelPause(stream)) {
					LOG(("%sFAILED: BASS_ChannelPause(%u): %s\n", indent, stream, get_bass_err())); //DAR_DEBUG
				}
				else {
					LOG(("%sSUCCESS: BASS_ChannelPause(%u)\n", indent, stream)); //DAR_DEBUG
				}
			}
		}
	}
	else {
		LOG(("%sRESUMING STREAM PLAYBACK (ALL)\n", indent)); //DAR_DEBUG
		// NOTE: channel_steam[0] is reserved for uninitialized streams
	   // Resume all channels
		for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
			HSTREAM stream = channel_stream[i];
			if (stream != BASS_NO_STREAM
				&& BASS_ChannelIsActive(stream) == BASS_ACTIVE_PAUSED) {
				if (!BASS_ChannelPlay(stream, 0)) {
					LOG(("%sFAILED: BASS_ChannelPlay(%u): %s\n", indent, stream, get_bass_err())); //DAR_DEBUG
				}
				else {
					LOG(("%sSUCCESS: BASS_ChannelPlay(%u)\n", indent, stream)); //DAR_DEBUG
				}
			}
		}
	}

	OUTDENT;
	LOG(("%sEND: ALT_SOUND_PAUSE\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

float get_min_ducking() {
	LOG(("%sBEGIN: GET_MIN_DUCKING\n", indent)); //DAR_DEBUG
	INDENT;

	float min_ducking = 1.f;
	int num_x_streams = 0;

	// NOTE: channel_steam[0] is reserved for uninitialized streams
	for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i] != BASS_NO_STREAM) {
			// stream is playing on the channel
			LOG(("%sCHANNEL_STREAM[%d]: ACTIVE\n", indent, i)); //DAR_DEBUG
			num_x_streams++;

			float channel_duck = channel_ducking[i];
			LOG(("%sCHANNEL_DUCKING[%d]: %.2f\n", indent, i, channel_duck)); //DAR_DEBUG

			// update new_ducking if needed
			if (channel_duck < 0) {
				// Special case.  Jingle ducking < 0 pauses music stream.
				// We don't want it to influence ducking of the rest of
				// the samples.
				LOG(("%sDUCKING < 0. SKIPPING...\n", indent)); //DAR_DEBUG
				continue;
			}
			else if (channel_duck < min_ducking) {
				min_ducking = channel_duck;
			}
		}
	}
	LOG(("%sNUM STREAMS ACTIVE: %d\n", indent, num_x_streams));
	LOG(("%sMIN DUCKING OF ALL ACTIVE CHANNELS: %.2f\n", indent, min_ducking)); //DAR_DEBUG

	OUTDENT;
	LOG(("%sEND: GET_MIN_DUCKING\n", indent)); //DAR_DEBUG
	return min_ducking;
}

// ---------------------------------------------------------------------------

void CALLBACK jingle_callback(HSYNC handle, DWORD channel, DWORD data, void *user) {
	// All BASS sync function run on the same thread.  A running SYNCPROC will
	// block others from running, so these functions should be fast.
	LOG(("%s\nBEGIN: JINGLE_CALLBACK\n", indent));

	LOG(("%sACQUIRING MUTEX\n", indent)); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("%sMUTEX ACQUIRED\n", indent)); //DAR_DEBUG
	INDENT;

	unsigned int ch_idx = (unsigned int)user;

	if (ch_idx != jin_stream_idx) {
		// This is not a jingle stream.  Get out
		LOG(("%sCH(%02d) is not a JINGLE stream.  Exiting.\n", indent, ch_idx)); //DAR_DEBUG

		OUTDENT;
		LOG(("%s\nEND: JINGLE_CALLBACK\n", indent));
		return;
	}

	HSTREAM *finished_stream = &channel_stream[ch_idx];
	LOG(("%sHANDLE: %u  CHANNEL: %u  DATA: %u  USER: %02d\n", indent, handle, channel, data,\
		 ch_idx)); //DAR_DEBUG
	LOG(("%sJINGLE STREAM(%u) FINISHED ON CH(%02d)\n", indent, *finished_stream, ch_idx)); //DAR_DEBUG


	// free stream resources
	if (!free_stream(*finished_stream)) {
		LOG(("%sFAILED: free_stream(%u): %s\n", indent, *finished_stream, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("%sSUCCESS: free_stream(%u)\n", indent, *finished_stream)); //DAR_DEBUG
	}

	// reset tracking variables
	*finished_stream = BASS_NO_STREAM;
	channel_ducking[ch_idx] = 1.f;
	jin_stream_idx = 0;

	// re-calculate music ducking based on active channels.
	LOG(("%sADJUSTING MUSIC VOLUME\n", indent)); //DAR_DEBUG

	if (mus_stream_idx == 0) {
		LOG(("%sNO MUSIC STREAM.  SKIPPING...\n", indent));

		OUTDENT;
		LOG(("%sEND: JINGLE_CALBACK\n", indent)); //DAR_DEBUG
		return;
	}

	float min_ducking = get_min_ducking();
	LOG(("%sMIN DUCKING VALUE: %.2f\n", indent, min_ducking)); //DAR_DEBUG

	HSTREAM cur_mus_stream = channel_stream[mus_stream_idx];
	float adj_mus_vol = music_vol * min_ducking;
	set_volume(cur_mus_stream, adj_mus_vol);

	// Resume paused music playback
	if (BASS_ChannelIsActive(cur_mus_stream) == BASS_ACTIVE_PAUSED) {
		LOG(("%sRESUMING MUSIC PLAYBACK\n", indent)); //DAR_DEBUG

		if (!BASS_ChannelPlay(cur_mus_stream, 0)) {
			LOG(("%sFAILED: BASS_ChannelPlay(%u): %s\n", indent, cur_mus_stream,\
				  get_bass_err())); //DAR_DEBUG
		}
		else {
			LOG(("%sSUCCESS: BASS_ChannelPlay(%u)\n", indent, cur_mus_stream)); //DAR_DEBUG
		}
	}

	OUTDENT;
	LOG(("%sEND: JINGLE_CALLBACK\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK sfx_callback(HSYNC handle, DWORD channel, DWORD data, void *user) {
	// All BASS sync function run on the same thread.  A running SYNCPROC will
	// block others from running, so these functions should be fast.
	LOG(("%s\nBEGIN: SFX_CALLBACK\n", indent));
	
	LOG(("%sACQUIRING MUTEX\n", indent)); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("%sMUTEX ACQUIRED\n", indent)); //DAR_DEBUG
	INDENT;
	
	unsigned int ch_idx = (unsigned int)user;

	if (ch_idx == mus_stream_idx || ch_idx == jin_stream_idx) {
		// This is not a jingle stream.  Get out
		LOG(("%sCH(%02d) is not an SFX stream.  Exiting.\n", indent, ch_idx)); //DAR_DEBUG

		OUTDENT;
		LOG(("%s\nEND: SFX_CALLBACK\n", indent));
		return;
	}

	HSTREAM* finished_stream = &channel_stream[ch_idx];
	LOG(("%sHANDLE: %u  CHANNEL: %u  DATA: %u  USER: %02d\n", indent, handle, channel,\
		 data, ch_idx)); //DAR_DEBUG
	LOG(("%sSFX STREAM(%u) FINISHED ON CH(%02d)\n", indent, *finished_stream, ch_idx)); //DAR_DEBUG
	
	// free stream resources
	if (!free_stream(*finished_stream)) {
		LOG(("%sFAILED: free_stream(%u): %s\n", indent, *finished_stream, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("%sSUCCESS: free_stream(%u)\n", indent, *finished_stream)); //DAR_DEBUG
	}

	// reset tracking variables
	*finished_stream = BASS_NO_STREAM;
	channel_ducking[ch_idx] = 1.f;
	
	// re-calculate music ducking based on active channels.
	LOG(("%sADJUSTING MUSIC VOLUME\n", indent)); //DAR_DEBUG

	if (mus_stream_idx == 0) {
		LOG(("%sNO MUSIC STREAM.  SKIPPING...\n", indent));

		OUTDENT;
		LOG(("%sEND: SFX_CALBACK\n", indent)); //DAR_DEBUG
		return;
	}

	float min_ducking = get_min_ducking();
	LOG(("%sMIN DUCKING VALUE: %.2f\n", indent, min_ducking)); //DAR_DEBUG

	HSTREAM cur_mus_stream = channel_stream[mus_stream_idx];
	float adj_mus_vol = music_vol * min_ducking;
	set_volume(cur_mus_stream, adj_mus_vol);

	OUTDENT;
	LOG(("%sEND: SFX_CALLBACK\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user) {
	// DAR@20230523
	// Normally, music is stopped programmatically in the main code.  This is
	// here in case a music stream ends before it is stopped by the main code.
	// This really should never happen.
	//
	//DAR@20230519 SYNCPROC functions run in a separate thread, and will block
	//             other sync processes, so these should be fast and care
	//             must be taken for thread safety with the main thread
	LOG(("%sBEGIN: MUSIC_CALLBACK\n", indent)); //DAR_DEBUG

	LOG(("%sACQUIRING MUTEX\n", indent)); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("%sMUTEX ACQUIRED\n", indent)); //DAR_DEBUG
	INDENT;

	unsigned int ch_idx = (unsigned int)user;

	if (ch_idx != mus_stream_idx) {
		// This is not a music stream.  Get out
		LOG(("%sCH(%02d) is not a MUSIC stream.  Exiting.\n", indent, ch_idx)); //DAR_DEBUG

		OUTDENT;
		LOG(("%s\nEND: MUSIC_CALLBACK\n", indent));
		return;
	}

	HSTREAM *finished_stream = &channel_stream[ch_idx];
	LOG(("%sHANDLE: %u  CHANNEL: %u  DATA: %u  USER: %02d\n", indent, handle, channel,\
		 data, ch_idx)); //DAR_DEBUG
	LOG(("%sMUSIC STREAM(%u) FINISHED ON CH(%02d)\n", indent, *finished_stream, ch_idx)); //DAR_DEBUG

	// free stream resources
	if (!free_stream(*finished_stream)) {
		LOG(("%sFAILED: free_stream(%u): %s\n", indent, *finished_stream, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("%sSUCCESS: free_stream(%u)\n", indent, *finished_stream)); //DAR_DEBUG
	}

	// reset tracking variables
	*finished_stream = BASS_NO_STREAM;
	channel_ducking[ch_idx] = 1.f;
	mus_stream_idx = 0;
	music_vol = 1.f;

	OUTDENT;
	LOG(("%sEND: MUSIC_CALLBACK\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

int load_samples(PinSamples* psd) {
	LOG(("%sBEGIN: LOAD_SAMPLES\n", indent)); //DAR_DEBUG
	INDENT;

	// No files yet
	psd->num_files = 0;

	// Try to load altsound lookup table from .csv if available
	AltsoundCsvParser csv_parser(g_szGameName);
	if (csv_parser.parse(psd)) {
		LOG(("%sSUCCESS: csv_parser.parse()\n", indent)); //DAR_DEBUG
	}
	else {
		AltsoundFileParser file_parser(g_szGameName);
		if (!file_parser.parse(psd)) {
			LOG(("%sFAILED: file_parser.parse()\n")); //DAR_DEBUG

			OUTDENT;
			LOG(("%sEND: LOAD_SAMPLES\n", indent)); //DAR_DEBUG
			return 1;
		}
		else {
			LOG(("%sSUCCESS: file_parser.parse()\n", indent)); //DAR_DEBUG
		}
	}
	LOG(("%sALTSOUND SAMPLES SUCCESSFULLY PARSED\n", indent));

	OUTDENT;
	LOG(("%sEND: LOAD_SAMPLES\n", indent)); //DAR_DEBUG
	return 0;
}

// ---------------------------------------------------------------------------

int find_free_channel(void) {
	LOG(("%sBEGIN: FIND_FREE_CHANNEL\n", indent));
	INDENT;

	int channel_idx = 0;

	// NOTE: channel_stream[0] is reserved for uninitialized streams
	for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i] == BASS_NO_STREAM) {
			channel_idx = i;
			break;
		}
	}

	if (channel_idx == 0) {
		LOG(("%sNO FREE CHANNELS AVAILABLE!\n", indent));
	}
	else {
		LOG(("%sFOUND FREE CHANNEL: %d\n", indent, channel_idx));
	}

	OUTDENT;
	LOG(("%sEND: FIND_FREE_CHANNEL\n", indent));
	return channel_idx;
}

// ---------------------------------------------------------------------------

void preprocess_commands(CmdData* cmds_out, int cmd_in) {
	LOG(("%sBEGIN: PREPROCESS_COMMANDS\n", indent));
	INDENT;

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("%sMAME_GEN: %d\n", indent, hardware_gen));

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
		LOG(("%sHardware Generation: GEN_WPCDCS, GEN_WPCSECURITY, GEN_WPC95DCS, GEN_WPC95\n", indent)); //DAR_DEBUG

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

				//LOG(("%sChange volume %.2f\n", indent, global_vol));
			}
			else
				LOG(("%sfiltered command %02X %02X %02X %02X\n", indent, cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]));

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
		LOG(("%sHardware Generation: GEN_WPCALPHA_2, GEN_WPCDMD, GEN_WPCFLIPTRON\n", indent)); //DAR_DEBUG

		*cmd_filter = 0;
		if ((cmd_buffer[2] == 0x79) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
		{
			//DAR@20230518 I don't know why we are nerfing volume here.
			// In any case, I'm going to delay setting it until we know
			// the complete picture including ducking
			//global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
			//				if (channel_0_stream != 0)
			//					BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

			//LOG(("%sChange volume %.2f\n", indent, global_vol));

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
		LOG(("%sHardware Generation: GEN_WPCALPHA_1, GEN_S11, GEN_S11X, GEN_S11B2, GEN_S11C\n", indent)); //DAR_DEBUG

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
		LOG(("%sHardware Generation: GEN_DEDMD16, GEN_DEDMD32, GEN_DEDMD64, GEN_DE\n", indent)); //DAR_DEBUG

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
		LOG(("%sHardware Generation: GEN_WS, GEN_WS_1, GEN_WS_2\n", indent)); //DAR_DEBUG

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

				//LOG(("%schange volume %.2f\n", indent, global_vol));

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
	LOG(("%sEND: PREPROCESS_COMMANDS\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void postprocess_commands(const unsigned int combined_cmd) {
	LOG(("%sBEGIN: POSTPROCESS_COMMANDS\n", indent)); //DAR_DEBUG
	INDENT;

	HSTREAM* cur_mus_stream = &channel_stream[mus_stream_idx];

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("%sMAME_GEN: %d\n", indent, hardware_gen)); //DAR_DEBUG

		// DAR_TODO what does this do?
	if (hardware_gen == GEN_WPCDCS
		|| hardware_gen == GEN_WPCSECURITY
		|| hardware_gen == GEN_WPC95DCS
		|| hardware_gen == GEN_WPC95)
	{
		if (combined_cmd == 0x03E3 && *cur_mus_stream != BASS_NO_STREAM) // stop music
		{
			LOG(("%sSTOP MUSIC (2)\n", indent)); //DAR_DEBUG
			stop_stream(*cur_mus_stream);

			*cur_mus_stream = BASS_NO_STREAM;
			mus_stream_idx = 0;
			music_vol = 1.0f;
		}
	}

	//!! old WPC machines music stop? -> 0x00 for SYS11?

	if (hardware_gen == GEN_DEDMD32)
	{
		if ((combined_cmd == 0x0018 || combined_cmd == 0x0023)
			&& *cur_mus_stream != BASS_NO_STREAM) // stop music //!! ???? 0x0019??
		{
			LOG(("%sSTOP MUSIC (3)\n", indent)); //DAR_DEBUG
			stop_stream(*cur_mus_stream);

			*cur_mus_stream = BASS_NO_STREAM;
			mus_stream_idx = 0;
			music_vol = 1.0f;
		}
	}

	if (hardware_gen == GEN_WS
		|| hardware_gen == GEN_WS_1
		|| hardware_gen == GEN_WS_2)
	{
		if (((combined_cmd == 0x0000 || (combined_cmd & 0xf0ff) == 0xf000))
			&& *cur_mus_stream != BASS_NO_STREAM) // stop music
		{
			LOG(("%sSTOP MUSIC (4)\n", indent)); //DAR_DEBUG
			stop_stream(*cur_mus_stream);

			*cur_mus_stream = BASS_NO_STREAM;
			mus_stream_idx = 0;
			music_vol = 1.0f;
		}
	}

	OUTDENT;
	LOG(("%sEND: POSTPROCESS_COMMANDS\n", indent)); //DAR_DEBUG
}
// ---------------------------------------------------------------------------
// Helper function to remove major path from filenames.  Returns just:
// <shortname>/<rest of path>
// ---------------------------------------------------------------------------

const char* get_short_path(const char* path_in) {
	const char* tmp_str = strstr(path_in, cached_machine_name);

	if (tmp_str) {
		return tmp_str;
	}
	else {
		return path_in;
	}
}

// ---------------------------------------------------------------------------
// Helper function to translate BASS error codes to printable strings
// ---------------------------------------------------------------------------

static const char * bass_err_names[] = {
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

const char* get_bass_err() {
	// returns string representation of BASS error codes
	int err = BASS_ErrorGetCode();
	if (err < 0) {
		return "BASS_ERROR_UNKNOWN";
	}
	else {
		return bass_err_names[err];
	}
}
