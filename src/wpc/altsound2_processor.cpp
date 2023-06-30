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
#include <unordered_map>
#include <string>

// Local includes
#include "osdepend.h"
#include "altsound2_csv_parser.hpp"

using std::string;

// NOTE:
// - SFX streams don't require tracking since multiple can play,
//   simultaneously, and other than adjusting ducking, have no other
//   impacts on active streams
//
static AltsoundStreamInfo* cur_mus_stream = nullptr;
static AltsoundStreamInfo* cur_callout_stream = nullptr;
static AltsoundStreamInfo* cur_solo_stream = nullptr;

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
	// cur_mus_stream and cur_jin_stream are copies of pointers stored in
	// channel_stream[].  They cannot be deleted here, since they will be
	// deleted when the channel_stream[] is destroyed.  Just set to null
	// here
	//
	// clean up stream tracking
	cur_mus_stream = nullptr;
	cur_callout_stream = nullptr;
	cur_solo_stream = nullptr;
	
	stopAllStreams();
}

// ---------------------------------------------------------------------------
// Function implementation
// ---------------------------------------------------------------------------

bool Altsound2Processor::handleCmd(const unsigned int cmd_combined_in)
{
	LOG(("BEGIN Altsound2Processor::handleCmd()\n")); //DAR_DEBUG

	LOG(("- Acquiring mutex...\n")); //DAR_DEBUG
	std::lock_guard<std::mutex> guard(io_mutex);

	if (!is_initialized || !is_stable) {
		if (!is_initialized) {
			LOG(("- Altsound2Processor is not initialized. Processing skipped\n"));
		}
		else {
			LOG(("- Altsound2Processor is unstable. Processing skipped\n"));
		}

		LOG(("END: Altsound2Processor::handleCmd()\n")); //DAR_DEBUG
		return false;
	}

	// get sample for playback
	int sample_idx = getSample(cmd_combined_in);

	if (sample_idx == -1) {
		// No matching command.  Clean up and exit
		LOG(("- FAILED: get_sample()\n", cmd_combined_in)); //DAR_DEBUG

		//LOG(("END: Altsound2Processor::handleCmd()\n")); //DAR_DEBUG
		return false;
	}
	LOG(("- SUCCESS: get_sample(%04X)\n", cmd_combined_in));

	AltsoundStreamInfo* new_stream = new AltsoundStreamInfo();

	// pre-populate stream info
	new_stream->sample_path = samples[sample_idx].fname;
	new_stream->gain = samples[sample_idx].gain;

	AltsoundSampleType sample_type = toSampleType(samples[sample_idx].type);

	// Instead of multiple if conditions, we can use a switch
	switch (sample_type) {
	case MUSIC:
		new_stream->stream_type = MUSIC;
		if (processStream(music_behavior, &music_callback, new_stream)) {
			channel_stream[new_stream->channel_idx] = cur_mus_stream = new_stream;
		}
		else {
			LOG(("- FAILED: processMusic()\n")); //DAR_DEBUG
		}
		break;
	case SFX:
		new_stream->stream_type = SFX;
		if (processStream(sfx_behavior, &sfx_callback, new_stream)) {
			channel_stream[new_stream->channel_idx] = new_stream;
		}
		else {
			LOG(("- FAILED: processSfx()\n")); //DAR_DEBUG
		}
		break;
	case CALLOUT:
		new_stream->stream_type = CALLOUT;
		if (processStream(callout_behavior, &callout_callback, new_stream)) {
			channel_stream[new_stream->channel_idx] = cur_callout_stream = new_stream;
		}
		else {
			LOG(("- FAILED: processCallout()\n")); //DAR_DEBUG
		}
		break;
	case SOLO:
		new_stream->stream_type = SOLO;
		if (processStream(solo_behavior, &solo_callback, new_stream)) {
			channel_stream[new_stream->channel_idx] = cur_solo_stream = new_stream;
		}
		else {
			LOG(("- FAILED: processSolo()\n")); //DAR_DEBUG
		}
		break;
	case OVERLAY:
		new_stream->stream_type = OVERLAY;
		if (processStream(overlay_behavior, &overlay_callback, new_stream)) {
			channel_stream[new_stream->channel_idx] = new_stream;
		}
		else {
			LOG(("- FAILED: processOverlay()\n")); //DAR_DEBUG
		}
		break;
	}

	// set volume for active streams
	adjustStreamVolumes();

	// Play pending sound determined above, if any
	if (new_stream->hstream != BASS_NO_STREAM) {
		if (!BASS_ChannelPlay(new_stream->hstream, 0)) {
			// Sound playback failed
			LOG(("- FAILED: BASS_ChannelPlay(%u): %s\n", new_stream->hstream, get_bass_err()));
		}
		else {
			LOG(("- SUCCESS: BASS_ChannelPlay(%u): CH(%d) CMD(%04X) SAMPLE(%s)\n", \
				new_stream->hstream, new_stream->channel_idx, cmd_combined_in, \
				getShortPath(new_stream->sample_path).c_str()));
		}
	}

	LOG(("END: Altsound2Processor::handleCmd()\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

void Altsound2Processor::init()
{
	// reset stream tracking
	cur_mus_stream = nullptr;
	cur_callout_stream = nullptr;
	cur_solo_stream = nullptr;

	//LOG(("BEGIN Altsound2Processor::init()\n")); //DAR_DEBUG
	if (!loadSamples()) {
		LOG(("FAILED: Altsound2Processor::loadSamples()\n"));

		//LOG(("END: Altsound2Processor::init()\n")); //DAR_DEBUG
	}
	LOG(("SUCCESS: Altsound2Processor::loadSamples()\n"));

	// if we are here, initialization succeeded
	is_initialized = true;

	//LOG(("END: Altsound2Processor::init()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::loadSamples()
{
	//LOG(("BEGIN loadSamples()\n")); //DAR_DEBUG
	string altsound_path = get_vpinmame_path();
	if (!altsound_path.empty()) {
		altsound_path += "\\altsound\\";
		altsound_path += game_name;
//		altsound_path += "\\altsound2.csv";
	}

	Altsound2CsvParser csv_parser(altsound_path);

	if (!csv_parser.parse(samples)) {
		LOG(("- FAILED: csv_parser.parse()\n")); //DAR_DEBUG

		//LOG(("END: Altsound2Processor::init()\n")); //DAR_DEBUG
		return is_initialized;
	}
	LOG(("- SUCCESS: csv_parser.parse()\n")); //DAR_DEBUG

    //LOG(("END: loadSamples\n")); //DAR_DEBUG
	return true;
}

// ---------------------------------------------------------------------------

int Altsound2Processor::getSample(const unsigned int cmd_combined_in)
{
	LOG(("BEGIN getSample()\n")); //DAR_DEBUG

	int sample_idx = -1;
	std::vector<int> matching_samples;

	// Look for sample that matches the current command
	for (int i = 0; i < samples.size(); ++i) {
		if (samples[i].id == cmd_combined_in) {
			matching_samples.push_back(i);
		}
	}

	if (matching_samples.empty()) {
		LOG(("- FAILED: No sample(s) found for ID: %04X\n", cmd_combined_in)); //DAR_DEBUG
	}
	else {
		LOG(("- SUCCESS: Found %lu sample(s) for ID: %04X\n", matching_samples.size(), cmd_combined_in)); //DAR_DEBUG
		
		// pick one to play at random
		sample_idx = matching_samples[rand() % matching_samples.size()];
		LOG(("- SAMPLE: %s\n", getShortPath(samples[sample_idx].fname).c_str()));
	}

	LOG(("END: getSample()\n")); //DAR_DEBUG
	return sample_idx;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::processStream(const BehaviorInfo& behavior, void* syncproc,
	                                   AltsoundStreamInfo* stream_out)
{
	std::string type_str = toString(stream_out->stream_type);
	LOG(("BEGIN processStream(%s)\n", type_str.c_str()));

	// process behaviors for this sample type
	bool success = processBehaviors(behavior, *stream_out);
	if (!success) {
		LOG(("FAILED: processBehaviors()\n"));
		return success;
	}

	// create new music stream
	success = createStream(syncproc, stream_out);
	if (!success) {
		LOG(("FAILED: createStream()\n"));
	}

	LOG(("END: processStream(%s)\n", type_str.c_str()));
	return success;
}

// ----------------------------------------------------------------------------
// This is the meat of the GSound system.  Each sample type defines behaviors
// that affect streams.  Rather than inferring them from combinations of CSV
// parameters, they are explicitly spelled out in the altsound.ini file.
// Altsound authors can tweak these behaviors to get the overal mixes they want.
// GSound represents a new option for authors, and is not intended to replace
// the current AltSound system
// ----------------------------------------------------------------------------

bool Altsound2Processor::processBehaviors(const BehaviorInfo& behavior,
	                                      const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processBehaviors()\n"));

	if (!processMusicBehavior(behavior, stream)) {
		return false;
	}

	if (!processCalloutBehavior(behavior, stream)) {
		return false;
	}

	if (!processSfxBehavior(behavior, stream)) {
		return false;
	}

	if (!processSoloBehavior(behavior, stream)) {
		return false;
	}

	if (!processOverlayBehavior(behavior, stream)) {
		return false;
	}

#ifdef _DEBUG
	printBehaviorData();
#endif
	LOG(("END: processBehaviors()\n"));
	return true;
}

// ----------------------------------------------------------------------------

void Altsound2Processor::postProcessBehaviors(AltsoundSampleType type_in)
{
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

#ifdef _DEBUG
	printBehaviorData();
#endif

}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processMusicBehavior(const BehaviorInfo& behavior,
	                                          const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processMusicBehavior()\n"));

	using BB = BehaviorInfo::BehaviorBits;

	// process impact of behavior on MUSIC streams
	if (behavior.stops.test(static_cast<size_t>(BB::MUSIC))) {
		// current sample behavior calls to stop MUSIC stream
		if (!stopMusicStream()) {
			LOG(("- FAILED: stopMusicStream()\n"));
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
					LOG(("- FAILED: BASS_ChannelPause(): %s\n", get_bass_err()));
					return false;
				}
				else {
					music_paused[streamTypeToIndex[stream.stream_type]] = true;
					LOG(("- SUCCESS: BASS_ChannelPause(%u)\n", cur_mus_stream->hstream));
				}
			}
		}
		else
			LOG(("- ERROR: MUSIC streams cannot pause another MUSIC stream\n"));
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
			LOG(("- ERROR: MUSIC streams cannot duck another MUSIC stream\n"));
			return false;
		}
	}

	LOG(("END: processMusicBehavior()\n"));
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processCalloutBehavior(const BehaviorInfo& behavior,
	                                            const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processCalloutBehavior()\n"));
	using BB = BehaviorInfo::BehaviorBits;

	// process callout
	if (behavior.stops.test(static_cast<size_t>(BB::CALLOUT))) {
		// current sample behavior calls to stop CALLOUT stream
		if (!stopCalloutStream()) {
			LOG(("- FAILED: stopCalloutStream()\n"));
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::CALLOUT))) {
		if (stream.stream_type != CALLOUT) {
			// current sample behavior calls to PAUSE callout stream.
			if (cur_callout_stream) {
				if (!BASS_ChannelPause(cur_callout_stream->hstream)) {
					LOG(("- FAILED: BASS_ChannelPause(): %s\n", get_bass_err()));
					return false;
				}
				else {
					callout_paused[streamTypeToIndex[stream.stream_type]] = true;
					LOG(("- SUCCESS: BASS_ChannelPause(%u)\n", cur_callout_stream->hstream));
				}
			}
		}
		else {
			LOG(("- ERROR: CALLOUT streams cannot pause another CALLOUT stream\n"));
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
			LOG(("- ERROR: CALLOUT streams cannot duck another CALLOUT stream\n"));
			return false;
		}
	}

	LOG(("END: processCalloutBehavior()\n"));
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processSfxBehavior(const BehaviorInfo& behavior,
	                                        const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processSfxBehavior()\n"));
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
			LOG(("- ERROR: SFX streams cannot stop another SFX stream\n"));
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::SFX))) {
		LOG(("- ERROR: SFX streams cannot be paused\n"));
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
			LOG(("- ERROR: SFX streams cannot duck another SFX stream\n"));
			return false;
		}
	}

	LOG(("END: processSfxBehavior()\n"));
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processSoloBehavior(const BehaviorInfo& behavior,
                                             const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processSoloBehavior()\n"));
	using BB = BehaviorInfo::BehaviorBits;

	// process solo
	if (behavior.stops.test(static_cast<size_t>(BB::SOLO))) {
		// current sample behavior calls to stop SOLO stream
		if (!stopSoloStream()) {
			LOG(("- FAILED: stopSoloStream()\n"));
			return false;
		}
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::SOLO))) {
		// current sample behavior calls to pause SOLO stream.
		if (stream.stream_type != SOLO) {
			if (cur_solo_stream) {
				if (!BASS_ChannelPause(cur_solo_stream->hstream)) {
					LOG(("- FAILED: BASS_ChannelPause(): %s\n", get_bass_err()));
					return false;
				}
				else {
					solo_paused[streamTypeToIndex[stream.stream_type]] = true;
					LOG(("- SUCCESS: BASS_ChannelPause(%u)\n", cur_solo_stream->hstream));
				}
			}
		}
		else {
			LOG(("- ERROR: SOLO streams cannot pause another SOLO stream\n"));
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
			LOG(("- ERROR: SOLO streams cannot duck another SOLO stream\n"));
			return false;
		}
	}

	LOG(("END: processSoloBehavior()\n"));
	return true;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processOverlayBehavior(const BehaviorInfo& behavior,
	                                           const AltsoundStreamInfo& stream)
{
	LOG(("BEGIN: processOverlayBehavior()\n"));
	using BB = BehaviorInfo::BehaviorBits;

	// process overlay
	if (behavior.stops.test(static_cast<size_t>(BB::OVERLAY))) {
		// current sample behavior calls to stop OVERLAY streams
		if (stream.stream_type != OVERLAY) {
			for (auto stream : channel_stream) {
				if (!stream || stream->stream_type != OVERLAY)
					continue;

				if (!stopStream(stream->hstream)) {
					LOG(("- FAILED: stopStream(%u)\n", stream->hstream));
					return false;
				}
			}
		}
		else
			LOG(("- ERROR: OVERLAY streams cannot stop another OVERLAY stream\n"));
	}
	else if (behavior.pauses.test(static_cast<size_t>(BB::OVERLAY))) {
		// curremt sample behavior calls for pausing OVERLAY streams
		if (stream.stream_type != OVERLAY) {
			// pause all active OVERLAY streams
			for (auto stream : channel_stream) {
				if (!stream || stream->stream_type != OVERLAY)
					continue;

				if (!BASS_ChannelPause(stream->hstream)) {
					LOG(("- FAILED: BASS_ChannelPause(): %s\n", get_bass_err()));
					return false;
				}
				else {
					overlay_paused[streamTypeToIndex[stream->stream_type]] = true;
					LOG(("- SUCCESS: BASS_ChannelPause(%u)\n", stream->hstream));
				}
			}
		}
		else
			LOG(("- ERROR: OVERLAY streams cannot pause another OVERLAY stream\n"));
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
			LOG(("- ERROR: OVERLAY streams cannot duck another OVERLAY stream\n"));
			return false;
		}
	}

	LOG(("END: processOverlayBehavior()\n"));
	return true;
}

