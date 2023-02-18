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
#include "audit.h"
#include "mech.h"

extern int throttle;
extern int autoframeskip;
extern int allow_sleep;

int g_fHandleKeyboard = 0;
int g_fHandleMechanics = 0;
int g_fDumpFrames = 0;
int g_fPause = 0;

#ifdef VPINMAME_ALTSOUND
char g_szGameName[256] = { 0 }; // String containing requested game name (may be different from ROM if aliased)
#endif
}

static int _isRunning = 0;
static int _timeToQuit = 0;
static PinmameConfig* _p_Config = nullptr;
static std::thread* _p_gameThread = nullptr;

static int _displaysInit;
static UINT8 _displayData[PINMAME_MAX_DISPLAYS][DMD_MAXX * DMD_MAXY];

static int _mechInit[MECH_MAXMECH / 2];
static PinmameMechInfo _mechInfo[MECH_MAXMECH / 2];

static PinmameAudioInfo _audioInfo;
static float _audioData[PINMAME_ACCUMULATOR_SAMPLES * 2];

static const PinmameKeyboardInfo _keyboardInfo[] = {
	{ "A", A, KEYCODE_A },
	{ "B", B, KEYCODE_B },
	{ "C", C, KEYCODE_C },
	{ "D", D, KEYCODE_D },
	{ "E", E, KEYCODE_E },
	{ "F", F, KEYCODE_F },
	{ "G", G, KEYCODE_G },
	{ "H", H, KEYCODE_H },
	{ "I", I, KEYCODE_I },
	{ "J", J, KEYCODE_J },
	{ "K", K, KEYCODE_K },
	{ "L", L, KEYCODE_L },
	{ "M", M, KEYCODE_M },
	{ "N", N, KEYCODE_N },
	{ "O", O, KEYCODE_O },
	{ "P", P, KEYCODE_P },
	{ "Q", Q, KEYCODE_Q },
	{ "R", R, KEYCODE_R },
	{ "S", S, KEYCODE_S },
	{ "T", T, KEYCODE_T },
	{ "U", U, KEYCODE_U },
	{ "V", V, KEYCODE_V },
	{ "W", W, KEYCODE_W },
	{ "X", X, KEYCODE_X },
	{ "Y", Y, KEYCODE_Y },
	{ "Z", Z, KEYCODE_Z },
	{ "0", NUMBER_0, KEYCODE_0 },
	{ "1", NUMBER_1, KEYCODE_1 },
	{ "2", NUMBER_2, KEYCODE_2 },
	{ "3", NUMBER_3, KEYCODE_3 },
	{ "4", NUMBER_4, KEYCODE_4 },
	{ "5", NUMBER_5, KEYCODE_5 },
	{ "6", NUMBER_6, KEYCODE_6 },
	{ "7", NUMBER_7, KEYCODE_7 },
	{ "8", NUMBER_8, KEYCODE_8 },
	{ "9", NUMBER_9, KEYCODE_9 },
	{ "KEYPAD 0", KEYPAD_1, KEYCODE_0_PAD },
	{ "KEYPAD 1", KEYPAD_2, KEYCODE_1_PAD },
	{ "KEYPAD 2", KEYPAD_2, KEYCODE_2_PAD },
	{ "KEYPAD 3", KEYPAD_3, KEYCODE_3_PAD },
	{ "KEYPAD 4", KEYPAD_4, KEYCODE_4_PAD },
	{ "KEYPAD 5", KEYPAD_5, KEYCODE_5_PAD },
	{ "KEYPAD 6", KEYPAD_6, KEYCODE_6_PAD },
	{ "KEYPAD 7", KEYPAD_7, KEYCODE_7_PAD },
	{ "KEYPAD 8", KEYPAD_8, KEYCODE_8_PAD },
	{ "KEYPAD 9", KEYPAD_9, KEYCODE_9_PAD },
	{ "F1", F1, KEYCODE_F1 },
	{ "F2", F2, KEYCODE_F2 },
	{ "F3", F3, KEYCODE_F3 },
	{ "F4", F4, KEYCODE_F4 },
	{ "F5", F5, KEYCODE_F5 },
	{ "F6", F6, KEYCODE_F6 },
	{ "F7", F7, KEYCODE_F7 },
	{ "F8", F8, KEYCODE_F8 },
	{ "F9", F9, KEYCODE_F9 },
	{ "F10", F10, KEYCODE_F10 },
	{ "F11", F11, KEYCODE_F11 },
	{ "F12", F12, KEYCODE_F12 },
	{ "ESCAPE", ESCAPE, KEYCODE_ESC },
	{ "GRAVE ACCENT", GRAVE_ACCENT, KEYCODE_TILDE },
	{ "MINUS", MINUS, KEYCODE_MINUS },
	{ "EQUALS", EQUALS, KEYCODE_EQUALS },
	{ "BACKSPACE", BACKSPACE, KEYCODE_BACKSPACE },
	{ "TAB", TAB, KEYCODE_TAB },
	{ "LEFT BRACKET", LEFT_BRACKET, KEYCODE_OPENBRACE },
	{ "RIGHT BRACKET", RIGHT_BRACKET, KEYCODE_CLOSEBRACE },
	{ "ENTER", ENTER, KEYCODE_ENTER },
	{ "SEMICOLON", SEMICOLON, KEYCODE_COLON },
	{ "QUOTE", QUOTE, KEYCODE_QUOTE },
	{ "BACKSLASH", BACKSLASH, KEYCODE_BACKSLASH },
	{ "COMMA", COMMA, KEYCODE_COMMA },
	{ "PERIOD", PERIOD, KEYCODE_STOP },
	{ "SLASH", SLASH, KEYCODE_SLASH },
	{ "SPACE", SPACE, KEYCODE_SPACE },
	{ "INSERT", INSERT, KEYCODE_INSERT },
	{ "DELETE", DELETE, KEYCODE_DEL },
	{ "HOME", HOME, KEYCODE_HOME },
	{ "END", END, KEYCODE_END },
	{ "PAGE UP", PAGE_UP, KEYCODE_PGUP },
	{ "PAGE DOWN", PAGE_DOWN, KEYCODE_PGDN },
	{ "LEFT", LEFT, KEYCODE_LEFT },
	{ "RIGHT", RIGHT, KEYCODE_RIGHT },
	{ "UP", UP, KEYCODE_UP },
	{ "DOWN", DOWN, KEYCODE_DOWN },
	{ "KEYPAD DIVIDE", KEYPAD_DIVIDE, KEYCODE_SLASH_PAD },
	{ "KEYPAD MULTIPLY", KEYPAD_MULTIPLY, KEYCODE_ASTERISK },
	{ "KEYPAD SUBTRACT", KEYPAD_SUBTRACT, KEYCODE_MINUS_PAD },
	{ "KEYPAD ADD", KEYPAD_ADD, KEYCODE_PLUS_PAD },
	{ "KEYPAD ENTER", KEYPAD_ENTER, KEYCODE_ENTER_PAD },
	{ "PRINT SCREEN", PRINT_SCREEN, KEYCODE_PRTSCR },
	{ "PAUSE", PAUSE, KEYCODE_PAUSE },
	{ "LEFT SHIFT", LEFT_SHIFT, KEYCODE_LSHIFT },
	{ "RIGHT SHIFT", RIGHT_SHIFT, KEYCODE_RSHIFT },
	{ "LEFT CONTROL", LEFT_CONTROL, KEYCODE_LCONTROL },
	{ "RIGHT CONTROL", RIGHT_CONTROL, KEYCODE_RCONTROL },
	{ "LEFT ALT", LEFT_ALT, KEYCODE_LALT },
	{ "RIGHT ALT", RIGHT_ALT, KEYCODE_RALT },
	{ "SCROLL LOCK", SCROLL_LOCK, KEYCODE_SCRLOCK },
	{ "NUM LOCK", NUM_LOCK, KEYCODE_NUMLOCK },
	{ "CAPS LOCK", CAPS_LOCK, KEYCODE_CAPSLOCK },
	{ "LEFT SUPER", LEFT_SUPER, KEYCODE_LWIN },
	{ "RIGHT SUPER", RIGHT_SUPER, KEYCODE_RWIN },
	{ "MENU", MENU, KEYCODE_MENU }
};

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
 * osd_init
 ******************************************************/

