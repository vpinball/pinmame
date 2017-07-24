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
#ifndef __SYSDEP_MIXER_PRIV_H
#define __SYSDEP_MIXER_PRIV_H
#include "sysdep_mixer.h"
#include "begin_code.h"

struct sysdep_mixer_struct {
   int flags;
   char channel_available[SYSDEP_MIXER_CHANNELS];
   char restore_channel[SYSDEP_MIXER_CHANNELS];
   int cache_left[SYSDEP_MIXER_CHANNELS];
   int cache_right[SYSDEP_MIXER_CHANNELS];
   int orig_left[SYSDEP_MIXER_CHANNELS];
   int orig_right[SYSDEP_MIXER_CHANNELS];
   void *_priv;
   int (*set)(struct sysdep_mixer_struct *mixer, int channel, int left,
      int right);
   int (*get)(struct sysdep_mixer_struct *mixer, int channel, int *left,
      int *right);
   void (*destroy)(struct sysdep_mixer_struct *mixer);
};

struct sysdep_mixer_create_params {
   const char *device;
};

#include "end_code.h"
#endif /* ifndef __SYSDEP_MIXER_PRIV_H */
