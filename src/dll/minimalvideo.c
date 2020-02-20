//============================================================
//
//	video.c - Win32 video handling
//
//============================================================

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "mamedbg.h"
#include "vidhrdw/vector.h"
#include "blit.h"
#include "video.h"
#include "rc.h"

#include "driver.h"
#include "core.h"
#include "vpintf.h"
#include "mame.h"

extern int g_fPause;
extern int g_iSyncFactor;
extern struct RunningMachine *Machine;
extern struct mame_display *current_display_ptr;

extern UINT8  g_raw_dmdbuffer[DMD_MAXY*DMD_MAXX];
extern UINT32 g_raw_colordmdbuffer[DMD_MAXY*DMD_MAXX];
extern UINT32 g_raw_dmdx;
extern UINT32 g_raw_dmdy;
extern UINT32 g_needs_DMD_update;
extern int g_cpu_affinity_mask;

extern char g_fShowWinDMD;

// from ticker.c
extern void uSleep(const UINT64 u);

//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
int frameskip;
int autoframeskip;

// speed throttling
int throttle = 1;
int fastfrms = 0;
int g_low_latency_throttle = 0;

// palette lookups
UINT8 palette_lookups_invalid;
UINT32 palette_16bit_lookup[65536];
UINT32 palette_32bit_lookup[65536];

// rotation
UINT8 blit_flipx;
UINT8 blit_flipy;
UINT8 blit_swapxy;

//============================================================
//	LOCAL VARIABLES
//============================================================

// screen info
char *screen_name;

// core video input parameters
static int video_width;
static int video_height;
static int video_depth;
static double video_fps;
static int video_attributes;

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

// timing measurements for throttling
static cycles_t last_skipcount0_time;
static cycles_t this_frame_base;
static int allow_sleep;

// average FPS calculation
static cycles_t start_time;
static cycles_t end_time;
static int frames_displayed;
static int frames_to_display;

// frameskipping
static int frameskip_counter;
static int frameskipadjust;

// game states that invalidate autoframeskip
static int game_was_paused;
static int game_is_paused;
static int debugger_was_visible;
//
//// frameskipping tables
//static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
//{
//	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
//	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
//	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
//	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
//	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
//	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
//	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
//	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
//	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
//	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
//	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
//	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
//};
//
//static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
//{
//	{ 1,1,1,1,1,1,1,1,1,1,1,1 },
//	{ 2,1,1,1,1,1,1,1,1,1,1,0 },
//	{ 2,1,1,1,1,0,2,1,1,1,1,0 },
//	{ 2,1,1,0,2,1,1,0,2,1,1,0 },
//	{ 2,1,0,2,1,0,2,1,0,2,1,0 },
//	{ 2,0,2,1,0,2,0,2,1,0,2,0 },
//	{ 2,0,2,0,2,0,2,0,2,0,2,0 },
//	{ 2,0,2,0,0,3,0,2,0,0,3,0 },
//	{ 3,0,0,3,0,0,3,0,0,3,0,0 },
//	{ 4,0,0,0,4,0,0,0,4,0,0,0 },
//	{ 6,0,0,0,0,0,6,0,0,0,0,0 },
//	{12,0,0,0,0,0,0,0,0,0,0,0 }
//};
//


//============================================================
//	OPTIONS
//============================================================
//
//// prototypes
//static int decode_cleanstretch(struct rc_option *option, const char *arg, int priority);
//static int video_set_resolution(struct rc_option *option, const char *arg, int priority);
//static int decode_effect(struct rc_option *option, const char *arg, int priority);
//static int decode_aspect(struct rc_option *option, const char *arg, int priority);
//static void update_visible_area(struct mame_display *display);

// internal variables
static char *cleanstretch;
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
	//{ "waitvsync", NULL, rc_bool, &win_wait_vsync, "0", 0, 0, NULL, "wait for vertical sync (reduces tearing)"},
	//{ "triplebuffer", "tb", rc_bool, &win_triple_buffer, "0", 0, 0, NULL, "triple buffering (only if fullscreen)" },
#ifdef VPINMAME
	// default window mode for VPM is run inside a window
	{ "window", "w", rc_bool, &win_window_mode, "1", 0, 0, NULL, "run in a window/run on full screen" },
	{ "ddraw", "dd", rc_bool, &win_use_ddraw, "1", 0, 0, NULL, "use DirectDraw for rendering" },
#else
	// default window mode for PinMAME is full screen
	//{ "window", "w", rc_bool, &win_window_mode, "0", 0, 0, NULL, "run in a window/run on full screen" },
	//{ "ddraw", "dd", rc_bool, &win_use_ddraw, "0", 0, 0, NULL, "use DirectDraw for rendering" },
