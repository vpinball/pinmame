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
#ifndef __SYSDEP_MIXER_H
#define __SYSDEP_MIXER_H

#include "rc.h"
#include "begin_code.h"

/* channel defines */
#define SYSDEP_MIXER_VOLUME	0
#define SYSDEP_MIXER_PCM1	1
#define SYSDEP_MIXER_PCM2	2
#define SYSDEP_MIXER_SYNTH	3
#define SYSDEP_MIXER_CD		4
#define SYSDEP_MIXER_LINE1	5
#define SYSDEP_MIXER_LINE2	6
#define SYSDEP_MIXER_LINE3	7
#define SYSDEP_MIXER_BASS	8
#define SYSDEP_MIXER_TREBLE	9
#define SYSDEP_MIXER_CHANNELS	10
#define SYSDEP_MIXER_NAMES { "Volume", "PCM 1", "PCM 2", "Synth", \
   "CD", "Line 1", "Line 2", "Line 3", "Bass", "Treble" }

/* flags for sysdep_mixer_create */
#define SYSDEP_MIXER_RESTORE_SETTINS_ON_EXIT 0x01

struct sysdep_mixer_struct;

int sysdep_mixer_init(struct rc_struct *rc, const char *plugin_path);
void sysdep_mixer_exit(void);

struct sysdep_mixer_struct *sysdep_mixer_create(const char *plugin,
   const char *device, int flags);
void sysdep_mixer_destroy(struct sysdep_mixer_struct *dsp);

int sysdep_mixer_channel_available(struct sysdep_mixer_struct *mixer,
   int channel);

int sysdep_mixer_set(struct sysdep_mixer_struct *mixer, int channel,
   int left, int right);
int sysdep_mixer_get(struct sysdep_mixer_struct *mixer, int channel,
   int *left, int *right);

#include "end_code.h"
#endif /* ifndef __SYSDEP_MIXER_H */
