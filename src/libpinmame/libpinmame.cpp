// license:BSD-3-Clause

#include "libpinmame.h"
#include <thread>

extern "C" {
#include "stdio.h"
#include "driver.h"
#include "core.h"
#include "vpintf.h"
#include "mame.h"
#include "video.h"
#include "sound.h"
#include "config.h"
#include "rc.h"

extern UINT32 g_raw_dmdx;
extern UINT32 g_raw_dmdy;
extern UINT32 g_needs_DMD_update;
extern unsigned char g_raw_dmdbuffer[DMD_MAXY * DMD_MAXX];
extern int channels;
struct rc_struct *rc;

int g_fHandleKeyboard = 0;
int g_fHandleMechanics = 0;
int g_fDumpFrames = 0;
int g_fPause = 0;

#ifdef VPINMAME_ALTSOUND
char g_szGameName[256] = { 0 }; // String containing requested game name (may be different from ROM if aliased)
#endif
}

static PinmameConfig _config = {
	48000,
	"",
	0,
	0,
	0
};

static int _isRunning = 0;
static int _timeToQuit = 0;
static UINT8 _frame[DMD_MAXY * DMD_MAXX];

static std::thread* _p_gameThread = nullptr;

/******************************************************
 * osd_init
 ******************************************************/

extern "C" int osd_init(void) {
	return 0;
}

/******************************************************
 * osd_exit
 ******************************************************/

extern "C" void osd_exit(void) {
}

/******************************************************
 * libpinmame_time_to_quit
 ******************************************************/

extern "C" int libpinmame_time_to_quit(void) {
	return _timeToQuit;
}

/******************************************************
 * libpinmame_update_displays
 ******************************************************/

extern "C" void libpinmame_update_displays(const struct core_dispLayout* p_layout, int* p_index, int* p_lastOffset) {
	if (!_config.cb_OnDisplayUpdate) {
		return;	
	}

	for (; p_layout->length; p_layout += 1) {
		if (p_layout->type == CORE_IMPORT) {
			libpinmame_update_displays(p_layout->lptr, p_index, p_lastOffset);
			continue;
		}
		else {
			PinmameDisplayLayout displayLayout;
			memset(&displayLayout, 0, sizeof(PinmameDisplayLayout));
			displayLayout.type = (PINMAME_DISPLAY_TYPE)p_layout->type;
			displayLayout.top = p_layout->top;
			displayLayout.left = p_layout->left;

			if (p_layout->type & CORE_DMD) {
				if(g_needs_DMD_update && (int)g_raw_dmdx > 0 && (int)g_raw_dmdy > 0) {
					displayLayout.height = g_raw_dmdy;
					displayLayout.width = g_raw_dmdx;

					const int shade_16_enabled = ((core_gameData->gen & (GEN_SAM|GEN_SPA|GEN_ALVG_DMD2)) ||
					  (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) || (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0));

					displayLayout.depth = shade_16_enabled ? 4 : 2;

					memcpy(_frame, g_raw_dmdbuffer, (g_raw_dmdx * g_raw_dmdy) * sizeof(unsigned char));

					(*(_config.cb_OnDisplayUpdate))(*p_index, _frame, &displayLayout);

					g_needs_DMD_update = 0; 
				}
			}
			else {
				displayLayout.length = p_layout->length;

				UINT16* p_drawSeg = coreGlobals.drawSeg;
				p_drawSeg += *p_lastOffset;

				memcpy(_frame, p_drawSeg, p_layout->length * sizeof(UINT16));

				*(p_lastOffset) += p_layout->length;

				(*(_config.cb_OnDisplayUpdate))(*p_index, _frame, &displayLayout);
			}

			(*p_index)++;
		}
	}
}

/******************************************************
 * ComposePath
 ******************************************************/

char* ComposePath(const char* path, const char* file) {
	size_t pathLength = strlen(path);
	size_t fileLength = strlen(file);
	char* newPath = (char*)malloc(pathLength + fileLength + 4);

	strcpy(newPath, path);
	strcpy(newPath + pathLength, file);
	return newPath;
}

