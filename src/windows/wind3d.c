//============================================================
//
//	wind3d.c - Win32 Direct3D 7 (with DirectDraw 7) code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef VPINMAME
#define NONAMELESSUNION
#endif // VPINMAME

#ifdef __GNUC__
 #define NONAMELESSUNION
#endif
#include <ddraw.h>
#include <d3d.h>

// additional definitions for d3d
#include "d3d_extra.h"

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"
#include "wind3dfx.h"

#include "winddraw.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern int verbose;

// from video.c
GUID *screen_guid_ptr;

// from wind3dfx.c
extern UINT32 win_d3d_tfactor;
extern UINT32 win_d3d_preprocess_tfactor;



//============================================================
//	DEBUGGING
//============================================================

#define SHOW_FLIP_TIMES 		0
#define SHOW_MODE_SCORE 		0
#define SHOW_PRESCALE 			0



//============================================================
//	GLOBAL VARIABLES
//============================================================

// effects surfaces
LPDIRECTDRAWSURFACE7 win_d3d_background_surface;
LPDIRECTDRAWSURFACE7 win_d3d_scanline_surface[2];

// effects settings
UINT8 win_d3d_effects_swapxy;
UINT8 win_d3d_effects_flipx;
UINT8 win_d3d_effects_flipy;

int win_d3d_use_filter;
int win_d3d_tex_manage;

UINT8 win_d3d_use_auto_effect;
UINT8 win_d3d_use_rgbeffect;
UINT8 win_d3d_use_scanlines;
UINT8 win_d3d_use_prescale;
UINT8 win_d3d_use_feedback;

// zoom level
int win_d3d_current_zoom;

// prescale level
int win_d3d_prescalex, win_d3d_prescaley;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectDraw7
typedef HRESULT (WINAPI *type_directdraw_create_ex) (GUID FAR *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter);

static HINSTANCE ddraw_dll7;

// DirectDraw objects
static LPDIRECTDRAW7 ddraw7;
static LPDIRECTDRAWGAMMACONTROL gamma_control;
static LPDIRECTDRAWSURFACE7 primary_surface;
static LPDIRECTDRAWSURFACE7 back_surface;
static LPDIRECTDRAWSURFACE7 blit_surface;
static LPDIRECTDRAWCLIPPER primary_clipper;

// DirectDraw object info
static DDSURFACEDESC2 primary_desc;
static DDSURFACEDESC2 blit_desc;
static DDSURFACEDESC2 texture_desc;
static DDSURFACEDESC2 preprocess_desc;
static int changed_resolutions;
static int forced_updates;

// Direct3D objects
static LPDIRECT3D7 d3d7;
static LPDIRECT3DDEVICE7 d3d_device7;
static LPDIRECTDRAWSURFACE7 texture_surface;
static LPDIRECTDRAWSURFACE7 preprocess_surface;
static D3DTLVERTEX preprocess_vertex[4];
static D3DTLVERTEX2 screen_vertex[4];

// Direct3D info
static D3DDEVICEDESC7 d3d_device_desc;
static DWORD d3dtop_scanlines;
static DWORD d3dtfg_image;
static DWORD d3dtfg_scanlines;
static DWORD d3dtfn_image;
static DWORD d3dtfn_scanlines;

// video bounds
static int max_width;
static int max_height;
static int pref_depth;
static int effect_min_xscale;
static int effect_min_yscale;
static struct rectangle last_bounds;
static double aspect_ratio;

// mode finding
static double best_score;
static int best_width;
static int best_height;
static int best_depth;
static int best_refresh;

//  misc
static int position_changed;
static int current_prescalex, current_prescaley;


// derived attributes
static int video_attributes;
static int needs_6bpp_per_gun;
static int pixel_aspect_ratio;



//============================================================
//	PROTOTYPES
//============================================================

