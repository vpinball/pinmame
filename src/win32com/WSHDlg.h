// WSHDlg.h : Declaration of the CWSHDlg

#ifndef __WSHDLG_H_
#define __WSHDLG_H_

#include "resource.h"       // main symbols
#include "WSHDlgCtrl.h"
#include "WSHDlgCtrls.h"
#include <atlctl.h>

/////////////////////////////////////////////////////////////////////////////
// CWSHDlg
class ATL_NO_VTABLE CWSHDlg : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWSHDlg, &CLSID_WSHDlg>,
	public ISupportErrorInfo,
	public IDispatchImpl<IWSHDlg, &IID_IWSHDlg, &LIBID_VPinMAMELib>,
	public IObjectSafetyImpl<CWSHDlg, INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CWSHDlg();
	~CWSHDlg();

DECLARE_REGISTRY_RESOURCEID(IDR_WSHDLG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWSHDlg)
	COM_INTERFACE_ENTRY(IWSHDlg)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWSHDlg
public:
	STDMETHOD(get_Ctrls)(/*[out, retval]*/ IWSHDlgCtrls **ppVal);
	STDMETHOD(Show)(long hParentWnd, /*[out, retval]*/ VARIANT *RetVal);
	STDMETHOD(AddCtrl)(/*[in]*/ BSTR sType, /*[in]*/ long x, /*[in]*/ long y, /*[in]*/ long w, /*[in]*/ long h, /*[in]*/ BSTR sTitle, /*[out,retval]*/ IUnknown **pRetVal);
	STDMETHOD(get_Title)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Title)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_h)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_h)(/*[in]*/ long newVal);
	STDMETHOD(get_w)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_w)(/*[in]*/ long newVal);
	STDMETHOD(get_y)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_y)(/*[in]*/ long newVal);
	STDMETHOD(get_x)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_x)(/*[in]*/ long newVal);

public:
	long m_x;
	long m_y;
	long m_w;
	long m_h;
	char m_szTitle[256];

	CComObject<CWSHDlgCtrls> *m_pWSHDlgCtrls;
};

#endif //__WSHDLG_H_
