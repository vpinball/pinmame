#include "stdafx.h"
#include "VPinMAME.h"
#include "VPinMAMEAboutDlg.h"
#include "VPinMAMEConfig.h"

#include "Controller.h"
#include "ControllerOptionsDlg.h"
#include "ControllerOptions.h"

#include <shlobj.h>

#include <atlwin.h>

// ATL doesn't define this
#define COMMAND_CODE_RANGE_HANDLER(idFirst, idLast, code, func) \
	if(uMsg == WM_COMMAND && code == HIWORD(wParam) && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

class COptionsDlg : public CDialogImpl<COptionsDlg> {
public:
	BEGIN_MSG_MAP(COptionsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtrlColorStatic)
		COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_SPEEDLIMIT, OnCheckBox)
		COMMAND_CODE_RANGE_HANDLER(IDC_SAMPLERATE, IDC_DMDPERC0, EN_CHANGE, OnEditCtrlChanged)
		COMMAND_CODE_RANGE_HANDLER(IDC_ANTIALIAS, IDC_ANTIALIAS, EN_CHANGE, OnEditCtrlChanged) 
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDABOUT, OnAbout)
		COMMAND_ID_HANDLER(IDRESETTODEFAULT, OnResetToDefault)
		COMMAND_ID_HANDLER(IDGETCOLOR, OnGetColor);
	END_MSG_MAP()

	enum { IDD = IDD_OPTIONSDLG };

