/* Sysdep sound dsp plugins listing
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
#ifndef __SYSDEP_DSP_PLUGINS_H
#define __SYSDEP_DSP_PLUGINS_H
#include "plugin_manager.h"
#include "begin_code.h"

#ifdef SYSDEP_DSP_OSS
extern struct plugin_struct sysdep_dsp_oss;
#endif
#ifdef SYSDEP_DSP_NETBSD
extern struct plugin_struct sysdep_dsp_netbsd;
#endif
#ifdef SYSDEP_DSP_SOLARIS
extern struct plugin_struct sysdep_dsp_solaris;
#endif
#ifdef SYSDEP_DSP_SOUNDKIT
extern struct plugin_struct sysdep_dsp_soundkit;
#endif
#ifdef SYSDEP_DSP_COREAUDIO
extern struct plugin_struct sysdep_dsp_coreaudio;
#endif
#ifdef SYSDEP_DSP_IRIX
extern struct plugin_struct sysdep_dsp_irix;
#endif
#ifdef SYSDEP_DSP_AIX
extern struct plugin_struct sysdep_dsp_aix;
#endif
#ifdef SYSDEP_DSP_ESOUND
extern struct plugin_struct sysdep_dsp_esound;
#endif
#ifdef SYSDEP_DSP_ALSA
extern struct plugin_struct sysdep_dsp_alsa;
#endif
#ifdef SYSDEP_DSP_ARTS_TEIRA
extern struct plugin_struct sysdep_dsp_arts;
#endif
#ifdef SYSDEP_DSP_ARTS_SMOTEK
extern struct plugin_struct sysdep_dsp_arts;
#endif
#ifdef SYSDEP_DSP_SDL
extern struct plugin_struct sysdep_dsp_sdl;
#endif
#ifdef SYSDEP_DSP_WAVEOUT
extern struct plugin_struct sysdep_dsp_waveout;
#endif

#include "end_code.h"
#endif /* ifndef __SYSDEP_DSP_PLUGINS_H */
