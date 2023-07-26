// ---------------------------------------------------------------------------
// altsound_standalone.cpp
// 07/23/23 - Dave Roscoe
//
// Standalone executive for AltSound for use by devs and authors.  This 
// executable links in all AltSound format processing, ingests sound
// commands from file, and plays them through the same libraries used for
// VPinMAME.  The command file can be generated from live gameplay or 
// created by hand, to test scripted sound playback scenarios.  Authors can
// use this to test mix levels of one or more sounds in any combination to
// finalize the AltSound mix for a table.  This is particularly useful when
// testing modes.  Authors can script the specific sequences by hand, or
// capture the data from live gameplay.  Then the file can be edited to
// include only what is needed.  From there, the author can iterate on the
// specific sounds-under-test, without having to create it repeatedly on the
// table.
//
// Devs can use this tool to isolate problem sounds and run it through a
// debugger as many times as need to find and fix a problem.  If a user finds
// a problem, all they need to do is:
// 1. enable sound command recording
// 2. set logging level to DEBUG
// 3. recreate the problem
// 4. send the problem description, along with the altsound.log and cmdlog.txt
// ---------------------------------------------------------------------------
// license:<TODO>

#if defined(_WIN32)
  #include <direct.h>
  #define CURRENT_DIRECTORY _getcwd
#else
  #include <unistd.h>
  #define CURRENT_DIRECTORY getcwd
#endif

#define NOMINMAX     // prevent conflicts with std::min/max
#include <windows.h> // DAR_TODO what is this for?

// Std Library includes
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
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

//// windows.h conflicts with std::min/max
//#ifdef min
//  #undef min
//#endif
//
//#ifdef max
//  #undef max
//#endif

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Externals
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

//// Structure for command data
//typedef struct _cmd_data {
//  unsigned int cmd_counter;
//  int stored_command;
//  unsigned int cmd_filter;
//  unsigned int cmd_buffer[ALT_MAX_CMDS];
//} CmdData;
//	  
//CmdData cmds;

// Struct to hold a sound command and its associated timing
struct TestData {
	unsigned long msec;
	uint32_t snd_cmd;
};

// Initialization support
bool is_initialized = FALSE;
bool altsound_stable = TRUE;

// Processing instance to specialize command handling
std::unique_ptr<AltsoundProcessorBase> processor;

// Use ROM control commands to control master volume
bool use_rom_ctrl = true;

// Record received sound commands for later playback
bool rec_snd_cmds = false;

// The current game ROM shortname
char g_szGameName[256];

// DAR@20230726
// The entire file is parsed into this structure
//
// Parsed command data for playback
std::vector<TestData> testData;

// Path to VPinMAME.  AltSound processors need this
string vpm_path;

// Root directory for AltSound packages
string altsound_root;

// ---------------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------------

// get path to VPinMAME
std::string get_vpinmame_path();

// Main entrypoint for AltSound handling
bool processCommand(int cmd);

// Function to initialize all altsound variables and data structures
bool init();

// Parse command log file for playback
bool parseCmdFile(const std::string& filename);

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

	if (!dir_exists(vpm_path)) {
		ALT_ERROR(0, "%s is not a valid directory", vpm_path.c_str());
		return 1;
	}

	altsound_root = vpm_path + "/altsound/";

	if (!dir_exists(altsound_root)) {
		ALT_ERROR(0, "%s directory not found", altsound_root.c_str());
		return 1;
	}

	if (!ALT_CALL(parseCmdFile("cmdlog.txt"))) {
		ALT_ERROR(0, "Failed to parse command file.");
		return 1;
	}
	ALT_INFO(1, "SUCCESS parseCmdFile()");
	ALT_INFO(1, "Num commands parsed: %d", testData.size());

	// initialize 
	is_initialized = ALT_CALL(init());
	if (!is_initialized) {
		ALT_ERROR(0, "Initialization failed.");
		return 1;
	}
	altsound_stable = is_initialized;

	// ready for playback
	std::cout << "Press Enter to begin playback..." << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	// start injecting sound commands
	ALT_INFO(1, "Starting playback for \"%s\"...", vpm_path.c_str());
	for (std::vector<TestData>::size_type i = 0; i < testData.size(); ++i) {
		const TestData& td = testData[i];
		if (!ALT_CALL(processCommand(td.snd_cmd))) {
			ALT_ERROR(0, "Command playback failed.  Terminating.");
			break;
		}

		// If this isn't the last command, sleep until the next one.
		if (i < testData.size() - 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(testData[i + 1].msec));
		}
		else {
			// sleep for 5 seconds before exiting
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
	}
	ALT_INFO(1, "Playback finished for \"%s\"...", vpm_path.c_str());
	
	std::cout << "Playback completed! Press Enter to exit..." << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	processor.reset();
	return 0;
}


// ----------------------------------------------------------------------------

