// WSHDlgCtrl.cpp : Implementation of CWSHDlgCtrl
#include "Stdafx.h"
#include "VPinMAME.h"
#include "WSHDlgCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CWSHDlgCtrl

STDMETHODIMP CWSHDlgCtrl::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IWSHDlgCtrl
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

const char* pszCtrlTypes[] = {
	"cmdbtn", "chkbox", "optbtn", "frame", "label", 0
};

CWSHDlgCtrl::CWSHDlgCtrl()
{
	m_x = -1;
	m_y = -1;
	m_w = 0;
	m_h = 0;
	strcpy(m_szTitle, "");
	m_iType = -1;
	m_vValue = 0;
	m_options = 0;
}

CWSHDlgCtrl::~CWSHDlgCtrl()
{
	m_x = m_x;
}

STDMETHODIMP CWSHDlgCtrl::get_Type(BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CComBSTR sType(pszCtrlTypes[m_iType]);
	*pVal = sType.Detach();

	return S_OK;
}

STDMETHODIMP CWSHDlgCtrl::get_Value(VARIANT *pVal)
{
	if ( !pVal )
		return S_FALSE;

	*pVal = m_vValue;

	return S_OK;
}

STDMETHODIMP CWSHDlgCtrl::put_Value(VARIANT newVal)
{
	m_vValue = newVal;

	return S_OK;
}

STDMETHODIMP CWSHDlgCtrl::get_Title(BSTR *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CComBSTR sTitle(m_szTitle);
	*pVal = sTitle.Detach();

	return S_OK;
}

STDMETHODIMP CWSHDlgCtrl::put_Title(BSTR newVal)
{
	WideCharToMultiByte(CP_ACP, 0, newVal, -1, m_szTitle, sizeof m_szTitle, NULL, NULL);

	return S_OK;
}

STDMETHODIMP CWSHDlgCtrl::Init(BSTR sType, long x, long y, long w, long h, BSTR sTitle, VARIANT vValue, long options)
{
	CComBSTR sHelp(sType);

	char szType[256];
	WideCharToMultiByte(CP_ACP, 0, sType, -1, szType, sizeof szType, NULL, NULL);

	m_iType = 0;
	while ( pszCtrlTypes[m_iType] ) {
		if ( strcmpi(szType, pszCtrlTypes[m_iType])==0 ) {
			switch (m_iType) {
			case 0:
				VariantChangeType(&m_vValue, &vValue, 0, VT_I4);
				break;

			case 1:
			case 2:
				VariantChangeType(&m_vValue, &vValue, 0, VT_BOOL);
				break;

			default:
				m_vValue.Copy(&vValue);
				break;
			}
			break;
		}
		else
			m_iType++;
	}

	if ( !pszCtrlTypes[m_iType] )
		return Error("Invalid control type.");

	m_x = x;
	m_y = y;
	m_w = w;
	m_h = h;
	WideCharToMultiByte(CP_ACP, 0, sTitle, -1, m_szTitle, sizeof m_szTitle, NULL, NULL);
	m_options = options;

	return S_OK;
}

HWND CWSHDlgCtrl::CreateControlWindow(HWND hParent, int iCtrlID, bool fStartGroup)
{
	HWND hWnd = 0;
	int iCommonOptions = WS_CHILD | WS_VISIBLE | (fStartGroup?WS_GROUP|WS_TABSTOP:0);

	switch ( m_iType ) {
	case 0:
		hWnd = CreateWindow("button", m_szTitle, m_options|iCommonOptions, m_x, m_y, m_w, m_h, hParent, (HMENU) iCtrlID, _Module.m_hInst, NULL);
		break;

	case 1:
		hWnd = CreateWindow("button", m_szTitle, BS_AUTOCHECKBOX|m_options|iCommonOptions, m_x, m_y, m_w, m_h, hParent, (HMENU) iCtrlID, _Module.m_hInst, NULL);
		if ( hWnd )
			SendMessage(hWnd, BM_SETCHECK, (m_vValue.boolVal==VARIANT_TRUE)?1:0, 0);
		break;

	case 2:
		hWnd = CreateWindow("button", m_szTitle, BS_AUTORADIOBUTTON|m_options|iCommonOptions, m_x, m_y, m_w, m_h, hParent, (HMENU) iCtrlID, _Module.m_hInst, NULL);
		if ( hWnd )
			SendMessage(hWnd, BM_SETCHECK, (m_vValue.boolVal==VARIANT_TRUE)?1:0, 0);
		break;

	case 3:
		hWnd = CreateWindow("button", m_szTitle, BS_GROUPBOX|m_options|iCommonOptions&~WS_TABSTOP, m_x, m_y, m_w, m_h, hParent, (HMENU) iCtrlID, _Module.m_hInst, NULL);
		break;

	case 4:
		hWnd = CreateWindow("static", m_szTitle, m_options|iCommonOptions&~WS_TABSTOP, m_x, m_y, m_w, m_h, hParent, (HMENU) iCtrlID, _Module.m_hInst, NULL);
		break;
	}

	return hWnd;
}