extern "C" int osd_init(void) {
	return 0;
}

/******************************************************
 * osd_get_key_list
 ******************************************************/

extern "C" const struct KeyboardInfo* osd_get_key_list(void) {
	return (const struct KeyboardInfo*)_keyboardInfo;
}

/******************************************************
 * osd_is_key_pressed
 ******************************************************/

extern "C" int osd_is_key_pressed(const int keycode) {
	if (_p_Config->fn_IsKeyPressed) {
		return (*(_p_Config->fn_IsKeyPressed))((PINMAME_KEYCODE)keycode);
	}
	return 0;
}

/******************************************************
 * osd_readkey_unicode
 ******************************************************/

extern "C" int osd_readkey_unicode(const int flush) {
	return 0;
}

/******************************************************
 * osd_start_audio_stream
 ******************************************************/

extern "C" int osd_start_audio_stream(const int stereo) {
	if (_p_Config->cb_OnAudioAvailable) {
		memset(&_audioInfo, 0, sizeof(PinmameAudioInfo));
		_audioInfo.format = _p_Config->audioFormat;
		_audioInfo.channels = stereo ? 2 : 1;
		_audioInfo.sampleRate = Machine->sample_rate;
		_audioInfo.framesPerSecond = Machine->drv->frames_per_second;
		_audioInfo.samplesPerFrame = Machine->sample_rate / Machine->drv->frames_per_second;
		_audioInfo.bufferSize = PINMAME_ACCUMULATOR_SAMPLES * 2;

		return (*(_p_Config->cb_OnAudioAvailable))(&_audioInfo);
	}
	return 0;
}

