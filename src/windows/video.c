//============================================================
//
//	video.c - Win32 video handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// undef WINNT for ddraw.h to prevent duplicate definition
#undef WINNT
#define DIRECTDRAW_VERSION 0x0300
#include <ddraw.h>

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "mamedbg.h"
#include "vidhrdw/vector.h"
#include "dirty.h"
#include "ticker.h"
#include "blit.h"
#include "video.h"
#include "window.h"
#include "rc.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win32_poll_input(void);
extern void win32_pause_input(int pause);
extern UINT8 trying_to_quit;

#ifdef PINMAME_EXT
// from wpc/snd_cmd.c
extern int recording;
#endif

//============================================================
//	PARAMETERS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS		12

// VERY IMPORTANT: osd_alloc_bitmap must allocate also a "safety area" 16 pixels wide all
// around the bitmap. This is required because, for performance reasons, some graphic
// routines don't clip at boundaries of the bitmap.
#define BITMAP_SAFETY			16



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
int frameskip;
int autoframeskip;

// dirty grid setting
int use_dirty = 0;
DIRTYGRID dirty_grid;

// gamma correction
float gamma_correct = 1.0;

// speed throttling
int throttle = 1;
int game_speed_percent = 100;

// palette lookups
UINT32 *palette_16bit_lookup;
UINT32 *palette_32bit_lookup;

// debugger palette
const UINT8 *dbg_palette;



//============================================================
//	LOCAL VARIABLES
//============================================================

// core video input parameters
static int video_depth;
static double video_fps;
static int video_attributes;
static int video_orientation;

// derived from video attributes
static int vector_game;
static int rgb_direct;

// current visible area bounds
static int vis_min_x;
static int vis_max_x;
static int vis_min_y;
static int vis_max_y;
static int vis_width;
static int vis_height;

// internal readiness states
static int warming_up;

// LED states
static int leds_old;

// palette attributes
static int modifiable_palette;
static int screen_colors;

// palette values
static UINT8 *current_palette;

// dirty palette tracking
static UINT8 *dirtycolor;
static int dirtypalette;

// brightness mapping
static int bright_lookup[256];
static int dirty_bright;
static float brightness;
static float brightness_adjust;

// timing measurements for throttling
static TICKER last_skipcount0_time;
static TICKER this_frame_base;

// FPS display info
static int showfps;
static int showfpstemp;
static int vups;
static int vfcount;

// average FPS calculation
static TICKER start_time;
static TICKER end_time;
static int frames_displayed;
static int frames_to_display;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// frameskipping tables
static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
};

static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 1,1,1,1,1,1,1,1,1,1,1,1 },
	{ 2,1,1,1,1,1,1,1,1,1,1,0 },
	{ 2,1,1,1,1,0,2,1,1,1,1,0 },
	{ 2,1,1,0,2,1,1,0,2,1,1,0 },
	{ 2,1,0,2,1,0,2,1,0,2,1,0 },
	{ 2,0,2,1,0,2,0,2,1,0,2,0 },
	{ 2,0,2,0,2,0,2,0,2,0,2,0 },
	{ 2,0,2,0,0,3,0,2,0,0,3,0 },
	{ 3,0,0,3,0,0,3,0,0,3,0,0 },
	{ 4,0,0,0,4,0,0,0,4,0,0,0 },
	{ 6,0,0,0,0,0,6,0,0,0,0,0 },
	{12,0,0,0,0,0,0,0,0,0,0,0 }
};



//============================================================
//	OPTIONS
//============================================================

// prototypes
static int video_set_resolution(struct rc_option *option, const char *arg, int priority);
static int decode_effect(struct rc_option *option, const char *arg, int priority);
static int decode_aspect(struct rc_option *option, const char *arg, int priority);

// internal variables
static char *resolution;
static char *effect;
static char *aspect;

