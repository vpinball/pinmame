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
	PINMAMEDLL_API int StartThreadedGame(char* gameName, bool showConsole = false);
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
	PINMAMEDLL_API int GetRawDMDPixels(unsigned char* buffer);


	// Audio related functions
	// -----------------------
	PINMAMEDLL_API int GetAudioChannels();
	PINMAMEDLL_API int GetPendingAudioSamples(float* buffer, int outChannels, int maxNumber);

	// Switch related functions
	// ------------------------
	PINMAMEDLL_API bool GetSwitch(int slot);
	PINMAMEDLL_API void SetSwitch(int slot, bool state);


	// Lamps related functions
	// -----------------------
	PINMAMEDLL_API int GetMaxLamps();
	PINMAMEDLL_API int GetChangedLamps(int* changedStates);

	// Solenoids related functions
	// ---------------------------
	PINMAMEDLL_API int GetMaxSolenoids();
	PINMAMEDLL_API int GetChangedSolenoids(int* changedStates);

	// GI strings related functions
	// ----------------------------
	PINMAMEDLL_API int GetMaxGIStrings();
	PINMAMEDLL_API int GetChangedGIs(int* changedStates);
