/* Sysdep ALSA mixer driver (based on Sysdep OSS sound mixer driver)

   Copyright 2000 Hans de Goede
   Copyright 2004 Shyouzou Sugitani
   
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>  /* ALSA sound library header */
#include "sysdep/sysdep_mixer.h"
#include "sysdep/sysdep_mixer_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct alsa_mixer_priv_data {
   snd_mixer_t *handle;
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_t *sid;
   long pmin, pmax;
};

/* public methods prototypes (static but exported through the sysdep_mixer or
   plugin struct) */
static void *alsa_mixer_create(const void *flags);
static void alsa_mixer_destroy(struct sysdep_mixer_struct *mixer);
static int alsa_mixer_set(struct sysdep_mixer_struct *mixer,
   int channel, int left, int right);
static int alsa_mixer_get(struct sysdep_mixer_struct *mixer,
   int channel, int *left, int *right);
   
/* private variables */

/* public variables */
static char *mixer_name = NULL;

struct rc_option alsa_mixer_opts[] = {
  /* name, shortname, type, dest, deflt, min, max, func, help */
  { "ALSA Mixer", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
  {"alsa-mixer", "amixer", rc_string, &mixer_name, "PCM", 0, 0, NULL,
   "Select which simple mixer control to use"},
  { NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

const struct plugin_struct sysdep_mixer_alsa = {
   "alsa",
   "sysdep_mixer",
   "ALSA mixer plugin",
   alsa_mixer_opts,
   NULL, /* no init */
   NULL, /* no exit */
   alsa_mixer_create,
   3     /* high priority */
};

/* public methods (static but exported through the sysdep_mixer or plugin
   struct) */
static void *alsa_mixer_create(const void *flags)
{
   int err;
   struct alsa_mixer_priv_data *priv = NULL;
   struct sysdep_mixer_struct *mixer = NULL;
   const struct sysdep_mixer_create_params *params = flags;
   char *card = "default";
   
   /* allocate the mixer struct */
   if (!(mixer = calloc(1, sizeof(struct sysdep_mixer_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_mixer_struct\n");
      return NULL;
   }
   
   /* alloc private data */
   if(!(priv = calloc(1, sizeof(struct alsa_mixer_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct alsa_mixer_priv_data\n");
      alsa_mixer_destroy(mixer);
      return NULL;
   }
   
   /* fill in the functions and some data */
   priv->handle = NULL;
   priv->elem = NULL;
   priv->sid = NULL;
   mixer->_priv = priv;
   mixer->set = alsa_mixer_set;
   mixer->get = alsa_mixer_get;
   mixer->destroy = alsa_mixer_destroy;
   
   /* open the mixer device */
   snd_mixer_selem_id_alloca(&priv->sid);
   snd_mixer_selem_id_set_index(priv->sid, 0);
   snd_mixer_selem_id_set_name(priv->sid, mixer_name);
   if((err = snd_mixer_open(&priv->handle, 0)) < 0) {
      perror("error: mixer open error");
      alsa_mixer_destroy(mixer);
      return NULL;
   }
   if((err = snd_mixer_attach(priv->handle, card)) < 0) {
      perror("error: mixer attach error");
      alsa_mixer_destroy(mixer);
      return NULL;
   }
   if((err = snd_mixer_selem_register(priv->handle, NULL, NULL)) < 0) {
      perror("error: mixer register error");
      alsa_mixer_destroy(mixer);
      return NULL;
   }
   if((err = snd_mixer_load(priv->handle)) < 0) {
      perror("error: mixer register error");
      alsa_mixer_destroy(mixer);
      return NULL;
   }
   priv->elem = snd_mixer_find_selem(priv->handle, priv->sid);
   if (!priv->elem) {
     perror("error: unable to find simple control");
     alsa_mixer_destroy(mixer);
     return NULL;
   }
   snd_mixer_selem_get_playback_volume_range(priv->elem, &priv->pmin, &priv->pmax);

   mixer->channel_available[SYSDEP_MIXER_PCM1] = 1;
   return mixer;
}

static void alsa_mixer_destroy(struct sysdep_mixer_struct *mixer)
{
   struct alsa_mixer_priv_data *priv = mixer->_priv;
   
   if(priv)
   {
      if(priv->handle)
         snd_mixer_close(priv->handle);
      
      free(priv);
   }
   free(mixer);
}
   
static int alsa_mixer_set(struct sysdep_mixer_struct *mixer,
   int channel, int left, int right)
{
   int err;
   float vol;
   struct alsa_mixer_priv_data *priv = mixer->_priv;
   
   vol = left * (priv->pmax - priv->pmin) / 100.0 + priv->pmin + 0.50;
   if((err = snd_mixer_selem_set_playback_volume(priv->elem, 0, (long)vol)) < 0) {
     perror("error: error setting left channel");
     return -1;
   }
   vol = right * (priv->pmax - priv->pmin) / 100.0 + priv->pmin + 0.50;
   if((err = snd_mixer_selem_set_playback_volume(priv->elem, 1, (long)vol)) < 0) {
     perror("error: error setting right channel");
     return -1;
   }
   return 0;
}

static int alsa_mixer_get(struct sysdep_mixer_struct *mixer,
   int channel, int *left, int *right)
{
   long vol;
   struct alsa_mixer_priv_data *priv = mixer->_priv;
   
   snd_mixer_selem_get_playback_volume(priv->elem, 0, &vol);
   *left = (vol - priv->pmin) * 100.0 / (priv->pmax - priv->pmin) + 0.50;
   snd_mixer_selem_get_playback_volume(priv->elem, 1, &vol);
   *right = (vol - priv->pmin) * 100.0 / (priv->pmax - priv->pmin) + 0.50;

   return 0;
}
