// ControllerGame.h : Declaration of the CGame

#ifndef __CONTROLLERGAME_H_
#define __CONTROLLERGAME_H_

#include "ControllerRoms.h"

#define CHECKOPTIONS_OPTIONSMASK			0x00FF

#define CHECKOPTIONS_SHOWRESULTSALLWAYS		0
#define CHECKOPTIONS_SHOWRESULTSIFFAIL		1
#define CHECKOPTIONS_SHOWRESULTSNEVER		2

#define CHECKOPTIONS_IGNORESOUNDROMS		0x4000
#define CHECKOPTIONS_SHOWDONTCARE			0x8000

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
	STDMETHOD(get_IsSupported)(/* [out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Manufacturer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Year)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_CloneOf)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Roms)(/*[out, retval]*/ IRoms* *pVal);
	STDMETHOD(ShowInfoDlg)(/*[in,defaultvalue(0)]*/ int nShowOptions, /*[in,defaultvalue(0)]*/ long hParentWnd, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_Settings)(/*[out, retval]*/ IGameSettings * *pVal);

	HRESULT Init(const struct GameDriver *gamedrv);

private:
	const struct GameDriver *m_gamedrv;
	CComObject<CRoms> *m_pRoms;
};

/* some helper functions */

int	  GetGameNumFromString(char *name);
char* GetGameRegistryKey(char *pszRegistryKey, char* pszGameName);
BOOL  GameUsedTheFirstTime(char* pszROMName);

BOOL GameWasNeverStarted(char* pszROMName);
void SetGameWasStarted(char* pszROMName);

#endif // __CONTROLLERGAME_H_