static double compute_mode_score(int width, int height, int depth, int refresh);
static int set_resolution(void);
static int create_surfaces(void);
static int create_blit_surface(void);
static int create_effects_surfaces(void);
static void set_brightness(void);
static int create_clipper(void);
static void erase_surfaces(void);
static int restore_surfaces(void);
static void release_surfaces(void);
static void compute_color_masks(const DDSURFACEDESC2 *desc);
static int render_to_blit(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
static int render_and_flip(LPRECT src, LPRECT dst, int update, int wait_for_lock);
static void init_vertices_preprocess(LPRECT src);
static void init_vertices_screen(LPRECT src, LPRECT dst);



//============================================================
//	erase_outer_rect
//============================================================

INLINE void erase_outer_rect(RECT *outer, RECT *inner, LPDIRECTDRAWSURFACE7 surface)
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
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the right edge
	if (inner->right < outer->right)
	{
		clear = *outer;
		clear.left = inner->right;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the top edge
	if (inner->top > outer->top)
	{
		clear = *outer;
		clear.bottom = inner->top;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}

	// clear the bottom edge
	if (inner->bottom < outer->bottom)
	{
		clear = *outer;
		clear.top = inner->bottom;
		if (surface)
			IDirectDrawSurface7_Blt(surface, &clear, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	win_d3d_test_hardware_caps
//============================================================

static int win_d3d_test_hardware_caps(void)
{
	// scanlines
	if (win_d3d_use_scanlines)
	{
		// ensure the grpahics hardware can handle 2 texture stages
		if (d3d_device_desc.wMaxSimultaneousTextures < 2 || d3d_device_desc.wMaxTextureBlendStages < 2)
		{
			fprintf(stderr, "Error: 3D hardware can't render scanlines (no support for multi-texturing)\n");
			return 1;
		}

		// determine optimal blending method
		d3dtop_scanlines = D3DTOP_ADD;
		if (d3d_device_desc.dwTextureOpCaps & D3DTEXOPCAPS_ADDSMOOTH)
		{
			d3dtop_scanlines = D3DTOP_ADDSMOOTH;
		}
		else if (verbose)
		{
			fprintf(stderr, "Warning: using fall-back method for scanline blending.\n");
		}
	}

	// filtering

	// assume bi-linear filtering as the default
	d3dtfg_image = D3DTFG_LINEAR;
	d3dtfg_scanlines = D3DTFG_LINEAR;
	if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
	{
		d3dtfn_image = D3DTFN_LINEAR;
		d3dtfn_scanlines = D3DTFN_LINEAR;
	}
	else
	{
		d3dtfn_image = D3DTFN_POINT;
		d3dtfn_scanlines = D3DTFN_POINT;
	}

	// override defaults if required
	switch (win_d3d_use_filter)
	{
		// point filtering
		case 0:
		{
			d3dtfg_image = D3DTFG_POINT;
			d3dtfn_image = D3DTFN_POINT;
			break;
		}
		// cubic filtering (flat kernel)
		case 2:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFAFLATCUBIC)
			{
				d3dtfg_image = D3DTFG_FLATCUBIC;
				d3dtfg_scanlines = D3DTFG_FLATCUBIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: flat bi-cubic filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
		// cubic filtering (gaussian kernel)
		case 3:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC)
			{
				d3dtfg_image = D3DTFG_GAUSSIANCUBIC;
				d3dtfg_scanlines = D3DTFG_GAUSSIANCUBIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: gaussian bi-cubic filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
		// anisotropic filtering
		case 4:
		{
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
			{
				d3dtfg_image = D3DTFG_ANISOTROPIC;
				d3dtfg_scanlines = D3DTFG_ANISOTROPIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: anisotropic (mag) filtering not supported, falling back to bi-linear\n");
			}
			if (d3d_device_desc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
			{
				d3dtfn_image = D3DTFN_ANISOTROPIC;
				d3dtfn_scanlines = D3DTFN_ANISOTROPIC;
			}
			else if (verbose)
			{
				fprintf(stderr, "Warning: anisotropic (min) filtering not supported, falling back to bi-linear\n");
			}
			break;
		}
	}

	// RGB effects
	switch (win_d3d_use_rgbeffect)
	{
		// multiply mode
		case 1:
		{
			if (!(d3d_device_desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
			{
				if (verbose)
					fprintf(stderr, "Error: RGB effects (multiply mode) not supported\n");

				return 1;
			}
			break;
		}
		// multiply & add mode
		case 2:
		{
			if (!(d3d_device_desc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) ||
				!(d3d_device_desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR))
			{
				if (verbose)
					fprintf(stderr, "Error: RGB effects (multiply & add mode) not supported\n");

				return 1;
			}
			break;
		}
	}

	return 0;
}



//============================================================
//	adjust prescale levels
//============================================================

static void adjust_prescale(int width, int height)
{
	// if prescale is disbled just set the levels to 1
	if (!win_d3d_use_prescale)
	{
		current_prescalex = current_prescaley = 1;
#if SHOW_PRESCALE
		fprintf(stderr, "prescale disabled\n");
#endif
		return;
	}

	current_prescalex = win_d3d_prescalex;

	// auto
	if (win_d3d_use_prescale == 2)
	{
		if (win_d3d_effects_swapxy)
		{
			if (win_d3d_use_scanlines)
				current_prescalex = (int)((double)height / max_height * 0.50);
			else
				current_prescalex = (int)((double)height / max_height * 0.67);
		}
		else
		{
			if (win_d3d_use_scanlines)
				current_prescalex = (int)((double)width / max_width * 0.50);
			else
				current_prescalex = (int)((double)width / max_width * 0.67);
		}
	}
	// full
	else if (win_d3d_use_prescale == 3)
	{
		if (win_d3d_effects_swapxy)
			current_prescalex = (int)((double)height / max_height);
		else
			current_prescalex = (int)((double)width / max_width);
	}

	// adjust if software scaling is used
	current_prescalex /= effect_min_xscale;

	if (current_prescalex > MAX_PRESCALE)
		current_prescalex = MAX_PRESCALE;
	if (current_prescalex < 1)
		current_prescalex = 1;

	current_prescaley = win_d3d_prescaley;

	// auto
	if (win_d3d_use_prescale == 2)
	{
		if (win_d3d_effects_swapxy)
		{
			if (win_d3d_use_scanlines)
				current_prescaley = (int)((double)width / max_width * 0.75);
			else
				current_prescaley = (int)((double)width / max_width * 0.67);
		}
		else
		{
			if (win_d3d_use_scanlines)
				current_prescaley = (int)((double)height / max_height * 0.75);
			else
				current_prescaley = (int)((double)height / max_height * 0.67);
		}
	}
	// full
	else if (win_d3d_use_prescale == 3)
	{
		if (win_d3d_effects_swapxy)
			current_prescaley = (int)((double)width / max_width);
		else
			current_prescaley = (int)((double)height / max_height);
	}

	// adjust if software scaling is used
	current_prescaley /= effect_min_yscale;

	if (current_prescaley > MAX_PRESCALE)
		current_prescaley = MAX_PRESCALE;
	if (current_prescaley < 1)
		current_prescaley = 1;

	// ensure the levels aren't too large
	if (preprocess_surface)
	{
		if (blit_swapxy)
		{
			while (current_prescalex >= 1 && preprocess_desc.dwWidth < (current_prescalex * win_visible_height))
				current_prescalex--;
			while (current_prescaley >= 1 && preprocess_desc.dwHeight < (current_prescaley * win_visible_width))
				current_prescaley--;
		}
		else
		{
			while (current_prescalex >= 1 && preprocess_desc.dwWidth < (current_prescalex * win_visible_width))
				current_prescalex--;
			while (current_prescaley >= 1 && preprocess_desc.dwHeight < (current_prescaley * win_visible_height))
				current_prescaley--;
		}
	}

	// if the prescale levels are 1, just disable prescale
	if (current_prescalex == 1 && current_prescaley == 1)
		win_d3d_use_prescale = 0;

#if SHOW_PRESCALE
	if (win_d3d_use_prescale)
		fprintf(stderr, "prescale set to %ix%i (image size %ix%i, window size %ix%i)\n", current_prescalex, current_prescaley, max_width, max_height, width, height);
	else
		fprintf(stderr, "prescale disabled\n");
#endif
}



//============================================================
//	win_d3d_wait_vsync
//============================================================

void win_d3d_wait_vsync(void)
{
	HRESULT result;
	BOOL is_vblank;

	// if we're not already in VBLANK, wait for it
	result = IDirectDraw7_GetVerticalBlankStatus(ddraw7, &is_vblank);
	if (result == DD_OK && !is_vblank)
		result = IDirectDraw7_WaitForVerticalBlank(ddraw7, DDWAITVB_BLOCKBEGIN, 0);
}



//============================================================
//	win_d3d_kill
//============================================================

void win_d3d_kill(void)
{
	// delete the d3d device
	if (d3d_device7 != NULL)
		IDirect3DDevice7_Release(d3d_device7);
	d3d_device7 = NULL;

	// release the surfaces
	release_surfaces();

	// restore resolutions & reset cooperative level
	if (ddraw7 != NULL)
		IDirectDraw7_SetCooperativeLevel(ddraw7, NULL, DDSCL_NORMAL);

	// delete the d3d object
	if (d3d7 != NULL)
		IDirect3D7_Release(d3d7);
	d3d7 = NULL;

	// delete the ddraw objects
	if (ddraw7 != NULL)
		IDirectDraw7_Release(ddraw7);
	ddraw7 = NULL;

	if (ddraw_dll7)
		FreeLibrary(ddraw_dll7);
	ddraw_dll7 = NULL;
}



//============================================================
//	win_d3d_init
//============================================================

int win_d3d_init(int width, int height, int depth, int attributes, double aspect, const struct win_effect_data *effect)
{
	type_directdraw_create_ex fn_directdraw_create_ex;

	DDSURFACEDESC2 currmode = { sizeof(DDSURFACEDESC2) };
	D3DVIEWPORT7 viewport;
	HRESULT result;

	// since we can't count on DirectX7 being present, load the dll
	ddraw_dll7 = LoadLibrary("ddraw.dll");
	if (ddraw_dll7 == NULL)
	{
		fprintf(stderr, "Error importing ddraw.dll\n");
		goto error_handling;
	}
	// and import the DirectDrawCreateEx function manually
	fn_directdraw_create_ex = (type_directdraw_create_ex)GetProcAddress(ddraw_dll7, "DirectDrawCreateEx");
	if (fn_directdraw_create_ex == NULL)
	{
		fprintf(stderr, "Error importing DirectDrawCreateEx() function\n");
		goto error_handling;
	}

	result = fn_directdraw_create_ex(screen_guid_ptr, (void **)&ddraw7, &IID_IDirectDraw7, NULL);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating DirectDraw7: %08x\n", (UINT32)result);
		goto error_handling;
	}

	result = IDirectDraw7_QueryInterface(ddraw7, &IID_IDirect3D7, (void **)&d3d7);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating Direct3D7: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// print initialisation message and available video memory
	if (verbose)
	{
		DDSCAPS2 caps = { 0 };
		DWORD mem_total;
		DWORD mem_free;

		caps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		fprintf(stderr, "Initialising DirectDraw & Direct3D 7 blitter");
		if (IDirectDraw7_GetAvailableVidMem(ddraw7, &caps, &mem_total, &mem_free) == DD_OK)
		{
			fprintf(stderr, " (%.2lfMB video memory available)", (double)mem_total / (1024 * 1024));
		}
		fprintf(stderr, "\n");
	}

	// set the graphics mode width/height to the window size
	if (width && height && depth)
	{
		max_width = width;
		max_height = height;
		pref_depth = depth;

		video_attributes = attributes;
		needs_6bpp_per_gun	= ((attributes & VIDEO_NEEDS_6BITS_PER_GUN) != 0);
		pixel_aspect_ratio	= (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK);
	}
	if (aspect)
	{
		aspect_ratio = aspect;
	}
	if (effect)
	{
		effect_min_xscale = effect->min_xscale;
		effect_min_yscale = effect->min_yscale;
	}

	// set the cooperative level
	// for non-window modes, we will use full screen here
	result = IDirectDraw7_SetCooperativeLevel(ddraw7, win_video_window, (win_window_mode ? DDSCL_NORMAL : DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) | DDSCL_FPUPRESERVE);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error setting cooperative level: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// init the effects
	win_d3d_effects_init(video_attributes);

	// set contraints on window size
	if (win_d3d_use_scanlines && win_force_int_stretch == FORCE_INT_STRECT_AUTO)
		win_default_constraints = win_d3d_effects_swapxy ? CONSTRAIN_INTEGER_WIDTH : CONSTRAIN_INTEGER_HEIGHT;

	// full screen mode: set the resolution
	changed_resolutions = 0;
	if (set_resolution())
		goto error_handling;

	// create the screen surfaces
	if (create_surfaces())
		goto error_handling;

	result = IDirect3D7_CreateDevice(d3d7, &IID_IDirect3DHALDevice, back_surface, &d3d_device7);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error creating Direct3D7 device: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// get device caps
	result = IDirect3DDevice7_GetCaps(d3d_device7, &d3d_device_desc);

	if (win_d3d_test_hardware_caps())
		goto error_handling;

	// create the blit surfaces
	if (create_blit_surface())
		goto error_handling;

	// create the effects surfaces
	if (create_effects_surfaces())
		goto error_handling;

	// erase all the surfaces we created
	erase_surfaces();

	// init the surfaces used for the effects
	if (win_d3d_effects_init_surfaces())
		goto error_handling;

	// attempt to get the current display mode
	result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
	if (result != DD_OK)
		goto error_handling;

	// now set up the viewport
	viewport.dwX = 0;
	viewport.dwY = 0;
	viewport.dwWidth = currmode.dwWidth;
	viewport.dwHeight = currmode.dwHeight;

	result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);
	if (result != DD_OK)
		goto error_handling;

	// set some Direct3D options
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_LIGHTING, FALSE);
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);

	// print available video memory
	if (verbose)
	{
		DDSCAPS2 caps = { 0 };
		DWORD mem_total;
		DWORD mem_free;

		caps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		if (IDirectDraw7_GetAvailableVidMem(ddraw7, &caps, &mem_total, &mem_free) == DD_OK)
		{
			fprintf(stderr, "Blitter initialisation complete (%.2lfMB video memory free)\n", (double)mem_free / (1024 * 1024));
		}
	}

	// force update of position and RGB effects
	position_changed = 1;

	// force some updates
	forced_updates = 5;

	return 0;

	// error handling
error_handling:
	win_d3d_kill();

	return 1;
}



//============================================================
//	enum_callback
//============================================================

static HRESULT WINAPI enum_callback(LPDDSURFACEDESC2 desc, LPVOID context)
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
			// needs < 24bpp mode		  // needs >= 24bpp mode
		{ { 0.00, 0.75, 0.25, 0.50 },	{ 0.00, 0.25, 0.50, 0.75 } },	// 8bpp source
		{ { 0.00, 1.00, 0.25, 0.50 },	{ 0.00, 0.50, 0.75, 1.00 } },	// 16bpp source
		{ { 0.00, 0.00, 0.00, 0.00 },	{ 0.00, 0.00, 0.00, 0.00 } },	// 24bpp source (doesn't exist)
		{ { 0.00, 0.50, 0.75, 1.00 },	{ 0.00, 0.50, 0.75, 1.00 } }	// 32bpp source
	};

	double size_score, aspect_score, depth_score, refresh_score, final_score;
	int target_width, target_height;

	// select wich depth matrix to use based on the game properties and enabled effects
	int matrix = (needs_6bpp_per_gun || win_d3d_use_feedback) ? 1 : 0;

	// first compute a score based on size

	// determine minimum requirements
	target_width = max_width * effect_min_xscale;
	target_height = max_height * effect_min_yscale;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_1_2)
		target_height *= 2;
	else if (win_old_scanlines)
		target_width *= 2, target_height *= 2;
	if (pixel_aspect_ratio == VIDEO_PIXEL_ASPECT_RATIO_2_1)
		target_width *= 2;

	// determine the zoom level based on the number of scanlines of the game
	if (win_keep_aspect && win_force_int_stretch != FORCE_INT_STRECT_FULL)
	{
		if (blit_swapxy && win_default_constraints != CONSTRAIN_INTEGER_HEIGHT)
		{
			// adjust width for the desired zoom level
			if (target_width < max_width * (win_gfx_zoom / effect_min_xscale))
				target_width = max_width * (win_gfx_zoom / effect_min_yscale);

			// match height according to the aspect ratios of the game, screen, and display mode
			target_height = (int)((double)target_width * height / aspect_ratio / width * win_screen_aspect + 0.5);
		}
		else
		{
			// adjust height for the desired zoom level
			if (target_height < max_height * (win_gfx_zoom / effect_min_xscale))
				target_height = max_height * (win_gfx_zoom / effect_min_yscale);

			// match width according to the aspect ratios of the game, screen, and display mode
			target_width = (int)((double)target_height * width * aspect_ratio / height / win_screen_aspect + 0.5);
		}
	}
	// ignore aspect ratio entirely
	else
	{
		// adjust width for the desired zoom level
		if (target_width < max_width * (win_gfx_zoom / effect_min_xscale))
			target_width = max_width * (win_gfx_zoom / effect_min_yscale);
		// adjust height for the desired zoom level
		if (target_height < max_height * (win_gfx_zoom / effect_min_xscale))
			target_height = max_height * (win_gfx_zoom / effect_min_yscale);
	}

	// compute initial score based on difference between target and current (adjusted for zoom level)
	size_score = 1.0 / (1.0 + (fabs(width - target_width) / win_screen_aspect + fabs(height - target_height)) / 16 / win_gfx_zoom);

	// if we're looking for a particular mode, make sure it matches
	if (win_gfx_width && win_gfx_height && (width != win_gfx_width || height != win_gfx_height))
		return 0.0;

	// if mode is too small, it's a zero, unless the user specified otherwise
	if ((width < max_width || height < max_height) && (!win_gfx_width || !win_gfx_height))
		return 0.0;

	// if mode is smaller than we'd like, it only scores up to 0.1
	if (width < target_width || height < target_height)
		size_score *= 0.1;

	// now compute the aspect ratio score
	if (win_d3d_use_rgbeffect)
		// strongly prefer square pixels
		aspect_score = 1.0 / (1.0 + fabs((double)width / height - win_screen_aspect) * 10.0);
	else
		// mildly prefer square pixels
		aspect_score = 1.0 / (1.0 + fabs((double)width / height - win_screen_aspect));

	// next compute depth score
	depth_score = depth_matrix[(pref_depth + 7) / 8 - 1][matrix][(depth + 7) / 8 - 1];

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

	// weight size/aspect highest, followed by depth and refresh
	final_score = (size_score * aspect_score * 100.0 + depth_score * 10.0 + refresh_score) / 111.0;

#if SHOW_MODE_SCORE
	fprintf(stderr, "%4ix%4i: size %1.6f aspect %1.4f depth %1.1f refresh %1.4f total %1.6f\n", width, height, size_score, aspect_score, depth_score, refresh_score, final_score);
#endif

	return final_score;
}



