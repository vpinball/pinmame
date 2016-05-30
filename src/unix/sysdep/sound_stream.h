/* Sound stream object

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
#ifndef __SOUND_STREAM_H
#define __SOUND_STREAM_H

#include "sysdep_dsp.h"
#include "begin_code.h"

struct sound_stream_struct;

struct sound_stream_struct *sound_stream_create(struct sysdep_dsp_struct* dsp,
   int type, int buf_size, int buf_count);
void sound_stream_destroy(struct sound_stream_struct *stream);

void sound_stream_write(struct sound_stream_struct *stream,
   unsigned char *data, int samples);

void sound_stream_update(struct sound_stream_struct *stream);

#include "end_code.h"
#endif /* ifndef __SOUND_STREAM_H */
