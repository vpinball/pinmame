/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __MAME32_H__
#define __MAME32_H__

#include "win32ui.h"

#if !defined(MAME32NAME)
#define MAME32NAME "MAME32"
#endif

#if !defined(MAMENAME)
#define MAMENAME "MAME"
#endif

#define HANDLE_MESSAGE(hwnd, message, fn)                               \
    case (message):                                                     \
    {                                                                   \
        *pResult = HANDLE_##message((hwnd), (wParam), (lParam), (fn));  \
        return TRUE;                                                    \
    }

#define PEEK_MESSAGE(hwnd, message, fn)                                 \
    case (message):                                                     \
    {                                                                   \
        *pResult = HANDLE_##message((hwnd), (wParam), (lParam), (fn));  \
        return FALSE;                                                   \
    }

struct tMAME32App
{
    HWND                    m_hwndUI;
    HWND                    m_hWnd;
    const char*             m_Name;

    BOOL                    m_bIsInitialized;
    BOOL                    m_bIsActive;
    BOOL                    m_bAutoPaused;
    BOOL                    m_bMamePaused;
    BOOL                    m_bDone;
    BOOL                    m_bMMXDetected;

    /*
        TRUE if using m_pTrak for standard analog inputs,
        FALSE if using m_pJoystick for standard analog inputs.
    */
    BOOL                    m_bUseAIMouse; 

    struct OSDDisplay*      m_pDisplay;
    struct OSDSound*        m_pSound;
    struct OSDKeyboard*     m_pKeyboard;
    struct OSDJoystick*     m_pJoystick;
    struct OSDTrak*         m_pTrak;

    struct OSDFMSynth*      m_pFMSynth;

#ifdef MAME_NET
    struct OSDNetwork*      m_pNetwork;
    BOOL                    m_bUseNetwork;
#endif /* MAME_NET */

    HWND                    (*CreateMAMEWindow)(void);
    void                    (*ProcessMessages)(void);
    BOOL                    (*PumpAndReturnMessage)(MSG* pMsg);
    void                    (*HandleAutoPause)(void);
    void                    (*Quit)(void);
    BOOL                    (*DetectMMX)(void);
    BOOL                    (*Done)(void);
};

extern void MAME32App_init(options_type *options);

extern struct tMAME32App MAME32App;

#endif
