//============================================================
//
//	window.c - Win32 window handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
#include "winddraw.h"
#include "video.h"
#include "blit.h"
#include "mamedbg.h"
#include "../window.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win_pause_input(int pause);
extern int win_is_mouse_captured(void);
extern UINT8 win_trying_to_quit;



//============================================================
//	PARAMETERS
//============================================================

// window styles
#ifdef VPINMAME
#define WINDOW_STYLE			WS_OVERLAPPED|WS_THICKFRAME
#else
#define WINDOW_STYLE			WS_OVERLAPPEDWINDOW
#endif
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
//	GLOBAL VARIABLES
//============================================================

// command line config
int	win_window_mode;
int	win_wait_vsync;
int	win_use_ddraw;
int	win_triple_buffer;
int	win_hw_stretch;
int	win_gfx_width;
int	win_gfx_height;
int win_gfx_depth;
int win_old_scanlines;
int win_switch_res;
int win_switch_bpp;
int win_start_maximized;
int win_keep_aspect;
int win_gfx_refresh;
int win_match_refresh;
int win_sync_refresh;
float win_gfx_brightness;
int win_blit_effect;
float win_screen_aspect = (4.0 / 3.0);

// windows
HWND win_video_window;
HWND win_debug_window;

// video bounds
double win_aspect_ratio_adjust = 1.0;

// visible bounds
RECT win_visible_rect;
int win_visible_width;
int win_visible_height;

// 16bpp color conversion
int win_color16_rsrc_shift = 3;
int win_color16_gsrc_shift = 3;
int win_color16_bsrc_shift = 3;
int win_color16_rdst_shift = 10;
int win_color16_gdst_shift = 5;
int win_color16_bdst_shift = 0;

// 32bpp color conversion
int win_color32_rdst_shift = 16;
int win_color32_gdst_shift = 8;
int win_color32_bdst_shift = 0;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DIB bitmap data
static UINT8 video_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *video_dib_info = (BITMAPINFO *)video_dib_info_data;
static UINT8 debug_dib_info_data[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
static BITMAPINFO *debug_dib_info = (BITMAPINFO *)debug_dib_info_data;
static UINT8 *converted_bitmap;

// video bounds
static double aspect_ratio;

// visible bounds
static int visible_area_set;

// event handling
static TICKER last_event_check;

// derived attributes
static int swap_xy;
static int dual_monitor;
static int vector_game;
static int pixel_aspect_ratio;

// cached bounding rects
static RECT non_fullscreen_bounds;
static RECT non_maximized_bounds;

// debugger
static int debug_focus;
static int in_background;

// effects table
static struct win_effect_data effect_table[] =
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
	{ "sharp",   EFFECT_SHARP,       2, 2, 2, 2 },
};



//============================================================
//	PROTOTYPES
//============================================================

static void update_system_menu(void);
static LRESULT CALLBACK video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);
static void draw_video_contents(HDC dc, struct mame_bitmap *bitmap, int update);

static void dib_draw_window(HDC dc, struct mame_bitmap *bitmap, int update);

static int create_debug_window(void);
static void draw_debug_contents(HDC dc, struct mame_bitmap *bitmap);
static LRESULT CALLBACK debug_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);



//============================================================
//	wnd_extra_width
//============================================================

INLINE int wnd_extra_width(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
#ifdef VPINMAME
	AdjustWindowRectEx(&window, GetWindowLong(win_video_window, GWL_STYLE), FALSE, GetWindowLong(win_video_window, GWL_EXSTYLE));
#else
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
#endif
	return (window.right - window.left) - 100;
}



//============================================================
//	wnd_extra_height
//============================================================

INLINE int wnd_extra_height(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
#ifdef VPINMAME
	AdjustWindowRectEx(&window, GetWindowLong(win_video_window, GWL_STYLE), FALSE, GetWindowLong(win_video_window, GWL_EXSTYLE));
#else
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
#endif
	return (window.bottom - window.top) - 100;
}



//============================================================
//	wnd_extra_left
//============================================================

