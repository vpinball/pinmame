// Settings.cpp : Implementation of CGameSettings
#include "stdafx.h"
#include "VPinMAME.h"
#include "VPinMAMEAboutDlg.h"
#include "ControllerGameSettings.h"

#include "ControllerRegKeys.h"
#include "ControllerGame.h"

#include <atlwin.h>

// from ControllerRun.cpp
extern BOOL IsEmulationRunning();

// we need this to adjust the game window if a game is running
#include "Controller.h"
extern "C" HWND win_video_window;

// declared in VPinMAMEConfig, enables,/disable script write access
extern int fAllowWriteAccess;

/////////////////////////////////////////////////////////////////////////////
// CGameSettingsDlg

class CGameSettingsDlg : public CDialogImpl<CGameSettingsDlg> {
public:
	BEGIN_MSG_MAP(CGameSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtrlColorStatic)
		COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_DOUBLESIZE, OnCheckBox)
		COMMAND_CODE_RANGE_HANDLER(IDC_SAMPLERATE, IDC_DMDPERC0, EN_CHANGE, OnEditCtrlChanged)
		COMMAND_CODE_RANGE_HANDLER(IDC_ANTIALIAS, IDC_ANTIALIAS, EN_CHANGE, OnEditCtrlChanged) 
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDABOUT, OnAbout)
		COMMAND_ID_HANDLER(IDRESETTODEFAULT, OnResetToDefault)
		COMMAND_ID_HANDLER(IDGETCOLOR, OnGetColor);
	END_MSG_MAP()

	enum { IDD = IDD_GAMESETTINGSDLG };

