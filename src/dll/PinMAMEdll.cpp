#include "PinMAMEdll.h"

//============================================================
//
//	winmain.c - Win32 main program
//
//============================================================

#define MAX_PATH          1024

// standard includes
#include <time.h>
#include <ctype.h>
#include <thread>
#include <conio.h>

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

	//extern HWND win_video_window;

	//extern HANDLE g_hEnterThrottle;
	extern int g_iSyncFactor;
	extern struct RunningMachine *Machine;
	extern struct mame_display *current_display_ptr;

	extern unsigned char  g_raw_dmdbuffer[DMD_MAXY*DMD_MAXX];
	extern unsigned int g_raw_colordmdbuffer[DMD_MAXY*DMD_MAXX];
	extern unsigned int g_raw_dmdx;
	extern unsigned int g_raw_dmdy;
	extern unsigned int g_needs_DMD_update;
	extern int g_cpu_affinity_mask;

	extern char g_fShowWinDMD;

	int g_fHandleKeyboard = 0, g_fHandleMechanics = 0;
	extern int verbose;
	extern int trying_to_quit;
	void OnSolenoid(int nSolenoid, int IsActive);
	void OnStateChange(int nChange);
	extern void win_timer_enable(int enabled);

	UINT8 win_trying_to_quit;
	volatile int g_fPause = 0;
	volatile int g_fDumpFrames = 0;
	struct rc_struct *rc;
}

//============================================================
// Console Debugging Section (Optionnal)
//============================================================

#define ENABLE_CONSOLE_DEBUG
#ifdef ENABLE_CONSOLE_DEBUG
#include <windows.h>
#include <WinCon.h>

bool g_useConsole = false;
char g_vpmPath[MAX_PATH];
int g_sampleRate = 48000;

void ShowConsole()
{
	g_useConsole = true;
	FILE * pConsole;
	AllocConsole();
	freopen_s(&pConsole, "CONOUT$", "wb", stdout);
}

void CloseConsole()
{
	if (g_useConsole)
		FreeConsole();
}
#endif


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
}


//============================================================
//	GLOBAL VARIABLES
//============================================================
int verbose;


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

char* composePath(char* path, char* file)
{
	int pathl = strlen(path);
	int filel = strlen(file);
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
	int res = run_game(game_index);
}

static std::thread* pRunningGame = nullptr;


//============================================================
//	Dll Exported Functions Section
//============================================================

// Setup Functions
// ---------------

void SetVPMPath(char* path)
{
	strcpy(g_vpmPath, path);
}

void SetSampleRate(int sampleRate)
{
	g_sampleRate = sampleRate;
}


// Game related functions
// ---------------------

int StartThreadedGame(char* gameName, bool showConsole)
{
#ifdef ENABLE_CONSOLE_DEBUG
	if (showConsole)
		ShowConsole();
#endif
	rc = cli_rc_create();

	int game_index = GetGameNumFromString(gameName);
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
	
	if (locking)
	{
		printf("Waiting for clean exit...\n");
		pRunningGame->join();
	}

 

	delete(pRunningGame);
	pRunningGame = nullptr;

#ifdef ENABLE_CONSOLE_DEBUG
	CloseConsole();
#endif
}

// Pause related functions
// -----------------------
void ResetGame()
{
	machine_reset();
}

void Pause()
{
	g_fPause = 1;
}

void Continue()
{
	g_fPause = 0;
}

bool IsPaused()
{
	return g_fPause > 0;
}


// DMD related functions
// ---------------------

bool NeedsDMDUpdate()
{
	return g_needs_DMD_update;
}

int GetRawDMDWidth()
{
	return g_raw_dmdx;
}

int GetRawDMDHeight()
{
	return g_raw_dmdy;
}

int GetRawDMDPixels(unsigned char* buffer)
{
	if (g_raw_dmdx < 0 || g_raw_dmdy < 0)
		return -1;
	memcpy(buffer, g_raw_dmdbuffer, g_raw_dmdx*g_raw_dmdy * sizeof(unsigned char));
	return g_raw_dmdx*g_raw_dmdy;
}


// Audio related functions
// -----------------------
int GetPendingAudioSamples(float* buffer, int maxNumber)
{
	return fillAudioBuffer(buffer, maxNumber);
}


// Switch related functions
// ------------------------
bool GetSwitch(int slot)
{
	return vp_getSwitch(slot) != 0;
}
void SetSwitch(int slot, bool state)
{
	vp_putSwitch(slot, state ? 1 : 0);
}

// Lamps related functions
// -----------------------
int GetMaxLamps() {	return CORE_MAXLAMPCOL * 8; }

int GetChangedLamps(int* changedStates)
{
	vp_tChgLamps chgLamps;
	int uCount = vp_getChangedLamps(chgLamps);

	int* out = changedStates;
	for (int i = 0; i < uCount; i++)
	{
		*(out++) = chgLamps[i].lampNo;
		*(out++) = chgLamps[i].currStat;
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
