// ---------------------------------------------------------------------------
// altsound2_processor.cpp
// 06/14/23 - Dave Roscoe
//
// Encapsulates all specialized processing for the Altsound2
// CSV format
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "altsound2_processor.hpp"

// Standard Library includes
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

// Local includes
#include "osdepend.h"
#include "altsound2_csv_parser.hpp"
#include "altsound_logger.hpp"

using std::string;

// NOTE:
// - SFX streams don't require tracking since multiple can play,
//   simultaneously, and other than adjusting ducking, have no other
//   impacts on active streams
//
static AltsoundStreamInfo* cur_mus_stream = nullptr;
static AltsoundStreamInfo* cur_callout_stream = nullptr;
static AltsoundStreamInfo* cur_solo_stream = nullptr;
static AltsoundStreamInfo* cur_overlay_stream = nullptr;

extern AltsoundLogger alog;

// ----------------------------------------------------------------------------
// Behavior Management Support Globals
// ----------------------------------------------------------------------------

// DAR@20230628
// Since multiple stream types can set ducking values for other stream
// types, and multiple stream types can pause other types, the answer to
// when to resume playback or what duck value to use is not straightforward.
// Playback should only resume when ALL other streams that wanted to pause
// it have finished.  To facilitate this, a series of arrays are defined
// to keep track of what stream types have paused another.
// Similarly, we want to duck streams based on the lowest value set by ALL
// streams, and adjust incrementally as each stream completes.  Another
// series of arrays keeps track of the duck value set by each stream.  We
// don't need to worry about stopped streams, since they are not persistent
// once stopped.
//
// Because the stream.stream_type enumeration values may not match the bitset
// values below, we need to map the AltsoundSampleType constants to the
// bitset value that matches.  This map allows for the retrieval of the
// correct index into the bookkeeping arrays to manage aggregate ducking
// and pausing behaviors.
static std::unordered_map<AltsoundSampleType, int> streamTypeToIndex = {
	{AltsoundSampleType::MUSIC,   0},
	{AltsoundSampleType::CALLOUT, 1},
	{AltsoundSampleType::SFX,     2},
	{AltsoundSampleType::SOLO,    3},
	{AltsoundSampleType::OVERLAY, 4}
};

// globals to manage stream ducking (% of specified gain)
std::array<float, NUM_STREAM_TYPES> music_duck_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
std::array<float, NUM_STREAM_TYPES> callout_duck_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
std::array<float, NUM_STREAM_TYPES> sfx_duck_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
std::array<float, NUM_STREAM_TYPES> solo_duck_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
std::array<float, NUM_STREAM_TYPES> overlay_duck_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// global arrays to manage stream pausing
std::array<bool, NUM_STREAM_TYPES> music_paused = { false, false, false, false, false };
std::array<bool, NUM_STREAM_TYPES> callout_paused = { false, false, false, false, false };
std::array<bool, NUM_STREAM_TYPES> sfx_paused = { false, false, false, false, false };
std::array<bool, NUM_STREAM_TYPES> solo_paused = { false, false, false, false, false };
std::array<bool, NUM_STREAM_TYPES> overlay_paused = { false, false, false, false, false };

// DAR@20230712
// A common mixing board function is to set gain levels for individual tracks
// and then apply a group volume to change volume for a group of tracks. while
// maintaining relative individual volumes.  For example, if we have 3 MUSIC
// tracks with 90, 60, 55 volume levels individually, setting the group volume
// to 90 will duck all MUSIC tracks by 10% while maintaining the volume
// relationships between the individual tracks.
//
// Group volumes for sample types
std::array<float, NUM_STREAM_TYPES> group_vol = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// ----------------------------------------------------------------------------

// stream behaviors
extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

// windef.h "min" conflicts with std::min
#ifdef min
  #undef min
#endif

// ---------------------------------------------------------------------------
// CTOR/DTOR
// ---------------------------------------------------------------------------

Altsound2Processor::Altsound2Processor(const char* gname_in)
: AltsoundProcessorBase(string(gname_in)),
  is_initialized(false),
  is_stable(true) // future use
{
	// perform initialization (load samples, etc)
	init();
}

