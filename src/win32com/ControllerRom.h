// ControllerRom.h : Declaration of the CRom

#ifndef CONTROLLERROM_H
#define CONTROLLERROM_H

#define AUD_ROM_UNKNOWN		0x0000
#define AUD_ROM_GOOD		0x0001
#define AUD_ROM_NEED_REDUMP	0x0002
#define AUD_ROM_NOT_FOUND	0x0004
#define AUD_NOT_AVAILABLE	0x0008
#define AUD_BAD_CHECKSUM	0x0010
#define AUD_MEM_ERROR		0x0020
#define AUD_LENGTH_MISMATCH	0x0040
#define AUD_ROM_NEED_DUMP	0x0080

/////////////////////////////////////////////////////////////////////////////
// CRom
class ATL_NO_VTABLE CRom : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRom, &CLSID_Rom>,
	public ISupportErrorInfo,
	public IDispatchImpl<IRom, &IID_IRom, &LIBID_VPinMAMELib>
{
public:
	CRom();
	~CRom();

	HRESULT Init(const struct GameDriver *gamedrv, const struct RomModule *region, const struct RomModule *rom);

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRom)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRom)
	COM_INTERFACE_ENTRY(IRom)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRom
public:
	STDMETHOD(Audit)(/*[in]*/ VARIANT_BOOL fStrict);
	STDMETHOD(get_StateDescription)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_State)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Flags)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_ExpChecksum)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Checksum)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_ExpLength)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Length)(/*[out, retval]*/ long *pVal);

private:
	long m_lGameNum;
	const struct GameDriver *m_gamedrv;
	const struct RomModule *m_rom;
	const struct RomModule *m_region;

	const char* m_pszName;
	DWORD m_dwState;
	DWORD m_dwLength;
	DWORD m_dwExpLength;
	DWORD m_dwChecksum;
	DWORD m_dwExpChecksum;
	DWORD m_dwRegionFlags;
};

#endif // CONTROLLERROM_H