// ---------------------------------------------------------------------------
// altsound_processor.cpp
// Dave Roscoe 06/14/2023
//
// Encapsulates all specialized processing for the legacy Altsound
// CSV and PinSound format
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "altsound_processor.hpp"

// Std Library includes
#include <algorithm>
#include <string>

// Local includes
#include "osdepend.h"
#include "altsound_csv_parser.hpp"
#include "altsound_file_parser.hpp"

using std::string;

// NOTE:
// - SFX streams don't require tracking since multiple can play,
//   simultaneously, and other than adjusting ducking, have no other
//   impacts on active streams
//
static AltsoundStreamInfo* cur_mus_stream = nullptr;
static AltsoundStreamInfo* cur_jin_stream = nullptr;

// windef.h "min" conflicts with std::min
#ifdef min
  #undef min
#endif

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

AltsoundProcessor::AltsoundProcessor(const char* gname_in,
	                                 const string& format_in)
:AltsoundProcessorBase(string(gname_in)),
  format(format_in),
  is_initialized(false),
  is_stable(true) // future use
{
	// initialize sample storage
	psd.ID = 0;
	psd.files_with_subpath = 0;
	psd.channel = 0;
	psd.gain = 0;
	psd.ducking = 0;
	psd.loop = 0;
	psd.stop = 0;
	psd.num_files = 0;

	// perform initialization (load samples, etc)
	init();
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

	// DAR@20230624
	// cur_mus_stream and cur_jin_stream are copies of pointers stored in
	// channel_stream[].  They cannot be deleted here, since they will be
	// deleted when the channel_stream[] is destroyed.  Just set to null
	// here
	//
	// clean up stream tracking
	cur_mus_stream = NULL;
	cur_jin_stream = NULL;

	stopAllStreams();
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool AltsoundProcessor::handleCmd(const unsigned int cmd_combined_in)
{
	LOG(("BEGIN AltsoundProcessor::handleCmd()\n")); //DAR_DEBUG

	LOG(("- Acquiring mutex...\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);

	if (!is_initialized || !is_stable) {
		if (!is_initialized) {
			LOG(("- AltsoundProcessor is not initialized. Processing skipped\n"));
		}
		else {
			LOG(("- AltsoundProcessor is unstable. Processing skipped\n"));
		}
		
		LOG(("END: AltsoundProcessor::handleCmd()\n")); //DAR_DEBUG
		return false;
	}

	// get sample for playback
	int sample_idx = getSample(cmd_combined_in);

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		LOG(("- FAILED: get_sample()\n", cmd_combined_in)); //DAR_DEBUG

		//LOG(("END: AltsoundProcessor::handleCmd()\n")); //DAR_DEBUG
		return false;
	}
	LOG(("- SUCCESS: get_sample(%04X)\n", cmd_combined_in));

	BOOL play_music = FALSE;
	BOOL play_jingle = FALSE;
	BOOL play_sfx = FALSE;

	AltsoundStreamInfo* new_stream = new AltsoundStreamInfo();
	HSTREAM stream = BASS_NO_STREAM;

	// pre-populate stream info
	new_stream->sample_path = psd.files_with_subpath[sample_idx];
	new_stream->stop_music  = psd.stop[sample_idx] != 0;
	new_stream->ducking     = psd.ducking[sample_idx];
	new_stream->loop        = psd.loop[sample_idx] == 100;
	new_stream->gain        = psd.gain[sample_idx];

	unsigned int sample_channel = psd.channel[sample_idx];
	if (sample_channel == 1) {
		// Command is for playing Jingle/Single
		new_stream->stream_type = JINGLE;

		if (process_jingle(new_stream)) {
			LOG(("- SUCCESS: process_jingle()\n")); //DAR_DEBUG
			
            // update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;
			
			play_jingle = TRUE; // Defer playback until the end
			cur_jin_stream = new_stream;
			stream = cur_jin_stream->hstream;
		}
		else {
			// An error ocurred during processing
			LOG(("- FAILED: process_jingle()\n")); //DAR_DEBUG
		}
	}

	if (sample_channel == 0) {
		// Command is for playing music
		new_stream->stream_type = MUSIC;

		if (process_music(new_stream)) {
			LOG(("- SUCCESS: process_music()\n")); //DAR_DEBUG

			// update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;
		   
			play_music = TRUE; // Defer playback until the end
			cur_mus_stream = new_stream;
			stream = cur_mus_stream->hstream;
		}
		else {
			// An error ocurred during processing
			LOG(("- FAILED: process_music()\n")); //DAR_DEBUG
		}
	}

	if (sample_channel == -1) {
		// Command is for playing voice/sfx
		new_stream->stream_type = SFX;

 		if (process_sfx(new_stream)) {
			LOG(("- SUCCESS: process_sfx()\n")); //DAR_DEBUG

			// update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;
		    
			play_sfx = TRUE;// Defer playback until the end
			stream = new_stream->hstream;
		}
		else {
			// An error ocurred during processing
			LOG(("- FAILED: process_sfx()\n")); //DAR_DEBUG
		}
	}

	if (!play_music && !play_jingle && !play_sfx) {
		LOG(("- FAILED: alt_sound_handle()\n")); //DAR_DEBUG

		//LOG(("END: AltsoundProcessor::process_cmd()\n")); //DAR_DEBUG
		return false;
	}

	// Get lowest ducking value from all channels (including music, jingle)
	float min_ducking = getMinDucking();
	LOG(("- MIN DUCKING VALUE: %.2f\n", min_ducking)); //DAR_DEBUG

	// set new music volume	
	if (cur_mus_stream) {
		// calculate ducked volume for music
		float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(cur_mus_stream->hstream, adj_mus_vol);
	}
	else {
		// No music is currently playing.  If a music file starts later,
		// the code above will ensure the volume will be set correctly
		LOG(("- NO MUSIC STREAM. SKIPPING.\n")); //DAR_DEBUG
	}

	// Play pending sound determined above, if any
	if (new_stream->hstream != BASS_NO_STREAM) {
		if (!BASS_ChannelPlay(stream, 0)) {
			// Sound playback failed
			LOG(("- FAILED: BASS_ChannelPlay(%u): %s\n", new_stream->hstream,\
				 get_bass_err())); //DAR_DEBUG
		}
		else {
			LOG(("- SUCCESS: BASS_ChannelPlay(%u): CH(%d) CMD(%04X) SAMPLE(%s)\n", \
				new_stream->hstream, new_stream->channel_idx, cmd_combined_in, \
				getShortPath(new_stream->sample_path).c_str()));
		}
	}
	
	LOG(("END: AltsoundProcessor::handleCmd()\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

void AltsoundProcessor::init()
{
	// reset stream tracking
	cur_mus_stream = NULL;
	cur_jin_stream = NULL;

	//LOG(("BEGIN AltsoundProcessor::init()\n")); //DAR_DEBUG
	if (!loadSamples()) {
		LOG(("FAILED: AltsoundProcessor::loadSamples()\n"));

		//LOG(("END: AltsoundProcessor::init()\n")); //DAR_DEBUG
	}
	LOG(("SUCCESS: AltsoundProcessor::loadSamples()\n"));

	// if we are here, initialization succeeded
	is_initialized = true;

	//LOG(("END: AltsoundProcessor::init()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::loadSamples() {
	//LOG(("BEGIN loadSamples()\n")); //DAR_DEBUG

	// No files yet
	psd.num_files = 0;

	if (format == "altsound") {
		AltsoundCsvParser csv_parser(game_name.c_str());

		if (!csv_parser.parse(&psd)) {
			LOG(("- FAILED: csv_parser.parse()\n")); //DAR_DEBUG

			//LOG(("END: AltsoundProcessor::init()\n")); //DAR_DEBUG
			return is_initialized;
		}
		LOG(("- SUCCESS: csv_parser.parse()\n")); //DAR_DEBUG
	}
	else if (format == "pinsound") {
		AltsoundFileParser file_parser(game_name.c_str());
		if (!file_parser.parse(&psd)) {
			LOG(("- FAILED: file_parser.parse()\n")); //DAR_DEBUG

			//LOG(("END: loadSamples\n")); //DAR_DEBUG
			return false;
		}
		else {
			LOG(("- SUCCESS: file_parser.parse()\n")); //DAR_DEBUG
		}
	}
	
	//LOG(("END: loadSamples\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

int AltsoundProcessor::getSample(const unsigned int cmd_combined_in) {
	//LOG(("BEGIN getSample()\n")); //DAR_DEBUG

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
			LOG(("- SUCCESS: Found %d sample(s) for ID: %04X\n", num_samples, cmd_combined_in)); //DAR_DEBUG

			// num_samples now contains the number of samples with the same ID
			// pick one to play at random
			sample_idx = i + rand() % num_samples;
			break;
		}
	}

	if (sample_idx == -1) {
		LOG(("- FAILED: No sample(s) found for ID: %04X\n", cmd_combined_in)); //DAR_DEBUG
	}
	
	//LOG(("END: getSample()\n")); //DAR_DEBUG
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_music(AltsoundStreamInfo* stream_out)
{
	LOG(("BEGIN process_music()\n"));

	bool success = false;

	// stop current music stream
	if (stopMusicStream()) {
		// create new music stream
		if (createStream(&music_callback, stream_out)) {
			// set music volume.  Will get ducked as needed later
			if (setStreamVolume(stream_out->hstream, stream_out->gain)) {
				success = true;
			}
			else {
				LOG(("- FAILED: setStreamVolume()\n"));
			}
		}
		else {
			LOG(("FAILED: createStream()\n"));
		}
	}
	else {
		LOG(("- FAILED: stopMusicStream()\n"));
	}

	LOG(("END: process_music()\n"));
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_jingle(AltsoundStreamInfo* stream_out)
{
	LOG(("BEGIN: process_jingle()\n"));
	bool success = true;

	// DAR@20230621
	// We are intentionally NOT setting "success" boolean here.  We
	// want to know if the operations below fails, but it doesn't affect
	// the ability to stop/create a jingle stream
	//
	// handle impact of jingle steam on current music stream
	if (cur_mus_stream) {
		if (stream_out->stop_music) {
			// STOP field set. Stop current music stream
			if (!stopMusicStream()) {
				//success = false;
				LOG(("- FAILED: stopMusicStream()\n"));
			}
		}
		else if (stream_out->ducking < 0.0f) {
			// Pause current music stream
			if (BASS_ChannelPause(cur_mus_stream->hstream)) {
				//success = false;
				LOG(("- FAILED: BASS_ChannelPause(): %s\n", get_bass_err()));
			}
		}
	}

	// handle jingle stream
	if (cur_jin_stream) {
		// stop current jingle stream
		success = stopJingleStream();
		if (!success) {
			LOG(("- FAILED: stopJingleStream()\n"));
		}
	}
	
	if (success) {
		success = createStream(&jingle_callback, stream_out);
		if (success) {
			success = setStreamVolume(stream_out->hstream, stream_out->gain);
			if (!success) {
				LOG(("- FAILED: setStreamVolume()\n")); // DAR_DEBUG
			}
		}
		else {
			LOG(("- FAILED: create_stream(): %s\n", get_bass_err()));
		}
	}

	LOG(("END: process_jingle()\n")); // DAR_DEBUG
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_sfx(AltsoundStreamInfo* stream_out)
{
	LOG(("BEGIN process_sfx()\n"));

	// create new sfx stream
	bool success = createStream(&sfx_callback, stream_out);
	if (success && !setStreamVolume(stream_out->hstream, stream_out->gain)) {
		success = false;
		LOG(("- FAILED: setStreamVolume()\n"));
	}
	else if (!success) {
		LOG(("- FAILED: createStream(): %s\n", get_bass_err()));
	}

	LOG(("END: process_sfx()\n"));
	return success;
}

// ---------------------------------------------------------------------------
// This method should ONLY be called by snd_alt.cpp
// This function exists to provide an external interface to stop music
// streams.  Some derivations of the base class require post-processing
// after calling stopMusicStreams, which cannot be replicated if called
// directly by an external function
// ---------------------------------------------------------------------------

bool AltsoundProcessor::stopMusic()
{
	if (!stopMusicStream()) {
		LOG(("FAILED: stopMusicStream()\n"));
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
bool AltsoundProcessor::stopMusicStream()
{
	// LOG(("BEGIN stopMusicStream()\n"));
	bool success = true;

	if (cur_mus_stream) {
		HSTREAM hstream = cur_mus_stream->hstream;
		unsigned int ch_idx = cur_mus_stream->channel_idx;
		LOG(("Current MUSIC stream(%s): HSTREAM: %u  CH: %02d\n",
			  getShortPath(cur_mus_stream->sample_path).c_str(), hstream,
			  cur_mus_stream->channel_idx));

		if (stopStream(hstream)) {
			LOG(("- Stopped MUSIC stream: %u  Chan: %02d\n", hstream, ch_idx));
			delete channel_stream[ch_idx];
			channel_stream[ch_idx] = nullptr;
			cur_mus_stream = nullptr;
		}
		else {
			success = false;
			LOG(("- FAILED: stopStream(%u)\n", hstream));
		}
	}

	// LOG(("END: stopMusicStream()\n"));
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::stopJingleStream()
{
	// LOG(("BEGIN stopJingleStream()\n"));
	bool success = false;

	if (cur_jin_stream) {
		HSTREAM hstream = cur_jin_stream->hstream;
		unsigned int ch_idx = cur_jin_stream->channel_idx;

		if (stopStream(hstream)) {
			LOG(("- Stopped JINGLE stream: %u  Chan: %02d\n", hstream, ch_idx));
			delete channel_stream[ch_idx];
			channel_stream[ch_idx] = nullptr;
			cur_jin_stream = nullptr;
			success = true;
		}
		else {
			LOG(("- FAILED: stopStream(%u)\n", hstream));
		}
	}

	// LOG(("END: stopJingleStream()\n"));
	return success;
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::jingle_callback(HSYNC handle, DWORD channel,\
	                                             DWORD data, void* user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("\nBEGIN: jingle_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel));

	LOG(("- Acquiring mutex...\n"));
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (!cur_jin_stream) {
		LOG(("Jingle callback hit, but no jingle stream set\n"));
	}
	else if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != JINGLE) {
		LOG(("instance HSTREAM is NOT a JINGLE stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;

	LOG(("- JINGLE stream(%u) finished on ch(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err())); // DAR_DEBUG
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;
	cur_jin_stream = nullptr;

	if (cur_mus_stream) {
		HSTREAM mus_hstream = cur_mus_stream->hstream;
		LOG(("- Adjusting MUSIC volume\n")); // DAR_DEBUG

		// re-calculate music ducking based on active channels.
		float min_ducking = getMinDucking();
		LOG(("- Min ducking value: %.2f\n", min_ducking)); // DAR_DEBUG

		// set new music volume
		float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(mus_hstream, adj_mus_vol);

		// DAR@20230622
		// This is a kludgy way to make sure we only resume paused playback
		// when the stream that paused it ends
		if (stream_inst->ducking < 0.0f && BASS_ChannelIsActive(mus_hstream) == BASS_ACTIVE_PAUSED) {
			LOG(("- Resuming MUSIC playback\n")); // DAR_DEBUG

			if (!BASS_ChannelPlay(mus_hstream, 0)) {
				LOG(("- FAILED: BASS_ChannelPlay(%u): %s\n", mus_hstream, get_bass_err()));
			}
		}
	}

	LOG(("END: jingle_callback()\n"));
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::sfx_callback(HSYNC handle, DWORD channel,\
	                                          DWORD data, void *user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("\nBEGIN: sfx_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel)); //DAR_DEBUG

	LOG(("- Acquiring mutex...\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	
	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);
	
	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != SFX) {
		LOG(("instance HSTREAM is NOT a SFX stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;
		
	LOG(("- SFX stream(%u) finished on CH(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err())); // DAR_DEBUG
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;

	if (cur_mus_stream) {
		HSTREAM mus_hstream = cur_mus_stream->hstream;
		LOG(("- Adjusting MUSIC volume\n")); // DAR_DEBUG

		// re-calculate music ducking based on active channels.
		float min_ducking = getMinDucking();
		LOG(("- Min ducking value: %.2f\n", min_ducking)); // DAR_DEBUG

		// set new music volume
		float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(mus_hstream, adj_mus_vol);
	}
	
	LOG(("END: sfx_callback()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::music_callback(HSYNC handle, DWORD channel,\
	                                            DWORD data, void *user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("BEGIN music_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel));

	LOG(("- Acquiring mutex...\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);
	
	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (!cur_jin_stream) {
		LOG(("MUSIC callback hit, but no MUSIC stream set\n"));
	}
	else if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != MUSIC) {
		LOG(("instance HSTREAM is NOT a MUSIC stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;

	LOG(("- MUSIC stream(%u) finished on ch(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err()));
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;
	cur_mus_stream = nullptr;
	
	LOG(("END: music_callback()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

float AltsoundProcessor::getMinDucking()
{
	//LOG(("BEGIN: getMinDucking()\n"));

	float min_ducking = 1.0f;
	int num_x_streams = 0;

	for (int index = 0; index < channel_stream.size(); ++index) {
		const auto stream = channel_stream[index];
		if (stream) {
			// stream defined on the channel
			LOG(("- channel_stream[%d]: STREAM: %u  DUCKING: %0.02f\n", index, \
				stream->hstream, stream->ducking));

			num_x_streams++;

			if (stream->ducking < 0) {
				// Special case. Jingle ducking < 0 pauses music stream in
				// traditional altsound packages. We don't want it to
				// influence actual ducking, since it would always be the
				// lowest value of all other samples.
				LOG(("- Ducking value < 0. Skipping...\n")); // DAR_DEBUG
				continue;
			}

			min_ducking = std::min(min_ducking, stream->ducking);
		}
	}

	LOG(("- Num active streams: %d\n", num_x_streams));
	LOG(("- Min ducking of all active channels: %.2f\n", min_ducking)); // DAR_DEBUG

	//LOG(("END: getMinDucking()\n")); // DAR_DEBUG
	return min_ducking;
}