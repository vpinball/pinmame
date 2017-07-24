#ifndef VPINMAMEDISCLAIMERDLG
#define VPINMAMEDISCLAIMERDLG
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

BOOL ShowDisclaimer(HWND hParentWnd, char* szDescription);

#endif // VPINMAMEDISCLAIMERDLG