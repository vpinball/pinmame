#ifndef WPCSPLASHWND_H
#define WPCSPLASHWND_H

void CreateSplashWnd(void **ppData, char* pszCredits=NULL);
void DestroySplashWnd(void **ppData);
void WaitForSplashWndToClose(void **ppData);

#endif // WPCSPLASHWND_H