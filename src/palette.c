#include "driver.h"
#include "state.h"

#define VERBOSE 0


static UINT8 *game_palette;		/* RGB palette as set by the driver */
static UINT8 *actual_palette;	/* actual RGB palette after brightness adjustments */
static double *brightness;


static int colormode;
enum
{
	PALETTIZED_16BIT,
	DIRECT_15BIT,
	DIRECT_32BIT
};
static int total_colors;
static double shadow_factor,highlight_factor;
static int palette_initialized;

UINT32 direct_rgb_components[3];



UINT16 *palette_shadow_table;


static void palette_reset_32_direct(void);
static void palette_reset_15_direct(void);
static void palette_reset_16_palettized(void);



int palette_start(void)
{
	int i;

	if ((Machine->drv->video_attributes & VIDEO_RGB_DIRECT) &&
			Machine->drv->color_table_len)
	{
		logerror("Error: VIDEO_RGB_DIRECT requires color_table_len to be 0.\n");
		return 1;
	}

	total_colors = Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		total_colors += Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		total_colors += Machine->drv->total_colors;
	if (total_colors > 65536)
	{
		logerror("Error: palette has more than 65536 colors.\n");
		return 1;
	}
	shadow_factor = PALETTE_DEFAULT_SHADOW_FACTOR;
	highlight_factor = PALETTE_DEFAULT_HIGHLIGHT_FACTOR;

	game_palette = malloc(3 * total_colors);
	actual_palette = malloc(3 * total_colors);
	brightness = malloc(sizeof(*brightness) * Machine->drv->total_colors);

	if (Machine->color_depth == 15)
		colormode = DIRECT_15BIT;
	else if (Machine->color_depth == 32)
		colormode = DIRECT_32BIT;
	else
		colormode = PALETTIZED_16BIT;

	Machine->pens = malloc(total_colors * sizeof(*Machine->pens));
	Machine->palette = malloc(total_colors * sizeof(*Machine->palette));
	Machine->debug_pens = malloc(DEBUGGER_TOTAL_COLORS * sizeof(*Machine->debug_pens));

	if (Machine->drv->color_table_len)
	{
		Machine->game_colortable = malloc(Machine->drv->color_table_len * sizeof(*Machine->game_colortable));
		Machine->remapped_colortable = malloc(Machine->drv->color_table_len * sizeof(*Machine->remapped_colortable));
	}
	else
	{
		Machine->game_colortable = 0;
		Machine->remapped_colortable = Machine->pens;	/* straight 1:1 mapping from palette to colortable */
	}
	Machine->debug_remapped_colortable = malloc(2*DEBUGGER_TOTAL_COLORS*DEBUGGER_TOTAL_COLORS * sizeof(*Machine->debug_remapped_colortable));

	if (colormode == PALETTIZED_16BIT)
	{
		/* we allocate a full 65536 entries table, to prevent memory corruption
		 * bugs should the tilemap contains pens >= total_colors
		 * (e.g. Machine->uifont->colortable[0] as returned by get_black_pen())
		 */
		palette_shadow_table = malloc(65536 * sizeof(*palette_shadow_table));
		if (palette_shadow_table == 0)
		{
			palette_stop();
			return 1;
		}
		for (i = 0;i < 65536;i++)
		{
			palette_shadow_table[i] = i;
			if ((Machine->drv->video_attributes & VIDEO_HAS_SHADOWS) && i < Machine->drv->total_colors)
				palette_shadow_table[i] += Machine->drv->total_colors;
		}
	}
	else
		palette_shadow_table = NULL;

	if ((Machine->drv->color_table_len && (Machine->game_colortable == 0 || Machine->remapped_colortable == 0))
			|| game_palette == 0 || actual_palette == 0 || brightness == 0
			|| Machine->pens == 0 || Machine->debug_pens == 0 || Machine->debug_remapped_colortable == 0)
	{
		palette_stop();
		return 1;
	}

	for (i = 0;i < Machine->drv->total_colors;i++)
		brightness[i] = 1.0;


	state_save_register_UINT8("palette", 0, "colors", game_palette, total_colors*3);
	state_save_register_UINT8("palette", 0, "actual_colors", actual_palette, total_colors*3);
	state_save_register_double("palette", 0, "brightness", brightness, Machine->drv->total_colors);
	switch (colormode)
	{
		case PALETTIZED_16BIT:
			state_save_register_func_postload(palette_reset_16_palettized);
			break;
		case DIRECT_15BIT:
			state_save_register_func_postload(palette_reset_15_direct);
			break;
		case DIRECT_32BIT:
			state_save_register_func_postload(palette_reset_32_direct);
			break;
	}

	return 0;
}