private:
	Controller*			m_pController;
	PCONTROLLEROPTIONS	m_pControllerOptions;
	bool				m_fChanged;
	HBRUSH				m_hBrushDMDColor;

	// helper functions
	void SetControlValues() {
		/******************************************/
		/*POPULATE CONTROLS FROM OPTIONS IN MEMORY*/
		/******************************************/

		// CheckBox Values
		CheckDlgButton(IDC_USECHEAT,(m_pControllerOptions->fUseCheat)?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(IDC_USESOUND,(m_pControllerOptions->fUseSound)?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(IDC_USESAMPLES,(m_pControllerOptions->fUseSamples)?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(IDC_COMPACTSIZE,(m_pControllerOptions->fCompactSize)?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(IDC_DOUBLESIZE,(m_pControllerOptions->fDoubleSize)?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(IDC_SPEEDLIMIT,(m_pControllerOptions->fSpeedLimit)?BST_CHECKED:BST_UNCHECKED);
		
		// Numeric Values
		SetDlgItemInt(IDC_ANTIALIAS,m_pControllerOptions->nAntiAlias,FALSE);
		SetDlgItemInt(IDC_SAMPLERATE,m_pControllerOptions->nSampleRate,FALSE);

		SetDlgItemInt(IDC_DMDRED,m_pControllerOptions->nDMDRED,FALSE);
		SetDlgItemInt(IDC_DMDGREEN,m_pControllerOptions->nDMDGREEN,FALSE);
		SetDlgItemInt(IDC_DMDBLUE,m_pControllerOptions->nDMDBLUE,FALSE);
		SetDlgItemInt(IDC_DMDPERC66,m_pControllerOptions->nDMDPERC66,FALSE);
		SetDlgItemInt(IDC_DMDPERC33,m_pControllerOptions->nDMDPERC33,FALSE);
		SetDlgItemInt(IDC_DMDPERC0,m_pControllerOptions->nDMDPERC0,FALSE);
	}

	void GetControlValues() {
		m_pControllerOptions->fUseCheat			= IsDlgButtonChecked(IDC_USECHEAT);
		m_pControllerOptions->fUseSound			= IsDlgButtonChecked(IDC_USESOUND);
		m_pControllerOptions->fUseSamples		= IsDlgButtonChecked(IDC_USESAMPLES);
		m_pControllerOptions->fCompactSize		= IsDlgButtonChecked(IDC_COMPACTSIZE);
		m_pControllerOptions->fDoubleSize		= IsDlgButtonChecked(IDC_DOUBLESIZE);
		m_pControllerOptions->fSpeedLimit		= IsDlgButtonChecked(IDC_SPEEDLIMIT);

		//Integer Values
		m_pControllerOptions->nAntiAlias		= GetDlgItemInt(IDC_ANTIALIAS,NULL,TRUE);
		m_pControllerOptions->nSampleRate		= GetDlgItemInt(IDC_SAMPLERATE,NULL,TRUE);

		m_pControllerOptions->nDMDRED			= GetDlgItemInt(IDC_DMDRED,NULL,TRUE);
		m_pControllerOptions->nDMDGREEN			= GetDlgItemInt(IDC_DMDGREEN,NULL,TRUE);
		m_pControllerOptions->nDMDBLUE			= GetDlgItemInt(IDC_DMDBLUE,NULL,TRUE);
		m_pControllerOptions->nDMDPERC66		= GetDlgItemInt(IDC_DMDPERC66,NULL,TRUE);
		m_pControllerOptions->nDMDPERC33		= GetDlgItemInt(IDC_DMDPERC33,NULL,TRUE);
		m_pControllerOptions->nDMDPERC0			= GetDlgItemInt(IDC_DMDPERC0,NULL,TRUE);
	}

	/***********************************************************************/
	/*Grab & Return Numeric values from the DMD Color & Intensity Textboxes*/
	/***********************************************************************/
	void GetDMD_RGB_Color(int *r, int *g, int *b, int *p2, int *p3, int *p4)
	{
		UINT value;
		value = GetDlgItemInt(IDC_DMDRED, NULL, TRUE);	
		*r = (int)value;
		value = GetDlgItemInt(IDC_DMDGREEN, NULL, TRUE);	
		*g = (int)value;
		value = GetDlgItemInt(IDC_DMDBLUE, NULL, TRUE);	
		*b = (int)value;
		value = GetDlgItemInt(IDC_DMDPERC66, NULL, TRUE);	
		*p2 = (int)value;
		value = GetDlgItemInt(IDC_DMDPERC33, NULL, TRUE);	
		*p3 = (int)value;
		value = GetDlgItemInt(IDC_DMDPERC0, NULL, TRUE);	
		*p4 = (int)value;
	}

	void SetChanged(bool fChanged) {
		m_fChanged = fChanged;
	}

	// message handlers
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM lParam, BOOL&) {
		CenterWindow();
		m_pController = (Controller*) lParam;
		if ( !m_pController )
			return 1;

		// init pControllerOptions
		m_pControllerOptions = &m_pController->m_ControllerOptions;

		char szTitle[256];
		if ( m_pControllerOptions->szROMName[0] )
			wsprintf(szTitle, "%s (%s)", m_pControllerOptions->szDescription, m_pControllerOptions->szROMName);
		else
			lstrcpy(szTitle, "Default Options");

		SetWindowText(szTitle);

		m_hBrushDMDColor = NULL;

		/*Init Dialog*/
		SetControlValues();
		SetChanged(false);

		return 1;
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
		if ( m_hBrushDMDColor )
			DeleteObject(m_hBrushDMDColor);
		return 0;
	}

	LRESULT OnCtrlColorStatic(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
		if(	::GetDlgCtrlID((HWND) lParam) == IDC_DMDSHOW1 ||
			::GetDlgCtrlID((HWND) lParam) == IDC_DMDSHOW2 ||
			::GetDlgCtrlID((HWND) lParam) == IDC_DMDSHOW3 ||
			::GetDlgCtrlID((HWND) lParam) == IDC_DMDSHOW4) {
		
			int r, g, b, np2, np3, np4;
			float p2, p3, p4;
			COLORREF thecolor;
			/*Pull values from Textboxes*/
			GetDMD_RGB_Color(&r,&g,&b,&np2,&np3,&np4);
			p2=(float)np2;
			p3=(float)np3;
			p4=(float)np4;
			switch (::GetDlgCtrlID((HWND) lParam)) {
			case IDC_DMDSHOW1:
				/*Do nothing, color is fine as is!*/
				break;

			case IDC_DMDSHOW2:
				/*Adjust % from original*/
				r = (int)(r*(p2/100.00));
				g = (int)(g*(p2/100.00));
				b = (int)(b*(p2/100.00));
				break;
			
			case IDC_DMDSHOW3:
				/*Adjust % from original*/
				r = (int)(r*(p3/100.00));
				g = (int)(g*(p3/100.00));
				b = (int)(b*(p3/100.00));
				break;
			case IDC_DMDSHOW4:
				/*Adjust % from original*/
				r = (int)(r*(p4/100.00));
				g = (int)(g*(p4/100.00));
				b = (int)(b*(p4/100.00));
				break;
			}
			
			/*create a color ref: 0x00bbggrr*/
			thecolor = ((BYTE)b<<16) + ((BYTE)g<<8) + (BYTE)r;

			/*If brush was already created destroy it*/
			if (m_hBrushDMDColor)
				DeleteObject(m_hBrushDMDColor);
			/*Create new brush*/
			
			m_hBrushDMDColor = CreateSolidBrush(thecolor);

			/*Change background color of item*/
			SetBkColor((HDC) wParam, thecolor);
			return (int) m_hBrushDMDColor;
		}
		return 0;
	}

	// command handlers
	LRESULT OnCheckBox(WORD, UINT, HWND, BOOL&) {
		SetChanged(true);
		return 0;
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
			PutOptions(m_pControllerOptions, m_pControllerOptions->szROMName);

			// emulation is running?
			if ( WaitForSingleObject(m_pController->m_hEmuIsRunning, 0)==WAIT_OBJECT_0 )
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
		BOOL fGameWasNeverStarted = GameWasNeverStarted(m_pController->m_szROM);

		/* Delete Game Specific Options from Registry and Reload Defaults */
		DelOptions(m_pControllerOptions->szROMName);
		GetOptions(m_pControllerOptions, m_pController->m_szROM);
		if ( !fGameWasNeverStarted )
			SetGameWasStarted(m_pController->m_szROM);

		SetControlValues();
		SetChanged(false);

		/*Force refresh to show new colors on screen*/
		InvalidateRect(NULL,TRUE);
		return 0;
	}

	LRESULT OnGetColor(WORD, UINT uCtrlId, HWND, BOOL&) {
		int r, g, b, p2, p3, p4;
		CHOOSECOLOR cc;                 // common dialog box structure 
		static COLORREF acrCustClr[16]; // array of custom colors 
		COLORREF rgbCurrent;
		/*Grab current color entries from Dialog Textboxes*/
		GetDMD_RGB_Color(&r,&g,&b,&p2,&p3,&p4);
		rgbCurrent = ((BYTE)b<<16) + ((BYTE)g<<8) + (BYTE)r;
			
		// Initialize CHOOSECOLOR 
		ZeroMemory(&cc, sizeof(CHOOSECOLOR));
		cc.lStructSize = sizeof(CHOOSECOLOR);
		cc.hwndOwner = m_hWnd;
		cc.lpCustColors = (LPDWORD) acrCustClr;
		cc.rgbResult = rgbCurrent;
		cc.Flags = CC_FULLOPEN | CC_RGBINIT;

		/*Launch Color Dialog Box - Use selected color from Dialog Textboxes*/
		if ( ChooseColor(&cc) )  {
			/*Now save new values back to textboxes!*/
			r = (int)GetRValue(cc.rgbResult);
			g = (int)GetGValue(cc.rgbResult);
			b = (int)GetBValue(cc.rgbResult);
			SetDlgItemInt(IDC_DMDRED,  r, FALSE);    
			SetDlgItemInt(IDC_DMDGREEN,g, FALSE);    
			SetDlgItemInt(IDC_DMDBLUE, b, FALSE); 

			/*Force refresh to show new colors on screen*/
			InvalidateRect(0,TRUE);				
		}

		return 0;
	}
};

/***************************/
/*LAUNCH THE OPTIONS DIALOG*/
/***************************/
void ShowOptionsDlg(HWND hParentWnd, Controller* pController) {
	COptionsDlg OptionsDlg;
	OptionsDlg.DoModal(hParentWnd, (LPARAM) pController);
}

class CPathsDlg : public CDialogImpl<CPathsDlg> {
public:
	BEGIN_MSG_MAP(CPathsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_CODE_RANGE_HANDLER(IDC_ROMDIRS, IDC_IMGDIR, EN_CHANGE, OnEditCtrlChanged)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDABOUT, OnAbout)
		COMMAND_ID_HANDLER(IDRESETTODEFAULT, OnResetToDefault)
		COMMAND_RANGE_HANDLER(IDDIRBUTTONROM, IDDIRBUTTONIMG, OnBrowseForDir)
	END_MSG_MAP()

	enum { IDD = IDD_PATHSDLG };

private:
	Controller*			m_pController;
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
		set_option("snapshot_directory", szPath, 0);

		GetDlgItemText(IDC_IMGDIR, szPath, sizeof(szPath));
		set_option("samplepath", szPath, 0);
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
		m_pController = (Controller*) lParam;
		if ( !m_pController )
			return 1;

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
			if ( WaitForSingleObject(m_pController->m_hEmuIsRunning, 0)==WAIT_OBJECT_0 )
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

/*************************/
/*LAUNCH THE PATHS DIALOG*/
/*************************/
void ShowPathsDlg(HWND hParentWnd, Controller* pController) {
	CPathsDlg PathsDlg;
	PathsDlg.DoModal(hParentWnd, (LPARAM) pController);
}
