#include <dirent.h>
#include <unistd.h>
#include "..\ext\bass\bass.h"

//#define ALT_LOG

#define VERBOSE 0

#if VERBOSE
 #define LOG(x) logerror x
#else
 #define LOG(x)
#endif


typedef struct pin_samples { // holds data for all sound files found
	char ** files_with_subpath;
	float * gain;
	signed char * ducking;
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

static HSTREAM jingle_stream = 0; // includes single_stream
static HSTREAM music_stream = 0;
#define ALT_MAX_VOICES 16
static HSTREAM voice_stream[ALT_MAX_VOICES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // includes sfx_stream (or must this be separated to only have 2 channels for sfx?)

int open_altsound_table(char* filename, Pin_samples* psd);

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
		static float music_vol = 1.0f;
		static signed char jingle_ducking = -1;
		static signed char voice_ducking[ALT_MAX_VOICES] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
		unsigned int i;

		int	attenuation = osd_get_mastervolume();
		float master_vol = 1.0f;
		while (attenuation++ < 0)
			master_vol /= 1.122018454f; // = (10 ^ (1/20)) = 1dB

		if (cached_machine_name != 0 && strstr(Machine->gamedrv->name, cached_machine_name) == 0) // another game has been loaded? -> previous data has to be free'd
		{
			cmd_counter = 0;
			cmd_storage = -1;

			cmd_filter = 0;
			for (i = 0; i < ALT_MAX_CMDS; ++i)
				cmd_buffer[i] = ~0;

			free(cached_machine_name);
			cached_machine_name = 0;

			jingle_stream = 0;
			music_stream = 0;
			for (i = 0; i < ALT_MAX_VOICES; ++i)
				voice_stream[i] = 0;

			global_vol = 1.0f;
			music_vol = 1.0f;
			jingle_ducking = -1;
			for (i = 0; i < ALT_MAX_VOICES; ++i)
				voice_ducking[i] = -1;

			BASS_Free();

			if (psd.num_files > 0)
			{
				for (i = 0; i < psd.num_files; ++i)
				{
					free(psd.files_with_subpath[i]);
					psd.files_with_subpath[i] = 0;
				}
				free(psd.files_with_subpath);
				psd.files_with_subpath = 0;
				free(psd.gain);
				psd.gain = 0;
				free(psd.ducking);
				psd.ducking = 0;
			}
		}

		if (cmd_storage == -1)
		{
			char cwd[1024];
			char cvpmd[1024];
			HINSTANCE hInst;
			char *lpHelp = cvpmd;
			char *lpSlash = NULL;
			DIR* dir;

			cached_machine_name = (char*)malloc(strlen(Machine->gamedrv->name) + 1);
			strcpy(cached_machine_name, Machine->gamedrv->name);

			getcwd(cwd, sizeof(cwd));

#ifndef _WIN64
			hInst = GetModuleHandle("VPinMAME.dll");
#else
			hInst = GetModuleHandle("VPinMAME64.dll");
#endif
			GetModuleFileName(hInst, cvpmd, 1024);

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
			const unsigned int PATH_LEN = strlen(cvpmd) + strlen(path_main) + strlen(Machine->gamedrv->name) + 1 + strlen(path_table) + 1;
			char* PATH = (char*)malloc(PATH_LEN);
			strcpy_s(PATH, PATH_LEN, cvpmd);
			strcat_s(PATH, PATH_LEN, path_main);
			strcat_s(PATH, PATH_LEN, Machine->gamedrv->name);
			strcat_s(PATH, PATH_LEN, path_table);

			psd.num_files = open_altsound_table(PATH, &psd);

			free(PATH);

			// OTHERWISE scan folders for old folder-based structure of the alternate sound package
			if (psd.num_files == 0) { 
				for (i = 0; i < 5; ++i)
				{
					const char* subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));

					const unsigned int PATHl = strlen(cvpmd) + strlen(path_main) + strlen(Machine->gamedrv->name) + 1 + strlen(subpath) + 1;
					PATH = (char*)malloc(PATHl);
					DIR *dir;
					struct dirent *entry;

					strcpy_s(PATH, PATHl, cvpmd);
					strcat_s(PATH, PATHl, path_main);
					strcat_s(PATH, PATHl, Machine->gamedrv->name);
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
							DIR backup_dir = *dir;
							DIR *dir2;
							struct dirent backup_entry = *entry;
							struct dirent *entry2;

							const unsigned int PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
							char* PATH2 = (char*)malloc(PATH2l);
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
					psd.files_with_subpath = (char**)malloc(psd.num_files*sizeof(char*));
					psd.gain = (float*)malloc(psd.num_files*sizeof(float));
					psd.ducking = (signed char*)malloc(psd.num_files*sizeof(signed char));
					psd.num_files = 0;
				}
				else
					psd.files_with_subpath = NULL;

				for (i = 0; i < 5; ++i)
				{
					const char* subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));
					const unsigned int PATHl = strlen(cvpmd) + strlen(path_main) + strlen(Machine->gamedrv->name) + 1 + strlen(subpath) + 1;
					char* PATH = (char*)malloc(PATHl);
					DIR *dir;
					unsigned int default_gain = 10;
					int default_ducking = -1; //!! default depends on type??
					struct dirent *entry;

					strcpy_s(PATH, PATHl, cvpmd);
					strcat_s(PATH, PATHl, path_main);
					strcat_s(PATH, PATHl, Machine->gamedrv->name);
					strcat_s(PATH, PATHl, "\\");
					strcat_s(PATH, PATHl, subpath);

					dir = opendir(PATH);
					if (!dir)
					{
						free(PATH);
						continue;
					}
					{
						const unsigned int PATHGl = strlen(PATH) + strlen("gain.txt") + 1;
						char* PATHG = (char*)malloc(PATHGl);
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
						const unsigned int PATHGl = strlen(PATH) + strlen("ducking.txt") + 1;
						char* PATHG = (char*)malloc(PATHGl);
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
							DIR backup_dir = *dir;
							struct dirent backup_entry = *entry;

							const unsigned int PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
							char* PATH2 = (char*)malloc(PATH2l);
							unsigned int gain = default_gain;
							int ducking = default_ducking;
							DIR *dir2;
							struct dirent *entry2;

							strcpy_s(PATH2, PATH2l, PATH);
							strcat_s(PATH2, PATH2l, entry->d_name);

							{
								const unsigned int PATHGl = strlen(PATH2) + 1 + strlen("gain.txt") + 1;
								char* PATHG = (char*)malloc(PATHGl);
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
							  const unsigned int PATHGl = strlen(PATH2) + 1 + strlen("ducking.txt") + 1;
							  char* PATHG = (char*)malloc(PATHGl);
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
								  const unsigned int PATH3l = strlen(PATH2) + 1 + strlen(entry2->d_name) + 1;

								  psd.files_with_subpath[psd.num_files] = (char*)malloc(PATH3l);
								  strcpy_s(psd.files_with_subpath[psd.num_files], PATH3l, PATH2);
								  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, "\\");
								  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, entry2->d_name);

								  psd.gain[psd.num_files] = alt_sound_gain(gain);
								  psd.ducking[psd.num_files] = min(ducking, (int)100);

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
			}
