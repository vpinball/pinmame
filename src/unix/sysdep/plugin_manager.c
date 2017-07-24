/* A simple plugin manager

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
#include <stdlib.h>
#include <string.h>
#include "plugin_manager.h"
#include "plugin_manager_priv.h"
#include "misc.h"

/* private methods */
static int plugin_manager_add(struct plugin_manager_struct *manager,
   struct plugin_manager_data *data)
{
   int i;
   
   /* does the plugin have the correct type? */
   if(strcmp(data->plugin->type, manager->type))
   {
      fprintf(stderr,
         "error: trying to add \"%s\" type plugin, to \"%s\" type plugin manager\n",
         data->plugin->type, manager->type);
      return -1;
   }
   
   for(i=0; i < manager->data_size; i++)
      if(!manager->data[i].plugin)
         break;
         
   if(i == manager->data_size)
   {
      struct plugin_manager_data *tmp;
      
      if(!(tmp=realloc(manager->data, (manager->data_size+BUF_SIZE) *
         sizeof(struct plugin_manager_data))))
      {
         fprintf(stderr, "error reallocating plugin data\n");
         return -1;
      }
      manager->data = tmp;
      memset(manager->data + manager->data_size, 0,
         sizeof(struct plugin_manager_data) * BUF_SIZE);
      manager->data_size += BUF_SIZE;
   }
   
   if (manager->rc && data->plugin->opts &&
      rc_register(manager->rc, data->plugin->opts))
      return -1;
   
   if (data->plugin->priority > manager->highest_priority)
      manager->highest_priority = data->plugin->priority;
   
   manager->data[i] = *data;

   return 0;
}

static void plugin_manager_remove(struct plugin_manager_struct *manager,
   int id)
{
   if(manager->rc && manager->data[id].plugin->opts)
      rc_unregister(manager->rc, manager->data[id].plugin->opts);
      
   if(manager->data[id].initialised && manager->data[id].plugin->exit)
      manager->data[id].plugin->exit();
   
   if(manager->data[id].plugin_handle)
   {
      /* TODO implement dlclose */
   }
   memset(manager->data + id, 0, sizeof(struct plugin_manager_data));
}

/* public methods (in plugin_manager.h) */
struct plugin_manager_struct *plugin_manager_create(const char *type,
   struct rc_struct *rc)
{
   struct plugin_manager_struct *manager = NULL;
   
   /* allocate the plugin_manager struct */
   if (!(manager = calloc(1, sizeof(struct plugin_manager_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct plugin_manager_struct\n");
      return NULL;
   }
   manager->type = type;
   manager->rc   = rc;
   return manager;
}

void plugin_manager_destroy(struct plugin_manager_struct *manager)
{
   plugin_manager_unload(manager, NULL);
   free(manager);
}

int plugin_manager_register(struct plugin_manager_struct *manager,
   const struct plugin_struct *plugin[])
{
   int i;
   struct plugin_manager_data data = { NULL, NULL, 0 };
   
   for(i=0; plugin[i]; i++)
   {
      data.plugin = plugin[i];
      if(plugin_manager_add(manager, &data))
         return -1;
   }
   return 0;
}

void plugin_manager_unregister(struct plugin_manager_struct *manager,
   const struct plugin_struct *plugin[])
{
   int i, j;
   
   /* remove all */
   if (!plugin)
   {
      plugin_manager_unload(manager, NULL);
      return;
   }
      
   for(i=0; plugin[i]; i++)
   {
      for(j=0; j < manager->data_size; j++)
      {
         if(manager->data[j].plugin == plugin[i])
         {
            plugin_manager_remove(manager, j);
            break;
         }
      }
   }
}

int plugin_manager_load(struct plugin_manager_struct *manager,
   const char *path, const char *name)
{
   /* TODO: implement me */
   return 0;
}

void plugin_manager_unload(struct plugin_manager_struct *manager,
   const char *name)
{
   int i;
   
   for(i=0; i < manager->data_size; i++)
   {
      if(manager->data[i].plugin &&
         (!name || !strcmp(name, manager->data[i].plugin->name)))
      {
         plugin_manager_remove(manager, i);
      }
   }
}

static void *plugin_manager_init_and_or_create(
   struct plugin_manager_struct *manager, const char *name, int load,
   void *flags)
{
   int i, priority;
   void *result = NULL;

   for(priority=manager->highest_priority; priority>=0; priority--)
   {
      for(i=0; i<manager->data_size; i++)
      {
         if(manager->data[i].plugin &&
            (manager->data[i].plugin->priority == priority) &&
            (!name || !strcmp(name, manager->data[i].plugin->name)) )
         {
            if(!manager->data[i].initialised &&
               manager->data[i].plugin->init &&
               manager->data[i].plugin->init())
            {  /* init failed */
               plugin_manager_remove(manager, i);
               continue;
            }
            /* no init needed, or init successfull */
            manager->data[i].initialised = 1;
            if (load)
            {
               if((result = manager->data[i].plugin->create(flags)))
               {
                  /* if no name was given tell them which plugin we're
                     using */
                  if(!name)
                     fprintf(stderr, "info: %s: using %s plugin\n",
                        manager->type, manager->data[i].plugin->name);
                  return result;
               }
            }
            else
               result = (void *)-1;
         }
      }
   }
   return result;
}

int plugin_manager_init_plugin(struct plugin_manager_struct *manager,
   const char *name)
{
   if(!plugin_manager_init_and_or_create(manager, name, 0, NULL))
      return -1;

   return 0;
}

void *plugin_manager_create_instance(struct plugin_manager_struct *manager,
   const char *name, void *flags)
{
   return plugin_manager_init_and_or_create(manager, name, 1, flags);
}

void plugin_manager_list_plugins(struct plugin_manager_struct *manager,
   FILE *f)
{
   int i;
   
   for(i=0; i < manager->data_size; i++)
      if(manager->data[i].plugin)
         fprint_columns(f, manager->data[i].plugin->name,
            manager->data[i].plugin->description);
}
