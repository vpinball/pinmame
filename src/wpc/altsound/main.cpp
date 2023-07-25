// license:BSD-3-Clause
// copyright-holders:Carsten Wächter

//#include "snd_alt.h"

#if defined(_WIN32)
  #include <direct.h>
  #define CURRENT_DIRECTORY _getcwd
#else
  #include <unistd.h>
  #define CURRENT_DIRECTORY getcwd
#endif
#include <windows.h>

// Std Library includes
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <time.h>
#include <vector>

#include "altsound_processor_base.hpp"
//#include "altsound_processor.hpp"
#include "gsound_processor.hpp"
//#include "altsound_file_parser.hpp"
#include "altsound_ini_processor.hpp"
#include "inipp.h"
#include "altsound_data.hpp"
#include "altsound_logger.hpp"

// namespace scope resolution
using std::string;
using std::endl;

AltsoundLogger alog("altsound.log");

// Data structure to hold sample behaviors
BehaviorInfo music_behavior;
BehaviorInfo callout_behavior;
BehaviorInfo sfx_behavior;
BehaviorInfo solo_behavior;
BehaviorInfo overlay_behavior;

// Instance of global thread synchronization mutex
std::mutex io_mutex;

// Instance of global array of BASS channels 
StreamArray channel_stream;

float master_vol = 1.0f;
float global_vol = 1.0f;

// windef.h "min" conflicts with std::min
#ifdef min
  #undef min
#endif

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define FILTERED_INCOMPLETE 999
#define ALT_MAX_CMDS 4

// ---------------------------------------------------------------------------
// Externals
// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Command storage and workspace
// Structure for command data
typedef struct _cmd_data {
  unsigned int cmd_counter;
  int stored_command;
  unsigned int cmd_filter;
  unsigned int cmd_buffer[ALT_MAX_CMDS];
} CmdData;
	  
CmdData cmds;

// Struct to hold a command and its associated timing
struct TestData {
	unsigned long msec;
	uint32_t snd_cmd;
};

// Initialization support
bool is_initialized = FALSE;
bool altsound_stable = TRUE;

// Processing instance to specialize command handling
AltsoundProcessorBase *processor = NULL;

// Use ROM control commands to control master volume
bool use_rom_ctrl = true;

// Record received sound commands for later playback
bool rec_snd_cmds = false;

char g_szGameName[256];
std::vector<TestData> testData;
string vpm_path;

// ---------------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------------

// get path to VPinMAME
std::string get_vpinmame_path();

#ifdef __cplusplus
  extern "C" {
#endif
  // Main entrypoint for AltSound handling
  void alt_sound_handle(int boardNo, int cmd);
  
  // Pause/Resume all streams
  void alt_sound_pause(BOOL pause);
  
  // Exit AltSound processing and clean up
  void alt_sound_exit();
#ifdef __cplusplus
  }
#endif

// Function to initialize all altound variables and data structures
BOOL alt_sound_init(CmdData* cmds_out);

void parseFile(const char* filename);

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <VPinMAME_path>" << std::endl;
		return 1;
	}

	// save path to VPinMAME
	const char* vpinmame_path = argv[1];
	vpm_path = string(vpinmame_path);

	std::string dll_path = vpm_path + "\\VpinMAME.dll";

	std::cout << "VPinMAME path: " << vpm_path << std::endl;

	HMODULE hDll = LoadLibrary(dll_path.c_str());
	if (hDll == NULL) {
		std::cout << "Failed to load DLL: " << dll_path << std::endl;
		return 1;
	}

	// parse commandfile
	parseFile("cmdlog.txt");

	std::cout << "Num commands parsed: " << testData.size() << std::endl;
	std::cout << "Parsed sound commands. Press any key to begin playback..." << std::endl;
	std::cout.flush(); // Flush the output stream
	getchar(); // Wait for user input

	// start injecting sound commands
	for (std::vector<TestData>::size_type i = 0; i < testData.size(); ++i) {
		const TestData& td = testData[i];
		alt_sound_handle(0, td.snd_cmd);

		// If this isn't the last command, sleep until the next one.
		if (i < testData.size() - 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(testData[i + 1].msec));
		}
	}

	std::cout << "Playback completed!  Press any key to exit..." << std::endl;
	getchar(); // Wait for user input

	return 0;
}

