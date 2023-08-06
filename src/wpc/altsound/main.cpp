// ---------------------------------------------------------------------------
// altsound_standalone.cpp
// 07/23/23 - Dave Roscoe
//
// Standalone executive for AltSound for use by devs and authors.  This 
// executable links in all AltSound format processing, ingests sound
// commands from a file, and plays them through the same libraries used for
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
// Devs can use this tool to isolate problems and run it through a
// debugger as many times as need to find and fix a problem.  If a user finds
// a problem, all they need to do is:
// 1. enable sound command recording
// 2. set logging level to DEBUG
// 3. recreate the problem
// 4. send the problem description, along with the altsound.log and cmdlog.txt
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------

#define NOMINMAX     // prevent conflicts with std::min/max

#if defined(_WIN32)
  #include <direct.h>
  #define CURRENT_DIRECTORY _getcwd
#else
  #include <unistd.h>
  #define CURRENT_DIRECTORY getcwd
#endif

#include "altsound_processor_base.hpp"
#include "altsound_processor.hpp"
#include "gsound_processor.hpp"
#include "altsound_file_parser.hpp"
#include "altsound_ini_processor.hpp"
#include "inipp.h"
#include "altsound_data.hpp"
#include "altsound_logger.hpp"

// namespace scope resolution
using std::string;
using std::endl;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Logger instance
AltsoundLogger alog("altsound.log");

// Data structures to hold sample behaviors
BehaviorInfo music_behavior;
BehaviorInfo callout_behavior;
BehaviorInfo sfx_behavior;
BehaviorInfo solo_behavior;
BehaviorInfo overlay_behavior;

// Instance of global thread synchronization mutex
std::mutex io_mutex;

// Instance of global array of BASS channels 
StreamArray channel_stream;

//float master_vol = 1.0f;
//float global_vol = 1.0f;

// Structure to hold a sound command and its associated timing
struct TestData {
	unsigned long msec;
	uint32_t snd_cmd;
};

struct InitData {
	std::string log_path;
	std::vector<TestData> test_data;
	std::string vpm_path;
	std::string altsound_path;
};

// Processing instance to specialize command handling
std::unique_ptr<AltsoundProcessorBase> processor;

// The current game ROM shortname
char g_szGameName[256];

// ---------------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------------

// Main entrypoint for AltSound handling
bool processCommand(int cmd);

// Function to initialize all altsound variables and data structures
std::pair<bool, InitData> init(const std::string& log_path);

// Parse command log file for playback
bool parseCmdFile(InitData& init_data);

// Handle playback of stored sound commands
bool playbackCommands(const std::vector<TestData>& testData);

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <gamename>-cmdlog.txt path" << std::endl;
		std::cout << "Where <gamename>-cmdlog.txt path is the full path and "
			      << "filename of recording file" << std::endl;
		return 1;
	}

	auto init_result = init(argv[1]);

	if (!init_result.first) {
		ALT_ERROR(0, "Initialization failed.");
		return 1;
	}
	
	std::cout << "Press Enter to begin playback..." << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	try {
		ALT_INFO(1, "Starting playback for \"%s\"...", init_result.second.altsound_path.c_str());
		if (!playbackCommands(init_result.second.test_data)) {
			ALT_ERROR(0, "Playback failed");
			return 1;
		}
		ALT_INFO(1, "Playback finished for \"%s\"...", init_result.second.altsound_path.c_str());
	}
	catch (const std::exception& e) {
		ALT_ERROR(0, "Unexpected error during playback: %s", e.what());
		return 1;
	}

	std::cout << "Playback completed! Press Enter to exit..." << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	processor.reset();
	return 0;
}

// ----------------------------------------------------------------------------

bool playbackCommands(const std::vector<TestData>& test_data)
{
	for (size_t i = 0; i < test_data.size(); ++i) {
		const TestData& td = test_data[i];
		if (!ALT_CALL(processCommand(td.snd_cmd))) {
			ALT_WARNING(0, "Command playback failed");
		}

		// Sleep for the duration specified in msec for each command, except for the last command.
		if (i < test_data.size() - 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(td.msec));
		}
		else {
			// Sleep for 5 seconds before exiting
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
	}
	return true;
}

// ----------------------------------------------------------------------------

