/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  DIJoystick.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <dinput.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "MAME32.h"
#include "DirectInput.h"
#include "DIJoystick.h"
#include "M32Util.h"
#include "trak.h"
#include "input.h"
#include "dxdecode.h"

/*
  limits:
  - 7 joysticks
  - 15 sticks on each joystick (15?)
  - 63 buttons on each joystick

  - 256 total inputs


   1 1 1 1 1 1
   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +---+-----------+---------+-----+
  |Dir|Axis/Button|   Stick | Joy |
  +---+-----------+---------+-----+

    Stick:  0 for buttons 1 for axis
    Joy:    1 for Mouse/track buttons

*/

#define JOYCODE(joy, stick, axis_or_button, dir) \
        ((((dir)            & 0x03) << 14) |     \
         (((axis_or_button) & 0x3f) <<  8) |     \
         (((stick)          & 0x1f) <<  3) |     \
         (((joy)            & 0x07) <<  0))

#define GET_JOYCODE_JOY(code)    (((code) >> 0) & 0x07)
#define GET_JOYCODE_STICK(code)  (((code) >> 3) & 0x1f)
#define GET_JOYCODE_AXIS(code)   (((code) >> 8) & 0x3f)
#define GET_JOYCODE_BUTTON(code) GET_JOYCODE_AXIS(code)
#define GET_JOYCODE_DIR(code)    (((code) >>14) & 0x03)

#define JOYCODE_STICK_BTN    0
#define JOYCODE_STICK_AXIS   1
#define JOYCODE_STICK_POV    2

#define JOYCODE_DIR_BTN      0
#define JOYCODE_DIR_NEG      1
#define JOYCODE_DIR_POS      2

/* use otherwise unused joystick codes for the three mouse buttons */
#define MOUSE_BUTTON(button)    JOYCODE(1, JOYCODE_STICK_BTN, button, 3)

#define MAX_JOY              256
#define MAX_JOY_NAME_LEN     20

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int              DIJoystick_init(options_type *options);
static void             DIJoystick_exit(void);
static void             DIJoystick_poll_joysticks(void);
static const struct JoystickInfo *DIJoystick_get_joy_list(void);
static int              DIJoystick_is_joy_pressed(int joycode);
static const char*      DIJoystick_joy_name(int joycode);
static void             DIJoystick_analogjoy_read(int player, int *analog_x, int *analog_y);
static int              DIJoystick_standard_analog_read(int player, int axis);
static BOOL             DIJoystick_Available(void);
static BOOL             DIJoystick_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

