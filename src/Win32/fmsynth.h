/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __FMSYNTH_H__
#define __FMSYNTH_H__

/*#define MAX_OPLCHIP 2  /* SOUND BLASTER 16 or compatible ?? */
#define MAX_OPLCHIP 1  /* SOUND BLASTER pro compatible ??  */

struct OSDFMSynth
{
    int  (*init)(options_type *options);
    void (*exit)(void);
    void (*opl_control)(int chip, int reg);
    void (*opl_write)(int chip, int data);
};

extern struct OSDFMSynth FMSynth;

#endif
