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
#ifndef __SOUND_STREAM_PRIV_H
#define __SOUND_STREAM_PRIV_H
#include "sysdep_dsp.h"
#include "begin_code.h"

struct sample_buf_fifo_struct;

struct sound_stream_sample_buf {
   int length;
   int pos;
   unsigned char *data;
};

struct sound_stream_struct {
   struct sysdep_dsp_struct *dsp;
   int bytes_per_sample;
   int sample_buf_size;
   int sample_buf_count;
   int output_buf_size;
   unsigned char *output_buf;
   struct sound_stream_sample_buf *sample_buf;
   struct sample_buf_fifo_struct *sample_buf_fifo;
   struct sample_buf_fifo_struct *empty_sample_buf_fifo;
};

#include "end_code.h"
#endif /* ifndef __SOUND_STREAM_PRIV_H */
