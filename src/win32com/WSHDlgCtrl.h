// WSHDlgCtrl.h : Declaration of the CWSHDlgCtrl

#ifndef __WSHDLGCTRL_H_
#define __WSHDLGCTRL_H_

#include "resource.h"       // main symbols

#define CTRLTYPE_PUSHBUTTON		0
#define CTRLTYPE_OKBUTTON		1
#define CTRLTYPE_CANCELBUTTON	2
#define CTRLTYPE_CHECKBOX		3
#define CTRLTYPE_RADIONBUTTON	4
#define CTRLTYPE_GROUPBOX		5
#define CTRLTYPE_LABEL			6

/////////////////////////////////////////////////////////////////////////////
// CWSHDlgCtrl
class ATL_NO_VTABLE CWSHDlgCtrl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWSHDlgCtrl, &CLSID_WSHDlgCtrl>,
	public ISupportErrorInfo,
	public IDispatchImpl<IWSHDlgCtrl, &IID_IWSHDlgCtrl, &LIBID_VPinMAMELib>
{
public:
	CWSHDlgCtrl();
	~CWSHDlgCtrl();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CWSHDlgCtrl)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWSHDlgCtrl)
	COM_INTERFACE_ENTRY(IWSHDlgCtrl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWSHDlgCtrl
public:
	STDMETHOD(get_Title)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Title)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Value)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ BSTR *pVal);

public:
	long m_x;
	long m_y;
	long m_w;
	long m_h;
	char m_szTitle[256];
	int  m_iType;
	CComVariant m_vValue;

	STDMETHODIMP Init(BSTR sType, long x, long y, long w, long h, BSTR sTitle);
};

#endif //__WSHDLGCTRL_H_