/******************************************************
 * osd_update_audio_stream
 ******************************************************/

extern "C" int osd_update_audio_stream(INT16* p_buffer) {
	if (_p_Config->cb_OnAudioUpdated) {
		int samplesThisFrame = mixer_samples_this_frame();

		if (_p_Config->audioFormat == AUDIO_FORMAT_INT16) {
			return (*(_p_Config->cb_OnAudioUpdated))((void*)p_buffer, samplesThisFrame);
		}

		for (int i = 0; i < samplesThisFrame * _audioInfo.channels; i++) {
			_audioData[i] = ((float)p_buffer[i]) / 32768.0;
		}

		return (*(_p_Config->cb_OnAudioUpdated))((void*)_audioData, samplesThisFrame);
	}
	return 0;
}

/******************************************************
 * osd_stop_audio_stream
 ******************************************************/

extern "C" void osd_stop_audio_stream(void) {
}

/******************************************************
 * osd_sound_enable
 ******************************************************/

extern "C" void osd_sound_enable(int enable)
{
}

/******************************************************
 * osd_set_mastervolume
 ******************************************************/

extern "C" void osd_set_mastervolume(int attenuation)
{
}

/******************************************************
 * osd_get_mastervolume
 ******************************************************/

extern "C" int osd_get_mastervolume(void)
{
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
 * libpinmame_update_display
 ******************************************************/

extern "C" void libpinmame_update_display(const int index, const struct core_dispLayout* p_layout, void* p_data) {
	PinmameDisplayLayout displayLayout;
	memset(&displayLayout, 0, sizeof(PinmameDisplayLayout));
	displayLayout.type = (PINMAME_DISPLAY_TYPE)p_layout->type;
	displayLayout.top = p_layout->top;
	displayLayout.left = p_layout->left;

	int dmd = ((p_layout->type & CORE_DMD) == CORE_DMD);

	if (dmd) {
		displayLayout.width = p_layout->length;
		displayLayout.height = p_layout->start;

		const int shade_16_enabled = ((core_gameData->gen & (GEN_SAM|GEN_SPA|GEN_ALVG_DMD2))
			|| (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0)
			|| (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0));

		displayLayout.depth = shade_16_enabled ? 4 : 2;
	}
	else {
		displayLayout.length = p_layout->length;
	}

	if (_displaysInit) {
		if (_p_Config->cb_OnDisplayUpdated) {
			if (dmd) {
				if (memcmp(_displayData[index], p_data, (displayLayout.width * displayLayout.height) * sizeof(UINT8))) {
					memcpy(_displayData[index], p_data, (displayLayout.width * displayLayout.height) * sizeof(UINT8));
					(*(_p_Config->cb_OnDisplayUpdated))(index, _displayData[index], &displayLayout);
				}
			}
			else {
				if (memcmp(_displayData[index], p_data, displayLayout.length * sizeof(UINT16))) {
					memcpy(_displayData[index], p_data, displayLayout.length * sizeof(UINT16));
					(*(_p_Config->cb_OnDisplayUpdated))(index, _displayData[index], &displayLayout);
				}
			}
		}
	}
	else {
		int displayCountIndex = 0;
		const int displayCount = GetDisplayCount(core_gameData->lcdLayout, &displayCountIndex);

		if (_p_Config->cb_OnDisplayAvailable) {
			(*(_p_Config->cb_OnDisplayAvailable))(index, displayCount, &displayLayout);
		}

		if (index == displayCount - 1) {
			_displaysInit = 1;
		}
	}
}

/******************************************************
 * libpinmame_forward_console_data
 ******************************************************/

extern "C" void libpinmame_forward_console_data(void* p_data, int size) {
	if (_p_Config->cb_OnConsoleDataUpdated) {
		(*(_p_Config->cb_OnConsoleDataUpdated))(p_data, size);
	}
}

/******************************************************
 * OnStateChange
 ******************************************************/

extern "C" void OnStateChange(const int state) {
	_isRunning = state;

	if (_p_Config->cb_OnStateUpdated) {
		(*(_p_Config->cb_OnStateUpdated))(state);
	}
}

/******************************************************
 * OnSolenoid
 ******************************************************/

extern "C" void OnSolenoid(const int solenoid, const int state) {
	if (_p_Config->cb_OnSolenoidUpdated) {
		PinmameSolenoidState solenoidState;
		solenoidState.solNo = solenoid;
		solenoidState.state = state;

		(*(_p_Config->cb_OnSolenoidUpdated))(&solenoidState);
	}
}

/******************************************************
 * libpinmame_update_mech
 ******************************************************/

extern "C" void libpinmame_update_mech(const int mechNo, mech_tMechData* p_mechData) {
	int index = mechNo - ((g_fHandleMechanics == 0) ? (MECH_MAXMECH / 2) : 0);

	int speed = p_mechData->speed / p_mechData->ret;

	if (_mechInit[index]) {
		if (_mechInfo[index].pos != p_mechData->pos || _mechInfo[index].speed != speed) {
			_mechInfo[index].pos = p_mechData->pos;
			_mechInfo[index].speed = speed;

			if (_p_Config->cb_OnMechUpdated) {
				(*(_p_Config->cb_OnMechUpdated))(index, &_mechInfo[index]);
			}
		}
	}
	else {
		_mechInit[index] = 1;

		_mechInfo[index].type = p_mechData->type;
		_mechInfo[index].length = p_mechData->length;
		_mechInfo[index].steps = p_mechData->steps;
		
		_mechInfo[index].pos = p_mechData->pos;
		_mechInfo[index].speed = speed;

		if (_p_Config->cb_OnMechAvailable) {
			(*(_p_Config->cb_OnMechAvailable))(index, &_mechInfo[index]);
		}
	}
}

/******************************************************
 * StartGame
 ******************************************************/

void StartGame(const int gameNum) {
	_displaysInit = 0;

	memset(_mechInit, 0, sizeof(_mechInit));
	memset(_mechInfo, 0, sizeof(_mechInfo));

	run_game(gameNum);

	OnStateChange(0);
}

/******************************************************
 * PinmameGetGame
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameGetGame(const char* const p_name, PinmameGameCallback callback) {
	if (_p_Config == nullptr) {
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
	if (_p_Config == nullptr) {
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
	if (_p_Config == nullptr) {
		_p_Config = (PinmameConfig*)malloc(sizeof(PinmameConfig));
	}

	memcpy(_p_Config, p_config, sizeof(PinmameConfig));

	fprintf(stdout, "PinmameSetConfig(): sampleRate=%d, vpmPath=%s\n", _p_Config->sampleRate, _p_Config->vpmPath);

	memset(&options, 0, sizeof(options));

	options.samplerate = _p_Config->sampleRate;
	options.skip_gameinfo = 1;
	options.skip_disclaimer = 1;

	setPath(FILETYPE_ROM, ComposePath(_p_Config->vpmPath, "roms"));
	setPath(FILETYPE_NVRAM, ComposePath(_p_Config->vpmPath, "nvram"));
	setPath(FILETYPE_SAMPLE, ComposePath(_p_Config->vpmPath, "samples"));
	setPath(FILETYPE_CONFIG, ComposePath(_p_Config->vpmPath, "cfg"));
	setPath(FILETYPE_HIGHSCORE, ComposePath(_p_Config->vpmPath, "hi"));
	setPath(FILETYPE_INPUTLOG, ComposePath(_p_Config->vpmPath, "inp"));
	setPath(FILETYPE_MEMCARD, ComposePath(_p_Config->vpmPath, "memcard"));
	setPath(FILETYPE_STATE, ComposePath(_p_Config->vpmPath, "sta"));

	throttle = 1;
	autoframeskip = 0;
	allow_sleep = 1;
}

/******************************************************
 * PinmameGetHandleKeyboard
 ******************************************************/

LIBPINMAME_API int PinmameGetHandleKeyboard() {
	return g_fHandleKeyboard;
}

/******************************************************
 * PinmameSetHandleKeyboard
 ******************************************************/

LIBPINMAME_API void PinmameSetHandleKeyboard(const int handleKeyboard) {
	g_fHandleKeyboard = handleKeyboard ? 1 : 0;
}

/******************************************************
 * PinmameGetHandleMechanics
 ******************************************************/

LIBPINMAME_API int PinmameGetHandleMechanics() {
	return g_fHandleMechanics;
}

/******************************************************
 * PinmameSetHandleMechanics
 ******************************************************/

LIBPINMAME_API void PinmameSetHandleMechanics(const int handleMechanics) {
	g_fHandleMechanics = handleMechanics;
}

/******************************************************
 * PinmameGetUseModulatedSolenoids
 ******************************************************/

LIBPINMAME_API int PinmameGetUseModulatedSolenoids() {
	return options.usemodsol ? 1 : 0;
}

/******************************************************
 * PinmameSetUseModulatedSolenoids
 ******************************************************/

LIBPINMAME_API void PinmameSetUseModulatedSolenoids(const int useModulatedSolenoids) {
    options.usemodsol = useModulatedSolenoids > 0;
}

/******************************************************
 * PinmameRun
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameRun(const char* const p_name) {
	if (_p_Config == nullptr) {
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

LIBPINMAME_API int PinmameGetSwitch(const int swNo) {
	return (_isRunning) ? vp_getSwitch(swNo) : 0;
}

/******************************************************
 * PinmameSetSwitch
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitch(const int swNo, const int state) {
	if (_isRunning) {
		 vp_putSwitch(swNo, state ? 1 : 0);
	}
}

/******************************************************
 * PinmameSetSwitches
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitches(const PinmameSwitchState* const p_states, const int numSwitches) {
	if (_isRunning) {
		for (int i = 0; i < numSwitches; ++i) {
			vp_putSwitch(p_states[i].swNo, p_states[i].state ? 1 : 0);
		}
	}
}

/******************************************************
 * PinmameGetSolenoidMask
 ******************************************************/

LIBPINMAME_API uint64_t PinmameGetSolenoidMask() {
	return vp_getSolMask64();
}

/******************************************************
 * PinmameSetSolenoidMask
 ******************************************************/

LIBPINMAME_API void PinmameSetSolenoidMask(const uint64_t mask) {
	vp_setSolMask(0, (int)(mask & 0xFFFFFFFF));
	vp_setSolMask(1, (int)((mask >> 32) & 0xFFFFFFFF));
}

/******************************************************
 * PinmameGetMaxSolenoids
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxSolenoids() {
	return (CORE_MAXSOL + CORE_MODSOL_MAX);
}

/******************************************************
 * PinmameGetSolenoid
 ******************************************************/

LIBPINMAME_API int PinmameGetSolenoid(const int solNo) {
	return (_isRunning) ? vp_getSolenoid(solNo) : 0;
}

/******************************************************
 * PinmameGetChangedSolenoids
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedSolenoids(PinmameSolenoidState* const p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgSols chgSols;
	const int count = vp_getChangedSolenoids(chgSols);
	if (count > 0) {
		memcpy(p_changedStates, chgSols, count * sizeof(PinmameSolenoidState));
	}
	return count;
}

/******************************************************
 * PinmameGetMaxLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxLamps() {
	return (CORE_MAXLAMPCOL * 8) + CORE_MAXRGBLAMPS;
}

/******************************************************
 * PinmameGetLamp
 ******************************************************/

LIBPINMAME_API int PinmameGetLamp(const int lampNo) {
	return (_isRunning) ? vp_getLamp(lampNo) : 0;
}

/******************************************************
 * PinmameGetChangedLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedLamps(PinmameLampState* const p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgLamps chgLamps;
	const int count = vp_getChangedLamps(chgLamps);
	if (count > 0) {
		memcpy(p_changedStates, chgLamps, count * sizeof(PinmameLampState));
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

LIBPINMAME_API int PinmameGetChangedGIs(PinmameGIState* const p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgGIs chgGIs;
	const int count = vp_getChangedGI(chgGIs);
	if (count > 0) {
		memcpy(p_changedStates, chgGIs, count * sizeof(PinmameGIState));
	}
	return count;
}

/******************************************************
 * PinmameGetMaxLEDs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxLEDs() {
	return CORE_SEGCOUNT;
}

/******************************************************
 * PinmameGetChangedLEDs
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedLEDs(const uint64_t mask, const uint64_t mask2, PinmameLEDState* const p_changedStates) {
	if (!_isRunning) {
		return -1;
	}

	vp_tChgLED chgLEDs;
	const int count = vp_getChangedLEDs(chgLEDs, mask, mask2);
	if (count > 0) {
		memcpy(p_changedStates, chgLEDs, count * sizeof(PinmameLEDState));
	}
	return count;
}

/******************************************************
 * PinmameGetMaxMechs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxMechs() {
	return MECH_MAXMECH / 2;
}

/******************************************************
 * PinmameSetMech
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameSetMech(const int mechNo, const PinmameMechConfig* const p_mechConfig) {
	if (g_fHandleMechanics) {
		return MECH_HANDLE_MECHANICS;
	}

	if (mechNo >= MECH_MAXMECH / 2) {
		return MECH_NO_INVALID;
	}

	mech_tInitData mechInitData;
	memset(&mechInitData, 0, sizeof(mech_tInitData));

	if (p_mechConfig != nullptr) {
		mechInitData.type = p_mechConfig->type;

		mechInitData.sol1 = p_mechConfig->sol1;
		mechInitData.sol2 = p_mechConfig->sol2;

		mechInitData.length = p_mechConfig->length;
		mechInitData.steps = p_mechConfig->steps;
		mechInitData.initialpos = p_mechConfig->initialPos;

		mechInitData.type = (mechInitData.type & 0xff0001ff) | MECH_ACC(p_mechConfig->acc);
		mechInitData.type = (mechInitData.type & 0x00ffffff) | MECH_RET(p_mechConfig->ret);

		for (int index = 0; index < PINMAME_MAX_MECHSW; index++) {
			mechInitData.sw[index].swNo = p_mechConfig->sw[index].swNo;
			mechInitData.sw[index].startPos = p_mechConfig->sw[index].startPos;
			mechInitData.sw[index].endPos = p_mechConfig->sw[index].endPos;
			mechInitData.sw[index].pulse = p_mechConfig->sw[index].pulse;
		}
	}

	_mechInit[mechNo] = 0;

	mech_add(mechNo + (MECH_MAXMECH / 2), &mechInitData);

	return OK;
}

/******************************************************
 * PinmameGetDIP
 ******************************************************/

LIBPINMAME_API int PinmameGetDIP(const int dipBank) {
	return (_isRunning) ? vp_getDIP(dipBank) : 0;
}

/******************************************************
 * PinmameSetDIP
 ******************************************************/

LIBPINMAME_API void PinmameSetDIP(const int dipBank, const int value) {
	if (_isRunning) {
		 vp_setDIP(dipBank, value);
	}
}