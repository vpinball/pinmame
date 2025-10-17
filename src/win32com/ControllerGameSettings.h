// ControllerGameSettings.h : Declaration of the CGameSettings
#pragma once

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGameSettings
class ATL_NO_VTABLE CGameSettings : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CGameSettings, &CLSID_GameSettings>,
	public ISupportErrorInfo,
	public IDispatchImpl<IGameSettings, &IID_IGameSettings, &LIBID_VPinMAMELib>
{
public:
	CGameSettings();
	~CGameSettings();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CGameSettings)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGameSettings)
	COM_INTERFACE_ENTRY(IGameSettings)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISettings
public:
	STDMETHOD(get_Value)(/*[in]*/ BSTR sName, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Value)(/*[in]*/ BSTR sName, /*[in]*/ VARIANT newVal);
	STDMETHOD(Clear)();
	STDMETHOD(ShowSettingsDlg)(/* [in,defaultvalue(0)] */ LONG_PTR hParentWnd);
	STDMETHOD(SetDisplayPosition)(/*[in]*/ VARIANT newValX, /*[in]*/ VARIANT newValY,/* [in,defaultvalue(0)] */ LONG_PTR hWnd);
	void Init(IGame * pGame);

private:
	IGame *m_pGame;
	char m_szROM[256];
	char m_szRegKey[256];
	char m_szRegKeyDef[256];
};
