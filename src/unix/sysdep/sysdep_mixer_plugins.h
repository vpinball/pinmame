/* Sysdep sound mixer plugins listing
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
#ifndef __SYSDEP_MIXER_PLUGINS_H
#define __SYSDEP_MIXER_PLUGINS_H
#include "plugin_manager.h"
#include "begin_code.h"

#ifdef SYSDEP_MIXER_OSS
extern struct plugin_struct sysdep_mixer_oss;
#endif
#ifdef SYSDEP_MIXER_NETBSD
extern struct plugin_struct sysdep_mixer_netbsd;
#endif
#ifdef SYSDEP_MIXER_SOLARIS
extern struct plugin_struct sysdep_mixer_solaris;
#endif
#ifdef SYSDEP_MIXER_NEXT
extern struct plugin_struct sysdep_mixer_next;
#endif
#ifdef SYSDEP_MIXER_IRIX
extern struct plugin_struct sysdep_mixer_irix;
#endif
#ifdef SYSDEP_MIXER_AIX
extern struct plugin_struct sysdep_mixer_aix;
#endif

#include "end_code.h"
#endif /* ifndef __SYSDEP_MIXER_PLUGINS_H */
