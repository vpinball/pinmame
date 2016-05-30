/* Sysdep display object

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
#ifndef __SYSDEP_DISPLAY_PRIV_H
#define __SYSDEP_DISPLAY_PRIV_H

#include "sysdep_display.h"
#include "sysdep_palette.h"
#include "begin_code.h"

struct sysdep_display_struct {
   struct sysdep_display_mode emu_mode;
   struct sysdep_display_mode hw_mode;
   struct sysdep_display_mode *modes;
   struct sysdep_palette_info palette_info;
   float aspect_ratio;
   int is_16bpp_capable;
   int widthscale;
   int heightscale;
   int scanlines;
   int mode;
   int opened;
   int mapped;
   int startx;
   int starty;
   int framebuffer_pitch;
   unsigned char *framebuffer;
   void *_priv;
};

#include "end_code.h"
#endif /* ifndef __SYSDEP_DISPLAY_PRIV_H */
