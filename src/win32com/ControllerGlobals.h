#ifndef CONSTROLLERGLOBALS_H
#define CONSTROLLERGLOBALS_H

#ifdef __cplusplus
extern "C" {
#endif

extern HWND			g_hMainWnd;			// handle to main window
extern BOOL			g_fActivateWindow;	// Set this to TRUE if COM Window should become the ACTIVE window!
extern int			g_fHandleMechanics;	// Signals wpc core to handle the mechanics for use
extern int			g_fHandleKeyboard;  // Signals wpc core to handle the keyboard
extern HANDLE		g_hGameRunning;		// Event handle used in osd_update_video_and_audio() to pause/resume the emulation

extern Controller*	g_pController;		// we need this, if we call OnSolenoid from the wpc core

#ifdef __cplusplus
}
#endif


#endif // CONSTROLLERGLOBALS_H
