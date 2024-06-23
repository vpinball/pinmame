// ---------------------------------------------------------------------------
// altsound_processor_base.hpp
//
// Base class implementation for AltsoundProcessor, which encapsulates
// command execution, and specialization in support of all AltSound formats
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#ifndef ALTSOUND_PROCESSOR_BASE_HPP
#define ALTSOUND_PROCESSOR_BASE_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

// Library includes
#include <mutex>

// Local includes
#include "altsound_data.hpp"
#include "../../ext/bass/bass.h"

// ---------------------------------------------------------------------------
// AltsoundProcessorBase class definition
// ---------------------------------------------------------------------------

class AltsoundProcessorBase {
public:

	// Default constructor
	AltsoundProcessorBase() = delete;

	// Copy constructor
	AltsoundProcessorBase(AltsoundProcessorBase&) = delete;

	// Standard constructor
	AltsoundProcessorBase(const std::string& game_name, const std::string& vpm_path);

	// Destructor
	virtual ~AltsoundProcessorBase();

	// Process ROM commands to the sound board
	virtual bool handleCmd(const unsigned int cmd_in);

	// external interface to stop playback of the current music stream
	virtual bool stopMusic() = 0;

	// ROM volume control accessor/mutator
	void romControlsVol(const bool use_rom_vol);
	bool romControlsVol();

	// command recording flag mutator
	void recordSoundCmds(const bool rec_sound_cmds);

	// initialize processing state
	virtual void init();

	// master volume accessor/mutator
	void setMasterVol(const float vol_in);
	static float getMasterVol();

	// global accessor/mutator
	void setGlobalVol(const float vol_in);
	static float getGlobalVol();

	// command skip count accessor/mutator
	void setSkipCount(const unsigned int skip_count_in);
	unsigned int getSkipCount() const;

public: // data

protected: // functions

	// populate sample data
	virtual bool loadSamples() = 0;

	// find sample matching provided command
	virtual unsigned int getSample(const unsigned int cmd_combined_in) = 0;

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

	// get volume on provided stream, -FLT_MAX on error
	static float getStreamVolume(HSTREAM stream_in);

	// Return ROM shortname
	const std::string& getGameName();

	// Return path to VPinMAME
	const std::string& getVpmPath();

	// Initialize log file
	bool startLogging(const std::string& gameName);

protected: // data
	
	std::string game_name;
	std::string vpm_path;

private: // functions

private: // data

	bool rec_snd_cmds = false;
	bool use_rom_ctrl = true;
	static float global_vol;
	static float master_vol;
	unsigned int skip_count;
};

// ----------------------------------------------------------------------------
// Inline functions
// ----------------------------------------------------------------------------

inline const std::string& AltsoundProcessorBase::getGameName() {
	return game_name;
}

// ----------------------------------------------------------------------------

inline const std::string& AltsoundProcessorBase::getVpmPath() {
	return vpm_path;
}

// ----------------------------------------------------------------------------

inline void AltsoundProcessorBase::romControlsVol(const bool use_rom_vol) {
	use_rom_ctrl = use_rom_vol;
}

// ----------------------------------------------------------------------------

inline bool AltsoundProcessorBase::romControlsVol() {
	return use_rom_ctrl;
}

// ----------------------------------------------------------------------------

inline void AltsoundProcessorBase::recordSoundCmds(const bool rec_sound_cmds) {
	rec_snd_cmds = rec_sound_cmds;
}

// ----------------------------------------------------------------------------

inline void AltsoundProcessorBase::setMasterVol(const float vol_in) {
	master_vol = vol_in;
}

// ----------------------------------------------------------------------------

inline float AltsoundProcessorBase::getMasterVol() {
	return master_vol;
}

// ----------------------------------------------------------------------------

inline float AltsoundProcessorBase::getGlobalVol() {
	return global_vol;
}

// ----------------------------------------------------------------------------

inline unsigned int AltsoundProcessorBase::getSkipCount() const {
	return skip_count;
}

// ----------------------------------------------------------------------------

inline void AltsoundProcessorBase::setSkipCount(const unsigned int skip_count_in) {
	skip_count = skip_count_in;
}

#endif // ALTSOUND_PROCESSOR_BASE_HPP
