#ifdef SYSDEP_DSP_ARTS_SMOTEK

/* Sysdep aRts sound dsp driver
 
   Copyright 2001 Petr Smotek
 
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
Version 0.1, May 16 2001
-initial release, based on the xmame oss and esound drivers done by
 Hans de Goede (Petr Smotek)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <artsc.h>

#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* #define ARTS_DEBUG */

/* our per instance private data struct */
struct arts_dsp_priv_data {
   arts_stream_t stream;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static void *arts_dsp_create(const void *flags);
static void arts_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int arts_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);

/* public variables */
const struct plugin_struct sysdep_dsp_arts = {
   "arts",
   "sysdep_dsp",
   "aRts DSP plugin",
   NULL,
   NULL,
   NULL,
   arts_dsp_create,
   2 
};

/* private variables */
static int arts_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *arts_dsp_create(const void *flags)
{
   int i, j, result, bits, channels, block;
   struct arts_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }
   
   /* alloc private data */
   if(!(priv = calloc(1, sizeof(struct arts_dsp_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct arts_dsp_priv_data\n");
      arts_dsp_destroy(dsp);
      return NULL;
   }
   
   /* fill in the functions and some data */
   priv->stream = NULL;
   dsp->_priv = priv;
   dsp->write = arts_dsp_write;
   dsp->destroy = arts_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;

   result = arts_init();
   if (result < 0)
   {
      fprintf(stderr,
         "arts_init error: %s\n", arts_error_text(result));
      arts_dsp_destroy(dsp);
      return NULL;
   }

   bits = (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8;
   channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 2 : 1;
   priv->stream = arts_play_stream(dsp->hw_info.samplerate, bits, channels,
      "xmame arts");

   block = (params->flags & SYSDEP_DSP_O_NONBLOCK) ? 0 : 1;
   result = arts_stream_set(priv->stream, ARTS_P_BLOCKING, block);
   if (result < 0)
   {
      fprintf(stderr,
         "arts_stream_set error: %s\n", arts_error_text(result));
      arts_dsp_destroy(dsp);
      return NULL;
   }
   else 
   {
      if (result != block)
      {
         fprintf(stderr,
            "arts_stream_set ARTS_P_BLOCKING to %d failed\n", block);
         arts_dsp_destroy(dsp);
         return NULL;
      }
   }

   /* calculate fragsize & number of frags */
   /* fragsize (as power of 2) */
   i = 7;
   if (dsp->hw_info.type & SYSDEP_DSP_16BIT) i++;
   if (dsp->hw_info.type & SYSDEP_DSP_STEREO) i++;
   i += dsp->hw_info.samplerate / 22050;
 
   /* number of frags */
   j = ((dsp->hw_info.samplerate * arts_dsp_bytes_per_sample[dsp->hw_info.type] * params->bufsize) / (0x01 << i)) + 1;

   arts_stream_set(priv->stream, ARTS_P_BUFFER_SIZE, (0x01<<i)*j);

#ifdef ARTS_DEBUG
   /* print some info messages ;) */
   fprintf(stderr, "info: aRts buffer size    : %d\n", 
      arts_stream_get(priv->stream, ARTS_P_BUFFER_SIZE));
   fprintf(stderr, "info: aRts buffer time    : %d\n",
      arts_stream_get(priv->stream, ARTS_P_BUFFER_TIME));
   fprintf(stderr, "info: aRts server latency : %d\n",
      arts_stream_get(priv->stream, ARTS_P_SERVER_LATENCY));
   fprintf(stderr, "info: aRts total latency  : %d\n",
      arts_stream_get(priv->stream, ARTS_P_TOTAL_LATENCY));
   fprintf(stderr, "info: aRts blocking       : %s\n",
      arts_stream_get(priv->stream, ARTS_P_BLOCKING) ? "yes" : "no");
#endif /* ifdef ARTS_DEBUG */

   return dsp;
}

static void arts_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct arts_dsp_priv_data *priv = dsp->_priv;
   
   if(priv)
   {
      if (priv->stream)
         arts_close_stream(priv->stream);
      arts_free();
      free(priv);
   }
   free(dsp);
}

static int arts_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
   int result;
   struct arts_dsp_priv_data *priv = dsp->_priv;

   result = arts_write(priv->stream, data, count *
      arts_dsp_bytes_per_sample[dsp->hw_info.type]);
      
   if (result < 0)
   {
      fprintf(stderr,
         "arts_write error: %s\n", arts_error_text(result));
      return -1;
   }
      
   return result / arts_dsp_bytes_per_sample[dsp->hw_info.type];
}

#endif /* ifdef SYSDEP_DSP_ARTS_SMOTEK */