void palette_stop(void)
{
	free(game_palette);
	game_palette = 0;
	free(actual_palette);
	actual_palette = 0;
	free(brightness);
	brightness = 0;
	if (Machine->game_colortable)
	{
		free(Machine->game_colortable);
		Machine->game_colortable = 0;
		/* remapped_colortable is malloc()ed only when game_colortable is, */
		/* otherwise it just points to Machine->pens */
		free(Machine->remapped_colortable);
	}
	Machine->remapped_colortable = 0;
	free(Machine->debug_remapped_colortable);
	Machine->debug_remapped_colortable = 0;
	free(Machine->pens);
	Machine->pens = 0;
	free(Machine->debug_pens);
	Machine->debug_pens = 0;
	free(palette_shadow_table);
	palette_shadow_table = 0;

	palette_initialized = 0;
}



int palette_init(void)
{
	int i;
	UINT8 *debug_palette;
	pen_t *debug_pens;

#ifdef MAME_DEBUG
	if (mame_debug)
	{
		debug_palette = debugger_palette;
		debug_pens = Machine->debug_pens;
	}
	else
#endif
	{
		debug_palette = NULL;
		debug_pens = NULL;
	}

	/* We initialize the palette and colortable to some default values so that */
	/* drivers which dynamically change the palette don't need an init_palette() */
	/* function (provided the default color table fits their needs). */

	for (i = 0;i < total_colors;i++)
	{
		game_palette[3*i + 0] = actual_palette[3*i + 0] = ((i & 1) >> 0) * 0xff;
		game_palette[3*i + 1] = actual_palette[3*i + 1] = ((i & 2) >> 1) * 0xff;
		game_palette[3*i + 2] = actual_palette[3*i + 2] = ((i & 4) >> 2) * 0xff;
	}

	/* Preload the colortable with a default setting, following the same */
	/* order of the palette. The driver can overwrite this in */
	/* init_palette() */
	for (i = 0;i < Machine->drv->color_table_len;i++)
		Machine->game_colortable[i] = i % total_colors;

	/* now the driver can modify the default values if it wants to. */
	if (Machine->drv->init_palette)
		(*Machine->drv->init_palette)(game_palette,Machine->game_colortable,memory_region(REGION_PROMS));


	switch (colormode)
	{
		case PALETTIZED_16BIT:
		{
			if (osd_allocate_colors(total_colors,game_palette,NULL,debug_palette,debug_pens))
				return 1;

			for (i = 0;i < total_colors;i++)
				Machine->pens[i] = i;

			/* refresh the palette to support shadows in PROM games */
			for (i = 0;i < Machine->drv->total_colors;i++)
				palette_set_color(i,game_palette[3*i + 0],game_palette[3*i + 1],game_palette[3*i + 2]);
		}
		break;

		case DIRECT_15BIT:
		{
			const UINT8 rgbpalette[3*3] = { 0xff,0x00,0x00, 0x00,0xff,0x00, 0x00,0x00,0xff };

			if (osd_allocate_colors(3,rgbpalette,direct_rgb_components,debug_palette,debug_pens))
				return 1;

			for (i = 0;i < total_colors;i++)
				Machine->pens[i] =
						(game_palette[3*i + 0] >> 3) * (direct_rgb_components[0] / 0x1f) +
						(game_palette[3*i + 1] >> 3) * (direct_rgb_components[1] / 0x1f) +
						(game_palette[3*i + 2] >> 3) * (direct_rgb_components[2] / 0x1f);

			break;
		}

		case DIRECT_32BIT:
		{
			const UINT8 rgbpalette[3*3] = { 0xff,0x00,0x00, 0x00,0xff,0x00, 0x00,0x00,0xff };

			if (osd_allocate_colors(3,rgbpalette,direct_rgb_components,debug_palette,debug_pens))
				return 1;

			for (i = 0;i < total_colors;i++)
				Machine->pens[i] =
						game_palette[3*i + 0] * (direct_rgb_components[0] / 0xff) +
						game_palette[3*i + 1] * (direct_rgb_components[1] / 0xff) +
						game_palette[3*i + 2] * (direct_rgb_components[2] / 0xff);

			break;
		}
	}

	for (i = 0;i < Machine->drv->color_table_len;i++)
	{
		pen_t color = Machine->game_colortable[i];

		/* check for invalid colors set by Machine->drv->init_palette */
		if (color < total_colors)
			Machine->remapped_colortable[i] = Machine->pens[color];
		else
			usrintf_showmessage("colortable[%d] (=%d) out of range (total_colors = %d)",
					i,color,total_colors);
	}

	for (i = 0;i < DEBUGGER_TOTAL_COLORS*DEBUGGER_TOTAL_COLORS;i++)
	{
		Machine->debug_remapped_colortable[2*i+0] = Machine->debug_pens[i / DEBUGGER_TOTAL_COLORS];
		Machine->debug_remapped_colortable[2*i+1] = Machine->debug_pens[i % DEBUGGER_TOTAL_COLORS];
	}

	palette_initialized = 1;

	return 0;
}



