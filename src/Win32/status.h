/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __STATUS_H__
#define __STATUS_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void StatusCreate(void);
int  GetStatusHeight(void);
void StatusWindowSize(UINT state,int cx,int cy);
void StatusDrawItem(const DRAWITEMSTRUCT *lpdi);
void StatusWrite(int leds_status);
void StatusUpdate(void);
void StatusUpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS);
void StatusSetString(char *str);
void StatusDelete(void);


#endif
