#include "altsound_file_parser.hpp"
#include "altsound_logger.hpp"

extern AltsoundLogger alog;
#if defined(_WIN32)
#include <direct.h>
#define CURRENT_DIRECTORY _getcwd
#else
#include <unistd.h>
#define CURRENT_DIRECTORY getcwd
#endif

//#ifdef __cplusplus
//  extern "C" {
//#endif

#include <dirent.h>
#include <iomanip>

//#ifdef __cplusplus
//  }
//#endif

AltsoundFileParser::AltsoundFileParser(const std::string& altsound_path_in)
	: altsound_path(altsound_path_in)
{
}

// ---------------------------------------------------------------------------

// DAR@20230525
// This support is for PinSound style of alternate sound mixes.
// The files are split into directories according to the following:
//
// <ROM shortname>\
//    music\
//        <instruction1>-name\
//            <soundfile1>.wav(or .ogg)
//            ...
//        <instruction2>-name\
//        ...
//    jingle\
//        <instruction1>-name\
//            <soundfile1>.wav(or .ogg)
//            ...
//        <instruction2>-name\
//        ...
//    sfx\
//        <instruction1>-name\
//            <soundfile1>.wav(or .ogg)
//            ...
//        <instruction2>-name\
//        ...
//    single\
//        <instruction1>-name\
//            <soundfile1>.wav(or .ogg)
//            ...
//        <instruction2>-name\
//        ...
//    voice\
//        <instruction1>-name\
//            <soundfile1>.wav(or .ogg)
//            ...
//        <instruction2>-name\
//        ...
bool AltsoundFileParser::parse(std::vector<AltsoundSampleInfo>& samples_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundFileParser::parse()");
	INDENT;

	int result = 0; // Assume success at start
	int num_files = 0;
	const std::string path_jingle = "jingle/";
	const std::string path_music = "music/";
	const std::string path_sfx = "sfx/";
	const std::string path_single = "single/";
	const std::string path_voice = "voice/";

	for (int i = 0; i < 5; ++i) {
		const std::string subpath = (i == 0) ? path_jingle :
			((i == 1) ? path_music :
			((i == 2) ? path_sfx :
				((i == 3) ? path_single : path_voice)));

		std::string PATH = altsound_path + '/' + subpath;
		ALT_INFO(0, "Search path: %s", PATH.c_str());

		DIR *dir;
		struct dirent *entry;

		// open root directory of the current sound class.  For example, from
		// the comments above:
		// <ROM shortname>\
		    //    music\			<--- sound class: "music"
			//
		dir = opendir(PATH.c_str());
		if (!dir) {
			// Path not found.. try the others
			ALT_INFO(0, "Not a directory: %s", PATH.c_str());
			continue;
		}

		// Loop through the directory entries
		entry = readdir(dir);
		while (entry != NULL) {
			if (entry->d_name[0] != '.'
				&& strstr(entry->d_name, ".txt") == 0
				&& strstr(entry->d_name, ".ini") == 0) {
				// Not a system file or txt file.  Assume it's a directory
				// (per PinSound format requirements).  Backup the current
				// directory stream and entry
				const DIR backup_dir = *dir;
				struct dirent backup_entry = *entry;

				DIR *dir2;
				struct dirent *entry2;

				// Create new path with subdirectory name, preserving the
				// parent directory stream and current entry
				std::string PATH2 = PATH + std::string(entry->d_name);
				ALT_INFO(1, "Search sub-path: %s", PATH2.c_str());

				// Open subfolder of parent sound class directory.  For example
				// from the comments above:
				// <ROM shortname>\
		            //    music\			            <--- sound class: "music"
					//        <instruction1>-name\      <--- ROM instruction ID-<snd_name>
					//
					// DAR@20230525
					// Found an odd problem on Windows.  A hidden file shows up in the
					// dir stream "desktop.ini" which crashes the program.  I believe
					// this is due to the opendir coming back as NULL, which seems like
					// it should be protected in any case
				dir2 = opendir(PATH2.c_str());
				ALT_INFO(1, "Directory stream created: %d", dir2);

				if (!dir2) {
					// Path not found.. try the others
					ALT_INFO(1, "Not a directory: %s", PATH2.c_str());
					entry = NULL;
					continue;
				}

				entry2 = readdir(dir2);
				ALT_INFO(0, "ENTRY: %d\n", entry2);

				while (entry2 != NULL) {
					ALT_INFO(1, "Found file: %s", entry2->d_name);

					if (entry2->d_name[0] != '.'
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0) {
						// not a system or txt file.  This must be a sound file.
						// Increase the number of files being tracked.
						ALT_INFO(1, "Adding file: %s", entry2->d_name);
						num_files++;
					}

					// get next sound file in subdirectory
					entry2 = readdir(dir2);
				}

				// All sound files for this sound class collected.
				// Close the directory stream and free allocated memory
				closedir(dir2);

				// Restore backed up parent folder and entry.
				*dir = backup_dir;
				*entry = backup_entry;
			}

			// get next entry in the parent directory stream
			entry = readdir(dir);
		}

		// All sound class directories processed.  Close directory stream
		closedir(dir);
	}

	if (num_files <= 0) {
		// No filenames parsed.. exit
		ALT_ERROR(0, "No files found!");

		ALT_DEBUG(0, "END AltsoundFileParser::parse()");
		return false;
	}

	// value
	for (int i = 0; i < 5; ++i) {
		float default_gain = .1f;
		float default_ducking = 1.f; //!! default depends on type??

		const std::string subpath = (i == 0) ? path_jingle :
			((i == 1) ? path_music :
			((i == 2) ? path_sfx :
				((i == 3) ? path_single : path_voice)));

		// Set default ducking values.  Can be overridden by ducking.txt file
		if (subpath == path_jingle || subpath == path_single) {
			default_ducking = .1f; // % music volume retained when active
		}
		if (subpath == path_sfx) {
			default_ducking = .8f;
		}
		if (subpath == path_voice) {
			default_ducking = .65f;
		}

		DIR *dir;
		struct dirent *entry;

		std::string PATH = altsound_path + '/' + subpath;
		ALT_INFO(0, "Current_path1: %s", PATH.c_str());

		dir = opendir(PATH.c_str());
		if (!dir)
		{
			// Path not found.. try the others
			ALT_INFO(0, "Path not found: %s", PATH.c_str());
			continue;
		}

		{// parse gain.txt
			FILE *f;

			const std::string PATHG = PATH + "gain.txt";
			ALT_INFO(0, "Current_path2: %s", PATHG.c_str());
			f = fopen(PATHG.c_str(), "r");

			if (f) {
				unsigned int tmp_gain = 0;
				// Set default gain from file
				fscanf(f, "%u", &tmp_gain);
				fclose(f);
				default_gain = tmp_gain > 100 ? 1.0f : (float)tmp_gain / 100.f;
			}
			else {
				// Default gain not found, use default defined above
			}
		}
		{// parse ducking.txt
			FILE *f;

			const std::string PATHG = PATH + "ducking.txt";
			ALT_INFO(0, "Current_path3: %s", PATHG.c_str());
			f = fopen(PATHG.c_str(), "r");

			if (f) {
				// Set default ducking from file
				int tmp_duck = 0;
				fscanf(f, "%d", &tmp_duck);
				fclose(f);
				default_ducking = tmp_duck > 100 ? 1.0f : (float)tmp_duck / 100.f;
			}
			else {
				// Default ducking not found, use default defined above
			}
		}

		// parse individual sound file data
		entry = readdir(dir);
		while (entry != NULL) {
			if (entry->d_name[0] != '.'
				&& strstr(entry->d_name, ".txt") == 0
				&& strstr(entry->d_name, ".ini") == 0)
			{
				// Not a system file or txt file.  Assume it's a directory
				// (per PinSound format requirements).  Backup the current
				// directory stream and entry
				const DIR backup_dir = *dir;
				struct dirent backup_entry = *entry;

				float gain = default_gain;
				float ducking = default_ducking;

				DIR *dir2;
				struct dirent *entry2;

				std::string PATH2 = PATH + entry->d_name;

				{// individual gain
					FILE *f;

					const std::string PATHG = PATH2 + '/' + "gain.txt";
					ALT_INFO(1, "Current_path4: %s", PATHG.c_str());
					f = fopen(PATHG.c_str(), "r");
					if (f)
					{
						unsigned int tmp_gain = 0;
						fscanf(f, "%u", &tmp_gain);
						fclose(f);
						gain = tmp_gain > 100 ? 1.0f : (float)tmp_gain / 100.f;
					}
				}
				{// individual ducking
					FILE *f;

					const std::string PATHG = PATH2 + '/' + "ducking.txt";
					ALT_INFO(1, "Current_path5: %s", PATHG.c_str());
					f = fopen(PATHG.c_str(), "r");
					if (f)
					{
						int tmp_ducking = 0;
						fscanf(f, "%d", &tmp_ducking);
						fclose(f);
						ducking = tmp_ducking > 100 ? 1.0f : (float)tmp_ducking / 100.f;
					}
				}

				ALT_INFO(1, "opendir(%s)", PATH2.c_str());
				dir2 = opendir(PATH2.c_str());
				entry2 = readdir(dir2);
				while (entry2 != NULL) {
					if (entry2->d_name[0] != '.'
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0)
					{
						//LOG(("CURRENT_ENTRY2: %s\n", entry->d_name));
						const char* ptr = strrchr(PATH2.c_str(), '/');
						char id[7] = { 0, 0, 0, 0, 0, 0, 0 };

						AltsoundSampleInfo sample;

						sample.fname = PATH2 + '/' + entry2->d_name;

						memcpy(id, ptr + 1, 6);
						std::string id_str(id);
						sample.id = std::stoul(trim(id), nullptr, 16);

						sample.gain = gain;
						sample.ducking = ducking;

						if (subpath == path_music) {
							sample.channel = 0;
							sample.loop = true;
							sample.stop = false;
						}

						if (subpath == path_jingle || subpath == path_single) {
							sample.channel = 1;
							sample.loop = false;
							sample.stop = (subpath == path_single) ? true : false;
						}

						if (subpath == path_sfx || subpath == path_voice) {
							sample.channel = -1;
							sample.loop = false;
							sample.stop = false;
						}

						samples_out.push_back(sample);

						std::ostringstream debug_stream;
						debug_stream << "ID = 0x" << std::setfill('0') << std::setw(4) << std::hex << sample.id << std::dec
							<< ", CHANNEL = " << sample.channel
							<< ", DUCKING = " << std::fixed << std::setprecision(2) << sample.ducking
							<< ", GAIN = " << std::fixed << std::setprecision(2) << sample.gain
							<< ", LOOP = " << sample.loop
							<< ", FNAME = " << sample.fname;

						ALT_DEBUG(0, debug_stream.str().c_str());
					}
					entry2 = readdir(dir2);
				}
				closedir(dir2);

				*dir = backup_dir;
				*entry = backup_entry;
			}
			entry = readdir(dir);
		}
		closedir(dir);
	}
	ALT_INFO(0, "Found %d samples", num_files);

	OUTDENT;
	ALT_DEBUG(0, "BEGIN AltsoundFileParser::parse()");
	return true;
}
