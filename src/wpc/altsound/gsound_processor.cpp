// ---------------------------------------------------------------------------
// gsound_processor.cpp
//
// Encapsulates all specialized processing for the G-Sound
// CSV format
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#define NOMINMAX
#include "gsound_processor.hpp"

// Standard Library includes
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>

// Local includes
#include "gsound_csv_parser.hpp"

using std::string;
using std::vector;

// reference to global AltSound logger
extern AltsoundLogger alog;

// DAR@20230721
// This is necessary because BASS callback streams run on a separate
// thread from the main application, and both need to access/modify
// common memory
//
// thread synchonization mutex. 
extern std::mutex io_mutex;

// Reference to the global channel stream array
extern StreamArray channel_stream;

// ----------------------------------------------------------------------------
// Behavior Management Support Globals
// ----------------------------------------------------------------------------

constexpr unsigned int UNSET_IDX = std::numeric_limits<unsigned int>::max();

// NOTE:
// SFX streams don't require tracking since multiple can play simultaneously.
// Other than adjusting ducking, they have no other impacts on active streams
//
// Single-play stream tracking
static unsigned int cur_mus_stream_idx = UNSET_IDX;
static unsigned int cur_callout_stream_idx = UNSET_IDX;
static unsigned int cur_solo_stream_idx = UNSET_IDX;
static unsigned int cur_overlay_stream_idx = UNSET_IDX;

std::map<AltsoundSampleType, unsigned int*> tracked_stream_idx_map = {
	{AltsoundSampleType::MUSIC, &cur_mus_stream_idx},
	{AltsoundSampleType::CALLOUT, &cur_callout_stream_idx},
	{AltsoundSampleType::SOLO, &cur_solo_stream_idx},
	{AltsoundSampleType::OVERLAY, &cur_overlay_stream_idx}
};

// DAR@20230628
// Because the stream.stream_type enumeration values may not match the bitset
// values below, we need to map the AltsoundSampleType constants to the
// bitset value that matches.  This map allows for the retrieval of the
// correct index into the bookkeeping arrays to manage aggregate ducking
// and pausing behaviors.
//
// Map stream sample types to configuration bitset values
static std::unordered_map<AltsoundSampleType, int> streamTypeToIndex = {
	{AltsoundSampleType::MUSIC,   0},
	{AltsoundSampleType::CALLOUT, 1},
	{AltsoundSampleType::SFX,     2},
	{AltsoundSampleType::SOLO,    3},
	{AltsoundSampleType::OVERLAY, 4}
};

// DAR@20230719
// The maps below contain the ducking and pausing behavior impacts of sample
// types on other sample types.  For example, the music_duck_vol map contains
// the impacts on MUSIC volume from the behaviors of the other sample types.
// Similarly, the music_paused map contains the paused status of MUSIC
// streams based on the behavior of the other streams.
//
// The map key is the stream ID (HSTREAM) of the stream that is setting the
// duck value.  This allows the correct entry to be removed when the 
// affecting stream ends
//
// Streams can have overlapping impacts on other streams.  When an affecting
// stream ends, it can't be assumed its safe to remove the behavior impact from
// the affected sample type.  The map key ensures ALL the behavior impacts are
// captured for each sample type.  Only when the map is cleared, can the impact
// be removed
//
// globals to manage stream ducking
std::unordered_map<unsigned int, float> music_duck_vol;
std::unordered_map<unsigned int, float> callout_duck_vol;
std::unordered_map<unsigned int, float> sfx_duck_vol;
std::unordered_map<unsigned int, float> solo_duck_vol;
std::unordered_map<unsigned int, float> overlay_duck_vol;

// convenience structure for working with ducking maps
std::unordered_map<AltsoundSampleType, std::unordered_map<unsigned int, float>*> duck_vol_map = {
	{MUSIC, &music_duck_vol},
	{CALLOUT, &callout_duck_vol},
	{SFX, &sfx_duck_vol},
	{SOLO, &solo_duck_vol},
	{OVERLAY, &overlay_duck_vol}
};

