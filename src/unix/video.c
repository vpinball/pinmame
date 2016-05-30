/*
 * X-Mame generic video code
 *
 */
#define __VIDEO_C_
#include <math.h>
#include "xmame.h"

#ifdef xgl
#include "video-drivers/glmame.h"
#endif
#include <stdio.h>
#include "driver.h"
#include "profiler.h"
#include "input.h"
#include "keyboard.h"
/* for uclock */
#include "sysdep/misc.h"
#include "effect.h"

#define FRAMESKIP_DRIVER_COUNT 2
static const int safety = 16;
int normal_widthscale = 1, normal_heightscale = 1;
int yarbsize = 0;
static char *vector_res = NULL;
static int use_auto_double = 1;
static int frameskipper = 0;
static int bitmap_depth;
static int using_15bpp_rgb_direct;
static int debugger_has_focus = 0;
static struct rectangle normal_visual;
static struct rectangle debug_visual;

static float f_beam;
static float f_flicker;
static float f_intensity;

static int use_artwork = 1;
static int use_backdrops = -1;
static int use_overlays = -1;
static int use_bezels = -1;

static int video_norotate = 0;
static int video_flipy = 0;
static int video_flipx = 0;
static int video_ror = 0;
static int video_rol = 0;
static int video_autoror = 0;
static int video_autorol = 0;

/* average FPS calculation */
static cycles_t start_time = 0;
static cycles_t end_time;
static int frames_displayed;
static int frames_to_display;

#if (defined svgafx) || (defined xfx) 
UINT16 *color_values;
#endif

int force_dirty_palette = 0;
int emulation_paused = 0;

extern UINT8 trying_to_quit;

/* some prototypes */
static int video_handle_scale(struct rc_option *option, const char *arg,
		int priority);
static int video_verify_beam(struct rc_option *option, const char *arg,
		int priority);
static int video_verify_flicker(struct rc_option *option, const char *arg,
		int priority);
static int video_verify_intensity(struct rc_option *option, const char *arg,
		int priority);
static int video_verify_bpp(struct rc_option *option, const char *arg,
		int priority);
static int video_verify_vectorres(struct rc_option *option, const char *arg,
		int priority);

#ifndef xgl
static void adjust_bitmap_and_update_display(struct mame_bitmap *srcbitmap,
		struct rectangle bounds);
#endif

static void change_debugger_focus(int new_debugger_focus);
static void update_debug_display(struct mame_display *display);
static void osd_free_colors(void);
static void round_rectangle_to_8(struct rectangle *rect);
static void update_visible_area(struct mame_display *display);
static void update_palette(struct mame_display *display, int force_dirty);

