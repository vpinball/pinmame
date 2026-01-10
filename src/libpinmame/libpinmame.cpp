// license:BSD-3-Clause

#include "libpinmame.h"

#include "../../ext/libsamplerate/samplerate.h"

#include <thread>
#include <vector>
#include <algorithm>

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
int g_fHandleMechanics = 0xFF;
int g_fDumpFrames = 0;
int g_fPause = 0;
PINMAME_DMD_MODE g_fDmdMode = PINMAME_DMD_MODE_BRIGHTNESS;
PINMAME_SOUND_MODE g_fSoundMode = PINMAME_SOUND_MODE_DEFAULT;

char g_szGameName[256] = {0}; //!! not set yet
}

static int _isRunning = 0;
static int _timeToQuit = 0;
static PinmameConfig* _p_Config = nullptr;
static std::thread* _p_gameThread = nullptr;
static void* _p_userData = nullptr;

static char _aliasFromFile[50];

static int _mechInit[MECH_MAXMECH];
static PinmameMechInfo _mechInfo[MECH_MAXMECH];

static PinmameAudioInfo _audioInfo;
static float _audioData[PINMAME_ACCUMULATOR_SAMPLES * 2];

static int _nvramInit = 0;
static uint8_t _nvram[CORE_MAXNVRAM];
static PinmameNVRAMState _nvramState[CORE_MAXNVRAM];

typedef struct {
	PinmameDisplayLayout layout;
	void* pData;
	int size;
	unsigned int frameId;
} PinmameDisplay;

static std::vector<PinmameDisplay*> _displays;

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

// Controller plugin message support

#include "plugins/ControllerPlugin.h"

typedef enum DeviceMappingType {
   LPM_DM_CORE_SOL1,         // srcId = bit mask against coreGlobals.nSolenoids
   LPM_DM_CORE_SOL2,         // srcId = bit mask against coreGlobals.nSolenoids2
   LPM_DM_CORE_CUST_SOL,     // srcId = index to use in call to core_gameData->hw.getSol(srcId), core_gameData->hw.getSol may not be null is used
   LPM_DM_CORE_GI,           // srcId = GI index (note that this is 0 based)
   LPM_DM_CORE_LAMP,         // srcId = lamp index (note that this is 0 based)
   LPM_DM_CORE_MECH,         // srcId = mech index (note that this is 0 based)
   LPM_DM_CUSTOM_MECH_POS,   // srcId = mech index (note that user defined mech start at MECH_MAXMECH/2 = 5)
   LPM_DM_CUSTOM_MECH_SPEED, // srcId = mech index (note that user defined mech start at MECH_MAXMECH/2 = 5)
   LPM_DM_PHYSOUT,           // srcId = physic output index (ee CORE_MODOUT_SOL0, CORE_MODOUT_GI0,...)
   LPM_DM_PHYSOUT_HOLD,      // srcId = index of flipper hold solenoid, which must be followed by the power solenoid
} DeviceMappingType;

typedef struct DeviceMapping
{
   DeviceMappingType type;
   int srcId;  
} DeviceMapping;

static struct
{
   MsgPluginAPI* msgApi;
   unsigned int endpointId;
   int registered;

   unsigned int onGameStartId, onGameEndId;

   unsigned int onInputSrcChangedId, getInputSrcId;
   InputSrcId inputDef;

   unsigned int onDevSrcChangedId, getDevSrcId;
   DevSrcId deviceDef;
   DeviceMapping* deviceMap;

   int nSegDisplays; // Number of block displays
   struct
   {
      SegSrcId srcId;
      int sortedSegPos; // Position of first element in sortedSegLayout
      unsigned int segFrameId;
   } segDisplays[16];
   int nSortedSegLayout;
   struct {
      const core_dispLayout* srcLayout;
      int srcType;                        // source CORE_SEGxx after performing conversions
      int displayIndex;                   // Index of this display
      int nElements;                      // Number of elements forming this display
      int elementIndex;                   // Index of this element inside display
      int statePos;                       // Position of state
      SegElementType segType;
   } sortedSegLayout[CORE_SEGCOUNT]; // Sorted individual segment element
   unsigned int onSegSrcChangedId, getSegSrcId;
   float segLuminances[CORE_SEGCOUNT * 16];
   float segPrevLuminances[CORE_SEGCOUNT * 16];

   int nDisplays = 0;
   struct
   {
      DisplaySrcId srcId;
      const core_tLCDLayout* layout;
   } displays[8];
   unsigned int onDisplaySrcChangedId, getDisplaySrcId;
} msgLocals = { 0 };


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
 * CheckGameAlias
 ******************************************************/

const char* CheckGameAlias(const char* const romName)
{
	if (!_p_Config || !_p_Config->vpmPath[0])
		return romName;

	char aliasPath[PINMAME_MAX_PATH];
	strcpy(aliasPath, _p_Config->vpmPath);
	const size_t len = strlen(_p_Config->vpmPath);
	if (len > 0 && aliasPath[len - 1] != '/' && aliasPath[len - 1] != '\\')
		strcat(aliasPath, "/");
	strcat(aliasPath, "alias.txt");

	FILE* file = fopen(aliasPath, "r");

	if (file != NULL) {
		char line[128];
		while (fgets(line, sizeof(line), file)) {
			// Skip lines that start with "#"
			if (line[0] == '#')
				continue;

			char* token = strtok(line, ", ");

			if (!strcasecmp(token, romName))
			{
				strcpy(_aliasFromFile,  strtok(NULL, " ,\n#;'"));
				fclose(file);
				return _aliasFromFile;
			}
		}
		fclose(file);
	}
	return romName;
}

/******************************************************
 * GetGameNumFromString
 ******************************************************/

int GetGameNumFromString(const char* const name)
{
	int gameNum = 0;
	const char* gameName = CheckGameAlias(name);

	while (drivers[gameNum]) {
		if (!strcasecmp(drivers[gameNum]->name, gameName))
			break;
		gameNum++;
	}

	if (!drivers[gameNum])
		return -1;

	return gameNum;
}

/******************************************************
 * UpdatePinmameDisplayBitmap
 ******************************************************/

