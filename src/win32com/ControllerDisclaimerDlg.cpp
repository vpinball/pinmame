#include "StdAfx.h"
#include "VPinMAME_h.h"

#include "ControllerDisclaimerDlg.h"

#include "resource.h"

#include <atlwin.h>

class CDisclaimerDlg : public CDialogImpl<CDisclaimerDlg> {
public:
	BEGIN_MSG_MAP(CDisclaimerDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	enum { IDD = IDD_DISCLAIMERDLG };

private:
	Controller*	m_pController;

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM lParam, BOOL&) {
		CenterWindow();
		if ( !lParam )
			return 1;

		SetDlgItemText(IDC_DISGAMESPEC, (char*) lParam);
		return 1;
	}

	LRESULT OnOK(WORD, UINT, HWND, BOOL&) {
		EndDialog(IsDlgButtonChecked(IDC_YESIAM)?IDOK:IDCANCEL);
		return 0;
	}

	LRESULT OnCancel(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}
};

BOOL ShowDisclaimer(HWND hParentWnd, char* szDescription) { 
	CDisclaimerDlg DisclaimerDlg;
	return DisclaimerDlg.DoModal(hParentWnd, (LPARAM) szDescription)==IDOK;
}
