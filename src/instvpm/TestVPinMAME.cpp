#include "Windows.h"
#include "Resource.h"
#include "Globals.h"

#include "Utils.h"
#include "TestVPinMAME.h"

#import "..\win32com\autogen\VPinMAME.tlb" no_namespace raw_interfaces_only

void EnableButtons(HWND hWnd, IController *pController);

/* the event sink for controller events; uhh, I love COM */
class CControllerEvents : public _IControllerEvents
{
public:
	// IUnknown
	STDMETHODIMP_(ULONG) AddRef() { 
		return ++m_uRef; 
	}

	STDMETHODIMP_(ULONG) Release() {
		if ( --m_uRef!=0 ) return m_uRef;
		delete this;
		return S_OK;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if ( riid==IID_IUnknown )
			*ppv = static_cast<_IControllerEvents*>(this);
		else if (riid==IID_IDispatch )
			*ppv = static_cast<_IControllerEvents*>(this);
		else if (riid==__uuidof(_IControllerEvents) )
			*ppv = static_cast<_IControllerEvents*>(this);
		else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	// IDispatch
	STDMETHODIMP_(long) GetTypeInfoCount(UINT *pCountTypeInfo) {
		*pCountTypeInfo = 0;
		return S_OK; 
	}

	STDMETHODIMP_(long) GetTypeInfo(UINT iInfo, LCID lcid, ITypeInfo **ppTInfo) {
		return E_NOTIMPL; 
	}

	STDMETHODIMP_(long) GetIDsOfNames(REFIID riid, BSTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) { 
		return E_NOTIMPL; 
	}
	
	STDMETHODIMP_(long) Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) { 
		switch ( dispIdMember ) {
		case 1: // OnSolenoid(int nSolenoid, int IsActive)
			if ( pDispParams->cArgs!=2 )
				return S_FALSE;

			return OnSolenoid(pDispParams->rgvarg[0].intVal, pDispParams->rgvarg[1].intVal);
		
		case 2:
			if ( pDispParams->cArgs!=1 )
				return S_FALSE;

			return OnStateChange(pDispParams->rgvarg[0].intVal);
		
		}
		return S_FALSE; 
	};

private:
	ULONG m_uRef;
	HWND  m_hWnd;	
	IController* m_pController;

private:
	STDMETHODIMP_(long) OnSolenoid(int nSolenoid, int IsActive) { return S_OK; };
	
	STDMETHODIMP_(long) OnStateChange(int nState) { 
		EnableButtons(m_hWnd, m_pController);
		return S_OK; 
	};

public:
	CControllerEvents(HWND hWnd, IController* pController) : m_uRef(0) { m_hWnd = hWnd; m_pController = pController; };
	~CControllerEvents() {};
};

typedef const char * (ZLIBVERSION)(void);

