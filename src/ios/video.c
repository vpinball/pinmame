#include <math.h>
#include <unistd.h>
#include "driver.h"
#include "vidhrdw/vector.h"
#include "video.h"

#include "ios.h"

extern UINT8 win_trying_to_quit;

#define FRAMESKIP_LEVELS			12

int frameskip = 0;
int autoframeskip = 1;
int throttle = 1;
static int allow_sleep = 1;
static int video_depth;
static double video_fps;
static int vector_game;
static int rgb_direct;
static int warming_up;
static cycles_t last_skipcount0_time;
static cycles_t this_frame_base = 0;
static cycles_t start_time;
static cycles_t end_time;
static int frames_displayed;
static int frames_to_display;
static int frameskip_counter;
static int frameskipadjust;
static int game_was_paused;
static int game_is_paused;
static int debugger_was_visible;
UINT8 palette_lookups_invalid;
UINT32 palette_16bit_lookup[65536];
UINT32 palette_32bit_lookup[65536];
UINT8* video_buffer;

/**
 * Frameskipping tables
 */

static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] = {
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

/**
 * 16bpp color conversion
 */

#define WIN_COLOR16_RSRC_SHIFT	3
#define WIN_COLOR16_GSRC_SHIFT	3
#define WIN_COLOR16_BSRC_SHIFT	3
#define WIN_COLOR16_RDST_SHIFT	10
#define WIN_COLOR16_GDST_SHIFT	5
#define WIN_COLOR16_BDST_SHIFT	0

#define WIN_COLOR16( _r__, _g__, _b__ ) \
(UINT16)( (( ((UINT8)(_r__)) >> WIN_COLOR16_RSRC_SHIFT) << WIN_COLOR16_RDST_SHIFT) | \
(( ((UINT8)(_g__)) >> WIN_COLOR16_GSRC_SHIFT) << WIN_COLOR16_GDST_SHIFT) | \
(( ((UINT8)(_b__)) >> WIN_COLOR16_BSRC_SHIFT) << WIN_COLOR16_BDST_SHIFT) )

/**
 * 32bpp color conversion
 */

#define WIN_COLOR32_RDST_SHIFT	16
#define WIN_COLOR32_GDST_SHIFT	8
#define WIN_COLOR32_BDST_SHIFT	0

#define WIN_COLOR32( _r__, _g__, _b__ ) \
(UINT32)( (((UINT8)(_r__)) << WIN_COLOR32_RDST_SHIFT) | \
(((UINT8)(_g__)) << WIN_COLOR32_GDST_SHIFT) | \
(((UINT8)(_b__)) << WIN_COLOR32_BDST_SHIFT) )

/**
 * update_palette()
 */

void update_palette(struct mame_display* display) {
	int i, j;
    
	for (i = 0; i < display->game_palette_entries; i += 32)	{
		UINT32 dirtyflags = palette_lookups_invalid ? ~0 : display->game_palette_dirty[i / 32];
 		
        if (dirtyflags)	{
			display->game_palette_dirty[i / 32] = 0;
            
			for (j = 0; (j < 32) && (i+j < display->game_palette_entries); j++, dirtyflags >>= 1) {
				if (dirtyflags & 1) {
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);
                    
					palette_16bit_lookup[i + j] = WIN_COLOR16(r, g, b) * 0x10001;
                    palette_32bit_lookup[i + j] = WIN_COLOR32(r, g, b);
                }
            }
		}
	}
    
	palette_lookups_invalid = 0;
}

/**
 * update_visible_area
 */

static void update_visible_area(struct mame_display *display) {
	set_ui_visarea(display->game_visible_area.min_x, display->game_visible_area.min_y,
                   display->game_visible_area.max_x, display->game_visible_area.max_y);
}

/**
 * check_inputs()
 */ 

void check_inputs(void) {
	if (input_ui_pressed(IPT_UI_FRAMESKIP_INC)) {
		if (autoframeskip) {
			autoframeskip = 0;
			frameskip = 0;
		}
		else if (frameskip == FRAMESKIP_LEVELS - 1) {
			frameskip = 0;
			autoframeskip = 1;
		}
		else {
			frameskip++;
        }
        
		ui_show_fps_temp(2.0);
	}
    
	if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC)) {
		if (autoframeskip) {
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS - 1;
		}
		else if (frameskip == 0) {
			autoframeskip = 1;
        }
        else {
			frameskip--;
        }
        
		ui_show_fps_temp(2.0);
	}
    
	if (input_ui_pressed(IPT_UI_THROTTLE)) {
		throttle ^= 1;
	}
}

