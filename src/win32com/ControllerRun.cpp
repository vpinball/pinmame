// ControllerRun.cpp : Implementation of the Controller.Run method
#include "stdafx.h"
#include "atlwin.h"
#include "mmsystem.h"

extern "C" {
#include "mame.h"
#include "driver.h"
#include "./windows/Window.h"
}

#include "VPinMAME_h.h"
#include "VPinMAMEAboutDlg.h"
#include "VPinMAMEConfig.h"

#include "Controller.h"
#include "ControllerRegKeys.h"
#include "ControllerGameSettings.h"
#include "ControllerSplashWnd.h"
#include "Resource.h"

extern "C" {
int	g_fHandleKeyboard   = TRUE;		// Signals wpc core to handle the keyboard
int	g_fHandleMechanics  = FALSE;	// Signals wpc core to handle the mechanics for use
int g_fMechSamples      = TRUE;		// Signal the common library to load the mech samples
HANDLE g_hGameRunning	= INVALID_HANDLE_VALUE;
int volatile g_fPause   = 0;		// referenced in usrintf.c to pause the game

int    g_iSyncFactor     = 0;
HANDLE g_hEnterThrottle  = INVALID_HANDLE_VALUE;
extern int g_iSyncFactor;
}

extern int dmd_border;
extern int dmd_title;
extern int dmd_pos_x;
extern int dmd_pos_y;
extern int dmd_doublesize;
extern int dmd_width;
extern int dmd_height;

extern int threadpriority;
extern int synclevel;

#define VPINMAMEONEVENTMSG	"VPinMAMEOnEventMsg"

#define EVENT_ONSOLENOID		0
#define EVENT_ONSTATECHANGE		1

// we need this, if we call OnSolenoid from the wpc core
static CController*	m_pController = NULL;

BOOL IsEmulationRunning()
{
	if ( !m_pController )
		return FALSE;

	return (WaitForSingleObject(m_pController->m_hThreadRun, 0)==WAIT_TIMEOUT)?TRUE:FALSE;
}

// these two functions are called from within PinMAME

extern "C" void OnSolenoid(int nSolenoid, int IsActive)  {
	if ( !m_pController )
		return;

	// post the call to the event window (which is created in the thread of the host app)
	if ( IsWindow(m_pController->m_hEventWnd) )
		PostMessage(m_pController->m_hEventWnd, RegisterWindowMessage(VPINMAMEONEVENTMSG), EVENT_ONSOLENOID, (WPARAM) ((IsActive?0x10000:0x00000) + nSolenoid));
}

extern "C" void	OnStateChange(int nState)  {
	if ( !m_pController )
		return;

	if ( nState ) {
		SetEvent(m_pController->m_hEmuIsRunning);
	}
	else
		ResetEvent(m_pController->m_hEmuIsRunning);

	// post the call to the event window (which is created in the thread of the host app)
	if ( IsWindow(m_pController->m_hEventWnd) )
		PostMessage(m_pController->m_hEventWnd, RegisterWindowMessage(VPINMAMEONEVENTMSG), EVENT_ONSTATECHANGE, (WPARAM) nState);
}