static BOOL CALLBACK DIJoystick_EnumDeviceProc(LPDIDEVICEINSTANCE pdidi, LPVOID pv);
static void DIJoystick_InitJoyList(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDJoystick  DIJoystick = 
{
    DIJoystick_init,                /* init              */
    DIJoystick_exit,                /* exit              */
    DIJoystick_get_joy_list,        /* get_joy_list      */
    DIJoystick_is_joy_pressed,      /* joy_pressed       */
    DIJoystick_poll_joysticks,      /* poll_joysticks    */
    DIJoystick_analogjoy_read,      /* analogjoy_read    */
    DIJoystick_standard_analog_read,/* standard_analog_read    */
    DIJoystick_Available,           /* Available         */
    DIJoystick_OnMessage,           /* OnMessage         */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

#define MAX_PHYSICAL_JOYSTICKS 20
#define MAX_AXES               20

typedef struct
{
    GUID guid;
    char *name;

    int offset; /* offset in dijoystate */
} axis_type;

typedef struct
{
    BOOL use_joystick;

    GUID guidDevice;
    char *name;

    BOOL is_light_gun;

    LPDIRECTINPUTDEVICE2 did;

    DWORD num_axes;
    axis_type axes[MAX_AXES];

    DWORD num_pov;
    DWORD num_buttons;

    DIJOYSTATE  dijs;

} joystick_type;

struct tDIJoystick_private
{
    int   use_count; /* the gui and game can both init/exit us, so keep track */
    BOOL  m_bCoinSlot;

    DWORD num_joysticks;
    joystick_type joysticks[MAX_PHYSICAL_JOYSTICKS]; /* actual joystick data! */
};

/* internal functions needing our declarations */
static BOOL CALLBACK DIJoystick_EnumAxisObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
static BOOL CALLBACK DIJoystick_EnumPOVObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
static BOOL CALLBACK DIJoystick_EnumButtonObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
static void ClearJoyState(DIJOYSTATE *pdijs);

static void InitJoystick(joystick_type *joystick);
static void ExitJoystick(joystick_type *joystick);

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDIJoystick_private   This;

static const GUID guidNULL = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

static struct JoystickInfo joylist[256] =
{
    /* will be filled later */
    { 0, 0, 0 } /* end of table */
};


/***************************************************************************
    External OSD functions  
 ***************************************************************************/
/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int DIJoystick_init(options_type* options)
{
    DWORD   i;
    HRESULT hr;

    This.use_count++;

    This.num_joysticks = 0;

    if (!options->use_joystick)
       return 0;

    if (di == NULL)
    {
        ErrorMsg("DirectInput not initialized");
        return 0;
    }

    /* enumerate for joystick devices */
    hr = IDirectInput_EnumDevices(di, DIDEVTYPE_JOYSTICK,
                 (LPDIENUMDEVICESCALLBACK)DIJoystick_EnumDeviceProc,
                 NULL,
                 DIEDFL_ATTACHEDONLY);
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput EnumDevices() failed: %s", DirectXDecodeError(hr));
        return 0;
    }

    /* create each joystick device, enumerate each joystick for axes, etc */
    for (i = 0; i < This.num_joysticks; i++)
    {
        InitJoystick(&This.joysticks[i]);
        /* User turned off joy option or a joy driver is not installed. */
        if (!options->use_joystick)
            This.joysticks[i].use_joystick = FALSE;
    }

    /* Are there any joysticks attached? */
    if (This.num_joysticks < 1)
    {
        /*ErrorMsg("DirectInput EnumDevices didn't find any joysticks");*/
        return 0;
    }
    
    DIJoystick_InitJoyList();

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void DIJoystick_exit(void)
{
    DWORD i;

    This.use_count--;

    if (This.use_count > 0)
        return;

    for (i = 0; i < This.num_joysticks; i++)
       ExitJoystick(&This.joysticks[i]);
    
    This.num_joysticks = 0;
}

static const struct JoystickInfo *DIJoystick_get_joy_list(void)
{
    return joylist;
}

static void DIJoystick_poll_joysticks(void)
{
    HRESULT hr;
    DWORD   i;
    
    This.m_bCoinSlot = 0;

    for (i = 0; i < This.num_joysticks; i++)
    {
        /* start by clearing the structure, then fill it in if possible */

        ClearJoyState(&This.joysticks[i].dijs);

        if (This.joysticks[i].did == NULL)
            continue;

        if (This.joysticks[i].use_joystick == FALSE)
            continue;

        hr = IDirectInputDevice2_Poll(This.joysticks[i].did);

        hr = IDirectInputDevice2_GetDeviceState(This.joysticks[i].did,sizeof(DIJOYSTATE),
                                                &This.joysticks[i].dijs);
        if (FAILED(hr))
        {
            if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
            {
                hr = IDirectInputDevice2_Acquire(This.joysticks[i].did);
            }
            continue;
        }
    }
}

/*
    check if the DIJoystick is moved in the specified direction, defined in
    osdepend.h. Return 0 if it is not pressed, nonzero otherwise.
*/

static int DIJoystick_is_joy_pressed(int joycode)
{
    int joy_num;
    int stick;
    int axis;
    int dir;

    DIJOYSTATE dijs;

    int value;
    int dz = 60;

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
    if (joy_num == 0 || This.num_joysticks < joy_num)
        return 0;
    joy_num--;
    
    if (This.joysticks[joy_num].use_joystick == FALSE)
        return 0;

    dijs = This.joysticks[joy_num].dijs;

    stick = GET_JOYCODE_STICK(joycode);

    if (stick == JOYCODE_STICK_BTN)
    {
        /* buttons */
        int button;

        button = GET_JOYCODE_BUTTON(joycode);
        button--;

        if (button >= This.joysticks[joy_num].num_buttons
        ||  GET_JOYCODE_DIR(joycode) != JOYCODE_DIR_BTN)
            return 0;

        return dijs.rgbButtons[button] != 0;
    }

    if (stick == JOYCODE_STICK_POV)
    {
        /* POV */
        int pov_value;
        int angle;
        int axis_value;
       
        int num_pov = GET_JOYCODE_BUTTON(joycode) / 4;
        int code    = GET_JOYCODE_BUTTON(joycode) % 4;
        axis = code / 2;
        dir  = code % 2;

        if (num_pov >= This.joysticks[joy_num].num_pov)
            return 0;

        pov_value = dijs.rgdwPOV[num_pov];
        if (LOWORD(pov_value) == 0xffff)
            return 0;

        angle = (pov_value + 27000) % 36000;
        angle = (36000 - angle) % 36000;
        angle /= 100;
       
        /* angle is now in degrees counterclockwise from x axis*/
        if (axis == 1)
            axis_value = 128 + (int)(127 * cos(2 * PI * angle / 360.0)); /* x */
        else
            axis_value = 128 + (int)(127 * sin(2 * PI * angle / 360.0)); /* y */

        if (dir == 1)
            return axis_value <= (128 - 128 * dz / 100);
        else
            return axis_value >= (128 + 128 * dz / 100);
    }

    /* sticks */
    
    axis = GET_JOYCODE_AXIS(joycode);
    dir  = GET_JOYCODE_DIR(joycode);
    
    if (axis == 0 || This.joysticks[joy_num].num_axes < axis)
        return 0;
    axis--;

    value = *(int *)(((byte *)&dijs) + This.joysticks[joy_num].axes[axis].offset);

    if (dir == JOYCODE_DIR_NEG)
        return value <= (128 - 128 * dz / 100);
    else
        return value >= (128 + 128 * dz / 100);
}

/* osd_analog_joyread() returns values from -128 to 128 */
static void DIJoystick_analogjoy_read(int player, int* analog_x, int* analog_y)
{
    int i;
    int light_gun_index = -1;

    assert(player >= 0 && player < 4);

    *analog_x = *analog_y = 0;

    for (i = 0; i < This.num_joysticks; i++)
    {
        if (This.joysticks[i].is_light_gun)
        {
            light_gun_index = i;
            break;
        }
    }

    if (light_gun_index == -1)
    {
        /* standard analog joy reading */

        if (This.num_joysticks <= player || This.joysticks[player].use_joystick == FALSE)
            return;
       
        *analog_x = This.joysticks[player].dijs.lX - 128;
        *analog_y = This.joysticks[player].dijs.lY - 128;
       
        return;
    }

    /* light gun reading */

    switch (player)
    {
    case 0:
    {
        int x, y;
        POINT pt = { 0, 0 };
        ClientToScreen(MAME32App.m_hWnd, &pt);
        
        x = This.joysticks[light_gun_index].dijs.lX;
        y = This.joysticks[light_gun_index].dijs.lY;

        /* need to go from screen pixel x,y to -128 to 128 scale */
        *analog_x = (x - pt.x) * 257 / Machine->drv->screen_width - 128;
        if (*analog_x > 128)
            *analog_x = 128;

        *analog_y = (y - pt.y) * 257 / Machine->drv->screen_height - 128;
        if (*analog_y > 128)
            *analog_y = 128;
        
        break;
    }

    case 1:
    {
        int x, y;
        POINT pt = { 0, 0 };
        ClientToScreen(MAME32App.m_hWnd, &pt);
        
        x = This.joysticks[light_gun_index].dijs.lZ;
        y = This.joysticks[light_gun_index].dijs.lRz;
        
        /* need to go from screen pixel x,y to -128 to 128 scale */
        *analog_x = (x - pt.x) * 257 / Machine->drv->screen_width - 128;
        if (*analog_x > 128)
            *analog_x = 128;

        *analog_y = (y - pt.y) * 257 / Machine->drv->screen_height - 128;
        if (*analog_y > 128)
            *analog_y = 128;

       break;
    }
    break;
    
    default:
        /* only support up to 2 light guns (in one joystick device) right now */
        ;
    }

}

static int DIJoystick_standard_analog_read(int player, int axis)
{
    int retval;
   
    assert(player >= 0 && player < 4);

    if (This.num_joysticks <= player || This.joysticks[player].use_joystick == FALSE)
       return 0;

    switch (axis)
    {
    case X_AXIS:
        retval = (This.joysticks[player].dijs.lRz - 128) /
                 (128 / (TRAK_MAXX_RES / 2));
        if (retval > TRAK_MAXX_RES)
            retval = TRAK_MAXX_RES;
        if (retval < -TRAK_MAXX_RES)
            retval = -TRAK_MAXX_RES;
        return retval;

    case Y_AXIS:
        retval = 0;
        return retval;
    }
    return 0;
}


static BOOL DIJoystick_Available(void)
{
    static BOOL bBeenHere = FALSE;
    static BOOL bAvailable = FALSE;
    HRESULT     hr;
    GUID        guidDevice = guidNULL;
    LPDIRECTINPUTDEVICE didTemp;
    LPDIRECTINPUTDEVICE didJoystick;

    if (di == NULL)
    {
        return FALSE;
    }

    if (bBeenHere == FALSE)
        bBeenHere = TRUE;
    else
        return bAvailable;

    /* enumerate for joystick devices */
    hr = IDirectInput_EnumDevices(di, DIDEVTYPE_JOYSTICK,
                                  inputEnumDeviceProc,
                                  &guidDevice,
                                  DIEDFL_ATTACHEDONLY);
    if (FAILED(hr))
    {
       return FALSE;
    }

    /* Are there any joysticks attached? */
    if (IsEqualGUID(&guidDevice, &guidNULL))
    {
        return FALSE;
    }

    hr = IDirectInput_CreateDevice(di, &guidDevice, &didTemp, NULL);
    if (FAILED(hr))
    {
        return FALSE;
    }

    /* Determine if DX5 is available by a QI for a DX5 interface. */
    hr = IDirectInputDevice_QueryInterface(didTemp,
                                           &IID_IDirectInputDevice2,
                                           (void**)&didJoystick);
    if (FAILED(hr))
    {
        bAvailable = FALSE;
    }
    else
    {
        bAvailable = TRUE;
        IDirectInputDevice_Release(didJoystick);
    }

    /* dispose of the temp interface */
    IDirectInputDevice_Release(didTemp);

    return bAvailable;
}

static BOOL DIJoystick_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    return FALSE;
}

int DIJoystick_GetNumPhysicalJoysticks()
{
    return This.num_joysticks;
}

char* DIJoystick_GetPhysicalJoystickName(int num_joystick)
{
    return This.joysticks[num_joystick].name;
}

int DIJoystick_GetNumPhysicalJoystickAxes(int num_joystick)
{
    return This.joysticks[num_joystick].num_axes;
}

char* DIJoystick_GetPhysicalJoystickAxisName(int num_joystick, int num_axis)
{
    return This.joysticks[num_joystick].axes[num_axis].name;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

BOOL CALLBACK DIJoystick_EnumDeviceProc(LPDIDEVICEINSTANCE pdidi, LPVOID pv)
{
   char buffer[5000];

   This.joysticks[This.num_joysticks].guidDevice = pdidi->guidInstance;

   sprintf(buffer, "%s (%s)", pdidi->tszProductName, pdidi->tszInstanceName);
   This.joysticks[This.num_joysticks].name = (char *)malloc(strlen(buffer) + 1);
   strcpy(This.joysticks[This.num_joysticks].name, buffer);

   This.num_joysticks++;

   return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumAxisObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                                    LPVOID pvRef)
{
    joystick_type* joystick = (joystick_type*)pvRef;
    DIPROPRANGE diprg;
    HRESULT hr;

    joystick->axes[joystick->num_axes].guid = lpddoi->guidType;

    joystick->axes[joystick->num_axes].name = (char *)malloc(strlen(lpddoi->tszName) + 1);
    strcpy(joystick->axes[joystick->num_axes].name, lpddoi->tszName);

    joystick->axes[joystick->num_axes].offset = lpddoi->dwOfs;

    /*ErrorMsg("got axis %s, offset %i",lpddoi->tszName, lpddoi->dwOfs);*/

    diprg.diph.dwSize       = sizeof(diprg);
    diprg.diph.dwHeaderSize = sizeof(diprg.diph);
    diprg.diph.dwObj        = lpddoi->dwOfs;
    diprg.diph.dwHow        = DIPH_BYOFFSET;
    diprg.lMin              = 0;
    diprg.lMax              = 255;

    hr = IDirectInputDevice2_SetProperty(joystick->did, DIPROP_RANGE, &diprg.diph);
    if (FAILED(hr)) /* if this fails, don't use this axis */
    {
        free(joystick->axes[joystick->num_axes].name);
        joystick->axes[joystick->num_axes].name = NULL;
        return DIENUM_CONTINUE;
    }

#ifdef JOY_DEBUG
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput SetProperty() joystick axis %s failed - %s\n",
                 joystick->axes[joystick->num_axes].name,
                 DirectXDecodeError(hr));
    }
#endif
    
    /* Set axis dead zone to 0; we need accurate #'s for analog joystick reading. */
   
    hr = SetDIDwordProperty(joystick->did, DIPROP_DEADZONE, lpddoi->dwOfs, DIPH_BYOFFSET, 0);

#ifdef JOY_DEBUG
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput SetProperty() joystick axis %s dead zone failed - %s\n",
                 joystick->axes[joystick->num_axes].name,
                 DirectXDecodeError(hr));
    }
