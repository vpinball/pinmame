/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Joystick.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <assert.h>
#include "mame32.h"
#include "Joystick.h"
#include "M32Util.h"
#include "trak.h"
#include "input.h"

#define MAX_JOY              256
#define MAX_JOY_NAME_LEN     20

#define OSD_ANALOGMAX       ( 127.0)
#define OSD_ANALOGMIN       (-128.0)

#define NUM_JOYSTICKS       4

/* Not sure if this is supported via the Multimedia API. */
#ifndef JOYSTICKID3
#define JOYSTICKID3 2
#endif
#ifndef JOYSTICKID4
#define JOYSTICKID4 3
#endif

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int              Joystick_init(options_type *options);
static void             Joystick_exit(void);
static void             Joystick_poll_joysticks(void);
static const struct JoystickInfo *Joystick_get_joy_list(void);
static int              Joystick_is_joy_pressed(int joycode);
static void             Joystick_analogjoy_read(int player, int *analog_x, int *analog_y);
static int              Joystick_standard_analog_read(int player, int axis);
static BOOL             Joystick_Available(void);
static BOOL             Joystick_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

static DWORD            Joystick_DeadZoneMin(DWORD dwMin, DWORD dwMax, UINT nDeadZone);
static DWORD            Joystick_DeadZoneMax(DWORD dwMin, DWORD dwMax, UINT nDeadZone);
static void             Joystick_InitJoyList(void);

#define SetDeadZone(Joy, Axis)                                                       \
            Joy.m_dw##Axis##DZmin = Joystick_DeadZoneMin(Joy.m_JoyCaps.w##Axis##min, \
                                                Joy.m_JoyCaps.w##Axis##max,          \
                                                Joy.m_nDeadZone);                    \
            Joy.m_dw##Axis##DZmax = Joystick_DeadZoneMax(Joy.m_JoyCaps.w##Axis##min, \
                                                Joy.m_JoyCaps.w##Axis##max,          \
                                                Joy.m_nDeadZone);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDJoystick  Joystick = 
{
    Joystick_init,                  /* init                 */
    Joystick_exit,                  /* exit                 */
    Joystick_get_joy_list,          /* get_joy_list         */
    Joystick_is_joy_pressed,        /* is_joy_pressed       */
    Joystick_poll_joysticks,        /* poll_joysticks       */
    Joystick_analogjoy_read,        /* analogjoy_read       */
    Joystick_standard_analog_read,  /* standard_analog_read */
    Joystick_Available,             /* Available            */
    Joystick_OnMessage,             /* OnMessage            */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tJoystickInfo
{
    BOOL        m_bUseJoystick;
    UINT        m_uJoyID;

    JOYINFOEX   m_JoyInfo;
    JOYCAPS     m_JoyCaps;

    UINT        m_nDeadZone;
    DWORD       m_dwXDZmin;
    DWORD       m_dwXDZmax;
    DWORD       m_dwYDZmin;
    DWORD       m_dwYDZmax;
    DWORD       m_dwZDZmin;
    DWORD       m_dwZDZmax;
    DWORD       m_dwRDZmin;
    DWORD       m_dwRDZmax;
    DWORD       m_dwUDZmin;
    DWORD       m_dwUDZmax;
    DWORD       m_dwVDZmin;
    DWORD       m_dwVDZmax;

    double      m_dXScale;
    double      m_dYScale;
};

struct tJoystick_private
{
    UINT                    m_nNumJoysticks;
    struct tJoystickInfo    m_Joy[NUM_JOYSTICKS];
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tJoystick_private This;
static struct JoystickInfo      joylist[MAX_JOY] =
{
    /* will be filled later */
    { 0, 0, 0 } /* end of table */
};

static const UINT g_nJoyID[NUM_JOYSTICKS] =
{
    JOYSTICKID1,
    JOYSTICKID2,
    JOYSTICKID3,
    JOYSTICKID4
};

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int Joystick_init(options_type *options)
{
    int         i;
    MMRESULT    mmResult;

    memset(&This, 0, sizeof(struct tJoystick_private));

    for (i = 0; i < NUM_JOYSTICKS; i++)
    {
        This.m_Joy[i].m_bUseJoystick = TRUE;
        This.m_Joy[i].m_nDeadZone    = 75;    /* No way to set dead zone right now */
        This.m_Joy[i].m_uJoyID       = g_nJoyID[i];
    }

    /* User turned off joy option or a joy driver is not installed. */
    if (!options->use_joystick
    ||  joyGetNumDevs() == 0)
    {
       This.m_Joy[0].m_bUseJoystick = FALSE;
       This.m_Joy[1].m_bUseJoystick = FALSE;
       This.m_Joy[2].m_bUseJoystick = FALSE;
       This.m_Joy[3].m_bUseJoystick = FALSE;       
    }

    for (i = 0; i < NUM_JOYSTICKS; i++)
    {        
        if (This.m_Joy[i].m_bUseJoystick == FALSE)
            continue;

        /* Determine if JOYSTICKID[1-4] is plugged in. */
        This.m_Joy[i].m_JoyInfo.dwSize  = sizeof(JOYINFOEX);
        This.m_Joy[i].m_JoyInfo.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
        
        mmResult = joyGetPosEx(This.m_Joy[i].m_uJoyID, &This.m_Joy[i].m_JoyInfo);
        
        if (mmResult == JOYERR_NOERROR)
        {
            joyGetDevCaps(This.m_Joy[i].m_uJoyID, &This.m_Joy[i].m_JoyCaps, sizeof(JOYCAPS));
            
            SetDeadZone(This.m_Joy[i], X);
            SetDeadZone(This.m_Joy[i], Y);

            /* Currently only analog support for X and Y axis. */
            This.m_Joy[i].m_dXScale = ((OSD_ANALOGMAX - OSD_ANALOGMIN) / (This.m_Joy[i].m_JoyCaps.wXmax - This.m_Joy[i].m_JoyCaps.wXmin));
            This.m_Joy[i].m_dYScale = ((OSD_ANALOGMAX - OSD_ANALOGMIN) / (This.m_Joy[i].m_JoyCaps.wYmax - This.m_Joy[i].m_JoyCaps.wYmin));

            /* Check for other axis */
            if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_HASZ)
            {
                This.m_Joy[i].m_JoyInfo.dwFlags |= JOY_RETURNZ;
                SetDeadZone(This.m_Joy[i], Z);
            }

            if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_HASR)
            {
                This.m_Joy[i].m_JoyInfo.dwFlags |= JOY_RETURNR;
                SetDeadZone(This.m_Joy[i], R);
            }

            if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_HASU)
            {
                This.m_Joy[i].m_JoyInfo.dwFlags |= JOY_RETURNU;
                SetDeadZone(This.m_Joy[i], U);
            }

            if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_HASV)
            {
                This.m_Joy[i].m_JoyInfo.dwFlags |= JOY_RETURNV;
                SetDeadZone(This.m_Joy[i], V);
            }

            if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_HASPOV
            &&  This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_POV4DIR)
            {
                This.m_Joy[i].m_JoyInfo.dwFlags |= JOY_RETURNPOV;
            }

            This.m_nNumJoysticks++;
        }
        else
        if (mmResult == JOYERR_UNPLUGGED)
        {
            This.m_Joy[i].m_bUseJoystick = FALSE;
        }
        else
        {
            /* Some other error with the joystick. Don't use it. */
            This.m_Joy[i].m_bUseJoystick = FALSE;
        }
    }

    Joystick_InitJoyList();

    /* Initialize JOYINFOEX */      
    Joystick_poll_joysticks();

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void Joystick_exit(void)
{
}