private:
	IGame*				m_pGame;
	char				m_szROM[256];
	bool				m_fChanged;
	HBRUSH				m_hBrushDMDColor;

	// helper functions
	void SetControlValues() {
		// Get a Settings object for the game specific settings
		IGameSettings *pGameSettings;
		m_pGame->get_Settings((IGameSettings**) &pGameSettings);

		// CheckBox Values
		BOOL fVal;

		pGameSettings->get_UseCheat(&fVal);
		CheckDlgButton(IDC_USECHEAT, (fVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);

		pGameSettings->get_UseSound(&fVal);
		CheckDlgButton(IDC_USESOUND, (fVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);

		pGameSettings->get_UseSamples(&fVal);
		CheckDlgButton(IDC_USESAMPLES, (fVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);

		pGameSettings->get_CompactSize(&fVal);
		CheckDlgButton(IDC_COMPACTSIZE, (fVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);

		pGameSettings->get_DoubleSize(&fVal);
		CheckDlgButton(IDC_DOUBLESIZE, (fVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		
		// Numeric Values
		long lVal;

		pGameSettings->get_AntiAlias(&lVal);
		SetDlgItemInt(IDC_ANTIALIAS, lVal, FALSE);

		pGameSettings->get_SampleRate(&lVal);
		SetDlgItemInt(IDC_SAMPLERATE, lVal, FALSE);

		pGameSettings->get_DisplayColorRed(&lVal);
		SetDlgItemInt(IDC_DMDRED, lVal, FALSE);

		pGameSettings->get_DisplayColorGreen(&lVal);
		SetDlgItemInt(IDC_DMDGREEN, lVal, FALSE);
		
		pGameSettings->get_DisplayColorBlue(&lVal);
		SetDlgItemInt(IDC_DMDBLUE, lVal, FALSE);

		pGameSettings->get_DisplayIntensity66(&lVal);
		SetDlgItemInt(IDC_DMDPERC66, lVal, FALSE);

		pGameSettings->get_DisplayIntensity33(&lVal);
		SetDlgItemInt(IDC_DMDPERC33, lVal ,FALSE);
		
		pGameSettings->get_DisplayIntensityOff(&lVal);
		SetDlgItemInt(IDC_DMDPERC0, lVal, FALSE);

		pGameSettings->Release();
	}

	void GetControlValues() {
		// Get a Settings object for the game specific settings
		IGameSettings *pGameSettings;
		m_pGame->get_Settings((IGameSettings**) &pGameSettings);

		pGameSettings->put_UseCheat(IsDlgButtonChecked(IDC_USECHEAT)?VARIANT_TRUE:VARIANT_FALSE);
		pGameSettings->put_UseSound(IsDlgButtonChecked(IDC_USESOUND)?VARIANT_TRUE:VARIANT_FALSE);
		pGameSettings->put_UseSamples(IsDlgButtonChecked(IDC_USESAMPLES)?VARIANT_TRUE:VARIANT_FALSE);
		
		pGameSettings->put_CompactSize(IsDlgButtonChecked(IDC_COMPACTSIZE)?VARIANT_TRUE:VARIANT_FALSE);
		pGameSettings->put_DoubleSize(IsDlgButtonChecked(IDC_DOUBLESIZE)?VARIANT_TRUE:VARIANT_FALSE);

		//Integer Values
		pGameSettings->put_AntiAlias(GetDlgItemInt(IDC_ANTIALIAS,NULL,TRUE));
		pGameSettings->put_SampleRate(GetDlgItemInt(IDC_SAMPLERATE,NULL,TRUE));

		pGameSettings->put_DisplayColorRed(GetDlgItemInt(IDC_DMDRED,NULL,TRUE));
		pGameSettings->put_DisplayColorGreen(GetDlgItemInt(IDC_DMDGREEN,NULL,TRUE));
		pGameSettings->put_DisplayColorBlue(GetDlgItemInt(IDC_DMDBLUE,NULL,TRUE));

		pGameSettings->put_DisplayIntensity66(GetDlgItemInt(IDC_DMDPERC66,NULL,TRUE));
		pGameSettings->put_DisplayIntensity33(GetDlgItemInt(IDC_DMDPERC33,NULL,TRUE));
		pGameSettings->put_DisplayIntensityOff(GetDlgItemInt(IDC_DMDPERC0,NULL,TRUE));

		pGameSettings->Release();
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
		m_pGame = (IGame*) lParam;
		if ( !m_pGame )
			return 1;

		char szTitle[256];

		CComBSTR sDescription;
		m_pGame->get_Description(&sDescription);
		char szDescription[256];
		WideCharToMultiByte(CP_ACP, 0, sDescription, -1, szDescription, sizeof szDescription, NULL, NULL);

		CComBSTR sROM;
		m_pGame->get_Name(&sROM);
		WideCharToMultiByte(CP_ACP, 0, sROM, -1, m_szROM, sizeof m_szROM, NULL, NULL);
		
		if ( m_szROM[0] )
			wsprintf(szTitle, "%s (%s)", szDescription, m_szROM);
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
		BOOL fGameWasNeverStarted = GameWasNeverStarted(m_szROM);

		/* Delete Game Specific Options from Registry and Reload Defaults */
		char szKey[MAX_PATH];
		lstrcpy(szKey, REG_BASEKEY);

		HKEY hKey;
		if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_WRITE, &hKey)==ERROR_SUCCESS ) {
			if ( !m_szROM )
				RegDeleteKey(hKey, REG_DEFAULT);
			else
				RegDeleteKey(hKey, m_szROM);
				
			RegCloseKey(hKey);
		}
		
		if ( !fGameWasNeverStarted )
			SetGameWasStarted(m_szROM);

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

/////////////////////////////////////////////////////////////////////////////
// CGameSettings

STDMETHODIMP CGameSettings::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IGameSettings
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CGameSettings::CGameSettings()
{
	strcpy(m_szRegKey, REG_BASEKEY);
	strcat(m_szRegKey, "\\");
	strcat(m_szRegKey, REG_DEFAULT);
	
	strcpy(m_szRegKeyDef, m_szRegKey);

	strcpy(m_szROM, "");
	m_pGame = NULL;
}

CGameSettings::~CGameSettings()
{
	if ( m_pGame )
		m_pGame->Release();
	m_pGame = NULL;
}

void CGameSettings::Init(IGame *pGame)
{
	if ( !pGame )
		return;

	m_pGame = pGame;
	m_pGame->AddRef();

	CComBSTR sROM;
	m_pGame->get_Name(&sROM);
	WideCharToMultiByte(CP_ACP, 0, sROM, -1, m_szROM, sizeof m_szROM, NULL, NULL);

	strcpy(m_szRegKey, REG_BASEKEY);
	strcat(m_szRegKey, "\\");
	if ( m_szROM[0] )
		strcat(m_szRegKey, m_szROM);
	else
		strcat(m_szRegKey, REG_DEFAULT);
}

#define GET_BOOL_VALUE(name,defval) \
{ \
	*pVal = defval?VARIANT_TRUE:VARIANT_FALSE; \
	DWORD dwValue; \
	DWORD dwType = REG_DWORD; \
	DWORD dwSize = sizeof dwValue; \
	HKEY hKey = 0; \
	long lResult = -1; \
	if ( (lResult=RegOpenKeyEx(HKEY_CURRENT_USER, m_szRegKey, 0, KEY_QUERY_VALUE, &hKey))==ERROR_SUCCESS ) { \
		lResult = RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE) &dwValue, &dwSize); \
		if ( lResult==ERROR_SUCCESS ) \
			*pVal = dwValue?VARIANT_TRUE:VARIANT_FALSE; \
		RegCloseKey(hKey); \
	} \
	if ( (lResult!=ERROR_SUCCESS) && lstrcmpi(m_szRegKey,m_szRegKeyDef) ) { \
		if ( RegOpenKeyEx(HKEY_CURRENT_USER, m_szRegKeyDef, 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS ) { \
			if ( RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE) &dwValue, &dwSize)==ERROR_SUCCESS ) \
				*pVal = dwValue?VARIANT_TRUE:VARIANT_FALSE; \
			RegCloseKey(hKey); \
		} \
	} \
}

#define SET_BOOL_VALUE(name) \
{ \
	if ( fAllowWriteAccess ) { \
		HKEY hKey; \
		DWORD dwDisposition; \
   		if ( RegCreateKeyEx(HKEY_CURRENT_USER, m_szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS ) \
			return S_FALSE; \
		DWORD dwValue = (newVal==VARIANT_TRUE)?1:0; \
		RegSetValueEx(hKey, name, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof dwValue); \
		RegCloseKey(hKey); \
	} \
}

#define GET_DW_VALUE(name,defval) \
{ \
	*pVal = defval; \
	DWORD dwValue; \
	DWORD dwType = REG_DWORD; \
	DWORD dwSize = sizeof dwValue; \
	HKEY hKey = 0; \
	long lResult = -1; \
	if ( (lResult=RegOpenKeyEx(HKEY_CURRENT_USER, m_szRegKey, 0, KEY_QUERY_VALUE, &hKey))==ERROR_SUCCESS ) { \
		lResult = RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE) &dwValue, &dwSize); \
		if ( lResult==ERROR_SUCCESS ) \
			*pVal = (long) dwValue; \
		RegCloseKey(hKey); \
	} \
	if ( (lResult!=ERROR_SUCCESS) && lstrcmpi(m_szRegKey,m_szRegKeyDef) ) { \
		if ( RegOpenKeyEx(HKEY_CURRENT_USER, m_szRegKeyDef, 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS ) { \
			if ( RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE) &dwValue, &dwSize)==ERROR_SUCCESS ) \
				*pVal = (long) dwValue; \
			RegCloseKey(hKey); \
		} \
	} \
}

