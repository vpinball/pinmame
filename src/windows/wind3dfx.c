//============================================================
//
//	wind3dfx.c - Win32 Direct3D 7 (with DirectDraw 7) effects
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

// standard C headers
#include <math.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "video.h"
#include "blit.h"
#include "rc.h"
#include "wind3d.h"
#include "wind3dfx.h"

// RGB effects pattern data
#include "pattern.h"



//============================================================
//	IMPORTS
//============================================================

// from input.c
extern int verbose;

// from wind3d.c (surfaces)
extern LPDIRECTDRAWSURFACE7 win_d3d_background_surface;
extern LPDIRECTDRAWSURFACE7 win_d3d_scanline_surface[2];

// from wind3d.c (global settings)
extern UINT8 win_d3d_use_auto_effect;
extern UINT8 win_d3d_use_rgbeffect;
extern UINT8 win_d3d_use_scanlines;
extern UINT8 win_d3d_use_prescale;
extern UINT8 win_d3d_use_feedback;

// from wind3d.c (prescale)
extern int win_d3d_prescalex, win_d3d_prescaley;

// from wind3d.c (zoom level)
extern int win_d3d_current_zoom;



//============================================================
//	GLOBAL VARIABLES
//============================================================

UINT32 win_d3d_tfactor;
UINT32 win_d3d_preprocess_tfactor;



//============================================================
//	LOCAL VARIABLES
//============================================================

// command line parsing
static int use_effect_preset = 0;
static int use_scanlines;
static int use_prescale;
static int use_feedback;
static int use_rotate;
static int use_pixelcounter = 0;

static int priority_use_effect_preset;
static int priority_use_scanlines;
static int priority_use_prescale;
static int priority_use_feedback;
static int priority_use_rotate;
static int priority_use_pixelcounter;

static char *d3d_rc_effect;
static char *d3d_rc_custom;
static char *d3d_rc_expert;
static int d3d_rc_scan;
static int d3d_rc_prescale;
static int d3d_rc_feedback;
static int d3d_rc_rotate;

struct d3d_preset_info {
	const char *presetname;
	int use_rgbeffect; int pattern_mode; int pattern_white_level; int pattern_black_level; int pattern_desaturation;
	int image_white_level_adjust; int use_feedback; int use_prescale; int use_scanlines;
	const char *pattern_name;
};

// built-in presets
static struct d3d_preset_info effects_preset[] =
{
	{ "",				0, 0, 0,	  0,	0,	  0,	0,	  -2,	-1,			"none" },						//  0

	// auto presets (all of these must enable the same effects)
	{ "",				2, 0, 0,	  0,	0,	  0,	0,	  -2,	0x00000000,	"none" },						//  1 (1 x zoom)
	{ "",				2, 0, 0x0080, 0x20, 0x00, 0x00, 0,	  -2,	0x008F8F8F, "4x4_rgb_pattern.i.rgb" },		//  2 (2 x zoom)
	{ "",				2, 0, 0x00A0, 0x30, 0x00, 0x00, 0,	  -2,	0x005F5F5F, "4x6_rgb_pattern.i.rgb" },		//  3 (3 x zoom)
	{ "",				2, 1, 0x0080, 0x20, 0x00, 0x20, 0,	  -2,	0x004F4F4F, "6x8_rgb_pattern.i.rgb" },		//  4 (4 x zoom)
	{ "",				2, 1, 0x0060, 0x10, 0x48, 0x20, 0,	  -2,	0x005F5F5F, "9x10_ellipsoid.i.rgb" },		//  5 (5 x zoom)
	{ "",				2, 0, 0,	  0,	0,	  0,	0,	  -2,	0x00000000,	"none" },						//  6
	{ "",				2, 0, 0,	  0,	0,	  0,	0,	  -2,	0x00000000,	"none" },						//  7

