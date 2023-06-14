// ---------------------------------------------------------------------------
// AltsoundProcessor
// Dave Roscoe 06/14/2023
//
// Encapsulates all specialized processing for the legacy Altsound
// CSV and PinSound format
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------

#include "altsound_processor.hpp"

// System includes
#include <time.h>
#include <mutex>

// Local includes
#include "osdepend.h"
#include "altsound_csv_parser.hpp"
#include "altsound_file_parser.hpp"

using std::string;

static std::mutex io_mutex;

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

AltsoundProcessor::AltsoundProcessor(const char* gname_in,
	                                 const float global_vol_in,
	                                 const float master_vol_in)
: game_name(gname_in),
  is_initialized(false),
  is_stable(true), // future use
  global_vol(global_vol_in),
  master_vol(master_vol_in),
  music_vol(1.0f)
{
	// initialize channel_stream storage
	for (char i = 0; i < ALT_MAX_CHANNELS; i++) {
		channel_stream[i] = AltsoundStreamInfo();
		channel_ducking[i] = 1.0f;
	}

	// initialize sample storage
	psd.ID = 0; //int *
	psd.files_with_subpath = 0; // char ** 
	psd.channel = 0; // signed char * 
	psd.gain = 0; // float * 
	psd.ducking = 0; // float * 
	psd.loop = 0; // unsigned char * 
	psd.stop = 0; // unsigned char * 
	psd.num_files = 0; // unsigned int 

	// NOTE:
	// - channel_stream[0] is reserved for uninitialized streams
	// - SFX streams don't require index tracking since multiple can play
	//   simultaneously
	//
    // initialize stream tracking variables
	cur_mus_stream = AltsoundStreamInfo();
	cur_jin_stream = AltsoundStreamInfo();

	// Seed random number generator
	srand(static_cast<unsigned int>(time(NULL)));
}

