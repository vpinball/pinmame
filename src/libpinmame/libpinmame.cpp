// license:BSD-3-Clause

#include "libpinmame.h"

#include "../../ext/libsamplerate/samplerate.h"

#include <thread>
#include <vector>
#include <algorithm>
#include <format>

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
#include "memory.h"

extern int throttle;
extern int autoframeskip;
extern int allow_sleep;

int g_fHandleKeyboard = 0;
int g_fHandleMechanics = 0xFF;
int g_fDumpFrames = 0;
int g_fPause = 0;
PINMAME_DMD_MODE g_fDmdMode = PINMAME_DMD_MODE_BRIGHTNESS;

char g_szGameName[256] = {}; // String containing requested game name (may be different from ROM if aliased), set by PinmameRun
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
	{ "MENU", PINMAME_KEYCODE_MENU, KEYCODE_MENU },
	{ nullptr, (PINMAME_KEYCODE)0, 0 }
};

// Controller plugin message support

#include "plugins/ControllerPlugin.h"
#include "PinMAMEPlugin.h"

#include "../ext/nlohmann/json.hpp"
using json = nlohmann::json;

typedef enum StateMappingType {
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
   LPM_DM_CORE_SWITCH,
   LPM_DM_CORE_DIPSWITCH,
   LPM_DM_MEMMAP
} StateMappingType;

typedef struct StateMapping
{
   int srcId;
} StateMapping;

static struct
{
   MsgPluginAPI* msgApi;
   unsigned int endpointId;
   bool registered;

   unsigned int onControllerChangeId;
   unsigned int onGetControllersId;

   struct StateGroup
   {
      StateSrcId stateDef;
      std::vector<StateMapping> stateMap;
      std::vector<StateDef> states;
   };
   std::array<StateGroup, PMPI_GROUP_VPM_MECH> stateGroups;
   std::unique_ptr<PinballPlugin::Controller::CtrlItemProvider<StateSrcId>> stateProvider;

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
   } displays[32]; // WPT declares 15 DMD layouts
   unsigned int onDisplaySrcChangedId, getDisplaySrcId;

   unsigned int onAudioCmdId, onDmdCmdId, onConsoleDataId;

   unsigned int onGetMachineStateId;

   struct MemMapState
   {
      std::string group;
      std::string name;
      unsigned int type;
      std::function<void(unsigned int index, void* pResult)> getState;
   };
   char memMapStringBuffer[256];
   std::vector<MemMapState> memMapStates;
} msgLocals = {};


/******************************************************
 * ComposePath
 ******************************************************/

static char* ComposePath(const char* const path, const char* const file)
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