// options struct
struct rc_option video_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "autoframeskip", "afs", rc_bool, &autoframeskip, "1", 0, 0, NULL, "skip frames to speed up emulation" },
	{ "frameskip", "fs", rc_int, &frameskip, "0", 0, 12, NULL, "set frameskip explicitly (autoframeskip needs to be off)" },
	{ "waitvsync", NULL, rc_bool, &wait_vsync, "0", 0, 0, NULL, "wait for vertical sync (reduces tearing)"},
	{ "triplebuffer", "tb", rc_bool, &use_triplebuf, "0", 0, 0, NULL, "triple buffering (only if fullscreen)" },
	{ "window", "w", rc_bool, &window_mode, "0", 0, 0, NULL, "run in a window/run on full screen" },
	{ "ddraw", "dd", rc_bool, &use_ddraw, "1", 0, 0, NULL, "use DirectDraw for rendering" },
	{ "hwstretch", "hws", rc_bool, &ddraw_stretch, "1", 0, 0, NULL, "stretch video using the hardware" },
	{ "resolution", "r", rc_string, &resolution, "auto", 0, 0, video_set_resolution, "set resolution" },
	{ "refresh", NULL, rc_int, &gfx_refresh, "0", 0, 0, NULL, "set specific monitor refresh rate" },
	{ "scanlines", "sl", rc_bool, &scanlines, "0", 0, 0, NULL, "emulate scanlines" },
	{ "switchres", NULL, rc_bool, &switchres, "1", 0, 0, NULL, "switch resolutions to best fit" },
	{ "switchbpp", NULL, rc_bool, &switchbpp, "1", 0, 0, NULL, "switch color depths to best fit" },
	{ "maximize", "max", rc_bool, &maximize, "1", 0, 0, NULL, "start out maximized" },
	{ "keepaspect", "ka", rc_bool, &keepaspect, "1", 0, 0, NULL, "enforce aspect ratio" },
	{ "matchrefresh", NULL, rc_bool, &matchrefresh, "0", 0, 0, NULL, "attempt to match the game's refresh rate" },
	{ "syncrefresh", NULL, rc_bool, &syncrefresh, "0", 0, 0, NULL, "syncronize only to the monitor refresh" },
	{ "dirty", NULL, rc_bool, &use_dirty, "1", 0, 0, NULL, "enable dirty video optimization" },
	{ "throttle", NULL, rc_bool, &throttle, "1", 0, 0, NULL, "throttle speed to the game's framerate" },
	{ "full_screen_brightness", "fsb", rc_float, &gfx_brightness, "0.0", 0.0, 4.0, NULL, "sets the brightness in full screen mode" },
	{ "frames_to_run", "ftr", rc_int, &frames_to_display, "0", 0, 0, NULL, "sets the number of frames to run within the game" },
	{ "effect", NULL, rc_string, &effect, "none", 0, 0, decode_effect, "specify the blitting effect" },
	{ "screen_aspect", NULL, rc_string, &aspect, "4:3", 0, 0, decode_aspect, "specify an alternate monitor aspect ratio" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//	color_correct
//============================================================

INLINE int color_correct(int value)
{
	return (int)(255.0 * (brightness * brightness_adjust) * pow(value * (1.0 / 255.0), 1 / gamma_correct));
}



//============================================================
//	mark_palette_dirty
//============================================================

void mark_palette_dirty(void)
{
	memset(dirtycolor, 1, screen_colors);
	dirtypalette = 1;
}



//============================================================
//	video_set_resolution
//============================================================

static int video_set_resolution(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		gfx_width = gfx_height = gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
	}
	else if (sscanf(arg, "%dx%dx%d", &gfx_width, &gfx_height, &gfx_depth) < 2)
	{
		gfx_width = gfx_height = gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	if ((gfx_depth != 0) &&
		(gfx_depth != 8) &&
		(gfx_depth != 16) &&
		(gfx_depth != 24) &&
		(gfx_depth != 32))
	{
		gfx_width = gfx_height = gfx_depth = 0;
		options.vector_width = options.vector_height = 0;
		fprintf(stderr, "error: invalid value for resolution: %s\n", arg);
		return -1;
	}
	options.vector_width = gfx_width;
	options.vector_height = gfx_height;

	option->priority = priority;
	return 0;
}



//============================================================
//	decode_effect
//============================================================

static int decode_effect(struct rc_option *option, const char *arg, int priority)
{
	bliteffect = lookup_effect(arg);
	if (bliteffect == -1)
	{
		fprintf(stderr, "error: invalid value for effect: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}



//============================================================
//	decode_aspect
//============================================================

static int decode_aspect(struct rc_option *option, const char *arg, int priority)
{
	int num, den;

	if (sscanf(arg, "%d:%d", &num, &den) != 2 || num == 0 || den == 0)
	{
		fprintf(stderr, "error: invalid value for aspect ratio: %s\n", arg);
		return -1;
	}
	screen_aspect = (double)num / (double)den;

	option->priority = priority;
	return 0;
}



//============================================================
//	osd_alloc_bitmap
//============================================================

struct osd_bitmap *osd_alloc_bitmap(int width, int height, int depth)
{
	struct osd_bitmap *bitmap;

	// verify it's a depth we can handle
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	// allocate memory for the bitmap struct
	bitmap = malloc(sizeof(struct osd_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth;
		unsigned char *bm;

		// initialize the basic parameters
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		// round the width to a quadword
		rdwidth = (width + 7) & ~7;
		rowlen = (rdwidth + 2 * BITMAP_SAFETY) * sizeof(unsigned char);

		// expand 32bpp and 15/16bpp depths
		if (depth == 32)
			rowlen *= 4;
		else if (depth == 15 || depth == 16)
			rowlen *= 2;

		// allocate memory for the bitmap itself
		bm = malloc((height + 2 * BITMAP_SAFETY) * rowlen);
		if (bm == NULL)
		{
			free(bitmap);
			return 0;
		}

		// clear ALL bitmap, including safety area, to avoid garbage on right
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		// allocate an array of line pointers
		bitmap->line = malloc((height + 2 * BITMAP_SAFETY) * sizeof(unsigned char *));
		if (bitmap->line == NULL)
		{
			free(bm);
			free(bitmap);
			return 0;
		}

		// initialize the line pointers
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
		{
			if (depth == 32)
				bitmap->line[i] = &bm[i * rowlen + 4*BITMAP_SAFETY];
			else if (depth == 15 || depth == 16)
				bitmap->line[i] = &bm[i * rowlen + 2*BITMAP_SAFETY];
			else
				bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY];
		}

		// adjust for the safety rows
		bitmap->line += BITMAP_SAFETY;

		// save a pointer to the bitmap data in the private pointer
		bitmap->_private = bm;
	}

	// return the result
	return bitmap;
}



//============================================================
//	osd_free_bitmap
//============================================================

void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	// skip if NULL
	if (!bitmap)
		return;

	// unadjust for the safety rows
	bitmap->line -= BITMAP_SAFETY;

	// free the memory
	free(bitmap->line);
	free(bitmap->_private);
	free(bitmap);
}



//============================================================
//	init_dirty
//============================================================

static void init_dirty(char dirty)
{
	memset(dirty_grid, dirty, sizeof(dirty_grid));
}



//============================================================
//	mark_full_screen_dirty
//============================================================

static void mark_full_screen_dirty(void)
{
	osd_mark_dirty(0, 0, 65535, 65535);
}



//============================================================
//	osd_mark_dirty
//============================================================

void osd_mark_dirty(int left, int top, int right, int bottom)
{
	int x, y;

	// don't bother if we're not handling dirty marking
	if (!use_dirty)
		return;

	// adjust for top/left
	left -= vis_min_x;
	right -= vis_min_x;
	top -= vis_min_y;
	bottom -= vis_min_y;

	// if completely out of range, bail
	if (top >= vis_height || bottom < 0 || left > vis_width || right < 0)
		return;

	// clamp to the bitmap size
	if (top < 0)
		top = 0;
	if (bottom >= vis_height)
		bottom = vis_height - 1;
	if (left < 0)
		left = 0;
	if (right >= vis_width)
		right = vis_width - 1;

	// iterate over 16-pixel chunks and mark it
	for (y = top; y <= bottom + 15; y += 16)
		for (x = left; x <= right + 15; x += 16)
			MARK_DIRTY(x, y);
}



//============================================================
//	osd_set_visible_area
//============================================================

void osd_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	// copy the new parameters
	vis_min_x = min_x;
	vis_max_x = max_x;
	vis_min_y = min_y;
	vis_max_y = max_y;

	// track these changes
	logerror("set visible area %d-%d %d-%d\n",min_x,max_x,min_y,max_y);

	// compute the visible width and height
	vis_width  = max_x - min_x + 1;
	vis_height = max_y - min_y + 1;

	// tell the UI where it can draw
	set_ui_visarea(vis_min_x, vis_min_y, vis_min_x + vis_width - 1, vis_min_y + vis_height - 1);

	// now adjust the window for the aspect ratio
	if (vis_width > 1 && vis_height > 1)
		adjust_window_for_visible(min_x, max_x, min_y, max_y);
}



//============================================================
//	osd_create_display
//============================================================

int osd_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
	logerror("width %d, height %d depth %d\n",width,height,depth);

	// copy the parameters into globals for later use
	video_depth			= (depth != 15) ? depth : 16;
	video_fps			= Machine->drv->frames_per_second;
	video_attributes	= attributes;
	video_orientation	= orientation;

	// initialize internal states
	brightness			= 1.0;
	brightness_adjust	= 1.0;
	dirty_bright		= 1;
	leds_old			= -1;

	// clamp the frameskip value to within range
	if (frameskip < 0)
		frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS)
		frameskip = FRAMESKIP_LEVELS - 1;

	// extract useful parameters from the attributes
	vector_game			= ((attributes & VIDEO_TYPE_VECTOR) != 0);
	rgb_direct			= ((attributes & VIDEO_RGB_DIRECT) != 0);
	if (use_dirty && !(attributes & VIDEO_SUPPORTS_DIRTY))
		use_dirty = 0;

	// create the window
	if (create_window(width, height, video_depth, attributes, orientation))
		return 1;

	// set visible area to nothing just to initialize it - it will be set by the core
	osd_set_visible_area(0,0,0,0);

	// indicate for later that we're just beginning
	warming_up = 1;

    return 0;
}



