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
#ifndef __PLUGIN_MANAGER_PRIV_H
#define __PLUGIN_MANAGER_PRIV_H

#include "begin_code.h"

struct plugin_manager_data {
   const struct plugin_struct *plugin; 
                                 /* the plugin_plugin struct describing
                                    the plugin, or NULL for a removed plugin */
   void *plugin_handle;          /* void * for the dynamic linker handles,
                                    or NULL for static / unloaded plugins */
   int initialised;              /* is the plugin checked? (plugins
                                    which fail there check are removed from
                                    the list */
};

struct plugin_manager_struct
{
   const char *type;             /* type of the plugins which are
                                    managed by this instance of the plugin
                                    manager */
   struct rc_struct *rc;         /* rc object where the options for
                                    added plugins should be registered */
   struct plugin_manager_data *data;
                                 /* array of data for the loaded plugins */
   int data_size;                /* size of this array, to know when it
                                    should be reallocated */
   int highest_priority;         /* keeps count of the highest priority plugin
                                    added */
};

#include "end_code.h"
#endif /* #ifndef __PLUGIN_MANAGER_PRIV_H */
