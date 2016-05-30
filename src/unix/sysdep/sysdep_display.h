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
#ifndef __SYSDEP_DISPLAY_H
#define __SYSDEP_DISPLAY_H

#include "begin_code.h"

/* sysdep_display_open params_present defines */
#define SYSDEP_DISPLAY_WIDTHSCALE	0x01
#define SYSDEP_DISPLAY_HEIGHTSCALE	0x02
#define SYSDEP_DISPLAY_SCANLINES	0x04
#define SYSDEP_DISPLAY_ASPECT_RATIO	0x08

struct sysdep_display_struct;
struct sysdep_palette_struct;
struct sysdep_palette_info;
struct sysdep_input_struct;

struct sysdep_display_open_params {
   int widthscale;
   int heightscale;
   int scanlines;
   float aspect_ratio;
};

struct sysdep_display_mode {
   int width;
   int height;
   int depth;
   float aspect_ratio;
   int _priv;
};

/* init / exit */
int sysdep_display_init(struct rc_struct *rc, const char *plugin_path);
void sysdep_display_exit(void);

/* create / destroy */
struct sysdep_display_struct *sysdep_display_create(const char *plugin);
void sysdep_display_destroy(struct sysdep_display_struct *display);

/* open / close */
int sysdep_display_open(struct sysdep_display_struct *display, int width,
   int height, int depth, struct sysdep_display_open_params *params,
   int params_present);
void sysdep_display_close(struct sysdep_display_struct *display);

/* map / unmap (set graphics mode / text mode) */
int sysdep_display_map(struct sysdep_display_struct *display);
int sysdep_display_unmap(struct sysdep_display_struct *display);

/* mode handling */
float sysdep_display_get_aspect_ratio(struct sysdep_display_struct *display);
struct sysdep_display_mode *sysdep_display_get_modes(
   struct sysdep_display_struct *display);

/* palette handling */
int sysdep_display_16bpp_capable(struct sysdep_display_struct *display);
const struct sysdep_palette_info *sysdep_display_get_palette_info(
   struct sysdep_display_struct *display);
int sysdep_display_alloc_palette(struct sysdep_display_struct *display,
   int writable_colors);
int sysdep_display_set_pen(struct sysdep_display_struct *display, int pen,
   unsigned char red, unsigned char green, unsigned char blue);

/* blit */   
int sysdep_display_blit(struct sysdep_display_struct *display,
   struct sysdep_palette_struct *palette,
   struct sysdep_bitmap *bitmap, int src_x, int src_y, int dest_x, int dest_y,
   int src_width, int src_height, int scale_x, int scale_y, int use_dirty);

/* input */ 
struct sysdep_input_struct *sysdep_display_open_input(
   struct sysdep_display_struct *display);

/* option handling / changing */
int sysdep_display_set_widthscale(struct sysdep_display_struct *display,
   int widthscale);
int sysdep_display_get_widthscale(struct sysdep_display_struct *display);

int sysdep_display_set_heightscale(struct sysdep_display_struct *display,
   int heightscale);
int sysdep_display_get_widthscale(struct sysdep_display_struct *display);
   
int sysdep_display_set_scanlines(struct sysdep_display_struct *display,
   int scanlines);
int sysdep_display_get_widthscale(struct sysdep_display_struct *display);
   
int sysdep_display_set_plugin(struct sysdep_display_struct **display,
   const char *plugin);
const char *sysdep_display_get_plugin(struct sysdep_display_struct *display);

int sysdep_display_handle_hotkeys(struct sysdep_display_struct **display,
   struct sysdep_input_struct *input);

#include "end_code.h"
#endif /* ifndef __SYSDEP_DISPLAY_H */
