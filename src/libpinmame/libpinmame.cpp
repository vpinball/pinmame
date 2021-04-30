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
#include "audit.h"

extern UINT32 g_raw_dmdx;
extern UINT32 g_raw_dmdy;
extern UINT32 g_needs_DMD_update;
extern unsigned char g_raw_dmdbuffer[DMD_MAXY * DMD_MAXX];
extern int channels;

int g_fHandleKeyboard = 0;
int g_fHandleMechanics = 0;
int g_fDumpFrames = 0;
int g_fPause = 0;
struct rc_struct* rc = nullptr;

#ifdef VPINMAME_ALTSOUND
char g_szGameName[256] = { 0 }; // String containing requested game name (may be different from ROM if aliased)
#endif
}

static PinmameConfig _config = {
	48000,
	"",
	0,
	0,
	0,
	0
};

static int _isRunning = 0;
static int _timeToQuit = 0;
static int _firstFrame = 0;
static UINT8 _frame[DMD_MAXY * DMD_MAXX];
static UINT16 _lastSeg[MAX_DISPLAYS][CORE_SEGCOUNT];

static std::thread* _p_gameThread = nullptr;

/******************************************************
 * ComposePath
 ******************************************************/

char* ComposePath(const char* const path, const char* const file) {
	const size_t pathLength = strlen(path);
	const size_t fileLength = strlen(file);
	char* const newPath = (char*)malloc(pathLength + fileLength + 4);

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
 * GetDisplayCount
 ******************************************************/

int GetDisplayCount(const struct core_dispLayout* p_layout, int* const p_index) {
	for (; p_layout->length; p_layout += 1) {
		if (p_layout->type == CORE_IMPORT) {
			GetDisplayCount(p_layout->lptr, p_index);
			continue;
		}
		
		(*p_index)++;
	}
	return *p_index;
}

/******************************************************
 * GetDisplays
 ******************************************************/

void GetDisplays(const struct core_dispLayout* p_layout, const int displayCount, int* const p_index) {
	for (; p_layout->length; p_layout += 1) {
		if (p_layout->type == CORE_IMPORT) {
			GetDisplays(p_layout->lptr, displayCount, p_index);
			continue;
		}
		PinmameDisplayLayout displayLayout;
		memset(&displayLayout, 0, sizeof(PinmameDisplayLayout));
		displayLayout.type = (PINMAME_DISPLAY_TYPE)p_layout->type;
		displayLayout.top = p_layout->top;
		displayLayout.left = p_layout->left;

		if (p_layout->type == CORE_DMD
			|| p_layout->type == (CORE_DMD | CORE_DMDNOAA)
			|| p_layout->type == (CORE_DMD | CORE_DMDNOAA | CORE_NODISP)) {
				displayLayout.height = g_raw_dmdy;
				displayLayout.width = g_raw_dmdx;

				const int shade_16_enabled = ((core_gameData->gen & (GEN_SAM|GEN_SPA|GEN_ALVG_DMD2)) 
					|| (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) 
					|| (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0));

				displayLayout.depth = shade_16_enabled ? 4 : 2;
		}
		else {
			displayLayout.length = p_layout->length;
		}

		(*(_config.cb_OnDisplayAvailable))(*p_index, displayCount, &displayLayout);

		(*p_index)++;
	}
}

/******************************************************
 * UpdateDisplays
 ******************************************************/

void UpdateDisplays(const struct core_dispLayout* p_layout, int* const p_index, int* const p_lastOffset) {
	for (; p_layout->length; p_layout += 1) {
		if (p_layout->type == CORE_IMPORT) {
			UpdateDisplays(p_layout->lptr, p_index, p_lastOffset);
			continue;
		}

		PinmameDisplayLayout displayLayout;
		memset(&displayLayout, 0, sizeof(PinmameDisplayLayout));
		displayLayout.type = (PINMAME_DISPLAY_TYPE)p_layout->type;
		displayLayout.top = p_layout->top;
		displayLayout.left = p_layout->left;

		if (p_layout->type == CORE_DMD
			|| p_layout->type == (CORE_DMD | CORE_DMDNOAA)
			|| p_layout->type == (CORE_DMD | CORE_DMDNOAA | CORE_NODISP)) {
			if (g_needs_DMD_update) {
				displayLayout.height = g_raw_dmdy;
				displayLayout.width = g_raw_dmdx;

				const int shade_16_enabled = ((core_gameData->gen & (GEN_SAM|GEN_SPA|GEN_ALVG_DMD2)) 
					|| (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) 
					|| (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0));

				displayLayout.depth = shade_16_enabled ? 4 : 2;

				memcpy(_frame, g_raw_dmdbuffer, (g_raw_dmdx * g_raw_dmdy) * sizeof(unsigned char));

				(*(_config.cb_OnDisplayUpdated))(*p_index, _frame, &displayLayout);

				g_needs_DMD_update = 0; 
			}
		}
		else {
			displayLayout.length = p_layout->length;

			const UINT16* const p_drawSeg = coreGlobals.drawSeg + *p_lastOffset;

			for (unsigned int i = 0; i < p_layout->length; i++) {
				if (_lastSeg[*p_index][i] != p_drawSeg[i]) {
					memcpy(_frame, p_drawSeg, p_layout->length * sizeof(UINT16));
					(*(_config.cb_OnDisplayUpdated))(*p_index, _frame, &displayLayout);

					break;
				}
			}

			memcpy(_lastSeg[*p_index], p_drawSeg, p_layout->length * sizeof(UINT16));
			*(p_lastOffset) += p_layout->length;
		}

		(*p_index)++;
	}
}

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

extern "C" void libpinmame_update_displays() {
	if (!_firstFrame) {
		if (_config.cb_OnDisplayUpdated) {
			int index = 0;
			int lastOffset = 0;
			UpdateDisplays(core_gameData->lcdLayout, &index, &lastOffset);
		}
	}
	else {
		if (_config.cb_OnDisplayAvailable) {
			int index = 0;
			const int displayCount = GetDisplayCount(core_gameData->lcdLayout, &index);

			index = 0;
			GetDisplays(core_gameData->lcdLayout, displayCount, &index);
		}
		_firstFrame = 0;
	}
}

/******************************************************
 * OnStateChange
 ******************************************************/

extern "C" void OnStateChange(const int state) {
	_isRunning = state;

	if (_config.cb_OnStateUpdated) {
		(*(_config.cb_OnStateUpdated))(state);
	}
}

/******************************************************
 * OnSolenoid
 ******************************************************/

extern "C" void OnSolenoid(const int solenoid, const int isActive) {
	if (_config.cb_OnSolenoidUpdated) {
		(*(_config.cb_OnSolenoidUpdated))(solenoid, isActive);
	}
}

/******************************************************
 * StartGame
 ******************************************************/

void StartGame(const int gameNum) {
	_firstFrame = 1;

	run_game(gameNum);

	OnStateChange(0);
}

/******************************************************
 * PinmameGetGame
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameGetGame(const char* const p_name, PinmameGameCallback callback) {
	if (rc == nullptr) {
		return CONFIG_NOT_SET;
	}

	int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0) {
		return GAME_NOT_FOUND;
	}

	PinmameGame game;
	memset(&game, 0, sizeof(PinmameGame));

	game.name = drivers[gameNum]->name;
	if (drivers[gameNum]->clone_of) {
		game.clone_of = drivers[gameNum]->clone_of->name;
	}
	game.description = drivers[gameNum]->description;
	game.year = drivers[gameNum]->year;
	game.manufacturer = drivers[gameNum]->manufacturer;
	game.flags = drivers[gameNum]->flags;
	game.found = RomsetMissing(gameNum) == 0;

	if (callback) {
		(*callback)(&game);
	}

	return OK;
}

/******************************************************
 * PinmameGetGames
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameGetGames(PinmameGameCallback callback) {
	if (rc == nullptr) {
		return CONFIG_NOT_SET;
	}

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
		game.flags = drivers[gameNum]->flags;
		game.found = RomsetMissing(gameNum) == 0;

		if (callback) {
			(*callback)(&game);
		}

		gameNum++;
	}

	return OK;
}

/******************************************************
 * PinmameSetConfig
 ******************************************************/

LIBPINMAME_API void PinmameSetConfig(const PinmameConfig* const p_config) {
	memcpy(&_config, p_config, sizeof(PinmameConfig));

	fprintf(stdout, "PinmameSetConfig(): sampleRate=%d, vpmPath=%s\n", _config.sampleRate, _config.vpmPath);

	if (rc == nullptr) {
		rc = cli_rc_create();

		rc_set_option(rc, "throttle", "1", 0);
		rc_set_option(rc, "sleep", "1", 0);
		rc_set_option(rc, "autoframeskip", "0", 0);
		rc_set_option(rc, "skip_gameinfo", "1", 0);
		rc_set_option(rc, "skip_disclaimer", "1", 0);
	}

	setPath(FILETYPE_ROM, ComposePath(_config.vpmPath, "roms"));
	setPath(FILETYPE_NVRAM, ComposePath(_config.vpmPath, "nvram"));
	setPath(FILETYPE_SAMPLE, ComposePath(_config.vpmPath, "samples"));
	setPath(FILETYPE_CONFIG, ComposePath(_config.vpmPath, "cfg"));
	setPath(FILETYPE_HIGHSCORE, ComposePath(_config.vpmPath, "hi"));
	setPath(FILETYPE_INPUTLOG, ComposePath(_config.vpmPath, "inp"));
	setPath(FILETYPE_MEMCARD, ComposePath(_config.vpmPath, "memcard"));
	setPath(FILETYPE_STATE, ComposePath(_config.vpmPath, "sta"));
}

/******************************************************
 * PinmameRun
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameRun(const char* const p_name) {
	if (rc == nullptr) {
		return CONFIG_NOT_SET;
	}

	if (_isRunning) {
		return GAME_ALREADY_RUNNING;
	}

	const int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0) {
		return GAME_NOT_FOUND;
	}

#ifdef VPINMAME_ALTSOUND
	strcpy_s(g_szGameName, p_name);
#endif

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

LIBPINMAME_API PINMAME_STATUS PinmamePause(const int pause) {
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
	const UINT64 hardwareGen = (_isRunning) ? core_gameData->gen : 0;
	return (PINMAME_HARDWARE_GEN)hardwareGen;
}

/******************************************************
 * PinmameGetSwitch
 ******************************************************/

LIBPINMAME_API int PinmameGetSwitch(const int slot) {
	return (_isRunning) ? vp_getSwitch(slot) : 0;
}

/******************************************************
 * PinmameSetSwitch
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitch(const int slot, const int state) {
	if (_isRunning) {
		 vp_putSwitch(slot, state ? 1 : 0);
	}
}

/******************************************************
 * PinmameSetSwitches
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitches(const int* const p_states, const int numSwitches) {
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

LIBPINMAME_API int PinmameGetChangedLamps(int* const p_changedStates) {
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

LIBPINMAME_API int PinmameGetChangedGIs(int* const p_changedStates) {
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

LIBPINMAME_API int PinmameGetPendingAudioSamples(float* const buffer, const int outChannels, const int maxNumber) {
	return (_isRunning) ? fillAudioBuffer(buffer, outChannels, maxNumber, 1) : -1;
}

/******************************************************
 * GetPendingAudioSamples16bit
 ******************************************************/

LIBPINMAME_API int PinmameGetPendingAudioSamples16bit(signed short* const buffer, const int outChannels, const int maxNumber) {
	return (_isRunning) ? fillAudioBuffer(buffer, outChannels, maxNumber, 0) : -1;
}
