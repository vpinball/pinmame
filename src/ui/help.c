/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.
	
***************************************************************************/

/***************************************************************************

  help.c

	Help wrapper code.

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
#include "help.h"

typedef HWND (WINAPI *HtmlHelpProc)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

/***************************************************************************
 Internal structures
***************************************************************************/

/***************************************************************************
 function prototypes
***************************************************************************/

static void Help_Load(void);

/***************************************************************************
 External function prototypes
***************************************************************************/

/***************************************************************************
 External variables
***************************************************************************/

/***************************************************************************
 Internal variables
***************************************************************************/

static HtmlHelpProc g_pHtmlHelp;
static HMODULE      g_hHelpLib;
static DWORD_PTR    g_dwCookie;

/**************************************************************************
 External functions
***************************************************************************/

int HelpInit(void)
{
	g_pHtmlHelp = NULL;
	g_hHelpLib  = NULL;

	g_dwCookie = 0;
	HelpFunction(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&g_dwCookie);
	return 0;
}

void HelpExit(void)
{
	HelpFunction(NULL, NULL, HH_CLOSE_ALL, 0);
	HelpFunction(NULL, NULL, HH_UNINITIALIZE, (DWORD_PTR)g_dwCookie);

	g_dwCookie  = 0;
	g_pHtmlHelp = NULL;

	if (g_hHelpLib)
	{
		FreeLibrary(g_hHelpLib);
		g_hHelpLib = NULL;
	}
}

HWND HelpFunction(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	if (g_pHtmlHelp == NULL)
		Help_Load();

	if (g_pHtmlHelp)
		return g_pHtmlHelp(hwndCaller, pszFile, uCommand, dwData);
	else
		return NULL;
}

/***************************************************************************
 Internal functions  
***************************************************************************/

static void Help_Load(void)
{
#if defined(__GNUC__)
	g_hHelpLib = LoadLibrary("hhctrl.ocx");
	if (g_hHelpLib)
	{
		FARPROC pProc = NULL;
		pProc = GetProcAddress(g_hHelpLib, "HtmlHelpA");
		if (pProc)
		{
			g_pHtmlHelp = (HtmlHelpProc)pProc;
		}
		else
		{
			FreeLibrary(g_hHelpLib);
			g_hHelpLib = NULL;
		}
	}
#else
	g_pHtmlHelp = HtmlHelp;
#endif
}
