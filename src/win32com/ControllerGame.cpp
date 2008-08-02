// ControllerGame.cpp : Implementation of CGame
#include "stdafx.h"
#include "VPinMAME_h.h"
#include "ControllerGame.h"
#include "ControllerGameSettings.h"

#include "ControllerRegkeys.h"
#include "VPinMAMEConfig.h"

extern "C" {
#include "driver.h"
#include "audit.h"
}

#include "alias.h"
#include <atlwin.h>


/////////////////////////////////////////////////////////////////////////////
// CGameInfoDlg

class CGameInfoDlg : public CDialogImpl<CGameInfoDlg> {
public:
	BEGIN_MSG_MAP(CGameInfoDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_DONTCARE, OnDontCare)
	END_MSG_MAP()

	enum { IDD = IDD_GAMEINFODLG };

	int		m_nShowOptions;

private:

	BOOL fROMSetOK;

	// message handlers
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM lParam, BOOL&) {
		CenterWindow();

		CGame* pGame = (CGame*) lParam;
		if ( !pGame )
			return 1;

		CComBSTR sHelp;
		char szHelp[256];

		pGame->get_Name(&sHelp);
		WideCharToMultiByte(CP_ACP, 0, sHelp, -1, szHelp, sizeof szHelp, NULL, NULL);
		::SetWindowText(GetDlgItem(IDC_ROMSETNAME), szHelp);
		sHelp.Empty();

		pGame->get_Description(&sHelp);
		WideCharToMultiByte(CP_ACP, 0, sHelp, -1, szHelp, sizeof szHelp, NULL, NULL);
		::SetWindowText(GetDlgItem(IDC_GAMENAME), szHelp);
		sHelp.Empty();

		::SetWindowText(GetDlgItem(IDC_ROMDIRS), (char*) get_option("rompath"));

		int nTabStops[6] = {70,100,150,200,260, 290};

		SendDlgItemMessage(IDC_ROMLIST, LB_SETTABSTOPS, 6, (LPARAM) &nTabStops);

		char szLine[256];

		fROMSetOK = false;

		int iError = 0;
		BOOL fOK	  = false;
		BOOL fMaybeOK = false;

		IRoms *pRoms;
		if ( FAILED(pGame->get_Roms(&pRoms)) )
			return 1;

		pRoms->Audit(VARIANT_FALSE);

		IEnumRoms *pEnumRoms;
		if ( FAILED(pRoms->get__NewEnum((IUnknown**) &pEnumRoms)) ) {
			pRoms->Release();
			return 1;
		}

		unsigned long uFetched;
		VARIANT vRom;
		IRom *pRom;

		int nCount = 0;

		fOK      = true;
		fMaybeOK = true;

		while ( SUCCEEDED(pEnumRoms->Next(1, &vRom, &uFetched)) && uFetched ) {
			HRESULT hr = vRom.pdispVal->QueryInterface(__uuidof(IRom), (void**) &pRom);
			VariantClear(&vRom);

			CComBSTR sName;
			char szName[256];
			pRom->get_Name(&sName);
			WideCharToMultiByte(CP_ACP, 0, sName, -1, szName, sizeof szName, NULL, NULL);
			sName.Empty();

			char szType[8];
			long lFlags;
			pRom->get_Flags(&lFlags);
			if ( lFlags & ROMREGION_SOUNDONLY )
				strcpy(szType,"sound");
			else
				strcpy(szType,"cpu");

			long lExpLength;
			pRom->get_ExpLength(&lExpLength);

			long lLength;
			pRom->get_Length(&lLength);

			long lExpChecksum;
			pRom->get_ExpChecksum(&lExpChecksum);

			long lChecksum;
			pRom->get_Checksum(&lChecksum);

			sprintf(szLine, "%s\t%s\t%i\t%i\t0x%08X\t0x%08X\t", szName, szType, lExpLength, lLength, lExpChecksum, lChecksum);

			pRom->get_StateDescription(&sHelp);
			WideCharToMultiByte(CP_ACP, 0, sHelp, -1, szHelp, sizeof szHelp, NULL, NULL);
			sHelp.Empty();
			strcat(szLine, szHelp);

			SendDlgItemMessage(IDC_ROMLIST, LB_ADDSTRING, 0, (LPARAM) &szLine);

			long lState;
			pRom->get_State(&lState);

			if ( !(m_nShowOptions & CHECKOPTIONS_IGNORESOUNDROMS) || !(lFlags & ROMREGION_SOUNDONLY) ) {
				fOK &= (lState==AUD_ROM_GOOD);
				fMaybeOK &= (lState==AUD_ROM_GOOD) || (lState==AUD_ROM_NEED_DUMP) || (lState==AUD_ROM_NEED_REDUMP)
					|| (lState==AUD_NOT_AVAILABLE) || (lState==AUD_LENGTH_MISMATCH) || (lState==AUD_BAD_CHECKSUM);
			}

			pRom->Release();
			nCount++;
		}

		pEnumRoms->Release();

		VARIANT_BOOL fAvailable;
		pRoms->get_Available(&fAvailable);
		if ( fAvailable==VARIANT_FALSE ) {
			strcpy(szLine, "The ROM set was not found, but this should be the contents:");
			SendDlgItemMessage(IDC_ROMLIST, LB_ADDSTRING, 0, (LPARAM) &szLine);
			iError = 2;
		}

		VARIANT_BOOL fIsSupported;
		pGame->get_IsSupported(&fIsSupported);
		if ( fIsSupported==VARIANT_FALSE ) {
			fMaybeOK = false;
			iError = 3;
		}

		if ( !nCount ) {
			strcpy(szLine, "No ROMs defined for this game");
			SendDlgItemMessage(IDC_ROMLIST, LB_ADDSTRING, 0, (LPARAM) &szLine);
			iError = 1;
		}

		switch ( iError ) {
		case 0:
			if ( fOK ) {
				strcpy(szLine, "ROM set is good; VPinMAME is able to use it");
				fROMSetOK = true;
			}
			else if ( fMaybeOK )
				strcpy(szLine, "There are CRC and/or length errors! VPinMAME may or may not successfully be able to use the ROM set.\r\nIf sound roms are missing, sound will be disabled.");
			else
				strcpy(szLine, "ROM set is bad: VPinMAME can't use it.");
			break;

		case 1:
			strcpy(szLine, "No ROMs defined for this game: VPinMAME can't run this game.");
			break;

		case 2:
			strcpy(szLine, "ROM set is missing: VPinMAME can't run the game.");
			break;

		case 3:
			strcpy(szLine, "This game is not supported by VPinMAME.");
			break;
		}

		::SetWindowText(GetDlgItem(IDC_STATE), szLine);

		switch ( m_nShowOptions & CHECKOPTIONS_OPTIONSMASK ) {
		case CHECKOPTIONS_SHOWRESULTSIFFAIL:
			if ( fROMSetOK )
				EndDialog(IDOK);
			break;

		case CHECKOPTIONS_SHOWRESULTSNEVER:
			EndDialog(fROMSetOK?IDOK:IDCANCEL);
			break;
		}

		::ShowWindow(GetDlgItem(IDC_DONTCARE), (m_nShowOptions&CHECKOPTIONS_SHOWDONTCARE) && !fOK && fMaybeOK);

		return 1;
	}

	LRESULT OnOK(WORD, UINT, HWND, BOOL&) {
		EndDialog(fROMSetOK?IDOK:IDCANCEL);
		return 0;
	}

	LRESULT OnCancel(WORD, UINT, HWND, BOOL&) {
		EndDialog(fROMSetOK?IDOK:IDCANCEL);
		return 0;
	}

	LRESULT OnDontCare(WORD, UINT, HWND, BOOL&) {
		EndDialog(IDIGNORE);
		return 0;
	}
