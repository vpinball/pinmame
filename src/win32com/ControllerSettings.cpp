// ControllerSettings.cpp : Implementation of CControllerSettings
#include "stdafx.h"
#include "VPinMAME_h.h"
#include "ControllerSettings.h"
#include "VPinMAMEConfig.h"
#include "VPinMAMEAboutDlg.h"

#include "ControllerRegKeys.h"

#include <atlwin.h>
#include <shlobj.h>

#include "DisplayInfoList.h"

// from ControllerRun.cpp
extern BOOL IsEmulationRunning();

// from VPinMAMEConfig.c
extern int fAllowWriteAccess;

// we need this to adjust the game window if a game is running
#include "Controller.h"
extern "C" HWND win_video_window;

/////////////////////////////////////////////////////////////////////////////
// CControllerSettingsDlg

class CControllerSettingsDlg : public CDialogImpl<CControllerSettingsDlg> {
public:
	BEGIN_MSG_MAP(CControllerSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_CODE_RANGE_HANDLER(IDC_ROMDIRS, IDC_IMGDIR, EN_CHANGE, OnEditCtrlChanged)
		COMMAND_HANDLER(IDC_ALLOWWRITEACCESS, BN_CLICKED, OnEditCtrlChanged)
		COMMAND_HANDLER(IDC_FULLSCREEN, BN_CLICKED, OnEditCtrlChanged)
		COMMAND_HANDLER(IDC_DISPLAYLIST, CBN_SELCHANGE, OnEditCtrlChanged)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDABOUT, OnAbout)
		COMMAND_ID_HANDLER(IDRESETTODEFAULT, OnResetToDefault)
		COMMAND_RANGE_HANDLER(IDDIRBUTTONROM, IDDIRBUTTONIMG, OnBrowseForDir)
	END_MSG_MAP()

	enum { IDD = IDD_CONTROLLERSETTINGSDLG };

private:
	bool			     m_fChanged;
	CControllerSettings* pControllerSettings;
	CWindow displayList;

private:
	// helper functions

	//============================================================
	//  ShowDisplays
	//============================================================
	// Uses the displays collection to fill in the IDC_DISPLAYLIST combo box.
	void ShowDisplays()
	{
		// Get the current display name (for selection)
		char* szDisplayName = (char*) get_option("screen");

		CDisplayInfoList displays = CDisplayInfoList();

		// Add each display to the combo box
		for (size_t i=0; i < displays.Count(); i++)
		{
			// Get the display at the index
			CDisplayInfo* display = displays.Item(i);

			// Add to the list
			// displayList.SendMessage(CB_ADDSTRING, 0, (LPARAM)display->GetDriverDescription());
			int index = displayList.SendMessage(CB_ADDSTRING, 0, (LPARAM) display->GetFriendlyName());

			char* szItemData = new char[256];
			if ( display->GetIsDefault() )
				lstrcpy(szItemData, "");
			else
				lstrcpy(szItemData, display->GetDriverName());
			displayList.SendMessage(CB_SETITEMDATA, (WPARAM) index, (LPARAM) szItemData);

			if ( lstrcmpi(szItemData, szDisplayName)==0 )
				displayList.SendMessage(CB_SETCURSEL, (WPARAM) index, (LPARAM) 0);

		}

		if ( displayList.SendMessage(CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0)<0 )
			displayList.SendMessage(CB_SETCURSEL, (WPARAM) 0, (LPARAM) 0);
	}

	void CleanupDisplayComboBox()
	{
		int count = displayList.SendMessage(CB_GETCOUNT, 0, 0);
		for(int i=0; i<count; i++) 
		{
			char* szItemData = (char*) displayList.SendMessage(CB_GETITEMDATA, (WPARAM) i, (LPARAM) NULL);	
			displayList.SendMessage(CB_SETITEMDATA, (WPARAM) i, (LPARAM) NULL);	

			delete szItemData;
		}
	}

