// InstallVPinMAME.cpp : Defines the entry point for the application.
//
#define _WIN32_DCOM

#include "Windows.h"
#include "Resource.h"
#include "Globals.h"

#include "Utils.h"
#include "TestVPinMAME.h"

int RegisterUnregisterVPinMAME(HWND hWnd, int fRegister)
{
	HRESULT hr;
	HMODULE hModule;

	// so now let's see if we can load vpinmame.dll
	hModule = LoadLibrary("vpinmame.dll");
	if ( !hModule ) {
		DisplayError(hWnd, GetLastError(), "The 'vpinmame.dll' file can't be loaded.", "Be sure to have this file in the current directory!");
		return 0;
	}

	char szModuleFilename[MAX_PATH];
	GetModuleFileName(hModule, szModuleFilename, sizeof szModuleFilename);

	char szVersion[256];
	GetVersionResourceEntry(szModuleFilename, TEXT("ProductVersion"), szVersion, sizeof szVersion);

	char szMsg[256];
	if ( fRegister )
		wsprintf(szMsg, "You have selected to install version %s of Visual PinMAME! \nAre you ready to proceed?", szVersion);
	else
		wsprintf(szMsg, "You have selected to uninstall version %s of Visual PinMAME! \nAre you ready to proceed?", szVersion);	

	if ( MessageBox(hWnd, szMsg, g_szCaption, MB_ICONQUESTION|MB_YESNO)==IDNO ) {
		if ( fRegister )
			MessageBox(hWnd, "Installation aborted!", g_szCaption, MB_ICONEXCLAMATION|MB_OK);
		else
			MessageBox(hWnd, "UnInstallation aborted!\nVisual PinMAME is still installed on your system", g_szCaption, MB_ICONEXCLAMATION|MB_OK);
		FreeLibrary(hModule);
		return 0;
	}

	if ( fRegister ) {
		typedef HRESULT DLLREGISTERSERVER(void);

		DLLREGISTERSERVER* DllRegisterServer = (DLLREGISTERSERVER*) GetProcAddress(hModule, "DllRegisterServer");
		if ( !DllRegisterServer ) {
			DisplayError(hWnd, GetLastError(), "Can't find the registering function (DllRegisterServer) in the vpinmame.dll.", "Please check if you have a valid vpinmame.dll!");
			FreeLibrary(hModule);
			return 0;
		}

		hr = (*DllRegisterServer)();
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Unable to register the class object!", "Please check if you have a valid vpinmame.dll!");
			FreeLibrary(hModule);
			return 0;
		}
		FreeLibrary(hModule);

		// load the class one time to check if it's working
		IUnknown *pUnknown;
		CLSID ClsID;

		hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Class couldn't be found. Registering failed");
			return 0;
		}

		hr = CoCreateInstance(ClsID, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &pUnknown);
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Can't create the Controller class");
			return 0;
		}
		
		pUnknown->Release();
	}
	else {
		typedef HRESULT DLLUNREGISTERSERVER(void);

		DLLUNREGISTERSERVER* DllUnregisterServer = (DLLUNREGISTERSERVER*) GetProcAddress(hModule, "DllUnregisterServer");
		if ( !DllUnregisterServer ) {
			DisplayError(hWnd, GetLastError(), "Can't find the unregistering function (DllUnegisterServer) in the vpinmame.dll.", "Please check if you have a valid vpinmame.dll!");
			FreeLibrary(hModule);
			return 0;
		}

		hr = (*DllUnregisterServer)();
		if ( FAILED(hr) ) {
			DisplayError(hWnd, hr, "Unable to unregister the class object!", "Please check if you have a valid vpinmame.dll!");
			FreeLibrary(hModule);
			return 0;
		}
		
		FreeLibrary(hModule);
	}

	if ( !fRegister ) {
		if ( MessageBox(hWnd, "Do you want also delete all registry entrys Visual PinMAME has made?", g_szCaption, MB_ICONQUESTION|MB_YESNO)==IDYES ) 
			RegDeleteKeyEx(HKEY_CURRENT_USER, "Software\\Freeware\\Visual PinMame");
	}

	return 1;
}

