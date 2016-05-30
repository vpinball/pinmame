/* Sysdep sound dsp object

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
Version 0.2, March 2000
-added a plugin parameter to create, to override the global plugin
 configuration (Hans de Goede)
-made bufsize a parameter to create, removed the global bufsize configuration,
 the code using us should have a much better idea of what bufsize should be
 then we do. (Hans de Goede)
-protected sysdep_dsp_init against being called twice (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdep_dsp.h"
#include "sysdep_dsp_priv.h"
#include "sysdep_dsp_plugins.h"
#include "plugin_manager.h"

/* private func prototypes */
static int sysdep_dsp_list_plugins(struct rc_option *option, const char *arg,
   int priority);

/* private variables */
static char *sysdep_dsp_plugin = NULL;
static int sysdep_dsp_use_timer = 0;
static int sysdep_dsp_bytes_per_sample[] = SYSDEP_DSP_BYTES_PER_SAMPLE;
static struct rc_struct *sysdep_dsp_rc = NULL;
static struct plugin_manager_struct *sysdep_dsp_plugin_manager = NULL;
static struct rc_option sysdep_dsp_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Digital sound related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "dsp-plugin", 	"dp",			rc_string,	&sysdep_dsp_plugin,
     NULL,		0,			0,		NULL,
     "Select which plugin to use for digital sound" },
   { "list-dsp-plugins", "ldp",			rc_use_function_no_arg, NULL,
     NULL,		0,			0,		sysdep_dsp_list_plugins,
     "List available sound-dsp plugins" },
   { "timer",		"ti",			rc_bool,	&sysdep_dsp_use_timer,
     "0",		0,			0,		NULL,
     "Use / don't use timer based audio (normally it will be used automagically when nescesarry)" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};
static const struct plugin_struct *sysdep_dsp_plugins[] = {
#ifdef SYSDEP_DSP_OSS
   &sysdep_dsp_oss,
#endif
#ifdef SYSDEP_DSP_NETBSD
   &sysdep_dsp_netbsd,
#endif
#ifdef SYSDEP_DSP_SOLARIS
   &sysdep_dsp_solaris,
#endif
#ifdef SYSDEP_DSP_SOUNDKIT
   &sysdep_dsp_soundkit,
#endif
#ifdef SYSDEP_DSP_COREAUDIO
   &sysdep_dsp_coreaudio,
#endif
#ifdef SYSDEP_DSP_IRIX
   &sysdep_dsp_irix,
#endif
#ifdef SYSDEP_DSP_AIX
   &sysdep_dsp_aix,
#endif
#ifdef SYSDEP_DSP_ESOUND
   &sysdep_dsp_esound,
#endif
#ifdef SYSDEP_DSP_ALSA
   &sysdep_dsp_alsa,
#endif
#ifdef SYSDEP_DSP_ARTS_TEIRA
   &sysdep_dsp_arts,
#endif
#ifdef SYSDEP_DSP_ARTS_SMOTEK
   &sysdep_dsp_arts,
#endif
#ifdef SYSDEP_DSP_SDL
   &sysdep_dsp_sdl,
#endif
#ifdef SYSDEP_DSP_WAVEOUT
   &sysdep_dsp_waveout,
#endif
   NULL
};

/* private methods */
static int sysdep_dsp_list_plugins(struct rc_option *option, const char *arg,
   int priority)
{
   fprintf(stdout, "Digital sound plugins:\n\n");
   plugin_manager_list_plugins(sysdep_dsp_plugin_manager, stdout);
   
   return -1;
}