bool UpdatePinmameDisplayBitmap(PinmameDisplay* pDisplay, struct mame_bitmap* p_bitmap)
{
	UINT8* __restrict const dst = (UINT8*)pDisplay->pData;
	bool diff = false;
	const int height = pDisplay->layout.height; // layout->start
	const int width = pDisplay->layout.width; // layout->length
	const int rotation = (pDisplay->layout.type & PINMAME_DISPLAY_TYPE_VIDEO_ROT90) ? 1 : 0;
	const int incr = (rotation == 0) ? 3 : ((rotation == 1) ? height*3 : -height*3);
	for (int y = 0; y < height; y++) {
		int pos = (rotation == 0) ? y*width : ((rotation == 1) ? (height - 1 - y) : (y + (width - 1)*height));
		pos *= 3;
		for (int x = 0; x < width; x++,pos+=incr) {
			UINT8 r,g,b;
			palette_get_color(p_bitmap->read(p_bitmap, /*cliprect->min_x +*/ x, /*cliprect->min_y +*/ y), &r, &g, &b);
			diff |= (dst[pos] != r || dst[pos + 1] != g || dst[pos + 2] != b);
			dst[pos    ] = r;
			dst[pos + 1] = g;
			dst[pos + 2] = b;
		}
	}

	/* Optimized implementation but missing rotation support
	if (p_bitmap->depth == 8) {
		for(int j = 0; j < pDisplay->layout.height; j++) {
			const UINT8* __restrict src = (UINT8*)p_bitmap->line[j];
			for(int i=0; i < pDisplay->layout.width; i++) {
				UINT8 r,g,b;
				palette_get_color((*src++),&r,&g,&b);
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = true;
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}
	else if(p_bitmap->depth == 15 || p_bitmap->depth == 16) {
		for(int y = 0; y < pDisplay->layout.height; y++) {
			const UINT16* __restrict src = (UINT16*)p_bitmap->line[y];
			for(int x=0; x < pDisplay->layout.width; x++) {
				UINT8 r,g,b;
				palette_get_color((*src++),&r,&g,&b);
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = true;
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}
	else {
      for(int j = 0; j < pDisplay->layout.height; j++) {
			const UINT32* __restrict src = (UINT32*)p_bitmap->line[j];
			for(int i=0; i < pDisplay->layout.width; i++) {
				UINT8 r,g,b;
				palette_get_color((*src++),&r,&g,&b);
				if (dst[0] != r || dst[1] != g || dst[2] != b)
					diff = true;
				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
			}
		}
	}*/

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

	src_short_to_float_array(p_buffer, _audioData, samplesThisFrame * _audioInfo.channels);

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

extern "C" int libpinmame_needs_update_display() { return _p_Config->cb_OnDisplayUpdated != nullptr; }

extern "C" void libpinmame_update_display(const struct core_dispLayout* layout, void* p_data)
{
	// If layout is null, update the custom DMD generated from alphanumeric segment displays
	int index = layout == nullptr ? (_displays.size() - 1) : layout->index;
	PinmameDisplay* pDisplay = _displays[index];
	if ((pDisplay->layout.type & CORE_SEGMASK) == CORE_VIDEO) {
		const bool changed = UpdatePinmameDisplayBitmap(pDisplay, (mame_bitmap*)p_data);
		if (changed)
			pDisplay->frameId++;
		if (_p_Config->cb_OnDisplayUpdated)
			(*(_p_Config->cb_OnDisplayUpdated))(index, changed ? pDisplay->pData : nullptr, &pDisplay->layout, _p_userData);
	}
	else if (_p_Config->cb_OnDisplayUpdated) {
		if ((pDisplay->layout.type & CORE_SEGMASK) == CORE_DMD) {
			if (memcmp(pDisplay->pData, p_data, pDisplay->size)) {
				memcpy(pDisplay->pData, p_data, pDisplay->size);
				pDisplay->frameId++;
				(*(_p_Config->cb_OnDisplayUpdated))(index, pDisplay->pData, &pDisplay->layout, _p_userData);
			}
			else
				(*(_p_Config->cb_OnDisplayUpdated))(index, nullptr, &pDisplay->layout, _p_userData);
		}
		else
		{
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
static void OnGameStart(void*);
static void OnGameEnd(void*);
extern "C" void OnStateChange(const int state)
{
   if (_isRunning == state)
      return;

   _isRunning = state;

   if (msgLocals.msgApi != NULL)
   {
      switch (state)
      {
      case 0: msgLocals.msgApi->RunOnMainThread(0, OnGameEnd, nullptr); break;
      case 1: msgLocals.msgApi->RunOnMainThread(0, OnGameStart, nullptr); break;
      case 2: break; // Starting, just wait to be started, nothing to do
      case 3: break; // Stopping, it is invalid to call PinMAME emulation state but we are still registered
      }
   }
	if (_p_Config->cb_OnStateUpdated)
		(*(_p_Config->cb_OnStateUpdated))(state, _p_userData);

	if (state == 1)
	{
		int displayCount = 0;
		bool hasDMDOrVideo = false;
      for (const struct core_dispLayout * layout = core_gameData->lcdLayout, * parent_layout = NULL; layout->length || (parent_layout && parent_layout->length); layout += 1) {
			if (layout->length == 0) { // Recursive import
				layout = parent_layout;
				parent_layout = NULL;
			}
			if (layout->type == CORE_IMPORT) {
				assert(parent_layout == NULL); // IMPORT of IMPORT is not currently supported as it is not used by any driver so far
				parent_layout = layout + 1;
				layout = layout->importedLayout - 1;
				continue;
			}
			hasDMDOrVideo |= (layout->type == CORE_VIDEO) || ((layout->type & CORE_DMD) == CORE_DMD);
         if (displayCount <= layout->index)
            displayCount = layout->index + 1;
      }
      if (hasDMDOrVideo)
         displayCount++;
      _displays.resize(displayCount);
      
      for (const struct core_dispLayout* layout = core_gameData->lcdLayout, * parent_layout = NULL; layout->length || (parent_layout && parent_layout->length); layout += 1) {
			if (layout->length == 0) { // Recursive import
				layout = parent_layout;
				parent_layout = NULL;
			}
			if ((layout->type & CORE_SEGMASK) == CORE_IMPORT) {
				assert(parent_layout == NULL); // IMPORT of IMPORT is not currently supported as it is not used by any driver so far
				parent_layout = layout + 1;
				layout = layout->importedLayout - 1;
				continue;
			}

			PinmameDisplay* pDisplay = new PinmameDisplay();
			memset(pDisplay, 0, sizeof(PinmameDisplay));
			pDisplay->layout.type = (PINMAME_DISPLAY_TYPE)layout->type;
			pDisplay->layout.top = layout->top;
			pDisplay->layout.left = layout->left;
			if ((layout->type & CORE_SEGMASK) == CORE_VIDEO) {
				pDisplay->layout.width = layout->length;
				pDisplay->layout.height = layout->start;
				pDisplay->layout.depth = 24;
				pDisplay->size = pDisplay->layout.width * pDisplay->layout.height * 3;
			}
			else if ((layout->type & CORE_SEGMASK) == CORE_DMD) {
				pDisplay->layout.width = layout->length;
				pDisplay->layout.height = layout->start;
				const int shade_16_enabled = (core_gameData->gen & (GEN_SAM | GEN_SPA | GEN_ALVG | GEN_ALVG_DMD2 | GEN_GTS3)) != 0;
				pDisplay->layout.depth = shade_16_enabled ? 4 : 2;
				pDisplay->size = pDisplay->layout.width * pDisplay->layout.height;
			}
			else {
				pDisplay->layout.length = layout->length;
				pDisplay->size = pDisplay->layout.length * sizeof(UINT16);
			}
			pDisplay->pData = malloc(pDisplay->size);
			memset(pDisplay->pData, 0, pDisplay->size);

			_displays[layout->index] = pDisplay;

			if (_p_Config->cb_OnDisplayAvailable)
				(*(_p_Config->cb_OnDisplayAvailable))(layout->index, displayCount, &pDisplay->layout, _p_userData);
		}
		// Additional DMD generated from segment data
		if (!hasDMDOrVideo)
		{
			PinmameDisplay* pDisplay = new PinmameDisplay();
			memset(pDisplay, 0, sizeof(PinmameDisplay));
			pDisplay->layout.type = PINMAME_DISPLAY_TYPE_DMD;
			pDisplay->layout.top = 0;
			pDisplay->layout.left = 0;
			pDisplay->layout.width = 128;
			pDisplay->layout.height = 32;
			pDisplay->layout.depth = 2;
			pDisplay->size = pDisplay->layout.width * pDisplay->layout.height;
			pDisplay->pData = malloc(pDisplay->size);
			memset(pDisplay->pData, 0, pDisplay->size);
			_displays[displayCount - 1] = pDisplay;
			if (_p_Config->cb_OnDisplayAvailable)
				(*(_p_Config->cb_OnDisplayAvailable))(displayCount - 1, displayCount, &pDisplay->layout, _p_userData);
		}
	}
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

PINMAMEAPI PINMAME_STATUS PinmameGetGame(const char* const p_name, PinmameGameCallback callback, void* const p_userData)
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

PINMAMEAPI PINMAME_STATUS PinmameGetGames(PinmameGameCallback callback, void* const p_userData)
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

PINMAMEAPI void PinmameSetConfig(const PinmameConfig* const p_config)
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

PINMAMEAPI void PinmameSetPath(const PINMAME_FILE_TYPE fileType, const char* const p_path)
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

PINMAMEAPI int PinmameGetCheat()
{
	return options.cheat;
}

/******************************************************
 * PinmameSetCheat
 ******************************************************/

PINMAMEAPI void PinmameSetCheat(const int cheat)
{
	options.cheat = cheat;
}

/******************************************************
 * PinmameGetHandleKeyboard
 ******************************************************/

PINMAMEAPI int PinmameGetHandleKeyboard()
{
	return g_fHandleKeyboard;
}

/******************************************************
 * PinmameSetHandleKeyboard
 ******************************************************/

PINMAMEAPI void PinmameSetHandleKeyboard(const int handleKeyboard)
{
	g_fHandleKeyboard = handleKeyboard ? 1 : 0;
}

/******************************************************
 * PinmameGetHandleMechanics
 ******************************************************/

PINMAMEAPI int PinmameGetHandleMechanics()
{
	return g_fHandleMechanics;
}

/******************************************************
 * PinmameSetHandleMechanics
 ******************************************************/

PINMAMEAPI void PinmameSetHandleMechanics(const int handleMechanics)
{
	g_fHandleMechanics = handleMechanics;
}

/******************************************************
 * PinmameSetDmdMode
 ******************************************************/

PINMAMEAPI void PinmameSetDmdMode(const PINMAME_DMD_MODE dmdMode)
{
	g_fDmdMode = dmdMode;
}

/******************************************************
 * PinmameGetDmdMode
 ******************************************************/

PINMAMEAPI PINMAME_DMD_MODE PinmameGetDmdMode()
{
	return g_fDmdMode;
}

/******************************************************
 * PinmameGetSoundMode
 ******************************************************/

PINMAMEAPI PINMAME_SOUND_MODE PinmameGetSoundMode()
{
	return g_fSoundMode;
}

/******************************************************
 * PinmameSetSoundMode
 ******************************************************/

PINMAMEAPI void PinmameSetSoundMode(const PINMAME_SOUND_MODE soundMode)
{
	g_fSoundMode = soundMode;
}

/******************************************************
 * PinmameRun
 ******************************************************/

PINMAMEAPI PINMAME_STATUS PinmameRun(const char* const p_name)
{
	if (!_p_Config)
		return PINMAME_STATUS_CONFIG_NOT_SET;

	if (_isRunning)
		return PINMAME_STATUS_GAME_ALREADY_RUNNING;

	const int gameNum = GetGameNumFromString(p_name);

	if (gameNum < 0)
		return PINMAME_STATUS_GAME_NOT_FOUND;

	OnStateChange(2); // Starting state (in between stopped and started)

	vp_init();

	_p_gameThread = new std::thread(StartGame, gameNum);

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameIsRunning
 ******************************************************/

PINMAMEAPI int PinmameIsRunning()
{
	return _isRunning;
}

/******************************************************
 * PinmameReset
 ******************************************************/

PINMAMEAPI PINMAME_STATUS PinmameReset()
{
	if (!_isRunning)
		return PINMAME_STATUS_EMULATOR_NOT_RUNNING;

	machine_reset();

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmamePause
 ******************************************************/

PINMAMEAPI PINMAME_STATUS PinmamePause(const int pause)
{
	if (!_isRunning)
		return PINMAME_STATUS_EMULATOR_NOT_RUNNING;

	g_fPause = pause;

	return PINMAME_STATUS_OK;
}

/******************************************************
 * PinmameIsPaused
 ******************************************************/

PINMAMEAPI int PinmameIsPaused()
{
	return g_fPause;
}

/******************************************************
 * PinmameStop
 ******************************************************/

PINMAMEAPI void PinmameStop()
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
		if (pDisplay && pDisplay->pData)
			free(pDisplay->pData);

		delete pDisplay;
	}

	_displays.clear();
}

/******************************************************
 * PinmameGetHardwareGen
 ******************************************************/

PINMAMEAPI PINMAME_HARDWARE_GEN PinmameGetHardwareGen()
{
	const UINT64 hardwareGen = (_isRunning) ? core_gameData->gen : 0;
	return (PINMAME_HARDWARE_GEN)hardwareGen;
}

/******************************************************
 * PinmameGetSwitch
 ******************************************************/

PINMAMEAPI int PinmameGetSwitch(const int swNo)
{
	return (_isRunning) ? vp_getSwitch(swNo) : 0;
}

/******************************************************
 * PinmameSetSwitch
 ******************************************************/

PINMAMEAPI void PinmameSetSwitch(const int swNo, const int state)
{
	if (!_isRunning)
		return;

	vp_putSwitch(swNo, state ? 1 : 0);
}

/******************************************************
 * PinmameSetSwitches
 ******************************************************/

PINMAMEAPI void PinmameSetSwitches(const PinmameSwitchState* const p_states, const int numSwitches)
{
	if (!_isRunning)
		return;

	for (int i = 0; i < numSwitches; ++i)
		vp_putSwitch(p_states[i].swNo, p_states[i].state ? 1 : 0);
}

/******************************************************
 * PinmameGetSolenoidMask
 ******************************************************/

PINMAMEAPI uint32_t PinmameGetSolenoidMask(const int low)
{
	return vp_getSolMask(low);
}

/******************************************************
 * PinmameSetSolenoidMask
 ******************************************************/

PINMAMEAPI void PinmameSetSolenoidMask(const int low, const uint32_t mask)
{
	vp_setSolMask(low, mask);
}

/******************************************************
 * PinmameGetModOutputType
 ******************************************************/

PINMAMEAPI PINMAME_MOD_OUTPUT_TYPE PinmameGetModOutputType(const int output, const int no)
{
	return (PINMAME_MOD_OUTPUT_TYPE)vp_getModOutputType(output, no);
}

/******************************************************
 * PinmameSetModOutputType
 ******************************************************/

PINMAMEAPI void PinmameSetModOutputType(const int output, const int no, const PINMAME_MOD_OUTPUT_TYPE type)
{
	vp_setModOutputType(output, no, (int)type);
}

/******************************************************
 * PinmameSetTimeFence
 ******************************************************/

PINMAMEAPI void PinmameSetTimeFence(const double timeInS)
{
	vp_setTimeFence(timeInS);
}

/******************************************************
 * PinmameGetMaxSolenoids
 ******************************************************/

PINMAMEAPI int PinmameGetMaxSolenoids()
{
	return CORE_MODOUT_SOL_MAX;
}

/******************************************************
 * PinmameGetSolenoid
 ******************************************************/

PINMAMEAPI int PinmameGetSolenoid(const int solNo)
{
	if (!_isRunning)
		return 0;

	if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL))
		core_update_pwm_outputs(CORE_MODOUT_SOL0 + solNo - 1, 1);

	return vp_getSolenoid(solNo);
}

/******************************************************
 * PinmameGetChangedSolenoids
 ******************************************************/

PINMAMEAPI int PinmameGetChangedSolenoids(PinmameSolenoidState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	core_update_pwm_solenoids();

	vp_tChgSols chgSols;
	const int count = vp_getChangedSolenoids(chgSols);
	if (count > 0)
		memcpy(p_changedStates, chgSols, count * sizeof(PinmameSolenoidState));
	return count;
}

/******************************************************
 * PinmameGetMaxLamps
 ******************************************************/

PINMAMEAPI int PinmameGetMaxLamps()
{
	return CORE_MODOUT_LAMP_MAX;
}

/******************************************************
 * PinmameGetLamp
 ******************************************************/

PINMAMEAPI int PinmameGetLamp(const int lampNo)
{
	if (!_isRunning)
		return 0;

	if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_LAMPS))
		core_update_pwm_outputs(CORE_MODOUT_LAMP0 + lampNo - 1, 1);

	return vp_getLamp(lampNo);
}

/******************************************************
 * PinmameGetChangedLamps
 ******************************************************/

PINMAMEAPI int PinmameGetChangedLamps(PinmameLampState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	core_update_pwm_lamps();

	vp_tChgLamps chgLamps;
	const int count = vp_getChangedLamps(chgLamps);
	if (count > 0)
		memcpy(p_changedStates, chgLamps, count * sizeof(PinmameLampState));
	return count;
}

/******************************************************
 * PinmameGetMaxGIs
 ******************************************************/

PINMAMEAPI int PinmameGetMaxGIs()
{
	return CORE_MODOUT_GI_MAX;
}

/******************************************************
 * PinmameGetGI
 ******************************************************/

PINMAMEAPI int PinmameGetGI(const int giNo)
{
	if (!_isRunning)
		return 0;

	if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_GI))
		core_update_pwm_outputs(CORE_MODOUT_GI0 + giNo - 1, 1);

	return vp_getGI(giNo);
}