// ----------------------------------------------------------------------------
// This methods is only meant to be called by snd_alt.cpp  It is used to safely
// call stopMusicStream, while maintaing the behavior bookkeeping
// ----------------------------------------------------------------------------
bool Altsound2Processor::stopMusic()
{
	//DAR_TODO add error checking
	if (!stopMusicStream()) {
		LOG(("FAILED: stopMusicStream()\n"));
		return false;
	}

	// update ducking behavior tracking
	postProcessBehaviors(CALLOUT);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	return true;
}

// ----------------------------------------------------------------------------
// Because of the complex bookkeeping involved with managing behaviors, this
// method should only be called as part of behavior processing.  Otherwise,
// calling this method can cause problems
// ----------------------------------------------------------------------------
bool Altsound2Processor::stopMusicStream()
{
	if (!cur_mus_stream) {
		// no MUSIC stream active
		return true;
	}

	HSTREAM hstream = cur_mus_stream->hstream;
	unsigned int ch_idx = cur_mus_stream->channel_idx;

	LOG(("Current MUSIC stream(%s): HSTREAM: %u  CH: %02d\n",
		getShortPath(getShortPath(cur_mus_stream->sample_path)).c_str(), hstream, ch_idx));

	bool success = stopStream(hstream);
	if (success) {
		LOG(("- Stopped MUSIC stream: %u  Chan: %02d\n", hstream, ch_idx));
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_mus_stream = nullptr;
	}
	else {
		LOG(("- FAILED: stopStream(%u)\n", hstream));
	}

	return true;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::stopCalloutStream()
{
	// LOG(("BEGIN stopCalloutStream()\n"));
	if (!cur_callout_stream) {
		// no CALLOUT stream active
		return true;
	}

	HSTREAM hstream = cur_callout_stream->hstream;
	unsigned int ch_idx = cur_callout_stream->channel_idx;

	LOG(("Current CALLOUT stream(%s): HSTREAM: %u  CH: %02d\n",
		getShortPath(getShortPath(cur_callout_stream->sample_path)).c_str(), hstream, ch_idx));

	bool success = stopStream(hstream);
	if (success) {
		LOG(("- Stopped CALLOUT stream: %u  Chan: %02d\n", hstream, ch_idx));
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_callout_stream = nullptr;
	}
	else {
		LOG(("- FAILED: stopStream(%u)\n", hstream));
	}

	// LOG(("END: stopCalloutStream()\n"));
	return success;
}

// ---------------------------------------------------------------------------

bool Altsound2Processor::stopSoloStream()
{
	// LOG(("BEGIN stopSoloStream()\n"));
	if (!cur_solo_stream) {
		// no SOLO stream active
		return true;
	}

	HSTREAM hstream = cur_solo_stream->hstream;
	unsigned int ch_idx = cur_solo_stream->channel_idx;

	LOG(("Current MUSIC stream(%s): HSTREAM: %u  CH: %02d\n",
		getShortPath(getShortPath(cur_mus_stream->sample_path)).c_str(), hstream, ch_idx));

	bool success = stopStream(hstream);
	if (success) {
		LOG(("- Stopped SOLO stream: %u  Chan: %02d\n", hstream, ch_idx));
		delete channel_stream[ch_idx];
		channel_stream[ch_idx] = nullptr;
		cur_solo_stream = nullptr;
	}
	else {
		LOG(("- FAILED: stopStream(%u)\n", hstream));
	}

	// LOG(("END: stopSoloStream()\n"));
	return success;
}

// ---------------------------------------------------------------------------

void CALLBACK Altsound2Processor::overlay_callback(HSYNC handle, DWORD channel, \
	                                               DWORD data, void* user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("\nBEGIN: overlay_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel));

	LOG(("- Acquiring mutex...\n"));
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != OVERLAY) {
		LOG(("instance HSTREAM is NOT an OVERLAY stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;

	LOG(("- OVERLAY stream(%u) finished on ch(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err())); // DAR_DEBUG
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;

	// update ducking behavior tracking
	postProcessBehaviors(OVERLAY);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	LOG(("END: overlay_callback()\n"));
}

// ---------------------------------------------------------------------------

void CALLBACK Altsound2Processor::callout_callback(HSYNC handle, DWORD channel,\
                                	               DWORD data, void *user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("\nBEGIN: callout_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel));

	LOG(("- Acquiring mutex...\n"));
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != CALLOUT) {
		LOG(("instance HSTREAM is NOT a CALLOUT stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;
		LOG(("- CALLOUT stream(%u) finished on ch(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err())); // DAR_DEBUG
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;
	cur_callout_stream = nullptr;

	// update ducking behavior tracking
	postProcessBehaviors(CALLOUT);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	LOG(("END: callout_callback()\n"));
}

// ---------------------------------------------------------------------------

void CALLBACK Altsound2Processor::sfx_callback(HSYNC handle, DWORD channel, \
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

	// update ducking behavior tracking
	postProcessBehaviors(SFX);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	LOG(("END: sfx_callback()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK Altsound2Processor::music_callback(HSYNC handle, DWORD channel, \
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
	if (stream_inst->hstream != hstream_in) {
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

	// update ducking behavior tracking
	postProcessBehaviors(MUSIC);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();
	
	LOG(("END: music_callback()\n")); //DAR_DEBUG
}

// ---------------------------------------------------------------------------

void CALLBACK Altsound2Processor::solo_callback(HSYNC handle, DWORD channel, \
	DWORD data, void* user)
{
	// All SYNCPROC functions run on the same thread, and will block other
	// sync processes, so these should be fast.  The SYNCPROC thread is
	// separate from the main thread so thread safety should be considered
	LOG(("\nBEGIN: overlay_callback()\n"));
	LOG(("- HSYNC: %u  HSTREAM: %u\n", handle, channel));

	LOG(("- Acquiring mutex...\n"));
	std::lock_guard<std::mutex> guard(io_mutex);

	HSTREAM hstream_in = static_cast<HSTREAM>(channel);
	AltsoundStreamInfo* stream_inst = static_cast<AltsoundStreamInfo*>(user);

	// DAR@20230621
	// The following is not strictly necessary, but I'm keeping it here until
	// I'm comfortable these situations don't/can't happen
	if (stream_inst->hstream != hstream_in) {
		LOG(("callback HSTREAM != instance HSTREAM\n"));
	}
	else if (stream_inst->stream_type != SOLO) {
		LOG(("instance HSTREAM is NOT an SOLO stream\n"));
	}

	HSTREAM inst_hstream = stream_inst->hstream;
	unsigned int inst_ch_idx = stream_inst->channel_idx;

	LOG(("- SOLO stream(%u) finished on ch(%02d)\n", inst_hstream, inst_ch_idx));

	// free stream resources
	if (!freeStream(inst_hstream)) {
		LOG(("- FAILED: free_stream(%u): %s\n", inst_hstream, get_bass_err())); // DAR_DEBUG
	}

	// reset tracking variables
	delete channel_stream[inst_ch_idx];
	channel_stream[inst_ch_idx] = nullptr;

	// update ducking behavior tracking
	postProcessBehaviors(SOLO);

	// re-adjust ducked volumes
	adjustStreamVolumes();

	// update paused streams
	processPausedStreams();

	LOG(("END: jingle_callback()\n"));
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::adjustStreamVolumes()
{
	LOG(("BEGIN: adjustStreamVolumes()\n"));
	bool success = true;
	int num_x_streams = 0;

	std::unordered_map<AltsoundSampleType, std::array<float, 5>*> volumeStatusMap = {
		{MUSIC, &music_duck_vol},
		{CALLOUT, &callout_duck_vol},
		{SFX, &sfx_duck_vol},
		{SOLO, &solo_duck_vol},
		{OVERLAY, &overlay_duck_vol}
	};

	for (const auto& stream : channel_stream) {
		if (stream) {
			// Stream defined on the channel
			float adjusted_vol = 0.0f;
			auto iter = volumeStatusMap.find(stream->stream_type);
			if (iter != volumeStatusMap.end()) {
				adjusted_vol = stream->gain * (*std::min_element(iter->second->begin(), iter->second->end()));
			}
			else {
				LOG(("ERROR: Unknown stream type\n"));
				return false;
			}

			if (!setStreamVolume(stream->hstream, adjusted_vol))
				success = false;

			LOG(("- channel_stream[%d]: STREAM: %u  DUCKING: %0.02f\n", num_x_streams, stream->hstream, stream->ducking));
			num_x_streams++;
		}
	}

	LOG(("- Num active streams: %d\n", num_x_streams));
	LOG(("END: adjustStreamVolumes()\n"));
	return success;
}

// ----------------------------------------------------------------------------

bool Altsound2Processor::processPausedStreams()
{
	LOG(("BEGIN: processPausedStreams()\n"));

	// DAR@20230628
	// Creating a map of the <stream>_paused arrays allows for more efficient
	// processing downstream
	//
	// Mapping of StreamType to pause array
	std::unordered_map<AltsoundSampleType, std::array<bool, NUM_STREAM_TYPES>*> pauseStatusMap = {
		{MUSIC, &music_paused},
		{CALLOUT, &callout_paused},
		{SFX, &sfx_paused},
		{SOLO, &solo_paused},
		{OVERLAY, &overlay_paused}
	};

	bool success = true;

	for (const auto& stream : channel_stream) {
		if (stream) {
			// If pauseStatusMap doesn't contain the stream type, log an error and return
			if (pauseStatusMap.count(stream->stream_type) == 0) {
				LOG(("ERROR: Unknown stream type\n"));
				return false;
			}

			auto& pauseStatusArray = *pauseStatusMap[stream->stream_type];
			if (!isAnyPaused(pauseStatusArray)) {
				HSTREAM hstream = stream->hstream;

				if (hstream != 0 && BASS_ChannelIsActive(hstream) == BASS_ACTIVE_PAUSED) {
					if (!BASS_ChannelPlay(hstream, 0)) {
						LOG(("- FAILED: BASS_ChannelPlay(%u): %s\n", hstream, get_bass_err()));
						success = false;
					}
				}
				else {
					LOG(("ERROR: Stream: %u is not paused\n", hstream));
					success = false;
				}
			}
		}
	}

	LOG(("END: processPausedStreams()\n"));
	return success;
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
// Helper DEBUG function to output all behavior bookkeeping data
// ---------------------------------------------------------------------------
void Altsound2Processor::printBehaviorData() {
	// Then you call these functions with your arrays
	printArray("music_duck_vol   ", music_duck_vol);
	printArray("callout_duck_vol ", callout_duck_vol);
	printArray("sfx_duck_vol     ", sfx_duck_vol);
	printArray("solo_duck_vol    ", solo_duck_vol);
	printArray("overlay_duck_vol ", overlay_duck_vol);
	
	LOG(("\n"));

	printArrayBool("music_paused   ", music_paused);
	printArrayBool("callout_paused ", callout_paused);
	printArrayBool("sfx_paused     ", sfx_paused);
	printArrayBool("solo_paused    ", solo_paused);
	printArrayBool("overlay_paused ", overlay_paused);
}

void Altsound2Processor::printArray(const std::string& name, const std::array<float, NUM_STREAM_TYPES>& arr) {
	std::ostringstream oss;
	oss << name << ": { ";

	for (const auto& val : arr) {
		oss << std::fixed << std::setprecision(2) << val << ", ";
	}
	oss << "}" << std::endl;
	LOG(("%s", oss.str().c_str()));
}

void Altsound2Processor::printArrayBool(const std::string& name, const std::array<bool, NUM_STREAM_TYPES>& arr) {
	std::ostringstream oss;
	oss << name << ": { ";
	
	for (const auto& val : arr) {
		oss << (val ? "true" : "false") << ", ";
	}
	oss << "}" << std::endl;
	LOG(("%s", oss.str().c_str()));
}