/* public methods (in sysdep_dsp.h) */
int sysdep_dsp_init(struct rc_struct *rc, const char *plugin_path)
{
   if(!sysdep_dsp_rc)
   {
      if(rc && rc_register(rc, sysdep_dsp_opts))
         return -1;
      sysdep_dsp_rc = rc;
   }
   
   if(!sysdep_dsp_plugin_manager)
   {
      if(!(sysdep_dsp_plugin_manager =
         plugin_manager_create("sysdep_dsp", rc)))
      {
         sysdep_dsp_exit();
         return -1;
      }
      /* no need to fail here, if we don't have any plugins, sysdep_dsp_create
         will always fail, but that doesn't have to be fatal, failing here
         usually is! */
      plugin_manager_register(sysdep_dsp_plugin_manager, sysdep_dsp_plugins);
      plugin_manager_load(sysdep_dsp_plugin_manager, plugin_path, NULL);
      if(plugin_manager_init_plugin(sysdep_dsp_plugin_manager, NULL))
      {
         fprintf(stderr, "warning: no dsp plugins available\n");
      }
   }

   return 0;
}

void sysdep_dsp_exit(void)
{
   if(sysdep_dsp_plugin_manager)
   {
      plugin_manager_destroy(sysdep_dsp_plugin_manager);
      sysdep_dsp_plugin_manager = NULL;
   }
   if(sysdep_dsp_rc)
   {
      rc_unregister(sysdep_dsp_rc, sysdep_dsp_opts);
      sysdep_dsp_rc = NULL;
   }
}

struct sysdep_dsp_struct *sysdep_dsp_create(const char *plugin,
   const char *device, int *samplerate, int *type, float bufsize,
   int flags)
{
   struct sysdep_dsp_struct *dsp = NULL;
   struct sysdep_dsp_create_params params;
   
   /* fill the params struct */
   params.bufsize = bufsize;
   params.device = device;
   params.samplerate = *samplerate;
   params.type = *type;
   params.flags = flags;
   
   /* create the instance */
   if (!(dsp = plugin_manager_create_instance(sysdep_dsp_plugin_manager,
      plugin? plugin:sysdep_dsp_plugin, &params)))
   {
      return NULL;
   }
   
   /* calculate buf_size if not done by the plugin */
   if(!dsp->hw_info.bufsize)
      dsp->hw_info.bufsize = bufsize * dsp->hw_info.samplerate;

   /* fill in the emu info struct */
   if(flags & SYSDEP_DSP_EMULATE_TYPE)
      dsp->emu_info.type = *type;
   else
      dsp->emu_info.type = dsp->hw_info.type;
   dsp->emu_info.samplerate = dsp->hw_info.samplerate;
   dsp->emu_info.bufsize = dsp->hw_info.bufsize;
   
   /* allocate convert buffer if nescesarry */
   if(memcmp(&dsp->emu_info, &dsp->hw_info, sizeof(struct sysdep_dsp_info)))
   {
      if(!(dsp->convert_buf =
         malloc(dsp->hw_info.bufsize *
            sysdep_dsp_bytes_per_sample[dsp->hw_info.type])))
      {
         fprintf(stderr,
            "error malloc failed for dsp convert buffer\n");
         sysdep_dsp_destroy(dsp);
         return NULL;
      }
   }
   
   if(sysdep_dsp_use_timer || !dsp->get_freespace)
      fprintf(stderr, "info: dsp: using timer-based audio\n");
   
   /* return actual type and samplerate */
   *type = dsp->emu_info.type;
   *samplerate = dsp->emu_info.samplerate;
   
   return dsp;
}

void sysdep_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
   if(dsp->convert_buf)
      free(dsp->convert_buf);
   dsp->destroy(dsp);
}

int sysdep_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
   uclock_t t;
   int result;
   
   if(!sysdep_dsp_use_timer && dsp->get_freespace)
      return dsp->get_freespace(dsp);
      
   /* fall back using uclock (which uses gettimeofday where available) */
   t = uclock();
   
   if (dsp->last_update == 0)
      result = 0;
   else
      result = ((t - dsp->last_update) * dsp->emu_info.samplerate) /
         UCLOCKS_PER_SEC;
   
   dsp->last_update = t;
   
   /* sanity check */
   if ((result < 0) || (result > dsp->emu_info.bufsize))
      result = 0;
   
   return result;
}

