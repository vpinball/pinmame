/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

enum
{
	FORMAT_PNG = 0,
	FORMAT_BMP,
	FORMAT_UNKNOWN,
	FORMAT_MAX = FORMAT_UNKNOWN
};

/* Located in ScreenShot.c */
extern char pic_format[FORMAT_MAX][4];

typedef struct _mybitmapinfo
{
	int bmWidth;
	int bmHeight;
	int bmColors;
} MYBITMAPINFO, *LPMYBITMAPINFO;

extern BOOL LoadScreenShot(int nGame, int nType);
extern BOOL DrawScreenShot(HWND hWnd);
extern void FreeScreenShot(void);
extern BOOL GetScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict);
extern BOOL ScreenShotLoaded(void);

extern BOOL    LoadDIB(LPCTSTR filename, HGLOBAL *phDIB, HPALETTE *pPal, BOOL flyer);
extern HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc);

#endif
