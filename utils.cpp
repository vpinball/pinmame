/**********************************************
 * C++ Common Utility Routines                *
 *                                            *
 * by Steve Ellenoff (sellenoff@hotmail.com)  *
 * 05/26/2002					              *
 **********************************************/

#include "utils.h"

//////////////////////////////////////////////////////////////////////
// ShowError - Displays message of error from GetLastError()
//////////////////////////////////////////////////////////////////////
void ShowError() {
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Display the string.
	MessageBox( NULL, (LPTSTR)lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//////////////////////////////////////////////////////////////////////
// CenterWindow - Centers a Dialog in the user's desktop
//////////////////////////////////////////////////////////////////////
void CenterWindow(HWND hWnd)
{
		/*Center Dialog*/
		RECT WindowRect;
		POINT ScreenPos;
		GetWindowRect(hWnd, &WindowRect);
		ScreenPos.x = (GetSystemMetrics(SM_CXSCREEN)-(WindowRect.right-WindowRect.left)) / 2;
		ScreenPos.y = (GetSystemMetrics(SM_CYSCREEN)-(WindowRect.bottom-WindowRect.top)) / 2;
		MoveWindow(hWnd, ScreenPos.x, ScreenPos.y, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, false);
}

//////////////////////////////////////////////////////////////////////
// PrependToString - Adds PrependString to the begging of String passed
//////////////////////////////////////////////////////////////////////
void PrependToString(char *szString, char *szStringToPrepend)
{
	char szTemp[MAX_PATH];
	//Hold Current String
	strcpy(szTemp,szString);
	//Copy Prepend to Current
	strcpy(szString,szStringToPrepend);
	//Append Held Original String to End.
	strcat(szString,szTemp);
}

//////////////////////////////////////////////////////////////////////
// CreateToolTipCore - Helper function for creating tool tips
//////////////////////////////////////////////////////////////////////
void CreateToolTipCore(HINSTANCE hInst, HWND hwnd, LPCSTR szTipText, BOOL bBalloon, UINT uBalloonFlags)
{
                 
    INITCOMMONCONTROLSEX iccex; // struct specifying control classes to register
    HWND hwndTT;                // handle to the ToolTip control
    TOOLINFO ti;				// struct specifying info about tool in ToolTip control
    unsigned int uid = 0;       // for ti initialization
    RECT rect;                  // for client area coordinates
	UINT nFlags1, nFlags2;

    /* INITIALIZE COMMON CONTROLS */
    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&iccex);

	nFlags1 = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;
	nFlags2 = TTF_SUBCLASS;
	if(bBalloon) {
		nFlags1 |= TTS_BALLOON;
		nFlags2 |= uBalloonFlags;
	}
	
	/* CREATE A TOOLTIP WINDOW */
	hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		nFlags1,		
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hwnd,
		NULL,
		hInst,
		NULL
		);

	if(hwndTT) {
		SetWindowPos(hwndTT,
			HWND_TOPMOST,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		/* GET COORDINATES OF THE MAIN CLIENT AREA */
		GetClientRect (hwnd, &rect);

		/* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = nFlags2;
		ti.hwnd = hwnd;
		ti.hinst = hInst;
		ti.uId = uid;
		ti.lpszText = (LPSTR)szTipText; // ToolTip control will cover the whole window
		ti.rect.left = rect.left;    
		ti.rect.top = rect.top;
		ti.rect.right = rect.right;
		ti.rect.bottom = rect.bottom;

		/* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
		SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	}
}

//////////////////////////////////////////////////////////////////////
// GetDllVersion - Returns the Version of a DLL File
//////////////////////////////////////////////////////////////////////
DWORD GetDllVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

        /*Because some DLLs might not implement this function, you
        must test for it explicitly. Depending on the particular 
        DLL, the lack of a DllGetVersion function can be a useful
        indicator of the version.
        */
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

//////////////////////////////////////////////////////////////////////
// CreateToolTip - Create a normal tool tip
//////////////////////////////////////////////////////////////////////
void CreateToolTip (HINSTANCE hInst, HWND hwnd, LPCSTR szTipText) {
	CreateToolTipCore(hInst,hwnd,szTipText,FALSE,0);
}

//////////////////////////////////////////////////////////////////////
// CreateBalloonTip - Create a balloon tool tip
//
// If the required common controls are not present to support this,
// a normal tool tip is created instead
//////////////////////////////////////////////////////////////////////
void CreateBalloonTip(HINSTANCE hInst, HWND hwnd, LPCSTR szTipText, UINT uBalloonStyles) {
	//Make sure user has correct version of Common Controls Installed!
	if(GetDllVersion("comctl32.dll") >= PACKVERSION(5,80))
		CreateToolTipCore(hInst,hwnd,szTipText,TRUE,uBalloonStyles);
	//Create standard tool tip instead!
	else
		CreateToolTip(hInst, hwnd, szTipText);
}


//////////////////////////////////////////////////////////////////////
// DisplayTextFile - Uses the shell command to open a text file
//
// Thanks to the MAME32 guys for this one.
//////////////////////////////////////////////////////////////////////
void DisplayTextFile(HWND hWnd, LPCSTR szName)
{
    HINSTANCE hErr;
    char szMsg[MAX_PATH];

    hErr = ShellExecute(hWnd, NULL, szName, NULL, NULL, SW_SHOWNORMAL);
    if ((int)hErr > 32)
        return;

    switch((int)hErr)
    {
    case 0:
		wsprintf(szMsg,"The operating system is out of memory or resources!");
        break;

    case ERROR_FILE_NOT_FOUND:
		wsprintf(szMsg,"The specified file: %s was not found!",szName);
        break;

    case SE_ERR_NOASSOC :
		wsprintf(szMsg,"There is no application associated with the given filename extension for\nFile:%s",szName);
        break;

    case SE_ERR_OOM :
		wsprintf(szMsg,"There was not enough memory to complete the operation!");
        break;

    case SE_ERR_PNF :
		wsprintf(szMsg,"The specified path was not found!");
        break;

    case SE_ERR_SHARE :
		wsprintf(szMsg,"A sharing violation occurred!");
        break;

    default:
		wsprintf(szMsg,"An Unknown Error Occurred!");
    }
	MessageBox(NULL,szMsg,"Unable to Proceed!",MB_ICONSTOP);
}

/////////////////////////////////////////////////////////////////////////////////
// GetModifiedFileTime - Returns a FILETIME structure for a file's modified time!
/////////////////////////////////////////////////////////////////////////////////
FILETIME GetModifiedFileTime(LPCSTR szFile, BOOL bShowErrors)
{
	WIN32_FILE_ATTRIBUTE_DATA w32struc;
	memset(&w32struc,0,sizeof(w32struc));
	if(!GetFileAttributesEx(szFile,GetFileExInfoStandard,&w32struc))
		if(bShowErrors)
			ShowError();
	return w32struc.ftLastWriteTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FindInFile:
//
//Searches specified file for any match on the entire string passed
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FindInFile(LPCSTR szFile, LPCSTR szString) {
	FILE *fp;
	char c;
	char szMessage[MAX_PATH];
	const char *p = szString;
	UINT nSearchLen = Len(szString);
	UINT nCharMatch = 0;
	BOOL bResult = FALSE;

	/*Read in files*/
	if((fp=fopen(szFile,"r"))==NULL) {
		wsprintf(szMessage,"Unable to open file: %s!",szFile);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE;
	}
	while(!feof(fp)){
		c = getc(fp);
		if(*p++==c) {
			nCharMatch++;
			//Did we find a match on the entire string?
			if(nCharMatch == nSearchLen) {
				bResult = TRUE;
				break;
			}
		}
		else {
			p = szString;
			nCharMatch = 0;
		}
	}
	fclose(fp);
	return bResult;
}
