/* Sound stream object

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
Version 0.1, March 2000
-initial release (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sound_stream.h"
#include "sound_stream_priv.h"
#include "fifo.h"

/* #define SOUND_STREAM_DEBUG */
/* #define SOUND_STREAM_WARNING */

/* private methods */
FIFO(INLINE, sample_buf, struct sound_stream_sample_buf *)

/* public methods (in sound_stream.h) */
struct sound_stream_struct *sound_stream_create(struct sysdep_dsp_struct* dsp,
   int type, int buf_size, int buf_count)
{
   int i, bytes_per_sample[] = SYSDEP_DSP_BYTES_PER_SAMPLE;
   struct sound_stream_struct *stream = NULL;
   
   /* allocate the sound_stream struct */
   if(!(stream = calloc(1, sizeof(struct sound_stream_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sound_stream_struct\n");
      return NULL;
   }
   
   /* fill in some value's */
   stream->dsp = dsp;
   stream->bytes_per_sample = bytes_per_sample[type];
   stream->sample_buf_size = buf_size;
   stream->sample_buf_count = buf_count;
   
   /* allocate the output buffer */
   stream->output_buf_size = sysdep_dsp_get_max_freespace(stream->dsp);
   if(!(stream->output_buf = calloc(stream->output_buf_size,
      stream->bytes_per_sample)))
   {
      fprintf(stderr,
         "error malloc failed for sound_stream output buffer\n");
      sound_stream_destroy(stream);
      return NULL;
   }
   
   /* create the fifo's */
   if(!(stream->sample_buf_fifo = sample_buf_fifo_create(
      stream->sample_buf_count)))
   {
      sound_stream_destroy(stream);
      return NULL;
   }
   if(!(stream->empty_sample_buf_fifo = sample_buf_fifo_create(
      stream->sample_buf_count)))
   {
      sound_stream_destroy(stream);
      return NULL;
   }
   
   /* create sample_buf_count sample_buf structs */
   if(!(stream->sample_buf = calloc(stream->sample_buf_count,
      sizeof(struct sound_stream_sample_buf))))
   {
      fprintf(stderr,
         "error malloc failed for struct sound_stream_sample_buf\n");
      sound_stream_destroy(stream);
      return NULL;
   }
   
   /* allocate the sample_buf-buffers and fill the empty_sample_buf_fifo */
   for(i = 0; i < stream->sample_buf_count; i++)
   {
      if(!(stream->sample_buf[i].data =
         calloc(stream->sample_buf_size, stream->bytes_per_sample)))
      {
         fprintf(stderr,
            "error malloc failed for sample_buf data\n");
         sound_stream_destroy(stream);
         return NULL;
      }
      sample_buf_fifo_put(stream->empty_sample_buf_fifo,
         &stream->sample_buf[i]);
   }
   
   return stream;
}

void sound_stream_destroy(struct sound_stream_struct *stream)
{
   int i;
   
   if(stream->output_buf)
      free(stream->output_buf);
      
   if(stream->sample_buf_fifo)
      sample_buf_fifo_destroy(stream->sample_buf_fifo);
   
   if(stream->empty_sample_buf_fifo)
      sample_buf_fifo_destroy(stream->empty_sample_buf_fifo);
      
   if(stream->sample_buf)
   {
      for(i = 0; i < stream->sample_buf_count; i++)
         if(stream->sample_buf[i].data)
            free(stream->sample_buf[i].data);
      
      free(stream->sample_buf);
   }
   
   free(stream);
}

void sound_stream_write(struct sound_stream_struct *stream,
   unsigned char *data, int samples)
{
   struct sound_stream_sample_buf *sample_buf;
   
   /* add the samples to our sample_buf_fifo */
   while(samples)
   {
      if(sample_buf_fifo_get(stream->empty_sample_buf_fifo, &sample_buf) == 0)
      {
         int samples_this_loop = samples;
         
         if(samples_this_loop > stream->sample_buf_size)
            samples_this_loop = stream->sample_buf_size;
         
         sample_buf->length = samples_this_loop;
         sample_buf->pos = 0;
         memcpy(sample_buf->data, data, samples_this_loop *
            stream->bytes_per_sample);
         sample_buf_fifo_put(stream->sample_buf_fifo, sample_buf);
         
         samples -= samples_this_loop;
         data += samples_this_loop * stream->bytes_per_sample;
      }
      else
      {
#ifdef SOUND_STREAM_WARNING
         fprintf(stderr, "warning: sound_stream: fifo full, dropping sample\n");
#endif
         samples = 0;
      }
   }
}

void sound_stream_update(struct sound_stream_struct *stream)
{
   int freespace;
   struct sound_stream_sample_buf *sample_buf;

   /* get freespace */   
   freespace = sysdep_dsp_get_freespace(stream->dsp);
#ifdef SOUND_STREAM_DEBUG
   fprintf(stderr, "debug: sound_stream: freespace = %d\n", freespace);
#endif

   while(freespace > 0)
   {
      int result, samples_this_loop;
      
      /* we peek a sample_buf since we might not completly use it, once we're
         done we get it to disgard it. If there haven't been any writes to the
         fifo yet it can be empty, in this case there is nothing we can do. */
      if(sample_buf_fifo_peek(stream->sample_buf_fifo, &sample_buf))
         return;
      
#ifdef SOUND_STREAM_DEBUG      
      fprintf(stderr, "sample_buf->length = %d, sample_buf->pos = %d\n",
         sample_buf->length, sample_buf->pos);
#endif
      
      samples_this_loop = sample_buf->length - sample_buf->pos;
      if (samples_this_loop > freespace)
         samples_this_loop = freespace;
      
      result = sysdep_dsp_write(stream->dsp,
         sample_buf->data + (sample_buf->pos * stream->bytes_per_sample),
         samples_this_loop);
      
      /* woops something went wrong (EAGAIN ?!), try again next update */
      if (result < 0)
      {
#ifdef SOUND_STREAM_WARNING
         fprintf(stderr, "warning: sound_stream: sysdep_dsp_write returned -1\n");
#endif
         return;
      }
      
      sample_buf->pos += result;
      
      /* was there enough space in the sound device to write the entire sample?
         otherwise try again next update */
      if (result < samples_this_loop)
      {
#ifdef SOUND_STREAM_WARNING
         fprintf(stderr,
            "warning: sound_stream: sysdep_dsp_write returned %d, expected %d\n",
            result, samples_this_loop);
#endif
         return;
      }
         
      /* is this sample_buf finished ? */
      if(sample_buf->pos == sample_buf->length)
      {
         /* if we have more then one sample_buf queued, queue the next,
            otherwise loop this one */
         if(sample_buf_fifo_in_use(stream->sample_buf_fifo) > 1)
         {
            sample_buf_fifo_get(stream->sample_buf_fifo, &sample_buf);
            sample_buf_fifo_put(stream->empty_sample_buf_fifo, sample_buf);
         }
         else
         {
#ifdef SOUND_STREAM_WARNING
            fprintf(stderr, "warning: sound_stream: fifo empty, looping sample\n");
#endif
            sample_buf->pos = 0;
         }
      }
      
      freespace -= result;
   }
}
