#include "PinMAMEdll.h"

//============================================================
// Console Debugging Section (Optional)
//============================================================

#if defined(_WIN32)
#define ENABLE_CONSOLE_DEBUG
#ifdef ENABLE_CONSOLE_DEBUG

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>

bool g_useConsole = false;

void ShowConsole()
{
	if (g_useConsole)
		return;
	g_useConsole = true;
	FILE* pConsole;
	AllocConsole();
	freopen_s(&pConsole, "CONOUT$", "wb", stdout);
}

void CloseConsole()
{
	if (g_useConsole)
		FreeConsole();
	g_useConsole = false;
}
#endif
#else
#define MAX_PATH          1024
#endif

#include <thread>
#include <../win32com/Alias.h> //!! move that one to some platform independent section

extern "C"
{
	// MAME headers
	#include "driver.h"
	#include "window.h"
	#include "input.h"
	#include "minimalconfig.h"
	#include "rc.h"
	#include "core.h"
	#include "vpintf.h"
	#include "mame.h"
	#include "dllsound.h"
	#include "cpuexec.h"

	extern unsigned char g_raw_dmdbuffer[DMD_MAXY*DMD_MAXX];
	extern unsigned int g_raw_colordmdbuffer[DMD_MAXY*DMD_MAXX];
	extern unsigned int g_raw_dmdx;
	extern unsigned int g_raw_dmdy;
	extern unsigned int g_needs_DMD_update;

	int g_fHandleKeyboard = 0, g_fHandleMechanics = 0;
	extern int trying_to_quit;
	void OnSolenoid(int nSolenoid, int IsActive);
	void OnStateChange(int nChange);
	extern void win_timer_enable(int enabled);

	UINT8 win_trying_to_quit;
	volatile int g_fPause = 0;
	volatile int g_fDumpFrames = 0;
	char g_fShowWinDMD = 0;
	char g_fShowPinDMD = 0; /* pinDMD */

	char g_szGameName[256] = { 0 }; // String containing requested game name (may be different from ROM if aliased)

	extern int channels;

	struct rc_struct *rc;
}

char g_vpmPath[MAX_PATH] = { 0 };
int g_sampleRate = 48000;
bool g_isGameReady = false;
static std::thread* pRunningGame = nullptr;

//============================================================
// Callback tests Section
//============================================================

void OnSolenoid(int nSolenoid, int IsActive)
{
	printf("Solenoid: %d %s\n", nSolenoid, (IsActive > 0 ? "ON" : "OFF"));
}

void OnStateChange(int nChange)
{
	printf("OnStateChange : %d\n", nChange);
	g_isGameReady = nChange > 0;
}


//============================================================
//	Game related Section
//============================================================

int GetGameNumFromString(char *name)
{
	int gamenum = 0;
	while (drivers[gamenum]) {
		if (!_stricmp(drivers[gamenum]->name, name))
			break;
		gamenum++;
	}
	if (!drivers[gamenum])
		return -1;
	else
		return gamenum;
}

char* composePath(const char* path, const char* file)
{
	size_t pathl = strlen(path);
	size_t filel = strlen(file);
	char *r = (char*)malloc(pathl + filel + 4);

	strcpy(r, path);
	strcpy(r+pathl, file);
	return r;
}

int set_option(const char *name, const char *arg, int priority)
{
	return rc_set_option(rc, name, arg, priority);
}


//============================================================
//	Running game thread
//============================================================
void gameThread(int game_index=-1)
{
	if (game_index == -1)
		return;
	/*int res =*/ run_game(game_index);
}


//============================================================
//	Dll Exported Functions Section
//============================================================

// Setup Functions
// ---------------

PINMAMEDLL_API void SetVPMPath(char* path)
{
	strcpy_s(g_vpmPath, path);
}

PINMAMEDLL_API void SetSampleRate(int sampleRate)
{
	g_sampleRate = sampleRate;
}


// Game related functions
// ---------------------

PINMAMEDLL_API int StartThreadedGame(char* gameNameOrg, bool showConsole)
{
	if (pRunningGame)
		return -1;

#ifdef ENABLE_CONSOLE_DEBUG
	if (showConsole)
		ShowConsole();
#endif
	trying_to_quit = 0;

	rc = cli_rc_create();

	strcpy_s(g_szGameName, gameNameOrg);
	const char* const gameName = checkGameAlias(g_szGameName);

	const int game_index = GetGameNumFromString(const_cast<char*>(gameName));
	if (game_index < 0)
		return -1;

	//options.skip_disclaimer = 1;
	//options.skip_gameinfo = 1;
	options.samplerate = g_sampleRate;

	win_timer_enable(1);
	g_fPause = 0;

	set_option("throttle", "1", 0);
	set_option("sleep", "1", 0);
	set_option("autoframeskip", "0", 0);
	set_option("skip_gameinfo", "1", 0);
	set_option("skip_disclaimer", "1", 0);

	printf("VPM path: %s\n", g_vpmPath);
	setPath(FILETYPE_ROM, composePath(g_vpmPath, "roms"));
	setPath(FILETYPE_NVRAM, composePath(g_vpmPath, "nvram"));
	setPath(FILETYPE_SAMPLE, composePath(g_vpmPath, "samples"));
	setPath(FILETYPE_CONFIG, composePath(g_vpmPath, "cfg"));
	setPath(FILETYPE_HIGHSCORE, composePath(g_vpmPath, "hi"));
	setPath(FILETYPE_INPUTLOG, composePath(g_vpmPath, "inp"));
	setPath(FILETYPE_MEMCARD, composePath(g_vpmPath, "memcard"));
	setPath(FILETYPE_STATE, composePath(g_vpmPath, "sta"));

	printf("GameIndex: %d\n", game_index);
	pRunningGame = new std::thread(gameThread, game_index);

	return game_index;
}

