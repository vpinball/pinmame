// ControllerGames.h : Declaration of the CGames

#ifndef __CONTROLLERGAMES_H_
#define __CONTROLLERGAMES_H_

#include "resource.h"       // main symbols
#include "ControllerGame.h"

/////////////////////////////////////////////////////////////////////////////
// CGames
class ATL_NO_VTABLE CGames : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGames, &CLSID_Games>,
	public ISupportErrorInfo,
	public IDispatchImpl<IGames, &IID_IGames, &LIBID_VPinMAMELib>
{
public:
	CGames();
	~CGames();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CGames)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGames)
	COM_INTERFACE_ENTRY(IGames)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IGames
public:
	STDMETHOD(get_Count)(long* pnCount);
	STDMETHOD(get_Item)(VARIANT *pKey, IGame **pGame);
	STDMETHOD(get__NewEnum)(IUnknown** ppunkEnum);

private:
	CComObject<CGame>*  *m_pGamesList;
	long				m_lGames;
};

#endif //__CONTROLLERGAMES_H_
