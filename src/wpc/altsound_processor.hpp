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

	// Standard constructor
	AltsoundProcessor(const char* gname_in,
		              const std::string& format_in);

	// Destructor
	~AltsoundProcessor();

	// Process ROM commands to the sound board
	bool handleCmd(const unsigned int cmd_in);

	// Stop currently-playing MUSIC stream
	bool stopMusicStream();

	// Stop currently-playing MUSIC stream
	bool stopJingleStream();

protected:

	// Default constructor
	AltsoundProcessor() {};

	// Copy Constructor
	AltsoundProcessor(AltsoundProcessor&) {};

private: // functions
	
	//
	void init() override;

	// parse CSV file and populate sample data
	bool loadSamples() override;
	
	// find sample matching provided command
	int getSample(const unsigned int cmd_combined_in) override;
	
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
	PinSamples psd;
};

// ---------------------------------------------------------------------------
// Inline functions
// ---------------------------------------------------------------------------