static const struct JoystickInfo *Joystick_get_joy_list(void)
{
    return joylist;
}

static void Joystick_poll_joysticks(void)
{
    int i = NUM_JOYSTICKS;

    if (This.m_nNumJoysticks == 0)
        return;

    while (i--)
    {
        if (This.m_Joy[i].m_bUseJoystick == TRUE)
            joyGetPosEx(This.m_Joy[i].m_uJoyID, &This.m_Joy[i].m_JoyInfo);
    }
}

/*
    check if the joystick is moved in the specified direction, defined in
    osdepend.h. Return 0 if it is not pressed, nonzero otherwise.
*/
int Joystick_is_joy_pressed(int joycode)
{
    /* Map from our button codes to the Multimedia API's joy codes. */
    static DWORD joyButtonCode[] =
    {
        JOY_BUTTON1,  JOY_BUTTON2,  JOY_BUTTON3,  JOY_BUTTON4,
        JOY_BUTTON5,  JOY_BUTTON6,  JOY_BUTTON7,  JOY_BUTTON8,
        JOY_BUTTON9,  JOY_BUTTON10, JOY_BUTTON11, JOY_BUTTON12,
        JOY_BUTTON13, JOY_BUTTON14, JOY_BUTTON15, JOY_BUTTON16,
        JOY_BUTTON17, JOY_BUTTON18, JOY_BUTTON19, JOY_BUTTON20,
        JOY_BUTTON21, JOY_BUTTON22, JOY_BUTTON23, JOY_BUTTON24,
        JOY_BUTTON25, JOY_BUTTON26, JOY_BUTTON27, JOY_BUTTON28,
        JOY_BUTTON29, JOY_BUTTON30, JOY_BUTTON31, JOY_BUTTON32
    };
    static DWORD joyPOVCode[] =
    {
        JOY_POVFORWARD,
        JOY_POVRIGHT,
        JOY_POVBACKWARD,
        JOY_POVLEFT
    };
    int joy_num, stick;

    /* special case for mouse buttons */
    /*
        Since MAME doesn't explicitly check for the trakball buttons,
        we piggyback on the joystick button method.
    */
    if (MAME32App.m_pTrak != NULL)
    {
        switch (joycode)
        {
            case MOUSE_BUTTON(1):
                if (MAME32App.m_pTrak->trak_pressed(TRAK_FIRE1))
                    return 1;
                break;

            case MOUSE_BUTTON(2):
                if (MAME32App.m_pTrak->trak_pressed(TRAK_FIRE2))
                    return 1;
                break;

            case MOUSE_BUTTON(3):
                if (MAME32App.m_pTrak->trak_pressed(TRAK_FIRE3))
                    return 1;
                break;

            case MOUSE_BUTTON(4):
                if (MAME32App.m_pTrak->trak_pressed(TRAK_FIRE4))
                    return 1;
                break;

            case MOUSE_BUTTON(5):
                if (MAME32App.m_pTrak->trak_pressed(TRAK_FIRE5))
                    return 1;
                break;
        }       
    }

    joy_num = GET_JOYCODE_JOY(joycode);

    /* do we have as many sticks? */
    if (joy_num == 0 || This.m_nNumJoysticks < joy_num)
        return 0;
    joy_num--;

    if (This.m_Joy[joy_num].m_bUseJoystick == FALSE)
        return 0;

    stick = GET_JOYCODE_STICK(joycode);

    if (stick == JOYCODE_STICK_BTN)
    {
        /* buttons */
        int button;

        button = GET_JOYCODE_BUTTON(joycode);
        if (button == 0 || This.m_Joy[joy_num].m_JoyCaps.wNumButtons < button)
            return 0;
        if (GET_JOYCODE_DIR(joycode) != JOYCODE_DIR_BTN)
            return 0;
        button--;

        return This.m_Joy[joy_num].m_JoyInfo.dwButtons & joyButtonCode[button];
    }
    else
    if (stick == JOYCODE_STICK_POV)
    {
        /* POV */
        int   button;
        DWORD dwPOV;

        button = GET_JOYCODE_BUTTON(joycode);
        if (button == 0 || 4 < button)
            return 0;
        button--;

        dwPOV = This.m_Joy[joy_num].m_JoyInfo.dwPOV;

        if (dwPOV == JOY_POVCENTERED)
            return 0;

        if (dwPOV == 4500)
        {
            if (joyPOVCode[button] == JOY_POVFORWARD
            ||  joyPOVCode[button] == JOY_POVRIGHT)
                return TRUE;
        }
        else
        if (dwPOV == 13500)
        {
            if (joyPOVCode[button] == JOY_POVBACKWARD
            ||  joyPOVCode[button] == JOY_POVRIGHT)
                return TRUE;
        }
        else
        if (dwPOV == 22500)
        {
            if (joyPOVCode[button] == JOY_POVBACKWARD
            ||  joyPOVCode[button] == JOY_POVLEFT)
                return TRUE;
        }
        else
        if (dwPOV == 31500)
        {
            if (joyPOVCode[button] == JOY_POVFORWARD
            ||  joyPOVCode[button] == JOY_POVLEFT)
                return TRUE;
        }

        return dwPOV == joyPOVCode[button];
    }
    else
    {
        /* sticks */
        int axis, dir;

        axis = GET_JOYCODE_AXIS(joycode);
        dir  = GET_JOYCODE_DIR(joycode);

        if (axis == 0 || This.m_Joy[joy_num].m_JoyCaps.wNumAxes < axis)
            return 0;
        axis--;

        switch (axis)
        {
        case 0: /* X */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwXpos <= This.m_Joy[joy_num].m_dwXDZmin;
                else
                    return This.m_Joy[joy_num].m_dwXDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwXpos;

        case 1: /* Y */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwYpos <= This.m_Joy[joy_num].m_dwYDZmin;
                else
                    return This.m_Joy[joy_num].m_dwYDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwYpos;

        case 2: /* Z */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwZpos <= This.m_Joy[joy_num].m_dwZDZmin;
                else
                    return This.m_Joy[joy_num].m_dwZDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwZpos;

        case 3: /* R */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwRpos <= This.m_Joy[joy_num].m_dwRDZmin;
                else
                    return This.m_Joy[joy_num].m_dwRDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwRpos;

        case 4: /* U */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwUpos <= This.m_Joy[joy_num].m_dwUDZmin;
                else
                    return This.m_Joy[joy_num].m_dwUDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwUpos;

        case 5: /* V */
                if (dir == JOYCODE_DIR_NEG)
                    return This.m_Joy[joy_num].m_JoyInfo.dwVpos <= This.m_Joy[joy_num].m_dwVDZmin;
                else
                    return This.m_Joy[joy_num].m_dwVDZmax <= This.m_Joy[joy_num].m_JoyInfo.dwVpos;
        }
    }

    return 0;
}

