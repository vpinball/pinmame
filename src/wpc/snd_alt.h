#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include "..\ext\bass\bass.h"

#define VERBOSE 0

#if VERBOSE
 #define LOG(x) logerror x
#else
 #define LOG(x)
#endif

typedef struct _pin_samples { // holds data for all sound files found
	int * ID;
	char ** files_with_subpath;
	signed char * channel;
	float * gain;
	signed char * ducking;
	unsigned char * loop;
	unsigned char * stop;
	unsigned int num_files;
} Pin_samples;


typedef struct _csvreader { // to read the data via a csv file
	FILE* f;
	int delimiter;
	int n_header_fields;
	char** header_fields;	// header split in fields
	int n_fields;
	char** fields;			// current row split in fields
} CsvReader;

int csv_get_colnumber_for_field(CsvReader* c, const char* fieldname);
int csv_get_hex_field(CsvReader* const c, const int field_index, int* pValue);
int csv_get_int_field(CsvReader* const c, const int field_index, int* pValue);

#define CSV_MAX_LINE_LENGTH 512
#define CSV_SUCCESS 0
#define CSV_ERROR_NO_SUCH_FIELD -1
#define CSV_ERROR_HEADER_NOT_FOUND -2
#define CSV_ERROR_NO_MORE_RECORDS -3
#define CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE -4
#define CSV_ERROR_FILE_NOT_FOUND -5
#define CSV_ERROR_LINE_FORMAT -6


const char* path_main = "\\altsound\\";
const char* path_jingle = "jingle\\";
const char* path_music = "music\\";
const char* path_sfx = "sfx\\";
const char* path_single = "single\\";
const char* path_voice = "voice\\";

const char* path_table = "\\altsound.csv";

extern char g_szGameName[256];
static char* cached_machine_name = 0;

float alt_sound_gain(const int gain) //!! which one?
{
/*#define DSBVOLUME_MIN               -10000
#define DSBVOLUME_MAX               0
	const float MusicVolumef = (float)(max(min(gain, 20), 0)*5);
	const int volume = (MusicVolumef == 0.0f) ? DSBVOLUME_MIN : (int)(logf(MusicVolumef)*(float)(1000.0 / log(10.0)) - 2000.0f); // 10 volume = -10Db
	return (float)(volume - DSBVOLUME_MIN)*(float)(1.0 / (DSBVOLUME_MAX - DSBVOLUME_MIN));*/
	return (float)gain / 20.f;
}

