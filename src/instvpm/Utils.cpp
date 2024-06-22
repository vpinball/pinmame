#define WIN32_LEAN_AND_MEAN
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
#include <Windows.h>
#include "Resource.h"

#include "Utils.h"

extern TCHAR g_szCaption[256];

void DisplayError(HWND hWnd, HRESULT hr, char* szReason, char* szAdvice)
{
	TCHAR szMsg[512];

	char *pszErrorMsg = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 
		NULL, hr, 0, (LPTSTR) &pszErrorMsg, 0, NULL);

	if ( szAdvice )
		wsprintf(szMsg, TEXT("%s\n\nError 0x%08x: %s\n%s"), szReason, hr, pszErrorMsg, szAdvice);
	else
		wsprintf(szMsg, TEXT("%s\n\nError 0x%08x: %s"), szReason, hr, pszErrorMsg);

	MessageBox(hWnd, szMsg, g_szCaption, MB_ICONEXCLAMATION|MB_OK);
	LocalFree(pszErrorMsg);
}

void DisplayCOMError(IUnknown *pUnknown, REFIID riid)
{
	if ( !pUnknown )
		return;

	ISupportErrorInfo *pSupportErrorInfo;

	HRESULT hr = pUnknown->QueryInterface(IID_ISupportErrorInfo, (void**) &pSupportErrorInfo);
	if ( FAILED(hr) )
		return;

	if ( pSupportErrorInfo->InterfaceSupportsErrorInfo(riid)==S_OK ) {
		IErrorInfo* pErrorInfo;
		GetErrorInfo(0, &pErrorInfo);

		BSTR sDescription;
		pErrorInfo->GetDescription(&sDescription);
		MessageBoxW(0, sDescription, L"Error from the VPinMAME.Controller interface.", MB_ICONERROR|MB_OK);

		SysFreeString(sDescription);
		pErrorInfo->Release();
	}

	pSupportErrorInfo->Release();
}


LONG RegDeleteKeyEx(HKEY hKey, LPCTSTR lpszSubKey)
{
	LONG lResult;

	if ( (lResult=RegOpenKeyEx(hKey, lpszSubKey, NULL, KEY_ALL_ACCESS, &hKey))!=ERROR_SUCCESS )
		return lResult;

	char  szKeyName[MAX_PATH];
	DWORD dwKeyNameSize = sizeof szKeyName;
	while ( RegEnumKeyEx(hKey, 0, (char*) &szKeyName, &dwKeyNameSize, NULL, NULL, NULL, NULL)==ERROR_SUCCESS ) { 
		if ( (lResult=RegDeleteKeyEx(hKey, szKeyName))!=ERROR_SUCCESS ) {
			RegCloseKey(hKey);
			return lResult;
		}
		dwKeyNameSize = sizeof szKeyName;
	}

	lResult = RegDeleteKey(hKey, "");
	RegCloseKey(hKey);

	return lResult;
}

int GetVersionResourceEntry(LPCTSTR lpszFilename, LPCTSTR lpszResourceEntry, LPTSTR lpszBuffer, WORD wMaxSize)
{
	DWORD	dwVerHandle;
	DWORD	dwInfoSize;
	LPVOID	lpEntry;
	UINT	wEntrySize;
	LPDWORD	lpdwVar;
	UINT	wVarSize;
	TCHAR	szEntry[256];

	if ( !lpszFilename || !lpszBuffer || !wMaxSize )
		return 0;

	lstrcpy(lpszBuffer, "");

	if ( !(dwInfoSize = GetFileVersionInfoSize((LPSTR) lpszFilename, &dwVerHandle)) ) {
		return 1;
	}

	HANDLE hVersionInfo = GlobalAlloc(GPTR, dwInfoSize);
	if ( !hVersionInfo )
		return 0;

	LPVOID lpEntryInfo = GlobalLock(hVersionInfo);
	if ( !lpEntryInfo )
		return 0;

	if ( !GetFileVersionInfo((LPSTR) lpszFilename, dwVerHandle, dwInfoSize, lpEntryInfo) )
		return 0;

	if ( !VerQueryValue(lpEntryInfo, "VarFileInfo\\Translation", (void**) &lpdwVar, &wVarSize) )
		return 0;

	DWORD dwTranslation = (*lpdwVar/65536) + (*lpdwVar%65536)*65536;

	wsprintf(szEntry, TEXT("StringFileInfo\\%0.8x\\%s"), dwTranslation, lpszResourceEntry);

	if ( !VerQueryValue(lpEntryInfo, szEntry, &lpEntry, &wEntrySize) )
		return 0;

	if ( wEntrySize>wMaxSize )
		wEntrySize = wMaxSize;

	strncpy(lpszBuffer, (LPSTR) lpEntry, wEntrySize);

	GlobalUnlock(hVersionInfo);
	GlobalFree(hVersionInfo);

	return 1;
}
