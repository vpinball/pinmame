// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

#include <time.h>
#include <dirent.h>
#include <unistd.h>

#include "gen.h"
#include "..\ext\bass\bass.h"

// ---------------------------------------------------------------------------
// Logging support
// ---------------------------------------------------------------------------

#ifdef VERBOSE //snd_command.c also defined this.  Causes warning without this
  #undef VERBOSE
  #define VERBOSE 1 //0 DAR_DEBUG
#endif

#if VERBOSE
  #ifdef LOG
    #undef LOG
    #define LOG(x) logerror x;
  #endif
  // allows for formatted log output
  #define FORMAT_OUTPUT 1 //0 DAR_DEBUG
#else
  #ifndef LOG
    #define LOG(x)
  #endif
  #define FORMAT_OUTPUT 0
#endif

#if FORMAT_OUTPUT
#define INDENT indent_level++; indent[0] = '\0'; \
  for (int i=0; i < indent_level; i++) {\
    indent[i] = '\t';\
    indent[i+1] = '\0';\
  }

#define OUTDENT indent_level--; indent_level = indent_level < 0?0:indent_level;\
  indent[0] = '\0'; \
  for (int i=0; i < indent_level; i++) {\
    indent[i] = '\t';\
	indent[i + 1] = '\0';\
  }

static int indent_level = 0;
static int indent_idx = 0;
static char* tab_str = "|    ";
#else
  #define INDENT
  #define OUTDENT
#endif

static char indent[100] = { '\0' }; // ensure null termination for empty string

// ---------------------------------------------------------------------------
// Altsound constants
// ---------------------------------------------------------------------------

#define CSV_MAX_LINE_LENGTH 512
#define CSV_SUCCESS 0
#define CSV_ERROR_NO_SUCH_FIELD -1
#define CSV_ERROR_HEADER_NOT_FOUND -2
#define CSV_ERROR_NO_MORE_RECORDS -3
#define CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE -4
#define CSV_ERROR_FILE_NOT_FOUND -5
#define CSV_ERROR_LINE_FORMAT -6
#define ALT_MAX_CMDS 4
#define ALT_MAX_CHANNELS 16
#define BASS_NO_STREAM 0
#define SFX_CHANNEL_START 2 // starting index for SFX channel

// ---------------------------------------------------------------------------
// Globals data structures
// ---------------------------------------------------------------------------

// Structure for sound file data
typedef struct _pin_samples {
	int * ID;
	char ** files_with_subpath;
	signed char * channel;
	float * gain;
	signed char * ducking;
	unsigned char * loop;
	unsigned char * stop;
	unsigned int num_files;
} PinSamples;

// Structure for CSV parsing
typedef struct _csv_reader {
	FILE* f;
	int delimiter;
	int n_header_fields;
	char** header_fields;	// header split in fields
	int n_fields;
	char** fields;			// current row split in fields
} CsvReader;

// Structure for command data
typedef struct _cmd_data {
	unsigned int cmd_counter;
	int stored_command;
	unsigned int cmd_filter;
	unsigned int cmd_buffer[ALT_MAX_CMDS];
} CmdData;

// ---------------------------------------------------------------------------
// Path globals
// ---------------------------------------------------------------------------

const char* path_main = "\\altsound\\";

// Storage for path to VPinMAME
char cvpmd[1024];

// Support for folder-based altsound
const char* path_jingle = "jingle\\";
const char* path_music  = "music\\";
const char* path_sfx    = "sfx\\";
const char* path_single = "single\\";
const char* path_voice  = "voice\\";

// Support for lookup-based altsound (CSV)
const char* path_table = "\\altsound.csv";

// ---------------------------------------------------------------------------
// Game(ROM) globals
// ---------------------------------------------------------------------------

extern char g_szGameName[256];
static char* cached_machine_name = 0;

// ---------------------------------------------------------------------------
// Sound stream globals
// ---------------------------------------------------------------------------

// Music channel stream index
static char ch0_stream_idx = 0;

// Jingle/Single channel stream index
static char ch1_stream_idx = 1;

// SFX/Voice channel stream index
static char chX_stream_idx = SFX_CHANNEL_START;