int DisplayDialogs(HWND hWnd, int nDialog)
{
	// Let's make a little bit COM
	HRESULT hr;
	IUnknown *pUnknown;
	CLSID ClsID;

	hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);
	if ( FAILED(hr) ) {
		DisplayError(hWnd, hr, "Class couldn't be found. Maybe it isn't registered");
		return 0;
	}

	hr = CoCreateInstance(ClsID, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**) &pUnknown);
	if ( FAILED(hr) ) {
		DisplayError(hWnd, hr, "Can't create the Controller class! \nPlease check that you have installed Visual PinMAME properly!");
		return 0;
	}

	// Don't want to include the header files for the class, so let's using IDispatch
	IDispatch *pDispatch;
	
	hr = pUnknown->QueryInterface(IID_IDispatch, (void**) &pDispatch);
	if ( FAILED(hr) ) {
		DisplayError(hWnd, hr, "Can't get the dispatch interface");
		pUnknown->Release();
		return 0;
	}

	OLECHAR *pszName[3] = {L"ShowPathesDialog", L"ShowOptsDialog", L"ShowAboutDialog"};
	DISPID dispid;

	hr = pDispatch->GetIDsOfNames(IID_NULL, &pszName[nDialog], 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if ( FAILED(hr) ) {
		DisplayError(hWnd, hr, "Can't get the dispatch interface");
		pDispatch->Release();
		pUnknown->Release();
		return 0;
	}
	
	VARIANT varParam;
	varParam.vt   = VT_I4;
	varParam.lVal = (long) hWnd;

	DISPPARAMS DispParams = {&varParam,NULL,1,0};
	EXCEPINFO ExcepInfo;
	hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &DispParams, NULL, &ExcepInfo, NULL);

	if ( FAILED(hr) ) {
		if ( hr==DISP_E_EXCEPTION )
			// actually we call the ONLY UNICODE function in Windows 9x
			MessageBoxW(0, ExcepInfo.bstrDescription, L"Error from the VPinMAME.Controller interface", MB_ICONERROR|MB_OK);
		else 
			DisplayError(hWnd, hr, "Can't get the dispatch interface");

		pDispatch->Release();
		pUnknown->Release();
		return 0;
	}

	pDispatch->Release();
	pUnknown->Release();

	return 1;
}

void EnabledButtons(HWND hWnd)
{
	HRESULT hr;
	CLSID ClsID;
	hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);

	EnableWindow(GetDlgItem(hWnd, IDC_UNREGISTER), !FAILED(hr));
	EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYPATHESDLG), !FAILED(hr));
	EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYOPTSDIALOG), !FAILED(hr));
	EnableWindow(GetDlgItem(hWnd, IDC_TESTDIALOG), !FAILED(hr));
	EnableWindow(GetDlgItem(hWnd, IDC_DISPLAYBAOUTDIALOG), !FAILED(hr));
}

void DisplayInstalledVersion(HWND hWnd)
{
	char szInstalledVersion[256];
	GetInstalledVersion(szInstalledVersion, sizeof szInstalledVersion);

	char szVersionText[256];
	if(strlen(szInstalledVersion) > 0)
		wsprintf(szVersionText, "* Visual PinMAME Version %s is currently installed on your computer *", szInstalledVersion);
	else
		wsprintf(szVersionText, "* Visual PinMAME is not currently installed on your computer *");
	SendMessage(GetDlgItem(hWnd, IDC_INSTALLEDVERSION), WM_SETTEXT, 0, (WPARAM) szVersionText);
}

int PASCAL MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HICON m_hIcon = 0;
	char szHelp[256];

	switch ( uMsg ) {
	case WM_INITDIALOG:
		RECT WindowRect;
		GetWindowRect(hWnd, &WindowRect);

		POINT ScreenPos;
		ScreenPos.x = (GetSystemMetrics(SM_CXSCREEN)-(WindowRect.right-WindowRect.left)) / 2;
		ScreenPos.y = (GetSystemMetrics(SM_CYSCREEN)-(WindowRect.bottom-WindowRect.top)) / 2;
		
		MoveWindow(hWnd, ScreenPos.x, ScreenPos.y, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, false);

		m_hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
		SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) m_hIcon);
		SendMessage(hWnd, WM_SETICON, FALSE, (LPARAM) m_hIcon);

		char szModuleFilename[MAX_PATH];
		GetModuleFileName(g_hInstance, szModuleFilename, sizeof szModuleFilename);

		char szVersion[256];
		GetVersionResourceEntry(szModuleFilename, TEXT("ProductVersion"), szVersion, sizeof szVersion);

		GetWindowText(hWnd, szHelp, sizeof szHelp);

		wsprintf(g_szCaption, szHelp, szVersion);
		SetWindowText(hWnd, g_szCaption);

		EnabledButtons(hWnd);

		DisplayInstalledVersion(hWnd);

		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;

		case IDC_REGISTER:
			if ( RegisterUnregisterVPinMAME(hWnd, true) )
				MessageBox(hWnd, "Visual PinMAME installed successfully!", g_szCaption, MB_ICONINFORMATION|MB_OK);
			DisplayInstalledVersion(hWnd);
			EnabledButtons(hWnd);
			break;

		case IDC_UNREGISTER:
			if ( RegisterUnregisterVPinMAME(hWnd, false) )
				MessageBox(hWnd, "Visual PinMAME uninstalled successfully! \nYou may now safely delete your Visual PinMAME Directory!", g_szCaption, MB_ICONINFORMATION|MB_OK);
			DisplayInstalledVersion(hWnd);
			EnabledButtons(hWnd);
			break;

		case IDC_DISPLAYPATHESDLG:
			DisplayDialogs(hWnd, 0);
			break;

		case IDC_DISPLAYOPTSDIALOG:
			DisplayDialogs(hWnd, 1);
			break;

		case IDC_TESTDIALOG:
			RunTest(hWnd);
			break;

		case IDC_DISPLAYBAOUTDIALOG:
			DisplayDialogs(hWnd, 2);
			break;
		}
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	g_hInstance = hInstance;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if ( FAILED(hr) ) {
		DisplayError(0, hr, "Failed to initialize the COM subsystem.", "Please check your system!");
		return 0;
	}

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDLG), 0, MainDlgProc);
	CoUninitialize();

	return 1;
}

