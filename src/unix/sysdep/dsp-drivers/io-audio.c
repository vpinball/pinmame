/*

	Travis's Note: io-audio isnt out yet, so lets stick
	with David's old code till it's out.
	
	I called this io-audio.c for reasons that I dont
	even understand. If you can think of a better name
	for this file lemme know.

*/
/* Sysdep Alsa 5 driver : 
	Written by Dave Rempel

	email : drempel@qnx.com

Note...I've only tried this under Neutrino 2.1....
it should work for Linux too though...

based on the OSS source code...

---------------------------------------------------------------------------
   Copyright 2000 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
----------------------------------------------------------------------------

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"
//#include "xmame.h"
//#include "sound.h"

#define TRUE 1
#define FALSE 0

typedef struct alsa_audio_device 
{
	snd_pcm_t			*m_AudioHandle;
	snd_mixer_t			*m_MixerHandle;
	int				m_Acard;
	int				m_Adevice;
	int				FdAudioDevice;
	snd_pcm_channel_info_t		m_Achaninfo;
	snd_mixer_group_t		m_Amixgroup;
	snd_pcm_channel_params_t	m_Aparams;
	snd_pcm_channel_setup_t		m_Asetup;
	snd_pcm_channel_status_t	m_Astatus;
	int				bMute;
	pthread_t			*RenderThread;
	int				m_Aformat;
	int				m_BytesPerSample;  //one channel
} AlsaAudioDevice_t;

/* our per instance private data struct */
struct alsa_dsp_priv_data 
{
   AlsaAudioDevice_t audio_dev;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *alsa_dsp_create(const void *flags);
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data, int count);
static int preferred_device=FALSE;

