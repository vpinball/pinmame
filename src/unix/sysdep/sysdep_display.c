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
/* Changelog
Version 0.1, March 2000
-initial release (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include "sysdep_display.h"
#include "sysdep_display_priv.h"
#include "sysdep_display_plugins.h"
#include "plugin_manager.h"

/* private func prototypes */
static int sysdep_display_list_plugins(struct rc_option *option,
   const char *arg, int priority);

/* private variables */
static int sysdep_display_widthscale = 1;
static int sysdep_display_heightscale = 1;
static int sysdep_display_scanlines = 0;
static float sysdep_display_hw_aspect_ratio = 1.33;
static int sysdep_display_keep_aspect = 1;
static char *sysdep_display_plugin = NULL;
static struct rc_struct *sysdep_display_rc = NULL;
static struct plugin_manager_struct *sysdep_display_plugin_manager = NULL;
static struct rc_option sysdep_display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Display related", NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "display-plugin", 	"dp",			rc_string,	&sysdep_display_plugin,
     NULL,		0,			0,		NULL,
     "Select which plugin to use for the display" },
   { "list-display-plugins",  "ldisp",		rc_use_function_no_arg, NULL,
     NULL,		0,			0,		sysdep_display_list_plugins,
     "List available display plugins" },
   { "widthscale",	"ws",			rc_int,		&sysdep_display_widthscale,
     "1",		1,			10,		NULL,
     "Select the X scale factor" },
   { "heightscale",	"hs",			rc_int,		&sysdep_display_heightscale,
     "1",		1,			10,		NULL,
     "Select the Y scale factor" },
   { "scanlines",       "sl",			rc_bool,	&sysdep_display_scanlines,
     "0",		0,			0,		NULL,
     "Emulate / don't emulates scanlines when scaling allong the Y axis" },
   { "display-aspect-ratio", "dar",		rc_float,	&sysdep_display_hw_aspect_ratio,
     "1.33",		0.5,			2.0,		NULL,
     "Set the aspect ratio of your monitor, this is used for mode selection "
     "calculations. Usually this is 4/3 (1.33) in some cases this is 3/4 "
     "(0.75) or even 16/9 (1.77)" },
   { "keep-aspect",     "ka",			rc_bool,	&sysdep_display_keep_aspect,
     "1",		0,			0,		NULL,
     "Try / don't try to keep the correct aspect ratio when selecting the best mode" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};
static const struct plugin_struct *sysdep_display_plugins[] = {
#ifdef SYSDEP_DISPLAY_X11
   &sysdep_display_x11,
#endif
#ifdef SYSDEP_DISPLAY_DGA
   &sysdep_display_dga,
#endif
#ifdef SYSDEP_DISPLAY_SVGALIB
   &sysdep_display_svgalib,
#endif
   NULL
};

/* private methods */
static int sysdep_display_list_plugins(struct rc_option *option,
   const char *arg, int priority)
{
   fprintf(stdout, "Display plugins:\n\n");
   plugin_manager_list_plugins(sysdep_display_plugin_manager, stdout);
   
   return -1;
}

static int sysdep_display_match_mode(struct sysdep_display_struct
         *display, int mode)
{
   int ideal_width  = display->emu_mode.width  * display->widthscale;
   int ideal_height = display->emu_mode.height * display->heightscale;
   
   /* does it fit at all ? */
   if((display->modes[mode].width  < ideal_width) ||
      (display->modes[mode].height < ideal_height) ||
      (display->modes[mode].depth  < display->emu_mode.depth))
      return 0;
   
   if(sysdep_display_keep_aspect)
   {
      float aspect_ratio_ratio = (display->aspect_ratio *
         display->modes[mode].aspect_ratio) / display->emu_mode.aspect_ratio;

      /* keep height? */
      if(aspect_ratio_ratio >= 1.0)
         ideal_width  *= aspect_ratio_ratio;
      else
         ideal_height /= aspect_ratio_ratio;
   }

   return ( 100 *
      ((float)ideal_width  /
         (abs(display->modes[mode].width  - ideal_width ) + ideal_width )) *
      ((float)ideal_height /
         (abs(display->modes[mode].height - ideal_height) + ideal_height)) *
      ((float)display->emu_mode.depth / display->modes[mode].depth));
}