#endif

    joystick->num_axes++;
   
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumPOVObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    joystick_type* joystick = (joystick_type*)pvRef;
    joystick->num_pov++;
   
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumButtonObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    joystick_type* joystick = (joystick_type*)pvRef;
    joystick->num_buttons++;

    return DIENUM_CONTINUE;
}

static void ClearJoyState(DIJOYSTATE *pdijs)
{
    memset(pdijs, 0, sizeof(DIJOYSTATE));
    pdijs->lX           = 128;
    pdijs->lY           = 128;
    pdijs->lZ           = 128;
    pdijs->lRx          = 128;
    pdijs->lRy          = 128;
    pdijs->lRz          = 128;
    pdijs->rglSlider[0] = 128;
    pdijs->rglSlider[1] = 128;
    pdijs->rgdwPOV[0]   = -1;
    pdijs->rgdwPOV[1]   = -1;
    pdijs->rgdwPOV[2]   = -1;
    pdijs->rgdwPOV[3]   = -1;
}

static void InitJoystick(joystick_type *joystick)
{
    LPDIRECTINPUTDEVICE didTemp;
    HRESULT hr;

    joystick->use_joystick = FALSE;

    joystick->did      = NULL;
    joystick->num_axes = 0;

    joystick->is_light_gun = (strcmp(joystick->name, "ACT LABS GS (ACT LABS GS)") == 0);

    /* get a did1 interface first... */
    hr = IDirectInput_CreateDevice(di, &joystick->guidDevice, &didTemp, NULL);
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput CreateDevice() joystick failed: %s\n", DirectXDecodeError(hr));
        return;
    }
    
    /* get a did2 interface to work with polling (most) joysticks */
    hr = IDirectInputDevice_QueryInterface(didTemp,
                                           &IID_IDirectInputDevice2,
                                           (void**)&joystick->did);

    /* dispose of the temp interface */
    IDirectInputDevice_Release(didTemp);

    /* check result of getting the did2 */
    if (FAILED(hr))
    {
        /* no error message because this happens in dx3 */
        /* ErrorMsg("DirectInput QueryInterface joystick failed\n"); */
        joystick->did = NULL;
        return;
    }

    
    hr = IDirectInputDevice2_SetCooperativeLevel(joystick->did, MAME32App.m_hWnd,
                                                 DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput SetCooperativeLevel() joystick failed: %s\n", DirectXDecodeError(hr));
        return;
    }


    hr = IDirectInputDevice2_SetDataFormat(joystick->did, &c_dfDIJoystick);
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput SetDataFormat() joystick failed: %s\n", DirectXDecodeError(hr));
        return;
    }

    if (joystick->is_light_gun)
    {
       /* setup light gun to report raw screen pixel data */

       DIPROPDWORD diprop;
       memset(&diprop, 0, sizeof(diprop));
       diprop.diph.dwSize       = sizeof(DIPROPDWORD);
       diprop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
       diprop.diph.dwObj        = 0;
       diprop.diph.dwHow        = DIPH_DEVICE;
       diprop.dwData            = DIPROPCALIBRATIONMODE_RAW;
       
       IDirectInputDevice2_SetProperty(joystick->did, DIPROP_CALIBRATIONMODE, &diprop.diph);
    }
    else
    {
        /* enumerate our axes */
        hr = IDirectInputDevice_EnumObjects(joystick->did,
                                            DIJoystick_EnumAxisObjectsProc,
                                            joystick,
                                            DIDFT_AXIS);
        if (FAILED(hr))
        {
            ErrorMsg("DirectInput EnumObjects() Axes failed: %s\n", DirectXDecodeError(hr));
            return;
        }

        /* enumerate our POV hats */
        joystick->num_pov = 0;
        hr = IDirectInputDevice_EnumObjects(joystick->did,
                                            DIJoystick_EnumPOVObjectsProc,
                                            joystick,
                                            DIDFT_POV);
        if (FAILED(hr))
        {
            ErrorMsg("DirectInput EnumObjects() POVs failed: %s\n", DirectXDecodeError(hr));
            return;
        }
    }

    /* enumerate our buttons */

    joystick->num_buttons = 0;
    hr = IDirectInputDevice_EnumObjects(joystick->did,
                                        DIJoystick_EnumButtonObjectsProc,
                                        joystick,
                                        DIDFT_BUTTON);
    if (FAILED(hr))
    {
        ErrorMsg("DirectInput EnumObjects() Buttons failed: %s\n", DirectXDecodeError(hr));
        return;
    }

    hr = IDirectInputDevice2_Acquire(joystick->did);
    if (FAILED(hr)) 
    {
        ErrorMsg("DirectInputDevice Acquire joystick failed!\n");
        return;
    }

    /* start by clearing the structures */

    ClearJoyState(&joystick->dijs);

    joystick->use_joystick = TRUE;
}

