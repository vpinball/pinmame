// ---------------------------------------------------------------------------
// gsound_processor.hpp
//
// Encapsulates all specialized processing for the G-Sound
// CSV format
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------
#pragma once

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

// Library includes
#include <string>
#include <array>
#include <random>
#include <unordered_map>

// Local includes
#include "altsound_processor_base.hpp"
#include "..\ext\bass\bass.h"
#include "altsound_logger.hpp"

constexpr int NUM_STREAM_TYPES = 5;

// ---------------------------------------------------------------------------
// GSoundProcessor class definition
// ---------------------------------------------------------------------------

class GSoundProcessor final : public AltsoundProcessorBase
{
public:

	// Default constructor
	GSoundProcessor() = delete;

	// Copy Constructor
	GSoundProcessor(GSoundProcessor&) = delete;

	// Standard constructor
	GSoundProcessor(const std::string& game_name, const std::string& vpm_path);

	// Destructor
	~GSoundProcessor() override;

	// External interface to stop MUSIC stream
	bool stopMusic() override;

	// Process ROM commands to the sound board
	bool handleCmd(const unsigned int cmd_in) override;

	// DEBUG helper fns to print all behavior data
	static void printBehaviorData();

protected:

private: // functions

	//
	void init() override;

	// parse CSV file and populate sample data
	bool loadSamples() override;

	// find sample matching provided command
	unsigned int getSample(const unsigned int cmd_combined_in) override;

	// process stream commands
	bool processStream(const BehaviorInfo& behavior, AltsoundStreamInfo* stream_out);

	// Process impacts of sample type behaviors
	bool processBehaviors(const BehaviorInfo& behavior, const AltsoundStreamInfo* stream);

	// Update behavior impacts when streams end
	static bool postProcessBehaviors(const BehaviorInfo& behavior, const AltsoundStreamInfo& finished_stream);

	// Stop the exclusive stream referenced by stream_ptr
	bool stopExclusiveStream(const AltsoundSampleType stream_type);

	// BASS SYNCPROC callback whan a stream ends
	static void CALLBACK common_callback(HSYNC handle, DWORD channel, DWORD data, void* user);

	// adjust volume of active streams to accommodate current ducking impacts
	static bool adjustStreamVolumes();

	// determine lowest ducking volume impacts on stream_type
	static float findLowestDuckVolume(AltsoundSampleType stream_type);

	// process PAUSED behavior impacts for all streams
	static bool processPausedStreams();

	// resume paused playback on streams that no longer need to be paused
	static bool tryResumeStream(const AltsoundStreamInfo& stream);

private: // data

	bool is_initialized;
	bool is_stable; // future use
	std::vector<GSoundSampleInfo> samples;
	std::mt19937 generator; // mersenne twister
};