	// normal presets
	{ "rgbminmask",		1, 0, 0x0100, 0x80, 0x40, 0x00, 0,	  0,	-1,			"4x6_rgb_pattern.i.rgb" },		//  8
	{ "dotmedmask",		1, 0, 0x0100, 0x80, 0x40, 0x00, 0,	  0x11,	-1,			"10x6_large_dot.i.rgb" },		//  9
	{ "rgbmedmask",		1, 0, 0x0100, 0x90, 0x80, 0x00, 0,	  0x11,	-1,			"12x10_large_ellipsoid.i.rgb" },// 10
	{ "rgbmicro",		1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"4x4_mame_rgbtiny.i.rgb" },		// 11
	{ "rgbtiny",		1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"8x8_mame_rgbtiny.i.rgb" },		// 12
	{ "aperturegrille",	2, 0, 0x0030, 0x00, 0x00, 0x20, 0,	  -2,	0x00BFBFBF,	"3x1_aperture_grille.i.rgb" },	// 13
	{ "dotmedbright",	2, 0, 0x0040, 0x00, 0x00, 0x08, 0,	  0x11,	-1,			"10x6_large_dot.i.rgb" },		// 14
	{ "rgbmaxbright",	2, 1, 0x00E0, 0x00, 0x40, 0x10, 0,	  0x11,	-1,			"18x10_large_round.i.rgb" },	// 15

	// presets from blit.c
	{ "scan25", 		0, 0, 0x0100, 0,	0,	  0,	0,	  -2,	0x003F3F3F,	"none" },						// 16
	{ "scan50", 		0, 0, 0x0100, 0,	0,	  0,	0,	  -2,	0x007F7F7F,	"none" },						// 17
	{ "scan75", 		0, 0, 0x0100, 0,	0,	  0,	0,	  -2,	0x00BFBFBF,	"none" },						// 18
	{ "scan75v", 		0, 0, 0x0100, 0,	0,	  0,	0,	  -2,	0x00BFBFBF,	"none" },						// 19
	{ "rgb16",			1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"16x8_rgb_pattern.i.rgb" },		// 20
	{ "rgb6",			1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"16x6_rgb_pattern.i.rgb" },		// 21
	{ "rgb4",			1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"16x4_rgb_pattern.i.rgb" },		// 22
	{ "rgb4v",			1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"16x4_rgb_pattern.i.rgb" },		// 23
	{ "rgb3",			1, 0, 0x0100, 0xC0, 0x00, 0x00, 0,	  0x22,	-1,			"16x3_rgb_pattern.i.rgb" },		// 24
	{ "sharp",			0, 0, 0,	  0,	0,	  0,	0,	  0x22,	-1,			"none" },						// 25


	{ NULL,				0, 0, 0,	  0,	0,	  0,	0,	  0,	-1,			NULL },
};

// the settings from -effect custom
static struct d3d_preset_info custom_preset;

// the settings in use
static struct d3d_preset_info active_preset;



//============================================================
//	PROTOTYPES
//============================================================

