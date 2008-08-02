// ControllerGames.cpp : Implementation of CGames
#include "stdafx.h"
#include "VPinMAME_h.h"
#include "ControllerGame.h"
#include "ControllerGames.h"

extern "C" {
#include "driver.h"
}

/////////////////////////////////////////////////////////////////////////////
// CEnumGames

class ATL_NO_VTABLE CEnumGames : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnumGames, &CLSID_EnumGames>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEnumGames, &IID_IEnumGames, &LIBID_VPinMAMELib>,
	public IEnumVARIANT
{
public:
	CEnumGames()
	{
		m_pGames = NULL;
	}

	~CEnumGames()
	{
		if ( m_pGames )
			m_pGames->Release();
		m_pGames = NULL;
 	}


DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CEnumGames)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumGames)
	COM_INTERFACE_ENTRY(IEnumGames)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_IEnumGames
		};
		for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i],riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IEnumGames
public:
	STDMETHODIMP Next(ULONG celt,VARIANT __RPC_FAR *rgVar, ULONG __RPC_FAR *pCeltFetched)
	{
		HRESULT hr = S_FALSE;

		if ( pCeltFetched )
			*pCeltFetched = 0;

		int i = 0;

		while ( m_lCurrent<m_lMax && celt ) {
			CComVariant varCelt(m_lCurrent++);
			VariantInit(&rgVar[i]);

			IGame* pGame;
			hr = m_pGames->get_Item(&varCelt, &pGame);
			if ( FAILED(hr) )
				return hr;

			rgVar[i].vt = VT_DISPATCH;
			rgVar[i].ppdispVal = new IDispatch*;
			hr = pGame->QueryInterface(IID_IDispatch, (void**) &rgVar[i].pdispVal);
			pGame->Release();
			
			if ( FAILED(hr) ) 
				return hr;

			celt--;
			if ( pCeltFetched )
				(*pCeltFetched)++;

			i++;
		}

		return celt ? S_FALSE : S_OK;
	}

    STDMETHODIMP Skip(ULONG celt)
	{
		m_lCurrent += celt;
		if ( m_lCurrent<m_lMax ) return S_OK;

		m_lCurrent = m_lMax;
		return S_FALSE;
	}

    STDMETHODIMP Reset(void)
	{
		m_lCurrent = 0;

		return S_OK;
	}
    
	STDMETHODIMP Clone(IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum)
	{
		return Clone((IUnknown**) ppEnum);
	}
	
	STDMETHODIMP Clone(IUnknown __RPC_FAR *__RPC_FAR *ppEnum)
	{
		HRESULT hr = E_POINTER;

		if ( ppEnum!=NULL ) {
			*ppEnum = NULL;

			CComObject<CEnumGames> *p = NULL;
			hr = CComObject<CEnumGames>::CreateInstance(&p);
			if ( SUCCEEDED(hr) ) {
				p->Init(m_pGames);
				p->m_lMax = m_lMax;
				p->m_lCurrent = m_lCurrent;

				hr = p->QueryInterface(IID_IEnumGames, (void**) ppEnum);
				if ( FAILED(hr) )
					delete p;
			}
		}
		return hr;
	}

	void Init(IGames *pGames) {
		m_pGames = pGames;
		m_pGames->AddRef();
		m_lCurrent = 0;
		m_pGames->get_Count(&m_lMax);
	}

public:
	IGames *m_pGames;
	long m_lCurrent;
	long m_lMax;
};

/////////////////////////////////////////////////////////////////////////////
// CGames

CGames::CGames()
{
	m_lGames = 0;
	while (drivers[m_lGames])
		m_lGames++;

	m_pGamesList = NULL;
	if ( m_lGames ) {
		m_pGamesList = new CComObject<CGame>*[m_lGames+1]; // [0] is the default game
		memset(m_pGamesList, 0x00, (m_lGames+1) * sizeof (CComObject<CGame>*));

		int i = 0;
		HRESULT hr = CComObject<CGame>::CreateInstance(&m_pGamesList[i]);
		if ( SUCCEEDED(hr) ) {
			m_pGamesList[i]->AddRef();
			hr = m_pGamesList[i]->Init(NULL);
			if ( FAILED(hr) ) {
				m_pGamesList[i]->Release();
				m_pGamesList[i] = NULL;
			}
		}

		i++;
		while ( i<=m_lGames ) {
			hr = CComObject<CGame>::CreateInstance(&m_pGamesList[i]);

			if ( SUCCEEDED(hr) ) {
				m_pGamesList[i]->AddRef();
				hr = m_pGamesList[i]->Init(drivers[i-1]);
				if ( FAILED(hr) ) {
					m_pGamesList[i]->Release();
					m_pGamesList[i] = NULL;
				}
			}
			i++;
		}
	}
}

CGames::~CGames()
{
	if ( m_pGamesList ) {
		int i = 0;
		while (i<m_lGames) {
			if ( m_pGamesList[i] ) {
				m_pGamesList[i]->Release();
				m_pGamesList[i] = NULL;
			}
			i++;
		}

		delete m_pGamesList;
		m_pGamesList = NULL;
	}
}

STDMETHODIMP CGames::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IGames
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CGames::get_Count(long* pnCount)
{
	*pnCount = m_lGames;
	return S_OK;
}

STDMETHODIMP CGames::get_Item(VARIANT *pKey, IGame **pGame)
{
	HRESULT hr = S_FALSE;
	*pGame = NULL;

	if (pKey->vt & VT_BSTR) {
		CComBSTR sKey;
		if ( pKey->vt & VT_BYREF )
			sKey = *pKey->pbstrVal;
		else
			sKey = pKey->bstrVal;
		if ( sKey.Length() )
			sKey.ToLower();

		int i = 0;
		while ( i<m_lGames ) {
			if ( m_pGamesList[i] ) {
				CComBSTR sGameName;
				m_pGamesList[i]->get_Name(&sGameName);
				if ( sGameName.Length() )
					sGameName.ToLower();
				if ( sGameName==sKey )
					break;
			}
			i++;
		}

		if ( i>m_lGames )
			i = 0; // return the "default" game

		if ( m_pGamesList[i] )
			hr = m_pGamesList[i]->QueryInterface(IID_IUnknown, (void**) pGame);
	}
	else {
		VariantChangeType(pKey, pKey, 0, VT_I4);

		if ( pKey->lVal<0 || pKey->lVal>m_lGames )
			return E_INVALIDARG;

		if ( m_pGamesList[pKey->lVal] )
			hr = m_pGamesList[pKey->lVal]->QueryInterface(IID_IUnknown, (void**) pGame);
	}

	return hr;
}

STDMETHODIMP CGames::get__NewEnum(IUnknown** ppunkEnum)
{
	*ppunkEnum = 0;

	CComObject<CEnumGames>* pe = 0;
	HRESULT hr = CComObject<CEnumGames>::CreateInstance(&pe);
	if ( SUCCEEDED(hr) ) {
		pe->AddRef();
		pe->Init(this);

		hr = pe->QueryInterface(IID_IUnknown, (void**) ppunkEnum);
		pe->Release();

		return hr;
	}
	return S_FALSE;
}
