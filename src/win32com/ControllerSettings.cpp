// ControllerSettings.cpp : Implementation of CControllerSettings
#include "stdafx.h"
#include "VPinMAME.h"
#include "ControllerSettings.h"
#include "VPinMAMEConfig.h"
#include "VPinMAMEAboutDlg.h"

#include "ControllerRegKeys.h"

#include <atlwin.h>
#include <shlobj.h>

// from ControllerRun.cpp
extern BOOL IsEmulationRunning();

// from VPinMAMEConfig.c
extern int fAllowWriteAccess;

/////////////////////////////////////////////////////////////////////////////
// CControllerSettingsDlg

class CControllerSettingsDlg : public CDialogImpl<CControllerSettingsDlg> {
public:
	BEGIN_MSG_MAP(CControllerSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_RANGE_HANDLER(IDC_ROMDIRS, IDC_IMGDIR, EN_CHANGE, OnEditCtrlChanged)
		COMMAND_HANDLER(IDC_ALLOWWRITEACCESS, BN_CLICKED, OnEditCtrlChanged)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDABOUT, OnAbout)
		COMMAND_ID_HANDLER(IDRESETTODEFAULT, OnResetToDefault)
		COMMAND_RANGE_HANDLER(IDDIRBUTTONROM, IDDIRBUTTONIMG, OnBrowseForDir)
	END_MSG_MAP()

	enum { IDD = IDD_CONTROLLERSETTINGSDLG };

private:
	bool				m_fChanged;

	// helper functions
	void SetControlValues() {
		/******************************************/
		/*POPULATE CONTROLS FROM OPTIONS IN MEMORY*/
		/******************************************/

		// Path Values
		SetDlgItemText(IDC_ROMDIRS,    (char*) get_option("rompath"));
		SetDlgItemText(IDC_CFGDIR,	   (char*) get_option("cfg_directory"));
		SetDlgItemText(IDC_NVRAMDIR,   (char*) get_option("nvram_directory"));
		SetDlgItemText(IDC_SAMPLEDIRS, (char*) get_option("samplepath"));
		SetDlgItemText(IDC_IMGDIR,     (char*) get_option("snapshot_directory"));

		char szRegKey[256];
		lstrcpy(szRegKey, REG_BASEKEY);
		
		CheckDlgButton(IDC_ALLOWWRITEACCESS, ReadRegistry(szRegKey, REG_DWALLOWWRITEACCESS, REG_DWALLOWWRITEACCESSDEF));
	}

	void GetControlValues() {
		char szPath[4096];
		
		GetDlgItemText(IDC_ROMDIRS, szPath, sizeof(szPath));
		set_option("rompath", szPath, 0);

		GetDlgItemText(IDC_CFGDIR, szPath, sizeof(szPath));
		set_option("cfg_directory", szPath, 0);

		GetDlgItemText(IDC_NVRAMDIR, szPath, sizeof(szPath));
		set_option("nvram_directory", szPath, 0);

		GetDlgItemText(IDC_SAMPLEDIRS, szPath, sizeof(szPath));
		set_option("samplepath", szPath, 0);

		GetDlgItemText(IDC_IMGDIR, szPath, sizeof(szPath));
		set_option("snapshot_directory", szPath, 0);

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

		/*Init Dialog*/
		SetControlValues();
		SetChanged(false);

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

			// save them
			Save_fileio_opts();

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
		/* Delete Paths settings for the registry and reload them */
		Delete_fileio_opts();
		Load_fileio_opts();

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


STDMETHODIMP CControllerSettings::get_RomPath(BSTR *pVal)
{
	CComBSTR Val((char*) get_option("rompath"));
	*pVal = Val.Detach();

	return S_OK;
}

STDMETHODIMP CControllerSettings::put_RomPath(BSTR newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	char *pszNewVal = new char[lstrlenW(newVal)+1];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, pszNewVal, lstrlenW(newVal)+1, NULL, NULL);

	set_option("rompath", pszNewVal, 0);
	
	delete pszNewVal;
	Save_fileio_opts();

	return S_OK;
}

STDMETHODIMP CControllerSettings::get_NVRamPath(BSTR *pVal)
{
	CComBSTR Val((char*) get_option("nvram_directory"));
	*pVal = Val.Detach();
	return S_OK;
}

STDMETHODIMP CControllerSettings::put_NVRamPath(BSTR newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	char *pszNewVal = new char[lstrlenW(newVal)+1];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, pszNewVal, lstrlenW(newVal)+1, NULL, NULL);

	set_option("nvram_directory", pszNewVal, 0);
	Save_fileio_opts();

	return S_OK;
}

STDMETHODIMP CControllerSettings::get_SamplesPath(BSTR *pVal)
{
	CComBSTR Val((char*) get_option("samplepath"));
	*pVal = Val.Detach();
	return S_OK;
}

STDMETHODIMP CControllerSettings::put_SamplesPath(BSTR newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	char *pszNewVal = new char[lstrlenW(newVal)+1];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, pszNewVal, lstrlenW(newVal)+1, NULL, NULL);

	set_option("samplepath", pszNewVal, 0);
	Save_fileio_opts();

	return S_OK;
}

STDMETHODIMP CControllerSettings::get_CfgPath(BSTR *pVal)
{
	CComBSTR Val((char*) get_option("cfg_directory"));
	*pVal = Val.Detach();
	return S_OK;
}

STDMETHODIMP CControllerSettings::put_CfgPath(BSTR newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	char *pszNewVal = new char[lstrlenW(newVal)+1];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, pszNewVal, lstrlenW(newVal)+1, NULL, NULL);

	set_option("cfg_directory", pszNewVal, 0);
	Save_fileio_opts();

	return S_OK;
}

STDMETHODIMP CControllerSettings::get_SnapshotPath(BSTR *pVal)
{
	CComBSTR Val((char*) get_option("snapshot_directory"));
	*pVal = Val.Detach();

	return S_OK;
}

STDMETHODIMP CControllerSettings::put_SnapshotPath(BSTR newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	char *pszNewVal = new char[lstrlenW(newVal)+1];
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, pszNewVal, lstrlenW(newVal)+1, NULL, NULL);

	set_option("snapshot_directory", pszNewVal, 0);
	Save_fileio_opts();

	return S_OK;
}

STDMETHODIMP CControllerSettings::ShowSettingsDlg(long hParentWnd)
{
	CControllerSettingsDlg ControllerSettingsDlg;
	ControllerSettingsDlg.DoModal((HWND) hParentWnd);

	return S_OK;
}

STDMETHODIMP CControllerSettings::get_InstallDir(BSTR *pVal)
{
	char szInstallDir[MAX_PATH];
	GetInstallDir(szInstallDir, sizeof szInstallDir);

	CComBSTR Val(szInstallDir);

	*pVal = Val.Detach();
	return S_OK;
}