bool processCommand(int cmd)
{
	ALT_DEBUG(0, "BEGIN processCommand()");

	if (!is_initialized || !altsound_stable) {
		ALT_WARNING(0, "Altsound unstable. Processing skipped.");

		OUTDENT;
		ALT_DEBUG(0, "END processCommand()");
		return false;
	}

	// Handle the resulting command
	if (!ALT_CALL(processor->handleCmd(cmd))) {
		ALT_WARNING(0, "FAILED processor::handleCmd()");

		OUTDENT;
		ALT_DEBUG(0, "END processCommand()");
		return false;
	}
	ALT_INFO(0, "SUCCESS processor::handleCmd()");
	
	ALT_DEBUG(0, "END processCommand()");
	return true;
}

// ---------------------------------------------------------------------------

bool init()
{
	ALT_DEBUG(0, "BEGIN init()");

	// initialize channel_stream storage
	std::fill(channel_stream.begin(), channel_stream.end(), nullptr);

	// Seed random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// get path to altsound folder for the current game
	std::string altsound_path = altsound_root + g_szGameName;
	ALT_INFO(1, "Path to altsound: %s", altsound_path.c_str());

	// parse .ini file
	AltsoundIniProcessor ini_proc;

	if (!ini_proc.parse_altsound_ini(altsound_path)) {
		ALT_ERROR(0, "Failed to parse_altsound_ini(%s)", altsound_path.c_str());
		
		ALT_DEBUG(0, "END alt_sound_init()");
		return FALSE;
	}

	string format = ini_proc.getAltsoundFormat();
	use_rom_ctrl = ini_proc.usingRomVolumeControl();
	rec_snd_cmds = ini_proc.recordSoundCmds();

	if (format == "g-sound") {
		// G-Sound only supports new CSV format. No need to specify format
		// in the constructor
		processor.reset(new GSoundProcessor(g_szGameName));
	}
//	else if (format == "altsound" || format == "pinsound") {
//		// Traditional altsound processor handles existing CSV and legacy
//		// PinSound format so it must be specified in the constructor
//		processor = new AltsoundProcessor(g_szGameName, format);
//	}
	else {
		ALT_ERROR(0, "Unknown AltSound format: %s", format.c_str());

		ALT_DEBUG(0, "END init()");
		return false;
	}

	if (!processor) {
		ALT_ERROR(0, "FAILED: Unable to create AltSound processor");
		
		ALT_DEBUG(0, "END init()");
		return false;
	}
	ALT_INFO(0, "%s processor created", format.c_str());

	// Initialize BASS
	int DSidx = -1; // BASS default device
	
	if (!BASS_Init(DSidx, 44100, 0, NULL, NULL))
	{
		ALT_ERROR(0, "BASS initialization error: %s", get_bass_err());
	}

	ALT_DEBUG(0, "END init()");
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to get path to VpinMAME
// ---------------------------------------------------------------------------

std::string get_vpinmame_path()
{
	return vpm_path;
}

// ----------------------------------------------------------------------------
// Command file parser
// ----------------------------------------------------------------------------

bool parseCmdFile(const std::string& filename)
{
	ALT_DEBUG(0, "BEGIN parseCmdFile");

	std::ifstream inFile(filename);
	if (!inFile) {
		ALT_ERROR(0, "Unable to open file: %s", filename.c_str());
		ALT_DEBUG(0, "END parseCmdFile");
		return false;
	}

	std::string line;

	// The first line is the game name
	if (std::getline(inFile, line)) {
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string gameName = line.substr(colonPos + 1);
			gameName.erase(std::remove_if(gameName.begin(), gameName.end(), ::isspace), gameName.end()); // Remove whitespaces

			// Ensure no overflow
			if (gameName.length() < sizeof(g_szGameName)) {
				std::strcpy(g_szGameName, gameName.c_str());
			}
			else {
				ALT_ERROR(0, "Game name is too long");
				ALT_DEBUG(0, "END parseCmdFile");
				return false;
			}
		}
	}

	// The rest of the lines are test data
	while (std::getline(inFile, line)) {
		// Ignore blank lines
		if (line.empty()) continue;

		std::istringstream ss(line);

		TestData data;
		std::string temp, command;

		// Parse msec and discard ','
		if (!std::getline(ss, temp, ',')) continue; // Ignore line if no comma
		try {
			data.msec = std::stoul(temp);
		}
		catch (std::exception const& e) {
			ALT_ERROR(0, "Unable to parse time: %s", temp.c_str());
			ALT_DEBUG(0, "END parseCmdFile");
			return false;
		}

		// Parse command as a hexadecimal value
		ss >> std::ws; // skip whitespaces
		if (!std::getline(ss, command, ',')) continue; // Ignore line if no second comma
		if (command.substr(0, 2) == "0x") {
			command = command.substr(2); // remove '0x'
		}
		else {
			ALT_DEBUG(0, "Command value is not in hexadecimal format: %s", command.c_str());
			ALT_DEBUG(0, "END parseCmdFile");
			return false;
		}

		try {
			data.snd_cmd = std::stoul(command, nullptr, 16);
		}
		catch (std::exception const& e) {
			ALT_DEBUG(0, "Unable to parse command: %s", command.c_str());
			ALT_DEBUG(0, "END parseCmdFile");
			return false;
		}

		testData.push_back(data);
	}

	inFile.close();
	ALT_DEBUG(0, "END parseCmdFile");
	return true;
}