/* public methods (in sysdep_display.h) */
int sysdep_display_init(struct rc_struct *rc, const char *plugin_path)
{
   if(!sysdep_display_rc)
   {
      if(rc && rc_register(rc, sysdep_display_opts))
         return -1;
      sysdep_display_rc = rc;
   }
   
   if(!sysdep_display_plugin_mananger)
   {
      if(!(sysdep_display_plugin_manager =
         plugin_manager_create("sysdep_display", rc)))
      {
         sysdep_display_exit();
         return -1;
      }
      /* no need to fail here, if we don't have any plugins,
         sysdep_display_create will always fail, but that doesn't have to be
         fatal, failing here usually is! */
      plugin_manager_register(sysdep_display_plugin_manager,
         sysdep_display_plugins);
      plugin_manager_load(sysdep_display_plugin_manager, plugin_path, NULL);
      if (plugin_manager_init_plugin(sysdep_display_plugin_manager, NULL))
      {
         fprintf(stderr, "warning: no display plugins available\n");
      }
   }

   return 0;
}

void sysdep_display_exit(void)
{
   if(sysdep_display_plugin_manager)
   {
      plugin_manager_destroy(sysdep_display_plugin_manager);
      sysdep_display_plugin_manager = NULL;
   }
   if(sysdep_display_rc)
   {
      rc_unregister(sysdep_display_rc, sysdep_display_opts);
      sysdep_display_rc = NULL;
   }
}


struct sysdep_display_struct *sysdep_display_create(const char *plugin)
{
   struct sysdep_display_struct *display = NULL;
   
   /* create the instance */
   if (!(display = plugin_manager_create_instance(sysdep_display_plugin_manager,
      plugin? plugin:sysdep_display_plugin, NULL)))
   {
      return NULL;
   }
   
   /* if the plugin couldn't give us an aspect ratio then use the user
      defined aspect ratio */
   if(!display->aspect_ratio)
      display->aspect_ratio = sysdep_display_aspect_ratio;
      
   /* set the mode to -1 */
   display->mode = -1;
   
   return display;
}

void sysdep_display_destroy(struct sysdep_display_struct *display)
{
   if(display->opened)
      display->close(display);
   
   display->destroy(display);
}


int sysdep_display_open(struct sysdep_display_struct *display, int width,
   int height, int depth, struct sysdep_display_open_params *params,
   int params_present)
{
   /* are we already open ? */
   if(display->opened)
      return -1;
   
   /* get all the params */
   display->emu_mode.width = width;
   display->emu_mode.height = height;
   display->emu_mode.depth = depth;
   
   if(params_present & SYSDEP_DISPLAY_ASPECT_RATIO)
      display->emu_mode.aspect_ratio = params->aspect_ratio;
   else
      display->emu_mode.aspect_ratio = display->aspect_ratio;
      
   if(params_present & SYSDEP_DISPLAY_WIDTHSCALE)
      display->widthscale = params->widthscale;
   else
      display->widthscale = sysdep_display_widthscale;
   
   if(params_present & SYSDEP_DISPLAY_HEIGHTSCALE)
      display->heightscale = params->heightscale;
   else
      display->heightscale = sysdep_display_heightscale;
      
   if(params_present & SYSDEP_DISPLAY_SCANLINES)
      display->scanlines = params->scanlines;
   else
      display->scanlines = sysdep_display_scanlines;
      
   /* if this plugin uses modes, then select the best mode */
   if(display->modes)
   {
      int best_score = 0;
      
      display->mode = 0;
      best_score = sysdep_display_match_mode(display, 0);
      
      for(i = 1; modes[i].width; i++)
      {
         int score = sysdep_display_match_mode(display, i);
         if(score >= best_score)
         {
            best_score = score;
            display->mode = i;
         }
      }
      display->hw_mode = display->modes[display->mode];
      display->startx = ((display->emu_mode.width  * display->widthscale) -
         display->hw_mode.width ) / 2;
      display->starty = ((display->emu_mode.heigth * display->heightscale) -
         display->hw_mode.height) / 2;
   }
   else
      display->hw_mode = display->emu_mode;
   
   if(display->open(display))
   {
      sysdep_display_close(display);
      return -1;
   }
      
   display->opened = 1;
   
   return 0;
}

