/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __UCLOCK_H__
#define __UCLOCK_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef LONGLONG uclock_t;

extern LONGLONG UCLOCKS_PER_SEC;

extern int          uclock_init(void);
extern int          uclock_exit(void);

extern uclock_t     uclock(void);

#endif
