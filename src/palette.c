/******************************************************************************

	palette.c

	Palette handling functions.

******************************************************************************/

#include "driver.h"
#include "state.h"
#include <math.h>

#define VERBOSE 0


/*-------------------------------------------------
	CONSTANTS
-------------------------------------------------*/

#define PEN_BRIGHTNESS_BITS		8
#define MAX_PEN_BRIGHTNESS		(4 << PEN_BRIGHTNESS_BITS)

enum
{
	PALETTIZED_16BIT,
	DIRECT_15BIT,
	DIRECT_32BIT
};


/*-------------------------------------------------
	GLOBAL VARIABLES
-------------------------------------------------*/

UINT32 direct_rgb_components[3];
UINT16 *palette_shadow_table;

data8_t *paletteram;
data8_t *paletteram_2;	/* use when palette RAM is split in two parts */
data16_t *paletteram16;
data16_t *paletteram16_2;
data32_t *paletteram32;



/*-------------------------------------------------
	LOCAL VARIABLES
-------------------------------------------------*/

static rgb_t *game_palette;			/* RGB palette as set by the driver */
static rgb_t *adjusted_palette;		/* actual RGB palette after brightness/gamma adjustments */
static UINT32 *dirty_palette;
static UINT16 *pen_brightness;

static UINT8 adjusted_palette_dirty;
static UINT8 debug_palette_dirty;

static UINT16 shadow_factor, highlight_factor;
static double global_brightness, global_brightness_adjust, global_gamma;

static UINT8 colormode;
static pen_t total_colors;
static pen_t total_colors_with_ui;

static UINT8 color_correct_table[(MAX_PEN_BRIGHTNESS * MAX_PEN_BRIGHTNESS) >> PEN_BRIGHTNESS_BITS];



/*-------------------------------------------------
	PROTOTYPES
-------------------------------------------------*/

static int palette_alloc(void);
static void palette_reset(void);
static void recompute_adjusted_palette(int brightness_or_gamma_changed);
static void internal_modify_pen(pen_t pen, rgb_t color, int pen_bright);



/*-------------------------------------------------
	rgb_to_direct15 - convert an RGB triplet to
	a 15-bit OSD-specified RGB value
-------------------------------------------------*/

INLINE UINT16 rgb_to_direct15(rgb_t rgb)
{
	return  (  RGB_RED(rgb) >> 3) * (direct_rgb_components[0] / 0x1f) +
			(RGB_GREEN(rgb) >> 3) * (direct_rgb_components[1] / 0x1f) +
			( RGB_BLUE(rgb) >> 3) * (direct_rgb_components[2] / 0x1f);
}



/*-------------------------------------------------
	rgb_to_direct32 - convert an RGB triplet to
	a 32-bit OSD-specified RGB value
-------------------------------------------------*/

INLINE UINT32 rgb_to_direct32(rgb_t rgb)
{
	return    RGB_RED(rgb) * (direct_rgb_components[0] / 0xff) +
			RGB_GREEN(rgb) * (direct_rgb_components[1] / 0xff) +
			 RGB_BLUE(rgb) * (direct_rgb_components[2] / 0xff);
}



/*-------------------------------------------------
	adjust_palette_entry - adjust a palette
	entry for brightness and gamma
-------------------------------------------------*/

