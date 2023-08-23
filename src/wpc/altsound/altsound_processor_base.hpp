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

	// Standard Constructor
	AltsoundProcessorBase(const std::string& game_name, const std::string& vpm_path);

	// Destructor
	virtual ~AltsoundProcessorBase();

	// Process ROM commands to the sound board
	virtual bool handleCmd(const unsigned int cmd_in);

	// external interface to stop playback of the current music stream
	virtual bool stopMusic() = 0;

	void romControlsVol(const bool use_rom_vol);
	bool romControlsVol();

	void recordSoundCmds(const bool rec_sound_cmds);

	// initialize processing state
	virtual void init();

	void setMasterVol(const float vol_in);
	static float getMasterVol();

	void setGlobalVol(const float vol_in);
	static float getGlobalVol();

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

	const std::string& getGameName();

	const std::string& getVpmPath();

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
};

// ----------------------------------------------------------------------------
// Inline functions
// ----------------------------------------------------------------------------

inline const std::string& AltsoundProcessorBase::getGameName() {
	return game_name;
}

inline const std::string& AltsoundProcessorBase::getVpmPath() {
	return vpm_path;
}

inline void AltsoundProcessorBase::romControlsVol(const bool use_rom_vol) {
	use_rom_ctrl = use_rom_vol;
}

inline bool AltsoundProcessorBase::romControlsVol() {
	return use_rom_ctrl;
}

inline void AltsoundProcessorBase::recordSoundCmds(const bool rec_sound_cmds) {
	rec_snd_cmds = rec_sound_cmds;
}

inline void AltsoundProcessorBase::setMasterVol(const float vol_in) {
	master_vol = vol_in;
}

inline float AltsoundProcessorBase::getMasterVol() {
	return master_vol;
}

inline void AltsoundProcessorBase::setGlobalVol(const float vol_in) {
	global_vol = vol_in;
}

inline float AltsoundProcessorBase::getGlobalVol() {
	return global_vol;
}

// ----------------------------------------------------------------------------

#endif // ALTSOUND_PROCESSOR_BASE_HPP