static HSTREAM channel_0 = 0; 
static HSTREAM channel_1 = 0; // includes single_stream
#define ALT_MAX_CHANNELS 16
static HSTREAM channel_x[ALT_MAX_CHANNELS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // includes sfx_stream (or must this be separated to only have 2 channels for sfx?)
static signed char channel_1_ducking = 100;
static signed char channel_x_ducking[ALT_MAX_CHANNELS] = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
static float channel_0_vol = 1.0f;
static signed char min_ducking = 100;

CsvReader* csv_open(const char* const filename, const int delimiter);
int csv_read_header(CsvReader* const c);
void csv_close(CsvReader* const c);
int csv_read_record(CsvReader* const c);

void CALLBACK ducking_callback(HSYNC handle, DWORD channel, DWORD data, void *user)
{
	if (channel == channel_1) {
		BASS_StreamFree(channel_1);
		channel_1 = 0;
		channel_1_ducking = 100;
	}

	if (channel == channel_x[(int)user]) {
		BASS_StreamFree(channel_x[(int)user]);
		channel_x[(int)user] = 0;
		channel_x_ducking[(int)user] = 100;
	}

	if (channel_0 != 0)
	{
		unsigned int i;
		float new_val;

		min_ducking = 100;
		if (channel_1 != 0 && channel_1_ducking < min_ducking)
			min_ducking = channel_1_ducking;
		for (i = 0; i < ALT_MAX_CHANNELS; ++i) {
			if (channel_x[i] != 0 && channel_x_ducking[i] < min_ducking)
				min_ducking = channel_x_ducking[i];
		}

		new_val = channel_0_vol*(float)((double)min_ducking / 100.);
		if (channel_0_vol != new_val)
			BASS_ChannelSetAttribute(channel_0, BASS_ATTRIB_VOL, new_val);
		
		// if channel 0 was stopped continue
		if (BASS_ChannelIsActive(channel_0) != BASS_ACTIVE_PLAYING)
			BASS_ChannelPlay(channel_0, 0);
	}
}

void alt_sound_handle(int boardNo, int cmd)
{
	if (TRUE) //!! only do search for dir as soon as sound is disabled? or even additional flag/interface call?
	{
		static unsigned int cmd_counter = 0;
		static unsigned int cmd_storage = -1;

		static unsigned int cmd_filter = 0;
#define ALT_MAX_CMDS 4
		static unsigned int cmd_buffer[ALT_MAX_CMDS] = { ~0, ~0, ~0, ~0 };

		static Pin_samples psd;

		static float global_vol = 1.0f;

		unsigned int i;

		int	attenuation = osd_get_mastervolume();
		float master_vol = 1.0f;
		while (attenuation++ < 0)
			master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB

		if (cached_machine_name != 0 && strstr(g_szGameName, cached_machine_name) == 0) // another game has been loaded? -> previous data has to be free'd
		{
			cmd_counter = 0;
			cmd_storage = -1;

			cmd_filter = 0;
			for (i = 0; i < ALT_MAX_CMDS; ++i)
				cmd_buffer[i] = ~0;

			free(cached_machine_name);
			cached_machine_name = 0;

			channel_0 = 0;
			channel_1 = 0;
			for (i = 0; i < ALT_MAX_CHANNELS; ++i)
				channel_x[i] = 0;

			global_vol = 1.0f;
			channel_0_vol = 1.0f;
			channel_1_ducking = 100;
			for (i = 0; i < ALT_MAX_CHANNELS; ++i)
				channel_x_ducking[i] = 100;

			BASS_Free();

			if (psd.num_files > 0)
			{
				for (i = 0; i < psd.num_files; ++i)
				{
					free(psd.files_with_subpath[i]);
					psd.files_with_subpath[i] = 0;
				}
				free(psd.ID);
				psd.ID = 0;
				free(psd.files_with_subpath);
				psd.files_with_subpath = 0;
				free(psd.gain);
				psd.gain = 0;
				free(psd.ducking);
				psd.ducking = 0;
				free(psd.channel);
				psd.channel = 0;
				free(psd.loop);
				psd.loop = 0;
				free(psd.stop);
				psd.stop = 0;
				psd.num_files = 0;
			}
		}

		// load sample information and init
		if (cmd_storage == -1)
		{
			char cwd[1024];
			char cvpmd[1024];
			HINSTANCE hInst;
			char *lpHelp = cvpmd;
			char *lpSlash = NULL;
			DIR* dir;

			CsvReader* c;
			size_t PATH_LEN;
			char* PATH;

			srand(time(NULL)); // randomize random number generator seed

			cached_machine_name = (char*)malloc(strlen(g_szGameName) + 1);
			strcpy(cached_machine_name, g_szGameName);

			_getcwd(cwd, sizeof(cwd));

#ifndef _WIN64
			hInst = GetModuleHandle("VPinMAME.dll");
#else
			hInst = GetModuleHandle("VPinMAME64.dll");
#endif
			GetModuleFileName(hInst, cvpmd, sizeof(cvpmd));

			while (*lpHelp) {
				if (*lpHelp == '\\')
					lpSlash = lpHelp;
				lpHelp++;
			}
			if (lpSlash)
				*lpSlash = '\0';
			//

			psd.num_files = 0;

			// try to load altsound lookup table/csv if available
			PATH_LEN = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1 + strlen(path_table) + 1;
			PATH = (char*)malloc(PATH_LEN);
			strcpy_s(PATH, PATH_LEN, cvpmd);
			strcat_s(PATH, PATH_LEN, path_main);
			strcat_s(PATH, PATH_LEN, g_szGameName);
			strcat_s(PATH, PATH_LEN, path_table);

			c = csv_open(PATH, ',');

			if (c) {
				int colID,colCHANNEL,colDUCK,colGAIN,colLOOP,colSTOP,colFNAME;
				long pos;
				csv_read_header(c);
				LOG(("n_headers: %d\n", c->n_header_fields));
				colID = csv_get_colnumber_for_field(c, "ID");
				colCHANNEL = csv_get_colnumber_for_field(c, "CHANNEL");
				colDUCK = csv_get_colnumber_for_field(c, "DUCK");
				colGAIN = csv_get_colnumber_for_field(c, "GAIN");
				colLOOP = csv_get_colnumber_for_field(c, "LOOP");
				colSTOP = csv_get_colnumber_for_field(c, "STOP");
				colFNAME = csv_get_colnumber_for_field(c, "FNAME");

				pos = ftell(c->f);

				while (csv_read_record(c) == 0) {
					int val;
					csv_get_hex_field(c, colID, &val);
					csv_get_int_field(c, colCHANNEL, &val);
					csv_get_int_field(c, colDUCK, &val);
					csv_get_int_field(c, colGAIN, &val);
					csv_get_int_field(c, colLOOP, &val);
					csv_get_int_field(c, colSTOP, &val);
					psd.num_files++;
				}

				if (psd.num_files > 0)
				{
					psd.ID = (int*)malloc(psd.num_files*sizeof(int));
					psd.files_with_subpath = (char**)malloc(psd.num_files*sizeof(char*));
					psd.channel = (signed char*)malloc(psd.num_files*sizeof(signed char));
					psd.gain = (float*)malloc(psd.num_files*sizeof(float));
					psd.ducking = (signed char*)malloc(psd.num_files*sizeof(signed char));
					psd.loop = (unsigned char*)malloc(psd.num_files*sizeof(unsigned char));
					psd.stop = (unsigned char*)malloc(psd.num_files*sizeof(unsigned char));
				}
				else
					psd.files_with_subpath = NULL;

				fseek(c->f, pos, SEEK_SET);

				for (i = 0; i < psd.num_files; ++i) {
					char filePath[4096];
					char tmpPath[4096];

					int val = 0;
					csv_read_record(c);
					csv_get_hex_field(c, colID, &val);
					psd.ID[i] = val;
					val = 0;
					psd.channel[i] = csv_get_int_field(c, colCHANNEL, &val) ? - 1 : val;
					val = 0;
					csv_get_int_field(c, colDUCK, &val);
					psd.ducking[i] = min(val, (int)100);
					val = 0;
					csv_get_int_field(c, colGAIN, &val);
					psd.gain[i] = val / 100.f;
					val = 0;
					csv_get_int_field(c, colLOOP, &val);
					psd.loop[i] = val;
					val = 0;
					csv_get_int_field(c, colSTOP, &val);
					psd.stop[i] = val;

					strcpy_s(filePath, sizeof(filePath), cvpmd);
					strcat_s(filePath, sizeof(filePath), path_main);
					strcat_s(filePath, sizeof(filePath), g_szGameName);
					strcat_s(filePath, sizeof(filePath), "\\");
					strcat_s(filePath, sizeof(filePath), c->fields[colFNAME]);
					GetFullPathName(filePath, sizeof(filePath), tmpPath, NULL);
					psd.files_with_subpath[i] = (char*)malloc(strlen(tmpPath)+1);
					strcpy(psd.files_with_subpath[i], tmpPath);
					LOG(("ID = %d, ", psd.ID[i])); LOG(("CHANNEL = %d, ", psd.channel[i])); LOG(("DUCK = %d, ", psd.ducking[i])); LOG(("GAIN = %.2f, ", psd.gain[i])); LOG(("LOOP = %d, ", psd.loop[i])); LOG(("STOP = %d, ", psd.stop[i])); LOG(("FNAME = '%s'\n", psd.files_with_subpath[i]));
				}

				csv_close(c);
				LOG(("found %d samples\n ", psd.num_files));
			}

			free(PATH);

			// OTHERWISE scan folders for old folder-based structure of the alternate sound package
			if (psd.num_files == 0) { 
				for (i = 0; i < 5; ++i)
				{
					const char* const subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));

					const size_t PATHl = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1 + strlen(subpath) + 1;
					DIR *dir;
					struct dirent *entry;
					PATH = (char*)malloc(PATHl);

					strcpy_s(PATH, PATHl, cvpmd);
					strcat_s(PATH, PATHl, path_main);
					strcat_s(PATH, PATHl, g_szGameName);
					strcat_s(PATH, PATHl, "\\");
					strcat_s(PATH, PATHl, subpath);

					dir = opendir(PATH);
					if (!dir)
					{
						free(PATH);
						continue;
					}

					entry = readdir(dir);
					while (entry != NULL)
					{
						if (entry->d_name[0] != '.' && strstr(entry->d_name, ".txt") == 0)
						{
							const DIR backup_dir = *dir;
							DIR *dir2;
							struct dirent backup_entry = *entry;
							struct dirent *entry2;

							const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
							char* const PATH2 = (char*)malloc(PATH2l);
							strcpy_s(PATH2, PATH2l, PATH);
							strcat_s(PATH2, PATH2l, entry->d_name);

							dir2 = opendir(PATH2);
							entry2 = readdir(dir2);
							while (entry2 != NULL)
							{
								if (entry2->d_name[0] != '.' && strstr(entry2->d_name, ".txt") == 0)
									psd.num_files++;
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

				if (psd.num_files > 0)
				{
					psd.ID = (int*)malloc(psd.num_files*sizeof(int));
					psd.files_with_subpath = (char**)malloc(psd.num_files*sizeof(char*));
					psd.channel = (signed char*)malloc(psd.num_files*sizeof(signed char));
					psd.gain = (float*)malloc(psd.num_files*sizeof(float));
					psd.ducking = (signed char*)malloc(psd.num_files*sizeof(signed char));
					psd.loop = (unsigned char*)malloc(psd.num_files*sizeof(unsigned char));
					psd.stop = (unsigned char*)malloc(psd.num_files*sizeof(unsigned char));
					psd.num_files = 0;
				}
				else
					psd.files_with_subpath = NULL;

				for (i = 0; i < 5; ++i)
				{
					const char* subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));
					const size_t PATHl = strlen(cvpmd) + strlen(path_main) + strlen(g_szGameName) + 1 + strlen(subpath) + 1;
					char* const PATH = (char*)malloc(PATHl);
					DIR *dir;
					struct dirent *entry;
					unsigned int default_gain = 10;
					int default_ducking = 100; //!! default depends on type??

					if (subpath == path_jingle || subpath == path_single) {
						default_ducking = 10;
					}
					if (subpath == path_sfx) {
						default_ducking = 80;
					}
					if (subpath == path_voice) {
						default_ducking = 65;
					}

					strcpy_s(PATH, PATHl, cvpmd);
					strcat_s(PATH, PATHl, path_main);
					strcat_s(PATH, PATHl, g_szGameName);
					strcat_s(PATH, PATHl, "\\");
					strcat_s(PATH, PATHl, subpath);

					dir = opendir(PATH);
					if (!dir)
					{
						free(PATH);
						continue;
					}
					{
						const size_t PATHGl = strlen(PATH) + strlen("gain.txt") + 1;
						char* const PATHG = (char*)malloc(PATHGl);
						FILE *f;
						strcpy_s(PATHG, PATHGl, PATH);
						strcat_s(PATHG, PATHGl, "gain.txt");
						f = fopen(PATHG, "r");
						if (f)
						{
							fscanf(f, "%u", &default_gain);
							fclose(f);
						}
						free(PATHG);
					}
					{
						const size_t PATHGl = strlen(PATH) + strlen("ducking.txt") + 1;
						char* const PATHG = (char*)malloc(PATHGl);
						FILE *f;
						strcpy_s(PATHG, PATHGl, PATH);
						strcat_s(PATHG, PATHGl, "ducking.txt");
						f = fopen(PATHG, "r");
						if (f)
						{
							fscanf(f, "%d", &default_ducking);
							fclose(f);
						}
						free(PATHG);
					}

					entry = readdir(dir);
					while (entry != NULL)
					{
						if (entry->d_name[0] != '.' && strstr(entry->d_name, ".txt") == 0)
						{
							const DIR backup_dir = *dir;
							struct dirent backup_entry = *entry;

							const size_t PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
							char* const PATH2 = (char*)malloc(PATH2l);
							unsigned int gain = default_gain;
							int ducking = default_ducking;
							DIR *dir2;
							struct dirent *entry2;

							strcpy_s(PATH2, PATH2l, PATH);
							strcat_s(PATH2, PATH2l, entry->d_name);

							{
								const size_t PATHGl = strlen(PATH2) + 1 + strlen("gain.txt") + 1;
								char* const PATHG = (char*)malloc(PATHGl);
								FILE *f;

								strcpy_s(PATHG, PATHGl, PATH2);
								strcat_s(PATHG, PATHGl, "\\");
								strcat_s(PATHG, PATHGl, "gain.txt");
								f = fopen(PATHG, "r");
								if (f)
								{
									fscanf(f, "%u", &gain);
									fclose(f);
								}
								free(PATHG);
							}
							{
								const size_t PATHGl = strlen(PATH2) + 1 + strlen("ducking.txt") + 1;
								char* const PATHG = (char*)malloc(PATHGl);
								FILE *f;

								strcpy_s(PATHG, PATHGl, PATH2);
								strcat_s(PATHG, PATHGl, "\\");
								strcat_s(PATHG, PATHGl, "ducking.txt");
								f = fopen(PATHG, "r");
								if (f)
								{
									fscanf(f, "%d", &ducking);
									fclose(f);
								}
								free(PATHG);
							}

						  dir2 = opendir(PATH2);
						  entry2 = readdir(dir2);
						  while (entry2 != NULL)
						  {
							  if (entry2->d_name[0] != '.' && strstr(entry2->d_name, ".txt") == 0)
							  {
								  const size_t PATH3l = strlen(PATH2) + 1 + strlen(entry2->d_name) + 1;
								  char* const ptr = strrchr(PATH2, '\\');
								  char id[7] = { 0, 0, 0, 0, 0, 0, 0 };

								  psd.files_with_subpath[psd.num_files] = (char*)malloc(PATH3l);
								  strcpy_s(psd.files_with_subpath[psd.num_files], PATH3l, PATH2);
								  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, "\\");
								  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, entry2->d_name);

								  memcpy(id, ptr + 1, 6);
								  sscanf(id, "%6d", &psd.ID[psd.num_files]);

								  psd.gain[psd.num_files] = alt_sound_gain(gain);
								  psd.ducking[psd.num_files] = min(ducking, (int)100);

								  if (subpath == path_music) {
									  psd.channel[psd.num_files] = 0;
									  psd.loop[psd.num_files] = 100;
									  psd.stop[psd.num_files] = 0;
								  }

								  if (subpath == path_jingle || subpath == path_single) {
									  psd.channel[psd.num_files] = 1;
									  psd.loop[psd.num_files] = 0;
									  psd.stop[psd.num_files] = (subpath == path_single) ? 1 : 0;
								  }

								  if (subpath == path_sfx || subpath == path_voice) {
									  psd.channel[psd.num_files] = -1;
									  psd.loop[psd.num_files] = 0;
									  psd.stop[psd.num_files] = 0;
								  }

								  LOG(("ID = %d, ", psd.ID[psd.num_files])); LOG(("CHANNEL = %d, ", psd.channel[psd.num_files])); LOG(("DUCK = %d, ", psd.ducking[psd.num_files])); LOG(("GAIN = %.2f, ", psd.gain[psd.num_files])); LOG(("LOOP = %d, ", psd.loop[psd.num_files])); LOG(("STOP = %d, ", psd.stop[psd.num_files])); LOG(("FNAME = '%s'\n", psd.files_with_subpath[psd.num_files]));
								  psd.num_files++;
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
				LOG(("found %d samples\n ", psd.num_files));
			}

			//
			dir = opendir(cwd);
			closedir(dir);

			//
			if (psd.num_files > 0)
			{
				int DSidx = -1;
				//!! GetRegInt("Player", "SoundDeviceBG", &DSidx);
				if (DSidx != -1)
					DSidx++; // as 0 is nosound //!! mapping is otherwise the same or not?!
				if (!BASS_Init(DSidx, 44100, 0, /*g_pvp->m_hwnd*/NULL, NULL)) //!! get sample rate from VPM? and window?
				{
					//sprintf_s(bla, "BASS music/sound library initialization error %d", BASS_ErrorGetCode());
				}

				// force internal PinMAME volume mixer to 0 to mute emulated sounds & musics
				//for (DSidx = 0; DSidx < MIXER_MAX_CHANNELS; DSidx++)
				//	if (mixer_get_name(DSidx) != NULL)
				//		mixer_set_volume(DSidx, 0);
				mixer_sound_enable_global_w(0);
			}
			else
				cmd_storage = 0;
		}
		// end of load sample information and init

		if (psd.num_files > 0)
		{
			int ch;
			// force internal PinMAME volume mixer to 0 to mute emulated sounds & musics
			// required for WPC89 sound board
			for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
				if (mixer_get_name(ch) != NULL)
					mixer_set_volume(ch, 0);

			cmd_counter++;

			for (i = ALT_MAX_CMDS - 1; i > 0; --i)
				cmd_buffer[i] = cmd_buffer[i - 1];
			cmd_buffer[0] = cmd;

			if ((core_gameData->gen == GEN_WPCDCS) ||
				(core_gameData->gen == GEN_WPCSECURITY) ||
				(core_gameData->gen == GEN_WPC95DCS) ||
				(core_gameData->gen == GEN_WPC95))
			{
				if (((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA)) // change volume?
					||
					((cmd_buffer[2] == 0x00) && (cmd_buffer[1] == 0x00) && (cmd_buffer[0] == 0x00))) // glitch in command buffer?
				{
					if ((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA) && (cmd_buffer[1] == (cmd_buffer[0]^0xFF))) // change volume op (following first byte = volume, second = ~volume, if these don't match: ignore)
					{
						global_vol = min((float)cmd_buffer[1] / 127.f, 1.0f);
						if (channel_0 != 0)
							BASS_ChannelSetAttribute(channel_0, BASS_ATTRIB_VOL, channel_0_vol * global_vol * master_vol);

						LOG(("change volume %.2f\n", global_vol));
					}
					else
						LOG(("filtered command %02X %02X %02X %02X\n", cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]));

					for (i = 0; i < ALT_MAX_CMDS; ++i)
						cmd_buffer[i] = ~0;

					cmd_counter = 0;
					cmd_filter = 1;
				}
				else
					cmd_filter = 0;
			}

			if ((core_gameData->gen == GEN_WPCALPHA_2) || //!! ?? test this gen actually
				(core_gameData->gen == GEN_WPCDMD) || // remaps everything to 16bit, a bit stupid maybe
				(core_gameData->gen == GEN_WPCFLIPTRON))
			{
				if (cmd_buffer[1] == 0x7A) // 16bit command second part //!! TZ triggers a 0xFF in the beginning -> check sequence and filter?
				{
					cmd_storage = cmd_buffer[1];
					cmd_counter = 0;
				}
				else if (cmd != 0x7A) // 8 bit command
				{
					cmd_storage = 0;
					cmd_counter = 0;
				}
				else // 16bit command first part
					cmd_counter = 1;
			}

			if ((core_gameData->gen == GEN_WPCALPHA_1) || // remaps everything to 16bit, a bit stupid maybe //!! test all these generations!
				(core_gameData->gen == GEN_S11) ||
				(core_gameData->gen == GEN_S11X) ||
				(core_gameData->gen == GEN_S11B2) ||
				(core_gameData->gen == GEN_S11C))
			{
				if (cmd != cmd_buffer[1]) //!! some stuff is doubled or tripled -> filter out?
				{
					cmd_storage = 0; // 8 bit command //!! 7F & 7E opcodes?
					cmd_counter = 0;
				}
				else // ignore
					cmd_counter = 1;
			}

			//!! stern should work out of the box, check elvis&LOTR again for special commands (stop,etc) though

			if ((core_gameData->gen == GEN_DEDMD16) || // remaps everything to 16bit, a bit stupid maybe
				(core_gameData->gen == GEN_DEDMD32) ||
				(core_gameData->gen == GEN_DEDMD64))
			{
				if (cmd != 0xFF && cmd != 0x00) // 8 bit command
				{
					cmd_storage = 0;
					cmd_counter = 0;
				}
				else // ignore
					cmd_counter = 1;

				if (cmd_buffer[1] == 0x00 && cmd == 0x00) // handle 0x0000 special //!! meh?
				{
					cmd_storage = 0;
					cmd_counter = 0;
				}
			}

			if ((core_gameData->gen == GEN_WS) || 
				(core_gameData->gen == GEN_WS_1) || 
				(core_gameData->gen == GEN_WS_2))
			{
				if ((cmd_buffer[2] == 0xFE && cmd_buffer[1] == 0x26 && (cmd & 0xF0) == 0xF0) || 
					(cmd_buffer[2] == 0xFE && cmd_buffer[1] == 0x25 && (cmd & 0xF0) == 0xF0) || 
					(cmd_buffer[2] == 0xFE && cmd_buffer[1] == 0x01 && (cmd & 0xF0) == 0xF0))
				{
					cmd_storage = 0;
					cmd_counter = 0;
				}
			}

			if (!cmd_filter && (cmd_counter & 1) == 0) // collect 16bits from two 8bit commands
			{
				unsigned int cmd_combined = (cmd_storage << 8) | cmd;

				unsigned int idx = -1;

				for (i = 0; i < psd.num_files; ++i)
					if (psd.ID[i] == cmd_combined)
					{
						// check if more samples are there for this command and randomly pick one
						unsigned int rnd = 0;
						do
						{
							rnd++;
							if (i + rnd >= psd.num_files)
								break;
						} while (psd.ID[i+rnd] == cmd_combined);

						idx = i + rand() % rnd;
						break;
					}

				if (idx != -1)
				{
					// play jingle or single
					if (psd.channel[idx] == 1)
					{
						channel_1_ducking = 100;

						if (channel_0 != 0)
						{
							if (psd.stop[idx] == 0)
							{
								if (psd.ducking[idx] < 0)
									BASS_ChannelPause(channel_0);
								else
									channel_1_ducking = psd.ducking[idx];
							}
							else
							{
								BASS_ChannelStop(channel_0);
								BASS_StreamFree(channel_0);
								channel_0 = 0;
								channel_0_vol = 1.0f;
							}
						}

						//

						if (channel_1 != 0)
						{
							BASS_ChannelStop(channel_1);
							BASS_StreamFree(channel_1);
							channel_1 = 0;
						}

						channel_1 = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, (psd.loop[idx] == 100) ? BASS_SAMPLE_LOOP : 0);

						if (channel_1 == 0)
						{
							LOG(("BASS music/sound library cannot load %s\n", psd.files_with_subpath[idx]));
						}
						else
						{
							BASS_ChannelSetAttribute(channel_1, BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);
							BASS_ChannelSetSync(channel_1, BASS_SYNC_END | BASS_SYNC_ONETIME, 0, ducking_callback, 0);
							LOG(("playing CH1: cmd %04X gain %.2f duck %d %s\n", cmd_combined, psd.gain[idx], psd.ducking[idx], psd.files_with_subpath[idx]));
							BASS_ChannelPlay(channel_1, 0);
							if (psd.ducking[idx] > 0 && psd.ducking[idx] < min_ducking) {
								float new_val;
								min_ducking = psd.ducking[idx];
								new_val = channel_0_vol*(float)((double)psd.ducking[idx] / 100.);
								if (channel_0_vol != new_val)
									BASS_ChannelSetAttribute(channel_0, BASS_ATTRIB_VOL, new_val);
							}
						}
					}

					// play music 
					if (psd.channel[idx] == 0)
					{
						if (channel_0 != 0)
						{
							BASS_ChannelStop(channel_0);
							BASS_StreamFree(channel_0);
							channel_0 = 0;
							channel_0_vol = 1.0f;
						}

						channel_0 = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, (psd.loop[idx] == 100) ? BASS_SAMPLE_LOOP : 0);

						if (channel_0 == 0)
						{
							LOG(("BASS music/sound library cannot load %s\n", psd.files_with_subpath[idx]));
						}
						else
						{
							channel_0_vol = psd.gain[idx];
							BASS_ChannelSetAttribute(channel_0, BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);
							LOG(("playing CH0: cmd %04X gain %.2f duck %d %s\n", cmd_combined, psd.gain[idx], psd.ducking[idx], psd.files_with_subpath[idx]));
							BASS_ChannelPlay(channel_0, 0);
						}
					}

					// play voice or sfx
					if (psd.channel[idx] == -1)
					{
						unsigned int channel_x_idx = -1;
						for (i = 0; i < ALT_MAX_CHANNELS; ++i)
							if (channel_x[i] == 0 || BASS_ChannelIsActive(channel_x[i]) != BASS_ACTIVE_PLAYING)
							{
								if (channel_x[i] != 0)
								{
									BASS_StreamFree(channel_x[i]);
									channel_x[i] = 0;
								}

								channel_x_idx = i;
								break;
							}

						/*if (channel_x_idx == -1) //!! kill off the longest playing channel as f.e. LAH has helicopter sound that isn't stopped it seems!
						{
							double max_pos = 0;
							unsigned int i;
							for (i = 0; i < ALT_MAX_CHANNELS; ++i)
							{
								const double pos = BASS_ChannelBytes2Seconds(channel_x[channel_x_idx], BASS_ChannelGetPosition(channel_x[channel_x_idx], BASS_POS_BYTE));
								if (pos > max_pos)
								{
									max_pos = pos;
									channel_x_idx = i;
								}
							}

							BASS_ChannelStop(channel_x[channel_x_idx]);
							BASS_StreamFree(channel_x[channel_x_idx]);
							channel_x[channel_x_idx] = 0;
						}*/

						if (channel_x_idx != -1)
						{
							channel_x_ducking[channel_x_idx] = psd.ducking[idx];

							channel_x[channel_x_idx] = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, (psd.loop[idx] == 100) ? BASS_SAMPLE_LOOP : 0);

							if (channel_x[channel_x_idx] == 0)
							{
								LOG(("BASS music/sound library cannot load %s\n", psd.files_with_subpath[idx]));
							}
							else
							{
								BASS_ChannelSetAttribute(channel_x[channel_x_idx], BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);
								BASS_ChannelSetSync(channel_x[channel_x_idx], BASS_SYNC_END | BASS_SYNC_ONETIME, 0, ducking_callback, (void*)channel_x_idx);
								LOG(("playing CHX: cmd %04X gain %.2f duck %d %s\n", cmd_combined, psd.gain[idx], psd.ducking[idx], psd.files_with_subpath[idx]));
								BASS_ChannelPlay(channel_x[channel_x_idx], 0);
							}

							if (psd.ducking[idx] > 0 && psd.ducking[idx] < min_ducking) {
								float new_val;
								min_ducking = psd.ducking[idx];
								new_val = channel_0_vol*(float)((double)psd.ducking[idx] / 100.);
								if (channel_0_vol != new_val)
									BASS_ChannelSetAttribute(channel_0, BASS_ATTRIB_VOL, new_val);
							}
						}
					}
				}

				//!! gain=10 is normallevel not gain=20 (but then it would also be clipped, so meh)

				{
					if (idx == -1)
						LOG(("%04X unknown %u\n", cmd_combined, boardNo));

					if ((core_gameData->gen == GEN_WPCDCS) ||
						(core_gameData->gen == GEN_WPCSECURITY) ||
						(core_gameData->gen == GEN_WPC95DCS) ||
						(core_gameData->gen == GEN_WPC95))
					{
						if (cmd_combined == 0x03E3 && channel_0 != 0) // stop music
						{
							BASS_ChannelStop(channel_0);
							BASS_StreamFree(channel_0);
							channel_0 = 0;
							channel_0_vol = 1.0f;
						}
					}

					//!! old WPC machines music stop? -> 0x00 for SYS11?

					if (core_gameData->gen == GEN_DEDMD32)
					{
						if ((cmd_combined == 0x0018 || cmd_combined == 0x0023) && channel_0 != 0) // stop music //!! ???? 0x0019??
						{
							BASS_ChannelStop(channel_0);
							BASS_StreamFree(channel_0);
							channel_0 = 0;
							channel_0_vol = 1.0f;
						}
					}

					if ((core_gameData->gen == GEN_WS) ||
						(core_gameData->gen == GEN_WS_1) ||
						(core_gameData->gen == GEN_WS_2))
					{
						if (((cmd_combined == 0x0000 || (cmd_combined & 0xf0ff) == 0xf000)) && channel_0 != 0) // stop music
						{
							BASS_ChannelStop(channel_0);
							BASS_StreamFree(channel_0);
							channel_0 = 0;
							channel_0_vol = 1.0f;
						}
					}
				}
			}
			else
				cmd_storage = cmd;
		}
	}
}

