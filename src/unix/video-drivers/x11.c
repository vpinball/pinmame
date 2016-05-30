/*
 * X-Mame video specifics code
 *
 */
#ifdef x11
#define __X11_C_

/*
 * Include files.
 */

#include <math.h>
#include <X11/Xlib.h>
#include "xmame.h"
#include "x11.h"
#include "input.h"
#include "keyboard.h"

#ifdef USE_HWSCALE
long hwscale_redmask;
long hwscale_greenmask;
long hwscale_bluemask;
#endif

extern int force_dirty_palette;

int x11_grab_keyboard;

struct rc_option display_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11 Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "x11-mode", "x11", rc_int, &x11_video_mode, "0", 0, X11_MODE_COUNT-1, NULL, "Select x11 video mode: (if compiled in)\n0 Normal windowed (hotkey left-alt + insert)\n1 DGA fullscreen (hotkey left-alt + home)\n2 Xv windowed\n3 Xv fullscreen" },
	{ NULL, NULL, rc_link, x11_window_opts, NULL, 0, 0, NULL, NULL },
#if defined DGA || defined USE_HWSCALE
	{ NULL, NULL, rc_link, mode_opts, NULL, 0, 0, NULL, NULL },
#endif
	{ NULL, NULL, rc_link, x11_input_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

struct x_func_struct {
	int  (*init)(void);
	int  (*create_display)(int depth);
	void (*close_display)(void);
	void (*update_display)(struct mame_bitmap *bitmap);
	int  (*alloc_palette)(int writable_colors);
	int  (*modify_pen)(int pen, unsigned char red, unsigned char green, unsigned char blue);
	int  (*_16bpp_capable)(void);
};

static struct x_func_struct x_func[X11_MODE_COUNT] = {
{ NULL,
  x11_window_create_display,
  x11_window_close_display,
  x11_window_update_display,
  x11_window_alloc_palette,
  x11_window_modify_pen,
  x11_window_16bpp_capable },
#ifdef USE_DGA
{ xf86_dga_init,
  xf86_dga_create_display,
  xf86_dga_close_display,
  xf86_dga_update_display,
  xf86_dga_alloc_palette,
  xf86_dga_modify_pen,
  xf86_dga_16bpp_capable },
#else
{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
#endif
#ifdef USE_XV
{ NULL,
  x11_window_create_display,
  x11_window_close_display,
  x11_window_update_display,
  x11_window_alloc_palette,
  x11_window_modify_pen,
  x11_window_16bpp_capable },
{ NULL,
  x11_window_create_display,
  x11_window_close_display,
  x11_window_update_display,
  x11_window_alloc_palette,
  x11_window_modify_pen,
  x11_window_16bpp_capable }
#else
{ NULL, NULL, NULL, NULL, NULL, NULL, NULL },
{ NULL, NULL, NULL, NULL, NULL, NULL, NULL }
#endif
};

int sysdep_init (void)
{
	int i;

	if(!(display = XOpenDisplay (NULL)))
	{
		/* Don't use stderr_file here it isn't assigned a value yet ! */
		fprintf (stderr, "Could not open display\n");
		return OSD_NOT_OK;
	}

	for (i=0;i<X11_MODE_COUNT;i++)
	{
		if(x_func[i].create_display)
			mode_available[i] = TRUE;
		else
			mode_available[i] = FALSE;

		if(x_func[i].init && (*x_func[i].init)() != OSD_OK)
			return OSD_NOT_OK;
	}

	return OSD_OK;
}

void sysdep_close(void)
{
	if(display)
		XCloseDisplay (display);
}

int sysdep_display_16bpp_capable(void)
{
	if (x11_video_mode >= X11_MODE_COUNT)
	{
		fprintf (stderr_file,
				"X11-mode %d does not exist, falling back to normal window code\n",
				x11_video_mode);
		x11_video_mode = X11_WINDOW;
	}

	if (!mode_available[x11_video_mode])
	{
		fprintf (stderr_file,
				"X11-mode %d not available, falling back to normal window code\n",
				x11_video_mode);
		x11_video_mode = X11_WINDOW;
	}

	return (*x_func[x11_video_mode]._16bpp_capable) ();
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display (int depth)
{
	return (*x_func[x11_video_mode].create_display)(depth);
}

void sysdep_display_close(void)
{
#ifdef AVICAPTURE
	/// done_dumper ();
#endif
	(*x_func[x11_video_mode].close_display)();
}

int x11_init_palette_info(void)
{
	memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));

	display_palette_info.depth = depth;

	if (depth == 8)
	{
		if (xvisual->class != PseudoColor)
		{
			fprintf(stderr_file, "X11: Error 8 bpp only supported on PseudoColor visuals\n");
			return OSD_NOT_OK;
		}
		display_palette_info.writable_colors = 256;
	}
	else
	{
		if (xvisual->class != TrueColor)
		{
			fprintf(stderr_file, "X11: Error: %d bpp modes only supported on TrueColor visuals\n",
					depth);
			return OSD_NOT_OK;
		}
#ifdef USE_HWSCALE
		if(use_hwscale)
		{
			display_palette_info.red_mask   = hwscale_redmask;
			display_palette_info.green_mask = hwscale_greenmask;
			display_palette_info.blue_mask  = hwscale_bluemask;
		}
		else
#endif
		{
			display_palette_info.red_mask   = xvisual->red_mask;
			display_palette_info.green_mask = xvisual->green_mask;
			display_palette_info.blue_mask  = xvisual->blue_mask;
		}
	}
	return OSD_OK;
}

int sysdep_display_alloc_palette (int writable_colors)
{
	return (*x_func[x11_video_mode].alloc_palette) (writable_colors);
}

int sysdep_display_set_pen (int pen, unsigned char red, unsigned char green,
		unsigned char blue)
{
	return (*x_func[x11_video_mode].modify_pen) (pen, red, green, blue);
}

void sysdep_update_display (struct mame_bitmap *bitmap)
{
	int new_video_mode = x11_video_mode;
	int current_palette_normal = (current_palette == normal_palette);

	int bitmap_depth = bitmap->depth;
	if (bitmap_depth == 15)
	{
		bitmap_depth = 16;
	}

	if (keyboard_pressed (KEYCODE_LALT))
	{
		if (keyboard_pressed_memory(KEYCODE_INSERT))
			new_video_mode = X11_WINDOW;

		if (keyboard_pressed_memory(KEYCODE_HOME))
			new_video_mode = X11_DGA;

		if (keyboard_pressed_memory(KEYCODE_DEL))
			new_video_mode = X11_XV_WINDOW;

		if (keyboard_pressed_memory(KEYCODE_END))
			new_video_mode = X11_XV_FULLSCREEN;
	}

	if (new_video_mode != x11_video_mode && mode_available[new_video_mode])
	{
		int old_video_mode = x11_video_mode;	
		x11_video_mode = new_video_mode;

		/* Close sound, my guess is DGA somehow (perhaps fork/exec?) makes
		   the filehandle open twice, so closing it here and re-openeing after
		   the transition should fix that.  Fixed it for me anyways.
		   -- Steve bpk@hoopajoo.net */
		osd_sound_enable( 0 );

		(*x_func[old_video_mode].close_display)();
		if ((*x_func[x11_video_mode].create_display)(bitmap_depth) != OSD_OK)
		{
			fprintf(stderr_file,
					"X11: Warning: Couldn't create display for new x11-mode\n"
					"   Trying again with the old x11-mode\n");
			(*x_func[x11_video_mode].close_display)();
			if ((*x_func[old_video_mode].create_display)(bitmap_depth) != OSD_OK)
				goto barf;
			else
				x11_video_mode = old_video_mode;
		}

		if (sysdep_palette_change_display(&normal_palette))
			goto barf;

		if (debug_palette && sysdep_palette_change_display(&debug_palette))
			goto barf;

		if (current_palette_normal)
			current_palette = normal_palette;
		else
			current_palette = debug_palette;

		if (sysdep_display_alloc_palette(video_colors_used))
			goto barf;

		/* Force the palette lookup table to be updated.  This is necessary 
		 * because switching to/from fullscreen mode could cause the screen 
		 * depth to change. */
		force_dirty_palette = 1;  

		xmame_keyboard_clear();
		/* poll mouse twice to clear internal vars */
		if (use_mouse)
		{
			sysdep_mouse_poll ();
			sysdep_mouse_poll ();
		}

		/* Re-enable sound */
		osd_sound_enable( 1 );
	}

#ifdef AVICAPTURE
   {
     static int inited=0;
     if (!inited)
     {
	     inited=1;
        init_dumper( visual_width, visual_height );
     }
     frame_dump( bitmap );
   }
   sched_yield();
#endif
	(*x_func[x11_video_mode].update_display) (bitmap);
	return;

barf:
	sysdep_display_close();   /* This cleans up and must be called to
				     restore the videomode with dga */
	fprintf (stderr_file,
			"X11: Error: couldn't create new display while switching display modes\n");
	exit (1);              /* ugly, anyone know a better way ? */
}

/* these aren't nescesarry under x11 since we have both a graphics window and
   a textwindow (xterm) */
int sysdep_set_video_mode (void)
{
	return OSD_OK;
}

void sysdep_set_text_mode (void)
{
}

#endif /* ifdef x11 */