struct rc_option video_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "bpp",		"b",			rc_int,		&options.color_depth,
     "0",		0,			0,		video_verify_bpp,
     "Specify the colordepth the core should render, one of: auto(0), 8, 16" },
   { "arbheight",	"ah",			rc_int,		&yarbsize,
     "0",		0,			4096,		NULL,
     "Scale video to exactly this height (0 = disable)" },
   { "heightscale",	"hs",			rc_int,		&normal_heightscale,
     "1",		1,			8,		NULL,
     "Set Y-Scale aspect ratio" },
   { "widthscale",	"ws",			rc_int,		&normal_widthscale,
     "1",		1,			8,		NULL,
     "Set X-Scale aspect ratio" },
   { "scale",		"s",			rc_use_function, NULL,
     NULL,		0,			0,		video_handle_scale,
     "Set X-Y Scale to the same aspect ratio. For vector games scale (and also width- and heightscale) may have value's like 1.5 and even 0.5. For scaling of regular games this will be rounded to an int" },
   { "effect",		"ef",			rc_int,		&effect,
     EFFECT_NONE,	EFFECT_NONE,		EFFECT_LAST,	NULL,
     "Video effect:\n"
	     "0 = none (default)\n"
	     "1 = scale2x (smooth scaling effect)\n"
	     "2 = scan2 (light scanlines)\n"
	     "3 = rgbstripe (3x2 rgb vertical stripes)\n"
	     "4 = rgbscan (2x3 rgb horizontal scanlines)\n"
	     "5 = scan3 (3x3 deluxe scanlines)\n" },
   { "autodouble",	"adb",			rc_bool,	&use_auto_double,
     "1",		0,			0,		NULL,
     "Enable/disable automatic scale doubling for 1:2 pixel aspect ratio games" },
   { "scanlines",	"sl",			rc_bool,	&use_scanlines,
     "0",		0,			0,		NULL,
     "Enable/disable displaying simulated scanlines" },
   { "artwork",		"art",			rc_bool,	&use_artwork,
     "1",		0,			0,		NULL,
     "Use additional game artwork (sets default for specific options below)" },
   { "use_backdrops",	"backdrop",		rc_bool,	&use_backdrops,
     "1",		0,			0,		NULL,
     "Use backdrop artwork" },
   { "use_overlays",	"overlay",		rc_bool,	&use_overlays,
     "1",		0,			0,		NULL,
     "Use overlay artwork" },
   { "use_bezels",	"bezel",		rc_bool,	&use_bezels,
     "1",		0,			0,		NULL,
     "Use bezel artwork" },
   { "artwork_crop",	"artcrop",		rc_bool,	&options.artwork_crop,
     "0",		0,			0,		NULL,
     "Crop artwork to game screen only." },
   { "artwork_resolution","artres",		rc_int,		&options.artwork_res,
     "0",		0,			0,		NULL,
     "Artwork resolution (0 for auto)" },
   { "frameskipper",	"fsr",			rc_int,		&frameskipper,
     "1",		0,			FRAMESKIP_DRIVER_COUNT-1, NULL,
     "Select which autoframeskip and throttle routines to use. Available choices are:\n0 Dos frameskip code\n1 Enhanced frameskip code by William A. Barath" },
   { "throttle",	"th",			rc_bool,	&throttle,
     "1",		0,			0,		NULL,
     "Enable/disable throttle" },
   { "frames_to_run",	"ftr",			rc_int,		&frames_to_display,
     "0",		0,			0,		NULL,
     "Sets the number of frames to run within the game" },
   { "sleepidle",	"si",			rc_bool,	&sleep_idle,
     "0",		0,			0,		NULL,
     "Enable/disable sleep during idle" },
   { "autoframeskip",	"afs",			rc_bool,	&autoframeskip,
     "1",		0,			0,		NULL,
     "Enable/disable autoframeskip" },
   { "maxautoframeskip", "mafs",		rc_int,		&max_autoframeskip,
     "8",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set highest allowed frameskip for autoframeskip" },
   { "frameskip",	"fs",			rc_int,		&frameskip,
     "0",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set frameskip when not using autoframeskip" },
   { "brightness",	"brt",			rc_float,	&options.brightness,
     "1.0",		0.5,			2.0,		NULL,
     "Set the brightness correction (0.5 - 2.0)" },
   { "pause_brightness","pb",			rc_float,	&options.pause_bright,
     "0.65",		0.5,			2.0,		NULL,
     "Additional pause brightness" },
   { "gamma",		"gc",			rc_float,	&options.gamma,
     "1.0",		0.5,			2.0,		NULL,
     "Set the gamma correction (0.5 - 2.0)" },
   { "norotate",	"nr",			rc_bool,	&video_norotate,
     "0",		0,			0,		NULL,
     "Do not apply rotation" },
   { "ror",		"rr",			rc_bool,	&video_ror,
     "0",		0,			0,		NULL,
     "Rotate screen clockwise" },
   { "rol",		"rl",			rc_bool,	&video_rol,
     "0",		0,			0,		NULL,
     "Rotate screen counter-clockwise" },
   { "autoror",		NULL,			rc_bool,	&video_autoror,
     "0",		0,			0,		NULL,
     "Automatically rotate screen clockwise for vertical games" },
   { "autorol",		NULL,			rc_bool,	&video_autorol,
     "0",		0,			0,		NULL,
     "Automatically rotate screen counter-clockwise for vertical games" },
   { "flipx",		"fx",			rc_bool,	&video_flipx,
     "0",		0,			0,		NULL,
     "Flip screen left-right" },
   { "flipy",		"fy",			rc_bool,	&video_flipy,
     "0",		0,			0,		NULL,
     "Flip screen upside-down" },
   { "Vector Games Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "vectorres",	"vres",			rc_string,	&vector_res,
     NULL,		0,			0,		video_verify_vectorres,
     "Always scale vectorgames to XresxYres, keeping their aspect ratio. This overrides the scale options" },
	{ "beam", "B", rc_float, &f_beam, "1.0", 1.0, 16.0, video_verify_beam, "Set the beam size for vector games" },
	{ "flicker", "f", rc_float, &f_flicker, "0.0", 0.0, 100.0, video_verify_flicker, "Set the flicker for vector games" },
	{ "intensity", NULL, rc_float, &f_intensity, "1.5", 0.5, 3.0, video_verify_intensity, "Set intensity in vector games" },
   { "antialias",	"aa",			rc_bool,	&options.antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "translucency",	"t",			rc_bool,	&options.translucency,
     "1",		0,			0,		NULL,
     "Enable/disable tranlucency" },
   { NULL,		NULL,			rc_link,	display_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority)
{
	if (rc_set_option2(video_opts, "widthscale", arg, priority))
		return -1;
	if (rc_set_option2(video_opts, "heightscale", arg, priority))
		return -1;

	option->priority = priority;

	return 0;
}

static int video_verify_beam(struct rc_option *option, const char *arg,
		int priority)
{
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	else if (options.beam > 0x00100000)
		options.beam = 0x00100000;

	option->priority = priority;

	return 0;
}

static int video_verify_flicker(struct rc_option *option, const char *arg,
		int priority)
{
	options.vector_flicker = (int)(f_flicker * 2.55);
	if (options.vector_flicker < 0)
		options.vector_flicker = 0;
	else if (options.vector_flicker > 255)
		options.vector_flicker = 255;

	option->priority = priority;

	return 0;
}

static int video_verify_intensity(struct rc_option *option, const char *arg,
		int priority)
{
	options.vector_intensity = f_intensity;
	option->priority = priority;
	return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority)
{
	if (options.color_depth != 0
			&& options.color_depth != 8
			&& options.color_depth != 15
			&& options.color_depth != 16
			&& options.color_depth != 32)
	{
		options.color_depth = 0;
		fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
		return -1;
	}

	option->priority = priority;

	return 0;
}

static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority)
{
	if (sscanf(arg, "%dx%d", &options.vector_width, &options.vector_height) != 2)
	{
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for vectorres: %s\n", arg);
		return -1;
	}

	option->priority = priority;

	return 0;
}

void osd_video_initpre()
{
	/* first start with the game's built-in orientation */
	int orientation = drivers[game_index]->flags & ORIENTATION_MASK;
	options.ui_orientation = orientation;

	if (options.ui_orientation & ORIENTATION_SWAP_XY)
	{
		/* if only one of the components is inverted, switch them */
		if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
				(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
			options.ui_orientation ^= ROT180;
	}

	/* override if no rotation requested */
	if (video_norotate)
		orientation = options.ui_orientation = ROT0;

	/* rotate right */
	if (video_ror)
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* rotate left */
	if (video_rol)
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* auto-rotate right (e.g. for rotating lcds), based on original orientation */
	if (video_autoror && (drivers[game_index]->flags & ORIENTATION_SWAP_XY))
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* auto-rotate left (e.g. for rotating lcds), based on original orientation */
	if (video_autorol && (drivers[game_index]->flags & ORIENTATION_SWAP_XY))
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* flip X/Y */
	if (video_flipx)
		orientation ^= ORIENTATION_FLIP_X;
	if (video_flipy)
		orientation ^= ORIENTATION_FLIP_Y;

	blit_flipx = ((orientation & ORIENTATION_FLIP_X) != 0);
	blit_flipy = ((orientation & ORIENTATION_FLIP_Y) != 0);
	blit_swapxy = ((orientation & ORIENTATION_SWAP_XY) != 0);

	if (options.vector_width == 0 && options.vector_height == 0)
	{
		options.vector_width = 640;
		options.vector_height = 480;
	}

	if (blit_swapxy)
	{
		int temp;
		temp = options.vector_width;
		options.vector_width = options.vector_height;
		options.vector_height = temp;
	}


	/* set the artwork options */
	options.use_artwork = ARTWORK_USE_ALL;
	if (use_backdrops == 0)
		options.use_artwork &= ~ARTWORK_USE_BACKDROPS;
	if (use_overlays == 0)
		options.use_artwork &= ~ARTWORK_USE_OVERLAYS;
	if (use_bezels == 0)
		options.use_artwork &= ~ARTWORK_USE_BEZELS;
	if (!use_artwork)
		options.use_artwork = ARTWORK_USE_NONE;
}

void orient_rect(struct rectangle *rect)
{
	int temp;

	/* apply X/Y swap first */
	if (blit_swapxy)
	{
		temp = rect->min_x;
		rect->min_x = rect->min_y;
		rect->min_y = temp;

		temp = rect->max_x;
		rect->max_x = rect->max_y;
		rect->max_y = temp;
	}

	/* apply X flip */
	if (blit_flipx)
	{
		temp = video_width - rect->min_x - 1;
		rect->min_x = video_width - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (blit_flipy)
	{
		temp = video_height - rect->min_y - 1;
		rect->min_y = video_height - rect->max_y - 1;
		rect->max_y = temp;
	}
}

int osd_create_display(const struct osd_create_params *params, 
		UINT32 *rgb_components)
{
	int r, g, b;

	bitmap_depth = (params->depth == 15) ? 16 : params->depth;
	using_15bpp_rgb_direct = (params->depth == 15);

	current_palette = normal_palette = NULL;
	debug_visual.min_x = 0;
	debug_visual.max_x = options.debug_width - 1;
	debug_visual.min_y = 0;
	debug_visual.max_y = options.debug_height - 1;

	if (use_auto_double)
	{
		if ((params->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) 
				== VIDEO_PIXEL_ASPECT_RATIO_1_2)
		{
			if (params->orientation & ORIENTATION_SWAP_XY)
				normal_widthscale *= 2;
			else
				normal_heightscale *= 2;
		}

		if ((params->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) 
				== VIDEO_PIXEL_ASPECT_RATIO_2_1)
		{
			if (params->orientation & ORIENTATION_SWAP_XY)
				normal_heightscale *= 2;
			else
				normal_widthscale *= 2;
		}
	}

#ifndef xgl
	if (blit_swapxy)
	{
		visual_width	= video_width	= params->height;
		visual_height	= video_height	= params->width;
	}
	else
	{
#endif
		visual_width	= video_width	= params->width;
		visual_height	= video_height	= params->height;
#ifndef xgl
	}
#endif
	video_depth = (params->depth == 15) ? 16 : params->depth;

	if (!blit_swapxy)
		aspect_ratio = (double)params->aspect_x 
			/ (double)params->aspect_y;
	else
		aspect_ratio = (double)params->aspect_y 
			/ (double)params->aspect_x;

	widthscale		= normal_widthscale;
	heightscale		= normal_heightscale;
	use_aspect_ratio	= normal_use_aspect_ratio;
	video_fps		= params->fps;

	if (sysdep_create_display(bitmap_depth) != OSD_OK)
		return -1;

#if 0 /* DEBUG */
	fprintf(stderr_file, "viswidth = %d, visheight = %d, visstartx= %d,"
			"visstarty= %d\n", visual_width, visual_height,
			visual.min_x, visual.min_y);
#endif

	/* a lot of display_targets need to have the display initialised before
	   initialising any input devices */
	if (osd_input_initpost() != OSD_OK)
		return -1;

	if (bitmap_depth == 16)
		fprintf(stderr_file,"Using 16bpp video mode\n");

	if (!(normal_palette = sysdep_palette_create(bitmap_depth, 65536)))
		return 1;

	/* alloc the total number of colors that can be used by the palette */
	if (sysdep_display_alloc_palette(65536))
	{
		osd_free_colors();
		return 1;
	}

	/* initialize the palette to a fixed 5-5-5 mapping */
	if (using_15bpp_rgb_direct)
	{
		for (r = 0; r < 32; r++)
			for (g = 0; g < 32; g++)
				for (b = 0; b < 32; b++)
				{
					int idx = (r << 10) | (g << 5) | b;
					sysdep_palette_set_pen(normal_palette,
							idx,
							(r << 3) | (r >> 2),
							(g << 3) | (g >> 2),
							(b << 3) | (b >> 2));
				}
	}
	else
	{
		for (r = 0; r < 32; r++)
			for (g = 0; g < 32; g++)
				for (b = 0; b < 32; b++)
				{
					int idx = (r << 10) | (g << 5) | b;
					sysdep_palette_set_pen(normal_palette,
							idx, r, g, b);
				}
	}

	current_palette = normal_palette;

	/*
	 * Mark the palette dirty for Xv when running certain NeoGeo games.
	 */
	sysdep_palette_mark_dirty(normal_palette);

	/* fill in the resulting RGB components */
	if (rgb_components)
	{
		if (bitmap_depth == 32)
		{
			rgb_components[0] = (0xff << 16) | (0x00 << 8) | 0x00;
			rgb_components[1] = (0x00 << 16) | (0xff << 8) | 0x00;
			rgb_components[2] = (0x00 << 16) | (0x00 << 8) | 0xff;
		}
		else
		{
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;
		}
	}

	return 0;
}

void osd_close_display(void)
{
	osd_free_colors();
	sysdep_display_close();

	/* print a final result to the stdout */
	if (frames_displayed != 0)
	{
		cycles_t cps = osd_cycles_per_second();
		fprintf(stderr_file, "Average FPS: %f (%d frames)\n", (double)cps / (end_time - start_time) * frames_displayed, frames_displayed);
	}
}

static void osd_change_display_settings(struct rectangle *new_visual,
		struct sysdep_palette_struct *new_palette, int new_widthscale,
		int new_heightscale, int new_use_aspect_ratio)
{
	int new_visual_width, new_visual_height;

	/* always update the visual info */
	visual = *new_visual;

	/* calculate the new visual width / height */
	new_visual_width  = visual.max_x - visual.min_x + 1;
	new_visual_height = visual.max_y - visual.min_y + 1;

	if (current_palette != new_palette)
		current_palette = new_palette;

	if (visual_width != new_visual_width
			|| visual_height != new_visual_height
			|| widthscale != new_widthscale
			|| heightscale != new_heightscale
			|| use_aspect_ratio != new_use_aspect_ratio)
	{
		sysdep_display_close();

		visual_width     = new_visual_width;
		visual_height    = new_visual_height;
		widthscale       = new_widthscale;
		heightscale      = new_heightscale;
		use_aspect_ratio = new_use_aspect_ratio;

		if (sysdep_create_display(bitmap_depth) != OSD_OK)
		{
			/* oops this sorta sucks */
			fprintf(stderr_file, "Argh, resizing the display failed in osd_set_visible_area, aborting\n");
			exit(1);
		}

		/* only realloc the palette if it has been initialised */
		if (current_palette && sysdep_display_alloc_palette(video_colors_used))
		{
			/* better restore the video mode before calling exit() */
			sysdep_display_close();
			/* oops this sorta sucks */
			fprintf(stderr_file, "Argh, (re)allocating the palette failed in osd_set_visible_area, aborting\n");
			exit(1);
		}

		/* to stop keys from getting stuck */
		xmame_keyboard_clear();

#if 0 /* DEBUG */
		fprintf(stderr_file, "viswidth = %d, visheight = %d,"
				"visstartx= %d, visstarty= %d\n",
				visual_width, visual_height, visual.min_x,
				visual.min_y);
#endif
	}
}

static void round_rectangle_to_8(struct rectangle *rect)
{
	if (rect->min_x & 7)
	{
		if ((rect->min_x - (rect->min_x & ~7)) < 4)
			rect->min_x &= ~7;
		else
			rect->min_x = (rect->min_x + 7) & ~7;
	}

	if ((rect->max_x + 1) & 7)
	{
		if (((rect->max_x + 1) - ((rect->max_x + 1) & ~7)) > 4)
			rect->max_x = ((rect->max_x + 1 + 7) & ~7) - 1;
		else
			rect->max_x = ((rect->max_x + 1) & ~7) - 1;
	}
}

static void update_visible_area(struct mame_display *display)
{
	normal_visual = display->game_visible_area;

#ifndef xgl
	if (blit_swapxy)
	{
		video_width = display->game_bitmap->height;
		video_height = display->game_bitmap->width;
	}
	else
	{
		video_width = display->game_bitmap->width;
		video_height = display->game_bitmap->height;
	}

	orient_rect(&normal_visual);
#endif

	/* 
	 * round to 8, since the new dirty code works with 8x8 blocks,
	 * and we need to round to sizeof(long) for the long copies anyway
	 */
	round_rectangle_to_8(&normal_visual);

	if (!debugger_has_focus)
		osd_change_display_settings(&normal_visual, normal_palette, 
				normal_widthscale, normal_heightscale, 
				normal_use_aspect_ratio);

	set_ui_visarea(display->game_visible_area.min_x,
			display->game_visible_area.min_y,
			display->game_visible_area.max_x,
			display->game_visible_area.max_y);
}

static void update_palette(struct mame_display *display, int force_dirty)
{
	int i, j;

	sysdep_palette_clear_dirty(current_palette);
	/* loop over dirty colors in batches of 32 */
	for (i = 0; i < display->game_palette_entries; i += 32)
	{
		UINT32 dirtyflags = display->game_palette_dirty[i / 32];
		if (dirtyflags || force_dirty)
		{
			display->game_palette_dirty[i / 32] = 0;

			/* loop over all 32 bits and update dirty entries */
			for (j = 0; (j < 32) && (i + j < display->game_palette_entries); j++, dirtyflags >>= 1)
				if (((dirtyflags & 1) || force_dirty) && (i + j < display->game_palette_entries))
				{
					/* extract the RGB values */
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);

					sysdep_palette_set_pen(current_palette,
							i + j, r, g, b);
					sysdep_palette_mark_dirty(current_palette);
				}
		}
	}
}

static void update_debug_display(struct mame_display *display)
{
	struct sysdep_palette_struct *backup_palette = current_palette;

	if (!debug_palette)
	{
		int  i, r, g, b;
		debug_palette = sysdep_palette_create(16, 65536);
		/* Initialize the lookup table for the debug palette. */

		for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
		{
			/* extract the RGB values */
			rgb_t rgbvalue = display->debug_palette[i];
			r = RGB_RED(rgbvalue);
			g = RGB_GREEN(rgbvalue);
			b = RGB_BLUE(rgbvalue);

			sysdep_palette_set_pen(debug_palette, 
					i, r, g, b);
		}
	}

	current_palette = debug_palette;
	sysdep_update_display(display->debug_bitmap);
	current_palette = backup_palette;
}

static void osd_free_colors(void)
{
	if (normal_palette)
	{
		sysdep_palette_destroy(normal_palette);
		normal_palette = NULL;
	}

	if (debug_palette)
	{
		sysdep_palette_destroy(debug_palette);
		debug_palette = NULL;
	}
}

static int skip_next_frame = 0;

typedef int (*skip_next_frame_func)(void);
static skip_next_frame_func skip_next_frame_functions[FRAMESKIP_DRIVER_COUNT] =
{
	dos_skip_next_frame,
	barath_skip_next_frame
};

typedef int (*show_fps_frame_func)(char *buffer);
static show_fps_frame_func show_fps_frame_functions[FRAMESKIP_DRIVER_COUNT] =
{
	dos_show_fps,
	barath_show_fps
};

int osd_skip_this_frame(void)
{
	return skip_next_frame;
}

int osd_get_frameskip(void)
{
	return autoframeskip ? -(frameskip + 1) : frameskip;
}

void change_debugger_focus(int new_debugger_focus)
{
	if ((!debugger_has_focus && new_debugger_focus)
			|| (debugger_has_focus && !new_debugger_focus))
	{
		if (new_debugger_focus)
			osd_change_display_settings(&debug_visual, debug_palette,
					1, 1, 0);
		else
			osd_change_display_settings(&normal_visual, normal_palette,
					normal_widthscale, normal_heightscale, normal_use_aspect_ratio);

		debugger_has_focus = new_debugger_focus;
	}
}

/* Update the display. */
void osd_update_video_and_audio(struct mame_display *display)
{
	int skip_this_frame;
	cycles_t curr;

	updatebounds = display->game_bitmap_update;

	/* increment frameskip? */
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		/* if autoframeskip, disable auto and go to 0 */
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = 0;
		}

		/* wrap from maximum to auto */
		else if (frameskip == FRAMESKIP_LEVELS - 1)
		{
			frameskip = 0;
			autoframeskip = 1;
		}

		/* else just increment */
		else
			frameskip++;

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);

		/* reset the frame counter so we'll measure the average FPS on a consistent status */
		frames_displayed = 0;
	}

	/* decrement frameskip? */
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		/* if autoframeskip, disable auto and go to max */
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS-1;
		}

		/* wrap from 0 to auto */
		else if (frameskip == 0)
			autoframeskip = 1;

		/* else just decrement */
		else
			frameskip--;

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);

		/* reset the frame counter so we'll measure the average FPS on a consistent status */
		frames_displayed = 0;
	}

	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		if (!keyboard_pressed(KEYCODE_LSHIFT)
				&& !keyboard_pressed(KEYCODE_RSHIFT))
		{
			throttle ^= 1;

			/*
			 * reset the frame counter so we'll measure the average
			 * FPS on a consistent status
			 */
			frames_displayed = 0;
		}
		else if (keyboard_pressed(KEYCODE_RSHIFT)
				|| keyboard_pressed(KEYCODE_LSHIFT))
			sleep_idle ^= 1;
	}

	if (keyboard_pressed(KEYCODE_LCONTROL))
	{ 
		if (keyboard_pressed_memory(KEYCODE_INSERT))
			frameskipper = 0;
		if (keyboard_pressed_memory(KEYCODE_HOME))
			frameskipper = 1;
	}

	if (keyboard_pressed(KEYCODE_LSHIFT))
	{
		int widthscale_mod  = 0;
		int heightscale_mod = 0;

		if (keyboard_pressed_memory(KEYCODE_INSERT))
			widthscale_mod = 1;
		if (keyboard_pressed_memory(KEYCODE_DEL))
			widthscale_mod = -1;
		if (keyboard_pressed_memory(KEYCODE_HOME))
			heightscale_mod = 1;
		if (keyboard_pressed_memory(KEYCODE_END))
			heightscale_mod = -1;
		if (keyboard_pressed_memory(KEYCODE_PGUP))
		{
			widthscale_mod  = 1;
			heightscale_mod = 1;
		}
		if (keyboard_pressed_memory (KEYCODE_PGDN))
		{
			widthscale_mod  = -1;
			heightscale_mod = -1;
		}
		if (widthscale_mod || heightscale_mod)
		{
			normal_widthscale  += widthscale_mod;
			normal_heightscale += heightscale_mod;

			if (normal_widthscale > 8)
				normal_widthscale = 8;
			else if (normal_widthscale < 1)
				normal_widthscale = 1;

			if (normal_heightscale > 8)
				normal_heightscale = 8;
			else if (normal_heightscale < 1)
				normal_heightscale = 1;

			if (!debugger_has_focus)
				osd_change_display_settings(&normal_visual,
						normal_palette,
						normal_widthscale,
						normal_heightscale,
						normal_use_aspect_ratio);
		}
	}

	skip_this_frame = skip_next_frame;
	skip_next_frame = (*skip_next_frame_functions[frameskipper])();

	if (sound_stream && sound_enabled)
		sound_stream_update(sound_stream);

	/* if the visible area has changed, update it */
	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
		update_visible_area(display);

	/* if the debugger focus changed, update it */
	if (display->changed_flags & DEBUG_FOCUS_CHANGED)
		change_debugger_focus(display->debug_focus);

	/*
	 * If the user presses the F5 key, toggle the debugger's 
	 * focus.  Eventually I'd like to just display both the 
	 * debug and regular windows at the same time.
	 */
	else if (input_ui_pressed(IPT_UI_TOGGLE_DEBUG) && mame_debug)
		change_debugger_focus(!debugger_has_focus);

	/* update the debugger */
	if ((display->changed_flags & DEBUG_BITMAP_CHANGED)
			&& debugger_has_focus)
		update_debug_display(display);

	/* if the game palette has changed, update it */
	if (force_dirty_palette)
	{
		update_palette(display, 1);
		force_dirty_palette = 0;
	}
	else if (display->changed_flags & GAME_PALETTE_CHANGED)
		update_palette(display, 0);

	if (skip_this_frame == 0
			&& (display->changed_flags & GAME_BITMAP_CHANGED)
			&& !debugger_has_focus)
	{
		/* at the end, we need the current time */
		curr = osd_cycles();

		/* update stats for the FPS average calculation */
		if (start_time == 0)
		{
			/* start the timer going 1 second into the game */
			if (timer_get_time() > 1.0)
				start_time = curr;
		}
		else
		{
			frames_displayed++;
			if (frames_displayed + 1 == frames_to_display)
				trying_to_quit = 1;
			end_time = curr;
		}

		profiler_mark(PROFILER_BLIT);
#ifdef xgl
		sysdep_update_display(display->game_bitmap);
#else
		adjust_bitmap_and_update_display(display->game_bitmap,
				updatebounds);
#endif
		profiler_mark(PROFILER_END);
	}

	/* if the LEDs have changed, update them */
	if (display->changed_flags & LED_STATE_CHANGED)
		sysdep_set_leds(display->led_state);

	osd_poll_joysticks();
}