DWORD FAR PASCAL CController::RunController(CController* pController)
{
	if ( !pController )
		return 0;

	VARIANT_BOOL fIsSupported;
	pController->m_pGame->get_IsSupported(&fIsSupported);
	if ( fIsSupported==VARIANT_FALSE ) {
		MessageBox(pController->m_hParentWnd, "This game is not supported by Visual PinMAME", "Failed to start", MB_ICONINFORMATION|MB_OK);
		return 1;
	}

	g_fHandleMechanics = pController->m_iHandleMechanics;
	g_fHandleKeyboard  = pController->m_fHandleKeyboard;
	g_fMechSamples = pController->m_fMechSamples;

	// Load the game specific settings
	LoadGameSettings(pController->m_szROM);

	// set some options for the mamew environment
	// set_option("window", "1", 0);
	set_option("resolution", "1x1x16", 0);
	set_option("debug_resolution", "640x480x16", 0);
	set_option("maximize", "0", 0);
	set_option("throttle", "1", 0);
	set_option("sleep", "1", 0);
	set_option("autoframeskip", "0", 0);
	set_option("skip_gameinfo", "1", 0);
	set_option("skip_disclaimer", "1", 0);
	set_option("keepaspect", "0", 0);

	VARIANT fHasSound;
	VariantInit(&fHasSound);
	pController->m_pGameSettings->get_Value(CComBSTR("sound"), &fHasSound);
	if (fHasSound.boolVal==VARIANT_FALSE)
		options.samplerate = 0; // indicates game sound disabled

#ifndef DEBUG
	// display the splash screen
	void* pSplashWnd = NULL;
	CreateSplashWnd(&pSplashWnd, pController->m_szSplashInfoLine);
#endif

	// set the global pointer to Controller
	m_pController = pController;

	g_fPause = 0;

	int iSyncLevel = synclevel;
	if ( iSyncLevel<0 )
		iSyncLevel = 0;
	else if ( iSyncLevel>60 )
		iSyncLevel = 60;

	if ( iSyncLevel ) {
		if ( iSyncLevel<=20 )
			g_iSyncFactor = 1024;
		else
			g_iSyncFactor = (int) (1024.0*(iSyncLevel/60.0));

		g_hEnterThrottle = CreateEvent(NULL, false, true, NULL);
	}
#ifndef DEBUG
	else {
		// just in case
		g_iSyncFactor = 0;
		g_hEnterThrottle = INVALID_HANDLE_VALUE;

		switch ( threadpriority ) {
			case 1:
				SetThreadPriority(pController->m_hThreadRun, THREAD_PRIORITY_ABOVE_NORMAL);
				break;
			case 2:
				SetThreadPriority(pController->m_hThreadRun, THREAD_PRIORITY_HIGHEST);
				break;
		}
	}
#endif

	vpm_game_init(pController->m_nGameNo);
	run_game(pController->m_nGameNo);
	vpm_game_exit(pController->m_nGameNo);

	if ( iSyncLevel ) {
		SetEvent(g_hEnterThrottle);
		CloseHandle(g_hEnterThrottle);
		g_hEnterThrottle = INVALID_HANDLE_VALUE;
		g_iSyncFactor = 0;
	}
	else
		SetThreadPriority(pController->m_hThreadRun, THREAD_PRIORITY_NORMAL);

	// fire the OnMachineStopped event
	OnStateChange(0);

	// reset the global pointer to Controller
	m_pController = NULL;


#ifndef DEBUG
	// destroy the splash screensync
	DestroySplashWnd(&pSplashWnd);
#endif

	return 0;
}

//============================================================
//	osd_init
//============================================================

extern HWND win_video_window;