/* osd_analog_joyread() returns values from -128 to 128 */
static void Joystick_analogjoy_read(int player, int *analog_x, int *analog_y)
{
    *analog_x = *analog_y = 0;

    if (This.m_nNumJoysticks == 0)
        return;

    if (NUM_JOYSTICKS <= player)
        return;

    if (This.m_Joy[player].m_bUseJoystick == FALSE)
        return;

    *analog_x = (int)((This.m_Joy[player].m_JoyInfo.dwXpos - This.m_Joy[player].m_JoyCaps.wXmin) * (This.m_Joy[player].m_dXScale) - 128.0);
    *analog_y = (int)((This.m_Joy[player].m_JoyInfo.dwYpos - This.m_Joy[player].m_JoyCaps.wYmin) * (This.m_Joy[player].m_dYScale) - 128.0);
}

static int Joystick_standard_analog_read(int player, int axis)
{
    /* Somebody send me a Wingman Warrior. Please. mjs */
    return 0;
}

static BOOL Joystick_Available(void)
{
    MMRESULT  mmResult;
    JOYINFOEX JoyInfoEx;
    int       i;

    memset(&JoyInfoEx, 0, sizeof(JoyInfoEx));
    JoyInfoEx.dwSize  = sizeof(JOYINFOEX);
    JoyInfoEx.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

    for (i = 0; i < NUM_JOYSTICKS; i++)
    {
        mmResult = joyGetPosEx(g_nJoyID[i], &JoyInfoEx);
        if (mmResult == JOYERR_NOERROR)
            return TRUE;
    }

    return FALSE;
}