/**
 * update_autoframeskip()
 */

void update_autoframeskip(void) {
	if (!game_was_paused && !debugger_was_visible && cpu_getcurrentframe() > 2 * FRAMESKIP_LEVELS) {
		const struct performance_info *performance = mame_get_performance_info();
        
		if (performance->game_speed_percent >= 99.5) {
			frameskipadjust++;
            
			if (frameskipadjust >= 3) {
				frameskipadjust = 0;
				if (frameskip > 0) {
                    frameskip--;
                }
			}
		}        
		else {
			if (performance->game_speed_percent < 80) {
				frameskipadjust -= (90 - performance->game_speed_percent) / 5;
            }
			else if (frameskip < 8) {
				frameskipadjust--;
            }
            
			while (frameskipadjust <= -2) {
				frameskipadjust += 2;
				if (frameskip < FRAMESKIP_LEVELS - 1) {
					frameskip++;
                }
			}
		}
	}
    
	game_was_paused = game_is_paused;
	debugger_was_visible = 0;
}

void throttle_speed_part(int part, int totalparts)
{
    //!! TODO defined in video.c windows, needs to be implemented
}

int g_low_latency_throttle = 0;

int g_iThrottleAdj = 0;

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


/**
 * throttle_speed
 */

static void throttle_speed(void) {
	static double ticks_per_sleep_msec = 0;
	cycles_t target;
    cycles_t curr;
    cycles_t cps;
    
	profiler_mark(PROFILER_IDLE);
    
	curr = osd_cycles();
	cps = osd_cycles_per_second();
	target = this_frame_base + (int)((double)frameskip_counter * (double)cps / video_fps);
    
	if (curr - target < 0) {
		if (ticks_per_sleep_msec == 0) {
			ticks_per_sleep_msec = (double)(cps / 1000);
        }
        
		while (curr - target < 0) {
			if (allow_sleep && (!autoframeskip || frameskip == 0) &&
				(target - curr) > (cycles_t)(ticks_per_sleep_msec * 1.1)) {
				cycles_t next;
                
				usleep(1000);
				next = osd_cycles();
				ticks_per_sleep_msec = (ticks_per_sleep_msec * 0.90) + ((double)(next - curr) * 0.10);
				curr = next;
			}
			else {
				curr = osd_cycles();
			}
		}
	}
    
	profiler_mark(PROFILER_END);
}

/**
 * render_frame()
 */ 

static void render_frame(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels) {
    cycles_t curr;
    
	if (throttle || game_is_paused) {
		throttle_speed();
    }
    
	curr = osd_cycles();
    
	if (start_time == 0) {
		if (timer_get_time() > 1.0) {
			start_time = curr;
        }
	}
	else {
		frames_displayed++;
        
		if (frames_displayed + 1 == frames_to_display) {
			win_trying_to_quit = 1;
        }
        
		end_time = curr;
	}
    
	if (frameskip_counter == 0) {
		last_skipcount0_time = curr;
    }
    
	profiler_mark(PROFILER_BLIT);
	
    //OLD SCHOOL 32 BIT MATH
    //UINT16* source = (bitmap->line[y]) + (x*(bitmap->depth/8));
    //UINT32* dest = (video_buffer + (x*4) + (y*bitmap->width*4));
    //*dest = palette_32bit_lookup[*source];

    if (video_depth == 32) {
        for (int y = 0; y < bitmap->height; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                UINT16* source = (bitmap->line[y]) + (x*(bitmap->depth >> 3));
                UINT32* dest = (video_buffer + (x << 2) + (y*bitmap->width << 2));
                *dest = palette_32bit_lookup[*source];
            }
        }
    }
    else {
        for (int y = 0; y < bitmap->height; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                UINT16* source = (bitmap->line[y]) + (x*(bitmap->depth >> 3));
                UINT16* dest = (video_buffer + (x << 1) + (y*bitmap->width << 1));
                *dest = palette_16bit_lookup[*source];
            }
        }
    }
            
    ipinmame_update_display();
    
	profiler_mark(PROFILER_END);
    
	if (throttle && autoframeskip && (frameskip_counter == 0)) {
		update_autoframeskip();
    }
}

/**
 * osd_create_display()
 */