INLINE void palette_set_color_15_direct(pen_t color,UINT8 red,UINT8 green,UINT8 blue)
{
	if (	actual_palette[3*color + 0] == red &&
			actual_palette[3*color + 1] == green &&
			actual_palette[3*color + 2] == blue)
		return;
	actual_palette[3*color + 0] = red;
	actual_palette[3*color + 1] = green;
	actual_palette[3*color + 2] = blue;
	Machine->pens[color] =
			(red   >> 3) * (direct_rgb_components[0] / 0x1f) +
			(green >> 3) * (direct_rgb_components[1] / 0x1f) +
			(blue  >> 3) * (direct_rgb_components[2] / 0x1f);
}

static void palette_reset_15_direct(void)
{
	pen_t color;
	for(color = 0; color < total_colors; color++)
		Machine->pens[color] =
				(game_palette[3*color + 0]>>3) * (direct_rgb_components[0] / 0x1f) +
				(game_palette[3*color + 1]>>3) * (direct_rgb_components[1] / 0x1f) +
				(game_palette[3*color + 2]>>3) * (direct_rgb_components[2] / 0x1f);
}

INLINE void palette_set_color_32_direct(pen_t color,UINT8 red,UINT8 green,UINT8 blue)
{
	if (	actual_palette[3*color + 0] == red &&
			actual_palette[3*color + 1] == green &&
			actual_palette[3*color + 2] == blue)
		return;
	actual_palette[3*color + 0] = red;
	actual_palette[3*color + 1] = green;
	actual_palette[3*color + 2] = blue;
	Machine->pens[color] =
			red   * (direct_rgb_components[0] / 0xff) +
			green * (direct_rgb_components[1] / 0xff) +
			blue  * (direct_rgb_components[2] / 0xff);
}

static void palette_reset_32_direct(void)
{
	pen_t color;
	for(color = 0; color < total_colors; color++)
		Machine->pens[color] =
				game_palette[3*color + 0] * (direct_rgb_components[0] / 0xff) +
				game_palette[3*color + 1] * (direct_rgb_components[1] / 0xff) +
				game_palette[3*color + 2] * (direct_rgb_components[2] / 0xff);
}

INLINE void palette_set_color_16_palettized(pen_t color,UINT8 red,UINT8 green,UINT8 blue)
{
	if (	actual_palette[3*color + 0] == red &&
			actual_palette[3*color + 1] == green &&
			actual_palette[3*color + 2] == blue)
		return;

	actual_palette[3*color + 0] = red;
	actual_palette[3*color + 1] = green;
	actual_palette[3*color + 2] = blue;

	if (palette_initialized)
		osd_modify_pen(Machine->pens[color],red,green,blue);
}