//============================================================
//	osd_close_display
//============================================================

void osd_close_display(void)
{
	// tear down the window
	destroy_window();

	// print a final result to the stdout
	if (frames_displayed != 0)
		printf("Average FPS: %f (%d frames)\n", (double)TICKS_PER_SEC / (end_time - start_time) * frames_displayed, frames_displayed);

	// free the array of dirty colors
	free(dirtycolor);
	dirtycolor = NULL;

	// free the current palette
	free(current_palette);
	current_palette = NULL;

	// free the 16bpp lookup table
	free(palette_16bit_lookup);
	palette_16bit_lookup = NULL;

	// free the 32bpp lookup table
	free(palette_32bit_lookup);
	palette_32bit_lookup = NULL;
}



//============================================================
//	init_direct_mapped_16bpp
//============================================================

static int init_direct_mapped_16bpp(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens)
{
	unsigned char *pal;
	int r, g, b;

	// 16bpp direct-mapped 5-5-5 palette = 32768 colors
	screen_colors = 32768;

	// allocate memory for the dirty buffer, palette, and 16bpp lookup
	dirtycolor = malloc(screen_colors * sizeof(dirtycolor[0]));
	current_palette = malloc(3 * screen_colors * sizeof(current_palette[0]));
	palette_16bit_lookup = malloc(screen_colors * sizeof(palette_16bit_lookup[0]));
	palette_32bit_lookup = malloc(screen_colors * sizeof(palette_32bit_lookup[0]));

	// handle failure
	if (dirtycolor == NULL || current_palette == NULL || palette_16bit_lookup == NULL || palette_32bit_lookup == NULL)
		return 1;

	// mark the palette dirty to start
	mark_palette_dirty();

	// initialize the palette to a fixed 5-5-5 mapping
	pal = current_palette;
	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
			{
				*pal++ = (r << 3) | (r >> 2);
				*pal++ = (g << 3) | (g >> 2);
				*pal++ = (b << 3) | (b >> 2);
			}

	// initialize the default palette
	pens[0] = 0x7c00;
	pens[1] = 0x03e0;
	pens[2] = 0x001f;

	// initialize the first 4 color table entries to something useful
	Machine->uifont->colortable[0] = 0x0000;
	Machine->uifont->colortable[1] = 0x7fff;
	Machine->uifont->colortable[2] = 0x7fff;
	Machine->uifont->colortable[3] = 0x0000;

	return 0;
}