#endif
	//{ "direct3d", "d3d", rc_bool, &win_use_d3d, "0", 0, 0, NULL, "use Direct3D for rendering" },
	//{ "hwstretch", "hws", rc_bool, &win_dd_hw_stretch, "1", 0, 0, NULL, "(dd) stretch video using the hardware" },
	//{ "screen", NULL, rc_string, &screen_name, NULL, 0, 0, NULL, "specify which screen to use" },
	//{ "cleanstretch", "cs", rc_string, &cleanstretch, "auto", 0, 0, decode_cleanstretch, "stretch to integer ratios" },
	//{ "resolution", "r", rc_string, &resolution, "auto", 0, 0, video_set_resolution, "set resolution" },
	//{ "zoom", "z", rc_int, &win_gfx_zoom, "2", 1, 8, NULL, "force specific zoom level" },
	//{ "refresh", NULL, rc_int, &win_gfx_refresh, "0", 0, 0, NULL, "set specific monitor refresh rate" },
	//{ "scanlines", "sl", rc_bool, &win_old_scanlines, "0", 0, 0, NULL, "emulate win_old_scanlines" },
	//{ "switchres", NULL, rc_bool, &win_switch_res, "1", 0, 0, NULL, "switch resolutions to best fit" },
	//{ "switchbpp", NULL, rc_bool, &win_switch_bpp, "1", 0, 0, NULL, "switch color depths to best fit" },
	//{ "maximize", "max", rc_bool, &win_start_maximized, "1", 0, 0, NULL, "start out maximized" },
	//{ "keepaspect", "ka", rc_bool, &win_keep_aspect, "1", 0, 0, NULL, "enforce aspect ratio" },
	//{ "matchrefresh", NULL, rc_bool, &win_match_refresh, "0", 0, 0, NULL, "attempt to match the game's refresh rate" },
	//{ "syncrefresh", NULL, rc_bool, &win_sync_refresh, "0", 0, 0, NULL, "syncronize only to the monitor refresh" },
	//{ "throttle", NULL, rc_bool, &throttle, "1", 0, 0, NULL, "throttle speed to the game's framerate" },
	//{ "full_screen_brightness", "fsb", rc_float, &win_gfx_brightness, "0.0", 0.0, 4.0, NULL, "sets the brightness in full screen mode" },
	//{ "frames_to_run", "ftr", rc_int, &frames_to_display, "0", 0, 0, NULL, "sets the number of frames to run within the game" },
	//{ "effect", NULL, rc_string, &effect, "none", 0, 0, decode_effect, "specify the blitting effect" },
	//{ "screen_aspect", NULL, rc_string, &aspect, "4:3", 0, 0, decode_aspect, "specify an alternate monitor aspect ratio" },
	//{ "sleep", NULL, rc_bool, &allow_sleep, "1", 0, 0, NULL, "allow " APPNAME " to give back time to the system when it's not needed" },
	//{ "rdtsc", NULL, rc_bool, &win_force_rdtsc, "0", 0, 0, NULL, "prefer RDTSC over QueryPerformanceCounter for timing" },
	//{ "high_priority", NULL, rc_bool, &win_high_priority, "0", 0, 0, NULL, "increase thread priority" },
	//{ NULL, NULL, rc_link, win_d3d_opts, NULL, 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};


//============================================================
//	osd_pause
//============================================================

void osd_pause(int paused)
{
	// note that we were paused during this autoframeskip cycle
	game_is_paused = paused;
	if (game_is_paused)
		game_was_paused = 1;

	//// tell the input system
	//win_pause_input(paused);
}


/******************************************************************************

Display

******************************************************************************/

/*
Create a display screen, or window, of the given dimensions (or larger). It is
acceptable to create a smaller display if necessary, in that case the user must
have a way to move the visibility window around.

The params contains all the information the
Attributes are the ones defined in driver.h, they can be used to perform
optimizations, e.g. dirty rectangle handling if the game supports it, or faster
blitting routines with fixed palette if the game doesn't change the palette at
run time. The VIDEO_PIXEL_ASPECT_RATIO flags should be honored to produce a
display of correct proportions.
Orientation is the screen orientation (as defined in driver.h) which will be done
by the core. This can be used to select thinner screen modes for vertical games
(ORIENTATION_SWAP_XY set), or even to ask the user to rotate the monitor if it's
a pivot model. Note that the OS dependent code must NOT perform any rotation,
this is done entirely in the core.
Depth can be 8 or 16 for palettized modes, meaning that the core will store in the
bitmaps logical pens which will have to be remapped through a palette at blit time,
and 15 or 32 for direct mapped modes, meaning that the bitmaps will contain RGB
triplets (555 or 888). For direct mapped modes, the VIDEO_RGB_DIRECT flag is set
in the attributes field.

Returns 0 on success.
*/
int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components)
{
	return 0;
}