/******************************************************
 * PinmameGetChangedGIs
 ******************************************************/

PINMAMEAPI int PinmameGetChangedGIs(PinmameGIState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	core_update_pwm_gis();

	vp_tChgGIs chgGIs;
	const int count = vp_getChangedGI(chgGIs);
	if (count > 0)
		memcpy(p_changedStates, chgGIs, count * sizeof(PinmameGIState));
	return count;
}

/******************************************************
 * PinmameGetMaxLEDs
 ******************************************************/

PINMAMEAPI int PinmameGetMaxLEDs()
{
	return CORE_MODOUT_SEG_MAX;
}

/******************************************************
 * PinmameGetChangedLEDs
 ******************************************************/

PINMAMEAPI int PinmameGetChangedLEDs(const uint64_t mask, const uint64_t mask2, PinmameLEDState* const p_changedStates)
{
	if (!_isRunning)
		return -1;

	core_update_pwm_segments();

	vp_tChgLED chgLEDs;
	const int count = vp_getChangedLEDs(chgLEDs, mask, mask2);
	if (count > 0)
		memcpy(p_changedStates, chgLEDs, count * sizeof(PinmameLEDState));
	return count;
}

/******************************************************
 * PinmameGetMaxMechs
 ******************************************************/

PINMAMEAPI int PinmameGetMaxMechs()
{
	return (MECH_MAXMECH / 2);
}

/******************************************************
 * PinmameGetMech
 ******************************************************/

PINMAMEAPI int PinmameGetMech(const int mechNo)
{
	return (_isRunning) ? vp_getMech(mechNo) : 0;
}

/******************************************************
 * PinmameSetMech
 ******************************************************/

