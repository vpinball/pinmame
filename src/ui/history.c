/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  history.c

    history functions.

***************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#if _MSC_VER >= 1800
 // Windows 2000 _WIN32_WINNT_WIN2K
 #define _WIN32_WINNT 0x0500
#elif _MSC_VER < 1600
 #define _WIN32_WINNT 0x0400
#else
 #define _WIN32_WINNT 0x0403
#endif
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>
#include <stdio.h>

#include <driver.h>
#include "M32Util.h"

#include "history.h"

extern int load_driver_history(const struct GameDriver *drv, char *buffer, int bufsize);

/**************************************************************
 * functions
 **************************************************************/

// Load indexes from history.dat if found
char * GetGameHistory(int driver_index)
{
	static char buffer[8192];
	buffer[0] = '\0';

	if (load_driver_history(drivers[driver_index],buffer,sizeof(buffer)) != 0)
		return buffer;

	return ConvertToWindowsNewlines(buffer);
}