int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components) {
    int r, g, b;
    
    ipinmame_logger("osd_create_display(): enter - width=%d, height=%d, depth=%d, fps=%f", params->width, params->height, params->depth, params->fps);
    
    video_depth = params->depth;
    video_fps = params->fps;
    
    if (frameskip < 0) {
        frameskip = 0;
    }
    else if (frameskip > FRAMESKIP_LEVELS) {
        frameskip = FRAMESKIP_LEVELS - 1;
    }
    
    vector_game	= ((params->video_attributes & VIDEO_TYPE_VECTOR) != 0);
	rgb_direct = ((params->video_attributes & VIDEO_RGB_DIRECT) != 0);
    
    for (r = 0; r < 32; r++) {
        for (g = 0; g < 32; g++) {
            for (b = 0; b < 32; b++) {
                int idx = (r << 10) | (g << 5) | b;
                int rr = (r << 3) | (r >> 2);
                int gg = (g << 3) | (g >> 2);
                int bb = (b << 3) | (b >> 2);
                palette_16bit_lookup[idx] = WIN_COLOR16(rr, gg, bb) * 0x10001;
                palette_32bit_lookup[idx] = WIN_COLOR32(rr, gg, bb);
            }
        }
    }
    
    if (rgb_components) {
		if (video_depth == 32) {
			rgb_components[0] = WIN_COLOR32(0xff, 0x00, 0x00);
			rgb_components[1] = WIN_COLOR32(0x00, 0xff, 0x00);
			rgb_components[2] = WIN_COLOR32(0x00, 0x00, 0xff);
		}
		else {
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;
		}
	}
    
    video_buffer = ipinmame_init_video(params->width, params->height, video_depth);
    
    warming_up = 1;
       
    ipinmame_logger("osd_create_display(): exit");
    
    return 0;
}

/**
 * osd_close_display
 */

void osd_close_display(void) {
    ipinmame_logger("osd_close_display(): enter");
    
    //win_destroy_window();
    
	if (frames_displayed != 0) {
		cycles_t cps = osd_cycles_per_second();
		ipinmame_logger("osd_close_display(): average FPS: %f (%d frames)", (double)cps / (end_time - start_time) * frames_displayed, frames_displayed);
	}
    
    ipinmame_logger("osd_close_display(): exit");
}

/**
 * osd_skip_this_frame
 */

int osd_skip_this_frame(void) {
    return skiptable[frameskip][frameskip_counter];
}

/**
 * osd_get_fps_text
 */

const char* osd_get_fps_text(const struct performance_info *performance) {
	static char buffer[1024];
	char *dest = buffer;
    
	dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps",
                    autoframeskip ? "auto" : "fskp", frameskip,
                    (int)(performance->game_speed_percent + 0.5),
                    (int)(performance->frames_per_second + 0.5),
                    (int)(Machine->drv->frames_per_second + 0.5));
    
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) {
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1) {
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}
    
	return buffer;
}

/**
 * osd_update_video_and_audio()
 */ 

void osd_update_video_and_audio(struct mame_display* display) {
	struct rectangle updatebounds = display->game_bitmap_update;
	cycles_t cps = osd_cycles_per_second();
    
	if (warming_up) {
		last_skipcount0_time = osd_cycles() - (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);
		warming_up = 0;
	}
    
	if (frameskip_counter == 0) {
		this_frame_base = last_skipcount0_time + (int)((double)FRAMESKIP_LEVELS * (double)cps / video_fps);
    }
    
    if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED) {
        update_visible_area(display);
    }
    
    if (display->changed_flags & GAME_PALETTE_CHANGED) {
    	update_palette(display);
    }
    
    if (display->changed_flags & GAME_BITMAP_CHANGED) {
		if (display->changed_flags & VECTOR_PIXELS_CHANGED) {
			render_frame(display->game_bitmap, &updatebounds, display->vector_dirty_pixels);
		}
        else {
			render_frame(display->game_bitmap, &updatebounds, NULL);
        }
    }
    
    if (display->changed_flags & LED_STATE_CHANGED) {
		// osd_set_leds(display->led_state);
    }
    
    frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;
    
    check_inputs();
}

/**
 * osd_override_snapshot
 */

struct mame_bitmap* osd_override_snapshot(struct mame_bitmap *bitmap, struct rectangle *bounds) {
    return NULL;
}

/**
 * osd_pause
 */

void osd_pause(int paused) {
	game_is_paused = paused; 
    
	if (game_is_paused) {
		game_was_paused = 1;
    }
}
