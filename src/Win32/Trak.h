/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef TRAK_H
#define TRAK_H

#include "osdepend.h"

enum ETrakCode
{
    TRAK_FIRE_ANY,
    TRAK_FIRE1,
    TRAK_FIRE2,
    TRAK_FIRE3,
    TRAK_FIRE4,
    TRAK_FIRE5
};

#define TRAK_MAXX_RES   120
#define TRAK_MAXY_RES   120

struct OSDTrak
{
    int     (*init)(options_type *options);
    void    (*exit)(void);
    void    (*trak_read)(int player, int *deltax, int *deltay);

    int     (*trak_pressed)(enum ETrakCode eTrakCode);    
    BOOL    (*OnMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

extern struct OSDTrak Trak;

#endif
