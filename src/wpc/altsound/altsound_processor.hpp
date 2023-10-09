// ---------------------------------------------------------------------------
// altsound_processor.hpp
//
// Encapsulates all specialized processing for the legacy Altsound
// CSV and PinSound format
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe, Carsten Wächter
// ---------------------------------------------------------------------------

#ifndef ALTSOUND_PROCESSOR_H
#define ALTSOUND_PROCESSOR_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

// Library includes
#include <string>
#include <array>

// Local includes
#include "altsound_processor_base.hpp"
#include "snd_alt.h"
#include "..\ext\bass\bass.h"

// ---------------------------------------------------------------------------
// AltsoundProcessor class definition
// ---------------------------------------------------------------------------

class AltsoundProcessor final : public AltsoundProcessorBase
{
public:

	// Default constructor
	AltsoundProcessor() = delete;

	// Copy Constructor
	AltsoundProcessor(AltsoundProcessor&) = delete;

	// Standard constructor
	AltsoundProcessor(const std::string& game_name_in,
		              const std::string& vpm_path_in,
		              const std::string& format_in);

	// Destructor
	~AltsoundProcessor();

	// Process ROM commands to the sound board
	bool handleCmd(const unsigned int cmd_in) override;

	// External interface to stop currently-playing MUSIC stream
	bool stopMusic() override;

protected:

private: // functions
	
	//
	void init() override;

	// parse CSV file and populate sample data
	bool loadSamples() override;
	
	// find sample matching provided command
	unsigned int getSample(const unsigned int cmd_combined_in) override;
	
	// 
	bool stopMusicStream();

	// Stop currently-playing MUSIC stream
	bool stopJingleStream();

	// get lowest ducking value of all active streams
	static float getMinDucking();

	// process music commands
	bool process_music(AltsoundStreamInfo* stream_out);
	
	// process jingle commands
	bool process_jingle(AltsoundStreamInfo* stream_out);
	
	// process sfx commands
	bool process_sfx(AltsoundStreamInfo* stream_out);

	// BASS SYNCPROC callback when jingle samples end
	static void CALLBACK jingle_callback(HSYNC handle, DWORD channel, DWORD data, void *user);
	
	// BASS SYNCPROC callback when sfx samples end
	static void CALLBACK sfx_callback(HSYNC handle, DWORD channel, DWORD data, void *user);
	
	// BASS SYNCPROC callback when music samples end
	static void CALLBACK music_callback(HSYNC handle, DWORD channel, DWORD data, void *user);

private: // data
	
	std::string format;
	bool is_initialized;
	bool is_stable; // future use
	CmdData cmds;
	std::vector<AltsoundSampleInfo> samples;
};

// ---------------------------------------------------------------------------
// Inline functions
// ---------------------------------------------------------------------------

#endif // ALTSOUND_PROCESSOR_H