public:
	LRESULT OnStnClickedState(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

/////////////////////////////////////////////////////////////////////////////
// CGame

STDMETHODIMP CGame::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_IGame
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CGame::CGame()
{
	m_gamedrv = NULL;
	m_pRoms = NULL;
}

CGame::~CGame()
{
	if ( m_pRoms )
		m_pRoms->Release();
	m_pRoms = NULL;
}

HRESULT CGame::Init(const struct GameDriver *gamedrv)
{
	m_gamedrv = gamedrv;

	HRESULT hr = CComObject<CRoms>::CreateInstance(&m_pRoms);
	if ( SUCCEEDED(hr) ) {
		m_pRoms->AddRef();
		hr = m_pRoms->Init(m_gamedrv);
		if ( FAILED(hr) ) {
			m_pRoms->Release();
			m_pRoms = NULL;
		}
	}
	else
		m_pRoms = NULL;

	return hr;
}

STDMETHODIMP CGame::get_Name(BSTR *pVal)
{
	if ( !m_gamedrv ) {
		CComBSTR Name("");
		*pVal = Name.Detach();
		return S_OK;
	}

	CComBSTR Name(m_gamedrv->name);
	*pVal = Name.Detach();

	return S_OK;
}

STDMETHODIMP CGame::get_Description(BSTR *pVal)
{
	if ( !m_gamedrv ) {
		CComBSTR Description("default game");
		*pVal = Description.Detach();
		return S_OK;
	}

	CComBSTR Description(m_gamedrv->description);
	*pVal = Description.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_Year(BSTR *pVal)
{
	if ( !m_gamedrv ) {
		CComBSTR Year("2002");
		*pVal = Year.Detach();
		return S_OK;
	}

	CComBSTR Year(m_gamedrv->year);
	*pVal = Year.Detach();

	return S_OK;
}

STDMETHODIMP CGame::get_Manufacturer(BSTR *pVal)
{
	if ( !m_gamedrv ) {
		CComBSTR Manufacturer("none");
		*pVal = Manufacturer.Detach();
		return S_OK;
	}

	CComBSTR Manufacturer(m_gamedrv->manufacturer);
	*pVal = Manufacturer.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_CloneOf(BSTR *pVal)
{
	CComBSTR CloneOf;

	if ( m_gamedrv && m_gamedrv->clone_of )
		CloneOf = m_gamedrv->clone_of->name;

	*pVal = CloneOf.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_Roms(IRoms* *pVal)
{
	if ( !pVal )
		return S_FALSE;

	return m_pRoms->QueryInterface(IID_IRoms, (void**) pVal);
}

STDMETHODIMP CGame::get_Settings(IGameSettings **pVal)
{
	CComObject<CGameSettings> *pGameSettings;

	HRESULT hr = CComObject<CGameSettings>::CreateInstance(&pGameSettings);
	if ( FAILED(hr) )
		return hr;

	pGameSettings->AddRef();
	pGameSettings->Init(this);

	*pVal = pGameSettings;

	return S_OK;
}

/* some helper functions */

/* Determine Game # from Given GameName String */
int GetGameNumFromString(char *name)
{
	int gamenum = 0;
	while (drivers[gamenum]) {
		if ( !_strcmpi(drivers[gamenum]->name, name) )
			break;
		gamenum++;
	}
	if ( !drivers[gamenum] )
		return -1;
	else
		return gamenum;
}

char* GetGameRegistryKey(char *pszRegistryKey, char* pszROMName)
{
	if ( !pszRegistryKey )
		return NULL;

	lstrcpy(pszRegistryKey, REG_BASEKEY);
	lstrcat(pszRegistryKey, "\\");

	// dealing with default options?
	if ( !pszROMName || !*pszROMName )
		lstrcat(pszRegistryKey, REG_DEFAULT);
	else if ( GetGameNumFromString(pszROMName)>=0 )
		lstrcat(pszRegistryKey, pszROMName);
	else
		lstrcpy(pszRegistryKey, "");

	return pszRegistryKey;
}

BOOL GameUsedTheFirstTime(char* pszROMName)
{
	char szKey[MAX_PATH];
	GetGameRegistryKey(szKey, pszROMName);

	if ( !szKey[0] )
		return false;

	HKEY hKey;
   	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS ) {
		DWORD dwDisposition;
   		if ( RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)==ERROR_SUCCESS )
			RegCloseKey(hKey);
		return true;
	}

	return false;
}

BOOL GameWasNeverStarted(char* pszROMName)
{
	char szKey[MAX_PATH];
	GetGameRegistryKey(szKey, pszROMName);

	if ( !szKey[0] )
		return true;

	return ReadRegistry(szKey, "", 0)==0;
}

void SetGameWasStarted(char* pszROMName)
{
	char szKey[MAX_PATH];
	GetGameRegistryKey(szKey, pszROMName);

	if ( !szKey[0] )
		return;

	WriteRegistry(szKey, "", 1);
}

STDMETHODIMP CGame::ShowInfoDlg(int nShowOptions, long hParentWnd, int *pVal)
{
	if ( !pVal )
		return S_FALSE;

	CGameInfoDlg GameInfoDlg;
	GameInfoDlg.m_nShowOptions = nShowOptions;
	*pVal = GameInfoDlg.DoModal((HWND) hParentWnd, (LPARAM) this);

	return S_OK;
}

STDMETHODIMP CGame::get_IsSupported(VARIANT_BOOL *pVal) {
  if (!pVal) return S_FALSE;
  *pVal = ((!m_gamedrv) || checkGameNotSupported(m_gamedrv)) ? VARIANT_FALSE : VARIANT_TRUE;
  return S_OK;
}
