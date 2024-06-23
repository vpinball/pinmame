// ---------------------------------------------------------------------------
// altsound_processor.cpp
//
// Encapsulates all specialized processing for the legacy Altsound
// CSV and PinSound format
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe, Carsten Wächter
// ---------------------------------------------------------------------------

#define NOMINMAX

#include "altsound_processor.hpp"

// Std Library includes
#include <algorithm>
#include <string>

// Local includes
#include "osdepend.h"
#include "altsound_csv_parser.hpp"
#include "altsound_file_parser.hpp"
#include "altsound_logger.hpp"

using std::string;

// NOTE:
// - SFX streams don't require tracking since multiple can play,
//   simultaneously, and other than adjusting ducking, have no other
//   impacts on active streams
//
static AltsoundStreamInfo* cur_mus_stream = nullptr;
static AltsoundStreamInfo* cur_jin_stream = nullptr;

// Instance of global thread synchronization mutex
extern std::mutex io_mutex;

// Instance of global array of BASS channels 
extern StreamArray channel_stream;

// Reference to global logger instance
extern AltsoundLogger alog;

constexpr unsigned int UNSET_IDX = std::numeric_limits<unsigned int>::max();

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

AltsoundProcessor::AltsoundProcessor(const std::string& game_name_in,
	                                 const std::string& vpm_path_in,
	                                 const string& format_in)
:AltsoundProcessorBase(game_name_in, vpm_path_in),
  format(format_in),
  is_initialized(false),
  is_stable(true) // future use
{
}

