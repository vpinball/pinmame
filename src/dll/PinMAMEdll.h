#pragma once

// standard includes
#include <thread>

#ifdef PINMAMEDLL_API
#define PINMAMEDLL_API __declspec(dllexport) 
#else
#define PINMAMEDLL_API __declspec(dllimport) 
#endif

extern "C"
{

	PINMAMEDLL_API void SetVPMPath(char* path);
	PINMAMEDLL_API void SetSampleRate(int sampleRate);

	PINMAMEDLL_API int StartThreadedGame(char* gameName, bool showConsole = false);
	PINMAMEDLL_API void StopThreadedGame(bool locking = true);
	//PINMAMEDLL_API void KillThreadedGame(char* gameName);
	PINMAMEDLL_API void ResetGame();

	PINMAMEDLL_API void Pause();
	PINMAMEDLL_API void Continue();
	PINMAMEDLL_API bool IsPaused();

	PINMAMEDLL_API bool NeedsDMDUpdate();
	PINMAMEDLL_API int GetRawDMDWidth();
	PINMAMEDLL_API int GetRawDMDHeight();
	PINMAMEDLL_API int GetRawDMDPixels(unsigned char* buffer);

	PINMAMEDLL_API int GetPendingAudioSamples(float* buffer, int maxNumber);
}
