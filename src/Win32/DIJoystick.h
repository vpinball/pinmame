/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DIJOYSTICK_H__
#define __DIJOYSTICK_H__

#include "joystick.h"

extern struct OSDJoystick DIJoystick;

BOOL DIJoystick_IsHappInterface();
BOOL DIJoystick_KeyPressed(int osd_key);

int DIJoystick_GetNumPhysicalJoysticks();
char * DIJoystick_GetPhysicalJoystickName(int num_joystick);

int DIJoystick_GetNumPhysicalJoystickAxes(int num_joystick);
char * DIJoystick_GetPhysicalJoystickAxisName(int num_joystick,int num_axis);

#endif