int sysdep_dsp_write(struct sysdep_dsp_struct *dsp,
   unsigned char *data, int count)
{
   if(!count)
      return 0;

   /* do we need to emulate? */
   if(memcmp(&dsp->emu_info, &dsp->hw_info, sizeof(struct sysdep_dsp_info)))
   {
      unsigned char *convert_buf = dsp->convert_buf;
      short *convert_buf_large = (short *)dsp->convert_buf;
      short *data_large = (short *)data;
      int i;
      
      if (count > dsp->emu_info.bufsize)
         count = dsp->emu_info.bufsize;
      
      switch((dsp->emu_info.type << 4) | dsp->hw_info.type)
      {
         /* 8bit mono -> */
         case 0x01: /* 16bit mono */
            for(i = 0; i < count; i++)
               convert_buf_large[i] = ((int)data[i] << 8) - 32768;
            break;
         case 0x02: /* 8bit stereo */
            for(i = 0; i < count; i++)
            {
               convert_buf[i * 2]     = data[i];
               convert_buf[i * 2 + 1] = data[i];
            }
            break;
         case 0x03: /* 16bit stereo */
            for(i = 0; i < count; i++)
            {
               convert_buf_large[i * 2]     = ((int)data[i] << 8) - 32768;
               convert_buf_large[i * 2 + 1] = convert_buf[i * 2];
            }
            break;
            
         /* 16bit mono -> */
         case 0x10: /* 8 bit mono */
            for(i = 0; i < count; i++)
               convert_buf[i] = (data_large[i] >> 8) + 128;
            break;
         case 0x12: /* 8bit stereo */
            for(i = 0; i < count; i++)
            {
               convert_buf[i * 2]     = (data_large[i] >> 8) + 128;
               convert_buf[i * 2 + 1] = convert_buf[i * 2];
            }
            break;
         case 0x13: /* 16bit stereo */
            for(i = 0; i < count; i++)
            {
               convert_buf_large[i * 2]     = data_large[i];
               convert_buf_large[i * 2 + 1] = data_large[i];
            }
            break;
            
         /* 8bit stereo -> */
         case 0x20: /* 8 bit mono */
            for(i = 0; i < count; i++)
               convert_buf[i] = (((int)data[i * 2] + (int)data[i * 2 + 1])
                  >> 1);
            break;
         case 0x21: /* 16bit mono */
            for(i = 0; i < count; i++)
               convert_buf_large[i] = (((int)data[i * 2] + 
                  (int)data[i * 2 + 1]) << 7) - 32768;
            break;
         case 0x23: /* 16bit stereo */
            for (i = 0; i < count; i++)
            {
               convert_buf_large[i * 2    ] = ((int)data[i * 2    ] << 8)
                  - 32768;
               convert_buf_large[i * 2 + 1] = ((int)data[i * 2 + 1] << 8)
                  - 32768;
            }
            break;
            
         /* 16bit stereo -> */
         case 0x30: /* 8 bit mono */
            for(i = 0; i < count; i++)
               convert_buf[i] = (((int)data_large[i * 2] +
                  (int)data_large[i * 2 + 1]) >> 9) + 128;
            break;
         case 0x31: /* 16bit mono */
            for(i = 0; i < count; i++)
               convert_buf_large[i] = (((int)data_large[i * 2] +
                  (int)data_large[i * 2 + 1]) >> 1);
            break;
         case 0x32: /* 8bit stereo */
            for (i = 0; i < count; i++)
            {
               convert_buf[i * 2    ] = (data_large[i * 2    ] >> 8) + 128;
               convert_buf[i * 2 + 1] = (data_large[i * 2 + 1] >> 8) + 128;
            }
            break;
      }
      data = dsp->convert_buf;
   }

   return dsp->write(dsp, data, count);
}

int sysdep_dsp_get_max_freespace(struct sysdep_dsp_struct *dsp)
{
   return dsp->emu_info.bufsize;
}
