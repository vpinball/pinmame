/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef HELP_H
#define HELP_H

#include <htmlhelp.h>

#ifdef MESS
/* MESS32 text files */
#define HELPTEXT_RELEASE    "Mess.txt"
#define HELPTEXT_WHATS_NEW  "Messnew.txt"
#else
/* MAME32 text files */
#define HELPTEXT_SUPPORT    "support.htm"
#define HELPTEXT_RELEASE    "windows.txt"
#define HELPTEXT_WHATS_NEW  "whatsnew.txt"
#endif

#if !defined(MAME32HELP)
#define MAME32HELP "mame32.chm"
#endif

extern int  Help_Init(void);
extern void Help_Exit(void);
extern HWND Help_HtmlHelp(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

#endif