extern "C" void alt_sound_handle(int boardNo, int cmd)
{
	ALT_DEBUG(0, "");
	ALT_DEBUG(0, "BEGIN alt_sound_handle()");
	INDENT;

	if (!is_initialized && altsound_stable) {
		is_initialized = alt_sound_init(&cmds);
		altsound_stable = is_initialized;
	}

	if (!is_initialized || !altsound_stable) {
		ALT_ERROR(0, "Altsound unstable. Processing skipped.");

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_handle()");
		return;
	}

	// Handle the resulting command
	if (!ALT_CALL(processor->handleCmd(cmd))) {
		ALT_WARNING(0, "FAILED processor::handleCmd()");

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_handle()");
		return;
	}
	ALT_INFO(0, "SUCCESS processor::handleCmd()");

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_handle()");
	ALT_DEBUG(0, "");
}

// ---------------------------------------------------------------------------

BOOL alt_sound_init(CmdData* cmds_out)
{
	ALT_DEBUG(0, "BEGIN alt_sound_init()");
	INDENT;

	// DAR@20230616
	// This shouldn't happen, so if it does, log it
	if (processor) {
		ALT_ERROR(0, "Processor already defined");
		delete processor;
		processor = NULL;
	}

	// initialize channel_stream storage
	std::fill(channel_stream.begin(), channel_stream.end(), nullptr);

	// Seed random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// get path to altsound folder for the current game
	string altsound_path = get_vpinmame_path();
	if (!altsound_path.empty()) {
		altsound_path += "\\altsound\\";
		altsound_path += g_szGameName;
		ALT_INFO(0, "Path to altsound: %s", altsound_path.c_str());
	}
	else {
		ALT_ERROR(0, "VPinMAME not found");

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return false;
	}

	// parse .ini file
	AltsoundIniProcessor ini_proc;
	string format;
	bool rom_ctrl = true;
	bool rec_cmds = false;

	if (!ini_proc.parse_altsound_ini(altsound_path, format, rec_cmds, rom_ctrl)) {
		ALT_ERROR(0, "Failed to parse_altsound_ini(%s)", altsound_path.c_str());
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}

	if (format == "g-sound") {
		// G-Sound only supports new CSV format. No need to specify format
		// in the constructor
		processor = new GSoundProcessor(g_szGameName);
	}
//	else if (format == "altsound" || format == "pinsound") {
//		// Traditional altsound processor handles existing CSV and legacy
//		// PinSound format so it must be specified in the constructor
//		processor = new AltsoundProcessor(g_szGameName, format);
//	}
	else {
		ALT_ERROR(0, "Unknown AltSound format: %s", format.c_str());

		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}

	if (!processor) {
		ALT_ERROR(0, "FAILED: Unable to create AltSound processor");
		
		OUTDENT;
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}
	ALT_INFO(0, "%s processor created", format.c_str());

	// update global variables with parsed data
	global_vol = 1.0f;
	master_vol = 1.0f;
	use_rom_ctrl = rom_ctrl;
	rec_snd_cmds = rec_cmds;

	// intialize the command bookkeeping structure
	cmds_out->cmd_counter = 0;
	cmds_out->stored_command = -1;
	cmds_out->cmd_filter = 0;
	std::fill_n(cmds_out->cmd_buffer, ALT_MAX_CMDS, ~0);

	int DSidx = -1; // BASS default device
	//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
	if (DSidx != -1)
		DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
	if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
	{
		ALT_ERROR(0, "BASS initialization error: %s", get_bass_err());
		//sprintf_s(bla, "BASS music/sound library initialization error %d", BASS_ErrorGetCode());
	}

	// disable the global mixer to mute ROM sounds in favor of altsound
	//mixer_sound_enable_global_w(0);

	//DAR@20230520
	// This code does not appear to be necessary. The call above which sets the
	// global mixer status seems to do the trick. If it's really
	// required for WPC89 generation, there should be an explicit check for it and
	// this code should be executed in that case only
	//	int ch;
	//	// force internal PinMAME volume mixer to 0 to mute emulated sounds & musics
	//	// required for WPC89 sound board
	//	LOG(("MUTING INTERNAL PINMAME MIXER"));
	//	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++) {
	//		const char* mixer_name = mixer_get_name(ch);
	//		if (mixer_name != NULL)	{
	//			mixer_set_volume(ch, 0);
	//		}
	//		else {
	//			LOG(("MIXER_NAME (Channel %d) IS NULL", ch));
	//		}
	//	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_init()");
	return TRUE;
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_exit() {
	//DAR_TODO clean up internal storage?
	ALT_DEBUG(0, "BEGIN alt_sound_exit()");
	INDENT;

	// reset static variables
	static CmdData cmds;

	// Initialization support
	is_initialized = FALSE;
	altsound_stable = TRUE;
	use_rom_ctrl = true;
	master_vol = 1.0f;
	global_vol = 1.0f;

	// clean up processor
	delete processor;
	processor = nullptr;

	// DAR@20230618
	// This needs to happen AFTER the processor is deleted.  BASS_Free() cleans
	// up resources that are still held by the processor.  If this is called
	// first, it will cause an error to be logged when shutting down
	if (BASS_Free() == FALSE) {
		ALT_ERROR(0, "FAILED BASS_Free(): %s", get_bass_err());
	}
	else {
		ALT_INFO(0, "SUCCESS BASS_Free()");
	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_exit()");
}

// ---------------------------------------------------------------------------

extern "C" void alt_sound_pause(BOOL pause) {
	ALT_DEBUG(0, "BEGIN alt_sound_pause()");
	INDENT;

	if (pause) {
		ALT_INFO(0, "Pausing stream playback (ALL)");

		// Pause all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING) {
				if (!BASS_ChannelPause(stream)) {
					ALT_WARNING(0, "FAILED BASS_ChannelPause(%u): %s", stream, get_bass_err());
				}
				else {
					ALT_INFO(0, "SUCCESS BASS_ChannelPause(%u)", stream);
				}
			}
		}
	}
	else {
		ALT_INFO(0, "Resuming stream playback (ALL)");

		// Resume all channels
		for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (!channel_stream[i])
				continue;

			HSTREAM stream = channel_stream[i]->hstream;
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PAUSED) {
				if (!BASS_ChannelPlay(stream, 0)) {
					ALT_WARNING(0, "FAILED BASS_ChannelPlay(%u): %s", stream, get_bass_err());
				}
				else {
					ALT_INFO(0, "SUCCESS BASS_ChannelPlay(%u)", stream);
				}
			}
		}
	}

	OUTDENT;
	ALT_DEBUG(0, "END alt_sound_pause()");
}

