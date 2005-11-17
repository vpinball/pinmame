//============================================================
//
//	winddraw.c - Win32 DirectDraw code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#ifdef VPINMAME
#define NONAMELESSUNION
#endif // VPINMAME
#include <windows.h>

#ifdef __GNUC__
#define NONAMELESSUNION
#endif

#include <ddraw.h>

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern int verbose;

// from video.c
GUID *screen_guid_ptr;


//============================================================
//	DEBUGGING
//============================================================

#define SHOW_FLIP_TIMES 		0



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectDraw objects
static LPDIRECTDRAW ddraw;
static LPDIRECTDRAW4 ddraw4;
static LPDIRECTDRAWGAMMACONTROL gamma_control;
static LPDIRECTDRAWSURFACE primary_surface;
static LPDIRECTDRAWSURFACE back_surface;
static LPDIRECTDRAWSURFACE blit_surface;
static LPDIRECTDRAWCLIPPER primary_clipper;

// DirectDraw object info
static DDCAPS ddraw_caps;
static DDSURFACEDESC primary_desc;
static DDSURFACEDESC blit_desc;
static int changed_resolutions;
static int forced_updates;

// video bounds
static int max_width;
static int max_height;
static int pref_depth;
static int effect_min_xscale;
static int effect_min_yscale;
static struct rectangle last_bounds;

// mode finding
static double best_score;
static int best_width;
static int best_height;
static int best_depth;
static int best_refresh;

// derived attributes
static int needs_6bpp_per_gun;
static int pixel_aspect_ratio;



//============================================================
//	PROTOTYPES
//============================================================

