// ControllerGame.cpp : Implementation of CGame
#include "stdafx.h"
#include "VPinMAME.h"
#include "ControllerGame.h"
#include "ControllerOptions.h"

extern "C" {
#include "driver.h"
}

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
	if ( m_pRoms ) {
		m_pRoms->Release();
		m_pRoms = NULL;
	}
}


HRESULT CGame::Init(const struct GameDriver *gamedrv)
{
	if ( !gamedrv )
		return S_FALSE;
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
	CComBSTR Name(m_gamedrv->name);

	*pVal = Name.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_Description(BSTR *pVal)
{
	CComBSTR Description(m_gamedrv->description);

	*pVal = Description.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_Year(BSTR *pVal)
{
	CComBSTR Year(m_gamedrv->year);

	*pVal = Year.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_Manufacturer(BSTR *pVal)
{
	CComBSTR Manufacturer(m_gamedrv->manufacturer);

	*pVal = Manufacturer.Detach();
	return S_OK;
}

STDMETHODIMP CGame::get_CloneOf(BSTR *pVal)
{
	CComBSTR CloneOf;

	if ( m_gamedrv->clone_of )
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