//============================================================
//	set_resolution
//============================================================

static int set_resolution(void)
{
	DDSURFACEDESC2 currmode = { sizeof(DDSURFACEDESC2) };
	double resolution_aspect;
	HRESULT result;

	// skip if not switching resolution
	if (!win_window_mode && (win_switch_res || win_switch_bpp))
	{
		// if we're only switching depth, set win_gfx_width and win_gfx_height to the current resolution
		if (!win_switch_res || !win_switch_bpp)
		{
			// attempt to get the current display mode
			result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
				goto error_handling;
			}

			// force to the current width/height
			if (!win_switch_res)
			{
				win_gfx_width = currmode.dwWidth;
				win_gfx_height = currmode.dwHeight;
			}
			if (!win_switch_bpp)
				win_gfx_depth = currmode.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
		}

		// enumerate display modes
		best_score = 0.0;
		result = IDirectDraw7_EnumDisplayModes(ddraw7, (win_match_refresh || win_gfx_refresh) ? DDEDM_REFRESHRATES : 0, NULL, NULL, enum_callback);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error enumerating modes: %08x\n", (UINT32)result);
			goto error_handling;
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
			// Set the refresh rate
			result = IDirectDraw7_SetDisplayMode(ddraw7, best_width, best_height, best_depth, best_refresh, 0);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error setting mode: %08x\n", (UINT32)result);
				goto error_handling;
			}
			changed_resolutions = 1;
		}
	}

	// attempt to get the current display mode
	result = IDirectDraw7_GetDisplayMode(ddraw7, &currmode);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting display mode: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// compute the adjusted aspect ratio
	resolution_aspect = (double)currmode.dwWidth / (double)currmode.dwHeight;
	win_aspect_ratio_adjust = resolution_aspect / win_screen_aspect;
	return 0;

	// error handling - non fatal in general
error_handling:

	return 0;
}



//============================================================
//	print_surface_size & print_surface_colour
//============================================================

static int print_surface_size(LPDIRECTDRAWSURFACE7 surface, char *string)
{
	DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

	if (IDirectDrawSurface7_GetSurfaceDesc(surface, &desc) != DD_OK)
		return 1;

	sprintf(string, "%dx%dx%d",
				(int)desc.dwWidth,
				(int)desc.dwHeight,
				(int)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount);

	return 0;
}