void osd_close_display(void)
{
}


/*
osd_skip_this_frame() must return 0 if the current frame will be displayed.
This can be used by drivers to skip cpu intensive processing for skipped
frames, so the function must return a consistent result throughout the
current frame. The function MUST NOT check timers and dynamically determine
whether to display the frame: such calculations must be done in
osd_update_video_and_audio(), and they must affect the FOLLOWING frames, not
the current one. At the end of osd_update_video_and_audio(), the code must
already know exactly whether the next frame will be skipped or not.
*/
int osd_skip_this_frame(void)
{
	return 0;
}


/*
Update video and audio. game_bitmap contains the game display, while
debug_bitmap an image of the debugger window (if the debugger is active; NULL
otherwise). They can be shown one at a time, or in two separate windows,
depending on the OS limitations. If only one is shown, the user must be able
to toggle between the two by pressing IPT_UI_TOGGLE_DEBUG; moreover,
osd_debugger_focus() will be used by the core to force the display of a
specific bitmap, e.g. the debugger one when the debugger becomes active.

leds_status is a bitmask of lit LEDs, usually player start lamps. They can be 
simulated using the keyboard LEDs, or in other ways e.g. by placing graphics
on the window title bar.
*/
//#include "vpintf.h"
void osd_update_video_and_audio(struct mame_display *display)
{
	//printf("g_needs_DMD_update:%d\n", g_needs_DMD_update);
	//printf("osd_update_video_and_audio: %dx%d \n", g_raw_dmdx, g_raw_dmdy);

	//if((int)g_raw_dmdx>0 && (int)g_raw_dmdy>0)
	//{
	//	for (int j = 0; j < g_raw_dmdy; j++)
	//	{
	//		for (int i = 0; i < 100;i++)//g_raw_dmdx; i++)
	//			printf("%s", (g_raw_dmdbuffer[j * g_raw_dmdx + i] > 20 ? "X" : " "));
	//		printf("\n");
	//	}
	//}
	//printf("\n");

	//printf("\r            %d     \r", g_raw_dmdbuffer[20*132 + 160]);
	vp_tChgLamps chgLamps;

	/////*-- Count changes --*/
	int uCount = vp_getChangedLamps(chgLamps);
	//if(uCount > 0)
	//	printf("%d changed\n", uCount);
}


/*
Provides a hook to allow the OSD system to override processing of a
snapshot.  This function will either return a new bitmap, for which the
caller is responsible for freeing.
*/
struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap, struct rectangle *bounds)
{
	struct rectangle newbounds;
	struct mame_bitmap *copy;
	int x, y, w, h, t;

	// if we can send it in raw, no need to override anything
	if (!blit_swapxy && !blit_flipx && !blit_flipy)
		return NULL;

	// allocate a copy
	w = blit_swapxy ? bitmap->height : bitmap->width;
	h = blit_swapxy ? bitmap->width : bitmap->height;
	copy = bitmap_alloc_depth(w, h, bitmap->depth);
	if (!copy)
		return NULL;

	// populate the copy
	for (y = bounds->min_y; y <= bounds->max_y; y++)
		for (x = bounds->min_x; x <= bounds->max_x; x++)
		{
			int tx = x, ty = y;

			// apply the rotation/flipping
			if (blit_swapxy)
			{
				t = tx; tx = ty; ty = t;
			}
			if (blit_flipx)
				tx = copy->width - tx - 1;
			if (blit_flipy)
				ty = copy->height - ty - 1;

			// read the old pixel and copy to the new location
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

	// compute the oriented bounds
	newbounds = *bounds;

	// apply X/Y swap first
	if (blit_swapxy)
	{
		t = newbounds.min_x; newbounds.min_x = newbounds.min_y; newbounds.min_y = t;
		t = newbounds.max_x; newbounds.max_x = newbounds.max_y; newbounds.max_y = t;
	}

	// apply X flip
	if (blit_flipx)
	{
		t = copy->width - newbounds.min_x - 1;
		newbounds.min_x = copy->width - newbounds.max_x - 1;
		newbounds.max_x = t;
	}

	// apply Y flip
	if (blit_flipy)
	{
		t = copy->height - newbounds.min_y - 1;
		newbounds.min_y = copy->height - newbounds.max_y - 1;
		newbounds.max_y = t;
	}

	*bounds = newbounds;
	return copy;
}

/*
Returns a pointer to the text to display when the FPS display is toggled.
This normally includes information about the frameskip, FPS, and percentage
of full game speed.
*/
const char *osd_get_fps_text(const struct performance_info *performance)
{
	return "NO FPS";
}

void throttle_speed_part(int part, int totalparts)
{
}

void SetThrottleAdj(int Adj)
{
}