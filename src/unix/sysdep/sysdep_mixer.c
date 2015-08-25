/* Sysdep sound mixer object

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
Version 0.1, February 2000
-initial release (Hans de Goede)
Version 0.2, March 2000
-added a plugin parameter to create, to override the global plugin
 configuration (Hans de Goede)
-protected sysdep_mixer_init against being called twice (Hans de Goede)
*/
#include "sysdep_mixer.h"
#include "sysdep_mixer_priv.h"
#include "sysdep_mixer_plugins.h"

/* #define SYSDEP_MIXER_DEBUG */

/* private func prototypes */
static int sysdep_mixer_list_plugins(struct rc_option *option,
   const char *arg, int priority);

/* private variables */
static char *sysdep_mixer_plugin = NULL;
static struct rc_struct *sysdep_mixer_rc = NULL;
static struct plugin_manager_struct *sysdep_mixer_plugin_manager = NULL;
static struct rc_option sysdep_mixer_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Sound mixer related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "sound-mixer-plugin", "smp",		rc_string,	&sysdep_mixer_plugin,
     NULL,		0,			0,		NULL,
     "Select which plugin to use for the sound mixer" },
   { "list-mixer-plugins", "lmp",		rc_use_function_no_arg, NULL,
     NULL,		0,			0,		sysdep_mixer_list_plugins,
     "List available sound-mixer plugins" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};
static const struct plugin_struct *sysdep_mixer_plugins[] = {
#ifdef SYSDEP_MIXER_OSS
   &sysdep_mixer_oss,
#endif
#ifdef SYSDEP_MIXER_NETBSD
   &sysdep_mixer_netbsd,
#endif
#ifdef SYSDEP_MIXER_SOLARIS
   &sysdep_mixer_solaris,
#endif
#ifdef SYSDEP_MIXER_NEXT
   &sysdep_mixer_next,
#endif
#ifdef DSYSDEP_MIXER_IRIX
   &sysdep_mixer_irix,
#endif
#ifdef DSYSDEP_MIXER_AIX
   &sysdep_mixer_aix,
#endif
   NULL
};
#ifdef SYSDEP_MIXER_DEBUG
const char *sysdep_mixer_names[] = SYSDEP_MIXER_NAMES;
#endif

/* private methods */
static int sysdep_mixer_list_plugins(struct rc_option *option,
   const char *arg, int priority)
{
   fprintf(stdout, "Sound mixer plugins:\n\n");
   plugin_manager_list_plugins(sysdep_mixer_plugin_manager, stdout);
   
   return -1;
}

/* public methods */
int sysdep_mixer_init(struct rc_struct *rc, const char *plugin_path)
{
   if(!sysdep_mixer_rc)
   {
      if(rc && rc_register(rc, sysdep_mixer_opts))
         return -1;
      sysdep_mixer_rc = rc;
   }
   
   if(!sysdep_mixer_plugin_manager)
   {
      if(!(sysdep_mixer_plugin_manager = plugin_manager_create("sysdep_mixer",
         rc)))
      {
         sysdep_mixer_exit();
         return -1;
      }
      /* no need to fail here, if we don't have any plugins,
         sysdep_mixer_create will always fail, but that doesn't have to be
         fatal, failing here usually is! */
      plugin_manager_register(sysdep_mixer_plugin_manager,
         sysdep_mixer_plugins);
      plugin_manager_load(sysdep_mixer_plugin_manager, plugin_path, NULL);
      if(plugin_manager_init_plugin(sysdep_mixer_plugin_manager, NULL))
      {
         fprintf(stderr, "warning: no mixer plugins available\n");
      }
   }

   return 0;
}

void sysdep_mixer_exit(void)
{
   if(sysdep_mixer_plugin_manager)
   {
      plugin_manager_destroy(sysdep_mixer_plugin_manager);
      sysdep_mixer_plugin_manager = NULL;
   }
   if(sysdep_mixer_rc)
   {
      rc_unregister(sysdep_mixer_rc, sysdep_mixer_opts);
      sysdep_mixer_rc = NULL;
   }
}

