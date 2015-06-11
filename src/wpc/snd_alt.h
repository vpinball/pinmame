#include <dirent.h>
#include <unistd.h>
#include "..\ext\bass\bass.h"

struct pin_samples { // holds data for all sound files found
	char ** files_with_subpath;
	char *  gain;
	char *  ducking;
	unsigned int num_files;
};

const char* path_main = "\\altsound\\";
const char* path_jingle = "jingle\\";
const char* path_music = "music\\";
const char* path_sfx = "sfx\\";
const char* path_single = "single\\";
const char* path_voice = "voice\\";

static char* cached_machine_name = 0;

void alt_sound_handle(int boardNo, int cmd)
{
	if (TRUE) //!! only do search for dir as soon as sound is disabled? or even additional flag/interface call?
	{
		static unsigned int cmd_counter = 0;
		static unsigned int cmd_storage = -1;

		static unsigned int cmd_filter = 0;
#define ALT_MAX_CMDS 4
		static unsigned int cmd_buffer[ALT_MAX_CMDS] = { ~0, ~0, ~0, ~0 };

		static struct pin_samples psd;

		static HSTREAM jingle_stream = 0; // includes single_stream
		static HSTREAM music_stream = 0;
#define ALT_MAX_VOICES 16
		static HSTREAM voice_stream[ALT_MAX_VOICES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // includes sfx_stream (or must this be separated to only have 2 channels for sfx?)

		if (cached_machine_name != 0 && strstr(Machine->gamedrv->name, cached_machine_name) == 0)
		{
			cmd_counter = 0;
			cmd_storage = -1;

			cmd_filter = 0;
			for (unsigned int i = 0; i < ALT_MAX_CMDS; ++i)
				cmd_buffer[i] = ~0;

			free(cached_machine_name);
			cached_machine_name = 0;

			jingle_stream = 0;
			music_stream = 0;
			for (unsigned int i = 0; i < ALT_MAX_VOICES; ++i)
				voice_stream[i] = 0;

			BASS_Free();

			if (psd.num_files > 0)
			{
				for (unsigned int i = 0; i < psd.num_files; ++i)
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
			cached_machine_name = (char*)malloc(strlen(Machine->gamedrv->name) + 1);
			strcpy(cached_machine_name, Machine->gamedrv->name);

			char cwd[1024];
			getcwd(cwd, sizeof(cwd));

			//
			char cvpmd[1024];
			HINSTANCE hInst = GetModuleHandle("VPinMAME.dll");
			GetModuleFileName(hInst, cvpmd, 1024);

			char *lpHelp = cvpmd;
			char *lpSlash = NULL;
			while (*lpHelp) {
				if (*lpHelp == '\\')
					lpSlash = lpHelp;
				lpHelp++;
			}
			if (lpSlash)
				*lpSlash = '\0';
			//

			psd.num_files = 0;

			for (unsigned int i = 0; i < 5; ++i)
			{
				const char* subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));

				const unsigned int PATHl = strlen(cvpmd) + strlen(path_main) + strlen(Machine->gamedrv->name) + 1 + strlen(subpath) + 1;
				char* PATH = (char*)malloc(PATHl);
				strcpy_s(PATH, PATHl, cvpmd);
				strcat_s(PATH, PATHl, path_main);
				strcat_s(PATH, PATHl, Machine->gamedrv->name);
				strcat_s(PATH, PATHl, "\\");
				strcat_s(PATH, PATHl, subpath);

				DIR *dir = opendir(PATH);
				if (!dir)
				{
					free(PATH);
					continue;
				}

				struct dirent *entry = readdir(dir);
				while (entry != NULL)
				{
					if (entry->d_name[0] != '.' && strstr(entry->d_name, ".txt") == 0)
					{
						DIR backup_dir = *dir;
						struct dirent backup_entry = *entry;

						const unsigned int PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
						char* PATH2 = (char*)malloc(PATH2l);
						strcpy_s(PATH2, PATH2l, PATH);
						strcat_s(PATH2, PATH2l, entry->d_name);

						DIR *dir2 = opendir(PATH2);
						struct dirent *entry2 = readdir(dir2);
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
				psd.gain = (char*)malloc(psd.num_files*sizeof(char));
				psd.ducking = (char*)malloc(psd.num_files*sizeof(char));
				psd.num_files = 0;
			}
			else
				psd.files_with_subpath = NULL;

			for (unsigned int i = 0; i < 5; ++i)
			{
				const char* subpath = (i == 0) ? path_jingle : ((i == 1) ? path_music : ((i == 2) ? path_sfx : ((i == 3) ? path_single : path_voice)));

				const unsigned int PATHl = strlen(cvpmd) + strlen(path_main) + strlen(Machine->gamedrv->name) + 1 + strlen(subpath) + 1;
				char* PATH = (char*)malloc(PATHl);
				strcpy_s(PATH, PATHl, cvpmd);
				strcat_s(PATH, PATHl, path_main);
				strcat_s(PATH, PATHl, Machine->gamedrv->name);
				strcat_s(PATH, PATHl, "\\");
				strcat_s(PATH, PATHl, subpath);

				DIR *dir = opendir(PATH);
				if (!dir)
				{
					free(PATH);
					continue;
				}

				unsigned int default_gain = 10;
				unsigned int default_ducking = 0;

				{
					const unsigned int PATHGl = strlen(PATH) + strlen("gain.txt") + 1;
					char* PATHG = (char*)malloc(PATHGl);
					strcpy_s(PATHG, PATHGl, PATH);
					strcat_s(PATHG, PATHGl, "gain.txt");
					FILE *f = fopen(PATHG, "r");
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
				  strcpy_s(PATHG, PATHGl, PATH);
				  strcat_s(PATHG, PATHGl, "ducking.txt");
				  FILE *f = fopen(PATHG, "r");
				  if (f)
				  {
					  fscanf(f, "%u", &default_ducking);
					  fclose(f);
				  }
				  free(PATHG);
			  }

			  struct dirent *entry = readdir(dir);
			  while (entry != NULL)
			  {
				  if (entry->d_name[0] != '.' && strstr(entry->d_name, ".txt") == 0)
				  {
					  DIR backup_dir = *dir;
					  struct dirent backup_entry = *entry;

					  const unsigned int PATH2l = strlen(PATH) + strlen(entry->d_name) + 1;
					  char* PATH2 = (char*)malloc(PATH2l);
					  strcpy_s(PATH2, PATH2l, PATH);
					  strcat_s(PATH2, PATH2l, entry->d_name);

					  unsigned int gain = default_gain;
					  unsigned int ducking = default_ducking;

					  {
						  const unsigned int PATHGl = strlen(PATH2) + 1 + strlen("gain.txt") + 1;
						  char* PATHG = (char*)malloc(PATHGl);
						  strcpy_s(PATHG, PATHGl, PATH2);
						  strcat_s(PATHG, PATHGl, "\\");
						  strcat_s(PATHG, PATHGl, "gain.txt");
						  FILE *f = fopen(PATHG, "r");
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
						  strcpy_s(PATHG, PATHGl, PATH2);
						  strcat_s(PATHG, PATHGl, "\\");
						  strcat_s(PATHG, PATHGl, "ducking.txt");
						  FILE *f = fopen(PATHG, "r");
						  if (f)
						  {
							  fscanf(f, "%u", &ducking);
							  fclose(f);
						  }
						  free(PATHG);
					  }

					  DIR *dir2 = opendir(PATH2);
					  struct dirent *entry2 = readdir(dir2);
					  while (entry2 != NULL)
					  {
						  if (entry2->d_name[0] != '.' && strstr(entry2->d_name, ".txt") == 0)
						  {
							  const unsigned int PATH3l = strlen(PATH2) + 1 + strlen(entry2->d_name) + 1;

							  psd.files_with_subpath[psd.num_files] = (char*)malloc(PATH3l);
							  strcpy_s(psd.files_with_subpath[psd.num_files], PATH3l, PATH2);
							  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, "\\");
							  strcat_s(psd.files_with_subpath[psd.num_files], PATH3l, entry2->d_name);

							  psd.gain[psd.num_files] = min(gain, 20);
							  psd.ducking[psd.num_files] = min(ducking, 100);

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

#ifdef ALT_LOG
			FILE* f = fopen("C:\\Pinmame\\bla.txt", "a");
			for (unsigned int i = 0; i < psd.num_files; ++i)
			    fprintf(f, "%s\n", psd.files_with_subpath[i]);
			fclose(f);
#endif

			//
			DIR* dir = opendir(cwd);
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
			}
			else
				cmd_storage = 0;
		}

		if (psd.num_files > 0)
		{
			cmd_counter++;

			for (unsigned int i = ALT_MAX_CMDS - 1; i > 0; --i)
				cmd_buffer[i] = cmd_buffer[i - 1];
			cmd_buffer[0] = cmd;

			if ((core_gameData->gen == GEN_WPCDCS) ||
				(core_gameData->gen == GEN_WPCSECURITY) ||
				(core_gameData->gen == GEN_WPC95DCS) ||
				(core_gameData->gen == GEN_WPC95))
			{
				if (((cmd_buffer[3] == 0x55) && (cmd_buffer[2] == 0xAA) && (cmd_buffer[1] == 0x00) && (cmd_buffer[0] == 0xFF))
					||
					((cmd_buffer[2] == 0x00) && (cmd_buffer[1] == 0x00) && (cmd_buffer[0] == 0x00)))
				{
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
				for (unsigned int i = 0; i < psd.num_files; ++i)
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
#ifdef ALT_LOG
				FILE* f = fopen("C:\\Pinmame\\bla_names.txt", "a");
#endif
				if (idx != -1)
				{
					if ((strstr(psd.files_with_subpath[idx], path_jingle) != 0) || (strstr(psd.files_with_subpath[idx], path_single) != 0))
					{
#ifdef ALT_LOG
						if ((strstr(psd.files_with_subpath[idx], path_jingle) != 0))
							fprintf(f, "%04X %s jingle %u %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
						else
							fprintf(f, "%04X %s single %u %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						if (music_stream != 0)
						{
							if ((strstr(psd.files_with_subpath[idx], path_jingle) != 0))
								BASS_ChannelPause(music_stream);
							else
							{
								BASS_ChannelStop(music_stream);
								BASS_StreamFree(music_stream);
								music_stream = 0;
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

						const float volf = (float)(psd.gain[idx] - 0)*(float)(1.0 / (20 - 0)); //!!
						BASS_ChannelSetAttribute(jingle_stream, BASS_ATTRIB_VOL, volf);

						BASS_ChannelPlay(jingle_stream, 0);
					}

					if (strstr(psd.files_with_subpath[idx], path_music) != 0)
					{
#ifdef ALT_LOG
						fprintf(f, "%04X %s music %u %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						if (music_stream != 0)
						{
							BASS_ChannelStop(music_stream);
							BASS_StreamFree(music_stream);
							music_stream = 0;
						}

						music_stream = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, BASS_SAMPLE_LOOP);
						if (music_stream == 0)
						{
							//sprintf_s(bla, "BASS music/sound library cannot load %s", psd.files_with_subpath[idx]);
						}

						const float volf = (float)(psd.gain[idx] - 0)*(float)(1.0 / (20 - 0)); //!!
						BASS_ChannelSetAttribute(music_stream, BASS_ATTRIB_VOL, volf);

						BASS_ChannelPlay(music_stream, 0);
					}

					if ((strstr(psd.files_with_subpath[idx], path_voice) != 0) || (strstr(psd.files_with_subpath[idx], path_sfx) != 0))
					{
#ifdef ALT_LOG
						if (strstr(psd.files_with_subpath[idx], path_voice) != 0)
							fprintf(f, "%04X %s voice %u %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
						else
							fprintf(f, "%04X %s sfx %u %u\n", cmd_combined, psd.files_with_subpath[idx], psd.gain[idx], psd.ducking[idx]);
#endif
						//

						unsigned int voice_idx = -1;
						for (unsigned int i = 0; i < ALT_MAX_VOICES; ++i)
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
							voice_stream[voice_idx] = BASS_StreamCreateFile(FALSE, psd.files_with_subpath[idx], 0, 0, 0);
							if (voice_stream[voice_idx] == 0)
							{
								//sprintf_s(bla, "BASS music/sound library cannot load %s", psd.files_with_subpath[idx]);
							}

							const float volf = (float)(psd.gain[idx] - 0)*(float)(1.0 / (20 - 0)); //!!
							BASS_ChannelSetAttribute(voice_stream[idx], BASS_ATTRIB_VOL, volf);

							BASS_ChannelPlay(voice_stream[voice_idx], 0);
						}
					}
				}

				//!! implement ducking!
				//!! gain=10 is normallevel not gain=20 (but then it would also be clipped, so meh)

				{
#ifdef ALT_LOG
					if (idx == -1)
						fprintf(f, "%04X unknown %u %u\n", cmd_combined, locals.boards, boardNo);
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
						}
					}

					for (unsigned int i = 0; i < ALT_MAX_VOICES; ++i) // clean up already finished voices
						if (voice_stream[i] != 0 && BASS_ChannelIsActive(voice_stream[i]) != BASS_ACTIVE_PLAYING)
						{
							BASS_StreamFree(voice_stream[i]);
							voice_stream[i] = 0;
						}

					// if jingle stopped, continue with music
					if (jingle_stream != 0 && BASS_ChannelIsActive(jingle_stream) != BASS_ACTIVE_PLAYING)
					{
						BASS_StreamFree(jingle_stream);
						jingle_stream = 0;

						if (music_stream != 0)
							BASS_ChannelPlay(music_stream, 0);
					}
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
