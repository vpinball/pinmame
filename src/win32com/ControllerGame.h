// ControllerGame.h : Declaration of the CGame

#ifndef __CONTROLLERGAME_H_
#define __CONTROLLERGAME_H_

#include "ControllerRoms.h"

/////////////////////////////////////////////////////////////////////////////
// CGame
class ATL_NO_VTABLE CGame : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGame, &CLSID_Game>,
	public ISupportErrorInfo,
	public IDispatchImpl<IGame, &IID_IGame, &LIBID_VPinMAMELib>
{
public:
	CGame();
	~CGame();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CGame)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGame)
	COM_INTERFACE_ENTRY(IGame)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IGame
public:
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Manufacturer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Year)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_CloneOf)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Roms)(/*[out, retval]*/ IRoms* *pVal);

public:
	HRESULT Init(const struct GameDriver *gamedrv);

private:
	const struct GameDriver *m_gamedrv;
	CComObject<CRoms> *m_pRoms;
};

#endif // __CONTROLLERGAME_H_