void alt_sound_exit()
{
	if (cached_machine_name != 0) // better free everything like above!
	{
		cached_machine_name[0] = '#';
		BASS_Free();
	}
}

void alt_sound_pause(BOOL pause)
{
	unsigned int i;
	if (pause)
	{
		for (i = 0; i < ALT_MAX_CHANNELS; ++i)
			if (channel_x[i] != 0 && BASS_ChannelIsActive(channel_x[i]) == BASS_ACTIVE_PLAYING)
				BASS_ChannelPause(channel_x[i]);

		if (channel_1 != 0 && BASS_ChannelIsActive(channel_1) == BASS_ACTIVE_PLAYING)
			BASS_ChannelPause(channel_1);

		if (channel_0 != 0 && BASS_ChannelIsActive(channel_0) == BASS_ACTIVE_PLAYING)
			BASS_ChannelPause(channel_0);
	}
	else
	{
		for (i = 0; i < ALT_MAX_CHANNELS; ++i)
			if (channel_x[i] != 0 && BASS_ChannelIsActive(channel_x[i]) == BASS_ACTIVE_PAUSED)
				BASS_ChannelPlay(channel_x[i],0);

		if (channel_1 != 0 && BASS_ChannelIsActive(channel_1) == BASS_ACTIVE_PAUSED)
			BASS_ChannelPlay(channel_1,0);

		if (channel_0 != 0 && BASS_ChannelIsActive(channel_0) == BASS_ACTIVE_PAUSED)
			BASS_ChannelPlay(channel_0,0);
	}
}

