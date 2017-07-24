#ifndef UTILS_H
#define UTILS_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void DisplayError(HWND hWnd, HRESULT hr, char* szReason, char* szAdvice=NULL);
void DisplayCOMError(IUnknown *pUnknown, REFIID riid);
LONG RegDeleteKeyEx(HKEY hKey, LPCTSTR lpszSubKey);
int GetVersionResourceEntry(LPCTSTR lpszFilename, LPCTSTR lpszResourceEntry, LPTSTR lpszBuffer, WORD wMaxSize);

#endif // UTILS_H


