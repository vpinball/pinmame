/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DIRECTSOUND_H__
#define __DIRECTSOUND_H__

#include "ControllerOptions.h"

struct OSDSound
{
    int     (*init)(PCONTROLLEROPTIONS pControllerOptions);
    void    (*exit)(void);
    int     (*start_audio_stream)(int stereo);
    int     (*update_audio_stream)(INT16* buffer);
    void    (*stop_audio_stream)(void);
    void    (*set_mastervolume)(int attenuation);
    int     (*get_mastervolume)(void);
    void    (*sound_enable)(int enable);
    void    (*update_audio)(void);
};

extern struct OSDSound DirectSound;

#endif
