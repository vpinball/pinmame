/* Sysdep Solaris sound dsp driver

   Copyright 2000 Hans de Goede, Mathis Rosenhauer
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
/* Changelog
Version 0.1, February 2000
-initial release, based on oss.c and on the old xmame sound driver done by 
 Juan Antonio Martinez, Keith Hargrove, Mathis Rosenhauer and others.
 (Mathis Rosenhauer)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/audioio.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct sol_dsp_priv_data {
	int fd;
	unsigned int samples_written;
	unsigned int buffer_samples;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *sol_dsp_create(const void *flags);
static void sol_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int sol_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int sol_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
						 int count);

/* public variables */
const struct plugin_struct sysdep_dsp_solaris = {
	"solaris",
	"sysdep_dsp",
	"Solaris DSP plugin",
	NULL, /* no options */
	NULL, /* no init */
	NULL, /* no exit */
	sol_dsp_create,
	3	  /* high priority */
};

/* private variables */
static int sol_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *sol_dsp_create(const void *flags)
{
	int i,j;
	audio_info_t info;   /* info about audio settings */
	audio_device_t dev;  /* info about audio hardware */
	struct sol_dsp_priv_data *priv = NULL;
	struct sysdep_dsp_struct *dsp = NULL;
	const struct sysdep_dsp_create_params *params = flags;
	const char *device = params->device;
	
	/* allocate the dsp struct */
	if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
	{
		perror("error malloc failed for struct sysdep_dsp_struct\n");
		return NULL;
	}
   
	/* alloc private data */
	if(!(priv = calloc(1, sizeof(struct sol_dsp_priv_data))))
	{
		perror("error malloc failed for struct sol_dsp_priv_data\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}
   
	/* fill in the functions and some data */
	priv->fd = -1;
	priv->samples_written = 0;
	dsp->_priv = priv;
	dsp->get_freespace = sol_dsp_get_freespace;
	dsp->write = sol_dsp_write;
	dsp->destroy = sol_dsp_destroy;
	dsp->hw_info.type = params->type;
	dsp->hw_info.samplerate = params->samplerate;
   
	/* open the sound device */
	if (!device)
		device = getenv("AUDIODEV");
	if (!device)
		device = "/dev/audio";
   
	if((priv->fd = open(device, O_WRONLY)) < 0)
	{
		fprintf(stderr, "error: opening %s\n", device);
		sol_dsp_destroy(dsp);
		return NULL;
	}
   
	/* empty buffers before change config */
	ioctl(priv->fd, AUDIO_DRAIN, 0);	/* drain everything out */
	ioctl(priv->fd, I_FLUSH, FLUSHRW);	/* flush everything		*/
		
	/* identify audio device. */
	if(ioctl(priv->fd, AUDIO_GETDEV, &dev) < 0)
	{
		perror("error: cannot get sound device type\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}

	fprintf(stderr, "info: sound device is a %s %s version %s\n",dev.config,dev.name,dev.version);
		
	/* get audio parameters. */
	if (ioctl(priv->fd, AUDIO_GETINFO, &info) < 0)
	{
		perror("AUDIO_GETINFO failed!\nRun with -nosound\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}

	/* set the number of bits */
	AUDIO_INITINFO(&info);

	if (dsp->hw_info.type & SYSDEP_DSP_16BIT)
	{
		info.play.encoding = i = AUDIO_ENCODING_LINEAR;
		info.play.precision = j = 16;
	}
	else
	{
		info.play.encoding = i = AUDIO_ENCODING_LINEAR8;
		info.play.precision = j = 8;
	}

	if (ioctl(priv->fd, AUDIO_SETINFO, &info) < 0)
	{
		perror("error: AUDIO_SETINFO\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}
	if ((info.play.encoding != i) || (info.play.precision != j))
	{
		if(dsp->hw_info.type & SYSDEP_DSP_16BIT)
		{
			fprintf(stderr, "warning: couldn't set sound to 16 bits,\ntrying again with 8 bits: ");
		}
		else
		{
			fprintf(stderr, "error: couldn't set sound to 8 bits,\n");
			sol_dsp_destroy(dsp);
			return NULL;
		}
		dsp->hw_info.type &= ~SYSDEP_DSP_16BIT;
		info.play.precision = 8;
		info.play.encoding = AUDIO_ENCODING_LINEAR8;
		if (ioctl(priv->fd, AUDIO_SETINFO, &info) < 0)
		{
			perror("error: AUDIO_SETINFO\n");
			sol_dsp_destroy(dsp);
			return NULL;
		}
		if (info.play.precision != 8 || info.play.encoding != AUDIO_ENCODING_LINEAR8)
		{
			fprintf(stderr, "failed\n");
			sol_dsp_destroy(dsp);
			return NULL;
		}
		fprintf(stderr, "success\n");
	}
   
   /* set the number of channels */
    info.play.channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2:1;
	if (ioctl(priv->fd, AUDIO_SETINFO, &info) < 0)
	{
		perror("error: AUDIO_SETINFO\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}
	if(info.play.channels == 2)
		dsp->hw_info.type |= SYSDEP_DSP_STEREO;
	else
		dsp->hw_info.type &= ~SYSDEP_DSP_STEREO;
   
	/* set the samplerate and buffer size*/
	info.play.sample_rate = dsp->hw_info.samplerate;
	priv->buffer_samples = dsp->hw_info.samplerate * params->bufsize;
	info.play.buffer_size = dsp->hw_info.bufsize = priv->buffer_samples * sol_dsp_bytes_per_sample[dsp->hw_info.type]; /* this seems to have no effect */
	if (ioctl(priv->fd, AUDIO_SETINFO, &info) < 0)
	{
		perror("error: AUDIO_SETINFO\n");
		sol_dsp_destroy(dsp);
		return NULL;
	}

	dsp->hw_info.samplerate = info.play.sample_rate;
	fprintf(stderr, "info: audiodevice %s set to %dbit linear %s %dHz\n",
			dev.name, info.play.precision,
			(info.play.channels & 2)? "stereo":"mono",
			info.play.sample_rate);
	
	return dsp;
}

static void sol_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
	struct sol_dsp_priv_data *priv = dsp->_priv;
   
	if(priv)
	{
		if(priv->fd >= 0)
			close(priv->fd);
		free(priv);
	}
	free(dsp);
}
   
static int sol_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
	audio_info_t info;
	struct sol_dsp_priv_data *priv = dsp->_priv;
   
	if (ioctl(priv->fd, AUDIO_GETINFO, &info) < 0)
	{
		perror("error: AUDIO_GETINFO\n");
		return -1;
	}
	return priv->buffer_samples - (priv->samples_written - info.play.samples);
}

static int sol_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data, int count)
{
	int result;
	struct sol_dsp_priv_data *priv = dsp->_priv;
	result = write(priv->fd, data, count * sol_dsp_bytes_per_sample[dsp->hw_info.type]);

	if (result < 0)
		return -1;
	
	result /= sol_dsp_bytes_per_sample[dsp->hw_info.type];
	priv->samples_written += result;
	return result;
}
