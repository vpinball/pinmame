#include "StdAfx.h"
#include "VPinMAMEAboutDlg.h"

#include "ControllerOptions.h"
#include "resource.h"

#include <atlwin.h>

class CAboutDlg : public CDialogImpl<CAboutDlg> {
public:
	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	enum { IDD = IDD_ABOUTDLG };

private:
	int GetVersionResourceEntry(LPCTSTR lpszFilename, LPCTSTR lpszResourceEntry, LPTSTR lpszBuffer, WORD wMaxSize) {
		DWORD   dwVerHandle; 
		DWORD	dwInfoSize;
		HANDLE  hVersionInfo;
		LPVOID  lpEntryInfo;
		LPVOID  lpEntry;
		UINT	wEntrySize;
		LPDWORD lpdwVar;
		UINT    wVarSize;
		DWORD   dwTranslation;
		TCHAR   szEntry[256];

		if ( !lpszFilename || !lpszBuffer || !wMaxSize )
			return 0;

		lstrcpy(lpszBuffer, "");

		if ( !(dwInfoSize = GetFileVersionInfoSize((LPSTR) lpszFilename, &dwVerHandle)) ) {
			return 1;
		}
		
		hVersionInfo = GlobalAlloc(GPTR, dwInfoSize);
		if ( !hVersionInfo )
			return 0;

		lpEntryInfo = GlobalLock(hVersionInfo);
		if ( !lpEntryInfo )
			return 0;

		if ( !GetFileVersionInfo((LPSTR) lpszFilename, dwVerHandle, dwInfoSize, lpEntryInfo) )
			return 0;

		if ( !VerQueryValue(lpEntryInfo, "VarFileInfo\\Translation", (void**) &lpdwVar, &wVarSize) )
			return 0;
		
		dwTranslation = (*lpdwVar/65536) + (*lpdwVar%65536)*65536;

		wsprintf(szEntry, "StringFileInfo\\%0.8x\\%s", dwTranslation, lpszResourceEntry);

		if ( !VerQueryValue(lpEntryInfo, szEntry, &lpEntry, &wEntrySize) )
			return 0;

		if ( wEntrySize>wMaxSize )
			wEntrySize = wMaxSize;

		lstrcpyn(lpszBuffer, (LPSTR) lpEntry, wEntrySize);

		GlobalUnlock(hVersionInfo);
		GlobalFree(hVersionInfo);

		return 1;
	}

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		CenterWindow();

		char szFilename[MAX_PATH];
		GetModuleFileName(_Module.m_hInst, szFilename, sizeof szFilename);

		char szVersion[256];
		GetVersionResourceEntry(szFilename, TEXT("ProductVersion"), szVersion, sizeof szVersion);

		char szVersionText[256], szBuildDateText[256], szFormat[256];  
		GetDlgItemText(IDC_VERSION, szFormat, sizeof szVersionText);
		wsprintf(szVersionText, szFormat, szVersion);
		SetDlgItemText(IDC_VERSION, szVersionText);
		wsprintf(szBuildDateText,GetBuildDateString());
		SetDlgItemText(IDC_BUILDDATE, szBuildDateText);
		return 1;
	}

	LRESULT OnOK(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnCancel(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}
};

void ShowAboutDlg(HWND hParent)
{
	CAboutDlg AboutDlg;
	AboutDlg.DoModal(hParent);
}

char * GetBuildDateString(void)
{
    static char tmp[120];
    wsprintf(tmp,"%s",__DATE__);
    return tmp;
}