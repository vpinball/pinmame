// license:BSD-3-Clause

#ifndef LIBPINMAME_H
#define LIBPINMAME_H

#if defined(_WIN32) || defined(_WIN64)
#define LIBPINMAME_API extern "C" __declspec(dllexport) 
#define CALLBACK __stdcall
#define strcasecmp _stricmp
#else
#define LIBPINMAME_API extern "C" //__declspec(dllimport) 
#define CALLBACK
#endif

typedef enum { 
	OK = 0,
	GAME_NOT_FOUND = 1,
	GAME_ALREADY_RUNNING = 2, 
	EMULATOR_NOT_RUNNING = 3
} PINMAME_STATUS;

typedef enum {
	SEG16 = 0,    // 16 segments
	SEG16R = 1,   // 16 segments with comma and period reversed
	SEG10 = 2,    // 9  segments and comma
	SEG9 = 3,     // 9  segments
	SEG8 = 4,     // 7  segments and comma
	SEG8D = 5,    // 7  segments and period
	SEG7 = 6,     // 7  segments
	SEG87 = 7,    // 7  segments, comma every three
	SEG87F = 8,   // 7  segments, forced comma every three
	SEG98 = 9,    // 9  segments, comma every three
	SEG98F = 10,  // 9  segments, forced comma every three
	SEG7S = 11,   // 7  segments, small
	SEG7SC = 12,  // 7  segments, small, with comma
	SEG16S = 13,  // 16 segments with split top and bottom line
	DMD = 14,     // DMD Display
	VIDEO = 15,   // VIDEO Display
	SEG16N = 16,  // 16 segments without commas
	SEG16D = 17   // 16 segments with periods only
} PINMAME_DISPLAY_TYPE;

typedef struct {
	const char* name;
	const char* clone_of;
	const char* description;
	const char* year;
	const char* manufacturer;
} PinmameGame;

typedef struct {
	PINMAME_DISPLAY_TYPE type;
	int top;
	int left;
	int length;
	int width;
	int height;
} PinmameDisplayLayout;

typedef void (CALLBACK *PinmameGameCallback)(PinmameGame* p_game);
typedef void (CALLBACK *PinmameOnStateChangeCallback)(int state);
typedef void (CALLBACK *PinmameOnSolenoidCallback)(int solenoid, int isActive);
typedef void (CALLBACK *PinmameDisplayLayoutCallback)(int index, PinmameDisplayLayout* p_displayLayout);
typedef void (CALLBACK *PinmameDisplayCallback)(int index, PinmameDisplayLayout* p_displayLayout);

typedef struct {
	int sampleRate;
	const char* p_vpmPath;
	PinmameOnStateChangeCallback cb_OnStateChange;
	PinmameOnSolenoidCallback cb_OnSolenoid;
} PinmameConfig;

LIBPINMAME_API void PinmameGetGames(PinmameGameCallback callback);
LIBPINMAME_API void PinmameSetConfig(PinmameConfig* p_config);
LIBPINMAME_API PINMAME_STATUS PinmameGetDisplayLayouts(PinmameDisplayLayoutCallback callback);
LIBPINMAME_API PINMAME_STATUS PinmameGetDisplays(void* p_buffer, PinmameDisplayCallback callback);
LIBPINMAME_API PINMAME_STATUS PinmameRun(const char* p_name);
LIBPINMAME_API int PinmameIsRunning();
LIBPINMAME_API PINMAME_STATUS PinmamePause(int pause);
LIBPINMAME_API PINMAME_STATUS PinmameReset();
LIBPINMAME_API void PinmameStop();
LIBPINMAME_API int PinmameGetSwitch(int slot);
LIBPINMAME_API void PinmameSetSwitch(int slot, int state);
LIBPINMAME_API void PinmameSetSwitches(int* p_states, int numSwitches);
LIBPINMAME_API int PinmameGetMaxLamps();
LIBPINMAME_API int PinmameGetChangedLamps(int* p_changedStates);
LIBPINMAME_API int PinmameGetMaxSolenoids();
LIBPINMAME_API int PinmameGetChangedSolenoids(int* p_changedStates);
LIBPINMAME_API int PinmameGetMaxGIs();
LIBPINMAME_API int PinmameGetChangedGIs(int* p_changedStates);
LIBPINMAME_API int PinmameGetAudioChannels();
LIBPINMAME_API int PinmameGetPendingAudioSamples(float* p_buffer, int outChannels, int maxNumber);
LIBPINMAME_API int PinmameGetPendingAudioSamples16bit(signed short* p_buffer, int outChannels, int maxNumber);

#endif