bool processCommand(int cmd)
{
	ALT_DEBUG(0, "BEGIN processCommand()");

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

std::pair<bool, InitData> init(const std::string& log_path)
{
	ALT_DEBUG(0, "BEGIN init()");

	InitData init_data;
	init_data.log_path = log_path;

	try {
		if (!ALT_CALL(parseCmdFile(init_data))) {
			throw std::runtime_error("Failed to parse command file.");
		}
		ALT_INFO(1, "SUCCESS parseCmdFile()");
		ALT_INFO(1, "Num commands parsed: %d", init_data.test_data.size());

		// initialize channel_stream storage
		std::fill(channel_stream.begin(), channel_stream.end(), nullptr);

		// parse .ini file
		AltsoundIniProcessor ini_proc;
		if (!ini_proc.parse_altsound_ini(init_data.altsound_path)) {
			// Error message and return
			ALT_ERROR(0, "Failed to parse_altsound_ini(%s)", init_data.altsound_path.c_str());
			return std::make_pair(false, InitData{});
		}

		string format = ini_proc.getAltsoundFormat();
		std::string game_name(g_szGameName);

		if (format == "g-sound") {
			// G-Sound only supports new CSV format. No need to specify format
			// in the constructor
			processor.reset(new GSoundProcessor(game_name, init_data.vpm_path));
		}
		else if (format == "altsound" || format == "legacy") {
			processor.reset(new AltsoundProcessor(game_name, init_data.vpm_path, format));
		}
		else {
			throw std::runtime_error("Unknown AltSound format: " + format);
		}

		if (!processor) {
			throw std::runtime_error("FAILED: Unable to create AltSound processor");
		}
		ALT_INFO(0, "%s processor created", format.c_str());

		// set ROM volume control preference
		processor->romControlsVol(ini_proc.usingRomVolumeControl());
		
		// set sound command recording preference
		processor->recordSoundCmds(ini_proc.recordSoundCmds());

		processor->init();

		// Initialize BASS
		int DSidx = -1; // BASS default device

		if (!BASS_Init(DSidx, 44100, 0, NULL, NULL))
		{
			ALT_ERROR(0, "BASS initialization error: %s", get_bass_err());
		}

		ALT_DEBUG(0, "END init()");
		return std::make_pair(true, init_data);
	}
	catch (const std::runtime_error& e) {
		ALT_ERROR(0, e.what());
		ALT_DEBUG(0, "END init()");
		return std::make_pair(false, InitData{});
	}
}

// ----------------------------------------------------------------------------
// Command file parser
// ----------------------------------------------------------------------------

bool parseCmdFile(InitData& init_data)
{
	ALT_DEBUG(0, "BEGIN parseCmdFile");

	try {
		std::ifstream inFile(init_data.log_path);
		if (!inFile.is_open()) {
			throw std::runtime_error("Unable to open file: " + init_data.log_path);
		}

		std::string line;

		// The first line is the game name
		if (std::getline(inFile, line)) {
			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string fullPath = line.substr(colonPos + 1);
				fullPath.erase(std::remove_if(fullPath.begin(), fullPath.end(), ::isspace),
					fullPath.end());

				// Normalize slashes
				std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

				// capture path to the altsound
				init_data.altsound_path = fullPath;
				ALT_INFO(1, "Path to altsound: %s", init_data.altsound_path.c_str());

				size_t lastSlashPos = fullPath.rfind('/');
				if (lastSlashPos != std::string::npos) {
					std::string gameName = fullPath.substr(lastSlashPos + 1);
					if (gameName.length() >= sizeof(g_szGameName)) {
						throw std::runtime_error("Game name is too long. Maximum length is " + std::to_string(sizeof(g_szGameName) - 1));
					}

					std::strcpy(g_szGameName, gameName.c_str());

					// capture path to VPinMAME
					init_data.vpm_path = fullPath.substr(0, fullPath.find("/altsound/"));
				}
			}
		}

		// The rest of the lines are test data
		while (std::getline(inFile, line)) {
			if (line.empty()) continue;

			std::istringstream ss(line);

			TestData data;
			std::string temp, command;

			if (!std::getline(ss, temp, ',')) continue;
			char* end;
			data.msec = std::strtoul(temp.c_str(), &end, 10);
			if (end == temp.c_str()) {
				throw std::runtime_error("Unable to parse time: " + temp);
			}

			const std::string HEX_PREFIX = "0x";
			ss >> std::ws;
			if (!std::getline(ss, command, ',')) continue;
			if (command.substr(0, HEX_PREFIX.length()) == HEX_PREFIX) {
				command = command.substr(HEX_PREFIX.length());
			}
			else {
				throw std::runtime_error("Command value is not in hexadecimal format: " + command);
			}

			data.snd_cmd = std::strtoul(command.c_str(), &end, 16);
			if (end == command.c_str()) {
				throw std::runtime_error("Unable to parse command: " + command);
			}

			init_data.test_data.push_back(data);
		}

		inFile.close();
		ALT_DEBUG(0, "END parseCmdFile");
		return true;
	}
	catch (const std::runtime_error& e) {
		ALT_ERROR(0, e.what());
		ALT_DEBUG(0, "END parseCmdFile");
		return false;
	}
}