void StopThreadedGame(bool locking)
{
	if (pRunningGame == nullptr)
		return;

	trying_to_quit = 1;
	OnStateChange(1);

	if (locking)
	{
		printf("Waiting for clean exit...\n");
		pRunningGame->join();
	}

	delete(pRunningGame);
	pRunningGame = nullptr;

	//rc_unregister(rc, opts);
	rc_destroy(rc);

	g_szGameName[0] = '\0';

#ifdef ENABLE_CONSOLE_DEBUG
	CloseConsole();
#endif
}

PINMAMEDLL_API bool IsGameReady()
{
	return g_isGameReady;
}

// Pause related functions
// -----------------------
PINMAMEDLL_API void ResetGame()
{
	machine_reset();
}

PINMAMEDLL_API void Pause()
{
	g_fPause = 1;
}

PINMAMEDLL_API void Continue()
{
	g_fPause = 0;
}

PINMAMEDLL_API bool IsPaused()
{
	return g_fPause > 0;
}


// DMD related functions
// ---------------------

PINMAMEDLL_API bool NeedsDMDUpdate()
{
	return !!g_needs_DMD_update;
}

PINMAMEDLL_API int GetRawDMDWidth()
{
	return g_raw_dmdx;
}

PINMAMEDLL_API int GetRawDMDHeight()
{
	return g_raw_dmdy;
}

PINMAMEDLL_API int GetRawDMDPixels(unsigned char* buffer)
{
	if (g_raw_dmdx == ~0u || g_raw_dmdy == ~0u)
		return -1;
	memcpy(buffer, g_raw_dmdbuffer, g_raw_dmdx*g_raw_dmdy * sizeof(unsigned char));
	g_needs_DMD_update = 0;
	return g_raw_dmdx*g_raw_dmdy;
}


// Audio related functions
// -----------------------
PINMAMEDLL_API int GetAudioChannels()
{
	return channels;
}

PINMAMEDLL_API int GetPendingAudioSamples(float* buffer,int outChannels, int maxNumber)
{
	return fillAudioBuffer(buffer, outChannels, maxNumber);
}


// Switch related functions
// ------------------------
PINMAMEDLL_API bool GetSwitch(int slot)
{
	return vp_getSwitch(slot) != 0;
}

PINMAMEDLL_API void SetSwitch(int slot, bool state)
{
	vp_putSwitch(slot, state ? 1 : 0);
}

// Lamps related functions
// -----------------------
PINMAMEDLL_API int GetMaxLamps() { return CORE_MAXLAMPCOL * 8; }

PINMAMEDLL_API int GetChangedLamps(int* changedStates)
{
	vp_tChgLamps chgLamps;
	const int uCount = vp_getChangedLamps(chgLamps);

	int* out = changedStates;
	for (int i = 0; i < uCount; i++)
	{
		*(out++) = chgLamps[i].lampNo;
		*(out++) = chgLamps[i].currStat;
	}
	return uCount;
}

// Solenoids related functions
// ---------------------------
PINMAMEDLL_API int GetMaxSolenoids() { return 64; }

PINMAMEDLL_API int GetChangedSolenoids(int* changedStates)
{
	vp_tChgSols chgSols;
	const int uCount = vp_getChangedSolenoids(chgSols);

	int* out = changedStates;
	for (int i = 0; i < uCount; i++)
	{
		*(out++) = chgSols[i].solNo;
		*(out++) = chgSols[i].currStat;
	}
	return uCount;
}

// GI strings related functions
// ----------------------------
PINMAMEDLL_API int GetMaxGIStrings() { return CORE_MAXGI; }

PINMAMEDLL_API int GetChangedGIs(int* changedStates)
{
	vp_tChgGIs chgGIs;
	const int uCount = vp_getChangedGI(chgGIs);

	int* out = changedStates;
	for (int i = 0; i < uCount; i++)
	{
		*(out++) = chgGIs[i].giNo;
		*(out++) = chgGIs[i].currStat;
	}
	return uCount;
}

//============================================================
//	osd_init
//============================================================

int osd_init(void)
{
	printf("osd_init\n");
	return 0;
}

//============================================================
//	osd_exit
//============================================================

void osd_exit(void)
{
	printf("osd_exit\n");
}
