//============================================================
//
//	window.h - Win32 window handling
//
//============================================================

#ifndef __WIN32_WINDOW__
#define __WIN32_WINDOW__



//============================================================
//	GLOBAL VARIABLES
//============================================================

// command line config
extern int			window_mode;
extern int			wait_vsync;
extern int			use_ddraw;
extern int			use_triplebuf;
extern int			ddraw_stretch;
extern int			gfx_width;
extern int			gfx_height;
extern int			gfx_depth;
extern int			scanlines;
extern int			switchres;
extern int			switchbpp;
extern int			maximize;
extern int			keepaspect;
extern int			gfx_refresh;
extern int			matchrefresh;
extern int			syncrefresh;
extern float		gfx_brightness;
extern int			bliteffect;
extern float		screen_aspect;

// windows
extern HWND			video_window;
extern HWND			debug_window;

// 16bpp color conversion
extern int			color16_rsrc_shift;
extern int			color16_gsrc_shift;
extern int			color16_bsrc_shift;
extern int			color16_rdst_shift;
extern int			color16_gdst_shift;
extern int			color16_bdst_shift;

// 32bpp color conversion
extern int			color32_rdst_shift;
extern int			color32_gdst_shift;
extern int			color32_bdst_shift;



//============================================================
//	PROTOTYPES
//============================================================

int win32_init_window(void);
int create_window(int width, int height, int depth, int attributes, int orientation);
void destroy_window(void);
void update_cursor_state(void);
void toggle_maximize(void);
void toggle_full_screen(void);

void adjust_window_for_visible(int min_x, int max_x, int min_y, int max_y);
void wait_for_vsync(void);

void update_video_window(struct osd_bitmap *bitmap);
void update_debug_window(struct osd_bitmap *bitmap);

void set_palette_entry(int index, UINT8 red, UINT8 green, UINT8 blue);

void process_events(void);
void process_events_periodic(void);
void osd_set_leds(int state);
int osd_get_leds(void);

int lookup_effect(const char *arg);



//============================================================
//	color16
//============================================================

INLINE UINT16 color16(UINT8 r, UINT8 g, UINT8 b)
{
	return ((r >> color16_rsrc_shift) << color16_rdst_shift) |
		   ((g >> color16_gsrc_shift) << color16_gdst_shift) |
		   ((b >> color16_bsrc_shift) << color16_bdst_shift);
}

INLINE UINT8 red16(UINT16 color)
{
	int val = (color >> color16_rdst_shift) << color16_rsrc_shift;
	return val | (val >> (8 - color16_rsrc_shift));
}

INLINE UINT8 green16(UINT16 color)
{
	int val = (color >> color16_gdst_shift) << color16_gsrc_shift;
	return val | (val >> (8 - color16_gsrc_shift));
}

INLINE UINT8 blue16(UINT16 color)
{
	int val = (color >> color16_bdst_shift) << color16_bsrc_shift;
	return val | (val >> (8 - color16_bsrc_shift));
}



//============================================================
//	color32
//============================================================

INLINE UINT32 color32(UINT8 r, UINT8 g, UINT8 b)
{
	return (r << color32_rdst_shift) |
		   (g << color32_gdst_shift) |
		   (b << color32_bdst_shift);
}

INLINE UINT8 red32(UINT32 color)
{
	return color >> color32_rdst_shift;
}

INLINE UINT8 green32(UINT32 color)
{
	return color >> color32_gdst_shift;
}

INLINE UINT8 blue32(UINT32 color)
{
	return color >> color32_bdst_shift;
}

#endif