static const char* CheckGameAlias(const char* const romName)
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

	if (file != nullptr) {
		char line[128];
		while (fgets(line, sizeof(line), file)) {
			// Skip lines that start with "#"
			if (line[0] == '#')
				continue;

			char* token = strtok(line, ", ");

			if (!strcasecmp(token, romName))
			{
				strcpy(_aliasFromFile,  strtok(nullptr, " ,\n#;'"));
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

static int GetGameNumFromString(const char* const name)
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

/* Expand a 5 or 6 bit colour channel to 8 bits, replicating the high bits into the low ones so that the channel maximum maps to 255 */
static inline UINT8 ExpandChannel5To8(const UINT32 v) { return (UINT8)((v << 3) | (v >> 2)); }
static inline UINT8 ExpandChannel6To8(const UINT32 v) { return (UINT8)((v << 2) | (v >> 4)); }

/******************************************************
 * UpdatePinmameDisplayBitmap
 ******************************************************/

static bool UpdatePinmameDisplayBitmap(PinmameDisplay* pDisplay, struct mame_bitmap* p_bitmap)
{
	UINT8* __restrict const dst = (UINT8*)pDisplay->pData;
	bool diff = false;
	const int height = pDisplay->layout.height; // layout->start
	const int width = pDisplay->layout.width; // layout->length
	const int rotation = (pDisplay->layout.type & PINMAME_DISPLAY_TYPE_VIDEO_ROT90) ? 1 : 0;
	const int incr = (rotation == 0) ? 3 : ((rotation == 1) ? height*3 : -height*3);
	/* A VIDEO_RGB_DIRECT screen holds packed colour, not palette indices - reading it through
	   palette_get_color() gives nonsense, and for a 15 bpp bitmap the index runs up to 0x7fff,
	   far past the palette. Machines with a true-colour screen (Pinball 2000's 640x240 among
	   them) therefore need the value unpacked instead of looked up. */
	const int direct = (Machine->drv->video_attributes & VIDEO_RGB_DIRECT) != 0;
	const int depth = p_bitmap->depth;

	/* Fast path for the larger highcolor screen of Pinball 2000 (640x480). Rather
	   than fetch every pixel through the bitmap's read() function pointer and re-test the format per pixel,
	   walk the rows directly - rp_16() is just ((UINT16*)line[y])[x], orientation being baked into the line
	   pointers. Worth about 25% at 640x480 */
	if (direct && (depth == 15) && (rotation == 0)) {
		unsigned int changed = 0; /* Accumulated bitwise so that the comparison stays branch free */
		for (int y = 0; y < height; y++) {
			const UINT16* __restrict const src = (const UINT16*)p_bitmap->line[y];
			UINT8* __restrict const row = dst + (size_t)y * width * 3;
			for (int x = 0; x < width; x++) {
				const UINT32 c = src[x];
				const UINT8 b = ExpandChannel5To8((c >> 10) & 0x1f);
				const UINT8 g = ExpandChannel5To8((c >>  5) & 0x1f);
				const UINT8 r = ExpandChannel5To8( c        & 0x1f);
				const size_t o = (size_t)x * 3;
				changed |= (unsigned int)((row[o] ^ r) | (row[o + 1] ^ g) | (row[o + 2] ^ b));
				row[o    ] = r;
				row[o + 1] = g;
				row[o + 2] = b;
			}
		}
		return changed != 0;
	}

	for (int y = 0; y < height; y++) {
		int pos = (rotation == 0) ? y*width : ((rotation == 1) ? (height - 1 - y) : (y + (width - 1)*height));
		pos *= 3;
		for (int x = 0; x < width; x++,pos+=incr) {
			UINT8 r,g,b;
			if (direct) {
				const UINT32 c = p_bitmap->read(p_bitmap, x, y);
				if (depth == 32) { b = (c >> 16) & 0xff; g = (c >> 8) & 0xff; r = c & 0xff; } //!! b & r positions correct?
				else if (depth == 16) { b = ExpandChannel5To8((c >> 11) & 0x1f); g = ExpandChannel6To8((c >> 5) & 0x3f); r = ExpandChannel5To8(c & 0x1f); } // RGB565
				else                  { b = ExpandChannel5To8((c >> 10) & 0x1f); g = ExpandChannel5To8((c >> 5) & 0x1f); r = ExpandChannel5To8(c & 0x1f); } // RGB555
			}
			else
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
	if(!_p_Config->cb_OnAudioUpdated)
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
	int index = layout == nullptr ? ((int)_displays.size() - 1) : layout->index;
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
			if (memcmp(pDisplay->pData, p_data, pDisplay->size)) {
				memcpy(pDisplay->pData, p_data, pDisplay->size);
				pDisplay->frameId++;
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

static void OnSoundCommand(void* userData)
{
   PinMAMEChildBoardEventMsg* msg = static_cast<PinMAMEChildBoardEventMsg*>(userData);
	if (msgLocals.msgApi != nullptr && msgLocals.registered)
		msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onAudioCmdId, msg);
	delete msg;
}

extern "C" void libpinmame_snd_cmd_log(int boardNo, int cmd)
{
	if (_p_Config->cb_OnSoundCommand)
		(*(_p_Config->cb_OnSoundCommand))(boardNo, cmd, _p_userData);

	if (msgLocals.msgApi != nullptr && msgLocals.registered)
	{
      PinMAMEChildBoardEventMsg* msg = new PinMAMEChildBoardEventMsg();
		msg->boardNo = static_cast<unsigned int>(boardNo);
		msg->cmd = static_cast<unsigned int>(cmd);
		msgLocals.msgApi->RunOnMainThread(msgLocals.endpointId, 0, OnSoundCommand, msg);
	}
}

/******************************************************
 * libpinmame_forward_console_data
 ******************************************************/

static void OnConsoleDataCommand(void* userData)
{
   PinMAMEConsoleDataMsg* msg = static_cast<PinMAMEConsoleDataMsg*>(userData);
   if (msgLocals.msgApi != nullptr && msgLocals.registered)
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onConsoleDataId, msg);
   
   delete[] msg->data;
   delete msg;
}

extern "C" void libpinmame_forward_console_data(void* p_data, int size)
{
	if (_p_Config->cb_OnConsoleDataUpdated)
   	(*(_p_Config->cb_OnConsoleDataUpdated))(p_data, size, _p_userData);

   if (p_data != nullptr && size > 0 && msgLocals.msgApi != nullptr && msgLocals.registered)
   {
      auto* pending = new PinMAMEConsoleDataMsg();
      pending->size = static_cast<uint32_t>(size);
      auto* data = new uint8_t[pending->size];
      memcpy(data, p_data, pending->size);
      pending->data = data;
      msgLocals.msgApi->RunOnMainThread(msgLocals.endpointId, 0, OnConsoleDataCommand, pending);
   }
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
      case 2: // Starting, just wait to be started, nothing to do
         break;
      case 1: // Running
         msgLocals.msgApi->RunOnMainThread(msgLocals.endpointId, 0, OnGameStart, nullptr);
         break;
      case 3: // Stopping, it is invalid to call PinMAME emulation state
      {
         // It would be cleaner to unregister everything here but we would have a race condition if unregistering here as it must be done
         // on Plugin API thread, while we are on emulation thread, blocking the Plugin API thread until stop is done. So we protect thread safe Getter/Setter against this state
         // msgLocals.msgApi->RunOnMainThread(msgLocals.endpointId, -1, [](void* userData) { msgLocals.stateProvider->ClearItems(); }, nullptr);
         // We also request a lock to block until any ongoing parallel state processing is ended before we invalidate internal PinMAME state
         std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
         break;
      }
      case 0: // Stopped
         msgLocals.msgApi->RunOnMainThread(msgLocals.endpointId, 0, OnGameEnd, nullptr);
         break;
      }
   }
	if (_p_Config->cb_OnStateUpdated)
		(*(_p_Config->cb_OnStateUpdated))(state, _p_userData);

	if (state == 1)
	{
		int displayCount = 0;
		bool hasDMDOrVideo = false;
		for (const struct core_dispLayout * layout = core_gameData->lcdLayout, * parent_layout = nullptr; layout->length || (parent_layout && parent_layout->length); layout += 1) {
			if (layout->length == 0) { // Recursive import
				layout = parent_layout;
				parent_layout = nullptr;
			}
			if (layout->type == CORE_IMPORT) {
				assert(parent_layout == nullptr); // IMPORT of IMPORT is not currently supported as it is not used by any driver so far
				parent_layout = layout + 1;
				layout = layout->importedLayout - 1;
				continue;
			}
			hasDMDOrVideo |= (layout->type == CORE_VIDEO) || ((layout->type & CORE_DMD) == CORE_DMD);
			if (displayCount <= layout->index)
				displayCount = layout->index + 1;
		}
		// Reserve one extra slot unconditionally.
		// Using hasDMDOrVideo as the gate is wrong here: segment-only games also create an extra
		// synthetic 128x32 DMD below, so they need one additional display slot as well.
		displayCount++;
		_displays.resize(displayCount);

		for (const struct core_dispLayout* layout = core_gameData->lcdLayout, * parent_layout = nullptr; layout->length || (parent_layout && parent_layout->length); layout += 1) {
			if (layout->length == 0) { // Recursive import
				layout = parent_layout;
				parent_layout = nullptr;
			}
			if ((layout->type & CORE_SEGMASK) == CORE_IMPORT) {
				assert(parent_layout == nullptr); // IMPORT of IMPORT is not currently supported as it is not used by any driver so far
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

static int StartGame(const int gameNum)
{
	memset(_mechInit, 0, sizeof(_mechInit));
	memset(_mechInfo, 0, sizeof(_mechInfo));

	const int err = run_game(gameNum);

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

	const int gameNum = GetGameNumFromString(p_name);

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
	// Enable the AT91 JIT (SAM games via sam.c, DE/Whitestar AT91 sound boards via desound.c;
	// 1 = default address range). Only effective in builds that compile a JIT in
	options.at91jit = 1;

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

	strncpy(g_szGameName, p_name, sizeof(g_szGameName) - 1);
	g_szGameName[sizeof(g_szGameName) - 1] = '\0';

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
	if (!_p_gameThread) {
		if (_isRunning) {
			libpinmame_log_error("PinmameStop(): run state is %d but game thread handle is null; forcing stopped state.", _isRunning);
			OnStateChange(0);
		}
		return;
	}

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

	if (_isRunning) {
		libpinmame_log_error("PinmameStop(): game thread joined but run state is %d; forcing stopped state.", _isRunning);
		OnStateChange(0);
	}
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
 * PinmameReadMainCPUByte
 ******************************************************/

PINMAMEAPI int PinmameReadMainCPUByte(const uint32_t address, uint8_t* const p_value)
{
	if (!_isRunning || p_value == nullptr)
	{
		return 0;
	}

	uint8_t* p_memory = static_cast<uint8_t*>(memory_get_read_ptr(0, address));
	if (p_memory == nullptr)
	{
		return 0;
	}

	*p_value = *p_memory;
	return 1;
}

/******************************************************
 * PinmameGetRawMemoryRegion
 ******************************************************/

PINMAMEAPI const uint8_t* PinmameGetRawMemoryRegion(const int region)
{
	return _isRunning ? memory_region(region) : nullptr;
}

/******************************************************
 * PinmameGetRawMemoryRegionLength
 ******************************************************/

PINMAMEAPI size_t PinmameGetRawMemoryRegionLength(const int region)
{
	return _isRunning ? memory_region_length(region) : 0;
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

static void OnGetMachineState(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   auto msg = static_cast<PinMAMEMachineStateMsg*>(msgData);
   msg->game = g_szGameName;
   msg->rom = Machine->gamedrv->name;
   msg->hardwareGen = core_gameData->gen;
}

static void OnGetControllers(const unsigned int eventId, void* userData, void* msgData)
{
   if (_isRunning != 1)
      return;

   auto msg = static_cast<GetControllersMsg*>(msgData);
   if (msg->count < msg->maxEntryCount)
   {
      static std::string gameId;
      // Broadcast the game name that was requested (which may be an alias registered through alias.txt)
      // so consumers see the same id the table script used, not the resolved driver name
      gameId = std::format("pinmame::{}", g_szGameName[0] ? g_szGameName : Machine->gamedrv->name);
      msg->entries[msg->count].endpointId = msgLocals.endpointId;
      msg->entries[msg->count].gameId = gameId.c_str();
   }
   msg->count++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device states

INLINE uint8_t saturatedByte(float v) { return static_cast<uint8_t>(255.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v)); }

static void GetDefaultValue(int type, void* pResult)
{
   switch (type)
   {
   case CTLPI_STATE_FORMAT_UINT8:*static_cast<uint8_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_UINT16:*static_cast<uint16_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_UINT32:*static_cast<uint32_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_UINT64:*static_cast<uint64_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_INT8:*static_cast<int8_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_INT16:*static_cast<int16_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_INT32:*static_cast<int32_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_INT64:*static_cast<int64_t*>(pResult) = 0; break;
   case CTLPI_STATE_FORMAT_FLOAT:*static_cast<float*>(pResult) = 0.f; break;
   case CTLPI_STATE_FORMAT_DOUBLE:*static_cast<double*>(pResult) = 0.0; break;
   case CTLPI_STATE_FORMAT_STRING: *static_cast<const char**>(pResult) = ""; break;
   }
}
static void GetSolenoid1State(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   if (options.usemodsol & CORE_MODOUT_FORCE_ON)
      core_update_pwm_outputs(CORE_MODOUT_SOL0 + core_BitColToNum(srcId), 1);
   *static_cast<float*>(pResult) = (coreGlobals.solenoids & srcId) != 0 ? 1.f : 0.f;
}
static void GetSolenoid1VPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<uint8_t*>(pResult) = (coreGlobals.solenoids & srcId) != 0 ? 1 : 0;
}
static void GetSolenoid2State(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   if (options.usemodsol & CORE_MODOUT_FORCE_ON)
      core_update_pwm_outputs(CORE_MODOUT_SOL0 + core_BitColToNum(srcId) + 32, 1);
   *static_cast<float*>(pResult) = (coreGlobals.solenoids2 & srcId) != 0 ? 1.f : 0.f;
}
static void GetSolenoid2VPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<uint8_t*>(pResult) = (coreGlobals.solenoids2 & srcId) != 0 ? 1 : 0;
}
static void GetCustomSolenoidState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   // TODO core_gameData->hw.getSol is supposed to return 0..255, but this would need to be checked on each driver
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<float*>(pResult) = static_cast<float>(core_gameData->hw.getSol(srcId)) / 255.f;
}
static void GetCustomSolenoidVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<uint8_t*>(pResult) = static_cast<uint8_t>(core_gameData->hw.getSol(srcId));
}
static void GetLampState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   if (options.usemodsol & CORE_MODOUT_FORCE_ON)
      core_update_pwm_outputs(CORE_MODOUT_LAMP0 + srcId, 1);
   *static_cast<float*>(pResult) = ((coreGlobals.lampMatrix[srcId / 8] >> (srcId % 8)) & 0x01) != 0 ? 1.f : 0.f;
}
static void GetLampVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   if (options.usemodsol & CORE_MODOUT_FORCE_ON)
      core_update_pwm_outputs(CORE_MODOUT_LAMP0 + srcId, 1);
   *static_cast<uint8_t*>(pResult) = ((coreGlobals.lampMatrix[srcId / 8] >> (srcId % 8)) & 0x01) != 0 ? 1 : 0;
}
static void GetGIState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   if (core_gameData->gen & GEN_ALLWPC) // WPC GI level is 0..8
      *static_cast<float*>(pResult) = static_cast<float>(coreGlobals.gi[srcId]) / 8.f;
   else // Whitestar and SAM GI levels are either 0 or 9
      *static_cast<float*>(pResult) = coreGlobals.gi[srcId] != 0 ? 1.f : 0.f;
}
static void GetGIVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<uint8_t*>(pResult) = static_cast<uint8_t>(coreGlobals.gi[srcId]);
}
static void GetPhysOutState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   core_update_pwm_outputs(srcId, 1);
   *static_cast<float*>(pResult) = coreGlobals.physicOutputState[srcId].value;
}
static void GetPhysOutVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   core_update_pwm_outputs(srcId, 1);
   *static_cast<uint8_t*>(pResult) = saturatedByte(coreGlobals.physicOutputState[srcId].value);
}
static void GetSwitchState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<uint8_t*>(pResult) = core_getSw(static_cast<int16_t>(srcId)) != 0 ? 0xFF : 0;
}
static void SetSwitchState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   if (_isRunning != 1) return;
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   core_setSw(static_cast<int16_t>(srcId), *static_cast<uint8_t*>(pResult) != 0);
}
static void GetDIPSwitchState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   const int bank = srcId / 8;
   const int mask = 1 << (srcId & 7);
   *static_cast<uint8_t*>(pResult) = (vp_getDIP(bank) & mask) != 0 ? 0xFF : 0;
}
static void SetDIPSwitchState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   if (_isRunning != 1) return;
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   const int bank = srcId / 8;
   const int mask = 1 << (srcId & 7);
   if (*static_cast<uint8_t*>(pResult) != 0)
      vp_setDIP(bank, vp_getDIP(bank) | mask);
   else
      vp_setDIP(bank, vp_getDIP(bank) & ~mask);
}
static void GetMemMapState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   msgLocals.memMapStates[srcId].getState(srcId, pResult);
}
static void GetCoreMechState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<int32_t*>(pResult) = core_gameData->hw.getMech ? core_gameData->hw.getMech(srcId) : 0;
}
static void GetCustomMechPosState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<float*>(pResult) = mech_getFloatPos(srcId);
}
static void GetCustomMechSpeedState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<float*>(pResult) = mech_getFloatSpeed(srcId);
}
static void GetCustomMechPosVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<int32_t*>(pResult) = mech_getPos(srcId);
}
static void GetCustomMechSpeedVPMState(CtlResId blockId, unsigned int stateIndex, void* pResult)
{
   std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
   if (_isRunning != 1) { GetDefaultValue(msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateDef.stateDefs[stateIndex].dataFormat, pResult); return; }
   const int srcId = msgLocals.stateGroups[blockId.resId - PMPI_GROUP_SOLENOID].stateMap[stateIndex].srcId;
   *static_cast<int32_t*>(pResult) = mech_getSpeed(srcId);
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

static const char* fmtString(const char *format, ...) {
    va_list args;

    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args) + 1; // +1 for the null terminator
    va_end(args);

    va_start(args, format);
    char *formatted_string = new char[size];
    vsnprintf(formatted_string, size, format, args);
    va_end(args);

    return formatted_string;
}

