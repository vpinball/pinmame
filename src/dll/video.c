//============================================================
//
//	video.c - Win32 video handling
//
//============================================================
// standard C headers

// MAME headers
#include "driver.h"
#include "mamedbg.h"
//#include "vidhrdw/vector.h"
//#include "blit.h"
#include "video.h"
//#include "window.h"
#include "rc.h"

#ifdef MESS
#include "menu.h"
#endif


//============================================================
//	IMPORTS
//============================================================

// from input.c
extern UINT8 win_trying_to_quit;

// from ticker.c
extern void uSleep(const UINT64 u);
extern void uOverSleep(const UINT64 u);
extern void uUnderSleep(const UINT64 u);

//============================================================
//	PARAMETERS
//============================================================

// frameskipping
#define FRAMESKIP_LEVELS			12



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
static double video_fps;

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

/*static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
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
	{ 12,0,0,0,0,0,0,0,0,0,0,0 }
};*/



//============================================================
//	OPTIONS
//============================================================

// internal variables
//static char *cleanstretch;
//static char *resolution;
//static char *effect;
//static char *aspect;

// options struct
struct rc_option video_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "autoframeskip", "afs", rc_bool, &autoframeskip, "1", 0, 0, NULL, "skip frames to speed up emulation" },
	{ "frameskip", "fs", rc_int, &frameskip, "0", 0, 12, NULL, "set frameskip explicitly (autoframeskip needs to be off)" },
	//{ "waitvsync", NULL, rc_bool, &win_wait_vsync, "0", 0, 0, NULL, "wait for vertical sync (reduces tearing)" },
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
	{ "screen", NULL, rc_string, &screen_name, NULL, 0, 0, NULL, "specify which screen to use" },
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
	{ "throttle", NULL, rc_bool, &throttle, "1", 0, 0, NULL, "throttle speed to the game's framerate" },
	//{ "full_screen_brightness", "fsb", rc_float, &win_gfx_brightness, "0.0", 0.0, 4.0, NULL, "sets the brightness in full screen mode" },
	{ "frames_to_run", "ftr", rc_int, &frames_to_display, "0", 0, 0, NULL, "sets the number of frames to run within the game" },
	//{ "effect", NULL, rc_string, &effect, "none", 0, 0, decode_effect, "specify the blitting effect" },
	//{ "screen_aspect", NULL, rc_string, &aspect, "4:3", 0, 0, decode_aspect, "specify an alternate monitor aspect ratio" },
	{ "sleep", NULL, rc_bool, &allow_sleep, "1", 0, 0, NULL, "allow " APPNAME " to give back time to the system when it's not needed" },
	//{ "rdtsc", NULL, rc_bool, &win_force_rdtsc, "0", 0, 0, NULL, "prefer RDTSC over QueryPerformanceCounter for timing" },
	//{ "high_priority", NULL, rc_bool, &win_high_priority, "0", 0, 0, NULL, "increase thread priority" },
	//	{ NULL, NULL, rc_link, win_d3d_opts, NULL, 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

//============================================================
//	osd_create_display
//============================================================

int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components)
{
	printf("osd_create_display: %.2f fps\n", params->fps);

	//	logerror("width %d, height %d depth %d\n", params->width, params->height, params->depth);

	// copy the parameters into globals for later use
	video_fps = params->fps;

	// clamp the frameskip value to within range
	if (frameskip < 0)
		frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS)
		frameskip = FRAMESKIP_LEVELS - 1;

	// indicate for later that we're just beginning
	warming_up = 1;
	return 0;
}



//============================================================
//	osd_close_display
//============================================================

