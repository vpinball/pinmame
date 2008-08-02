// WSHDlgCtrl.cpp : Implementation of CWSHDlgCtrl
#include "Stdafx.h"
#include "VPinMAME_h.h"
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
	"cmdbtn", "okbtn", "cancelbtn", "chkbox", "optbtn", "frame", "label", 0
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
	switch ( m_iType ) {
	case CTRLTYPE_PUSHBUTTON:
	case CTRLTYPE_OKBUTTON:
	case CTRLTYPE_CANCELBUTTON:
	case CTRLTYPE_CHECKBOX:
	case CTRLTYPE_RADIONBUTTON:
		VariantChangeType(&m_vValue, &newVal, 0, VT_I4);
		break;

	default:
		m_vValue = newVal;
	}

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

STDMETHODIMP CWSHDlgCtrl::Init(BSTR sType, long x, long y, long w, long h, BSTR sTitle)
{
	CComBSTR sHelp(sType);

	char szType[256];
	WideCharToMultiByte(CP_ACP, 0, sType, -1, szType, sizeof szType, NULL, NULL);

	m_iType = 0;
	while ( pszCtrlTypes[m_iType] ) {
		if ( _strcmpi(szType, pszCtrlTypes[m_iType])==0 ) {
			switch (m_iType) {
			case CTRLTYPE_OKBUTTON:
				m_vValue = 1;
				break;

			case CTRLTYPE_CANCELBUTTON:
				m_vValue = 2;
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

	return S_OK;
}
