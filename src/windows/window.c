//============================================================
//
//	window.c - Win32 window handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// undef WINNT for ddraw.h to prevent duplicate definition
#undef WINNT
#include <ddraw.h>

// missing stuff from the mingw headers
#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#define ENUM_REGISTRY_SETTINGS      ((DWORD)-2)
#endif

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "ticker.h"
#include "window.h"
#include "video.h"
#include "blit.h"
#include "../window.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win32_pause_input(int pause);
extern int is_mouse_captured(void);
extern UINT8 trying_to_quit;
extern int verbose;



//============================================================
//	DEBUGGING
//============================================================

#define SHOW_FLIP_TIMES 	0



//============================================================
//	PARAMETERS
//============================================================

// window styles
#define WINDOW_STYLE			WS_OVERLAPPEDWINDOW
#define WINDOW_STYLE_EX			0

// debugger window styles
#define DEBUG_WINDOW_STYLE		WS_OVERLAPPED
#define DEBUG_WINDOW_STYLE_EX	0

// full screen window styles
#define FULLSCREEN_STYLE		WS_OVERLAPPED
#define FULLSCREEN_STYLE_EX		WS_EX_TOPMOST

// menu items
#define MENU_FULLSCREEN			1000



//============================================================
//	TYPE DEFINITIONS
//============================================================

struct effect_data
{
	const char *name;
	int effect;
	int min_xscale;
	int min_yscale;
	int max_xscale;
	int max_yscale;
};



//============================================================
//	GLOBAL VARIABLES
//============================================================

// command line config
int	window_mode;
int	wait_vsync;
int	use_ddraw;
int	use_triplebuf;
int	ddraw_stretch;
int	gfx_width;
int	gfx_height;
int gfx_depth;
int scanlines;
int switchres;
int switchbpp;
int maximize;
int keepaspect;
int gfx_refresh;
int matchrefresh;
int syncrefresh;
float gfx_brightness;
int bliteffect;
float screen_aspect = (4.0 / 3.0);

// windows
HWND video_window;
HWND debug_window;

// 16bpp color conversion
int color16_rsrc_shift = 3;
int color16_gsrc_shift = 3;
int color16_bsrc_shift = 3;
int color16_rdst_shift = 10;
int color16_gdst_shift = 5;
int color16_bdst_shift = 0;

// 32bpp color conversion
int color32_rdst_shift = 16;
int color32_gdst_shift = 8;
int color32_bdst_shift = 0;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DIB bitmap data
static UINT8 video_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *video_dib_info = (BITMAPINFO *)video_dib_info_data;
static UINT8 debug_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *debug_dib_info = (BITMAPINFO *)debug_dib_info_data;
static UINT8 *converted_bitmap;

// DirectDraw objects
static LPDIRECTDRAW ddraw;
static LPDIRECTDRAW4 ddraw4;
static LPDIRECTDRAWGAMMACONTROL gamma_control;
static LPDIRECTDRAWSURFACE primary_surface;
static LPDIRECTDRAWSURFACE back_surface;
static LPDIRECTDRAWSURFACE blit_surface;
static LPDIRECTDRAWCLIPPER primary_clipper;
static LPDIRECTDRAWPALETTE primary_palette;

// palette data
static PALETTEENTRY primary_palette_data[256];
static int local_palette_dirty = 1;

// DirectDraw object info
static DDCAPS ddraw_caps;
static DDSURFACEDESC primary_desc;
static DDSURFACEDESC blit_desc;
static int changed_resolutions;
static int forced_updates;
static int visible_area_set;

// video bounds
static int max_width;
static int max_height;
static int pref_depth;
static double aspect_ratio;
static double aspect_ratio_adjust = 1.0;

// visible bounds
static RECT visible_rect;
static int visible_width;
static int visible_height;

// event handling
static TICKER last_event_check;

// derived attributes
static int swap_xy;
static int dual_monitor;
static int vector_game;
static int needs_6bpp_per_gun;
static int pixel_aspect_ratio;

// mode finding
static double best_score;
static int best_width;
static int best_height;
static int best_depth;
static int best_refresh;

// cached bounding rects
static RECT non_fullscreen_bounds;
static RECT non_maximized_bounds;

// debugger
static int debug_focus;

// effects table
static struct effect_data effect_table[] =
{
	{ "none",    EFFECT_NONE,        1, 1, 3, 4 },
	{ "scan25",  EFFECT_SCANLINE_25, 1, 2, 3, 4 },
	{ "scan50",  EFFECT_SCANLINE_50, 1, 2, 3, 4 },
	{ "scan75",  EFFECT_SCANLINE_75, 1, 2, 3, 4 },
	{ "rgb16",   EFFECT_RGB16,       2, 2, 2, 2 },
	{ "rgb6",    EFFECT_RGB6,        2, 2, 2, 2 },
	{ "rgb4",    EFFECT_RGB4,        2, 2, 2, 2 },
	{ "rgb4v",   EFFECT_RGB4V,       2, 2, 2, 2 },
	{ "rgb3",    EFFECT_RGB3,        2, 2, 2, 2 },
	{ "rgbtiny", EFFECT_RGB_TINY,    2, 2, 2, 2 },
	{ "scan75v", EFFECT_SCANLINE_75V,2, 2, 2, 2 },
};



//============================================================
//	PROTOTYPES
//============================================================

static void update_system_menu(void);
static LRESULT CALLBACK video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
static void constrain_to_aspect_ratio(RECT *rect, int adjustment);
static void draw_video_contents(HDC dc, struct osd_bitmap *bitmap, int update);
static void adjust_window(void);

static UINT32 *prepare_palette(struct blit_params *params);

static void dib_draw_window(HDC dc, struct osd_bitmap *bitmap, int update);

static int ddraw_init(void);
static void ddraw_kill(void);
static double ddraw_compute_mode_score(int width, int height, int depth, int refresh);
static int ddraw_set_resolution(void);
static int ddraw_create_surfaces(void);
static int ddraw_create_blit_surface(void);
static int ddraw_create_palette(void);
static void ddraw_set_brightness(void);
static int ddraw_create_clipper(void);
static void ddraw_erase_surfaces(void);
static void ddraw_release_surfaces(void);
static void ddraw_compute_color_masks(const DDSURFACEDESC *desc);
static int ddraw_draw_window(struct osd_bitmap *bitmap, int update);
static int ddraw_render_to_blit(struct osd_bitmap *bitmap, int update);
static int ddraw_render_to_primary(struct osd_bitmap *bitmap, int update);
static int ddraw_blit_flip(LPDIRECTDRAWSURFACE target_surface, LPRECT src, LPRECT dst, int update);

static int create_debug_window(void);
static void draw_debug_contents(HDC dc, struct osd_bitmap *bitmap);
static LRESULT CALLBACK debug_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);



//============================================================
//	wnd_extra_width
//============================================================

INLINE int wnd_extra_width(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
	return (window.right - window.left) - 100;
}



//============================================================
//	wnd_extra_height
//============================================================

INLINE int wnd_extra_height(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
	return (window.bottom - window.top) - 100;
}



//============================================================
//	wnd_extra_left
//============================================================

INLINE int wnd_extra_left(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
	return 100 - window.left;
}



//============================================================
//	get_aligned_window_pos
//============================================================

INLINE int get_aligned_window_pos(int x)
{
	DEVMODE mode;

	// get the current destination depth
	if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode))
	{
		int bytes_per_pixel = (mode.dmBitsPerPel + 7) / 8;
		int pixels_per_16bytes = 16 / bytes_per_pixel;
		int extra_left = wnd_extra_left();
		x = (((x + extra_left) + pixels_per_16bytes - 1) / pixels_per_16bytes) * pixels_per_16bytes - extra_left;
	}
	return x;
}



//============================================================
//	set_aligned_window_pos
//============================================================

INLINE void set_aligned_window_pos(HWND wnd, HWND insert, int x, int y, int cx, int cy, UINT flags)
{
	SetWindowPos(wnd, insert, get_aligned_window_pos(x), y, cx, cy, flags);
}



//============================================================
//	compute_multipliers
//============================================================

INLINE void compute_multipliers(const RECT *rect, int *xmult, int *ymult)
{
	// first compute simply
	*xmult = (rect->right - rect->left) / visible_width;
	*ymult = (rect->bottom - rect->top) / visible_height;

	// clamp to the hardcoded max
	if (*xmult > MAX_X_MULTIPLY)
		*xmult = MAX_X_MULTIPLY;
	if (*ymult > MAX_Y_MULTIPLY)
		*ymult = MAX_Y_MULTIPLY;

	// clamp to the effect max
	if (*xmult > effect_table[bliteffect].max_xscale)
		*xmult = effect_table[bliteffect].max_xscale;
	if (*ymult > effect_table[bliteffect].max_yscale)
		*ymult = effect_table[bliteffect].max_yscale;

	// adjust for pixel aspect ratio
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		if (*ymult > 1)
			*ymult &= ~1;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		if (*xmult > 1)
			*xmult &= ~1;

	// make sure we have at least 1
	if (*xmult < 1)
		*xmult = 1;
	if (*ymult < 1)
		*ymult = 1;
}



//============================================================
//	determine_effect
//============================================================

INLINE int determine_effect(const struct blit_params *params)
{
	// default to what was selected
	int result = effect_table[bliteffect].effect;

	// if we're out of range, revert to NONE
	if (params->dstxscale < effect_table[bliteffect].min_xscale ||
		params->dstxscale > effect_table[bliteffect].max_xscale ||
		params->dstyscale < effect_table[bliteffect].min_yscale ||
		params->dstyscale > effect_table[bliteffect].max_yscale)
		result = EFFECT_NONE;

	return result;
}



