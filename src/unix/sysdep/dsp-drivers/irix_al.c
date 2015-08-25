/* Sysdep Irix_al sound dsp driver

   Copyright 2001 by Brandon Corey
   
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

   Addendum: This file may also be used under the terms of the MAME license, by
   permission of Brandon Corey.  (For the text of the license, see 
   http://www.mame.net/readme.html.)
*/
/* Changelog
Version 0.1, April 15, 2001
- initial release
Version 0.2, April 15, 2001
- added sample frequency code so that a sample frequency other than the
	system default is usable
*/

/* Notes/Future
1) I've only compiled this with MIPSPro, although it should work with gcc.
	If someone could test that and email me, I'd appreciate it.
2) Will add mixer support for the next version
3) Use IRIX_DEBUG and IRIX_DEBUG_VERBOSE for Extra Info
4) Use FORCEMONO to force MONO output
5) Use FORCE8BIT to force 8 Bit output

Email: brandon@blackboxcentral.com
*/

/* #define IRIX_DEBUG */
/* #define IRIX_DEBUG_VERBOSE */
/* #define FORCEMONO */
/* #define FORCE8BIT */
/* #define SYSDEP_DSP_IRIX */

#ifdef SYSDEP_DSP_IRIX
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dmedia/audio.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct irix_dsp_priv_data {
	ALport devAudio;
	ALconfig devAudioConfig;	
	unsigned int buffer_samples;
	int sampwidth;
	int sampchan;
	int port_status;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *irix_dsp_create(const void *flags);
static void irix_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int irix_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int irix_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);

/* public variables */
const struct plugin_struct sysdep_dsp_irix = {
   "irix_al",
   "sysdep_dsp",
   "IrixAL DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   irix_dsp_create,
   3     /* high priority as direct device access */
};


/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *irix_dsp_create(const void *flags)
{
	ALpv pvs[4];
	long tempbits, tempchan;
	long oldrate;
	long ratechange = 0;
	struct irix_dsp_priv_data *priv = NULL;
	struct sysdep_dsp_struct *dsp = NULL;
	const struct sysdep_dsp_create_params *params = flags;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr, "Error: malloc failed for struct sysdep_dsp_struct.\n"); 
      return NULL;
   }
   
   /* alloc private data */
   if(!(priv = calloc(1, sizeof(struct irix_dsp_priv_data))))
   {
      fprintf(stderr, "Error: malloc failed for struct irix_dsp_priv_data.\n");
      irix_dsp_destroy(dsp);
      return NULL;
   }
   
	/* fill in the functions and some data */
	priv->port_status = -1;
   	dsp->_priv = priv;
	dsp->get_freespace = irix_dsp_get_freespace;
   	dsp->write = irix_dsp_write;
   	dsp->destroy = irix_dsp_destroy;
   	dsp->hw_info.type = params->type;
   	dsp->hw_info.samplerate = params->samplerate;

   	/* open the sound device */
	pvs[0].param = AL_MAX_PORTS;
	pvs[1].param = AL_UNUSED_PORTS;
	if (alGetParams(AL_SYSTEM,pvs,2) < 0) {
		fprintf(stderr, "alGetParams failed: %s\n", alGetErrorString(oserror()));
		irix_dsp_destroy(dsp);
		return NULL;
	}  

	if (pvs[1].value.i < 1) {
		fprintf(stderr, "No available audio ports.\n");
		irix_dsp_destroy(dsp);
		return NULL;
	}

	pvs[0].param = AL_RATE;
	if (alGetParams(AL_DEFAULT_OUTPUT,pvs,1) < 0) {
		fprintf(stderr, "alGetParams failed: %s\n", alGetErrorString(oserror()));
		irix_dsp_destroy(dsp);
		return NULL;
	}

    /* If samplerate is different than systems, override */
	oldrate = pvs[0].value.i;
	if (pvs[0].value.i != dsp->hw_info.samplerate)
	{
		ratechange = 1;
		fprintf(stderr, "System sample rate was %dHz, forcing %dHz instead.\n",
			pvs[0].value.i, dsp->hw_info.samplerate);
	}

	/* create a clean config descriptor */
	if ( (priv->devAudioConfig = alNewConfig() ) == (ALconfig) NULL ) {
		fprintf(stderr, "Cannot get a Descriptor. Exiting..\n");
		irix_dsp_destroy(dsp);
		return NULL;
	}

#ifdef FORCE8BIT
	dsp->hw_info.type &= ~SYSDEP_DSP_16BIT;
