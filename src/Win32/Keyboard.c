/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Keyboard.c

 ***************************************************************************/

#include "driver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include "MAME32.h"
#include "M32Util.h"
#include "Keyboard.h"
#include "status.h"

#define NUMKEYSTATES    256

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int          Keyboard_init(options_type *options);
static void         Keyboard_exit(void);

static const struct KeyboardInfo * Keyboard_get_key_list(void);
static void         Keyboard_customize_inputport_defaults(struct ipd *defaults);
static int          Keyboard_is_key_pressed(int keycode);
static int          Keyboard_readkey_unicode(int flush);


static BOOL         Keyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void         OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static void         OnChar(HWND hWnd, TCHAR ch, int cRepeat);

static void         Keyboard_AdjustKeylist(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDKeyboard Keyboard = 
{
    Keyboard_init,                         /* init                         */
    Keyboard_exit,                         /* exit                         */
    Keyboard_get_key_list,                 /* get_key_list                 */
    Keyboard_customize_inputport_defaults, /* customize_inputport_defaults */
    Keyboard_is_key_pressed,               /* is_key_pressed               */
    Keyboard_readkey_unicode,              /* readkey_unicode              */

    Keyboard_OnMessage                     /* OnMessage                    */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tKeyboard_private
{
    BOOL    m_bPauseKeyPressed;
    int     m_DefaultInput;
    TCHAR   m_chPressed;
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
static int Keyboard_init(options_type *options)
{
    int     i;
    BYTE    KeyState[NUMKEYSTATES];

    memset(&This, 0, sizeof(struct tKeyboard_private));

    This.m_DefaultInput = options->default_input;

    /* Clear keyboard state. */
    GetKeyboardState(KeyState);

    for (i = 0; i < NUMKEYSTATES; i++)
        KeyState[i] &= 0x01;

    SetKeyboardState(KeyState);

    Keyboard_AdjustKeylist();

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void Keyboard_exit(void)
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
    { "LSHIFT",     VK_LSHIFT,          KEYCODE_LSHIFT },
    { "RSHIFT",     VK_RSHIFT,          KEYCODE_RSHIFT },
    { "LCTRL",      VK_LCONTROL,        KEYCODE_LCONTROL },
    { "RCTRL",      VK_RCONTROL,        KEYCODE_RCONTROL },
    { "LALT",       VK_LMENU,           KEYCODE_LALT },
    { "RALT",       VK_RMENU,           KEYCODE_RALT },
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

/*
  return a list of all available keys (see input.h)
*/
static const struct KeyboardInfo* Keyboard_get_key_list(void)
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
static void Keyboard_customize_inputport_defaults(struct ipd *defaults)
{
    Keyboard_CustomizeInputportDefaults(This.m_DefaultInput, defaults);
}

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_customize_inputport_defaults().
*/
static int Keyboard_is_key_pressed(int keycode)
{
    SHORT state;

    if (keycode == VK_ESCAPE && MAME32App.Done())
        return 1;

    MAME32App.ProcessMessages();

    if (keycode == VK_PAUSE)
    {
        if (This.m_bPauseKeyPressed == TRUE)
        {
            This.m_bPauseKeyPressed = FALSE;
            return 1;
        }
    }

    state = GetAsyncKeyState(keycode);

    /* If the high-order bit is 1, the key is down; otherwise, it is up */
    if (state & 0x8000)
        return 1;

    return 0;
}

static int Keyboard_readkey_unicode(int flush)
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

void Keyboard_CustomizeInputportDefaults(int DefaultInput, struct ipd *defaults)
{
#ifndef NEOFREE
#ifndef TINY_COMPILE
    extern struct GameDriver driver_neogeo;
#endif
#endif

    if (DefaultInput == INPUT_LAYOUT_HR
    ||  DefaultInput == INPUT_LAYOUT_HRSE)
    {
        while (defaults->type != IPT_END)
        {
            int j;
            for (j = 0; j < SEQ_MAX; ++j)
            {
                if (defaults->seq[j] == KEYCODE_UP)    defaults->seq[j] = KEYCODE_8_PAD;
                if (defaults->seq[j] == KEYCODE_DOWN)  defaults->seq[j] = KEYCODE_2_PAD;
                if (defaults->seq[j] == KEYCODE_LEFT)  defaults->seq[j] = KEYCODE_4_PAD;
                if (defaults->seq[j] == KEYCODE_RIGHT) defaults->seq[j] = KEYCODE_6_PAD;
            }
            if (defaults->type == IPT_UI_SELECT)                           seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
            if (defaults->type == IPT_START1)                              seq_set_1(&defaults->seq, KEYCODE_1);
            if (defaults->type == IPT_START2)                              seq_set_1(&defaults->seq, KEYCODE_2);
            if (defaults->type == IPT_COIN1)                               seq_set_1(&defaults->seq, KEYCODE_3);
            if (defaults->type == IPT_COIN2)                               seq_set_1(&defaults->seq, KEYCODE_4);
            if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_R);
            if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_F);
            if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_D);
            if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_G);
            if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_8_PAD);
            if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_2_PAD);
            if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_4_PAD);
            if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_6_PAD);
            if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
            if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_LALT);
            if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_SPACE);
            if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_LSHIFT);
            if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_Z);
            if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1))             seq_set_1(&defaults->seq, KEYCODE_X);
            if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_A);
            if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_S);
            if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_Q);
            if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_W);
            if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_E);
            if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2))             seq_set_1(&defaults->seq, KEYCODE_OPENBRACE);