	//============================================================
	//  SetControlValues
	//============================================================
	// Sets the values from the configuration data into the dialog controls
	void SetControlValues() {
		/******************************************/
		/*POPULATE CONTROLS FROM OPTIONS IN MEMORY*/
		/******************************************/

		VARIANT vValue;
		VariantInit(&vValue);

		// Path Values
		SetDlgItemText(IDC_ROMDIRS,    (char*) get_option("rompath"));
		SetDlgItemText(IDC_CFGDIR,	   (char*) get_option("cfg_directory"));
		SetDlgItemText(IDC_NVRAMDIR,   (char*) get_option("nvram_directory"));
		SetDlgItemText(IDC_SAMPLEDIRS, (char*) get_option("samplepath"));
		SetDlgItemText(IDC_IMGDIR,     (char*) get_option("snapshot_directory"));

		// Set check boxes
		pControllerSettings->get_Value(CComBSTR("window"), &vValue);
		CheckDlgButton(IDC_FULLSCREEN, (vValue.boolVal==VARIANT_TRUE)?BST_UNCHECKED:BST_CHECKED); // NOTE: Inverted
		VariantClear(&vValue);

		char szRegKey[256];
		lstrcpy(szRegKey, REG_BASEKEY);
		
		CheckDlgButton(IDC_ALLOWWRITEACCESS, ReadRegistry(szRegKey, REG_DWALLOWWRITEACCESS, REG_DWALLOWWRITEACCESSDEF));
	}

	//============================================================
	//  GetControlValues
	//============================================================
	// Gets the values from the dialog controls and stores them into the configuration data
	void GetControlValues() {
		char szPath[4096];

		GetDlgItemText(IDC_ROMDIRS, szPath, sizeof(szPath));
		pControllerSettings->put_Value(CComBSTR("rompath"), CComVariant(szPath));

		GetDlgItemText(IDC_CFGDIR, szPath, sizeof(szPath));
		pControllerSettings->put_Value(CComBSTR("cfg_directory"), CComVariant(szPath));

		GetDlgItemText(IDC_NVRAMDIR, szPath, sizeof(szPath));
		pControllerSettings->put_Value(CComBSTR("nvram_directory"), CComVariant(szPath));

		GetDlgItemText(IDC_SAMPLEDIRS, szPath, sizeof(szPath));
		pControllerSettings->put_Value(CComBSTR("samplepath"), CComVariant(szPath));

		GetDlgItemText(IDC_IMGDIR, szPath, sizeof(szPath));
		pControllerSettings->put_Value(CComBSTR("snapshot_directory"), CComVariant(szPath));

		int index = displayList.SendMessage(CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
		char* szItemData = (char*) displayList.SendMessage(CB_GETITEMDATA, (WPARAM) index, (LPARAM) NULL);
		pControllerSettings->put_Value(CComBSTR("screen"), CComVariant(szItemData));

		pControllerSettings->put_Value(CComBSTR("window"), CComVariant((BOOL) !IsDlgButtonChecked(IDC_FULLSCREEN))); // NOTE: Inverted

		fAllowWriteAccess = IsDlgButtonChecked(IDC_ALLOWWRITEACCESS);

		char szRegKey[256];
		lstrcpy(szRegKey, REG_BASEKEY);

		WriteRegistry(szRegKey, REG_DWALLOWWRITEACCESS, fAllowWriteAccess);
	}

	/***************************************************************/
	/*LAUNCH A GETDIRECTORY() WINDOW AND CAPTURE SELECTED DIRECTORY*/
	/***************************************************************/
	BOOL BrowseForDirectory(char* pResult) 
	{
		BOOL        bResult = FALSE;
		LPMALLOC	piMalloc;
		BROWSEINFO  Info;
		ITEMIDLIST* pItemIDList = NULL;
		char        buf[MAX_PATH];
    
		if (!SUCCEEDED(SHGetMalloc(&piMalloc)))
			return FALSE;

		Info.hwndOwner      = m_hWnd;
		Info.pidlRoot       = NULL;
		Info.pszDisplayName = buf;
		Info.lpszTitle      = (LPCSTR)"Directory name:";
		Info.ulFlags        = BIF_RETURNONLYFSDIRS;
		Info.lpfn           = NULL;
		Info.lParam         = (LPARAM) NULL;

		pItemIDList = SHBrowseForFolder(&Info);

		if (pItemIDList != NULL)
		{
			if (SHGetPathFromIDList(pItemIDList, buf) == TRUE)
			{
				strncpy(pResult, buf, MAX_PATH);
				bResult = TRUE;
			}
			piMalloc->Free(pItemIDList);
		}
		else
		{
			bResult = FALSE;
		}
		piMalloc->Release();
		return bResult;
	}

	void SetChanged(bool fChanged) {
		m_fChanged = fChanged;
	}

	// message handlers
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM lParam, BOOL&) {
		CenterWindow();

		pControllerSettings = (CControllerSettings*) lParam;

		// Get the display list control (combo box)
		displayList = CWindow(GetDlgItem(IDC_DISPLAYLIST));

		// Show displays
		ShowDisplays();

		/*Init Dialog*/
		SetControlValues();
		SetChanged(false);

		return 1;
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM lParam, BOOL&) {
		CleanupDisplayComboBox();

		return 1;
	}

