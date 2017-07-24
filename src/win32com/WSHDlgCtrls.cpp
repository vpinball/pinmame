// WSHDlgCtrls.cpp : Implementation of WSHDlgCtrls
#include "Stdafx.h"
#include "VPinMAME_h.h"
#include "WSHDlgCtrls.h"

/////////////////////////////////////////////////////////////////////////////
// CEnumGames
class ATL_NO_VTABLE CEnumWSHDlgCtrls : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnumWSHDlgCtrls, &CLSID_EnumWSHDlgCtrls>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEnumWSHDlgCtrls, &IID_IEnumWSHDlgCtrls, &LIBID_VPinMAMELib>,
	public IEnumVARIANT
{
public:
	CEnumWSHDlgCtrls()
	{
		m_pWSHDlgCtrls = NULL;
	}

	~CEnumWSHDlgCtrls()
	{
		if ( m_pWSHDlgCtrls )
			m_pWSHDlgCtrls->Release();
		m_pWSHDlgCtrls = NULL;
 	}


DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CEnumWSHDlgCtrls)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumWSHDlgCtrls)
	COM_INTERFACE_ENTRY(IEnumWSHDlgCtrls)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_IEnumWSHDlgCtrls
		};
		for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i],riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IEnumWSHDlgCtrls
public:
	STDMETHODIMP Next(ULONG celt,VARIANT __RPC_FAR *rgVar, ULONG __RPC_FAR *pCeltFetched)
	{
		HRESULT hr = S_FALSE;

		if ( pCeltFetched )
			*pCeltFetched = 0;

		int i = 0;

		long lMax;
		m_pWSHDlgCtrls->get_Count(&lMax);

		while ( m_lCurrent<lMax && celt ) {
			CComVariant varCelt(m_lCurrent++);
			VariantInit(&rgVar[i]);

			IWSHDlgCtrl* pWSHDlgCtrl;
			hr = m_pWSHDlgCtrls->get_Item(&varCelt, &pWSHDlgCtrl);
			if ( FAILED(hr) )
				return hr;

			rgVar[i].vt = VT_DISPATCH;
			rgVar[i].ppdispVal = new IDispatch*;
			hr = pWSHDlgCtrl->QueryInterface(IID_IDispatch, (void**) &rgVar[i].pdispVal);
			pWSHDlgCtrl->Release();
			
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
		long lMax;
		m_pWSHDlgCtrls->get_Count(&lMax);

		m_lCurrent += celt;
		if ( m_lCurrent<lMax ) return S_OK;

		m_lCurrent = lMax;
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

			CComObject<CEnumWSHDlgCtrls> *p = NULL;
			hr = CComObject<CEnumWSHDlgCtrls>::CreateInstance(&p);
			if ( SUCCEEDED(hr) ) {
				p->Init(m_pWSHDlgCtrls);
				p->m_lCurrent = m_lCurrent;

				hr = p->QueryInterface(IID_IEnumWSHDlgCtrls, (void**) ppEnum);
				if ( FAILED(hr) )
					delete p;
			}
		}
		return hr;
	}

	void Init(IWSHDlgCtrls *pWSHDlgCtrls) {
		m_pWSHDlgCtrls = pWSHDlgCtrls;
		m_pWSHDlgCtrls->AddRef();
		m_lCurrent = 0;
	}

public:
	IWSHDlgCtrls *m_pWSHDlgCtrls;
	long m_lCurrent;
};

/////////////////////////////////////////////////////////////////////////////
// CWSHDlgCtrls

CWSHDlgCtrls::CWSHDlgCtrls()
{
	m_lCount = 0;
	m_pCtrlList = NULL;
}

CWSHDlgCtrls::~CWSHDlgCtrls()
{
	if ( m_pCtrlList ) {
		for (int i=0;i<m_lCount;i++)
			m_pCtrlList[i]->Release();

		delete m_pCtrlList;
	
		m_pCtrlList = NULL;
		m_lCount = 0;
	}
}

STDMETHODIMP CWSHDlgCtrls::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IWSHDlgCtrls
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CWSHDlgCtrls::get_Count(long* pnCount)
{
	*pnCount = m_lCount;
	return S_OK;
}

STDMETHODIMP CWSHDlgCtrls::get_Item(VARIANT *pKey, IWSHDlgCtrl **ppWSHDlgCtrl)
{
	HRESULT hr = S_FALSE;
	*ppWSHDlgCtrl = NULL;

	VariantChangeType(pKey, pKey, 0, VT_I4);

	if ( pKey->lVal<0 || pKey->lVal>m_lCount )
		return E_INVALIDARG;

	if ( m_pCtrlList[pKey->lVal] )
		hr = m_pCtrlList[pKey->lVal]->QueryInterface(IID_IUnknown, (void**) ppWSHDlgCtrl);

	return hr;
}

STDMETHODIMP CWSHDlgCtrls::get__NewEnum(IUnknown** ppunkEnum)
{
	*ppunkEnum = 0;

	CComObject<CEnumWSHDlgCtrls>* pe = 0;
	HRESULT hr = CComObject<CEnumWSHDlgCtrls>::CreateInstance(&pe);
	if ( SUCCEEDED(hr) ) {
		pe->AddRef();
		pe->Init(this);

		hr = pe->QueryInterface(IID_IUnknown, (void**) ppunkEnum);
		pe->Release();

		return hr;
	}
	return S_FALSE;
}

STDMETHODIMP CWSHDlgCtrls::add_Item(IWSHDlgCtrl *pWSHDlgCtrl)
{
	if ( !pWSHDlgCtrl )
		return S_FALSE;

	CComObject<CWSHDlgCtrl>*  *pNewCtrlList;

	pNewCtrlList = new CComObject<CWSHDlgCtrl>*[m_lCount+1];
	if ( m_pCtrlList ) {
		memcpy(pNewCtrlList, m_pCtrlList, m_lCount * sizeof (CComObject<CWSHDlgCtrl>*));
		delete m_pCtrlList;
	}

	m_pCtrlList = pNewCtrlList;

	pWSHDlgCtrl->AddRef();
	m_pCtrlList[m_lCount++] = (CComObject<CWSHDlgCtrl>*)(pWSHDlgCtrl);

	return S_OK;
}
