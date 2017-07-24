#ifndef WPCABOUTDLG_H
#define WPCABOUTDLG_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void ShowAboutDlg(HWND hParent);
char * GetBuildDateString(void);

#endif // WPCABOUTDLG_H