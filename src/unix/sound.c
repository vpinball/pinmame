/*
 * X-Mame sound code
 */

#include "xmame.h"
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_mixer.h"
#include "sysdep/sound_stream.h"
#include "driver.h"
#ifdef AVICAPTURE
#include <lame/lame.h>
#endif

/* #define SOUND_DEBUG */

static int sound_fake = 0;
static int sound_samplerate = 44100;
static float sound_bufsize = 3.0;
static int sound_attenuation = -3;
static char *sound_dsp_device = NULL;
static char *sound_mixer_device = NULL;
static struct sysdep_dsp_struct *sound_dsp = NULL;
static struct sysdep_mixer_struct *sound_mixer = NULL;
static int sound_samples_per_frame = 0;
static int type;

static int sound_set_options(struct rc_option *option, const char *arg,
   int priority)
{
  if(sound_enabled)
  {
     options.samplerate = sound_samplerate;
  }
  else if(sound_fake)
  {
     options.samplerate = 8000;
  }
  else
     options.samplerate = 0;
  
  option->priority = priority;
  
  return 0;
}

struct rc_option sound_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Sound Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "sound",		"snd",			rc_bool,	&sound_enabled,
     "1",		0,			0,		sound_set_options,
     "Enable/disable sound (if available)" },
   { "samples",		"sam",			rc_bool,	&options.use_samples,
     "1",		0,			0,		NULL,
     "Use/don't use samples (if available)" },
   { "fakesound",	"fsnd",			rc_set_int,	&sound_fake,
     NULL,		1,			0,		sound_set_options,
     "Generate sound even when sound is disabled, this is needed for some games which won't run without sound" },
   { "samplefreq",	"sf",			rc_int,		&sound_samplerate,
     "44100",		8000,			96000,		sound_set_options,
     "Set the playback sample-frequency/rate" },
   { "bufsize", 	"bs",			rc_float,	&sound_bufsize,
     "3.0",		1.0,			30.0,		NULL,
     "Number of frames of sound to buffer" },
   { "volume",		"v",			rc_int,		&sound_attenuation,
     "-3",		-32,			0,		NULL,
     "Set volume to <int> db, (-32 (soft) - 0(loud) )" },
   { "audiodevice",	"ad",			rc_string,	&sound_dsp_device,
     NULL,		0,			0,		NULL,
     "Use an alternative audiodevice" },
   { "mixerdevice",	"md",			rc_string,	&sound_mixer_device,
     NULL,		0,			0,		NULL,
     "Use an alternative mixerdevice" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

/* attenuation in dB */
void osd_set_mastervolume (int attenuation)
{
   float f = attenuation;
   
   if(!sound_mixer)
      return;
      
   f += 32.0;
   f *= 100.0;
   f /= 32.0;
   f += 0.50; /* for rounding */
#ifdef SOUND_DEBUG
   fprintf(stderr, "sound.c: setting volume to %d (%d)\n",
      attenuation, (int)f);
#endif
   
   sysdep_mixer_set(sound_mixer, SYSDEP_MIXER_PCM1, f, f);
}

int osd_get_mastervolume (void)
{
   int left, right;
   float f;
   
   if(!sound_mixer)
      return -32;
      
   if(sysdep_mixer_get(sound_mixer, SYSDEP_MIXER_PCM1, &left, &right))
      return -32;
      
   f = left;
   f *= 32.0;
   f /= 100.0;
   f -= 32.5; /* 32 + 0.5 for rounding */
#ifdef SOUND_DEBUG
   fprintf(stderr, "sound.c: got volume %d (%d)\n", (int)f, left);
#endif
   return f;
}

void osd_sound_enable (int enable_it)
{
   if (sound_stream && enable_it)
   {
      sound_enabled = 1;
      if (!sound_dsp)
      {
	 if (!(sound_dsp = sysdep_dsp_create(NULL,
	    sound_dsp_device,
	    &options.samplerate,
	    &type,
	    sound_bufsize * (1 / Machine->drv->frames_per_second),
	    SYSDEP_DSP_EMULATE_TYPE | SYSDEP_DSP_O_NONBLOCK)))
	    sound_enabled = 0;
	 else
	 {
	    sound_stream_destroy(sound_stream);
	    if (!(sound_stream = sound_stream_create(sound_dsp, type,
	       sound_samples_per_frame, 3)))
	    {
	       osd_stop_audio_stream();
	       sound_enabled = 0;
	    }
	 }
      }
   }
   else
   {
      if (sound_dsp)
      {
	 sysdep_dsp_destroy(sound_dsp);
	 sound_dsp = NULL;
      }
      sound_enabled = 0;
   }
}

int osd_start_audio_stream(int stereo)
{
   type = SYSDEP_DSP_16BIT | (stereo? SYSDEP_DSP_STEREO:SYSDEP_DSP_MONO);
   
   sound_stream = NULL;

   /* create dsp */
   if(sound_enabled)
   {
      if(!(sound_dsp = sysdep_dsp_create(NULL,
         sound_dsp_device,
         &options.samplerate,
         &type,
         sound_bufsize * (1 / Machine->drv->frames_per_second),
         SYSDEP_DSP_EMULATE_TYPE | SYSDEP_DSP_O_NONBLOCK)))
      {
         osd_stop_audio_stream();
         sound_enabled = 0;
      }
   }
   
   /* create sound_stream */
   if(sound_enabled)
   {
      /* sysdep_dsp_open may have changed the samplerate */
      Machine->sample_rate = options.samplerate;
      
      /* calculate samples_per_frame */
      sound_samples_per_frame = Machine->sample_rate /
         Machine->drv->frames_per_second;
#ifdef SOUND_DEBUG
      fprintf(stderr, "debug: sound: samples_per_frame = %d\n",
         sound_samples_per_frame);
#endif
      if(!(sound_stream = sound_stream_create(sound_dsp, type,
         sound_samples_per_frame, 3)))
      {
         osd_stop_audio_stream();
         sound_enabled = 0;
      }
   }

   /* if sound is not enabled, set the samplerate of the core to 0 */
   if(!sound_enabled)
   {
      if(sound_fake)
         Machine->sample_rate = options.samplerate = 8000;
      else
         Machine->sample_rate = options.samplerate = 0;
      
      /* calculate samples_per_frame */
      sound_samples_per_frame = Machine->sample_rate /
         Machine->drv->frames_per_second;
      
      return sound_samples_per_frame;
   }
   
   /* create a mixer instance */
   sound_mixer = sysdep_mixer_create(NULL, sound_mixer_device,
      SYSDEP_MIXER_RESTORE_SETTINS_ON_EXIT);
   
   /* check if the user specified a volume, and ifso set it */
   if(sound_mixer && rc_get_priority2(sound_opts, "volume"))
      osd_set_mastervolume(sound_attenuation);
   
   return sound_samples_per_frame;
}


#ifdef AVICAPTURE
static FILE *audio_fp = NULL;
static lame_global_flags* fl = NULL;
#endif

int osd_update_audio_stream(INT16 *buffer)
{
#ifdef AVICAPTURE
   static int times=0;

   if (sound_enabled)
   {
	   static int opened = 0;
      unsigned char audio_outbuf[100000];
	   int bytes;

	   if (!opened)
      {
	      opened=1;
	      fl = lame_init();
	      lame_set_in_samplerate( fl, Machine->sample_rate );
	      lame_set_num_channels( fl, (type & SYSDEP_DSP_STEREO) ? 2 : 1 );
	      lame_set_quality( fl, 5 );
	      lame_init_params( fl );
	      audio_fp = fopen("audio.outf","wb");
      }
   	
      if (type & SYSDEP_DSP_STEREO)
			bytes = lame_encode_buffer_interleaved (
				fl, buffer, sound_samples_per_frame, 
				audio_outbuf, sizeof(audio_outbuf));
		else
			bytes = lame_encode_buffer (
				fl, buffer, buffer, sound_samples_per_frame, 
				audio_outbuf, sizeof(audio_outbuf));
	   fwrite( audio_outbuf,1, bytes, audio_fp );
	}
#endif /* AVICAPTURE */
 
   /* sound enabled ? */
   if (sound_enabled)
      sound_stream_write(sound_stream, (unsigned char *)buffer,
         sound_samples_per_frame);
   
   return sound_samples_per_frame;
}

void osd_stop_audio_stream(void)
{
   if(sound_stream)
      sound_stream_destroy(sound_stream);
   
   if(sound_dsp)
      sysdep_dsp_destroy(sound_dsp);
   
   if(sound_mixer)
      sysdep_mixer_destroy(sound_mixer);

#ifdef AVICAPTURE
   if (audio_fp )
   {
     unsigned char buf[10000];
     int bytes = lame_encode_flush(fl, buf, sizeof(buf));
     fwrite( buf,1, bytes, audio_fp );
     fclose( audio_fp );
     lame_close(fl);
   }
#endif /* AVICAPTURE */

}