static void ExitJoystick(joystick_type *joystick)
{
    DWORD i;
    
    if (joystick->did != NULL)
    {
        IDirectInputDevice_Unacquire(joystick->did);
        IDirectInputDevice_Release(joystick->did);
        joystick->did = NULL;
    }
    
    for (i=0;i<joystick->num_axes;i++)
    {
        if (joystick->axes[i].name)
            free(joystick->axes[i].name);
        joystick->axes[i].name = NULL;
    }
    
    if (joystick->name != NULL)
    {
        free(joystick->name);
        joystick->name = NULL;
    }
}

static void DIJoystick_InitJoyList(void)
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
    static char joynames[MAX_JOY][MAX_JOY_NAME_LEN + 1]; /* will be used to store names */
    static const char* JoyPOVName[] = { "Forward", "Backward", "Right", "Left"};
    int tot, i, j, pov;
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

    for (i = 0; i < This.num_joysticks; i++)
    {
        for (j = 0; j < This.joysticks[i].num_axes; j++)
        {
            sprintf(buf, "J%d %.7s %s -",
                    i + 1,
                    This.joysticks[i].name,
                    This.joysticks[i].axes[j].name);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_AXIS, j + 1, JOYCODE_DIR_NEG);
            tot++;
            
            sprintf(buf, "J%d %.7s %s +",
                    i + 1,
                    This.joysticks[i].name,
                    This.joysticks[i].axes[j].name);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_AXIS, j + 1, JOYCODE_DIR_POS);
            tot++;
        }
        for (j = 0; j < This.joysticks[i].num_buttons; j++)
        {
            sprintf(buf, "J%d Button %d", i + 1, j);
            strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
            joynames[tot][MAX_JOY_NAME_LEN] = 0;
            joylist[tot].name = joynames[tot];
            joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_BTN, j + 1, JOYCODE_DIR_BTN);
            tot++;
        }
        for (pov=0;pov<This.joysticks[i].num_pov;pov++)
        {
            for (j = 0; j < 4; j++)
            {
                sprintf(buf, "J%i POV%i %s", i + 1, pov + 1, JoyPOVName[j]);
                strncpy(joynames[tot], buf, MAX_JOY_NAME_LEN);
                joynames[tot][MAX_JOY_NAME_LEN] = 0;
                joylist[tot].name = joynames[tot];
                joylist[tot].code = JOYCODE(i + 1, JOYCODE_STICK_POV, 4 * pov + j, JOYCODE_DIR_BTN);
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