#ifndef xgl
void adjust_bitmap_and_update_display(struct mame_bitmap *srcbitmap,
		struct rectangle bounds)
{
	orient_rect(&bounds);

	sysdep_update_display(srcbitmap);
}
#endif

#ifndef xgl
struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap, 
		struct rectangle *bounds)
{
	struct rectangle newbounds;
	struct mame_bitmap *copy;
	int x, y, w, h, t;

	/* if we can send it in raw, no need to override anything */
	if (!blit_swapxy && !blit_flipx && !blit_flipy)
		return NULL;

	/* allocate a copy */
	w = blit_swapxy ? bitmap->height : bitmap->width;
	h = blit_swapxy ? bitmap->width : bitmap->height;
	copy = bitmap_alloc_depth(w, h, bitmap->depth);
	if (!copy)
		return NULL;

	/* populate the copy */
	for (y = bounds->min_y; y <= bounds->max_y; y++)
		for (x = bounds->min_x; x <= bounds->max_x; x++)
		{
			int tx = x, ty = y;

			/* apply the rotation/flipping */
			if (blit_swapxy)
			{
				t = tx; tx = ty; ty = t;
			}
			if (blit_flipx)
				tx = copy->width - tx - 1;
			if (blit_flipy)
				ty = copy->height - ty - 1;

			/* read the old pixel and copy to the new location */
			switch (copy->depth)
			{
				case 15:
				case 16:
					*((UINT16 *)copy->base + ty * copy->rowpixels + tx) =
						*((UINT16 *)bitmap->base + y * bitmap->rowpixels + x);
					break;

				case 32:
					*((UINT32 *)copy->base + ty * copy->rowpixels + tx) =
						*((UINT32 *)bitmap->base + y * bitmap->rowpixels + x);
					break;
			}
		}