struct sysdep_mixer_struct *sysdep_mixer_create(const char *plugin,
   const char *device, int flags)
{
   int i;
   struct sysdep_mixer_struct *mixer = NULL;
   struct sysdep_mixer_create_params params;
   
   /* fill the params struct */
   params.device = device;
   
   /* create the instance */
   if(!(mixer = plugin_manager_create_instance(sysdep_mixer_plugin_manager,
      plugin? plugin:sysdep_mixer_plugin, &params)))
      return NULL;
   
   /* fill the mixer cache and save the original settings */
   for(i = 0; i < SYSDEP_MIXER_CHANNELS; i++)
   {
      if(mixer->channel_available[i])
      {
#ifdef SYSDEP_MIXER_DEBUG
	 fprintf(stderr, "debug: mixer got channel %s\n",
	    sysdep_mixer_names[i]);
#endif
         if(mixer->get(mixer, i, &mixer->cache_left[i],
            &mixer->cache_right[i]))
         {
            sysdep_mixer_destroy(mixer);
            return NULL;
         }
         mixer->orig_left[i]  = mixer->cache_left[i];
         mixer->orig_right[i] = mixer->cache_right[i];
      }
   }
   
   /* save our flags */
   mixer->flags = flags;
   
   return mixer;
}

void sysdep_mixer_destroy(struct sysdep_mixer_struct *mixer)
{
   int i, left, right;
   
   /* restore orig settings if requested */
   for(i = 0; i < SYSDEP_MIXER_CHANNELS; i++)
   {
      /* check that the channel wasn't modified under our ass */
      sysdep_mixer_get(mixer, i, &left, &right);
      if(mixer->restore_channel[i])
      {
#ifdef SYSDEP_MIXER_DEBUG
         fprintf(stderr, "debug: sysdep_mixer: restoring channel %s\n",
	    sysdep_mixer_names[i]);
#endif
         sysdep_mixer_set(mixer, i, mixer->orig_left[i],
            mixer->orig_right[i]);
      }
   }
   mixer->destroy(mixer);
}

int sysdep_mixer_channel_available(struct sysdep_mixer_struct *mixer,
   int channel)
{
   return mixer->channel_available[channel];
}

int sysdep_mixer_set(struct sysdep_mixer_struct *mixer, int channel,
   int left, int right)
{
   if(!mixer->channel_available[channel])
      return -1;

   if(mixer->set(mixer, channel, left, right))
      return -1;
   
   mixer->cache_left[channel]  = left;
   mixer->cache_right[channel] = right;
   if(mixer->flags & SYSDEP_MIXER_RESTORE_SETTINS_ON_EXIT)
      mixer->restore_channel[channel] = 1;
   
   return 0;
}

int sysdep_mixer_get(struct sysdep_mixer_struct *mixer, int channel,
   int *left, int *right)
{
   if(!mixer->channel_available[channel])
      return -1;

   if(mixer->get(mixer, channel, left, right))
      return -1;
   
   /* if we're close to the cached values use the cache, to avoid repeated
      calls to get->set, causing the volume to slide */
   if((*left >= (mixer->cache_left[channel] - 5)) &&
      (*left <= (mixer->cache_left[channel] + 5)) &&
      (*right >= (mixer->cache_right[channel] - 5)) &&
      (*right <= (mixer->cache_right[channel] + 5)))
   {
#ifdef SYSDEP_MIXER_DEBUG
      fprintf(stderr, "debug: sysdep_mixer: cached\n");
#endif
      *left = mixer->cache_left[channel];
      *right = mixer->cache_right[channel];
   }
   else 
   {
#ifdef SYSDEP_MIXER_DEBUG
      fprintf(stderr, "debug: sysdep_mixer: modified under our ass\n");
#endif
      /* The channel was modified under our ass, no need to restore it anymore.
         Save the new values as original values, so that if we change it
         later on we restore the new values */
      mixer->restore_channel[channel] = 0;
      mixer->cache_left[channel]  = *left;
      mixer->cache_right[channel] = *right;
      mixer->orig_left[channel]  = *left;
      mixer->orig_right[channel] = *right;
   }
      
   return 0;
}
