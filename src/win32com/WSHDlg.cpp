// WSHDlg.cpp : Implementation of CWSHDlg
#include "Stdafx.h"
#include "VPinMAME.h"
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

STDMETHODIMP CWSHDlg::AddCtrl(BSTR sType, long x, long y, long w, long h, BSTR sTitle, VARIANT vValue, long options, IUnknown **pRetVal)
{
	if ( !pRetVal )
		return S_FALSE;
	*pRetVal = NULL;

	CComObject<CWSHDlgCtrl>*  pNewCtrl;

	HRESULT hr = CComObject<CWSHDlgCtrl>::CreateInstance(&pNewCtrl);
	if ( FAILED(hr) ) 
		return hr;

	hr = pNewCtrl->Init(sType, x, y, w, h, sTitle, vValue, options);
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

INT_PTR CALLBACK WSHDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

STDMETHODIMP CWSHDlg::Show(VARIANT *RetVal)
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
	RetVal->lVal = DialogBoxIndirectParam(_Module.m_hInst, &Template.DlgTemplate, 0, WSHDlgProc, (LPARAM) this);
	DWORD dwLastError = GetLastError();

	return S_OK;
}

#define CTRLID_START 1000

void SaveDlgValues(HWND hDlg, CWSHDlgCtrls *pWSHDlgCtrls)
{
	for (int i=0; i<pWSHDlgCtrls->m_lCount; i++) {
		switch ( pWSHDlgCtrls->m_pCtrlList[i]->m_iType ) {
		case 1:
		case 2:
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.Clear();
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.vt = VT_BOOL;
			pWSHDlgCtrls->m_pCtrlList[i]->m_vValue.boolVal = IsDlgButtonChecked(hDlg, CTRLID_START+i)?VARIANT_TRUE:VARIANT_FALSE;
			break;
		}
	}
}

INT_PTR CALLBACK WSHDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CWSHDlg		*pWSHDlg = NULL;
	static CWSHDlgCtrls *pWSHDlgCtrls = NULL;

	long i;
	int  iPreviousType;
	RECT Rect;
	int  iBorderX, iBorderY;
	HFONT hFont;

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
		iBorderX = 0;
		iBorderY = 0;

		iPreviousType = -1;
		for (i=0; i<pWSHDlgCtrls->m_lCount; i++) {
			pWSHDlgCtrls->m_pCtrlList[i]->CreateControlWindow(hDlg,CTRLID_START+i,(pWSHDlgCtrls->m_pCtrlList[i]->m_iType!=iPreviousType));
			SendDlgItemMessage(hDlg, CTRLID_START+i, WM_SETFONT, (WPARAM) hFont, 1);
			iPreviousType = pWSHDlgCtrls->m_pCtrlList[i]->m_iType;
			if ( !iPreviousType ) // pushbutton
				iPreviousType = -1;

			RECT ControlRect;
			SetRect(&ControlRect,
				pWSHDlgCtrls->m_pCtrlList[i]->m_x,
				pWSHDlgCtrls->m_pCtrlList[i]->m_y,
				pWSHDlgCtrls->m_pCtrlList[i]->m_x + pWSHDlgCtrls->m_pCtrlList[i]->m_w,
				pWSHDlgCtrls->m_pCtrlList[i]->m_y + pWSHDlgCtrls->m_pCtrlList[i]->m_h
			);
			UnionRect(&Rect, &Rect, &ControlRect);

			if ( (i==0) || (iBorderX>pWSHDlgCtrls->m_pCtrlList[i]->m_x) )
				iBorderX = pWSHDlgCtrls->m_pCtrlList[i]->m_x;
			if ( (i==0) || iBorderY>pWSHDlgCtrls->m_pCtrlList[i]->m_y )
				iBorderY = pWSHDlgCtrls->m_pCtrlList[i]->m_y;
		}
		Rect.right += iBorderX;
		Rect.bottom += iBorderY;
	    AdjustWindowRect(&Rect, GetWindowLong(hDlg, GWL_STYLE), FALSE);
		if ( pWSHDlg->m_w>0 )
			Rect.right = pWSHDlg->m_w;
		if ( pWSHDlg->m_h>0 )
			Rect.bottom = pWSHDlg->m_h;

		SetWindowPos(hDlg, 0, pWSHDlg->m_x, pWSHDlg->m_y, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOZORDER);

		return 0;

	case WM_COMMAND:
		if ( HIWORD(wParam)!=BN_CLICKED )
			break;

		if ( LOWORD(wParam)>=CTRLID_START ) {
			if ( pWSHDlgCtrls->m_pCtrlList[LOWORD(wParam)-CTRLID_START]->m_iType==0) {
				SaveDlgValues(hDlg, pWSHDlgCtrls);
				EndDialog(hDlg, pWSHDlgCtrls->m_pCtrlList[LOWORD(wParam)-CTRLID_START]->m_vValue.lVal);
			}
		}
		else {
			if ( LOWORD(wParam)==1 )
				SaveDlgValues(hDlg, pWSHDlgCtrls);

 			EndDialog(hDlg, LOWORD(wParam));
		}
		break;

	default:
		return 0;
	}

	return 1;
};
