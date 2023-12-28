// license:BSD-3-Clause

#include "libpinmame.h"

#include <thread>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#endif

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
PINMAME_DMD_MODE g_fDmdMode = PINMAME_DMD_MODE_BRIGHTNESS;
PINMAME_SOUND_MODE g_fSoundMode = PINMAME_SOUND_MODE_DEFAULT;
}

int _isRunning = 0;
int _timeToQuit = 0;
PinmameConfig* _p_Config = nullptr;
std::thread* _p_gameThread = nullptr;
void* _p_userData = nullptr;

int _mechInit[MECH_MAXMECH];
PinmameMechInfo _mechInfo[MECH_MAXMECH];

PinmameAudioInfo _audioInfo;
float _audioData[PINMAME_ACCUMULATOR_SAMPLES * 2];

typedef struct {
	PinmameDisplayLayout layout;
	void* pData;
	int size;
} PinmameDisplay;

std::vector<PinmameDisplay*> _displays;

static const PinmameKeyboardInfo _keyboardInfo[] = {
	{ "A", PINMAME_KEYCODE_A, KEYCODE_A },
	{ "B", PINMAME_KEYCODE_B, KEYCODE_B },
	{ "C", PINMAME_KEYCODE_C, KEYCODE_C },
	{ "D", PINMAME_KEYCODE_D, KEYCODE_D },
	{ "E", PINMAME_KEYCODE_E, KEYCODE_E },
	{ "F", PINMAME_KEYCODE_F, KEYCODE_F },
	{ "G", PINMAME_KEYCODE_G, KEYCODE_G },
	{ "H", PINMAME_KEYCODE_H, KEYCODE_H },
	{ "I", PINMAME_KEYCODE_I, KEYCODE_I },
	{ "J", PINMAME_KEYCODE_J, KEYCODE_J },
	{ "K", PINMAME_KEYCODE_K, KEYCODE_K },
	{ "L", PINMAME_KEYCODE_L, KEYCODE_L },
	{ "M", PINMAME_KEYCODE_M, KEYCODE_M },
	{ "N", PINMAME_KEYCODE_N, KEYCODE_N },
	{ "O", PINMAME_KEYCODE_O, KEYCODE_O },
	{ "P", PINMAME_KEYCODE_P, KEYCODE_P },
	{ "Q", PINMAME_KEYCODE_Q, KEYCODE_Q },
	{ "R", PINMAME_KEYCODE_R, KEYCODE_R },
	{ "S", PINMAME_KEYCODE_S, KEYCODE_S },
	{ "T", PINMAME_KEYCODE_T, KEYCODE_T },
	{ "U", PINMAME_KEYCODE_U, KEYCODE_U },
	{ "V", PINMAME_KEYCODE_V, KEYCODE_V },
	{ "W", PINMAME_KEYCODE_W, KEYCODE_W },
	{ "X", PINMAME_KEYCODE_X, KEYCODE_X },
	{ "Y", PINMAME_KEYCODE_Y, KEYCODE_Y },
	{ "Z", PINMAME_KEYCODE_Z, KEYCODE_Z },
	{ "0", PINMAME_KEYCODE_NUMBER_0, KEYCODE_0 },
	{ "1", PINMAME_KEYCODE_NUMBER_1, KEYCODE_1 },
	{ "2", PINMAME_KEYCODE_NUMBER_2, KEYCODE_2 },
	{ "3", PINMAME_KEYCODE_NUMBER_3, KEYCODE_3 },
	{ "4", PINMAME_KEYCODE_NUMBER_4, KEYCODE_4 },
	{ "5", PINMAME_KEYCODE_NUMBER_5, KEYCODE_5 },
	{ "6", PINMAME_KEYCODE_NUMBER_6, KEYCODE_6 },
	{ "7", PINMAME_KEYCODE_NUMBER_7, KEYCODE_7 },
	{ "8", PINMAME_KEYCODE_NUMBER_8, KEYCODE_8 },
	{ "9", PINMAME_KEYCODE_NUMBER_9, KEYCODE_9 },
	{ "KEYPAD 0", PINMAME_KEYCODE_KEYPAD_1, KEYCODE_0_PAD },
	{ "KEYPAD 1", PINMAME_KEYCODE_KEYPAD_2, KEYCODE_1_PAD },
	{ "KEYPAD 2", PINMAME_KEYCODE_KEYPAD_2, KEYCODE_2_PAD },
	{ "KEYPAD 3", PINMAME_KEYCODE_KEYPAD_3, KEYCODE_3_PAD },
	{ "KEYPAD 4", PINMAME_KEYCODE_KEYPAD_4, KEYCODE_4_PAD },
	{ "KEYPAD 5", PINMAME_KEYCODE_KEYPAD_5, KEYCODE_5_PAD },
	{ "KEYPAD 6", PINMAME_KEYCODE_KEYPAD_6, KEYCODE_6_PAD },
	{ "KEYPAD 7", PINMAME_KEYCODE_KEYPAD_7, KEYCODE_7_PAD },
	{ "KEYPAD 8", PINMAME_KEYCODE_KEYPAD_8, KEYCODE_8_PAD },
	{ "KEYPAD 9", PINMAME_KEYCODE_KEYPAD_9, KEYCODE_9_PAD },
	{ "F1", PINMAME_KEYCODE_F1, KEYCODE_F1 },
	{ "F2", PINMAME_KEYCODE_F2, KEYCODE_F2 },
	{ "F3", PINMAME_KEYCODE_F3, KEYCODE_F3 },
	{ "F4", PINMAME_KEYCODE_F4, KEYCODE_F4 },
	{ "F5", PINMAME_KEYCODE_F5, KEYCODE_F5 },
	{ "F6", PINMAME_KEYCODE_F6, KEYCODE_F6 },
	{ "F7", PINMAME_KEYCODE_F7, KEYCODE_F7 },
	{ "F8", PINMAME_KEYCODE_F8, KEYCODE_F8 },
	{ "F9", PINMAME_KEYCODE_F9, KEYCODE_F9 },
	{ "F10", PINMAME_KEYCODE_F10, KEYCODE_F10 },
	{ "F11", PINMAME_KEYCODE_F11, KEYCODE_F11 },
	{ "F12", PINMAME_KEYCODE_F12, KEYCODE_F12 },
	{ "ESCAPE", PINMAME_KEYCODE_ESCAPE, KEYCODE_ESC },
	{ "GRAVE ACCENT", PINMAME_KEYCODE_GRAVE_ACCENT, KEYCODE_TILDE },
	{ "MINUS", PINMAME_KEYCODE_MINUS, KEYCODE_MINUS },
	{ "EQUALS", PINMAME_KEYCODE_EQUALS, KEYCODE_EQUALS },
	{ "BACKSPACE", PINMAME_KEYCODE_BACKSPACE, KEYCODE_BACKSPACE },
	{ "TAB", PINMAME_KEYCODE_TAB, KEYCODE_TAB },
	{ "LEFT BRACKET", PINMAME_KEYCODE_LEFT_BRACKET, KEYCODE_OPENBRACE },
	{ "RIGHT BRACKET", PINMAME_KEYCODE_RIGHT_BRACKET, KEYCODE_CLOSEBRACE },
	{ "ENTER", PINMAME_KEYCODE_ENTER, KEYCODE_ENTER },
	{ "SEMICOLON", PINMAME_KEYCODE_SEMICOLON, KEYCODE_COLON },
	{ "QUOTE", PINMAME_KEYCODE_QUOTE, KEYCODE_QUOTE },
	{ "BACKSLASH", PINMAME_KEYCODE_BACKSLASH, KEYCODE_BACKSLASH },
	{ "COMMA", PINMAME_KEYCODE_COMMA, KEYCODE_COMMA },
	{ "PERIOD", PINMAME_KEYCODE_PERIOD, KEYCODE_STOP },
	{ "SLASH", PINMAME_KEYCODE_SLASH, KEYCODE_SLASH },
	{ "SPACE", PINMAME_KEYCODE_SPACE, KEYCODE_SPACE },
	{ "INSERT", PINMAME_KEYCODE_INSERT, KEYCODE_INSERT },
	{ "DELETE", PINMAME_KEYCODE_DELETE, KEYCODE_DEL },
	{ "HOME", PINMAME_KEYCODE_HOME, KEYCODE_HOME },
	{ "END", PINMAME_KEYCODE_END, KEYCODE_END },
	{ "PAGE UP", PINMAME_KEYCODE_PAGE_UP, KEYCODE_PGUP },
	{ "PAGE DOWN", PINMAME_KEYCODE_PAGE_DOWN, KEYCODE_PGDN },
	{ "LEFT", PINMAME_KEYCODE_LEFT, KEYCODE_LEFT },
	{ "RIGHT", PINMAME_KEYCODE_RIGHT, KEYCODE_RIGHT },
	{ "UP", PINMAME_KEYCODE_UP, KEYCODE_UP },
	{ "DOWN", PINMAME_KEYCODE_DOWN, KEYCODE_DOWN },
	{ "KEYPAD DIVIDE", PINMAME_KEYCODE_KEYPAD_DIVIDE, KEYCODE_SLASH_PAD },
	{ "KEYPAD MULTIPLY", PINMAME_KEYCODE_KEYPAD_MULTIPLY, KEYCODE_ASTERISK },
	{ "KEYPAD SUBTRACT", PINMAME_KEYCODE_KEYPAD_SUBTRACT, KEYCODE_MINUS_PAD },
	{ "KEYPAD ADD", PINMAME_KEYCODE_KEYPAD_ADD, KEYCODE_PLUS_PAD },
	{ "KEYPAD ENTER", PINMAME_KEYCODE_KEYPAD_ENTER, KEYCODE_ENTER_PAD },
	{ "PRINT SCREEN", PINMAME_KEYCODE_PRINT_SCREEN, KEYCODE_PRTSCR },
	{ "PAUSE", PINMAME_KEYCODE_PAUSE, KEYCODE_PAUSE },
	{ "LEFT SHIFT", PINMAME_KEYCODE_LEFT_SHIFT, KEYCODE_LSHIFT },
	{ "RIGHT SHIFT", PINMAME_KEYCODE_RIGHT_SHIFT, KEYCODE_RSHIFT },
	{ "LEFT CONTROL", PINMAME_KEYCODE_LEFT_CONTROL, KEYCODE_LCONTROL },
	{ "RIGHT CONTROL", PINMAME_KEYCODE_RIGHT_CONTROL, KEYCODE_RCONTROL },
	{ "LEFT ALT", PINMAME_KEYCODE_LEFT_ALT, KEYCODE_LALT },
	{ "RIGHT ALT", PINMAME_KEYCODE_RIGHT_ALT, KEYCODE_RALT },
	{ "SCROLL LOCK", PINMAME_KEYCODE_SCROLL_LOCK, KEYCODE_SCRLOCK },
	{ "NUM LOCK", PINMAME_KEYCODE_NUM_LOCK, KEYCODE_NUMLOCK },
	{ "CAPS LOCK", PINMAME_KEYCODE_CAPS_LOCK, KEYCODE_CAPSLOCK },
	{ "LEFT SUPER", PINMAME_KEYCODE_LEFT_SUPER, KEYCODE_LWIN },
	{ "RIGHT SUPER", PINMAME_KEYCODE_RIGHT_SUPER, KEYCODE_RWIN },
	{ "MENU", PINMAME_KEYCODE_MENU, KEYCODE_MENU }
};

