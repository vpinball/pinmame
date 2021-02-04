// license:BSD-3-Clause

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define LIBPINMAME_API extern "C" __declspec(dllexport) 
#define CALLBACK __stdcall
#else
#define LIBPINMAME_API extern "C" //__declspec(dllimport) 
#define CALLBACK
#endif

typedef struct {
   const char* name;
   const char* clone_of;
   const char* description;
   const char* year;
   const char* manufacturer;
} GameInfoStruct;

typedef void (CALLBACK *GameInfoCallback)(GameInfoStruct* gameInfoStruct);

// Setup related functions
// -----------------------
// Call these before doing anything else

LIBPINMAME_API void SetVPMPath(char* path);
LIBPINMAME_API void SetSampleRate(int sampleRate);

// Game related functions
// ----------------------

LIBPINMAME_API void GetGames(GameInfoCallback callback);
LIBPINMAME_API int StartThreadedGame(char* gameName, bool showConsole = false);
LIBPINMAME_API void StopThreadedGame(bool locking = true);
//LIBPINMAME_API void KillThreadedGame(char* gameName);
LIBPINMAME_API void ResetGame();
// IsGameReady will only be true after a 'while', i.e. after calling StartThreadedGame plus X msecs!
LIBPINMAME_API bool IsGameReady();

// ALL THE FOLLOWING FUNCTIONS WILL ONLY HAVE A MEANINGFUL EFFECT IF IsGameReady() IS TRUE!

// Pause related functions
// -----------------------

LIBPINMAME_API void Pause();
LIBPINMAME_API void Continue();
LIBPINMAME_API bool IsPaused();

// DMD related functions
// ---------------------

LIBPINMAME_API bool NeedsDMDUpdate();
LIBPINMAME_API int GetRawDMDWidth();
LIBPINMAME_API int GetRawDMDHeight();
// needs pre-allocated GetRawDMDWidth()*GetRawDMDHeight()*sizeof(unsigned char) buffer
// returns GetRawDMDWidth()*GetRawDMDHeight()
LIBPINMAME_API int GetRawDMDPixels(unsigned char* buffer);

// Audio related functions
// -----------------------
// returns internally used channels by the game (1=mono,2=stereo)

LIBPINMAME_API int GetAudioChannels();
// needs pre-allocated maxNumber*sizeof(float) buffer
// returns actually processed samples (note that this is pre-multiplied by the requested outChannels (1=mono,2=stereo), same as maxNumber)
LIBPINMAME_API int GetPendingAudioSamples(float* buffer, int outChannels, int maxNumber);
LIBPINMAME_API int GetPendingAudioSamples16bit(signed short* buffer, int outChannels, int maxNumber);

// Switch related functions
// ------------------------

LIBPINMAME_API bool GetSwitch(int slot);
LIBPINMAME_API void SetSwitch(int slot, bool state);
// Set all/a list of switches: For each switch, 2 ints are passed in: slot and state (0 or 1)
// As an exception, this call will also set switches to an initial state, i.e. even if IsGameReady() is still false
LIBPINMAME_API void SetSwitches(int* states, int numSwitches);

// Lamps related functions
// -----------------------

LIBPINMAME_API int GetMaxLamps();
// needs pre-allocated GetMaxLamps()*sizeof(int)*2 buffer (i.e. for each lamp: lampNo and currStat)
// returns actually changed lamps
LIBPINMAME_API int GetChangedLamps(int* changedStates);

// Solenoids related functions
// ---------------------------

LIBPINMAME_API int GetMaxSolenoids();
// needs pre-allocated GetMaxSolenoids()*sizeof(int)*2 buffer (i.e. for each solenoid: solNo and currStat)
// returns actually changed solenoids
LIBPINMAME_API int GetChangedSolenoids(int* changedStates);

// GI strings related functions
// ----------------------------

LIBPINMAME_API int GetMaxGIStrings();
// needs pre-allocated GetMaxGIStrings()*sizeof(int)*2 buffer (i.e. for each GI: giNo and currStat)
// returns actually changed GI strings
LIBPINMAME_API int GetChangedGIs(int* changedStates);
