/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse

    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  DirectSound.c

  Since the MAME core does sound mixing, all we do is setup the primary
  buffer to the same output format as the core is generating, and then
  create a secondary streaming buffer that we write all data to.

 ***************************************************************************/

#include "driver.h"

#define WIN32_LEAN_AND_MEAN
#include <math.h>
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#include "mame.h"

#include "OSDepend.h"
#include "M32Util.h"
#include "DirectSound.h"
#include "DXDecode.h"
#include "Display.h"

extern HWND	g_hMainWnd;
#define SAMPLELOG
#define MAX_SAMPLE_ADD 16
/***************************************************************************
    function prototypes
 ***************************************************************************/

static int      DirectSound_init(PCONTROLLEROPTIONS pControllerOptions);
static void     DirectSound_exit(void);
static int      DirectSound_start_audio_stream(int stereo);
static int      DirectSound_update_audio_stream(INT16* buffer);
static void     DirectSound_stop_audio_stream(void);
static void     DirectSound_set_mastervolume(int volume);
static int      DirectSound_get_mastervolume(void);
static void     DirectSound_sound_enable(int enable);
static void     DirectSound_update_audio(void);
#ifdef SAMPLELOG
static int sampleAddLog[MAX_SAMPLE_ADD*2+2];
#define LOGOVERRUN (MAX_SAMPLE_ADD*2)
#define LOGUNDERRUN (MAX_SAMPLE_ADD*2+1)

