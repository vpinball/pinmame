/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  MidasSound.c

 ***************************************************************************/

#include "driver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mame.h"
#include "MAME32.h"
#include "osdepend.h"
#include "MidasSound.h"
#include "M32Util.h"
#include "midasdll.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int      MidasSound_init(options_type *options);
static void     MidasSound_exit(void);
static int      MidasSound_start_audio_stream(int stereo);
static int      MidasSound_update_audio_stream(INT16* buffer);
static void     MidasSound_stop_audio_stream(void);
static void     MidasSound_set_mastervolume(int attenuation);
static int      MidasSound_get_mastervolume(void);
static void     MidasSound_sound_enable(int enable);
static void     MidasSound_update_audio(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDSound MIDASSound = 
{
    MidasSound_init,                    /* init                    */
    MidasSound_exit,                    /* exit                    */
    MidasSound_start_audio_stream,      /* start_audio_stream      */
    MidasSound_update_audio_stream,     /* update_audio_stream     */
    MidasSound_stop_audio_stream,       /* stop_audio_stream       */
    MidasSound_set_mastervolume,        /* set_mastervolume        */
    MidasSound_get_mastervolume,        /* get_mastervolume        */
    MidasSound_sound_enable,            /* sound_enable            */
    MidasSound_update_audio             /* update_audio            */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tSound_private
{
    MIDASstreamHandle   m_hStream;
    BOOL                m_bInitialized;
    DWORD               m_dwMasterVolume;
    int                 m_nAttenuation;
    int                 m_nSamplesPerFrame;
    int                 m_nBytesPerFrame;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tSound_private    This;
  
/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int MidasSound_init(options_type *options)
{
    BOOL    bResult;
    DWORD   dwOutputMode;
    DWORD   dwMixRate;
    DWORD   dwMixBufLen;
    DWORD   dwMixBufBlocks;
    DWORD   dwFilterMode;
    DWORD   dwMixingMode;
    DWORD   dwDSoundMode;
    DWORD   dwDSoundBufLen;

    This.m_hStream           = NULL;
    This.m_bInitialized      = FALSE;
    This.m_dwMasterVolume    = 100;
    This.m_nAttenuation      = 0;
    This.m_nSamplesPerFrame  = 0;
    This.m_nBytesPerFrame    = 0;

    /* This function must be called before ANY other MIDAS function. */
    bResult = MIDASstartup();
    if (bResult == FALSE)
        goto error;

    /*
        "The output mode should normally be set to a 16-bit mode,
        as using 8-bit modes does not decrease CPU usage, only sound quality.
        Using mono output instead of stereo, however, can decrease
        CPU usage by up to 50%."
    */
    if (options->stereo == TRUE
    && (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO))
        dwOutputMode = MIDAS_MODE_16BIT_STEREO;
    else
        dwOutputMode = MIDAS_MODE_16BIT_MONO;

    dwMixRate       = Machine->sample_rate;
    dwMixBufLen     = 200; /* MIDAS Default of 500 has too much latency. */
    dwMixBufBlocks  = MIDASgetOption(MIDAS_OPTION_MIXBUFBLOCKS);
    dwFilterMode    = MIDASgetOption(MIDAS_OPTION_FILTER_MODE);
    dwMixingMode    = MIDASgetOption(MIDAS_OPTION_MIXING_MODE);
    dwDSoundMode    = MIDAS_DSOUND_STREAM;
    dwDSoundBufLen  = MIDASgetOption(MIDAS_OPTION_DSOUND_BUFLEN);

    /*
        All MIDAS configuration options must be set before MIDASinit() is called.
    */
    MIDASsetOption(MIDAS_OPTION_DSOUND_HWND,    (DWORD)MAME32App.m_hWnd);
    MIDASsetOption(MIDAS_OPTION_OUTPUTMODE,     dwOutputMode);
    MIDASsetOption(MIDAS_OPTION_MIXRATE,        dwMixRate);
    MIDASsetOption(MIDAS_OPTION_MIXBUFLEN,      dwMixBufLen);
    MIDASsetOption(MIDAS_OPTION_MIXBUFBLOCKS,   dwMixBufBlocks);
    MIDASsetOption(MIDAS_OPTION_FILTER_MODE,    dwFilterMode);
    MIDASsetOption(MIDAS_OPTION_MIXING_MODE,    dwMixingMode);
    MIDASsetOption(MIDAS_OPTION_DSOUND_MODE,    dwDSoundMode);
    MIDASsetOption(MIDAS_OPTION_DSOUND_BUFLEN,  dwDSoundBufLen);

    bResult = MIDASinit();
    if (bResult == FALSE)
        goto error;

    bResult = MIDASstartBackgroundPlay(4 * 60);
    if (bResult == FALSE)
        goto error;

    bResult = MIDASopenChannels(1);
    if (bResult == FALSE)
        goto error;
    
    /* update the Machine structure to reflect the actual sample rate */
    Machine->sample_rate = MIDASgetOption(MIDAS_OPTION_MIXRATE);

/*
    printf("MIDAS Sound Options:\n");
    printf("outputmode:    %d\n", MIDASgetOption(MIDAS_OPTION_OUTPUTMODE));
    printf("mixrate:       %d\n", MIDASgetOption(MIDAS_OPTION_MIXRATE));
    printf("mixbuflen:     %d\n", MIDASgetOption(MIDAS_OPTION_MIXBUFLEN));
    printf("mixbufblocks:  %d\n", MIDASgetOption(MIDAS_OPTION_MIXBUFBLOCKS));
    printf("filtermode:    %d\n", MIDASgetOption(MIDAS_OPTION_FILTER_MODE));
    printf("mixingmode:    %d\n", MIDASgetOption(MIDAS_OPTION_MIXING_MODE));
    printf("dsoundmode:    %d\n", MIDASgetOption(MIDAS_OPTION_DSOUND_MODE));
    printf("dsoundbuflen:  %d\n", MIDASgetOption(MIDAS_OPTION_DSOUND_BUFLEN));
*/

    MIDASsetAmplification(This.m_dwMasterVolume);

    This.m_bInitialized = TRUE;
    return 0;

error:
    ErrorMsg("MIDAS: %s", MIDASgetErrorMessage(MIDASgetLastError()));
    /* update the Machine structure to show that sound is disabled */
    Machine->sample_rate = 0;
    return 1;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void MidasSound_exit(void)
{
    if (This.m_bInitialized == FALSE)
        return;

    if (This.m_hStream != NULL)
        MIDASstopStream(This.m_hStream);
    This.m_hStream = NULL;

    MIDAScloseChannels();
    MIDASstopBackgroundPlay();
    MIDASclose();

    This.m_bInitialized = FALSE;
}

/*
    osd_start_audio_stream() is called at the start of the emulation to
    initialize the output stream.
    
    osd_update_audio_stream() is called every frame to feed new data.
    
    osd_stop_audio_stream() is called when the emulation is stopped.

    The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.

    When the stream is stereo, left and right samples are alternated in the
    stream.

    osd_start_audio_stream() and osd_stop_audio_stream() must return the number of
    samples (or couples of samples, when using stereo) required for next frame.
    This will be around Machine->sample_rate / Machine->drv->frames_per_second,
    the code may adjust it at will to keep timing accurate and to maintain audio
    and video in sync when using vsync.
*/
static int MidasSound_start_audio_stream(int stereo)
{
    unsigned int nBufLength;

    assert(This.m_bInitialized == TRUE);
    if (This.m_bInitialized == FALSE)
        return 0;

    if (Machine->sample_rate == 0)
        return 0;

    if (stereo)
        stereo = 1; /* make sure it's either 0 or 1 */

    /* determine the number of samples and bytes per frame */
    This.m_nSamplesPerFrame = (double)Machine->sample_rate / Machine->drv->frames_per_second;
    This.m_nBytesPerFrame   = This.m_nSamplesPerFrame * sizeof(INT16) * (stereo + 1);

    /* Stream playback buffer length in milliseconds */
    nBufLength = 8.0 * 1000.0 / Machine->drv->frames_per_second;

    This.m_hStream = MIDASplayStreamPolling((stereo) ? MIDAS_SAMPLE_16BIT_STEREO :
                                                       MIDAS_SAMPLE_16BIT_MONO,
                                            Machine->sample_rate,
                                            nBufLength);
    if (This.m_hStream == NULL)
    {
        ErrorMsg("MIDASplayStreamPolling: %s", MIDASgetErrorMessage(MIDASgetLastError()));
        return 0;
    }

    return This.m_nSamplesPerFrame;
}

static int MidasSound_update_audio_stream(INT16* buffer)
{
    assert(This.m_bInitialized == TRUE);
    assert(Machine->sample_rate != 0);

    MIDASfeedStreamData(This.m_hStream,
                        (unsigned char*)buffer,
                        This.m_nBytesPerFrame,
                        FALSE);
    
    return This.m_nSamplesPerFrame;
}

static void MidasSound_stop_audio_stream(void)
{
    assert(This.m_bInitialized == TRUE);

    if (This.m_hStream != NULL)
        MIDASstopStream(This.m_hStream);
    This.m_hStream = NULL;
}

/*
    Control master volume, attenuation is the attenuation in dB
    (a negative number).
 */
static void MidasSound_set_mastervolume(int attenuation)
{
    float volume;

    assert(This.m_bInitialized == TRUE);

    This.m_nAttenuation = attenuation;

    volume = 100.0f;
    while (attenuation++ < 0)
        volume /= 1.122018454f;   /* = (10 ^ (1/20)) = 1dB */

    This.m_dwMasterVolume = (int)volume;
    MIDASsetAmplification(This.m_dwMasterVolume);
}

static int MidasSound_get_mastervolume()
{
    assert(This.m_bInitialized == TRUE);

    return This.m_nAttenuation;
}

static void MidasSound_sound_enable(int enable)
{
    assert(This.m_bInitialized == TRUE);

    if (enable)
        MIDASsetAmplification(This.m_dwMasterVolume);
    else
        MIDASsetAmplification(0);
}

static void MidasSound_update_audio(void)
{

}

/***************************************************************************
    Internal functions
 ***************************************************************************/