extern "C" int osd_init(void)
{
	extern int win_init_input(void);
	int result;

	result = win_init_window();
	if (result == 0)
		result = win_init_input();

	SetWindowPos(win_video_window, HWND_TOPMOST,
		0,
		0,
		0,
		0,
		SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

	RECT windowRect;
	GetClientRect(win_video_window, &windowRect);

	int maxX = GetSystemMetrics(SM_CXSCREEN) - windowRect.right;
	int maxY = GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom;

	if ( dmd_pos_x<0 )
		dmd_pos_x = 0;
	else if ( dmd_pos_x>maxX)
		dmd_pos_x = maxX;

	CComVariant vValueX(dmd_pos_x);
	m_pController->m_pGameSettings->put_Value(CComBSTR("dmd_pos_x"), vValueX);

	if ( dmd_pos_y<0 )
		dmd_pos_y = 0;
	else if ( dmd_pos_y>maxY)
		dmd_pos_y = maxY;

	CComVariant vValueY(dmd_pos_y);
	m_pController->m_pGameSettings->put_Value(CComBSTR("dmd_pos_y"), vValueY);

	return result;
}

//============================================================
//	osd_exit
//============================================================

extern "C" void osd_exit(void)
{
	extern void win_shutdown_input(void);
	win_shutdown_input();
	osd_set_leds(0);
}

// event window

static BOOL fEventWindowClassCreated = false;
// event window

LRESULT CALLBACK EventWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int uVPinMAMEOnEventMessage = 0;
	static CController* pController  = NULL;

	switch ( uMsg ) {
	case WM_CREATE:
		pController = (CController*) ((LPCREATESTRUCT) lParam)->lpCreateParams;
		uVPinMAMEOnEventMessage = RegisterWindowMessage(VPINMAMEONEVENTMSG);
		break;

	default:
		if ( uMsg==uVPinMAMEOnEventMessage ) {
			switch ( wParam ) {
			case EVENT_ONSOLENOID:
				pController->Fire_OnSolenoid(LOWORD(lParam),HIWORD(lParam));
				break;

			case EVENT_ONSTATECHANGE:
				pController->Fire_OnStateChange(lParam);
				break;
			}

			return true;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CreateEventWindow(CController* pController)
{
	if ( !pController )
		return;

	if ( IsWindow(pController->m_hEventWnd) )
		return;

	if ( !fEventWindowClassCreated )
	{
		WNDCLASS wc = { 0 };

		// initialize the description of the window class
		wc.lpszClassName 	= "VPINMAMEEVENTWND";
		wc.hInstance 		= _Module.m_hInst;
		wc.lpfnWndProc		= EventWindowProc;
		wc.hCursor			= NULL;
		wc.hIcon			= NULL;
		wc.lpszMenuName		= NULL;
		wc.hbrBackground	= NULL;
		wc.style			= 0;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;

		// register the class; fail if we can't
		fEventWindowClassCreated = RegisterClass(&wc);
	}

	pController->m_hEventWnd = CreateWindow( "VPINMAMEEVENTWND", "", 0, 0, 0, 0, 0, NULL, NULL, _Module.m_hInst, (LPVOID*) pController);
}

void DestroyEventWindow(CController* pController)
{
	if ( !pController )
		return;

	if ( !IsWindow(pController->m_hEventWnd) )
		return;

	DestroyWindow(pController->m_hEventWnd);

	pController->m_hEventWnd = NULL;
}


//============================================================
//	window hook to the main window and some helper functions
//============================================================

// set the position of the window according to the values save in the registry for the game
void AdjustWindowPosition(HWND hWnd, CController *pController)
{
	int xpos, ypos;

	char szKey[256];
	GetGameRegistryKey(szKey, pController->m_szROM);

	xpos = dmd_pos_x;
	ypos = dmd_pos_y;

	if ( pController->m_fWindowHidden )
		SetWindowPos(hWnd, 0, xpos, ypos, 0, 0, SWP_NOSIZE|SWP_HIDEWINDOW);
	else
		SetWindowPos(hWnd, HWND_TOPMOST, xpos, ypos, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);

	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}

// save the current window position to the registry, based on the game name
void SaveWindowPosition(HWND hWnd, CController *pController)
{
	RECT Rect;
	GetWindowRect(hWnd, &Rect);

	dmd_pos_x = Rect.left;
	CComVariant vValueX(dmd_pos_x);
	pController->m_pGameSettings->put_Value(CComBSTR("dmd_pos_x"), vValueX);

	dmd_pos_y = Rect.top;
	CComVariant vValueY(dmd_pos_y);
	pController->m_pGameSettings->put_Value(CComBSTR("dmd_pos_y"), vValueY);

	GetClientRect(hWnd, &Rect);
	if ( dmd_doublesize )
		Rect.right /= 2;

	if ( dmd_doublesize )
		Rect.bottom /= 2;

	vValueX = Rect.right;
	pController->m_pGameSettings->put_Value(CComBSTR("dmd_width"), vValueX);

	vValueY = Rect.bottom;
	pController->m_pGameSettings->put_Value(CComBSTR("dmd_height"), vValueY);
}

// set the window style: 0: title (includes border), 1: only border, 2: without border
void SetWindowStyle(HWND hWnd, int iWindowStyle)
{
	long lNewStyle = GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE;

	if ( IsWindow(GetParent(hWnd)) ) {
		switch (iWindowStyle) {
		case 0:
			lNewStyle |= WS_CHILD|WS_THICKFRAME|WS_CAPTION;
			break;

		case 1:
			lNewStyle |= WS_CHILD|WS_BORDER;
			break;

		case 2:
			lNewStyle |= WS_CHILD;
			break;

		default:
			return;
		}
	}
	else {
		switch (iWindowStyle) {
		case 0:
			lNewStyle |= WS_OVERLAPPED|WS_THICKFRAME|WS_CAPTION;
			break;

		case 1:
			lNewStyle |= WS_OVERLAPPED|WS_THICKFRAME;
			break;

		case 2:
			lNewStyle |= WS_OVERLAPPED;
			break;

		default:
			return;
		}
	}

	RECT Rect;
	SetWindowLong(hWnd, GWL_STYLE, lNewStyle);
	GetClientRect(hWnd,  &Rect);

	int iWidth = Machine->uiwidth;
	if ( dmd_width>0 )
		iWidth = dmd_width;

	int iHeight = Machine->uiheight;
	if ( dmd_height>0 )
		iHeight = dmd_height;

	Rect.right = iWidth * (dmd_doublesize?2:1);
	Rect.bottom = iHeight * (dmd_doublesize?2:1);;

	AdjustWindowRect(&Rect, lNewStyle, FALSE);

	SetWindowPos(hWnd, 0, 0, 0, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);

	// is that really necessary?
	if ( m_pController->m_fWindowHidden )
		ShowWindow(hWnd, SW_HIDE);
}

// osd_hook function, will be called by the window function for the video window
// each time a message was received and handled. Set pfhandled to true if the
// return value of this function should be used. If you set it to false (default)
// in some cases DefWindowProc will be called by the video window handler

extern "C" LRESULT CALLBACK osd_hook(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam, BOOL *pfhandled)
{
	LRESULT iResult = 0;

	static CController* pController = NULL;

	static UINT uVPinMAMEAdjustWindowMsg = 0;

	static BOOL fMoveWindow = FALSE;
	static int iWindowMovedX = 0;
	static int iWindowMovedY = 0;

	static int iWindowStyle = -1;

	switch ( message ) {

	// Only some inits
	case WM_CREATE:
		pController = m_pController;
		uVPinMAMEAdjustWindowMsg = RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG);

		*pfhandled = TRUE;
		break;

	// Clear var for Move window as user drags it with left mouse button!
	case WM_LBUTTONUP:
		ReleaseCapture();
		fMoveWindow = FALSE;

		// save the new position to the registry
		SaveWindowPosition(wnd, pController);

		*pfhandled = TRUE;
		break;

    // Track vars for Move window as user drags it with left mouse button!
	case WM_LBUTTONDOWN:
		SetCapture(wnd);
		fMoveWindow = TRUE;

		RECT windowRect, clientRect;
		GetWindowRect(wnd, &windowRect);
		GetClientRect(wnd, &clientRect);
		ClientToScreen(wnd, (LPPOINT) &clientRect.left);
		ClientToScreen(wnd, (LPPOINT) &clientRect.right);

		iWindowMovedX = LOWORD(lparam) - (windowRect.left-clientRect.left);
		iWindowMovedY = HIWORD(lparam) - (windowRect.top-clientRect.top);

		*pfhandled = TRUE;
		break;

	// Move window as user drags it with left mouse button!
	case WM_MOUSEMOVE:
		if ( fMoveWindow ) {
			POINT pt = {(short) LOWORD(lparam), (short) HIWORD(lparam)};
			ClientToScreen(wnd, &pt);
			SetWindowPos(wnd, 0, pt.x-iWindowMovedX, pt.y-iWindowMovedY, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}

		*pfhandled = TRUE;
		break;

	// Show context menu if the user uses the right mouse button
	case WM_RBUTTONUP:
		// Get the menu from the resource
		HMENU hMenu;
                hMenu = GetSubMenu(LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_CTXMENU)), 0);

		// Check the proper entry for the current display mode
		switch ( iWindowStyle ) {
		case 0: // title + border
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_TITLE, MF_BYCOMMAND|MF_CHECKED);
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_BORDER, MF_BYCOMMAND|MF_CHECKED);
			break;

		case 1: // no title
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_TITLE, MF_BYCOMMAND|MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_BORDER, MF_BYCOMMAND|MF_CHECKED);
			break;

		case 2: // borderless
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_TITLE, MF_BYCOMMAND|MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_CTRLCTXMENU_DISPLAY_BORDER, MF_BYCOMMAND|MF_UNCHECKED);
			break;
		}

		POINT Pos;
		Pos.x = LOWORD(lparam);
		Pos.y = HIWORD(lparam);
		ClientToScreen(wnd, &Pos);

		TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, Pos.x, Pos.y, 0, wnd, NULL);

		*pfhandled = TRUE;
		break;

	// If the display is locked, return the active window state
	// to the previous one
	case WM_ACTIVATE:
		if ( !pController->m_fDisplayLocked )
			return 0;

		if ( wparam==WA_INACTIVE )
			return 0;

		::SetActiveWindow((HWND) lparam);
		*pfhandled = TRUE;
		break;

	// On destroy, save the current window position for future use
	case WM_DESTROY:
		SaveWindowPosition(wnd, pController);
		*pfhandled = TRUE;
		break;

	// handle the messages from the context menu
	case WM_COMMAND:
		switch ( LOWORD(wparam) ) {
		case ID_CTRLCTXMENU_GAMEOPTIONS:
			pController->m_pGameSettings->ShowSettingsDlg(0);

			*pfhandled = TRUE;
			break;

		case ID_CTRLCTXMENU_PATHES:
			pController->m_pControllerSettings->ShowSettingsDlg(0);

			*pfhandled = TRUE;
			break;

		case ID_CTRLCTXMENU_DISPLAY_TITLE:
			{
				CComVariant vValue((VARIANT_BOOL) VARIANT_TRUE);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_border"), vValue);
                                vValue = (VARIANT_BOOL)((iWindowStyle == 0) ? VARIANT_FALSE : VARIANT_TRUE);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_title"), vValue);
			}

			*pfhandled = TRUE;
			break;

		case ID_CTRLCTXMENU_DISPLAY_BORDER:
			{
				CComVariant vValue((VARIANT_BOOL) VARIANT_FALSE);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_title"), vValue);
                                vValue = (VARIANT_BOOL)((iWindowStyle == 2) ? VARIANT_TRUE : VARIANT_FALSE);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_border"), vValue);
			}

			*pfhandled = TRUE;
			break;
#if 0 // not needed in new flat menu
		case ID_CTRLCTXMENU_DISPLAY_DMDONLY:
			{
				CComVariant vValue((VARIANT_BOOL) VARIANT_FALSE);

				pController->m_pGameSettings->put_Value(CComBSTR("dmd_title"), vValue);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_border"), vValue);
			}

			*pfhandled = TRUE;
			break;
#endif
		case ID_CTRLCTXMENU_DISPLAY_RESTORESIZE:
			{
				CComVariant vValue((int) 0);

				pController->m_pGameSettings->put_Value(CComBSTR("dmd_width"), vValue);
				pController->m_pGameSettings->put_Value(CComBSTR("dmd_height"), vValue);

				*pfhandled = TRUE;
			}
			break;

		case ID_CTRLCTXMENU_INFO:
			ShowAboutDlg(wnd);

			*pfhandled = TRUE;
			break;

		case ID_CTRLCTXMENU_STOPEMULATION:
			PostMessage(wnd, WM_CLOSE, 0, 0);

			*pfhandled = TRUE;
			break;
		}
	}

	// adjust the window layout
	if ( message==uVPinMAMEAdjustWindowMsg ) {
		iWindowStyle = 0;

		if ( !dmd_title ) {
			if ( dmd_border )
				iWindowStyle = 1; // Border without title
			else
				iWindowStyle = 2; // No border
		}

		SetWindowStyle(wnd, iWindowStyle);
		AdjustWindowPosition(wnd, pController);

		*pfhandled = TRUE;
		iResult = 1;
	}

	return iResult;
}

// special hook for VPM
extern "C" void VPM_ShowVideoWindow()
{
	if ( m_pController==NULL )
		return;

	SendMessage(win_video_window, RegisterWindowMessage(VPINMAMEADJUSTWINDOWMSG), 0, 0);
	if ( IsWindow(m_pController->m_hParentWnd) )
		SetForegroundWindow(m_pController->m_hParentWnd);

	if ( !m_pController->m_fWindowHidden )
		ShowWindow(win_video_window, SW_SHOWNOACTIVATE);
	else
		ShowWindow(win_video_window, SW_HIDE);
}