int CheckzLib(HWND hWnd)
{
	// Let's check if the zlib.dll is installed
	HMODULE hModule = LoadLibrary("zlib.dll");
	if ( !hModule ) {
		DisplayError(hWnd, GetLastError(), "The 'zlib.dll' file can't be loaded.", "Be sure to have this file either in the system directory or in the directory where the vpinmame.dll is located!");
		return 0;
	}

	ZLIBVERSION *pzlibVersion = (ZLIBVERSION*) GetProcAddress(hModule, "zlibVersion");
	if ( !pzlibVersion ) {
		FreeLibrary(hModule);
		DisplayError(hWnd, GetLastError(), "The installed 'zlib.dll' is to old or corrupt.", "Be sure to have an actual version of this file.");
		return 0;
	}

	char szzlibVersion[256];
	lstrcpy(szzlibVersion, pzlibVersion());

	if (strcmp(szzlibVersion, "1.1.3")<0) {
		FreeLibrary(hModule);
		
		char szErrorMsg[256];
		wsprintf(szErrorMsg, "The actual installed version (%s) is to old for using it with VPinMAME. Please use at least version 1.1.3", szzlibVersion);
		MessageBox(hWnd, szErrorMsg, g_szCaption, MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	FreeLibrary(hModule);
	return 1;
}

const char* GetInstalledVersion(char* szVersion, int iSize)
{
	if ( !szVersion )
		return NULL;

	lstrcpy(szVersion, "None.");

	HRESULT hr;

	CLSID ClsID;
	hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);
	if ( FAILED(hr) )
		return szVersion;

	OLECHAR sClsID[256];
	StringFromGUID2(ClsID, (LPOLESTR) sClsID, 256);

	char szClsID[256];
	WideCharToMultiByte(CP_ACP, 0, (LPOLESTR) sClsID, -1, szClsID, sizeof szClsID, NULL, NULL);

	char szRegKey[256];
	lstrcpy(szRegKey, "CLSID\\");
	lstrcat(szRegKey, szClsID);
	lstrcat(szRegKey, "\\InProcServer32");

	char szFilePathName[MAX_PATH];
	LONG dwSize = sizeof szFilePathName;
	if ( RegQueryValue(HKEY_CLASSES_ROOT, szRegKey, szFilePathName, &dwSize)!=ERROR_SUCCESS )
		return szVersion;

	if ( !GetVersionResourceEntry(szFilePathName, TEXT("ProductVersion"), szVersion, iSize) )
		return szVersion;

	return szVersion;
}


typedef struct tagGAMEINFO {
	char	szGameName[64];
	BOOL	fROMOk; // this is not used at the moment
} GAMEINFO, *PGAMEINFO;

void DeleteListContent(HWND hWnd)
{
	HWND hGamesList = GetDlgItem(hWnd, IDC_GAMESLIST);

	int nCount = SendMessage(hGamesList, LB_GETCOUNT, 0, 0);
	for(int i=0; i<nCount; i++) {
		PGAMEINFO pGameInfo = (PGAMEINFO) SendMessage(hGamesList, LB_GETITEMDATA, i, 0);
		delete pGameInfo;
	}

	SendMessage(hGamesList, LB_RESETCONTENT, 0, 0);
}

/***********************************************************/
/* Pulls all Machine Names and populates to Control Passed */
/***********************************************************/
BOOL PopulateList(HWND hWnd, IController *pController)
{
	DeleteListContent(hWnd);

	HWND hGamesList = GetDlgItem(hWnd, IDC_GAMESLIST);

	VARIANT varArrayNames;
	if ( FAILED(pController->get_Machines(NULL,&varArrayNames)) )
		return FALSE;

	/* PROCESS THE MACHINE NAMES ARRAY AND POPULATE LISTBOX */
	LONG lstart, lend;
    VARIANT HUGEP *pbstr;

	SAFEARRAY *psa = varArrayNames.parray;
	if ( SUCCEEDED(SafeArrayAccessData(psa, (void HUGEP**) &pbstr)) ) {
		// Get the lower and upper bound
		if (FAILED(SafeArrayGetLBound(psa, 1, &lstart))) 
			lstart=0;
		if (FAILED(SafeArrayGetUBound(psa, 1, &lend))) 
			lend=0;
		
		// Grab each machine name and populate the control
		for(long idx=lstart; idx <= lend; idx++)
		{		
			BSTR sGameName;
			char szGameName[256];
			char szListEntry[256];
			sGameName = pbstr[idx].bstrVal;
			if ( sGameName ) {
				WideCharToMultiByte(CP_ACP, 0, sGameName, -1, szGameName, sizeof szGameName, NULL, NULL);
				
				lstrcpy(szListEntry, szGameName); // for future versions here we can add the full game name 
				int nIndex = SendMessage(hGamesList, LB_ADDSTRING, 0, (LPARAM) szListEntry);
				
				PGAMEINFO pGameInfo = new GAMEINFO;
				lstrcpy(pGameInfo->szGameName, szGameName);
				pGameInfo->fROMOk = true;
				SendMessage(hGamesList, LB_SETITEMDATA, nIndex, (WPARAM) pGameInfo);
			}
		}
	}
	SafeArrayUnaccessData(psa);
	SafeArrayDestroy(psa);

	SendMessage(hGamesList, LB_SETCURSEL, 0, 0);
	return TRUE;
}

/***************************************************************************************/
/* Starts a game /*
/***************************************************************************************/
void RunGame(HWND hWnd, IController *pController)
{   
	int iIndex = SendDlgItemMessage(hWnd,IDC_GAMESLIST, LB_GETCURSEL, 0, 0);

	if ( iIndex<0 )
		return;

	pController->Stop();

	PGAMEINFO pGameInfo = (PGAMEINFO) SendDlgItemMessage(hWnd, IDC_GAMESLIST, LB_GETITEMDATA, iIndex, 0);

	BSTR sGameName;
	sGameName = SysAllocStringLen(NULL, strlen(pGameInfo->szGameName));

	MultiByteToWideChar(CP_ACP, 0,pGameInfo->szGameName, -1, sGameName, strlen(pGameInfo->szGameName)); 
	pController->put_GameName(sGameName);
	SysFreeString(sGameName);

	pController->put_HandleKeyboard(true);

	if ( FAILED(pController->Run(0,0)) ) 
		DisplayCOMError(pController, __uuidof(IController));

	SysFreeString(sGameName);
}

/***************************************************************************************/
/* Displays the game options /*
/***************************************************************************************/
void GameOptions(HWND hWnd, IController *pController)
{   
	int iIndex = SendDlgItemMessage(hWnd,IDC_GAMESLIST, LB_GETCURSEL, 0, 0);

	if ( iIndex<0 )
		return;

	PGAMEINFO pGameInfo = (PGAMEINFO) SendDlgItemMessage(hWnd, IDC_GAMESLIST, LB_GETITEMDATA, iIndex, 0);

	BSTR sGameName;
	sGameName = SysAllocStringLen(NULL, strlen(pGameInfo->szGameName));

	MultiByteToWideChar(CP_ACP, 0,pGameInfo->szGameName, -1, sGameName, strlen(pGameInfo->szGameName)); 
	pController->put_GameName(sGameName);
	SysFreeString(sGameName);

	if ( FAILED(pController->ShowOptsDialog((long) hWnd)) ) 
		DisplayCOMError(pController, __uuidof(IController));

	SysFreeString(sGameName);
}

void EnableButtons(HWND hWnd, IController *pController) {
	long fRunning;
	pController->get_Running(&fRunning);
	fRunning = fRunning==0?1:0;

	char szState[256];
	if ( fRunning ) {
		char szGameName[256];
		BSTR sGameName;
		pController->get_GameName(&sGameName);
		WideCharToMultiByte(CP_ACP, 0, (LPOLESTR) sGameName, -1, szGameName, sizeof szGameName, NULL, NULL);
		wsprintf(szState, "'%s' is running", szGameName);
	}
	else
		lstrcpy(szState, "No game is running.");

	SendDlgItemMessage(hWnd, IDC_STATE, WM_SETTEXT, 0, (WPARAM) &szState);

	EnableWindow(GetDlgItem(hWnd, IDC_START), SendDlgItemMessage(hWnd, IDC_GAMESLIST, LB_GETCURSEL, 0,0)>=0);
	EnableWindow(GetDlgItem(hWnd, IDC_STOP), fRunning);
	EnableWindow(GetDlgItem(hWnd, IDC_OPTIONS), SendDlgItemMessage(hWnd, IDC_GAMESLIST, LB_GETCURSEL, 0,0)>=0);
}

int PASCAL RunDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HICON m_hIcon = 0;
	static IController *pController = NULL;

	static IConnectionPointContainer *pControllerConnectionPointContainer = NULL;
	static IConnectionPoint *pControllerConnectionPoint = NULL;
	static CControllerEvents *pControllerEvents = NULL;
	static DWORD dwControllerCookie = 0;

	HRESULT hr;

	switch ( uMsg ) {
	case WM_INITDIALOG:
		/* check if we have the correct zlib.dll */
		if ( !CheckzLib(hWnd) ) {
			EndDialog(hWnd, IDCANCEL);
			return FALSE;
		}

		/* create the controller object */

		CLSID ClsID;
		hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Class couldn't be found. Maybe it isn't registered");
			EndDialog(hWnd, IDCANCEL);
			return FALSE;
		}

		hr = CoCreateInstance(ClsID, NULL, CLSCTX_INPROC_SERVER, __uuidof(IController), (void**) &pController);
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Can't create the Controller class! \nPlease check that you have installed Visual PinMAME properly!");
			EndDialog(hWnd, IDCANCEL);
			return FALSE;
		}

		// connect to the controller
		hr = pController->QueryInterface(IID_IConnectionPointContainer, (void**) &pControllerConnectionPointContainer);

		if ( SUCCEEDED(hr) )
			hr = pControllerConnectionPointContainer->FindConnectionPoint(__uuidof(_IControllerEvents), &pControllerConnectionPoint);

		if ( SUCCEEDED(hr) ) {
			pControllerEvents = new CControllerEvents(hWnd, pController);

			hr = pControllerConnectionPoint->Advise((IUnknown*) pControllerEvents, &dwControllerCookie);
			if ( FAILED(hr) ) {
				pControllerEvents = NULL;
				pControllerConnectionPoint->Release();
				pControllerConnectionPointContainer->Release();
			}
		}

		if ( !PopulateList(hWnd, pController) ) {
			DisplayError(hWnd, hr, "Retrieve the list of games. Please check your installation.");
			EndDialog(hWnd, IDCANCEL);
			return FALSE;
		}

		/* center the window */
		RECT WindowRect;
		GetWindowRect(hWnd, &WindowRect);

		POINT ScreenPos;
		ScreenPos.x = (GetSystemMetrics(SM_CXSCREEN)-(WindowRect.right-WindowRect.left)) / 2;
		ScreenPos.y = (GetSystemMetrics(SM_CYSCREEN)-(WindowRect.bottom-WindowRect.top)) / 2;
		
		MoveWindow(hWnd, ScreenPos.x, ScreenPos.y, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, false);

		/* set the icon */
		m_hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
		SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) m_hIcon);
		SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM) m_hIcon);

		char szVersion[256];
		GetInstalledVersion(szVersion, sizeof szVersion);
		SendDlgItemMessage(hWnd, IDC_VERSION, WM_SETTEXT, 0, (WPARAM) &szVersion);

		EnableButtons(hWnd, pController);
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hWnd, LOWORD(wParam));
			break;

		case IDC_START:
			RunGame(hWnd, pController);
			break;

		case IDC_STOP:
			pController->Stop();
			break;

		case IDC_OPTIONS:
			GameOptions(hWnd, pController);
			break;

		case IDC_GAMESLIST:
			switch (HIWORD(wParam)) {
			case LBN_DBLCLK:
				RunGame(hWnd, pController);
				break;

			case LBN_SELCHANGE:
				EnableButtons(hWnd, pController);
				break;
			}
			break;
		}
		break;

	case WM_DESTROY:
		/* disconnect from the controller */
		if ( pControllerEvents ) {
			pControllerConnectionPoint->Unadvise(dwControllerCookie);
			pControllerConnectionPoint->Release();
			pControllerConnectionPointContainer->Release();
		}

		/* release the controller object */
		if ( pController ) {
			/* the destructor will call "stop", so just release the object */
			pController->Release();
			pController = NULL;
		}

		DeleteListContent(hWnd);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void RunTest(HWND hWnd)
{
	if ( !CheckzLib(hWnd) )
		return;

	if ( DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_TESTDLG), hWnd, RunDlgProc)==-1 ) {
		int iError = GetLastError();
	};
}