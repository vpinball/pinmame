#ifndef CONTROLLERSPLASHWND_H
#define CONTROLLERSPLASHWND_H

void CreateSplashWnd(void **ppData, char* pszCredits=NULL);
void DestroySplashWnd(void **ppData);
void WaitForSplashWndToClose(void **ppData);

#endif // CONTROLLERSPLASHWND_H
