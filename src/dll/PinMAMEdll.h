#pragma once

#if 1
#define PINMAMEDLL_API extern "C" __declspec(dllexport) 
#else
#define PINMAMEDLL_API extern "C" __declspec(dllimport) 
#endif


	// Setup related functions
	// -----------------------
	PINMAMEDLL_API void SetVPMPath(char* path);
	PINMAMEDLL_API void SetSampleRate(int sampleRate);

	// Game related functions
	// ----------------------
	PINMAMEDLL_API int  StartThreadedGame(char* gameName, bool showConsole = false);
	PINMAMEDLL_API void StopThreadedGame(bool locking = true);
	//PINMAMEDLL_API void KillThreadedGame(char* gameName);
	PINMAMEDLL_API void ResetGame();
	PINMAMEDLL_API bool IsGameReady();

	// Pause related functions
	// -----------------------
	PINMAMEDLL_API void Pause();
	PINMAMEDLL_API void Continue();
	PINMAMEDLL_API bool IsPaused();


	// DMD related functions
	// ---------------------
	PINMAMEDLL_API bool NeedsDMDUpdate();
	PINMAMEDLL_API int GetRawDMDWidth();
	PINMAMEDLL_API int GetRawDMDHeight();
	// needs pre-allocated GetRawDMDWidth()*GetRawDMDHeight()*sizeof(unsigned char) buffer
	// returns GetRawDMDWidth()*GetRawDMDHeight()
	PINMAMEDLL_API int GetRawDMDPixels(unsigned char* buffer);


	// Audio related functions
	// -----------------------
	// returns internally used channels by the game (1=mono,2=stereo)
	PINMAMEDLL_API int GetAudioChannels();
	// needs pre-allocated maxNumber*sizeof(float) buffer
	// returns actually processed samples (note that this is pre-multiplied by the requested outChannels (1=mono,2=stereo), same as maxNumber)
	PINMAMEDLL_API int GetPendingAudioSamples(float* buffer, int outChannels, int maxNumber);

	// Switch related functions
	// ------------------------
	PINMAMEDLL_API bool GetSwitch(int slot);
	PINMAMEDLL_API void SetSwitch(int slot, bool state);


	// Lamps related functions
	// -----------------------
	PINMAMEDLL_API int GetMaxLamps();
	// needs pre-allocated GetMaxLamps()*sizeof(int)*2 buffer (i.e. for each lamp: lampNo and currStat)
	// returns actually changed lamps
	PINMAMEDLL_API int GetChangedLamps(int* changedStates);

	// Solenoids related functions
	// ---------------------------
	PINMAMEDLL_API int GetMaxSolenoids();
	// needs pre-allocated GetMaxSolenoids()*sizeof(int)*2 buffer (i.e. for each solenoid: solNo and currStat)
	// returns actually changed solenoids
	PINMAMEDLL_API int GetChangedSolenoids(int* changedStates);

	// GI strings related functions
	// ----------------------------
	PINMAMEDLL_API int GetMaxGIStrings();
	// needs pre-allocated GetMaxGIStrings()*sizeof(int)*2 buffer (i.e. for each GI: giNo and currStat)
	// returns actually changed GI strings
	PINMAMEDLL_API int GetChangedGIs(int* changedStates);