//============================================================
//	erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, HDC dc, LPDIRECTDRAWSURFACE surface)
{
	HBRUSH brush = GetStockObject(BLACK_BRUSH);
	DDBLTFX blitfx = { sizeof(DDBLTFX) };
	RECT clear;

	// erase the blit surface
	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;

	// clear the left edge
	if (inner->left > outer->left)
	{
		clear = *outer;
		clear.right = inner->left;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		if (dc)
			FillRect(dc, &clear, brush);
	}
}



//============================================================
//	get_work_area
//============================================================

INLINE void get_work_area(RECT *maximum)
{
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, maximum, 0))
	{
		// clamp to the width specified
		if (gfx_width && (maximum->right - maximum->left) > (gfx_width + wnd_extra_width()))
		{
			int diff = (maximum->right - maximum->left) - (gfx_width + wnd_extra_width());
			maximum->left += diff / 2;
			maximum->right -= diff - (diff / 2);
		}

		// clamp to the height specified
		if (gfx_height && (maximum->bottom - maximum->top) > (gfx_height + wnd_extra_height()))
		{
			int diff = (maximum->bottom - maximum->top) - (gfx_height + wnd_extra_height());
			maximum->top += diff / 2;
			maximum->bottom -= diff - (diff / 2);
		}
	}
}



//============================================================
//	win32_init_window
//============================================================

int win32_init_window(void)
{
	static int classes_created = 0;
	char title[256];

	// disable scanlines if a bliteffect is active
	if (bliteffect != 0)
		scanlines = 0;

	// set up window class and register it
	if (!classes_created)
	{
		WNDCLASS wc = { 0 };

		// initialize the description of the window class
		wc.lpszClassName 	= "MAME";
		wc.hInstance 		= GetModuleHandle(NULL);
		wc.lpfnWndProc		= video_window_proc;
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
		wc.lpszMenuName		= NULL;
		wc.hbrBackground	= NULL;
		wc.style			= 0;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;

		// register the class; fail if we can't
		if (!RegisterClass(&wc))
			return 1;

		// possibly register the debug window class
		if (options.mame_debug)
		{
			wc.lpszClassName 	= "MAMEDebug";
			wc.lpfnWndProc		= debug_window_proc;

			// register the class; fail if we can't
			if (!RegisterClass(&wc))
				return 1;
		}
	}

	// make the window title
	sprintf(title, "MAME: %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);

	// create the window, but don't show it yet
	video_window = CreateWindowEx(window_mode ? WINDOW_STYLE_EX : FULLSCREEN_STYLE_EX,
			"MAME", title, window_mode ? WINDOW_STYLE : FULLSCREEN_STYLE,
			20, 20, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!video_window)
		return 1;

	// possibly create the debug window, but don't show it yet
	if (options.mame_debug)
		if (create_debug_window())
			return 1;

	// update system menu
	update_system_menu();
	return 0;
}



//============================================================
//	create_window
//============================================================

int create_window(int width, int height, int depth, int attributes, int orientation)
{
	int i, result;

	// clear the initial state
	visible_area_set = 0;

	// extract useful parameters from the orientation
	swap_xy				= ((orientation & ORIENTATION_SWAP_XY) != 0);

	// extract useful parameters from the attributes
	pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);
	dual_monitor		= ((attributes & VIDEO_DUAL_MONITOR) != 0);
	needs_6bpp_per_gun	= ((attributes & VIDEO_NEEDS_6BITS_PER_GUN) != 0);
	vector_game			= ((attributes & VIDEO_TYPE_VECTOR) != 0);

	// handle failure if we couldn't create the video window
	if (!video_window)
		return 1;

	// allocate a temporary bitmap in case we need it
	converted_bitmap = malloc(MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT * 4);
	if (!converted_bitmap)
		return 1;

	// override the width/height with the vector resolution
	if (vector_game && options.vector_width && options.vector_height)
	{
		width = options.vector_width;
		height = options.vector_height;
	}

	// adjust the window position
	set_aligned_window_pos(video_window, NULL, 20, 20,
			width + wnd_extra_width() + 2, height + wnd_extra_height() + 2,
			SWP_NOZORDER);

	// make sure we paint the window once here
	update_video_window(NULL);

	// set the graphics mode width/height to the window size
	max_width = width;
	max_height = height;
	pref_depth = depth;

	// fill in the bitmap info header
	video_dib_info->bmiHeader.biSize			= sizeof(video_dib_info->bmiHeader);
	video_dib_info->bmiHeader.biPlanes			= 1;
	video_dib_info->bmiHeader.biCompression		= BI_RGB;
	video_dib_info->bmiHeader.biSizeImage		= 0;
	video_dib_info->bmiHeader.biXPelsPerMeter	= 0;
	video_dib_info->bmiHeader.biYPelsPerMeter	= 0;
	video_dib_info->bmiHeader.biClrUsed			= 0;
	video_dib_info->bmiHeader.biClrImportant	= 0;

	// initialize the palette to a gray ramp
	for (i = 0; i < 255; i++)
	{
		video_dib_info->bmiColors[i].rgbRed			= i;
		video_dib_info->bmiColors[i].rgbGreen		= i;
		video_dib_info->bmiColors[i].rgbBlue		= i;
		video_dib_info->bmiColors[i].rgbReserved	= i;

		primary_palette_data[i].peRed	= i;
		primary_palette_data[i].peGreen	= i;
		primary_palette_data[i].peBlue	= i;
	}

	// copy that same data into the debug DIB info
	memcpy(debug_dib_info_data, video_dib_info_data, sizeof(debug_dib_info_data));

	// finish off by trying to initialize DirectDraw
	if (use_ddraw)
	{
		result = ddraw_init();
		if (result)
			return result;
	}

	// determine the aspect ratio: hardware stretch case
	if (ddraw_stretch && use_ddraw)
	{
		// if it's explicitly specified, use it
		if (attributes & VIDEO_ASPECT_RATIO_MASK)
		{
			double num = (double)VIDEO_ASPECT_RATIO_NUM(attributes);
			double den = (double)VIDEO_ASPECT_RATIO_DEN(attributes);
			aspect_ratio = swap_xy ? den / num : num / den;
		}

		// otherwise, attempt to deduce the result
		else
		{
			if (!dual_monitor)
				aspect_ratio = swap_xy ? (3.0 / 4.0) : (4.0 / 3.0);
			else
				aspect_ratio = swap_xy ? (6.0 / 4.0) : (4.0 / 6.0);
		}
	}

	// determine the aspect ratio: software stretch case
	else
	{
		aspect_ratio = (double)width / (double)height;
		if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
			aspect_ratio *= 2.0;
		else if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
			aspect_ratio /= 2.0;
	}

	return 0;
}



//============================================================
//	destroy_window
//============================================================

void destroy_window(void)
{
	// kill directdraw
	ddraw_kill();

	// kill the window if it still exists
	if (video_window)
		DestroyWindow(video_window);
}



//============================================================
//	update_cursor_state
//============================================================

void update_cursor_state(void)
{
	if (window_mode && !is_mouse_captured())
		while (ShowCursor(TRUE) < 0) ;
	else
		while (ShowCursor(FALSE) >= 0) ;
}



//============================================================
//	update_system_menu
//============================================================

static void update_system_menu(void)
{
	HMENU menu;

	// revert the system menu
	GetSystemMenu(video_window, TRUE);

	// add to the system menu
	menu = GetSystemMenu(video_window, FALSE);
	if (menu)
		AppendMenu(menu, MF_ENABLED | MF_STRING, MENU_FULLSCREEN, "Full Screen\tAlt+Enter");
}



//============================================================
//	update_video_window
//============================================================

void update_video_window(struct osd_bitmap *bitmap)
{
	// get the client DC and draw to it
	if (video_window)
	{
		HDC dc = GetDC(video_window);
		draw_video_contents(dc, bitmap, 0);
		ReleaseDC(video_window, dc);
	}
}



//============================================================
//	draw_video_contents
//============================================================