#ifndef NEOFREE
#ifndef TINY_COMPILE
            if (DefaultInput == INPUT_LAYOUT_HRSE &&
                (Machine->gamedrv->clone_of == &driver_neogeo ||
                (Machine->gamedrv->clone_of && Machine->gamedrv->clone_of->clone_of == &driver_neogeo)))
            {
                if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_C);
                if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_LSHIFT);
                if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_Z);
                if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_X);
                if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_CLOSEBRACE);
                if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_W);
                if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_E);
                if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_OPENBRACE);
                if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
                if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
            }
#endif
#endif
            defaults++;
        }
    }
}

/***************************************************************************
    Message handlers
 ***************************************************************************/

static BOOL Keyboard_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg)
    {
        HANDLE_MESSAGE(hWnd, WM_KEYDOWN, OnKey);
        HANDLE_MESSAGE(hWnd, WM_CHAR,    OnChar);
    }
    return FALSE;
}

static void OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    /*
        Detect the 'Pause' key.
        This is necessary since the Pause key is not
        easily detected when polling with GetAsyncKeyState().
        The WM_KEYDOWN and WM_KEYUP messages are to close together.
    */
    if (vk == VK_PAUSE)
        This.m_bPauseKeyPressed = TRUE;
}

static void OnChar(HWND hWnd, TCHAR ch, int cRepeat)
{
    This.m_chPressed = ch;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void Keyboard_AdjustKeylist(void)
{
    struct KeyboardInfo* pKeylist = keylist;

    /*
        On NT, right and left keys are different,
        so set r/l independent keys to KEYCODE_OTHER.

    */
    if (OnNT())
    {
        while (pKeylist->name != 0)
        {
            if (pKeylist->code == VK_SHIFT)    pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_CONTROL)  pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_MENU)     pKeylist->standardcode = KEYCODE_OTHER;

            pKeylist++;
        }
    }
    else
    {
        while (pKeylist->name != 0)
        {
            if (pKeylist->code == VK_RSHIFT)    pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_LSHIFT)    pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_RCONTROL)  pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_LCONTROL)  pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_RMENU)     pKeylist->standardcode = KEYCODE_OTHER;
            if (pKeylist->code == VK_LMENU)     pKeylist->standardcode = KEYCODE_OTHER;

            pKeylist++;
        }
    }
}
