/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  LED.h

 ***************************************************************************/

#ifndef __LED_H__
#define __LED_H__

extern void LED_init(void);
extern void LED_exit(void);
extern void LED_write(int nLED, BOOL bOn);
extern void LED_StatusWrite(int leds_status);

#endif