static int print_surface_colour(LPDIRECTDRAWSURFACE7 surface, char *string)
{
	DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

	if (IDirectDrawSurface7_GetSurfaceDesc(surface, &desc) != DD_OK)
		return 1;

	sprintf(string, "R=%08x G=%08x B=%08x",
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask,
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask,
				(UINT32)desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask);


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
	if (!win_window_mode)
	{
		int buffer_count = 1;
		if (win_triple_buffer)
			buffer_count = 2;

		primary_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		primary_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
#ifdef DDSD_DEPTH
		// this is correct for current DX8/9 includes
		primary_desc.DUMMYUNIONNAMEN(5).dwBackBufferCount = buffer_count;
#else
		// this is correct for DX7 includes
		primary_desc.dwBackBufferCount = buffer_count;
#endif
	}

	// then create the primary surface
	result = IDirectDraw7_CreateSurface(ddraw7, &primary_desc, &primary_surface, NULL);
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Error creating primary surface: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// get a description of the primary surface
	result = IDirectDrawSurface7_GetSurfaceDesc(primary_surface, &primary_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting primary surface desc: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// print out the good stuff
	if (verbose)
	{
		char size[80];
		char colour[80];

		if (win_window_mode)
			fprintf(stderr, "Rendering to an off-screen surface and using blt()\n");
		else
			fprintf(stderr, "Using a %s buffer and pageflipping\n", win_triple_buffer ? "triple" : "double");

		print_surface_size(primary_surface, size);
		print_surface_colour(primary_surface, colour);

		fprintf(stderr, "Primary surface created: %s (%s)\n", size, colour);
	}

	// determine the color masks and force the palette to recalc
	compute_color_masks(&primary_desc);

	// if this is a full-screen mode, attempt to create a color control object
	if (!win_window_mode && win_gfx_brightness != 0.0)
		set_brightness();

	// window mode: allocate the back surface seperately
	if (win_window_mode)
	{
        DDSURFACEDESC2 back_desc = { sizeof(DDSURFACEDESC2) };

		back_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		back_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;

		back_desc.dwWidth = primary_desc.dwWidth;
		back_desc.dwHeight = primary_desc.dwHeight;

		// then create the primary surface
		result = IDirectDraw7_CreateSurface(ddraw7, &back_desc, &back_surface, NULL);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error creating back surface: %08x\n", (UINT32)result);
			goto error_handling;
		}
	}
	// full screen mode: get the back surface
	else
	{
		DDSCAPS2 caps = { DDSCAPS_BACKBUFFER };
		result = IDirectDrawSurface7_GetAttachedSurface(primary_surface, &caps, &back_surface);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error getting attached back surface: %08x\n", (UINT32)result);
			goto error_handling;
		}
	}

	// create a clipper for windowed mode
	if (win_window_mode)
	{
		if (create_clipper())
			goto error_handling;
	}

	return 0;

	// error handling
error_handling:

	return 1;
}



//============================================================
//	create_effects_surfaces
//============================================================

static int create_effects_surfaces(void)
{
	HRESULT result = DD_OK;

	// create a surface that will hold the RGB effects pattern, based on the primary surface
	if (win_d3d_use_rgbeffect)
	{
        DDSURFACEDESC2 desc = { sizeof(DDSURFACEDESC2) };

		desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

		// we can save video memory because we only need to partially cover the screen
		if (!win_window_mode && win_keep_aspect /*&& win_force_int_stretch != FORCE_INT_STRECT_FULL*/)
		{
			desc.dwWidth = primary_desc.dwWidth;
			desc.dwHeight = (int)((double)primary_desc.dwHeight / aspect_ratio * win_screen_aspect + 0.5);
			if (desc.dwHeight > primary_desc.dwHeight)
			{
				desc.dwHeight = primary_desc.dwHeight;
				desc.dwWidth = (int)((double)primary_desc.dwWidth * aspect_ratio / win_screen_aspect + 0.5);
			}
		}
		else
		{
			desc.dwWidth = primary_desc.dwWidth;
			desc.dwHeight = primary_desc.dwHeight;
		}

		// create the RGB effects surface
		result = IDirectDraw7_CreateSurface(ddraw7, &desc, &win_d3d_background_surface, NULL);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error creating RGB effects surface: %08x\n", (UINT32)result);
			goto error_handling;
		}

		if (verbose)
		{
			char size[80];

			print_surface_size(win_d3d_background_surface, size);
			fprintf(stderr, "RGB effects surface created: %s\n", size);
		}
	}

	// create 2 surfaces that will hold different sizes of scanlines, based on the primary surface
	if (win_d3d_use_scanlines)
	{
		int i;

		for (i = 0; i < 2; i++)
		{
	        DDSURFACEDESC2 desc =  { sizeof(DDSURFACEDESC2) };

			desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
			desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;

			desc.dwWidth = 2 << i;
			desc.dwHeight = 2 << i;

			if (desc.dwWidth < d3d_device_desc.dwMinTextureWidth || desc.dwHeight < d3d_device_desc.dwMinTextureHeight)
			{
				fprintf(stderr, "Error creating scanline surface[%d]: %08x\n", i, (UINT32)result);
				goto error_handling;
			}

			// create the surface
			result = IDirectDraw7_CreateSurface(ddraw7, &desc, &win_d3d_scanline_surface[i], NULL);
			if (result != DD_OK)
			{
				fprintf(stderr, "Error creating scanline surface[%d]: %08x\n", i, (UINT32)result);
				goto error_handling;
			}

			if (verbose)
			{
				char size[80];

				print_surface_size(win_d3d_scanline_surface[i], size);
				fprintf(stderr, "Scanline surface[%i] created: %s\n", i, size);
			}
		}
	}

	return 0;

error_handling:

	return 1;
}



//============================================================
//	callback for IDirect3DDevice7_EnumTextureFormats
//============================================================

