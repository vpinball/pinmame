/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  DebugKeyboard.c

 ***************************************************************************/

#if defined(MAME_DEBUG)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "MAME32.h"
#include "M32Util.h"
#include "Keyboard.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int          DebugKeyboard_init(options_type *options);
static void         DebugKeyboard_exit(void);
static const struct KeyboardInfo * DebugKeyboard_get_key_list(void);
static void         DebugKeyboard_customize_inputport_defaults(struct ipd *defaults);
static int          DebugKeyboard_is_key_pressed(int keycode);
static int          DebugKeyboard_wait_keypress(void);
static int          DebugKeyboard_readkey_unicode(int flush);

static BOOL         DebugKeyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

static int  DebugKeyboard_Keypressed(void);
static void DebugKeyboard_flush(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDKeyboard DebugKeyboard = 
{
    { DebugKeyboard_init },                         /* init                         */
    { DebugKeyboard_exit },                         /* exit                         */
    { DebugKeyboard_get_key_list },                 /* get_key_list                 */
    { DebugKeyboard_customize_inputport_defaults }, /* customize_inputport_defaults */
    { DebugKeyboard_is_key_pressed },               /* is_key_pressed               */
    { DebugKeyboard_readkey_unicode },              /* readkey_unicode              */

    { DebugKeyboard_OnMessage }                     /* OnMessage                    */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static unsigned char m_bPressed[0x100];

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static int DebugKeyboard_init(options_type *options)
{
    memset(m_bPressed, 0x00, sizeof(m_bPressed));

    return 0;
}

static void DebugKeyboard_exit(void)
{
}

static struct KeyboardInfo keylist[] =
{
    { "A",          'A',                KEYCODE_A },
    { "B",          'B',                KEYCODE_B },
    { "C",          'C',                KEYCODE_C },
    { "D",          'D',                KEYCODE_D },
    { "E",          'E',                KEYCODE_E },
    { "F",          'F',                KEYCODE_F },
    { "G",          'G',                KEYCODE_G },
    { "H",          'H',                KEYCODE_H },
    { "I",          'I',                KEYCODE_I },
    { "J",          'J',                KEYCODE_J },
    { "K",          'K',                KEYCODE_K },
    { "L",          'L',                KEYCODE_L },
    { "M",          'M',                KEYCODE_M },
    { "N",          'N',                KEYCODE_N },
    { "O",          'O',                KEYCODE_O },
    { "P",          'P',                KEYCODE_P },
    { "Q",          'Q',                KEYCODE_Q },
    { "R",          'R',                KEYCODE_R },
    { "S",          'S',                KEYCODE_S },
    { "T",          'T',                KEYCODE_T },
    { "U",          'U',                KEYCODE_U },
    { "V",          'V',                KEYCODE_V },
    { "W",          'W',                KEYCODE_W },
    { "X",          'X',                KEYCODE_X },
    { "Y",          'Y',                KEYCODE_Y },
    { "Z",          'Z',                KEYCODE_Z },
    { "0",          '0',                KEYCODE_0 },
    { "1",          '1',                KEYCODE_1 },
    { "2",          '2',                KEYCODE_2 },
    { "3",          '3',                KEYCODE_3 },
    { "4",          '4',                KEYCODE_4 },
    { "5",          '5',                KEYCODE_5 },
    { "6",          '6',                KEYCODE_6 },
    { "7",          '7',                KEYCODE_7 },
    { "8",          '8',                KEYCODE_8 },
    { "9",          '9',                KEYCODE_9 },
    { "0 PAD",      VK_NUMPAD0,         KEYCODE_0_PAD },
    { "1 PAD",      VK_NUMPAD1,         KEYCODE_1_PAD },
    { "2 PAD",      VK_NUMPAD2,         KEYCODE_2_PAD },
    { "3 PAD",      VK_NUMPAD3,         KEYCODE_3_PAD },
    { "4 PAD",      VK_NUMPAD4,         KEYCODE_4_PAD },
    { "5 PAD",      VK_NUMPAD5,         KEYCODE_5_PAD },
    { "6 PAD",      VK_NUMPAD6,         KEYCODE_6_PAD },
    { "7 PAD",      VK_NUMPAD7,         KEYCODE_7_PAD },
    { "8 PAD",      VK_NUMPAD8,         KEYCODE_8_PAD },
    { "9 PAD",      VK_NUMPAD9,         KEYCODE_9_PAD },
    { "F1",         VK_F1,              KEYCODE_F1 },
    { "F2",         VK_F2,              KEYCODE_F2 },
    { "F3",         VK_F3,              KEYCODE_F3 },
    { "F4",         VK_F4,              KEYCODE_F4 },
    { "F5",         VK_F5,              KEYCODE_F5 },
    { "F6",         VK_F6,              KEYCODE_F6 },
    { "F7",         VK_F7,              KEYCODE_F7 },
    { "F8",         VK_F8,              KEYCODE_F8 },
    { "F9",         VK_F9,              KEYCODE_F9 },
    { "F10",        VK_F10,             KEYCODE_F10 },
    { "F11",        VK_F11,             KEYCODE_F11 },
    { "F12",        VK_F12,             KEYCODE_F12 },
    { "ESC",        VK_ESCAPE,          KEYCODE_ESC },
    { "~",          0xC0,               KEYCODE_TILDE },
    { "-",          0xBD,               KEYCODE_MINUS },
    { "=",          0xBB,               KEYCODE_EQUALS },
    { "BKSPACE",    VK_BACK,            KEYCODE_BACKSPACE },
    { "TAB",        VK_TAB,             KEYCODE_TAB },
    { "[",          0xDB,               KEYCODE_OPENBRACE },
    { "]",          0xDD,               KEYCODE_CLOSEBRACE },
    { "ENTER",      VK_RETURN,          KEYCODE_ENTER },
    { ";",          0xBA,               KEYCODE_COLON },
    { "'",          0xDE,               KEYCODE_QUOTE },
    { "\\",         0xDC,               KEYCODE_BACKSLASH },
    { ",",          0xBC,               KEYCODE_COMMA },
    { ".",          0xBE,               KEYCODE_STOP },
    { "/",          0xBF,               KEYCODE_SLASH },
    { "SPACE",      VK_SPACE,           KEYCODE_SPACE },
    { "INS",        VK_INSERT,          KEYCODE_INSERT },
    { "DEL",        VK_DELETE,          KEYCODE_DEL },
    { "HOME",       VK_HOME,            KEYCODE_HOME },
    { "END",        VK_END,             KEYCODE_END },
    { "PGUP",       VK_PRIOR,           KEYCODE_PGUP },
    { "PGDN",       VK_NEXT,            KEYCODE_PGDN },
    { "LEFT",       VK_LEFT,            KEYCODE_LEFT },
    { "RIGHT",      VK_RIGHT,           KEYCODE_RIGHT },
    { "UP",         VK_UP,              KEYCODE_UP },
    { "DOWN",       VK_DOWN,            KEYCODE_DOWN },
    { "/ PAD",      VK_DIVIDE,          KEYCODE_SLASH_PAD },
    { "* PAD",      VK_MULTIPLY,        KEYCODE_ASTERISK },
    { "- PAD",      VK_SUBTRACT,        KEYCODE_MINUS_PAD },
    { "+ PAD",      VK_ADD,             KEYCODE_PLUS_PAD },
    { ". PAD",      VK_DECIMAL,         KEYCODE_DEL_PAD },
    { "PRTSCR",     VK_PRINT,           KEYCODE_PRTSCR },
    { "PAUSE",      VK_PAUSE,           KEYCODE_PAUSE },
#if 0
    { "LSHIFT",     VK_LSHIFT,          KEYCODE_LSHIFT },
    { "RSHIFT",     VK_RSHIFT,          KEYCODE_RSHIFT },
    { "LCTRL",      VK_LCONTROL,        KEYCODE_LCONTROL },
    { "RCTRL",      VK_RCONTROL,        KEYCODE_RCONTROL },
    { "LALT",       VK_LMENU,           KEYCODE_LALT },
    { "RALT",       VK_RMENU,           KEYCODE_RALT },
#endif
    { "SHIFT",      VK_SHIFT,           KEYCODE_LSHIFT },
    { "CTRL",       VK_CONTROL,         KEYCODE_LCONTROL },
    { "ALT",        VK_MENU,            KEYCODE_LALT },
    { "LWIN",       VK_LWIN,            KEYCODE_OTHER },
    { "RWIN",       VK_RWIN,            KEYCODE_OTHER },
    { "MENU",       VK_APPS,            KEYCODE_OTHER },
    { "SCRLOCK",    VK_SCROLL,          KEYCODE_SCRLOCK },
    { "NUMLOCK",    VK_NUMLOCK,         KEYCODE_NUMLOCK },
    { "CAPSLOCK",   VK_CAPITAL,         KEYCODE_CAPSLOCK },
    { 0, 0, 0 } /* end of table */
};

static const struct KeyboardInfo* DebugKeyboard_get_key_list(void)
{
    return keylist;
}

static void DebugKeyboard_customize_inputport_defaults(struct ipd *defaults)
{
}

static int DebugKeyboard_is_key_pressed(int keycode)
{
    INPUT_RECORD ir;
    int read;

    while (DebugKeyboard_Keypressed())
    {
        if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read) && read == 1)
        {
            if (ir.EventType == KEY_EVENT)
            {
                if (ir.Event.KeyEvent.wVirtualKeyCode < sizeof(m_bPressed))
                {
                    m_bPressed[ir.Event.KeyEvent.wVirtualKeyCode] = ir.Event.KeyEvent.bKeyDown;
                }
            }
        }
        else
        {
            break;
        }
    }
    if (keycode < sizeof(m_bPressed))
    {
        return m_bPressed[keycode];
    }
    return 0;
}


