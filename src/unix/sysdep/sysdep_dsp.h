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
#ifndef __SYSDEP_DSP_H
#define __SYSDEP_DSP_H

#include "rc.h"
#include "begin_code.h"

#define SYSDEP_DSP_BYTES_PER_SAMPLE { 1, 2, 2, 4 }

/* valid flags for type */
#define SYSDEP_DSP_8BIT   0x00
#define SYSDEP_DSP_16BIT  0x01
#define SYSDEP_DSP_MONO   0x00
#define SYSDEP_DSP_STEREO 0x02

/* valid flags for sysdep_dsp_create */
#define SYSDEP_DSP_EMULATE_TYPE 0x01
/* TODO: implement SYSDEP_DSP_EMULATE_SAMPLERATE */
/* #define SYSDEP_DSP_EMULATE_SAMPLERATE 0x02 */
#define SYSDEP_DSP_O_NONBLOCK 0x04

struct sysdep_dsp_struct;

int sysdep_dsp_init(struct rc_struct *rc, const char *plugin_path);
void sysdep_dsp_exit(void);

struct sysdep_dsp_struct *sysdep_dsp_create(const char *plugin,
   const char *device, int *samplerate, int *type, float bufsize, int flags);
void sysdep_dsp_destroy(struct sysdep_dsp_struct *dsp);

int sysdep_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
int sysdep_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
   int count);
int sysdep_dsp_get_max_freespace(struct sysdep_dsp_struct *dsp);

#include "end_code.h"
#endif /* ifndef __SYSDEP_DSP_H */