static void draw_video_contents(HDC dc, struct osd_bitmap *bitmap, int update)
{
	static struct osd_bitmap *last;

	// if no bitmap, use the last one we got
	if (bitmap == NULL)
		bitmap = last;

	// if no bitmap, just fill
	if (bitmap == NULL)
	{
		RECT fill;
		GetClientRect(video_window, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}
	last = bitmap;

	// if we're iconic, don't bother
	if (IsIconic(video_window))
		return;

	// handle forced updates
	if (forced_updates)
	{
		forced_updates--;
		update = 1;
	}

	// if we're in a window, constrain to a 16-byte aligned boundary
	if (window_mode && !update)
	{
		RECT original;
		int newleft;

		GetWindowRect(video_window, &original);
		newleft = get_aligned_window_pos(original.left);
		if (newleft != original.left)
			SetWindowPos(video_window, NULL, newleft, original.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// if we have a blit surface, use that
	if (use_ddraw && ddraw_draw_window(bitmap, update))
		return;

	// draw to the window with a DIB
	dib_draw_window(dc, bitmap, update);
}



//============================================================
//	video_window_proc
//============================================================

static LRESULT CALLBACK video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// handle a few messages
	switch (message)
	{
		// paint: redraw the last bitmap
		case WM_PAINT:
		{
			PAINTSTRUCT pstruct;
			HDC hdc = BeginPaint(wnd, &pstruct);
			draw_video_contents(hdc, NULL, 1);
			EndPaint(wnd, &pstruct);
			break;
		}

		// get min/max info: set the minimum window size
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO *minmax = (MINMAXINFO *)lparam;
			minmax->ptMinTrackSize.x = visible_width + 2 + wnd_extra_width();
			minmax->ptMinTrackSize.y = visible_height + 2 + wnd_extra_height();
			break;
		}

		// sizing: constrain to the aspect ratio unless control key is held down
		case WM_SIZING:
		{
			RECT *rect = (RECT *)lparam;
			if (keepaspect && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
				constrain_to_aspect_ratio(rect, wparam);
			InvalidateRect(video_window, NULL, FALSE);
			break;
		}

		// syscommands: catch maximize
		case WM_SYSCOMMAND:
		{
			InvalidateRect(video_window, NULL, FALSE);
			if ((wparam & 0xfff0) == SC_MAXIMIZE)
			{
				toggle_maximize();
				break;
			}
			else if (wparam == MENU_FULLSCREEN)
			{
				toggle_full_screen();
				break;
			}
			return DefWindowProc(wnd, message, wparam, lparam);
		}

		// destroy: close down the app
		case WM_DESTROY:
			ddraw_kill();
			trying_to_quit = 1;
			video_window = 0;
			break;

		// everything else: defaults
		default:
			return DefWindowProc(wnd, message, wparam, lparam);
	}

	return 0;
}



//============================================================
//	constrain_to_aspect_ratio
//============================================================

static void constrain_to_aspect_ratio(RECT *rect, int adjustment)
{
	double adjusted_ratio = aspect_ratio;
	int extrawidth = wnd_extra_width();
	int extraheight = wnd_extra_height();
	int newwidth, newheight, adjwidth, adjheight;
	RECT minrect, maxrect, temp;

	// adjust if hardware stretching
	if (use_ddraw && ddraw_stretch)
		adjusted_ratio *= aspect_ratio_adjust;

	// determine the minimum rect
	minrect = *rect;
	minrect.right = minrect.left + (visible_width + 2) + extrawidth;
	minrect.bottom = minrect.top + (int)((double)(visible_width + 2) / adjusted_ratio) + extraheight;
	temp = *rect;
	temp.right = temp.left + (int)((double)(visible_height + 2) * adjusted_ratio) + extrawidth;
	temp.bottom = temp.top + (visible_height + 2) + extraheight;
	if (temp.right > minrect.right || temp.bottom > minrect.bottom)
		minrect = temp;

	// expand the initial rect past the minimum
	temp = *rect;
	UnionRect(rect, &temp, &minrect);

	// determine the maximum rect
	if (window_mode)
		get_work_area(&maxrect);
	else
	{
		maxrect.left = maxrect.top = 0;
		maxrect.right = primary_desc.dwWidth;
		maxrect.bottom = primary_desc.dwHeight;
	}

	// clamp the initial rect to its maxrect box
	temp = *rect;
	IntersectRect(rect, &temp, &maxrect);

	// if we're not forcing the aspect ratio, just return the intersection
	if (!keepaspect)
		return;

	// compute the new requested width/height
	newwidth = rect->right - rect->left - extrawidth;
	newheight = rect->bottom - rect->top - extraheight;

	// compute the adjusted width/height
	adjwidth = (int)((double)newheight * adjusted_ratio);
	adjheight = (int)((double)newwidth / adjusted_ratio);

	// if we're going to be too small, expand outward
	if (adjwidth < minrect.right - minrect.left - extrawidth)
	{
		adjwidth = minrect.right - minrect.left - extrawidth;
		newheight = (int)((double)adjwidth / adjusted_ratio);
	}
	if (adjheight < minrect.bottom - minrect.top - extraheight)
	{
		adjheight = minrect.bottom - minrect.top - extraheight;
		newwidth = (int)((double)adjheight * adjusted_ratio);
	}

	// if we're going to be too big, expand inward
	if (adjwidth > maxrect.right - maxrect.left - extrawidth)
	{
		adjwidth = maxrect.right - maxrect.left - extrawidth;
		newheight = (int)((double)adjwidth / adjusted_ratio);
	}
	if (adjheight > maxrect.bottom - maxrect.top - extraheight)
	{
		adjheight = maxrect.bottom - maxrect.top - extraheight;
		newwidth = (int)((double)adjheight * adjusted_ratio);
	}

	// based on which corner we're adjusting, constrain in different ways
	switch (adjustment)
	{
		case WMSZ_BOTTOM:
			rect->right = rect->left + adjwidth + extrawidth;
			rect->bottom = rect->top + newheight + extraheight;
			break;

		case WMSZ_BOTTOMLEFT:
			if (newwidth * adjheight > adjwidth * newheight)
			{
				rect->bottom = rect->top + adjheight + extraheight;
				rect->left = rect->right - newwidth - extrawidth;
			}
			else
			{
				rect->left = rect->right - adjwidth - extrawidth;
				rect->bottom = rect->top + newheight + extraheight;
			}
			break;

		case WMSZ_BOTTOMRIGHT:
			if (newwidth * adjheight > adjwidth * newheight)
			{
				rect->bottom = rect->top + adjheight + extraheight;
				rect->right = rect->left + newwidth + extrawidth;
			}
			else
			{
				rect->right = rect->left + adjwidth + extrawidth;
				rect->bottom = rect->top + newheight + extraheight;
			}
			break;

		case WMSZ_LEFT:
			rect->top = rect->bottom - adjheight - extraheight;
			rect->left = rect->right - newwidth - extrawidth;
			break;

		case WMSZ_RIGHT:
			rect->bottom = rect->top + adjheight + extraheight;
			rect->right = rect->left + newwidth + extrawidth;
			break;

		case WMSZ_TOP:
			rect->left = rect->right - adjwidth - extrawidth;
			rect->top = rect->bottom - newheight - extraheight;
			break;

		case WMSZ_TOPLEFT:
			if (newwidth * adjheight > adjwidth * newheight)
			{
				rect->top = rect->bottom - adjheight - extraheight;
				rect->left = rect->right - newwidth - extrawidth;
			}
			else
			{
				rect->left = rect->right - adjwidth - extrawidth;
				rect->top = rect->bottom - newheight - extraheight;
			}
			break;

		case WMSZ_TOPRIGHT:
			if (newwidth * adjheight > adjwidth * newheight)
			{
				rect->top = rect->bottom - adjheight - extraheight;
				rect->right = rect->left + newwidth + extrawidth;
			}
			else
			{
				rect->right = rect->left + adjwidth + extrawidth;
				rect->top = rect->bottom - newheight - extraheight;
			}
			break;
	}
}



//============================================================
//	adjust_window_for_visible
//============================================================

void adjust_window_for_visible(int min_x, int max_x, int min_y, int max_y)
{
	// set the new values
	visible_rect.left = min_x;
	visible_rect.top = min_y;
	visible_rect.right = max_x + 1;
	visible_rect.bottom = max_y + 1;
	visible_width = visible_rect.right - visible_rect.left;
	visible_height = visible_rect.bottom - visible_rect.top;

	// if we're not using hardware stretching, recompute the aspect ratio
	if (!ddraw_stretch || !use_ddraw)
	{
		aspect_ratio = (double)visible_width / (double)visible_height;
		if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
			aspect_ratio *= 2.0;
		else if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
			aspect_ratio /= 2.0;
	}

	// adjust the window
	adjust_window();

	// first time through here, we need to show the window
	if (!visible_area_set)
	{
		// let's also maximize the window
		if (window_mode)
		{
			RECT bounds, work;

			// compute the non-maximized bounds here
			get_work_area(&work);
			GetWindowRect(video_window, &bounds);
			non_maximized_bounds.left = work.left + ((work.right - work.left) - (bounds.right - bounds.left)) / 2;
			non_maximized_bounds.top = work.top + ((work.bottom - work.top) - (bounds.bottom - bounds.top)) / 2;
			non_maximized_bounds.right = non_maximized_bounds.left + bounds.right - bounds.left;
			non_maximized_bounds.bottom = non_maximized_bounds.top + bounds.bottom - bounds.top;

			// if maximizing, toggle it
			if (maximize)
				toggle_maximize();

			// otherwise, just enforce the bounds
			else
				set_aligned_window_pos(video_window, NULL, non_maximized_bounds.left, non_maximized_bounds.top,
						non_maximized_bounds.right - non_maximized_bounds.left,
						non_maximized_bounds.bottom - non_maximized_bounds.top,
						SWP_NOZORDER);
		}

		// kludge to fix full screen mode for the non-ddraw case
		if (!use_ddraw && !window_mode)
		{
			window_mode = 1;
			toggle_full_screen();
			memset(&non_fullscreen_bounds, 0, sizeof(non_fullscreen_bounds));
		}

		// show the result
		ShowWindow(video_window, SW_SHOW);
		SetForegroundWindow(video_window);
		update_video_window(NULL);

		// update the cursor state
		update_cursor_state();

		// unpause the input devices
		win32_pause_input(0);
		visible_area_set = 1;
	}
}



//============================================================
//	toggle_maximize
//============================================================

void toggle_maximize(void)
{
	RECT current, maximum;

	// get the current position
	GetWindowRect(video_window, &current);

	// get the desktop work area
	get_work_area(&maximum);

	// if already at max, restore the saved position
	if ((current.right - current.left) >= (maximum.right - maximum.left) ||
		(current.bottom - current.top) >= (maximum.bottom - maximum.top))
	{
		current = non_maximized_bounds;
	}

	// otherwise, save the non_maximized_bounds position and set the new one
	else
	{
		int xoffset, yoffset;

		// save the current location
		non_maximized_bounds = current;

		// compute the max size
		current = maximum;
		constrain_to_aspect_ratio(&current, WMSZ_BOTTOMRIGHT);

		// if we're not stretching, compute the multipliers
		if (!ddraw_stretch || !use_ddraw)
		{
			int xmult, ymult;

			current.right -= wnd_extra_width() + 2;
			current.bottom -= wnd_extra_height() + 2;
			compute_multipliers(&current, &xmult, &ymult);
			current.right = current.left + visible_width * xmult + wnd_extra_width() + 2;
			current.bottom = current.top + visible_height * ymult + wnd_extra_height() + 2;
		}

		// center it
		xoffset = ((maximum.right - maximum.left) - (current.right - current.left)) / 2;
		yoffset = ((maximum.bottom - maximum.top) - (current.bottom - current.top)) / 2;
		current.left += xoffset;
		current.right += xoffset;
		current.top += yoffset;
		current.bottom += yoffset;
	}

	// set the new position
	set_aligned_window_pos(video_window, NULL, current.left, current.top,
			current.right - current.left, current.bottom - current.top,
			SWP_NOZORDER);
}



//============================================================
//	toggle_full_screen
//============================================================

void toggle_full_screen(void)
{
	// rip down DirectDraw
	if (use_ddraw)
		ddraw_kill();
	else
	{
		DEVMODE device_data;
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &device_data);
		primary_desc.dwWidth = device_data.dmPelsWidth;
		primary_desc.dwHeight = device_data.dmPelsHeight;
		primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount = device_data.dmBitsPerPel;
	}

	// hide the window
	ShowWindow(video_window, SW_HIDE);
	if (window_mode && debug_window)
		ShowWindow(debug_window, SW_HIDE);

	// toggle the window mode
	window_mode = !window_mode;

	// adjust the window style and z order
	if (window_mode)
	{
		// adjust the style
		SetWindowLong(video_window, GWL_STYLE, WINDOW_STYLE);
		SetWindowLong(video_window, GWL_EXSTYLE, WINDOW_STYLE_EX);
		set_aligned_window_pos(video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// force to the bottom, then back on top
		set_aligned_window_pos(video_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		set_aligned_window_pos(video_window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// adjust the bounds
		if (non_fullscreen_bounds.right != non_fullscreen_bounds.left)
			set_aligned_window_pos(video_window, HWND_TOP, non_fullscreen_bounds.left, non_fullscreen_bounds.top,
						non_fullscreen_bounds.right - non_fullscreen_bounds.left, non_fullscreen_bounds.bottom - non_fullscreen_bounds.top,
						SWP_NOZORDER);
		else
		{
			set_aligned_window_pos(video_window, HWND_TOP, 0, 0, visible_width + 2, visible_height + 2, SWP_NOZORDER);
			toggle_maximize();
		}
	}
	else
	{
		// save the bounds
		GetWindowRect(video_window, &non_fullscreen_bounds);

		// adjust the style
		SetWindowLong(video_window, GWL_STYLE, FULLSCREEN_STYLE);
		SetWindowLong(video_window, GWL_EXSTYLE, FULLSCREEN_STYLE_EX);
		set_aligned_window_pos(video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// set topmost
		set_aligned_window_pos(video_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	// adjust the window to compensate for the change
	adjust_window();
	update_system_menu();

	// show and adjust the window
	ShowWindow(video_window, SW_SHOW);
	if (window_mode && debug_window)
		ShowWindow(debug_window, SW_SHOW);

	// reinit
	if (use_ddraw)
		if (ddraw_init())
			exit(1);

	// make sure the window is properly readjusted
	adjust_window();
}



//============================================================
//	adjust_window
//============================================================

static void adjust_window(void)
{
	RECT original, window;

	// get the current size
	GetWindowRect(video_window, &original);

	// adjust the window size so the client area is what we want
	if (window_mode)
	{
		// constrain the existing size to the aspect ratio
		window = original;
		constrain_to_aspect_ratio(&window, WMSZ_BOTTOMRIGHT);
	}

	// in full screen, make sure it covers the primary display
	else
	{
		// compute the desired new bounds
		window.left = window.top = 0;
		window.right = primary_desc.dwWidth;
		window.bottom = primary_desc.dwHeight;
	}

	// adjust the position if different
	if (original.left != window.left ||
		original.top != window.top ||
		original.right != window.right ||
		original.bottom != window.bottom ||
		original.left != get_aligned_window_pos(original.left))
		set_aligned_window_pos(video_window, window_mode ? HWND_TOP : HWND_TOPMOST,
				window.left, window.top,
				window.right - window.left, window.bottom - window.top, 0);

	// update the cursor state
	update_cursor_state();
}



//============================================================
//	process_events_periodic
//============================================================

void process_events_periodic(void)
{
	TICKER curr = ticker();
	if (curr - last_event_check < TICKS_PER_SEC / 8)
		return;
	process_events();
}



//============================================================
//	process_events
//============================================================

void process_events(void)
{
	MSG message;

	// remember the last time we did this
	last_event_check = ticker();

	// loop over all messages in the queue
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			// special case for quit
			case WM_QUIT:
				exit(0);
				break;

			// ignore keyboard messages
			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_KEYDOWN:
			case WM_CHAR:
				break;

			// process everything else
			default:
				TranslateMessage(&message);
				DispatchMessage(&message);
				break;
		}
	}
}



//============================================================
//	wait_for_vsync
//============================================================

void wait_for_vsync(void)
{
	HRESULT result;
	BOOL is_vblank;

	// if we're not using DirectDraw, skip
	if (!use_ddraw)
		return;

	// if we're not already in VBLANK, wait for it
	result = IDirectDraw_GetVerticalBlankStatus(ddraw, &is_vblank);
	if (result == DD_OK && !is_vblank)
		result = IDirectDraw_WaitForVerticalBlank(ddraw, DDWAITVB_BLOCKBEGIN, 0);
}



//============================================================
//	osd_get_leds
//============================================================

int osd_get_leds(void)
{
	BYTE key_states[256];
	int result = 0;

	// get the current state
	GetKeyboardState(&key_states[0]);

	// set the numl0ck bit
	result |= (key_states[VK_NUMLOCK] & 1);
	result |= (key_states[VK_CAPITAL] & 1) << 1;
	result |= (key_states[VK_SCROLL] & 1) << 2;
	return result;
}



//============================================================
//	osd_set_leds
//============================================================

void osd_set_leds(int state)
{
	static OSVERSIONINFO osinfo = { sizeof(OSVERSIONINFO) };
	static int version_ready = 0;
	BYTE key_states[256];
	int oldstate, newstate;

	// if we don't yet have a version number, get it
	if (!version_ready)
	{
		version_ready = 1;
		GetVersionEx(&osinfo);
	}

	// thanks to Lee Taylor for the original version of this code

	// get the current state
	GetKeyboardState(&key_states[0]);

	// see if the numlock key matches the state
	oldstate = key_states[VK_NUMLOCK] & 1;
	newstate = state & 1;

	// if not, simulate a key up/down
	if (oldstate != newstate && osinfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
	{
		keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	}
	key_states[VK_NUMLOCK] = (key_states[VK_NUMLOCK] & ~1) | newstate;

	// see if the caps lock key matches the state
	oldstate = key_states[VK_CAPITAL] & 1;
	newstate = (state >> 1) & 1;

	// if not, simulate a key up/down
	if (oldstate != newstate && osinfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
	{
		keybd_event(VK_CAPITAL, 0x3a, 0, 0);
		keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_KEYUP, 0);
	}
	key_states[VK_CAPITAL] = (key_states[VK_CAPITAL] & ~1) | newstate;

	// see if the scroll lock key matches the state
	oldstate = key_states[VK_SCROLL] & 1;
	newstate = (state >> 2) & 1;

	// if not, simulate a key up/down
	if (oldstate != newstate && osinfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
	{
		keybd_event(VK_SCROLL, 0x46, 0, 0);
		keybd_event(VK_SCROLL, 0x46, KEYEVENTF_KEYUP, 0);
	}
	key_states[VK_SCROLL] = (key_states[VK_SCROLL] & ~1) | newstate;

	// if we're on Win9x, use SetKeyboardState
	if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		SetKeyboardState(&key_states[0]);
}



//============================================================
//	set_palette_entry
//============================================================

void set_palette_entry(int index, UINT8 red, UINT8 green, UINT8 blue)
{
	// set the DIB colors
	if (index < 256)
	{
		video_dib_info->bmiColors[index].rgbRed = red;
		video_dib_info->bmiColors[index].rgbGreen = green;
		video_dib_info->bmiColors[index].rgbBlue = blue;
	}

	// set the DirectDraw colors
	if (index < 256)
	{
		primary_palette_data[index].peRed = red;
		primary_palette_data[index].peGreen = green;
		primary_palette_data[index].peBlue = blue;
	}

	// mark the local palette dirty
	local_palette_dirty = 1;
}



//============================================================
//	prepare_palette
//============================================================

static UINT32 *prepare_palette(struct blit_params *params)
{
	int i;

	// destination 8bpp doesn't need a palette to do its blit
	if (params->dstdepth == 8)
		return NULL;

	// 8bpp source always needs a palette otherwise
	if (params->srcdepth == 8)
	{
		static UINT32	palette[256];

		// rebuild the palette
		for (i = 0; i < 256; i++)
			if (params->dstdepth == 16)
			{
				UINT16 temp = color16(video_dib_info->bmiColors[i].rgbRed,
									 video_dib_info->bmiColors[i].rgbGreen,
									 video_dib_info->bmiColors[i].rgbBlue);
				palette[i] = (temp << 16) | temp;
			}
			else
				palette[i] = color32(video_dib_info->bmiColors[i].rgbRed,
									 video_dib_info->bmiColors[i].rgbGreen,
									 video_dib_info->bmiColors[i].rgbBlue);
		return palette;
	}

	// 16bpp source only needs a palette if RGB direct or modifiable
	else if (params->srcdepth == 15 || params->srcdepth == 16)
		return (params->dstdepth == 16) ? palette_16bit_lookup : palette_32bit_lookup;

	// nobody else needs it
	return NULL;
}



//============================================================
//	dib_draw_window
//============================================================

static void dib_draw_window(HDC dc, struct osd_bitmap *bitmap, int update)
{
	int depth = (bitmap->depth == 15) ? 16 : bitmap->depth;
	struct blit_params params;
	int xmult, ymult;
	RECT client;
	int cx, cy;

	// compute the multipliers
	GetClientRect(video_window, &client);
	compute_multipliers(&client, &xmult, &ymult);

	// blit to our temporary bitmap
	params.dstdata		= (void *)(((UINT32)converted_bitmap + 15) & ~15);
	params.dstpitch		= (((visible_width * xmult) + 3) & ~3) * depth / 8;
	params.dstdepth		= depth;
	params.dstxoffs		= 0;
	params.dstyoffs		= 0;
	params.dstxscale	= xmult;
	params.dstyscale	= (!scanlines || ymult == 1) ? ymult : ymult - 1;
	params.dstyskip		= (!scanlines || ymult == 1) ? 0 : 1;
	params.dsteffect	= determine_effect(&params);

	params.srcdata		= bitmap->line[0];
	params.srcpitch		= bitmap->line[1] - bitmap->line[0];
	params.srcdepth		= bitmap->depth;
	params.srclookup	= prepare_palette(&params);
	params.srcxoffs		= visible_rect.left;
	params.srcyoffs		= visible_rect.top;
	params.srcwidth		= visible_width;
	params.srcheight	= visible_height;

	params.dirtydata	= use_dirty ? dirty_grid : NULL;
	params.dirtypitch	= DIRTY_H;

	perform_blit(&params, update);

	// fill in bitmap-specific info
	video_dib_info->bmiHeader.biWidth = params.dstpitch / (depth / 8);
	video_dib_info->bmiHeader.biHeight = -visible_height * ymult;
	video_dib_info->bmiHeader.biBitCount = depth;

	// compute the center position
	cx = client.left + ((client.right - client.left) - visible_width * xmult) / 2;
	cy = client.top + ((client.bottom - client.top) - visible_height * ymult) / 2;

	// blit to the screen
	StretchDIBits(dc, cx, cy, visible_width * xmult, visible_height * ymult,
				0, 0, visible_width * xmult, visible_height * ymult,
				converted_bitmap, video_dib_info, DIB_RGB_COLORS, SRCCOPY);

	// erase the edges if updating
	if (update)
	{
		RECT inner;

		inner.left = cx;
		inner.top = cy;
		inner.right = cx + visible_width * xmult;
		inner.bottom = cy + visible_height * ymult;
		erase_outer_rect(&client, &inner, dc, NULL);
	}
}



//============================================================
//	lookup_effect
//============================================================

int lookup_effect(const char *arg)
{
	int effindex;

	// loop through all the effects and find a match
	for (effindex = 0; effindex < sizeof(effect_table) / sizeof(effect_table[0]); effindex++)
		if (!strcmp(arg, effect_table[effindex].name))
			return effindex;

	return -1;
}



//============================================================
//	ddraw_init
//============================================================

static int ddraw_init(void)
{
	HRESULT result;
	DDCAPS hel_caps;

	// now attempt to create it
	result = DirectDrawCreate(NULL, &ddraw, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating DirectDraw: %08x\n", (UINT32)result);
		goto cant_create_ddraw;
	}

	// see if we can get a DDraw4 object
	result = IDirectDraw_QueryInterface(ddraw, &IID_IDirectDraw4, (void **)&ddraw4);
	if (result != DD_OK)
		ddraw4 = NULL;

	// get the capabilities
	ddraw_caps.dwSize = sizeof(ddraw_caps);
	hel_caps.dwSize = sizeof(hel_caps);
	result = IDirectDraw_GetCaps(ddraw, &ddraw_caps, &hel_caps);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting DirectDraw capabilities: %08x\n", (UINT32)result);
		goto cant_get_caps;
	}

	// determine if hardware stretching is available
	if (ddraw_stretch)
		ddraw_stretch = ((ddraw_caps.dwCaps & DDCAPS_BLTSTRETCH) != 0);
	if (ddraw_stretch && verbose)
		fprintf(stderr, "Hardware stretching supported\n");

	// set the cooperative level
	// for non-window modes, we will use full screen here
	result = IDirectDraw_SetCooperativeLevel(ddraw, video_window, window_mode ? DDSCL_NORMAL : DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting cooperative level: %08x\n", (UINT32)result);
		goto cant_set_coop_level;
	}

	// full screen mode: set the resolution
	changed_resolutions = 0;
	if (ddraw_set_resolution())
		goto cant_set_resolution;

	// create the surfaces
	if (ddraw_create_surfaces())
		goto cant_create_surfaces;

	// force some updates
	forced_updates = 5;
	mark_palette_dirty();
	return 0;

	// error handling
cant_create_surfaces:
cant_set_resolution:
cant_set_coop_level:
cant_get_caps:
	IDirectDraw_Release(ddraw);
cant_create_ddraw:
	ddraw = NULL;
	return 0;
}



//============================================================
//	ddraw_kill
//============================================================

static void ddraw_kill(void)
{
	// release the surfaces
	ddraw_release_surfaces();

	// restore resolutions
	if (ddraw != NULL && changed_resolutions)
		IDirectDraw_RestoreDisplayMode(ddraw);

	// reset cooperative level
	if (ddraw != NULL && video_window != 0)
		IDirectDraw_SetCooperativeLevel(ddraw, video_window, DDSCL_NORMAL);

	// delete the core object
	if (ddraw != NULL)
		IDirectDraw_Release(ddraw);
	ddraw = NULL;
}



//============================================================
//	ddraw_enum_callback
//============================================================

static HRESULT WINAPI ddraw_enum_callback(LPDDSURFACEDESC desc, LPVOID context)
{
	int depth = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	double score;

	// make sure we have color masks
	ddraw_compute_color_masks(desc);

	// compute this mode's score
	score = ddraw_compute_mode_score(desc->dwWidth, desc->dwHeight, depth, 0);

	// is it the best?
	if (score > best_score)
	{
		// if so, remember it
		best_score = score;
		best_width = desc->dwWidth;
		best_height = desc->dwHeight;
		best_depth = depth;
		best_refresh = 0;
	}
	return DDENUMRET_OK;
}



//============================================================
//	ddraw_enum2_callback
//============================================================

static HRESULT WINAPI ddraw_enum2_callback(LPDDSURFACEDESC2 desc, LPVOID context)
{
	int refresh = (matchrefresh || gfx_refresh) ? desc->DUMMYUNIONNAMEN(2).dwRefreshRate : 0;
	int depth = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	double score;

	// make sure we have color masks
	ddraw_compute_color_masks((LPDDSURFACEDESC)desc);

	// compute this mode's score
	score = ddraw_compute_mode_score(desc->dwWidth, desc->dwHeight, depth, refresh);

	// is it the best?
	if (score > best_score)
	{
		// if so, remember it
		best_score = score;
		best_width = desc->dwWidth;
		best_height = desc->dwHeight;
		best_depth = depth;
		best_refresh = refresh;
	}
	return DDENUMRET_OK;
}



//============================================================
//	ddraw_compute_mode_score
//============================================================

static double ddraw_compute_mode_score(int width, int height, int depth, int refresh)
{
	static const double depth_matrix[4][2][4] =
	{
			// !needs_6bpp_per_gun		  // needs_6bpp_per_gun
		{ { 1.00, 0.75, 0.25, 0.50 },	{ 1.00, 0.25, 0.50, 0.75 } },	// 8bpp source
		{ { 0.25, 1.00, 0.25, 0.50 },	{ 0.25, 0.50, 0.75, 1.00 } },	// 16bpp source
		{ { 0.00, 0.00, 0.00, 0.00 },	{ 0.00, 0.00, 0.00, 0.00 } },	// 24bpp source (doesn't exist)
		{ { 0.25, 0.50, 0.75, 1.00 },	{ 0.25, 0.50, 0.75, 1.00 } }	// 32bpp source
	};

	double size_score, depth_score, refresh_score, final_score;
	int minxscale = effect_table[bliteffect].min_xscale;
	int minyscale = effect_table[bliteffect].min_yscale;
	int target_width, target_height;

	// first compute a score based on size

	// if not stretching, we need to keep minx and miny scale equal
	if (!ddraw_stretch)
	{
		if (minxscale > minyscale)
			minyscale = minxscale;
		else
			minxscale = minyscale;
	}

	// determine minimum requirements
	target_width = max_width * minxscale;
	target_height = max_height * minyscale;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		target_height *= 2;
	else if (scanlines)
		target_width *= 2, target_height *= 2;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		target_width *= 2;

	// hardware stretch modes prefer at least 2x expansion
	if (ddraw_stretch)
	{
		if (target_width < max_width * 2 + 2)
			target_width = max_width * 2 + 2;
		if (target_height < max_height * 2 + 2)
			target_height = max_height * 2 + 2;
	}

	// compute initial score based on difference between target and current
	size_score = 1.0 / (1.0 + fabs(width - target_width) + fabs(height - target_height));

	// if we're looking for a particular mode, make sure it matches
	if (gfx_width && gfx_height && (width != gfx_width || height != gfx_height))
		return 0.0;

	// if mode is too small, it's a zero, unless the user specified otherwise
	if ((width < max_width || height < max_height) && (!gfx_width || !gfx_height))
		return 0.0;

	// if mode is smaller than we'd like, it only scores up to 0.1
	if (width < target_width || height < target_height)
		size_score *= 0.1;

	// next compute depth score
	depth_score = depth_matrix[(pref_depth + 7) / 8 - 1][needs_6bpp_per_gun][(depth + 7) / 8 - 1];

	// hardware stretch and effects require 16bpp
	if ((ddraw_stretch || bliteffect != 0) && depth < 16)
		return 0.0;

	// if we're looking for a particular depth, make sure it matches
	if (gfx_depth && depth != gfx_depth)
		return 0.0;

	// finally, compute refresh score
	refresh_score = 1.0 / (1.0 + fabs((double)refresh - Machine->drv->frames_per_second));

	// if we're looking for a particular refresh, make sure it matches
	if (gfx_refresh && refresh && refresh != gfx_refresh)
		return 0.0;

	// if refresh is smaller than we'd like, it only scores up to 0.1
	if ((double)refresh < Machine->drv->frames_per_second)
		refresh_score *= 0.1;

	// weight size highest, followed by depth and refresh
	final_score = (size_score * 100.0 + depth_score * 10.0 + refresh_score) / 111.0;
	return final_score;
}



//============================================================
//	ddraw_set_resolution
//============================================================

static int ddraw_set_resolution(void)
{
	DDSURFACEDESC currmode = { sizeof(DDSURFACEDESC) };
	double resolution_aspect;
	HRESULT result;

	// skip if not switching resolution
	if (!window_mode && (switchres || switchbpp))
	{
		// if we're only switching depth, set gfx_width and gfx_height to the current resolution
		if (!switchres || !switchbpp)
		{
			// attempt to get the current display mode
			result = IDirectDraw_GetDisplayMode(ddraw, &currmode);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
				goto cant_get_mode;
			}

			// force to the current width/height
			if (!switchres)
			{
				gfx_width = currmode.dwWidth;
				gfx_height = currmode.dwHeight;
			}
			if (!switchbpp)
				gfx_depth = currmode.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
		}

		// enumerate display modes
		best_score = 0.0;
		if (ddraw4)
			result = IDirectDraw4_EnumDisplayModes(ddraw4, (matchrefresh || gfx_refresh) ? DDEDM_REFRESHRATES : 0, NULL, NULL, ddraw_enum2_callback);
		else
			result = IDirectDraw_EnumDisplayModes(ddraw, 0, NULL, NULL, ddraw_enum_callback);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error enumerating modes: %08x\n", (UINT32)result);
			goto cant_enumerate_modes;
		}

		if (verbose)
		{
			if (best_refresh)
				fprintf(stderr, "Best mode = %dx%dx%d @ %d Hz\n", best_width, best_height, best_depth, best_refresh);
			else
				fprintf(stderr, "Best mode = %dx%dx%d @ default Hz\n", best_width, best_height, best_depth);
		}

		// set it
		if (best_width != 0)
		{
			// use the DDraw 4 version to set the refresh rate if we can
			if (ddraw4)
				result = IDirectDraw4_SetDisplayMode(ddraw4, best_width, best_height, best_depth, best_refresh, 0);
			else
				result = IDirectDraw_SetDisplayMode(ddraw, best_width, best_height, best_depth);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error setting mode: %08x\n", (UINT32)result);
				goto cant_set_mode;
			}
			changed_resolutions = 1;
		}
	}

	// attempt to get the current display mode
	result = IDirectDraw_GetDisplayMode(ddraw, &currmode);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
		goto cant_get_mode;
	}

	// compute the adjusted aspect ratio
	resolution_aspect = (double)currmode.dwWidth / (double)currmode.dwHeight;
	aspect_ratio_adjust = resolution_aspect / screen_aspect;
	return 0;

	// error handling - non fatal in general
cant_set_mode:
cant_get_mode:
cant_enumerate_modes:
	return 0;
}



//============================================================
//	ddraw_create_surfaces
//============================================================

static int ddraw_create_surfaces(void)
{
	HRESULT result;

	// make a description of the primary surface
	memset(&primary_desc, 0, sizeof(primary_desc));
	primary_desc.dwSize = sizeof(primary_desc);
	primary_desc.dwFlags = DDSD_CAPS;
	primary_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	// for full screen mode, allocate flipping surfaces
	if (!window_mode && use_triplebuf)
	{
		primary_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		primary_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		primary_desc.dwBackBufferCount = 2;
	}

	// then create the primary surface
	result = IDirectDraw_CreateSurface(ddraw, &primary_desc, &primary_surface, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating primary surface: %08x\n", (UINT32)result);
		goto cant_create_primary;
	}

	// get a description of the primary surface
	result = IDirectDrawSurface_GetSurfaceDesc(primary_surface, &primary_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting primary surface desc: %08x\n", (UINT32)result);
		goto cant_get_primary_desc;
	}

	// if this is a full-screen, 8bpp video mode, make a palette
	if (!window_mode && primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 8)
	{
		if (ddraw_create_palette())
			goto cant_make_palette;
	}

	// if this is a full-screen mode, attempt to create a color control object
	if (!window_mode && gfx_brightness != 0.0)
		ddraw_set_brightness();

	// print out the good stuff
	if (verbose)
		fprintf(stderr, "Primary surface created: %dx%dx%d (R=%08x G=%08x B=%08x)\n",
				(int)primary_desc.dwWidth,
				(int)primary_desc.dwHeight,
				(int)primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount,
				(UINT32)primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask,
				(UINT32)primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask,
				(UINT32)primary_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask);

	// full screen mode: get the back surface
	back_surface = NULL;
	if (!window_mode && use_triplebuf)
	{
		DDSCAPS caps = { DDSCAPS_BACKBUFFER };
		result = IDirectDrawSurface_GetAttachedSurface(primary_surface, &caps, &back_surface);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error getting attached back surface: %08x\n", (UINT32)result);
			goto cant_get_back_surface;
		}
	}

	// stretch mode: create a blit surface
	if (ddraw_stretch)
	{
		if (ddraw_create_blit_surface())
			goto cant_create_blit;
	}

	// create a clipper for windowed mode
	if (window_mode)
	{
		if (ddraw_create_clipper())
			goto cant_init_clipper;
	}

	// erase all the surfaces we created
	ddraw_erase_surfaces();

	// compute the mask colors
	ddraw_compute_color_masks(&primary_desc);
	return 0;

	// error handling
cant_init_clipper:
	if (blit_surface)
		IDirectDrawSurface_Release(blit_surface);
	blit_surface = NULL;
cant_create_blit:
cant_get_back_surface:
	if (gamma_control)
		IDirectDrawColorControl_Release(gamma_control);
	gamma_control = NULL;
	if (primary_palette)
		IDirectDrawPalette_Release(primary_palette);
	primary_palette = NULL;
cant_make_palette:
cant_get_primary_desc:
	IDirectDrawSurface_Release(primary_surface);
	primary_surface = NULL;
cant_create_primary:
	return 1;
}



//============================================================
//	ddraw_create_blit_surface
//============================================================

static int ddraw_create_blit_surface(void)
{
	HRESULT result;

	// now make a description of our blit surface, based on the primary surface
	blit_desc = primary_desc;
	blit_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
	blit_desc.dwWidth = (max_width * effect_table[bliteffect].min_xscale) + 18;
	blit_desc.dwHeight = (max_height * effect_table[bliteffect].min_yscale) + 2;
	blit_desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;

	// then create the blit surface
	result = IDirectDraw_CreateSurface(ddraw, &blit_desc, &blit_surface, NULL);

	// fall back to system memory if video mem doesn't work
	if (result != DD_OK)
	{
		blit_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
		result = IDirectDraw_CreateSurface(ddraw, &blit_desc, &blit_surface, NULL);
	}
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating blit surface: %08x\n", (UINT32)result);
		goto cant_create_blit;
	}

	// get a description of the blit surface
	result = IDirectDrawSurface_GetSurfaceDesc(blit_surface, &blit_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting blit surface desc: %08x\n", (UINT32)result);
		goto cant_get_blit_desc;
	}

	// print out the good stuff
	if (verbose)
		fprintf(stderr, "Blit surface created: %dx%dx%d (R=%08x G=%08x B=%08x)\n",
				(int)blit_desc.dwWidth,
				(int)blit_desc.dwHeight,
				(int)blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount,
				(UINT32)blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask,
				(UINT32)blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask,
				(UINT32)blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask);
	return 0;

	// error handling
cant_get_blit_desc:
	if (blit_surface)
		IDirectDrawSurface_Release(blit_surface);
	blit_surface = NULL;
cant_create_blit:
	return 1;
}