// globals to manage stream pausing
std::unordered_map<unsigned int, bool> music_paused;
std::unordered_map<unsigned int, bool> callout_paused;
std::unordered_map<unsigned int, bool> sfx_paused;
std::unordered_map<unsigned int, bool> solo_paused;
std::unordered_map<unsigned int, bool> overlay_paused;

// convenience structure for working with paused maps
std::unordered_map<AltsoundSampleType, std::unordered_map<unsigned int, bool>*> paused_status_map = {
	{MUSIC, &music_paused},
	{CALLOUT, &callout_paused},
	{SFX, &sfx_paused},
	{SOLO, &solo_paused},
	{OVERLAY, &overlay_paused}
};


// DAR@20230712
// A common mixing board function is to set gain levels for individual tracks
// and then apply a group volume to change volume for a group of tracks. while
// maintaining relative individual volumes.  For example, if we have 3 MUSIC
// tracks with 90, 60, 55 gain levels individually, setting the group volume
// to 90 will duck all MUSIC tracks by 10% while maintaining the volume
// relationships between the individual tracks.  Each index into the array
// represents the sample type they apply to.  For example, index 0 is MUSIC
// group volume.  Index 1 is CALLOUT These values can be set in
// the configuration file
//
// Group volumes for sample types
std::array<float, NUM_STREAM_TYPES> group_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// ----------------------------------------------------------------------------

// DAR@20230719
// Sample type behaviors determine what impacts they will have on other sample
// types.  For example, music_behavior defines what impacts MUSIC samples have
// on other non-MUSIC sample types.
//
// NOTE: With the exception of STOP, self-impacting behaviors are not supported.
//
// external reference to stream behaviors
extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

// convenience structure for working with behaviors
std::unordered_map<AltsoundSampleType, BehaviorInfo*> behavior_map = {
	{MUSIC, &music_behavior},
	{CALLOUT, &callout_behavior},
	{SFX, &sfx_behavior},
	{SOLO, &solo_behavior},
	{OVERLAY, &overlay_behavior}
};

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

GSoundProcessor::GSoundProcessor(const std::string& _game_name, const std::string& _vpm_path)
: AltsoundProcessorBase(_game_name, _vpm_path),
  is_initialized(false),
  is_stable(true), // future use
  generator(std::random_device()()) // seed random number generator
{
}

