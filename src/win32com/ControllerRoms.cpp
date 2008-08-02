// ControllerRoms.cpp : Implementation of CRoms
#include "stdafx.h"
#include "VPinMAME_h.h"
#include "ControllerRom.h"
#include "ControllerRoms.h"

extern "C" {
#include "driver.h"
}

/////////////////////////////////////////////////////////////////////////////
// CEnumRoms
class ATL_NO_VTABLE CEnumRoms : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnumRoms, &CLSID_EnumRoms>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEnumRoms, &IID_IEnumRoms, &LIBID_VPinMAMELib>,
	public IEnumVARIANT
{
public:
	CEnumRoms()
	{
		m_pRoms = NULL;
	}

	~CEnumRoms()
	{
		if ( m_pRoms )
			m_pRoms->Release();
		m_pRoms = NULL;
 	}


DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CEnumRoms)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumRoms)
	COM_INTERFACE_ENTRY(IEnumRoms)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_IEnumRoms
		};
		for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i],riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IEnumRoms
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

			IRom* pRom;
			hr = m_pRoms->get_Item(&varCelt, &pRom);
			if ( FAILED(hr) )
				return hr;

			rgVar[i].vt = VT_DISPATCH;
			hr = pRom->QueryInterface(IID_IDispatch, (void**) &rgVar[i].pdispVal);
			pRom->Release();

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

			CComObject<CEnumRoms> *p = NULL;
			hr = CComObject<CEnumRoms>::CreateInstance(&p);
			if ( SUCCEEDED(hr) ) {
				p->Init(m_pRoms);
				p->m_lMax = m_lMax;
				p->m_lCurrent = m_lCurrent;

				hr = p->QueryInterface(IID_IEnumRoms, (void**) ppEnum);
				if ( FAILED(hr) )
					delete p;
			}
		}
		return hr;
	}

	void Init(IRoms *pRoms) {
		m_pRoms = pRoms;
		m_pRoms->AddRef();
		m_lCurrent = 0;
		m_pRoms->get_Count(&m_lMax);
	}

public:
	IRoms *m_pRoms;
	long m_lCurrent;
	long m_lMax;
};

/////////////////////////////////////////////////////////////////////////////
// CRoms

CRoms::CRoms()
{
	m_lRoms = 0;
	m_pRomsList = NULL;
	m_gamedrv = NULL;
}

CRoms::~CRoms()
{
	Deinit();
}

STDMETHODIMP CRoms::Init(const struct GameDriver *gamedrv)
{
	if ( !gamedrv ) {
		m_lRoms = 0;
		m_pRomsList = NULL;
		m_gamedrv = NULL;
		return S_OK;
	}

	m_gamedrv = gamedrv;

	const struct RomModule *region, *rom;

	for (region = rom_first_region(m_gamedrv); region; region = rom_next_region(region)) {
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			m_lRoms++;
		}
	}

	if ( m_lRoms ) {
		m_pRomsList = new CComObject<CRom>*[m_lRoms];
		memset(m_pRomsList, 0x00, m_lRoms * sizeof (CComObject<CRom>*));

		int i = 0;
		for (region = rom_first_region(m_gamedrv); region; region = rom_next_region(region)) {
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				HRESULT hr = CComObject<CRom>::CreateInstance(&m_pRomsList[i]);

				if ( SUCCEEDED(hr) ) {
					m_pRomsList[i]->AddRef();
					hr = m_pRomsList[i]->Init(m_gamedrv, region, rom);
					if ( FAILED(hr) ) {
						m_pRomsList[i]->Release();
						m_pRomsList[i] = NULL;
					}
				}
				i++;
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CRoms::Deinit()
{
	if ( m_pRomsList ) {
		int i = 0;
		while (i<m_lRoms) {
			if ( m_pRomsList[i] ) {
				m_pRomsList[i]->Release();
				m_pRomsList[i] = NULL;
			}
			i++;
		}

		delete m_pRomsList;
		m_pRomsList = NULL;
	}

	return S_OK;
}

STDMETHODIMP CRoms::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRoms
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CRoms::get_Count(long* pnCount)
{
	*pnCount = m_lRoms;
	return S_OK;
}

STDMETHODIMP CRoms::get_Item(VARIANT *pKey, IRom **pRom)
{
	HRESULT hr = S_FALSE;
	*pRom = NULL;

	if (pKey->vt & VT_BSTR) {
		CComBSTR sKey;
		if ( pKey->vt & VT_BYREF )
			sKey = *pKey->pbstrVal;
		else
			sKey = pKey->bstrVal;
		sKey.ToLower();

		int i = 0;
		while ( i<m_lRoms ) {
			if ( m_pRomsList[i] ) {
				CComBSTR sRomName;
				m_pRomsList[i]->get_Name(&sRomName);
				sRomName.ToLower();
				if ( sRomName==sKey )
					break;
			}
			i++;
		}

		if ( i<m_lRoms ) {
			if ( m_pRomsList[i] ) 
				hr = m_pRomsList[i]->QueryInterface(IID_IUnknown, (void**) pRom);
		}
	}
	else {
		VariantChangeType(pKey, pKey, 0, VT_I4);

		if ( pKey->lVal<0 || pKey->lVal>m_lRoms )
			return E_INVALIDARG;

		if ( m_pRomsList[pKey->lVal] ) 
			hr = m_pRomsList[pKey->lVal]->QueryInterface(IID_IUnknown, (void**) pRom);
	}

	return hr;
}

STDMETHODIMP CRoms::get__NewEnum(IUnknown** ppunkEnum)
{
	*ppunkEnum = 0;

	CComObject<CEnumRoms>* pe = 0;
	HRESULT hr = CComObject<CEnumRoms>::CreateInstance(&pe);
	if ( SUCCEEDED(hr) ) {
		pe->AddRef();
		pe->Init(this);

		hr = pe->QueryInterface(IID_IUnknown, (void**) ppunkEnum);
		pe->Release();

		return hr;
	}
	return S_FALSE;
}

STDMETHODIMP CRoms::get_Available(VARIANT_BOOL *pVal)
{
	if ( !m_gamedrv )
		return S_FALSE;

	/* check for existence of romset */
	*pVal = VARIANT_TRUE;
	if (!mame_faccess (m_gamedrv->name, FILETYPE_ROM))
	{
		/* if the game is a clone, check for parent */
		if (m_gamedrv->clone_of == 0 ||
				!mame_faccess(m_gamedrv->clone_of->name,FILETYPE_ROM))
			*pVal = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP CRoms::get_State(long *pVal)
{
	*pVal = 0;

	long lState = 1, lSubState;

	int i = 0;
	while ( i<m_lRoms ) {
		m_pRomsList[i++]->get_State(&lSubState);
		if ( !lSubState )
			return S_OK;
		if ( lSubState!=1 )
			lState = 2;
	}

	*pVal = lState;
	return S_OK;
}

STDMETHODIMP CRoms::get_StateDescription(BSTR *pVal)
{
	long lState;
	get_State(&lState);

	CComBSTR sStateDescription;
	switch ( lState ) {
	case 1:
		sStateDescription = "Ok";
		break;

	case 2:
		sStateDescription = "Bad";
		break;

	default:
		sStateDescription = "Unknown";
		break;
	}

	*pVal = sStateDescription.Detach();
	return S_OK;
}

STDMETHODIMP CRoms::Audit(VARIANT_BOOL fStrict)
{
	int i = 0;
	while ( i<m_lRoms )
		m_pRomsList[i++]->Audit(fStrict);

	return S_OK;
}