#ifdef ALT_LOG
			FILE* f = fopen("C:\\Pinmame\\altsound_files.txt", "a");
			for (unsigned int i = 0; i < psd.num_files; ++i)
			    fprintf(f, "%s\n", psd.files_with_subpath[i]);
			fclose(f);
#endif

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

#ifdef ALT_LOG
			FILE* f = fopen("C:\\Pinmame\\altsound_commands.txt", "a");
#endif

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
						if (music_stream != 0)
							BASS_ChannelSetAttribute(music_stream, BASS_ATTRIB_VOL, music_vol * global_vol * master_vol);
#ifdef ALT_LOG
						fprintf(f, "change volume %.2f\n", global_vol);
#endif
					}
#ifdef ALT_LOG
					else
						fprintf(f, "filtered command %02X %02X %02X %02X\n", cmd_buffer[3], cmd_buffer[2], cmd_buffer[1], cmd_buffer[0]);
#endif
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

			if (!cmd_filter && (cmd_counter & 1) == 0) // collect 16bits from two 8bit commands
			{
				unsigned int cmd_combined = (cmd_storage << 8) | cmd;

				unsigned int idx = -1;
				char cmd_str[9];
				sprintf(cmd_str, "\\%06u-", cmd_combined);
				for (i = 0; i < psd.num_files; ++i)
					if (strstr(psd.files_with_subpath[i], cmd_str) != 0)
					{
						// check if more samples are there for this command and randomly pick one
						unsigned int rnd = 0;
						do
						{
							rnd++;
							if (i + rnd >= psd.num_files)
								break;
						} while (strstr(psd.files_with_subpath[i + rnd], cmd_str) != 0);

						idx = i + rand() % rnd;
						break;
					}

				if (idx != -1)
				{
					if ((strstr(psd.files_with_subpath[idx], path_jingle) != 0) || (strstr(psd.files_with_subpath[idx], path_single) != 0))
					{
#ifdef ALT_LOG
						if ((strstr(psd.files_with_subpath[idx], path_jingle) != 0))
							fprintf(f, "%04X %s jingle %.2f %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
						else
							fprintf(f, "%04X %s single %.2f %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						jingle_ducking = -1;

						if (music_stream != 0)
						{
							if (strstr(psd.files_with_subpath[idx], path_jingle) != 0)
							{
								if (psd.ducking[idx] < 0)
									BASS_ChannelPause(music_stream);
								else
									jingle_ducking = psd.ducking[idx];
							}
							else
							{
								BASS_ChannelStop(music_stream);
								BASS_StreamFree(music_stream);
								music_stream = 0;
								music_vol = 1.0f;
							}
						}

						//

						if (jingle_stream != 0)
						{
							BASS_ChannelStop(jingle_stream);
							BASS_StreamFree(jingle_stream);
							jingle_stream = 0;
						}

						jingle_stream = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, 0);
						if (jingle_stream == 0)
						{
							//sprintf_s(bla, "BASS music/sound library cannot load %s", psd.files_with_subpath[idx]);
						}
						else
						{
							BASS_ChannelSetAttribute(jingle_stream, BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);

							BASS_ChannelPlay(jingle_stream, 0);
						}
					}

					if (strstr(psd.files_with_subpath[idx], path_music) != 0)
					{
#ifdef ALT_LOG
						fprintf(f, "%04X %s music %.2f %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						if (music_stream != 0)
						{
							BASS_ChannelStop(music_stream);
							BASS_StreamFree(music_stream);
							music_stream = 0;
							music_vol = 1.0f;
						}

						music_stream = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, BASS_SAMPLE_LOOP);
						if (music_stream == 0)
						{
							//sprintf_s(bla, "BASS music/sound library cannot load %s", psd.files_with_subpath[idx]);
						}
						else
						{
							music_vol = psd.gain[idx];
							BASS_ChannelSetAttribute(music_stream, BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);

							BASS_ChannelPlay(music_stream, 0);
						}
					}

					if ((strstr(psd.files_with_subpath[idx], path_voice) != 0) || (strstr(psd.files_with_subpath[idx], path_sfx) != 0))
					{
#ifdef ALT_LOG
						if (strstr(psd.files_with_subpath[idx], path_voice) != 0)
							fprintf(f, "%04X %s voice %.2f %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
						else
							fprintf(f, "%04X %s sfx %.2f %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						unsigned int voice_idx = -1;
						for (i = 0; i < ALT_MAX_VOICES; ++i)
							if (voice_stream[i] == 0 || BASS_ChannelIsActive(voice_stream[i]) != BASS_ACTIVE_PLAYING)
							{
								if (voice_stream[i] != 0)
								{
									BASS_StreamFree(voice_stream[i]);
									voice_stream[i] = 0;
								}

								voice_idx = i;
								break;
							}

						/*if (voice_idx == -1) //!! kill off the longest playing channel as f.e. LAH has helicopter sound that isn't stopped it seems!
						{
							double max_pos = 0;
							for (unsigned int i = 0; i < ALT_MAX_VOICES; ++i)
							{
								const double pos = BASS_ChannelBytes2Seconds(voice_stream[voice_idx], BASS_ChannelGetPosition(voice_stream[voice_idx], BASS_POS_BYTE));
								if (pos > max_pos)
								{
									max_pos = pos;
									voice_idx = i;
								}
							}

							BASS_ChannelStop(voice_stream[voice_idx]);
							BASS_StreamFree(voice_stream[voice_idx]);
							voice_stream[voice_idx] = 0;
						}*/

						if (voice_idx != -1)
						{
							voice_ducking[voice_idx] = psd.ducking[idx];

							voice_stream[voice_idx] = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, 0);
							if (voice_stream[voice_idx] == 0)
							{
								//sprintf_s(bla, "BASS music/sound library cannot load %s", psd.files_with_subpath[idx]);
							}
							else
							{
								BASS_ChannelSetAttribute(voice_stream[voice_idx], BASS_ATTRIB_VOL, psd.gain[idx] * global_vol * master_vol);

								BASS_ChannelPlay(voice_stream[voice_idx], 0);
							}
						}
					}
				}

				//!! gain=10 is normallevel not gain=20 (but then it would also be clipped, so meh)

				{
#ifdef ALT_LOG
					if (idx == -1)
						fprintf(f, "%04X unknown %u\n", cmd_combined, boardNo);
#endif
					if ((core_gameData->gen == GEN_WPCDCS) ||
						(core_gameData->gen == GEN_WPCSECURITY) ||
						(core_gameData->gen == GEN_WPC95DCS) ||
						(core_gameData->gen == GEN_WPC95))
					{
						if (cmd_combined == 0x03E3 && music_stream != 0) // stop music
						{
							BASS_ChannelStop(music_stream);
							BASS_StreamFree(music_stream);
							music_stream = 0;
							music_vol = 1.0f;
						}
					}

					//!! old WPC machines music stop? -> 0x00 for SYS11?

					if (core_gameData->gen == GEN_DEDMD32)
					{
						if ((cmd_combined == 0x0018 || cmd_combined == 0x0023) && music_stream != 0) // stop music //!! ???? 0x0019??
						{
							BASS_ChannelStop(music_stream);
							BASS_StreamFree(music_stream);
							music_stream = 0;
							music_vol = 1.0f;
						}
					}

					//
					//
					//

					for (i = 0; i < ALT_MAX_VOICES; ++i) // clean up already finished voices
						if (voice_stream[i] != 0 && BASS_ChannelIsActive(voice_stream[i]) != BASS_ACTIVE_PLAYING)
						{
							BASS_StreamFree(voice_stream[i]);
							voice_stream[i] = 0;
							voice_ducking[i] = -1;
						}

					// if jingle stopped, continue with music
					if (jingle_stream != 0 && BASS_ChannelIsActive(jingle_stream) != BASS_ACTIVE_PLAYING)
					{
						BASS_StreamFree(jingle_stream);
						jingle_stream = 0;
						jingle_ducking = -1;

						if (music_stream != 0)
							BASS_ChannelPlay(music_stream, 0);
					}

#if 0 // needs callback/timer to function properly!
					if (music_stream != 0)
					{
						signed char min_ducking = 100;
						if (jingle_stream != 0 && jingle_ducking >= 0 && jingle_ducking < min_ducking)
							min_ducking = jingle_ducking;
						for (unsigned int i = 0; i < ALT_MAX_VOICES; ++i)
							if (voice_stream != 0 && voice_ducking[i] >= 0 && voice_ducking[i] < min_ducking)
								min_ducking = voice_ducking[i];

						float new_val = music_vol*((double)min_ducking / 100.);
						if (music_vol != new_val)
							BASS_ChannelSetAttribute(music_stream, BASS_ATTRIB_VOL, new_val);
					}
#endif
				}

#ifdef ALT_LOG
				fclose(f);
#endif
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
		for (i = 0; i < ALT_MAX_VOICES; ++i)
			if (voice_stream[i] != 0 && BASS_ChannelIsActive(voice_stream[i]) == BASS_ACTIVE_PLAYING)
				BASS_ChannelPause(voice_stream[i]);

		if (jingle_stream != 0 && BASS_ChannelIsActive(jingle_stream) == BASS_ACTIVE_PLAYING)
			BASS_ChannelPause(jingle_stream);

		if (music_stream != 0 && BASS_ChannelIsActive(music_stream) == BASS_ACTIVE_PLAYING)
			BASS_ChannelPause(music_stream);
	}
	else
	{
		for (i = 0; i < ALT_MAX_VOICES; ++i)
			if (voice_stream[i] != 0 && BASS_ChannelIsActive(voice_stream[i]) == BASS_ACTIVE_PAUSED)
				BASS_ChannelPlay(voice_stream[i],0);

		if (jingle_stream != 0 && BASS_ChannelIsActive(jingle_stream) == BASS_ACTIVE_PAUSED)
			BASS_ChannelPlay(jingle_stream,0);

		if (music_stream != 0 && BASS_ChannelIsActive(music_stream) == BASS_ACTIVE_PAUSED)
			BASS_ChannelPlay(music_stream,0);
	}
}

//
// CSV parsing
//

/**
* init struct and open file
*/
static CsvReader* csv_open(const char* filename, const int delimiter) {
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
		memcpy(buff, buff + 1, nchars - 1);

	length = strlen(buff);
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
			int size = (capacity + 10)*sizeof(char*);
			fields = capacity == 0 ? malloc(size) : realloc(fields, size);
			capacity += 10;
		}
		// allocate field
		if (allocField) {
			allocField = 0;
			fields[field_number] = strdup(p);
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
	char* const buf = (char*)malloc(CSV_MAX_LINE_LENGTH);
	size_t size = 0;
	const int len = fgetline(buf, &size, c->f);
	if (len < 0) {
		if (buf) free(buf);
		return CSV_ERROR_HEADER_NOT_FOUND;
	}

	// parse line and look for headers
	parse_line(c, buf, 1);
	if (buf) free(buf);

	return 0;
}

static int csv_get_colnumber_for_field(CsvReader* c, const char* fieldname) {
	int i;
	for (i = 0; i < c->n_header_fields; i++) {
		if (strcmp(c->header_fields[i], fieldname) == 0) return i;
	}

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
	char* const buf = (char*)malloc(CSV_MAX_LINE_LENGTH);
	size_t size = 0;
	const int len = fgetline(buf, &size, c->f);
	if (len < 0) {
		if (buf) free(buf);
		return CSV_ERROR_NO_MORE_RECORDS;
	}

	free_record(c);
	// parse line and look for headers
	parse_line(c, buf, 0);
	if (buf) free(buf);

	return 0;
}

static int open_altsound_table(char* const filename, Pin_samples* const psd) {
	CsvReader* const c = csv_open(filename, ',');
	if (c) {
		int i;
		csv_read_header(c);
		LOG(("n_headers: %d\n", c->n_header_fields));
		for (i = 0; i< c->n_header_fields; i++) {
			LOG(("header[%d]: '%s'\n", i, c->header_fields[i]));
		}
		{
		int colID = csv_get_colnumber_for_field(c, "ID");
		int colBACKGND = csv_get_colnumber_for_field(c, "BACKGND");
		int colDUCK = csv_get_colnumber_for_field(c, "DUCK");
		int colGAIN = csv_get_colnumber_for_field(c, "GAIN");
		int colLOOP = csv_get_colnumber_for_field(c, "LOOP");
		int colSTOP = csv_get_colnumber_for_field(c, "STOP");
		int colFNAME = csv_get_colnumber_for_field(c, "FNAME");

		int row = 0;
		while (csv_read_record(c) == 0) {
			int val = 0;
			csv_get_hex_field(c, colID, &val);
			LOG(("ID = %d, ", val));
			val = 0;
			csv_get_int_field(c, colBACKGND, &val);
			LOG(("BACKGND = %d, ", val));
			val = 0;
			csv_get_int_field(c, colDUCK, &val);
			LOG(("DUCK = %d, ", val));
			val = 0;
			csv_get_int_field(c, colGAIN, &val);
			LOG(("GAIN = %d, ", val));
			val = 0;
			csv_get_int_field(c, colLOOP, &val);
			LOG(("LOOP = %d, ", val));
			val = 0;
			csv_get_int_field(c, colSTOP, &val);
			LOG(("STOP = %d, ", val));
			LOG(("FNAME = '%s'\n", c->fields[colFNAME]));
			row++;
		}
		}

		csv_close(c);
		return row;
	}
	return 0;
}