static int DebugKeyboard_wait_keypress(void)
{
    INPUT_RECORD ir;
    int read;

    DebugKeyboard_flush();

    while (!MAME32App.Done())
    {
        if (DebugKeyboard_Keypressed())
        {
            if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read) && read == 1)
            {
                if (ir.EventType == KEY_EVENT)
                {
                    if (ir.Event.KeyEvent.wVirtualKeyCode < sizeof(m_bPressed))
                    {
                        m_bPressed[ir.Event.KeyEvent.wVirtualKeyCode] = ir.Event.KeyEvent.bKeyDown;
                    }
                    if (ir.Event.KeyEvent.bKeyDown)
                    {
                        return ir.Event.KeyEvent.wVirtualKeyCode;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    if (MAME32App.Done())
    {
       return VK_F12;
    }
    return 0;
}

static int DebugKeyboard_readkey_unicode(int flush)
{
    INPUT_RECORD ir;
    int read;

    if (flush)
    {
        DebugKeyboard_flush();
    }
    while (DebugKeyboard_Keypressed())
    {
        if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read) && read == 1)
        {
            if (ir.EventType == KEY_EVENT)
            {
                if (ir.Event.KeyEvent.wVirtualKeyCode < sizeof(m_bPressed))
                {
                    m_bPressed[ir.Event.KeyEvent.wVirtualKeyCode] = ir.Event.KeyEvent.bKeyDown;
                }
                if (ir.Event.KeyEvent.bKeyDown)
                {
                    return ir.Event.KeyEvent.uChar.UnicodeChar;
                }
            }
        }
        else
        {
            break;
        }
    }
    return 0;
}

void DebugKeyboard_CustomizeInputportDefaults(int DefaultInput, struct ipd *defaults)
{
}

/***************************************************************************
    Message handlers
 ***************************************************************************/

static BOOL DebugKeyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    return FALSE;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static int DebugKeyboard_Keypressed(void)
{
    int count;

    MAME32App.ProcessMessages();

    count = 0;
    if (!GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &count))
    {
        return 0;
    }

    return count >= 1;
}

static void DebugKeyboard_flush(void)
{
    INPUT_RECORD ir;
    int read;

    while (DebugKeyboard_Keypressed())
    {
        if (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &read) && read == 1)
        {
            if (ir.EventType==KEY_EVENT)
            {
                if (ir.Event.KeyEvent.wVirtualKeyCode<sizeof(m_bPressed))
                {
                    m_bPressed[ir.Event.KeyEvent.wVirtualKeyCode]=ir.Event.KeyEvent.bKeyDown;
                }
            }
        }
        else
        {
            break;
        }
    }
}

#endif /* defined(MAME_DEBUG) */
