/***************************************************************************

   M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  NullSound.c

 ***************************************************************************/

#include "mame.h"
#include "osdepend.h"
#include "mame32.h"
#include "NullSound.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int      NullSound_init(options_type *options);
static void     NullSound_exit(void);
static int      NullSound_start_audio_stream(int stereo);
static int      NullSound_update_audio_stream(INT16* buffer);
static void     NullSound_stop_audio_stream(void);
static void     NullSound_set_mastervolume(int volume);
static int      NullSound_get_mastervolume(void);
static void     NullSound_sound_enable(int enable);
static void     NullSound_update_audio(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDSound NullSound = 
{
    NullSound_init,                     /* init                    */
    NullSound_exit,                     /* exit                    */
    NullSound_start_audio_stream,       /* start_audio_stream      */
    NullSound_update_audio_stream,      /* update_audio_stream     */
    NullSound_stop_audio_stream,        /* stop_audio_stream       */
    NullSound_set_mastervolume,         /* set_mastervolume        */
    NullSound_get_mastervolume,         /* get_mastervolume        */
    NullSound_sound_enable,             /* sound_enable            */
    NullSound_update_audio              /* update_audio            */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static unsigned int m_nAttenuation;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static int NullSound_init(options_type *options)
{
    /* update the Machine structure to show that sound is disabled */
    Machine->sample_rate = 0;
    m_nAttenuation = 0;
    return 0;
}

static void NullSound_exit(void)
{

}

static int NullSound_start_audio_stream(int stereo)
{
    return 0;
}

static int NullSound_update_audio_stream(INT16* buffer)
{
    return 0;
}

static void NullSound_stop_audio_stream(void)
{

}

static void NullSound_set_mastervolume(int attenuation)
{
    m_nAttenuation = attenuation;
}

static int NullSound_get_mastervolume()
{
    return m_nAttenuation;
}

static void NullSound_sound_enable(int enable)
{

}

static void NullSound_update_audio(void)
{
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

