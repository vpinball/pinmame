// ---------------------------------------------------------------------------
// altsound2_processor.cpp
// 06/14/23 - Dave Roscoe
//
// Encapsulates all specialized processing for the Altsound2
// CSV format
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND2_PROCESSOR_H
#define ALTSOUND2_PROCESSOR_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Library includes
#include <string>
#include <array>
#include <unordered_map>

// Local includes
#include "altsound_processor_base.hpp"
#include "snd_alt.h"
#include "..\ext\bass\bass.h"

#define NUM_STREAM_TYPES 5

typedef std::unordered_map<AltsoundSampleType, std::array<bool, NUM_STREAM_TYPES>*> PausedStatusMap;

// ---------------------------------------------------------------------------
// Altsound2Processor class definition
// ---------------------------------------------------------------------------

class Altsound2Processor final : public AltsoundProcessorBase
{
public:

	// Default constructor
	Altsound2Processor() = delete;

	// Copy Constructor
	Altsound2Processor(Altsound2Processor&) = delete;

	// Standard constructor
	Altsound2Processor(const char* gname_in);

	// Destructor
	~Altsound2Processor();

	// External interface to stop MUSIC stream
	bool stopMusic() override;

	// Process ROM commands to the sound board
	bool handleCmd(const unsigned int cmd_in) override;

protected:

private: // functions

	//
	void init() override;

	// parse CSV file and populate sample data
	bool loadSamples() override;

	// find sample matching provided command
	int getSample(const unsigned int cmd_combined_in) override;

	// process stream commands
	bool processStream(const BehaviorInfo& behavior, AltsoundStreamInfo* stream_out);

	// Execute actions that implement the sample type behaviors
	bool processBehaviors(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Process effect of passed behavior structure on MUSIC streams
	bool processMusicBehavior(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Process effect of passed behavior on CALLOUT streams
	bool processCalloutBehavior(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Process effect of passed behavior on SFX streams
	bool processSfxBehavior(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Process effect of passed behavior on SOLO streams
	bool processSoloBehavior(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Process effect of passed behavior on OVERLAY streams
	bool processOverlayBehavior(const BehaviorInfo& behavior, const AltsoundStreamInfo& stream);

	// Stop currently-playing MUSIC stream
	bool stopMusicStream();

	// Stop currently-playing CALLOUT stream
	bool stopCalloutStream();

	// Stop currently-playing SOLO stream
	bool stopSoloStream();

	// BASS SYNCPROC callback whan a stream ends
	static void CALLBACK common_callback(HSYNC handle, DWORD channel, DWORD data, void* user);

	// Update behaviors when streams end
	static void Altsound2Processor::postProcessBehaviors(AltsoundSampleType type);

	// Helper function to check is a stream type still needs to be paused
	static bool isAnyPaused(const std::array<bool, NUM_STREAM_TYPES>& pauseStatusArray);

	// adjust volume of active streams to accommodate ducking
	static bool adjustStreamVolumes();

	// resume paused playback on streams that no longer need to be paused
	static bool processPausedStreams();

	static PausedStatusMap buildPauseStatusMap();

	static bool tryResumeStream(const AltsoundStreamInfo& stream, 
		                        const PausedStatusMap& pauseStatusMap);

	// DEBUG helper fns to print all behavior data
	static void printBehaviorData();
	static void printArray(const std::string& name, const std::array<float, NUM_STREAM_TYPES>& arr);
	static void printArrayBool(const std::string& name, const std::array<bool, NUM_STREAM_TYPES>& arr);

private: // data

	bool is_initialized;
	bool is_stable; // future use
	std::vector<SampleInfo> samples;
};

// ---------------------------------------------------------------------------
// Inline functions
// ---------------------------------------------------------------------------

#endif // ALTSOUND2_PROCESSOR_H