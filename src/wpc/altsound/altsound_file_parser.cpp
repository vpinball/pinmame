#include "altsound_file_parser.hpp"

#ifdef __cplusplus
  extern "C" {
#endif

#include <dirent.h>

#ifdef __cplusplus
  }
#endif

AltsoundFileParser::AltsoundFileParser(const char *gname_in)
: g_szGameName(gname_in)
{
	char* lpHelp = cvpmd;
	char* lpSlash = NULL;
	size_t BASE_PATH_LEN;
	int result = 0; // Assume success at start

	// Get path to VPinMAME
	HINSTANCE hInst;
#ifndef _WIN64
	hInst = GetModuleHandle("VPinMAME.dll");
#else
	hInst = GetModuleHandle("VPinMAME64.dll");
#endif
	GetModuleFileName(hInst, cvpmd, sizeof(cvpmd));

	// Get rid of "VPinMAME.dll" from path name
	while (*lpHelp) {
		if (*lpHelp == '\\')
			lpSlash = lpHelp;
		lpHelp++;
	}
	if (lpSlash)
		*lpSlash = '\0';

	// Determine base path length
	base_path_length = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1;
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
bool AltsoundFileParser::parse(PinSamples* psd)
{
	DIR* dir;
	char* PATH;
	char cwd[1024];
	_getcwd(cwd, sizeof(cwd));
	int result = 0; // Assume success at start
	const char* path_jingle = "jingle\\";
	const char* path_music = "music\\";
	const char* path_sfx = "sfx\\";
	const char* path_single = "single\\";
	const char* path_voice = "voice\\";

	for (int i = 0; i < 5; ++i) {
		const char* const subpath = (i == 0) ? path_jingle :
			((i == 1) ? path_music :
			((i == 2) ? path_sfx :
				((i == 3) ? path_single : path_voice)));

		// Determine full path size and allocate memory
		const size_t PATHl = base_path_length + strlen(subpath) + 1;
		PATH = (char*)malloc(PATHl);
		strcpy_s(PATH, PATHl, cvpmd);
		strcat_s(PATH, PATHl, path_main);
		strcat_s(PATH, PATHl, g_szGameName);
		strcat_s(PATH, PATHl, "\\");
		strcat_s(PATH, PATHl, subpath);
		LOG(("SEARCH PATH: %s\n", PATH));

		DIR *dir;
		struct dirent *entry;

		// open root directory of the current sound class.  For example, from
		// the comments above:
		// <ROM shortname>\
		    //    music\			<--- sound class: "music"
			//
		dir = opendir(PATH);
		if (!dir) {
			// Path not found.. try the others
			LOG(("Not a directory: %s\n", PATH));
			free(PATH);
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
				const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
				char* const PATH2 = (char*)malloc(PATH2l);
				strcpy_s(PATH2, PATH2l, PATH);
				strcat_s(PATH2, PATH2l, entry->d_name);
				LOG(("SEARCH SUB-PATH: %s\n", PATH2));

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
				dir2 = opendir(PATH2);
				LOG(("DIRECTORY STREAM CREATED: %d\n", dir2));
				if (!dir2) {
					// Path not found.. try the others
					LOG(("Not a directory: %s\n", PATH2));
					free(PATH2);
					entry = NULL;
					continue;
				}

				entry2 = readdir(dir2);
				LOG(("ENTRY: %d\n", entry2));

				while (entry2 != NULL) {
					LOG(("FOUND FILE: %s\n", entry2->d_name));
					if (entry2->d_name[0] != '.'
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0)
						// not a system or txt file.  This must be a sound file.
						// Increase the number of files being tracked.
						LOG(("ADDING FILE: %s\n", entry2->d_name));
					psd->num_files++;

					// get next sound file in subdirectory
					entry2 = readdir(dir2);
				}

				// All sound files for this sound class collected.
				// Close the directory stream and free allocated memory
				closedir(dir2);
				free(PATH2);

				// Restore backed up parent folder and entry.
				*dir = backup_dir;
				*entry = backup_entry;
			}

			// get next entry in the parent directory stream
			entry = readdir(dir);
		}

		// All sound class directories processed.  Close directory stream and
		// free allocated memory
		closedir(dir);
		free(PATH);
	}

	if (psd->num_files <= 0) {
		// No filenames parsed.. exit
		LOG(("NO FILES FOUND!\n"));
		psd->files_with_subpath = NULL;

		
		LOG(("END: PARSE_ALTSOUND_FOLDERS\n"));
		return false;
	}

	// Allocate memory to store sound sample data
	psd->ID = (int*)malloc(psd->num_files * sizeof(int));
	psd->files_with_subpath = (char**)malloc(psd->num_files * sizeof(char*));
	psd->channel = (signed char*)malloc(psd->num_files * sizeof(signed char));
	psd->gain = (float*)malloc(psd->num_files * sizeof(float));
	psd->ducking = (float*)malloc(psd->num_files * sizeof(float));
	psd->loop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	psd->stop = (unsigned char*)malloc(psd->num_files * sizeof(unsigned char));
	psd->num_files = 0;

	// Search each parent sound class directory and parse gain.txt and
	// ducking.txt files to determine default gain and ducking for any
	// sound class instruction # sample that does not define an explicit
	// value
	for (int i = 0; i < 5; ++i) {
		float default_gain = .1f;
		float default_ducking = 1.f; //!! default depends on type??

		const char* const subpath = (i == 0) ? path_jingle :
			((i == 1) ? path_music :
			((i == 2) ? path_sfx :
				((i == 3) ? path_single : path_voice)));
		// DAR@20230525
		// For information's sake, PinSound defines the default ducking values as:
		//    SFX    : 90%
		//    VOICE  : 85%
		//    JINGLE :  5%
		//
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

		// Detmine path size and allocate memory
		const size_t PATHl = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1 + strlen(subpath) + 1;
		char* const PATH = (char*)malloc(PATHl);

		DIR *dir;
		struct dirent *entry;

		strcpy_s(PATH, PATHl, cvpmd);
		strcat_s(PATH, PATHl, path_main);
		strcat_s(PATH, PATHl, g_szGameName);
		strcat_s(PATH, PATHl, "\\");
		strcat_s(PATH, PATHl, subpath);
		LOG(("CURRENT_PATH1: %s\n", PATH));

		dir = opendir(PATH);
		if (!dir)
		{
			// Path not found.. try the others
			LOG(("Path not found: %s\n", PATH));
			free(PATH);
			continue;
		}
		{// parse gain.txt
			const size_t PATHGl = strlen(PATH) + strlen("gain.txt") + 1;
			char* const PATHG = (char*)malloc(PATHGl);
			FILE *f;
			strcpy_s(PATHG, PATHGl, PATH);
			strcat_s(PATHG, PATHGl, "gain.txt");
			LOG(("CURRENT_PATH2: %s\n", PATHG));
			f = fopen(PATHG, "r");

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

			free(PATHG);
		}
		{// parse ducking.txt
			const size_t PATHGl = strlen(PATH) + strlen("ducking.txt") + 1;
			char* const PATHG = (char*)malloc(PATHGl);
			FILE *f;
			strcpy_s(PATHG, PATHGl, PATH);
			strcat_s(PATHG, PATHGl, "ducking.txt");
			LOG(("CURRENT_PATH3: %s\n", PATHG));
			f = fopen(PATHG, "r");

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
			free(PATHG);
		}

		// parse individual sound file data
		entry = readdir(dir);
		while (entry != NULL) {
			if (entry->d_name[0] != '.'
				&& strstr(entry->d_name, ".txt") == 0
				&& strstr(entry->d_name, ".ini") == 0)
			{
				//LOG(("CURRENT_ENTRY1: %s\n", entry->d_name));
				// Not a system file or txt file.  Assume it's a directory
				// (per PinSound format requirements).  Backup the current
				// directory stream and entry
				const DIR backup_dir = *dir;
				struct dirent backup_entry = *entry;

				const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
				char* const PATH2 = (char*)malloc(PATH2l);
				float gain = default_gain;
				float ducking = default_ducking;

				DIR *dir2;
				struct dirent *entry2;

				strcpy_s(PATH2, PATH2l, PATH);
				strcat_s(PATH2, PATH2l, entry->d_name);

				{// individual gain
					const size_t PATHGl = strlen(PATH2) + 1 + strlen("gain.txt") + 1;
					char* const PATHG = (char*)malloc(PATHGl);
					FILE *f;

					strcpy_s(PATHG, PATHGl, PATH2);
					strcat_s(PATHG, PATHGl, "\\");
					strcat_s(PATHG, PATHGl, "gain.txt");
					LOG(("CURRENT_PATH4: %s\n", PATHG));
					f = fopen(PATHG, "r");
					if (f)
					{
						unsigned int tmp_gain = 0;
						fscanf(f, "%u", &tmp_gain);
						fclose(f);
						gain = tmp_gain > 100 ? 1.0f : (float)tmp_gain / 100.f;
					}
					free(PATHG);
				}
				{// individual ducking
					const size_t PATHGl = strlen(PATH2) + 1 + strlen("ducking.txt") + 1;
					char* const PATHG = (char*)malloc(PATHGl);
					FILE *f;

					strcpy_s(PATHG, PATHGl, PATH2);
					strcat_s(PATHG, PATHGl, "\\");
					strcat_s(PATHG, PATHGl, "ducking.txt");
					LOG(("CURRENT_PATH5: %s\n", PATHG));
					f = fopen(PATHG, "r");
					if (f)
					{
						int tmp_ducking = 0;
						fscanf(f, "%d", &tmp_ducking);
						fclose(f);
						ducking = tmp_ducking > 100 ? 1.0f : (float)tmp_ducking / 100.f;
					}
					free(PATHG);
				}

				LOG(("OPENDIR: %s\n", PATH2));
				dir2 = opendir(PATH2);
				entry2 = readdir(dir2);
				while (entry2 != NULL) {
					if (entry2->d_name[0] != '.'
						&& strstr(entry2->d_name, ".txt") == 0
						&& strstr(entry2->d_name, ".ini") == 0)
					{
						//LOG(("CURRENT_ENTRY2: %s\n", entry->d_name));
						const size_t PATH3l = strlen(PATH2) + 1 + strlen(entry2->d_name) + 1;
						char* const ptr = strrchr(PATH2, '\\');
						char id[7] = { 0, 0, 0, 0, 0, 0, 0 };

						psd->files_with_subpath[psd->num_files] = (char*)malloc(PATH3l);
						strcpy_s(psd->files_with_subpath[psd->num_files], PATH3l, PATH2);
						strcat_s(psd->files_with_subpath[psd->num_files], PATH3l, "\\");
						strcat_s(psd->files_with_subpath[psd->num_files], PATH3l, entry2->d_name);

						memcpy(id, ptr + 1, 6);
						sscanf(id, "%6d", &psd->ID[psd->num_files]);

						psd->gain[psd->num_files] = gain;
						psd->ducking[psd->num_files] = ducking;

						if (subpath == path_music) {
							psd->channel[psd->num_files] = 0;
							psd->loop[psd->num_files] = 100;
							psd->stop[psd->num_files] = 0;
						}

						if (subpath == path_jingle || subpath == path_single) {
							psd->channel[psd->num_files] = 1;
							psd->loop[psd->num_files] = 0;
							psd->stop[psd->num_files] = (subpath == path_single) ? 1 : 0;
						}

						if (subpath == path_sfx || subpath == path_voice) {
							psd->channel[psd->num_files] = -1;
							psd->loop[psd->num_files] = 0;
							psd->stop[psd->num_files] = 0;
						}

						LOG(("ID = %d, CHANNEL = %d, DUCK = %.2f, GAIN = %.2f, LOOP = %d, STOP = %d, FNAME = '%s'\n" \
							, psd->ID[i], psd->channel[i], psd->ducking[i], psd->gain[i], psd->loop[i] \
							, psd->stop[i], psd->files_with_subpath[i]));

						psd->num_files++;
					}
					entry2 = readdir(dir2);
				}
				closedir(dir2);
				free(PATH2);

				*dir = backup_dir;
				*entry = backup_entry;
			}
			entry = readdir(dir);
		}
		closedir(dir);
		free(PATH);
	}
	LOG(("Found %d samples\n", psd->num_files));

	dir = opendir(cwd);
	closedir(dir);

	LOG(("END: PARSE_ALTSOUND_FOLDERS\n"));
	return true;
}