/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse
    
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef WIN32UI_H
#define WIN32UI_H

#include <driver.h>

enum
{
	TAB_PICKER = 0,
	TAB_DISPLAY,
	TAB_MISC,
	NUM_TABS
};

typedef struct
{
	INT resource;
	const char *icon_name;
} ICONDATA;

HWND GetMainWindow(void);
HWND GetTreeView(void);
int GetNumGames(void);
void GetRealColumnOrder(int order[]);
HICON LoadIconFromFile(const char *iconname);
void UpdateScreenShot(void);
void ResizePickerControls(HWND hWnd);

// Move The in "The Title (notes)" to "Title, The (notes)"
char * ModifyThe(const char *str);

// Convert Ampersand so it can display in a static control
char * ConvertAmpersandString(const char *s);

// globalized for painting tree control
HBITMAP GetBackgroundBitmap(void);
HPALETTE GetBackgroundPalette(void);
MYBITMAPINFO * GetBackgroundInfo(void);

int GetMinimumScreenShotWindowWidth(void);

// we maintain an array of drivers sorted by name, useful all around
int GetDriverIndex(const struct GameDriver *driver);
int GetGameNameIndex(const char *name);
int GetIndexFromSortedIndex(int sorted_index);

int Mame32Main(HINSTANCE    hInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow);

BOOL MouseHasBeenMoved(void);

#endif
