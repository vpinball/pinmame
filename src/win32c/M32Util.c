/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
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
#include "M32Util.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

/***************************************************************************
    External variables
 ***************************************************************************/

BOOL    bErrorMsgBox = TRUE;

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
    static FILE*    pFile = NULL;
    DWORD           dwWritten;
    char            buf[5000];
    va_list         va;

    va_start(va, fmt);

    if (bErrorMsgBox == TRUE)
    {
        wvsprintf(buf, fmt, va);
        MessageBox(GetActiveWindow(), buf, "Hallo", MB_OK | MB_ICONERROR);
    }

    lstrcpy(buf, "Hallo");
    lstrcat(buf, ": ");

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

/* Used by common.c */
int strcasecmp(const char* a, const char* b)
{
    return stricmp(a, b);
}

/* Parse the given comma-delimited string into a LOGFONT structure */
void FontDecodeString(char *string, LOGFONT *f)
{
    char *ptr;
    
    sscanf(string, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i",
        &f->lfHeight,
        &f->lfWidth,
        &f->lfEscapement,
        &f->lfOrientation,
        &f->lfWeight,
        &f->lfItalic,
        &f->lfUnderline,
        &f->lfStrikeOut,
        &f->lfCharSet,
        &f->lfOutPrecision,
        &f->lfClipPrecision,
        &f->lfQuality,
        &f->lfPitchAndFamily);
    ptr = strrchr(string, ',');
    if (ptr != NULL)
        strcpy(f->lfFaceName, ptr + 1);
}

/* Encode the given LOGFONT structure into a comma-delimited string */
void FontEncodeString(LOGFONT *f, char *string)
{
    sprintf(string, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%s",
        f->lfHeight,
        f->lfWidth,
        f->lfEscapement,
        f->lfOrientation,
        f->lfWeight,
        f->lfItalic,
        f->lfUnderline,
        f->lfStrikeOut,
        f->lfCharSet,
        f->lfOutPrecision,
        f->lfClipPrecision,
        f->lfQuality,
        f->lfPitchAndFamily,
        f->lfFaceName
        );
}

UINT GetDepth(HWND hWnd)
{
    UINT    nBPP;
    HDC     hDC;

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

char* MyStrStrI(const char* pFirst, const char* pSrch)
{
    char* cp = (char*)pFirst;
    char* s1;
    char* s2;
    
    while (*cp)
    {
        s1 = cp;
        s2 = (char*)pSrch;
        
        while (*s1 && *s2 && !_strnicmp(s1, s2, 1))
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

