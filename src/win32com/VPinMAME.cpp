// VPinMAME.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for PinMAME.idl by adding the following
//      files to the Outputs.
//          PinMAME_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f PinMAMEps.mk in the project directory.
#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "VPinMAME_h.h"
#include "dlldatax.h"

#define NO_EXPIRATION_DATE 1
#define EXP_YEAR 2108
#define EXP_MONTH   8
#define EXP_DAY     1

#include "VPinMAME_i.c"
#include "Controller.h"
#include "WSHDlg.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Controller, CController)
OBJECT_ENTRY(CLSID_WSHDlg, CWSHDlg)
END_OBJECT_MAP()

BOOL IsSingleThreadedApartment()
{
	HRESULT hr;

	CLSID ClsID;
	hr = CLSIDFromProgID(OLESTR("VPinMAME.Controller"), &ClsID);
	if ( FAILED(hr) )
		return FALSE;

	OLECHAR sClsID[256];
	StringFromGUID2(ClsID, (LPOLESTR) sClsID, 256);

	char szClsID[256];
	WideCharToMultiByte(CP_ACP, 0, (LPOLESTR) sClsID, -1, szClsID, sizeof szClsID, NULL, NULL);

	char szRegKey[256];
	lstrcpy(szRegKey, "CLSID\\");
	lstrcat(szRegKey, szClsID);
	lstrcat(szRegKey, "\\InprocServer32");

	HKEY hKey;
	if ( RegOpenKey(HKEY_CLASSES_ROOT, szRegKey, &hKey)!=ERROR_SUCCESS )
		return FALSE;

	char szThreadingModel[MAX_PATH];
	ULONG uSize = sizeof szThreadingModel;
	DWORD dwType = REG_SZ;
	if ( RegQueryValueEx(hKey, "ThreadingModel", NULL, &dwType, (LPBYTE) &szThreadingModel, &uSize)!=ERROR_SUCCESS ) {
		RegCloseKey(hKey);

		// if we don't have that entry, return TRUE (old style, but single threaded)
		return TRUE;
	}
	RegCloseKey(hKey);

	// if we don't have that entry, return TRUE (old style, but single threaded)
	if ( !szThreadingModel[0] )
		return TRUE;

	return _strcmpi(szThreadingModel, "Apartment")?FALSE:TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_VPinMAMELib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	#ifndef NO_EXPIRATION_DATE

	// check if we are outdated
	// delete this for an official release
	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);
	if ((SystemTime.wYear * 500 + SystemTime.wMonth * 35 + SystemTime.wDay) >
		(EXP_YEAR         * 500 + EXP_MONTH         * 35 + EXP_DAY)) {
		char szTitle[256];
		LoadString(_Module.m_hInst, IDS_EXPIREDTITLE, szTitle, sizeof szTitle);
		char szText[256];
		LoadString(_Module.m_hInst, IDS_EXPIRED, szText, sizeof szText);
		MessageBox(0, szText, szTitle, MB_ICONINFORMATION|MB_OK);
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	#endif

/*
	if ( !IsSingleThreadedApartment() ) {
		MessageBox(0, "Wrong threading model, please reinstall VPinMAME!", "Unable to run", MB_ICONINFORMATION|MB_OK);
		return CLASS_E_CLASSNOTAVAILABLE;
	}
*/

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    return _Module.UnregisterServer(TRUE);
}