static double compute_mode_score(int width, int height, int depth, int refresh);
static int set_resolution(void);
static int create_surfaces(void);
static int create_blit_surface(void);
static void set_brightness(void);
static int create_clipper(void);
static void erase_surfaces(void);
static void release_surfaces(void);
static void compute_color_masks(const DDSURFACEDESC *desc);
static int render_to_blit(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
static int render_to_primary(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
static int blit_and_flip(LPDIRECTDRAWSURFACE target_surface, LPRECT src, LPRECT dst, int update);



//============================================================
//	win_ddraw_fullscreen_margins
//============================================================

void win_ddraw_fullscreen_margins(DWORD desc_width, DWORD desc_height, RECT *margins)
{
	margins->left = 0;
	margins->top = 0;
	margins->right = desc_width;
	margins->bottom = desc_height;
	if (win_has_menu())
	{
		static int height_with_menubar = 0;
		if (height_with_menubar == 0)
		{
			RECT with_menu = { 100, 100, 200, 200 };
			RECT without_menu = { 100, 100, 200, 200 };
			AdjustWindowRect(&with_menu, WS_OVERLAPPED, TRUE);
			AdjustWindowRect(&without_menu, WS_OVERLAPPED, FALSE);
			height_with_menubar = (with_menu.bottom - with_menu.top) - (without_menu.bottom - without_menu.top);
		}
		margins->top = height_with_menubar;
	}
}



//============================================================
//	erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, LPDIRECTDRAWSURFACE surface)
{
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
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
		if (surface)
			IDirectDrawSurface_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	win_ddraw_wait_vsync
//============================================================

void win_ddraw_wait_vsync(void)
{
	HRESULT result;
	BOOL is_vblank;

	// if we're not already in VBLANK, wait for it
	result = IDirectDraw_GetVerticalBlankStatus(ddraw, &is_vblank);
	if (result == DD_OK && !is_vblank)
		result = IDirectDraw_WaitForVerticalBlank(ddraw, DDWAITVB_BLOCKBEGIN, 0);
}



//============================================================
//	win_ddraw_init
//============================================================

int win_ddraw_init(int width, int height, int depth, int attributes, const struct win_effect_data *effect)
{
	HRESULT result;
	DDCAPS hel_caps;

	// set the graphics mode width/height to the window size
	if (width && height && depth)
	{
		max_width = width;
		max_height = height;
		pref_depth = depth;
		needs_6bpp_per_gun	= ((attributes & VIDEO_NEEDS_6BITS_PER_GUN) != 0);
		pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);
	}
	if (effect)
	{
		effect_min_xscale = effect->min_xscale;
		effect_min_yscale = effect->min_yscale;
	}

	// now attempt to create it
	result = DirectDrawCreate(screen_guid_ptr, &ddraw, NULL);
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
	if (win_dd_hw_stretch)
		win_dd_hw_stretch = ((ddraw_caps.dwCaps & DDCAPS_BLTSTRETCH) != 0);
	if (win_dd_hw_stretch && verbose)
		fprintf(stderr, "Hardware stretching supported\n");

	// set the cooperative level
	// for non-window modes, we will use full screen here
	result = IDirectDraw_SetCooperativeLevel(ddraw, win_video_window, win_window_mode ? DDSCL_NORMAL : DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting cooperative level: %08x\n", (UINT32)result);
		goto cant_set_coop_level;
	}

	// full screen mode: set the resolution
	changed_resolutions = 0;
	if (set_resolution())
		goto cant_set_resolution;

	// create the surfaces
	if (create_surfaces())
		goto cant_create_surfaces;

	// force some updates
	forced_updates = 5;
	return 0;

	// error handling
cant_create_surfaces:
cant_set_resolution:
cant_set_coop_level:
cant_get_caps:
	IDirectDraw_Release(ddraw);
cant_create_ddraw:
	ddraw = NULL;
	return 1;
}



//============================================================
//	win_ddraw_kill
//============================================================

void win_ddraw_kill(void)
{
	// release the surfaces
	release_surfaces();

	// restore resolutions
	if (ddraw != NULL && changed_resolutions)
		IDirectDraw_RestoreDisplayMode(ddraw);

	// reset cooperative level
	if (ddraw != NULL && win_video_window != 0)
		IDirectDraw_SetCooperativeLevel(ddraw, win_video_window, DDSCL_NORMAL);

	// delete the core objects
	if (ddraw4 != NULL)
		IDirectDraw4_Release(ddraw4);
	ddraw4 = NULL;
	if (ddraw != NULL)
		IDirectDraw_Release(ddraw);
	ddraw = NULL;
}



//============================================================
//	enum_callback
//============================================================

static HRESULT WINAPI enum_callback(LPDDSURFACEDESC desc, LPVOID context)
{
	int depth = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	double score;

	// compute this mode's score
	score = compute_mode_score(desc->dwWidth, desc->dwHeight, depth, 0);

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
//	enum2_callback
//============================================================

static HRESULT WINAPI enum2_callback(LPDDSURFACEDESC2 desc, LPVOID context)
{
	int refresh = (win_match_refresh || win_gfx_refresh) ? desc->DUMMYUNIONNAMEN(2).dwRefreshRate : 0;
	int depth = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	double score;

	// compute this mode's score
	score = compute_mode_score(desc->dwWidth, desc->dwHeight, depth, refresh);

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
//	compute_mode_score
//============================================================

static double compute_mode_score(int width, int height, int depth, int refresh)
{
	static const double depth_matrix[4][2][4] =
	{
			// !needs_6bpp_per_gun		  // needs_6bpp_per_gun
		{ { 0.00, 0.75, 0.25, 0.50 },	{ 0.00, 0.25, 0.50, 0.75 } },	// 8bpp source
		{ { 0.00, 1.00, 0.25, 0.50 },	{ 0.00, 0.50, 0.75, 1.00 } },	// 16bpp source
		{ { 0.00, 0.00, 0.00, 0.00 },	{ 0.00, 0.00, 0.00, 0.00 } },	// 24bpp source (doesn't exist)
		{ { 0.00, 0.50, 0.75, 1.00 },	{ 0.00, 0.50, 0.75, 1.00 } }	// 32bpp source
	};

	double size_score, depth_score, refresh_score, final_score;
	int target_width, target_height;

	// first compute a score based on size

	// if not stretching, we need to keep minx and miny scale equal
	if (!win_dd_hw_stretch)
	{
		if (effect_min_yscale > effect_min_xscale)
			effect_min_xscale = effect_min_yscale;
		else
			effect_min_yscale = effect_min_xscale;
	}

	// determine minimum requirements
	target_width = max_width * effect_min_xscale;
	target_height = max_height * effect_min_yscale;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		target_height *= 2;
	else if (win_old_scanlines)
		target_width *= 2, target_height *= 2;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		target_width *= 2;

	// hardware stretch modes prefer at least win_gfx_zoom times expansion (default is 2)
	if (win_dd_hw_stretch)
	{
		if (target_width < max_width * win_gfx_zoom + 2)
			target_width = max_width * win_gfx_zoom + 2;
		if (target_height < max_height * win_gfx_zoom + 2)
			target_height = max_height * win_gfx_zoom + 2;
	}

	// compute initial score based on difference between target and current
	size_score = 1.0 / (1.0 + fabs(width - target_width) + fabs(height - target_height));

	// if we're looking for a particular mode, make sure it matches
	if (win_gfx_width && win_gfx_height && (width != win_gfx_width || height != win_gfx_height))
		return 0.0;

	// if mode is too small, it's a zero, unless the user specified otherwise
	if ((width < max_width || height < max_height) && (!win_gfx_width || !win_gfx_height))
		return 0.0;

	// if mode is smaller than we'd like, it only scores up to 0.1
	if (width < target_width || height < target_height)
		size_score *= 0.1;

	// next compute depth score
	depth_score = depth_matrix[(pref_depth + 7) / 8 - 1][needs_6bpp_per_gun][(depth + 7) / 8 - 1];

	// only 16bpp and above now supported
	if (depth < 16)
		return 0.0;

	// if we're looking for a particular depth, make sure it matches
	if (win_gfx_depth && depth != win_gfx_depth)
		return 0.0;

	// finally, compute refresh score
	refresh_score = 1.0 / (1.0 + fabs((double)refresh - Machine->drv->frames_per_second));

	// if we're looking for a particular refresh, make sure it matches
	if (win_gfx_refresh && refresh && refresh != win_gfx_refresh)
		return 0.0;

	// if refresh is smaller than we'd like, it only scores up to 0.1
	if ((double)refresh < Machine->drv->frames_per_second)
		refresh_score *= 0.1;

	// weight size highest, followed by depth and refresh
	final_score = (size_score * 100.0 + depth_score * 10.0 + refresh_score) / 111.0;
	return final_score;
}



//============================================================
//	set_resolution
//============================================================

static int set_resolution(void)
{
	DDSURFACEDESC currmode = { sizeof(DDSURFACEDESC) };
	double resolution_aspect;
	HRESULT result;

	// skip if not switching resolution
	if (!win_window_mode && (win_switch_res || win_switch_bpp))
	{
		// if we're only switching depth, set win_gfx_width and win_gfx_height to the current resolution
		if (!win_switch_res || !win_switch_bpp)
		{
			// attempt to get the current display mode
			result = IDirectDraw_GetDisplayMode(ddraw, &currmode);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
				goto cant_get_mode;
			}

			// force to the current width/height
			if (!win_switch_res)
			{
				win_gfx_width = currmode.dwWidth;
				win_gfx_height = currmode.dwHeight;
			}
			if (!win_switch_bpp)
				win_gfx_depth = currmode.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
		}

		// enumerate display modes
		best_score = 0.0;
		if (ddraw4)
			result = IDirectDraw4_EnumDisplayModes(ddraw4, (win_match_refresh || win_gfx_refresh) ? DDEDM_REFRESHRATES : 0, NULL, NULL, enum2_callback);
		else
			result = IDirectDraw_EnumDisplayModes(ddraw, 0, NULL, NULL, enum_callback);
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
	win_aspect_ratio_adjust = resolution_aspect / win_screen_aspect;
	return 0;

	// error handling - non fatal in general
cant_set_mode:
cant_get_mode:
cant_enumerate_modes:
	return 0;
}



//============================================================
//	create_surfaces
//============================================================

static int create_surfaces(void)
{
	HRESULT result;

	// make a description of the primary surface
	memset(&primary_desc, 0, sizeof(primary_desc));
	primary_desc.dwSize = sizeof(primary_desc);
	primary_desc.dwFlags = DDSD_CAPS;
	primary_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	// for full screen mode, allocate flipping surfaces
	if (!win_window_mode && win_triple_buffer)
	{
		primary_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		primary_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		primary_desc.dwBackBufferCount = 2;
	}

	// then create the primary surface
	result = IDirectDraw_CreateSurface(ddraw, &primary_desc, &primary_surface, NULL);
	if (result != DD_OK)
	{
		if (verbose)
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

	// determine the color masks and force the palette to recalc
	compute_color_masks(&primary_desc);

	// if this is a full-screen mode, attempt to create a color control object
	if (!win_window_mode && win_gfx_brightness != 0.0)
		set_brightness();

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
	if (!win_window_mode && win_triple_buffer)
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
	if (win_dd_hw_stretch)
	{
		if (create_blit_surface())
			goto cant_create_blit;
	}

	// create a clipper for windowed mode
	if (win_window_mode)
	{
		if (create_clipper())
			goto cant_init_clipper;
	}

	// erase all the surfaces we created
	erase_surfaces();
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
cant_get_primary_desc:
	IDirectDrawSurface_Release(primary_surface);
	primary_surface = NULL;
cant_create_primary:
	return 1;
}



//============================================================
//	create_blit_surface
//============================================================

static int create_blit_surface(void)
{
	int width, height;
	HRESULT result;
	int done = 0;

	// determine the width/height of the blit surface
	while (!done)
	{
		done = 1;

		// first compute the ideal size
		width = (max_width * effect_min_xscale) + 18;
		height = (max_height * effect_min_yscale) + 2;

		// if it's okay, keep it
		if (width <= primary_desc.dwWidth && height <= primary_desc.dwHeight)
			break;

		// reduce the width
		if (width > primary_desc.dwWidth)
		{
			if (effect_min_xscale > 1)
			{
				done = 0;
				effect_min_xscale--;
			}
		}

		// reduce the height
		if (height > primary_desc.dwHeight)
		{
			if (effect_min_yscale > 1)
			{
				done = 0;
				effect_min_yscale--;
			}
		}
	}

	// now make a description of our blit surface, based on the primary surface
	blit_desc = primary_desc;
	blit_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
	blit_desc.dwWidth = width;
	blit_desc.dwHeight = height;
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
		if (verbose)
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

	// blit surface color masks override the primary surface color masks
	compute_color_masks(&blit_desc);

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
//	set_brightness
//============================================================

static void set_brightness(void)
{
	HRESULT result;

	// see if we can get a GammaControl object
	result = IDirectDrawSurface_QueryInterface(primary_surface, &IID_IDirectDrawGammaControl, (void **)&gamma_control);
	if (result != DD_OK)
	{
		if (verbose)
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
			double val = ((float)i / 255.0) * win_gfx_brightness;
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
//	create_clipper
//============================================================

static int create_clipper(void)
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
	result = IDirectDrawClipper_SetHWnd(primary_clipper, 0, win_video_window);
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
//	erase_surfaces
//============================================================

static void erase_surfaces(void)
{
	DDBLTFX blitfx = { sizeof(DDBLTFX) };
	HRESULT result = DD_OK;
	int i;

	// erase the blit surface
	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;
	if (blit_surface)
		result = IDirectDrawSurface_Blt(blit_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// loop through enough to get all the back buffers
	if (!win_window_mode)
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
//	release_surfaces
//============================================================

static void release_surfaces(void)
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

	// release the primary surface
	if (primary_surface)
		IDirectDrawSurface_Release(primary_surface);
	primary_surface = NULL;
}



//============================================================
//	compute_color_masks
//============================================================

static void compute_color_masks(const DDSURFACEDESC *desc)
{
	// 16bpp case
	if (desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
	{
		int temp;

		// red
		win_color16_rdst_shift = win_color16_rsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_rdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_rsrc_shift++;

		// green
		win_color16_gdst_shift = win_color16_gsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_gdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_gsrc_shift++;

		// blue
		win_color16_bdst_shift = win_color16_bsrc_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_bdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_bsrc_shift++;
	}

	// 24/32bpp case
	else if (desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 24 ||
			 desc->ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 32)
	{
		int temp;

		// red
		win_color32_rdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_rdst_shift++;

		// green
		win_color32_gdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_gdst_shift++;

		// blue
		win_color32_bdst_shift = 0;
		temp = desc->ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_bdst_shift++;
	}

	// mark the lookups invalid
	palette_lookups_invalid = 1;
}



//============================================================
//	win_ddraw_draw
//============================================================

int win_ddraw_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int result;

	// handle forced updates
	if (forced_updates)
	{
		forced_updates--;
		update = 1;
	}

	// if we don't have our surfaces, try to recreate them
	if (!primary_surface)
	{
		release_surfaces();
		if (create_surfaces())
			return 0;
	}

	// if we're using hardware stretching, render to the blit surface,
	// then blit that and stretch
	if (win_dd_hw_stretch)
		result = render_to_blit(bitmap, bounds, vector_dirty_pixels, update);

	// otherwise, render directly to the primary/back surface
	else
		result = render_to_primary(bitmap, bounds, vector_dirty_pixels, update);

	return result;
}



//============================================================
//	lock_must_succeed
//============================================================

static int lock_must_succeed(const struct rectangle *bounds, void *vector_dirty_pixels)
{
	// determine up front if this lock must succeed; by default, it depends on
	// whether or not we're throttling
	int result = throttle;

	// if we're using dirty pixels, we must succeed as well, or else we will leave debris
	if (vector_dirty_pixels)
		result = 1;

	// if we're blitting a different source rect than before, we also must
	// succeed, or else we will miss some areas
	if (bounds)
	{
		if (bounds->min_x != last_bounds.min_x || bounds->min_y != last_bounds.min_y ||
			bounds->max_x != last_bounds.max_x || bounds->max_y != last_bounds.max_y)
			result = 1;
		last_bounds = *bounds;
	}

	return result;
}



//============================================================
//	render_to_blit
//============================================================

static int render_to_blit(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int dstdepth = blit_desc.ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	int wait_for_lock = lock_must_succeed(bounds, vector_dirty_pixels);
	LPDIRECTDRAWSURFACE target_surface;
	struct win_blit_params params;
	HRESULT result;
	RECT src, dst, margins;
	int dstxoffs;

tryagain:
	// attempt to lock the blit surface
#ifdef VPINMAME
        if (!blit_surface) goto surface_lost;
#endif /* VPINMAME */
	result = IDirectDrawSurface_Lock(blit_surface, NULL, &blit_desc, wait_for_lock ? DDLOCK_WAIT : 0, NULL);
	if (result == DDERR_SURFACELOST)
		goto surface_lost;

	// if it was busy (and we're not throttling), just punt
	if (result == DDERR_SURFACEBUSY || result == DDERR_WASSTILLDRAWING)
		return 1;
	if (result != DD_OK)
	{
		if (verbose)
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
	params.dstxscale	= effect_min_xscale;
	params.dstyscale	= effect_min_yscale;
	params.dstyskip		= 0;
	params.dsteffect	= win_determine_effect(&params);

	params.srcdata		= bitmap->base;
	params.srcpitch		= bitmap->rowbytes;
	params.srcdepth		= bitmap->depth;
	params.srclookup	= win_prepare_palette(&params);
	params.srcxoffs		= win_visible_rect.left;
	params.srcyoffs		= win_visible_rect.top;
	params.srcwidth		= win_visible_width;
	params.srcheight	= win_visible_height;

	params.vecdirty		= vector_dirty_pixels;

	params.flipx		= blit_flipx;
	params.flipy		= blit_flipy;
	params.swapxy		= blit_swapxy;

	// adjust for more optimal bounds
	if (bounds && !update && !vector_dirty_pixels)
	{
		params.dstxoffs += (bounds->min_x - win_visible_rect.left) * effect_min_xscale;
		params.dstyoffs += (bounds->min_y - win_visible_rect.top) * effect_min_yscale;
		params.srcxoffs += bounds->min_x - win_visible_rect.left;
		params.srcyoffs += bounds->min_y - win_visible_rect.top;
		params.srcwidth = bounds->max_x - bounds->min_x + 1;
		params.srcheight = bounds->max_y - bounds->min_y + 1;
	}

	win_perform_blit(&params, 0);

	// unlock the surface
	IDirectDrawSurface_Unlock(blit_surface, NULL);

	// make the src rect
	src.left = dstxoffs - 1;
	src.top = 0;
	src.right = (dstxoffs + win_visible_width * effect_min_xscale) + 1;
	src.bottom = (win_visible_height * effect_min_yscale) + 2;

	// window mode
	if (win_window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(win_video_window, &dst);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[0]);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[1]);

		// target surface is the primary
		target_surface = primary_surface;
	}

	// full screen mode
	else
	{
		// win_start_maximized the rect, constraining to the aspect ratio
		dst.left = dst.top = 0;
		dst.right = primary_desc.dwWidth;
		dst.bottom = primary_desc.dwHeight;
		win_constrain_to_aspect_ratio(&dst, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DISPLAY);

		// center
		dst.left += (primary_desc.dwWidth - (dst.right - dst.left)) / 2;
		dst.top += (primary_desc.dwHeight - (dst.bottom - dst.top)) / 2;
		dst.right += dst.left;
		dst.bottom += dst.top;

		// target surface is the back buffer
		target_surface = back_surface ? back_surface : primary_surface;
		win_ddraw_fullscreen_margins(primary_desc.dwWidth, primary_desc.dwHeight, &margins);
		if (dst.left < margins.left)
			dst.left = margins.left;
		if (dst.top < margins.top)
			dst.top = margins.top;
		if (dst.right > margins.right)
			dst.right = margins.right;
		if (dst.bottom > margins.bottom)
			dst.bottom = margins.bottom;
	}

	// blit and flip
	if (!blit_and_flip(target_surface, &src, &dst, update))
		return 0;

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Recreating surfaces\n");

	// go ahead and adjust the window
	win_adjust_window();

	// release and recreate the surfaces
	release_surfaces();
	if (!create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	blit_and_flip
//============================================================

static int blit_and_flip(LPDIRECTDRAWSURFACE target_surface, LPRECT src, LPRECT dst, int update)
{
	HRESULT result;

	// sync to VBLANK?
	if ((win_wait_vsync || win_sync_refresh) && throttle && mame_get_performance_info()->game_speed_percent > 95)
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
		if (verbose)
			fprintf(stderr, "Unable to blt blit_surface: %08x\n", (UINT32)result);
		return 0;
	}

	// erase the edges if updating
	if (update)
	{
		RECT outer;
		win_ddraw_fullscreen_margins(primary_desc.dwWidth, primary_desc.dwHeight, &outer);
		erase_outer_rect(&outer, dst, target_surface);
	}

	// full screen mode: flip
	if (!win_window_mode && back_surface && result != DDERR_WASSTILLDRAWING)
	{
#if SHOW_FLIP_TIMES
		static cycles_t total;
		static int count;
		cycles_t start = osd_cycles(), stop;
#endif

		IDirectDrawSurface_Flip(primary_surface, NULL, DDFLIP_WAIT);

#if SHOW_FLIP_TIMES
		stop = osd_cycles();
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
	win_adjust_window();

	// release and recreate the surfaces
	release_surfaces();
	if (!create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	render_to_primary
//============================================================

static int render_to_primary(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int wait_for_lock = lock_must_succeed(bounds, vector_dirty_pixels);
	DDSURFACEDESC temp_desc = { sizeof(temp_desc) };
	LPDIRECTDRAWSURFACE target_surface;
	struct win_blit_params params;
	int xmult, ymult, dstdepth;
	HRESULT result;
	RECT outer, inner, temp;

tryagain:
	// window mode
	if (win_window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(win_video_window, &outer);
		ClientToScreen(win_video_window, &((LPPOINT)&outer)[0]);
		ClientToScreen(win_video_window, &((LPPOINT)&outer)[1]);
		inner = outer;

		// target surface is the primary
		target_surface = primary_surface;
	}

	// full screen mode
	else
	{
		// win_start_maximized the rect, constraining to the aspect ratio
		win_ddraw_fullscreen_margins(primary_desc.dwWidth, primary_desc.dwHeight, &outer);
		inner = outer;
		win_constrain_to_aspect_ratio(&inner, WMSZ_BOTTOMRIGHT, 0, COORDINATES_DISPLAY);

		// target surface is the back buffer
		target_surface = back_surface ? back_surface : primary_surface;
	}

	// compute the multipliers
	win_compute_multipliers(&inner, &xmult, &ymult);

	// center within the display rect
	inner.left = outer.left + ((outer.right - outer.left) - (win_visible_width * xmult)) / 2;
	inner.top = outer.top + ((outer.bottom - outer.top) - (win_visible_height * ymult)) / 2;
	inner.right = inner.left + win_visible_width * xmult;
	inner.bottom = inner.top + win_visible_height * ymult;

	// make sure we're not clipped
	if (win_window_mode)
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
	result = IDirectDrawSurface_Lock(target_surface, NULL, &temp_desc, wait_for_lock ? DDLOCK_WAIT : 0, NULL);
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
		if (verbose)
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
	params.dstyscale	= (!win_old_scanlines || ymult == 1) ? ymult : ymult - 1;
	params.dstyskip		= (!win_old_scanlines || ymult == 1) ? 0 : 1;
	params.dsteffect	= win_determine_effect(&params);

	params.srcdata		= bitmap->base;
	params.srcpitch		= bitmap->rowbytes;
	params.srcdepth		= bitmap->depth;
	params.srclookup	= win_prepare_palette(&params);
	params.srcxoffs		= win_visible_rect.left;
	params.srcyoffs		= win_visible_rect.top;
	params.srcwidth		= win_visible_width;
	params.srcheight	= win_visible_height;

	params.vecdirty		= vector_dirty_pixels;

	params.flipx		= blit_flipx;
	params.flipy		= blit_flipy;
	params.swapxy		= blit_swapxy;

	// need to disable vector dirtying if we're rendering directly to a back buffer
	if (!win_window_mode && back_surface)
		params.vecdirty = NULL;

	// adjust for more optimal bounds
	if (bounds && !update && !vector_dirty_pixels)
	{
		params.dstxoffs += (bounds->min_x - win_visible_rect.left) * xmult;
		params.dstyoffs += (bounds->min_y - win_visible_rect.top) * ymult;
		params.srcxoffs += bounds->min_x - win_visible_rect.left;
		params.srcyoffs += bounds->min_y - win_visible_rect.top;
		params.srcwidth = bounds->max_x - bounds->min_x + 1;
		params.srcheight = bounds->max_y - bounds->min_y + 1;
	}

	win_perform_blit(&params, update);

	// unlock the surface
	IDirectDrawSurface_Unlock(target_surface, NULL);

	// erase the edges if updating
	if (update)
		erase_outer_rect(&outer, &inner, target_surface);

	// full screen mode: flip
	if (!win_window_mode && back_surface && result != DDERR_WASSTILLDRAWING)
	{
#if SHOW_FLIP_TIMES
		static cycles_t total;
		static int count;
		cycles_t start = osd_cycles(), stop;
#endif

		IDirectDrawSurface_Flip(primary_surface, NULL, DDFLIP_WAIT);

#if SHOW_FLIP_TIMES
		stop = osd_cycles();
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
	win_adjust_window();

	// release and recreate the surfaces
	release_surfaces();
	if (!create_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}