#endif
#ifdef FORCEMONO
	dsp->hw_info.type &= ~SYSDEP_DSP_STEREO;
#endif

	priv->buffer_samples = dsp->hw_info.samplerate * params->bufsize;

	tempchan = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2:1;
	tempbits = (dsp->hw_info.type & SYSDEP_DSP_16BIT)? 2:1;
	
	priv->buffer_samples = priv->buffer_samples * tempchan;

	priv->sampwidth = tempbits;
	priv->sampchan = tempchan;

#ifdef IRIX_DEBUG
	fprintf(stderr, "Sample Rate Requested: %d\n", dsp->hw_info.samplerate);
	fprintf(stderr, "Buffer Size Requested: %d\n", priv->buffer_samples);
#endif

	fprintf(stderr, "Setting sound to %dHz, %d bit, %s\n",dsp->hw_info.samplerate,
		tempbits * 8, (tempchan == 2) ? "stereo" : "mono");

	/* channel specific audio parameters */
	alSetChannels(priv->devAudioConfig, tempchan);						/* channels */
	alSetQueueSize(priv->devAudioConfig,(long) priv->buffer_samples);	/* buffer size */
	alSetSampFmt(priv->devAudioConfig, AL_SAMPFMT_TWOSCOMP);			/* BTC */
	alSetWidth(priv->devAudioConfig, tempbits);							/* bitrate */

	/* global audio parameters */
	pvs[0].param = AL_MASTER_CLOCK;
	pvs[0].value.i = AL_CRYSTAL_MCLK_TYPE;
	pvs[1].param = AL_RATE;
	pvs[1].value.i = dsp->hw_info.samplerate;
	if (alSetParams(AL_DEFAULT_OUTPUT,pvs,2) < 0) {
		fprintf(stderr, "Error: Cannot configure the sound system.\n");
		irix_dsp_destroy(dsp);
		return(NULL);
	}

	/* Get new sample rate */
	pvs[0].param = AL_RATE;
	if (alGetParams(AL_DEFAULT_OUTPUT,pvs,1) < 0) {
		fprintf(stderr, "failed\n");
		irix_dsp_destroy(dsp);
		return NULL;
	}

    /* Verify rate was changed */
	if ((ratechange == 1) && (oldrate == pvs[0].value.i)) 
	  fprintf(stderr, "Error changing sample rate, using default of: %dHz.\n",
		pvs[0].value.i);

	/* Open the audio port with the parameters we setup */
	if ( (priv->devAudio=alOpenPort("audio_fd","w",priv->devAudioConfig) ) == (ALport) NULL ) {
		fprintf(stderr, "Error: Cannot get an audio channel descriptor.\n");
		irix_dsp_destroy(dsp);
		return NULL;
	}

	/* since we don't have FD's with DMEDIA, we use this to inform us of success */
	priv->port_status = 0;

   return dsp;
}

static void irix_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct irix_dsp_priv_data *priv = dsp->_priv;
 
#ifdef IRIX_DEBUG 
	fprintf(stderr, "Destroying sound channel.\n");
#endif
 
   if(priv)
   {
	  if(priv->port_status >= 0) alClosePort(priv->devAudio);
	  alFreeConfig(priv->devAudioConfig);
      
      free(priv);
   }
   free(dsp);
}


static int irix_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
	struct irix_dsp_priv_data *priv = dsp->_priv;

	return alGetFillable(priv->devAudio);
}

   
static int irix_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
	unsigned char *outbuf=data;
   int result;
   struct irix_dsp_priv_data *priv = dsp->_priv;
	int playcnt;
	int maxsize;

	maxsize = alGetFillable(priv->devAudio);

/*	count = count * priv->sampchan; */

	if (count <= maxsize) playcnt = count;
		else playcnt = maxsize;

	outbuf += (priv->sampwidth - 1);	

	for (;outbuf<(data+count);outbuf+=priv->sampwidth) *outbuf ^= 0x80;

	result = alWriteFrames(priv->devAudio, (void *)data, playcnt);

	if (result < 0) {
		fprintf(stderr, "Error %d: failure writing to irix stream\n",oserror());
		return -1;
	}


#ifdef IRIX_DEBUG_VERBOSE
	fprintf(stderr, "Wrote %d samples OK.\n",playcnt);
#endif

/*	return playcnt / priv->sampchan; */
	return playcnt;
}

#endif /* ifdef SYSDEP_DSP_IRIX */