PINMAMEAPI PINMAME_STATUS PinmameSetMech(const int mechNo, const PinmameMechConfig* const p_mechConfig)
{
	if (g_fHandleMechanics)
		return PINMAME_STATUS_MECH_HANDLE_MECHANICS;

	if (mechNo < 1 || mechNo > (MECH_MAXMECH / 2))
		return PINMAME_STATUS_MECH_NO_INVALID;

	mech_tInitData mechInitData;
	memset(&mechInitData, 0, sizeof(mech_tInitData));

	if (p_mechConfig) {
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

PINMAMEAPI int PinmameGetMaxSoundCommands()
{
	return MAX_CMD_LOG;
}

/******************************************************
 * PinmameGetNewSoundCommands
 ******************************************************/

PINMAMEAPI int PinmameGetNewSoundCommands(PinmameSoundCommand* const p_newCommands)
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

PINMAMEAPI int PinmameGetDIP(const int dipBank)
{
	return (_isRunning) ? vp_getDIP(dipBank) : 0;
}

/******************************************************
 * PinmameSetDIP
 ******************************************************/

PINMAMEAPI void PinmameSetDIP(const int dipBank, const int value)
{
	if (!_isRunning)
		return;

	vp_setDIP(dipBank, value);
}

/******************************************************
 * PinmameGetMaxNVRAM
 ******************************************************/

PINMAMEAPI int PinmameGetMaxNVRAM()
{
	return CORE_MAXNVRAM;
}

/******************************************************
 * PinmameGetNVRAM
 ******************************************************/

PINMAMEAPI int PinmameGetNVRAM(PinmameNVRAMState* const p_nvramStates)
{
	if (!_isRunning)
		return -1;

	if (!(Machine && Machine->drv && Machine->drv->nvram_handler))
		return -1;

	mame_file* nvram_file = (mame_file*)malloc(sizeof(mame_file));
	memset(nvram_file, 0, sizeof(mame_file));
	nvram_file->type = RAM_FILE;
	(*Machine->drv->nvram_handler)(nvram_file, 1);

	if (nvram_file->offset == 0) {
		mame_fclose(nvram_file);
		return -1;
	}

	int size = std::min((int)nvram_file->offset, (int)CORE_MAXNVRAM);
	for (int i = 0; i < size; ++i) {
		p_nvramStates[i].nvramNo = i;
		p_nvramStates[i].currStat = nvram_file->data[i];
		p_nvramStates[i].oldStat = 0;
	}

	return size;
}

/******************************************************
 * PinmameGetChangedNVRAM
 ******************************************************/

PINMAMEAPI int PinmameGetChangedNVRAM(PinmameNVRAMState* const p_nvramStates)
{
	if (!_isRunning)
		return -1;

	if (!(Machine && Machine->drv && Machine->drv->nvram_handler))
		return -1;

	mame_file* nvram_file = (mame_file*)malloc(sizeof(mame_file));
	memset(nvram_file, 0, sizeof(mame_file));
	nvram_file->type = RAM_FILE;
	(*Machine->drv->nvram_handler)(nvram_file, 1);

	if (nvram_file->offset == 0) {
		mame_fclose(nvram_file);
		return -1;
	}

	int count = 0;
	int size = std::min((int)nvram_file->offset, (int)CORE_MAXNVRAM);

	if (_nvramInit == 0) {
		memcpy(_nvram, nvram_file->data, size);
		_nvramInit = 1;
	}
	else {
		for (int i = 0; i < size; ++i) {
			if (_nvram[i] != nvram_file->data[i]) {
				p_nvramStates[count].nvramNo = i;
				p_nvramStates[count].currStat = nvram_file->data[i];
				p_nvramStates[count].oldStat = _nvram[i];
				count++;

				_nvram[i] = nvram_file->data[i];
			}
		}
	}

	mame_fclose(nvram_file);

	return count;
}

/******************************************************
 * PinmameSetUserData
 ******************************************************/

PINMAMEAPI void PinmameSetUserData(void* const p_userData)
{
	_p_userData = (void*)p_userData;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// PinmameSetMsgAPI and Core MsgAPI implementation
//

///////////////////////////////////////////////////////////////////////////////////////////////////
// Controller inputs

static void OnGetInputSrc(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   GetInputSrcMsg* msg = (GetInputSrcMsg*)msgData;
   if (msgLocals.inputDef.nInputs > 0)
   {
      if (msg->count < msg->maxEntryCount)
         memcpy(&msg->entries[msg->count], &msgLocals.inputDef, sizeof(InputSrcId));
      msg->count++;
   }
}

int GetInputState(const unsigned int index)
{
   if (index >= msgLocals.inputDef.nInputs || _isRunning != 1)
      return 0;

   switch (msgLocals.inputDef.inputDefs[index].groupId)
   {
   case 0x0001:
      return core_getSw(msgLocals.inputDef.inputDefs[index].deviceId);
   
   case 0x0002:
   {
      const UINT8 bank = vp_getDIP(msgLocals.inputDef.inputDefs[index].deviceId / 8);
      const UINT8 mask = 1 << (msgLocals.inputDef.inputDefs[index].deviceId & 7);
      return (bank & mask) != 0;
   }
   
   default:
      return 0;
   }
}

void SetInputState(const unsigned int index, const int isSet)
{
   if (index >= msgLocals.inputDef.nInputs || _isRunning != 1)
      return;

   switch (msgLocals.inputDef.inputDefs[index].groupId)
   {
   case 0x0001:
      core_setSw(msgLocals.inputDef.inputDefs[index].deviceId, isSet);
      break;
   
   case 0x0002:
   {
      const UINT8 bank = vp_getDIP(msgLocals.inputDef.inputDefs[index].deviceId / 8);
      const UINT8 mask = 1 << (msgLocals.inputDef.inputDefs[index].deviceId & 7);
      if (isSet)
         vp_setDIP(bank, vp_getDIP(bank) | mask);
      else
         vp_setDIP(bank, vp_getDIP(bank) & ~mask);
      break;
   }
   
   default:
      break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device state

static void OnGetDevSrc(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   GetDevSrcMsg* msg = (GetDevSrcMsg*)msgData;
   if (msgLocals.deviceDef.nDevices > 0)
   {
      if (msg->count < msg->maxEntryCount)
         memcpy(&msg->entries[msg->count], &msgLocals.deviceDef, sizeof(DevSrcId));
      msg->count++;
   }
}

INLINE UINT8 saturatedByte(float v) { return (UINT8)(255.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v)); }

static uint8_t GetDeviceByteState(const unsigned int index)
{
   if (index >= msgLocals.deviceDef.nDevices || _isRunning != 1)
      return 0;

   switch (msgLocals.deviceMap[index].type)
   {
   case LPM_DM_CORE_SOL1:
      return (coreGlobals.solenoids & msgLocals.deviceMap[index].srcId) ? 255: 0;

   case LPM_DM_CORE_SOL2:
      return (coreGlobals.solenoids2 & msgLocals.deviceMap[index].srcId) ? 255 : 0;
      
   case LPM_DM_CORE_CUST_SOL:
      // TODO core_gameData->hw.getSol is supposed to return 0..255, but this would need to be checked on each driver...
      return core_gameData->hw.getSol(msgLocals.deviceMap[index].srcId);
      
   case LPM_DM_CORE_GI:
      if (core_gameData->gen & GEN_ALLWPC) // WPC GI level is 0..8
         return (coreGlobals.gi[msgLocals.deviceMap[index].srcId] * 255) / 8;
      else // Whitestar and SAM GI levels are either 0 or 9
         return coreGlobals.gi[msgLocals.deviceMap[index].srcId] == 0 ? 0 : 255;
      
   case LPM_DM_CORE_LAMP:
      return ((coreGlobals.lampMatrix[msgLocals.deviceMap[index].srcId / 8] >> (msgLocals.deviceMap[index].srcId % 8)) & 0x01) ? 255 : 0;
  
   case LPM_DM_PHYSOUT:
      core_update_pwm_outputs(msgLocals.deviceMap[index].srcId, 1);
      return saturatedByte(coreGlobals.physicOutputState[msgLocals.deviceMap[index].srcId].value);
      
   case LPM_DM_CORE_MECH:
      return core_gameData->hw.getMech ? core_gameData->hw.getMech(msgLocals.deviceMap[index].srcId) : 0;

   case LPM_DM_CUSTOM_MECH_POS:
      return mech_getPos(msgLocals.deviceMap[index].srcId);

   case LPM_DM_CUSTOM_MECH_SPEED:
      return mech_getSpeed(msgLocals.deviceMap[index].srcId);

   default:
      return 0;
   }
}

static float GetDeviceFloatState(const unsigned int index)
{
   if (index >= msgLocals.deviceDef.nDevices || _isRunning != 1)
      return 0.f;

   switch (msgLocals.deviceMap[index].type)
   {
   case LPM_DM_CORE_SOL1:
      return (coreGlobals.solenoids & msgLocals.deviceMap[index].srcId) ? 1.f : 0.f;

   case LPM_DM_CORE_SOL2:
      return (coreGlobals.solenoids2 & msgLocals.deviceMap[index].srcId) ? 1.f : 0.f;
      
   case LPM_DM_CORE_CUST_SOL:
      // TODO core_gameData->hw.getSol is supposed to return 0..255, but this would need to be checked on each driver...
      return ((float)core_gameData->hw.getSol(msgLocals.deviceMap[index].srcId)) / 255.f;
      
   case LPM_DM_CORE_GI:
      if (core_gameData->gen & GEN_ALLWPC) // WPC GI level is 0..8
         return ((float)coreGlobals.gi[msgLocals.deviceMap[index].srcId]) / 8.f;
      else // Whitestar and SAM GI levels are either 0 or 9
         return coreGlobals.gi[msgLocals.deviceMap[index].srcId] == 0 ? 0.f : 1.f;
      
   case LPM_DM_CORE_LAMP:
      return ((coreGlobals.lampMatrix[msgLocals.deviceMap[index].srcId / 8] >> (msgLocals.deviceMap[index].srcId % 8)) & 0x01) ? 1.f : 0.f;
  
   case LPM_DM_PHYSOUT:
      core_update_pwm_outputs(msgLocals.deviceMap[index].srcId, 1);
      return coreGlobals.physicOutputState[msgLocals.deviceMap[index].srcId].value;
      
   case LPM_DM_CORE_MECH:
      return (float)(core_gameData->hw.getMech ? core_gameData->hw.getMech(msgLocals.deviceMap[index].srcId) : 0);

   case LPM_DM_CUSTOM_MECH_POS:
      return mech_getFloatPos(msgLocals.deviceMap[index].srcId);

   case LPM_DM_CUSTOM_MECH_SPEED:
      return mech_getFloatSpeed(msgLocals.deviceMap[index].srcId);

   default:
      return 0.f;
   }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Alphanumeric segment displays

static void OnGetSegSrc(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   GetSegSrcMsg* msg = (GetSegSrcMsg*)msgData;
   for (unsigned int index = 0; index < msgLocals.nSegDisplays; index++, msg->count++)
      if (msg->count < msg->maxEntryCount)
         memcpy(&msg->entries[msg->count], &msgLocals.segDisplays[index].srcId, sizeof(SegSrcId));
}

static SegDisplayFrame GetSegDisplay(const CtlResId id)
{
   assert(id.endpointId == msgLocals.endpointId);
   assert(id.resId < msgLocals.nSegDisplays);
   
   const int startElement = msgLocals.segDisplays[id.resId].sortedSegPos;
   const int nElements = msgLocals.segDisplays[id.resId].srcId.nElements;
   
   if (_isRunning != 1)
      return { msgLocals.segDisplays[id.resId].segFrameId, msgLocals.segLuminances + (startElement * 16) };
   
   static int nSegments[] = { 16, 16, 10, 9, 8, 8, 7, 8, 7, 10, 9, 7, 8, 16, 0, 0, 15, 15 }; // Number of segments (including dot/comma) corresponding to CORE_SEGxx
   for (int i = startElement; i < startElement + nElements; i++)
   {
      const int type = msgLocals.sortedSegLayout[i].srcLayout->type & CORE_SEGALL;
      assert(type < sizeof(nSegments) / sizeof(nSegments[0]));
      const int nSegs = nSegments[type];
      if (coreGlobals.nAlphaSegs) // Always return modulated value if available
      {
         int pos = CORE_MODOUT_SEG0 + msgLocals.sortedSegLayout[i].statePos * 16;
         if (msgLocals.sortedSegLayout[i].srcLayout->type & CORE_SEGHIBIT) pos += 8;
         for (int j = 0; j < nSegs; j++, pos++) // Loop over each segments of the current character (up to 16)
         {
            core_update_pwm_outputs(pos, 1);
            msgLocals.segLuminances[i * 16 + j] = coreGlobals.physicOutputState[pos].value;
         }
      }
      else
      {
         UINT16 segs = coreGlobals.segments[msgLocals.sortedSegLayout[i].statePos].w;
         if (msgLocals.sortedSegLayout[i].srcLayout->type & CORE_SEGHIBIT) segs >>= 8;
         for (int j = 0; j < nSegs; j++, segs >>= 1) // Loop over each segments of the current character (up to 16)
            msgLocals.segLuminances[i * 16 + j] = (segs & 1) ? 1.f : 0.f;
      }
      
      if ((type == CORE_SEG9) || (type == CORE_SEG98) || (type == CORE_SEG98F)) {
         // Bottom half of vertical center is controlled by upper half
         msgLocals.segLuminances[i * 16 + 9] = msgLocals.segLuminances[i * 16 + 8];
      }
      if (type == CORE_SEG16R) {
         // Reverse comma / dot
         float v = msgLocals.segLuminances[i * 16 + 15];
         msgLocals.segLuminances[i * 16 + 15] = msgLocals.segLuminances[i * 16 + 7];
         msgLocals.segLuminances[i * 16 + 7] = v;
      }
      if ((type == CORE_SEG98F) || (type == CORE_SEG87F)) {
         // Comma is lit if at least one segment is on
         msgLocals.segLuminances[i * 16 + 7] = 0.f;
         for (int j = 0; j < nSegs; j++)
            if (msgLocals.segLuminances[i * 16 + j] > msgLocals.segLuminances[i * 16 + 7])
               msgLocals.segLuminances[i * 16 + 7] = msgLocals.segLuminances[i * 16 + j];
      }
   }
   
   if (memcmp(msgLocals.segPrevLuminances + (startElement * 16), msgLocals.segLuminances + (startElement * 16), nElements * 16 * sizeof(float)) != 0)
   {
      memcpy(msgLocals.segPrevLuminances + (startElement * 16), msgLocals.segLuminances + (startElement * 16), nElements * 16 * sizeof(float));
      msgLocals.segDisplays[id.resId].segFrameId++;
   }
   
   return { msgLocals.segDisplays[id.resId].segFrameId, msgLocals.segLuminances + (startElement * 16) };
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Video & Dot Matrix Displays

static void OnGetDisplaySrc(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   GetDisplaySrcMsg* msg = static_cast<GetDisplaySrcMsg*>(msgData);
   for (unsigned int index = 0; index < msgLocals.nDisplays; index++, msg->count++)
      if (msg->count < msg->maxEntryCount)
         memcpy(&msg->entries[msg->count], &msgLocals.displays[index].srcId, sizeof(DisplaySrcId));
}

static DisplayFrame GetDisplayFrame(const CtlResId id)
{
   if ((id.endpointId != msgLocals.endpointId) || (id.resId >= msgLocals.nDisplays) || (_isRunning != 1))
      return { 0, nullptr };
   if ((msgLocals.displays[id.resId].layout->type & CORE_SEGMASK) == CORE_VIDEO) {
      const PinmameDisplay* pDisplay = _displays[msgLocals.displays[id.resId].layout->index];
      return { pDisplay->frameId, pDisplay->pData };
   }
   else {
      unsigned int frameId;
      const float* lumFrame = core_dmd_update_pwm(msgLocals.displays[id.resId].layout, &frameId);
      return { frameId, lumFrame };
   }
}

static DisplayFrame GetDisplayIdFrame(const CtlResId id)
{
   if ((id.endpointId != msgLocals.endpointId) || (id.resId >= msgLocals.nDisplays) || (_isRunning != 1))
      return { 0, nullptr };
   unsigned int frameId;
   const UINT8* rawFrame = core_dmd_update_identify(msgLocals.displays[id.resId].layout, &frameId);
   return { frameId, rawFrame };
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Overall game messages

static char* fmtString(const char *format, ...) {
    va_list args;

    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args) + 1; // +1 for the null terminator
    va_end(args);

    va_start(args, format);
    char *formatted_string = (char *)malloc(size);
    vsnprintf(formatted_string, size, format, args);
    va_end(args);

    return formatted_string;
}

static void SetupMsgApi()
{
   if (msgLocals.msgApi == NULL)
      return;
   if (_isRunning != 1)
      return;

   assert(msgLocals.registered == 0);
   msgLocals.registered = 1;

   msgLocals.onGameStartId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_EVT_ON_GAME_START);
   msgLocals.onGameEndId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_EVT_ON_GAME_END);

   // -- Prepare data structures for displays
   msgLocals.nDisplays = 0;
   int nSegLayouts = 0;
   const core_tLCDLayout* segLayout[128] = { 0 };
   memset(msgLocals.displays, 0, sizeof(msgLocals.displays));
   for (const core_dispLayout * layout = core_gameData->lcdLayout, * parent_layout = NULL; layout->length || (parent_layout && parent_layout->length); layout++) {
      if (layout->length == 0) { layout = parent_layout; parent_layout = NULL; }
      switch (layout->type & CORE_SEGMASK)
      {
      case CORE_IMPORT: assert(parent_layout == NULL); parent_layout = layout + 1; layout = layout->importedLayout - 1; break;
      case CORE_DMD: // DMD displays and LED matrices (for example RBION,... search for CORE_NODISP to list them)
      case CORE_VIDEO: // Video display for games like Baby PacMan, frames are stored as RGB8
         if ((layout->type & CORE_SEGMASK) == CORE_VIDEO)
         {
            if (layout->type & CORE_VIDEO_ROT90)
            {
               msgLocals.displays[msgLocals.nDisplays].srcId.width = layout->start;
               msgLocals.displays[msgLocals.nDisplays].srcId.height = layout->length;
            }
            else
            {
               msgLocals.displays[msgLocals.nDisplays].srcId.width = layout->length;
               msgLocals.displays[msgLocals.nDisplays].srcId.height = layout->start;
            }
         }
         else
         {
            msgLocals.displays[msgLocals.nDisplays].srcId.width = layout->length;
            msgLocals.displays[msgLocals.nDisplays].srcId.height = layout->start;
         }
         msgLocals.displays[msgLocals.nDisplays].layout = layout;
         msgLocals.displays[msgLocals.nDisplays].srcId.id = { msgLocals.endpointId, static_cast<uint32_t>(msgLocals.nDisplays) };
         msgLocals.displays[msgLocals.nDisplays].srcId.groupId = { msgLocals.endpointId, 0 };
         if ((layout->type & CORE_SEGMASK) == CORE_VIDEO)
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_UNKNOWN; // Report a CRT display ?
         else if ((layout->length < 128) || (layout->start < 16))
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_UNKNOWN; // Mini display, usually LEDs
         else if (core_gameData->gen == GEN_SAM)
            // TODO return the right information:
            // - Before POTC, all tables used Neon Plasma display
            // - Then, due to RoHS, european versions of POTC to Family Guy use a modified PinLED display
            //   Then the 520-5052-05 red led matrix is used
            //   Then, starting with Tranformers, the 520-5052-15 orange/red led matrix is used
            // - All US Stern games before AC/DC use a 128 x 32 neon plasma (520-5052-00), then LED (520-5052-15)
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_UNKNOWN;
         else if (core_gameData->gen == GEN_SPA)
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_UNKNOWN;
         else
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_NEON_PLASMA;
         msgLocals.displays[msgLocals.nDisplays].srcId.frameFormat = ((layout->type & CORE_SEGMASK) == CORE_VIDEO) ? CTLPI_DISPLAY_FORMAT_SRGB888 : CTLPI_DISPLAY_FORMAT_LUM8;
         msgLocals.displays[msgLocals.nDisplays].srcId.GetRenderFrame = &GetDisplayFrame;
         if ((layout->type & CORE_SEGMASK) != CORE_VIDEO)
         {
            msgLocals.displays[msgLocals.nDisplays].srcId.identifyFormat = ((core_gameData->gen & (GEN_SAM | GEN_SPA | GEN_ALVG_DMD2)) || (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) || (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0)) ? CTLPI_DISPLAY_ID_FORMAT_BITPLANE4 : CTLPI_DISPLAY_ID_FORMAT_BITPLANE2;
            msgLocals.displays[msgLocals.nDisplays].srcId.GetIdentifyFrame = &GetDisplayIdFrame;
         }
         msgLocals.nDisplays++;
         break;
      default: // Alphanumeric segment displays
         segLayout[nSegLayouts] = layout;
         nSegLayouts++;
         break; 
      }
   }
   if (msgLocals.nDisplays > 0)
   {
      msgLocals.onDisplaySrcChangedId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_DISPLAY_ON_SRC_CHG_MSG);
      msgLocals.getDisplaySrcId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_DISPLAY_GET_SRC_MSG);
      msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.getDisplaySrcId, OnGetDisplaySrc, NULL);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDisplaySrcChangedId, NULL);
   }
   
   // -- Prepare data structures for segment displays
   // Layout declaration in drivers were originaly made for rendering and are (sadly) also used to unswizzle alphanum segment data.
   // We need to interpret them to build back the displays list with their individual components (for example see Space Gambler or WPC).
   // The CORE_SEG mask also mix segment layouts (number of segment, with/without dot & comma, ...) with segment addressing (which output
   // drive the segment, is the segment driven together with another segment, ...). The CORE_SEG mask can also include a comma every 
   // three digit information, and the CORE_SEGREV flag which indicates that the memory position is in reversed order.
   // We resolve all these to simply expose physical layouts, with stable output order. To do so, we convert them to individual 
   // elements and group them based on their declaration order and render position, then process the additional flags at setup here,
   // or when accessing data (for example, to process shared segment command).
   msgLocals.nSortedSegLayout = 0;
   memset(msgLocals.sortedSegLayout, 0, sizeof(msgLocals.sortedSegLayout));
   for (int i = 0; i < nSegLayouts; i++)
   {
      // Split layout into individual components, converting type to the plugin API enum, eventually applying forced commas and reversed order
      for (int j = 0; j < segLayout[i]->length; j++)
      {
         assert(msgLocals.nSortedSegLayout < CORE_SEGCOUNT);
         msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcLayout = segLayout[i];
         msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = segLayout[i]->type;
         SegElementType type;
         int isThousands = ((segLayout[i]->length - 1 - j) > 0) && ((segLayout[i]->length - 1 - j) % 3 == 0);
         switch (segLayout[i]->type & CORE_SEGALL) {
         case CORE_SEG7:   type = CTLPI_SEG_LAYOUT_7; break;
         case CORE_SEG7S:  type = CTLPI_SEG_LAYOUT_7; break;
         case CORE_SEG7SC: type = CTLPI_SEG_LAYOUT_7; break;
         case CORE_SEG87F:
            type = isThousands ? CTLPI_SEG_LAYOUT_7C : CTLPI_SEG_LAYOUT_7;
            if (!isThousands)
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG7 | (segLayout[i]->type & ~CORE_SEGALL);
            break;
         case CORE_SEG87:
            type = isThousands ? CTLPI_SEG_LAYOUT_7C : CTLPI_SEG_LAYOUT_7; break;
            if (!isThousands)
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG7 | (segLayout[i]->type & ~CORE_SEGALL);
            else
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG8 | (segLayout[i]->type & ~CORE_SEGALL);
            break;
         case CORE_SEG8:   type = CTLPI_SEG_LAYOUT_7C; break;
         case CORE_SEG8D:  type = CTLPI_SEG_LAYOUT_7D; break;
         case CORE_SEG9:   type = CTLPI_SEG_LAYOUT_9; break;
         case CORE_SEG10:  type = CTLPI_SEG_LAYOUT_9C; break;
         case CORE_SEG98F:
            type = isThousands ? CTLPI_SEG_LAYOUT_9C : CTLPI_SEG_LAYOUT_9;
            if (!isThousands)
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG9 | (segLayout[i]->type & ~CORE_SEGALL);
            break;
         case CORE_SEG98:
            type = isThousands ? CTLPI_SEG_LAYOUT_9C : CTLPI_SEG_LAYOUT_9;
            if (!isThousands)
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG9 | (segLayout[i]->type & ~CORE_SEGALL);
            else
               msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].srcType = CORE_SEG10 | (segLayout[i]->type & ~CORE_SEGALL);
            break;
         case CORE_SEG16N: type = CTLPI_SEG_LAYOUT_14; break;
         case CORE_SEG16D: type = CTLPI_SEG_LAYOUT_14D; break;
         case CORE_SEG16:  type = CTLPI_SEG_LAYOUT_14DC; break;
         case CORE_SEG16R: type = CTLPI_SEG_LAYOUT_14DC; break;
         case CORE_SEG16S: type = CTLPI_SEG_LAYOUT_16; break;
         }
         msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].segType = type;
         msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].displayIndex = segLayout[i]->left + j * 2;
         if (segLayout[i]->type & CORE_SEGREV)
            msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].statePos = segLayout[i]->start + segLayout[i]->length - 1 - j;
         else
            msgLocals.sortedSegLayout[msgLocals.nSortedSegLayout].statePos = segLayout[i]->start + j;
         msgLocals.nSortedSegLayout++;
      }
   }
   msgLocals.nSegDisplays = 0;
   int segDisplayStart = 0;
   memset(msgLocals.segDisplays, 0, sizeof(msgLocals.segDisplays));
   for (int i = 0; i < msgLocals.nSortedSegLayout; i++)
   {
      if ((i == msgLocals.nSortedSegLayout - 1) // Last element
         || (msgLocals.sortedSegLayout[i].srcLayout->top != msgLocals.sortedSegLayout[i + 1].srcLayout->top) // Next element is on another line
         || (msgLocals.sortedSegLayout[i].displayIndex + 2 != msgLocals.sortedSegLayout[i + 1].displayIndex)) // There is gap before next element
         // end of block could also be a change of element size (based on element type) but does not seems to be used
      {
         msgLocals.segDisplays[msgLocals.nSegDisplays].sortedSegPos = segDisplayStart;
         msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.id = { msgLocals.endpointId, static_cast<uint32_t>(msgLocals.nSegDisplays) };
         msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.groupId = { msgLocals.endpointId, 0 };
         msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements = i + 1 - segDisplayStart;
         msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.GetState = &GetSegDisplay;
         switch (core_gameData->gen)
         { // TODO review and implement more hardware hints (and maybe move all hardware definitions to drivers)
         case GEN_BY17:
         case GEN_BY35:
         case GEN_BY6803:
         case GEN_BY6803A:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = CTLPI_SEG_HARDWARE_NEON_PLASMA;
            break;

         case GEN_S3:
         case GEN_S3C:
         case GEN_S4:
         case GEN_S6:
         case GEN_S7:
         case GEN_S9:
         case GEN_S11:
         case GEN_S11X: // GEN_S11A & GEN_S11B
         case GEN_S11B2:
         case GEN_S11C:
         case GEN_DE:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = CTLPI_SEG_HARDWARE_NEON_PLASMA;
            break;
            
         case GEN_WPCALPHA_1:
         case GEN_WPCALPHA_2:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = CTLPI_SEG_HARDWARE_NEON_PLASMA;
            break;
            
         case GEN_STMPU100:
         case GEN_STMPU200:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = CTLPI_SEG_HARDWARE_NEON_PLASMA;
            break;

         case GEN_GTS1:
         case GEN_GTS80:
         case GEN_GTS80B:
         case GEN_GTS3:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements == 4 ? CTLPI_SEG_HARDWARE_GTS1_4DIGIT
                                                                         : msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements == 6 ? CTLPI_SEG_HARDWARE_GTS1_6DIGIT
                                                                         : msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements == 7 ? CTLPI_SEG_HARDWARE_GTS80A_7DIGIT
                                                                         : msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements == 20 ? CTLPI_SEG_HARDWARE_GTS80B_20DIGIT
                                                                         : CTLPI_SEG_HARDWARE_UNKNOWN; // This one should not happen but need to be checked (some playfield LED displays maybe ?)
            break;
            
         default:
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.hardware = CTLPI_SEG_HARDWARE_UNKNOWN;
            break;
         }
         for (int j = segDisplayStart; j <= i; j++)
         {
            msgLocals.sortedSegLayout[j].displayIndex = msgLocals.nSegDisplays;
            msgLocals.sortedSegLayout[j].elementIndex = j - segDisplayStart;
            msgLocals.sortedSegLayout[j].nElements = msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements;
            msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.elementType[j - segDisplayStart] = msgLocals.sortedSegLayout[j].segType;
         }
         segDisplayStart = i + 1;
         msgLocals.nSegDisplays++;
      }
   }
   if (msgLocals.nSegDisplays > 0)
   {
      msgLocals.onSegSrcChangedId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_SEG_ON_SRC_CHG_MSG);
      msgLocals.getSegSrcId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_SEG_GET_SRC_MSG);
      msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.getSegSrcId, OnGetSegSrc, nullptr);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onSegSrcChangedId, nullptr);
   }

   // -- Controlled devices
   // The existing output layout is the result of years of evolution, starting with the WPC hardware (hence
   // the dedicated GI outputs with 0..8 values), then adding different levels of device emulation (merging
   // binary output states over a given period, some smoothing, physic model,...) and solving conflicts
   // by moving the outputs to free slots. This 'legacy' mapping is preserved for backward compatibility:
   // - First devices correspond to 'legacy' solenoids outputs, keeping the existing ordering,
   // - Followed by GI bulbs, identified by their group id, with the original ordering,
   // - Followed by lamp matrix, identified by their group Id and device Id which match the lamp number in
   //   the matrix (which has always been different from the lamp index, as lamps are organized by rows of
   //   8 lamps, for example, first lamp is usually lamp #11)
   //
   // The following group ids are defined and used by PinMame:
   // - 0x0000 unwired
   // - 0x0001 solenoids/flashers driven by power driver board (main high/low current outputs)
   // - 0x0002 auxiliary boards (magnets, outputs, WPC fliptronics, ...)
   // - 0x0003 custom driver outputs (see CORE_FIRSTCUSTSOL)
   // - 0x0010 PinMame internal state: for example emulated plunger, or fake 'game on' solenoid used for fast flippers, ...
   // - 0x0100 GI driven by power driver board (WPC, Whitestar & SAM are the only one with dedicated GI
   //          outputs, other hardwares use generic outputs to drive a GI relay/thyristor)
   // - 0x0200 lamp matrix driven by power driver board
   // - 0x0300 emulated mechanical parts
   //
   // Existing state access is implemented in core_getSol, core_getAllSol and core_getAllPhysicSols. Sadly,
   // these functions do not always return the same value. When difference exists, the implementation of 
   // core_getAllSol is taken as it is supposed to be the most widely used.
   //
   const int hasSAMModulatedLeds = (core_gameData->gen & GEN_SAM) && (core_gameData->hw.lampCol > 2);
   const int nLamps = (hasSAMModulatedLeds || coreGlobals.nLamps) ? coreGlobals.nLamps : (8 * (CORE_CUSTLAMPCOL + core_gameData->hw.lampCol));
   const int nSols = (coreGlobals.nSolenoids ? coreGlobals.nSolenoids : (CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol));
   msgLocals.deviceDef.id.endpointId = msgLocals.endpointId;
   msgLocals.deviceDef.id.resId = 0;
   msgLocals.deviceDef.GetByteState = &GetDeviceByteState;
   msgLocals.deviceDef.GetFloatState = &GetDeviceFloatState;
   msgLocals.deviceDef.nDevices = nSols + coreGlobals.nGI + nLamps + MECH_MAXMECH;
   msgLocals.deviceDef.deviceDefs = (DeviceDef *)malloc(msgLocals.deviceDef.nDevices * sizeof(DeviceDef));
   memset(msgLocals.deviceDef.deviceDefs, 0, msgLocals.deviceDef.nDevices * sizeof(DeviceDef));
   msgLocals.deviceMap = (DeviceMapping *)malloc(msgLocals.deviceDef.nDevices * sizeof(DeviceMapping));
   memset(msgLocals.deviceMap, 0, msgLocals.deviceDef.nDevices * sizeof(DeviceMapping));
   const int isPhysSol = (coreGlobals.nSolenoids > 0) && ((options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)) != 0);
   // 1..28, solenoid/flasher outputs from driver board
   for (int i = 0; i < 28; i++)
   {
      msgLocals.deviceDef.deviceDefs[i].name = fmtString("Sol #%02d", i + 1);
      msgLocals.deviceDef.deviceDefs[i].groupId = 0x0001;
      msgLocals.deviceDef.deviceDefs[i].deviceId = i + 1;
      msgLocals.deviceMap[i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL1;
      msgLocals.deviceMap[i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + i) : (1 << i);
   }
   // 29..32, WPC: fake GameOn solenoids for fast flip (not modulated, stored in 0x0F00 of solenoids2, why 4 bits (actually only 3) ?)
   if (core_gameData->gen & GEN_ALLWPC)
   {
      for (int i = 28; i < 32; i++)
      {
         msgLocals.deviceDef.deviceDefs[i].name = fmtString("WPC Fake GameOn #%d", i + 1);
         msgLocals.deviceDef.deviceDefs[i].groupId = 0x0010;
         msgLocals.deviceDef.deviceDefs[i].deviceId = i + 1;
         msgLocals.deviceMap[i].type = LPM_DM_CORE_SOL2;
         msgLocals.deviceMap[i].srcId = 1 << (i - 28 + 8);
      }
   }
   // 29..32, solenoid outputs from driver board
   // Note: core_getSol only implement for S11 while core_getAllSol implements for all system (but is it used by other systems ?)
   else // if (core_gameData->gen & GEN_ALLS11)
   {
      for (int i = 28; i < 32; i++)
      {
         msgLocals.deviceDef.deviceDefs[i].name = fmtString("Sol #%02d", i + 1);
         msgLocals.deviceDef.deviceDefs[i].groupId = 0x0001;
         msgLocals.deviceDef.deviceDefs[i].deviceId = i + 1;
         msgLocals.deviceMap[i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL1;
         msgLocals.deviceMap[i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + i) : (1 << i);
      }
   }
   // 33, SAM: fake GameOn solenoid for fast flip
   // Note: core_getSol returns it replicated 4 times for 33..36 while core_getAllSol only returns it as 33 (34..36 are unused)
   if (core_gameData->gen & GEN_SAM)
   {
      msgLocals.deviceDef.deviceDefs[32].name = fmtString("SAM Fake GameOn");
      msgLocals.deviceDef.deviceDefs[32].groupId = 0x0010;
      msgLocals.deviceDef.deviceDefs[32].deviceId = 0;
      msgLocals.deviceMap[32].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL2;
      msgLocals.deviceMap[32].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + 32) : 0x00000010;
   }
   // 33..36: Whitestar various extension boards (stored in 0x00F0 of solenoids2, which is upper flipper for other hardwares)
   // Note: core_getSol does not implement this while core_getAllSol does
   else if (core_gameData->gen & GEN_ALLWS)
   {
      for (int i = 32; i < 36; i++)
      {
         msgLocals.deviceDef.deviceDefs[i].name = fmtString("Ext Sol #%02d", i - 32 + 1);
         msgLocals.deviceDef.deviceDefs[i].groupId = 0x0002;
         msgLocals.deviceDef.deviceDefs[i].deviceId = i - 32 + 1;
         msgLocals.deviceMap[i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL2;
         msgLocals.deviceMap[i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + i) : (1 << (i - 32 + 4));
      }
   }
   // 33..36, WPC fliptronic board: upper flipper solenoids that may also be used as generic modulated outputs
   // Note: core_getSol returns each coil state while core_getAllSol will set hold coil if either of Hold/Power is set
   else if (core_gameData->gen & (GEN_WPCFLIPTRON | GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS))
   {
      // TODO Ensure earlier generation do not have ext board in this area (they do not have upper flipper)
      // GEN_WPCALPHA_1: dd / fh
      // GEN_WPCALPHA_2: fh / bop / hd
      // GEN_WPCDMD: t2 / gi / Slugfest
      for (int i = 0; i < 4; i++)
      {
         int isFlipperSol = (i < 2) ? (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR))
                                    : (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL));
         if (isFlipperSol)
         {
            // Note: WPC fliptronics and later implement modulated outputs on these (not really modulated though)
            const int isCPU = (i < 2) ? (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR))
                                      : (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL));
            msgLocals.deviceDef.deviceDefs[32 + i].name = fmtString("Upper %s Flipper: %s solenoid %s",
               (i < 2) ? "Right" : "Left",
               (i & 1) ? "Hold|Power" : "Power",
               isCPU ? "(CPU controlled)" : "(emulated wired)");
            msgLocals.deviceDef.deviceDefs[32 + i].groupId = 0x0002;
            msgLocals.deviceDef.deviceDefs[32 + i].deviceId = 33 + i;
            msgLocals.deviceMap[32 + i].type = LPM_DM_CORE_SOL2;
            msgLocals.deviceMap[32 + i].srcId = isCPU ? (((i & 1) ? 0x30 : 0x10) << (i & 2))  // Power bit or Power|Hold bits
                                                      : ((                 0x10) << (i    )); // Not CPU controlled, just return emulated state
         }
         else
         {
            msgLocals.deviceDef.deviceDefs[32 + i].name = fmtString("Sol #%02d (%s)", 29 + i, (core_gameData->gen & (GEN_WPC95 | GEN_WPC95DCS)) ? "WPC95" : "Fliptronic");
            msgLocals.deviceDef.deviceDefs[32 + i].groupId = 0x0002;
            msgLocals.deviceDef.deviceDefs[32 + i].deviceId = 33 + i;
            msgLocals.deviceMap[32 + i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL2;
            msgLocals.deviceMap[32 + i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + 32 + i) : (1 << (i + 4));
         }
      }
   }
   // 37..44, WPC95: 4 low power digital outputs (duplicated 37..40 / 41..44, stored in 0xF0000000 of solenoids)
   if (core_gameData->gen & (GEN_WPC95 | GEN_WPC95DCS))
   {
      for (int i = 0; i < 8; i++)
      {
         msgLocals.deviceDef.deviceDefs[36 + i].name = fmtString("WPC95 LPDC #%02d", 37 + (i & 3));
         msgLocals.deviceDef.deviceDefs[36 + i].groupId = 0x0001;
         msgLocals.deviceDef.deviceDefs[36 + i].deviceId = 37 + (i & 3);
         msgLocals.deviceMap[36 + i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL1;
         msgLocals.deviceMap[36 + i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + 36 + (i & 3)) : (1 << (36 + (i & 3)));
      }
   }
   // 37..44, S11, SAM, SPA: extension board with 8 outputs (stored in 0xFF00 of solenoids2)
   else if (core_gameData->gen & (GEN_ALLS11 | GEN_SAM | GEN_SPA))
   {
      for (int i = 0; i < 8; i++)
      {
         if (core_gameData->gen & GEN_ALLS11)
            msgLocals.deviceDef.deviceDefs[36 + i].name = fmtString("S11 Ext Sol #%d", i + 1);
         else if (core_gameData->gen & GEN_SAM)
            msgLocals.deviceDef.deviceDefs[36 + i].name = fmtString("SAM Ext Sol #%d", i + 1);
         else
            msgLocals.deviceDef.deviceDefs[36 + i].name = fmtString("SPA Ext Sol #%d", i + 1);
         msgLocals.deviceDef.deviceDefs[36 + i].groupId = 0x0002;
         msgLocals.deviceDef.deviceDefs[36 + i].deviceId = i + 1;
         msgLocals.deviceMap[36 + i].type = isPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_SOL2;
         msgLocals.deviceMap[36 + i].srcId = isPhysSol ? (CORE_MODOUT_SOL0 + 40 + i) : (1 << (i + 8));
      }
   }
   // 45..48, lower flipper solenoids
   // Note: core_getSol returns each coil state while core_getAllSol will set hold coil if either of Hold/Power coil is set
   // Note: WPC fliptronics and later implement modulated outputs on these (not really modulated though)
   for (int i = 0; i < 4; i++)
   {
      const int isCPU = (i < 2) ? (core_gameData->hw.flippers & FLIP_SOL(FLIP_LR))
                                : (core_gameData->hw.flippers & FLIP_SOL(FLIP_LL));
      msgLocals.deviceDef.deviceDefs[44 + i].name = fmtString("Lower %s Flipper: %s solenoid %s",
         (i < 2) ? "Right" : "Left",
         (i & 1) ? "Hold|Power" : "Power",
         isCPU ? "(CPU controlled)" : "(Emulated wired)");
      msgLocals.deviceDef.deviceDefs[44 + i].groupId = 0x0002;
      msgLocals.deviceDef.deviceDefs[44 + i].deviceId = (core_gameData->gen & GEN_ALLWPC) ? (29 + i) : (45 + i);
      msgLocals.deviceMap[44 + i].type = LPM_DM_CORE_SOL2;
      msgLocals.deviceMap[44 + i].srcId = isCPU ? (((i & 1) ? 0x03 : 0x01) << (i & 2))  // Power bit or Power|Hold bits
                                                : ((                 0x01) << (i    )); // Not CPU controlled, just return emulated state
   }
   // 49, simulated fake plunger, not broadcasted
   // 50, unused, reserved
   // 51..66, custom through core_gameData->hw.getSol or physic model
   for (int i = CORE_FIRSTCUSTSOL - 1; i < nSols; i++)
   {
      const int isCusPhysSol = isPhysSol && (i < coreGlobals.nSolenoids);
      if ((isCusPhysSol == 0) && (core_gameData->hw.getSol == NULL))
         continue;
      if (isCusPhysSol && (core_gameData->gen & GEN_ALLWPC))
      {
         int id = (core_gameData->gen & (GEN_WPC95 | GEN_WPC95DCS)) ? (42 + i - (CORE_FIRSTCUSTSOL - 1))  // WPC95 extension board maps to Sol 42..49 (1..28 base sols, 29..36 flippers, 37..40 LPDC, 41 unknown)
                                                                    : (37 + i - (CORE_FIRSTCUSTSOL - 1)); // WPC extension board maps to Sol 37..44 (1..28 base sols, 29..36 fliptronics)
         msgLocals.deviceDef.deviceDefs[i].name = fmtString("Ext Sol #%02d", id);
         msgLocals.deviceDef.deviceDefs[i].groupId = 0x0002;
         msgLocals.deviceDef.deviceDefs[i].deviceId = id;
      }
      else
      {
         msgLocals.deviceDef.deviceDefs[i].name = fmtString("Custom Sol #%02d", i);
         msgLocals.deviceDef.deviceDefs[i].groupId = 0x0004;
         msgLocals.deviceDef.deviceDefs[i].deviceId = i - (CORE_FIRSTCUSTSOL - 1) + 1;
      }
      msgLocals.deviceMap[i].type = isCusPhysSol ? LPM_DM_PHYSOUT : LPM_DM_CORE_CUST_SOL;
      msgLocals.deviceMap[i].srcId = isCusPhysSol ? (CORE_MODOUT_SOL0 + i) : (i + 1);
   }
   // GI dedicated drivers (WPC, Whitestar, SAM)
   const int isPhysGI = (coreGlobals.nGI > 0) && ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_GI) != 0);
   for (int i = 0; i < coreGlobals.nGI; i++)
   {
      msgLocals.deviceDef.deviceDefs[nSols + i].name = fmtString("GI #%d", i + 1);
      msgLocals.deviceDef.deviceDefs[nSols + i].groupId = 0x0100;
      msgLocals.deviceDef.deviceDefs[nSols + i].deviceId = i + 1;
      msgLocals.deviceMap[nSols + i].type = isPhysGI ? LPM_DM_PHYSOUT : LPM_DM_CORE_GI;
      msgLocals.deviceMap[nSols + i].srcId = isPhysGI ? (CORE_MODOUT_GI0 + i) : i;
   }   
   // Lamp matrix
   const int isPhysLamp = (coreGlobals.nLamps > 0) && ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_LAMPS) != 0);
   for (int i = 0; i < nLamps; i++)
   {
      int l = coreData->m2lamp ? coreData->m2lamp((i / 8) + 1, i & 7) : i;
      msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + i].name = fmtString("Lamp #%x%x", (i / 8) + 1, (i & 7) + 1);
      msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + i].groupId = 0x0200;
      msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + i].deviceId = l;
      msgLocals.deviceMap[nSols + coreGlobals.nGI + i].type = isPhysLamp ? LPM_DM_PHYSOUT : LPM_DM_CORE_LAMP;
      msgLocals.deviceMap[nSols + coreGlobals.nGI + i].srcId = isPhysLamp ? (CORE_MODOUT_LAMP0 + i) : i;
   }   
   // Emulated mechanical devices (we don't know which ones are available so always declare all of them)
   // This is somewhat hacky as the definition depends on g_fHandleMechanics and we do not update if it changes (it must be defined before)
   for (int i = 0; i < MECH_MAXMECH; i++)
   {
      msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].groupId = 0x0300;
      if (g_fHandleMechanics == 0)
      {
         if (i < MECH_MAXMECH/2)
         {
            msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].name = fmtString("User Mech Pos #%02d", i);
            msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].deviceId = i + 1;
            msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].type = LPM_DM_CUSTOM_MECH_POS;
            msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].srcId = MECH_MAXMECH/2 + i;
         }
         else
         {
            int j = i - MECH_MAXMECH/2;
            msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].name = fmtString("User Mech Speed #%02d", j);
            msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].deviceId = -(j + 1);
            msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].type = LPM_DM_CUSTOM_MECH_SPEED;
            msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].srcId = MECH_MAXMECH/2 + j;
         }
      }
      else
      {
         msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].name = fmtString("PinMame Mech #%02d", i);
         msgLocals.deviceDef.deviceDefs[nSols + coreGlobals.nGI + nLamps + i].deviceId = i;
         msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].type = LPM_DM_CORE_MECH;
         msgLocals.deviceMap[nSols + coreGlobals.nGI + nLamps + i].srcId = i;
      }
   }
   if (msgLocals.deviceDef.nDevices > 0)
   {
      msgLocals.onDevSrcChangedId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_DEVICE_ON_SRC_CHG_MSG);
      msgLocals.getDevSrcId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_DEVICE_GET_SRC_MSG);
      msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.getDevSrcId, OnGetDevSrc, NULL);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDevSrcChangedId, NULL);
   }
   
   // -- Inputs: cabinet, matrix & dip switches
   const int nSwitches = (CORE_STDSWCOLS + core_gameData->hw.swCol) * 8;
   const int nDips = coreData->coreDips;
   msgLocals.inputDef.id.endpointId = msgLocals.endpointId;
   msgLocals.inputDef.id.resId = 0;
   msgLocals.inputDef.GetInputState = &GetInputState;
   msgLocals.inputDef.SetInputState = &SetInputState;
   msgLocals.inputDef.nInputs = nSwitches + nDips;
   msgLocals.inputDef.inputDefs = (DeviceDef *)malloc(msgLocals.inputDef.nInputs * sizeof(DeviceDef));
   memset(msgLocals.inputDef.inputDefs, 0, msgLocals.inputDef.nInputs * sizeof(DeviceDef));
   for (int i = 0; i < nSwitches; i++)
   {
      int l = coreData->m2sw ? coreData->m2sw((i / 8) + 1, i & 7) : i;
      msgLocals.inputDef.inputDefs[i].name = fmtString("Switch #%02x", l);
      msgLocals.inputDef.inputDefs[i].groupId = 0x0001;
      msgLocals.inputDef.inputDefs[i].deviceId = l;
   }
   for (int i = 0; i < nDips; i++)
   {
      msgLocals.inputDef.inputDefs[nSwitches + i].name = fmtString("DIP #%02d", i + 1);
      msgLocals.inputDef.inputDefs[nSwitches + i].groupId = 0x0002;
      msgLocals.inputDef.inputDefs[nSwitches + i].deviceId = i + 1;
   }
   if (msgLocals.inputDef.nInputs > 0)
   {
      msgLocals.onInputSrcChangedId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_INPUT_ON_SRC_CHG_MSG);
      msgLocals.getInputSrcId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_INPUT_GET_SRC_MSG);
      msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.getInputSrcId, OnGetInputSrc, NULL);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onInputSrcChangedId, NULL);
   }
}

