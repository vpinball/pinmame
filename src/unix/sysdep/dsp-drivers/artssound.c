/* Sysdep aRts sound dsp driver

   Copyright 2001 Manuel Teira
   
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
Version 0.1, April 2001
-initial release, based on the esound mame sound driver
*/

#ifdef SYSDEP_DSP_ARTS_TEIRA
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <artsc.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"


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

static int btime=10;

struct rc_option arts_dsp_opts[] = {
/* name, shortname, type, dest, */
/* deflt, min, max, func, help */
{"artsBufferTime","abt",rc_int, &btime,
"10",1,1000,NULL,"aRts buffer delay time"},
{NULL,NULL,rc_end,NULL,NULL,0,0,NULL,NULL}
};

/* public variables */
const struct plugin_struct sysdep_dsp_arts = {
   "arts",
   "sysdep_dsp",
   "aRts DSP plugin",
   arts_dsp_opts, 
   NULL, /* no init */
   NULL, /* no exit */
   arts_dsp_create,
   3     /* high priority */
};

/* private variables */
static int arts_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *arts_dsp_create(const void *flags)
{
   struct sysdep_dsp_struct *dsp = NULL;
   struct arts_dsp_priv_data *priv = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }

   if(!(priv = calloc(1, sizeof(struct arts_dsp_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct arts_dsp_priv_data\n");
      return NULL;
   }

   
   /* fill in the functions and some data */
   priv->stream=0; 
   dsp->_priv = priv;
   dsp->write = arts_dsp_write;
   dsp->destroy = arts_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;
   dsp->hw_info.bufsize = 1024;
   
   /* open the sound device */
   arts_init();
   priv->stream=arts_play_stream(dsp->hw_info.samplerate,
				 (dsp->hw_info.type&SYSDEP_DSP_16BIT)?16:8,
				 (dsp->hw_info.type&SYSDEP_DSP_STEREO)?2:1,
				 "xmame arts");
   
   /* Set the buffering time */
   arts_stream_set(priv->stream,ARTS_P_BUFFER_TIME,btime);

   /* set non-blocking mode if selected */
   if(params->flags & SYSDEP_DSP_O_NONBLOCK)
   {
	   arts_stream_set(priv->stream,ARTS_P_BLOCKING,0);
   }
   
   return dsp;
}

static void arts_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct arts_dsp_priv_data *priv = dsp->_priv;
   if(priv)
   {
	   arts_close_stream(priv->stream);
	   arts_free();
	   free(priv);
   }
   free(dsp);
}
   
static int arts_dsp_write(struct sysdep_dsp_struct *dsp,
			  unsigned char *data,
			  int count)
{
   int result;
   struct arts_dsp_priv_data *priv = dsp->_priv;

   result=arts_write(priv->stream,
		     data,
		     count * arts_dsp_bytes_per_sample[dsp->hw_info.type]);
      
   if (result<0)
   {
      fprintf(stderr, "error: arts_write error: %s\n",
	      arts_error_text(result));
      return -1;
   }
   return result/arts_dsp_bytes_per_sample[dsp->hw_info.type];
}

#endif /* ifdef SYSDEP_DSP_ARTS_TEIRA */