//============================================================
//	init_direct_mapped_32bpp
//============================================================

static int init_direct_mapped_32bpp(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens)
{
	int i, r, g, b;

	// 32bpp direct-mapped 8-8-8 palette = 2 colors?
	screen_colors = 2;

	// allocate memory for the dirty buffer and palette
	dirtycolor = malloc(screen_colors * sizeof(dirtycolor[0]));
	current_palette = malloc(3 * screen_colors * sizeof(current_palette[0]));

	// handle failure
	if (dirtycolor == NULL || current_palette == NULL)
		return 1;

	// mark the palette dirty to start
	mark_palette_dirty();

	// initialize the default palette
	for (i = 0; i < totalcolors; i++)
	{
		r = color_correct(palette[3 * i + 0]);
		g = color_correct(palette[3 * i + 1]);
		b = color_correct(palette[3 * i + 2]);
		*pens++ = color32(r, g, b);
	}

	// initialize the first 4 color table entries to something useful
	Machine->uifont->colortable[0] = color32(0, 0, 0);
	Machine->uifont->colortable[1] = color32(255, 255, 255);
	Machine->uifont->colortable[2] = color32(255, 255, 255);
	Machine->uifont->colortable[3] = color32(0, 0, 0);

	return 0;
}



//============================================================
//	osd_allocate_colors
//============================================================

