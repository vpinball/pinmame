#include "StdAfx.h"
#include "VPinMAMEAboutDlg.h"

#include "resource.h"

#include <atlwin.h>

/* October 1,2003 - Added Hyperlink to About Box  (SJE) */


//Sorry Tom, some day you'll show me how to make this work in the class interface instead!
static 	HBRUSH hLinkBrush = 0;
static	HFONT  hFont = 0;

class CAboutDlg : public CDialogImpl<CAboutDlg> {
public:
	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroyDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStaticDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_HOMEPAGELINK, OnHomePageLink)
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

	/*Dialog Init*/
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		CenterWindow();

		char szFilename[MAX_PATH];
		GetModuleFileName(_Module.m_hInst, szFilename, sizeof szFilename);

		char szVersion[256];
		GetVersionResourceEntry(szFilename, TEXT("ProductVersion"), szVersion, sizeof szVersion);

		char szVersionText[256], szBuildDateText[256], szFormat[256];  
		GetDlgItemText(IDC_VERSION, szFormat, sizeof szVersionText);
		wsprintf(szVersionText, szFormat, szVersion);

		//Add compile time specific strings to version string
		char szAdjust[MAX_PATH];
		wsprintf(szAdjust,"%s","");
#ifdef MAME_DEBUG
		wsprintf(szAdjust,"%s","DEBUG");
#else
	#ifdef TEST_NEW_STERN
		wsprintf(szAdjust,"%s","STERN");
	#endif
	#ifdef NO_EXPIRATION_DATE
		if(strcmp(szAdjust,"STERN")==0)
			wsprintf(szAdjust,"%s","DEV");
		else
			wsprintf(szAdjust,"%s%s",szAdjust,"NO EXPIRE");
	#endif
#endif
		if(strlen(szAdjust)) wsprintf(szVersionText, "%s (%s)",szVersionText,szAdjust);
		//
		SetDlgItemText(IDC_VERSION, szVersionText);
		wsprintf(szBuildDateText,GetBuildDateString());
		SetDlgItemText(IDC_BUILDDATE, szBuildDateText);

		//Hyperlink stuff
		{
			HDC hDC;
			UINT nHeight;
			//Create a font for later use with hyperlink
			hDC = GetDC();
			nHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);	//This line comes from the Win32API help for specifying point size!
			hFont = CreateFont(nHeight, 0, 0, 0, FW_NORMAL, 0, TRUE, 0, ANSI_CHARSET, 0, 0, 0, FF_DONTCARE, "Tahoma");
			ReleaseDC(hDC);
			/*Create new brush of color of the background for later use*/
			hLinkBrush = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
			//Set Cursor for Static Email Link to our Hyperlink Cursor
			// in PlatformSDK\Include\WinUser.h there is a define IDC_HAND,
			// so the mouse cursor will turn to a pointing finger over the link.
			// However this won't compile on certain OS's, but MAKEINTRESOURCE(32649)
			// seems to work fine, so we just use this one instead...
			SetClassLong(GetDlgItem(IDC_HOMEPAGELINK),GCL_HCURSOR, (long)LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		}
		//MUST RETURN 1
		return 1;
	}

	/*OK CLICKED*/
	LRESULT OnOK(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDOK);
		return 0;
	}

	/*CANCEL CLICKED*/
	LRESULT OnCancel(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}

	/*HOMEPAGELINK CLICKED*/
	LRESULT OnHomePageLink(WORD, UINT, HWND, BOOL&) {
		ShellExecute(GetActiveWindow(),"open","http://www.pinmame.com",NULL,NULL,SW_SHOW);
		return 0;
	}
	
	/*Dialog Destroy*/
	LRESULT OnDestroyDialog(UINT, WPARAM, LPARAM, BOOL&) {
		DeleteObject(hLinkBrush);
		DeleteObject(hFont);
		return 0;
	}

	/*Coloring*/
	LRESULT OnColorStaticDialog(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
		int nID = (int)::GetDlgCtrlID((HWND)lParam);
		if (nID == IDC_HOMEPAGELINK) {
			/*Change the color of the text to blue*/
			SetTextColor((HDC)wParam, RGB(0,0,255));
			SetBkMode((HDC)wParam, TRANSPARENT);
			SelectObject((HDC)wParam, hFont);
			return (LRESULT) hLinkBrush;
		}
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