// WSHDlgCtrls.h : Declaration of the WSHDlgCtrls

#ifndef __WSHDLGCTRLS_H_
#define __WSHDLGCTRLS_H_

#include "resource.h"       // main symbols
#include "WSHDlgCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CWSHDlgCtrls
class ATL_NO_VTABLE CWSHDlgCtrls : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWSHDlgCtrls, &CLSID_WSHDlgCtrls>,
	public ISupportErrorInfo,
	public IDispatchImpl<IWSHDlgCtrls, &IID_IWSHDlgCtrls, &LIBID_VPinMAMELib>
{
public:
	CWSHDlgCtrls();
	~CWSHDlgCtrls();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CWSHDlgCtrls)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWSHDlgCtrls)
	COM_INTERFACE_ENTRY(IWSHDlgCtrls)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWSHDlgCtrls
public:
	STDMETHOD(get_Count)(long* pnCount);
	STDMETHOD(get_Item)(VARIANT *pKey, IWSHDlgCtrl **ppWSHDlgCtrl);
	STDMETHOD(get__NewEnum)(IUnknown** ppunkEnum);
	STDMETHOD(add_Item)(IWSHDlgCtrl *pWSHDlgCtrl);

public:
	CComObject<CWSHDlgCtrl>*  *m_pCtrlList;
	long				       m_lCount;
};

#endif //__WSHDLGCTRLS_H_