static void ReleaseMsgApi()
{
   if ((msgLocals.msgApi == NULL) || (msgLocals.registered == 0))
      return;

   msgLocals.msgApi->ReleaseMsgID(msgLocals.onGameStartId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onGameEndId);
   
   if (msgLocals.nDisplays > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getDisplaySrcId, OnGetDisplaySrc);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDisplaySrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onDisplaySrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getDisplaySrcId);
      msgLocals.nDisplays = 0;
   }
   
   if (msgLocals.nSegDisplays > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getSegSrcId, OnGetSegSrc);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onSegSrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onSegSrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getSegSrcId);
      msgLocals.nSegDisplays = 0;
   }
   
   if (msgLocals.deviceDef.nDevices > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getDevSrcId, OnGetDevSrc);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDevSrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onDevSrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getDevSrcId);
      for (int i = 0; i < msgLocals.deviceDef.nDevices; i++)
         if (msgLocals.deviceDef.deviceDefs[i].name)
            free(msgLocals.deviceDef.deviceDefs[i].name);
      msgLocals.deviceDef.nDevices = 0;
      free(msgLocals.deviceDef.deviceDefs);
      free(msgLocals.deviceMap);
   }
   
   if (msgLocals.inputDef.nInputs > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getInputSrcId, OnGetInputSrc);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onInputSrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onInputSrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getInputSrcId);
      for (int i = 0; i < msgLocals.inputDef.nInputs; i++)
         if (msgLocals.inputDef.inputDefs[i].name)
            free(msgLocals.inputDef.inputDefs[i].name);
      msgLocals.inputDef.nInputs = 0;
      free(msgLocals.inputDef.inputDefs);
   }

   // Clear everything but the Msg API setup
   MsgPluginAPI* msgApi = msgLocals.msgApi;
   unsigned int endpointId = msgLocals.endpointId;
   memset(&msgLocals, 0, sizeof(msgLocals));
   msgLocals.msgApi = msgApi;
   msgLocals.endpointId = endpointId;
}

static void OnGameStart(void*)
{
   if (msgLocals.msgApi == NULL)
      return;
   SetupMsgApi();
   CtlOnGameStartMsg msg = { Machine->gamedrv->name };
   msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onGameStartId, reinterpret_cast<void*>(&msg));
}

static void OnGameEnd(void*)
{
   if (msgLocals.msgApi == NULL)
      return;
   msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onGameEndId, nullptr);
   ReleaseMsgApi();
}

PINMAMEAPI void PinmameSetMsgAPI(MsgPluginAPI* msgApi, unsigned int endpointId)
{
   ReleaseMsgApi();
   msgLocals.msgApi = msgApi;
   msgLocals.endpointId = endpointId;
   SetupMsgApi();
}
