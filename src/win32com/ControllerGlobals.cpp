// all global variable, which will be accesed by the mame core and the COM object

#include "StdAfx.h"
#include "VPinMAME.h"
#include "Controller.h"
#include "ControllerOptions.h"

extern "C" {
HWND		g_hMainWnd			= 0;					// handle to main window
BOOL		g_fActivateWindow	= FALSE;				// Set this to TRUE if COM Window should become the ACTIVE window!
int			g_fHandleKeyboard   = TRUE;					// Signals wpc core to handle the keyboard
int			g_fHandleMechanics  = FALSE;				// Signals wpc core to handle the mechanics for use
HANDLE		g_hGameRunning		= INVALID_HANDLE_VALUE; // Event handle used in osd_update_video_and_audio() to pause/resume the emulation

BOOL g_bOsDebug;
BOOL g_fSTAModel = FALSE;	// Are we using STA threading model?
int verbose;

Controller*	g_pController;								// we need this, if we call OnSolenoid from the wpc core

PCONTROLLEROPTIONS	g_pControllerOptions = NULL;
PCONTROLLERPATHS	g_pControllerPaths  = NULL;
}