/******************************************************
 * GetGameNumFromString
 ******************************************************/

int GetGameNumFromString(const char* const name) {
	int gameNum = 0;

	while (drivers[gameNum]) {
		if (!strcasecmp(drivers[gameNum]->name, name)) {
			break;
		}
		gameNum++;
	}

	if (!drivers[gameNum]) {
		return -1;
	}

	return gameNum;
}

/******************************************************
 * OnStateChange
 ******************************************************/

extern "C" void OnStateChange(int state) {
	_isRunning = state;

	if (_config.cb_OnStateChange) {
		(*(_config.cb_OnStateChange))(state);
	}
}

/******************************************************
 * OnSolenoid
 ******************************************************/

extern "C" void OnSolenoid(int solenoid, int isActive) {
	if (_config.cb_OnSolenoid) {
		(*(_config.cb_OnSolenoid))(solenoid, isActive);
	}
}

/******************************************************
 * StartGame
 ******************************************************/

void StartGame(int gameNum) {
	run_game(gameNum);

	OnStateChange(0);
}

/******************************************************
 * PinmameGetGames
 ******************************************************/

LIBPINMAME_API void PinmameGetGames(PinmameGameCallback callback) {
	int gameNum = 0;

	while (drivers[gameNum]) {
		PinmameGame game;
		memset(&game, 0, sizeof(PinmameGame));

		game.name = drivers[gameNum]->name;
		if (drivers[gameNum]->clone_of) {
			game.clone_of = drivers[gameNum]->clone_of->name;
		}

		game.description = drivers[gameNum]->description;
		game.year = drivers[gameNum]->year;
		game.manufacturer = drivers[gameNum]->manufacturer;

		if (callback) {
			(*callback)(&game);
		}

		gameNum++;
	}
}

/******************************************************
 * PinmameSetConfig
 ******************************************************/

LIBPINMAME_API void PinmameSetConfig(PinmameConfig* p_config) {
	memcpy(&_config, p_config, sizeof(PinmameConfig));

	fprintf(stdout, "PinmameSetConfig(): sampleRate=%d, vpmPath=%s\n", _config.sampleRate, _config.vpmPath);
}

/******************************************************
 * PinmameRun
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameRun(const char* p_name) {
	if (_isRunning) {
		return GAME_ALREADY_RUNNING;
	}

	int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0) {
		return GAME_NOT_FOUND;
	}

#ifdef VPINMAME_ALTSOUND
	strcpy_s(g_szGameName, p_name);
#endif

	rc = cli_rc_create();

	rc_set_option(rc, "throttle", "1", 0);
	rc_set_option(rc, "sleep", "1", 0);
	rc_set_option(rc, "autoframeskip", "0", 0);
	rc_set_option(rc, "skip_gameinfo", "1", 0);
	rc_set_option(rc, "skip_disclaimer", "1", 0);

	setPath(FILETYPE_ROM, ComposePath(_config.vpmPath, "roms"));
	setPath(FILETYPE_NVRAM, ComposePath(_config.vpmPath, "nvram"));
	setPath(FILETYPE_SAMPLE, ComposePath(_config.vpmPath, "samples"));
	setPath(FILETYPE_CONFIG, ComposePath(_config.vpmPath, "cfg"));
	setPath(FILETYPE_HIGHSCORE, ComposePath(_config.vpmPath, "hi"));
	setPath(FILETYPE_INPUTLOG, ComposePath(_config.vpmPath, "inp"));
	setPath(FILETYPE_MEMCARD, ComposePath(_config.vpmPath, "memcard"));
	setPath(FILETYPE_STATE, ComposePath(_config.vpmPath, "sta"));

	vp_init();

	_p_gameThread = new std::thread(StartGame, gameNum);

	return OK;
}

/******************************************************
 * PinmameIsRunning
 ******************************************************/