static void palette_reset_16_palettized(void)
{
	if (palette_initialized)
	{
		pen_t color;
		for (color=0; color<total_colors; color++)
			osd_modify_pen(Machine->pens[color],
						   game_palette[3*color + 0],
						   game_palette[3*color + 1],
						   game_palette[3*color + 2]);
   }
}


INLINE void adjust_shadow(UINT8 *r,UINT8 *g,UINT8 *b,double factor)
{
	if (factor > 1)
	{
		int max = *r;
		if (*g > max) max = *g;
		if (*b > max) max = *b;

		if ((int)(max * factor + 0.5) >= 256)
			factor = 255.0 / max;
	}

	*r = *r * factor + 0.5;
	*g = *g * factor + 0.5;
	*b = *b * factor + 0.5;
}

void palette_set_color(pen_t color,UINT8 r,UINT8 g,UINT8 b)
{
	if (color >= total_colors)
	{
logerror("error: palette_set_color() called with color %d, but only %d allocated.\n",color,total_colors);
		return;
	}

	game_palette[3*color + 0] = r;
	game_palette[3*color + 1] = g;
	game_palette[3*color + 2] = b;

	if (color < Machine->drv->total_colors && brightness[color] != 1.0)
	{
		r = r * brightness[color] + 0.5;
		g = g * brightness[color] + 0.5;
		b = b * brightness[color] + 0.5;
	}

	Machine->palette[color] = MAKE_RGB(r,g,b);

	switch (colormode)
	{
		case PALETTIZED_16BIT:
			palette_set_color_16_palettized(color,r,g,b);
			break;
		case DIRECT_15BIT:
			palette_set_color_15_direct(color,r,g,b);
			break;
		case DIRECT_32BIT:
			palette_set_color_32_direct(color,r,g,b);
			break;
	}

	if (color < Machine->drv->total_colors)
	{
		/* automatically create darker shade for shadow handling */
		if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		{
			UINT8 nr=r,ng=g,nb=b;

			adjust_shadow(&nr,&ng,&nb,shadow_factor);

			color += Machine->drv->total_colors;	/* carry this change over to highlight handling */
			palette_set_color(color,nr,ng,nb);
		}

		/* automatically create brighter shade for highlight handling */
		if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		{
			UINT8 nr=r,ng=g,nb=b;

			adjust_shadow(&nr,&ng,&nb,highlight_factor);

			color += Machine->drv->total_colors;
			palette_set_color(color,nr,ng,nb);
		}
	}
}

void palette_get_color(pen_t color,UINT8 *r,UINT8 *g,UINT8 *b)
{
	if (color == get_black_pen())
	{
		*r = *g = *b = 0;
	}
	else if (color >= total_colors)
	{
		usrintf_showmessage("palette_get_color() out of range");
	}
	else
	{
		*r = game_palette[3*color + 0];
		*g = game_palette[3*color + 1];
		*b = game_palette[3*color + 2];
	}
}

void palette_set_brightness(pen_t color,double bright)
{
	if (brightness[color] != bright)
	{
		brightness[color] = bright;

		palette_set_color(color,game_palette[3*color + 0],game_palette[3*color + 1],game_palette[3*color + 2]);
	}
}

void palette_set_shadow_factor(double factor)
{
	if (shadow_factor != factor)
	{
		int i;

		shadow_factor = factor;

		if (palette_initialized)
		{
			for (i = 0;i < Machine->drv->total_colors;i++)
				palette_set_color(i,game_palette[3*i + 0],game_palette[3*i + 1],game_palette[3*i + 2]);
		}
	}
}

void palette_set_highlight_factor(double factor)
{
	if (highlight_factor != factor)
	{
		int i;

		highlight_factor = factor;

		for (i = 0;i < Machine->drv->total_colors;i++)
			palette_set_color(i,game_palette[3*i + 0],game_palette[3*i + 1],game_palette[3*i + 2]);
	}
}


pen_t get_black_pen(void)
{
	return Machine->uifont->colortable[0];
}


/******************************************************************************

 Commonly used palette RAM handling functions

******************************************************************************/

data8_t *paletteram;
data8_t *paletteram_2;	/* use when palette RAM is split in two parts */
data16_t *paletteram16;
data16_t *paletteram16_2;
data32_t *paletteram32;

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