static BOOL Joystick_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
   return FALSE;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void Joystick_InitJoyList(void)
{
    static int joyequiv[][2] =
    {
        { JOYCODE(1,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_NEG), JOYCODE_1_LEFT },
        { JOYCODE(1,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_POS), JOYCODE_1_RIGHT },
        { JOYCODE(1,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG), JOYCODE_1_UP },
        { JOYCODE(1,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS), JOYCODE_1_DOWN },

        { JOYCODE(1,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON1 },
        { JOYCODE(1,JOYCODE_STICK_BTN,2,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON2 },
        { JOYCODE(1,JOYCODE_STICK_BTN,3,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON3 },
        { JOYCODE(1,JOYCODE_STICK_BTN,4,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON4 },
        { JOYCODE(1,JOYCODE_STICK_BTN,5,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON5 },
        { JOYCODE(1,JOYCODE_STICK_BTN,6,JOYCODE_DIR_BTN),  JOYCODE_1_BUTTON6 },

        { JOYCODE(2,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_NEG), JOYCODE_2_LEFT },
        { JOYCODE(2,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_POS), JOYCODE_2_RIGHT },
        { JOYCODE(2,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG), JOYCODE_2_UP },
        { JOYCODE(2,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS), JOYCODE_2_DOWN },

        { JOYCODE(2,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON1 },
        { JOYCODE(2,JOYCODE_STICK_BTN,2,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON2 },
        { JOYCODE(2,JOYCODE_STICK_BTN,3,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON3 },
        { JOYCODE(2,JOYCODE_STICK_BTN,4,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON4 },
        { JOYCODE(2,JOYCODE_STICK_BTN,5,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON5 },
        { JOYCODE(2,JOYCODE_STICK_BTN,6,JOYCODE_DIR_BTN),  JOYCODE_2_BUTTON6 },

        { JOYCODE(3,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_NEG), JOYCODE_3_LEFT },
        { JOYCODE(3,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_POS), JOYCODE_3_RIGHT },
        { JOYCODE(3,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG), JOYCODE_3_UP },
        { JOYCODE(3,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS), JOYCODE_3_DOWN },

        { JOYCODE(3,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON1 },
        { JOYCODE(3,JOYCODE_STICK_BTN,2,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON2 },
        { JOYCODE(3,JOYCODE_STICK_BTN,3,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON3 },
        { JOYCODE(3,JOYCODE_STICK_BTN,4,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON4 },
        { JOYCODE(3,JOYCODE_STICK_BTN,5,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON5 },
        { JOYCODE(3,JOYCODE_STICK_BTN,6,JOYCODE_DIR_BTN),  JOYCODE_3_BUTTON6 },

        { JOYCODE(4,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_NEG), JOYCODE_4_LEFT },
        { JOYCODE(4,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_POS), JOYCODE_4_RIGHT },
        { JOYCODE(4,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG), JOYCODE_4_UP },
        { JOYCODE(4,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS), JOYCODE_4_DOWN },

        { JOYCODE(4,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON1 },
        { JOYCODE(4,JOYCODE_STICK_BTN,2,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON2 },
        { JOYCODE(4,JOYCODE_STICK_BTN,3,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON3 },
        { JOYCODE(4,JOYCODE_STICK_BTN,4,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON4 },
        { JOYCODE(4,JOYCODE_STICK_BTN,5,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON5 },
        { JOYCODE(4,JOYCODE_STICK_BTN,6,JOYCODE_DIR_BTN),  JOYCODE_4_BUTTON6 },
        { 0,0 }
    };
    static char joynames[MAX_JOY][MAX_JOY_NAME_LEN+1];  /* will be used to store names */
    static char* JoyAxisName[] = { "X Axis", "Y Axis", "Z Axis",
                                   "R Axis", "U Axis", "V Axis"};
    static char* JoyPOVName[] = { "POV Forward",
                                  "POV Right",
                                  "POV Backward",
                                  "POV Left" };
    int tot, i, j;
    char buf[256];
    
    tot = 0;

    /* first of all, map mouse buttons */
    for (j = 0; j < 5; j++)
    {
        sprintf(buf, "Mouse B%d", j + 1);
        strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
        joynames[tot][MAX_JOY_NAME_LEN] = 0;
        joylist[tot].name = joynames[tot];
        joylist[tot].code = MOUSE_BUTTON(j + 1);
        tot++;
    }

    for (i = 0; i < This.m_nNumJoysticks; i++)
    {
        for (j = 0; j < This.m_Joy[i].m_JoyCaps.wNumAxes; j++)
        {
            sprintf(buf, "J%d %.8s %s -",
                    i + 1,
                    This.m_Joy[i].m_JoyCaps.szPname,
                    JoyAxisName[j]);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_AXIS, j + 1, JOYCODE_DIR_NEG);
            tot++;
            
            sprintf(buf, "J%d %.8s %s +",
                    i + 1,
                    This.m_Joy[i].m_JoyCaps.szPname,
                    JoyAxisName[j]);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_AXIS, j + 1, JOYCODE_DIR_POS);
            tot++;
        }

        for (j = 0; j < This.m_Joy[i].m_JoyCaps.wNumButtons; j++)
        {
            sprintf(buf, "J%d Button %d", i + 1, j);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_BTN, j + 1, JOYCODE_DIR_BTN);
            tot++;
        }

        if (This.m_Joy[i].m_JoyCaps.wCaps & JOYCAPS_POV4DIR)
        {
            for (j = 0; j < sizeof(JoyPOVName) / sizeof(JoyPOVName[0]); j++)
            {
                sprintf(buf, "J%d %s", i + 1, JoyPOVName[j]);
                strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
                joynames[tot][MAX_JOY_NAME_LEN] = 0;
                joylist[tot].name = joynames[tot];
                joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_POV, j + 1, JOYCODE_DIR_BTN);
                tot++;
            }
        }
    }
    
    /* terminate array */
    joylist[tot].name = 0;
    joylist[tot].code = 0;
    joylist[tot].standardcode = 0;
    
    /* fill in equivalences */
    for (i = 0; i < tot; i++)
    {
        joylist[i].standardcode = JOYCODE_OTHER;
        
        j = 0;
        while (joyequiv[j][0] != 0)
        {
            if (joyequiv[j][0] == joylist[i].code)
            {
                joylist[i].standardcode = joyequiv[j][1];
                break;
            }
            j++;
        }
    }
}

static DWORD Joystick_DeadZoneMin(DWORD dwMin, DWORD dwMax, UINT nDeadZone)
{
    return (DWORD)((dwMax - dwMin) * ((100.0 - nDeadZone) / 200.0));
}

static DWORD Joystick_DeadZoneMax(DWORD dwMin, DWORD dwMax, UINT nDeadZone)
{
    return (DWORD)((dwMax - dwMin) * ((100.0 + nDeadZone) / 200.0));
}

