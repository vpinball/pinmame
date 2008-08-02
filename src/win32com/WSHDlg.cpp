// WSHDlg.cpp : Implementation of CWSHDlg
#include "Stdafx.h"
#include "VPinMAME_h.h"
#include "WSHDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CWSHDlg

CWSHDlg::CWSHDlg()
{
	m_x = -1;
	m_y = -1;
	m_w = 0;
	m_h = 0;
	strcpy(m_szTitle, "");

	HRESULT hr = CComObject<CWSHDlgCtrls>::CreateInstance(&m_pWSHDlgCtrls);
	if ( SUCCEEDED(hr) )
		m_pWSHDlgCtrls->AddRef();
}

CWSHDlg::~CWSHDlg()
{
	if ( m_pWSHDlgCtrls )
		m_pWSHDlgCtrls->Release();
}

STDMETHODIMP CWSHDlg::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IWSHDlg
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CWSHDlg::get_x(long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = m_x;

	return S_OK;
}

STDMETHODIMP CWSHDlg::put_x(long newVal)
{
	m_x = newVal;

	return S_OK;
}

STDMETHODIMP CWSHDlg::get_y(long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = m_y;

	return S_OK;
}

STDMETHODIMP CWSHDlg::put_y(long newVal)
{
	m_y = newVal;

	return S_OK;
}

STDMETHODIMP CWSHDlg::get_w(long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = m_w;

	return S_OK;
}

STDMETHODIMP CWSHDlg::put_w(long newVal)
{
	m_w = newVal;

	return S_OK;
}

STDMETHODIMP CWSHDlg::get_h(long *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = m_h;

	return S_OK;
}

STDMETHODIMP CWSHDlg::put_h(long newVal)
{
	m_h = newVal;

	return S_OK;
}

STDMETHODIMP CWSHDlg::get_Title(BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CComBSTR sTitle(m_szTitle);
	*pVal = sTitle.Detach();

	return S_OK;
}

STDMETHODIMP CWSHDlg::put_Title(BSTR newVal)
{
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, m_szTitle, sizeof m_szTitle, NULL, NULL);

	return S_OK;
}

STDMETHODIMP CWSHDlg::AddCtrl(BSTR sType, long x, long y, long w, long h, BSTR sTitle, IUnknown **pRetVal)
{
	if ( !pRetVal )
		return S_FALSE;
	*pRetVal = NULL;

	CComObject<CWSHDlgCtrl>*  pNewCtrl;

	HRESULT hr = CComObject<CWSHDlgCtrl>::CreateInstance(&pNewCtrl);
	if ( FAILED(hr) ) 
		return hr;

	hr = pNewCtrl->Init(sType, x, y, w, h, sTitle);
	if ( FAILED(hr) )
		return hr;

	m_pWSHDlgCtrls->add_Item(pNewCtrl);

	pNewCtrl->QueryInterface(IID_IUnknown, (void**) pRetVal);

	return S_OK;
}

STDMETHODIMP CWSHDlg::get_Ctrls(IWSHDlgCtrls **ppVal)
{
	if ( !ppVal )
		return S_FALSE;

	return m_pWSHDlgCtrls->QueryInterface(IID_IWSHDlgCtrls, (void**) ppVal);
}

int _stdcall WSHDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

STDMETHODIMP CWSHDlg::Show(long hParentWnd, VARIANT *RetVal)
{
	if ( !RetVal )
		return S_FALSE;

	#pragma pack(1)
	struct {
		DLGTEMPLATE DlgTemplate;
		WORD wMenu;
		WORD wClass;
		WORD wTitle;
		WORD wFont;
		WCHAR szFontName[32];
	} Template;
	#pragma pack()

	Template.DlgTemplate.style = DS_SETFONT | DS_ABSALIGN | DS_MODALFRAME | DS_SETFOREGROUND | DS_3DLOOK | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
	Template.DlgTemplate.dwExtendedStyle = 0;
	Template.DlgTemplate.cdit = 0;
	Template.DlgTemplate.x = (short) m_x;
	Template.DlgTemplate.y = (short) m_y;
	Template.DlgTemplate.cx = (short) m_w;
	Template.DlgTemplate.cy = (short) m_h;

	Template.wMenu = 0;
	Template.wClass = 0;
	Template.wTitle = 0;
	Template.wFont = 8;
	wcscpy(Template.szFontName, L"Helv");

	VariantInit(RetVal);
	RetVal->vt = VT_I4;
	RetVal->lVal = DialogBoxIndirectParam(_Module.m_hInst, &Template.DlgTemplate, (HWND) hParentWnd, WSHDlgProc, (LPARAM) this);
	DWORD dwLastError = GetLastError();

	return S_OK;
}

#define CTRLID_START 1000

void SaveDlgValues(HWND hDlg, CWSHDlgCtrls *pWSHDlgCtrls)
{
	for (int i=0; i<pWSHDlgCtrls->m_lCount; i++) {
		switch ( pWSHDlgCtrls->m_pCtrlList[i]->m_iType ) {
		case CTRLTYPE_CHECKBOX:
		case CTRLTYPE_RADIONBUTTON:
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.Clear();
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.vt = VT_I4;
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.lVal = IsDlgButtonChecked(hDlg, CTRLID_START+i);
			break;
		}
	}
}