int osd_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable,
	const UINT8 *debug_palette, UINT32 *debug_pens)
{
	int i;

	// stash the debug palette pointer for later
	dbg_palette = debug_palette;

	// if we have debug pens, map them 1:1
	if (debug_pens)
		for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
			debug_pens[i] = i;

	// note the modifiable state
	modifiable_palette = modifiable;

	// handle direct-mapped modes separately
	if (rgb_direct)
	{
		if (video_depth == 16)
			return init_direct_mapped_16bpp(totalcolors, palette, pens);
		else
			return init_direct_mapped_32bpp(totalcolors, palette, pens);
	}

	// otherwise, set the screen colors based on the total number of colors
	screen_colors = totalcolors;
	if (video_depth != 8)
		screen_colors += 2;
	else
		screen_colors = 256;

	// allocate memory for the dirty buffer, palette, and 16bpp lookup
	dirtycolor = malloc(screen_colors * sizeof(dirtycolor[0]));
	current_palette = malloc(3 * screen_colors * sizeof(current_palette[0]));
	palette_16bit_lookup = malloc(screen_colors * sizeof(palette_16bit_lookup[0]));
	palette_32bit_lookup = malloc(screen_colors * sizeof(palette_32bit_lookup[0]));

	// handle failure
	if (dirtycolor == NULL || current_palette == NULL || palette_16bit_lookup == NULL || palette_32bit_lookup == NULL)
		return 1;

	// mark the palette dirty to start
	mark_palette_dirty();

	// initialize the palette to black
	for (i = 0; i < screen_colors; i++)
		current_palette[3 * i + 0] = current_palette[3 * i + 1] = current_palette[3 * i + 2] = 0;

	// 8bpp palette-compressed case
	if (video_depth == 8 && totalcolors >= 255)
	{
		int bestblackscore = 3 * 255 * 255, bestwhitescore = 0;
		int bestblack = 0, bestwhite = 0;

		// find the closest black & white colors
		for (i = 0; i < totalcolors; i++)
		{
			int r = palette[3 * i + 0];
			int g = palette[3 * i + 1];
			int b = palette[3 * i + 2];
			int score = r*r + g*g + b*b;

			// best black so far?
			if (score < bestblackscore)
			{
				bestblack = i;
				bestblackscore = score;
			}

			// best white so far?
			if (score > bestwhitescore)
			{
				bestwhite = i;
				bestwhitescore = score;
			}
		}

		// initialize the pens 1:1
		for (i = 0; i < totalcolors; i++)
			pens[i] = i;

		// map black to pen 0, otherwise the screen border will not be black
		pens[bestblack] = 0;
		pens[0] = bestblack;

		// set the UI font
		Machine->uifont->colortable[0] = pens[bestblack];
		Machine->uifont->colortable[1] = pens[bestwhite];
		Machine->uifont->colortable[2] = pens[bestwhite];
		Machine->uifont->colortable[3] = pens[bestblack];
	}
	else
	{
		// reserve color 1 for the user interface text */
		current_palette[3*1+0] = current_palette[3*1+1] = current_palette[3*1+2] = 0xff;
		Machine->uifont->colortable[0] = 0;
		Machine->uifont->colortable[1] = 1;
		Machine->uifont->colortable[2] = 1;
		Machine->uifont->colortable[3] = 0;

		// fill the palette starting from the end, so we mess up badly written
		// drivers which don't go through Machine->pens[]
		for (i = 0; i < totalcolors; i++)
			pens[i] = (screen_colors - 1) - i;
	}

	// initialize the current palette with the input palette
	for (i = 0; i < totalcolors; i++)
	{
		current_palette[3 * pens[i] + 0] = palette[3 * i];
		current_palette[3 * pens[i] + 1] = palette[3 * i + 1];
		current_palette[3 * pens[i] + 2] = palette[3 * i + 2];
	}

	return 0;
}