//
// CSV parsing
//

/**
* init struct and open file
*/
static CsvReader* csv_open(const char* const filename, const int delimiter) {
	CsvReader* const c = malloc(sizeof(CsvReader));
	c->f = fopen(filename, "r");
	if (c->f == NULL) {
		free(c);
		return NULL;
	}

	c->delimiter = delimiter;
	c->n_header_fields = 0;
	c->n_fields = 0;

	return c;
}

/**
* trim field buffer from end to start
*/
static void trim(const char* const start, char* end) {
	while (end > start) {
		end--;
		if (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') {
			*end = 0;
		}
		else {
			break;
		}
	}
}

/* Get a line of text from a file, discarding any end-of-line characters */
static int fgetline(char* const buff, const int nchars, FILE* const file)
{
	int length;

	if (fgets(buff, nchars, file) == NULL)
		return -1;
	if (buff[0] == '\r')
		memmove(buff, buff + 1, nchars - 1);

	length = (int)strlen(buff);
	while (length && (buff[length - 1] == '\r' || buff[length - 1] == '\n'))
		length--;
	buff[length] = 0;

	return length;
}

static int parse_line(CsvReader* const c, char* line, const int header) {
	char* p = line;
	char* d, *f;
	int capacity = 0;
	int field_number = 0;
	int enclosed_in_quotes = 0;
	int escaped = 0;
	int allocField = 1;
	int justAfterDelim = 1; 		// if set skip whitespace
	char** fields = header ? c->header_fields : c->fields;

	while (*p) {
		// realloc field array
		if (field_number == capacity) {
			int i;
			int size = (capacity + 10)*sizeof(char*);
			fields = capacity == 0 ? malloc(size) : realloc(fields, size);
			capacity += 10;
			for (i = field_number; i<capacity; i++) fields[i] = NULL;
		}
		// allocate field
		if (allocField) {
			allocField = 0;
			fields[field_number] = _strdup(p);
			f = d = fields[field_number];
		}
		if (enclosed_in_quotes) {
			if (*p == '"' && !escaped) {
				enclosed_in_quotes = 0;
			}
			else if (*p == '\\' && !escaped) {
				escaped = 1;
			}
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t')) {
					justAfterDelim = 0;
				}
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}
		else { // not in quotes
			if (*p == '"' && !escaped) {
				enclosed_in_quotes = 1;
			}
			else if (*p == c->delimiter && !escaped) {
				// terminate current field
				*d = 0;
				trim(f, d);
				// next field
				field_number++;
				allocField = 1;
				justAfterDelim = 1;
			}
			else if (*p == '\\' && !escaped) {
				escaped = 1;
			}
			else {
				if (justAfterDelim && (*p == ' ' || *p == '\t')) {
					justAfterDelim = 0;
				}
				else {
					*d++ = *p;	// copy char to target
					escaped = 0;
					justAfterDelim = 0;
				}
			}
		}

		p++;
	}

	*d = 0;
	trim(f, d);

	if (enclosed_in_quotes) return CSV_ERROR_LINE_FORMAT; // quote still open
	if (escaped) return CSV_ERROR_LINE_FORMAT; // esc still open

	if (header) {
		c->header_fields = fields;
		c->n_header_fields = field_number + 1;
	}
	else {
		c->fields = fields;
		c->n_fields = field_number + 1;
	}

	return CSV_SUCCESS;
}

