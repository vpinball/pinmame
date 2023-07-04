// ---------------------------------------------------------------------------
// altsound_processor_base.hpp
// 06/23/23 - Dave Roscoe
//
// Base class implementation for AltsoundProcessor, which encapsulates
// command execution, and specialization in support of all AltSound formats
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND_PROCESSOR_BASE_HPP
#define ALTSOUND_PROCESSOR_BASE_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Library includes
#include <mutex>

// Local includes
#include "altsound_data.hpp"
#include "bass.h"

// ---------------------------------------------------------------------------
// AltsoundProcessorBase class definition
// ---------------------------------------------------------------------------

class AltsoundProcessorBase {
public:
	
	// Default constructor
	AltsoundProcessorBase() = delete;

	// Copy constructor
	AltsoundProcessorBase(AltsoundProcessorBase&) = delete;

	// Standard Constructor
	AltsoundProcessorBase(const std::string& game_name_in);

	// Destructor
	~AltsoundProcessorBase();

	// Process ROM commands to the sound board
	virtual bool handleCmd(const unsigned int cmd_in) = 0;

	// external interface to stop playback of the current music stream
	virtual bool stopMusic() = 0;

public: // data
	
protected: // functions

	// populate sample data
	virtual bool loadSamples() = 0;

	// initialize processing state
	virtual void init() = 0;

	// find sample matching provided command
	virtual int getSample(const unsigned int cmd_combined_in) = 0;

	// Create stream for BASS playback
	bool createStream(void* syncproc_in, AltsoundStreamInfo* stream_out);

	// get short path of current game <gamename>/subpath/filename
	std::string getShortPath(const std::string& path_in);

	// stop playback on all active streams
	bool stopAllStreams();

	// stop playback of provided stream handle
	static bool stopStream(HSTREAM hstream_in);

	// free BASS resources of provided stream handle
	static bool freeStream(const HSTREAM stream_in);

	// find available sound channel for sample playback
	static bool findFreeChannel(unsigned int& channel_out);

	// set volume on provided stream
	static bool setStreamVolume(HSTREAM stream_in, const float vol_in);

protected: // data
	
	std::string game_name;

private: // functions
private: // data
};

#endif // ALTSOUND_PROCESSOR_BASE_HPP