struct rc_option alsa_opts[] = {
	{"QNX Audio related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL},
	{"audio-preferred" , "audio-primary", rc_bool, &preferred_device, "1", 0, 0, NULL, "Use the preferred device or use the primary device."},
	{NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

/* public variables */
const struct plugin_struct sysdep_dsp_alsa= 
{
   "alsa",
   "sysdep_dsp",
   "ALSA 5 DSP plugin",
   alsa_opts, /* options */
   NULL, /* no init */
   NULL, /* no exit */
   alsa_dsp_create,
   3     /* high priority */
};

/* private variables */
static int alsa_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *alsa_dsp_create(const void *flags)
{
	int i, j;
//	audio_buf_info info;
	struct alsa_dsp_priv_data *priv = NULL;
	struct sysdep_dsp_struct *dsp = NULL;
	const struct sysdep_dsp_create_params *params = flags;
	const char *device = params->device;
	int err;
	int bytespersample;

	fprintf(stderr,"info: dsp_create called\n");
   
	/* allocate the dsp struct */
	if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
	{
		fprintf(stderr,
			"error: malloc failed for struct sysdep_dsp_struct\n");
		return NULL;
	}
   
	/* alloc private data */
	if(!(priv = calloc(1, sizeof(struct alsa_dsp_priv_data))))
	{
		fprintf(stderr,
			"error: malloc failed for struct dsp_priv_data\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	}
   
	/* fill in the functions and some data */
	dsp->_priv = priv;
	dsp->get_freespace = alsa_dsp_get_freespace;
	dsp->write = alsa_dsp_write;
	dsp->destroy = alsa_dsp_destroy;
	dsp->hw_info.type = params->type;
	dsp->hw_info.samplerate = params->samplerate;

	priv->audio_dev.bMute = 0;
	priv->audio_dev.m_AudioHandle = NULL;
	priv->audio_dev.m_MixerHandle = NULL;
	priv->audio_dev.m_Acard = 0;
	priv->audio_dev.m_Adevice = 0;


	if (preferred_device)
	{	
 		if((err = snd_pcm_open_preferred(&(priv->audio_dev.m_AudioHandle), &priv->audio_dev.m_Acard,
                                       &priv->audio_dev.m_Adevice, SND_PCM_OPEN_PLAYBACK)) < 0)
    	{
			fprintf(stderr,"info: snd_pcm_open_preferred failed: %s \n", snd_strerror(err));
    	            alsa_dsp_destroy(dsp);
			return NULL;
    	}
	}
	else
	{
		fprintf(stderr,"info: audio is using primary device\n");
		if((err = snd_pcm_open(&(priv->audio_dev.m_AudioHandle), 0, 0, SND_PCM_OPEN_PLAYBACK)) < 0)
		{
			fprintf(stderr,"info: snd_pcm_open failed: %s \n", snd_strerror(err));
			alsa_dsp_destroy(dsp);
			return NULL;
		}
	}
	
	memset (&(priv->audio_dev.m_Achaninfo), 0, sizeof (priv->audio_dev.m_Achaninfo));
	priv->audio_dev.m_Achaninfo.channel = SND_PCM_CHANNEL_PLAYBACK;
	if ((err = snd_pcm_plugin_info (priv->audio_dev.m_AudioHandle, &(priv->audio_dev.m_Achaninfo))) < 0)
	{
		fprintf (stderr, "info: snd_pcm_plugin_info failed: %s\n", snd_strerror (err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	//needed to enable the count status parameter, mmap plugin disables this
	if((err = snd_plugin_set_disable(priv->audio_dev.m_AudioHandle, PLUGIN_DISABLE_MMAP)) < 0)
	{
		fprintf (stderr, "info: snd_plugin_set_disable failed: %s\n", snd_strerror (err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	/* calculate and set the fragsize & number of frags */
	/* fragsize (as power of 2) */
	i = 8;
	if (dsp->hw_info.type & SYSDEP_DSP_16BIT) i++;
	if (dsp->hw_info.type & SYSDEP_DSP_STEREO) i++;
	i += dsp->hw_info.samplerate / 22000;

	/* number of frags */
	j = ((dsp->hw_info.samplerate * alsa_dsp_bytes_per_sample[dsp->hw_info.type] *
		params->bufsize) / (0x01 << i)) + 1;

	bytespersample=1;
//	dsp->hw_info.type &= ~SYSDEP_DSP_16BIT;
//	dsp->hw_info.type &= ~SYSDEP_DSP_STEREO;
	if (dsp->hw_info.type & SYSDEP_DSP_16BIT) bytespersample++;
	if (dsp->hw_info.type & SYSDEP_DSP_STEREO) bytespersample <<= 1;
	
	memset( &(priv->audio_dev.m_Aparams), 0, sizeof(priv->audio_dev.m_Aparams));
	priv->audio_dev.m_Aparams.mode = SND_PCM_MODE_BLOCK;
	priv->audio_dev.m_Aparams.channel = SND_PCM_CHANNEL_PLAYBACK;
	priv->audio_dev.m_Aparams.start_mode = SND_PCM_START_FULL;
	priv->audio_dev.m_Aparams.stop_mode = SND_PCM_STOP_ROLLOVER;
#if 0
	priv->audio_dev.m_Aparams.buf.stream.queue_size = 512 * bytespersample;
	priv->audio_dev.m_Aparams.buf.stream.fill = SND_PCM_FILL_SILENCE;
	priv->audio_dev.m_Aparams.buf.stream.max_fill = 512 * bytespersample;
#endif
        priv->audio_dev.m_Aparams.format.interleave = 1;
        priv->audio_dev.m_Aparams.format.rate = dsp->hw_info.samplerate;
        priv->audio_dev.m_Aparams.format.voices = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 2 : 1;

	priv->audio_dev.m_Aparams.buf.block.frag_size = 1000;
	priv->audio_dev.m_Aparams.buf.block.frags_min = 1;
	priv->audio_dev.m_Aparams.buf.block.frags_max = 5;

        priv->audio_dev.m_BytesPerSample = bytespersample;
        priv->audio_dev.m_Aparams.format.format = 
#ifdef LSB_FIRST
				(dsp->hw_info.type & SYSDEP_DSP_16BIT) ? SND_PCM_SFMT_S16_LE : SND_PCM_SFMT_U8; 
#else
				(dsp->hw_info.type & SYSDEP_DSP_16BIT) ? SND_PCM_SFMT_S16_BE : SND_PCM_SFMT_U8;
#endif

	if ((err = snd_pcm_plugin_params (priv->audio_dev.m_AudioHandle, &(priv->audio_dev.m_Aparams))) < 0)
	{
		fprintf (stderr, "info: snd_pcm_plugin_params failed: %s\n", snd_strerror (err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	if ((err = snd_pcm_plugin_prepare (priv->audio_dev.m_AudioHandle, SND_PCM_CHANNEL_PLAYBACK)) < 0)
	{
		fprintf (stderr, "warning: snd_pcm_plugin_prepare failed: %s\n", snd_strerror (err));
	}

	memset (&(priv->audio_dev.m_Asetup), 0, sizeof (priv->audio_dev.m_Asetup));
	priv->audio_dev.m_Asetup.channel = SND_PCM_CHANNEL_PLAYBACK;
	if ((err = snd_pcm_plugin_setup (priv->audio_dev.m_AudioHandle, &(priv->audio_dev.m_Asetup))) < 0)
	{
		fprintf (stderr, "warning: snd_pcm_plugin_setup failed: %s\n", snd_strerror (err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}
	
	memset (&(priv->audio_dev.m_Astatus), 0, sizeof (priv->audio_dev.m_Astatus));
	priv->audio_dev.m_Astatus.channel = SND_PCM_CHANNEL_PLAYBACK;
	if ((err = snd_pcm_plugin_status (priv->audio_dev.m_AudioHandle, &(priv->audio_dev.m_Astatus))) < 0)
	{
		fprintf (stderr, "warning: snd_pcm_plugin_status failed: %s\n", snd_strerror (err));
        }
	dsp->hw_info.bufsize = priv->audio_dev.m_Asetup.buf.stream.queue_size  
	             		/ alsa_dsp_bytes_per_sample[dsp->hw_info.type];
#if 0
	if ((err=snd_pcm_nonblock_mode(priv->audio_dev.m_AudioHandle, 1))<0)
	{
		fprintf(stderr, "error: error with non block mode: %s\n", snd_strerror (err));
	}
#endif
	return dsp;
}

/* BUG: Core dumps if -nosound isn't present when using a soundless setup... */
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
	struct alsa_dsp_priv_data *priv = dsp->_priv;
	int err;
	
	fprintf (stderr,"info: dsp_destroy called\n");
	
	if(priv)
	{
		if ((err = snd_pcm_plugin_flush(priv->audio_dev.m_AudioHandle,SND_PCM_CHANNEL_PLAYBACK)) < 0)
			fprintf (stderr, "warning: snd_pcm_plugin_playback_drain failed: %s\n", snd_strerror (err));

	        err =  snd_pcm_close(priv->audio_dev.m_AudioHandle);
	        if (err != 0)
			printf("info: snd_pcm_close failed: %s\n",snd_strerror(err));

		priv->audio_dev.m_AudioHandle = NULL;
		free(priv);
	}
	free(dsp);
	
}
   
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
	struct alsa_dsp_priv_data *priv = dsp->_priv;
	snd_pcm_channel_status_t status={0};	
 
 	snd_pcm_plugin_status(priv->audio_dev.m_AudioHandle, &status);
 
	return status.free / alsa_dsp_bytes_per_sample[dsp->hw_info.type];
}

static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
	int BytesWritten, BytesToWrite;
        unsigned int bytesleft;
        int err;
	struct alsa_dsp_priv_data *priv = dsp->_priv;
        snd_pcm_channel_status_t  status = {0};

	BytesToWrite = count*alsa_dsp_bytes_per_sample[dsp->hw_info.type];
	BytesWritten = snd_pcm_plugin_write( priv->audio_dev.m_AudioHandle, data, BytesToWrite );
#if 0	
	if (BytesWritten == 0)
        {
		return count;
        }
#endif	
	return BytesWritten / alsa_dsp_bytes_per_sample[dsp->hw_info.type];
}
