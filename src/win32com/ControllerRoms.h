// ControllerRoms.h : Declaration of the CRoms

#ifndef __ROMS_H_
#define __ROMS_H_

#include "resource.h"       // main symbols
#include "ControllerRom.h"
#include "ControllerRoms.h"

/////////////////////////////////////////////////////////////////////////////
// CRoms
class ATL_NO_VTABLE CRoms : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRoms, &CLSID_Roms>,
	public ISupportErrorInfo,
	public IDispatchImpl<IRoms, &IID_IRoms, &LIBID_VPinMAMELib>
{
public:
	CRoms();
	~CRoms();

// Init and deinit
	STDMETHOD(Init)(const struct GameDriver *gamedrv);
	STDMETHOD(Deinit)();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRoms)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRoms)
	COM_INTERFACE_ENTRY(IRoms)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRoms
public:
	STDMETHOD(Audit)(/*[in]*/ VARIANT_BOOL fStrict);
	STDMETHOD(get_StateDescription)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_State)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Available)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Count)(long* pnCount);
	STDMETHOD(get_Item)(VARIANT *pKey, IRom **pRom);
	STDMETHOD(get__NewEnum)(IUnknown** ppunkEnum);

private:
	const struct GameDriver *m_gamedrv;
	long				m_lRoms;
	CComObject<CRom>*  *m_pRomsList;
};

#endif //__ROMS_H_
