// ControllerGameSettings.h : Declaration of the CGameSettings

#ifndef __CONTROLLERGAMESETTINGSC_H_
#define __CONTROLLERGAMESETTINGSC_H_

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
	STDMETHOD(get_DisplayPosY)(long hParentWnd, /*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayPosY)(long hParentWnd, /*[in]*/ long newVal);
	STDMETHOD(get_DisplayPosX)(long hParentWnd, /*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayPosX)(long hParentWnd, /*[in]*/ long newVal);
	STDMETHOD(Clear)();
	STDMETHOD(ShowSettingsDlg)(/* [in,defaultvalue(0)] */ long hParentWnd);
	STDMETHOD(get_SampleRate)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SampleRate)(/*[in]*/ long newVal);
	STDMETHOD(get_UseSound)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UseSound)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_UseCheat)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UseCheat)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_UseSamples)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UseSamples)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_DoubleSize)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_DoubleSize)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_CompactSize)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_CompactSize)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_DisplayOnly)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_DisplayOnly)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Border)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Border)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Title)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Title)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_DisplayColorBlue)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayColorBlue)(/*[in]*/ long newVal);
	STDMETHOD(get_DisplayColorGreen)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayColorGreen)(/*[in]*/ long newVal);
	STDMETHOD(get_DisplayColorRed)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayColorRed)(/*[in]*/ long newVal);
	STDMETHOD(get_DisplayIntensityOff)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayIntensityOff)(/*[in]*/ long newVal);
	STDMETHOD(get_DisplayIntensity33)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayIntensity33)(/*[in]*/ long newVal);
	STDMETHOD(get_DisplayIntensity66)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DisplayIntensity66)(/*[in]*/ long newVal);
	STDMETHOD(get_AntiAlias)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_AntiAlias)(/*[in]*/ long newVal);
	void Init(IGame * pGame);

private:
	IGame *m_pGame;
	char m_szROM[256];
	char m_szRegKey[256];
	char m_szRegKeyDef[256];
};

#endif //__SETTINGS_H_