//============================================================
//	osd_modify_pen
//============================================================

void osd_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
	// error if the palette is non-modifiable
	if (!modifiable_palette)
	{
		logerror("error: osd_modify_pen() called with modifiable_palette == 0\n");
		return;
	}

	// only register changes
	if (current_palette[3 * pen + 0] != red ||
		current_palette[3 * pen + 1] != green ||
		current_palette[3 * pen + 2] != blue)
	{
		// set the new palette
		current_palette[3 * pen + 0] = red;
		current_palette[3 * pen + 1] = green;
		current_palette[3 * pen + 2] = blue;

		// mark the color and palette dirty
		dirtycolor[pen] = 1;
		dirtypalette = 1;
	}
}



//============================================================
//	osd_get_pen
//============================================================

void osd_get_pen(int pen, unsigned char *red, unsigned char *green, unsigned char *blue)
{
	// 16bpp non-modifiable case
	if (video_depth != 8 && !modifiable_palette)
	{
		*red	= red16(pen);
		*green 	= green16(pen);
		*blue	= blue16(pen);
	}

	// modifiable cases
	else
	{
		*red	= current_palette[3 * pen + 0];
		*green	= current_palette[3 * pen + 1];
		*blue	= current_palette[3 * pen + 2];
	}
}



//============================================================
//	osd_skip_this_frame
//============================================================

int osd_skip_this_frame(void)
{
	// skip the current frame?
	return skiptable[frameskip][frameskip_counter];
}



//============================================================
//	check_inputs
//============================================================

static void check_inputs(void)
{
	static int alt_enter_pressed;

	// increment frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
	{
		// if autoframeskip, disable auto and go to 0
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = 0;
		}

		// wrap from maximum to auto
		else if (frameskip == FRAMESKIP_LEVELS - 1)
		{
			frameskip = 0;
			autoframeskip = 1;
		}

		// else just increment
		else
			frameskip++;

		// display the FPS counter for 2 seconds
		if (!showfps)
			showfpstemp = (int)(2.0 * video_fps);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// decrement frameskip?
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
	{
		// if autoframeskip, disable auto and go to max
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS-1;
		}

		// wrap from 0 to auto
		else if (frameskip == 0)
			autoframeskip = 1;

		// else just decrement
		else
			frameskip--;

		// display the FPS counter for 2 seconds
		if (!showfps)
			showfpstemp = (int)(2.0 * video_fps);

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// toggle throttle?
	if (input_ui_pressed(IPT_UI_THROTTLE))
	{
		throttle ^= 1;

		// reset the frame counter so we'll measure the average FPS on a consistent status
		frames_displayed = 0;
	}

	// toggle display FPS?
	if (input_ui_pressed(IPT_UI_SHOW_FPS))
	{
		// if we're temporarily on, turn it off immediately
		if (showfpstemp)
		{
			showfpstemp = 0;
			schedule_full_refresh();
		}

		// otherwise, just toggle; force a refresh if going off
		else
		{
			showfps ^= 1;
			if (!showfps)
				schedule_full_refresh();
		}
	}

	// check for toggling fullscreen mode
	if (code_pressed(KEYCODE_ENTER) &&
		(code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_RALT)))
	{
		if (!alt_enter_pressed)
			toggle_full_screen();
		alt_enter_pressed = 1;
	}
	else
		alt_enter_pressed = 0;
}



//============================================================
//	throttle_speed
//============================================================

static void throttle_speed(void)
{
	TICKER target;
	TICKER curr;

	// if we're only syncing to the refresh, bail now
	if (syncrefresh)
		return;

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// get the current time and the target time
	curr = ticker();
	target = this_frame_base + (int)((double)frameskip_counter * (double)TICKS_PER_SEC / video_fps);

	// sync
	if (curr - target < 0)
	{
		do
		{
			curr = ticker();
		} while (curr - target < 0);
	}

	// idle time done
	profiler_mark(PROFILER_END);
}



//============================================================
//	update_palette_8
//============================================================

void update_palette_8(void)
{
	int i;

	// loop over dirty colors
	for (i = 0; i < screen_colors; i++)
		if (dirtycolor[i])
		{
			int r = current_palette[3 * i + 0];
			int g = current_palette[3 * i + 1];
			int b = current_palette[3 * i + 2];

			// don't adjust the brightness of UI text
			if (i != Machine->uifont->colortable[1])
			{
				r = bright_lookup[r];
				g = bright_lookup[g];
				b = bright_lookup[b];
			}

			// set the entry
			set_palette_entry(i, r, g, b);

			// no longer dirty
			dirtycolor[i] = 0;
		}
}