GSoundProcessor::~GSoundProcessor()
{
	// clean up stream tracking
	cur_mus_stream_idx = UNSET_IDX;
	cur_callout_stream_idx = UNSET_IDX;
	cur_solo_stream_idx = UNSET_IDX;
	cur_overlay_stream_idx = UNSET_IDX;
	
	stopAllStreams();
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool GSoundProcessor::handleCmd(const unsigned int cmd_combined_in)
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::handleCmd()");
	INDENT;

	ALT_DEBUG(1, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	// Pass command to base class for processing
	AltsoundProcessorBase::handleCmd(cmd_combined_in);

	if (!is_initialized || !is_stable) {
		if (!is_initialized) {
			ALT_ERROR(1, "GSoundProcessor is not initialized. Processing skipped");
		}
		else {
			ALT_ERROR(1, "GSoundProcessor is unstable. Processing skipped");
		}
		
		OUTDENT;
		ALT_DEBUG(0, "END: GSoundProcessor::handleCmd()");
		return false;
	}

	// DAR@20231018
	// Sometimes ROMs send sound commands at startup that result in unwanted
	// sound playback.  These commands may be valid during normal runtime, so
	// we don't want to simply ignore them.  Instead, authors can choose to
	// skip a number of initial commands.  This can be set in the .ini file
	// 
	// Skip this command?
	unsigned int skip_count = AltsoundProcessorBase::getSkipCount();
	if (skip_count > 0) {
		--skip_count;
		AltsoundProcessorBase::setSkipCount(skip_count);
		ALT_DEBUG(0, "Sound command skipped, (%d) remaining", skip_count);
		ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
		return true;
	}

	// get sample for playback
	const int sample_idx = ALT_CALL(getSample(cmd_combined_in));

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		ALT_ERROR(1, "FAILED GSoundProcessor::get_sample()", cmd_combined_in);

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
		return false;
	}

	AltsoundStreamInfo* new_stream = new AltsoundStreamInfo();

	// pre-populate stream info
	new_stream->sample_path = samples[sample_idx].fname;
	new_stream->gain = samples[sample_idx].gain;
	new_stream->loop = samples[sample_idx].loop;
	new_stream->ducking_profile = samples[sample_idx].ducking_profile;

	const AltsoundSampleType sample_type = toSampleType(samples[sample_idx].type);

	switch (sample_type) {
	case MUSIC:
		new_stream->stream_type = MUSIC;
		if (ALT_CALL(processStream(music_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
			cur_mus_stream_idx = new_stream->channel_idx;
		}
		else {
			ALT_ERROR(1, "FAILED GSoundProcessor::processMusic()");
			
			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		break;
	case SFX:
		new_stream->stream_type = SFX;
		if (ALT_CALL(processStream(sfx_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
		}
		else {
			ALT_ERROR(1, "FAILED GSoundProcessor::processSfx()");
			
			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		break;
	case CALLOUT:
		new_stream->stream_type = CALLOUT;
		if (ALT_CALL(processStream(callout_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
			cur_callout_stream_idx = new_stream->channel_idx;
		}
		else {
			ALT_ERROR(1, "FAILED GSoundProcessor::processCallout()");
			
			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		break;
	case SOLO:
		new_stream->stream_type = SOLO;
		if (ALT_CALL(processStream(solo_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
			cur_solo_stream_idx = new_stream->channel_idx;
		}
		else {
			ALT_ERROR(1,"FAILED GSoundProcessor::processSolo()");

			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		break;
	case OVERLAY:
		new_stream->stream_type = OVERLAY;
		if (ALT_CALL(processStream(overlay_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
			cur_overlay_stream_idx = new_stream->channel_idx;
		}
		else {
			ALT_ERROR(1, "FAILED GSoundProcessor::processOverlay()");
			
			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		break;
	}

	// set volume for active streams
	ALT_CALL(adjustStreamVolumes());

	// Play pending sound determined above, if any
	const std::string shortPathStr = getShortPath(new_stream->sample_path);
	const char* sample_short_path = shortPathStr.c_str();
	const char* stream_type_str = toString(new_stream->stream_type);

	if (new_stream->hstream != BASS_NO_STREAM) {
		ALT_INFO(1, "Playing %s stream: %s", stream_type_str, sample_short_path);
		ALT_DEBUG(1, "HSTREAM(%u)  CH(%02d)  CMD(%04X)  SAMPLE(%s)", new_stream->hstream,
			      new_stream->channel_idx, cmd_combined_in, sample_short_path);

		if (!BASS_ChannelPlay(new_stream->hstream, 0)) {
			// Sound playback failed
			ALT_ERROR(2, "FAILED %s stream playback: %s", stream_type_str, get_bass_err());

			OUTDENT;
			ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
			return false;
		}
		else {
			ALT_INFO(2, "SUCCESS %s stream playback", stream_type_str);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::handleCmd()");
	return true;
}

// ---------------------------------------------------------------------------

void GSoundProcessor::init()
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::init()");
	INDENT;

	// reset stream tracking variables
	cur_mus_stream_idx = UNSET_IDX;
	cur_callout_stream_idx = UNSET_IDX;
	cur_solo_stream_idx = UNSET_IDX;
	cur_overlay_stream_idx = UNSET_IDX;
	
	if (!loadSamples()) {
		ALT_ERROR(1, "FAILED GSoundProcessor::loadSamples()");
	}
	ALT_INFO(1, "SUCCESS: GSoundProcessor::loadSamples()");

	// populate group volumes
	group_vol[streamTypeToIndex[MUSIC]]   = music_behavior.group_vol;
	group_vol[streamTypeToIndex[CALLOUT]] = callout_behavior.group_vol;
	group_vol[streamTypeToIndex[SFX]]     = sfx_behavior.group_vol;
	group_vol[streamTypeToIndex[OVERLAY]] = overlay_behavior.group_vol;
	group_vol[streamTypeToIndex[SOLO]]    = solo_behavior.group_vol;

	// if we are here, initialization succeeded
	is_initialized = true;

	// Initialize base class
	AltsoundProcessorBase::init();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::init()");
}

// ---------------------------------------------------------------------------

bool GSoundProcessor::loadSamples()
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::loadSamples()");
	INDENT;

	// DAR_TODO make vpm_path and game_name private w/accessors?
	string altsound_path = vpm_path; // in base class
	if (!altsound_path.empty()) {
		altsound_path += "/altsound/";
		altsound_path += game_name;
	}

	GSoundCsvParser csv_parser(altsound_path);

	if (!csv_parser.parse(samples)) {
		ALT_ERROR(1, "FAILED GSoundCsvParser::parse()");

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::init()");
		return false;
	}
	ALT_INFO(1, "SUCCESS GSoundCsvParser::parse()");

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::init()");
	return true;
}

// ---------------------------------------------------------------------------

unsigned int GSoundProcessor::getSample(const unsigned int cmd_combined_in)
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::getSample()");
	INDENT;

	int matching_sample_count = 0;
	unsigned int sample_idx = UNSET_IDX;

	std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());

	for (size_t i = 0; i < samples.size(); ++i) {
		if (samples[i].id == cmd_combined_in) {
			matching_sample_count++;

			// reservoir sampling approach
			// Each matching sample has equal chance (1/matching_sample_count) to
			// become the selected one.
			if (distribution(generator) % matching_sample_count == 0) {
				sample_idx = static_cast<unsigned int>(i);
			}
		}
	}

	if (matching_sample_count == 0) {
		ALT_INFO(0, "No sample(s) found for ID: %04X", cmd_combined_in);
	}
	else {
		ALT_INFO(0, "Found %d sample(s) for ID: %04X", matching_sample_count, cmd_combined_in);
		if (sample_idx != UNSET_IDX) {
			ALT_INFO(0, "Sample: %s", getShortPath(samples[sample_idx].fname).c_str());
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::getSample()");
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool GSoundProcessor::processStream(const BehaviorInfo& behavior,
	                                AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::processStream()");
	INDENT;

	// create new stream
	bool success = ALT_CALL(createStream(&common_callback, stream_out));

	if (!success) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::createStream()");

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::processStream()");
		return success;
	}

	// process behaviors for this sample type
	success = processBehaviors(behavior, stream_out);

	if (!success) {
		ALT_ERROR(0, "FAILED: GSoundProcessor::processBehaviors()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::processStream()");
	return success;
}

// ----------------------------------------------------------------------------
// This is the meat of the G-Sound system.  Each sample type defines behaviors
// that affect streams.  Rather than inferring them from combinations of CSV
// parameters, they are explicitly spelled out. Altsound authors can tweak
// these behaviors to get the overall mixes they want.  G-Sound represents a new
// option for authors.
//
// How to think of behavior processing:
// Every time a new sample is processed, its associated behavior definition
// and the new stream info is sent here.  Each sample type is checked against
// the new sample's behavior definition to determine its impact on the
// other stream types, and the bookkeeping is updated to reflect those
// impacts.
// ----------------------------------------------------------------------------

bool GSoundProcessor::processBehaviors(const BehaviorInfo& behavior, const AltsoundStreamInfo* stream)
{
	ALT_DEBUG(0, "BEGIN: GSoundProcessor::processBehaviors()");
	INDENT;

	// Syntactic sugar for accessing bitset values
	using BB = BehaviorInfo::BehaviorBits;

	// Iterate through all sample types
	for (const auto& iter : streamTypeToIndex)
	{
		AltsoundSampleType sampleType = iter.first;
		int behaviorBitIndex = iter.second;

		// Retrieve the behavior bit for the current sample type
		BB sampleBehaviorBit = static_cast<BB>(behaviorBitIndex);
		ALT_DEBUG(1, "Processing %s behaviors on %s streams", toString(stream->stream_type),
			      toString(sampleType));

		// Process STOP behavior impact
		if (behavior.stops.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			// current stream behavior calls to stop current stream
			if (!stopExclusiveStream(sampleType)) {
				ALT_ERROR(1, "FAILED GSoundProcessor::stopExclusicStream(%s)", toString(sampleType));

				OUTDENT;
				ALT_DEBUG(0, "END GSoundProcessor::processBehaviors()");
				return false;
			}
		}
		else if (behavior.pauses.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			// Add pause impact from the current stream on the current sample type
			if (sampleType != stream->stream_type) {
				// get tracked stream index
				const unsigned int* cur_stream_idx = tracked_stream_idx_map[sampleType];

				if (*cur_stream_idx != UNSET_IDX) {
					HSTREAM hstream = channel_stream[*cur_stream_idx]->hstream;
					if (!BASS_ChannelPause(hstream)) {
						ALT_ERROR(1, "FAILED BASS_ChannelPause(): %s", get_bass_err());

						OUTDENT;
						ALT_DEBUG(0, "END GSoundProcessor::processMusicImpacts()");
						return false;
					}
					else {
						ALT_INFO(1, "SUCCESS BASS_ChannelPause(%u)", hstream);
					}
				}
				
				// DAR@20230723
				// Whether or not there was actually a stream playing, record the fact
				// that the current behavior wanted it paused.  This way, if a stream
				// starts that should be paused, it will be, as long as the stream that
				// set the behavior is still playing
				auto& map = *paused_status_map[sampleType];
				map[stream->hstream] = true;
			}
			else
			{
				ALT_WARNING(1, "%s streams cannot pause another %s stream", toString(sampleType), toString(sampleType));
			}
		}

		// Process DUCKING behavior impact
		if (behavior.ducks.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			// Add ducking impact from the current stream on the current sample type
			if (sampleType != stream->stream_type)	{
				// Get ducking volume for this sample to apply to the current sample type
				if (stream->ducking_profile != 0) {
					const float duck_vol = behavior.getDuckVolume(stream->ducking_profile, sampleType);

					auto& map = *duck_vol_map[sampleType];
					map[stream->hstream] = duck_vol;
				}
			}
			else
			{
				ALT_WARNING(1, "%s streams cannot duck another %s stream", toString(sampleType), toString(sampleType));
			}
		}
	}
	
	// DEBUG helper
	printBehaviorData();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::processBehaviors()");
	return true;
}

// ----------------------------------------------------------------------------
// This function is called after a stream ends or is stopped, to process the
// behavior impacts of the stream ending on other streams
// ----------------------------------------------------------------------------

bool GSoundProcessor::postProcessBehaviors(const BehaviorInfo& behavior,
	                                       const AltsoundStreamInfo& finished_stream)
{
	ALT_DEBUG(0, "BEGIN: GSoundProcessor::postProcessBehaviors()");
	INDENT;

	// Syntactic sugar for accessing bitset values
	using BB = BehaviorInfo::BehaviorBits;

	// Iterate through all sample types
	for (const auto& iter : streamTypeToIndex)
	{
		AltsoundSampleType sampleType = iter.first;
		int behaviorBitIndex = iter.second;
		const char* finished_stream_type_str = toString(finished_stream.stream_type);
		ALT_DEBUG(1, "Post-processing %s behaviors on %s streams", finished_stream_type_str, toString(sampleType));

		// Retrieve the behavior bit for the current sample type
		BB sampleBehaviorBit = static_cast<BB>(behaviorBitIndex);

		// Process STOP behavior impact
		if (behavior.stops.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			// Currently ended stream stopped the current sample type. Nothing to do.
		}

		// Process PAUSE behavior impact
		if (behavior.pauses.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			auto& map = *paused_status_map[sampleType];

			if (!map.empty()) {
				unsigned int stream_id = finished_stream.hstream;
				auto it = map.find(stream_id);
				if (it != map.end()) {
					ALT_DEBUG(1, "Erasing pausing impact from %s stream: %u", toString(finished_stream.stream_type), finished_stream.hstream);
					map.erase(it);
				}
			}
		}

		// Process DUCKING behavior impact
		ALT_DEBUG(1, "Post-processing %s ducking impact on %s streams", toString(finished_stream.stream_type), toString(sampleType));
		if (behavior.ducks.test(static_cast<size_t>(sampleBehaviorBit)))
		{
			auto& map = *duck_vol_map[sampleType];

			if (!map.empty()) {
				unsigned int stream_id = finished_stream.hstream;
				auto it = map.find(stream_id);
				if (it != map.end()) {
					ALT_DEBUG(1, "Erasing ducking impact from %s stream: %u", toString(finished_stream.stream_type), finished_stream.hstream);
					map.erase(it);
				}
			}
		}
	}

	// DEBUG helper
	printBehaviorData();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::postProcessBehaviors()");
	return true;
}

// ----------------------------------------------------------------------------
// This method is only meant to be called by snd_alt.cpp  It is used to safely
// call stopExclusiveStream(MUSIC), while maintaining the behavior bookkeeping
// ----------------------------------------------------------------------------

bool GSoundProcessor::stopMusic()
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::stopMusic()");
	INDENT;

	//DAR_TODO add error checking
	if (!stopExclusiveStream(MUSIC)) {
		ALT_ERROR(1, "FAILED GSoundProcessor::stopMusicStream()");

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::stopMusic()");
		return false;
	}

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::stopMusic()");
	return true;
}

// ----------------------------------------------------------------------------
// This method is used to stop one of the exclusive (one-at-a-time) sample tyoe
// streams.  The argument to this function must be the address of one of the
// global tracking variables for exclusive streams: cur_mus_stream,
// cur_callout_stream, 	cur_solo_stream, and cur_callout_stream.  Because of the
// complex bookkeeping involved with managing behaviors, this method should only
// be called as part of behavior processing.  Otherwise, it will corrupt the
// bookkeeping and cause problems
// ----------------------------------------------------------------------------

bool GSoundProcessor::stopExclusiveStream(const AltsoundSampleType stream_type)
{
	ALT_DEBUG(0, "BEGIN: GSoundProcessor::stopExclusiveStream()");
	INDENT;

	// Get the current stream index for the sample type
	unsigned int* cur_stream_idx = tracked_stream_idx_map[stream_type];

	if (*cur_stream_idx == UNSET_IDX) {
		ALT_INFO(1, "No active %s stream", toString(stream_type));

		OUTDENT;
		ALT_DEBUG(0, "END: GSoundProcessor::stopExclusiveStream()");
		return true;
	}

	AltsoundStreamInfo* cur_stream = channel_stream[*cur_stream_idx];

	// DAR@20230724
	// We have to first post-process the sample behavior to remove any
	// impacts the stream we are about to stop may have had
	postProcessBehaviors(*behavior_map[stream_type], *cur_stream);

	HSTREAM hstream = cur_stream->hstream;
	const unsigned int ch_idx = cur_stream->channel_idx;

	ALT_INFO(1, "Current stream(%s): HSTREAM: %u  CH: %02d",
		getShortPath(getShortPath(cur_stream->sample_path)).c_str(), hstream, ch_idx);

	const bool success = stopStream(hstream);
	if (success) {
		ALT_INFO(1, "Stopped %s stream: %u  Chan: %02d", toString(stream_type), hstream, ch_idx);
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		*cur_stream_idx = UNSET_IDX;
	}
	else {
		ALT_ERROR(1, "FAILED stopStream(%u)", hstream);
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END: GSoundProcessor::stopExclusiveStream()");
	return true;
}

// ----------------------------------------------------------------------------

void CALLBACK GSoundProcessor::common_callback(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	ALT_DEBUG(0, "\nBEGIN: GSoundProcessor::common_callback()");
	INDENT;

	ALT_INFO(1, "HSYNC: %u  HSTREAM: %u", handle, channel);
	ALT_DEBUG(1, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	const AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);
	HSTREAM inst_hstream = stream_inst->hstream;

	if (inst_hstream != hstream_in) {
		ALT_ERROR(1, "Callback HSTREAM != instance HSTREAM");

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::common_callback()");
		return;
	}

	const unsigned int inst_ch_idx = stream_inst->channel_idx;
	const AltsoundSampleType stream_type = stream_inst->stream_type;

	BehaviorInfo behavior;
	switch (stream_type) {
	case SOLO:
		behavior = solo_behavior;
		cur_solo_stream_idx = UNSET_IDX;
		break;

	case MUSIC:
		behavior = music_behavior;
		// DAR@20230706
		// This callback gets hit when the sample ends even if it's set to loop.
		// If it's cleaned up here, it will not loop. This is not desirable.  A future
		// use may be to create an independent music callback that can limit the
		// number of loops.
		//cur_mus_stream_idx = UNSET_IDX;
		break;

	case SFX:
		behavior = sfx_behavior;
		break;

	case CALLOUT:
		behavior = callout_behavior;
		cur_callout_stream_idx = UNSET_IDX;
		break;

	case OVERLAY:
		behavior = overlay_behavior;
		cur_overlay_stream_idx = UNSET_IDX;
		break;

	default:
		ALT_ERROR(1, "Unknown stream type");

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::common_callback()");
		return;
	}

	// update ducking behavior tracking
	postProcessBehaviors(behavior, *stream_inst);

	if (stream_type != MUSIC) {
		// free stream resources
		if (!ALT_CALL(freeStream(inst_hstream))) {
			ALT_ERROR(1, "FAILED AltsoundProcessorBase::free_stream(%u): %s", inst_hstream, get_bass_err());
		}

		delete channel_stream[inst_ch_idx];
		channel_stream[inst_ch_idx] = nullptr;
	}

	// re-adjust stream volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	ALT_INFO(1, "%s stream(%u) finished on ch(%02d)", toString(stream_type), inst_hstream, inst_ch_idx);

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::common_callback()");
}

// ----------------------------------------------------------------------------

bool GSoundProcessor::adjustStreamVolumes()
{
	ALT_INFO(0, "BEGIN GSoundProcessor::adjustStreamVolumes()");
	INDENT;

	bool success = true;
	int num_x_streams = 0;

	for (const auto& streamPtr : channel_stream) {
		if (!streamPtr) continue; // Stream is not defined

		const auto& stream = *streamPtr; // Dereference pointer for readability
		float ducking_value = 1.0f;

		const float grp_vol = behavior_map[stream.stream_type]->group_vol;

		const AltsoundSampleType stream_type = stream.stream_type;

		switch (stream_type) {
		case MUSIC:
			ducking_value = findLowestDuckVolume(MUSIC);
			break;
		case CALLOUT:
			ducking_value = findLowestDuckVolume(CALLOUT);
			break;
		case SFX:
			ducking_value = findLowestDuckVolume(SFX);
			break;
		case SOLO:
			ducking_value = findLowestDuckVolume(SOLO);
			break;
		case OVERLAY:
			ducking_value = findLowestDuckVolume(OVERLAY);
			break;
		default:
			ALT_WARNING(1, "Sample type not found. Using default");
		}

		ALT_DEBUG(1, "%s ducking volume: %.02f", toString(stream_type), ducking_value);

		const float adjusted_vol = stream.gain * ducking_value * grp_vol;
		if (!setStreamVolume(stream.hstream, adjusted_vol)) {
			ALT_ERROR(1, "FAILED setStreamVolume()");
			success = false;
		}
		ALT_DEBUG(1, "%s stream %u gain:  %.02f  group_vol:  %.02f ducked_vol:  %.02f", toString(stream.stream_type),
			stream.hstream, stream.gain, grp_vol, ducking_value);

		num_x_streams++;
	}

	ALT_INFO(1, "Num active streams: %d", num_x_streams);

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::adjustStreamVolumes()");
	return success;
}


// ----------------------------------------------------------------------------

bool GSoundProcessor::processPausedStreams()
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::processPausedStreams()");
	INDENT;

	bool success = true;
	for (const auto* stream : channel_stream) {
		if (stream) {
			success &= tryResumeStream(*stream);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::processPausedStreams()");
	return success;
}

// ----------------------------------------------------------------------------

bool GSoundProcessor::tryResumeStream(const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::tryResumeStream()");
	INDENT;

	// Find the pause map for the stream type
	auto pauseStatusMapIter = paused_status_map.find(stream.stream_type);

	// If the stream type is not found, log an error and return
	if (pauseStatusMapIter == paused_status_map.end()) {
		ALT_ERROR(1, "Unknown stream type");
		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::tryResumeStream()");
		return false;
	}

	// Get a reference to the relevant pause map
	const auto& pauseMap = *pauseStatusMapIter->second;

	// Search the pause map for any value that is true (indicating a pause)
	const bool shouldRemainPaused = std::any_of(pauseMap.begin(), pauseMap.end(),
		[](const auto& valuePair) { return valuePair.second; });

	if (!shouldRemainPaused) {
		HSTREAM hstream = stream.hstream;

		if (BASS_ChannelIsActive(hstream) == BASS_ACTIVE_PAUSED) {
			if (!BASS_ChannelPlay(hstream, 0)) {
				ALT_ERROR(1, "FAILED BASS_ChannelPlay(%u): %s", hstream, get_bass_err());
				OUTDENT;
				ALT_DEBUG(0, "END GSoundProcessor::tryResumeStream()");
				return false;
			}
			ALT_INFO(1, "Successfully resumed stream: %u", hstream);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::tryResumeStream()");
	return true;
}

// ----------------------------------------------------------------------------
// Helper function to find the lowest volume in the ducking map associated
// with the passed-in stream type
// ----------------------------------------------------------------------------

float GSoundProcessor::findLowestDuckVolume(AltsoundSampleType stream_type)
{
	ALT_DEBUG(0, "BEGIN GSoundProcessor::findLowestDuckVolume()");
	INDENT;

	const auto map = duck_vol_map[stream_type];

	if (map->empty()) {
		// If there are no entries in the map, return the default volume.
		ALT_DEBUG(1, "%s duck_vol_map is empty()", toString(stream_type));

		OUTDENT;
		ALT_DEBUG(0, "END GSoundProcessor::findLowestDuckVolume()");
		return 1.0f;
	}

	// Search for lowest volume in the map, defaulting to 1.0f if empty
	const float min_vol = std::accumulate(map->begin(), map->end(), 1.0f,
		[](float currentMin, const auto& pair) { return std::min(currentMin, pair.second); });

	ALT_DEBUG(1, "Min ducking value for %s streams: %.02f", toString(stream_type), min_vol);


	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::findLowestDuckVolume()");
	return min_vol;
}

// ---------------------------------------------------------------------------
// Helper DEBUG functions to output all behavior bookkeeping data
// ---------------------------------------------------------------------------

void GSoundProcessor::printBehaviorData() {
	ALT_DEBUG(0, "BEGIN GSoundProcessor::printBehaviorData()");
	INDENT;

	ALT_DEBUG(0, "Printing duck_vol_map:");
	for (const auto& duckPair : duck_vol_map) {
		const char* streamTypeStr = toString(duckPair.first);
		ALT_DEBUG(0, "Stream Type: %s", streamTypeStr);
		for (const auto& volPair : *(duckPair.second)) {
			ALT_DEBUG(0, "Stream ID: %lu, Duck Volume: %f", volPair.first, volPair.second);
		}
	}

	ALT_DEBUG(0, "Printing paused_status_map:");
	for (const auto& pausedPair : paused_status_map) {
		const char* streamTypeStr = toString(pausedPair.first);
		ALT_DEBUG(0, "Stream Type: %s", streamTypeStr);
		for (const auto& statusPair : *(pausedPair.second)) {
			ALT_DEBUG(0, "Stream ID: %lu, Pause Status: %s", statusPair.first, statusPair.second ? "true" : "false");
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END GSoundProcessor::printBehaviorData()");
}

// ----------------------------------------------------------------------------
