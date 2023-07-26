// ---------------------------------------------------------------------------
// gsound_processor.cpp
// 06/14/23 - Dave Roscoe
//
// Encapsulates all specialized processing for the G-Sound
// CSV format
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef GSOUND_PROCESSOR_H
#define GSOUND_PROCESSOR_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Library includes
#include <string>
#include <array>
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
	GSoundProcessor(const char* gname_in);

	// Destructor
	~GSoundProcessor();

	// External interface to stop MUSIC stream
	bool stopMusic() override;

	// Process ROM commands to the sound board
	bool handleCmd(const unsigned int cmd_in) override;

	// DEBUG helper fns to print all behavior data
	static void GSoundProcessor::printBehaviorData();

	//// Helper function to print vector elements
	//template<typename T>
	//static void printVector(const std::vector<T>& vec)
	//{
	//	std::stringstream ss;
	//	ss << std::fixed << std::setprecision(2) << "[ ";
	//	for (const auto& element : vec)
	//	{
	//		if (std::is_same<T, bool>::value)
	//		{
	//			ss << (element ? "true" : "false") << " ";
	//		}
	//		else
	//		{
	//			ss << element << " ";
	//		}
	//	}
	//	ss << "]";
	//	ALT_DEBUG(1, ss.str().c_str());
	//}

	//// Helper function to print arrays of vectors
	//template<typename T, size_t N>
	//static void printArray(const std::array<std::vector<T>, N>& arr)
	//{
	//	for (size_t i = 0; i < N; ++i)
	//	{
	//		std::stringstream ss;
	//		ss << "Element " << i << ": ";
	//		printVector(arr[i]);
	//		ss << std::endl;
	//	}
	//}


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

	// adjust volume of active streams to accommodate ducking
	static bool adjustStreamVolumes();

	static float findLowestDuckVolume(AltsoundSampleType stream_type);
	
	// resume paused playback on streams that no longer need to be paused
	static bool processPausedStreams();

	static bool tryResumeStream(const AltsoundStreamInfo& stream);

	void GSoundProcessor::startLogging(const std::string& gameName);

private: // data

	bool is_initialized;
	bool is_stable; // future use
	std::vector<SampleInfo> samples;
};

// ---------------------------------------------------------------------------
// Inline functions
// ---------------------------------------------------------------------------

#endif // GSOUND_PROCESSOR_H