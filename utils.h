#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>

#include "foxpro.h"

#define PACKVERSION(major,minor) MAKELONG(minor,major)

void ShowError();
void CenterWindow(HWND hWnd);
void PrependToString(char *szString, char *szStringToPrepend);
void CreateToolTip (HINSTANCE hInst, HWND hwnd, LPCSTR szTipText);
void CreateBalloonTip(HINSTANCE hInst, HWND hwnd, LPCSTR szTipText, UINT uBalloonStyles);
DWORD GetDllVersion(LPCTSTR lpszDllName);
void DisplayTextFile(HWND hWnd, LPCSTR szName);
FILETIME GetModifiedFileTime(LPCSTR szFile, BOOL bShowErrors);
BOOL FindInFile(LPCSTR szFile, LPCSTR szString);

#endif