static HRESULT CALLBACK enum_textures_callback(LPDDPIXELFORMAT pixelformat, LPVOID preferred_pixelformat)
{
    if (pixelformat->dwFlags == DDPF_RGB && pixelformat->DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
    {
		// use RGB:555 format if supported by the 3D hardware and scaling effect
		if (pixelformat->DUMMYUNIONNAMEN(2).dwRBitMask == 0x7C00 &&
			pixelformat->DUMMYUNIONNAMEN(3).dwGBitMask == 0x03E0 &&
			pixelformat->DUMMYUNIONNAMEN(4).dwBBitMask == 0x001F)
		{
			memcpy(preferred_pixelformat, pixelformat, sizeof(DDPIXELFORMAT));

			return D3DENUMRET_CANCEL;
		}
	}

	// use RGB:565 format otherwise
	if (pixelformat->DUMMYUNIONNAMEN(2).dwRBitMask == 0xF800 &&
		pixelformat->DUMMYUNIONNAMEN(3).dwGBitMask == 0x07E0 &&
		pixelformat->DUMMYUNIONNAMEN(4).dwBBitMask == 0x001F)
	{
			memcpy(preferred_pixelformat, pixelformat, sizeof(DDPIXELFORMAT));
    }

    return D3DENUMRET_OK;
}



//============================================================
//	create_blit_surface
//============================================================

static int create_blit_surface(void)
{
	DDPIXELFORMAT preferred_pixelformat = { 0 };
	int width, height;
	int texture_width = 256, texture_height = 256;
	HRESULT result;
	int done = 0;

	// determine the width/height of the blit surface
	while (!done)
	{
		done = 1;

		// first compute the ideal size
		width = (blit_swapxy ? max_height : max_width) * effect_min_xscale + 16;
		height = (blit_swapxy ? max_width : max_height) * effect_min_yscale + 0;

		// reduce the width if needed
		if (width > blit_swapxy ? primary_desc.dwHeight : primary_desc.dwWidth)
		{
			if (effect_min_xscale > 1)
			{
				done = 0;
				effect_min_xscale--;
			}
		}

		// reduce the height if needed
		if (height > blit_swapxy ? primary_desc.dwWidth : primary_desc.dwHeight)
		{
			if (effect_min_yscale > 1)
			{
				done = 0;
				effect_min_yscale--;
			}
		}
	}

	// determine the width/height of the texture surface (assume ^2 sizes are required)
	while (texture_width < (width - 16))
		texture_width <<= 1;
	while (texture_height < (height - 0))
		texture_height <<= 1;

	if (win_d3d_tex_manage && texture_width < width)
		texture_width <<= 1;

	if (texture_width < d3d_device_desc.dwMinTextureWidth)
		texture_width = d3d_device_desc.dwMinTextureWidth;
	if (texture_height < d3d_device_desc.dwMinTextureHeight)
		texture_height = d3d_device_desc.dwMinTextureHeight;

	if (d3d_device_desc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		if (texture_width > texture_height)
		{
			texture_height = texture_width;
		}
		else if (texture_height > texture_width)
		{
			texture_width = texture_height;
		}
	}

	if (texture_width > d3d_device_desc.dwMaxTextureWidth || texture_height > d3d_device_desc.dwMaxTextureHeight)
	{
		if (verbose)
			fprintf(stderr, "Error: required texture size too large (max: %ix%i, need %ix%i)\n", (int)d3d_device_desc.dwMaxTextureWidth, (int)d3d_device_desc.dwMaxTextureHeight, texture_width, texture_height);
		goto error_handling;
	}

	// make a description of our blit surface, based on the primary surface
	blit_desc = primary_desc;
	blit_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;

	// determine the preferred blit/texture surface colour format
	if (!needs_6bpp_per_gun)
		IDirect3DDevice7_EnumTextureFormats(d3d_device7, &enum_textures_callback, (LPVOID)&preferred_pixelformat);
	// if we have a preferred colour format, use it
	if (preferred_pixelformat.dwSize)
		blit_desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat = preferred_pixelformat;

	if (win_d3d_tex_manage)
	{
		// the blit surface will also be the texture surface
		blit_desc.dwWidth = texture_width;
		blit_desc.dwHeight = texture_height;
		blit_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
		blit_desc.ddsCaps.dwCaps2 = DDSCAPS2_HINTDYNAMIC | DDSCAPS2_TEXTUREMANAGE;
	}
	else
	{
		// use separate blit and texture surfaces
		blit_desc.dwWidth = width;
		blit_desc.dwHeight = height;
		blit_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
		blit_desc.ddsCaps.dwCaps2 = 0;
	}

	// then create the blit surface
	result = IDirectDraw7_CreateSurface(ddraw7, &blit_desc, &blit_surface, NULL);
	if (result != DD_OK)
	{
		if (verbose)
			fprintf(stderr, "Error creating blit surface: %08x\n", (UINT32)result);
		goto error_handling;
	}

	texture_desc = blit_desc;
	texture_surface = blit_surface;

	// modify the description of our texture surface, based on the blit surface
	if (!win_d3d_tex_manage)
	{
		// fill in the differences
		texture_desc.dwWidth = texture_width;
		texture_desc.dwHeight = texture_height;
		texture_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
		blit_desc.ddsCaps.dwCaps2 = 0;

		// then create the texture surface
		result = IDirectDraw7_CreateSurface(ddraw7, &texture_desc, &texture_surface, NULL);
		if (result != DD_OK)
		{
			if (verbose)
				fprintf(stderr, "Error creating texture surface: %08x\n", (UINT32)result);
			goto error_handling;
		}
	}

	// now adjust prescale settings to that so we get the maximum
#if SHOW_PRESCALE
	if (win_d3d_use_prescale)
		fprintf(stderr, "max ");
#endif
	adjust_prescale(primary_desc.dwWidth, primary_desc.dwHeight);

	// now create texture used for prescale and feedback (always the same format as the primary surface)
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
		preprocess_desc = win_d3d_use_feedback ? primary_desc : texture_desc;
		preprocess_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
		preprocess_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
		preprocess_desc.ddsCaps.dwCaps2 = 0;

		preprocess_desc.dwWidth = texture_width;
		preprocess_desc.dwHeight = texture_height;

		while (preprocess_desc.dwWidth < (width - 16) * current_prescalex)
			preprocess_desc.dwWidth <<= 1;
		while (preprocess_desc.dwHeight < (height - 0) * current_prescaley)
			preprocess_desc.dwHeight <<= 1;

		if (preprocess_desc.dwWidth > d3d_device_desc.dwMaxTextureWidth || preprocess_desc.dwHeight > d3d_device_desc.dwMaxTextureHeight)
		{
			if (verbose)
				fprintf(stderr, "Warning: required texture size too large for prescale, disabling prescale\n");

			if (preprocess_desc.dwWidth > d3d_device_desc.dwMaxTextureWidth)
			{
				preprocess_desc.dwWidth = texture_width;
				win_d3d_prescalex = 1;
			}
			if (preprocess_desc.dwHeight > d3d_device_desc.dwMaxTextureHeight)
			{
				preprocess_desc.dwHeight = texture_height;
				win_d3d_prescaley = 1;
			}
			if (!win_d3d_prescalex && !win_d3d_prescaley)
				win_d3d_use_prescale = 0;
		}
	}
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
#if 0
		int can_render_to_texture = 0;

		// create the pre-processing surface
		result = IDirectDraw7_CreateSurface(ddraw7, &preprocess_desc, &preprocess_surface, NULL);
		if (result == DD_OK)
		{
			LPDIRECTDRAWSURFACE7 previous_target;
			if (IDirect3DDevice7_GetRenderTarget(d3d_device7, &previous_target) == DD_OK)
			{
				if (IDirect3DDevice7_SetRenderTarget(d3d_device7, preprocess_surface, 0) == DD_OK)
				{
					can_render_to_texture = 1;
				}
				else
				{
					if (verbose)
						fprintf(stderr, "Error: feedback or prescale effects not supported.\n");
				}
				IDirect3DDevice7_SetRenderTarget(d3d_device7, previous_target, 0);
				IDirectDrawSurface7_Release(previous_target);
			}
		}
		if (!can_render_to_texture)
#else
		// create the pre-processing surface
		result = IDirectDraw7_CreateSurface(ddraw7, &preprocess_desc, &preprocess_surface, NULL);
		if (result != DD_OK)
#endif
		{
			if (verbose)
				fprintf(stderr, "Error creating preprocess surface: %08x\n", (UINT32)result);
			goto error_handling;
		}
	}

	// get a description of the blit surface
	if (blit_surface != texture_surface)
	{
		result = IDirectDrawSurface7_GetSurfaceDesc(blit_surface, &blit_desc);
		if (result != DD_OK)
		{
			fprintf(stderr, "Error getting blit surface desc: %08x\n", (UINT32)result);
			goto error_handling;
		}
	}

	// get a description of the texture surface
	result = IDirectDrawSurface7_GetSurfaceDesc(texture_surface, &texture_desc);
	if (result != DD_OK)
	{
		fprintf(stderr, "Error getting blit surface desc: %08x\n", (UINT32)result);
		goto error_handling;
	}

	// blit surface color masks override the primary surface color masks
	compute_color_masks(&blit_desc);

	// print out the good stuff
	if (verbose)
	{
		char size[80];
		char colour[80];

		print_surface_size(texture_surface, size);
		print_surface_colour(texture_surface, colour);
		fprintf(stderr, "%s surface created: %s (%s)\n", (texture_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE) ? "Managed texture" : "Texture", size, colour);
		if (blit_surface != texture_surface)
		{
			print_surface_size(blit_surface, size);
			fprintf(stderr, "Blit surface created: %s\n", size);
		}
		if (preprocess_surface)
		{
			print_surface_size(preprocess_surface, size);
			fprintf(stderr, "Pre-process texture surface created: %s\n", size);
		}
	}

	return 0;

	// error handling
error_handling:

	return 1;
}



//============================================================
//	set_brightness
//============================================================