static void SetupMsgApi()
{
   assert(msgLocals.msgApi != nullptr);

   // For the time being, the API only covers a running controller (the setup/info part is not yet exposed), so we do not have anything to register if we are not running
   if (_isRunning != 1)
      return;

   assert(!msgLocals.registered);
   msgLocals.registered = true;

   msgLocals.onControllerChangeId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_CONTROLLERS_ON_CHG_MSG);
   msgLocals.onGetControllersId = msgLocals.msgApi->GetMsgID(CTLPI_NAMESPACE, CTLPI_CONTROLLERS_GET_MSG);
   msgLocals.onAudioCmdId = msgLocals.msgApi->GetMsgID(PMPI_NAMESPACE, PMPI_EVT_ON_AUDIO_CMD);
   msgLocals.onDmdCmdId = msgLocals.msgApi->GetMsgID(PMPI_NAMESPACE, PMPI_EVT_ON_DISPLAY_CMD);
   msgLocals.onConsoleDataId = msgLocals.msgApi->GetMsgID(PMPI_NAMESPACE, PMPI_EVT_ON_CONSOLE_DATA);
   msgLocals.onGetMachineStateId = msgLocals.msgApi->GetMsgID(PMPI_NAMESPACE, PMPI_GET_MACHINE_STATE);
   msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.onGetControllersId, OnGetControllers, nullptr);
   msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.onGetMachineStateId, OnGetMachineState, nullptr);

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
         if (msgLocals.nDisplays >= (int)(sizeof(msgLocals.displays) / sizeof(msgLocals.displays[0])))
         {
            libpinmame_log_error("SetupMsgApi: too many display layouts, ignoring extra display");
            break;
         }
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
            msgLocals.displays[msgLocals.nDisplays].srcId.hardware = CTLPI_DISPLAY_HARDWARE_CRT_DISPLAY;
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
         msgLocals.displays[msgLocals.nDisplays].srcId.frameFormat = ((layout->type & CORE_SEGMASK) == CORE_VIDEO) ? CTLPI_DISPLAY_FORMAT_SRGB888 : CTLPI_DISPLAY_FORMAT_LUM32F;
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
      msgLocals.msgApi->SubscribeMsg(msgLocals.endpointId, msgLocals.getDisplaySrcId, OnGetDisplaySrc, nullptr);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDisplaySrcChangedId, nullptr);
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
                                                                         : msgLocals.segDisplays[msgLocals.nSegDisplays].srcId.nElements == 2 ? CTLPI_SEG_HARDWARE_GTS1_4DIGIT // Ball & Credit, reported as 2x2 (while hardware is 1x4 with a space in between)
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

   // -- Hardware states (controlled devices, switches,...)
   // The existing output layout is the result of years of evolution, starting with the WPC hardware (hence
   // the dedicated GI outputs with 0..8 values), then adding different levels of device emulation (merging
   // binary output states over a given period, some smoothing, physic model,...) and solving conflicts
   // by moving the outputs to free slots. This 'legacy' mapping is preserved for backward compatibility:
   // - state groups are defined for new (normalized physic states) and legacy (VPinMAME) outputs
   // - mappingId correspond to 'legacy' mapping
   //
   // Existing solenoid access is implemented in core_getSol, core_getAllSol and core_getAllPhysicSols. Sadly,
   // these functions do not always return the same value. When difference exists, the implementation of 
   // core_getAllSol is taken as it is supposed to be the most widely used.
   //
   for (uint32_t i = PMPI_GROUP_SOLENOID; i <= PMPI_GROUP_VPM_MECH; i++)
   {
      auto& group = msgLocals.stateGroups[i - PMPI_GROUP_SOLENOID];
      group.stateMap.clear();
      group.states.clear();
   }
   auto setupGroup = [](uint32_t groupId, const char* name, const char* desc)
      {
         msgLocals.stateGroups[groupId - PMPI_GROUP_SOLENOID].stateDef = {
            .id = { msgLocals.endpointId, groupId },
            .name = name,
            .desc = desc,
            .nStates = static_cast<unsigned int>(msgLocals.stateGroups[groupId - PMPI_GROUP_SOLENOID].states.size()),
            .stateDefs = msgLocals.stateGroups[groupId - PMPI_GROUP_SOLENOID].states.data()
         };
      };
   auto addDevice = [](int groupId, const char* label, const char* desc, uint16_t mappingId, int format, int type, void(MSGPIAPI* get)(CtlResId, unsigned int, void*), void(MSGPIAPI* set)(CtlResId, unsigned int, void*), int mappingSrc)
      {
         msgLocals.stateGroups[groupId - PMPI_GROUP_SOLENOID].states.emplace_back(label, desc, mappingId, format, type, get, set);
         msgLocals.stateGroups[groupId - PMPI_GROUP_SOLENOID].stateMap.emplace_back(mappingSrc);
      };
   // 'Solenoid' outputs (in fact all sort of controlled or emulated outputs with a messy mapping)
   {
      const int nSols = coreGlobals.nSolenoids ? coreGlobals.nSolenoids : (CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol);
      const bool isPhysSol = (coreGlobals.nSolenoids > 0) && ((options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)) != 0);
      auto addPhysSol = [&isPhysSol, &addDevice](const char* label, const char* desc, const char* descVPM, uint16_t mappingId, void(MSGPIAPI* get)(CtlResId, unsigned int, void*), void(MSGPIAPI* getVPM)(CtlResId, unsigned int, void*), int mappingSrc, int physSolIndex)
         {
            if (isPhysSol && physSolIndex < coreGlobals.nSolenoids)
            {
               addDevice(PMPI_GROUP_SOLENOID, label, desc, mappingId, CTLPI_STATE_FORMAT_FLOAT, core_get_pwm_output_type(CORE_MODOUT_SOL0 + physSolIndex) == 1 ? CTLPI_STATE_TYPE_RELATIVE_BRIGHTNESS : CTLPI_STATE_TYPE_CUSTOM, GetPhysOutState, nullptr, CORE_MODOUT_SOL0 + physSolIndex);
               addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), descVPM, mappingId, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, GetPhysOutVPMState, nullptr, CORE_MODOUT_SOL0 + physSolIndex);
            }
            else
            {
               addDevice(PMPI_GROUP_SOLENOID, label, desc, mappingId, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_CUSTOM, get, nullptr, mappingSrc);
               addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), descVPM, mappingId, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, getVPM, nullptr, mappingSrc);
            }
         };
      // 1..28, solenoid/flasher outputs from driver board
      for (uint16_t i = 1; i <= 28; i++)
         addPhysSol(fmtString("Output #%02d", i), nullptr, nullptr, i, GetSolenoid1State, GetSolenoid1VPMState, 1 << (i - 1), i - 1);
      // 29..32
      {
         // 29..31, WPC 29 & 30 are J111 GPIO, 31 is a fake GameOn solenoids for fast flip (not modulated, stored in 0x0F00 of solenoids2)
         if (core_gameData->gen & GEN_ALLWPC)
         {
            addPhysSol(fmtString("GPIO #1 (WPC J111.1)"), nullptr, nullptr, 29, GetSolenoid2State, GetSolenoid2VPMState, 0x0100, 28);
            addPhysSol(fmtString("GPIO #2 (WPC J111.2)"), nullptr, nullptr, 30, GetSolenoid2State, GetSolenoid2VPMState,  0x0200, 29);
            if (core_gameData->gen & (GEN_WPCALPHA_1 | GEN_WPCALPHA_2 | GEN_WPCDMD)) // Pre Fliptronic real GameOn
               addPhysSol(fmtString("WPC GameOn"), nullptr, nullptr, 31, GetSolenoid2State, GetSolenoid2VPMState,  0x0400, 30);
            else // Fliptronic ROM controlled flippers, with (sadly) an overlay of J111 third output and the fake GameOn (which is only available if fastflip is defined)
               addPhysSol(fmtString("GPIO #3 (WPC J111.3) overlayed with FastFlip Fake GameOn"), nullptr, nullptr, 31, GetSolenoid2State, GetSolenoid2VPMState,  0x0400, 30);
         }
         // 29..32, solenoid outputs from driver board
         // Note: core_getSol only implement for S11 while core_getAllSol implements for all system (but is it used by other systems ?)
         else // if (core_gameData->gen & GEN_ALLS11)
         {
            for (uint16_t i = 29; i <= 32; i++)
               addPhysSol(fmtString("Output #%02d", i), nullptr, nullptr, i, GetSolenoid1State, GetSolenoid1VPMState,  1 << (i - 1), i - 1);
         }
      }
      // 33..36
      {
         // 33, SAM: fake GameOn solenoid for fast flip
         // Note: core_getSol returns it replicated 4 times for 33..36 while core_getAllSol only returns it as 33 (34..36 are unused)
         if (core_gameData->gen & GEN_SAM)
            addPhysSol(fmtString("SAM Fake GameOn"), nullptr, nullptr, 33, GetSolenoid2State, GetSolenoid2VPMState,  0x00000010, 32);
         // 33..36: Whitestar various extension boards (stored in 0x00F0 of solenoids2, which is upper flipper for other hardwares)
         // Note: core_getSol does not implement this while core_getAllSol does
         else if (core_gameData->gen & GEN_ALLWS)
         {
            for (uint16_t i = 33; i <= 36; i++)
               addPhysSol(fmtString("Whitestar Ext Sol #%02d", i - 32), nullptr, nullptr, i, GetSolenoid2State, GetSolenoid2VPMState,  1 << (i - 33 + 4), i - 1);
         }
         // 33..36, WPC fliptronic board: upper flipper solenoids that may also be used as generic modulated outputs (Solenoids 29..32 in schematics)
         // Note: core_getSol returns each coil state while core_getAllSol will set hold coil if either of Hold/Power is set
         else if (core_gameData->gen & (GEN_WPCFLIPTRON | GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS))
         {
            // TODO Ensure earlier generation do not have ext board in this area (they do not have upper flipper)
            // GEN_WPCALPHA_1: dd / fh
            // GEN_WPCALPHA_2: fh / bop / hd
            // GEN_WPCDMD: t2 / gi / Slugfest
            for (uint16_t i = 33; i < 37; i++)
            {
               const bool isFlipperCoil = core_gameData->hw.flippers & FLIP_SOL((i < 35) ? FLIP_UR : FLIP_UL);
               const char* label;
               if (isFlipperCoil)
                  label = fmtString("Output #%02d - Upper %s Flipper %s solenoid (CPU controlled)", i, (i < 35) ? "Right" : "Left", (i & 1) ? "Power" : "Hold|Power");
               else if (core_gameData->gen & (GEN_WPC95 | GEN_WPC95DCS))
                  label = fmtString("Output #%02d (WPC95 J120)", i);
               else
                  label = fmtString("Output #%02d (Fliptronic)", i);
               if (coreGlobals.hasModulatedFlippers && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON)))
               {
                  int type = core_get_pwm_output_type(CORE_MODOUT_SOL0 + i - 1) == 1 ? CTLPI_STATE_TYPE_RELATIVE_BRIGHTNESS : CTLPI_STATE_TYPE_CUSTOM;
                  addDevice(PMPI_GROUP_SOLENOID, label, nullptr, i, CTLPI_STATE_FORMAT_FLOAT, type, GetPhysOutState, nullptr, CORE_MODOUT_SOL0 + i - 1);
                  addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), nullptr, i, CTLPI_STATE_FORMAT_UINT8, type, GetPhysOutVPMState, nullptr, CORE_MODOUT_SOL0 + i - 1);
               }
               else
               {
                  int mask;
                  switch (i)
                  {
                  case 33:mask = isFlipperCoil ? 0x10 : 0x10; break; // Power bit
                  case 34:mask = isFlipperCoil ? CORE_URFLIPSOLBITS : 0x20; break; // Power|Hold bits
                  case 35:mask = isFlipperCoil ? 0x40 : 0x40; break; // Power bit
                  case 36:mask = isFlipperCoil ? CORE_ULFLIPSOLBITS : 0x80; break; // Power|Hold bits
                  }
                  addDevice(PMPI_GROUP_SOLENOID, label, nullptr, i, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_CUSTOM, GetSolenoid2State, nullptr, mask);
                  addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), nullptr, i, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, GetSolenoid2VPMState, nullptr, mask);
               }
            }
         }
      }
      // 37..44
      {
         // 37..44, WPC95: 4 low power digital outputs (duplicated 37..40 / 41..44, stored in 0xF0000000 of solenoids)
         if (core_gameData->gen & (GEN_WPC95 | GEN_WPC95DCS))
         {
            for (uint16_t i = 0; i < 8; i++)
               addPhysSol(fmtString("Output #%02d (WPC95 J110 LPDC)", 37 + (i & 3)),
                  nullptr, nullptr, 37 + i, GetSolenoid1State, GetSolenoid1VPMState,  1 << (36 + (i & 3)), 36 + (i & 3));
         }
         // 37..44, S11, SAM, SPA: extension board with 8 outputs (stored in 0xFF00 of solenoids2)
         else if (core_gameData->gen & (GEN_ALLS11 | GEN_SAM | GEN_SPA))
         {
            for (uint16_t i = 37; i <= 44; i++)
               addPhysSol(
                  fmtString("%s Ext Output #%d", (core_gameData->gen & GEN_ALLS11) ? "S11" : (core_gameData->gen & GEN_SAM) ? "SAM" : "SPA", i - 36),
                  nullptr, nullptr, i, GetSolenoid2State, GetSolenoid2VPMState,  1 << (8 + i - 37), 40 + i - 37);
         }
      }
      // 45..48, lower flipper solenoids
      // Note: core_getSol returns each coil state while core_getAllSol will set hold coil if either of Hold/Power coil is set
      for (uint16_t i = 45; i < 49; i++)
      {
         const bool isCPU = core_gameData->hw.flippers & FLIP_SOL((i < 47) ? FLIP_LR : FLIP_LL);
         const char* leftRight = (i < 47) ? "Right" : "Left";
         const char* bits = (i & 1) ? "Power" : "Hold|Power";
         const char* label;
         if (isCPU && (core_gameData->gen & GEN_ALLWPC))
            label = fmtString("Output #%02d - Lower %s Flipper %s solenoid (CPU controlled)", 29 + (i - 45), leftRight, bits);
         else if (isCPU)
            label = fmtString("Lower %s Flipper: %s solenoid (CPU controlled)", leftRight, bits);
         else
            label = fmtString("Lower %s Flipper: %s solenoid (emulated wired)", leftRight, bits);
         if (coreGlobals.hasModulatedFlippers && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON)))
         {
            int type = core_get_pwm_output_type(CORE_MODOUT_SOL0 + i - 1) == 1 ? CTLPI_STATE_TYPE_RELATIVE_BRIGHTNESS : CTLPI_STATE_TYPE_CUSTOM;
            addDevice(PMPI_GROUP_SOLENOID, label, nullptr, i, CTLPI_STATE_FORMAT_FLOAT, type, GetPhysOutState, nullptr, CORE_MODOUT_SOL0 + i - 1);
            addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), nullptr, i, CTLPI_STATE_FORMAT_UINT8, type, GetPhysOutVPMState, nullptr, CORE_MODOUT_SOL0 + i - 1);
         }
         else
         {
            int mask;
            switch (i)
            {
            case 45:mask = 0x01; break; // Power bit
            case 46:mask = CORE_LRFLIPSOLBITS; break; // Power|Hold bits
            case 47:mask = 0x04; break; // Power bit
            case 48:mask = CORE_LLFLIPSOLBITS; break; // Power|Hold bits
            }
            addDevice(PMPI_GROUP_SOLENOID, label, nullptr, i, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_CUSTOM, GetSolenoid2State, nullptr, mask);
            addDevice(PMPI_GROUP_VPM_SOLENOID, fmtString("%s", label), nullptr, i, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, GetSolenoid2VPMState, nullptr, mask);
         }
      }
      // 49, simulated fake plunger, not broadcasted
      // 50, unused, reserved
      // 51..66, custom through core_gameData->hw.getSol or physic model
      // Note for WPC except WPC95, the first 8 custom solenoids are actually the extension boards (report in group 0x0002 with an adapyted label ?)
      for (uint16_t i = CORE_FIRSTCUSTSOL - 1; i < nSols; i++)
         addPhysSol(fmtString("Custom Output #%02d", i), nullptr, nullptr, i + 1, GetCustomSolenoidState, GetCustomSolenoidVPMState, i + 1, i);

      setupGroup(PMPI_GROUP_SOLENOID, "Solenoids", "Generic high/low current outputs (solenoids & flashers but also auxiliary boards, custom driver outputs and PinMAME internal state)");
      setupGroup(PMPI_GROUP_VPM_SOLENOID, "VPinMAME Solenoids", "Backward compatible VPinMAME states (less precise, meaning depends on game driver)");
   }
   // GI dedicated drivers (WPC 0..8, Whitestar 0/9, SAM 0/9)
   {
      const bool isPhysGI = (coreGlobals.nGI > 0) && ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_GI) != 0);
      for (uint16_t i = 0; i < coreGlobals.nGI; i++)
      {
         addDevice(PMPI_GROUP_GI, fmtString("GI #%d", i + 1), nullptr, i, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_RELATIVE_BRIGHTNESS, isPhysGI ? GetPhysOutState : GetGIState, nullptr, isPhysGI ? (CORE_MODOUT_GI0 + i) : i);
         addDevice(PMPI_GROUP_VPM_GI, fmtString("GI #%d", i + 1), fmtString((core_gameData->gen& GEN_ALLWPC) ? "Value in 0..8 range" : "0 (off) or 9 (on)"), i, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, isPhysGI ? GetPhysOutVPMState : GetGIVPMState, nullptr, isPhysGI ? (CORE_MODOUT_GI0 + i) : i);
      }
      setupGroup(PMPI_GROUP_GI, "GIs", "General Illumination strings (WPC, Whitestar & SAM are the only one with dedicated GI outputs, other hardwares use generic outputs to drive a GI relay/thyristor)");
      setupGroup(PMPI_GROUP_VPM_GI, "VPinMAME GIs", "Backward compatible VPinMAME states (less precise, meaning depends on game driver)");
   }
   // Lamp matrix
   {
      const int hasSAMModulatedLeds = (core_gameData->gen & GEN_SAM) && (core_gameData->hw.lampCol > 2);
      const int nLamps = (hasSAMModulatedLeds || coreGlobals.nLamps) ? coreGlobals.nLamps : (8 * (CORE_CUSTLAMPCOL + core_gameData->hw.lampCol));
      const bool isPhysLamp = (coreGlobals.nLamps > 0) && ((options.usemodsol & CORE_MODOUT_ENABLE_PHYSOUT_LAMPS) != 0);
      for (uint16_t i = 0; i < nLamps; i++)
      {
         uint16_t l = coreData->m2lamp ? static_cast<uint16_t>(coreData->m2lamp((i / 8) + 1, i & 7)) : i;
         addDevice(PMPI_GROUP_LAMP, fmtString("Lamp #%x%x", (i / 8) + 1, (i & 7) + 1), nullptr, l, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_RELATIVE_BRIGHTNESS, isPhysLamp ? GetPhysOutState : GetLampState, nullptr, isPhysLamp ? (CORE_MODOUT_LAMP0 + i) : i);
         addDevice(PMPI_GROUP_VPM_LAMP, fmtString("Lamp #%x%x", (i / 8) + 1, (i & 7) + 1), fmtString("FIXME define value"), l, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_CUSTOM, isPhysLamp ? GetPhysOutVPMState : GetLampVPMState, nullptr, isPhysLamp ? (CORE_MODOUT_LAMP0 + i) : i);
      }
      setupGroup(PMPI_GROUP_LAMP, "Lamps", "Playfield, cabinet and backglass lamps (either matrix or directly controlled)");
      setupGroup(PMPI_GROUP_VPM_LAMP, "VPinMAME Lamps", "Backward compatible VPinMAME states (less precise, meaning depends on game driver)");
   }
   // Emulated mechanical devices (we don't know which ones are available so always declare all of them)
   // This is somewhat hacky as the definition depends on g_fHandleMechanics and we do not update if it changes (it must be defined before)
   for (uint16_t i = 0; i < MECH_MAXMECH; i++)
   {
      if (g_fHandleMechanics == 0)
      {
         if (i < MECH_MAXMECH / 2)
         {
            addDevice(PMPI_GROUP_MECH, fmtString("User Mech Pos #%02d", i), nullptr, i + 1, CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_CUSTOM, GetCustomMechPosState, nullptr, MECH_MAXMECH / 2 + i);
            addDevice(PMPI_GROUP_VPM_MECH, fmtString("User Mech Pos #%02d", i), nullptr, i + 1, CTLPI_STATE_FORMAT_INT32, CTLPI_STATE_TYPE_CUSTOM, GetCustomMechPosVPMState, nullptr, MECH_MAXMECH / 2 + i);
         }
         else
         {
            uint16_t j = i - MECH_MAXMECH/2;
            addDevice(PMPI_GROUP_MECH, fmtString("User Mech Speed #%02d", j), nullptr, -(j + 1), CTLPI_STATE_FORMAT_FLOAT, CTLPI_STATE_TYPE_CUSTOM, GetCustomMechSpeedState, nullptr, MECH_MAXMECH / 2 + j);
            addDevice(PMPI_GROUP_VPM_MECH, fmtString("User Mech Speed #%02d", j), nullptr, -(j + 1), CTLPI_STATE_FORMAT_INT32, CTLPI_STATE_TYPE_CUSTOM, GetCustomMechSpeedVPMState, nullptr, MECH_MAXMECH / 2 + j);
         }
      }
      else
      {
         addDevice(PMPI_GROUP_MECH, fmtString("PinMame Mech #%02d", i), nullptr, i, CTLPI_STATE_FORMAT_INT32, CTLPI_STATE_TYPE_CUSTOM, GetCoreMechState, nullptr, i);
      }
   }
   setupGroup(PMPI_GROUP_MECH, "Mechs", "Emulated mechanical parts (driver or user defined)");
   setupGroup(PMPI_GROUP_VPM_MECH, "VPinMAME Mechs", "Backward compatible VPinMAME states (less precise data format)");
   // Playfield & cabinet switches
   for (uint16_t i = 0; i < (CORE_STDSWCOLS + core_gameData->hw.swCol) * 8; i++)
   {
      const int swNo = coreData->m2sw ? coreData->m2sw(i / 8, i & 7) : i; // Note that some hardware use negative switch indices to identify cabinet switches (for example Whitestar)
      assert(i == (coreData->sw2m ? coreData->sw2m(swNo) : ((swNo / 10) * 8 + (swNo % 10 - 1))));
      addDevice(PMPI_GROUP_SWITCH, swNo < 0 ? fmtString("Cabinet #%02x", 16 + swNo) : fmtString("Playfield #%02x", swNo), nullptr, static_cast<uint16_t>(swNo), CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_SWITCH, GetSwitchState, SetSwitchState, swNo);
   }
   setupGroup(PMPI_GROUP_SWITCH, "Switches", "Playfield & cabinet switches");
   // DIP switches
   for (uint16_t i = 0; i < coreData->coreDips; i++)
   {
      addDevice(PMPI_GROUP_DIPSWITCH, fmtString("DIP #%02d", i + 1), nullptr, i + 1, CTLPI_STATE_FORMAT_UINT8, CTLPI_STATE_TYPE_SWITCH, GetDIPSwitchState, SetDIPSwitchState, i + 1);
   }
   setupGroup(PMPI_GROUP_DIPSWITCH, "DIP Switches", "Hardware onboard DIP switches");
   // MemMap Game states
   for (uint16_t i = 0; i < msgLocals.memMapStates.size(); i++)
   {
      addDevice(PMPI_GROUP_GAMESTATE, fmtString("%s", msgLocals.memMapStates[i].name.c_str()), fmtString("%s", msgLocals.memMapStates[i].group.c_str()), i, msgLocals.memMapStates[i].type, CTLPI_STATE_TYPE_CUSTOM, GetMemMapState, nullptr, i);
   }
   setupGroup(PMPI_GROUP_GAMESTATE, "Game States", "Live game states gathered from internal machine memory");
   //
   msgLocals.stateProvider = std::make_unique<PinballPlugin::Controller::CtrlItemProvider<StateSrcId>>(msgLocals.msgApi, msgLocals.endpointId, CTLPI_STATE_GET_SRC_MSG, CTLPI_STATE_ON_SRC_CHG_MSG);
   std::vector<StateSrcId> stateGroups;
   for (uint32_t i = PMPI_GROUP_SOLENOID; i <= PMPI_GROUP_VPM_MECH; i++)
      stateGroups.push_back(msgLocals.stateGroups[i - PMPI_GROUP_SOLENOID].stateDef);
   msgLocals.stateProvider->AddItems(stateGroups);
}