static int win_d3d_decode_scan(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_feedback(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_prescale(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_rotate(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_custom(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_effect(struct rc_option *option, const char *arg, int priority);
static int win_d3d_decode_expert(struct rc_option *option, const char *arg, int priority);

static int effects_rgb_init(void);
static int effects_scanline_init(void);

static int replicate_pattern(int pattern_xsize, int pattern_ysize, UINT8 *pattern_data);
static void win_ddrawsurf_plot_pixel(const LPDDSURFACEDESC2 surface_desc, int x, int y, int colour);



//============================================================
//	Options
//============================================================

struct rc_option win_d3d_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows Direct3D 2D video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "d3dtexmanage", NULL, rc_bool, &win_d3d_tex_manage, "1", 0, 0, NULL, "use DirectX texture management" },
	{ "d3dfilter", "flt", rc_int, &win_d3d_use_filter, "1", 0, 4, NULL, "interpolation method" },

	{ "d3dfeedback", NULL, rc_int, &d3d_rc_feedback, "0", 0, 100, win_d3d_decode_feedback, "feedback strength" },
	{ "d3dscan", NULL, rc_int, &d3d_rc_scan, "100", 0, 100, win_d3d_decode_scan, "scanline intensity" },
	{ "d3deffectrotate", NULL, rc_bool, &d3d_rc_rotate, "1", 0, 0, win_d3d_decode_rotate, "enable rotation of effects for rotated games" },
	{ "d3dprescale", NULL, rc_string, &d3d_rc_prescale, "auto", 0, 0, win_d3d_decode_prescale, "prescale effect" },
	{ "d3deffect", NULL, rc_string, &d3d_rc_effect, "none", 0, 0, win_d3d_decode_effect, "specify the blitting effects" },
	{ "d3dcustom", NULL, rc_string, &d3d_rc_custom, NULL, 0, 0, win_d3d_decode_custom, "customised blitting effects preset" },
	{ "d3dexpert", NULL, rc_string, &d3d_rc_expert, NULL, 0, 0, win_d3d_decode_expert, "additional customised settings (undocumented)" },


	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//	Effect command line parsing
//============================================================

static int win_d3d_decode_scan(struct rc_option *option, const char *arg, int priority)
{
	option->priority = priority;

	if (priority_use_scanlines <= priority)
	{
		double intensity;

		priority_use_scanlines = priority;
		sscanf(arg, "%lf", &intensity);
		use_scanlines = (int)(intensity * 255 / 100);
		if (use_scanlines < 0 || use_scanlines >= 255)
		{
			use_scanlines = -1;

			return 0;
		}
	}

	return 0;
}

static int win_d3d_decode_feedback(struct rc_option *option, const char *arg, int priority)
{
	option->priority = priority;

	if (priority_use_feedback <= priority)
	{
		double intensity;

		priority_use_feedback = priority;
		sscanf(arg, "%lf", &intensity);
		use_feedback = (int)(intensity * 255 / 100);
		if (use_feedback < 0 || use_feedback >= 255)
		{
			use_feedback = 0;
			return 0;
		}
	}

	return 0;
}

static int win_d3d_decode_prescale(struct rc_option *option, const char *arg, int priority)
{
	option->priority = priority;

	if (priority_use_prescale <= priority)
	{
		long prescale;

		priority_use_prescale = priority;

		// none: don't use prescale effect
		if (!strcmp(arg, "none"))
		{
			use_prescale = 0;
		}
		// auto: let blitter decide
		else if (!strcmp(arg, "auto"))
		{
			use_prescale = -2;
		}
		// full: use a strong effect
		else if (!strcmp(arg, "full"))
		{
			use_prescale = -3;
		}
		// force a specific value
		else if ((prescale = strtol(arg, NULL, 0)) != 0)
		{
			if (prescale < 2)
				prescale = 2;
			if (prescale > MAX_PRESCALE)
				prescale = MAX_PRESCALE;

			use_prescale = (prescale << 4) | prescale;
		}
		else
		{
			fprintf(stderr, "error: invalid value for d3dprescale: %s\n", arg);
			return -1;
		}
	}

	return 0;
}

static int win_d3d_decode_rotate(struct rc_option *option, const char *arg, int priority)
{
	option->priority = priority;

	if (priority_use_rotate <= priority)
	{
		priority_use_rotate = priority;
		if (d3d_rc_rotate)
		{
			use_rotate = 1;
		}
		else
		{
			use_rotate = 0;
		}
	}

	return 0;
}

static int win_d3d_decode_custom(struct rc_option *option, const char *arg, int priority)
{
	option->priority = priority;

	if (priority_use_effect_preset <= priority)
	{
		static char pattern_name[256];

		sscanf(arg, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%255s",
				&custom_preset.use_rgbeffect, &custom_preset.pattern_mode,
				&custom_preset.pattern_white_level, &custom_preset.pattern_black_level,
				&custom_preset.pattern_desaturation, &custom_preset.image_white_level_adjust,
				&custom_preset.use_feedback, &custom_preset.use_prescale, &custom_preset.use_scanlines,
				pattern_name);

		if (!custom_preset.pattern_white_level || !strlen(pattern_name))
			return 1;

		priority_use_effect_preset = priority;
		custom_preset.pattern_name = pattern_name;
		use_effect_preset = -1;
	}

	return 0;
}

static int win_d3d_decode_effect(struct rc_option *option, const char *arg, int priority)
	{
	int i;

	// ensure .ini settings will still be processed
	option->priority = 0;

	// no effects
	if (!strcmp(arg, "none"))
	{
		if (priority_use_effect_preset <= priority)
		{
			priority_use_effect_preset = priority;
			win_d3d_use_auto_effect = 0;
			use_effect_preset = 0;
		}

		return 0;
	}

	// use a preset
	for (i = 0; effects_preset[i].presetname; i++)
	{
		if (!strcmp(arg, effects_preset[i].presetname))
		{
			if (priority_use_effect_preset <= priority)
			{
				priority_use_effect_preset = priority;
				win_d3d_use_auto_effect = 0;
				use_effect_preset = i;
			}
			return 0;
		}
	}

	// auto preset
	if (!strcmp(arg, "auto"))
	{
		if (priority_use_effect_preset <= priority)
		{
			priority_use_effect_preset = priority;
			win_d3d_use_auto_effect = 1;
		}
		return 0;
	}

	// couldn't parse the setting

	return 1;
}

static int win_d3d_decode_expert(struct rc_option *option, const char *arg, int priority)
{
	// ensure .ini settings will still be processed
	option->priority = 0;

	// extra interface for normal settings (takes a floating point value)

	// scanlines
	if (!strncmp(arg, "scan", 4))
		return win_d3d_decode_scan(option, arg + 4, 0);

	// feedback
	if (!strncmp(arg, "feedback", 8))
		return win_d3d_decode_feedback(option, arg + 8, 0);

	// extra expert settings

	// disable all effects
	if (!strcmp(arg, "disable"))
	{
		if (priority_use_effect_preset <= priority)
		{
			priority_use_effect_preset = priority;
			win_d3d_use_auto_effect = 0;
			use_effect_preset = 0;
		}
		if (priority_use_scanlines <= priority)
		{
			priority_use_scanlines = priority;
			use_scanlines = -1;
		}
		if (priority_use_feedback <= priority)
		{
			priority_use_feedback = priority;
			use_feedback = 0;
		}
		if (priority_use_prescale <= priority)
		{
			priority_use_prescale = priority;
			use_prescale = 0;
		}

		return 0;
	}

	// complete prescale control
	if (!strncmp(arg, "prescale", 8))
	{
		if (priority_use_prescale <= priority)
		{
			priority_use_prescale = priority;

			sscanf(arg + 8, "%i", &use_prescale);
			if (use_prescale < 0 || use_prescale >= 255)
			{
				use_prescale = 0;
				return 1;
			}
		}
		return 0;
	}

	// alternate scanline textures
	if (!strcmp(arg, "pixelcounter"))
	{
		if (priority_use_pixelcounter <= priority)
		{
			priority_use_pixelcounter = priority;
			use_pixelcounter = 1;
		}
		return 0;
	}

	// complete control over rotation of effects effects
	if (!strncmp(arg, "rotate", 6))
	{
		if (priority_use_rotate <= priority)
		{
			priority_use_rotate = priority;
			sscanf(arg + 6, "%i", &use_rotate);
			if (use_rotate < 0 || use_rotate > 255)
			{
				use_rotate = 1;
				return 1;
			}

			if (use_rotate == 0)
				use_rotate = 1;
		}
		return 0;
	}

	// disable individual effects
	if (!strcmp(arg, "nopixelcounter"))
	{
		if (priority_use_pixelcounter <= priority)
		{
			priority_use_pixelcounter = priority;
			use_pixelcounter = 0;
		}
		return 0;
	}
	if (!strcmp(arg, "norotate"))
	{
		if (priority_use_rotate <= priority)
		{
			priority_use_rotate = priority;
			use_rotate = 0;
		}
		return 0;
	}

	// try the regular settings
	if (win_d3d_decode_effect(option, arg, priority) == 0)
		return 0;

	// couldn't parse the setting

	return 1;
}



//============================================================
//	win_d3d_effects_in_use
//============================================================

int win_d3d_effects_in_use(void)
{
	if (win_d3d_use_auto_effect ||
		use_effect_preset ||
		use_scanlines != -1 ||
		use_feedback ||
		(use_prescale < -1 && use_prescale > 0x11))
	{
		return 1;
	}

	return 0;
}



//============================================================
//	Global effects initialisation
//============================================================

int win_d3d_effects_init(int attributes)
{
	int scanline_intensity;

	if (win_d3d_use_auto_effect)
	{
		int zoom = (win_d3d_current_zoom > MAX_AUTOEFFECT_ZOOM) ? MAX_AUTOEFFECT_ZOOM : win_d3d_current_zoom;
		active_preset = effects_preset[zoom > 1 ? zoom : 1];
	}
	else
	{
		active_preset = use_effect_preset == -1 ? custom_preset : effects_preset[use_effect_preset];
	}

	win_d3d_use_rgbeffect = active_preset.use_rgbeffect;

	if (use_rotate == 0)
	{
		win_d3d_effects_swapxy = win_d3d_effects_flipx = win_d3d_effects_flipy = 0;
	}
	else if (use_rotate == 1)
	{
		win_d3d_effects_swapxy = blit_swapxy;
		win_d3d_effects_flipx = blit_flipx;
		win_d3d_effects_flipy = blit_flipy;
	}
	else
	{
		win_d3d_effects_swapxy = (use_rotate >> 4) & 1;
		win_d3d_effects_flipx = (use_rotate >> 5) & 1;
		win_d3d_effects_flipy = (use_rotate >> 6) & 1;
	}

	// scanlines
	if (use_scanlines != -1)
		active_preset.use_scanlines = (use_scanlines << 16) |
									  (use_scanlines <<  8) |
									  (use_scanlines <<  0);

	// do some adjustments if the effects are rotated to compensate for imperfect CRTs (this only affects scanlines and RGB effects)
	if (win_d3d_effects_swapxy)
	{
		active_preset.pattern_white_level = active_preset.pattern_white_level * 240 / 256;
		active_preset.pattern_desaturation += (256 - active_preset.pattern_desaturation) * 112 / 256;
		if (active_preset.use_scanlines != -1)
		{
			active_preset.use_scanlines = ((((active_preset.use_scanlines >> 16) & 255) * 224 / 256) << 16) |
										  ((((active_preset.use_scanlines >>  8) & 255) * 224 / 256) <<  8) |
										  ((((active_preset.use_scanlines >>  0) & 255) * 224 / 256) <<  0);
		}
	}

	// scanlines
	if (active_preset.use_scanlines == -1)
	{
		scanline_intensity = 0x00FFFFFF;
		win_d3d_use_scanlines = 0;
	}
	else
	{
		scanline_intensity = active_preset.use_scanlines;
		win_d3d_use_scanlines = 1;
	}

	// set up the texture colours for the scanline effect
	win_d3d_tfactor = ((255 - active_preset.image_white_level_adjust) << 24) | scanline_intensity;

	// feedback
	if (use_feedback)
		active_preset.use_feedback = use_feedback;

	win_d3d_use_feedback = active_preset.use_feedback ? 1 : 0;

	// set up the texture colours for the feedback effect
	if (win_d3d_use_feedback)
	{
		win_d3d_preprocess_tfactor = (active_preset.use_feedback		 << 24) |
									 ((255 - active_preset.use_feedback) << 16) |
									 ((255 - active_preset.use_feedback) <<  8) |
									 ((255 - active_preset.use_feedback) <<  0);
	}
	else
	{
		win_d3d_preprocess_tfactor = 0xFFFFFFFF;
	}

	// prescale
	win_d3d_prescalex = 1;
	win_d3d_prescaley = 1;
	win_d3d_use_prescale = 0;

	if (win_d3d_use_filter)
	{
		int prescale = (use_prescale == -2) ? active_preset.use_prescale : use_prescale;

		// auto
		if (prescale == -2)
		{
			win_d3d_use_prescale = 2;
		}
		// full
		else if (prescale == -3)
		{
			win_d3d_use_prescale = 3;
		}
		// force a specific size
		else if (prescale > 0x11)
		{
			win_d3d_use_prescale = 1;

			win_d3d_prescalex = prescale >> 4;
			win_d3d_prescaley = prescale & 15;
		}
	}

	if (attributes & VIDEO_TYPE_VECTOR)
	{
		win_d3d_use_auto_effect = 0;
		win_d3d_use_rgbeffect = 0;
		win_d3d_use_scanlines = 0;
		win_d3d_use_prescale = 0;

		win_d3d_prescalex = 1;
		win_d3d_prescaley = 1;
	}

	return 0;
}



//============================================================
//	Initialise surfaces used for effects
//============================================================

int win_d3d_effects_init_surfaces(void)
{
	if (win_d3d_background_surface)
		if (effects_rgb_init())
			return 1;

	if (win_d3d_scanline_surface[0] && win_d3d_scanline_surface[1])
		if (effects_scanline_init())
			return 1;

	return 0;
}



//============================================================
//	Initialise scanline surfaces
//============================================================

static int effects_scanline_init(void)
{
	int scanline_data[2][4] = { { 0xFFFFFF, 0x000000, -1, -1 }, { 0x9F9F9F, 0xFFFFFF, 0x9F9F9F, 0x000000 } };

	DDSURFACEDESC2 surface_desc = { sizeof(DDSURFACEDESC2) };
	HRESULT result;
	unsigned int x, y;
	int i;

	for (i = 0; i < 2; i++)
	{
		// lock the surface
		result = IDirectDrawSurface7_Lock(win_d3d_scanline_surface[i], NULL, &surface_desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
		if (result != DD_OK)
			return 1;

		// copy the data
		for (y = 0; y < (2 << i); y++)
			for (x = 0; x < (2 << i); x++)
				if (use_pixelcounter)
				{
					if (x && y)
						win_ddrawsurf_plot_pixel(&surface_desc, x, y, 0xFFFFFF);
					else
						win_ddrawsurf_plot_pixel(&surface_desc, x, y, 0x000000);
				}
				else
				{
					win_ddrawsurf_plot_pixel(&surface_desc, x, y, scanline_data[i][y]);
				}

		// unlock the surface
		result = IDirectDrawSurface7_Unlock(win_d3d_scanline_surface[i], NULL);
	}

	return 0;
}



//============================================================
//	Initialise RGB effects surface
//============================================================

static int effects_rgb_init(void)
{
	UINT8 *pattern_rgb_data;
	int x, y, pattern_xsize, pattern_ysize;
	int patternsize;
	int internal_pattern, i;
	const char *pattern_name;

	// fail if we have no RGB effects pattern
	if (active_preset.pattern_name == NULL)
		return 1;
	if (!strlen(active_preset.pattern_name))
		return 1;

	// exit without an error if the pattern name is "none"
	if (!strcmp("none", active_preset.pattern_name))
		return 0;

	// strip any path information from the pattern name
	i = strlen(active_preset.pattern_name);
	while (i > 0)
	{
		if (active_preset.pattern_name[i - 1] == '/' || active_preset.pattern_name[i - 1] == '\\')
			break;
		i--;
	}
	pattern_name = &active_preset.pattern_name[i];

	// now get the size of the pattern from the pattern name
	sscanf(pattern_name, "%i x %i", &pattern_xsize, &pattern_ysize);
	patternsize = pattern_xsize * pattern_ysize * 4;

	// copy the pattern
	pattern_rgb_data = (UINT8 *)malloc(patternsize);

	for (i = 0, internal_pattern = 0; builtin_patterns[i].name; i++)
	{
		if (!strcmp(builtin_patterns[i].name, active_preset.pattern_name))
		{
			internal_pattern = 1;
			memcpy(pattern_rgb_data, builtin_patterns[i].data, patternsize);
			break;
		}
	}

	if (!internal_pattern) {
		FILE* fp = fopen(active_preset.pattern_name, "rb");
		int filesize;

		if (fp == NULL)
		{
			if (verbose)
				fprintf(stderr, "Unable to find RGB effects pattern\n");
			free(pattern_rgb_data);
			return 1;
		}
		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		if (filesize != patternsize)
		{
			if (verbose)
				fprintf(stderr, "RGB pattern has a wrong filesize (expected %i bytes, found %i)\n", patternsize, filesize);
			free(pattern_rgb_data);
			return 1;
		}
		fseek(fp, 0, SEEK_SET);
		fread(pattern_rgb_data, 1, patternsize, fp);
		fclose(fp);
	}

	// Pre-process the pattern
	for (y = 0; y < pattern_ysize; y++)
	{
		UINT8 *pattern = &pattern_rgb_data[y * (pattern_xsize << 2)];
		for (x = 0; x < pattern_xsize; x++)
		{
			int desaturate = 0;
			int max = 0;
			int n, z;

			if (pattern[(x << 2) + 0] != pattern[(x << 2) + 1] ||
				pattern[(x << 2) + 0] != pattern[(x << 2) + 2] ||
				pattern[(x << 2) + 1] != pattern[(x << 2) + 2])
			{

				if (pattern[(x << 2) + 0] >= pattern[(x << 2) + 1] &&
					pattern[(x << 2) + 0] >= pattern[(x << 2) + 2])
				{
					max = pattern[(x << 2) + 0];
					desaturate |= 1;
				}
				if (pattern[(x << 2) + 1] >= pattern[(x << 2) + 0] &&
					pattern[(x << 2) + 1] >= pattern[(x << 2) + 2])
				{
					max = pattern[(x << 2) + 1];
					desaturate |= 2;
				}
				if (pattern[(x << 2) + 2] >= pattern[(x << 2) + 0] &&
					pattern[(x << 2) + 2] >= pattern[(x << 2) + 1])
				{
					max = pattern[(x << 2) + 2];
					desaturate |= 4;
				}

				desaturate ^= 7;
			}

			for (z = 0; z < 3; z++)
			{
				if ((active_preset.pattern_mode & 1) == 0 || pattern[(x << 2) + 3])
				{
					n = pattern[(x << 2) + z];
					if (desaturate & (1 << z))
						n += (max - n) * active_preset.pattern_desaturation / 256;

					n = n * active_preset.pattern_white_level / 256;
					pattern[(x << 2) + z] = (n > 255) ? 255 : active_preset.pattern_black_level +
															  (n * (256 - active_preset.pattern_black_level) / 256);
				}
			}
		}
	}

	// replicate pattern over surface
	replicate_pattern(pattern_xsize, pattern_ysize, pattern_rgb_data);

	free(pattern_rgb_data);

	return 0;
}

static int replicate_pattern(int pattern_xsize, int pattern_ysize, UINT8 *pattern_data)
{
	DDSURFACEDESC2 surface_desc = { sizeof(DDSURFACEDESC2) };
	HRESULT result;
	unsigned int x, y;
	int colour;

	// lock the surface
	result = IDirectDrawSurface7_Lock(win_d3d_background_surface, NULL, &surface_desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
	if (result != DD_OK)
		return 1;

	// the RGB pattern needs to be rotated
	if (win_d3d_effects_swapxy)
	{
		for (y = 0; y < surface_desc.dwHeight; y++)
		{
			int ry = win_d3d_effects_flipy ? surface_desc.dwHeight - 1 - y : y;
			UINT8 *pattern = &pattern_data[(ry % pattern_xsize) << 2];
			for (x = 0; x < surface_desc.dwWidth; x++)
			{
				int rx = win_d3d_effects_flipx ? surface_desc.dwWidth - 1 - x : x;
				colour  = pattern[((rx % pattern_ysize) * (pattern_xsize << 2)) + 0] << 16;
				colour |= pattern[((rx % pattern_ysize) * (pattern_xsize << 2)) + 1] <<  8;
				colour |= pattern[((rx % pattern_ysize) * (pattern_xsize << 2)) + 2] <<  0;

				win_ddrawsurf_plot_pixel(&surface_desc, x, y, colour);
			}
		}
	}
	// no rotation needed
	else
	{
		for (y = 0; y < surface_desc.dwHeight; y++)
		{
			int ry = win_d3d_effects_flipy ? surface_desc.dwHeight - 1 - y : y;
			UINT8 *pattern = &pattern_data[(ry % pattern_ysize) * pattern_xsize << 2];
			for (x = 0; x < surface_desc.dwWidth; x++)
			{
				int rx = win_d3d_effects_flipx ? surface_desc.dwWidth - 1 - x : x;
				colour  = pattern[((rx % pattern_xsize) << 2) + 0] << 16;
				colour |= pattern[((rx % pattern_xsize) << 2) + 1] <<  8;
				colour |= pattern[((rx % pattern_xsize) << 2) + 2] <<  0;

				win_ddrawsurf_plot_pixel(&surface_desc, x, y, colour);
			}
		}
	}

	result = IDirectDrawSurface7_Unlock(win_d3d_background_surface, NULL);

	return 0;

}



//============================================================
//	Plot a pixel on a DirectX7 surface (no bounds checking!)
//============================================================

static void win_ddrawsurf_plot_pixel(const LPDDSURFACEDESC2 surface_desc, int x, int y, int colour)
{
	UINT32 dest_colour = 0;
	int shift, mask;

	// first convert the 24-bit colour to the format of the surface

	// red
	for (shift = 0, mask = surface_desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(2).dwRBitMask; !((0x80000000 >> shift) & mask); shift++) { }
	dest_colour |= ((colour <<  8) >> shift) & mask;

	// green
	for (shift = 0, mask = surface_desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(3).dwGBitMask; !((0x80000000 >> shift) & mask); shift++) { }
	dest_colour |= ((colour << 16) >> shift) & mask;

	// blue
	for (shift = 0, mask = surface_desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(4).dwBBitMask; !((0x80000000 >> shift) & mask); shift++) { }
	dest_colour |= ((colour << 24) >> shift) & mask;

	// now write the colour value
	switch (surface_desc->DUMMYUNIONNAMEN(4).ddpfPixelFormat.DUMMYUNIONNAMEN(1).dwRGBBitCount >> 3)
	{
		// 16-bit
		case 2:
		{
			((UINT16 *)((char *)surface_desc->lpSurface + y * surface_desc->DUMMYUNIONNAMEN(1).lPitch))[x] = (UINT16)dest_colour;
			 break;
		}
		// 32-bit
		case 4:
		{
			((UINT32 *)((char *)surface_desc->lpSurface + y * surface_desc->DUMMYUNIONNAMEN(1).lPitch))[x] = (UINT32)dest_colour;
			break;
		}
	}
}