INLINE rgb_t adjust_palette_entry(rgb_t entry, int pen_bright)
{
	int r = color_correct_table[(RGB_RED(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	int g = color_correct_table[(RGB_GREEN(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	int b = color_correct_table[(RGB_BLUE(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	return MAKE_RGB(r,g,b);
}



/*-------------------------------------------------
	mark_pen_dirty - mark a given pen index dirty
-------------------------------------------------*/

INLINE void mark_pen_dirty(int pen)
{
	dirty_palette[pen / 32] |= 1 << (pen % 32);
}



/*-------------------------------------------------
	palette_start - palette initialization that
	takes place before the display is created
-------------------------------------------------*/

int palette_start(void)
{
	/* init statics */
	adjusted_palette_dirty = 1;
	debug_palette_dirty = 1;

	shadow_factor = (int)(PALETTE_DEFAULT_SHADOW_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));
	highlight_factor = (int)(PALETTE_DEFAULT_HIGHLIGHT_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));
	global_brightness = (options.brightness > .001) ? options.brightness : 1.0;
	global_brightness_adjust = 1.0;
	global_gamma = (options.gamma > .001) ? options.gamma : 1.0;

	/* determine the color mode */
	if (Machine->color_depth == 15)
		colormode = DIRECT_15BIT;
	else if (Machine->color_depth == 32)
		colormode = DIRECT_32BIT;
	else
		colormode = PALETTIZED_16BIT;

	/* ensure that RGB direct video modes don't have a colortable */
	if ((Machine->drv->video_attributes & VIDEO_RGB_DIRECT) &&
			Machine->drv->color_table_len)
	{
		logerror("Error: VIDEO_RGB_DIRECT requires color_table_len to be 0.\n");
		return 1;
	}

	/* compute the total colors, including shadows and highlights */
	total_colors = Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		total_colors += Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		total_colors += Machine->drv->total_colors;
	total_colors_with_ui = total_colors;

	/* make sure we still fit in 16 bits */
	if (total_colors > 65536)
	{
		logerror("Error: palette has more than 65536 colors.\n");
		return 1;
	}

	/* allocate all the data structures */
	if (palette_alloc())
		return 1;

	/* set up save/restore of the palette */
	state_save_register_UINT32("palette", 0, "colors", game_palette, total_colors);
	state_save_register_UINT16("palette", 0, "brightness", pen_brightness, Machine->drv->total_colors);
	state_save_register_func_postload(palette_reset);

	return 0;
}



//* AAT041603
/*-------------------------------------------------
	palette_set_shadow_mode(mode)

		mode: 0 = use preset 0 (default shadow)
		      1 = use preset 1 (default highlight)
		      2 = use preset 2 *
		      3 = use preset 3 *

	* Preset 2 & 3 work independently under 32bpp,
	  supporting up to four different types of
	  shadows at one time. They mirror preset 1 & 2
	  in lower depth settings to maintain
	  compatibility.


	palette_set_shadow_factor32(factor)

		factor: 1.0(normal) to 0.0(darker)


	palette_set_highlight_factor32(factor)

		factor: 1.0(normal) to 2.0(brighter)


	palette_set_shadow_dRGB32(mode, dr, dg, db, noclip)

		mode:    0 to   3, which preset to configure

		  dr: -255 to 255,   red displacement
		  dg: -255 to 255, green displacement
		  db: -255 to 255,  blue displacement

		noclip: 0 = resultant RGB clipped at 0x00/0xff
		        1 = resultant RGB wraparound 0x00/0xff


	* Color shadows only work under 32bpp.
	  This function has no effect in lower color
	  depths where

		palette_set_shadow_factor32() or
		palette_set_highlight_factor32()

	  should be used instead.

	* 32-bit shadows are lossy. Even with zero RGB
	  the affected area will still look slightly
	  darkened. Drivers should ensure shadow pen
	  entries in gfx_drawmode_table are set to
	  DRAWMODE_NONE to avoid this side-effect.

-------------------------------------------------*/
#define MAX_SHADOW_PRESETS 4

static UINT32 *shadow_table_base[MAX_SHADOW_PRESETS];


static void internal_set_shadow_preset(int mode, double factor, int dr, int dg, int db, int noclip, int style, int init)
{
	static double oldfactor[MAX_SHADOW_PRESETS] = {0,0,0,0};
	static int oldRGB[MAX_SHADOW_PRESETS][3] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
	static int oldclip;

	UINT32 *table_ptr32;
	int i, r, g, b;

	if (mode < 0 || mode >= MAX_SHADOW_PRESETS) return;

	if ((table_ptr32 = shadow_table_base[mode]) == NULL) return;

	if (style)
	{
		if (factor < 0) factor = 0;

		if (!init && oldfactor[mode] == factor) return;

		oldfactor[mode] = factor;

		if (colormode != DIRECT_32BIT)
		{
			if (style == 1) palette_set_shadow_factor(factor); else
			if (style == 2) palette_set_highlight_factor(factor);
			return;
		}

		for (i=0; i<32768; i++)
		{
			r = (int)(factor * (i & 0x7c00));
			g = (int)(factor * (i & 0x03e0));
			b = (int)(factor * (i & 0x001f));

			if (r < 0x7c00) r &= 0x7c00; else r = 0x7c00;
			if (g < 0x03e0) g &= 0x03e0; else g = 0x03e0;
			if (b < 0x001f) b &= 0x001f; else b = 0x001f;

			table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
		}
	}
	else
	{
		if (colormode != DIRECT_32BIT) return;

		if (dr < -0xff) dr = -0xff; else if (dr > 0xff) dr = 0xff;
		if (dg < -0xff) dg = -0xff; else if (dg > 0xff) dg = 0xff;
		if (db < -0xff) db = -0xff; else if (db > 0xff) db = 0xff;
		dr >>= 3; dg >>= 3; db >>= 3;

		if (!init && oldclip==noclip && oldRGB[mode][0]==dr && oldRGB[mode][1]==dg && oldRGB[mode][2]==db) return;

		#ifdef MAME_DEBUG
			//usrintf_showmessage("shadow %d recalc %d %d %d %02x", mode, dr, dg, db, noclip);
		#endif

		oldclip = noclip;
		oldRGB[mode][0] = dr; oldRGB[mode][1] = dg; oldRGB[mode][2] = db;

		dr <<= 10; dg <<= 5;

		if (noclip)
		{
			for (i=0; i<32768; i++)
			{
				r = (i & 0x7c00) + dr;
				g = (i & 0x03e0) + dg;
				b = (i & 0x001f) + db;

				r &= 0x7c00;
				g &= 0x03e0;
				b &= 0x001f;

				table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
			}
		}
		else
		{
			for (i=0; i<32768; i++)
			{
				r = (i & 0x7c00) + dr;
				g = (i & 0x03e0) + dg;
				b = (i & 0x001f) + db;

				if (r < 0) r = 0; else if (r > 0x7c00) r = 0x7c00;
				if (g < 0) g = 0; else if (g > 0x03e0) g = 0x03e0;
				if (b < 0) b = 0; else if (b > 0x001f) b = 0x001f;

				table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
			}
		}
	}
}


void palette_set_shadow_mode(int mode)
{
	if (mode >= 0 && mode < MAX_SHADOW_PRESETS) palette_shadow_table = (UINT16*)shadow_table_base[mode];
}


void palette_set_shadow_factor32(double factor)
{
	internal_set_shadow_preset(0, factor, 0, 0, 0, 0, 1, 0);
}


void palette_set_highlight_factor32(double factor)
{
	internal_set_shadow_preset(1, factor, 0, 0, 0, 0, 2, 0);
}


void palette_set_shadow_dRGB32(int mode, int dr, int dg, int db, int noclip)
{
	internal_set_shadow_preset(mode, 0, dr, dg, db, noclip, 0, 0);
}



/*-------------------------------------------------
	palette_alloc - allocate memory for palette
	structures
-------------------------------------------------*/

static int palette_alloc(void)
{
	int max_total_colors = total_colors + 2;
	int i;

	/* allocate memory for the raw game palette */
	game_palette = auto_malloc(max_total_colors * sizeof(game_palette[0]));
	if (!game_palette)
		return 1;
	for (i = 0; i < max_total_colors; i++)
		game_palette[i] = MAKE_RGB((i & 1) * 0xff, ((i >> 1) & 1) * 0xff, ((i >> 2) & 1) * 0xff);

	/* allocate memory for the adjusted game palette */
	adjusted_palette = auto_malloc(max_total_colors * sizeof(adjusted_palette[0]));
	if (!adjusted_palette)
		return 1;
	for (i = 0; i < max_total_colors; i++)
		adjusted_palette[i] = game_palette[i];

	/* allocate memory for the dirty palette array */
	dirty_palette = auto_malloc((max_total_colors + 31) / 32 * sizeof(dirty_palette[0]));
	if (!dirty_palette)
		return 1;
	for (i = 0; i < max_total_colors; i++)
		mark_pen_dirty(i);

	/* allocate memory for the pen table */
	Machine->pens = auto_malloc(total_colors * sizeof(Machine->pens[0]));
	if (!Machine->pens)
		return 1;
	for (i = 0; i < total_colors; i++)
		Machine->pens[i] = i;

	/* allocate memory for the per-entry brightness table */
	pen_brightness = auto_malloc(Machine->drv->total_colors * sizeof(pen_brightness[0]));
	if (!pen_brightness)
		return 1;
	for (i = 0; i < Machine->drv->total_colors; i++)
		pen_brightness[i] = 1 << PEN_BRIGHTNESS_BITS;

	/* allocate memory for the colortables, if needed */
	if (Machine->drv->color_table_len)
	{
		/* first for the raw colortable */
		Machine->game_colortable = auto_malloc(Machine->drv->color_table_len * sizeof(Machine->game_colortable[0]));
		if (!Machine->game_colortable)
			return 1;
		for (i = 0; i < Machine->drv->color_table_len; i++)
			Machine->game_colortable[i] = i % total_colors;

		/* then for the remapped colortable */
		Machine->remapped_colortable = auto_malloc(Machine->drv->color_table_len * sizeof(Machine->remapped_colortable[0]));
		if (!Machine->remapped_colortable)
			return 1;
	}

	/* otherwise, keep the game_colortable NULL and point the remapped_colortable to the pens */
	else
	{
		Machine->game_colortable = NULL;
		Machine->remapped_colortable = Machine->pens;	/* straight 1:1 mapping from palette to colortable */
	}

	/* allocate memory for the debugger pens */
	Machine->debug_pens = auto_malloc(DEBUGGER_TOTAL_COLORS * sizeof(Machine->debug_pens[0]));
	if (!Machine->debug_pens)
		return 1;
	for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
		Machine->debug_pens[i] = i;

	/* allocate memory for the debugger colortable */
	Machine->debug_remapped_colortable = auto_malloc(2 * DEBUGGER_TOTAL_COLORS * DEBUGGER_TOTAL_COLORS * sizeof(Machine->debug_remapped_colortable[0]));
	if (!Machine->debug_remapped_colortable)
		return 1;
	for (i = 0; i < DEBUGGER_TOTAL_COLORS * DEBUGGER_TOTAL_COLORS; i++)
	{
		Machine->debug_remapped_colortable[2*i+0] = i / DEBUGGER_TOTAL_COLORS;
		Machine->debug_remapped_colortable[2*i+1] = i % DEBUGGER_TOTAL_COLORS;
	}

#if 0
	/* allocate the shadow lookup table for 16bpp modes */
	palette_shadow_table = NULL;
	if (colormode == PALETTIZED_16BIT)
	{
		/* we allocate a full 65536 entries table, to prevent memory corruption
		 * bugs should the tilemap contains pens >= total_colors
		 * (e.g. Machine->uifont->colortable[0] as returned by get_black_pen())
		 */
		palette_shadow_table = auto_malloc(65536 * sizeof(palette_shadow_table[0]));
		if (!palette_shadow_table)
			return 1;

		/* map entries up to the total_colors so they point to the next block of colors */
		for (i = 0; i < 65536; i++)
		{
			palette_shadow_table[i] = i;
			if ((Machine->drv->video_attributes & VIDEO_HAS_SHADOWS) && i < Machine->drv->total_colors)
				palette_shadow_table[i] += Machine->drv->total_colors;
		}
	}
#else
	{
		//* AAT041603
		UINT16 *table_ptr16;
		UINT32 *table_ptr32;
		int c = Machine->drv->total_colors;
		int cx2 = c << 1;

		for (i=0; i<MAX_SHADOW_PRESETS; i++) shadow_table_base[i] = NULL;
		palette_shadow_table = NULL;

		if (colormode != DIRECT_32BIT)
		{
			if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
			{
				if (!(table_ptr16 = auto_malloc(65536 * sizeof(UINT16)))) return 1;
				if (palette_shadow_table == NULL) palette_shadow_table = table_ptr16;
				shadow_table_base[0] = shadow_table_base[2] = (UINT32*)table_ptr16;

				for (i=0; i<c; i++) table_ptr16[i] = c + i;
				for (i=c; i<65536; i++) table_ptr16[i] = i;

				internal_set_shadow_preset(0, PALETTE_DEFAULT_SHADOW_FACTOR32, 0, 0, 0, 0, 1, 1);
			}

			if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
			{
				if (!(table_ptr16 = auto_malloc(65536 * sizeof(UINT16)))) return 1;
				if (palette_shadow_table == NULL) palette_shadow_table = table_ptr16;
				shadow_table_base[1] = shadow_table_base[3] = (UINT32*)table_ptr16;

				for (i=0; i<c; i++) table_ptr16[i] = cx2 + i;
				for (i=c; i<65536; i++) table_ptr16[i] = i;

				internal_set_shadow_preset(1, PALETTE_DEFAULT_HIGHLIGHT_FACTOR32, 0, 0, 0, 0, 2, 1);
			}
		}
		else
		{
			if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
			{
				if (!(table_ptr32 = auto_malloc(65536 * sizeof(UINT32)))) return 1;
				if (palette_shadow_table == NULL) palette_shadow_table = (UINT16*)table_ptr32;

				shadow_table_base[0] = table_ptr32;
				shadow_table_base[2] = table_ptr32 + 32768;

				internal_set_shadow_preset(0, PALETTE_DEFAULT_SHADOW_FACTOR32, 0, 0, 0, 0, 1, 1);
			}

			if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
			{
				if (!(table_ptr32 = auto_malloc(65536 * sizeof(UINT32)))) return 1;
				if (palette_shadow_table == NULL) palette_shadow_table = (UINT16*)table_ptr32;

				shadow_table_base[1] = table_ptr32;
				shadow_table_base[3] = table_ptr32 + 32768;

				internal_set_shadow_preset(1, PALETTE_DEFAULT_HIGHLIGHT_FACTOR32, 0, 0, 0, 0, 2, 1);
			}
		}
	}
#endif

	return 0;
}



/*-------------------------------------------------
	palette_init - palette initialization that
	takes place after the display is created
-------------------------------------------------*/

int palette_init(void)
{
	int i;

	/* recompute the default palette and initalize the color correction table */
	recompute_adjusted_palette(1);

	/* now let the driver modify the initial palette and colortable */
	if (Machine->drv->init_palette)
		(*Machine->drv->init_palette)(Machine->game_colortable, memory_region(REGION_PROMS));

	/* switch off the color mode */
	switch (colormode)
	{
		/* 16-bit paletteized case */
		case PALETTIZED_16BIT:
		{
			/* refresh the palette to support shadows in static palette games */
			for (i = 0; i < Machine->drv->total_colors; i++)
				palette_set_color(i, RGB_RED(game_palette[i]), RGB_GREEN(game_palette[i]), RGB_BLUE(game_palette[i]));

			/* map the UI pens */
			if (total_colors_with_ui <= 65534)
			{
				game_palette[total_colors + 0] = adjusted_palette[total_colors + 0] = MAKE_RGB(0x00,0x00,0x00);
				game_palette[total_colors + 1] = adjusted_palette[total_colors + 1] = MAKE_RGB(0xff,0xff,0xff);
				Machine->uifont->colortable[0] = Machine->uifont->colortable[3] = total_colors_with_ui++;
				Machine->uifont->colortable[1] = Machine->uifont->colortable[2] = total_colors_with_ui++;
			}
			else
			{
				game_palette[0] = adjusted_palette[0] = MAKE_RGB(0x00,0x00,0x00);
				game_palette[65535] = adjusted_palette[65535] = MAKE_RGB(0xff,0xff,0xff);
				Machine->uifont->colortable[0] = Machine->uifont->colortable[3] = 0;
				Machine->uifont->colortable[1] = Machine->uifont->colortable[2] = 65535;
			}
			break;
		}

		/* 15-bit direct case */
		case DIRECT_15BIT:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < total_colors; i++)
				Machine->pens[i] = rgb_to_direct15(game_palette[i]);

			/* map the UI pens */
			Machine->uifont->colortable[0] = Machine->uifont->colortable[3] = rgb_to_direct15(MAKE_RGB(0x00,0x00,0x00));
			Machine->uifont->colortable[1] = Machine->uifont->colortable[2] = rgb_to_direct15(MAKE_RGB(0xff,0xff,0xff));
			break;
		}

		case DIRECT_32BIT:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < total_colors; i++)
				Machine->pens[i] = rgb_to_direct32(game_palette[i]);

			/* map the UI pens */
			Machine->uifont->colortable[0] = Machine->uifont->colortable[3] = rgb_to_direct32(MAKE_RGB(0x00,0x00,0x00));
			Machine->uifont->colortable[1] = Machine->uifont->colortable[2] = rgb_to_direct32(MAKE_RGB(0xff,0xff,0xff));
			break;
		}
	}

	/* now compute the remapped_colortable */
	for (i = 0; i < Machine->drv->color_table_len; i++)
	{
		pen_t color = Machine->game_colortable[i];

		/* check for invalid colors set by Machine->drv->init_palette */
		if (color < total_colors)
			Machine->remapped_colortable[i] = Machine->pens[color];
		else
			usrintf_showmessage("colortable[%d] (=%d) out of range (total_colors = %d)",
					i,color,total_colors);
	}

	/* all done */
	return 0;
}



/*-------------------------------------------------
	palette_get_total_colors_with_ui - returns
	the total number of palette entries including
	UI
-------------------------------------------------*/

int palette_get_total_colors_with_ui(void)
{
	int result = Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		result += Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		result += Machine->drv->total_colors;
	if (result <= 65534)
		result += 2;
	return result;
}



/*-------------------------------------------------
	palette_update_display - update the display
	state with our latest info
-------------------------------------------------*/

void palette_update_display(struct mame_display *display)
{
	/* palettized case: point to the palette info */
	if (colormode == PALETTIZED_16BIT)
	{
		display->game_palette = adjusted_palette;
		display->game_palette_entries = total_colors_with_ui;
		display->game_palette_dirty = dirty_palette;

		if (adjusted_palette_dirty)
			display->changed_flags |= GAME_PALETTE_CHANGED;
	}

	/* direct case: no palette mucking */
	else
	{
		display->game_palette = NULL;
		display->game_palette_entries = 0;
		display->game_palette_dirty = NULL;
	}

	/* debugger always has a palette */
#ifdef MAME_DEBUG
	display->debug_palette = debugger_palette;
	display->debug_palette_entries = DEBUGGER_TOTAL_COLORS;
#endif

	/* update the dirty state */
	if (debug_palette_dirty)
		display->changed_flags |= DEBUG_PALETTE_CHANGED;

	/* clear the dirty flags */
	adjusted_palette_dirty = 0;
	debug_palette_dirty = 0;
}



/*-------------------------------------------------
	internal_modify_single_pen - change a single
	pen and recompute its adjusted RGB value
-------------------------------------------------*/

static void internal_modify_single_pen(pen_t pen, rgb_t color, int pen_bright)
{
	rgb_t adjusted_color;

	/* skip if out of bounds or not ready */
	if (pen >= total_colors)
		return;

	/* update the raw palette */
	game_palette[pen] = color;

	/* now update the adjusted color if it's different */
	adjusted_color = adjust_palette_entry(color, pen_bright);
	if (adjusted_color != adjusted_palette[pen])
	{
		/* change the adjusted palette entry */
		adjusted_palette[pen] = adjusted_color;
		adjusted_palette_dirty = 1;

		/* update the pen value or mark the palette dirty */
		switch (colormode)
		{
			/* 16-bit palettized: just mark it dirty for later */
			case PALETTIZED_16BIT:
				mark_pen_dirty(pen);
				break;

			/* 15/32-bit direct: update the Machine->pens array */
			case DIRECT_15BIT:
				Machine->pens[pen] = rgb_to_direct15(adjusted_color);
				break;

			case DIRECT_32BIT:
				Machine->pens[pen] = rgb_to_direct32(adjusted_color);
				break;
		}
	}
}



/*-------------------------------------------------
	internal_modify_pen - change a pen along with
	its corresponding shadow/highlight
-------------------------------------------------*/

static void internal_modify_pen(pen_t pen, rgb_t color, int pen_bright)
{
	/* first modify the base pen */
	internal_modify_single_pen(pen, color, pen_bright);

	/* see if we need to handle shadow/highlight */
	if (pen < Machine->drv->total_colors)
	{
		/* check for shadows */
		if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		{
			pen += Machine->drv->total_colors;
			internal_modify_single_pen(pen, color, (pen_bright * shadow_factor) >> PEN_BRIGHTNESS_BITS);
		}

		/* check for highlights */
		if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		{
			pen += Machine->drv->total_colors;
			internal_modify_single_pen(pen, color, (pen_bright * highlight_factor) >> PEN_BRIGHTNESS_BITS);
		}
	}
}



/*-------------------------------------------------
	recompute_adjusted_palette - recompute the
	entire palette after some major event
-------------------------------------------------*/

static void recompute_adjusted_palette(int brightness_or_gamma_changed)
{
	int i;

	/* regenerate the color correction table if needed */
	if (brightness_or_gamma_changed)
		for (i = 0; i < sizeof(color_correct_table); i++)
		{
			int value = (int)(255.0 * (global_brightness * global_brightness_adjust) * pow((double)i * (1.0 / 255.0), 1.0 / global_gamma) + 0.5);
			color_correct_table[i] = (value < 0) ? 0 : (value > 255) ? 255 : value;
		}

	/* now update all the palette entries */
	for (i = 0; i < Machine->drv->total_colors; i++)
		internal_modify_pen(i, game_palette[i], pen_brightness[i]);
}



/*-------------------------------------------------
	palette_reset - called after restore to
	actually update the palette
-------------------------------------------------*/

static void palette_reset(void)
{
	/* recompute everything */
	recompute_adjusted_palette(0);
}



/*-------------------------------------------------
	palette_set_color - set a single palette
	entry
-------------------------------------------------*/

void palette_set_color(pen_t pen, UINT8 r, UINT8 g, UINT8 b)
{
	/* make sure we're in range */
	if (pen >= total_colors)
	{
		logerror("error: palette_set_color() called with color %d, but only %d allocated.\n", pen, total_colors);
		return;
	}

	/* set the pen value */
	internal_modify_pen(pen, MAKE_RGB(r, g, b), pen_brightness[pen]);
}

/* handy wrapper for palette_set_color */
void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count)
{
        while(color_count--)
        {
                palette_set_color(color_base++, colors[0], colors[1], colors[2]);
                colors += 3;
        }
}

/*-------------------------------------------------
	palette_get_color - return a single palette
	entry
-------------------------------------------------*/

void palette_get_color(pen_t pen, UINT8 *r, UINT8 *g, UINT8 *b)
{
	/* special case the black pen */
	if (pen == get_black_pen())
		*r = *g = *b = 0;

	/* record the result from the game palette */
	else if (pen < total_colors)
	{
		*r = RGB_RED(game_palette[pen]);
		*g = RGB_GREEN(game_palette[pen]);
		*b = RGB_BLUE(game_palette[pen]);
	}
	else
		usrintf_showmessage("palette_get_color() out of range");
}





/*-------------------------------------------------
	palette_set_brightness - set the per-pen
	brightness factor
-------------------------------------------------*/

void palette_set_brightness(pen_t pen, double bright)
{
	/* compute the integral brightness value */
	int brightval = (int)(bright * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (brightval > MAX_PEN_BRIGHTNESS)
		brightval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the array and the adjusted palette */
	if (pen_brightness[pen] != brightval)
	{
		pen_brightness[pen] = brightval;
		internal_modify_pen(pen, game_palette[pen], brightval);
	}
}



/*-------------------------------------------------
	palette_set_shadow_factor - set the global
	shadow brightness factor
-------------------------------------------------*/

void palette_set_shadow_factor(double factor)
{
	/* compute the integral shadow factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (shadow_factor != factorval)
	{
		shadow_factor = factorval;
		recompute_adjusted_palette(0);
	}
}



/*-------------------------------------------------
	palette_set_highlight_factor - set the global
	highlight brightness factor
-------------------------------------------------*/

void palette_set_highlight_factor(double factor)
{
	/* compute the integral highlight factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (highlight_factor != factorval)
	{
		highlight_factor = factorval;
		recompute_adjusted_palette(0);
	}
}



/*-------------------------------------------------
	palette_set_global_gamma - set the global
	gamma factor
-------------------------------------------------*/

void palette_set_global_gamma(double _gamma)
{
	/* if the gamma changed, recompute */
	if (global_gamma != _gamma)
	{
		global_gamma = _gamma;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
	palette_get_global_gamma - return the global
	gamma factor
-------------------------------------------------*/

double palette_get_global_gamma(void)
{
	return global_gamma;
}



/*-------------------------------------------------
	palette_set_global_brightness - set the global
	brightness factor
-------------------------------------------------*/

void palette_set_global_brightness(double brightness)
{
	/* if the gamma changed, recompute */
	if (global_brightness != brightness)
	{
		global_brightness = brightness;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
	palette_set_global_brightness_adjust - set
	the global brightness adjustment factor
-------------------------------------------------*/

void palette_set_global_brightness_adjust(double adjustment)
{
	/* if the gamma changed, recompute */
	if (global_brightness_adjust != adjustment)
	{
		global_brightness_adjust = adjustment;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
	palette_get_global_brightness - return the global
	brightness factor
-------------------------------------------------*/

double palette_get_global_brightness(void)
{
	return global_brightness;
}



/*-------------------------------------------------
	get_black_pen - use this if you need to
	fillbitmap() the background with black
-------------------------------------------------*/

pen_t get_black_pen(void)
{
	return Machine->uifont->colortable[0];
}




/******************************************************************************

 Commonly used palette RAM handling functions

******************************************************************************/

READ_HANDLER( paletteram_r )
{
	return paletteram[offset];
}

READ_HANDLER( paletteram_2_r )
{
	return paletteram_2[offset];
}

READ16_HANDLER( paletteram16_word_r )
{
	return paletteram16[offset];
}

READ16_HANDLER( paletteram16_2_word_r )
{
	return paletteram16_2[offset];
}

READ32_HANDLER( paletteram32_r )
{
	return paletteram32[offset];
}

WRITE_HANDLER( paletteram_RRRGGGBB_w )
{
	int r,g,b;
	int bit0,bit1,bit2;


	paletteram[offset] = data;

	/* red component */
	bit0 = (data >> 5) & 0x01;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 2) & 0x01;
	bit1 = (data >> 3) & 0x01;
	bit2 = (data >> 4) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (data >> 0) & 0x01;
	bit2 = (data >> 1) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color(offset,r,g,b);
}

WRITE_HANDLER( paletteram_BBBGGGRR_w )
{
	int r,g,b;
	int bit0,bit1,bit2;

	paletteram[offset] = data;

	/* blue component */
	bit0 = (data >> 5) & 0x01;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 2) & 0x01;
	bit1 = (data >> 3) & 0x01;
	bit2 = (data >> 4) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	r = 0x55 * bit0 + 0xaa * bit1;

	palette_set_color(offset,r,g,b);
}

WRITE_HANDLER( paletteram_BBGGGRRR_w )
{
	int r,g,b;
	int bit0,bit1,bit2;


	paletteram[offset] = data;

	/* red component */
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color(offset,r,g,b);
}


WRITE_HANDLER( paletteram_IIBBGGRR_w )
{
	int r,g,b,i;


	paletteram[offset] = data;

	i = (data >> 6) & 0x03;
	/* red component */
	r = (data << 2) & 0x0c;
	if (r) r |= i;
	r *= 0x11;
	/* green component */
	g = (data >> 0) & 0x0c;
	if (g) g |= i;
	g *= 0x11;
	/* blue component */
	b = (data >> 2) & 0x0c;
	if (b) b |= i;
	b *= 0x11;

	palette_set_color(offset,r,g,b);
}


WRITE_HANDLER( paletteram_BBGGRRII_w )
{
	int r,g,b,i;


	paletteram[offset] = data;

	i = (data >> 0) & 0x03;
	/* red component */
	r = (((data >> 0) & 0x0c) | i) * 0x11;
	/* green component */
	g = (((data >> 2) & 0x0c) | i) * 0x11;
	/* blue component */
	b = (((data >> 4) & 0x0c) | i) * 0x11;

	palette_set_color(offset,r,g,b);
}


INLINE void changecolor_xxxxBBBBGGGGRRRR(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 0) & 0x0f;
	g = (data >> 4) & 0x0f;
	b = (data >> 8) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBGGGGRRRR(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_swap_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBGGGGRRRR(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split1_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBGGGGRRRR(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_xxxxBBBBGGGGRRRR(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE16_HANDLER( paletteram16_xxxxBBBBGGGGRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xxxxBBBBGGGGRRRR(offset,paletteram16[offset]);
}


INLINE void changecolor_xxxxBBBBRRRRGGGG(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 4) & 0x0f;
	g = (data >> 0) & 0x0f;
	b = (data >> 8) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBRRRRGGGG(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_swap_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBRRRRGGGG(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split1_w )
{
	paletteram[offset] = data;
	changecolor_xxxxBBBBRRRRGGGG(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_xxxxBBBBRRRRGGGG(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE16_HANDLER( paletteram16_xxxxBBBBRRRRGGGG_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xxxxBBBBRRRRGGGG(offset,paletteram16[offset]);
}


INLINE void changecolor_xxxxRRRRBBBBGGGG(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 8) & 0x0f;
	g = (data >> 0) & 0x0f;
	b = (data >> 4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split1_w )
{
	paletteram[offset] = data;
	changecolor_xxxxRRRRBBBBGGGG(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_xxxxRRRRBBBBGGGG(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}


INLINE void changecolor_xxxxRRRRGGGGBBBB(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 8) & 0x0f;
	g = (data >> 4) & 0x0f;
	b = (data >> 0) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xxxxRRRRGGGGBBBB_w )
{
	paletteram[offset] = data;
	changecolor_xxxxRRRRGGGGBBBB(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE_HANDLER( paletteram_xxxxRRRRGGGGBBBB_swap_w )
{
	paletteram[offset] = data;
	changecolor_xxxxRRRRGGGGBBBB(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE16_HANDLER( paletteram16_xxxxRRRRGGGGBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xxxxRRRRGGGGBBBB(offset,paletteram16[offset]);
}


INLINE void changecolor_RRRRGGGGBBBBxxxx(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_swap_w )
{
	paletteram[offset] = data;
	changecolor_RRRRGGGGBBBBxxxx(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split1_w )
{
	paletteram[offset] = data;
	changecolor_RRRRGGGGBBBBxxxx(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_RRRRGGGGBBBBxxxx(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBxxxx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_RRRRGGGGBBBBxxxx(offset,paletteram16[offset]);
}


INLINE void changecolor_BBBBGGGGRRRRxxxx(pen_t color,int data)
{
	int r,g,b;


	r = (data >>  4) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >> 12) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_swap_w )
{
	paletteram[offset] = data;
	changecolor_BBBBGGGGRRRRxxxx(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_split1_w )
{
	paletteram[offset] = data;
	changecolor_BBBBGGGGRRRRxxxx(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_BBBBGGGGRRRRxxxx_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_BBBBGGGGRRRRxxxx(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE16_HANDLER( paletteram16_BBBBGGGGRRRRxxxx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_BBBBGGGGRRRRxxxx(offset,paletteram16[offset]);
}


INLINE void changecolor_xBBBBBGGGGGRRRRR(pen_t color,int data)
{
	int r,g,b;


	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_w )
{
	paletteram[offset] = data;
	changecolor_xBBBBBGGGGGRRRRR(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_swap_w )
{
	paletteram[offset] = data;
	changecolor_xBBBBBGGGGGRRRRR(offset / 2,paletteram[offset | 1] | (paletteram[offset & ~1] << 8));
}

WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split1_w )
{
	paletteram[offset] = data;
	changecolor_xBBBBBGGGGGRRRRR(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split2_w )
{
	paletteram_2[offset] = data;
	changecolor_xBBBBBGGGGGRRRRR(offset,paletteram[offset] | (paletteram_2[offset] << 8));
}

WRITE16_HANDLER( paletteram16_xBBBBBGGGGGRRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xBBBBBGGGGGRRRRR(offset,paletteram16[offset]);
}


INLINE void changecolor_xRRRRRGGGGGBBBBB(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 10) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >>  0) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_xRRRRRGGGGGBBBBB_w )
{
	paletteram[offset] = data;
	changecolor_xRRRRRGGGGGBBBBB(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE16_HANDLER( paletteram16_xRRRRRGGGGGBBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xRRRRRGGGGGBBBBB(offset,paletteram16[offset]);
}


INLINE void changecolor_xGGGGGRRRRRBBBBB(pen_t color,int data)
{
	int r,g,b;


	r = (data >>  5) & 0x1f;
	g = (data >> 10) & 0x1f;
	b = (data >>  0) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE16_HANDLER( paletteram16_xGGGGGRRRRRBBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xGGGGGRRRRRBBBBB(offset,paletteram16[offset]);
}


INLINE void changecolor_xGGGGGBBBBBRRRRR(pen_t color,int data)
{
	int r,g,b;


	r = (data >>  0) & 0x1f;
	g = (data >> 10) & 0x1f;
	b = (data >>  5) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE16_HANDLER( paletteram16_xGGGGGBBBBBRRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_xGGGGGBBBBBRRRRR(offset,paletteram16[offset]);
}


INLINE void changecolor_RRRRRGGGGGBBBBBx(pen_t color,int data)
{
	int r,g,b;


	r = (data >> 11) & 0x1f;
	g = (data >>  6) & 0x1f;
	b = (data >>  1) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE_HANDLER( paletteram_RRRRRGGGGGBBBBBx_w )
{
	paletteram[offset] = data;
	changecolor_RRRRRGGGGGBBBBBx(offset / 2,paletteram[offset & ~1] | (paletteram[offset | 1] << 8));
}

WRITE16_HANDLER( paletteram16_RRRRRGGGGGBBBBBx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_RRRRRGGGGGBBBBBx(offset,paletteram16[offset]);
}


INLINE void changecolor_IIIIRRRRGGGGBBBB(pen_t color,int data)
{
	int i,r,g,b;


	static const int ztable[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };

	i = ztable[(data >> 12) & 15];
	r = ((data >> 8) & 15) * i;
	g = ((data >> 4) & 15) * i;
	b = ((data >> 0) & 15) * i;

	palette_set_color(color,r,g,b);

	if (!(Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN))
		usrintf_showmessage("driver should use VIDEO_NEEDS_6BITS_PER_GUN flag");
}

WRITE16_HANDLER( paletteram16_IIIIRRRRGGGGBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_IIIIRRRRGGGGBBBB(offset,paletteram16[offset]);
}


INLINE void changecolor_RRRRGGGGBBBBIIII(pen_t color,int data)
{
	int i,r,g,b;


	static const int ztable[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };

	i = ztable[(data >> 0) & 15];
	r = ((data >> 12) & 15) * i;
	g = ((data >>  8) & 15) * i;
	b = ((data >>  4) & 15) * i;

	palette_set_color(color,r,g,b);

	if (!(Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN))
		usrintf_showmessage("driver should use VIDEO_NEEDS_6BITS_PER_GUN flag");
}

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBIIII_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_RRRRGGGGBBBBIIII(offset,paletteram16[offset]);
}


WRITE16_HANDLER( paletteram16_xrgb_word_w )
{
	int r, g, b;
	data16_t data0, data1;

	COMBINE_DATA(paletteram16 + offset);

	offset &= ~1;

	data0 = paletteram16[offset];
	data1 = paletteram16[offset + 1];

	r = data0 & 0xff;
	g = data1 >> 8;
	b = data1 & 0xff;

	palette_set_color(offset>>1, r, g, b);

	if (!(Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN))
		usrintf_showmessage("driver should use VIDEO_NEEDS_6BITS_PER_GUN flag");
}


INLINE void changecolor_RRRRGGGGBBBBRGBx(pen_t color,int data)
{
	int r,g,b;

	r = ((data >> 11) & 0x1e) | ((data>>3) & 0x01);
	g = ((data >>  7) & 0x1e) | ((data>>2) & 0x01);
	b = ((data >>  3) & 0x1e) | ((data>>1) & 0x01);
	r = (r<<3) | (r>>2);
	g = (g<<3) | (g>>2);
	b = (b<<3) | (b>>2);

	palette_set_color(color,r,g,b);
}

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBRGBx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_RRRRGGGGBBBBRGBx(offset,paletteram16[offset]);
}



/******************************************************************************

 Commonly used color PROM handling functions

******************************************************************************/


/***************************************************************************

	Standard black and white palette.
	Color 0 is pure black, color 1 is pure white.

***************************************************************************/

PALETTE_INIT( black_and_white )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
}


/***************************************************************************

  This assumes the commonly used resistor values:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( RRRR_GGGG_BBBB )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
}
