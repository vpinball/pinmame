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
#ifndef __PLUGIN_MANAGER_H
#define __PLUGIN_MANAGER_H
#include <stdio.h>
#include "rc.h"
#include "begin_code.h"

struct plugin_struct
{
   const char *name;             /* name of plugin */
   const char *type;             /* type of plugin */
   const char *description;      /* description of plugin */
   struct rc_option *opts;       /* options for this plugin */
   int (*init)(void);            /* f-ptr which inits the plugin if nescesarry
                                    */
   void (*exit)(void);           /* cleans up before unloading */
   void *(*create)(const void *flags);
                                 /* creates an instance of the object
                                    associated with the type of pluging we're
                                    managing.
                                    The plugin system has no knowledge of this
                                    object, but the object which uses the
                                    plugin system should have knowledge about
                                    it. */
   int priority;                 /* higher priority plugins are checked first
                                    when checking multiple plugins. (for
                                    auto plugin selection for example) */
};

struct plugin_manager_struct;

/* Creates a plugin manager struct, doesn't do much else */
struct plugin_manager_struct *plugin_manager_create(const char *type,
   struct rc_struct *rc);

/* Free all data, unload all plugins etc */
void plugin_manager_destroy(struct plugin_manager_struct *manager);

/* Register the NULL ptr terminated list of plugins, mainly usefull,
   to register static plugins. */
int plugin_manager_register(struct plugin_manager_struct *manager,
   const struct plugin_struct *plugin[]);

/* Unregister (and if not static unloads) the NULL ptr terminated list of
   plugins, if plugin == NULL, all plugins are unregistered */
void plugin_manager_unregister(struct plugin_manager_struct *manager,
   const struct plugin_struct *plugin[]);

/* Loads plugin(s) from:
   <path>/<name>.so
   If name == NULL all plugins in <path> are loaded.
   
   Returns:
   0 if one or more plugins we're successfully loaded -1 otherwise */
int plugin_manager_load(struct plugin_manager_struct *manager,
   const char *path, const char *name);
   
/* Unloads (if not static) and unregisters plugins matching name, if
   name == NULL, all plugins are unloaded.
   If one or more plugins where unloaded successfully 0 is returned,
   otherwise -1 is returned */
void plugin_manager_unload(struct plugin_manager_struct *manager,
   const char *name);

/* Initialises the plugin(s) matching name. If name == NULL all plugins are
   initialised starting with the ones with the highest priority.
   If one or more plugins where initialised successfully 0 is returned,
   otherwise -1 is returned */
int plugin_manager_init_plugin(struct plugin_manager_struct *manager,
   const char *name);

/* Creates an instance of the object associated with the type of plugin which
   is being managed. The instance is created using the plugin matching name,
   which will first be initialised if nescesarry. If name == NULL all plugins
   are tried untill the instance has been created, starting with the ones with
   the highest priority.
   On success 0 is returned, on failure -1 is returned */
void *plugin_manager_create_instance(struct plugin_manager_struct *manager,
   const char *name, void *flags);
   
void plugin_manager_list_plugins(struct plugin_manager_struct *manager,
   FILE *f);

#include "end_code.h"
#endif /* #ifndef __PLUGIN_MANAGER_H */