/******************************************************
 * ComposePath
 ******************************************************/

char* ComposePath(const char* const path, const char* const file)
{
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

int GetGameNumFromString(const char* const name)
{
	int gameNum = 0;

	while (drivers[gameNum]) {
		if (!strcasecmp(drivers[gameNum]->name, name))
			break;
		gameNum++;
	}

	if (!drivers[gameNum])
		return -1;

	return gameNum;
}

/******************************************************
 * GetDisplayCount
 ******************************************************/

int GetDisplayCount(const struct core_dispLayout* p_layout, int* const p_index)
{
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
 * UpdatePinmameDisplayBitmap
 ******************************************************/

int UpdatePinmameDisplayBitmap(PinmameDisplay* pDisplay, const struct mame_bitmap* p_bitmap)
{
	UINT8 r,g,b;
	UINT8* dst = (UINT8*)pDisplay->pData;
	int diff = 0;

	if (p_bitmap->depth == 8) {
		for(int j = 0; j < pDisplay->layout.height; j++) {
			UINT8* src = (UINT8*)p_bitmap->line[j];
			for(int i=0; i < pDisplay->layout.width; i++) {
				palette_get_color((*src++),&r,&g,&b);
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = 1;
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}
	else if(p_bitmap->depth == 15 || p_bitmap->depth == 16) {
		for(int j = 0; j < pDisplay->layout.height; j++) {
			UINT16* src = (UINT16*)p_bitmap->line[j];
			for(int i=0; i < pDisplay->layout.width; i++) {
				palette_get_color((*src++),&r,&g,&b);
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = 1;
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}
	else {
		for(int j = 0; j < pDisplay->layout.height; j++) {
			UINT32* src = (UINT32*)p_bitmap->line[j];
			for(int i=0; i < pDisplay->layout.width; i++) {
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = 1;
				palette_get_color((*src++),&r,&g,&b);
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}

	return diff;
}

/******************************************************
 * osd_init
 ******************************************************/

extern "C" int osd_init(void)
{
	return 0;
}

/******************************************************
 * osd_get_key_list
 ******************************************************/

extern "C" const struct KeyboardInfo* osd_get_key_list(void)
{
	return (const struct KeyboardInfo*)_keyboardInfo;
}

/******************************************************
 * osd_is_key_pressed
 ******************************************************/

extern "C" int osd_is_key_pressed(const int keycode)
{
	if (_p_Config->fn_IsKeyPressed)
		return (*(_p_Config->fn_IsKeyPressed))((PINMAME_KEYCODE)keycode, _p_userData);

	return 0;
}

/******************************************************
 * osd_readkey_unicode
 ******************************************************/

extern "C" int osd_readkey_unicode(const int flush)
{
	return 0;
}

/******************************************************
 * osd_start_audio_stream
 ******************************************************/

extern "C" int osd_start_audio_stream(const int stereo)
{
	if (!_p_Config->cb_OnAudioAvailable)
		return 0;

	memset(&_audioInfo, 0, sizeof(PinmameAudioInfo));
	_audioInfo.format = _p_Config->audioFormat;
	_audioInfo.channels = stereo ? 2 : 1;
	_audioInfo.sampleRate = Machine->sample_rate;
	_audioInfo.framesPerSecond = Machine->drv->frames_per_second;
	_audioInfo.samplesPerFrame = (int)(Machine->sample_rate / Machine->drv->frames_per_second);
	_audioInfo.bufferSize = PINMAME_ACCUMULATOR_SAMPLES * 2;

	return (*(_p_Config->cb_OnAudioAvailable))(&_audioInfo, _p_userData);
}

/******************************************************
 * osd_update_audio_stream
 ******************************************************/

extern "C" int osd_update_audio_stream(INT16* p_buffer)
{
	if(!_p_Config->cb_OnAudioUpdated || g_fSoundMode != PINMAME_SOUND_MODE_DEFAULT)
		return 0;

	const int samplesThisFrame = mixer_samples_this_frame();

	if (_p_Config->audioFormat == PINMAME_AUDIO_FORMAT_INT16)
		return (*(_p_Config->cb_OnAudioUpdated))((void*)p_buffer, samplesThisFrame, _p_userData);

	const int samplesEnd = samplesThisFrame * _audioInfo.channels;
	for (int i = 0; i < samplesEnd; i++)
		_audioData[i] = (float)p_buffer[i] * (float)(1.0/32768.0);

	return (*(_p_Config->cb_OnAudioUpdated))((void*)_audioData, samplesThisFrame, _p_userData);
}

/******************************************************
 * osd_stop_audio_stream
 ******************************************************/

extern "C" void osd_stop_audio_stream(void)
{
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

extern "C" void osd_exit(void)
{
}

/******************************************************
 * libpinmame_time_to_quit
 ******************************************************/

extern "C" int libpinmame_time_to_quit(void)
{
	return _timeToQuit;
}

/******************************************************
 * libpinmame_update_display
 ******************************************************/

extern "C" void libpinmame_update_display(const int index, const struct core_dispLayout* p_layout, void* p_data)
{
	PinmameDisplay* pDisplay = nullptr;

	if (_displays.size() < index + 1) {
		pDisplay = new PinmameDisplay();
		memset(pDisplay, 0, sizeof(PinmameDisplay));

		pDisplay->layout.type = (PINMAME_DISPLAY_TYPE)p_layout->type;
		pDisplay->layout.top = p_layout->top;
		pDisplay->layout.left = p_layout->left;

		if (p_layout->type == CORE_VIDEO) {
			pDisplay->layout.width = p_layout->length;
			pDisplay->layout.height = p_layout->start;

			pDisplay->layout.depth = 24;

			pDisplay->size = pDisplay->layout.width * pDisplay->layout.height * 3;
		}
		else if ((p_layout->type & CORE_DMD) == CORE_DMD) {
			pDisplay->layout.width = p_layout->length;
			pDisplay->layout.height = p_layout->start;

			if (p_layout->type & CORE_DMDSEG)
				pDisplay->layout.depth = 2;
			else {
				const int shade_16_enabled = ((core_gameData->gen & (GEN_SAM|GEN_SPA|GEN_ALVG_DMD2))
					|| (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0)
					|| (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0));
				pDisplay->layout.depth = shade_16_enabled ? 4 : 2;
			}

			pDisplay->size = pDisplay->layout.width * pDisplay->layout.height;
		}
		else {
			pDisplay->layout.length = p_layout->length;

			pDisplay->size = pDisplay->layout.length * sizeof(UINT16);
		}

		pDisplay->pData = malloc(pDisplay->size);
		memset(pDisplay->pData, 0, pDisplay->size);

		_displays.push_back(pDisplay);

		if (!_p_Config->cb_OnDisplayAvailable)
			return;

		int displayCountIndex = 0;
		const int displayCount = GetDisplayCount(core_gameData->lcdLayout, &displayCountIndex);

		(*(_p_Config->cb_OnDisplayAvailable))(index, displayCount, &pDisplay->layout, _p_userData);
	}
	else {
		if (!_p_Config->cb_OnDisplayUpdated)
			return;

		pDisplay = _displays[index];

		if (pDisplay->layout.type == CORE_VIDEO) {
			if (UpdatePinmameDisplayBitmap(pDisplay, (mame_bitmap*)p_data))
				(*(_p_Config->cb_OnDisplayUpdated))(index, pDisplay->pData, &pDisplay->layout, _p_userData);
			else
				(*(_p_Config->cb_OnDisplayUpdated))(index, nullptr, &pDisplay->layout, _p_userData);
		}
		else {
			if (memcmp(pDisplay->pData, p_data, pDisplay->size)) {
				memcpy(pDisplay->pData, p_data, pDisplay->size);
				(*(_p_Config->cb_OnDisplayUpdated))(index, pDisplay->pData, &pDisplay->layout, _p_userData);
			}
			else
				(*(_p_Config->cb_OnDisplayUpdated))(index, nullptr, &pDisplay->layout, _p_userData);
		}
	}
}

/******************************************************
 * libpinmame_snd_cmd_log
 ******************************************************/

extern "C" void libpinmame_snd_cmd_log(int boardNo, int cmd)
{
	if (!_p_Config->cb_OnSoundCommand)
		return;

	(*(_p_Config->cb_OnSoundCommand))(boardNo, cmd, _p_userData);
}

/******************************************************
 * libpinmame_forward_console_data
 ******************************************************/

extern "C" void libpinmame_forward_console_data(void* p_data, int size)
{
	if (!_p_Config->cb_OnConsoleDataUpdated)
		return;

	(*(_p_Config->cb_OnConsoleDataUpdated))(p_data, size, _p_userData);
}

/******************************************************
 * OnStateChange
 ******************************************************/

extern "C" void OnStateChange(const int state)
{
	_isRunning = state;

	if (!_p_Config->cb_OnStateUpdated)
		return;

	(*(_p_Config->cb_OnStateUpdated))(state, _p_userData);
}

/******************************************************
 * OnSolenoid
 ******************************************************/

extern "C" void OnSolenoid(const int solenoid, const int state)
{
	if (!_p_Config->cb_OnSolenoidUpdated)
		return;

	PinmameSolenoidState solenoidState;
	solenoidState.solNo = solenoid;
	solenoidState.state = state;

	(*(_p_Config->cb_OnSolenoidUpdated))(&solenoidState, _p_userData);
}

/******************************************************
 * libpinmame_log_info
 ******************************************************/

extern "C" void libpinmame_log_info(const char* format, ...)
{
	if (!_p_Config->cb_OnLogMessage)
		return;

	va_list args;
	va_start(args, format);
	(*(_p_Config->cb_OnLogMessage))(PINMAME_LOG_LEVEL_INFO, format, args, _p_userData);
	va_end(args);
}

/******************************************************
 * libpinmame_log_error
 ******************************************************/

extern "C" void libpinmame_log_error(const char* format, ...)
{
	if (!_p_Config->cb_OnLogMessage)
		return;

	va_list args;
	va_start(args, format);
	(*(_p_Config->cb_OnLogMessage))(PINMAME_LOG_LEVEL_ERROR, format, args, _p_userData);
	va_end(args);
}

/******************************************************
 * libpinmame_update_mech
 ******************************************************/

extern "C" void libpinmame_update_mech(const int mechNo, mech_tMechData* p_mechData)
{
	int speed = p_mechData->speed / p_mechData->ret;

	if (_mechInit[mechNo]) {
		if (_mechInfo[mechNo].pos != p_mechData->pos || _mechInfo[mechNo].speed != speed) {
			_mechInfo[mechNo].pos = p_mechData->pos;
			_mechInfo[mechNo].speed = speed;

			if (!_p_Config->cb_OnMechUpdated)
				return;

			if (g_fHandleMechanics == 0)
				(*(_p_Config->cb_OnMechUpdated))(mechNo - (MECH_MAXMECH / 2) + 1, &_mechInfo[mechNo], _p_userData);
			else
				(*(_p_Config->cb_OnMechUpdated))(mechNo, &_mechInfo[mechNo], _p_userData);
		}
	}
	else {
		_mechInit[mechNo] = 1;

		_mechInfo[mechNo].type = p_mechData->type;
		_mechInfo[mechNo].length = p_mechData->length;
		_mechInfo[mechNo].steps = p_mechData->steps;
		
		_mechInfo[mechNo].pos = p_mechData->pos;
		_mechInfo[mechNo].speed = speed;

		if (!_p_Config->cb_OnMechAvailable)
			return;

		if (g_fHandleMechanics == 0)
			(*(_p_Config->cb_OnMechAvailable))(mechNo - (MECH_MAXMECH / 2) + 1, &_mechInfo[mechNo], _p_userData);
		else
			(*(_p_Config->cb_OnMechAvailable))(mechNo, &_mechInfo[mechNo], _p_userData);
	}
}

/******************************************************
 * StartGame
 ******************************************************/

int StartGame(const int gameNum)
{
	int err;

	memset(_mechInit, 0, sizeof(_mechInit));
	memset(_mechInfo, 0, sizeof(_mechInfo));

	err = run_game(gameNum);

	OnStateChange(0);

	return err;
}

/******************************************************
 * PinmameGetGame
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameGetGame(const char* const p_name, PinmameGameCallback callback, const void* p_userData)
{
	if (!_p_Config)
		return PINMAME_STATUS_CONFIG_NOT_SET;

	int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0)
		return PINMAME_STATUS_GAME_NOT_FOUND;

	PinmameGame game;
	memset(&game, 0, sizeof(PinmameGame));

	game.name = drivers[gameNum]->name;
	if (drivers[gameNum]->clone_of)
		game.clone_of = drivers[gameNum]->clone_of->name;
	game.description = drivers[gameNum]->description;
	game.year = drivers[gameNum]->year;
	game.manufacturer = drivers[gameNum]->manufacturer;
	game.flags = drivers[gameNum]->flags;
	game.found = RomsetMissing(gameNum) == 0;

	if (callback)
		(*callback)(&game, p_userData);

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameGetGames
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameGetGames(PinmameGameCallback callback, const void* p_userData)
{
	if (!_p_Config)
		return PINMAME_STATUS_CONFIG_NOT_SET;

	int gameNum = 0;

	while (drivers[gameNum]) {
		PinmameGame game;
		memset(&game, 0, sizeof(PinmameGame));

		game.name = drivers[gameNum]->name;
		if (drivers[gameNum]->clone_of)
			game.clone_of = drivers[gameNum]->clone_of->name;
		game.description = drivers[gameNum]->description;
		game.year = drivers[gameNum]->year;
		game.manufacturer = drivers[gameNum]->manufacturer;
		game.flags = drivers[gameNum]->flags;
		game.found = RomsetMissing(gameNum) == 0;

		if (callback)
			(*callback)(&game, p_userData);

		gameNum++;
	}

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameSetConfig
 ******************************************************/

LIBPINMAME_API void PinmameSetConfig(const PinmameConfig* const p_config)
{
	if (!_p_Config)
		_p_Config = (PinmameConfig*)malloc(sizeof(PinmameConfig));

	memcpy(_p_Config, p_config, sizeof(PinmameConfig));

	libpinmame_log_info("PinmameSetConfig(): sampleRate=%d, vpmPath=%s", _p_Config->sampleRate, _p_Config->vpmPath);

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
 * PinmameSetPath
 ******************************************************/

LIBPINMAME_API void PinmameSetPath(const PINMAME_FILE_TYPE fileType, const char* const p_path)
{
	if (!p_path)
		return;

	char* const newPath = (char*)malloc(strlen(p_path) + 1);
	strcpy(newPath, p_path);

	switch(fileType) {
		case PINMAME_FILE_TYPE_ROMS:
			setPath(FILETYPE_ROM, newPath);
			break;
		case PINMAME_FILE_TYPE_NVRAM:
			setPath(FILETYPE_NVRAM, newPath);
			break;
		case PINMAME_FILE_TYPE_SAMPLES:
			setPath(FILETYPE_SAMPLE, newPath);
			break;
		case PINMAME_FILE_TYPE_CONFIG:
			setPath(FILETYPE_CONFIG, newPath);
			break;
		case PINMAME_FILE_TYPE_HIGHSCORE:
			setPath(FILETYPE_HIGHSCORE, newPath);
			break;
	}
}

/******************************************************
 * PinmameGetCheat
 ******************************************************/

LIBPINMAME_API int PinmameGetCheat()
{
	return options.cheat;
}

/******************************************************
 * PinmameSetCheat
 ******************************************************/

LIBPINMAME_API void PinmameSetCheat(const int cheat)
{
	options.cheat = cheat;
}

/******************************************************
 * PinmameGetHandleKeyboard
 ******************************************************/

LIBPINMAME_API int PinmameGetHandleKeyboard()
{
	return g_fHandleKeyboard;
}

/******************************************************
 * PinmameSetHandleKeyboard
 ******************************************************/

LIBPINMAME_API void PinmameSetHandleKeyboard(const int handleKeyboard)
{
	g_fHandleKeyboard = handleKeyboard ? 1 : 0;
}

/******************************************************
 * PinmameGetHandleMechanics
 ******************************************************/

LIBPINMAME_API int PinmameGetHandleMechanics()
{
	return g_fHandleMechanics;
}

/******************************************************
 * PinmameSetHandleMechanics
 ******************************************************/

LIBPINMAME_API void PinmameSetHandleMechanics(const int handleMechanics)
{
	g_fHandleMechanics = handleMechanics;
}

/******************************************************
 * PinmameSetDmdMode
 ******************************************************/

LIBPINMAME_API void PinmameSetDmdMode(const PINMAME_DMD_MODE dmdMode)
{
	g_fDmdMode = dmdMode;
}

/******************************************************
 * PinmameGetDmdMode
 ******************************************************/

LIBPINMAME_API PINMAME_DMD_MODE PinmameGetDmdMode()
{
	return g_fDmdMode;
}

/******************************************************
 * PinmameGetSoundMode
 ******************************************************/

LIBPINMAME_API PINMAME_SOUND_MODE PinmameGetSoundMode()
{
	return g_fSoundMode;
}

/******************************************************
 * PinmameSetSoundMode
 ******************************************************/

LIBPINMAME_API void PinmameSetSoundMode(const PINMAME_SOUND_MODE soundMode)
{
	g_fSoundMode = soundMode;
}

/******************************************************
 * PinmameRun
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameRun(const char* const p_name)
{
	if (!_p_Config)
		return PINMAME_STATUS_CONFIG_NOT_SET;

	if (_isRunning)
		return PINMAME_STATUS_GAME_ALREADY_RUNNING;

	const int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0)
		return PINMAME_STATUS_GAME_NOT_FOUND;

	vp_init();

	_p_gameThread = new std::thread(StartGame, gameNum);

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameIsRunning
 ******************************************************/

LIBPINMAME_API int PinmameIsRunning()
{
	return _isRunning;
}

/******************************************************
 * PinmameReset
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameReset()
{
	if (!_isRunning)
		return PINMAME_STATUS_EMULATOR_NOT_RUNNING;

	machine_reset();

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmamePause
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmamePause(const int pause)
{
	if (!_isRunning)
		return PINMAME_STATUS_EMULATOR_NOT_RUNNING;

	g_fPause = pause;

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameIsPaused
 ******************************************************/

LIBPINMAME_API int PinmameIsPaused()
{
	return g_fPause;
}

/******************************************************
 * PinmameStop
 ******************************************************/

LIBPINMAME_API void PinmameStop()
{
	if (!_p_gameThread)
		return;

	g_fPause = 0;
	_timeToQuit = 1;

	_p_gameThread->join();

	delete _p_gameThread;
	_p_gameThread = nullptr;

	_timeToQuit = 0;

	for (PinmameDisplay* pDisplay : _displays) {
		if (pDisplay->pData)
			free(pDisplay->pData);

		delete pDisplay;
	}

	_displays.clear();
}

/******************************************************
 * PinmameGetHardwareGen
 ******************************************************/

LIBPINMAME_API PINMAME_HARDWARE_GEN PinmameGetHardwareGen()
{
	const UINT64 hardwareGen = (_isRunning) ? core_gameData->gen : 0;
	return (PINMAME_HARDWARE_GEN)hardwareGen;
}

/******************************************************
 * PinmameGetSwitch
 ******************************************************/

LIBPINMAME_API int PinmameGetSwitch(const int swNo)
{
	return (_isRunning) ? vp_getSwitch(swNo) : 0;
}

/******************************************************
 * PinmameSetSwitch
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitch(const int swNo, const int state)
{
	if (!_isRunning)
		return;

	vp_putSwitch(swNo, state ? 1 : 0);
}

/******************************************************
 * PinmameSetSwitches
 ******************************************************/

LIBPINMAME_API void PinmameSetSwitches(const PinmameSwitchState* const p_states, const int numSwitches)
{
	if (!_isRunning)
		return;

	for (int i = 0; i < numSwitches; ++i)
		vp_putSwitch(p_states[i].swNo, p_states[i].state ? 1 : 0);
}

/******************************************************
 * PinmameGetSolenoidMask
 ******************************************************/

LIBPINMAME_API uint32_t PinmameGetSolenoidMask(const int low)
{
	return vp_getSolMask(low);
}

/******************************************************
 * PinmameSetSolenoidMask
 ******************************************************/

LIBPINMAME_API void PinmameSetSolenoidMask(const int low, const uint32_t mask)
{
	vp_setSolMask(low, mask);
}

/******************************************************
 * PinmameGetMaxSolenoids
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxSolenoids()
{
	return (CORE_MAXSOL + CORE_MODSOL_MAX);
}

/******************************************************
 * PinmameGetSolenoid
 ******************************************************/

LIBPINMAME_API int PinmameGetSolenoid(const int solNo)
{
	return (_isRunning) ? vp_getSolenoid(solNo) : 0;
}

/******************************************************
 * PinmameGetChangedSolenoids
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedSolenoids(PinmameSolenoidState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	vp_tChgSols chgSols;
	const int count = vp_getChangedSolenoids(chgSols);
	if (count > 0)
		memcpy(p_changedStates, chgSols, count * sizeof(PinmameSolenoidState));
	return count;
}

/******************************************************
 * PinmameGetMaxLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxLamps()
{
	return (CORE_MAXLAMPCOL * 8) + CORE_MAXRGBLAMPS;
}

/******************************************************
 * PinmameGetLamp
 ******************************************************/

LIBPINMAME_API int PinmameGetLamp(const int lampNo)
{
	return (_isRunning) ? vp_getLamp(lampNo) : 0;
}

/******************************************************
 * PinmameGetChangedLamps
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedLamps(PinmameLampState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	vp_tChgLamps chgLamps;
	const int count = vp_getChangedLamps(chgLamps);
	if (count > 0)
		memcpy(p_changedStates, chgLamps, count * sizeof(PinmameLampState));
	return count;
}

/******************************************************
 * PinmameGetMaxGIs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxGIs()
{
	return CORE_MAXGI;
}

/******************************************************
 * PinmameGetGI
 ******************************************************/

LIBPINMAME_API int PinmameGetGI(const int giNo)
{
	return (_isRunning) ? vp_getGI(giNo) : 0;
}

/******************************************************
 * PinmameGetChangedGIs
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedGIs(PinmameGIState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	vp_tChgGIs chgGIs;
	const int count = vp_getChangedGI(chgGIs);
	if (count > 0)
		memcpy(p_changedStates, chgGIs, count * sizeof(PinmameGIState));
	return count;
}

/******************************************************
 * PinmameGetMaxLEDs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxLEDs()
{
	return CORE_SEGCOUNT;
}

/******************************************************
 * PinmameGetChangedLEDs
 ******************************************************/

LIBPINMAME_API int PinmameGetChangedLEDs(const uint64_t mask, const uint64_t mask2, PinmameLEDState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	vp_tChgLED chgLEDs;
	const int count = vp_getChangedLEDs(chgLEDs, mask, mask2);
	if (count > 0)
		memcpy(p_changedStates, chgLEDs, count * sizeof(PinmameLEDState));
	return count;
}

/******************************************************
 * PinmameGetMaxMechs
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxMechs()
{
	return (MECH_MAXMECH / 2);
}

/******************************************************
 * PinmameGetMech
 ******************************************************/

LIBPINMAME_API int PinmameGetMech(const int mechNo)
{
	return (_isRunning) ? vp_getMech(mechNo) : 0;
}

/******************************************************
 * PinmameSetMech
 ******************************************************/

LIBPINMAME_API PINMAME_STATUS PinmameSetMech(const int mechNo, const PinmameMechConfig* const p_mechConfig)
{
	if (g_fHandleMechanics)
		return PINMAME_STATUS_MECH_HANDLE_MECHANICS;

	if (mechNo < 1 || mechNo > (MECH_MAXMECH / 2))
		return PINMAME_STATUS_MECH_NO_INVALID;

	mech_tInitData mechInitData;
	memset(&mechInitData, 0, sizeof(mech_tInitData));

	if (!p_mechConfig) {
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

	mech_add((MECH_MAXMECH / 2) + mechNo - 1, &mechInitData);

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameGetMaxSoundCommands
 ******************************************************/

LIBPINMAME_API int PinmameGetMaxSoundCommands()
{
	return MAX_CMD_LOG;
}

/******************************************************
 * PinmameGetNewSoundCommands
 ******************************************************/

LIBPINMAME_API int PinmameGetNewSoundCommands(PinmameSoundCommand* const p_newCommands)
{
	if (!_isRunning)
		return -1;

	vp_tChgSound chgSounds;
	const int count = vp_getNewSoundCommands(chgSounds);
	if (count > 0)
		memcpy(p_newCommands, chgSounds, count * sizeof(PinmameSoundCommand));
	return count;
}

/******************************************************
 * PinmameGetDIP
 ******************************************************/

LIBPINMAME_API int PinmameGetDIP(const int dipBank)
{
	return (_isRunning) ? vp_getDIP(dipBank) : 0;
}

/******************************************************
 * PinmameSetDIP
 ******************************************************/

LIBPINMAME_API void PinmameSetDIP(const int dipBank, const int value)
{
	if (!_isRunning)
		return;

	vp_setDIP(dipBank, value);
}

/******************************************************
 * PinmameSetUserData
 ******************************************************/

LIBPINMAME_API void PinmameSetUserData(const void* p_userData)
{
	_p_userData = (void*)p_userData;
}
