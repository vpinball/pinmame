/* Sysdep esound sound dsp driver

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
-initial release, based on the driver submitted by riq <riq@ciudad.com.ar>,
 amongst others (Hans de Goede)
*/
#ifdef SYSDEP_DSP_ESOUND
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <esd.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* #define ESOUND_DEBUG */

/* our per instance private data struct */
struct esound_dsp_priv_data {
   int fd;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *esound_dsp_create(const void *flags);
static void esound_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int esound_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);

/* public variables */
const struct plugin_struct sysdep_dsp_esound = {
   "esound",
   "sysdep_dsp",
   "Esound DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   esound_dsp_create,
   2     /* lower priority as direct device access */
};

/* private variables */
static int esound_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *esound_dsp_create(const void *flags)
{
#ifdef ESOUND_DEBUG
   int server_fd;
   esd_server_info_t *info = NULL;
#endif
   struct esound_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   esd_format_t esd_format;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }
   
   /* alloc private data */
   if(!(priv = calloc(1, sizeof(struct esound_dsp_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct esound_dsp_priv_data\n");
      esound_dsp_destroy(dsp);
      return NULL;
   }
   
   /* fill in the functions and some data */
   priv->fd = -1;
   dsp->_priv = priv;
   dsp->write = esound_dsp_write;
   dsp->destroy = esound_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;
   
   /* open the sound device */
   esd_format = ESD_STREAM | ESD_PLAY ;
   esd_format |= (dsp->hw_info.type & SYSDEP_DSP_16BIT)?
      ESD_BITS16 : ESD_BITS8;
   esd_format |= (dsp->hw_info.type & SYSDEP_DSP_STEREO)?
      ESD_STEREO : ESD_MONO;
   
#ifdef ESOUND_DEBUG
   if((server_fd = esd_open_sound(params->device)) < 0)
   {
      fprintf(stderr, "error: esound server open failed\n");
      esound_dsp_destroy(dsp);
      return NULL;
   }
   
   if(!(info = esd_get_server_info(server_fd)))
   {
      fprintf(stderr, "error: esound get server info failed\n");
      esd_close(server_fd);
      esound_dsp_destroy(dsp);
      return NULL;
   }
   
   esd_print_server_info(info);
   esd_free_server_info(info);
   esd_close(server_fd);
#endif
   
   if((priv->fd = esd_play_stream(esd_format, dsp->hw_info.samplerate,
      params->device, "mame esound")) < 0)
   {
      fprintf(stderr, "error: esound open stream failed\n");
      esound_dsp_destroy(dsp);
      return NULL;
   }

   /* set non-blocking mode if selected */
   if(params->flags & SYSDEP_DSP_O_NONBLOCK)
   {
      long flags = fcntl(priv->fd, F_GETFL);
      if((flags < 0) || (fcntl(priv->fd, F_SETFL, flags|O_NONBLOCK) < 0))
      {
         perror("Esound-driver, error: fnctl");
         esound_dsp_destroy(dsp);
         return NULL;
      }
   }
   
   return dsp;
}

static void esound_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct esound_dsp_priv_data *priv = dsp->_priv;
   
   if(priv)
   {
      if(priv->fd >= 0)
         close(priv->fd);
      
      free(priv);
   }
   free(dsp);
}
   
static int esound_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
   int result;
   struct esound_dsp_priv_data *priv = dsp->_priv;

   result = write(priv->fd, data, count *
      esound_dsp_bytes_per_sample[dsp->hw_info.type]);
      
   if (result < 0)
   {
      fprintf(stderr, "error: esound write to stream failed\n");
      return -1;
   }
      
   return result / esound_dsp_bytes_per_sample[dsp->hw_info.type];
}

#endif /* ifdef SYSDEP_DSP_ESOUND */
