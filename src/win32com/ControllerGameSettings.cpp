// Settings.cpp : Implementation of CGameSettings
#include "StdAfx.h"
#include "VPinMAME_h.h"
#include "VPinMAMEAboutDlg.h"
#include "VPinMAMEConfig.h"
#include "ControllerGameSettings.h"

#include "ControllerRegkeys.h"
#include "ControllerGame.h"

#include <atlwin.h>

// from ControllerRun.cpp
extern BOOL IsEmulationRunning();

// we need this to adjust the game window if a game is running
#include "Controller.h"
extern "C" HWND win_video_window;

// declared in VPinMAMEConfig, enables,/disable script write access
extern int fAllowWriteAccess;

// color struct
struct RGB { int r; int g; int b; };

/////////////////////////////////////////////////////////////////////////////
// CGameSettingsDlg

class CGameSettingsDlg : public CDialogImpl<CGameSettingsDlg> {
public:
	BEGIN_MSG_MAP(CGameSettingsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtrlColorStatic)
                MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftButtonDown)
		COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_DOUBLESIZE, OnCheckBox)
		COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_PINDMD, OnCheckBox)
		COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_WINDMD, OnCheckBox)
                COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_IGNOREROMCRC, OnCheckBox)
                COMMAND_RANGE_HANDLER(IDC_USECHEAT, IDC_CABINETMODE, OnCheckBox)
                COMMAND_ID_HANDLER(IDC_DMD_COLORIZE, OnDmdColorize)
		COMMAND_CODE_RANGE_HANDLER(IDC_SAMPLERATE, IDC_DMDPERC0, EN_CHANGE, OnEditCtrlChanged)
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

        RGB m_dmd66, m_dmd33, m_dmd0; // colorized DMD values
        COLORREF m_acrCustClr[16];    // custom color list for color chooser dialog

	// helper functions
	void SetControlValues() {
		// Get a Settings object for the game specific settings
		IGameSettings *pGameSettings;
		m_pGame->get_Settings((IGameSettings**) &pGameSettings);

		VARIANT vValue;
		VariantInit(&vValue);

		pGameSettings->get_Value(CComBSTR("cheat"), &vValue);
		CheckDlgButton(IDC_USECHEAT, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("sound"), &vValue);
		CheckDlgButton(IDC_USESOUND, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("samples"), &vValue);
		CheckDlgButton(IDC_USESAMPLES, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_compact"), &vValue);
		CheckDlgButton(IDC_COMPACTSIZE, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_doublesize"), &vValue);
		CheckDlgButton(IDC_DOUBLESIZE, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("samplerate"), &vValue);
		SetDlgItemInt(IDC_SAMPLERATE, vValue.lVal, FALSE);
		VariantClear(&vValue);
		
		pGameSettings->get_Value(CComBSTR("dmd_antialias"), &vValue);
		SetDlgItemInt(IDC_ANTIALIAS, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("synclevel"), &vValue);
		SetDlgItemInt(IDC_SYNCLEVEL, vValue.lVal, TRUE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("showpindmd"), &vValue);
		CheckDlgButton(IDC_PINDMD, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("showwindmd"), &vValue);
		CheckDlgButton(IDC_WINDMD, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
		VariantClear(&vValue);

                pGameSettings->get_Value(CComBSTR("fastframes"), &vValue);
                SetDlgItemInt(IDC_FASTFRAMES, vValue.lVal, TRUE);
                VariantClear(&vValue);

                pGameSettings->get_Value(CComBSTR("ignore_rom_crc"), &vValue);
                CheckDlgButton(IDC_IGNOREROMCRC, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
                VariantClear(&vValue);

                pGameSettings->get_Value(CComBSTR("cabinet_mode"), &vValue);
                CheckDlgButton(IDC_CABINETMODE, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
                VariantClear(&vValue);

                // colorized dmd
                pGameSettings->get_Value(CComBSTR("dmd_colorize"), &vValue);
                CheckDlgButton(IDC_DMD_COLORIZE, (vValue.boolVal==VARIANT_TRUE)?BST_CHECKED:BST_UNCHECKED);
                VariantClear(&vValue);

                pGameSettings->get_Value(CComBSTR("dmd_red66"), &vValue);
                m_dmd66.r = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_green66"), &vValue);
                m_dmd66.g = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_blue66"), &vValue);
                m_dmd66.b = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_red33"), &vValue);
                m_dmd33.r = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_green33"), &vValue);
                m_dmd33.g = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_blue33"), &vValue);
                m_dmd33.b = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_red0"), &vValue);
                m_dmd0.r = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_green0"), &vValue);
                m_dmd0.g = vValue.lVal;
                pGameSettings->get_Value(CComBSTR("dmd_blue0"), &vValue);
                m_dmd0.b = vValue.lVal;

                show_hide_colorize_ctls();

		pGameSettings->get_Value(CComBSTR("dmd_red"), &vValue);
		SetDlgItemInt(IDC_DMDRED, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_green"), &vValue);
		SetDlgItemInt(IDC_DMDGREEN, vValue.lVal, FALSE);
		VariantClear(&vValue);
		
		pGameSettings->get_Value(CComBSTR("dmd_blue"), &vValue);
		SetDlgItemInt(IDC_DMDBLUE, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_perc66"), &vValue);
		SetDlgItemInt(IDC_DMDPERC66, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_perc33"), &vValue);
		SetDlgItemInt(IDC_DMDPERC33, vValue.lVal ,FALSE);
		VariantClear(&vValue);
		
		pGameSettings->get_Value(CComBSTR("dmd_perc0"), &vValue);
		SetDlgItemInt(IDC_DMDPERC0, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("dmd_opacity"), &vValue);
		SetDlgItemInt(IDC_DMDOPACITY, vValue.lVal, FALSE);
		VariantClear(&vValue);

		pGameSettings->get_Value(CComBSTR("resampling_quality"), &vValue);
		SetDlgItemInt(IDC_RESAMPLEQ, vValue.lVal, FALSE);
		VariantClear(&vValue);

#if defined(VPINMAME_ALTSOUND) || defined(VPINMAME_PINSOUND)
		pGameSettings->get_Value(CComBSTR("sound_mode"), &vValue);
		SetDlgItemInt(IDC_SOUNDMODE, vValue.lVal, FALSE);
		VariantClear(&vValue);
#endif

		pGameSettings->Release();
	}

	void GetControlValues() {
		// Get a Settings object for the game specific settings
		IGameSettings *pGameSettings;
		m_pGame->get_Settings((IGameSettings**) &pGameSettings);

		pGameSettings->put_Value(CComBSTR("cheat"), CComVariant((BOOL) IsDlgButtonChecked(IDC_USECHEAT)));
		pGameSettings->put_Value(CComBSTR("sound"), CComVariant((BOOL) IsDlgButtonChecked(IDC_USESOUND)));
		pGameSettings->put_Value(CComBSTR("samples"), CComVariant((BOOL) IsDlgButtonChecked(IDC_USESAMPLES)));
		pGameSettings->put_Value(CComBSTR("dmd_compact"), CComVariant((BOOL) IsDlgButtonChecked(IDC_COMPACTSIZE)));
		pGameSettings->put_Value(CComBSTR("dmd_doublesize"), CComVariant((BOOL) IsDlgButtonChecked(IDC_DOUBLESIZE)));

		pGameSettings->put_Value(CComBSTR("samplerate"), CComVariant((int) GetDlgItemInt(IDC_SAMPLERATE,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("dmd_antialias"), CComVariant((int) GetDlgItemInt(IDC_ANTIALIAS,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("synclevel"), CComVariant((int) GetDlgItemInt(IDC_SYNCLEVEL,NULL,TRUE)));

		pGameSettings->put_Value(CComBSTR("showpindmd"), CComVariant((BOOL) IsDlgButtonChecked(IDC_PINDMD)));
		pGameSettings->put_Value(CComBSTR("showwindmd"), CComVariant((BOOL) IsDlgButtonChecked(IDC_WINDMD)));

                pGameSettings->put_Value(CComBSTR("fastframes"), CComVariant((int) GetDlgItemInt(IDC_FASTFRAMES,NULL,TRUE)));
                pGameSettings->put_Value(CComBSTR("ignore_rom_crc"), CComVariant((BOOL) IsDlgButtonChecked(IDC_IGNOREROMCRC)));
                pGameSettings->put_Value(CComBSTR("cabinet_mode"), CComVariant((BOOL) IsDlgButtonChecked(IDC_CABINETMODE)));

		pGameSettings->put_Value(CComBSTR("dmd_red"), CComVariant((int) GetDlgItemInt(IDC_DMDRED,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("dmd_green"), CComVariant((int) GetDlgItemInt(IDC_DMDGREEN,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("dmd_blue"), CComVariant((int) GetDlgItemInt(IDC_DMDBLUE,NULL,TRUE)));

		pGameSettings->put_Value(CComBSTR("dmd_perc66"), CComVariant((int) GetDlgItemInt(IDC_DMDPERC66,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("dmd_perc33"), CComVariant((int) GetDlgItemInt(IDC_DMDPERC33,NULL,TRUE)));
		pGameSettings->put_Value(CComBSTR("dmd_perc0"), CComVariant((int) GetDlgItemInt(IDC_DMDPERC0,NULL,TRUE)));

		pGameSettings->put_Value(CComBSTR("dmd_colorize"), CComVariant((BOOL) IsDlgButtonChecked(IDC_DMD_COLORIZE)));
		pGameSettings->put_Value(CComBSTR("dmd_red66"), CComVariant(m_dmd66.r));
		pGameSettings->put_Value(CComBSTR("dmd_green66"), CComVariant(m_dmd66.g));
		pGameSettings->put_Value(CComBSTR("dmd_blue66"), CComVariant(m_dmd66.b));
		pGameSettings->put_Value(CComBSTR("dmd_red33"), CComVariant(m_dmd33.r));
		pGameSettings->put_Value(CComBSTR("dmd_green33"), CComVariant(m_dmd33.g));
		pGameSettings->put_Value(CComBSTR("dmd_blue33"), CComVariant(m_dmd33.b));
		pGameSettings->put_Value(CComBSTR("dmd_red0"), CComVariant(m_dmd0.r));
		pGameSettings->put_Value(CComBSTR("dmd_green0"), CComVariant(m_dmd0.g));
		pGameSettings->put_Value(CComBSTR("dmd_blue0"), CComVariant(m_dmd0.b));

		pGameSettings->put_Value(CComBSTR("dmd_opacity"), CComVariant((int) GetDlgItemInt(IDC_DMDOPACITY,NULL,TRUE)));

		pGameSettings->put_Value(CComBSTR("resampling_quality"), CComVariant((int)GetDlgItemInt(IDC_RESAMPLEQ, NULL, TRUE)));

#if defined(VPINMAME_ALTSOUND) || defined(VPINMAME_PINSOUND)
		pGameSettings->put_Value(CComBSTR("sound_mode"), CComVariant((int) GetDlgItemInt(IDC_SOUNDMODE,NULL,TRUE)));
#endif

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

                        if (IsDlgButtonChecked(IDC_DMD_COLORIZE)) {
                                /*Colorized mode - use the individual colors*/
                                switch (::GetDlgCtrlID((HWND) lParam)) {
                                case IDC_DMDSHOW1:
                                        GetDMD_RGB_Color(&r,&g,&b,&np2,&np3,&np4);
                                        break;

                                case IDC_DMDSHOW2:
                                        r = m_dmd66.r;
                                        g = m_dmd66.g;
                                        b = m_dmd66.b;
                                        break;

                                case IDC_DMDSHOW3:
                                        r = m_dmd33.r;
                                        g = m_dmd33.g;
                                        b = m_dmd33.b;
                                        break;

                                case IDC_DMDSHOW4:
                                        r = m_dmd0.r;
                                        g = m_dmd0.g;
                                        b = m_dmd0.b;
                                        break;
                                }
                        } else {
                                
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
			return (LRESULT) m_hBrushDMDColor;
		}
		return 0;
	}

        BOOL click_in_color(POINT pt, int id, RGB &color)
        {
                RECT rc;
                ::GetWindowRect(GetDlgItem(id), &rc);
                ::MapWindowPoints(m_hWnd, HWND_DESKTOP, &pt, 1);
                if (PtInRect(&rc, pt)) {
                        /* get the current color in the packed byte format */
                        COLORREF rgbCur = ((BYTE)color.b<<16) + ((BYTE)color.g<<8) + (BYTE)color.r;

                        /*initialize the color chooser dialog */
                        CHOOSECOLOR cc;
                        ZeroMemory(&cc, sizeof(cc));
                        cc.lStructSize = sizeof(cc);
                        cc.hwndOwner = m_hWnd;
                        cc.lpCustColors = (LPDWORD) m_acrCustClr;
                        cc.rgbResult = rgbCur;
                        cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                        /* launch the dialog */
                        if (ChooseColor(&cc)) {
                                /* save the new colors back to the caller's RGB struct */
                                color.r = (int)GetRValue(cc.rgbResult);
                                color.g = (int)GetGValue(cc.rgbResult);
                                color.b = (int)GetBValue(cc.rgbResult);

                                /* force refresh for new colors */
                                InvalidateRect(0, TRUE);

                                /* note the update */
                                SetChanged(true);
                        }
                        return true;
                }
                return false;
        }
        
        LRESULT OnLeftButtonDown(UINT, WPARAM, LPARAM lpar, BOOL&) {
                if (IsDlgButtonChecked(IDC_DMD_COLORIZE)) {
                        // check if the click is in a color patch text box
					POINT pt = { ((int)(short)LOWORD(lpar)), ((int)(short)HIWORD(lpar)) };
                        RGB dmd100;
                        int np2, np3, np4;
                        GetDMD_RGB_Color(&dmd100.r, &dmd100.g, &dmd100.b, &np2, &np3, &np4);
                        if (click_in_color(pt, IDC_DMDSHOW1, dmd100)) {
                                SetDlgItemInt(IDC_DMDRED, dmd100.r, FALSE);
                                SetDlgItemInt(IDC_DMDGREEN, dmd100.g, FALSE);
                                SetDlgItemInt(IDC_DMDBLUE, dmd100.b, FALSE);
                                return 0;
                        }
                        else if (click_in_color(pt, IDC_DMDSHOW2, m_dmd66)
                                 || click_in_color(pt, IDC_DMDSHOW3, m_dmd33)
                                 || click_in_color(pt, IDC_DMDSHOW4, m_dmd0))
                        {
                                return 0;
                        }
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
			if (m_szROM[0] == '\0')
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
		COLORREF rgbCurrent;
		/*Grab current color entries from Dialog Textboxes*/
		GetDMD_RGB_Color(&r,&g,&b,&p2,&p3,&p4);
		rgbCurrent = ((BYTE)b<<16) + ((BYTE)g<<8) + (BYTE)r;
			
		// Initialize CHOOSECOLOR 
		ZeroMemory(&cc, sizeof(CHOOSECOLOR));
		cc.lStructSize = sizeof(CHOOSECOLOR);
		cc.hwndOwner = m_hWnd;
                cc.lpCustColors = (LPDWORD) m_acrCustClr;
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

        void show_hide_colorize_ctls()
        {
                int shade_ctls[] = {
                        IDC_DMDRED,
                        IDC_DMDGREEN,
                        IDC_DMDBLUE,
                        IDGETCOLOR,
                        IDC_DMDPERC66,
                        IDC_DMDPERC33,
                        IDC_DMDPERC0,
                        IDC_DMD_STATIC1,
                        IDC_DMD_STATIC2,
                        IDC_DMD_STATIC3,
                        IDC_DMD_STATIC4,
                        IDC_DMD_STATIC5,
                        IDC_DMD_STATIC6,
                        IDC_DMD_STATIC7,
                        IDC_DMD_STATIC8,
                        0
                };
                int colorize_ctls[] = {
                        IDC_DMD_STATIC1A,
                        0
                };
                int clr = IsDlgButtonChecked(IDC_DMD_COLORIZE);
				int i;
                for (i = 0 ; shade_ctls[i] != 0 ; ++i)
                        ::ShowWindow(GetDlgItem(shade_ctls[i]), clr ? SW_HIDE : SW_SHOW);
                for (i = 0 ; colorize_ctls[i] != 0 ; ++i)
                        ::ShowWindow(GetDlgItem(colorize_ctls[i]), clr ? SW_SHOW : SW_HIDE);
        }

        LRESULT OnDmdColorize(WORD, UINT uCtrlId, HWND, BOOL&) {
                show_hide_colorize_ctls();
                InvalidateRect(NULL,TRUE); // to update the color patches
                SetChanged(true);
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

STDMETHODIMP CGameSettings::ShowSettingsDlg(LONG_PTR hParentWnd)
{
	switch ( hParentWnd ) {
	case 0:
		break;

	case 1:
		hParentWnd = (LONG_PTR) ::GetActiveWindow();
		if ( !hParentWnd )
			hParentWnd = (LONG_PTR) GetForegroundWindow();
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

	DeleteGameSettings(m_szROM);

	if ( !fGameWasNeverStarted )
		SetGameWasStarted(m_szROM);

	return S_OK;
}

STDMETHODIMP CGameSettings::get_Value(BSTR sName, VARIANT *pVal)
{
	char szName[4096];
	WideCharToMultiByte(CP_ACP, 0, sName, -1, szName, sizeof szName, NULL, NULL);

	return GetSetting(m_szROM, szName, pVal)?S_OK:S_FALSE;
}

extern "C" 
{
	CController*	m_pController;	
}

STDMETHODIMP CGameSettings::put_Value(BSTR sName, VARIANT newVal)
{
	char szName[4096];
	WideCharToMultiByte(CP_ACP, 0, sName, -1, szName, sizeof szName, NULL, NULL);

	HRESULT hr = PutSetting(m_szROM, szName, newVal)?S_OK:S_FALSE;
	if ( SUCCEEDED(hr) ) {
		if ( IsEmulationRunning() && SettingAffectsRunningGame(szName) ) {
			VariantChangeType(&newVal, &newVal, 0, VT_BSTR);

			char szValue[4096];
			WideCharToMultiByte(CP_ACP, 0, newVal.bstrVal, -1, szValue, sizeof szName, NULL, NULL);
			set_option(szName, szValue, 0);
			PostMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);
		}
	}

	return hr;
}

STDMETHODIMP CGameSettings::SetDisplayPosition(VARIANT newValX, VARIANT newValY, LONG_PTR hWnd)
{
	VariantChangeType(&newValX, &newValX, 0, VT_I4);
	VariantChangeType(&newValY, &newValY, 0, VT_I4);
	if ( IsWindow((HWND) hWnd) ) {
		RECT rect;
		GetClientRect((HWND) hWnd, &rect);
		::ClientToScreen((HWND) hWnd, (LPPOINT) &rect.left);
		newValX.lVal += rect.left;
		newValY.lVal += rect.top;
	}

	HRESULT hr = put_Value(CComBSTR("dmd_pos_x"), newValX);
	if ( SUCCEEDED(hr) )
		hr = put_Value(CComBSTR("dmd_pos_y"), newValY);

	return hr;
}