void sysdep_display_close(struct sysdep_display_struct *display)
{
  /* Save these 3 so that they can be restored after clearing the
     display struct. */
  struct sysdep_display_mode *modes = display->modes;
  float aspect_ratio = display->aspect_ratio;
  void *_priv = display->_priv;
  
  /* close the display */
  if(display->opened)
     display->close(display);
  
  /* clear the display struct */
  memset(display, 0, sizeof(struct sysdep_display_struct));
  
  /* and restore the vital data */
  display->modes = modes;
  display->aspect_ratio = aspect_ratio;
  display->_priv = _priv;
  display->mode = -1;
}


/* map / unmap (set graphics mode / text mode) */
int sysdep_display_map(struct sysdep_display_struct *display)
{
}

int sysdep_display_unmap(struct sysdep_display_struct *display)
{
}


/* mode handling */
float sysdep_display_get_aspect_ratio(struct sysdep_display_struct *display)
{
   return display->aspect_ratio;
}

const struct sysdep_display_mode *sysdep_display_get_modes(
   struct sysdep_display_struct *display)
{
   static const struct sysdep_display_mode no_modes[2] = {
      { -1, -1, -1, 0, 0 },
         0,  0,  0, 0, 0 }
      };
      
   /* does this plugin use modes? */
   if (display->modes)
      return display->modes;
   else
      return no_modes;
}


/* palette handling */
int sysdep_display_16bpp_capable(struct sysdep_display_struct *display)
{
   return display->is_16bpp_capable;
}

const struct sysdep_palette_info *sysdep_display_get_palette_info(
   struct sysdep_display_struct *display)
{
   return &(display->palette_info);
}

int sysdep_display_alloc_palette(struct sysdep_display_struct *display,
   int writable_colors)
{
   if(display->alloc_palette)
      return display->alloc_palette(display, writable_colors);
      
   return 0;
}

int sysdep_display_set_pen(struct sysdep_display_struct *display, int pen,
   unsigned char red, unsigned char green, unsigned char blue)
{
   if(display->set_pen)
      return display->set_pen(display, pen, red, green, blue);
   
   return -1;
}

   
int sysdep_display_blit(struct sysdep_display_struct *display,
   struct sysdep_bitmap *bitmap, int src_x, int src_y, int dest_x, int dest_y,
   int src_width, int src_height, int scale_x, int scale_y, int use_dirty)
{
   /* adjust scale_xx and dest_xx for display scaling and panning */
   scale_x *= display->widthscale;
   scale_y *= display->heightscale;
   dest_x  *= display->widthscale;
   dest_y  *= display->heightscale;
   dest_x  += display->startx;
   dest_y  += display->starty;
   
   /* 4 different scenarios:
      -The display plugin has it's own blit function, and we use that.
      -The display plugin has it's own blit function, but the display
       types (the display palette_info structs) don't match, or scaling is
       requested, so we first blit to a dummy framebuffer, and then call the
       system's blit function to update the real framebuffer.
      -the display plugin gives us direct access to its framebuffer, so
       we use our own blit code to blit to this.
      -the display plugin gives us direct access to its framebuffer, so
       we blit to it, and wants us to call a function afterwards to notify it
       about the updating of his framebuffer.
   */
   
   questions:
   -do we solve this by writing seperate code 4 the 4 blit scenarios, or do
    we introduce's if statements for this in the blit code.
   -how about special cases like doublebuffering no double buffering etc.
}


struct sysdep_input_struct *sysdep_display_open_input(
   struct sysdep_display_struct *display)
{
}


int sysdep_display_set_widthscale(struct sysdep_display_struct **display,
   int widthscale)
{
}
int sysdep_display_get_widthscale(struct sysdep_display_struct *display)
{
}

int sysdep_display_set_heightscale(struct sysdep_display_struct **display,
   int heightscale)
{
}
int sysdep_display_get_widthscale(struct sysdep_display_struct *display)
{
}
   
int sysdep_display_set_scanlines(struct sysdep_display_struct *display,
   int scanlines)
{
}
int sysdep_display_get_widthscale(struct sysdep_display_struct *display)
{
}
   
int sysdep_display_set_plugin(struct sysdep_display_struct **display,
   const char *plugin)
{
}
const char *sysdep_display_get_plugin(struct sysdep_display_struct *display)
{
}

int sysdep_display_handle_hotkeys(struct sysdep_display_struct **display,
   struct sysdep_input_struct *input)
{
}

