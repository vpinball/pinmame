/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "osdepend.h"

#define OSD_MAX_VALUE (OSD_MAX_PSEUDO)

struct OSDKeyboard
{
    int             (*init)(options_type *options);
    void            (*exit)(void);
    const struct KeyboardInfo * (*get_key_list)(void);
    void            (*customize_inputport_defaults)(struct ipd *defaults);
    int             (*is_key_pressed)(int keycode);
    int             (*readkey_unicode)(int flush);

    BOOL            (*OnMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

extern struct OSDKeyboard Keyboard;

extern void Keyboard_CustomizeInputportDefaults(int DefaultInput, struct ipd *defaults);

#endif
