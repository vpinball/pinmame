#ifdef SYSDEP_DSP_SDL3

/* Sysdep SDL3 sound dsp driver

   Successor of the SDL 1.2 dsp driver (sdl.c) by Jack Burton / Stefano
   Ceccherini and Caz Jones.

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

/* The samples are pushed into an SDL_AudioStream bound to the default
   playback device. SDL takes care of format/samplerate conversion and of
   feeding the device from its own thread. The free space reported to the
   sound_stream code is the difference between the wanted queue depth and
   what is still queued in the stream, which paces the emulation the same
   way the ALSA/OSS drivers do. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* Minimum queue depth in seconds. The generic -bufsize option (in frames)
   is used when it asks for more. */
#define SDL3_DSP_MIN_QUEUE_SECONDS 0.1f

struct sdl3_dsp_priv_data {
   SDL_AudioStream *stream;
   int bytes_per_sample;
   int queue_samples; /* wanted queue depth in samples */
};

/* public methods prototypes */
static void *sdl3_dsp_create(const void *flags);
static void sdl3_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int sdl3_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int sdl3_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);

/* public variables */
const struct plugin_struct sysdep_dsp_sdl3 = {
   "sdl3",
   "sysdep_dsp",
   "SDL3 DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   sdl3_dsp_create,
   3     /* high priority */
};

static int sdl3_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

static void *sdl3_dsp_create(const void *flags)
{
   struct sdl3_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   SDL_AudioSpec spec;
   float queue_seconds;

   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr, "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }

   /* alloc private data */
   if (!(priv = calloc(1, sizeof(struct sdl3_dsp_priv_data))))
   {
      fprintf(stderr, "error malloc failed for struct sdl3_dsp_priv_data\n");
      sdl3_dsp_destroy(dsp);
      return NULL;
   }

   /* fill in the functions and some data */
   dsp->_priv = priv;
   dsp->get_freespace = sdl3_dsp_get_freespace;
   dsp->write = sdl3_dsp_write;
   dsp->destroy = sdl3_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;

   /* SDL reference counts the subsystems, the video driver may already have
      called SDL_Init */
   if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
   {
      fprintf(stderr, "SDL3 dsp: error: SDL audio init failed: %s\n", SDL_GetError());
      sdl3_dsp_destroy(dsp);
      return NULL;
   }

   SDL_zero(spec);
   spec.format = (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? SDL_AUDIO_S16 : SDL_AUDIO_S8;
   spec.channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 2 : 1;
   spec.freq = dsp->hw_info.samplerate;

   priv->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
   if (!priv->stream)
   {
      fprintf(stderr, "SDL3 dsp: error: opening the audio device failed: %s\n", SDL_GetError());
      sdl3_dsp_destroy(dsp);
      return NULL;
   }

   priv->bytes_per_sample = sdl3_dsp_bytes_per_sample[dsp->hw_info.type];

   queue_seconds = params->bufsize;
   if (queue_seconds < SDL3_DSP_MIN_QUEUE_SECONDS)
      queue_seconds = SDL3_DSP_MIN_QUEUE_SECONDS;
   priv->queue_samples = (int)(queue_seconds * dsp->hw_info.samplerate);
   dsp->hw_info.bufsize = priv->queue_samples;

   if (!SDL_ResumeAudioStreamDevice(priv->stream))
   {
      fprintf(stderr, "SDL3 dsp: error: starting the audio device failed: %s\n", SDL_GetError());
      sdl3_dsp_destroy(dsp);
      return NULL;
   }

   fprintf(stderr, "SDL3 dsp: info: driver %s, %dbit linear %s %dHz, queue %d samples (%.0f ms)\n",
      SDL_GetCurrentAudioDriver(),
      (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? "stereo" : "mono",
      dsp->hw_info.samplerate, priv->queue_samples, queue_seconds * 1000.0f);

   return dsp;
}

static void sdl3_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   struct sdl3_dsp_priv_data *priv = dsp->_priv;

   if (priv)
   {
      if (priv->stream)
      {
         SDL_DestroyAudioStream(priv->stream);
         SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
      free(priv);
   }
   free(dsp);
}

static int sdl3_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
   struct sdl3_dsp_priv_data *priv = dsp->_priv;
   int queued = SDL_GetAudioStreamQueued(priv->stream);

   if (queued < 0)
   {
      fprintf(stderr, "SDL3 dsp: error: SDL_GetAudioStreamQueued failed: %s\n", SDL_GetError());
      return 0;
   }
   queued /= priv->bytes_per_sample;

   return (queued < priv->queue_samples) ? priv->queue_samples - queued : 0;
}

static int sdl3_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
   struct sdl3_dsp_priv_data *priv = dsp->_priv;

   if (!SDL_PutAudioStreamData(priv->stream, data, count * priv->bytes_per_sample))
   {
      fprintf(stderr, "SDL3 dsp: error: SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
      return -1;
   }
   return count;
}

#endif /* ifdef SYSDEP_DSP_SDL3 */