LIBPINMAME_API int PinmameIsRunning() {
	return _isRunning;
}

/******************************************************
 * PinmameReset
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameReset() {
	if (_isRunning) {
		machine_reset();

		return OK;
	}

	return EMULATOR_NOT_RUNNING;
}

/******************************************************
 * PinmamePause
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmamePause(int pause) {
	if (_isRunning) {
		g_fPause = pause;

		return OK;
	}

	return EMULATOR_NOT_RUNNING;
}

/******************************************************
 * PinmameStop
 ******************************************************/

LIBPINMAME_API void PinmameStop() {
	if (_p_gameThread) {
		_timeToQuit = 1;

		_p_gameThread->join();

		delete(_p_gameThread);
		_p_gameThread = nullptr;

#ifdef VPINMAME_ALTSOUND
		g_szGameName[0] = '\0';
#endif

		_timeToQuit = 0;
	}
}

/******************************************************
 * PinmameGetHardwareGen
 ******************************************************/

LIBPINMAME_API PINMAME_HARDWARE_GEN PinmameGetHardwareGen() {
	UINT64 hardwareGen = (_isRunning) ? core_gameData->gen : 0;
	return (PINMAME_HARDWARE_GEN)hardwareGen;
}

/******************************************************
 * PinmameGetSwitch
 ******************************************************/

LIBPINMAME_API int PinmameGetSwitch(int slot) {
	return (_isRunning) ? vp_getSwitch(slot) : 0;
}

/******************************************************
 * PinmameSetSwitch
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitch(int slot, int state) {
	if (_isRunning) {
		 vp_putSwitch(slot, state ? 1 : 0);
	}
}

/******************************************************
 * PinmameSetSwitches
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitches(int* p_states, int numSwitches) {
	if (_isRunning) {
		for (int i = 0; i < numSwitches; ++i) {
			vp_putSwitch(p_states[i*2], p_states[i*2+1] ? 1 : 0);
		}
	}
}

/******************************************************
 * PinmameGetMaxLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxLamps() {
	return CORE_MAXLAMPCOL * 8;
}

/******************************************************
 * PinmameGetChangedLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedLamps(int* p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgLamps chgLamps;
	const int count = vp_getChangedLamps(chgLamps);
	if (count == 0) {
		return 0;
	}

	int* p_out = p_changedStates;
	for (int i = 0; i < count; i++) {
		*(p_out++) = chgLamps[i].lampNo;
		*(p_out++) = chgLamps[i].currStat;
	}

	return count;
}

/******************************************************
 * PinmameGetMaxGIs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxGIs() {
	return CORE_MAXGI;
}

/******************************************************
 * PinmameGetChangedGIs
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedGIs(int* p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgGIs chgGIs;
	const int count = vp_getChangedGI(chgGIs);
	if (count == 0) {
		return 0;
	}

	int* out = p_changedStates;
	for (int i = 0; i < count; i++) {
		*(out++) = chgGIs[i].giNo;
		*(out++) = chgGIs[i].currStat;
	}
	return count;
}

/******************************************************
 * PinmameGetAudioChannels
 ******************************************************/

LIBPINMAME_API int PinmameGetAudioChannels() {
	return (_isRunning) ? channels : -1;
}

/******************************************************
 * PinmameGetAudioChannels
 ******************************************************/

LIBPINMAME_API int PinmameGetPendingAudioSamples(float* buffer, int outChannels, int maxNumber) {
	return (_isRunning) ? fillAudioBuffer(buffer, outChannels, maxNumber, 1) : -1;
}

/******************************************************
 * GetPendingAudioSamples16bit
 ******************************************************/

LIBPINMAME_API int PinmameGetPendingAudioSamples16bit(signed short* buffer, int outChannels, int maxNumber) {
	return (_isRunning) ? fillAudioBuffer(buffer, outChannels, maxNumber, 0) : -1;
}