//============================================================
//	update_palette_16
//============================================================

void update_palette_16(void)
{
	int i;

	// any palette change here requires everything to be dirtied
	if (use_dirty)
		init_dirty(1);
	dirtypalette = 0;

	// loop over dirty colors
	for (i = 0; i < screen_colors; i++)
		if (dirtycolor[i])
		{
			int r = current_palette[3 * i + 0];
			int g = current_palette[3 * i + 1];
			int b = current_palette[3 * i + 2];

			// don't adjust the brightness of UI text
			if (i != Machine->uifont->colortable[1])
			{
				r = bright_lookup[r];
				g = bright_lookup[g];
				b = bright_lookup[b];
			}

			// set the 16-bit color value
			palette_16bit_lookup[i] = color16(r, g, b) * 0x10001;
			palette_32bit_lookup[i] = color32(r, g, b);

			// no longer dirty
			dirtycolor[i] = 0;
		}
}



//============================================================
//	display_fps
//============================================================

static void display_fps(struct osd_bitmap *bitmap)
{
	int divdr = 100 * FRAMESKIP_LEVELS;
	int fps = (video_fps * (double)(FRAMESKIP_LEVELS - frameskip) * game_speed_percent + (divdr / 2)) / divdr;
	char buf[30];

	// display the FPS, frameskip, percent, fps and target fps
	sprintf(buf, "%s%2d%4d%%%4d/%d fps", autoframeskip ? "auto" : "fskp", frameskip, game_speed_percent, fps, (int)(video_fps + 0.5));
	ui_text(bitmap, buf, Machine->uiwidth - strlen(buf) * Machine->uifontwidth, 0);

	// for vector games, add the number of vector updates
	if (vector_game)
	{
		sprintf(buf, " %d vector updates", vups);
		ui_text(bitmap, buf, Machine->uiwidth - strlen(buf) * Machine->uifontwidth, Machine->uifontheight);
	}

	// update the temporary FPS display state
	if (showfpstemp)
	{
		showfpstemp--;
		if (!showfps && showfpstemp == 0)
			schedule_full_refresh();
	}
}



//============================================================
//	update_autoframeskip
//============================================================

void update_autoframeskip(void)
{
	// if we're too fast, attempt to increase the frameskip
	if (game_speed_percent >= 100)
	{
		frameskipadjust++;

		// but only after 3 consecutive frames where we are too fast
		if (frameskipadjust >= 3)
		{
			frameskipadjust = 0;
			if (frameskip > 0) frameskip--;
		}
	}

	// if we're too slow, attempt to increase the frameskip
	else
	{
		// if below 80% speed, be more aggressive
		if (game_speed_percent < 80)
			frameskipadjust -= (90 - game_speed_percent) / 5;

		// if we're close, only force it up to frameskip 8
		else if (frameskip < 8)
			frameskipadjust--;

		// perform the adjustment
		while (frameskipadjust <= -2)
		{
			frameskipadjust += 2;
			if (frameskip < FRAMESKIP_LEVELS - 1)
				frameskip++;
		}
	}
}



//============================================================
//	render_frame
//============================================================

