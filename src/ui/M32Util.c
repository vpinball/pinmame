/***************************************************************************

  M.A.M.E.32	-  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  M32Util.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <assert.h>
#include <stdio.h>
#include "MAME32.h"
#include "M32Util.h"

/***************************************************************************
	function prototypes
 ***************************************************************************/

/***************************************************************************
	External variables
 ***************************************************************************/

BOOL bErrorMsgBox = TRUE;

/***************************************************************************
	Internal structures
 ***************************************************************************/

/***************************************************************************
	Internal variables
 ***************************************************************************/


/***************************************************************************
	External functions
 ***************************************************************************/

/*
	ErrorMsg
*/
void __cdecl ErrorMsg(const char* fmt, ...)
{
	static FILE*	pFile = NULL;
	DWORD			dwWritten;
	char			buf[5000];
	va_list 		va;

	va_start(va, fmt);

	if (bErrorMsgBox == TRUE)
	{
		wvsprintf(buf, fmt, va);
		MessageBox(GetActiveWindow(), buf, MAME32NAME, MB_OK | MB_ICONERROR);
	}

	lstrcat(buf, MAME32NAME ": ");

	wvsprintf(&buf[lstrlen(buf)], fmt, va);
	lstrcat(buf, "\n");

	OutputDebugString(buf);

	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, strlen(buf), &dwWritten, NULL);

	if (pFile == NULL)
		pFile = fopen("debug.txt", "wt");
	if (pFile != NULL)
		fprintf(pFile, buf);

	va_end(va);
}

void __cdecl TraceMsg(const char* fmt, ...)
{
	char	buf[5000];
	va_list va;

	va_start(va, fmt);

	lstrcat(buf, MAME32NAME ": ");

	wvsprintf(&buf[lstrlen(buf)], fmt, va);
	lstrcat(buf, "\n");

	OutputDebugString(buf);

	va_end(va);
}

UINT GetDepth(HWND hWnd)
{
	UINT	nBPP;
	HDC 	hDC;

	hDC = GetDC(hWnd);
	
	nBPP = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);

	ReleaseDC(hWnd, hDC);

	return nBPP;
}

BOOL OnNT()
{
	OSVERSIONINFO version_info;

	version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&version_info);

	if (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
		return TRUE;
	else
		return FALSE;
}

/*
 * Return TRUE if comctl32.dll is version 4.71 or greater
 * otherwise return FALSE.
 */
BOOL GetDllVersion()
{
	HMODULE hModule = GetModuleHandle("comctl32");

	if (hModule)
	{
		FARPROC lpfnICCE = GetProcAddress(hModule, "InitCommonControlsEx");

		if (NULL != lpfnICCE)
		{
			FARPROC lpfnDLLI = GetProcAddress(hModule, "DllInstall");

			if (NULL != lpfnDLLI) 
			{
				/* comctl 4.71 or greater */
				return TRUE;
			}
			/* comctl 4.7 - fall through
			 * return FALSE;
			 */
		}
		/* comctl 4.0 - fall through
		 * return FALSE;
		 */
	}
	/* DLL not found */
	return FALSE;
}

void DisplayTextFile(HWND hWnd, char *cName)
{
	HINSTANCE hErr;
	char	  *msg = 0;

	hErr = ShellExecute(hWnd, NULL, cName, NULL, NULL, SW_SHOWNORMAL);
	if ((int)hErr > 32)
		return;

	switch((int)hErr)
	{
	case 0:
		msg = "The operating system is out of memory or resources.";
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = "The specified file was not found."; 
		break;

	case SE_ERR_NOASSOC :
		msg = "There is no application associated with the given filename extension.";
		break;

	case SE_ERR_OOM :
		msg = "There was not enough memory to complete the operation.";
		break;

	case SE_ERR_PNF :
		msg = "The specified path was not found.";
		break;

	case SE_ERR_SHARE :
		msg = "A sharing violation occurred.";
		break;

	default:
		msg = "Unknown error.";
	}
 
	MessageBox(NULL, msg, cName, MB_OK); 
}

char* MyStrStrI(const char* pFirst, const char* pSrch)
{
	char* cp = (char*)pFirst;
	char* s1;
	char* s2;
	
	while (*cp)
	{
		s1 = cp;
		s2 = (char*)pSrch;
		
		while (*s1 && *s2 && !strnicmp(s1, s2, 1))
			s1++, s2++;
		
		if (!*s2)
			return cp;
		
		cp++;
	}
	return NULL;
}

/***************************************************************************
	Internal functions
 ***************************************************************************/

