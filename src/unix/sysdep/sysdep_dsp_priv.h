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
#ifndef __SYSDEP_DSP_PRIV_H
#define __SYSDEP_DSP_PRIV_H

#include "misc.h"
#include "begin_code.h"

struct sysdep_dsp_info {
   int samplerate;
   int type;
   int bufsize;
};

struct sysdep_dsp_struct {
   struct sysdep_dsp_info hw_info;
   struct sysdep_dsp_info emu_info;
   unsigned char *convert_buf;
   uclock_t last_update;
   void *_priv;
   int (*get_freespace)(struct sysdep_dsp_struct *dsp);
   int (*write)(struct sysdep_dsp_struct *dsp, unsigned char *data,
      int count);
   void (*destroy)(struct sysdep_dsp_struct *dsp);
};

struct sysdep_dsp_create_params {
   float bufsize;
   const char *device;
   int samplerate;
   int type;
   int flags;
};

#include "end_code.h"
#endif /* ifndef __SYSDEP_DSP_PRIV_H */