static void set_brightness(void)
{
	HRESULT result;

	// see if we can get a GammaControl object
	result = IDirectDrawSurface7_QueryInterface(primary_surface, &IID_IDirectDrawGammaControl, (void **)&gamma_control);
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
	result = IDirectDraw7_CreateClipper(ddraw7, 0, &primary_clipper, NULL);
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

	blitfx.DUMMYUNIONNAMEN(5).dwFillColor = 0;

	// erase the blit surface
	if (blit_surface && blit_surface != texture_surface)
		result = IDirectDrawSurface7_Blt(blit_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// erase the texture surface
	if (texture_surface)
		result = IDirectDrawSurface7_Blt(texture_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// erase the preprocess surface
	if (preprocess_surface)
		result = IDirectDrawSurface7_Blt(preprocess_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	// erase the RGB effects surface
	if (win_d3d_background_surface)
		result = IDirectDrawSurface7_Blt(win_d3d_background_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);

	win_d3d_effects_init_surfaces();

	if (!win_window_mode)
	{
		// loop through enough to get all the back buffers
		for (i = 0; i < 5; i++)
		{
			// first flip
			result = IDirectDrawSurface7_Flip(primary_surface, NULL, DDFLIP_NOVSYNC | DDFLIP_WAIT);

			// then do a color fill blit
			result = IDirectDrawSurface7_Blt(back_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		}
	}
	else
	{
		// do a color fill blit on both primary and back buffer
		result = IDirectDrawSurface7_Blt(primary_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
		result = IDirectDrawSurface7_Blt(back_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &blitfx);
	}
}



//============================================================
//	restore_surfaces
//============================================================

static int restore_surfaces(void)
{
	HRESULT result;

#define RESTORE_SURFACE(s) do { if (s) { result = IDirectDrawSurface7_Restore(s); if (result != DD_OK) { return 1; } } } while (0)

	// restore the blit surface
	if (blit_surface != texture_surface)
		RESTORE_SURFACE(blit_surface);

	// restore the texture surface
	RESTORE_SURFACE(texture_surface);

	// restore the preprocess surface
	RESTORE_SURFACE(preprocess_surface);

	// restore the scanline surfaces
	RESTORE_SURFACE(win_d3d_scanline_surface[0]);
	RESTORE_SURFACE(win_d3d_scanline_surface[1]);

	// restore the rgb-effects surface
	RESTORE_SURFACE(win_d3d_background_surface);

	// restore the back surface seperately if needed
	if (back_surface && win_window_mode)
		RESTORE_SURFACE(back_surface);
	// restore the primary surface
	RESTORE_SURFACE(primary_surface);

	// all surfaces are successfully restored

	// erase all surfaces
	erase_surfaces();

	return 0;

#undef RESTORE_SURFACE

}

//============================================================
//	release_surfaces
//============================================================

static void release_surfaces(void)
{

#define RELEASE_SURFACE(s) do { if (s) { IDirectDrawSurface7_Release(s); s = NULL; } } while (0)

	// release the blit surface
	if (blit_surface != texture_surface)
		RELEASE_SURFACE(blit_surface);
	else
		blit_surface = NULL;

	// release the texture surface
	RELEASE_SURFACE(texture_surface);

	// release the preprocess surface
	RELEASE_SURFACE(preprocess_surface);

	// release the clipper
	if (primary_clipper)
		IDirectDrawClipper_Release(primary_clipper);
	primary_clipper = NULL;

	// release the color controls
	if (gamma_control)
		IDirectDrawColorControl_Release(gamma_control);
	gamma_control = NULL;

	// release the scanline surfaces
	RELEASE_SURFACE(win_d3d_scanline_surface[0]);
	RELEASE_SURFACE(win_d3d_scanline_surface[1]);

	// release the background surface
	RELEASE_SURFACE(win_d3d_background_surface);

	// release the back surface seperately if needed
	if (win_window_mode)
		RELEASE_SURFACE(back_surface);
	else
		back_surface = NULL;

	// release the primary surface
	RELEASE_SURFACE(primary_surface);

#undef RELEASE_SURFACE

}



//============================================================
//	compute_color_masks
//============================================================

static void compute_color_masks(const DDSURFACEDESC2 *desc)
{
	// 16bpp case
	if (desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 16)
	{
		int temp;

		// red
		win_color16_rdst_shift = win_color16_rsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_rdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_rsrc_shift++;

		// green
		win_color16_gdst_shift = win_color16_gsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_gdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_gsrc_shift++;

		// blue
		win_color16_bdst_shift = win_color16_bsrc_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color16_bdst_shift++;
		while (!(temp & 0x80))
			temp <<= 1, win_color16_bsrc_shift++;
	}

	// 24/32bpp case
	else if (desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 24 ||
			 desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount == 32)
	{
		int temp;

		// red
		win_color32_rdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_rdst_shift++;

		// green
		win_color32_gdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_gdst_shift++;

		// blue
		win_color32_bdst_shift = 0;
		temp = desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask;
		while (!(temp & 1))
			temp >>= 1, win_color32_bdst_shift++;
	}

	// mark the lookups invalid
	palette_lookups_invalid = 1;
}



//============================================================
//	win_d3d_draw
//============================================================

int win_d3d_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update)
{
	int result;

	// handle forced updates
	if (forced_updates)
	{
		forced_updates--;
		update = 1;
	}

	// if the surfaces are lost, restore them
	if (IDirectDrawSurface7_IsLost(primary_surface) == DDERR_SURFACELOST)
		restore_surfaces();

	// render to the blit surface,
	result = render_to_blit(bitmap, bounds, vector_dirty_pixels, update);

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
	int dstdepth = blit_desc.DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount;
	int wait_for_lock = lock_must_succeed(bounds, vector_dirty_pixels);
	int blit_width, blit_height;
	struct win_blit_params params;
	HRESULT result;
	RECT src, dst, margins;
	int dstxoffs;

	if (blit_swapxy)
	{
		blit_width = win_visible_height;
		blit_height = win_visible_width;
	}
	else
	{
		blit_width = win_visible_width;
		blit_height = win_visible_height;
	}

tryagain:
	if (win_d3d_tex_manage)
	{
		// we only need to lock a part of the surface
		src.left = 0;
		src.top = 0;
		src.right = blit_width *  effect_min_xscale + 16;
		src.bottom = blit_height * effect_min_yscale;

		// attempt to lock the blit surface
		result = IDirectDrawSurface7_Lock(blit_surface, &src, &blit_desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS | (wait_for_lock ? DDLOCK_WAIT : 0), NULL);
	}
	else
	{
		// attempt to lock the blit surface
		result = IDirectDrawSurface7_Lock(blit_surface, NULL, &blit_desc, DDLOCK_SURFACEMEMORYPTR | (wait_for_lock ? DDLOCK_WAIT : 0), NULL);
	}
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
	params.dstyoffs		= 0;
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

	params.flipx		= 0;
	params.flipy		= 0;
	params.swapxy		= 0;

	// adjust for more optimal bounds
	if (bounds && !update && !vector_dirty_pixels)
	{
		struct rectangle bounds_disoriented = { bounds->min_x, bounds->max_x, bounds->min_y, bounds->max_y };

		win_disorient_rect(&bounds_disoriented);

		if (blit_swapxy)
		{
			params.dstxoffs += (bounds_disoriented.min_x + (blit_flipy ? win_visible_rect.top : -win_visible_rect.top)) * effect_min_xscale;
			params.dstyoffs += (bounds_disoriented.min_y + (blit_flipx ? win_visible_rect.left : -win_visible_rect.left)) * effect_min_yscale;
		}
		else
		{
			params.dstxoffs += (bounds_disoriented.min_x + (blit_flipx ? win_visible_rect.left : -win_visible_rect.left)) * effect_min_xscale;
			params.dstyoffs += (bounds_disoriented.min_y + (blit_flipy ? win_visible_rect.top : -win_visible_rect.top)) * effect_min_yscale;
		}
		params.srcxoffs += bounds->min_x - win_visible_rect.left;
		params.srcyoffs += bounds->min_y - win_visible_rect.top;
		params.srcwidth = bounds->max_x - bounds->min_x + 1;
		params.srcheight = bounds->max_y - bounds->min_y + 1;
	}

	win_perform_blit(&params, 0);

	// unlock the surface
	IDirectDrawSurface7_Unlock(blit_surface, NULL);

	// blit the image to the texture surface when texture management in't used
	if (!win_d3d_tex_manage)
	{
		RECT blt_src = { params.dstxoffs, params.dstyoffs, params.dstxoffs, params.dstyoffs };

		if (blit_swapxy)
		{
			blt_src.right += params.srcheight * effect_min_yscale;
			blt_src.bottom += params.srcwidth * effect_min_xscale;
		}
		else
		{
			blt_src.right += params.srcwidth * effect_min_xscale;
			blt_src.bottom += params.srcheight * effect_min_yscale;
		}

		result = IDirectDrawSurface7_BltFast(texture_surface, params.dstxoffs - dstxoffs, params.dstyoffs, blit_surface, &blt_src, DDBLTFAST_WAIT);
		if (result == DDERR_SURFACELOST)
			goto surface_lost;
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt blit_surface to texture_surface: %08x\n", (UINT32)result);
			return 0;
		}
	}

	// make the src rect
	src.left = dstxoffs;
	src.top = 0;
	src.right = dstxoffs + (blit_width * effect_min_xscale);
	src.bottom = blit_height * effect_min_yscale;

	// window mode
	if (win_window_mode)
	{
		// just convert the client area to screen coords
		GetClientRect(win_video_window, &dst);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[0]);
		ClientToScreen(win_video_window, &((LPPOINT)&dst)[1]);
	}

	// full screen mode
	else
	{
		// constrain to the screen/window size
		dst.left = dst.top = 0;
		dst.right = primary_desc.dwWidth;
		dst.bottom = primary_desc.dwHeight;

			win_constrain_to_aspect_ratio(&dst, WMSZ_BOTTOMRIGHT, win_default_constraints, COORDINATES_DISPLAY);

		// center
		dst.left += (primary_desc.dwWidth - (dst.right - dst.left)) / 2;
		dst.top += (primary_desc.dwHeight - (dst.bottom - dst.top)) / 2;
		dst.right += dst.left;
		dst.bottom += dst.top;
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

	// render and flip
	if (!render_and_flip(&src, &dst, update, wait_for_lock))
		return 0;

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Restoring surfaces\n");

	// go ahead and adjust the window
	win_adjust_window();

	// restore the surfaces
	if (!restore_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	render_and_flip
//============================================================

static int render_and_flip(LPRECT src, LPRECT dst, int update, int wait_for_lock)
{
	static RECT prev_src, prev_dst;
	LPDIRECTDRAWSURFACE7 texture;
	D3DVIEWPORT7 viewport;
	HRESULT result;
	RECT margins;

	// determine the current zoom level
	win_d3d_current_zoom = win_d3d_effects_swapxy ? (dst->right - dst->left) / win_visible_width :
													(dst->bottom - dst->top) / win_visible_height;

	// determing if the window position has changed
	if (src->left != prev_src.left ||
		src->right != prev_src.right ||
		src->top != prev_src.top ||
		src->bottom != prev_src.bottom ||
		dst->left != prev_dst.left ||
		dst->right != prev_dst.right ||
		dst->top != prev_dst.top ||
		dst->bottom != prev_dst.bottom)
	{
		position_changed = 1;

		prev_src.left = src->left;
		prev_src.right = src->right;
		prev_src.top = src->top;
		prev_src.bottom = src->bottom;

		prev_dst.left = dst->left;
		prev_dst.right = dst->right;
		prev_dst.top = dst->top;
		prev_dst.bottom = dst->bottom;
	}

	if (position_changed)
	{
		win_d3d_effects_init(video_attributes);
		if (win_d3d_use_auto_effect)
		{
			win_d3d_effects_init_surfaces();
		}

		// adjust prescale settings to the new zoom level
		adjust_prescale(dst->right - dst->left, dst->bottom - dst->top);

		init_vertices_screen(src, dst);

		if (win_d3d_use_prescale || win_d3d_use_feedback)
			init_vertices_preprocess(src);

		position_changed = 0;
	}

tryagain:
	// if the surface is lost, bail an try again
	result = IDirectDrawSurface7_IsLost(back_surface);
	if (result == DDERR_SURFACELOST)
		goto surface_lost;

	texture = texture_surface;

	// determine if we need to render to the pre-process texture first
	if (win_d3d_use_prescale || win_d3d_use_feedback)
	{
		result = IDirect3DDevice7_SetRenderTarget(d3d_device7, preprocess_surface, 0);

		// prepare x/y coordinates
		viewport.dwX = 0;
		viewport.dwY = 0;
		viewport.dwWidth = preprocess_desc.dwWidth;
		viewport.dwHeight = preprocess_desc.dwHeight;

		// set up the viewport
		result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);

		result = IDirect3DDevice7_BeginScene(d3d_device7);

		if (win_d3d_use_feedback)
		{
			// alpha blend the new image with the previous one
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCALPHA);

			// stage 0 holds the new image, the previous one is still on the texture
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		}
		else
		{
			// just do a simple copy
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

			// stage 0 holds the image
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}

		// never use texture filtering
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, D3DTFN_POINT);

		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 0);

		// stage 1 isn't used
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		IDirect3DDevice7_SetTexture(d3d_device7, 0, texture);

		// now render the image using 3d hardware
		result = IDirect3DDevice7_DrawPrimitive(d3d_device7, D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, (void*)preprocess_vertex, 4, 0);
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to render to preprocess texture: %08x\n", (UINT32)result);
			return 0;
		}

		IDirect3DDevice7_SetTexture(d3d_device7, 0, NULL);

		result = IDirect3DDevice7_EndScene(d3d_device7);

		texture = preprocess_surface;
	}

	result = IDirect3DDevice7_SetRenderTarget(d3d_device7, back_surface, 0);

	// set up the viewport
	win_ddraw_fullscreen_margins(primary_desc.dwWidth, primary_desc.dwHeight, &margins);
	viewport.dwX = margins.left;
	viewport.dwY = margins.top;
	viewport.dwWidth = margins.right - margins.left;
	viewport.dwHeight = margins.bottom - margins.top;

	result = IDirect3DDevice7_SetViewport(d3d_device7, &viewport);

	result = IDirect3DDevice7_BeginScene(d3d_device7);

	// set the Diret3D state

	switch (win_d3d_use_rgbeffect)
	{
		case 1:
		{
			// RGB effects multiply
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		}
		case 2:
		{
			// RGB effects multiply & add
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		}
		default:
		{
			// no RGB effects
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
		}
	}

	IDirect3DDevice7_SetRenderState(d3d_device7, D3DRENDERSTATE_TEXTUREFACTOR, win_d3d_tfactor);

	// determine if we should render scanlines
	if (!win_d3d_use_scanlines || win_d3d_current_zoom < 2)
	{
		// no scanlines

		// indicate how to filter the image texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, d3dtfg_image);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MINFILTER, d3dtfn_image);

		// stage 0 holds the image
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 0);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

		// stage 1 isn't used
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		IDirect3DDevice7_SetTexture(d3d_device7, 0, texture);
	}
	else
	{
		// render scanlines

		// indicate how to filter the image texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_MAGFILTER, d3dtfg_image);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_MINFILTER, d3dtfn_image);

		// indicate how to filter the scanline texture
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MAGFILTER, d3dtfg_scanlines);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_MINFILTER, d3dtfn_scanlines);

		// stage 0 holds the scanlines
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_COLOROP, d3dtop_scanlines);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_TEXCOORDINDEX, 1);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

		// stage 1 holds the image
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_TEXCOORDINDEX, 0);
		IDirect3DDevice7_SetTextureStageState(d3d_device7, 1, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

		// use the best scanline texture for the current zoom level
		IDirect3DDevice7_SetTexture(d3d_device7, 0, win_d3d_scanline_surface[win_d3d_current_zoom >= 4 ? 1 : 0]);
		IDirect3DDevice7_SetTexture(d3d_device7, 1, texture);
	}

	// now render the image using 3d hardware
	result = IDirect3DDevice7_DrawPrimitive(d3d_device7, D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX2, (void*)screen_vertex, 4, 0);
	if (result != DD_OK)
	{
		// error, print the error and fall back
		if (verbose)
			fprintf(stderr, "Unable to render to back_surface: %08x\n", (UINT32)result);
		return 0;
	}

	IDirect3DDevice7_SetTexture(d3d_device7, 0, NULL);
	IDirect3DDevice7_SetTexture(d3d_device7, 1, NULL);

	result = IDirect3DDevice7_EndScene(d3d_device7);

	// sync to VBLANK?
	if ((win_wait_vsync || win_sync_refresh) && throttle && mame_get_performance_info()->game_speed_percent > 95)
	{
		BOOL is_vblank;

		// this counts as idle time
		profiler_mark(PROFILER_IDLE);

		result = IDirectDraw7_GetVerticalBlankStatus(ddraw7, &is_vblank);
		if (!is_vblank)
			result = IDirectDraw7_WaitForVerticalBlank(ddraw7, DDWAITVB_BLOCKBEGIN, 0);

		// idle time done
		profiler_mark(PROFILER_END);
	}

	// erase the edges if updating
	if (update)
	{
		RECT outer;
		win_ddraw_fullscreen_margins(primary_desc.dwWidth, primary_desc.dwHeight, &outer);
		erase_outer_rect(&outer, dst, win_window_mode ? primary_surface : back_surface);
	}

	// if in windowed mode, blit the image from the back surface to the primary surface
	if (win_window_mode)
	{
		// blit from the upper left of the back surface to the correct position on the screen
		RECT rect = { 0, 0, dst->right - dst->left, dst->bottom - dst->top };

		result = IDirectDrawSurface7_Blt(primary_surface, dst, back_surface, &rect, DDBLT_ASYNC, NULL);
		if (result == DDERR_SURFACELOST)
			goto surface_lost;
		if (result != DD_OK)
		{
			// otherwise, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt back_surface to primary_surface: %08x\n", (UINT32)result);
			return 0;
		}
	}
	// full screen mode: flip
	else
	{
#if SHOW_FLIP_TIMES
		static cycles_t total;
		static int count;
		cycles_t start = osd_cycles(), stop;
#endif

		result = IDirectDrawSurface7_Flip(primary_surface, NULL, DDFLIP_WAIT | ((!win_triple_buffer || !throttle) ?  DDFLIP_NOVSYNC : 0));

#if SHOW_FLIP_TIMES
		stop = osd_cycles();
		if (++count > 100)
		{
			total += stop - start;
			usrintf_showmessage("Avg Flip = %d", (int)(total / (count - 100)));
		}
#endif
	}

	// blit the pattern for the RGB effects
	if (win_d3d_use_rgbeffect)
	{
		int zoom = (win_d3d_current_zoom > MAX_AUTOEFFECT_ZOOM) ? MAX_AUTOEFFECT_ZOOM : win_d3d_current_zoom;

		if (win_d3d_use_auto_effect && (win_window_mode ||
										!win_keep_aspect ||
										win_force_int_stretch == FORCE_INT_STRECT_NONE ||
										win_force_int_stretch == FORCE_INT_STRECT_HOR ||
										zoom != win_d3d_current_zoom))
		{
			RECT rgb_dst;
			RECT rgb_src = { 0, 0,
							 win_d3d_effects_swapxy ? zoom * win_visible_width : dst->right - dst->left,
							 win_d3d_effects_swapxy ? dst->bottom - dst->top : zoom * win_visible_height};

			if (win_window_mode)
			{
				rgb_dst.left = 0; rgb_dst.top = 0;
				rgb_dst.right = dst->right - dst->left;
				rgb_dst.bottom = dst->bottom - dst->top;
			}

			result = IDirectDrawSurface7_Blt(back_surface, win_window_mode ? &rgb_dst : dst, win_d3d_background_surface, &rgb_src, DDBLT_ASYNC, NULL);
		}
		else
		{
			RECT rect = { 0, 0, dst->right - dst->left, dst->bottom - dst->top };

			result = IDirectDrawSurface7_BltFast(back_surface, win_window_mode ? 0 : dst->left, win_window_mode ? 0 : dst->top, win_d3d_background_surface, &rect, DDBLTFAST_WAIT);
		}
		if (result != DD_OK)
		{
			// error, print the error and fall back
			if (verbose)
				fprintf(stderr, "Unable to blt RGB effect to back_surface: %08x\n",	(UINT32)result);
			return 0;
		}
	}

	return 1;

