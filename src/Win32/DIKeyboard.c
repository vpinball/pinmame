/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  DIKeyboard.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <dinput.h>
#include "MAME32.h"
#include "M32Util.h"
#include "DirectInput.h"
#include "DIKeyboard.h"
#include "DIJoystick.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int          DIKeyboard_init(options_type *options);
static void         DIKeyboard_exit(void);

static const struct KeyboardInfo * DIKeyboard_get_key_list(void);
static void         DIKeyboard_customize_inputport_defaults(struct ipd *defaults);
static int          DIKeyboard_is_key_pressed(int keycode);
static int          DIKeyboard_readkey_unicode(int flush);

static BOOL         DIKeyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void         OnActivateApp(HWND hWnd, BOOL fActivate, DWORD dwThreadId);
static void         OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static void         OnChar(HWND hWnd, TCHAR ch, int cRepeat);

static void         DIKeyboard_PollKeyboard(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDKeyboard  DIKeyboard = 
{
    DIKeyboard_init,                         /* init                         */
    DIKeyboard_exit,                         /* exit                         */
    DIKeyboard_get_key_list,                 /* get_key_list                 */
    DIKeyboard_customize_inputport_defaults, /* customize_inputport_defaults */
    DIKeyboard_is_key_pressed,               /* is_key_pressed               */
    DIKeyboard_readkey_unicode,              /* readkey_unicode              */

    DIKeyboard_OnMessage,                    /* OnMessage                    */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tKeyboard_private
{
    LPDIRECTINPUTDEVICE  m_didKeyboard;
    byte                 m_key[256];
    int                  m_DefaultInput;
    TCHAR                m_chPressed;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tKeyboard_private This;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int DIKeyboard_init(options_type *options)
{
    HRESULT hr;
    int i;

    This.m_didKeyboard  = NULL;
    This.m_DefaultInput = options->default_input;
    This.m_chPressed    = 0;

    for (i = 0; i < 256; i++)
        This.m_key[i] = 0;


    if (di == NULL)
    {
        ErrorMsg("DirectInput not initialized");
        return 1;
    }

    /* setup the keyboard */
    hr = IDirectInput_CreateDevice(di, &GUID_SysKeyboard, &This.m_didKeyboard, NULL);

    if (FAILED(hr)) 
    {
        ErrorMsg("DirectInputCreateDevice failed!\n");
        return 1;
    }
   
    hr = IDirectInputDevice_SetDataFormat(This.m_didKeyboard, &c_dfDIKeyboard);

    if (FAILED(hr)) 
    {
        ErrorMsg("DirectInputDevice SetDataFormat failed\n");
        return 1;
    }
   
    hr = IDirectInputDevice_SetCooperativeLevel(This.m_didKeyboard, MAME32App.m_hWnd,
                                                DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
   
    if (FAILED(hr)) 
    {
        ErrorMsg("DirectInputDevice SetCooperativeLevel failed!\n");
        return 1;
    }
   
    hr = IDirectInputDevice_Acquire(This.m_didKeyboard);

    return 0;
}

/*
   put here cleanup routines to be executed when the program is terminated.
 */
static void DIKeyboard_exit(void)
{
    if (!This.m_didKeyboard)
        return;

    /*
     Cleanliness is next to godliness.  Unacquire the device
     one last time just in case we got really confused and tried
     to exit while the device is still acquired.
     */
    IDirectInputDevice_Unacquire(This.m_didKeyboard);
    IDirectInputDevice_Release(This.m_didKeyboard);

    This.m_didKeyboard = NULL;
}

static struct KeyboardInfo keylist[] =
{
    { "A",          DIK_A,              KEYCODE_A },
    { "B",          DIK_B,              KEYCODE_B },
    { "C",          DIK_C,              KEYCODE_C },
    { "D",          DIK_D,              KEYCODE_D },
    { "E",          DIK_E,              KEYCODE_E },
    { "F",          DIK_F,              KEYCODE_F },
    { "G",          DIK_G,              KEYCODE_G },
    { "H",          DIK_H,              KEYCODE_H },
    { "I",          DIK_I,              KEYCODE_I },
    { "J",          DIK_J,              KEYCODE_J },
    { "K",          DIK_K,              KEYCODE_K },
    { "L",          DIK_L,              KEYCODE_L },
    { "M",          DIK_M,              KEYCODE_M },
    { "N",          DIK_N,              KEYCODE_N },
    { "O",          DIK_O,              KEYCODE_O },
    { "P",          DIK_P,              KEYCODE_P },
    { "Q",          DIK_Q,              KEYCODE_Q },
    { "R",          DIK_R,              KEYCODE_R },
    { "S",          DIK_S,              KEYCODE_S },
    { "T",          DIK_T,              KEYCODE_T },
    { "U",          DIK_U,              KEYCODE_U },
    { "V",          DIK_V,              KEYCODE_V },
    { "W",          DIK_W,              KEYCODE_W },
    { "X",          DIK_X,              KEYCODE_X },
    { "Y",          DIK_Y,              KEYCODE_Y },
    { "Z",          DIK_Z,              KEYCODE_Z },
    { "0",          DIK_0,              KEYCODE_0 },
    { "1",          DIK_1,              KEYCODE_1 },
    { "2",          DIK_2,              KEYCODE_2 },
    { "3",          DIK_3,              KEYCODE_3 },
    { "4",          DIK_4,              KEYCODE_4 },
    { "5",          DIK_5,              KEYCODE_5 },
    { "6",          DIK_6,              KEYCODE_6 },
    { "7",          DIK_7,              KEYCODE_7 },
    { "8",          DIK_8,              KEYCODE_8 },
    { "9",          DIK_9,              KEYCODE_9 },
    { "0 PAD",      DIK_NUMPAD0,        KEYCODE_0_PAD },
    { "1 PAD",      DIK_NUMPAD1,        KEYCODE_1_PAD },
    { "2 PAD",      DIK_NUMPAD2,        KEYCODE_2_PAD },
    { "3 PAD",      DIK_NUMPAD3,        KEYCODE_3_PAD },
    { "4 PAD",      DIK_NUMPAD4,        KEYCODE_4_PAD },
    { "5 PAD",      DIK_NUMPAD5,        KEYCODE_5_PAD },
    { "6 PAD",      DIK_NUMPAD6,        KEYCODE_6_PAD },
    { "7 PAD",      DIK_NUMPAD7,        KEYCODE_7_PAD },
    { "8 PAD",      DIK_NUMPAD8,        KEYCODE_8_PAD },
    { "9 PAD",      DIK_NUMPAD9,        KEYCODE_9_PAD },
    { "F1",         DIK_F1,             KEYCODE_F1 },
    { "F2",         DIK_F2,             KEYCODE_F2 },
    { "F3",         DIK_F3,             KEYCODE_F3 },
    { "F4",         DIK_F4,             KEYCODE_F4 },
    { "F5",         DIK_F5,             KEYCODE_F5 },
    { "F6",         DIK_F6,             KEYCODE_F6 },
    { "F7",         DIK_F7,             KEYCODE_F7 },
    { "F8",         DIK_F8,             KEYCODE_F8 },
    { "F9",         DIK_F9,             KEYCODE_F9 },
    { "F10",        DIK_F10,            KEYCODE_F10 },
    { "F11",        DIK_F11,            KEYCODE_F11 },
    { "F12",        DIK_F12,            KEYCODE_F12 },
    { "ESC",        DIK_ESCAPE,         KEYCODE_ESC },
    { "~",          DIK_GRAVE,          KEYCODE_TILDE },
    { "-",          DIK_MINUS,          KEYCODE_MINUS },
    { "=",          DIK_EQUALS,         KEYCODE_EQUALS },
    { "BKSPACE",    DIK_BACKSPACE,      KEYCODE_BACKSPACE },
    { "TAB",        DIK_TAB,            KEYCODE_TAB },
    { "[",          DIK_LBRACKET,       KEYCODE_OPENBRACE },
    { "]",          DIK_RBRACKET,       KEYCODE_CLOSEBRACE },
    { "ENTER",      DIK_RETURN,         KEYCODE_ENTER },
    { ";",          DIK_COLON,          KEYCODE_COLON },
    { ":",          DIK_APOSTROPHE,     KEYCODE_QUOTE },
    { "\\",         DIK_BACKSLASH,      KEYCODE_BACKSLASH },
    { ",",          DIK_COMMA,          KEYCODE_COMMA },
    { ".",          DIK_PERIOD,         KEYCODE_STOP },
    { "/",          DIK_SLASH,          KEYCODE_SLASH },
    { "SPACE",      DIK_SPACE,          KEYCODE_SPACE },
    { "INS",        DIK_INSERT,         KEYCODE_INSERT },
    { "DEL",        DIK_DELETE,         KEYCODE_DEL },
    { "HOME",       DIK_HOME,           KEYCODE_HOME },
    { "END",        DIK_END,            KEYCODE_END },
    { "PGUP",       DIK_PGUP,           KEYCODE_PGUP },
    { "PGDN",       DIK_PGDN,           KEYCODE_PGDN },
    { "LEFT",       DIK_LEFT,           KEYCODE_LEFT },
    { "RIGHT",      DIK_RIGHT,          KEYCODE_RIGHT },
    { "UP",         DIK_UP,             KEYCODE_UP },
    { "DOWN",       DIK_DOWN,           KEYCODE_DOWN },
    { "/ PAD",      DIK_DIVIDE,         KEYCODE_SLASH_PAD },
    { "* PAD",      DIK_MULTIPLY,       KEYCODE_ASTERISK },
    { "- PAD",      DIK_SUBTRACT,       KEYCODE_MINUS_PAD },
    { "+ PAD",      DIK_ADD,            KEYCODE_PLUS_PAD },
    { ". PAD",      DIK_DECIMAL,        KEYCODE_DEL_PAD },
    { "ENTER PAD",  DIK_NUMPADENTER,    KEYCODE_ENTER_PAD },
    /*{ "PRTSCR",       DIK_PRTSCR,         KEYCODE_PRTSCR },*/
    /*{ "PAUSE",        DIK_PAUSE,          KEYCODE_PAUSE },*/
    { "LSHIFT",     DIK_LSHIFT,         KEYCODE_LSHIFT },
    { "RSHIFT",     DIK_RSHIFT,         KEYCODE_RSHIFT },
    { "LCTRL",      DIK_LCONTROL,       KEYCODE_LCONTROL },
    { "RCTRL",      DIK_RCONTROL,       KEYCODE_RCONTROL },
    { "ALT",        DIK_LALT,           KEYCODE_LALT },
    { "ALTGR",      DIK_RALT,           KEYCODE_RALT },
    { "LWIN",       DIK_LWIN,           KEYCODE_OTHER },
    { "RWIN",       DIK_RWIN,           KEYCODE_OTHER },
    { "MENU",       DIK_APPS,           KEYCODE_OTHER },
    { "SCRLOCK",    DIK_SCROLL,         KEYCODE_SCRLOCK },
    { "NUMLOCK",    DIK_NUMLOCK,        KEYCODE_NUMLOCK },
    { "CAPSLOCK",   DIK_CAPSLOCK,       KEYCODE_CAPSLOCK },
    { 0, 0, 0 } /* end of table */
};

/*
  return a list of all available keys (see input.h)
*/
static const struct KeyboardInfo* DIKeyboard_get_key_list(void)
{
    return keylist;
}

/*
  inptport.c defines some general purpose defaults for key bindings. They may be
  further adjusted by the OS dependant code to better match the available keyboard,
  e.g. one could map pause to the Pause key instead of P, or snapshot to PrtScr
  instead of F12. Of course the user can further change the settings to anything
  he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys you want.
*/
static void DIKeyboard_customize_inputport_defaults(struct ipd *defaults)
{
    Keyboard_CustomizeInputportDefaults(This.m_DefaultInput, defaults);
}

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_customize_inputport_defaults().
*/
static int DIKeyboard_is_key_pressed(int keycode)
{
   if (keycode == DIK_ESCAPE && MAME32App.Done())
   {
      return 1;
   }

   MAME32App.ProcessMessages();

   DIKeyboard_PollKeyboard();

   return This.m_key[keycode] != 0;
}

static int DIKeyboard_readkey_unicode(int flush)
{
    if (flush)
    {
        This.m_chPressed = 0;
        return 0;
    }

    MAME32App.ProcessMessages();

    if (This.m_chPressed)
    {
        TCHAR   ch = This.m_chPressed;
        This.m_chPressed = 0;
        return ch;
    }
    return 0;
}

BOOL DIKeyboard_Available(void)
{
    HRESULT     hr;
    const GUID  guidNULL = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
    GUID        guidDevice = guidNULL;

    if (di == NULL)
    {
        return FALSE;
    }

    /* enumerate for keyboard devices */
    hr = IDirectInput_EnumDevices(di, DIDEVTYPE_KEYBOARD,
                                  inputEnumDeviceProc,
                                  &guidDevice,
                                  DIEDFL_ATTACHEDONLY);
    if (FAILED(hr))
    {
        return FALSE;
    }

    if (!IsEqualGUID(&guidDevice, &guidNULL))
    {
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************
    Message handlers
 ***************************************************************************/

static BOOL DIKeyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg)
    {
        PEEK_MESSAGE(hWnd, WM_ACTIVATEAPP,  OnActivateApp);
        HANDLE_MESSAGE(hWnd, WM_KEYDOWN,    OnKey);
        HANDLE_MESSAGE(hWnd, WM_KEYUP,      OnKey);
        HANDLE_MESSAGE(hWnd, WM_CHAR,       OnChar);
    }
    return FALSE;
}

static void OnActivateApp(HWND hWnd, BOOL fActivate, DWORD dwThreadId)
{
    if (!This.m_didKeyboard)
        return;
   
    if (MAME32App.m_bIsActive == TRUE)
        IDirectInputDevice_Acquire(This.m_didKeyboard);
    else
        IDirectInputDevice_Unacquire(This.m_didKeyboard);
}

static void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
}

static void OnChar(HWND hWnd, TCHAR ch, int cRepeat)
{
    This.m_chPressed = ch;
}

/***************************************************************************
  Internal functions
 ***************************************************************************/

static void DIKeyboard_PollKeyboard()
{
    HRESULT hr;

    if (This.m_didKeyboard == NULL)
       return;

again:
   
    hr = IDirectInputDevice_GetDeviceState(This.m_didKeyboard, sizeof(This.m_key), &This.m_key);
    if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) 
    {
        /*
            DirectInput is telling us that the input stream has
            been interrupted.  We aren't tracking any state
            between polls, so we don't have any special reset
            that needs to be done.  We just re-acquire and
            try again.
        */
        hr = IDirectInputDevice_Acquire(This.m_didKeyboard);
        if (SUCCEEDED(hr)) 
            goto again;
    }
}
