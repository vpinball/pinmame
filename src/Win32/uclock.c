/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  uclock.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include <mmsystem.h>
#include "uclock.h"

/***************************************************************************
    External variables
 ***************************************************************************/

LONGLONG UCLOCKS_PER_SEC = -1;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static BOOL bHavePerformanceCounter;

/***************************************************************************
    External functions  
 ***************************************************************************/

uclock_t uclock(void)
{
    LARGE_INTEGER   tick_count;
    
    if (bHavePerformanceCounter == TRUE)
    {
        QueryPerformanceCounter(&tick_count);
        return tick_count.QuadPart;
    }
    else
    {
        return timeGetTime();
    }
}

int uclock_init(void)
{
    LARGE_INTEGER   tick_frequency;

    if (UCLOCKS_PER_SEC != -1)
        return 0;

    if (QueryPerformanceFrequency(&tick_frequency) == TRUE)
    {
        bHavePerformanceCounter = TRUE;     
        UCLOCKS_PER_SEC = tick_frequency.QuadPart;       
    }
    else
    {
        MMRESULT    hResult;

        bHavePerformanceCounter = FALSE;
        UCLOCKS_PER_SEC = CLOCKS_PER_SEC;

        hResult = timeBeginPeriod(1);
        if (hResult == TIMERR_NOCANDO)
            return 1;
    }
    return 0;
}

int uclock_exit(void)
{
    if (bHavePerformanceCounter == TRUE)
    {
        return 0;
    }
    else
    {
        MMRESULT    hResult;
        hResult = timeEndPeriod(1);
        if (hResult == TIMERR_NOCANDO)
            return 1;
    }
    return 1;
}