AltsoundProcessor::~AltsoundProcessor()
{
	// DAR@20230624
	// cur_mus_stream and cur_jin_stream are copies of pointers stored in
	// channel_stream[].  They cannot be deleted here, since they will be
	// deleted when the channel_stream[] is destroyed.  Just set to null
	// here
	//
	// clean up stream tracking
	cur_mus_stream = nullptr;
	cur_jin_stream = nullptr;

	stopAllStreams();
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool AltsoundProcessor::handleCmd(const unsigned int cmd_combined_in)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::handleCmd()");
	INDENT;

	ALT_DEBUG(0, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	// Pass command to base class for processing
	AltsoundProcessorBase::handleCmd(cmd_combined_in);

	if (!is_initialized || !is_stable) {
		if (!is_initialized) {
			ALT_ERROR(0, "AltsoundProcessor is not initialized. Processing skipped");
		}
		else {
			ALT_ERROR(0, "AltsoundProcessor is unstable. Processing skipped");
		}

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundProcessor::handleCmd()");
		return false;
	}

	// Skip this command?
	unsigned int skip_count = AltsoundProcessorBase::getSkipCount();
	if (skip_count > 0) {
		--skip_count;
		AltsoundProcessorBase::setSkipCount(skip_count);
		ALT_DEBUG(0, "Sound command skipped, (%d) remaining", skip_count);
		ALT_DEBUG(0, "END AltsoundProcessor::handleCmd()");
		return true;
	}

	// get sample for playback
	const unsigned int sample_idx = getSample(cmd_combined_in);

	if (sample_idx == UNSET_IDX) {
		// No matching command.  Clean up and exit
		ALT_ERROR(0, "FAILED AltsoundProcessor::get_sample(%u)", cmd_combined_in);

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundProcessor::handleCmd()");
		return false;
	}

	BOOL play_music = FALSE;
	BOOL play_jingle = FALSE;
	BOOL play_sfx = FALSE;

	AltsoundStreamInfo* new_stream = new AltsoundStreamInfo();
	HSTREAM stream = BASS_NO_STREAM;

	// pre-populate stream info
	new_stream->sample_path = samples[sample_idx].fname;
	new_stream->stop_music  = samples[sample_idx].stop;
	new_stream->ducking     = samples[sample_idx].ducking;
	new_stream->loop        = samples[sample_idx].loop;
	new_stream->gain        = samples[sample_idx].gain;

	const int sample_channel = samples[sample_idx].channel;

	if (sample_channel == 1) {
		// Command is for playing Jingle/Single
		new_stream->stream_type = JINGLE;

		if (process_jingle(new_stream)) {
			ALT_INFO(0, "SUCCESS AltsoundProcessor::process_jingle()");

			// update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;

			play_jingle = TRUE; // Defer playback until the end
			cur_jin_stream = new_stream;
			stream = cur_jin_stream->hstream;
		}
		else {
			// An error occurred during processing
			ALT_ERROR(0, "FAILED AltsoundProcessor::process_jingle()");
		}
	}

	if (sample_channel == 0) {
		// Command is for playing music
		new_stream->stream_type = MUSIC;

		if (process_music(new_stream)) {
			ALT_INFO(0, "SUCCESS AltsoundProcessor::process_music()");

			// update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;
		   
			play_music = TRUE; // Defer playback until the end
			cur_mus_stream = new_stream;
			stream = cur_mus_stream->hstream;
		}
		else {
			// An error occurred during processing
			ALT_ERROR(0, "FAILED AltsoundProcessor::process_music()");
		}
	}

	if (sample_channel == -1) {
		// Command is for playing voice/sfx
		new_stream->stream_type = SFX;

		if (process_sfx(new_stream)) {
			ALT_INFO(0, "SUCCESS AltsoundProcessor::process_sfx()");

			// update stream storage
			channel_stream[new_stream->channel_idx] = new_stream;

			play_sfx = TRUE;// Defer playback until the end
			stream = new_stream->hstream;
		}
		else {
			// An error occurred during processing
			ALT_ERROR(0, "FAILED AltsoundProcessor::process_sfx()");
		}
	}

	if (!play_music && !play_jingle && !play_sfx) {
		ALT_ERROR(0, "FAILED AltsoundProcessor::alt_sound_handle()");

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundProcessor::process_cmd()");
		return false;
	}

	// Get lowest ducking value from all channels (including music, jingle)
	const float min_ducking = getMinDucking();
	ALT_INFO(0, "Min ducking value: %.02f", min_ducking);

	// set new music volume	
	if (cur_mus_stream) {
		// calculate ducked volume for music
		const float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(cur_mus_stream->hstream, adj_mus_vol);
	}
	else {
		// No music is currently playing.  If a music file starts later,
		// the code above will ensure the volume will be set correctly
		ALT_INFO(0, "No music stream. Skipping.");
	}

	// Play pending sound determined above, if any
	if (new_stream->hstream != BASS_NO_STREAM) {
		if (!BASS_ChannelPlay(stream, 0)) {
			// Sound playback failed
			ALT_ERROR(0, "FAILED BASS_ChannelPlay(%u): %s", new_stream->hstream,\
				  get_bass_err());
		}
		else {
			ALT_INFO(0, "SUCCESS BASS_ChannelPlay(%u): CH(%d) CMD(%04X) SAMPLE(%s)", \
				 new_stream->hstream, new_stream->channel_idx, cmd_combined_in, \
				 getShortPath(new_stream->sample_path).c_str());
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::handleCmd()");
	return true;
}

// ---------------------------------------------------------------------------

void AltsoundProcessor::init()
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::init()");
	OUTDENT;

	// reset stream tracking
	cur_mus_stream = nullptr;
	cur_jin_stream = nullptr;

	if (!loadSamples()) {
		ALT_ERROR(0, "FAILED AltsoundProcessor::loadSamples()");
		is_initialized = false;

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundProcessor::init()");
		return;
	}
	ALT_INFO(0, "SUCCESS AltsoundProcessor::loadSamples()");

	// if we are here, initialization succeeded
	is_initialized = true;

	// Initialize base class
	AltsoundProcessorBase::init();

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::init()");
}

// ----------------------------------------------------------------------------

void AltsoundProcessorBase::setGlobalVol(const float vol_in)
{
	std::vector<float> vol(channel_stream.size());

	for (size_t index = 0; index < channel_stream.size(); ++index) {
		const auto stream = channel_stream[index];
		if (stream) {
			vol[index] = getStreamVolume(stream->hstream);
		}
	}

	global_vol = vol_in;

	for (size_t index = 0; index < channel_stream.size(); ++index) {
		const auto stream = channel_stream[index];
		if (stream) {
			setStreamVolume(stream->hstream, vol[index]);
		}
	}
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::loadSamples()
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::loadSamples()");
	INDENT;

	string altsound_path = vpm_path; // in base class
	if (!altsound_path.empty()) {
		altsound_path += "/altsound/";
		altsound_path += game_name;
	}

	if (format == "altsound") {
		AltsoundCsvParser csv_parser(altsound_path);

		if (!csv_parser.parse(samples)) {
			ALT_ERROR(0, "FAILED AltsoundCsvParser::parse()");

			OUTDENT;
			ALT_DEBUG(0, "END AltsoundProcessor::loadSamples()");
			return false;
		}
		ALT_INFO(0, "SUCCESS AltsoundCsvParser::parse()");
	}
	else if (format == "legacy") {
		AltsoundFileParser file_parser(altsound_path);
		if (!file_parser.parse(samples)) {
			ALT_ERROR(0, "FAILED AltsoundFileParser::parse()");
			
			OUTDENT;
			ALT_DEBUG(0, "END AltsoundProcessor::loadSamples");
			return false;
		}
		ALT_INFO(0, "SUCCESS AltsoundFileParser::parse()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::loadSamples");
	return true;
}

// ---------------------------------------------------------------------------

unsigned int AltsoundProcessor::getSample(const unsigned int cmd_combined_in)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::getSample()");
	INDENT;

	unsigned int sample_idx = UNSET_IDX;

	// Look for sample that matches the current command
	for (size_t i = 0; i < samples.size(); ++i) {
		if (samples[i].id == cmd_combined_in) {
			// Current ID matches the command. Check if there are more samples for this
			// command and randomly pick one
			unsigned int num_samples = 0;
			do {
				num_samples++;

				if (i + num_samples >= samples.size())
					// sample index exceeds the number of samples
					break;

				// Loop while the next sample ID matches the command
			} while (samples[i + num_samples].id == cmd_combined_in);
			ALT_INFO(0, "SUCCESS Found %d sample(s) for ID: %04X", num_samples, cmd_combined_in);

			// num_samples now contains the number of samples with the same ID
			// pick one to play at random
			sample_idx = static_cast<unsigned int>(i) + rand() % num_samples;
			break;
		}
	}

	if (sample_idx == UNSET_IDX) {
		ALT_WARNING(0, "FAILED No sample(s) found for ID: %04X", cmd_combined_in);
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::getSample()");
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_music(AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::process_music()");
	INDENT;

	bool success = false;

	// stop current music stream
	if (stopMusicStream()) {
		// DAR@20230703
		// If you set a callback for end-of-stream for MUSIC streams,
		// the looping behavior will not work.
		//
		// create new music stream
		if (createStream(nullptr, stream_out)) {
			// set music volume.  Will get ducked as needed later
			if (setStreamVolume(stream_out->hstream, stream_out->gain)) {
				success = true;
			}
			else {
				ALT_ERROR(0, "FAILED AltsoundProcessorBase::setStreamVolume()");
			}
		}
		else {
			ALT_ERROR(0, "FAILED AltsoundProcessorBase::createStream()");
		}
	}
	else {
		ALT_ERROR(0, "FAILED AltsoundProcessor::stopMusicStream()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::process_music()");
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_jingle(AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::process_jingle()");
	INDENT;

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
				ALT_WARNING(0, "FAILED AltsoundProcessor::stopMusicStream()");
			}
		}
		else if (stream_out->ducking < 0.0f) {
			// Pause current music stream
			if (BASS_ChannelPause(cur_mus_stream->hstream)) {
				ALT_WARNING(0, "FAILED BASS_ChannelPause(): %s", get_bass_err());
			}
		}
	}

	// handle jingle stream
	if (cur_jin_stream) {
		// stop current jingle stream
		success = stopJingleStream();
		if (!success) {
			ALT_ERROR(0, "FAILED AltsoundProcessor::stopJingleStream()");
		}
	}
	
	if (success) {
		success = createStream(&jingle_callback, stream_out);
		if (success) {
			success = setStreamVolume(stream_out->hstream, stream_out->gain);
			if (!success) {
				ALT_WARNING(0, "FAILED AltsoundProcessorBase::setStreamVolume()");
			}
		}
		else {
			ALT_ERROR(0, "FAILED AltsoundProcessorBase::create_stream(): %s", get_bass_err());
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::process_jingle()");
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::process_sfx(AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::process_sfx()");
	INDENT;

	// create new sfx stream
	bool success = createStream(&sfx_callback, stream_out);
	if (success && !setStreamVolume(stream_out->hstream, stream_out->gain)) {
		success = false;
		ALT_WARNING(0, "FAILED AltsoundProcessorBase::setStreamVolume()");
	}
	else if (!success) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::createStream(): %s", get_bass_err());
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::process_sfx()");
	return success;
}

// ---------------------------------------------------------------------------
// This method should ONLY be called by snd_alt.cpp
// This method exists to provide an external interface to stop music
// streams.  Some derivations of the base class require post-processing
// after calling stopMusicStreams, which cannot be replicated if called
// directly by an external function
// ---------------------------------------------------------------------------

bool AltsoundProcessor::stopMusic()
{
	if (!stopMusicStream()) {
		ALT_ERROR(0, "FAILED AltsoundProcessor::stopMusicStream()");
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
bool AltsoundProcessor::stopMusicStream()
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::stopMusicStream()");
	INDENT;

	bool success = true;

	if (cur_mus_stream) {
		HSTREAM hstream = cur_mus_stream->hstream;
		const unsigned int ch_idx = cur_mus_stream->channel_idx;
		ALT_INFO(0, "Current MUSIC stream(%s): HSTREAM: %u  CH: %02d",
			  getShortPath(cur_mus_stream->sample_path).c_str(), hstream,
			  cur_mus_stream->channel_idx);

		if (stopStream(hstream)) {
			ALT_INFO(0, "Stopped MUSIC stream: %u  Chan: %02d", hstream, ch_idx);
			delete channel_stream[ch_idx];
			channel_stream[ch_idx] = nullptr;
			cur_mus_stream = nullptr;
		}
		else {
			success = false;
			ALT_ERROR(0, "FAILED AltsoundProcessorBase::stopStream(%u)", hstream);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::stopMusicStream()");
	return success;
}

// ---------------------------------------------------------------------------

bool AltsoundProcessor::stopJingleStream()
{
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::stopJingleStream()");
	INDENT;

	bool success = false;

	if (cur_jin_stream) {
		HSTREAM hstream = cur_jin_stream->hstream;
		const unsigned int ch_idx = cur_jin_stream->channel_idx;

		if (stopStream(hstream)) {
			ALT_INFO(0, "Stopped JINGLE stream: %u  Chan: %02d", hstream, ch_idx);
			delete channel_stream[ch_idx];
			channel_stream[ch_idx] = nullptr;
			cur_jin_stream = nullptr;
			success = true;
		}
		else {
			ALT_ERROR(0, "FAILED AltsoundProcessorBase::stopStream(%u)", hstream);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::stopJingleStream()");
	return success;
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::jingle_callback(HSYNC handle, DWORD channel,\
	                                             DWORD data, void* user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::jingle_callback()");
	INDENT;

	ALT_INFO(0, "HSYNC: %u  HSTREAM: %u", handle, channel);
	ALT_DEBUG(0, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	const AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (!cur_jin_stream) {
		ALT_WARNING(0, "Jingle callback hit, but no jingle stream set");
	}
	else if (stream_inst->hstream != hstream_in) {
		ALT_WARNING(0, "Callback HSTREAM != instance HSTREAM");
	}
	else if (stream_inst->stream_type != JINGLE) {
		ALT_WARNING(0, "Instance HSTREAM is NOT a JINGLE stream");
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	const unsigned int inst_ch_idx = stream_inst->channel_idx;

	ALT_INFO(0, "JINGLE stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);

	// free stream resources
	if (!freeStream(inst_hstream)) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::free_stream(%u): %s", inst_hstream, get_bass_err());
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;
	cur_jin_stream = nullptr;

	if (cur_mus_stream) {
		HSTREAM mus_hstream = cur_mus_stream->hstream;
		ALT_INFO(0, "Adjusting MUSIC volume");

		// re-calculate music ducking based on active channels.
		const float min_ducking = getMinDucking();
		ALT_INFO(0, "Min ducking value: %.02f", min_ducking);

		// set new music volume
		const float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(mus_hstream, adj_mus_vol);

		// DAR@20230622
		// This is a kludgy way to make sure we only resume paused playback
		// when the stream that paused it ends
		if (stream_inst->ducking < 0.0f && BASS_ChannelIsActive(mus_hstream) == BASS_ACTIVE_PAUSED) {
			ALT_INFO(0, "Resuming MUSIC playback");

			if (!BASS_ChannelPlay(mus_hstream, 0)) {
				ALT_ERROR(0, "FAILED BASS_ChannelPlay(%u): %s", mus_hstream, get_bass_err());
			}
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::jingle_callback()");
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::sfx_callback(HSYNC handle, DWORD channel,\
	                                          DWORD data, void *user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	ALT_DEBUG(0, "BEGIN: AltsoundProcessor::sfx_callback()");
	INDENT;

	ALT_INFO(0, "HSYNC: %u  HSTREAM: %u", handle, channel);
	ALT_DEBUG(0, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	const AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (stream_inst->hstream != hstream_in) {
		ALT_WARNING(0, "Callback HSTREAM != instance HSTREAM");
	}
	else if (stream_inst->stream_type != SFX) {
		ALT_WARNING(0, "instance HSTREAM is NOT a SFX stream");
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	const unsigned int inst_ch_idx = stream_inst->channel_idx;

	ALT_INFO(0, "SFX stream(%u) finished on CH(%02d)", inst_hstream, inst_ch_idx);

	// free stream resources
	if (!freeStream(inst_hstream)) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::free_stream(%u): %s", inst_hstream, get_bass_err());
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;

	if (cur_mus_stream) {
		HSTREAM mus_hstream = cur_mus_stream->hstream;
		ALT_INFO(0, "Adjusting MUSIC volume");

		// re-calculate music ducking based on active channels.
		const float min_ducking = getMinDucking();
		ALT_INFO(0, "Min ducking value: %.02f", min_ducking);

		// set new music volume
		const float adj_mus_vol = cur_mus_stream->gain * min_ducking;
		setStreamVolume(mus_hstream, adj_mus_vol);
	}

	OUTDENT;
	ALT_DEBUG(0, "END: AltsoundProcessor::sfx_callback()");
}

// ---------------------------------------------------------------------------

void CALLBACK AltsoundProcessor::music_callback(HSYNC handle, DWORD channel,\
	                                            DWORD data, void *user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	ALT_DEBUG(0, "BEGIN AltsoundProcessor::music_callback()");
	INDENT;

	ALT_INFO(0, "HSYNC: %u  HSTREAM: %u", handle, channel);
	ALT_DEBUG(0, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	const AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (!cur_jin_stream) {
		ALT_WARNING(0, "MUSIC callback hit, but no MUSIC stream set");
	}
	else if (stream_inst->hstream != hstream_in) {
		ALT_WARNING(0, "callback HSTREAM != instance HSTREAM");
	}
	else if (stream_inst->stream_type != MUSIC) {
		ALT_WARNING(0, "instance HSTREAM is NOT a MUSIC stream");
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	const unsigned int inst_ch_idx = stream_inst->channel_idx;

	ALT_INFO(0, "MUSIC stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);

	// free stream resources
	if (!freeStream(inst_hstream)) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::free_stream(%u): %s", inst_hstream, get_bass_err());
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;
	cur_mus_stream = nullptr;

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundProcessor::music_callback()");
}

// ---------------------------------------------------------------------------

float AltsoundProcessor::getMinDucking()
{
	ALT_DEBUG(0, "BEGIN: AltsoundProcessor::getMinDucking()");
	INDENT;

	float min_ducking = 1.0f;
	int num_x_streams = 0;

	for (size_t index = 0; index < channel_stream.size(); ++index) {
		const auto stream = channel_stream[index];
		if (stream) {
			// stream defined on the channel
			ALT_INFO(1, "Channel_stream[%u]: STREAM: %u  DUCKING: %0.02f", index, stream->hstream, stream->ducking);

			num_x_streams++;

			if (stream->ducking < 0) {
				// Special case. Jingle ducking < 0 pauses music stream in
				// traditional altsound packages. We don't want it to
				// influence actual ducking, since it would always be the
				// lowest value of all other samples.
				ALT_INFO(1, "Ducking value < 0. Skipping...");
				continue;
			}

			min_ducking = std::min(min_ducking, stream->ducking);
		}
	}
	ALT_INFO(0, "Num active streams: %d", num_x_streams);
	ALT_INFO(0, "Min ducking of all active channels: %.2f", min_ducking);

	OUTDENT;
	ALT_DEBUG(0, "END: AltsoundProcessor::getMinDucking()");
	return min_ducking;
}