Altsound2Processor::~Altsound2Processor()
{
	// DAR@20230624
	// These cannot be deleted here, since they will be deleted when the
	// channel_stream[] is destroyed.
	//
	// clean up stream tracking
	cur_mus_stream = nullptr;
	cur_callout_stream = nullptr;
	cur_solo_stream = nullptr;
	cur_overlay_stream = nullptr;
	
	stopAllStreams();
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool Altsound2Processor::handleCmd(const unsigned int cmd_combined_in)
{
	ALT_INFO(0, "BEGIN Altsound2Processor::handleCmd()");
	ALT_DEBUG(1, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	if (!is_initialized || !is_stable) {
		if (!is_initialized) {
			ALT_ERROR(1, "Altsound2Processor is not initialized. Processing skipped");
		}
		else {
			ALT_ERROR(1, "Altsound2Processor is unstable. Processing skipped");
		}
		
		OUTDENT;
		ALT_DEBUG(0, "END: Altsound2Processor::handleCmd()");
		return false;
	}

	// get sample for playback
	int sample_idx = ALT_CALL(getSample(cmd_combined_in));

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		ALT_ERROR(1, "FAILED Altsound2Processor::get_sample()", cmd_combined_in);

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
		return false;
	}

	AltsoundStreamInfo* new_stream = new AltsoundStreamInfo();

	// pre-populate stream info
	new_stream->sample_path = samples[sample_idx].fname;
	new_stream->gain = samples[sample_idx].gain;
	new_stream->loop = samples[sample_idx].loop;

	AltsoundSampleType sample_type = toSampleType(samples[sample_idx].type);

	// Instead of multiple if conditions, we can use a switch
	switch (sample_type) {
	case MUSIC:
		new_stream->stream_type = MUSIC;
		if (ALT_CALL(processStream(music_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = cur_mus_stream = new_stream;
		}
		else {
			ALT_ERROR(1, "FAILED Altsound2Processor::processMusic()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			return false;
		}
		break;
	case SFX:
		new_stream->stream_type = SFX;
		if (ALT_CALL(processStream(sfx_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = new_stream;
		}
		else {
			ALT_ERROR(1, "FAILED Altsound2Processor::processSfx()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			return false;
		}
		break;
	case CALLOUT:
		new_stream->stream_type = CALLOUT;
		if (ALT_CALL(processStream(callout_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = cur_callout_stream = new_stream;
		}
		else {
			ALT_ERROR(1, "FAILED Altsound2Processor::processCallout()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			false;
		}
		break;
	case SOLO:
		new_stream->stream_type = SOLO;
		if (ALT_CALL(processStream(solo_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = cur_solo_stream = new_stream;
		}
		else {
			ALT_ERROR(1,"FAILED Altsound2Processor::processSolo()");

			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			return false;
		}
		break;
	case OVERLAY:
		new_stream->stream_type = OVERLAY;
		if (ALT_CALL(processStream(overlay_behavior, new_stream))) {
			channel_stream[new_stream->channel_idx] = cur_overlay_stream = new_stream;
		}
		else {
			ALT_ERROR(1, "FAILED Altsound2Processor::processOverlay()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			return false;
		}
		break;
	}

	// set volume for active streams
	ALT_CALL(adjustStreamVolumes());

	// Play pending sound determined above, if any
	std::string shortPathStr = getShortPath(new_stream->sample_path);
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
			ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
			return false;
		}
		else {
			ALT_INFO(2, "SUCCESS %s stream playback", stream_type_str);
		}
	}


	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::handleCmd()");
	return true;
}

// ---------------------------------------------------------------------------

void Altsound2Processor::init()
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::init()");
	INDENT;

	// reset stream tracking
	cur_mus_stream = nullptr;
	cur_callout_stream = nullptr;
	cur_solo_stream = nullptr;
	cur_callout_stream = nullptr;
	
	if (!loadSamples()) {
		ALT_ERROR(1, "FAILED Altsound2Processor::loadSamples()");
	}
	ALT_INFO(1, "SUCCESS: Altsound2Processor::loadSamples()");

	// populate group volumes
	group_vol[streamTypeToIndex[MUSIC]]   = music_behavior.group_vol;
	group_vol[streamTypeToIndex[CALLOUT]] = callout_behavior.group_vol;
	group_vol[streamTypeToIndex[SFX]]     = sfx_behavior.group_vol;
	group_vol[streamTypeToIndex[OVERLAY]] = overlay_behavior.group_vol;
	group_vol[streamTypeToIndex[SOLO]]    = solo_behavior.group_vol;

	// if we are here, initialization succeeded
	is_initialized = true;

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::init()");
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::loadSamples()
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::loadSamples()");
	INDENT;

	string altsound_path = get_vpinmame_path();
	if (!altsound_path.empty()) {
		altsound_path += "\\altsound\\";
		altsound_path += game_name;
	}

	Altsound2CsvParser csv_parser(altsound_path);

	if (!csv_parser.parse(samples)) {
		ALT_ERROR(1, "FAILED Altsound2CsvParser::parse()");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::init()");
		return is_initialized;
	}
	ALT_INFO(1, "SUCCESS Altsound2CsvParser::parse()");

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::init()");
	return true;
}

// ---------------------------------------------------------------------------

int Altsound2Processor::getSample(const unsigned int cmd_combined_in)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::getSample()");
	INDENT;

	int sample_idx = -1;
	std::vector<int> matching_samples;

	// Look for sample that matches the current command
	for (int i = 0; i < samples.size(); ++i) {
		if (samples[i].id == cmd_combined_in) {
			matching_samples.push_back(i);
		}
	}

	if (matching_samples.empty()) {
		ALT_INFO(0, "No sample(s) found for ID: %04X", cmd_combined_in);
	}
	else {
		ALT_INFO(0, "Found %lu sample(s) for ID: %04X", matching_samples.size(), cmd_combined_in);
		
		// pick one to play at random
		sample_idx = matching_samples[rand() % matching_samples.size()];
		ALT_INFO(0, "Sample: %s", getShortPath(samples[sample_idx].fname).c_str());
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::getSample()");
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::processStream(const BehaviorInfo& behavior,
	                                   AltsoundStreamInfo* stream_out)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processStream()");
	INDENT;

//	std::string type_str = toString(stream_out->stream_type);

	// process behaviors for this sample type
	bool success = processBehaviors(behavior, *stream_out);
	if (!success) {
		ALT_ERROR(0, "FAILED: Altsound2Processor::processBehaviors()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processStream()");
		return success;
	}

	// create new stream
	success = ALT_CALL(createStream(&common_callback, stream_out));

	if (!success) {
		ALT_ERROR(0, "FAILED AltsoundProcessorBase::createStream()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processStream()");
	return success;
}

// ----------------------------------------------------------------------------
// This is the meat of the G-Sound system.  Each sample type defines behaviors
// that affect streams.  Rather than inferring them from combinations of CSV
// parameters, they are explicitly spelled out. Altsound authors can tweak
// these behaviors to get the overal mixes they want.  G-Sound represents a new
// option for authors.
//
// How to think of behavior processing:
// Every time a new sample is processed, the associated behavior definition
// and the new stream info is sent here.  Each successive behavior processing
// call determines the impact of the new stream on the other stream types.
// For example, if the new stream is an SFX strean, the SFX behavior info and
// stream are sent to processMusicImpacts where the impact of the SFX
// behaviors on MUSIC streams is determined.  It is then passed to the
// behavior processing of the other types for similar processing
// ----------------------------------------------------------------------------

bool Altsound2Processor::processBehaviors(const BehaviorInfo& behavior,
	                                      const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processBehaviors()");
	INDENT;

	if (!processMusicImpacts(behavior, stream)) {
		ALT_ERROR(1, "FAILED Altsound2Processor::processMusicBehavior()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
		return false;
	}

	if (!processCalloutImpacts(behavior, stream)) {
		ALT_ERROR(1, "FAILED  processCalloutBehavior()");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
		return false;
	}

	if (!processSfxImpacts(behavior, stream)) {
		ALT_ERROR(1, "FAILED Altsound2Processor::processSfxBehavior()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
		return false;
	}

	if (!processSoloImpacts(behavior, stream)) {
		ALT_ERROR(1, "FAILED Altsound2Processor::processSoloBehavior()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
		return false;
	}

	if (!processOverlayImpacts(behavior, stream)) {
		ALT_ERROR(1, "FAILED Altsound2Processor::processOverlayBehavior()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
		return false;
	}

	// DEBUG helper
	printBehaviorData();

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processBehaviors()");
	return true;
}

// ----------------------------------------------------------------------------

void Altsound2Processor::postProcessBehaviors(AltsoundSampleType type_in)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::postProcessBehaviors()");
	INDENT;

	// get index into the relevant ducking and paused arrays for the provided
	// sample type
	int idx = streamTypeToIndex[type_in];

	// update ducking behavior
	music_duck_vol[idx] = 1.0f;
	callout_duck_vol[idx] = 1.0f;
	sfx_duck_vol[idx] = 1.0f;
	solo_duck_vol[idx] = 1.0f;
	overlay_duck_vol[idx] = 1.0f;

	// update pausing behavior
	music_paused[idx] = false;
	callout_paused[idx] = false;
	sfx_paused[idx] = false;
	solo_paused[idx] = false;
	overlay_paused[idx] = false;

	// DEBUG helper
	printBehaviorData();

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::postProcessBehaviors()");
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processMusicImpacts(const BehaviorInfo& behavior,
	                                          const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN: Altsound2Processor::processMusicImpacts()");
	INDENT;

	using BB = BehaviorInfo::BehaviorBits;

	// process impact of behavior on MUSIC streams
	if (behavior.stops.test(static_cast<size_t>(BB::MUSIC))) {
		// current sample behavior calls to stop MUSIC stream
		if (!stopMusicStream()) {
			ALT_ERROR(1, "FAILED Altsound2Processor::stopMusicStream()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processMusicImpacts()");
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::MUSIC))) {
		// current sample behavior calls to PAUSE music stream.
		// DAR_TODO what if no stream playing, and then starts?  Wouldn't we still
		// want it paused until the stream that wants it paused ends?
		if (stream.stream_type != MUSIC) {
			if (cur_mus_stream) {
				if (!BASS_ChannelPause(cur_mus_stream->hstream)) {
					ALT_ERROR(1, "FAILED BASS_ChannelPause(): %s", get_bass_err());
					
					OUTDENT;
					ALT_DEBUG(0, "END Altsound2Processor::processMusicImpacts()");
					return false;
				}
				else {
					music_paused[streamTypeToIndex[stream.stream_type]] = true;
					ALT_INFO(1, "SUCCESS BASS_ChannelPause(%u)", cur_mus_stream->hstream);
				}
			}
		}
		else
			ALT_ERROR(1, "MUSIC streams cannot pause another MUSIC stream");
	}

	// DAR@20230627
	// It is possible to both PAUSE and DUCK a stream, to control its volume
	// upon resumption of playback, so this is checked outside of above checks
	if (behavior.ducks.test(static_cast<size_t>(BB::MUSIC))) {
		// current sample behavior calls to DUCK music stream
		if (stream.stream_type != MUSIC) {
			// DAR@20230627
			// Regardless of whether there are current MUSIC streams active
			// we want to set the ducking value. If a MUSIC stream starts before this
			// stream ends, it needs to be ducked correctly
			music_duck_vol[streamTypeToIndex[stream.stream_type]] = behavior.music_duck_vol;
		}
		else {
			ALT_ERROR(1, "MUSIC streams cannot duck another MUSIC stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processMusicImpacts()");
			return false;
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processMusicImpacts()");
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processCalloutImpacts(const BehaviorInfo& behavior,
	                                            const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processCalloutImpacts()");
	INDENT;

	using BB = BehaviorInfo::BehaviorBits;

	// process callout
	if (behavior.stops.test(static_cast<size_t>(BB::CALLOUT))) {
		// current sample behavior calls to stop CALLOUT stream
		if (!stopCalloutStream()) {
			ALT_ERROR(1, "FAILED Altsound2Processor::stopCalloutStream()");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processCalloutImpacts()");
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::CALLOUT))) {
		if (stream.stream_type != CALLOUT) {
			// current sample behavior calls to PAUSE callout stream.
			if (cur_callout_stream) {
				if (!BASS_ChannelPause(cur_callout_stream->hstream)) {
					ALT_ERROR(1, "FAILED BASS_ChannelPause(): %s", get_bass_err());
					
					OUTDENT;
					ALT_DEBUG(0, "END Altsound2Processor::processCalloutImpacts()");
					return false;
				}
				else {
					callout_paused[streamTypeToIndex[stream.stream_type]] = true;
					ALT_INFO(1, "SUCCESS BASS_ChannelPause(%u)", cur_callout_stream->hstream);
				}
			}
		}
		else {
			ALT_ERROR(1, "CALLOUT streams cannot pause another CALLOUT stream");
		}
	}

	// DAR@20230627
	// It is possible to both PAUSE and DUCK a stream, to control its volume
	// upon resumption of playback, so this is checked outside of above checks
	if (behavior.ducks.test(static_cast<size_t>(BB::CALLOUT))) {
		if (stream.stream_type != CALLOUT) {
			// current sample behavior calls to DUCK callout stream

			// DAR@20230627
			// Regardless of whether there is a current CALLOUT stream active
			// we want to set the ducking value. If a CALLOUT stream starts before this
			// stream ends, it needs to be ducked correctly
			callout_duck_vol[streamTypeToIndex[stream.stream_type]] = behavior.callout_duck_vol;
		}
		else {
			ALT_ERROR(1, "CALLOUT streams cannot duck another CALLOUT stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processCalloutImpacts()");
			return false;
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processCalloutImpacts()");
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processSfxImpacts(const BehaviorInfo& behavior,
	                                        const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processSfxImpacts()");
	INDENT;

	using BB = BehaviorInfo::BehaviorBits;

	// process sfx
	if (behavior.stops.test(static_cast<size_t>(BB::SFX))) {
		// current sample behavior call to STOP SFX streams
		if (stream.stream_type != SFX) {
			// DAR@20230627
			// It's not feasible to actually stop SFX streams.  Chances are high
			// new sounds will start before the sample that caused them to stop
			// has finished.  Instead, we can duck them to 0 so all future SFX
			// streams will be muted as well
			//
			// DAR@20230627
			// Regardless of whether there are current SFX streams active
			// we want to set the ducking value. If an SFX stream starts before this
			// stream ends, it needs to be ducked correctly
			sfx_duck_vol[streamTypeToIndex[stream.stream_type]] = 0.0; // mute future SFX streams
		}
		else {
			ALT_ERROR(1, "SFX streams cannot stop another SFX stream");

			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processSfxImpacts()");
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::SFX))) {
		ALT_ERROR(1, "SFX streams cannot be paused");
	}

	// process ducking
	if (behavior.ducks.test(static_cast<size_t>(BB::SFX))) {
		if (stream.stream_type != SFX) {
			// current sample behavior calls to duck SFX streams
			//
			// DAR@20230627
			// Regardless of whether there are current SFX streams active
			// we want to set the ducking value. If an SFX stream starts before this
			// stream ends, it needs to be ducked correctly
			sfx_duck_vol[streamTypeToIndex[stream.stream_type]] = behavior.sfx_duck_vol;
		}
		else {
			ALT_ERROR(1, "SFX streams cannot duck another SFX stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processSfxImpacts()");
			return false;
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processSfxImpacts()");
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processSoloImpacts(const BehaviorInfo& behavior,
                                             const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processSoloImpacts()");
	INDENT;

	using BB = BehaviorInfo::BehaviorBits;

	// process solo
	if (behavior.stops.test(static_cast<size_t>(BB::SOLO))) {
		// current sample behavior calls to stop SOLO stream
		if (!stopSoloStream()) {
			ALT_ERROR(1, "FAILED Altsound2Processor::stopSoloStream()");

			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processSoloImpacts()");
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::SOLO))) {
		// current sample behavior calls to pause SOLO stream.
		if (stream.stream_type != SOLO) {
			if (cur_solo_stream) {
				if (!BASS_ChannelPause(cur_solo_stream->hstream)) {
					ALT_ERROR(1, "FAILED BASS_ChannelPause(): %s", get_bass_err());

					OUTDENT;
					ALT_DEBUG(0, "END Altsound2Processor::processSoloImpacts()");
					return false;
				}
				else {
					solo_paused[streamTypeToIndex[stream.stream_type]] = true;
					ALT_INFO(1, "SUCCESS BASS_ChannelPause(%u)", cur_solo_stream->hstream);
				}
			}
		}
		else {
			ALT_ERROR(1, "SOLO streams cannot pause another SOLO stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processSoloImpacts()");
			return false;
		}
	}

	// DAR@20230627
	// It is possible to both PAUSE and DUCK a stream, to control its volume
	// upon resumption of playback, so this is checked outside of above checks
	if (behavior.ducks.test(static_cast<size_t>(BB::SOLO))) {
		// current sample behavior calls to duck SOLO stream
		if (stream.stream_type != SOLO) {
			// DAR@20230627
			// Regardless of whether there are current SFX streams active
			// we want to set the ducking value. If an SFX stream starts before this
			// stream ends, it needs to be ducked correctly
			solo_duck_vol[streamTypeToIndex[stream.stream_type]] = behavior.solo_duck_vol;
		}
		else {
			ALT_ERROR(1, "SOLO streams cannot duck another SOLO stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processSoloBehavior()");
			return false;
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processSoloBehavior()");
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processOverlayImpacts(const BehaviorInfo& behavior,
	                                           const AltsoundStreamInfo& stream)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processOverlayImpacts()");
	INDENT;

	using BB = BehaviorInfo::BehaviorBits;

	// process overlay
	if (behavior.stops.test(static_cast<size_t>(BB::OVERLAY))) {
		// current sample behavior calls to stop OVERLAY stream
		if (!stopOverlayStream()) {
			ALT_ERROR(1, "FAILED Altsound2Processor::stopOverlayStream()");

			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processOverlayImpacts()");
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::OVERLAY))) {
		// current sample behavior calls to pause OVERLAY stream.
		// DAR_TODO what if no stream playing, and then starts?  Wouldn't we still
		// want it paused until the stream that wants it paused ends?
		if (stream.stream_type != OVERLAY) {
			if (cur_overlay_stream) {
				if (!BASS_ChannelPause(cur_overlay_stream->hstream)) {
					ALT_ERROR(1, "FAILED BASS_ChannelPause(): %s", get_bass_err());

					OUTDENT;
					ALT_DEBUG(0, "END Altsound2Processor::processOverlayImpacts()");
					return false;
				}
				else {
					overlay_paused[streamTypeToIndex[stream.stream_type]] = true;
					ALT_INFO(1, "SUCCESS BASS_ChannelPause(%u)", cur_overlay_stream->hstream);
				}
			}
		}
		else
			ALT_ERROR(1, "OVERLAY streams cannot pause another OVERLAY stream");
	}

	// DAR@20230627
	// It is possible to both PAUSE and DUCK a stream, to control its volume
	// upon resumption of playback, so this is checked outside of above checks
	if (behavior.ducks.test(static_cast<size_t>(BB::OVERLAY))) {
		if (stream.stream_type != OVERLAY) {
			// current sample behavior calls to duck SFX streams
			//
			// DAR@20230627
			// Regardless of whether there are current OVERLAY streams active
			// we want to set the ducking value. If an OVERLAY stream starts before this
			// stream ends, it needs to be ducked correctly
			overlay_duck_vol[streamTypeToIndex[stream.stream_type]] = behavior.overlay_duck_vol;
		}
		else {
			ALT_ERROR(1, "OVERLAY streams cannot duck another OVERLAY stream");
			
			OUTDENT;
			ALT_DEBUG(0, "END Altsound2Processor::processOverlayImpacts()");
			return false;
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processOverlayImpacts()");
	return true;
}

// ----------------------------------------------------------------------------
// This methods is only meant to be called by snd_alt.cpp  It is used to safely
// call stopMusicStream, while maintaing the behavior bookkeeping
// ----------------------------------------------------------------------------
bool Altsound2Processor::stopMusic()
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::stopMusic()");
	INDENT;

	//DAR_TODO add error checking
	if (!stopMusicStream()) {
		ALT_ERROR(1, "FAILED Altsound2Processor::stopMusicStream()");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopMusic()");
		return false;
	}

	// update ducking behavior tracking
	postProcessBehaviors(CALLOUT);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::stopMusic()");
	return true;
}

// ----------------------------------------------------------------------------
// Because of the complex bookkeeping involved with managing behaviors, this
// method should only be called as part of behavior processing.  Otherwise,
// calling this method can cause problems
// ----------------------------------------------------------------------------
bool Altsound2Processor::stopMusicStream()
{
	ALT_DEBUG(0, "BEGIN: Altsound2Processor::stopMusicStream()");
	INDENT;

	if (!cur_mus_stream) {
		ALT_INFO(1, "No active MUSIC stream");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopMusicStream()");
		return true;
	}

	HSTREAM hstream = cur_mus_stream->hstream;
	unsigned int ch_idx = cur_mus_stream->channel_idx;

	ALT_INFO(1, "Current MUSIC stream(%s): HSTREAM: %u  CH: %02d",
		getShortPath(getShortPath(cur_mus_stream->sample_path)).c_str(), hstream, ch_idx);

	bool success = stopStream(hstream);
	if (success) {
		ALT_INFO(1, "Stopped MUSIC stream: %u  Chan: %02d", hstream, ch_idx);
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_mus_stream = nullptr;
	}
	else {
		ALT_ERROR(1, "FAILED stopStream(%u)", hstream);

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopMusicStream()");
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::stopMusicStream()");
	return true;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::stopCalloutStream()
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::stopCalloutStream()");
	INDENT;

	if (!cur_callout_stream) {
		ALT_INFO(1, "No active CALLOUT stream");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopCalloutStream()");
		return true;
	}

	HSTREAM hstream = cur_callout_stream->hstream;
	unsigned int ch_idx = cur_callout_stream->channel_idx;

	ALT_INFO(1, "Current CALLOUT stream(%s): HSTREAM: %u  CH: %02d",
		getShortPath(getShortPath(cur_callout_stream->sample_path)).c_str(), hstream, ch_idx);

	bool success = stopStream(hstream);
	if (success) {
		ALT_INFO(1, "Stopped CALLOUT stream: %u  Chan: %02d", hstream, ch_idx);
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_callout_stream = nullptr;
	}
	else {
		ALT_ERROR(1, "FAILED Altsound2Processor::stopStream(%u)", hstream);
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopCalloutStream()");
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::stopCalloutStream()");
	return success;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::stopSoloStream()
{
    ALT_DEBUG(0, "BEGIN Altsound2Processor::stopSoloStream()");
	INDENT;

	if (!cur_solo_stream) {
		ALT_INFO(1, "No active SOLO stream");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopSoloStream()");
		return true;
	}

	HSTREAM hstream = cur_solo_stream->hstream;
	unsigned int ch_idx = cur_solo_stream->channel_idx;

	ALT_INFO(1, "Current SOLO stream(%s): HSTREAM: %u  CH: %02d",
		getShortPath(getShortPath(cur_solo_stream->sample_path)).c_str(), hstream, ch_idx);

	bool success = stopStream(hstream);
	if (success) {
		ALT_INFO(1, "Stopped SOLO stream: %u  Chan: %02d", hstream, ch_idx);
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_solo_stream = nullptr;
	}
	else {
		ALT_ERROR(1, "FAILED stopStream(%u)", hstream);

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopSoloStream()");
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::stopSoloStream()");
	return success;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::stopOverlayStream()
{
	ALT_DEBUG(0, "BEGIN: Altsound2Processor::stopOverlayStream()");
	INDENT;

	if (!cur_overlay_stream) {
		ALT_INFO(1, "No active OVERLAY stream");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopOverlayStream()");
		return true;
	}

	HSTREAM hstream = cur_overlay_stream->hstream;
	unsigned int ch_idx = cur_overlay_stream->channel_idx;

	ALT_INFO(1, "Current OVERLAY stream(%s): HSTREAM: %u  CH: %02d",
		getShortPath(getShortPath(cur_overlay_stream->sample_path)).c_str(), hstream, ch_idx);

	bool success = stopStream(hstream);
	if (success) {
		ALT_INFO(1, "Stopped OVERLAY stream: %u  Chan: %02d", hstream, ch_idx);
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_overlay_stream = nullptr;
	}
	else {
		ALT_ERROR(1, "FAILED stopStream(%u)", hstream);

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::stopOverlayStream()");
		return false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::stopOverlayStream()");
	return true;
}

// ----------------------------------------------------------------------------

void CALLBACK Altsound2Processor::common_callback(HSYNC handle, DWORD channel,
	                                              DWORD data, void* user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	ALT_DEBUG(0, "\nBEGIN: Altsound2Processor::common_callback()");
	INDENT;

	ALT_INFO(1, "HSYNC: %u  HSTREAM: %u", handle, channel);
	ALT_DEBUG(1, "Acquiring mutex");
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);
	HSTREAM inst_hstream = stream_inst->hstream;

	if (inst_hstream != hstream_in) {
		ALT_ERROR(1, "Callback HSTREAM != instance HSTREAM");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::common_callback()");
		return;
	}

	unsigned int inst_ch_idx = stream_inst->channel_idx;

	switch (stream_inst->stream_type) {
	case SOLO:
		cur_solo_stream = nullptr;
		ALT_INFO(1, "SOLO stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);
		break;

	case MUSIC:
		// DAR@20230706
		// This callback gets hit when the sample ends even if its set to loop.
		// If it's cleaned up here, it will not loop.  A future use may be to
		// create an independing music callback that can limit the number of loops.
		//cur_mus_stream = nullptr;
		//ALT_INFO(1, "MUSIC stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);
		return;
		break;

	case SFX:
		ALT_INFO(1, "SFX stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);
		break;

	case CALLOUT:
		cur_callout_stream = nullptr;
		ALT_INFO(1, "CALLOUT stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);
		break;

	case OVERLAY:
		cur_overlay_stream = nullptr;
		ALT_INFO(1, "OVERLAY stream(%u) finished on ch(%02d)", inst_hstream, inst_ch_idx);
		break;

	default:
		ALT_ERROR(1, "Unknown stream type");

		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::common_callback()");
		return;
	}

	// free stream resources
	if (!ALT_CALL(freeStream(inst_hstream))) {
		ALT_ERROR(1, "FAILED AltsoundProcessorBase::free_stream(%u): %s", inst_hstream, get_bass_err());
	}

	// DAR@20230702 Must do this BEFORE deleting the
	// channel_stream object or else the data is destroyed
	//
	// update ducking behavior tracking
	postProcessBehaviors(stream_inst->stream_type);

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::common_callback()");
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::adjustStreamVolumes()
{
	ALT_INFO(0, "BEGIN Altsound2Processor::adjustStreamVolumes()");
	INDENT;

	bool success = true;
	int num_x_streams = 0;

	std::unordered_map<AltsoundSampleType, std::array<float, 5>*> volumeStatusMap = {
		{MUSIC, &music_duck_vol},
		{CALLOUT, &callout_duck_vol},
		{SFX, &sfx_duck_vol},
		{SOLO, &solo_duck_vol},
		{OVERLAY, &overlay_duck_vol}
	};

	for (const auto& streamPtr : channel_stream) {
		if (!streamPtr) continue; // Stream is not defined

		const auto& stream = *streamPtr; // Dereference pointer for readability

		auto iter = volumeStatusMap.find(stream.stream_type);
		if (iter == volumeStatusMap.end()) {
			ALT_ERROR(1, "Unknown stream type");
			success = false;
			continue;
		}

		// get ducking volume
		float ducking_value = *std::min_element(iter->second->begin(), iter->second->end());

		// get group volume
		float grp_vol = group_vol[streamTypeToIndex[stream.stream_type]];

		ALT_DEBUG(1, "%s stream %u gain:  %.02f  group_vol:  %.02f ducked_vol:  %.02f", toString(stream.stream_type), 
			       stream.hstream, stream.gain, grp_vol, ducking_value);

		float adjusted_vol = stream.gain * ducking_value * grp_vol;
		if (!setStreamVolume(stream.hstream, adjusted_vol)) {
			ALT_ERROR(1, "FAILED setStreamVolume()");
			success = false;
		}
		num_x_streams++;
	}

	ALT_INFO(1, "Num active streams: %d", num_x_streams);

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::adjustStreamVolumes()");
	return success;
}


// ----------------------------------------------------------------------------

bool Altsound2Processor::processPausedStreams()
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::processPausedStreams()");
	INDENT;

	auto pauseStatusMap = buildPauseStatusMap();

	bool success = true;
	for (const auto& stream : channel_stream) {
		if (stream) {
			success &= tryResumeStream(*stream, pauseStatusMap);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::processPausedStreams()");
	return success;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::tryResumeStream(const AltsoundStreamInfo& stream,
	                                     const PausedStatusMap& pauseStatusMap)
{
	ALT_DEBUG(0, "BEGIN Altsound2Processor::tryResumeStream()");
	INDENT;

	auto pauseStatusMapIter = pauseStatusMap.find(stream.stream_type);
	if (pauseStatusMapIter == pauseStatusMap.end()) {
		ALT_ERROR(1, "Unknown stream type");
		
		OUTDENT;
		ALT_DEBUG(0, "END Altsound2Processor::tryResumeStream()");
		return false;
	}

	auto& pauseStatusArray = *pauseStatusMapIter->second;
	if (!isAnyPaused(pauseStatusArray)) {
		HSTREAM hstream = stream.hstream;

		if (BASS_ChannelIsActive(hstream) == BASS_ACTIVE_PAUSED) {
			if (!BASS_ChannelPlay(hstream, 0)) {
				ALT_ERROR(1, "FAILED BASS_ChannelPlay(%u): %s", hstream, get_bass_err());

				OUTDENT;
				ALT_DEBUG(0, "END Altsound2Processor::tryResumeStream()");
				return false;
			}
			ALT_INFO(1, "Successfully resumed stream: %u", hstream);
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END Altsound2Processor::tryResumeStream()");
	return true;
}

// ----------------------------------------------------------------------------
// Helper function to build the common map of stream types to paused_status
// arrays.  This is used for behavior processing and management
// ----------------------------------------------------------------------------

PausedStatusMap Altsound2Processor::buildPauseStatusMap()
{
	return {
		{MUSIC, &music_paused},
		{CALLOUT, &callout_paused},
		{SFX, &sfx_paused},
		{SOLO, &solo_paused},
		{OVERLAY, &overlay_paused}
	};
}

// ---------------------------------------------------------------------------
// Helper function to check if any of the stream type are still requesting a
// pause
// ---------------------------------------------------------------------------

bool Altsound2Processor::isAnyPaused(const std::array<bool, NUM_STREAM_TYPES>& pauseStatusArray)
{
	return std::any_of(pauseStatusArray.begin(), pauseStatusArray.end(), [](bool value) {
		return value;
	});
}

// ---------------------------------------------------------------------------
// Helper DEBUG functions to output all behavior bookkeeping data
// ---------------------------------------------------------------------------
void Altsound2Processor::printBehaviorData() {
	ALT_DEBUG(0, "");
	ALT_DEBUG(1, "                     MUS   CALL   SFX   SOLO  OVER");
	ALT_DEBUG(1, "                   ---------------------------------");

	printArray("music_duck_vol   ", music_duck_vol);
	printArray("callout_duck_vol ", callout_duck_vol);
	printArray("sfx_duck_vol     ", sfx_duck_vol);
	printArray("solo_duck_vol    ", solo_duck_vol);
	printArray("overlay_duck_vol ", overlay_duck_vol);
	ALT_DEBUG(1, "");
	printArray("group_vol        ", group_vol);
	ALT_DEBUG(1, "");
	printArrayBool("music_paused   ", music_paused);
	printArrayBool("callout_paused ", callout_paused);
	printArrayBool("sfx_paused     ", sfx_paused);
	printArrayBool("solo_paused    ", solo_paused);
	printArrayBool("overlay_paused ", overlay_paused);
	ALT_DEBUG(0, "");
}

void Altsound2Processor::printArray(const std::string& name, const std::array<float, NUM_STREAM_TYPES>& arr) {
	std::ostringstream oss;
	oss << name << ": { ";

	for (const auto& val : arr) {
		oss << std::fixed << std::setprecision(2) << val << ", ";
	}
	oss << "}";
	ALT_DEBUG(1,"%s", oss.str().c_str());
}

void Altsound2Processor::printArrayBool(const std::string& name, const std::array<bool, NUM_STREAM_TYPES>& arr) {
	std::ostringstream oss;
	oss << name << ": { ";
	
	for (const auto& val : arr) {
		oss << (val ? "true" : "false") << ", ";
	}
	oss << "}";
	ALT_DEBUG(1, "%s", oss.str().c_str());
}