static int csv_read_header(CsvReader* const c) {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, c->f);
	if (len < 0)
		return CSV_ERROR_HEADER_NOT_FOUND;

	// parse line and look for headers
	parse_line(c, buf, 1);

	return 0;
}

static int csv_get_colnumber_for_field(CsvReader* c, const char* fieldname) {
	int i;
	for (i = 0; i < c->n_header_fields; i++)
		if (strcmp(c->header_fields[i], fieldname) == 0)
			return i;

	return CSV_ERROR_NO_SUCH_FIELD;
}

static void free_record(CsvReader* const c) {
	int i;
	for (i = 0; i < c->n_fields; i++) {
		free(c->fields[i]);
	}
	if (c->n_fields) free(c->fields);
}

static int csv_get_int_field(CsvReader* const c, const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "%d", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static int csv_get_hex_field(CsvReader* const c, const int field_index, int* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "0x%x", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}


static int csv_get_float_field(CsvReader* const c, const int field_index, float* pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		if (sscanf(c->fields[field_index], "%f", pValue) == 1) return 0;
		return CSV_ERROR_LINE_FORMAT;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static int csv_get_str_field(CsvReader* const c, const int field_index, char** pValue) {
	if (field_index >= 0 && field_index < c->n_fields) {
		*pValue = c->fields[field_index];
		return 0;
	}

	return CSV_ERROR_FIELD_INDEX_OUT_OF_RANGE;
}

static void csv_close(CsvReader* const c) {
	if (c) {
		int i;
		free_record(c);
		for (i = 0; i < c->n_header_fields; i++) {
			if (c->header_fields[i]) free(c->header_fields[i]);
		}
		if (c->n_header_fields > 0) free(c->header_fields);
		if (c->f) fclose(c->f);
		free(c);
	}
}

static int csv_read_record(CsvReader* const c) {
	char buf[CSV_MAX_LINE_LENGTH];
	const int len = fgetline(buf, CSV_MAX_LINE_LENGTH, c->f);
	if (len < 0)
		return CSV_ERROR_NO_MORE_RECORDS;

	free_record(c);
	// parse line and look for headers
	parse_line(c, buf, 0);

	return 0;
}