#endif
/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDSound DirectSound =
{
    { DirectSound_init },                     /* init                    */
    { DirectSound_exit },                     /* exit                    */
    { DirectSound_start_audio_stream },       /* start_audio_stream      */
    { DirectSound_update_audio_stream },      /* update_audio_stream     */
    { DirectSound_stop_audio_stream },        /* stop_audio_stream       */
    { DirectSound_set_mastervolume },         /* set_mastervolume        */
    { DirectSound_get_mastervolume },         /* get_mastervolume        */
    { DirectSound_sound_enable },             /* sound_enable            */
    { DirectSound_update_audio }              /* update_audio            */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

#define BUFFER_SIZE_MILLIS 200 /* our play buffer is this long in milliseconds */

/* keep ahead by this many samples, to help prevent gaps */
#define EXTRA_SAMPLES 0


/***************************************************************************
    Internal variables
 ***************************************************************************/

static HANDLE hDLL;
static LPDIRECTSOUND ds;

static LPDIRECTSOUNDBUFFER dsb;
static BOOL is_stereo;
static int stereo_factor; // 1 = mono 2 = stereo, used for multiplying
static int buffer_length; // in bytes
static int voice_pos; // in bytes

/* global sample tracking */
static BOOL new_sound_data;

static INT16 *stream_cache_data;
static int stream_cache_len;

static double samples_per_frame;
static double samples_left_over;
static UINT32 samples_this_frame;


static int attenuation = 0;

/***************************************************************************
    External OSD functions
 ***************************************************************************/

typedef HRESULT (WINAPI *dsc_proc)(GUID FAR *lpGUID,LPDIRECTSOUND FAR *lplpDS,
                                   IUnknown FAR *pUnkOuter);

static int DirectSound_init(PCONTROLLEROPTIONS pControllerOptions)
{
	// now attempt to create it
	if ( FAILED(DirectSoundCreate(NULL, &ds, NULL)) ) {
        ErrorMsg("Unable to initialize DirectSound");
        return 1;
	}

	attenuation = 0;
    new_sound_data = FALSE;
	return 0;
}


static void DirectSound_exit(void)
{
   if (hDLL == NULL)
       return;

   if (dsb != NULL)
   {
       IDirectSoundBuffer_Stop(dsb);
       IDirectSoundBuffer_Release(dsb);
       dsb = NULL;
   }

   if (ds != NULL)
   {
       IDirectSound_Release(ds);
       ds = NULL;
   }
}

static int DirectSound_start_audio_stream(int stereo)
{
    HRESULT hr;

    DSBUFFERDESC dsbd;
    LPDIRECTSOUNDBUFFER primary_buffer;
    WAVEFORMATEX wfx;

    VOID *area1,*area2;
    DWORD len_area1,len_area2;


    is_stereo = stereo;
    stereo_factor = (is_stereo ? 2 : 1);

	/* determine the number of samples per frame */
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;

	/* compute how many samples to generate this frame */
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over + EXTRA_SAMPLES;
	samples_left_over -= (double)samples_this_frame;

    hr = IDirectSound_SetCooperativeLevel(ds, g_hMainWnd, DSSCL_PRIORITY);
    if (FAILED(hr))
        ErrorMsg("Unable to set priority cooperative level for sound: %s",DirectXDecodeError(hr));


    memset(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    hr = IDirectSound_CreateSoundBuffer(ds,&dsbd,&primary_buffer,NULL);
    if (FAILED(hr))
    {
        ErrorMsg("Unable to create primary sound buffer: %s",DirectXDecodeError(hr));
        return 1;
    }

    memset(&wfx,0,sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (stereo != 0) ? 2 : 1;
    wfx.nSamplesPerSec = Machine->sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    hr = IDirectSoundBuffer_SetFormat(primary_buffer,&wfx);

    if (FAILED(hr))
    {
        ErrorMsg("Unable to set format of primary sound buffer: %s",DirectXDecodeError(hr));
        IDirectSoundBuffer_Release(primary_buffer);
        return 1;
    }
    IDirectSoundBuffer_Play(primary_buffer,0,0,DSBPLAY_LOOPING);
    IDirectSoundBuffer_Release(primary_buffer);


    hr = IDirectSound_SetCooperativeLevel(ds,g_hMainWnd,DSSCL_NORMAL);
    if (FAILED(hr))
        ErrorMsg("Unable to set normal cooperative level for sound: %s",DirectXDecodeError(hr));

    wfx.nChannels = stereo_factor;

    dsb = NULL;

    memset(&dsbd,0,sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2   // Always a good idea
       | DSBCAPS_CTRLPAN
       | DSBCAPS_CTRLVOLUME
       | DSBCAPS_GLOBALFOCUS;        // Allows background playing


    buffer_length = Machine->sample_rate*sizeof(INT16)*stereo_factor*BUFFER_SIZE_MILLIS/1000;
    // sound pukes if it's not aligned to 16 byte length
    buffer_length = (buffer_length + 15) & ~15;

    dsbd.dwBufferBytes = buffer_length;
    dsbd.lpwfxFormat = &wfx;

    hr = IDirectSound_CreateSoundBuffer(ds,&dsbd,&dsb,NULL);
    if (FAILED(hr))
    {
        ErrorMsg("Unable to create secondary sound buffer: %s",DirectXDecodeError(hr));
        dsb = NULL;
        return 0;
    }

    // clear it out, because we start playing it instantly
    hr = IDirectSoundBuffer_Lock(dsb,0,buffer_length,&area1,&len_area1,&area2,&len_area2,0);
    if (FAILED(hr))
    {
        ErrorMsg("Unable to lock secondary sound buffer: %s",DirectXDecodeError(hr));
    }
    else
    {
        memset(area1,0,len_area1);
        memset(area2,0,len_area2);

        hr = IDirectSoundBuffer_Unlock(dsb,area1,len_area1,area2,len_area2);
        if (FAILED(hr))
            ErrorMsg("Unable to unlock secondary sound buffer: %s",DirectXDecodeError(hr));
    }

    hr = IDirectSoundBuffer_Play(dsb,0,0,DSBPLAY_LOOPING);
    if (FAILED(hr))
        ErrorMsg("Unable to play secondary sound buffer: %s",DirectXDecodeError(hr));

    voice_pos = 0;

    return samples_this_frame;
}

static int DirectSound_update_audio_stream(INT16* buffer)
{
    new_sound_data = TRUE;

	/* compute how many samples to generate next frame */
    stream_cache_data = buffer;
	stream_cache_len = samples_this_frame;

	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over + EXTRA_SAMPLES;
	samples_left_over -= (double)samples_this_frame;

	return samples_this_frame;
}

static void DirectSound_stop_audio_stream(void)
{
    if (dsb != NULL)
    {
        IDirectSoundBuffer_Stop(dsb);
        IDirectSoundBuffer_Release(dsb);
        dsb = NULL;
    }
#ifdef SAMPLELOG
	{
		int ii;
		FILE *f = fopen("addLog.log","w");
		for (ii = 0; ii < MAX_SAMPLE_ADD*2; ii++)
			fprintf(f,"%3d %6d\n",ii-MAX_SAMPLE_ADD,sampleAddLog[ii]);
		fprintf(f,"Over-run  %6d\n",sampleAddLog[LOGOVERRUN]);
		fprintf(f,"Under-run %6d\n",sampleAddLog[LOGUNDERRUN]);
		fclose(f);
		memset(sampleAddLog,0,sizeof(sampleAddLog));
	}
#endif
}

static void DirectSound_set_mastervolume(int volume)
{
	attenuation = volume;

    if (attenuation != -1 && dsb != NULL)
    {
        HRESULT hr;

        hr = IDirectSoundBuffer_SetVolume(dsb,100*attenuation);
        if (FAILED(hr))
            ErrorMsg("Unable to set volume %s",DirectXDecodeError(hr));
	}
}


static int DirectSound_get_mastervolume()
{
   return attenuation;
}

static void DirectSound_sound_enable(int enable)
{
	if (enable)
	{
		osd_set_mastervolume(attenuation);
	}
	else
	{
        if (dsb != NULL)
        {
            HRESULT hr;

            hr = IDirectSoundBuffer_SetVolume(dsb, DSBVOLUME_MIN);
            if (FAILED(hr))
                ErrorMsg("Unable to set volume %s", DirectXDecodeError(hr));
	    }
	}
}

static void DirectSound_update_audio(void) {
  static int lastmargin = 0, sampleadd = 0;
  HRESULT hr;
  int nvoice_pos;
  int length = stream_cache_len*sizeof(INT16)*stereo_factor;

  if (dsb == NULL || new_sound_data == FALSE) return;

  profiler_mark(PROFILER_MIXER);

  nvoice_pos = voice_pos + length; if (nvoice_pos >= buffer_length) nvoice_pos -= buffer_length;

  if (Display_Throttled()) { /* sync with audio only when speed throttling is not turned off */
    LONG playpos, writepos;

    IDirectSoundBuffer_GetCurrentPosition(dsb,&playpos,&writepos);
    // Check if voice_pos is between playpos & writepos
    if (((playpos < writepos) && ((voice_pos >= playpos) && (voice_pos < writepos))) ||
        ((playpos > writepos) && ((voice_pos >= playpos) || (voice_pos < writepos)))) {
			// We are behind the playing. Skip the missing samples
			voice_pos = writepos;
#ifdef SAMPLELOG
			sampleAddLog[LOGUNDERRUN] += 1;
#endif
		}
		// Check if nvoice_pos is between playpos & writepos
    if (((playpos < writepos) && ((nvoice_pos >= playpos) && (nvoice_pos < writepos))) ||
        ((playpos > writepos) && ((nvoice_pos >= playpos) || (nvoice_pos < writepos)))) {
			// We are playing to fast??. Down't overwrite current area
			length = playpos - voice_pos; if (length < 0) length += buffer_length;
			nvoice_pos = playpos; sampleadd = 0;
#ifdef SAMPLELOG
			sampleAddLog[LOGOVERRUN] += 1;
#endif
		}
    {	// see if we need to speed up or slow down sample generating
	  int margin = voice_pos - playpos;
  	  if (margin < 0) margin += buffer_length;
      margin = (margin - buffer_length/2)*MAX_SAMPLE_ADD*2/buffer_length;
	  sampleadd = -margin;
  	  lastmargin = margin; samples_left_over = sampleadd;
#ifdef SAMPLELOG
  	  sampleAddLog[sampleadd+MAX_SAMPLE_ADD]++;
#endif
    }
  }
  {
    VOID *area1,*area2;
    DWORD len_area1,len_area2;
    hr = IDirectSoundBuffer_Lock(dsb, voice_pos, length, &area1, &len_area1, &area2, &len_area2, 0);
    if (FAILED(hr)) ErrorMsg("Unable to lock secondary sound buffer: %s",DirectXDecodeError(hr));
    else {
      memcpy(area1, stream_cache_data, len_area1);
      memcpy(area2, ((byte *)stream_cache_data) + len_area1, len_area2);

      hr = IDirectSoundBuffer_Unlock(dsb,area1,len_area1,area2,len_area2);
      if (FAILED(hr)) ErrorMsg("Unable to unlock secondary sound buffer: %s",DirectXDecodeError(hr));
    }
	}
  voice_pos = nvoice_pos;
  new_sound_data = FALSE;
  profiler_mark(PROFILER_END);
}
