// Controller.h: Definition of the Controller class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Controller_H__D2811491_40D6_4656_9AA7_8FF85FD63543__INCLUDED_)
#define AFX_Controller_H__D2811491_40D6_4656_9AA7_8FF85FD63543__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Resource.h"       // main symbols
#include "VPinMAMECP.h"

#include "ControllerSettings.h"
#include "ControllerGames.h"
#include "ControllerGame.h"
#include "ControllerGameSettings.h"
#include <atlctl.h>

#define VPINMAMEADJUSTWINDOWMSG "VPinMAMEAdjustWindowMsg"

/////////////////////////////////////////////////////////////////////////////
// Controller

class CController : 
	public IConnectionPointContainerImpl<CController>,
	public IDispatchImpl<IController, &IID_IController, &LIBID_VPinMAMELib>, 
	public ISupportErrorInfo,
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CController,&CLSID_Controller>,
	public CProxy_IControllerEvents< CController >,
	public IProvideClassInfo2Impl<&CLSID_Controller, &DIID__IControllerEvents>,
	public IObjectSafetyImpl<CController, INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	HANDLE				m_hThreadRun;				// Handle to the running thread!
	HANDLE				m_hEmuIsRunning;			// Event handle for running emulation

	// options not stored in the registry
	BOOL				m_fHandleKeyboard;			// Enable the keyboard the the ball simulator (usefully for debug a game)
	int					m_iHandleMechanics;			// which mechanics should be handled by VPM
	BOOL				m_fDisplayLocked;			// Lock the display window so you can't move it around with the mouse
	BOOL				m_fWindowHidden;			// visibility of the controller window, not persistent, will be set to false
													// every time a game name is set
	BOOL				m_fMechSamples;				// play the samples for the mech sounds, n ot persitent, will be set to false
													// every time a game name is set

	char				m_szROM[256];				// String containing rom name (game name)
	int					m_nGameNo;					// the number of the actual game in the drivers list, -1 if no game selected
	IGame				*m_pGame;					// Pointer to a game object of the actual game
	IGameSettings		*m_pGameSettings;			// Pointer to a settings object of the actual game

	HWND				m_hParentWnd;				// the parent hWnd of the DMD window
	char				m_szSplashInfoLine[256];	// info line for the splash dialog

	HWND				m_hEventWnd;				// handle to a window which will send the COM events

	CComObject<CGames>				*m_pGames;
	CComObject<CControllerSettings>	*m_pControllerSettings;

	CController::CController();
	CController::~CController();

private:
	void GetProductVersion(int *nVersionNo0, int *nVersionNo1, int *nVersionNo2, int *nVersionNo3);
	static DWORD FAR PASCAL RunController(CController* pController);

public:

BEGIN_COM_MAP(CController)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IController)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CController)
CONNECTION_POINT_ENTRY(DIID__IControllerEvents)
END_CONNECTION_POINT_MAP()

DECLARE_NOT_AGGREGATABLE(CController)

DECLARE_REGISTRY_RESOURCEID(IDR_CONTROLLER)
DECLARE_PROTECT_FINAL_CONSTRUCT()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IController
public:
	STDMETHOD(get_ChangedLEDs)(/*[in]*/ int nHigh, int nLow, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Settings)(/*[out, retval]*/ IControllerSettings * *pVal);
	STDMETHOD(get_Games)(/*[out, retval]*/ IGames* *pVal);
	STDMETHOD(get_Version)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SolMask)(/*[in]*/ int nLow, /*[out, retval]*/ long *pVal);
	STDMETHOD(put_SolMask)(/*[in]*/ int nLow, /*[in]*/ long newVal);
	STDMETHOD(put_Mech)(/*[in]*/ int param, /*[in]*/int newVal);
	STDMETHOD(get_LockDisplay)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_LockDisplay)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_GetMech)(/*[in]*/ int mechNo, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_GIStrings)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Dip)(/*[in]*/ int nNo, /*[out, retval]*/ int *pVal);
	STDMETHOD(put_Dip)(/*[in]*/ int nNo, /*[in]*/ int newVal);
	STDMETHOD(get_Solenoids)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_SplashInfoLine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SplashInfoLine)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ChangedGIStrings)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_GIString)(int nString, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_HandleMechanics)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_HandleMechanics)(/*[in]*/ int newVal);
	STDMETHOD(get_Running)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Machines)(/*[in]*/ BSTR sMachine, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Pause)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Pause)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_HandleKeyboard)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_HandleKeyboard)(/*[int]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_GameName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_GameName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ChangedSolenoids)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Switches)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Switches)(/*[out, retval]*/ VARIANT newVal);
	STDMETHOD(get_ChangedLamps)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Lamps)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_WPCNumbering)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Switch)(/*[in]*/ int nSwitchNo, /*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Switch)(/*[in]*/ int nSwitchNo, /*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_Solenoid)(/*[in]*/ int nSolenoid, /*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Lamp)(/*[in]*/ int nLamp, /*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(ShowAboutDialog)(/*[in]*/ long hParentWnd=0);
	STDMETHOD(Stop)();
	STDMETHOD(Run)(/*[in,defaultvalue(0)]*/ long hParentWnd=0, /*[in,defaultvalue(100)]*/ int nMinVersion=0);
	STDMETHOD(get_Hidden)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Hidden)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_MechSamples)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_MechSamples)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_Game)(/*[out, retval]*/ IGame * *pVal);
	STDMETHOD(GetWindowRect)(/*[in,defaultvalue(0)]*/ long hWnd, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(GetClientRect)(/*[in,defaultvalue(0)]*/ long hWnd, /*[out, retval]*/ VARIANT *pVal);

/* depricated methods/properties */
	STDMETHOD(get_NewSoundCommands)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_ImgDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ImgDir)(/*[in]*/ BSTR newVal);
	STDMETHOD(ShowPathesDialog)(/*[in,defaultvalue(0)]*/ long hParentWnd);
	STDMETHOD(get_Antialias)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Antialias)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(SetDisplayPosition)(/*[in]*/ int x, /*[in]*/ int y, /*[in]*/ long hParentWindow);
	STDMETHOD(get_DoubleSize)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_DoubleSize)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_SampleRate)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_SampleRate)(/*[in]*/ int newVal);
	STDMETHOD(get_WindowPosY)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_WindowPosY)(/*[in]*/ int newVal);
	STDMETHOD(get_WindowPosX)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_WindowPosX)(/*[in]*/ int newVal);
	STDMETHOD(get_BorderSizeY)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_BorderSizeY)(/*[in]*/ int newVal);
	STDMETHOD(get_BorderSizeX)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_BorderSizeX)(/*[in]*/ int newVal);
	STDMETHOD(get_ShowFrame)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowFrame)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ShowDMDOnly)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowDMDOnly)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(CheckROMS)(/*[in,defaultvalue(0)]*/ int nShowOptions, /*[in,defaultvalue(0)]*/ long hParentWnd, /*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_ShowTitle)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowTitle)(/*[in]*/ VARIANT_BOOL newpVal);
	STDMETHOD(get_UseSamples)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UseSamples)(/*[int]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_InstallDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_CfgDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_CfgDir)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SamplesDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SamplesDir)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_NVRamDir)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_NVRamDir)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_RomDirs)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_RomDirs)(/*[in]*/ BSTR newVal);
	STDMETHOD(ShowOptsDialog)(/*[in]*/ long hParentWnd=0);
};

#endif // !defined(AFX_Controller_H__D2811491_40D6_4656_9AA7_8FF85FD63543__INCLUDED_)
