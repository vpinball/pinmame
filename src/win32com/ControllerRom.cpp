// ControllerRom.cpp : Implementation of CRom
#include "stdafx.h"
#include "VPinMAME_h.h"
#include "ControllerRom.h"

extern "C" {
#include "driver.h"
#include "audit.h"
}

DWORD CRom::GetChecksumFromHash(char* szHash)
{
	DWORD dwChecksum = 0;
	if ( szHash && *szHash )
		sscanf(szHash, "c:%lx", &dwChecksum);

	return dwChecksum;
}

STDMETHODIMP CRom::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IRom
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CRom::CRom()
{
	m_gamedrv = NULL;
	m_rom = NULL;

	m_pszName = NULL;
	m_dwState = 0;
	m_dwLength = 0;
	m_dwExpLength = 0;
	m_dwChecksum = 0;
	m_dwExpChecksum = 0;
	m_dwRegionFlags = 0;
}

CRom::~CRom()
{
}


HRESULT CRom::Init(const struct GameDriver *gamedrv, const struct RomModule *region, const struct RomModule *rom)
{
	if ( !gamedrv || !region ||!rom )
		return S_FALSE;

	m_gamedrv = gamedrv;
	m_region = region;
	m_rom = rom;

	m_pszName = ROM_GETNAME(m_rom);

	m_dwState = 0;
	m_dwLength = 0;
	m_dwExpLength = 0;

	const struct RomModule *chunk;
	for (chunk = rom_first_chunk(m_rom); chunk; chunk = rom_next_chunk(chunk))
		m_dwExpLength += ROM_GETLENGTH(chunk);

	m_dwChecksum = 0;

	char szExpChecksum[256];
	lstrcpy(szExpChecksum, ROM_GETHASHDATA(m_rom));
	m_dwExpChecksum = GetChecksumFromHash(szExpChecksum);

	m_dwRegionFlags = ROMREGION_GETFLAGS(m_region);

	return S_OK;
}

STDMETHODIMP CRom::get_Name(BSTR *pVal)
{
	if ( !m_pszName )
		return S_FALSE;

	CComBSTR Name(m_pszName);

	*pVal = Name.Detach();
	return S_OK;
}


STDMETHODIMP CRom::get_State(long *pVal)
{
	*pVal = m_dwState;

	return S_OK;
}

STDMETHODIMP CRom::get_StateDescription(BSTR *pVal)
{
	CComBSTR sStateDescription;

	switch ( m_dwState ) {
	case AUD_ROM_GOOD:
		sStateDescription = "OK";
		break;

	case AUD_ROM_NEED_REDUMP:
		sStateDescription = "Need redump";
		break;

	case AUD_ROM_NOT_FOUND:
		sStateDescription = "Not found";
		break;

	case AUD_NOT_AVAILABLE:
		sStateDescription = "Not available";
		break;

	case AUD_BAD_CHECKSUM:
		sStateDescription = "Bad checksum";
		break;

	case AUD_MEM_ERROR:
		sStateDescription = "Memory error";
		break;
	
	case AUD_LENGTH_MISMATCH:
		sStateDescription = "Length mismatch";
		break;

	case AUD_ROM_NEED_DUMP:
		sStateDescription = "Need dump";
		break;

	default:
		sStateDescription = "Unknown";
		break;
	}

	*pVal = sStateDescription.Detach();

	return S_OK;
}

STDMETHODIMP CRom::Audit(VARIANT_BOOL fStrict)
{
	int err;
	char szHash[256];
	lstrcpy(szHash, fStrict?"":ROM_GETHASHDATA(m_rom));

	/* obtain CRC-32 and length of ROM file */
	const struct GameDriver *drv = m_gamedrv;
	do
	{
		err = mame_fchecksum(drv->name, m_pszName, (unsigned int*) &m_dwLength, szHash);
		drv = drv->clone_of;
	} while (err && drv);

	if (err)
	{
		if (!m_dwChecksum)
			/* not found but it's not good anyway */
			m_dwState = AUD_NOT_AVAILABLE;
		else
			/* not found */
			m_dwState = AUD_ROM_NOT_FOUND;
	}
	else {
		m_dwChecksum = GetChecksumFromHash(szHash);

		/* all cases below assume the ROM was at least found */
		if (m_dwExpLength != m_dwLength)
			m_dwState = AUD_LENGTH_MISMATCH;
		else if (m_dwExpChecksum != m_dwChecksum)
		{
			if (!m_dwExpChecksum)
				m_dwState = AUD_ROM_NEED_DUMP; /* new case - found but not known to be dumped */
			else if (m_dwChecksum == BADCRC(m_dwExpChecksum))
				m_dwState = AUD_ROM_NEED_REDUMP;
			else
				m_dwState = AUD_BAD_CHECKSUM;
		}
		else
			m_dwState = AUD_ROM_GOOD;
	}

	return S_OK;
}

STDMETHODIMP CRom::get_Length(long *pVal)
{
	*pVal = m_dwLength;

	return S_OK;
}

STDMETHODIMP CRom::get_ExpLength(long *pVal)
{
	*pVal = m_dwExpLength;

	return S_OK;
}

STDMETHODIMP CRom::get_Checksum(long *pVal)
{
	*pVal = m_dwChecksum;

	return S_OK;
}


STDMETHODIMP CRom::get_ExpChecksum(long *pVal)
{
	*pVal = m_dwExpChecksum;

	return S_OK;
}

STDMETHODIMP CRom::get_Flags(long *pVal)
{
	*pVal = m_dwRegionFlags;

	return S_OK;
}
