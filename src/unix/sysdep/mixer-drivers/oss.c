/* Sysdep Open Sound System sound mixer driver

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
-initial release (Hans de Goede)
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
#include "sysdep/sysdep_mixer.h"
#include "sysdep/sysdep_mixer_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct oss_mixer_priv_data {
   int fd;
};

/* public methods prototypes (static but exported through the sysdep_mixer or
   plugin struct) */
static void *oss_mixer_create(const void *flags);
static void oss_mixer_destroy(struct sysdep_mixer_struct *mixer);
static int oss_mixer_set(struct sysdep_mixer_struct *mixer,
   int channel, int left, int right);
static int oss_mixer_get(struct sysdep_mixer_struct *mixer,
   int channel, int *left, int *right);
   
/* private variables */
const int oss_mixer_sysdep_to_oss[] = {
   SOUND_MIXER_VOLUME, /* SYSDEP_MIXER_VOLUME */
   SOUND_MIXER_PCM,    /* SYSDEP_MIXER_PCM1 */
   SOUND_MIXER_ALTPCM, /* SYSDEP_MIXER_PCM2 */
   SOUND_MIXER_SYNTH,  /* SYSDEP_MIXER_SYNTH */
   SOUND_MIXER_CD,     /* SYSDEP_MIXER_CD */
   SOUND_MIXER_LINE,   /* SYSDEP_MIXER_LINE1 */
   SOUND_MIXER_LINE1,  /* SYSDEP_MIXER_LINE2 */
   SOUND_MIXER_LINE2,  /* SYSDEP_MIXER_LINE3 */
   SOUND_MIXER_BASS,   /* SYSDEP_MIXER_BASS */
   SOUND_MIXER_TREBLE, /* SYSDEP_MIXER_TREBLE */
};

/* public variables */
const struct plugin_struct sysdep_mixer_oss = {
   "oss",
   "sysdep_mixer",
   "Open Sound System mixer plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   oss_mixer_create,
   3     /* high priority */
};

/* public methods (static but exported through the sysdep_mixer or plugin
   struct) */
static void *oss_mixer_create(const void *flags)
{
   int i, j;
   struct oss_mixer_priv_data *priv = NULL;
   struct sysdep_mixer_struct *mixer = NULL;
   const struct sysdep_mixer_create_params *params = flags;
   const char *device = params->device;
   
   /* allocate the mixer struct */
   if (!(mixer = calloc(1, sizeof(struct sysdep_mixer_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_mixer_struct\n");
      return NULL;
   }
   
   /* alloc private data */
   if(!(priv = calloc(1, sizeof(struct oss_mixer_priv_data))))
   {
      fprintf(stderr,
         "error malloc failed for struct oss_mixer_priv_data\n");
      oss_mixer_destroy(mixer);
      return NULL;
   }
   
   /* fill in the functions and some data */
   priv->fd = -1;
   mixer->_priv = priv;
   mixer->set = oss_mixer_set;
   mixer->get = oss_mixer_get;
   mixer->destroy = oss_mixer_destroy;
   
   /* open the mixer device */
   if (!device)
      device = "/dev/mixer";
   
   if((priv->fd = open(device, O_WRONLY, 0)) < 0) {
      perror("error: /dev/mixer");
      oss_mixer_destroy(mixer);
      return NULL;
   }
   
   /* and check which channels are available */
   if (ioctl(priv->fd, SOUND_MIXER_READ_DEVMASK, &i))
   {
      perror("error: SOUND_MIXER_READ_DEVMASK");
      oss_mixer_destroy(mixer);
      return NULL;
   }
   for(j=0; j < SYSDEP_MIXER_CHANNELS; j++)
      if(i & (0x01 << oss_mixer_sysdep_to_oss[j]))
         mixer->channel_available[j] = 1;
   
   return mixer;
}

static void oss_mixer_destroy(struct sysdep_mixer_struct *mixer)
{
   struct oss_mixer_priv_data *priv = mixer->_priv;
   
   if(priv)
   {
      if(priv->fd >= 0)
         close(priv->fd);
      
      free(priv);
   }
   free(mixer);
}
   
static int oss_mixer_set(struct sysdep_mixer_struct *mixer,
   int channel, int left, int right)
{
   int i = left | right << 8;
   struct oss_mixer_priv_data *priv = mixer->_priv;
   
   if(ioctl(priv->fd, MIXER_WRITE(oss_mixer_sysdep_to_oss[channel]), &i))
   {
      perror("error: SOUND_MIXER_WRITE");
      return -1;
   }
   return 0;
}

static int oss_mixer_get(struct sysdep_mixer_struct *mixer,
   int channel, int *left, int *right)
{
   int value;
   struct oss_mixer_priv_data *priv = mixer->_priv;
   
   if(ioctl(priv->fd, MIXER_READ(oss_mixer_sysdep_to_oss[channel]),
      &value))
   {
      perror("error: SOUND_MIXER_READ");
      return -1;
   }
      
   *left = value & 0xFF;
   *right = (value >> 8) & 0xFF;

   return 0;
}