#define SET_DW_VALUE(name) \
{ \
	if ( fAllowWriteAccess ) { \
		HKEY hKey; \
		DWORD dwDisposition; \
   		if ( RegCreateKeyEx(HKEY_CURRENT_USER, m_szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS ) \
			return S_FALSE; \
		DWORD dwValue = (DWORD) newVal; \
		RegSetValueEx(hKey, name, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof dwValue); \
		RegCloseKey(hKey); \
	} \
}


STDMETHODIMP CGameSettings::get_UseSamples(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWUSESAMPLES, REG_DWUSESAMPLESDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_UseSamples(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWUSESAMPLES);
	return S_OK;
}

STDMETHODIMP CGameSettings::get_UseCheat(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWUSECHEAT, REG_DWUSECHEATDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_UseCheat(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWUSECHEAT);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_UseSound(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWUSESOUND, REG_DWUSESOUNDDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_UseSound(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWUSESOUND);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_AntiAlias(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWANTIALIAS, REG_DWANTIALIASDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_AntiAlias(long newVal)
{
	SET_DW_VALUE(REG_DWANTIALIAS);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayIntensity66(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDPERC66, REG_DWDMDPERC66DEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayIntensity66(long newVal)
{
	SET_DW_VALUE(REG_DWDMDPERC66);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayIntensity33(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDPERC33, REG_DWDMDPERC33DEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayIntensity33(long newVal)
{
	SET_DW_VALUE(REG_DWDMDPERC33);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayIntensityOff(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDPERC0, REG_DWDMDPERC0DEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayIntensityOff(long newVal)
{
	SET_DW_VALUE(REG_DWDMDPERC0);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayColorRed(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDRED, REG_DWDMDREDDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayColorRed(long newVal)
{
	SET_DW_VALUE(REG_DWDMDRED);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayColorGreen(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDGREEN, REG_DWDMDGREENDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayColorGreen(long newVal)
{
	SET_DW_VALUE(REG_DWDMDGREEN);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayColorBlue(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWDMDBLUE, REG_DWDMDBLUEDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayColorBlue(long newVal)
{
	SET_DW_VALUE(REG_DWDMDBLUE);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_Title(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWTITLE, REG_DWTITLEDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_Title(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWTITLE);

	if ( IsWindow(win_video_window) )
		PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_Border(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWBORDER, REG_DWBORDERDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_Border(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWBORDER);

	if ( IsWindow(win_video_window) )
		PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayOnly(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWDISPLAYONLY, REG_DWDISPLAYONLYDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayOnly(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWDISPLAYONLY);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_CompactSize(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWCOMPACTSIZE, REG_DWCOMPACTSIZEDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_CompactSize(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWCOMPACTSIZE);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DoubleSize(BOOL *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_BOOL_VALUE(REG_DWDOUBLESIZE, REG_DWDOUBLESIZEDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DoubleSize(BOOL newVal)
{
	SET_BOOL_VALUE(REG_DWDOUBLESIZE);

	if ( IsWindow(win_video_window) )
		PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_SampleRate(long *pVal)
{
	if ( ! pVal )
		return S_FALSE;

	GET_DW_VALUE(REG_DWSAMPLERATE, REG_DWSAMPLERATEDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_SampleRate(long newVal)
{
	if ( newVal<0 )
		return S_FALSE;

	SET_DW_VALUE(REG_DWSAMPLERATE);

	return S_OK;
}

STDMETHODIMP CGameSettings::ShowSettingsDlg(long hParentWnd)
{
	switch ( hParentWnd ) {
	case 0:
		break;

	case 1:
		hParentWnd = (long) ::GetActiveWindow();
		if ( !hParentWnd )
			hParentWnd = (long) GetForegroundWindow();
		break;

	default:
		if ( !IsWindow((HWND) hParentWnd) )
			hParentWnd = 0;
	}

	CGameSettingsDlg GameSettingsDlg;
	GameSettingsDlg.DoModal((HWND) hParentWnd, (LPARAM) m_pGame);

	return S_OK;
}

STDMETHODIMP CGameSettings::Clear()
{
	BOOL fGameWasNeverStarted = GameWasNeverStarted(m_szROM);

	RegDeleteKey(HKEY_CURRENT_USER, m_szRegKey);

	if ( !fGameWasNeverStarted )
		SetGameWasStarted(m_szRegKey);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayPosX(long hParentWnd, long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	if ( IsWindow(win_video_window) ) {
		RECT Rect;
		GetWindowRect(win_video_window, &Rect);
		POINT point;
		point.x = Rect.top;
		point.y = Rect.left;
		ScreenToClient(win_video_window, &point);

		*pVal = point.x;
	}
	else
		GET_DW_VALUE(REG_DWWINDOWPOSX, REG_DWWINDOWPOSXDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayPosX(long hParentWnd, long newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	POINT Pos = {newVal,0};
	if ( IsWindow((HWND) hParentWnd) )
		ClientToScreen((HWND) hParentWnd, &Pos);
	newVal = Pos.x;

	SET_DW_VALUE(REG_DWWINDOWPOSX);

	if ( IsWindow(win_video_window) )
		PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_DisplayPosY(long hParentWnd, long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	if ( IsWindow(win_video_window) ) {
		RECT Rect;
		GetWindowRect(win_video_window, &Rect);
		POINT point;
		point.x = Rect.top;
		point.y = Rect.left;
		ScreenToClient(win_video_window, &point);

		*pVal = point.y;
	}
	else
		GET_DW_VALUE(REG_DWWINDOWPOSY, REG_DWWINDOWPOSYDEF);

	return S_OK;
}

STDMETHODIMP CGameSettings::put_DisplayPosY(long hParentWnd, long newVal)
{
	if ( !fAllowWriteAccess )
		return S_OK;

	POINT Pos = {0,newVal};
	if ( IsWindow((HWND) hParentWnd) )
		ClientToScreen((HWND) hParentWnd, &Pos);
	newVal = Pos.y;

	SET_DW_VALUE(REG_DWWINDOWPOSY);

	if ( IsWindow(win_video_window) )
		PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);

	return S_OK;
}
