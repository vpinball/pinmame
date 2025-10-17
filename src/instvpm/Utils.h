#pragma once

#include <unknwn.h>

void DisplayError(HWND hWnd, HRESULT hr, char* szReason, char* szAdvice=NULL);
void DisplayCOMError(IUnknown *pUnknown, REFIID riid);
LONG RegDeleteKeyEx(HKEY hKey, LPCTSTR lpszSubKey);
int GetVersionResourceEntry(LPCTSTR lpszFilename, LPCTSTR lpszResourceEntry, LPTSTR lpszBuffer, WORD wMaxSize);