void osd_close_display(void)
{
	// print a final result to the stdout
	if (frames_displayed != 0)
	{
		cycles_t cps = osd_cycles_per_second();
		printf("Average FPS: %f (%d frames)\n", (double)cps / (end_time - start_time) * frames_displayed, frames_displayed);
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
//	osd_get_fps_text
//============================================================

const char *osd_get_fps_text(const struct performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;

	// display the FPS, frameskip, percent, fps and target fps
	dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
		autoframeskip ? "auto" : "fskp", frameskip,
		(int)(performance->game_speed_percent + 0.5),
		(int)(performance->frames_per_second + 0.5),
		(int)(Machine->drv->frames_per_second + 0.5));

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



//============================================================
//	check_inputs
//============================================================

static void check_inputs(void)
{
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
		ui_show_fps_temp(2.0);

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
			frameskip = FRAMESKIP_LEVELS - 1;
		}

		// wrap from 0 to auto
		else if (frameskip == 0)
			autoframeskip = 1;

		// else just decrement
		else
			frameskip--;

		// display the FPS counter for 2 seconds
		ui_show_fps_temp(2.0);

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


#ifdef MESS
	// check for toggling menu bar
	if (input_ui_pressed(IPT_OSD_2))
		win_toggle_menubar();
#endif
}



//============================================================
//	throttle_speed
//============================================================
#ifdef VPINMAME
extern HANDLE g_hEnterThrottle;
extern int    g_iSyncFactor;
int           iCurrentSyncValue = 512;
#endif
int    g_iThrottleAdj = 0;


#ifdef DEBUG_SOUND
void DebugSound(char *s)
{
	FILE *stream = fopen("C:\\temp\\sndlog.txt", "a");
	fprintf(stream, "%s\n", s);
	fclose(stream);
}
#endif

void SetThrottleAdj(int adj)
{
#ifdef DEBUG_SOUND
	char tmp[81];

	static int last = 0;
	if (adj != last)
	{
		sprintf(tmp, "Set throttle adj: %d (cur %d)", adj, g_iThrottleAdjCur);
		DebugSound(tmp);
		last = adj;
	}
#endif
	g_iThrottleAdj = adj;
}

static void throttle_speed()
{
	throttle_speed_part(1, 1);
}

// Throttle code changed to support parital frame syncing.
// The emulated machine can often read, and respond to input by firing flippers in less than 10ms, but
// if the emulation only runs in 60hz "chunks", we may need multiple frames to read and respond 
// to flipper input.  By distributing the emulation more evenly over a frame, it creates more opportunities
// for the emulated machine to "see" the input and respond to it before the pinball simulator starts to draw its frame.

void throttle_speed_part(int part, int totalparts)
{
	static double ticks_per_sleep_msec = 0;
	cycles_t target, curr, cps;

#ifdef VPINMAME
	if ((g_hEnterThrottle != INVALID_HANDLE_VALUE) && g_iSyncFactor) {
		if (g_iSyncFactor >= 1024)
			SetEvent(g_hEnterThrottle);
		else {
			iCurrentSyncValue += g_iSyncFactor;
			if (iCurrentSyncValue >= 1024) {
				SetEvent(g_hEnterThrottle);
				iCurrentSyncValue -= 1024;
			}
		}
	}
#endif

	//// if we're only syncing to the refresh, bail now
	//if (win_sync_refresh)
	//	return;

	// this counts as idle time
	profiler_mark(PROFILER_IDLE);

	// get the current time and the target time
	curr = osd_cycles();
	cps = osd_cycles_per_second();

	target = this_frame_base + (int)((double)frameskip_counter * (double)cps / video_fps);

	// If we are throttling to a fractional vsync, adjust target to the partial target.
	if (totalparts != 1)
	{
		// Meh.  The points in the code where frameskip counter gets updated is different from where the frame base is
		// reset.  Makes this delay computation complicated.
		if (frameskip_counter == 0)
			target += (int)((double)(FRAMESKIP_LEVELS) * (double)cps / video_fps);
		// MAGIC: Experimentation with actual resuts show the most even distribution if I throttle to 1/7th increments at each 25% timestep.
		target -= ((cycles_t)((double)cps / (video_fps * (totalparts + 3)))) * (totalparts - part + 3);
	}

	// initialize the ticks per sleep
	if (ticks_per_sleep_msec == 0)
		ticks_per_sleep_msec = (double)cps / 1000.;

	// Adjust target for sound catchup
	if (g_iThrottleAdj)
	{
		target -= (cycles_t)(g_iThrottleAdj*ticks_per_sleep_msec);
	}
	// sync
	if (curr - target < 0)
	{
#ifdef DEBUG_THROTTLE
		{
			char tmp[91];
			sprintf(tmp, "Throt: part %d of %d FS: %d Delta: %lld\n", part, totalparts, frameskip_counter, curr - target);
			OutputDebugString(tmp);
		}
#endif

		// loop until we reach the target time
		while (curr - target < 0)
		{
#if 1 // VPINMAME
			//if((INT64)((target - curr)/(ticks_per_sleep_msec*1.1))-1 > 0) // pessimistic estimate of stuff below, but still stutters then
			//	uSleep((UINT64)((target - curr)*1000/(ticks_per_sleep_msec*1.1))-1);
			if (totalparts > 1)
				uUnderSleep((UINT64)((target - curr) * 1000 / ticks_per_sleep_msec)); // will sleep too short
			else
				uOverSleep((UINT64)((target - curr) * 1000 / ticks_per_sleep_msec)); // will sleep too long
#else
			// if we have enough time to sleep, do it
			// ...but not if we're autoframeskipping and we're behind
			if (allow_sleep && (!autoframeskip || frameskip == 0) &&
				(target - curr) > (cycles_t)(ticks_per_sleep_msec * 1.1))
			{
				cycles_t next;

				// keep track of how long we actually slept
				uSleep(100); //1000?
				next = osd_cycles();
				ticks_per_sleep_msec = (ticks_per_sleep_msec * 0.90) + ((double)(next - curr) * 0.10);
				curr = next;
			}
			else
#endif
			{
				// update the current time
				curr = osd_cycles();
			}
		}
	}
	else if (curr - target >= (int)(cps / video_fps) && totalparts == 1)
	{
		// We're behind schedule by a frame or more.  Something must
		// have taken longer than it should have (e.g., a CPU emulator
		// time slice must have gone on too long).  We don't have a
		// time machine, so we can't go back and sync this frame to a
		// time in the past, but we can at least sync up the current
		// frame with the current real time.
		//
		// Note that the 12-frame "skip" cycle would eventually get
		// things back in sync even without this adjustment, but it
		// can cause audio glitching if we wait until then.  The skip
		// cycle will try to make up for the lost time by giving shorter
		// time slices to the next batch of 12 frames, but the way it
		// does its calculation, the time taken out of those short
		// frames will pile up in the *next next* skip cycle, causing
		// a long (order of 100ms) pause that can manifset as an audio
		// glitch and/or video hiccup.
		//
		// The adjustment here is simply the amount of real time by
		// which we're behind schedule.  Add this to the base time,
		// since the real time for this frame is later than we expected.
		this_frame_base += curr - target;
	}

	// idle time done
	profiler_mark(PROFILER_END);
}

//============================================================
//	update_autoframeskip
//============================================================

void update_autoframeskip(void)
{
	// don't adjust frameskip if we're paused or if the debugger was
	// visible this cycle or if we haven't run yet
	if (!game_was_paused && !debugger_was_visible && cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS)
	{
		const struct performance_info *performance = mame_get_performance_info();

		// if we're too fast, attempt to increase the frameskip
		if (performance->game_speed_percent >= 99.5)
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
			if (performance->game_speed_percent < 80)
				frameskipadjust -= (int)((90. - performance->game_speed_percent) / 5.);

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

	// clear the other states
	game_was_paused = game_is_paused;
	debugger_was_visible = 0;
}



//============================================================
//	render_frame
//============================================================

static void update_timing()
{
	cycles_t curr;

	if (fastfrms >= 0)
	{
		if (fastfrms-- == 0) throttle = 1;
		else throttle = 0;
	}

	// if we're throttling, synchronize
	if (throttle || game_is_paused)
		throttle_speed();

	// at the end, we need the current time
	curr = osd_cycles();

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
			win_trying_to_quit = 1;
		end_time = curr;
	}

	// if we're at the start of a frameskip sequence, compute the speed
	if (frameskip_counter == 0)
		last_skipcount0_time = curr;

	//// update the bitmap we're drawing
	//profiler_mark(PROFILER_BLIT);
	//win_update_video_window(bitmap, bounds, vector_dirty_pixels);
	//profiler_mark(PROFILER_END);

	// if we're throttling and autoframeskip is on, adjust
	if (throttle && autoframeskip && frameskip_counter == 0)
		update_autoframeskip();
}



//============================================================
//	osd_update_video_and_audio
//============================================================

void osd_update_video_and_audio(struct mame_display *display)
{
	cycles_t cps = osd_cycles_per_second();

	// if this is the first time through, initialize the previous time value
	if (warming_up)
	{
		last_skipcount0_time = osd_cycles() - (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);
		warming_up = 0;
	}

	// if this is the first frame in a sequence, adjust the base time for this frame
	if (frameskip_counter == 0)
		this_frame_base = last_skipcount0_time + (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);

	// if we're not skipping this frame, draw it
	if (display->changed_flags & GAME_BITMAP_CHANGED)
		update_timing();

	//// if the LEDs have changed, update them
	//if (display->changed_flags & LED_STATE_CHANGED)
	//	osd_set_leds(display->led_state);

	// increment the frameskip counter
	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	// check for inputs
	check_inputs();
}



//============================================================
//	osd_override_snapshot
//============================================================

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