INLINE int wnd_extra_left(void)
{
	RECT window = { 100, 100, 200, 200 };
	if (!win_window_mode)
		return 0;
	AdjustWindowRectEx(&window, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
	return 100 - window.left;
}



//============================================================
//	get_aligned_window_pos
//============================================================

INLINE int get_aligned_window_pos(int x)
{
	// get a DC for the screen
	HDC dc = GetDC(NULL);
	if (dc)
	{
		// determine the pixel depth
		int bytes_per_pixel = (GetDeviceCaps(dc, BITSPIXEL) + 7) / 8;
		if (bytes_per_pixel)
		{
			// compute the amount necessary to align to 16 byte boundary
			int pixels_per_16bytes = 16 / bytes_per_pixel;
			int extra_left = wnd_extra_left();
#ifndef VPINMAME
			x = (((x + extra_left) + pixels_per_16bytes - 1) / pixels_per_16bytes) * pixels_per_16bytes - extra_left;
#else
			if ( x>=0 )
				x = (((x + extra_left) + pixels_per_16bytes - 1) / pixels_per_16bytes) * pixels_per_16bytes - extra_left;
			else
				x = (((x - extra_left) + pixels_per_16bytes - 1) / pixels_per_16bytes) * pixels_per_16bytes - extra_left;
#endif
		}

		// release the DC
		ReleaseDC(NULL, dc);
	}
	return x;
}



//============================================================
//	get_screen_bounds
//============================================================

INLINE void get_screen_bounds(RECT *bounds)
{
	// get a DC for the screen
	HDC dc = GetDC(NULL);

	// reset the bounds to a reasonable default
	bounds->top = bounds->left = 0;
	bounds->right = 640;
	bounds->bottom = 480;
	if (dc)
	{
		// get the bounds from the DC
		bounds->right = GetDeviceCaps(dc, HORZRES);
		bounds->bottom = GetDeviceCaps(dc, VERTRES);

		// release the DC
		ReleaseDC(NULL, dc);
	}
}



//============================================================
//	set_aligned_window_pos
//============================================================

INLINE void set_aligned_window_pos(HWND wnd, HWND insert, int x, int y, int cx, int cy, UINT flags)
{
	flags |= SWP_NOACTIVATE;
	SetWindowPos(wnd, insert, get_aligned_window_pos(x), y, cx, cy, flags);
}



//============================================================
//	erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, HDC dc)
{
	HBRUSH brush = GetStockObject(BLACK_BRUSH);
	RECT clear;

	// clear the left edge
	if (inner->left > outer->left)
	{
		clear = *outer;
		clear.right = inner->left;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (dc)
			FillRect(dc, &clear, brush);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
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
		if (win_gfx_width && (maximum->right - maximum->left) > (win_gfx_width + wnd_extra_width()))
		{
			int diff = (maximum->right - maximum->left) - (win_gfx_width + wnd_extra_width());
			maximum->left += diff / 2;
			maximum->right -= diff - (diff / 2);
		}

		// clamp to the height specified
		if (win_gfx_height && (maximum->bottom - maximum->top) > (win_gfx_height + wnd_extra_height()))
		{
			int diff = (maximum->bottom - maximum->top) - (win_gfx_height + wnd_extra_height());
			maximum->top += diff / 2;
			maximum->bottom -= diff - (diff / 2);
		}
	}
}



//============================================================
//	win_init_window
//============================================================

int win_init_window(void)
{
	static int classes_created = 0;
	char title[256];

	// disable win_old_scanlines if a win_blit_effect is active
	if (win_blit_effect != 0)
		win_old_scanlines = 0;

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
		// in case this is VPM, register the class regardless of the MAME options
		#ifndef VPINMAME
		if (options.mame_debug)
		#endif
		{
			wc.lpszClassName 	= "MAMEDebug";
			wc.lpfnWndProc		= debug_window_proc;

			// register the class; fail if we can't
			if (!RegisterClass(&wc))
				return 1;
		}
		#ifdef VPINMAME
		classes_created = 1;
		#endif
	}

	// make the window title
	// make the window title
#ifdef PINMAME
#ifdef VPINMAME
	sprintf(title, "%s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);
#else
	sprintf(title, "PINMAME: %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);
#endif
#else /* PINMAME */
#ifndef MESS
	sprintf(title, "MAME: %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);
#else
	sprintf(title, "MESS: %s [%s]", Machine->gamedrv->description, Machine->gamedrv->name);
#endif
#endif /* PINMAME */
	// create the window, but don't show it yet
	win_video_window = CreateWindowEx(win_window_mode ? WINDOW_STYLE_EX : FULLSCREEN_STYLE_EX,
			"MAME", title, win_window_mode ? WINDOW_STYLE : FULLSCREEN_STYLE,
			20, 20, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!win_video_window)
		return 1;

	// possibly create the debug window, but don't show it yet
	if (options.mame_debug)
		if (create_debug_window())
			return 1;

	// update system menu
	update_system_menu();
	return 0;
}

// moved them here (from local static vars in draw_video_contents and 
// draw_debug_contents) so they can be initialized in win_create_window
// (th, vpm team)

static struct mame_bitmap *last_video_bitmap = NULL;
static struct mame_bitmap *last_debug_bitmap = NULL;

//============================================================
//	win_create_window
//============================================================

int win_create_window(int width, int height, int depth, int attributes, int orientation)
{
	int i, result;

	// clear the last drawn bitmaps
	last_video_bitmap  = NULL;
	last_debug_bitmap = NULL;

	// clear the initial state
	visible_area_set = 0;

	// extract useful parameters from the orientation
	swap_xy				= ((orientation & ORIENTATION_SWAP_XY) != 0);

	// extract useful parameters from the attributes
	pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);
	dual_monitor		= ((attributes & VIDEO_DUAL_MONITOR) != 0);
	vector_game			= ((attributes & VIDEO_TYPE_VECTOR) != 0);

	// handle failure if we couldn't create the video window
	if (!win_video_window)
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
	set_aligned_window_pos(win_video_window, NULL, 20, 20,
			width + wnd_extra_width() + 2, height + wnd_extra_height() + 2,
			SWP_NOZORDER);

	// make sure we paint the window once here
	win_update_video_window(NULL);

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
	}

	// copy that same data into the debug DIB info
	memcpy(debug_dib_info_data, video_dib_info_data, sizeof(debug_dib_info_data));

	// finish off by trying to initialize DirectDraw
	if (win_use_ddraw)
	{
		result = win_ddraw_init(width, height, depth, attributes, &effect_table[win_blit_effect]);
		if (result)
			return result;
	}

	// determine the aspect ratio: hardware stretch case
	if (win_hw_stretch && win_use_ddraw)
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
//	win_destroy_window
//============================================================

void win_destroy_window(void)
{
	// kill directdraw
	win_ddraw_kill();

	// kill the window if it still exists
	if (win_video_window)
		DestroyWindow(win_video_window);
}



//============================================================
//	win_update_cursor_state
//============================================================

void win_update_cursor_state(void)
{
	if (win_window_mode && !win_is_mouse_captured())
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
	GetSystemMenu(win_video_window, TRUE);

	// add to the system menu
	menu = GetSystemMenu(win_video_window, FALSE);
	if (menu)
		AppendMenu(menu, MF_ENABLED | MF_STRING, MENU_FULLSCREEN, "Full Screen\tAlt+Enter");
}



//============================================================
//	win_update_video_window
//============================================================

void win_update_video_window(struct mame_bitmap *bitmap)
{
	// get the client DC and draw to it
	if (win_video_window)
	{
		HDC dc = GetDC(win_video_window);
		draw_video_contents(dc, bitmap, 0);
		ReleaseDC(win_video_window, dc);
	}
}



//============================================================
//	draw_video_contents
//============================================================

static void draw_video_contents(HDC dc, struct mame_bitmap *bitmap, int update)
{
	// if no bitmap, use the last one we got
	if (bitmap == NULL)
		bitmap = last_video_bitmap;

	// if no bitmap, just fill
	if (bitmap == NULL)
	{
		RECT fill;
		GetClientRect(win_video_window, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}
	last_video_bitmap = bitmap;

	// if we're iconic, don't bother
	if (IsIconic(win_video_window))
		return;

	// if we're in a window, constrain to a 16-byte aligned boundary
	if (win_window_mode && !update)
	{
		RECT original;
		int newleft;

		GetWindowRect(win_video_window, &original);
		newleft = get_aligned_window_pos(original.left);
		if (newleft != original.left)
			SetWindowPos(win_video_window, NULL, newleft, original.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// if we have a blit surface, use that
	if (win_use_ddraw && win_ddraw_draw(bitmap, update))
		return;

	// draw to the window with a DIB
	dib_draw_window(dc, bitmap, update);
}



//============================================================
//	video_window_proc
//============================================================

#ifdef VPINMAME
extern LRESULT CALLBACK osd_hook(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam, BOOL *pfhandled);
#endif 

static LRESULT CALLBACK video_window_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
#ifdef VPINMAME
	LRESULT lresult = 0;
	BOOL    fhandled = FALSE;
#endif

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
			minmax->ptMinTrackSize.x = win_visible_width + 2 + wnd_extra_width();
			minmax->ptMinTrackSize.y = win_visible_height + 2 + wnd_extra_height();
			break;
		}

		// sizing: constrain to the aspect ratio unless control key is held down
		case WM_SIZING:
		{
#ifndef VPINMAME
			RECT *rect = (RECT *)lparam;
			if (win_keep_aspect && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
				win_constrain_to_aspect_ratio(rect, wparam);
#endif
			InvalidateRect(win_video_window, NULL, FALSE);
			break;
		}

		// syscommands: catch win_start_maximized
		case WM_SYSCOMMAND:
		{
			InvalidateRect(win_video_window, NULL, FALSE);
			if ((wparam & 0xfff0) == SC_MAXIMIZE)
			{
				win_toggle_maximize();
				break;
			}
			else if (wparam == MENU_FULLSCREEN)
			{
				win_toggle_full_screen();
				break;
			}
			return DefWindowProc(wnd, message, wparam, lparam);
		}

		// destroy: close down the app
		case WM_DESTROY:
			win_ddraw_kill();
			win_trying_to_quit = 1;
			win_video_window = 0;
			break;

		// track whether we are in the foreground
		case WM_ACTIVATEAPP:
			in_background = !wparam;
			break;

		// everything else: defaults
		default:
#ifdef VPINMAME
			lresult = osd_hook(wnd, message, wparam, lparam, &fhandled);
			return fhandled?lresult:DefWindowProc(wnd, message, wparam, lparam);
#else

			return DefWindowProc(wnd, message, wparam, lparam);
#endif
	}

#ifdef VPINMAME
	lresult = osd_hook(wnd, message, wparam, lparam, &fhandled);
	return fhandled?lresult:0;
#else
	return 0;
#endif
}


//============================================================
//	win_constrain_to_aspect_ratio
//============================================================

void win_constrain_to_aspect_ratio(RECT *rect, int adjustment)
{
	double adjusted_ratio = aspect_ratio;
	int extrawidth = wnd_extra_width();
	int extraheight = wnd_extra_height();
	int newwidth, newheight, adjwidth, adjheight;
	RECT minrect, maxrect, temp;

	// adjust if hardware stretching
	if (win_use_ddraw && win_hw_stretch)
		adjusted_ratio *= win_aspect_ratio_adjust;

	// determine the minimum rect
	minrect = *rect;
	minrect.right = minrect.left + (win_visible_width + 2) + extrawidth;
	minrect.bottom = minrect.top + (int)((double)(win_visible_width + 2) / adjusted_ratio) + extraheight;
	temp = *rect;
	temp.right = temp.left + (int)((double)(win_visible_height + 2) * adjusted_ratio) + extrawidth;
	temp.bottom = temp.top + (win_visible_height + 2) + extraheight;
	if (temp.right > minrect.right || temp.bottom > minrect.bottom)
		minrect = temp;

	// expand the initial rect past the minimum
	temp = *rect;
	UnionRect(rect, &temp, &minrect);

	// determine the maximum rect
	if (win_window_mode)
		get_work_area(&maxrect);
	else
		get_screen_bounds(&maxrect);

	// clamp the initial rect to its maxrect box
	temp = *rect;
	IntersectRect(rect, &temp, &maxrect);

	// if we're not forcing the aspect ratio, just return the intersection
	if (!win_keep_aspect)
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

void win_adjust_window_for_visible(int min_x, int max_x, int min_y, int max_y)
{
	// set the new values
	win_visible_rect.left = min_x;
	win_visible_rect.top = min_y;
	win_visible_rect.right = max_x + 1;
	win_visible_rect.bottom = max_y + 1;
	win_visible_width = win_visible_rect.right - win_visible_rect.left;
	win_visible_height = win_visible_rect.bottom - win_visible_rect.top;

	// if we're not using hardware stretching, recompute the aspect ratio
	if (!win_hw_stretch || !win_use_ddraw)
	{
		aspect_ratio = (double)win_visible_width / (double)win_visible_height;
		if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
			aspect_ratio *= 2.0;
		else if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
			aspect_ratio /= 2.0;
	}

	// adjust the window
	win_adjust_window();

	// first time through here, we need to show the window
	if (!visible_area_set)
	{
		// let's also win_start_maximized the window
		if (win_window_mode)
		{
			RECT bounds, work;

			// compute the non-maximized bounds here
			get_work_area(&work);
			GetWindowRect(win_video_window, &bounds);
			non_maximized_bounds.left = work.left + ((work.right - work.left) - (bounds.right - bounds.left)) / 2;
			non_maximized_bounds.top = work.top + ((work.bottom - work.top) - (bounds.bottom - bounds.top)) / 2;
			non_maximized_bounds.right = non_maximized_bounds.left + bounds.right - bounds.left;
			non_maximized_bounds.bottom = non_maximized_bounds.top + bounds.bottom - bounds.top;

			// if maximizing, toggle it
			if (win_start_maximized)
				win_toggle_maximize();

			// otherwise, just enforce the bounds
			else
				set_aligned_window_pos(win_video_window, NULL, non_maximized_bounds.left, non_maximized_bounds.top,
						non_maximized_bounds.right - non_maximized_bounds.left,
						non_maximized_bounds.bottom - non_maximized_bounds.top,
						SWP_NOZORDER);
		}

		// kludge to fix full screen mode for the non-ddraw case
		if (!win_use_ddraw && !win_window_mode)
		{
			win_window_mode = 1;
			win_toggle_full_screen();
			memset(&non_fullscreen_bounds, 0, sizeof(non_fullscreen_bounds));
		}

#ifndef VPINMAME
		// show the result
		ShowWindow(win_video_window, SW_SHOW);
		SetForegroundWindow(win_video_window);
#endif
		win_update_video_window(NULL);

		// update the cursor state
		win_update_cursor_state();

		// unpause the input devices
		win_pause_input(0);
		visible_area_set = 1;
	}
}



//============================================================
//	win_toggle_maximize
//============================================================

void win_toggle_maximize(void)
{
	RECT current, maximum;

	// get the current position
	GetWindowRect(win_video_window, &current);

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
		win_constrain_to_aspect_ratio(&current, WMSZ_BOTTOMRIGHT);

		// if we're not stretching, compute the multipliers
		if (!win_hw_stretch || !win_use_ddraw)
		{
			int xmult, ymult;

			current.right -= wnd_extra_width() + 2;
			current.bottom -= wnd_extra_height() + 2;
			win_compute_multipliers(&current, &xmult, &ymult);
			current.right = current.left + win_visible_width * xmult + wnd_extra_width() + 2;
			current.bottom = current.top + win_visible_height * ymult + wnd_extra_height() + 2;
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
	set_aligned_window_pos(win_video_window, NULL, current.left, current.top,
			current.right - current.left, current.bottom - current.top,
			SWP_NOZORDER);
}



//============================================================
//	win_toggle_full_screen
//============================================================

void win_toggle_full_screen(void)
{
	// rip down DirectDraw
	if (win_use_ddraw)
		win_ddraw_kill();

	// hide the window
	ShowWindow(win_video_window, SW_HIDE);
	if (win_window_mode && win_debug_window)
		ShowWindow(win_debug_window, SW_HIDE);

	// toggle the window mode
	win_window_mode = !win_window_mode;

	// adjust the window style and z order
	if (win_window_mode)
	{
		// adjust the style
		SetWindowLong(win_video_window, GWL_STYLE, WINDOW_STYLE);
		SetWindowLong(win_video_window, GWL_EXSTYLE, WINDOW_STYLE_EX);
		set_aligned_window_pos(win_video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// force to the bottom, then back on top
		set_aligned_window_pos(win_video_window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		set_aligned_window_pos(win_video_window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		// adjust the bounds
		if (non_fullscreen_bounds.right != non_fullscreen_bounds.left)
			set_aligned_window_pos(win_video_window, HWND_TOP, non_fullscreen_bounds.left, non_fullscreen_bounds.top,
						non_fullscreen_bounds.right - non_fullscreen_bounds.left, non_fullscreen_bounds.bottom - non_fullscreen_bounds.top,
						SWP_NOZORDER);
		else
		{
			set_aligned_window_pos(win_video_window, HWND_TOP, 0, 0, win_visible_width + 2, win_visible_height + 2, SWP_NOZORDER);
			win_toggle_maximize();
		}
	}
	else
	{
		// save the bounds
		GetWindowRect(win_video_window, &non_fullscreen_bounds);

		// adjust the style
		SetWindowLong(win_video_window, GWL_STYLE, FULLSCREEN_STYLE);
		SetWindowLong(win_video_window, GWL_EXSTYLE, FULLSCREEN_STYLE_EX);
		set_aligned_window_pos(win_video_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		// set topmost
		set_aligned_window_pos(win_video_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	// adjust the window to compensate for the change
	win_adjust_window();
	update_system_menu();

	// show and adjust the window
	ShowWindow(win_video_window, SW_SHOW);
	if (win_window_mode && win_debug_window)
		ShowWindow(win_debug_window, SW_SHOW);

	// reinit
	if (win_use_ddraw)
		if (win_ddraw_init(0, 0, 0, 0, NULL))
			exit(1);

	// make sure the window is properly readjusted
	win_adjust_window();
}



//============================================================
//	win_adjust_window
//============================================================

void win_adjust_window(void)
{
	RECT original, window;

	// get the current size
	GetWindowRect(win_video_window, &original);

	// adjust the window size so the client area is what we want
	if (win_window_mode)
	{
		// constrain the existing size to the aspect ratio
		window = original;
		win_constrain_to_aspect_ratio(&window, WMSZ_BOTTOMRIGHT);
	}

	// in full screen, make sure it covers the primary display
	else
		get_screen_bounds(&window);

	// adjust the position if different
	if (original.left != window.left ||
		original.top != window.top ||
		original.right != window.right ||
		original.bottom != window.bottom ||
		original.left != get_aligned_window_pos(original.left))
		set_aligned_window_pos(win_video_window, win_window_mode ? HWND_TOP : HWND_TOPMOST,
				window.left, window.top,
				window.right - window.left, window.bottom - window.top, 0);

	// update the cursor state
	win_update_cursor_state();
}



//============================================================
//	win_process_events_periodic
//============================================================

void win_process_events_periodic(void)
{
	TICKER curr = ticker();
	if (curr - last_event_check < TICKS_PER_SEC / 8)
		return;
	win_process_events();
}



//============================================================
//	win_process_events
//============================================================

int win_process_events(void)
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

	// return 1 if we slept this frame
	return 0;
}



//============================================================
//	wait_for_vsync
//============================================================

void win_wait_for_vsync(void)
{
	// if we have DirectDraw, we can use that
	if (win_use_ddraw)
		win_ddraw_wait_vsync();
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
//	win_prepare_palette
//============================================================

UINT32 *win_prepare_palette(struct win_blit_params *params)
{
	// 16bpp source only needs a palette if RGB direct or modifiable
	if (params->srcdepth == 15 || params->srcdepth == 16)
		return (params->dstdepth == 16) ? palette_16bit_lookup : palette_32bit_lookup;

	// nobody else needs it
	return NULL;
}



//============================================================
//	dib_draw_window
//============================================================

static void dib_draw_window(HDC dc, struct mame_bitmap *bitmap, int update)
{
	int depth = (bitmap->depth == 15) ? 16 : bitmap->depth;
	struct win_blit_params params;
	int xmult, ymult;
	RECT client;
	int cx, cy;

	// compute the multipliers
	GetClientRect(win_video_window, &client);
	win_compute_multipliers(&client, &xmult, &ymult);

	// blit to our temporary bitmap
	params.dstdata		= (void *)(((UINT32)converted_bitmap + 15) & ~15);
	params.dstpitch		= (((win_visible_width * xmult) + 3) & ~3) * depth / 8;
	params.dstdepth		= depth;
	params.dstxoffs		= 0;
	params.dstyoffs		= 0;
	params.dstxscale	= xmult;
	params.dstyscale	= (!win_old_scanlines || ymult == 1) ? ymult : ymult - 1;
	params.dstyskip		= (!win_old_scanlines || ymult == 1) ? 0 : 1;
	params.dsteffect	= win_determine_effect(&params);

	params.srcdata		= bitmap->line[0];
	params.srcpitch		= ((UINT8 *)bitmap->line[1]) - ((UINT8 *)bitmap->line[0]);
	params.srcdepth		= bitmap->depth;
	params.srclookup	= win_prepare_palette(&params);
	params.srcxoffs		= win_visible_rect.left;
	params.srcyoffs		= win_visible_rect.top;
	params.srcwidth		= win_visible_width;
	params.srcheight	= win_visible_height;

	params.dirtydata	= use_dirty ? dirty_grid : NULL;
	params.dirtypitch	= DIRTY_H;

	win_perform_blit(&params, update);

	// fill in bitmap-specific info
	video_dib_info->bmiHeader.biWidth = params.dstpitch / (depth / 8);
	video_dib_info->bmiHeader.biHeight = -win_visible_height * ymult;
	video_dib_info->bmiHeader.biBitCount = depth;

	// compute the center position
	cx = client.left + ((client.right - client.left) - win_visible_width * xmult) / 2;
	cy = client.top + ((client.bottom - client.top) - win_visible_height * ymult) / 2;

	// blit to the screen
	StretchDIBits(dc, cx, cy, win_visible_width * xmult, win_visible_height * ymult,
				0, 0, win_visible_width * xmult, win_visible_height * ymult,
				converted_bitmap, video_dib_info, DIB_RGB_COLORS, SRCCOPY);

	// erase the edges if updating
	if (update)
	{
		RECT inner;

		inner.left = cx;
		inner.top = cy;
		inner.right = cx + win_visible_width * xmult;
		inner.bottom = cy + win_visible_height * ymult;
		erase_outer_rect(&client, &inner, dc);
	}
}



//============================================================
//	lookup_effect
//============================================================

int win_lookup_effect(const char *arg)
{
	int effindex;

	// loop through all the effects and find a match
	for (effindex = 0; effindex < sizeof(effect_table) / sizeof(effect_table[0]); effindex++)
		if (!strcmp(arg, effect_table[effindex].name))
			return effindex;

	return -1;
}



//============================================================
//	win_determine_effect
//============================================================

int win_determine_effect(const struct win_blit_params *params)
{
	// default to what was selected
	int result = effect_table[win_blit_effect].effect;

	// if we're out of range, revert to NONE
	if (params->dstxscale < effect_table[win_blit_effect].min_xscale ||
		params->dstxscale > effect_table[win_blit_effect].max_xscale ||
		params->dstyscale < effect_table[win_blit_effect].min_yscale ||
		params->dstyscale > effect_table[win_blit_effect].max_yscale)
		result = EFFECT_NONE;

	return result;
}



//============================================================
//	win_compute_multipliers
//============================================================

void win_compute_multipliers(const RECT *rect, int *xmult, int *ymult)
{
	// first compute simply
	*xmult = (rect->right - rect->left) / win_visible_width;
	*ymult = (rect->bottom - rect->top) / win_visible_height;

	// clamp to the hardcoded max
	if (*xmult > MAX_X_MULTIPLY)
		*xmult = MAX_X_MULTIPLY;
	if (*ymult > MAX_Y_MULTIPLY)
		*ymult = MAX_Y_MULTIPLY;

	// clamp to the effect max
	if (*xmult > effect_table[win_blit_effect].max_xscale)
		*xmult = effect_table[win_blit_effect].max_xscale;
	if (*ymult > effect_table[win_blit_effect].max_yscale)
		*ymult = effect_table[win_blit_effect].max_yscale;

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
	win_debug_window = CreateWindowEx(DEBUG_WINDOW_STYLE_EX, "MAMEDebug", title, DEBUG_WINDOW_STYLE,
			work_bounds.right - (bounds.right - bounds.left),
			work_bounds.bottom - (bounds.bottom - bounds.top),
			bounds.right - bounds.left, bounds.bottom - bounds.top,
			win_video_window, NULL, GetModuleHandle(NULL), NULL);
	if (!win_debug_window)
		return 1;
#endif

	return 0;
}



//============================================================
//	win_update_debug_window
//============================================================

void win_update_debug_window(struct mame_bitmap *bitmap)
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
	if (win_debug_window)
	{
		HDC dc = GetDC(win_debug_window);
		draw_debug_contents(dc, bitmap);
		ReleaseDC(win_debug_window, dc);
	}
#endif
}



//============================================================
//	draw_debug_contents
//============================================================

static void draw_debug_contents(HDC dc, struct mame_bitmap *bitmap)
{
	UINT8 *bitmap_base;
	int i;

	// if no bitmap, use the last one we got
	if (bitmap == NULL)
		bitmap = last_debug_bitmap;

	// if no bitmap, just fill
	if (bitmap == NULL || !debug_focus || bitmap->depth != 8)
	{
		RECT fill;
		GetClientRect(win_debug_window, &fill);
		FillRect(dc, &fill, (HBRUSH)GetStockObject(BLACK_BRUSH));
		return;
	}
	last_debug_bitmap = bitmap;

	// if we're iconic, don't bother
	if (IsIconic(win_debug_window))
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
	debug_dib_info->bmiHeader.biWidth = (((UINT8 *)bitmap->line[1]) - ((UINT8 *)bitmap->line[0])) / (bitmap->depth / 8);
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
			InvalidateRect(win_debug_window, NULL, FALSE);
			break;
		}

		// destroy: close down the app
		case WM_DESTROY:
			win_debug_window = 0;
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
	if (debug_focus && win_debug_window)
	{
		// if full screen, turn it off
		if (!win_window_mode)
			win_toggle_full_screen();

		// show and restore the window
		ShowWindow(win_debug_window, SW_SHOW);
		ShowWindow(win_debug_window, SW_RESTORE);

		// make frontmost
		SetForegroundWindow(win_debug_window);

		// force an update
		win_update_debug_window(NULL);
	}

	// if not focuessed, bring the game frontmost
	else if (!debug_focus && win_debug_window)
	{
		// hide the window
		ShowWindow(win_debug_window, SW_HIDE);

		// make video frontmost
		SetForegroundWindow(win_video_window);
	}
}