//============================================================
//	ddraw_create_palette
//============================================================

static int ddraw_create_palette(void)
{
	HRESULT result;

	// create the palette object
	result = IDirectDraw_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, primary_palette_data, &primary_palette, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating palette: %08x\n", (UINT32)result);
		goto cant_make_palette;
	}

	// set the palette
	result = IDirectDrawSurface_SetPalette(primary_surface, primary_palette);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting palette: %08x\n", (UINT32)result);
		goto cant_set_palette;
	}
	return 0;

	// error handling
cant_set_palette:
	if (primary_palette)
		IDirectDrawPalette_Release(primary_palette);
	primary_palette = NULL;
cant_make_palette:
	return 1;
}



//============================================================
//	ddraw_set_brightness
//============================================================

static void ddraw_set_brightness(void)
{
	HRESULT result;

	// see if we can get a GammaControl object
	result = IDirectDrawSurface_QueryInterface(primary_surface, &IID_IDirectDrawGammaControl, (void **)&gamma_control);
	if (result != DD_OK)
	{
		fprintf(stderr, "Warning: could not create gamma control to change brightness: %08x\n", (UINT32)result);
		gamma_control = NULL;
	}

	// if we got it, proceed
	if (gamma_control)
	{
		DDGAMMARAMP ramp;
		int i;

		// fill the gamma ramp
		for (i = 0; i < 256; i++)
		{
			double val = ((float)i / 255.0) * gfx_brightness;
			if (val > 1.0)
				val = 1.0;
			ramp.red[i] = ramp.green[i] = ramp.blue[i] = (WORD)(val * 65535.0);
		}

		// attempt to get the current settings
		result = IDirectDrawGammaControl_SetGammaRamp(gamma_control, 0, &ramp);
		if (result != DD_OK)
			fprintf(stderr, "Error setting gamma ramp: %08x\n", (UINT32)result);
	}
}



