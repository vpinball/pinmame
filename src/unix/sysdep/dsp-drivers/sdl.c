#ifdef SYSDEP_DSP_SDL

/* Sysdep SDL sound dsp driver

   Copyright 2001 Jack Burton aka Stefano Ceccherini
   <burton666@freemail.it>
   
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

/* Thank goes to Caz Jones <turok2@currantbun.com>, who fixed this plugin 
   and made it finally sound good.
*/
/* Changelog
Version 0.1, January 2002
-initial release, based on various xmame dsp plugins and Yoshi's code


*/

// SDL defines this to __inline__ which no longer works with gcc 5+ ?
// TODO find the correct way to handle this, similar issue with ALSA
#ifndef SDL_FORCE_INLINE
#if ( (defined(__GNUC__) && (__GNUC__ >= 5)))
#define SDL_FORCE_INLINE __attribute__((always_inline)) static inline
#endif
#endif /* take the definition from SDL */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* callback function prototype */
static void sdl_fill_sound(void *unused, Uint8 *stream, int len);

/* public methods prototypes */
static void *sdl_dsp_create(const void *flags);
static void sdl_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int sdl_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);


/* public variables */
const struct plugin_struct sysdep_dsp_sdl = {
   "sdl",
   "sysdep_dsp",
   "Simple Direct Library DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   sdl_dsp_create,
   3     /* high priority */
};

struct sdl_info {
   SDL_AudioStream *stream;
};

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
static void *sdl_dsp_create(const void *flags)
{
   struct sdl_dsp_priv_data *priv = NULL;
   struct sysdep_dsp_struct *dsp = NULL;
   const struct sysdep_dsp_create_params *params = flags;
   const char *device = params->device;
   SDL_AudioSpec *audiospec;
   
   /* allocate the dsp struct */
   if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_dsp_struct\n");
      return NULL;
   }
   
   
   if (!(audiospec = calloc(1, sizeof(SDL_AudioSpec))))
   {
   		fprintf(stderr, "error malloc failed for SDL_AudioSpec\n");
   		sdl_dsp_destroy(dsp);
   		return NULL;
   }
   
   /* fill in the functions and some data */
   dsp->_priv = priv;
   dsp->write = sdl_dsp_write;
   dsp->destroy = sdl_dsp_destroy;
   dsp->hw_info.type = params->type;
   dsp->hw_info.samplerate = params->samplerate;

   audiospec->format = (dsp->hw_info.type & SYSDEP_DSP_16BIT)?
   							SDL_AUDIO_S16 : SDL_AUDIO_S8;
   audiospec->channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? 2:1;
   audiospec->freq = dsp->hw_info.samplerate;

   /* Open audio device */
   if(SDL_WasInit(SDL_INIT_VIDEO)!=0)   /* If sdl video system is already */
      SDL_InitSubSystem(SDL_INIT_AUDIO);/* initialized, we just initialize */
   else									/* the audio subsystem */
   	  SDL_Init(SDL_INIT_AUDIO);   		/* else we MUST use "SDL_Init" */
   										/* (untested) */

	fprintf(stderr, "sdl info: driver = %s\n", SDL_GetCurrentAudioDriver());

	// get audio subsystem and open a queue with above spec
	SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audiospec, NULL, NULL);
	if (stream == NULL) {
		fprintf(stderr, "sdl error: SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
		return NULL;
	}

	const SDL_AudioDeviceID device_id = SDL_GetAudioStreamDevice(stream);
	SDL_ResumeAudioDevice(device_id);

	// get the actual obtained spec
	SDL_GetAudioStreamFormat(stream, audiospec, NULL);

	// print the id
	fprintf(stderr, "sdl info: device id = %d\n", device_id);
	// print the obtained contents
	fprintf(stderr, "sdl info: obtained->format = %d\n", audiospec->format);
	fprintf(stderr, "sdl info: obtained->channels = %d\n", audiospec->channels);
	fprintf(stderr, "sdl info: obtained->freq = %d\n", audiospec->freq);

	// free the spec
	free(audiospec);

	struct sdl_info *info = (struct sdl_info *)calloc(1, sizeof(struct sdl_info));
	info->stream = stream;

	dsp->_priv = info;
   
   fprintf(stderr, "sdl info: audiodevice %s set to %dbit linear %s %dHz\n",
      device, (dsp->hw_info.type & SYSDEP_DSP_16BIT)? 16:8,
      (dsp->hw_info.type & SYSDEP_DSP_STEREO)? "stereo":"mono",
      dsp->hw_info.samplerate);
      
   return dsp;
}

static void sdl_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   const struct sdl_info *info = (struct sdl_info *)dsp->_priv;
   SDL_DestroyAudioStream(info->stream);
   free(dsp);
}
   

static int sdl_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count)
{
	// count is the number of samples to write
	const int len = (dsp->hw_info.type & SYSDEP_DSP_STEREO)? count * 4 : count * 2;

	// get stream from dsp
	const struct sdl_info *info = (struct sdl_info *)dsp->_priv;

	const bool success = SDL_PutAudioStreamData(info->stream, data, len);
	if (!success) {
		fprintf(stderr, "error: SDL_PutAudioStreamData() failed: %s\n", SDL_GetError());
		return 0;
	}
	return	count;
}

#endif /* ifdef SYSDEP_DSP_SDL */