int _stdcall WSHDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CWSHDlg		*pWSHDlg = NULL;
	static CWSHDlgCtrls *pWSHDlgCtrls = NULL;
	static int iIDOkButton		= -1;
	static int iIDCancelButton	= -1;

	long i;
	int  iPreviousType;
	RECT Rect;
	int  iBorderX, iBorderY;
	HFONT hFont;
	bool fPlaceStandardButtons;

	switch ( uMsg ) {
	case WM_INITDIALOG:
		if ( !lParam) {
			EndDialog(hDlg, IDCANCEL);
			return 0;
		}

		pWSHDlg = (CWSHDlg*) lParam;
		pWSHDlgCtrls = pWSHDlg->m_pWSHDlgCtrls;

		SetWindowText(hDlg, pWSHDlg->m_szTitle);

		hFont = (HFONT) SendMessage(hDlg, WM_GETFONT, 0, 0);

		SetRectEmpty(&Rect);
		Rect.right = 1;
		Rect.bottom = 1;
		iBorderX = 32767;
		iBorderY = 32767;

		iPreviousType = -1;
		iIDOkButton = -1;
		iIDCancelButton = -1;
		fPlaceStandardButtons = false;

		for (i=0; i<pWSHDlgCtrls->m_lCount; i++) {
			HWND  hWnd = 0;
			int iCommonOptions = WS_CHILD | WS_VISIBLE | ((iPreviousType!=pWSHDlgCtrls->m_pCtrlList[i]->m_iType)?WS_GROUP|WS_TABSTOP:0);
			
			int x = pWSHDlgCtrls->m_pCtrlList[i]->m_x;
			int y = pWSHDlgCtrls->m_pCtrlList[i]->m_y;
			int w = pWSHDlgCtrls->m_pCtrlList[i]->m_w;
			int h = pWSHDlgCtrls->m_pCtrlList[i]->m_h;

			RECT ControlRect;
			SetRect(&ControlRect, x, y, x + w, y + h);

			switch ( pWSHDlgCtrls->m_pCtrlList[i]->m_iType ) {
			case CTRLTYPE_PUSHBUTTON:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_PUSHBUTTON|iCommonOptions, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);
				UnionRect(&Rect, &Rect, &ControlRect);
				if ( iBorderX>x ) iBorderX = x;
				if ( iBorderY>y ) iBorderY = y;
				
				iPreviousType = -1;
				break;

			case CTRLTYPE_OKBUTTON:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_DEFPUSHBUTTON|iCommonOptions, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);
				iIDOkButton = CTRLID_START+i;
				if ( (x>=0) || (y>=0) ) {
					UnionRect(&Rect, &Rect, &ControlRect);
					if ( iBorderX>x ) iBorderX = x;
					if ( iBorderY>y ) iBorderY = y;
				}
				else
					fPlaceStandardButtons = true;

				iPreviousType = -1;
				break;

			case CTRLTYPE_CANCELBUTTON:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_PUSHBUTTON|iCommonOptions, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);
				iIDCancelButton = CTRLID_START+i;
				if ( (x>=0) || (y>=0) ) {
					UnionRect(&Rect, &Rect, &ControlRect);
					if ( iBorderX>x ) iBorderX = x;
					if ( iBorderY>y ) iBorderY = y;
				}
				else
					fPlaceStandardButtons = true;

				iPreviousType = -1;
				break;

			case CTRLTYPE_CHECKBOX:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_AUTOCHECKBOX|iCommonOptions, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);
				if ( hWnd )
					SendMessage(hWnd, BM_SETCHECK, pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.lVal, 0);
				
				UnionRect(&Rect, &Rect, &ControlRect);
				if ( iBorderX>x ) iBorderX = x;
				if ( iBorderY>y ) iBorderY = y;
				
				iPreviousType = pWSHDlgCtrls->m_pCtrlList[i]->m_iType;
				break;

			case CTRLTYPE_RADIONBUTTON:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_AUTORADIOBUTTON|iCommonOptions, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);
				if ( hWnd )
					SendMessage(hWnd, BM_SETCHECK, pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.lVal, 0);

				UnionRect(&Rect, &Rect, &ControlRect);
				if ( iBorderX>x ) iBorderX = x;
				if ( iBorderY>y ) iBorderY = y;
				
				iPreviousType = pWSHDlgCtrls->m_pCtrlList[i]->m_iType;
				break;

			case CTRLTYPE_GROUPBOX:
				hWnd = CreateWindow("button", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, BS_GROUPBOX|iCommonOptions&~WS_TABSTOP, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);

				UnionRect(&Rect, &Rect, &ControlRect);
				if ( iBorderX>x ) iBorderX = x;
				if ( iBorderY>y ) iBorderY = y;

				iPreviousType = pWSHDlgCtrls->m_pCtrlList[i]->m_iType;
				break;

			case CTRLTYPE_LABEL:
				hWnd = CreateWindow("static", pWSHDlgCtrls->m_pCtrlList[i]->m_szTitle, iCommonOptions&~WS_TABSTOP, x, y, w, h, hDlg, (HMENU) (CTRLID_START+i), _Module.m_hInst, NULL);

				UnionRect(&Rect, &Rect, &ControlRect);
				if ( iBorderX>x ) iBorderX = x;
				if ( iBorderY>y ) iBorderY = y;

				iPreviousType = pWSHDlgCtrls->m_pCtrlList[i]->m_iType;
				break;
			}
			if ( hWnd )
				SendMessage(hWnd, WM_SETFONT, (WPARAM) hFont, 1);
		}
		if ( iBorderX==32767 )
			iBorderX = 0;
		Rect.right += iBorderX;
		
		if ( iBorderY==32767 )
			iBorderY = 0;
		Rect.bottom += iBorderY;

		if ( pWSHDlg->m_w>0 )
			Rect.right = pWSHDlg->m_w;
		if ( pWSHDlg->m_h>0 )
			Rect.bottom = pWSHDlg->m_h;

		if ( fPlaceStandardButtons ) {
			int iButtonHeight = 0;
			int iButtonWidth = 0;
			if ( iIDOkButton!=-1 )
				iButtonWidth = pWSHDlgCtrls->m_pCtrlList[iIDOkButton-CTRLID_START]->m_w;
			if ( iIDCancelButton!=-1 ) {
				if ( iButtonWidth ) iButtonWidth += 5;
				iButtonWidth += pWSHDlgCtrls->m_pCtrlList[iIDCancelButton-CTRLID_START]->m_w;
			}
			int iPosX = (Rect.right - iButtonWidth) / 2;

			if ( iIDOkButton!=-1 ) {
				iButtonHeight = pWSHDlgCtrls->m_pCtrlList[iIDOkButton-CTRLID_START]->m_h;
				SetWindowPos(
					GetDlgItem(hDlg, iIDOkButton), 
					0,
					iPosX,
					Rect.bottom,
					0,0,SWP_NOZORDER|SWP_NOSIZE);
				iPosX += 5 + pWSHDlgCtrls->m_pCtrlList[iIDOkButton-CTRLID_START]->m_w;
			}

			if ( iIDCancelButton!=-1 ) {
				if ( iButtonHeight<pWSHDlgCtrls->m_pCtrlList[iIDCancelButton-CTRLID_START]->m_h )
					iButtonHeight = pWSHDlgCtrls->m_pCtrlList[iIDCancelButton-CTRLID_START]->m_h;
				SetWindowPos(
					GetDlgItem(hDlg, iIDCancelButton), 
					0,
					iPosX,
					Rect.bottom,
					0,0,SWP_NOZORDER|SWP_NOSIZE);
			}

			Rect.bottom += (iButtonHeight + iBorderY);
		}

		AdjustWindowRect(&Rect, GetWindowLong(hDlg, GWL_STYLE), FALSE);

		int x,y;

		x = pWSHDlg->m_x;
		if ( x<0 ) 
			x = (GetSystemMetrics(SM_CXSCREEN)-(Rect.right-Rect.left)) / 2;

		y = pWSHDlg->m_y;
		if ( y<0 ) 
			y = (GetSystemMetrics(SM_CYSCREEN)-(Rect.bottom-Rect.top)) / 2;

		SetWindowPos(hDlg, 0, x, y, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOZORDER);

		return 0;

	case WM_COMMAND:
		if ( HIWORD(wParam)==BN_CLICKED ) {
			switch (LOWORD(wParam)) {
			case IDOK:
				SaveDlgValues(hDlg, pWSHDlgCtrls);
				if ( iIDOkButton==-1 )
					EndDialog(hDlg, IDOK);
				else
					EndDialog(hDlg, pWSHDlgCtrls->m_pCtrlList[iIDOkButton-CTRLID_START]->m_vValue.lVal);
				break;

			case IDCANCEL:
				if ( iIDCancelButton==-1 )
					EndDialog(hDlg, IDCANCEL);
				else
					EndDialog(hDlg, pWSHDlgCtrls->m_pCtrlList[iIDCancelButton-CTRLID_START]->m_vValue.lVal);
				break;

			default:
				if ( LOWORD(wParam)<CTRLID_START )
					break;

				switch ( pWSHDlgCtrls->m_pCtrlList[LOWORD(wParam)-CTRLID_START]->m_iType ) {
				case CTRLTYPE_PUSHBUTTON:
				case CTRLTYPE_OKBUTTON:
					SaveDlgValues(hDlg, pWSHDlgCtrls);
					EndDialog(hDlg, pWSHDlgCtrls->m_pCtrlList[LOWORD(wParam)-CTRLID_START]->m_vValue.lVal);
					break;

				case CTRLTYPE_CANCELBUTTON:
					EndDialog(hDlg, pWSHDlgCtrls->m_pCtrlList[LOWORD(wParam)-CTRLID_START]->m_vValue.lVal);
					break;
				}
				break;
			}
		}
		break;

	default:
		return 0;
	}

	return 1;
};