	LRESULT OnEditCtrlChanged(WORD, UINT, HWND, BOOL&) {
		SetChanged(true);
		return 0;
	}

	LRESULT OnOK(WORD, UINT, HWND, BOOL&) {
		if ( m_fChanged ) {
			// get settings from the controls
			GetControlValues();

			// emulation is running?
			if ( IsEmulationRunning() )
				MessageBox("Your changes will not take effect until you restart Visual PinMAME!","Notice!",MB_OK | MB_ICONINFORMATION);
		}
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnCancel(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}

	LRESULT OnAbout(WORD, UINT, HWND, BOOL&) {
		ShowAboutDlg(m_hWnd);
		return 0;
	}

	LRESULT OnResetToDefault(WORD, UINT, HWND, BOOL&) {
		/* Delete controller (global) settings for the registry and reload them */
		DeleteGlobalSettings();
		LoadGlobalSettings();

		SetControlValues();
		SetChanged(false);

		return 0;
	}

	LRESULT OnBrowseForDir(WORD, UINT uCtrlId, HWND, BOOL&) {
		char szDir[MAX_PATH];
		if ( !BrowseForDirectory(szDir) )
			return 0;

		switch ( uCtrlId ) {
		case IDDIRBUTTONROM:
			SetDlgItemText(IDC_ROMDIRS, szDir);
			break;
		case IDDIRBUTTONCFG:
				SetDlgItemText(IDC_CFGDIR, szDir);
			break;
		case IDDIRBUTTONNVRAM:
				SetDlgItemText(IDC_NVRAMDIR, szDir);
			break;
		case IDDIRBUTTONSAMPLE:
				SetDlgItemText(IDC_SAMPLEDIRS, szDir);
			break;
		case IDDIRBUTTONIMG:
				SetDlgItemText(IDC_IMGDIR, szDir);
			break;
		}
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CControllerSettings

STDMETHODIMP CControllerSettings::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IControllerSettings
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CControllerSettings::CControllerSettings()
{
}

STDMETHODIMP CControllerSettings::ShowSettingsDlg(long hParentWnd)
{
	CControllerSettingsDlg ControllerSettingsDlg;
	ControllerSettingsDlg.DoModal((HWND) hParentWnd, (LPARAM) this);

	return S_OK;
}

STDMETHODIMP CControllerSettings::Clear()
{
	DeleteGlobalSettings();
	
	return S_OK;
}

STDMETHODIMP CControllerSettings::get_Value(BSTR sName, VARIANT *pVal)
{
	char szName[4096];
	WideCharToMultiByte(CP_ACP, 0, sName, -1, szName, sizeof szName, NULL, NULL);

	return GetSetting(NULL, szName, pVal)?S_OK:S_FALSE;
}

STDMETHODIMP CControllerSettings::put_Value(BSTR sName, VARIANT newVal)
{
	char szName[4096];
	WideCharToMultiByte(CP_ACP, 0, sName, -1, szName, sizeof szName, NULL, NULL);

	HRESULT hr = PutSetting(NULL, szName, newVal);
	if ( SUCCEEDED(hr) ) {
		VariantChangeType(&newVal, &newVal, 0, VT_BSTR);
		char szValue[4096];
		WideCharToMultiByte(CP_ACP, 0, newVal.bstrVal, -1, szValue, sizeof szName, NULL, NULL);
		set_option(szName, szValue, 0);

		if ( IsEmulationRunning() && SettingAffectsRunningGame(szName) ) 
			PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);
	}

	return hr;
}

STDMETHODIMP CControllerSettings::get_InstallDir(BSTR *pVal)
{
	char szInstallDir[MAX_PATH];
	GetInstallDir(szInstallDir, sizeof szInstallDir);

	*pVal = CComBSTR(szInstallDir).Detach();

	return S_OK;
}