	/* compute the oriented bounds */
	newbounds = *bounds;

	/* apply X/Y swap first */
	if (blit_swapxy)
	{
		t = newbounds.min_x; newbounds.min_x = newbounds.min_y; newbounds.min_y = t;
		t = newbounds.max_x; newbounds.max_x = newbounds.max_y; newbounds.max_y = t;
	}

	/* apply X flip */
	if (blit_flipx)
	{
		t = copy->width - newbounds.min_x - 1;
		newbounds.min_x = copy->width - newbounds.max_x - 1;
		newbounds.max_x = t;
	}

	/* apply Y flip */
	if (blit_flipy)
	{
		t = copy->height - newbounds.min_y - 1;
		newbounds.min_y = copy->height - newbounds.max_y - 1;
		newbounds.max_y = t;
	}

	*bounds = newbounds;
	return copy;
}
#endif

void osd_pause(int paused)
{
	emulation_paused = paused;	
}

const char *osd_get_fps_text(const struct performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;

	int chars_filled
		= (*show_fps_frame_functions[frameskipper])(dest);

	if (chars_filled)
		dest += chars_filled;
	else
	{
		/* display the FPS, frameskip, percent, fps and target fps */
		dest += sprintf(dest, "%s%s%s%2d%4d%%%4d/%d fps", 
				throttle ? "T " : "",
				(throttle && sleep_idle) ? "S " : "",
				autoframeskip ? "auto" : "fskp", frameskip, 
				(int)(performance->game_speed_percent + 0.5), 
				(int)(performance->frames_per_second + 0.5),
				(int)(Machine->drv->frames_per_second + 0.5));
	}

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}

	/* return a pointer to the static buffer */
	return buffer;
}

/*
 * We don't want to sleep when idle while the setup menu is 
 * active, since this causes problems with registering 
 * keypresses.
 */
int should_sleep_idle()
{
	return sleep_idle && !setup_active();
}
