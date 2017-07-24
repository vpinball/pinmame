#ifndef CONTROLLERSPLASHWND_H
#define CONTROLLERSPLASHWND_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void CreateSplashWnd(void **ppData, char* pszCredits=NULL);
void DestroySplashWnd(void **ppData);
void WaitForSplashWndToClose(void **ppData);

#endif // CONTROLLERSPLASHWND_H