static void ReleaseMsgApi()
{
   assert(msgLocals.msgApi != nullptr);

   // Only release if we actually registered (we had a running machine)
   if (!msgLocals.registered)
      return;
   msgLocals.registered = false;

   msgLocals.msgApi->UnsubscribeMsg(msgLocals.onGetMachineStateId, OnGetMachineState, nullptr);
   msgLocals.msgApi->UnsubscribeMsg(msgLocals.onGetControllersId, OnGetControllers, nullptr);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onControllerChangeId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onGetControllersId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onAudioCmdId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onDmdCmdId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onConsoleDataId);
   msgLocals.msgApi->ReleaseMsgID(msgLocals.onGetMachineStateId);

   if (msgLocals.nDisplays > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getDisplaySrcId, OnGetDisplaySrc, nullptr);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onDisplaySrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onDisplaySrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getDisplaySrcId);
      memset(msgLocals.displays, 0, sizeof(msgLocals.displays));
      msgLocals.nDisplays = 0;
   }
   
   if (msgLocals.nSegDisplays > 0)
   {
      msgLocals.msgApi->UnsubscribeMsg(msgLocals.getSegSrcId, OnGetSegSrc, nullptr);
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onSegSrcChangedId, nullptr);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.onSegSrcChangedId);
      msgLocals.msgApi->ReleaseMsgID(msgLocals.getSegSrcId);
      msgLocals.nSegDisplays = 0;
      memset(&msgLocals.segDisplays, 0, sizeof(msgLocals.segDisplays));
      msgLocals.nSortedSegLayout = 0;
      memset(&msgLocals.sortedSegLayout, 0, sizeof(msgLocals.sortedSegLayout));
      memset(&msgLocals.segLuminances, 0, sizeof(msgLocals.segLuminances));
      memset(&msgLocals.segPrevLuminances, 0, sizeof(msgLocals.segPrevLuminances));
   }

   if (msgLocals.stateProvider)
   {
      std::lock_guard lock(msgLocals.stateProvider->GetListMutex());
      for (const auto& group : msgLocals.stateProvider->GetItems())
      {
         for (int i = 0; i < group.nStates; i++)
         {
            if (const char* name = group.stateDefs[i].name; name)
            {
               group.stateDefs[i].name = nullptr;
               delete[] name;
            }
            if (const char* desc = group.stateDefs[i].desc; desc)
            {
               group.stateDefs[i].desc = desc;
               delete[] desc;
            }
         }
      }
   }
   msgLocals.stateProvider = nullptr;
}

