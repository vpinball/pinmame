/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2000 Michael Soderstrom and Chris Kirmse
    
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef WIN32UI_H
#define WIN32UI_H

#include "driver.h"
#include "options.h"
#include "ScreenShot.h"

#define GAME_BROKEN (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)

enum
{
    TAB_PICKER = 0,
    TAB_DISPLAY,
    TAB_MISC,
    NUM_TABS
};

typedef struct
{
    BOOL neogeo;
} game_data_type;

/* global variables */
extern char *column_names[COLUMN_MAX];

extern options_type*   GetPlayingGameOptions(void);
extern game_data_type* GetGameData(void);

extern HWND  GetMainWindow(void);
extern int   GetNumGames(void);
extern void  GetRealColumnOrder(int order[]);
extern HICON LoadIconFromFile(char *iconname);
extern BOOL  GameUsesTrackball(int game);
extern void  UpdateScreenShot(void);
extern void  ResizePickerControls(HWND hWnd);
extern int   UpdateLoadProgress(const char* name, int current, int total);

// Move The in "The Title (notes)" to "Title, The (notes)"
extern char *ModifyThe(const char *str);

/* globalized for painting tree control */
extern HBITMAP      hBitmap;
extern HPALETTE     hPALbg;
extern MYBITMAPINFO bmDesc;

#endif