static void render_frame(struct osd_bitmap *bitmap)
{
	TICKER curr;
	int i;

	// if we're throttling, synchronize
	if (throttle)
		throttle_speed();

	// at the end, we need the current time
	curr = ticker();

	// update stats for the FPS average calculation
	if (start_time == 0)
	{
		// start the timer going 1 second into the game
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

	// if we're at the start of a frameskip sequence, compute the speed
	if (frameskip_counter == 0)
	{
		int divdr = (int)(video_fps * (double)(curr - last_skipcount0_time) / (double)(100 * FRAMESKIP_LEVELS));
		game_speed_percent = (TICKS_PER_SEC + divdr/2) / divdr;
		last_skipcount0_time = curr;
	}

	// for vector games, compute the vector update count once/second
	vfcount += waittable[frameskip][frameskip_counter];
	if (vfcount >= (int)video_fps)
	{
		// from vidhrdw/avgdvg.c
		extern int vector_updates;

		vfcount -= (int)video_fps;
		vups = vector_updates;
		vector_updates = 0;
	}

	// display the FPS value
	if (showfps || showfpstemp)
		display_fps(bitmap);

#ifdef PINMAME_EXT
	/*Display the status of recording a .wav file*/
	if (recording != 0)
	{
		char buf[30];
		static int dots=0;
		switch(recording){
			/*recording failed*/
			case -1:
				sprintf(buf,"Unable to Record Wave File!!!");
				break;
			/*recording now*/
			case 1:
				sprintf(buf,"Recording Wave File%c%c%c",(dots>=25)?'.':' ',(dots>=50)?'.':' ',(dots>=75)?'.':' ');
				dots++;
				if(dots>100) dots=0;
				break;
			/*recoring done*/
			case 2:
				dots=0;
				recording = 0;
			default:
				sprintf(buf,"                            ");
		}
		ui_text(bitmap,buf,Machine->uiwidth-strlen(buf)*Machine->uifontwidth,0);
	}
#endif /* PINMAME_EXT */

	// recompute the brightness adjustment table if necessary
	if (dirty_bright)
	{
		dirty_bright = 0;
		for (i = 0; i < 256; i++)
			bright_lookup[i] = color_correct(i);
	}

	// update the palette based on the depth
	if (dirtypalette)
	{
		dirtypalette = 0;

		if (bitmap->depth == 8)
			update_palette_8();
		else if (bitmap->depth == 15 || bitmap->depth == 16)
			update_palette_16();
	}

	// update the bitmap we're drawing
	profiler_mark(PROFILER_BLIT);
	update_video_window(bitmap);
	profiler_mark(PROFILER_END);

	// if we're being dirty, reset
	if (use_dirty)
		init_dirty(0);

	// if we're throttling and autoframeskip is on, adjust
	if (throttle && autoframeskip && frameskip_counter == 0)
		update_autoframeskip();
}



//============================================================
//	osd_update_video_and_audio
//============================================================

void osd_update_video_and_audio(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap, int leds_status)
{
	// if the LEDs have changed, update them
	if (leds_old != leds_status)
	{
		leds_old = leds_status;
		osd_set_leds(leds_status);
	}

	// if this is the first time through, initialize the previous time value
	if (warming_up)
	{
		last_skipcount0_time = ticker() - (int)((double)FRAMESKIP_LEVELS * (double)TICKS_PER_SEC / video_fps);
		warming_up = 0;
	}

	// if this is the first frame in a sequence, adjust the base time for this frame
	if (frameskip_counter == 0)
		this_frame_base = last_skipcount0_time + (int)((double)FRAMESKIP_LEVELS * (double)TICKS_PER_SEC / video_fps);

	// if we're not skipping this frame, draw it
	if (!osd_skip_this_frame())
		render_frame(game_bitmap);

	// update the debugger
	if (debug_bitmap)
		update_debug_window(debug_bitmap);

	// check for inputs
	check_inputs();

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// poll the joystick values here
	process_events();
	win32_poll_input();
}



//============================================================
//	osd_set_gamma
//============================================================

void osd_set_gamma(float _gamma)
{
	// set the global gamma correction
	gamma_correct = _gamma;

	// mark the palette dirty, and set a flag to rebuild the brightness table
	mark_palette_dirty();
	dirty_bright = 1;
}



//============================================================
//	osd_get_gamma
//============================================================

float osd_get_gamma(void)
{
	return gamma_correct;
}



//============================================================
//	osd_set_brightness
//============================================================

void osd_set_brightness(int _brightness)
{
	// set the global brightness
	brightness = (float)_brightness * 0.01;

	// mark the palette dirty, and set a flag to rebuild the brightness table
	mark_palette_dirty();
	dirty_bright = 1;
}



//============================================================
//	osd_get_brightness
//============================================================

int osd_get_brightness(void)
{
	return (int)(brightness * 100.0);
}



//============================================================
//	osd_save_snapshot
//============================================================

void osd_save_snapshot(struct osd_bitmap *bitmap)
{
	save_screen_snapshot(bitmap);
}



//============================================================
//	osd_pause
//============================================================

void osd_pause(int paused)
{
	// tell the input system
	win32_pause_input(paused);

	// set the brightness adjustment based on whether or not we are paused
	if (paused)
		brightness_adjust = 0.65;
	else
		brightness_adjust = 1.0;

	// mark the palette dirty, and set a flag to rebuild the brightness table
	mark_palette_dirty();
	dirty_bright = 1;
}