// ---------------------------------------------------------------------------
// Helper function to get path to VpinMAME
// ---------------------------------------------------------------------------

std::string get_vpinmame_path()
{
	return vpm_path;
}

void parseFile(const char* filename) {
	std::ifstream inFile(filename);
	if (!inFile) {
		std::cerr << "Unable to open file " << filename << std::endl;
		exit(1); // or handle the error in a manner you prefer
	}

	std::string line;

	// The first line is the game name
	if (std::getline(inFile, line)) {
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			strncpy(g_szGameName, line.substr(colonPos + 1).c_str(), sizeof(g_szGameName) - 1);
			g_szGameName[sizeof(g_szGameName) - 1] = '\0'; // Null-terminate in case of overflow
		}
	}

	// The rest of the lines are test data
	while (std::getline(inFile, line)) {
		std::istringstream ss(line);

		TestData data;
		std::string temp, command;

		// Parse msec and discard ','
		std::getline(ss, temp, ',');
		data.msec = std::stoul(temp);

		// Parse command as a hexadecimal value
		ss >> std::ws; // skip whitespaces
		std::getline(ss, command, ',');
		command = command.substr(2); // remove '0x'

		data.snd_cmd = std::stoul(command, nullptr, 16);
		testData.push_back(data);
	}

	inFile.close();
}