AltsoundProcessor::~AltsoundProcessor()
{
	// clean up sample data
	for (int i = 0; i < psd.num_files; ++i)
	{
		free(psd.files_with_subpath[i]);
		psd.files_with_subpath[i] = 0;
	}

	free(psd.ID);
	free(psd.files_with_subpath);
	free(psd.gain);
	free(psd.ducking);
	free(psd.channel);
	free(psd.loop);
	free(psd.stop);

	psd.ID = 0;
	psd.files_with_subpath = 0;
	psd.gain = 0;
	psd.ducking = 0;
	psd.channel = 0;
	psd.loop = 0;
	psd.stop = 0;

	psd.num_files = 0;
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_cmd(const unsigned int cmd_combined_in)
{
	LOG(("BEGIN: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
	LOG(("ACQUIRING MUTEX\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("MUTEX ACQUIRED\n")); //DAR_DEBUG
	

	if (!is_stable) {
		LOG(("Altsound system unstable. Processing skipped\n"));

		
		LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
	}

	static bool samples_loaded = false;

	if (!samples_loaded) {
		// Load sound samples
		if (load_samples()) {
			LOG(("SUCCESS: LOAD_SAMPLES\n"));
			samples_loaded = true;
		}
		else {
			LOG(("FAILED: LOAD_SAMPLES\n"));
			psd.num_files = 0;
			is_stable = false; // prevent re-entry

			
			LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
			return false;
		}
	}

	// get sample for playback
	int sample_idx = get_sample(cmd_combined_in);

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		LOG(("FAILED: get_sample()\n", cmd_combined_in)); //DAR_DEBUG

		//// DAR@20230520
		//// Not sure why this is needed, but to preserve original functionality,
		//// this needs to be called in this case, before we exit
		//postprocess_commands(cmd_combined);

		
		LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
		return false;
	}
	LOG(("SUCCESS: GET_SAMPLE(%04X)\n", cmd_combined_in));

	BOOL play_music = FALSE;
	BOOL play_jingle = FALSE;
	BOOL play_sfx = FALSE;

	AltsoundStreamInfo new_stream;
	HSTREAM stream = BASS_NO_STREAM;

	unsigned int sample_channel = psd.channel[sample_idx];
	if (sample_channel == 1) {
		// Command is for playing Jingle/Single
		if (process_jingle(&psd, sample_idx, new_stream)) {
			LOG(("SUCCESS: process_jingle()\n")); //DAR_DEBUG

			// update stream storage
			channel_stream[new_stream.channel_idx] = new_stream;
			
			play_jingle = TRUE; // Defer playback until the end
			cur_jin_stream = new_stream;
			stream = cur_jin_stream.handle;
		}
		else {
			// An error ocurred during processing
			LOG(("FAILED: process_jingle()\n")); //DAR_DEBUG
		}
	}

	if (sample_channel == 0) {
		// Command is for playing music
		if (process_music(&psd, sample_idx, new_stream)) {
			LOG(("SUCCESS: process_music()\n")); //DAR_DEBUG

			// update stream storage
			channel_stream[new_stream.channel_idx] = new_stream;
		   
			play_music = TRUE; // Defer playback until the end
			cur_mus_stream = new_stream;
			stream = cur_mus_stream.handle;
		}
		else {
			// An error ocurred during processing
			LOG(("FAILED: process_music()\n")); //DAR_DEBUG
		}
	}

	if (sample_channel == -1) {
		// Command is for playing voice/sfx
		if (process_sfx(&psd, sample_idx, new_stream)) {
			LOG(("SUCCESS: process_sfx()\n")); //DAR_DEBUG

			// update stream storage
			channel_stream[new_stream.channel_idx] = new_stream;
		    
			play_sfx = TRUE;// Defer playback until the end
			stream = new_stream.handle;
		}
		else {
			// An error ocurred during processing
			LOG(("FAILED: process_sfx()\n")); //DAR_DEBUG
		}
	}

	if (!play_music && !play_jingle && !play_sfx) {
		LOG(("FAILED: alt_sound_handle()\n")); //DAR_DEBUG

		
		LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
		return false;
	}

	// Get lowest ducking value from all channels (including music, jingle)
	float min_ducking = get_min_ducking();
	LOG(("MIN DUCKING VALUE: %.2f\n", min_ducking)); //DAR_DEBUG

	// set new music volume	
	if (cur_mus_stream.handle != BASS_NO_STREAM) {
		// calculate ducked volume for music
		float adj_mus_vol = music_vol * min_ducking;

		set_volume(cur_mus_stream.handle, adj_mus_vol);
	}
	else {
		// No music is currently playing.  If a music file starts later,
		// the code above will ensure the volume will be set correctly
		LOG(("NO MUSIC STREAM. VOLUME NOT SET\n")); //DAR_DEBUG
	}

	// Play pending sound determined above, if any
	if (new_stream.handle != BASS_NO_STREAM) {
		if (!BASS_ChannelPlay(stream, 0)) {
			// Sound playback failed
			LOG(("FAILED: BASS_ChannelPlay(%u): %s\n", new_stream.handle,\
				 get_bass_err())); //DAR_DEBUG
		}
		else {
			LOG(("SUCCESS: BASS_ChannelPlay(%u): CH(%d) CMD(%04X) SAMPLE(%s)\n", \
				new_stream.handle, new_stream.channel_idx, cmd_combined_in, \
				get_short_path(psd.files_with_subpath[sample_idx])));
		}
	}

	
	LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

//void AltsoundProcessor::initialize_sample_data() {
//	LOG(("BEGIN: INITIALIZE_SAMPLE_DATA\n"));
//	
//	//DAR_TODO fix this... crashes at startup, need to initialize this structure
//	// in constructor
//	for (int i = 0; i < psd.num_files; ++i)
//	{
//		free(psd.files_with_subpath[i]);
//		psd.files_with_subpath[i] = 0;
//	}
//
//	free(psd.ID);
//	free(psd.files_with_subpath);
//	free(psd.gain);
//	free(psd.ducking);
//	free(psd.channel);
//	free(psd.loop);
//	free(psd.stop);
//
//	psd.ID = 0;
//	psd.files_with_subpath = 0;
//	psd.gain = 0;
//	psd.ducking = 0;
//	psd.channel = 0;
//	psd.loop = 0;
//	psd.stop = 0;
//
//	psd.num_files = 0;
//
//	
//	LOG(("END: INITIALIZE_SAMPLE_DATA\n"));
//}

bool AltsoundProcessor::load_samples() {
	LOG(("BEGIN: LOAD_SAMPLES\n")); //DAR_DEBUG
	

	// No files yet
	psd.num_files = 0;

	// Try to load altsound lookup table from .csv if available
	AltsoundCsvParser csv_parser(game_name.c_str());
	if (csv_parser.parse(&psd)) {
		LOG(("SUCCESS: csv_parser.parse()\n")); //DAR_DEBUG
	}
	else {
		AltsoundFileParser file_parser(game_name.c_str());
		if (!file_parser.parse(&psd)) {
			LOG(("FAILED: file_parser.parse()\n")); //DAR_DEBUG

			
			LOG(("END: LOAD_SAMPLES\n")); //DAR_DEBUG
			return false;
		}
		else {
			LOG(("SUCCESS: file_parser.parse()\n")); //DAR_DEBUG
		}
	}
	LOG(("ALTSOUND SAMPLES SUCCESSFULLY PARSED\n"));

	
	LOG(("END: LOAD_SAMPLES\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

int AltsoundProcessor::get_sample(const unsigned int cmd_combined_in) {
	LOG(("BEGIN: GET_SAMPLE\n")); //DAR_DEBUG
	

	unsigned int sample_idx = -1;

	// Look for sample that matches the current command
	for (int i = 0; i < psd.num_files; ++i) {
		if (psd.ID[i] == cmd_combined_in) {
			// Current ID matches the command. Check if there are more samples for this
			// command and randomly pick one
			unsigned int num_samples = 0;
			do {
				num_samples++;

				if (i + num_samples >= psd.num_files)
					// sample index exceeds the number of samples
					break;

				// Loop while the next sample ID matches the command
			} while (psd.ID[i + num_samples] == cmd_combined_in);
			LOG(("SUCCESS: FOUND %d SAMPLE(S) FOR ID: %04X\n", num_samples, cmd_combined_in)); //DAR_DEBUG

			// num_samples now contains the number of samples with the same ID
			// pick one to play at random
			sample_idx = i + rand() % num_samples;
			break;
		}
	}

	if (sample_idx == -1) {
		LOG(("FAILED: NO SAMPLE(S) FOUND FOR ID: %04X\n", cmd_combined_in)); //DAR_DEBUG
	}

	
	LOG(("END: GET_SAMPLE\n")); //DAR_DEBUG
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_music(PinSamples* psd_in, int sample_idx_in,\
	                                  AltsoundStreamInfo& stream_out) {
	LOG(("BEGIN: PROCESS_MUSIC\n"));
	

	LOG(("CURRENT MUSIC STREAM: %u  CH: %02d\n", cur_mus_stream,\
		 cur_mus_stream.channel_idx)); //DAR_DEBUG

	if (cur_mus_stream.handle != BASS_NO_STREAM) {
		LOG(("STOPPING MUSIC STREAM: %u\n", cur_mus_stream.handle)); //DAR_DEBUG

		// Music stream defined.  Stop it if still playing
		stop_stream(cur_mus_stream.handle);
		//DAR_TODO trap error?

		channel_ducking[cur_mus_stream.channel_idx] = 1.0f;
		cur_mus_stream = AltsoundStreamInfo();
		music_vol = 1.0f;
	}

	char* fname = psd_in->files_with_subpath[sample_idx_in];
	int loops = psd_in->loop[sample_idx_in];

	// Create playback stream
	if (!create_stream(fname, loops, &AltsoundProcessor::music_callback, stream_out)) {
		// Failed to create stream
		LOG(("FAILED: create_stream(): %s\n", get_bass_err()));

		
		LOG(("END: PROCESS_MUSIC\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: create_stream()\n")); //DAR_DEBUG

	// Everything is good.  Update output
	stream_out.stream_type = MUSIC;

	// set ducking for music
	//channel_ducking[mus_stream_idx] = psd_in->ducking[index_in];
	channel_ducking[stream_out.channel_idx] = 1.0f; // no ducking
	LOG(("MUSIC DUCKING: %.2f\n", channel_ducking[stream_out.channel_idx])); //DAR_DEBUG

	// Set music volume.  This will get ducked as needed in the main handler
	LOG(("SETTING MUSIC VOLUME\n")); //DAR_DEBUG
	music_vol = psd_in->gain[sample_idx_in]; // updates global storage
	set_volume(stream_out.handle, music_vol);

	
	LOG(("END: PROCESS_MUSIC\n"));
	return TRUE;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_jingle(PinSamples* psd_in, int sample_idx_in,\
                                       AltsoundStreamInfo& stream_out) {
	LOG(("BEGIN: PROCESS_JINGLE\n")); //DAR_DEBUG
	

	AltsoundStreamInfo jin_stream = cur_jin_stream;
	AltsoundStreamInfo mus_stream = cur_mus_stream;

	LOG(("CURRENT JINGLE STREAM: %u  CH: %02d\n", jin_stream.handle, jin_stream.channel_idx)); //DAR_DEBUG
	LOG(("CURRENT MUSIC STREAM: %u  CH: %02d\n", mus_stream.handle, mus_stream.channel_idx)); //DAR_DEBUG

	if (jin_stream.handle != BASS_NO_STREAM) {
		LOG(("STOPPING JINGLE STREAM: %u\n", jin_stream.handle)); //DAR_DEBUG

		// Jingle/Single already playing.  Stop it
		if (!stop_stream(jin_stream.handle)) {
			LOG(("&sFAILED: stop_stream(%u)\n", jin_stream.handle));

			
			LOG(("END: PROCESS_JINGLE\n")); //DAR_DEBUG
			return FALSE;
		}
		LOG(("SUCCESS: stop_stream(%u)\n", jin_stream.handle));

		channel_ducking[jin_stream.channel_idx] = 1.0f;
		cur_jin_stream = AltsoundStreamInfo();
	}

	if (mus_stream.handle != BASS_NO_STREAM) {
		// Music stream playing.  Pause/stop it
		if (psd_in->stop[sample_idx_in] != 0) {
			// STOP field is set, stop music
			LOG(("STOP FIELD SET\n")); //DAR_DEBUG
			LOG((" STOPPING MUSIC STREAM: %u\n", mus_stream.handle)); //DAR_DEBUG

			stop_stream(mus_stream.handle);
			//DAR_TODO trap error?

			channel_ducking[mus_stream.channel_idx] = 1.0f;
			cur_mus_stream = AltsoundStreamInfo();
			music_vol = 1.0f;
		}
		else if (psd_in->ducking[sample_idx_in] < 0.f) {
			// Ducking is less than zero.  Pause music
			LOG(("PAUSING MUSIC STREAM\n")); //DAR_DEBUG

			if (!BASS_ChannelPause(mus_stream.handle)) {
				LOG(("FAILED: BASS_ChannelPause(%u): %s\n", mus_stream.handle, get_bass_err())); //DAR_DEBUG
			}
			else {
				LOG(("SUCCESS: BASS_ChannelPause(%u)\n", mus_stream.handle)); //DAR_DEBUG
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
	if (!create_stream(fname, loops, &jingle_callback, stream_out)) {
		LOG(("FAILED: create_stream(): %s\n", get_bass_err()));

		
		LOG(("END: PROCESS_JINGLE\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: create_stream()\n"));

	// Everything is good.  Update output
	stream_out.stream_type = JINGLE;

	// set ducking for channel
	channel_ducking[stream_out.channel_idx] = psd_in->ducking[sample_idx_in];
	LOG(("JINGLE DUCKING: %.2f\n", channel_ducking[stream_out.channel_idx])); //DAR_DEBUG

	// Set volume for jingle/single
	LOG(("SETTING JINGLE VOLUME\n")); //DAR_DEBUG
	set_volume(stream_out.handle, psd_in->gain[sample_idx_in]);

	
	LOG(("END: PROCESS_JINGLE\n")); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_sfx(PinSamples* psd_in, int sample_idx_in,\
	                                AltsoundStreamInfo& stream_out) {
	LOG(("BEGIN: PROCESS_SFX\n")); //DAR_DEBUG
	

	char* fname = psd_in->files_with_subpath[sample_idx_in];
	int loops = psd_in->loop[sample_idx_in];
	unsigned int ch_idx = 0;

	// create playback stream
	if (!create_stream(fname, loops, &sfx_callback, stream_out)) {
		LOG(("FAILED: create_stream(): %s\n", get_bass_err()));

		
		LOG(("END: PROCESS_SFX\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: create_stream()\n")); //DAR_DEBUG

	// Everything is good.  Update output
	stream_out.stream_type = SFX;

	// set ducking for sfx/voice
	channel_ducking[stream_out.channel_idx] = psd_in->ducking[sample_idx_in];
	LOG(("SFX/VOICE DUCKING: %.2f\n", channel_ducking[stream_out.channel_idx])); //DAR_DEBUG

	// Set volume for sfx/voice
	LOG(("SETTING SFX VOLUME\n")); //DAR_DEBUG
	set_volume(stream_out.handle, psd_in->gain[sample_idx_in]);

	
	LOG(("END: PROCESS_SFX\n")); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::create_stream(const char* fname_in, const int loop_count_in,\
	                                  SYNCPROC* syncproc_in, AltsoundStreamInfo& stream_out)
{
	LOG(("BEGIN: CREATE STREAM\n")); //DAR_DEBUG
	

	// Find an available channel
	unsigned int ch_idx = find_free_channel();

	if (!ch_idx) {
		LOG(("FAILED: find_free_channel()\n")); //DAR_DEBUG

		
		LOG(("END: CREATE_STREAM\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: find_free_channel(): %d\n", ch_idx)); //DAR_DEBUG

	const char* short_fname = get_short_path(fname_in);

	//  DAR@20230517
	// This seems to suggest that either we loop BASS_SAMPLE_LOOP times ONLY
	// if the LOOP field is set to 100, otherwise don't loop at all.  Why?
	//
	// Create playback stream
	HSTREAM hstream = BASS_StreamCreateFile(FALSE, fname_in, 0, 0, \
		(loop_count_in == 100) ? BASS_SAMPLE_LOOP : 0);
	if (hstream == BASS_NO_STREAM) {
		// Failed to create stream
		LOG(("FAILED: BASS_StreamCreateFile(%s): %s\n", \
			get_short_path(fname_in), get_bass_err()));

		// clean up
		free_stream(hstream);

		
		LOG(("END: CREATE STREAM\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: BASS_StreamCreateFile(%s): %u\n", get_short_path(fname_in), \
		hstream)); //DAR_DEBUG

	if (syncproc_in) {
		// Set callback to execute when sample playback ends
		HSYNC new_hsync = create_sync(hstream, syncproc_in, ch_idx);

		if (new_hsync == 0) {
			LOG(("FAILED: create_sync()\n"));

			// clean up
			free_stream(hstream);

			
			LOG(("END: CREATE STREAM\n")); //DAR_DEBUG
			return FALSE;
		}
		else {
			LOG(("SUCCESS: create_sync()\n")); //DAR_DEBUG
		}
	}

	// Everything is good.  Update external data
	stream_out.channel_idx = ch_idx;
	stream_out.handle = hstream;

	
	LOG(("END: CREATE STREAM\n")); //DAR_DEBUG
	return TRUE;
}

// ----------------------------------------------------------------------------

HSYNC AltsoundProcessor::create_sync(HSTREAM stream_in, SYNCPROC* syncproc_in, \
	                                 unsigned int stream_idx_in) {
	LOG(("BEGIN: CREATE_SYNC\n")); //DAR_DEBUG
	

	// Set callback to execute when sample playback ends
	HSYNC new_hsync = BASS_ChannelSetSync(stream_in, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, \
		syncproc_in, (void*)this);
	if (new_hsync == 0) {
		LOG(("FAILED: BASS_ChannelSetSync(): STREAM: %u ERROR: %s\n", stream_in, \
			get_bass_err()));

		
		LOG(("END: CREATE_SYNC\n")); //DAR_DEBUG
		return 0;
	}
	else {
		LOG(("SUCCESS: BASS_ChannelSetSync(): STREAM: %u  HSYNC: %u\n", stream_in, new_hsync)); //DAR_DEBUG
	}

	
	LOG(("END: CREATE_SYNC\n")); //DAR_DEBUG
	return new_hsync;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::stop_stream(HSTREAM stream_in) {
	LOG(("BEGIN: STOP_STREAM\n")); //DAR_DEBUG
	

	if (!BASS_ChannelStop(stream_in)) {
		LOG(("FAILED: BASS_ChannelStop(%u): %s\n", stream_in, get_bass_err())); //DAR_DEBUG

		
		LOG(("END: STOP_STREAM\n")); //DAR_DEBUG
		return FALSE;
	}
	else {
		LOG(("SUCCESS: BASS_ChannelStop(%u)\n", stream_in)); //DAR_DEBUG
	}

	// Free BASS stream
	// Note: this call also removes any predefined BASS sync
	if (!free_stream(stream_in)) {
		LOG(("FAILED: free_stream(%u)\n", stream_in));

		
		LOG(("END: STOP_STREAM\n")); //DAR_DEBUG
		return FALSE;
	}
	LOG(("SUCCESS: free_stream(%u)\n", stream_in));

	
	LOG(("END: STOP_STREAM\n")); //DAR_DEBUG

	return TRUE;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::stop_music_stream() {
	return stop_stream(cur_mus_stream.handle);
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::free_stream(HSTREAM stream_in) {
	LOG(("BEGIN: FREE_STREAM\n")); //DAR_DEBUG
	

	if (!BASS_StreamFree(stream_in)) {
		LOG(("FAILED: BASS_StreamFree(%u): %s\n", stream_in, get_bass_err())); //DAR_DEBUG

		
		LOG(("END: FREE_STREAM\n")); //DAR_DEBUG
		return FALSE;
	}
	else {
		LOG(("SUCCESS: BASS_StreamFree(%u)\n", stream_in)); //DAR_DEBUG
	}

	
	LOG(("END: FREE_STREAM\n")); //DAR_DEBUG
	return TRUE;
}

// ---------------------------------------------------------------------------

int AltsoundProcessor::set_volume(HSTREAM stream_in, const float vol_in) {
	LOG(("BEGIN: SET_VOLUME\n")); //DAR_DEBUG
	

	float new_vol = vol_in * global_vol * master_vol;
	LOG(("VOL: %.2f (GAIN: %.2f  GLOBAL_VOL: %.2f  MASTER_VOL %.2f)\n", new_vol, vol_in, global_vol, master_vol)); //DAR_DEBUG

	if (!BASS_ChannelSetAttribute(stream_in, BASS_ATTRIB_VOL, new_vol)) {
		LOG(("FAILED: BASS_ChannelSetAttribute(%u, BASS_ATTRIB_VOL, %.2f): %s\n", stream_in, new_vol, get_bass_err()));
		//DAR_TODO trap error?
	}
	else {
		LOG(("SUCCESS: BASS_ChannelSetAttribute(%u, BASS_ATTRIB_VOL, %.2f)\n", stream_in, new_vol)); //DAR_DEBUG
	}

	
	LOG(("END: SET_VOLUME\n")); //DAR_DEBUG
	return 0;
}

// ---------------------------------------------------------------------------

float AltsoundProcessor::get_min_ducking() {
	LOG(("BEGIN: GET_MIN_DUCKING\n")); //DAR_DEBUG
	

	float min_ducking = 1.f;
	int num_x_streams = 0;

	// NOTE: channel_steam[0] is reserved for uninitialized streams
	for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i].handle != BASS_NO_STREAM) {
			// stream is playing on the channel
			LOG(("CHANNEL_STREAM[%d]: ACTIVE\n", i)); //DAR_DEBUG
			num_x_streams++;

			float channel_duck = channel_ducking[i];
			LOG(("CHANNEL_DUCKING[%d]: %.2f\n", i, channel_duck)); //DAR_DEBUG

			// update new_ducking if needed
			if (channel_duck < 0) {
				// Special case.  Jingle ducking < 0 pauses music stream.
				// We don't want it to influence ducking of the rest of
				// the samples.
				LOG(("DUCKING < 0. SKIPPING...\n")); //DAR_DEBUG
				continue;
			}
			else if (channel_duck < min_ducking) {
				min_ducking = channel_duck;
			}
		}
	}
	LOG(("NUM STREAMS ACTIVE: %d\n", num_x_streams));
	LOG(("MIN DUCKING OF ALL ACTIVE CHANNELS: %.2f\n", min_ducking)); //DAR_DEBUG

	
	LOG(("END: GET_MIN_DUCKING\n")); //DAR_DEBUG
	return min_ducking;
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::jingle_callback(HSYNC handle, DWORD channel, DWORD data, void *user) {
	// All BASS sync function run on the same thread.  A running SYNCPROC will
	// block others from running, so these functions should be fast.
	LOG(("\nBEGIN: JINGLE_CALLBACK\n"));

	LOG(("ACQUIRING MUTEX\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("MUTEX ACQUIRED\n")); //DAR_DEBUG
	

	// DAR@20230613
	// Passed in instance of this class to avoid having to make everything
	// static.  Only the callback function itself needs to be static
	AltsoundProcessor* proc = static_cast<AltsoundProcessor*>(user);
	HSTREAM stream_in = static_cast<HSTREAM>(channel);
	StreamArray& streams = proc->getStreams();
	DuckingArray& ducking = proc->getStreamDucking();

	AltsoundStreamInfo& mus_stream = proc->getMusicStream();
	AltsoundStreamInfo& jin_stream = proc->getJingleStream();

	if (stream_in != jin_stream.handle) {
		// This is not the expected music stream.  Get out
		LOG(("Stream(%u) does not match current JINGLE stream.  Exiting.\n",\
			 stream_in)); //DAR_DEBUG

		
		LOG(("\nEND: MUSIC_CALLBACK\n"));
		return;
	}

	unsigned int ch_idx = jin_stream.channel_idx;

	LOG(("HANDLE: %u  CHANNEL: %u\n", handle, channel)); //DAR_DEBUG
	LOG(("JINGLE STREAM(%u) FINISHED ON CH(%02d)\n", stream_in, ch_idx)); //DAR_DEBUG


	// free stream resources
	if (!proc->free_stream(stream_in)) {
		LOG(("FAILED: free_stream(%u): %s\n", stream_in, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("SUCCESS: free_stream(%u)\n", stream_in)); //DAR_DEBUG
	}

	// reset tracking variables
	streams[ch_idx] = AltsoundStreamInfo();
	ducking[ch_idx] = 1.0f;
	proc->setJingleStream(streams[ch_idx]);

	// re-calculate music ducking based on active channels.
	LOG(("ADJUSTING MUSIC VOLUME\n")); //DAR_DEBUG

	if (mus_stream.channel_idx == 0) {
		LOG(("NO MUSIC STREAM.  SKIPPING...\n"));

		
		LOG(("END: JINGLE_CALBACK\n")); //DAR_DEBUG
		return;
	}

	float min_ducking = proc->get_min_ducking();
	LOG(("MIN DUCKING VALUE: %.2f\n", min_ducking)); //DAR_DEBUG

	float adj_mus_vol = proc->getGlobalMusicVol() * min_ducking;
	proc->set_volume(mus_stream.handle, adj_mus_vol);

	// Resume paused music playback
	if (BASS_ChannelIsActive(mus_stream.handle) == BASS_ACTIVE_PAUSED) {
		LOG(("RESUMING MUSIC PLAYBACK\n")); //DAR_DEBUG

		if (!BASS_ChannelPlay(mus_stream.handle, 0)) {
			LOG(("FAILED: BASS_ChannelPlay(%u): %s\n", mus_stream.handle, \
				get_bass_err())); //DAR_DEBUG
		}
		else {
			LOG(("SUCCESS: BASS_ChannelPlay(%u)\n", mus_stream.handle)); //DAR_DEBUG
		}
	}

	
	LOG(("END: JINGLE_CALLBACK\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::sfx_callback(HSYNC handle, DWORD channel,\
	                                          DWORD data, void *user) {
	// All BASS sync function run on the same thread.  A running SYNCPROC will
	// block others from running, so these functions should be fast.
	LOG(("\nBEGIN: SFX_CALLBACK\n"));

	LOG(("ACQUIRING MUTEX\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("MUTEX ACQUIRED\n")); //DAR_DEBUG
	

	// DAR@20230613
	// Passed in instance of this class to avoid having to make everything
	// static.  Only the callback function itself needs to be static
	AltsoundProcessor* proc = static_cast<AltsoundProcessor*>(user);
	HSTREAM stream_in = static_cast<HSTREAM>(channel);
	StreamArray& streams = proc->getStreams();
	DuckingArray& ducking = proc->getStreamDucking();

	AltsoundStreamInfo& mus_stream = proc->getMusicStream();
	AltsoundStreamInfo& jin_stream = proc->getJingleStream();

	if (stream_in == mus_stream.handle || stream_in == jin_stream.handle) {
		// This is not an sfx stream.  Get out
		LOG(("Stream(%u) is not an SFX stream.  Exiting.\n", stream_in)); //DAR_DEBUG

		
		LOG(("\nEND: SFX_CALLBACK\n"));
		return;
	}

	// find stream
	int idx = 0;
	for (; idx < streams.size(); idx++) {
		if (streams[idx].handle == stream_in)
			break;
	}

	if (idx == streams.size()) {
		// music stream not found
		LOG(("ERROR: Stream %u not found!\n", channel)); // DAR_DEBUG
		
		LOG(("\nEND: MUSIC_CALLBACK\n"));
		return;
	}
	AltsoundStreamInfo sfx_stream = streams[idx];
	unsigned int ch_idx = sfx_stream.channel_idx;

	LOG(("HANDLE: %u  STREAM: %u\n", handle, stream_in)); //DAR_DEBUG
	LOG(("MUSIC STREAM(%u) FINISHED ON CH(%02d)\n", stream_in, ch_idx)); //DAR_DEBUG

	// free stream resources
	if (!proc->free_stream(sfx_stream.handle)) {
		LOG(("FAILED: free_stream(%u): %s\n", sfx_stream.handle, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("SUCCESS: free_stream(%u)\n", sfx_stream.handle)); //DAR_DEBUG
	}

    // reset tracking variables
	streams[ch_idx] = AltsoundStreamInfo();
	ducking[ch_idx] = 1.0f;

	// re-calculate music ducking based on active channels.
	LOG(("ADJUSTING MUSIC VOLUME\n")); //DAR_DEBUG

	if (mus_stream.channel_idx == 0) {
		LOG(("NO MUSIC STREAM.  SKIPPING...\n"));

		
		LOG(("END: SFX_CALBACK\n")); //DAR_DEBUG
		return;
	}

	float min_ducking = proc->get_min_ducking();
	LOG(("MIN DUCKING VALUE: %.2f\n", min_ducking)); //DAR_DEBUG

	float adj_mus_vol = proc->getGlobalMusicVol() * min_ducking;
	proc->set_volume(mus_stream.handle, adj_mus_vol);

	
	LOG(("END: SFX_CALLBACK\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::music_callback(HSYNC handle, DWORD channel,\
	                                            DWORD data, void *user) {
	// DAR@20230523
	// Normally, music is stopped programmatically in the main code.  This is
	// here in case a music stream ends before it is stopped by the main code.
	// This really should never happen.
	//
	//DAR@20230519 SYNCPROC functions run in a separate thread, and will block
	//             other sync processes, so these should be fast and care
	//             must be taken for thread safety with the main thread
	LOG(("BEGIN: MUSIC_CALLBACK\n")); //DAR_DEBUG

	LOG(("ACQUIRING MUTEX\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	LOG(("MUTEX ACQUIRED\n")); //DAR_DEBUG
	

	// DAR@20230613
	// Passed in instance of this class to avoid having to make everything
	// static.  Only the callback function itself needs to be static
	AltsoundProcessor* proc = static_cast<AltsoundProcessor*>(user);
	HSTREAM stream_in = static_cast<HSTREAM>(channel);
	StreamArray& streams = proc->getStreams();
	DuckingArray& ducking = proc->getStreamDucking();
	AltsoundStreamInfo mus_stream = proc->getMusicStream();

	if (mus_stream.handle != stream_in) {
		// This is not the expected music stream.  Get out
		LOG(("Stream(%u) does not match current MUSIC stream.  Exiting.\n", stream_in)); //DAR_DEBUG

		
		LOG(("\nEND: MUSIC_CALLBACK\n"));
		return;
	}
	unsigned int ch_idx = mus_stream.channel_idx;

	LOG(("HANDLE: %u  STREAM: %u\n", handle, stream_in)); //DAR_DEBUG
	LOG(("MUSIC STREAM(%u) FINISHED ON CH(%02d)\n", stream_in, ch_idx)); //DAR_DEBUG

    // free stream resources
	if (!proc->free_stream(mus_stream.handle)) {
		LOG(("FAILED: free_stream(%u): %s\n", mus_stream.handle, get_bass_err())); //DAR_DEBUG
	}
	else {
		LOG(("SUCCESS: free_stream(%u)\n", mus_stream.handle)); //DAR_DEBUG
	}

	// reset tracking variables
	streams[ch_idx] = AltsoundStreamInfo();
	ducking[ch_idx] = 1.0f;
	proc->setMusicStream(streams[ch_idx]);
	proc->setGlobalMusicVol(1.0f);

	
	LOG(("END: MUSIC_CALLBACK\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

int AltsoundProcessor::find_free_channel(void) {
	LOG(("BEGIN: FIND_FREE_CHANNEL\n"));
	
	
	int channel_idx = 0;

	// NOTE: channel_stream[0] is reserved for uninitialized streams
	for (int i = 1; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i].handle == BASS_NO_STREAM) {
			channel_idx = i;
			break;
		}
	}

	if (channel_idx == 0) {
		LOG(("NO FREE CHANNELS AVAILABLE!\n"));
	}
	else {
		LOG(("FOUND FREE CHANNEL: %d\n", channel_idx));
	}

	
	LOG(("END: FIND_FREE_CHANNEL\n"));
	return channel_idx;
}

// ---------------------------------------------------------------------------
// Helper function to remove major path from filenames.  Returns just:
// <shortname>/<rest of path>
// ---------------------------------------------------------------------------

const char* AltsoundProcessor::get_short_path(const char* path_in) {
	const char* tmp_str = strstr(path_in, game_name.c_str());

	if (tmp_str) {
		return tmp_str;
	}
	else {
		return path_in;
	}
}

