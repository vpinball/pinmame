//============================================================
//
//  multidef.h - Win32 Multi Monitors
//
//============================================================

#ifndef __MULTIDEF__
#define __MULTIDEF__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

/*
 The MinGW implementations of windef.h and winuser.h define
 a bunch of things that prevent the standard MS multimon.h
 header from working as designed.  This is a workaround.

 Note, we can only include multimon.h exactly once.  So any
 derivative builds like MAME32 should include this file
 instead of multimon.h
*/

#if defined(SM_CMONITORS) && (WINVER < 0x0500)

#undef MONITOR_DEFAULTTONULL
#undef MONITOR_DEFAULTTOPRIMARY
#undef MONITOR_DEFAULTTONEAREST
#undef MONITORINFOF_PRIMARY

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002
#define MONITORINFOF_PRIMARY        0x00000001

#endif /* (SM_CMONITORS) && (WINVER < 0x0500) */


#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

#endif /* __MULTIDEF__ */
