/* Sysdep NetBSD sound dsp driver

   Copyright 2000 Hans de Goede, Krister Walfridsson
   
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
*/
/* Changelog
Version 0.1, February 2000
-initial release, based on the xmame OSS dsp driver version 0.1, and using
 some inspiration from the old xmame NetBSD driver. (Krister Walfridsson)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* Our per instance private data struct. */
struct netbsd_dsp_priv_data {
  int fd;
};

/* Public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct.) */
static void *netbsd_dsp_create(const void *flags);
static void netbsd_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int netbsd_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int netbsd_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
			    int count);

/* Public variables. */
const struct plugin_struct sysdep_dsp_netbsd = {
  "netbsd",
  "sysdep_dsp",
  "NetBSD DSP plugin",
  NULL, /* no options */
  NULL, /* no init */
  NULL, /* no exit */
  netbsd_dsp_create,
  3     /* high priority */
};

/* Private variables.  */
static int netbsd_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* Public methods (static but exported through the sysdep_dsp or plugin
   struct.) */
static void *netbsd_dsp_create(const void *flags)
{
  int block_size, blocks;
  audio_info_t a_info;
  int desired_encoding, desired_precision;
  struct netbsd_dsp_priv_data *priv = NULL;
  struct sysdep_dsp_struct *dsp = NULL;
  const struct sysdep_dsp_create_params *params = flags;
  const char *device = params->device;

  /* Allocate the dsp struct. */
  if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
    {
      fprintf(stderr,
	      "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
    }

  /* Alloc private data. */
  if (!(priv = calloc(1, sizeof(struct netbsd_dsp_priv_data))))
    {
      fprintf(stderr,
	      "error malloc failed for struct netbsd_dsp_priv_data\n");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }

  /* Fill in the functions and some data. */
  priv->fd = -1;
  dsp->_priv = priv;
  dsp->get_freespace = netbsd_dsp_get_freespace;
  dsp->write = netbsd_dsp_write;
  dsp->destroy = netbsd_dsp_destroy;
  dsp->hw_info.type = params->type;
  dsp->hw_info.samplerate = params->samplerate;

  /* Open the sound device. */
  if (!device)
    device = "/dev/audio";

  if((priv->fd = open(device, O_WRONLY, 0)) < 0)
    {
      perror("error: /dev/audio");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }

  /* Empty buffers before change config. */
  ioctl(priv->fd, AUDIO_FLUSH, 0);

  /* Set the number of bits. */
  AUDIO_INITINFO(&a_info);
  if (dsp->hw_info.type & SYSDEP_DSP_16BIT)
    {
      desired_encoding = AUDIO_ENCODING_SLINEAR;
      desired_precision = 16;
    }
  else
    {
      desired_encoding = AUDIO_ENCODING_ULINEAR;
      desired_precision = 8;
    }
  a_info.play.encoding = desired_encoding;
  a_info.play.precision = desired_precision;
  if (ioctl(priv->fd, AUDIO_SETINFO, &a_info) < 0)
    {
      perror("error: AUDIO_SETINFO");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }
  if (a_info.play.encoding != desired_encoding
      || a_info.play.precision != desired_precision)
    {
      if (desired_precision == 8)
	{
	  fprintf(stderr, "error: couldn't set sound to 8 bits,\n");
	  netbsd_dsp_destroy(dsp);
	  return NULL;
	}

      fprintf(stderr,
	      "warning: couldn't set sound to 16 bits,\n"
	      "   trying again with 8 bits: ");

      AUDIO_INITINFO(&a_info);
      a_info.play.encoding = AUDIO_ENCODING_ULINEAR;
      a_info.play.precision = 8;
      if (ioctl(priv->fd, AUDIO_SETINFO, &a_info) < 0)
	{
	  perror("error: AUDIO_SETINFO");
	  netbsd_dsp_destroy(dsp);
	  return NULL;
	}

      if (a_info.play.encoding != AUDIO_ENCODING_ULINEAR
	  || a_info.play.precision != 8)
	{
	  fprintf(stderr, "failed\n");
	  netbsd_dsp_destroy(dsp);
	  return NULL;
	}
      fprintf(stderr, "success\n");

      dsp->hw_info.type &= ~SYSDEP_DSP_16BIT;
    }

  /* Set the number of channels.  */
  AUDIO_INITINFO(&a_info);
  a_info.play.channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2 : 1;
  if (ioctl(priv->fd, AUDIO_SETINFO, &a_info) < 0)
    {
      perror("error: AUDIO_SETINFO");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }
  if (a_info.play.channels == 1)
    dsp->hw_info.type &= ~SYSDEP_DSP_STEREO;
  else
    dsp->hw_info.type |= SYSDEP_DSP_STEREO;

  /* Set the sample rate.  */
  AUDIO_INITINFO(&a_info);
  a_info.play.sample_rate = dsp->hw_info.samplerate;
  a_info.mode = AUMODE_PLAY | AUMODE_PLAY_ALL;
  if (ioctl(priv->fd, AUDIO_SETINFO, &a_info) < 0)
    {
      perror("error: AUDIO_SETINFO");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }
  dsp->hw_info.samplerate = a_info.play.sample_rate;
   
  /* Calculate and set the block size and number of blocks.
     This is basically the same thing as frags in OSS, so the
     calculations are taken from the OSS driver. The a_info.play.seek
     used in the freespace calculation doesn't seem to be updated
     as often as we should like, so the buffer need to consist of
     at least 4 blocks. */
  block_size = 1<<9;
  if (dsp->hw_info.type & SYSDEP_DSP_16BIT) block_size<<=1;
  if (dsp->hw_info.type & SYSDEP_DSP_STEREO) block_size<<=1;

  blocks = ((dsp->hw_info.samplerate
	     * netbsd_dsp_bytes_per_sample[dsp->hw_info.type] *
	     params->bufsize) / block_size) + 1;
  blocks = (blocks < 4) ? 4 : blocks;

  AUDIO_INITINFO(&a_info);
  a_info.blocksize = block_size;
  a_info.play.buffer_size = block_size * blocks;
  if (ioctl(priv->fd, AUDIO_SETINFO, &a_info) < 0)
    {
      perror("error: AUDIO_SETINFO");
      netbsd_dsp_destroy(dsp);
      return NULL;
    }
  fprintf(stderr, "info: setting blocksize to %d, buffer_size to %d\n",
	  a_info.blocksize, a_info.play.buffer_size);

  dsp->hw_info.bufsize =
    block_size * blocks / netbsd_dsp_bytes_per_sample[dsp->hw_info.type];

  fprintf(stderr,
	  "info: audiodevice %s set to %dbit linear %s %dHz\n",
	  device,
	  (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8,
	  (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? "stereo" : "mono",
	  dsp->hw_info.samplerate);

  return dsp;
}

static void netbsd_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
  struct netbsd_dsp_priv_data *priv = dsp->_priv;

  if (priv)
    {
      if (priv->fd >= 0)
	close(priv->fd);

      free(priv);
    }
  free(dsp);
}

static int netbsd_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
  audio_info_t a_info;
  struct netbsd_dsp_priv_data *priv = dsp->_priv;

  AUDIO_INITINFO(&a_info);
  if (ioctl(priv->fd, AUDIO_GETINFO, &a_info) < 0)
    {
      perror("error: AUDIO_GETINFO");
      return -1;
    }

  return  dsp->hw_info.bufsize
    - a_info.play.seek/netbsd_dsp_bytes_per_sample[dsp->hw_info.type];
}

static int netbsd_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
			    int count)
{
  int result;
  int bytes_per_sample;
  struct netbsd_dsp_priv_data *priv = dsp->_priv;

  bytes_per_sample = netbsd_dsp_bytes_per_sample[dsp->hw_info.type];

  result = write(priv->fd, data, count * bytes_per_sample);

  if (result < 0)
    return -1;
      
  return result / bytes_per_sample;
}
