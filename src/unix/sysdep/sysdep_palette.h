/* Sysdep palette abstraction and emulation object

   Copyright 1999,2000 Hans de Goede
   
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
#ifndef __SYSDEP_PALETTE_H
#define __SYSDEP_PALETTE_H

#include "begin_code.h"

/* This struct is used to describe both the emulated palette and the
   display's palette.
   
   The following modes are considered valid for the display's palette
   (depth, writable_colors) :
   8,  256
   16, 0
   24, 0
   32, 0
   
   The following modes can be emulated:
   8,  1-256
   16, 0
   16, 1-65536
   
   Using this code with a display with any other palette, or requesting
   any other palette to be emulated is unsupported, and may result
   in undefined behaviour.
*/
struct sysdep_palette_info
{
   int writable_colors; /* 0 for truecolor, the number of writable colors
                           for psuedo color */
   int depth;           /* pixel size (not colordepth!) in bpp (8,16,24,32) */
   int red_shift;       /* shifts and masks to calculate true_color palette */
   int green_shift;     /* entries */
   int blue_shift;
   int red_mask;
   int green_mask;
   int blue_mask;
};

struct sysdep_palette_struct
{
   struct sysdep_palette_info emulated;
   int dirty;             /* Used by the Xv patch for updating YUV lookup */
   int *lookup;           /* lookup table to be used for blitters to convert
                             the emulated palette to the physical palette */
};

/* This function creates a sysdep palette object for the current
   display, which can be used with the display update functios.

   Parameters:
   depth           Color depth of the palette to be emulated valid values:
                   8 or 16.
   writable_colors The number of writable colors you want or 0 if you want
                   true color. Valid values for depth == 8: 1-256, for
                   depth == 16: 0-65536.
                   
   Return value:
   A pointer to the sysdep palette object, or NULL on failure.
   Upon failure an error message wil be printed to stderr.
*/
struct sysdep_palette_struct *sysdep_palette_create(int depth,
   int writable_colors);
   
/* destructor */
void sysdep_palette_destroy(struct sysdep_palette_struct *palette);

/* for pseudo color modes */   
int sysdep_palette_set_pen(struct sysdep_palette_struct *palette, int pen,
   unsigned char red, unsigned char green, unsigned char blue);
   
/* for true color modes */   
int sysdep_palette_make_pen(struct sysdep_palette_struct *palette,
   unsigned char red, unsigned char green, unsigned char blue);

/* This function has to be called if the display is changed, it recreates
   the palette object with the settings from the new display */
int sysdep_palette_change_display(struct sysdep_palette_struct **palette);

/* Added by AMR for Xv patch - used for updating YUV palette */
void sysdep_palette_mark_dirty(struct sysdep_palette_struct *palette);

void sysdep_palette_clear_dirty(struct sysdep_palette_struct *palette);

#include "end_code.h"
#endif /* ifndef __SYSDEP_PALETTE_H */