surface_lost:
	if (verbose)
		fprintf(stderr, "Restoring surfaces\n");

	// go ahead and adjust the window
	win_adjust_window();

	// restore the surfaces
	if (!restore_surfaces())
		goto tryagain;

	// otherwise, return failure
	return 0;
}



//============================================================
//	set_texture_coordinates
//============================================================

static void set_texture_coordinates(LPRECT src, int texture_width, int texture_height)
{
	if (win_d3d_tex_manage)
	{
		if (blit_swapxy)
		{
			screen_vertex[2].tu = screen_vertex[3].tu = (float)(!blit_flipy ? src->right : src->left) / texture_width;
			screen_vertex[0].tu = screen_vertex[1].tu = (float)(!blit_flipy ? src->left : src->right) / texture_width;
			screen_vertex[1].tv = screen_vertex[3].tv = (float)(!blit_flipx ? src->bottom : src->top) / texture_height;
			screen_vertex[0].tv = screen_vertex[2].tv = (float)(!blit_flipx ? src->top : src->bottom) / texture_height;
		}
		else
		{
			screen_vertex[0].tu = screen_vertex[2].tu = (float)(blit_flipx ? src->right : src->left) / texture_width;
			screen_vertex[1].tu = screen_vertex[3].tu = (float)(blit_flipx ? src->left : src->right) / texture_width;
			screen_vertex[0].tv = screen_vertex[1].tv = (float)(blit_flipy ? src->bottom : src->top) / texture_height;
			screen_vertex[2].tv = screen_vertex[3].tv = (float)(blit_flipy ? src->top : src->bottom) / texture_height;
		}
	}
	else
	{
		if (blit_swapxy)
		{
			screen_vertex[2].tu = screen_vertex[3].tu = !blit_flipy ? (float)(src->right - src->left) / texture_width : 0;
			screen_vertex[0].tu = screen_vertex[1].tu = !blit_flipy ? 0 : (float)(src->right - src->left)  / texture_width;
			screen_vertex[1].tv = screen_vertex[3].tv = !blit_flipx ? (float)(src->bottom - src->top) / texture_height : 0;
			screen_vertex[0].tv = screen_vertex[2].tv = !blit_flipx ? 0 : (float)(src->bottom - src->top) / texture_height;
		}
		else
		{
			screen_vertex[0].tu = screen_vertex[2].tu = blit_flipx ? (float)(src->right - src->left) / texture_width : 0;
			screen_vertex[1].tu = screen_vertex[3].tu = blit_flipx ? 0 : (float)(src->right - src->left) / texture_width;
			screen_vertex[0].tv = screen_vertex[1].tv = blit_flipy ? (float)(src->bottom - src->top) / texture_height : 0;
			screen_vertex[2].tv = screen_vertex[3].tv = blit_flipy ? 0 : (float)(src->bottom - src->top) / texture_height;
		}
	}
}