// Channel stream storage (all channels)
static HSTREAM channel_stream[ALT_MAX_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Storage for all channel duck values applied to channel 0
static signed char channel_ducking[ALT_MAX_CHANNELS] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

// ---------------------------------------------------------------------------
// Sound volume globals
// ---------------------------------------------------------------------------

static float channel_0_vol = 1.0f;
static float master_vol = 1.0f;
static float global_vol = 1.0f;

// ---------------------------------------------------------------------------
// CSV parsing function prototypes
// ---------------------------------------------------------------------------

CsvReader* csv_open(const char* const filename, const int delimiter);
void csv_close(CsvReader* const c);
int csv_read_header(CsvReader* const c);
int csv_read_record(CsvReader* const c);
int csv_get_colnumber_for_field(CsvReader* c, const char* fieldname);
int csv_get_hex_field(CsvReader* const c, const int field_index, int* pValue);
int csv_get_int_field(CsvReader* const c, const int field_index, int* pValue);
int parse_altsound_csv(size_t BASE_PATH_LEN, PinSamples* psd);

// ---------------------------------------------------------------------------
// Folder-based altsound parsing function prototypes
// ---------------------------------------------------------------------------

int parse_altsound_folders(size_t BASE_PATH_LEN, PinSamples* psd);

// ---------------------------------------------------------------------------

// BASS callback for cleaning up after jingle/sfx playback
void CALLBACK ducking_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

// BASS callback for cleaning up after music playback
void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

// Function to initialize all altound variables and data structures
int alt_sound_init(CmdData* cmds_out, PinSamples* psd_out);

// Function to parse altsound data and populate interal sample storage
int load_samples(PinSamples* pinsamples_out);

// Function to initialize command structure to default conditions
void initialize_cmds(CmdData* cmds_out);

// Function to initialize pinsound sample data
void initialize_sample_data(PinSamples* psd_out);

// Function to pre-process commands based on ROM hardware platform
void preprocess_commands(CmdData* cmds_out, int cmd);

// Return a free channel withing ALT_MAX_CHANNELS
int find_free_channel(void);

//DAR_TODO
// This is called after all other command processing.
// ? Does it matter?
// ? can it be called right after combining?
// ? If any of the 16-bit commands trigger channel 0 to stop, can the rest of
// the function be allowed to run?
// Function to process combined commands based on ROM hardware platform
void postprocess_commands(const unsigned int combined_cmd);

// Process command to play Jingle/Single
int process_channel1(PinSamples* psd_in, int index_in);

// Process command to play SFX/Voice
int process_channelX(PinSamples* psd_in, int index_in);

// Process command to play Music
int process_channel0(PinSamples* psd_in, int index_in);

// Helper function to parse BASS error codes to strings
static const char* bass_errstr(int err_val_in);
// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

void alt_sound_handle(int boardNo, int cmd)
{
	LOG(("%sBEGIN: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
	INDENT;

	static CmdData cmds;
	static PinSamples psd;
	static BOOL init_once = TRUE;
	static BOOL init_successful = TRUE;
	static BOOL altsound_stable = TRUE; // future use

	//DAR@20230519 not yet sure what this does
	int	attenuation = osd_get_mastervolume();

	while (attenuation++ < 0) {
		master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB
	}
	LOG(("%sMaster Volume (Post Attenuation): %f\n ", indent, master_vol)); //DAR_DEBUG

	if (cached_machine_name != 0 && strstr(g_szGameName, cached_machine_name) == 0)
	{// another game has been loaded? -> previous data has to be free'd
		LOG(("%sANOTHER GAME LOADED\n", indent)); //DAR_DEBUG

		free(cached_machine_name);
		cached_machine_name = 0;
 
		init_once = TRUE; // allow initialization to occur
	}

	if (init_once) {
		init_once = FALSE; // prevent re-entry

		if (alt_sound_init(&cmds, &psd) == 0) {
			LOG(("%sSUCCESS: ALT_SOUND_INIT\n", indent));
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

	//DAR@20230519 Why? Doesnt this affect the order of previously received commands?
	//Shift all commands up to free up slot 0
	for (int i = ALT_MAX_CMDS - 1; i > 0; --i)
		cmds.cmd_buffer[i] = cmds.cmd_buffer[i - 1];
	
	//DAR@20230519 Why? Doesnt that affect the order of previously received commands?
	cmds.cmd_buffer[0] = cmd; //add command to slot 0

	// pre-process commands based on ROM hardware platform
	preprocess_commands(&cmds, cmd);

	// syntactic candy
	unsigned int* cmd_buffer = cmds.cmd_buffer;
	unsigned int* cmd_counter = &cmds.cmd_counter;
	unsigned int* cmd_filter = &cmds.cmd_filter;
	int* stored_command = &cmds.stored_command;

	if (*cmd_filter || (*cmd_counter & 1) != 0)	{
		// Some commands are 16-bits collected from two 8-bit commands.  If
		// the command is filtered or we have not received enough data yet,
		// try again on the next command
		//
		// NOTE:
		// Command size and filter requirements are ROM hardware platform
		// dependent.  The command preprocessor will take care of the
		// bookkeeping

		// Store the command
		*stored_command = cmd;

		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}

	// combine stored command with the current
	const unsigned int cmd_combined = (*stored_command << 8) | cmd;

	// Look for sample that matches the current command
	unsigned int sample_idx = -1;
	for (int i = 0; i < psd.num_files; ++i) {
		if (psd.ID[i] == cmd_combined) {
			// Current ID matches the command. Check if there are more samples for this
			// command and randomly pick one
			unsigned int num_samples = 0;
			do {
				num_samples++;

				if (i + num_samples >= psd.num_files)
					// sample index exceeds the number of samples
					break;

				// Loop while the next sample ID matches the command
			} while (psd.ID[i + num_samples] == cmd_combined);

			// num_samples now contains the number of samples with the same ID
			// pick one to play at random
			sample_idx = i + rand() % num_samples;
			break;
		}
	}

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		LOG(("%sCMD: %04X unknown, BOARD: %u\n", indent, cmd_combined, boardNo));

		// DAR@20230520
        // Not sure why this is needed, but to preserve original functionality,
        // this needs to be called in this case, before we exit
		postprocess_commands(cmd_combined);

		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}

	BOOL play_music  = FALSE;
	BOOL play_jingle = FALSE;
	BOOL play_sfx    = FALSE;

	// Storage for the channel index about to play or -1 if there's an error
	int channel_idx = 0;

	if (psd.channel[sample_idx] == 1) {
		// Command is for playing Jingle/Single
		channel_idx = process_channel1(&psd, sample_idx);

		if (channel_idx != -1) {
			// Defer playback until the end
			play_jingle = TRUE;
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: PROCESS_CHANNEL1()\n", indent)); //DAR_DEBUG
		}
	}

	if (psd.channel[sample_idx] == 0) {
		// Command is for playing music
		channel_idx = process_channel0(&psd, sample_idx);

		if (channel_idx != -1) {
			// Defer playback until the end
			play_music = TRUE;
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: PROCESS_CHANNEL0()\n", indent)); //DAR_DEBUG
		}
	}

	if (psd.channel[sample_idx] == -1)	{
		// Command is for playing voice/sfx
		channel_idx = process_channelX(&psd, sample_idx);

		if (channel_idx != -1) {
			// Defer playback until the end
			play_sfx = TRUE;
		}
		else {
			// An error ocurred during processing
			LOG(("%sFAILED: PROCESS_CHANNELX()\n", indent)); //DAR_DEBUG
		}
	}

	if (channel_idx == -1) {
		LOG(("%sFAILED: ALT_SOUND_HANDLE()\n", indent)); //DAR_DEBUG
		
		OUTDENT;
		LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
		return;
	}

	LOG(("%sPREPARING TO DUCK CHANNEL 0\n", indent)); //DAR_DEBUG
	// Get lowest ducking value from all channels (including music, jingle)
	int min_ducking = 100;
	for (int i = 0; i < ALT_MAX_CHANNELS; i++) {
		if (channel_ducking[i] < min_ducking) {
			// Lower value, update current value
			min_ducking = channel_ducking[i];
		}
	}
	LOG(("%sMIN DUCKING VALUE: %d\n", indent, min_ducking)); //DAR_DEBUG

	HSTREAM* ch0_stream = &channel_stream[ch0_stream_idx];
	HSTREAM* ch1_stream = &channel_stream[ch1_stream_idx];
	HSTREAM* chX_stream = &channel_stream[chX_stream_idx];

	// set new music volume	
	if (*ch0_stream != BASS_NO_STREAM) {
		// calculate ducked volume for channel 0
		float adj_ch_0_vol = channel_0_vol * (float)((double)min_ducking / 100.);
		float new_vol = adj_ch_0_vol * global_vol * master_vol;

		LOG(("%sSETTING NEW CH0 VOLUME: %f\n", indent, new_vol)); //DAR_DEBUG
		if (!BASS_ChannelSetAttribute(*ch0_stream, BASS_ATTRIB_VOL, new_vol)) {
			LOG(("%sFAILED: BASS_ChannelSetAttribute(ch0_stream, BASS_ATTRIB_VOL)\n", indent)); //DAR_DEBUG
			//DAR_TODO get BASS error code
		}
		else {
			LOG(("%sSUCCESS: BASS_ChannelSetAttribute(ch0_stream, BASS_ATTRIB_VOL)", indent)); //DAR_DEBUG
		}
	}
	else {
		// Nothing is currently playing on CH0.  If a sample starts on
		// channel 0 later, the code above will ensure the volume will
		// be set correctly
		LOG(("%sNO CH0 STREAM. VOLUME SET SKIPPED\n", indent)); //DAR_DEBUG
	}

	// Play pending sound determined above, if any
	HSTREAM stream = play_music  ? *ch0_stream :
		             play_jingle ? *ch1_stream :
		             play_sfx    ? *chX_stream : BASS_NO_STREAM;
	
	if (stream != BASS_NO_STREAM) {
		LOG(("%sPlaying CH%d: cmd %04X gain %.2f duck %d %s\n", indent, channel_idx, cmd_combined, psd.gain[sample_idx], psd.ducking[sample_idx], psd.files_with_subpath[sample_idx]));
		if (!BASS_ChannelPlay(stream, 0)) {
			// Sound playback failed
			LOG(("%sFAILED: BASS_ChannelPlay()\n")); //DAR_DEBUG
			//DAR_TODO get BASS error code
		}
	}

    //DAR@20230522 I don't know what this does yet
	postprocess_commands(cmd_combined);

	OUTDENT;
	LOG(("%sEND: ALT_SOUND_HANDLE\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

int alt_sound_init(CmdData* cmds_out, PinSamples* psd_out)
{
	LOG(("%sBEGIN: ALT_SOUND_INIT\n", indent));
	INDENT;

	int result = 0; // assume success at start

	global_vol = 1.0f;
	channel_0_vol = 1.0f;

	// intialize the command and sample data bookkeeping structures
	initialize_cmds(cmds_out);
	initialize_sample_data(psd_out);

	// NOTE: CH0,1 Have fixed indexes. No need to reset
	chX_stream_idx = SFX_CHANNEL_START;

	// initialize static channel structures
	for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
		channel_stream[i] = BASS_NO_STREAM;
		channel_ducking[i] = 100;
	}

	// Seed random number generator
	srand((unsigned int)time(NULL));

	int DSidx = -1; // BASS default device
	//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
	if (DSidx != -1)
		DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
	if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
	{
		LOG(("%sBASS initialization error: %s\n", indent, bass_errstr(BASS_ErrorGetCode())));
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

void initialize_cmds(CmdData* cmds_out)
{
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

void initialize_sample_data(PinSamples* psd_out)
{
	LOG(("%sBEGIN: INITIALIZE_SAMPLE_DATA\n",indent));
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

int process_channel0(PinSamples* psd_in, int index_in)
{
	LOG(("%sBEGIN: PROCESS_CHANNEL0\n", indent));
	INDENT;

	// Stop existing playing for channel 0 if needed
	HSTREAM* ch0_stream = &channel_stream[ch0_stream_idx];

	if (*ch0_stream != BASS_NO_STREAM)
	{// Channel 0 stream defined.  Stop it if still playing
		
		if (!BASS_ChannelStop(*ch0_stream)) {
			LOG(("%sFAILED: BASS_ChannelStop(ch0_stream)\n", indent)); //DAR_DEBUG
			//DAR_TODO get BASS error code
		}
		LOG(("%sSTOP CHANNEL 0\n", indent)); //DAR_DEBUG
		
		// Free BASS stream
		// Note: this call also removes any predefined BASS sync
		if (!BASS_StreamFree(*ch0_stream)) {
			LOG(("%sFAILED: BASS_StreamFree(ch0_stream)\n", indent)); //DAR_DEBUG
			//DAR_TODO get BASS error code
		}

		*ch0_stream = BASS_NO_STREAM;
		channel_0_vol = 1.0f;
	}

	int channel_idx = 0; // music is always on channel 0
	LOG(("%sASSIGNED CHANNEL: %d\n", indent, channel_idx)); //DAR_DEBUG

	//DAR@20230517 This seems to suggest that either we loop BASS_SAMPLE_LOOP times ONLY if the LOOP field
	//             is set to 100, otherwise don't loop at all.  Why?
	// Create playback stream
	//DAR_TODO how is a comparison of a string (loop) and an int working? (probably not!)
	HSTREAM temp0_stream = BASS_StreamCreateFile(FALSE, psd_in->files_with_subpath[index_in], 0, 0, (psd_in->loop[index_in] == 100) ? BASS_SAMPLE_LOOP : 0);

	if (temp0_stream == BASS_NO_STREAM)
	{// Failed to create stream
		LOG(("%sFAILED: BASS_StreamCreateFile(temp0_stream)\n", indent));
		//DAR_TODO get bass error code
		OUTDENT;
		LOG(("END: PROCESS_CHANNEL0\n")); //DAR_DEBUG
		return -1;
	}
	LOG(("%sCH_0_STREAM_HANDLE: %u\n", indent, temp0_stream)); //DAR_DEBUG

	// Set callback to execute when sample playback ends
	HSYNC sync_handle = BASS_ChannelSetSync(temp0_stream, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, &music_callback, (void*)channel_idx);

	if (sync_handle == 0) {
		LOG(("%sFAILED: BASS_ChannelSetSync(temp0_stream)\n", indent));
		//DAR_TODO get BASS error code
		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
		return -1;
	}
	LOG(("%sSYNCH_HANDLE: %u\n", indent, sync_handle)); //DAR_DEBUG

	// Update bookkeeping
	channel_stream[channel_idx] = temp0_stream;
	ch0_stream_idx = channel_idx;
	ch0_stream = &channel_stream[channel_idx];

	// Set music volume.  This will likely get ducked in the main handler
	channel_0_vol = psd_in->gain[index_in]; //DAR_TODO is channel_0_vol necessary?
	float music_vol = psd_in->gain[index_in] * global_vol * master_vol;
	LOG(("%sMUSIC_VOL: %f  GAIN: %f  GLOBAL_VOL: %f  MASTER_VOL %f\n", indent, music_vol, psd_in->gain[index_in], global_vol, master_vol)); //DAR_DEBUG

	if (!BASS_ChannelSetAttribute(*ch0_stream, BASS_ATTRIB_VOL, music_vol)) {
		LOG(("%sFAILED: BASS_ChannelSetAttribute(ch0_stream, BASS_ATTRIB_VOL)\n", indent));
		//DAR_TODO get BASS error code
	}

	// set ducking for channel.
	//channel_ducking[channel_idx] = psd_in->ducking[index_in];
	channel_ducking[channel_idx] = 100; // no ducking
	LOG(("%sMUSIC DUCKING: %d\n", indent, channel_ducking[channel_idx])); //DAR_DEBUG

	OUTDENT;
	LOG(("%sEND: PROCESS_CHANNEL0\n",indent));
	return channel_idx;
}

// ---------------------------------------------------------------------------

int process_channel1(PinSamples* psd_in, int index_in)
{
	LOG(("%sBEGIN: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
	INDENT;

	HSTREAM* channel_1_stream = &channel_stream[ch1_stream_idx];
	HSTREAM* channel_0_stream = &channel_stream[ch0_stream_idx];

	if (*channel_1_stream != BASS_NO_STREAM)
	{// Jingle/Single already playing.  Stop it
		if (!BASS_ChannelStop(*channel_1_stream)) {
			LOG(("%sFAILED: BASS_ChannelStop()\n", indent)); //DAR_DEBUG
			//DAR_TODO get BASS error code

			OUTDENT;
			LOG(("%sEND: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
			return -1;
		}
		LOG(("%sSTOP CHANNEL 1 (1)\n", indent)); //DAR_DEBUG
		
		// Note: this call also removes any predefined BASS sync
		BASS_StreamFree(*channel_1_stream);
		*channel_1_stream = BASS_NO_STREAM;
	}

	// Is a sound playing on channel 0?
	if (*channel_0_stream != BASS_NO_STREAM)
	{// Yes, figure out what to do about it
		LOG(("%sCHANNEL 0 PLAYING MUSIC\n", indent)); //DAR_DEBUG

		if (psd_in->stop[index_in] == 0)
		{// STOP field not set, don't stop sound on channel 0
			if (psd_in->ducking[index_in] < 0)
			{// Yes, pause channel 0
				LOG(("%sCH1 Ducking Value < 0...Pausing Channel 0\n", indent)); //DAR_DEBUG
				BASS_ChannelPause(*channel_0_stream);
			}
		}
		else
		{// STOP field is set, stop sound on channel 0
			if (!BASS_ChannelStop(*channel_0_stream)) {
				LOG(("%sFAILED: BASS_ChannelStop()\n", indent)); //DAR_DEBUG
				//DAR_TODO get BASS error code

				OUTDENT;
				LOG(("%sEND: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
				return -1;
			}
			LOG(("%sSTOP CHANNEL 0 (5)\n", indent)); //DAR_DEBUG

			if (!BASS_StreamFree(*channel_0_stream)) {
				LOG(("%sFAILED: BASS_StreamFree()\n")); //DAR_DEBUG
				//DAR_TODO get BASS error code


			}
			*channel_0_stream = BASS_NO_STREAM;
			channel_0_vol = 1.0f;
		}
	}
	else
	{// No music playing on Channel 0
		LOG(("%sNO MUSIC PLAYING ON CHANNEL 0\n", indent)); //DAR_DEBUG
	}

	int channel_idx = 1; // jingle/single is always channel 1
	LOG(("%sASSIGNED CHANNEL: %d\n", indent, channel_idx)); //DAR_DEBUG

	//DAR@20230517 This seems to suggest that either we loop BASS_SAMPLE_LOOP times ONLY if the LOOP field
	//             is set to 100, otherwise don't loop at all.  Why?
	// Create playback stream
	//DAR_TODO how is a comparison of a string (loop) and an int working? (probably not!)
	*channel_1_stream = BASS_StreamCreateFile(FALSE, psd_in->files_with_subpath[index_in], 0, 0, (psd_in->loop[index_in] == 100) ? BASS_SAMPLE_LOOP : 0);

	if (*channel_1_stream == BASS_NO_STREAM)
	{// Failed to create stream
		LOG(("%sFAILED: BASS_StreamCreateFile()%s\n", indent));
		//DAR_TODO get BASS error code
		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
		return -1;
	}
	LOG(("%sCH_1_STREAM_HANDLE: %u\n", indent, *channel_1_stream)); //DAR_DEBUG

	// Set callback to execute when sample playback ends
	HSYNC sync_handle = BASS_ChannelSetSync(*channel_1_stream, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, &ducking_callback, (void*)channel_idx);

	if (sync_handle == 0) {
		LOG(("%sFAILED: BASS_ChannelSetSync()%s\n", indent));
		//DAR_TODO get BASS error code
		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
		return -1;
	}
	LOG(("%sSYNCH_HANDLE: %u\n", indent, sync_handle)); //DAR_DEBUG

	// Set volume for jingle/single
	float jingle_vol = psd_in->gain[index_in] * global_vol * master_vol;
	LOG(("%sJINGLE_VOL: %f  GAIN: %f  GLOBAL_VOL: %f  MASTER_VOL %f\n", indent, jingle_vol, psd_in->gain[index_in], global_vol, master_vol)); //DAR_DEBUG

	if (!BASS_ChannelSetAttribute(*channel_1_stream, BASS_ATTRIB_VOL, jingle_vol)) {
		LOG(("%sFAILED: BASS_ChannelSetAttribute()\n")); //DAR_DEBUG
		//DAR_TODO get BASS error code
	}

	// set ducking for channel
	channel_ducking[channel_idx] = psd_in->ducking[index_in];
	LOG(("%sJINGLE DUCKING: %d\n", indent, channel_ducking[channel_idx])); //DAR_DEBUG

	OUTDENT;
	LOG(("%sEND: PROCESS_CHANNEL1\n", indent)); //DAR_DEBUG
	return channel_idx;
}

// ---------------------------------------------------------------------------

int process_channelX(PinSamples* psd_in, int index_in)
{
	LOG(("%sBEGIN: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
	INDENT;

    // Find an available channel
	int channel_idx = find_free_channel();

	if (channel_idx == -1) {
		LOG(("%sFAILED: FIND_FREE_CHANNEL\n", indent)); //DAR_DEBUG

		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
		return channel_idx;
	}
	LOG(("%sASSIGNED CHANNEL: %d\n", indent, channel_idx)); //DAR_DEBUG
	
	// create stream for sample
	HSTREAM tempX_stream = BASS_StreamCreateFile(FALSE, psd_in->files_with_subpath[index_in], 0, 0, (psd_in->loop[index_in] == 100) ? BASS_SAMPLE_LOOP : 0);

	if (tempX_stream == BASS_NO_STREAM) {
		LOG(("%sFAILED: BASS_StreamCreateFile(tempX_stream)\n", indent));
		//DAR_TODO get BASS error code
		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
		return -1;
	}
	LOG(("%sSFX_STREAM_HANDLE: %u\n", indent, tempX_stream)); //DAR_DEBUG

	// Set callback to execute when sample playback ends
	HSYNC sync_handle = BASS_ChannelSetSync(tempX_stream, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, &ducking_callback, (void*)channel_idx);

	if (sync_handle == 0) {
		LOG(("%sFAILED: BASS_ChannelSetSync(tempX_stream)\n", indent));
		//DAR_TODO get BASS error code
		OUTDENT;
		LOG(("%sEND: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
		return -1;
	}
	LOG(("%sSYNCH_HANDLE: %u\n", indent, sync_handle)); //DAR_DEBUG

	// Update bookkeeping
	channel_stream[channel_idx] = tempX_stream;
	chX_stream_idx = channel_idx;
	HSTREAM* chX_stream = &channel_stream[channel_idx];

	// Set volume for sfx/voice
	float sfx_vol = psd_in->gain[index_in] * global_vol * master_vol;
	LOG(("%sSFX_VOL: %f  GAIN: %f  GLOBAL_VOL: %f  MASTER_VOL %f\n", indent, sfx_vol, psd_in->gain[index_in], global_vol, master_vol)); //DAR_DEBUG
	
	if (!BASS_ChannelSetAttribute(*chX_stream, BASS_ATTRIB_VOL, sfx_vol)) {
		LOG(("%sFAILED: BASS_ChannelSetAttribute(chX_stream, BASS_ATTRIB_VOL)\n")); //DAR_DEBUG
		//DAR_TODO get BASS error code
	}

	// set ducking for channel
	channel_ducking[channel_idx] = psd_in->ducking[index_in];
	LOG(("%sSFX DUCKING: %d\n", indent, channel_ducking[channel_idx])); //DAR_DEBUG

	OUTDENT;
	LOG(("%sEND: PROCESS_CHANNELX\n", indent)); //DAR_DEBUG
	return channel_idx;
}

// ---------------------------------------------------------------------------

void alt_sound_exit()
{
	//DAR_TODO clean up internal storage?
	LOG(("%sBEGIN: ALT_SOUND_EXIT\n", indent));
	INDENT;

	if (cached_machine_name != 0) // better free everything like above!
	{
		cached_machine_name[0] = '#';
		if (BASS_Free() == FALSE) {
			LOG(("%sFAILED: BASS_Free()\n", indent));
			//DAR_TODO get BASS error code
		}
		else {
			LOG(("%sSUCCESS: BASS_Free()\n", indent));
		}
	}
	
	OUTDENT;
	LOG(("%sEND: ALT_SOUND_EXIT\n", indent));
}

// ---------------------------------------------------------------------------

void alt_sound_pause(BOOL pause)
{
	LOG(("BEGIN: ALT_SOUND_PAUSE\n")); //DAR_DEBUG
	INDENT;

	HSTREAM* channel_0_stream = &channel_stream[ch0_stream_idx];
	HSTREAM* channel_1_stream = &channel_stream[ch1_stream_idx];

	unsigned int i;
	if (pause)
	{
		// Pause SFX/Voice
		for (i = SFX_CHANNEL_START; i < ALT_MAX_CHANNELS; ++i) {
			if (channel_stream[i] != BASS_NO_STREAM 
				&& BASS_ChannelIsActive(channel_stream[i]) == BASS_ACTIVE_PLAYING) {
				
				LOG(("%sPausing Channel X Stream: %u (2)\n", indent, channel_stream[i])); //DAR_DEBUG
				BASS_ChannelPause(channel_stream[i]);
			}
		}

		// Pause Jingle/Single
		if (*channel_1_stream != BASS_NO_STREAM 
			&& BASS_ChannelIsActive(*channel_1_stream) == BASS_ACTIVE_PLAYING) {
			
			LOG(("%sPausing Channel 1 Stream: %u (3)\n", indent, *channel_1_stream)); //DAR_DEBUG
			BASS_ChannelPause(*channel_1_stream);
		}

		if (*channel_0_stream != BASS_NO_STREAM 
			&& BASS_ChannelIsActive(*channel_0_stream) == BASS_ACTIVE_PLAYING) {
		    
			LOG(("%sPausing Channel 0 Stream: %u (4)\n", indent, *channel_0_stream)); //DAR_DEBUG
		    BASS_ChannelPause(*channel_0_stream);
	    }
	}
	else
	{
		// Resume SFX/Voice
		for (i = SFX_CHANNEL_START; i < ALT_MAX_CHANNELS; ++i)
			if (channel_stream[i] != BASS_NO_STREAM
				&& BASS_ChannelIsActive(channel_stream[i]) == BASS_ACTIVE_PAUSED) {
				
				LOG(("%sResuming Channel X Stream: %u (2)\n", indent, channel_stream[i])); //DAR_DEBUG
				BASS_ChannelPlay(channel_stream[i], 0);
			}

		// Resume Jingle/Single
		if (*channel_1_stream != 0
			&& BASS_ChannelIsActive(*channel_1_stream) == BASS_ACTIVE_PAUSED) {
			
			LOG(("%sResuming Channel 1 Stream: %u (3)\n", indent, *channel_1_stream)); //DAR_DEBUG
			BASS_ChannelPlay(*channel_1_stream, 0);
		}

		// Resume Music
		if (*channel_0_stream != BASS_NO_STREAM
			&& BASS_ChannelIsActive(*channel_0_stream) == BASS_ACTIVE_PAUSED) {

			LOG(("%sPausing Channel 0 Stream: %u (3)\n", indent, *channel_1_stream)); //DAR_DEBUG
			BASS_ChannelPlay(*channel_0_stream, 0);
		}
	}
	
	OUTDENT;
	LOG(("%sEND: ALT_SOUND_PAUSE\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK ducking_callback(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	// DAR@20230519 
	// SYNCPROC functions run in a separate thread, so care must be taken for
	// thread safety with the main thread.  As it is, this code is not
	// threadsafe, but I have mitigated this as much as possible without
	// introducing platform-dependent thread management
	//
	// All BASS sync function run on the same thread.  A running SYNCPROC will
	// block others from running, so these functions should be fast.
	LOG(("%sBEGIN: DUCKING_CALLBACK\n", indent));
	INDENT;

	LOG(("%sHANDLE: %u  CHANNEL: %u  DATA: %u  USER: %d\n", indent, handle, channel, data, (int)user)); //DAR_DEBUG
	LOG(("%sSTREAM FINISHED ON CHANNEL %d\n", indent, (int)user)); //DAR_DEBUG
    BASS_StreamFree(channel_stream[(int)user]);
	channel_stream[(int)user] = BASS_NO_STREAM;
	channel_ducking[(int)user] = 100;

	// re-compute min ducking based on active channels.
	int new_ducking = 100;
	int num_x_streams = 0;

	// NOTE: This is intentionally checking all streams, including ch0,1
	for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i] != BASS_NO_STREAM) {
			// another stream is playing on the channel
			LOG(("%sCHANNEL_STREAM[%d] ACTIVE\n", indent, i)); //DAR_DEBUG
			num_x_streams++;

			int channel_duck = channel_ducking[i];
			LOG(("%sCHANNEL_DUCKING[%d]: %d\n", indent, i, channel_duck)); //DAR_DEBUG
			
			// update new_ducking if needed
			if (channel_duck < new_ducking)
				new_ducking = channel_duck;
		}
	}
	LOG(("%sNUM STREAMS ACTIVE: %d\n", indent, num_x_streams));
	LOG(("%sMIN DUCKING OF ALL ACTIVE CHANNELS: %d\n", indent, new_ducking)); //DAR_DEBUG

	HSTREAM* channel_0_stream = &channel_stream[ch0_stream_idx];
	if (*channel_0_stream != BASS_NO_STREAM) {
		// channel_0_stream defined.  Set new volume
		float adj_ch_0_vol = channel_0_vol * (float)((double)new_ducking / 100.);
		float new_vol = adj_ch_0_vol * global_vol * master_vol;
		BASS_ChannelSetAttribute(*channel_0_stream, BASS_ATTRIB_VOL, new_vol);
		LOG(("%sNEW CHANNEL 0 VOLUME: %f\n", indent, new_vol)); //DAR_DEBUG

		// Resume stopped/paused channel 0
		if (BASS_ChannelIsActive(*channel_0_stream) != BASS_ACTIVE_PLAYING) {
			LOG(("%sRE-ATIVATING STOPPED/PAUSED CHANNEL 0\n", indent)); //DAR_DEBUG
			BASS_ChannelPlay(*channel_0_stream, 0);
		}
	}
	
	OUTDENT;
	LOG(("%sEND: DUCKING_CALBACK\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	// DAR@20230523
	// This callback is ONLY used when Music (channel 0) playback ends.  Normally,
	// music is stopped programmatically in the main code.  This is here, just in
	// case music ends before it is stopped by the main code.  Because the BASS
	// streams run on a different thread, if the music ends before the code checks
	// for it, we would think the channel has been freed when it has not, leaving
	// the stream and associated data in memory.  It would also mess up our
	// bookkeeping.  I don't expect this happens very often

	//DAR@20230519 SYNCPROC functions run in a separate thread, and will block
	//             other sync processes, so these should be fast and care
	//             must be taken for thread safety with the main thread
	LOG(("%sBEGIN: MUSIC_CALLBACK\n", indent)); //DAR_DEBUG
	INDENT;

	LOG(("HANDLE: %u  CHANNEL : %u  DATA : %u  USER : %u\n", indent, handle, channel, data, user)); //DAR_DEBUG
	LOG(("%sCHANNEL STREAM FINISHED: %u\n", indent, channel)); //DAR_DEBUG
	BASS_StreamFree(channel_stream[(int)user]);
	channel_stream[(int)user] = BASS_NO_STREAM;
	channel_ducking[(int)user] = 100;

	OUTDENT;
	LOG(("%sEND: MUSIC_CALLBACK: HANDLE: %u  CHANNEL: %u  DATA: %u  USER: %u\n", indent, handle, channel, data, user)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

int load_samples(PinSamples* psd)
{
	LOG(("%sBEGIN: LOAD_SAMPLES\n", indent)); //DAR_DEBUG
	INDENT;

	char* lpHelp = cvpmd;
	char* lpSlash = NULL;
	size_t BASE_PATH_LEN;
	int result = 0; // Assume success at start

	// Get path to VPinMAME
	HINSTANCE hInst;
#ifndef _WIN64
	hInst = GetModuleHandle("VPinMAME.dll");
#else
	hInst = GetModuleHandle("VPinMAME64.dll");
#endif
	GetModuleFileName(hInst, cvpmd, sizeof(cvpmd));

	// Get rid of "VPinMAME.dll" from path name
	while (*lpHelp) {
		if (*lpHelp == '\\')
			lpSlash = lpHelp;
		lpHelp++;
	}
	if (lpSlash)
		*lpSlash = '\0';

	// Determine base path length
	BASE_PATH_LEN = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1;

	// No files yet
	psd->num_files = 0;

	// Try to read CSV first
	if (parse_altsound_csv(BASE_PATH_LEN, psd) != 0) {
		LOG(("%sUnable to parse CSV.  Attempting folder parsing...\n", indent));
		result = parse_altsound_folders(BASE_PATH_LEN, psd);
	}

	if (result == 0) {
		LOG(("%sALTSOUND SAMPLES SUCCESSFULLY PARSED\n", indent));
	}
	else {
		LOG(("%sFAILED TO PARSE ALTSOUND SAMPLES\n", indent));
	}

	OUTDENT;
	LOG(("%sEND: LOAD_SAMPLES\n", indent)); //DAR_DEBUG
	return result;
}

// ---------------------------------------------------------------------------

int find_free_channel(void)
{
	LOG(("%sBEGIN: FIND_FREE_CHANNEL\n", indent));
	INDENT;

	int channel_idx = -1;
	
	for (int i = SFX_CHANNEL_START; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i] == BASS_NO_STREAM) {
			channel_idx = i;
			break;
		}
	}

	if (channel_idx < 0)
		LOG(("%sNO FREE CHANNELS AVAILABLE!\n", indent));

	OUTDENT;
	LOG(("%sEND: FIND_FREE_CHANNEL\n", indent));
	return channel_idx;
}

// ---------------------------------------------------------------------------

int parse_altsound_csv(size_t BASE_PATH_LEN, PinSamples* psd)
{
	LOG(("%sBEGIN: PARSE_ALTSOUND_CSV\n", indent)); //DAR_DEBUG
	INDENT;

	size_t PATH_LEN;
	char* PATH;
	CsvReader* c;
	int result = 0; // assume success at start

	int err = CSV_ERROR_FILE_NOT_FOUND;

	cached_machine_name = (char*)malloc(strlen(g_szGameName) + 1);
	strcpy(cached_machine_name, g_szGameName);

	// Build path to CSV file
	PATH_LEN = BASE_PATH_LEN + strlen(path_table) + 1;
	PATH = (char*)malloc(PATH_LEN);
	strcpy_s(PATH, PATH_LEN, cvpmd);
	strcat_s(PATH, PATH_LEN, path_main);
	strcat_s(PATH, PATH_LEN, g_szGameName);
	strcat_s(PATH, PATH_LEN, path_table);

	// Try to load altsound lookup table from .csv if available
	c = csv_open(PATH, ',');

	if (c == NULL) {
		// file not found
		LOG(("%sFile: %s not found!\n", indent, PATH));
		
		OUTDENT;
		LOG(("%sEND: PARSE_ALTSOUND_CSV\n", indent)); //DAR_DEBUG
		return 1;
	}

	// File opened, read header
	if (csv_read_header(c) != CSV_SUCCESS) {
		LOG(("%sfailed to read CSV header\n", indent));
		free(c);
		
		OUTDENT;
		LOG(("%sEND: PARSE_ALTSOUND_CSV\n", indent)); //DAR_DEBUG
		return 1;
	}

	int colID, colCHANNEL, colDUCK, colGAIN, colLOOP, colSTOP, colFNAME;
	LOG(("%snum_headers: %d\n", indent, c->n_header_fields));

	colID = csv_get_colnumber_for_field(c, "ID");
	colCHANNEL = csv_get_colnumber_for_field(c, "CHANNEL");
	colDUCK = csv_get_colnumber_for_field(c, "DUCK");
	colGAIN = csv_get_colnumber_for_field(c, "GAIN");
	colLOOP = csv_get_colnumber_for_field(c, "LOOP");
	colSTOP = csv_get_colnumber_for_field(c, "STOP");
	colFNAME = csv_get_colnumber_for_field(c, "FNAME");

	size_t pos = ftell(c->f);

	while (csv_read_record(c) == CSV_SUCCESS) {
		int val;
		csv_get_hex_field(c, colID, &val);
		csv_get_int_field(c, colCHANNEL, &val);
		csv_get_int_field(c, colDUCK, &val);
		csv_get_int_field(c, colGAIN, &val);
		csv_get_int_field(c, colLOOP, &val);
		csv_get_int_field(c, colSTOP, &val);
		psd->num_files++;
	}

	if (psd->num_files > 0)	{
		psd->ID = (int*)malloc(psd->num_files * sizeof(int));
		psd->files_with_subpath = (char**)malloc(psd->num_files * sizeof(char*));
		psd->channel = (signed char*)malloc(psd->num_files * sizeof(signed char));
		psd->gain = (float*)malloc(psd->num_files * sizeof(float));
		psd->ducking = (signed char*)malloc(psd->num_files * sizeof(signed char));
		psd->loop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
		psd->stop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	}
	else
		// DAR@20230516 It seems we should be cleaning up and getting out
		//              here.
		psd->files_with_subpath = NULL;

	fseek(c->f, pos, SEEK_SET);

	for (int i = 0; i < psd->num_files; ++i) {
		char filePath[4096];
		char tmpPath[4096];

		int val = 0;
		const int err = csv_read_record(c);
		if (err != CSV_SUCCESS)
		{
			LOG(("%sFAILED TO READ CSV RECORD %d\n", indent, i));
			psd->num_files = i;
			result = 1; // indicates failure
			break;
		}

		csv_get_hex_field(c, colID, &val);
		psd->ID[i] = val;
		val = 0;
		psd->channel[i] = csv_get_int_field(c, colCHANNEL, &val) ? -1 : val;
		val = 0;
		csv_get_int_field(c, colDUCK, &val);
		psd->ducking[i] = min(val, (int)100);
		val = 0;
		csv_get_int_field(c, colGAIN, &val);
		psd->gain[i] = val / 100.f;
		val = 0;
		csv_get_int_field(c, colLOOP, &val);
		psd->loop[i] = val;
		val = 0;
		csv_get_int_field(c, colSTOP, &val);
		psd->stop[i] = val;

		strcpy_s(filePath, sizeof(filePath), cvpmd);
		strcat_s(filePath, sizeof(filePath), path_main);
		strcat_s(filePath, sizeof(filePath), g_szGameName);
		strcat_s(filePath, sizeof(filePath), "\\");
		strcat_s(filePath, sizeof(filePath), c->fields[colFNAME]);
		GetFullPathName(filePath, sizeof(filePath), tmpPath, NULL);
		psd->files_with_subpath[i] = (char*)malloc(strlen(tmpPath) + 1);
		strcpy(psd->files_with_subpath[i], tmpPath);
		
		LOG(("%sID = %d, CHANNEL = %d, DUCK = %d, GAIN = %.2f, LOOP = %d, STOP = %d, FNAME = '%s'\n" \
			, indent, psd->ID[i], psd->channel[i], psd->ducking[i], psd->gain[i], psd->loop[i] \
			, psd->stop[i], psd->files_with_subpath[i]));
	}

	csv_close(c);
	LOG(("%sFOUND %d SAMPLES\n", indent, psd->num_files));

	// Clean up allocated memory
	free(PATH);

	OUTDENT;
	LOG(("%sEND: PARSE_ALTSOUND_CSV\n", indent)); //DAR_DEBUG
	return result;
}

// ---------------------------------------------------------------------------

int parse_altsound_folders(size_t BASE_PATH_LEN, PinSamples* psd)
{
	// DAR@20230525
	// This support is for PinSound style of alternate sound mixes.
	// The files are split into directories according to the following:
	//
	// <ROM shortname>\
	//    music\
	//        <instruction1>-name\
	//            <soundfile1>.wav(or .ogg)
	//            ...
	//        <instruction2>-name\
	//        ...
	//    jingle\
	//        <instruction1>-name\
	//            <soundfile1>.wav(or .ogg)
	//            ...
	//        <instruction2>-name\
	//        ...
	//    sfx\
	//        <instruction1>-name\
	//            <soundfile1>.wav(or .ogg)
	//            ...
	//        <instruction2>-name\
	//        ...
	//    single\
	//        <instruction1>-name\
	//            <soundfile1>.wav(or .ogg)
	//            ...
	//        <instruction2>-name\
	//        ...
	//    voice\
	//        <instruction1>-name\
	//            <soundfile1>.wav(or .ogg)
	//            ...
	//        <instruction2>-name\
	//        ...

    //DAR_TODO make sure every opendir has a closedir
	//DAR_TODO add logging to folder parsing code
	//DAR_TODO break up parsing to separate functions
	//DAR_TODO can these be moved to separate files?
	LOG(("%sBEGIN: PARSE_ALTSOUND_FOLDERS\n", indent));
	INDENT;

	DIR* dir;
	char* PATH;
	char cwd[1024];
	_getcwd(cwd, sizeof(cwd));
	int result = 0; // Assume success at start

	// Determine number of sound files in the altsound package
	for (int i = 0; i < 5; ++i)	{
		const char* const subpath = (i == 0) ? path_jingle : 
			                        ((i == 1) ? path_music : 
			                        ((i == 2) ? path_sfx : 
									((i == 3) ? path_single : path_voice)));

		// Determine full path size and allocate memory
		const size_t PATHl = BASE_PATH_LEN + strlen(subpath) + 1;
		PATH = (char*)malloc(PATHl);
		strcpy_s(PATH, PATHl, cvpmd);
		strcat_s(PATH, PATHl, path_main);
		strcat_s(PATH, PATHl, g_szGameName);
		strcat_s(PATH, PATHl, "\\");
		strcat_s(PATH, PATHl, subpath);
		LOG(("%sSEARCH PATH: %s\n", indent, PATH));

		DIR *dir;
		struct dirent *entry;

		// open root directory of the current sound class.  For example, from
		// the comments above:
		// <ROM shortname>\
	    //    music\			<--- sound class: "music"
		//
		dir = opendir(PATH);
		if (!dir) {
			// Path not found.. try the others
			LOG(("%sNot a directory: %s\n", indent, PATH));
			free(PATH);
			continue;
		}

		// Loop through the directory entries
		entry = readdir(dir);
		while (entry != NULL) {
			if (entry->d_name[0] != '.' 
				&& strstr(entry->d_name, ".txt") == 0
				&& strstr(entry->d_name, ".ini") == 0) {
				// Not a system file or txt file.  Assume it's a directory
				// (per PinSound format requirements).  Backup the current
				// directory stream and entry
				const DIR backup_dir = *dir;
				struct dirent backup_entry = *entry;
				
				DIR *dir2;
				struct dirent *entry2;

				// Create new path with subdirectory name, preserving the
				// parent directory stream and current entry
				const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
				char* const PATH2 = (char*)malloc(PATH2l);
				strcpy_s(PATH2, PATH2l, PATH);
				strcat_s(PATH2, PATH2l, entry->d_name);
				LOG(("%sSEARCH SUB-PATH: %s\n", indent, PATH2));

				// Open subfolder of parent sound class directory.  For example
				// from the comments above:
				// <ROM shortname>\
	            //    music\			            <--- sound class: "music"
	            //        <instruction1>-name\      <--- ROM instruction ID-<snd_name>
				//
				// DAR@20230525
				// Found an odd problem on Windows.  A hidden file shows up in the
				// dir stream "desktop.ini" which crashes the program.  I believe
				// this is due to the opendir coming back as NULL, which seems like
				// it should be protected in any case
				dir2 = opendir(PATH2);
				LOG(("%sDIRECTORY STREAM CREATED: %d\n", indent, dir2));
				if (!dir2) {
					// Path not found.. try the others
					LOG(("%sNot a directory: %s\n", indent, PATH2));
					free(PATH2);
					entry = NULL;
					continue;
				}

				entry2 = readdir(dir2);
				LOG(("%sENTRY: %d\n", indent, entry2));

				while (entry2 != NULL)  {
					LOG(("%sFOUND FILE: %s\n", indent, entry2->d_name));
					if (entry2->d_name[0] != '.' 
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0)
						// not a system or txt file.  This must be a sound file.
						// Increase the number of files being tracked.
						LOG(("%sADDING FILE: %s\n", indent, entry2->d_name));
						psd->num_files++;
					
					// get next sound file in subdirectory
					entry2 = readdir(dir2);
				}

				// All sound files for this sound class collected.
				// Close the directory stream and free allocated memory
				closedir(dir2);
				free(PATH2);

				// Restore backed up parent folder and entry.
				*dir = backup_dir;
				*entry = backup_entry;
			}

			// get next entry in the parent directory stream
			entry = readdir(dir);
		}

		// All sound class directories processed.  Close directory stream and
		// free allocated memory
		closedir(dir);
		free(PATH);
	}

	if (psd->num_files <= 0) {
		// No filenames parsed.. exit
		LOG(("%sNO FILES FOUND!\n", indent));
		psd->files_with_subpath = NULL;

		OUTDENT;
		LOG(("%sEND: PARSE_ALTSOUND_FOLDERS\n", indent));
		return -1;
	}

	// Allocate memory to store sound sample data
	psd->ID = (int*)malloc(psd->num_files * sizeof(int));
	psd->files_with_subpath = (char**)malloc(psd->num_files * sizeof(char*));
	psd->channel = (signed char*)malloc(psd->num_files * sizeof(signed char));
	psd->gain = (float*)malloc(psd->num_files * sizeof(float));
	psd->ducking = (signed char*)malloc(psd->num_files * sizeof(signed char));
	psd->loop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	psd->stop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	psd->num_files = 0;

	// Search each parent sound class directory and parse gain.txt and
	// ducking.txt files to determine default gain and ducking for any
	// sound class instruction # sample that does not define an explicit
	// value
	for (int i = 0; i < 5; ++i)	{
		unsigned int default_gain = 10;
		int default_ducking = 100; //!! default depends on type??

		const char* const subpath = (i == 0) ? path_jingle :
			((i == 1) ? path_music :
			((i == 2) ? path_sfx :
				((i == 3) ? path_single : path_voice)));

		// DAR@20230525
	    // For information's sake, PinSound defines the default ducking values as:
	    //    SFX    : 90%
	    //    VOICE  : 85%
	    //    JINGLE :  5%
		//
		// Set default ducking values.  Can be overridden by ducking.txt file
		if (subpath == path_jingle || subpath == path_single) {
			default_ducking = 10; // % music volume retained when active
		}
		if (subpath == path_sfx) {
			default_ducking = 80;
		}
		if (subpath == path_voice) {
			default_ducking = 65;
		}

		// Detmine path size and allocate memory
		const size_t PATHl = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1 + strlen(subpath) + 1;
		char* const PATH = (char*)malloc(PATHl);

		DIR *dir;
		struct dirent *entry;

		strcpy_s(PATH, PATHl, cvpmd);
		strcat_s(PATH, PATHl, path_main);
		strcat_s(PATH, PATHl, g_szGameName);
		strcat_s(PATH, PATHl, "\\");
		strcat_s(PATH, PATHl, subpath);
		LOG(("%sCURRENT_PATH1: %s\n", indent, PATH)); //DAR_DEBUG

		dir = opendir(PATH);
		if (!dir)
		{
			// Path not found.. try the others
			LOG(("%sPath not found: %s\n", indent, PATH));
			free(PATH);
			continue;
		}
		{// parse gain.txt
			const size_t PATHGl = strlen(PATH) + strlen("gain.txt") + 1;
			char* const PATHG = (char*)malloc(PATHGl);
			FILE *f;
			strcpy_s(PATHG, PATHGl, PATH);
			strcat_s(PATHG, PATHGl, "gain.txt");
			LOG(("%sCURRENT_PATH2: %s\n", indent, PATHG)); //DAR_DEBUG
			f = fopen(PATHG, "r");
			
			if (f) {
				// Set default gain from file
				fscanf(f, "%u", &default_gain);
				fclose(f);
			}
			else {
				// Default gain not found, use default defined above
			}

			free(PATHG);
		}
		{// parse ducking.txt
			const size_t PATHGl = strlen(PATH) + strlen("ducking.txt") + 1;
			char* const PATHG = (char*)malloc(PATHGl);
			FILE *f;
			strcpy_s(PATHG, PATHGl, PATH);
			strcat_s(PATHG, PATHGl, "ducking.txt");
			LOG(("%sCURRENT_PATH3: %s\n", indent, PATHG)); //DAR_DEBUG
			f = fopen(PATHG, "r");
			
			if (f) {
				// Set default ducking from file
				fscanf(f, "%d", &default_ducking);
				fclose(f);
			}
			else {
				// Default ducking not found, use default defined above
			}
			free(PATHG);
		}

		// parse individual sound file data
		entry = readdir(dir);
		while (entry != NULL)
		{
			if (entry->d_name[0] != '.'
				&& strstr(entry->d_name, ".txt") == 0
				&& strstr(entry->d_name, ".ini") == 0)
			{
				LOG(("%sCURRENT_ENTRY1: %s\n", indent, entry->d_name)); //DAR_DEBUG
				// Not a system file or txt file.  Assume it's a directory
				// (per PinSound format requirements).  Backup the current
				// directory stream and entry
				const DIR backup_dir = *dir;
				struct dirent backup_entry = *entry;

				const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
				char* const PATH2 = (char*)malloc(PATH2l);
				unsigned int gain = default_gain;
				int ducking = default_ducking;
				
				DIR *dir2;
				struct dirent *entry2;

				strcpy_s(PATH2, PATH2l, PATH);
				strcat_s(PATH2, PATH2l, entry->d_name);

				{// individual gain
					const size_t PATHGl = strlen(PATH2) + 1 + strlen("gain.txt") + 1;
					char* const PATHG = (char*)malloc(PATHGl);
					FILE *f;

					strcpy_s(PATHG, PATHGl, PATH2);
					strcat_s(PATHG, PATHGl, "\\");
					strcat_s(PATHG, PATHGl, "gain.txt");
					LOG(("%sCURRENT_PATH4: %s\n", indent, PATHG)); //DAR_DEBUG
					f = fopen(PATHG, "r");
					if (f)
					{
						fscanf(f, "%u", &gain);
						fclose(f);
					}
					free(PATHG);
				}
				{// individual ducking
					const size_t PATHGl = strlen(PATH2) + 1 + strlen("ducking.txt") + 1;
					char* const PATHG = (char*)malloc(PATHGl);
					FILE *f;

					strcpy_s(PATHG, PATHGl, PATH2);
					strcat_s(PATHG, PATHGl, "\\");
					strcat_s(PATHG, PATHGl, "ducking.txt");
					LOG(("%sCURRENT_PATH5: %s\n", indent, PATHG)); //DAR_DEBUG
					f = fopen(PATHG, "r");
					if (f)
					{
						fscanf(f, "%d", &ducking);
						fclose(f);
					}
					free(PATHG);
				}

				LOG(("%sOPENDIR: %s\n", indent, PATH2)); //DAR_DEBUG
				dir2 = opendir(PATH2);
				entry2 = readdir(dir2);
				while (entry2 != NULL)
				{
					if (entry2->d_name[0] != '.'
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0)
					{
						LOG(("%sCURRENT_ENTRY2: %s\n", indent, entry->d_name)); //DAR_DEBUG
						const size_t PATH3l = strlen(PATH2) + 1 + strlen(entry2->d_name) + 1;
						char* const ptr = strrchr(PATH2, '\\');
						char id[7] = { 0, 0, 0, 0, 0, 0, 0 };

						psd->files_with_subpath[psd->num_files] = (char*)malloc(PATH3l);
						strcpy_s(psd->files_with_subpath[psd->num_files], PATH3l, PATH2);
						strcat_s(psd->files_with_subpath[psd->num_files], PATH3l, "\\");
						strcat_s(psd->files_with_subpath[psd->num_files], PATH3l, entry2->d_name);

						memcpy(id, ptr + 1, 6);
						sscanf(id, "%6d", &psd->ID[psd->num_files]);
						//DAR_TODO ID is always 0.. not good

						float adj_gain = (float)gain / 20.f;
						psd->gain[psd->num_files] = adj_gain;
						psd->ducking[psd->num_files] = min(ducking, (int)100);

						if (subpath == path_music) {
							psd->channel[psd->num_files] = 0;
							psd->loop[psd->num_files] = 100;
							psd->stop[psd->num_files] = 0;
						}

						if (subpath == path_jingle || subpath == path_single) {
							psd->channel[psd->num_files] = 1;
							psd->loop[psd->num_files] = 0;
							psd->stop[psd->num_files] = (subpath == path_single) ? 1 : 0;
						}

						if (subpath == path_sfx || subpath == path_voice) {
							psd->channel[psd->num_files] = -1;
							psd->loop[psd->num_files] = 0;
							psd->stop[psd->num_files] = 0;
						}

						LOG(("%sID = %d, CHANNEL = %d, DUCK = %d, GAIN = %.2f, LOOP = %d, STOP = %d, FNAME = '%s'\n" \
							 , indent, psd->ID[i], psd->channel[i], psd->ducking[i], psd->gain[i], psd->loop[i] \
							 , psd->stop[i], psd->files_with_subpath[i]));
						
						psd->num_files++;
					}
					entry2 = readdir(dir2);
				}
				closedir(dir2);
				free(PATH2);

				*dir = backup_dir;
				*entry = backup_entry;
			}
			entry = readdir(dir);
		}
		closedir(dir);
		free(PATH);
	}
	LOG(("%sFound %d samples\n", indent,  psd->num_files));

	dir = opendir(cwd);
	closedir(dir);

	OUTDENT;
	LOG(("%sEND: PARSE_ALTSOUND_FOLDERS\n", indent));
	return result;
}

/**
* init struct and open file
*/
//DAR_TODO add logging to CSV parsing functions
static CsvReader* csv_open(const char* const filename, const int delimiter) {
	CsvReader* const c = malloc(sizeof(CsvReader));

	c->f = fopen(filename, "r");
	if (c->f == NULL) {
		free(c);
		return NULL;
	}

	c->delimiter = delimiter;
	c->n_header_fields = 0;
	c->n_fields = 0;

	return c;
}

/**
* trim field buffer from end to start
*/
static void trim(const char* const start, char* end) {
	while (end > start) {
		end--;
		if (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
			*end = 0;
		}
		else {
			break;
		}
	}
}

/* Get a line of text from a file, discarding any end-of-line characters */
static int fgetline(char* const buff, const int nchars, FILE* const file)
{
	int length;

	if (fgets(buff, nchars, file) == NULL)
		return -1;
	if (buff[0] == '\r')
		memmove(buff, buff + 1, nchars - 1);

	length = (int)strlen(buff);
	while (length && (buff[length - 1] == '\r' || buff[length - 1] == '\n'))
		length--;
	buff[length] = 0;

	return length;
}

static int parse_line(CsvReader* const c, char* line, const int header) {
	char* p = line;
	char* d = NULL;
	char* f = NULL;
	int capacity = 0;
	int field_number = 0;
	int enclosed_in_quotes = 0;
	int escaped = 0;
	int allocField = 1;
	int justAfterDelim = 1; 		// if set skip whitespace
	char** fields = header ? c->header_fields : c->fields;

	while (*p) {
		// realloc field array
		if (field_number == capacity) {
			int i;
			const int size = (capacity + 10)*sizeof(char*);
			fields = capacity == 0 ? malloc(size) : realloc(fields, size);
			capacity += 10;
			for (i = field_number; i<capacity; i++) fields[i] = NULL;
		}
		// allocate field
		if (allocField) {
			allocField = 0;
			fields[field_number] = _strdup(p);
			f = d = fields[field_number];
		}
		if (enclosed_in_quotes) {
			if (*p == '"' && !escaped)
				enclosed_in_quotes = 0;
			else if (*p == '\\' && !escaped)
				escaped = 1;
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t'))
					justAfterDelim = 0;
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}
		else { // not in quotes
			if (*p == '"' && !escaped)
				enclosed_in_quotes = 1;
			else if (*p == c->delimiter && !escaped) {
				// terminate current field
				*d = 0;
				trim(f, d);
				// next field
				field_number++;
				allocField = 1;
				justAfterDelim = 1;
			}
			else if (*p == '\\' && !escaped)
				escaped = 1;
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t'))
					justAfterDelim = 0;
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}

		p++;
	}

	if (!d || !f)
		return CSV_ERROR_LINE_FORMAT;

	*d = 0;
	trim(f, d);

	if (enclosed_in_quotes) return CSV_ERROR_LINE_FORMAT; // quote still open
	if (escaped) return CSV_ERROR_LINE_FORMAT; // esc still open

	if (header) {
		c->header_fields = fields;
		c->n_header_fields = field_number + 1;
	}
	else {
		c->fields = fields;
		c->n_fields = field_number + 1;
	}

	return CSV_SUCCESS;
}

static int csv_read_header(CsvReader* const c) {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, c->f);
	if (len < 0)
		return CSV_ERROR_HEADER_NOT_FOUND;

	// parse line and look for headers
	return parse_line(c, buf, 1);
}

static int csv_get_colnumber_for_field(CsvReader* c, const char* fieldname) {
	int i;
	for (i = 0; i < c->n_header_fields; i++)
		if (strcmp(c->header_fields[i], fieldname) == 0)
			return i;

	return CSV_ERROR_NO_SUCH_FIELD;
}

static void free_record(CsvReader* const c) {
	int i;
	for (i = 0; i < c->n_fields; i++)
		free(c->fields[i]);

	if (c->n_fields) free(c->fields);
}

static int csv_get_int_field(CsvReader* const c, const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "%d", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static int csv_get_hex_field(CsvReader* const c, const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "0x%x", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static int csv_get_float_field(CsvReader* const c, const int field_index, float* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "%f", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static int csv_get_str_field(CsvReader* const c, const int field_index, char** pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		*pValue = c->fields[field_index];
		return 0;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static void csv_close(CsvReader* const c) {
	if (c) {
		int i;
		free_record(c);
		for (i = 0; i < c->n_header_fields; i++) {
			if (c->header_fields[i]) free(c->header_fields[i]);
		}
		if (c->n_header_fields > 0) free(c->header_fields);
		if (c->f) fclose(c->f);
		free(c);
	}
}

static int csv_read_record(CsvReader* const c) {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, c->f);
	if (len < 0)
		return CSV_ERROR_NO_MORE_RECORDS;

	free_record(c);
	// parse line and look for data
	return parse_line(c, buf, 0);
}

// ---------------------------------------------------------------------------

void preprocess_commands(CmdData* cmds_out, int cmd_in)
{
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
			if ((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA) && (cmd_buffer[1] == (cmd_buffer[0] ^ 0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
			{
				//DAR@20230518 I don't know why this is nerfing the
				//             volume.  In any case, going to wait until
				//             the end to apply it, so we can account for
				//             ducking without multiple volume changes
				global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
				//					if (channel_0_stream != 0)
				//						BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

				LOG(("%sChange volume %.2f\n", indent, global_vol));
			}
			else
				LOG(("%sfiltered command %02X %02X %02X %02X\n", indent, cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]));

			for (int i = 0; i < ALT_MAX_CMDS; ++i) {
				LOG(("%sCMD_BUFFER BEFORE BIT OPERATION: %u\n", indent, cmd_buffer[i])); //DAR_DEBUG
				cmd_buffer[i] = ~0;
				LOG(("%sCMD_BUFFER AFTER BITWISE OPERATION: %u\n", indent, cmd_buffer[i])); //DAR_DEBUG
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
			global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
			//				if (channel_0_stream != 0)
			//					BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

			LOG(("%sChange volume %.2f\n", indent, global_vol));

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
				global_vol = (float)(0x2F - cmd_in) / 31.f;
				//					if (channel_0_stream != 0)
				//						BASS_ChannelSetAttribute(channel_0_stream, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

				LOG(("%schange volume %.2f\n", indent, global_vol));

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

void postprocess_commands(const unsigned int combined_cmd)
{
	LOG(("%sBEGIN: POSTPROCESS_COMMANDS\n", indent)); //DAR_DEBUG
	INDENT;

	HSTREAM* channel_0_stream = &channel_stream[ch0_stream_idx];

	// Get hardware generation
	UINT64 hardware_gen = core_gameData->gen;
	LOG(("%sMAME_GEN: %d\n", indent, hardware_gen)); //DAR_DEBUG

		// DAR_TODO what does this do?
	if (hardware_gen == GEN_WPCDCS
		|| hardware_gen == GEN_WPCSECURITY
		|| hardware_gen == GEN_WPC95DCS
		|| hardware_gen == GEN_WPC95)
	{
		if (combined_cmd == 0x03E3 && *channel_0_stream != BASS_NO_STREAM) // stop music
		{
			BASS_ChannelStop(*channel_0_stream);
			LOG(("%sSTOP CHANNEL 0 (2)\n", indent)); //DAR_DEBUG
			
			BASS_StreamFree(*channel_0_stream);
			*channel_0_stream = BASS_NO_STREAM;
			channel_0_vol = 1.0f;
		}
	}

	//!! old WPC machines music stop? -> 0x00 for SYS11?

	if (hardware_gen == GEN_DEDMD32)
	{
		if ((combined_cmd == 0x0018 || combined_cmd == 0x0023) && *channel_0_stream != BASS_NO_STREAM) // stop music //!! ???? 0x0019??
		{
			BASS_ChannelStop(*channel_0_stream);
			LOG(("%sSTOP CHANNEL 0 (3)\n", indent)); //DAR_DEBUG
			
			BASS_StreamFree(*channel_0_stream);
			*channel_0_stream = BASS_NO_STREAM;
			channel_0_vol = 1.0f;
		}
	}

	if (hardware_gen == GEN_WS
		|| hardware_gen == GEN_WS_1
		|| hardware_gen == GEN_WS_2)
	{
		if (((combined_cmd == 0x0000 || (combined_cmd & 0xf0ff) == 0xf000)) && *channel_0_stream != 0) // stop music
		{
			BASS_ChannelStop(*channel_0_stream);
			LOG(("%sSTOP CHANNEL 0 (4)\n", indent)); //DAR_DEBUG
			BASS_StreamFree(*channel_0_stream);
			*channel_0_stream = BASS_NO_STREAM;
			channel_0_vol = 1.0f;
		}
	}
	
	OUTDENT;
	LOG(("%sEND: POSTPROCESS_COMMANDS\n", indent)); //DAR_DEBUG
}

// ---------------------------------------------------------------------------
// Helper function to translate BASS error codes to printable strings
// ---------------------------------------------------------------------------

static const char * bass_err_names[] = {
    [BASS_OK]                 = "BASS_OK",
    [BASS_ERROR_MEM]          = "BASS_ERROR_MEM",
    [BASS_ERROR_FILEOPEN]     = "BASS_ERROR_FILEOPEN",
    [BASS_ERROR_DRIVER]       = "BASS_ERROR_DRIVER",
    [BASS_ERROR_BUFLOST]      = "BASS_ERROR_BUFLOST",
    [BASS_ERROR_HANDLE]       = "BASS_ERROR_HANDLE",
    [BASS_ERROR_FORMAT]       = "BASS_ERROR_FORMAT",
    [BASS_ERROR_POSITION]     = "BASS_ERROR_POSITION",
    [BASS_ERROR_INIT]         = "BASS_ERROR_INIT",
    [BASS_ERROR_START]        = "BASS_ERROR_START",
    [BASS_ERROR_SSL]          = "BASS_ERROR_SSL",
    [BASS_ERROR_REINIT]       = "BASS_ERROR_REINIT",
    [BASS_ERROR_ALREADY]      = "BASS_ERROR_ALREADY",
    [BASS_ERROR_NOTAUDIO]     = "BASS_ERROR_NOTAUDIO",
    [BASS_ERROR_NOCHAN]       = "BASS_ERROR_NOCHAN",
    [BASS_ERROR_ILLTYPE]      = "BASS_ERROR_ILLTYPE",
    [BASS_ERROR_ILLPARAM]     = "BASS_ERROR_ILLPARAM",
    [BASS_ERROR_NO3D]         = "BASS_ERROR_NO3D",
    [BASS_ERROR_NOEAX]        = "BASS_ERROR_NOEAX",
    [BASS_ERROR_DEVICE]       = "BASS_ERROR_DEVICE",
    [BASS_ERROR_NOPLAY]       = "BASS_ERROR_NOPLAY",
    [BASS_ERROR_FREQ]         = "BASS_ERROR_FREQ",
    [BASS_ERROR_NOTFILE]      = "BASS_ERROR_NOTFILE",
    [BASS_ERROR_NOHW]         = "BASS_ERROR_NOHW",
    [BASS_ERROR_EMPTY]        = "BASS_ERROR_EMPTY",
    [BASS_ERROR_NONET]        = "BASS_ERROR_NONET",
    [BASS_ERROR_CREATE]       = "BASS_ERROR_CREATE",
    [BASS_ERROR_NOFX]         = "BASS_ERROR_NOFX",
    [BASS_ERROR_NOTAVAIL]     = "BASS_ERROR_NOTAVAIL",
    [BASS_ERROR_DECODE]       = "BASS_ERROR_DECODE",
    [BASS_ERROR_DX]           = "BASS_ERROR_DX",
    [BASS_ERROR_TIMEOUT]      = "BASS_ERROR_TIMEOUT",
    [BASS_ERROR_FILEFORM]     = "BASS_ERROR_FILEFORM",
    [BASS_ERROR_SPEAKER]      = "BASS_ERROR_SPEAKER",
    [BASS_ERROR_VERSION]      = "BASS_ERROR_VERSION",
    [BASS_ERROR_CODEC]        = "BASS_ERROR_CODEC",
    [BASS_ERROR_ENDED]        = "BASS_ERROR_ENDED",
    [BASS_ERROR_BUSY]         = "BASS_ERROR_BUSY",
    [BASS_ERROR_UNSTREAMABLE] = "BASS_ERROR_UNSTREAMABLE",
    [BASS_ERROR_PROTOCOL]     = "BASS_ERROR_PROTOCOL",
    [BASS_ERROR_DENIED]       = "BASS_ERROR_DENIED"
 //   [BASS_ERROR_UNKNOWN]      = ""
};

static const char* bass_errstr(int err_val_in) {
	if (err_val_in < 0) {
		return "BASS_ERROR_UNKNOWN";
	}
	else {
		return bass_err_names[err_val_in];
	}
}