//============================================================
//	ddraw_create_clipper
//============================================================

static int ddraw_create_clipper(void)
{
	HRESULT result;

	// create a clipper for the primary surface
	result = IDirectDraw_CreateClipper(ddraw, 0, &primary_clipper, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating clipper: %08x\n", (UINT32)result);
		goto cant_create_clipper;
	}

	// set the clipper's hwnd
	result = IDirectDrawClipper_SetHWnd(primary_clipper, 0, video_window);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting clipper hwnd: %08x\n", (UINT32)result);
		goto cant_set_hwnd;
	}

	// set the clipper on the primary surface
	result = IDirectDrawSurface_SetClipper(primary_surface, primary_clipper);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting clipper on primary surface: %08x\n", (UINT32)result);
		goto cant_set_surface;
	}
	return 0;

	// error handling
cant_set_surface:
cant_set_hwnd:
	IDirectDrawClipper_Release(primary_clipper);
cant_create_clipper:
	return 1;
}



//============================================================
//	ddraw_erase_surfaces
//============================================================

static void ddraw_erase_surfaces(void)
{
	DDBLTFX blitfx = { sizeof(DDBLTFX) };
	HRESULT result = DD_OK;
	int i;

	// erase the blit surface
	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;
	if (blit_surface)
		result = IDirectDrawSurface_Blt(blit_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// loop through enough to get all the back buffers
	if (!window_mode)
	{
		if (back_surface)
			for (i = 0; i < 5; i++)
			{
				// first flip
				result = IDirectDrawSurface_Flip(primary_surface, NULL, DDFLIP_WAIT);

				// then do a color fill blit
				result = IDirectDrawSurface_Blt(back_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
			}
		else
			result = IDirectDrawSurface_Blt(primary_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	ddraw_release_surfaces
//============================================================

static void ddraw_release_surfaces(void)
{
	// release the blit surface
	if (blit_surface)
		IDirectDrawSurface_Release(blit_surface);
	blit_surface = NULL;

	// release the clipper
	if (primary_clipper)
		IDirectDrawClipper_Release(primary_clipper);
	primary_clipper = NULL;

	// release the color controls
	if (gamma_control)
		IDirectDrawColorControl_Release(gamma_control);
	gamma_control = NULL;

	// release the palette
	if (primary_palette)
		IDirectDrawPalette_Release(primary_palette);
	primary_palette = NULL;

	// release the primary surface
	if (primary_surface)
		IDirectDrawSurface_Release(primary_surface);
	primary_surface = NULL;
}



//============================================================
//	ddraw_compute_color_masks
//============================================================

static void ddraw_compute_color_masks(const DDSURFACEDESC *desc)
{
	// 16bpp case
	if (desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
	{
		int temp;

		// red
		color16_rdst_shift = color16_rsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, color16_rdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, color16_rsrc_shift++;

		// green
		color16_gdst_shift = color16_gsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, color16_gdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, color16_gsrc_shift++;

		// blue
		color16_bdst_shift = color16_bsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, color16_bdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, color16_bsrc_shift++;
	}

	// 24/32bpp case
	else if (desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 24 ||
			 desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 32)
	{
		int temp;

		// red
		color32_rdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, color32_rdst_shift++;

		// green
		color32_gdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, color32_gdst_shift++;

		// blue
		color32_bdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, color32_bdst_shift++;
	}
}



//============================================================
//	ddraw_draw_window
//============================================================

static int ddraw_draw_window(struct osd_bitmap *bitmap, int update)
{
	int result;

	// update the palette
	if (update || local_palette_dirty)
	{
		if (primary_palette)
			IDirectDrawPalette_SetEntries(primary_palette, 0, 0, 256, primary_palette_data);
		else
			memset(dirty_grid, 1, sizeof(dirty_grid));
		local_palette_dirty = 0;
	}

	// if we're using hardware stretching, render to the blit surface,
	// then blit that and stretch
	if (ddraw_stretch)
		result = ddraw_render_to_blit(bitmap, update);

	// otherwise, render directly to the primary/back surface
	else
		result = ddraw_render_to_primary(bitmap, update);

	return result;
}



//============================================================
//	ddraw_render_to_blit
//============================================================

static int ddraw_render_to_blit(struct osd_bitmap *bitmap, int update)
{
	int dstdepth = blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	LPDIRECTDRAWSURFACE target_surface;
	struct blit_params params;
	HRESULT result;
	RECT src, dst;
	int dstxoffs;

tryagain:
	// attempt to lock the blit surface
	result = IDirectDrawSurface_Lock(blit_surface, NULL, &blit_desc, (throttle || use_dirty) ? DDLOCK_WAIT : 0, NULL);

	if (result == DDERR_SURFACELOST)
		goto surface_lost;

	// if it was busy (and we're not throttling), just punt
	if (result == DDERR_SURFACEBUSY || result == DDERR_WASSTILLDRAWING)
		return 1;
	if (result != DD_OK)
	{
		fprintf(stderr, "Unable to lock blit_surface: %08x\n", (UINT32)result);
		return 0;
	}

	// align the destination to 16 bytes
	dstxoffs = (((UINT32)blit_desc.lpSurface + 16) & ~15) - (UINT32)blit_desc.lpSurface;
	dstxoffs /= (dstdepth / 8);

	// perform the low-level blit
	params.dstdata		= blit_desc.lpSurface;
	params.dstpitch		= blit_desc.DUMMYUNIONNAMEN(1).lPitch;
	params.dstdepth		= dstdepth;
	params.dstxoffs		= dstxoffs;
	params.dstyoffs		= 1;
	params.dstxscale	= effect_table[bliteffect].min_xscale;
	params.dstyscale	= effect_table[bliteffect].min_yscale;
	params.dstyskip		= 0;
	params.dsteffect	= determine_effect(&params);

	params.srcdata		= bitmap->line[0];
	params.srcpitch		= bitmap->line[1] - bitmap->line[0];
	params.srcdepth		= bitmap->depth;
	params.srclookup	= prepare_palette(&params);
	params.srcxoffs		= visible_rect.left;
	params.srcyoffs		= visible_rect.top;
	params.srcwidth		= visible_width;
	params.srcheight	= visible_height;

	params.dirtydata	= use_dirty ? dirty_grid : NULL;
	params.dirtypitch	= DIRTY_H;

	perform_blit(&params, 0);

	// unlock the surface
	IDirectDrawSurface_Unlock(blit_surface, NULL);

	// make the src rect
	src.left = dstxoffs - 1;
	src.top = 0;
	src.right = (dstxoffs + visible_width * effect_table[bliteffect].min_xscale) + 1;
	src.bottom = (visible_height * effect_table[bliteffect].min_yscale) + 2;

	// window mode
	if (window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(video_window, &dst);
		ClientToScreen(video_window, &((LPPOINT)&dst)[0]);
		ClientToScreen(video_window, &((LPPOINT)&dst)[1]);

		// target surface is the primary
		target_surface = primary_surface;
	}

	// full screen mode
	else
	{
		// maximize the rect, constraining to the aspect ratio
		dst.left = dst.top = 0;
		dst.right = primary_desc.dwWidth;
		dst.bottom = primary_desc.dwHeight;
		constrain_to_aspect_ratio(&dst, WMSZ_BOTTOMRIGHT);

		// center
		dst.left += (primary_desc.dwWidth - (dst.right - dst.left)) / 2;
		dst.top += (primary_desc.dwHeight - (dst.bottom - dst.top)) / 2;
		dst.right += dst.left;
		dst.bottom += dst.top;

		// target surface is the back buffer
		target_surface = back_surface ? back_surface : primary_surface;
	}

	// blit and flip
	if (!ddraw_blit_flip(target_surface, &src, &dst, update))
		return 0;

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Recreating surfaces\n");

	// go ahead and adjust the window
	adjust_window();

	// release and recreate the surfaces
	ddraw_release_surfaces();
	if (!ddraw_create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	ddraw_blit_flip
//============================================================

static int ddraw_blit_flip(LPDIRECTDRAWSURFACE target_surface, LPRECT src, LPRECT dst, int update)
{
	HRESULT result;

	// sync to VBLANK?
	if ((wait_vsync || syncrefresh) && throttle && game_speed_percent > 95)
	{
		BOOL is_vblank;

		// this counts as idle time
		profiler_mark(PROFILER_IDLE);

		result = IDirectDraw_GetVerticalBlankStatus(ddraw, &is_vblank);
		if (!is_vblank)
			result = IDirectDraw_WaitForVerticalBlank(ddraw, DDWAITVB_BLOCKBEGIN, 0);

		// idle time done
		profiler_mark(PROFILER_END);
	}

tryagain:
	// do the blit
	result = IDirectDrawSurface_Blt(target_surface, dst, blit_surface, src, DDBLT_ASYNC, NULL);
	if (result == DDERR_SURFACELOST)
		goto surface_lost;
	if (result != DD_OK && result != DDERR_WASSTILLDRAWING)
	{
		// otherwise, print the error and fall back
		fprintf(stderr, "Unable to blt blit_surface: %08x\n", (UINT32)result);
		return 0;
	}

	// erase the edges if updating
	if (update)
	{
		RECT outer;
		outer.top = outer.left = 0;
		outer.right = primary_desc.dwWidth;
		outer.bottom = primary_desc.dwHeight;
		erase_outer_rect(&outer, dst, NULL, target_surface);
	}

	// full screen mode: flip
	if (!window_mode && back_surface && result != DDERR_WASSTILLDRAWING)
	{
#if SHOW_FLIP_TIMES
		static TICKER total;
		static int count;
		TICKER start = ticker(), stop;
#endif

		IDirectDrawSurface_Flip(primary_surface, NULL, DDFLIP_NOVSYNC);

#if SHOW_FLIP_TIMES
		stop = ticker();
		if (++count > 100)
		{
			total += stop - start;
			usrintf_showmessage("Avg Flip = %d", (int)(total / (count - 100)));
		}
#endif
	}
	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Recreating surfaces\n");

	// go ahead and adjust the window
	adjust_window();

	// release and recreate the surfaces
	ddraw_release_surfaces();
	if (!ddraw_create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	ddraw_render_to_primary
//============================================================

static int ddraw_render_to_primary(struct osd_bitmap *bitmap, int update)
{
	DDSURFACEDESC temp_desc = { sizeof(temp_desc) };
	LPDIRECTDRAWSURFACE target_surface;
	struct blit_params params;
	int xmult, ymult, dstdepth;
	HRESULT result;
	RECT outer, inner, temp;

tryagain:
	// window mode
	if (window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(video_window, &outer);
		ClientToScreen(video_window, &((LPPOINT)&outer)[0]);
		ClientToScreen(video_window, &((LPPOINT)&outer)[1]);
		inner = outer;

		// target surface is the primary
		target_surface = primary_surface;
	}

	// full screen mode
	else
	{
		// maximize the rect, constraining to the aspect ratio
		outer.left = outer.top = 0;
		outer.right = primary_desc.dwWidth;
		outer.bottom = primary_desc.dwHeight;
		inner = outer;
		constrain_to_aspect_ratio(&inner, WMSZ_BOTTOMRIGHT);

		// target surface is the back buffer
		target_surface = back_surface ? back_surface : primary_surface;
	}

	// compute the multipliers
	compute_multipliers(&inner, &xmult, &ymult);

	// center within the display rect
	inner.left = outer.left + ((outer.right - outer.left) - (visible_width * xmult)) / 2;
	inner.top = outer.top + ((outer.bottom - outer.top) - (visible_height * ymult)) / 2;
	inner.right = inner.left + visible_width * xmult;
	inner.bottom = inner.top + visible_height * ymult;

	// make sure we're not clipped
	if (window_mode)
	{
		UINT8 clipbuf[sizeof(RGNDATA) + sizeof(RECT)];
		RGNDATA *clipdata = (RGNDATA *)clipbuf;
		DWORD clipsize = sizeof(clipbuf);

		// get the size of the clip list; bail if we don't get back just a single rect
		result = IDirectDrawClipper_GetClipList(primary_clipper, &inner, clipdata, &clipsize);
		IntersectRect(&temp, (RECT *)clipdata->Buffer, &inner);
		if (result != DD_OK || !EqualRect(&temp, &inner))
			return 0;
	}

	// attempt to lock the target surface
	result = IDirectDrawSurface_Lock(target_surface, NULL, &temp_desc, throttle ? DDLOCK_WAIT : 0, NULL);
	if (result == DDERR_SURFACELOST)
		goto surface_lost;
	dstdepth = temp_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;

	// try to align the destination
	while (inner.left > outer.left && (((UINT32)temp_desc.lpSurface + ((dstdepth + 7) / 8) * inner.left) & 15) != 0)
		inner.left--, inner.right--;

	// clamp to the display rect
	IntersectRect(&temp, &inner, &outer);
	inner = temp;

	// if it was busy (and we're not throttling), just punt
	if (result == DDERR_SURFACEBUSY || result == DDERR_WASSTILLDRAWING)
		return 1;
	if (result != DD_OK)
	{
		fprintf(stderr, "Unable to lock target_surface: %08x\n", (UINT32)result);
		return 0;
	}

	// perform the low-level blit
	params.dstdata		= temp_desc.lpSurface;
	params.dstpitch		= temp_desc.DUMMYUNIONNAMEN(1).lPitch;
	params.dstdepth		= dstdepth;
	params.dstxoffs		= inner.left;
	params.dstyoffs		= inner.top;
	params.dstxscale	= xmult;
	params.dstyscale	= (!scanlines || ymult == 1) ? ymult : ymult - 1;
	params.dstyskip		= (!scanlines || ymult == 1) ? 0 : 1;
	params.dsteffect	= determine_effect(&params);

	params.srcdata		= bitmap->line[0];
	params.srcpitch		= bitmap->line[1] - bitmap->line[0];
	params.srcdepth		= bitmap->depth;
	params.srclookup	= prepare_palette(&params);
	params.srcxoffs		= visible_rect.left;
	params.srcyoffs		= visible_rect.top;
	params.srcwidth		= visible_width;
	params.srcheight	= visible_height;

	params.dirtydata	= use_dirty ? dirty_grid : NULL;
	params.dirtypitch	= DIRTY_H;

	perform_blit(&params, update);

	// unlock the surface
	IDirectDrawSurface_Unlock(target_surface, NULL);

	// erase the edges if updating
	if (update)
		erase_outer_rect(&outer, &inner, NULL, target_surface);

	// full screen mode: flip
	if (!window_mode && back_surface && result != DDERR_WASSTILLDRAWING)
	{
#if SHOW_FLIP_TIMES
		static TICKER total;
		static int count;
		TICKER start = ticker(), stop;
#endif

		IDirectDrawSurface_Flip(primary_surface, NULL, DDFLIP_NOVSYNC);

#if SHOW_FLIP_TIMES
		stop = ticker();
		if (++count > 100)
		{
			total += stop - start;
			usrintf_showmessage("Avg Flip = %d", (int)(total / (count - 100)));
		}
#endif
	}
	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Recreating surfaces\n");

	// go ahead and adjust the window
	adjust_window();

	// release and recreate the surfaces
	ddraw_release_surfaces();
	if (!ddraw_create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	create_debug_window
//============================================================

static int create_debug_window(void)
{
#ifdef MAME_DEBUG
	RECT bounds, work_bounds;
	char title[256];

	sprintf(title, "Debug: %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);

	// get the adjusted bounds
	bounds.top = bounds.left = 0;
	bounds.right = options.debug_width;
	bounds.bottom = options.debug_height;
	AdjustWindowRectEx(&bounds, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);

	// get the work bounds
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_bounds, 0);

	// create the window
	debug_window = CreateWindowEx(DEBUG_WINDOW_STYLE_EX, "MAMEDebug", title, DEBUG_WINDOW_STYLE,
			work_bounds.right - (bounds.right - bounds.left),
			work_bounds.bottom - (bounds.bottom - bounds.top),
			bounds.right - bounds.left, bounds.bottom - bounds.top,
			video_window, NULL, GetModuleHandle(NULL), NULL);
	if (!debug_window)
		return 1;
#endif

	return 0;
}



//============================================================
//	update_debug_window
//============================================================

void update_debug_window(struct osd_bitmap *bitmap)
{
#ifdef MAME_DEBUG
	// if the window isn't 8bpp, force it there and clear it
	if (bitmap != NULL && bitmap->depth != 8)
	{
		bitmap->depth = 8;
		fillbitmap(bitmap, 0, NULL);
		win_invalidate_video();
		return;
	}

	// get the client DC and draw to it
	if (debug_window)
	{
		HDC dc = GetDC(debug_window);
		draw_debug_contents(dc, bitmap);
		ReleaseDC(debug_window, dc);
	}
#endif
}



//============================================================
//	draw_debug_contents
//============================================================

static void draw_debug_contents(HDC dc, struct osd_bitmap *bitmap)
{
	static struct osd_bitmap *last;
	UINT8 *bitmap_base;
	int i;

	// if no bitmap, use the last one we got
	if (bitmap == NULL)
		bitmap = last;

	// if no bitmap, just fill
	if (bitmap == NULL || !debug_focus || bitmap->depth != 8)
	{
		RECT fill;
		GetClientRect(debug_window, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}
	last = bitmap;

	// if we're iconic, don't bother
	if (IsIconic(debug_window))
		return;

	// default to using the raw bitmap data
	bitmap_base = bitmap->line[0];

	// for 8bpp bitmaps, update the debug colors
	for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
	{
		debug_dib_info->bmiColors[i].rgbRed		= dbg_palette[i * 3 + 0];
		debug_dib_info->bmiColors[i].rgbGreen	= dbg_palette[i * 3 + 1];
		debug_dib_info->bmiColors[i].rgbBlue	= dbg_palette[i * 3 + 2];
	}

	// fill in bitmap-specific info
	debug_dib_info->bmiHeader.biWidth = (bitmap->line[1] - bitmap->line[0]) / (bitmap->depth / 8);
	debug_dib_info->bmiHeader.biHeight = -bitmap->height;
	debug_dib_info->bmiHeader.biBitCount = bitmap->depth;

	// blit to the screen
	StretchDIBits(dc, 0, 0, bitmap->width, bitmap->height,
			0, 0, bitmap->width, bitmap->height,
			bitmap_base, debug_dib_info, DIB_RGB_COLORS, SRCCOPY);
}



//============================================================
//	debug_window_proc
//============================================================

static LRESULT CALLBACK debug_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// handle a few messages
	switch (message)
	{
		// paint: redraw the last bitmap
		case WM_PAINT:
		{
			PAINTSTRUCT pstruct;
			HDC hdc = BeginPaint(wnd, &pstruct);
			draw_debug_contents(hdc, NULL);
			EndPaint(wnd, &pstruct);
			break;
		}

		// get min/max info: set the minimum window size
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO *minmax = (MINMAXINFO *)lparam;
			minmax->ptMinTrackSize.x = 640;
			minmax->ptMinTrackSize.y = 480;
			break;
		}

		// sizing: constrain to the aspect ratio unless control key is held down
		case WM_SIZING:
		{
			InvalidateRect(debug_window, NULL, FALSE);
			break;
		}

		// destroy: close down the app
		case WM_DESTROY:
			debug_window = 0;
			break;

		// everything else: defaults
		default:
			return DefWindowProc(wnd, message, wparam, lparam);
	}

	return 0;
}



//============================================================
//	osd_debugger_focus
//============================================================

void osd_debugger_focus(int focus)
{
	debug_focus = focus;

	// if focused, make sure the window is visible
	if (debug_focus && debug_window)
	{
		// if full screen, turn it off
		if (!window_mode)
			toggle_full_screen();

		// show and restore the window
		ShowWindow(debug_window, SW_SHOW);
		ShowWindow(debug_window, SW_RESTORE);

		// make frontmost
		SetForegroundWindow(debug_window);

		// force an update
		update_debug_window(NULL);
	}

	// if not focuessed, bring the game frontmost
	else if (!debug_focus && debug_window)
	{
		// hide the window
		ShowWindow(debug_window, SW_HIDE);

		// make video frontmost
		SetForegroundWindow(video_window);
	}
}