//============================================================
//	init_vertices_screen
//============================================================

static void init_vertices_screen(LPRECT src, LPRECT dst)
{
	int i;

	for (i = 0; i < 4; i++)
	{
		screen_vertex[i].rhw = 1;
		screen_vertex[i].color = 0xFFFFFFFF;
		screen_vertex[i].specular = 0xFFFFFFFF;
	}

	// set the texture coordinates
	set_texture_coordinates(src, texture_desc.dwWidth, texture_desc.dwHeight);

	// set the vertex coordinates
	if (win_window_mode)
	{
		// render to the upper left of the back surface
		screen_vertex[0].sx = -0.5f;								   screen_vertex[0].sy = -0.5f;
		screen_vertex[1].sx = -0.5f + (float)(dst->right - dst->left); screen_vertex[1].sy = -0.5f;
		screen_vertex[2].sx = -0.5f;								   screen_vertex[2].sy = -0.5f + (float)(dst->bottom - dst->top);
		screen_vertex[3].sx = -0.5f + (float)(dst->right - dst->left); screen_vertex[3].sy = -0.5f + (float)(dst->bottom - dst->top);
	}
	else
	{
		screen_vertex[0].sx = -0.5f + (float)dst->left;  screen_vertex[0].sy = -0.5f + (float)dst->top;
		screen_vertex[1].sx = -0.5f + (float)dst->right; screen_vertex[1].sy = -0.5f + (float)dst->top;
		screen_vertex[2].sx = -0.5f + (float)dst->left;  screen_vertex[2].sy = -0.5f + (float)dst->bottom;
		screen_vertex[3].sx = -0.5f + (float)dst->right; screen_vertex[3].sy = -0.5f + (float)dst->bottom;
	}

	// set texture coordinates for scanlines
	if (win_d3d_use_scanlines)
	{
		if (win_d3d_effects_swapxy)
		{
			screen_vertex[2].tv1 = screen_vertex[0].tv1 = win_d3d_effects_flipy ? (float)win_visible_width : 0;
			screen_vertex[3].tv1 = screen_vertex[1].tv1 = win_d3d_effects_flipy ? 0 : (float)win_visible_width;
			screen_vertex[2].tu1 = screen_vertex[3].tu1 = win_d3d_effects_flipx ? (float)win_visible_height : 0;
			screen_vertex[0].tu1 = screen_vertex[1].tu1 = win_d3d_effects_flipx ? 0 : (float)win_visible_height;
		}
		else
		{
			screen_vertex[0].tu1 = screen_vertex[2].tu1 = win_d3d_effects_flipx ? (float)win_visible_width : 0;
			screen_vertex[1].tu1 = screen_vertex[3].tu1 = win_d3d_effects_flipx ? 0 : (float)win_visible_width;
			screen_vertex[0].tv1 = screen_vertex[1].tv1 = win_d3d_effects_flipy ? (float)win_visible_height : 0;
			screen_vertex[2].tv1 = screen_vertex[3].tv1 = win_d3d_effects_flipy ? 0 : (float)win_visible_height;
		}
	}
}



//============================================================
//	init_vertices_preprocess
//============================================================

static void init_vertices_preprocess(LPRECT src)
{
	RECT rect = { 0, 0, (src->right - src->left) * current_prescalex, (src->bottom - src->top) * current_prescaley };
	int i;

	for (i = 0; i < 4; i++)
	{
		preprocess_vertex[i].rhw = 1;
		preprocess_vertex[i].color = win_d3d_preprocess_tfactor;
		preprocess_vertex[i].specular = 0xFFFFFFFF;
	}

	// set the texture coordinates for the preprocess vertices
	if (win_d3d_tex_manage)
	{
		preprocess_vertex[0].tu = preprocess_vertex[2].tu = (float)src->left / texture_desc.dwWidth;
		preprocess_vertex[1].tu = preprocess_vertex[3].tu = (float)src->right / texture_desc.dwWidth;
		preprocess_vertex[0].tv = preprocess_vertex[1].tv = (float)src->top / texture_desc.dwHeight;
		preprocess_vertex[2].tv = preprocess_vertex[3].tv = (float)src->bottom / texture_desc.dwHeight;
	}
	else
	{
		preprocess_vertex[0].tu = preprocess_vertex[2].tu = 0;
		preprocess_vertex[1].tu = preprocess_vertex[3].tu = (float)(src->right - src->left) / texture_desc.dwWidth;
		preprocess_vertex[0].tv = preprocess_vertex[1].tv = 0;
		preprocess_vertex[2].tv = preprocess_vertex[3].tv = (float)(src->bottom - src->top) / texture_desc.dwHeight;
	}

	// set the texture coordinates for the screen vertices
	set_texture_coordinates(&rect, preprocess_desc.dwWidth, preprocess_desc.dwHeight);

	// set vertex coordinates for rendering to the pre-process texture
	preprocess_vertex[0].sx = -0.5f + rect.left;  preprocess_vertex[0].sy = -0.5f + rect.top;
	preprocess_vertex[1].sx = -0.5f + rect.right; preprocess_vertex[1].sy = -0.5f + rect.top;
	preprocess_vertex[2].sx = -0.5f + rect.left;  preprocess_vertex[2].sy = -0.5f + rect.bottom;
	preprocess_vertex[3].sx = -0.5f + rect.right; preprocess_vertex[3].sy = -0.5f + rect.bottom;
}
