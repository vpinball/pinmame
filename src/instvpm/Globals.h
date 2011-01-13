#ifndef GLOBALS_H
#define GLOBALS_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

extern HINSTANCE	g_hInstance;
extern char			g_szCaption[256];

#endif // GLOBALS_H