static void OnGameStart(void*)
{
   SetupMsgApi();
   if (msgLocals.registered)
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onControllerChangeId, nullptr);
}

static void OnGameEnd(void*)
{
   if (msgLocals.registered)
      msgLocals.msgApi->BroadcastMsg(msgLocals.endpointId, msgLocals.onControllerChangeId, nullptr);
   ReleaseMsgApi();
}

PINMAMEAPI void PinmameSetMsgAPI(MsgPluginAPI* msgApi, unsigned int endpointId)
{
   if (msgLocals.msgApi)
      ReleaseMsgApi();
   msgLocals.msgApi = msgApi;
   msgLocals.endpointId = endpointId;
   if (msgLocals.msgApi)
      SetupMsgApi();
}

PINMAMEAPI void PinmameSetMemMap(uint8_t* platform, size_t platformSize, uint8_t* game, size_t gameSize)
{
   const auto parseAddress = [](const json& node, unsigned int& value) -> bool
      {
         if (node.is_number_unsigned())
         {
            value = node.get<unsigned int>();
            return true;
         }
         if (node.is_string())
         {
            const std::string& s = node.get_ref<const std::string&>();
            try
            {
               size_t pos = 0;
               // base 0 lets std::stoll auto-detect "0x"/"0X" (hex), leading "0" (octal), or decimal
               value = std::stoll(s, &pos, 0);
               if (pos != s.size())
                  return false; // trailing garbage after the number
               return true;
            }
            catch (const std::exception&)
            {
               return false;
            }
         }
         return false;
      };

   struct MemRegion { unsigned int start; unsigned int end; int nibble; };
   std::vector<MemRegion> memRegions;
   bool isLittleEndianPlatform = false;
   if (platform && platformSize)
   {
      std::string platformString(platform, platform + platformSize);
      json platformDef = json::parse(platformString);
      if (platformDef.is_object() && platformDef.contains("endian") && platformDef["endian"].is_string() && platformDef["endian"].get<std::string>() == "little")
         isLittleEndianPlatform = true;
      if (platformDef.is_object() && platformDef.contains("memory_layout") && platformDef["memory_layout"].is_array())
      {
         for (const auto& regionDef : platformDef["memory_layout"]) {
            int nibble = -1;
            if (!regionDef.contains("nibble") || !regionDef["nibble"].is_string())
               continue;
            if (regionDef["nibble"].get<std::string>() == "low")
               nibble = 1;
            else if (regionDef["nibble"].get<std::string>() == "hight")
               nibble = 2;
            else
               continue;
            unsigned int address;
            if (!regionDef.contains("address") || !parseAddress(regionDef["address"], address))
               continue;
            unsigned int size;
            if (!regionDef.contains("size") || !parseAddress(regionDef["size"], size))
               continue;
            memRegions.emplace_back(address, address + size - 1, nibble);
         }
      }
   }

   std::string gameString(game, game + gameSize);
   json memMapDef = json::parse(gameString);

   std::function<void(const json&, const std::string&, std::map<std::string, json>)> traverse_and_collect;
   traverse_and_collect = [&traverse_and_collect, &parseAddress, &memRegions, isLittleEndianPlatform](const json& node, const std::string& group, const std::map<std::string, json>& inherited_fields) {
      if (!node.is_object())
         return;

      std::map<std::string, json> fields = inherited_fields;
      if (node.contains("offsets") || node.contains("start"))
      {
         // Game state node
         for (const auto& [key, value] : node.items())
            fields[key] = value;
         std::string stateGroup;
         std::string desc;
         if (fields.contains("short_label") && fields["short_label"].is_string())
         {
            desc = fields["short_label"].get<std::string>();
            stateGroup = group + '\\' + desc;
            if (fields.contains("label") && fields["label"].is_string())
               desc = fields["label"].get<std::string>();
         }
         else if (fields.contains("label") && fields["label"].is_string())
         {
            desc = fields["label"].get<std::string>();
            stateGroup = group + '\\' + desc;
         }
         else
         {
            stateGroup = group;
         }

         if (!fields.contains("encoding") || !fields["encoding"].is_string())
            return;
         const std::string encoding = fields["encoding"].get<std::string>();
         if (encoding == "dipsw")
            return; // Drop as DIP switches are already directly exposed

         // Data block is defined by either:
         // - start and end (byte at end is included)
         // - start and end = start + length - 1
         // - offsets (if there is a single one, then it is to be considered as 'start' like above)
         // 
         // Then bytes are processed:
         // - reversed if self.little_endian() and encoding in ['bcd', 'int', 'bits', 'bool']
         //   . it may be defined in the metadata with 'big_endian' => unused in latest maps
         //   . it may be defined at the entry level with 'endian' = 'little' or 'big'
         // - compacted by nibbles if it is not set to 'both' nibbles (4 high or 4 low bits)
         //   . if entry has 'packed=false', replace file's default with 'nibble=low' => unused in latest maps
         //   . if entry defines its own 'nibble', use it
         //   . if not, search in the platform memory_layout, the nibble that applies to the first address
         //   . if not defined, defaults to both nibbles
         // - masked if 'mask' is set
         //
         // For encoding, see https://github.com/tomlogic/py-pinmame-nvmaps/blob/00ea7847545c9edd84613db99c7dd54976ddf568/nvram_parser.py#L738
         std::vector<unsigned int> offsets;
         if (fields.contains("offsets") && fields["offsets"].is_array())
         {
            for (const auto& subNode : fields["offsets"]) {
               unsigned int offset;
               if (!parseAddress(subNode, offset))
                  return;
               offsets.push_back(offset);
            }
            if (unsigned int length; offsets.size() == 1 && fields.contains("length") && parseAddress(fields["length"], length))
            {
               for (unsigned int i = 1; i < length; i++)
                  offsets.push_back(offsets[0] + i);
            }
         }
         else if (unsigned int start; fields.contains("start") && parseAddress(fields["start"], start))
         {
            if (unsigned int end; fields.contains("end") && parseAddress(fields["end"], end))
            {
               if (end < start)
                  return;
               for (unsigned int i = start; i <= end; i++)
                  offsets.push_back(i);
            }
            else if (unsigned int length; fields.contains("length") && parseAddress(fields["length"], length))
            {
               for (unsigned int i = 0; i < length; i++)
                  offsets.push_back(start + i);
            }
            else
            {
               offsets.push_back(start);
            }
         }
         else
         {
            return;
         }
         if (offsets.empty())
            return;

         unsigned int type = 0;
         std::function<void(unsigned int index, void* pResult)> getter;
         if (encoding == "int" || encoding == "bcd" || encoding == "bits" || encoding == "bool" || encoding == "enum")
         {
            const bool isBCD = encoding == "bcd";
            const bool isBool = encoding == "bool";
            unsigned int byteMask = 0xFF;
            if (fields.contains("mask"))
               parseAddress(fields["mask"], byteMask);
            int nibble = 0;
            for (const auto& region : memRegions)
            {
               if (region.start <= offsets[0] && offsets[0] <= region.end)
               {
                  nibble = region.nibble;
                  break;
               }
            }
            if (fields.contains("packed") && fields["packed"].is_string() && fields["packed"].get<std::string>() == "false")
               nibble = 1;
            else if (fields.contains("nibble") && fields["nibble"].is_string())
            {
               const std::string nibbleLiteral = fields["nibble"].get<std::string>();
               if (nibbleLiteral == "both")
                  nibble = 0;
               else if (nibbleLiteral == "low")
                  nibble = 1;
               else if (nibbleLiteral == "high")
                  nibble = 2;
            }
            const bool isReversed = fields.contains("invert") && fields["invert"].is_string() && fields["invert"].get<std::string>() == "true";
            bool isLittleEndian = isLittleEndianPlatform;
            if (fields.contains("endian") && fields["endian"].is_string())
            {
               const std::string endian = fields["endian"].get<std::string>();
               if (endian == "little")
                  isLittleEndian = true;
               else if (endian == "big")
                  isLittleEndian = false;
            }
            const double valueScale = fields.contains("scale") && fields["scale"].is_number() ? fields["scale"].get<double>() : 1.0;
            const double valueOffset = fields.contains("offset") && fields["offset"].is_number() ? fields["offset"].get<double>() : 0.0;
            type = CTLPI_STATE_FORMAT_INT64;
            getter = [offsets, nibble, byteMask, isBCD, isBool, isReversed, isLittleEndian, valueScale, valueOffset](unsigned int index, void* pResult)
               {
                  int64_t v = 0;

                  // Decode memory (as we are running asynchronously so we can't use cpunum_read_byte which would switch CPU context)
                  // FIXME We take for granted that we will read in a continuous block of RAM (dangerous)
                  // FIXME we consider that the map always apply to CPU #0 which should be true, but there may be exceptions to this
                  const unsigned int baseOffset = isLittleEndian ? offsets.back() : offsets.front();
                  const uint8_t* ptr = static_cast<const uint8_t*>(memory_find_base(0, baseOffset));
                  if (ptr == nullptr)
                     return;
                  if (isBCD)
                  {
                     if (isLittleEndian)
                     {
                        for (auto it = offsets.rbegin(); it != offsets.rend(); ++it)
                        {
                           const uint8_t num = (*(ptr + *it - baseOffset)) & byteMask;
                           const uint8_t dig1 = (num & 0x0F);
                           const uint8_t dig2 = (num >> 4);
                           if (nibble == 0)
                              v = (v * 100) + (dig2 > 9 ? 0 : dig2 * 10) + (dig1 > 9 ? 0 : dig1);
                           else if (nibble == 1)
                              v = (v * 10) + (dig1 > 9 ? 0 : dig1);
                           else if (nibble == 2)
                              v = (v * 10) + (dig2 > 9 ? 0 : dig1);
                        }
                     }
                     else
                     {
                        for (auto offset : offsets)
                        {
                           const uint8_t num = (*(ptr + offset - baseOffset)) & byteMask;
                           const uint8_t dig1 = (num & 0x0F);
                           const uint8_t dig2 = (num >> 4);
                           if (nibble == 0)
                              v = (v * 100) + (dig2 > 9 ? 0 : dig2 * 10) + (dig1 > 9 ? 0 : dig1);
                           else if (nibble == 1)
                              v = (v * 10) + (dig1 > 9 ? 0 : dig1);
                           else if (nibble == 2)
                              v = (v * 10) + (dig2 > 9 ? 0 : dig1);
                        }
                     }
                  }
                  else
                  {
                     if (isLittleEndian)
                     {
                        for (auto it = offsets.rbegin(); it != offsets.rend(); ++it)
                        {
                           const uint8_t num = (*(ptr + *it - baseOffset)) & byteMask;
                           if (nibble == 0)
                              v = (v << 8) | num;
                           else if (nibble == 1)
                              v = (v << 4) | (num & 0x0F);
                           else if (nibble == 2)
                              v = (v << 4) | (num >> 4);
                        }
                     }
                     else
                     {
                        for (auto offset : offsets)
                        {
                           const uint8_t num = (*(ptr + offset - baseOffset)) & byteMask;
                           if (nibble == 0)
                              v = (v << 8) | num;
                           else if (nibble == 1)
                              v = (v << 4) | (num & 0x0F);
                           else if (nibble == 2)
                              v = (v << 4) | (num >> 4);
                        }
                     }
                  }

                  if (valueOffset != 0.0 || valueScale != 1.0)
                     v = static_cast<int64_t>(valueOffset + valueScale * static_cast<double>(v));

                  if (isBool)
                  {
                     if (isReversed)
                        v = v == 0 ? 1 : 0;
                     else
                        v = v == 0 ? 0 : 1;
                  }

                  *static_cast<int64_t*>(pResult) = v;
               };
         }
         else if (encoding == "ch")
         {
            type = CTLPI_STATE_FORMAT_STRING;
            getter = [offsets](unsigned int index, void* pResult)
               {
                  const unsigned int baseOffset = offsets[0];
                  const uint8_t* ptr = static_cast<const uint8_t*>(memory_find_base(0, baseOffset));
                  if (ptr == nullptr)
                     return;

                  assert(offsets.size() < 255);
                  char* pStr = msgLocals.memMapStringBuffer;
                  for (auto offset : offsets)
                  {
                     *pStr = *(ptr + (offset - baseOffset));
                     pStr++;
                  }
                  *pStr = 0;
                  *static_cast<const char**>(pResult) = msgLocals.memMapStringBuffer;
               };
         }
         else
            return;

         msgLocals.memMapStates.emplace_back(stateGroup, desc, type, getter);
      }
      else
      {
         // Other nodes: goes down the group hierarchy
         for (const auto& [key, value] : node.items())
            if (!value.is_object() && !value.is_array())
               fields[key] = value;

         std::string subGroup = group.empty() ? "" : (group + '\\');
         for (const auto& [key, value] : node.items()) {
            if (value.is_object()) {
               traverse_and_collect(value, subGroup + key, fields);
            }
            else if (value.is_array()) {
               for (const auto& subNode : value) {
                  traverse_and_collect(subNode, subGroup + key, fields);
               }
            }
         }
      }
      };

   msgLocals.memMapStates.clear();
   traverse_and_collect(memMapDef, "", {});
   std::stable_sort(msgLocals.memMapStates.begin(), msgLocals.memMapStates.end(), [](const auto& a, const auto& b) {
      if (int s = a.group.compare(b.group); s != 0)
         return s < 0;
      if (int s = a.name.compare(b.name); s != 0)
         return s < 0;
      return false;
      });
}
