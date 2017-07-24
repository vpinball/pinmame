/* Sysdep Open Sound System sound dsp driver

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
*/
/* Changelog
Version 0.1, January 2000
-initial release, based on the xmame driver done by Mike Oliphant
 (oliphant@ling.ed.ac.uk), amongst others (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#if defined (__ARCH_openbsd)
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

#ifdef __ARCH_openbsd
#define AUDIO_DEVICE   "/dev/audio"
#else
#define AUDIO_DEVICE   "/dev/dsp"
#endif

/* our per instance private data struct */
struct oss_dsp_priv_data {
   int fd;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *oss_dsp_create(const void *flags);
static void oss_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int oss_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int oss_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);

/* public variables */
const struct plugin_struct sysdep_dsp_oss = {
   "oss",
   "sysdep_dsp",
   "Open Sound System DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   oss_dsp_create,
   3     /* high priority */
};

/* private variables */
static int oss_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *oss_dsp_create(const void *flags)
{
   int i, j;
   audio_buf_info info;
   struct oss_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   const char *device = params->device;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }
   
   /* alloc private data */
   if (!(priv = calloc(1, sizeof(struct oss_dsp_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct oss_dsp_priv_data\n");
      oss_dsp_destroy(dsp);
      return NULL;
   }
   
   /* fill in the functions and some data */
   priv->fd = -1;
   dsp->_priv = priv;
   dsp->get_freespace = oss_dsp_get_freespace;
   dsp->write = oss_dsp_write;
   dsp->destroy = oss_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;
   
   /* open the sound device */
   if (!device)
      device = AUDIO_DEVICE;
   
   /* always open in non-blocking mode, otherwise the open itself may
      block, hanging the entire application */
   if ((priv->fd = open(device, O_WRONLY|O_NONBLOCK, 0)) < 0) {
      perror("error: " AUDIO_DEVICE);
      oss_dsp_destroy(dsp);
      return NULL;
   }
   
   /* set back to blocking mode, unless non-blocking mode was selected */
   if (!(params->flags & SYSDEP_DSP_O_NONBLOCK))
   {
      if (fcntl(priv->fd, F_SETFL, O_WRONLY) < 0)
      {
         perror("OSS-driver, error: fnctl");
         oss_dsp_destroy(dsp);
         return NULL;
      }
   }
   
   /* set the number of bits */
#ifdef LSB_FIRST
   i = j = (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? AFMT_S16_LE : AFMT_U8;
#else
   i = j = (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? AFMT_S16_BE : AFMT_U8;
#endif

   if (ioctl(priv->fd, SNDCTL_DSP_SETFMT, &i) < 0)
   {
      perror("error: SNDCTL_DSP_SETFMT");
      oss_dsp_destroy(dsp);
      return NULL;
   }

   if (i != j)
   {
      if (dsp->hw_info.type & SYSDEP_DSP_16BIT)
      {
         fprintf(stderr, "warning: couldn't set sound to 16 bits,\n"
            "   trying again with 8 bits: ");
      }
      else
      {
         fprintf(stderr, "error: couldn't set sound to 8 bits,\n");
         oss_dsp_destroy(dsp);
         return NULL;
      }

      dsp->hw_info.type &= ~SYSDEP_DSP_16BIT;
      i = AFMT_U8;
      if (ioctl(priv->fd, SNDCTL_DSP_SETFMT, &i) < 0)
      {
         perror("error: SNDCTL_DSP_SETFMT");
         oss_dsp_destroy(dsp);
         return NULL;
      }

      if (i != AFMT_U8)
      {
         fprintf(stderr, "failed\n");
         oss_dsp_destroy(dsp);
         return NULL;
      }
      fprintf(stderr, "success\n");
   }
   
   /* set the number of channels */
   i = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 1 : 0;
   if (ioctl(priv->fd, SNDCTL_DSP_STEREO, &i) < 0)
   {
      perror("error: SNDCTL_DSP_STEREO");
      oss_dsp_destroy(dsp);
      return NULL;
   }

   if (i)
      dsp->hw_info.type |= SYSDEP_DSP_STEREO;
   else
      dsp->hw_info.type &= ~SYSDEP_DSP_STEREO;
   
   /* set the samplerate */
   if (ioctl(priv->fd, SNDCTL_DSP_SPEED, &dsp->hw_info.samplerate) < 0)
   {
      perror("error: SNDCTL_DSP_SPEED");
      oss_dsp_destroy(dsp);
      return NULL;
   }
   
   /* calculate and set the fragsize & number of frags */
   /* fragsize (as power of 2) */
   i = 7;
   if (dsp->hw_info.type & SYSDEP_DSP_16BIT) i++;
   if (dsp->hw_info.type & SYSDEP_DSP_STEREO) i++;
   i += dsp->hw_info.samplerate / 22000;
   
   /* number of frags */
   j = ((dsp->hw_info.samplerate * oss_dsp_bytes_per_sample[dsp->hw_info.type] *
      params->bufsize) / (0x01 << i)) + 1;
      
   /* set the fraginfo */
   i = j = i | (j << 16);
   fprintf(stderr, "info: setting fragsize to %d, numfrags to %d\n",
   	1 << (i & 0x0000FFFF), i >> 16);
   if (ioctl(priv->fd, SNDCTL_DSP_SETFRAGMENT, &i) < 0)
   {
      perror("error: SNDCTL_DSP_SETFRAGMENT");
      oss_dsp_destroy(dsp);
      return NULL;
   }

   if (ioctl(priv->fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
   {
      perror("warning: SNDCTL_DSP_GETOSPACE");
      fprintf(stderr, "   falling back to timer based-audio\n");
      dsp->get_freespace = NULL;
   }
   else
   {
      fprintf(stderr, "info: fragsize = %d, numfrags = %d\n", info.fragsize,
         info.fragstotal);
      /* i = requested fragsize, j = requested numfrags */
      i = 1 << (j & 0x0000FFFF);
      j = j >> 16;
      if ((info.fragsize < (i / 2)) || (info.fragsize > (i * 2)) ||
         (info.fragstotal < (j - 2)) || (info.fragstotal > (j + 2)))
      {
         fprintf(stderr, "warning: obtained fragsize/numfrags differs too much from requested\n"
            "   you may wish to adjust the bufsize setting in your xmamerc file, or try\n"
	    "   timer-based audio by adding -timer to your command line\n");
      }
      else if (info.bytes > (info.fragsize * info.fragstotal))
      {
         fprintf(stderr, "warning: freespace > (fragsize * numfrags) assuming buggy FreeBSD PCM driver,\n"
            "   falling back to timer based-audio\n");
         dsp->get_freespace = NULL;
      }
      else
         /* info.fragstotal + 1 to work around yet more OSS bugs ;( */
         dsp->hw_info.bufsize = (info.fragsize * (info.fragstotal + 1)) /
            oss_dsp_bytes_per_sample[dsp->hw_info.type];
   }

   fprintf(stderr, "info: audiodevice %s set to %dbit linear %s %dHz\n",
      device, (dsp->hw_info.type & SYSDEP_DSP_16BIT)? 16:8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO)? "stereo":"mono",
      dsp->hw_info.samplerate);
      
   return dsp;
}

static void oss_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct oss_dsp_priv_data *priv = dsp->_priv;
   
   if (priv)
   {
      if (priv->fd >= 0)
         close(priv->fd);
      
      free(priv);
   }
   free(dsp);
}
   
static int oss_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
   audio_buf_info info;
   struct oss_dsp_priv_data *priv = dsp->_priv;
   
   if (ioctl(priv->fd, SNDCTL_DSP_GETOSPACE, &info) < 0)
   {
      perror("error: SNDCTL_DSP_GETOSPACE");
      return -1;
   }
   return info.bytes / oss_dsp_bytes_per_sample[dsp->hw_info.type];
}

static int oss_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
   int result;
   struct oss_dsp_priv_data *priv = dsp->_priv;

   result = write(priv->fd, data, count *
      oss_dsp_bytes_per_sample[dsp->hw_info.type]);
      
   if (result < 0)
      return -1;
      
   return result / oss_dsp_bytes_per_sample[dsp->hw_info.type];
}
