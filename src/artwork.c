/*********************************************************************

  artwork.c

  Generic backdrop/overlay functions.

  Created by Mike Balfour - 10/01/1998

  Added some overlay and backdrop functions
  for vector games. Mathis Rosenhauer - 10/09/1998

  MAB - 09 MAR 1999 - made some changes to artwork_create
  MLR - 29 MAR 1999 - added disks to artwork_create
  MLR - 24 MAR 2000 - support for true color artwork
  MLR - 17 JUL 2001 - removed 8bpp code

*********************************************************************/

#include "driver.h"
#include "png.h"
#include "artwork.h"
#include <ctype.h>

#define LIMIT5(x) ((x < 0x1f)? x : 0x1f)
#define LIMIT8(x) ((x < 0xff)? x : 0xff)

/* the backdrop instance */
struct artwork_info *artwork_backdrop = NULL;

/* the overlay instance */
struct artwork_info *artwork_overlay = NULL;

struct mame_bitmap *artwork_real_scrbitmap;

static int my_stricmp( const char *dst, const char *src)
{
	while (*src && *dst)
	{
		if( tolower(*src) != tolower(*dst) ) return *dst - *src;
		src++;
		dst++;
	}
	return *dst - *src;
}

static void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
	float min, max, delta;

	min = MIN( r, MIN( g, b ));
	max = MAX( r, MAX( g, b ));
	*v = max;

	delta = max - min;

	if( delta > 0  )
		*s = delta / max;
	else {
		*s = 0;
		*h = 0;
		return;
	}

	if( r == max )
		*h = ( g - b ) / delta;
	else if( g == max )
		*h = 2 + ( b - r ) / delta;
	else
		*h = 4 + ( r - g ) / delta;

	*h *= 60;
	if( *h < 0 )
		*h += 360;
}

static void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;

	if( s == 0 ) {
		*r = *g = *b = v;
		return;
	}

	h /= 60;
	i = h;
	f = h - i;
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
	case 0: *r = v; *g = t; *b = p; break;
	case 1: *r = q; *g = v; *b = p; break;
	case 2: *r = p; *g = v; *b = t; break;
	case 3: *r = p; *g = q; *b = v; break;
	case 4: *r = t; *g = p; *b = v; break;
	default: *r = v; *g = p; *b = q; break;
	}

}

static void merge_cmy(struct artwork_info *a, struct mame_bitmap *source, struct mame_bitmap *source_alpha,int sx, int sy)
{
	int c1, c2, m1, m2, y1, y2, pen1, pen2, max, alpha;
	int x, y, w, h;
	struct mame_bitmap *dest, *dest_alpha;

	dest = a->orig_artwork;
	dest_alpha = a->alpha;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		w = source->height;
		h = source->width;
	}
	else
	{
		h = source->height;
		w = source->width;
	}

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++)
		{
			pen1 = read_pixel(dest, sx + x, sy + y);

			c1 = 0x1f - (pen1 >> 10);
			m1 = 0x1f - ((pen1 >> 5) & 0x1f);
			y1 = 0x1f - (pen1 & 0x1f);

			pen2 = read_pixel(source, x, y);
			c2 = 0x1f - (pen2 >> 10) + c1;
			m2 = 0x1f - ((pen2 >> 5) & 0x1f) + m1;
			y2 = 0x1f - (pen2 & 0x1f) + y1;

			max = MAX(c2, MAX(m2, y2));
			if (max > 0x1f)
			{
				c2 = (c2 * 0x1f) / max;
				m2 = (m2 * 0x1f) / max;
				y2 = (y2 * 0x1f) / max;
			}

			alpha = MIN (0xff, read_pixel(source_alpha, x, y)
						 + read_pixel(dest_alpha, sx + x, sy + y));
			plot_pixel(dest, sx + x, sy + y,
					   ((0x1f - c2) << 10)
					   | ((0x1f - m2) << 5)
					   | (0x1f - y2));

			plot_pixel(dest_alpha, sx + x, sy + y, alpha);
		}
}

/*********************************************************************
  allocate_artwork_mem

  Allocates memory for all the bitmaps.
 *********************************************************************/
static void allocate_artwork_mem (int width, int height, struct artwork_info **a)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = height;
		height = width;
		width = temp;
	}

	*a = (struct artwork_info *)auto_malloc(sizeof(struct artwork_info));
	if (*a == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return;
	}

	if (((*a)->orig_artwork = auto_bitmap_alloc(width, height)) == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return;
	}
	fillbitmap((*a)->orig_artwork,0,0);

	if (((*a)->alpha = auto_bitmap_alloc(width, height)) == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return;
	}
	fillbitmap((*a)->alpha,0,0);

	if (((*a)->artwork = auto_bitmap_alloc(width,height)) == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return;
	}

	if (((*a)->artwork1 = auto_bitmap_alloc(width,height)) == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return;
	}

	if (((*a)->rgb = (UINT32*)auto_malloc(width*height*sizeof(UINT32)))==0)
	{
		logerror("Not enough memory.\n");
		return;
	}
}

/*********************************************************************
  create_disk

  Creates a disk with radius r in the color of pen. A new bitmap
  is allocated for the disk.

*********************************************************************/
static struct mame_bitmap *create_disk (int r, int fg, int bg)
{
	struct mame_bitmap *disk;

	int x = 0, twox = 0;
	int y = r;
	int twoy = r+r;
	int p = 1 - r;
	int i;

	if ((disk = bitmap_alloc(twoy, twoy)) == 0)
	{
		logerror("Not enough memory for artwork!\n");
		return NULL;
	}

	/* background */
	fillbitmap (disk, bg, 0);

	while (x < y)
	{
		x++;
		twox +=2;
		if (p < 0)
			p += twox + 1;
		else
		{
			y--;
			twoy -= 2;
			p += twox - twoy + 1;
		}

		for (i = 0; i < twox; i++)
		{
			plot_pixel(disk, r-x+i, r-y	 , fg);
			plot_pixel(disk, r-x+i, r+y-1, fg);
		}

		for (i = 0; i < twoy; i++)
		{
			plot_pixel(disk, r-y+i, r-x	 , fg);
			plot_pixel(disk, r-y+i, r+x-1, fg);
		}
	}
	return disk;
}

/*********************************************************************
  artwork_remap

  Creates the final artwork by adding the start_pen to the
  original artwork.
 *********************************************************************/
static void artwork_remap(struct artwork_info *a)
{
	int x, y;
	if (Machine->color_depth == 16)
	{
		for ( y = 0; y < a->orig_artwork->height; y++)
			for (x = 0; x < a->orig_artwork->width; x++)
				((UINT16 *)a->artwork->line[y])[x] = ((UINT16 *)a->orig_artwork->line[y])[x]+a->start_pen;
	}
	else
		copybitmap(a->artwork, a->orig_artwork ,0,0,0,0,NULL,TRANSPARENCY_NONE,0);
}

/*********************************************************************
  init_palette

  This sets the palette colors used by the backdrop. It is a simple
  rgb555 palette of 32768 entries.
 *********************************************************************/
static void init_palette(int start_pen)
{
	int r, g, b;

	for (r = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++)
				palette_set_color(start_pen++, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
}

/*********************************************************************

  Reads a PNG for a artwork struct and converts it into a format
  usable for mame.
 *********************************************************************/
static int decode_png(const char *file_name, struct mame_bitmap **bitmap, struct mame_bitmap **alpha, struct png_info *p)
{
	UINT8 *tmp;
	UINT32 x, y, pen;
	void *fp;
	int file_name_len, depth;
	char file_name2[256];

	depth = Machine->color_depth;

	/* check for .png */
	strcpy(file_name2, file_name);
	file_name_len = strlen(file_name2);
	if ((file_name_len < 4) || my_stricmp(&file_name2[file_name_len - 4], ".png"))
	{
		strcat(file_name2, ".png");
	}

	if (!(fp = osd_fopen(Machine->gamedrv->name, file_name2, OSD_FILETYPE_ARTWORK, 0)))
	{
		logerror("Unable to open PNG %s\n", file_name);
		return 0;
	}

	if (!png_read_file(fp, p))
	{
		osd_fclose (fp);
		return 0;
	}
	osd_fclose (fp);

	if (p->bit_depth > 8)
	{
		logerror("Unsupported bit depth %i (8 bit max.)\n", p->bit_depth);
		return 0;
	}

	if (p->interlace_method != 0)
	{
		logerror("Interlace unsupported\n");
		return 0;
	}

	switch (p->color_type)
	{
	case 3:
		/* Convert to 8 bit */
		png_expand_buffer_8bit (p);

		png_delete_unused_colors (p);

		if ((*bitmap = bitmap_alloc(p->width,p->height)) == 0)
		{
			logerror("Unable to allocate memory for artwork\n");
			return 0;
		}

		tmp = p->image;
		/* convert to 15/32 bit */
		if (p->num_trans > 0)
			if ((*alpha = bitmap_alloc(p->width,p->height)) == 0)
			{
				logerror("Unable to allocate memory for artwork\n");
				return 0;
			}

		for (y=0; y<p->height; y++)
			for (x=0; x<p->width; x++)
			{
				if (depth == 32)
					pen = (p->palette[*tmp * 3] << 16) | (p->palette[*tmp * 3 + 1] << 8) | p->palette[*tmp * 3 + 2];
				else
					pen = ((p->palette[*tmp * 3] & 0xf8) << 7) | ((p->palette[*tmp * 3 + 1] & 0xf8) << 2) | (p->palette[*tmp * 3 + 2] >> 3);
				plot_pixel(*bitmap, x, y, pen);

				if (p->num_trans > 0)
				{
					if (*tmp < p->num_trans)
						plot_pixel(*alpha, x, y, p->trans[*tmp]);
					else
						plot_pixel(*alpha, x, y, 255);
				}
				tmp++;
			}

		free (p->palette);
		break;

	case 6:
		if ((*alpha = bitmap_alloc(p->width,p->height)) == 0)
		{
			logerror("Unable to allocate memory for artwork\n");
			return 0;
		}

	case 2:
		if ((*bitmap = bitmap_alloc(p->width,p->height)) == 0)
		{
			logerror("Unable to allocate memory for artwork\n");
			return 0;
		}

		tmp = p->image;
		for (y=0; y<p->height; y++)
			for (x=0; x<p->width; x++)
			{
				if (depth == 32)
					pen = (tmp[0] << 16) | (tmp[1] << 8) | tmp[2];
				else
					pen = ((tmp[0] & 0xf8) << 7) | ((tmp[1] & 0xf8) << 2) | (tmp[2] >> 3);
				plot_pixel(*bitmap, x, y, pen);

				if (p->color_type == 6)
				{
					plot_pixel(*alpha, x, y, tmp[3]);
					tmp += 4;
				}
				else
					tmp += 3;
			}

		break;

	default:
		logerror("Unsupported color type %i \n", p->color_type);
		return 0;
		break;
	}
	free (p->image);
	return 1;
}

/*********************************************************************
  load_png

  This is what loads your backdrop in from disk.
  start_pen = the first pen available for the backdrop to use
 *********************************************************************/

static void load_png(const char *filename, unsigned int start_pen,
					 int width, int height, struct artwork_info **a)
{
	struct mame_bitmap *picture = 0, *alpha = 0;
	struct png_info p;
	int scalex, scaley;

	/* If the user turned artwork off, bail */
	if (!options.use_artwork) return;

	if (!decode_png(filename, &picture, &alpha, &p))
		return;

	allocate_artwork_mem(width, height, a);


	if (*a==NULL)
		return;

	/* Scale the original picture to be the same size as the visible area */
	scalex = 0x10000 * picture->width  / (*a)->artwork->width;
	scaley = 0x10000 * picture->height / (*a)->artwork->height;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int tmp;
		tmp = scalex;
		scalex = scaley;
		scaley = tmp;
	}

	copyrozbitmap((*a)->orig_artwork, picture, 0, 0, scalex, 0, 0, scaley, 0, 0, TRANSPARENCY_NONE, 0, 0);
	/* We don't need the original any more */
	bitmap_free(picture);

	if (alpha)
	{
		copyrozbitmap((*a)->alpha, alpha, 0, 0, scalex, 0, 0, scaley, 0, 0, TRANSPARENCY_NONE, 0, 0);
		bitmap_free(alpha);
	}

	if (Machine->color_depth == 16)
	{
		(*a)->start_pen = start_pen;
		init_palette(start_pen);
	}
	else
		(*a)->start_pen = 0;

	artwork_remap(*a);
logerror("Png Loaded %d %d\n", width, height);
}

static void load_png_fit(const char *filename, unsigned int start_pen, struct artwork_info **a)
{
	load_png(filename, start_pen, Machine->scrbitmap->width, Machine->scrbitmap->height, a);
}


/*********************************************************************
  overlay_init


 *********************************************************************/
static void overlay_init(struct artwork_info *a)
{
	int i,j, rgb555;
	UINT8 r,g,b;
	float h, s, v, rf, gf, bf;
	int offset, height, width;
	struct mame_bitmap *overlay, *overlay1, *orig;

	offset = a->start_pen;
	height = a->artwork->height;
	width = a->artwork->width;
	overlay = a->artwork;
	overlay1 = a->artwork1;
	orig = a->orig_artwork;

	if (a->alpha)
	{
		for ( j=0; j<height; j++)
			for (i=0; i<width; i++)
			{
				UINT32 v1,v2;
				UINT16 alpha = ((UINT16 *)a->alpha->line[j])[i];

				rgb555 = ((UINT16 *)orig->line[j])[i];
				r = rgb555 >> 10;
				g = (rgb555 >> 5) & 0x1f;
				b = rgb555 &0x1f;
				v1 = (MAX(r, MAX(g, b)));
				v2 = (v1 * (alpha >> 3)) / 0x1f;
				a->rgb[j*width+i] = (v1 << 24) | (v2 << 16) | rgb555;

				RGBtoHSV( r/31.0, g/31.0, b/31.0, &h, &s, &v );

				HSVtoRGB( &rf, &gf, &bf, h, s, v * alpha/255.0);
				r = rf*31; g = gf*31; b = bf*31;
				((UINT16 *)overlay->line[j])[i] = ((r << 10) | (g << 5) | b) + offset;

				HSVtoRGB( &rf, &gf, &bf, h, s, 1);
				r = rf*31; g = gf*31; b = bf*31;
				((UINT16 *)overlay1->line[j])[i] = ((r << 10) | (g << 5) | b) + offset;
			}
	}
}

/*********************************************************************
  overlay_draw

  Supports different levels of intensity on the screen and different
  levels of transparancy of the overlay.
 *********************************************************************/

static void overlay_draw(struct mame_bitmap *dest, struct mame_bitmap *source)
{
	int i, j;
	int height, width;
	int r, g, b, bp, v, vn, black, start_pen;
	UINT8 r8, g8, b8;
	UINT16 *src, *dst, *bg, *fg;
	UINT32 bright[65536];
	UINT32 *rgb;

	memset (bright, 0xff, sizeof(int)*65536);
	height = source->height;
	width = source->width;

	switch (Machine->color_depth)
	{
	case 16:
		if (artwork_overlay->start_pen == 2)
		{
			/* fast version */
			height = artwork_overlay->artwork->height;
			width = artwork_overlay->artwork->width;

			for ( j = 0; j < height; j++)
			{
				dst = (UINT16 *)dest->line[j];
				src = (UINT16 *)source->line[j];
				bg = (UINT16 *)artwork_overlay->artwork->line[j];
				fg = (UINT16 *)artwork_overlay->artwork1->line[j];
				for (i = width; i > 0; i--)
				{
					if (*src!=0)
						*dst = *fg;
					else
						*dst = *bg;
					dst++;
					src++;
					fg++;
					bg++;
				}
			}
		}
		else /* slow version */
		{
			rgb = artwork_overlay->rgb;
			start_pen = artwork_overlay->start_pen;
			copybitmap(dest, artwork_overlay->artwork ,0,0,0,0,NULL,TRANSPARENCY_NONE,0);
			black = -1;
			for ( j = 0; j < height; j++)
			{
				dst = (UINT16 *)dest->line[j];
				src = (UINT16 *)source->line[j];

				for (i = width; i > 0; i--)
				{
					if (*src != black)
					{
						bp = bright[*src];
						if (bp)
						{
							if (bp == 0xffffffff)
							{
								palette_get_color(*src, &r8, &g8, &b8);
								bright[*src]=bp=(222*r8+707*g8+71*b8)/1000;
							}

							v = *rgb >> 24;
							vn =(*rgb >> 16) & 0x1f;
							vn += ((0x1f - vn) * (bp >> 3)) / 0x1f;
							if (v > 0)
							{
								r = (((*rgb >> 10) & 0x1f) * vn) / v;
								g = (((*rgb >> 5)  & 0x1f) * vn) / v;
								b = ((*rgb & 0x1f) * vn) / v;
								*dst = ((r << 10) | (g << 5) | b) + start_pen;
							}
							else
								*dst = ((vn << 10) | (vn << 5) | vn) + start_pen;
						}
						else
							black = *src;
					}
					src++;
					dst++;
					rgb++;
				}
			}

		}
		break;
	case 15:
		rgb = artwork_overlay->rgb;
		start_pen = artwork_overlay->start_pen;
		copybitmap(dest, artwork_overlay->artwork ,0,0,0,0,NULL,TRANSPARENCY_NONE,0);
		black = -1;
		for ( j = 0; j < height; j++)
		{
			dst = (UINT16 *)dest->line[j];
			src = (UINT16 *)source->line[j];

			for (i = width; i > 0; i--)
			{
				if (*src != black)
				{
					bp = bright[*src];
					if (bp)
					{
						if (bp == 0xffffffff)
							bright[*src]=bp=(222*(*src >> 10)
											 +707*((*src >> 5) & 0x1f)
											 +71*(*src & 0x1f))/1000;

						v = *rgb >> 24;
						vn =(*rgb >> 16) & 0x1f;
						vn += ((0x1f - vn) * bp) / 0x1f;
						if (v > 0)
						{
							r = (((*rgb >> 10) & 0x1f) * vn) / v;
							g = (((*rgb >> 5)  & 0x1f) * vn) / v;
							b = ((*rgb & 0x1f) * vn) / v;
							*dst = ((r << 10) | (g << 5) | b);
						}
						else
							*dst = ((vn << 10) | (vn << 5) | vn);
					}
					else
						black = *src;
				}
				src++;
				dst++;
				rgb++;
			}
		}
		break;
	default:
		logerror ("Color depth of %d not supported with overlays\n", Machine->color_depth);
		break;
	}
}

/*********************************************************************
  backdrop_draw

 *********************************************************************/

static void backdrop_draw(struct mame_bitmap *dest, struct mame_bitmap *source)
{
	int i, j;

	copybitmap(dest, artwork_backdrop->artwork ,0,0,0,0,NULL,TRANSPARENCY_NONE,0);

	switch (Machine->color_depth)
	{
	case 16:
	{
		int brgb, black = -1;
		UINT8 r, g, b;
		UINT16 *dst, *bdr, *src;

		for ( j = 0; j < source->height; j++)
		{
			dst = (UINT16 *)dest->line[j];
			src = (UINT16 *)source->line[j];
			bdr = (UINT16 *)artwork_backdrop->artwork->line[j];
			for (i = 0; i < source->width; i++)
			{
				if (*src != black)
				{
					palette_get_color(*src, &r, &g, &b);

					if ((r == 0) && (g == 0) && (b == 0))
					{
						black = *src;
					}

					r >>= 3;
					g >>= 3;
					b >>= 3;
					brgb = *bdr - artwork_backdrop->start_pen;
					r += brgb >> 10;
					if (r > 0x1f) r = 0x1f;
					g += (brgb >> 5) & 0x1f;
					if (g > 0x1f) g = 0x1f;
					b += brgb & 0x1f;
					if (b > 0x1f) b = 0x1f;
					*dst = ((r << 10) | (g << 5) | b) + artwork_backdrop->start_pen;
				}
				dst++;
				src++;
				bdr++;
			}
		}
		break;
	}

	case 15:
	{
		UINT16 *dst, *src, *bdr;

		for ( j = 0; j < source->height; j++)
		{
			dst = (UINT16 *)dest->line[j];
			src = (UINT16 *)source->line[j];
			bdr = (UINT16 *)artwork_backdrop->artwork->line[j];
			for (i = 0; i < source->width; i++)
			{
				if (*src)
				{
					*dst = LIMIT5((*src & 0x1f) + (*bdr & 0x1f))
						| (LIMIT5(((*src >> 5) & 0x1f) + ((*bdr >> 5) & 0x1f)) << 5)
						| (LIMIT5((*src >> 10) + (*bdr >> 10)) << 10);
				}
				dst++;
				src++;
				bdr++;
			}
		}
		break;
	}

	case 32:
	{
		UINT32 *dst, *src, *bdr;

		for ( j = 0; j < source->height; j++)
		{
			dst = (UINT32 *)dest->line[j];
			src = (UINT32 *)source->line[j];
			bdr = (UINT32 *)artwork_backdrop->artwork->line[j];
			for (i = 0; i < source->width; i++)
			{
				if (*src)
				{
					*dst = LIMIT8((*src & 0xff) + (*bdr & 0xff))
						| (LIMIT8(((*src >> 8) & 0xff) + ((*bdr >> 8) & 0xff)) << 8)
						| (LIMIT8((*src >> 16) + (*bdr >> 16)) << 16);
				}
				dst++;
				src++;
				bdr++;
			}
		}
		break;
	}
	}
}

void artwork_draw(struct mame_bitmap *dest, struct mame_bitmap *source, int full_refresh)
{
	if (artwork_backdrop) backdrop_draw(dest, source);
	if (artwork_overlay) overlay_draw(dest, source);
}

/*********************************************************************
  artwork_free

  Don't forget to clean up when you're done with the backdrop!!!
 *********************************************************************/

void artwork_free(struct artwork_info **a)
{
	if (*a)
	{
		if ((*a)->artwork)
			bitmap_free((*a)->artwork);
		if ((*a)->artwork1)
			bitmap_free((*a)->artwork1);
		if ((*a)->alpha)
			bitmap_free((*a)->alpha);
		if ((*a)->orig_artwork)
			bitmap_free((*a)->orig_artwork);
		if ((*a)->rgb)
			free ((*a)->rgb);
		free(*a);

		*a = NULL;
	}
}

void artwork_kill (void)
{
}

void overlay_load(const char *filename, unsigned int start_pen)
{
	int width, height;

	/* replace the real display with a fake one, this way drivers can access Machine->scrbitmap
	   the same way as before */

	width = Machine->scrbitmap->width;
	height = Machine->scrbitmap->height;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = height;
		height = width;
		width = temp;
	}

	load_png_fit(filename, start_pen, &artwork_overlay);

	if (artwork_overlay)
	{
		if ((artwork_real_scrbitmap = auto_bitmap_alloc(width, height)) == 0)
		{
			artwork_kill();
			logerror("Not enough memory for artwork!\n");
			return;
		}
		overlay_init(artwork_overlay);
	}
}

void backdrop_load(const char *filename, unsigned int start_pen)
{
	int width, height;

	/* replace the real display with a fake one, this way drivers can access Machine->scrbitmap
	   the same way as before */

	load_png_fit(filename, start_pen, &artwork_backdrop);

	if (artwork_backdrop)
	{
		width = artwork_backdrop->artwork->width;
		height = artwork_backdrop->artwork->height;

		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int temp;

			temp = height;
			height = width;
			width = temp;
		}

		if ((artwork_real_scrbitmap = auto_bitmap_alloc(width, height)) == 0)
		{
			artwork_kill();
			logerror("Not enough memory for artwork!\n");
			return;
		}
	}
}

void artwork_load(struct artwork_info **a, const char *filename, unsigned int start_pen)
{
	load_png_fit(filename, start_pen, a);
}

void artwork_load_size(struct artwork_info **a, const char *filename, unsigned int start_pen,
					   int width, int height)
{
	load_png(filename, start_pen, width, height, a);
}

/*********************************************************************
  artwork_elements scale

  scales an array of artwork elements to width and height. The first
  element (which has to be a box) is used as reference. This is useful
  for atwork with disks.

*********************************************************************/

void artwork_elements_scale(struct artwork_element *ae, int width, int height)
{
	int scale_w, scale_h;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		scale_w = (height << 16)/(ae->box.max_x + 1);
		scale_h = (width << 16)/(ae->box.max_y + 1);
	}
	else
	{
		scale_w = (width << 16)/(ae->box.max_x + 1);
		scale_h = (height << 16)/(ae->box.max_y + 1);
	}
	while (ae->box.min_x >= 0)
	{
		ae->box.min_x = (ae->box.min_x * scale_w) >> 16;
		ae->box.max_x = (ae->box.max_x * scale_w) >> 16;
		ae->box.min_y = (ae->box.min_y * scale_h) >> 16;
		if (ae->box.max_y >= 0)
			ae->box.max_y = (ae->box.max_y * scale_h) >> 16;
		ae++;
	}
}

/*********************************************************************
  overlay_create

  This works similar to artwork_load but generates artwork from
  an array of artwork_element. This is useful for very simple artwork
  like the overlay in the Space invaders series of games.  The overlay
  is defined to be the same size as the screen.
  The end of the array is marked by an entry with negative coordinates.
  Boxes and disks are supported. Disks are marked max_y == -1,
  min_x == x coord. of center, min_y == y coord. of center, max_x == radius.
  If there are transparent and opaque overlay elements, the opaque ones
  have to be at the end of the list to stay compatible with the PNG
  artwork.
 *********************************************************************/
void overlay_create(const struct artwork_element *ae, unsigned int start_pen)
{
	struct mame_bitmap *disk, *disk_alpha, *box, *box_alpha;
	int pen, transparent_pen = -1, disk_type, white_pen;
	int width, height;

	allocate_artwork_mem(Machine->scrbitmap->width, Machine->scrbitmap->height, &artwork_overlay);

	if (artwork_overlay==NULL)
		return;

	/* replace the real display with a fake one, this way drivers can access Machine->scrbitmap
	   the same way as before */

	width = Machine->scrbitmap->width;
	height = Machine->scrbitmap->height;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = height;
		height = width;
		width = temp;
	}

	if ((artwork_real_scrbitmap = auto_bitmap_alloc(width, height)) == 0)
	{
		artwork_kill();
		logerror("Not enough memory for artwork!\n");
		return;
	}

	transparent_pen = 0xffff;
	white_pen = 0x7fff;
	fillbitmap (artwork_overlay->orig_artwork, white_pen, 0);
	fillbitmap (artwork_overlay->alpha, 0, 0);

	while (ae->box.min_x >= 0)
	{
		int alpha = ae->alpha;

		if (alpha == OVERLAY_DEFAULT_OPACITY)
		{
			alpha = 0x18;
		}

		pen = ((ae->red & 0xf8) << 7) | ((ae->green & 0xf8) << 2) | (ae->blue >> 3);
		if (ae->box.max_y < 0) /* disk */
		{
			int r = ae->box.max_x;
			disk_type = ae->box.max_y;

			switch (disk_type)
			{
			case -1: /* disk overlay */
				if ((disk = create_disk (r, pen, white_pen)) == NULL)
				{
					artwork_kill();
					return;
				}
				if ((disk_alpha = create_disk (r, alpha, 0)) == NULL)
				{
					artwork_kill();
					return;
				}
				merge_cmy (artwork_overlay, disk, disk_alpha, ae->box.min_x - r, ae->box.min_y - r);
				bitmap_free(disk_alpha);
				bitmap_free(disk);
				break;

			case -2: /* punched disk */
				if ((disk = create_disk (r, pen, transparent_pen)) == NULL)
				{
					artwork_kill();
					return;
				}
				copybitmap(artwork_overlay->orig_artwork,disk,0, 0,
						   ae->box.min_x - r,
						   ae->box.min_y - r,
						   0,TRANSPARENCY_PEN, transparent_pen);
				/* alpha */
				if ((disk_alpha = create_disk (r, alpha, transparent_pen)) == NULL)
				{
					artwork_kill();
					return;
				}
				copybitmap(artwork_overlay->alpha,disk_alpha,0, 0,
						   ae->box.min_x - r,
						   ae->box.min_y - r,
						   0,TRANSPARENCY_PEN, transparent_pen);
				bitmap_free(disk_alpha);
				bitmap_free(disk);
				break;

			}
		}
		else
		{
			if ((box = bitmap_alloc(ae->box.max_x - ae->box.min_x + 1,
										 ae->box.max_y - ae->box.min_y + 1)) == 0)
			{
				logerror("Not enough memory for artwork!\n");
				artwork_kill();
				return;
			}
			if ((box_alpha = bitmap_alloc(ae->box.max_x - ae->box.min_x + 1,
										 ae->box.max_y - ae->box.min_y + 1)) == 0)
			{
				logerror("Not enough memory for artwork!\n");
				artwork_kill();
				return;
			}
			fillbitmap (box, pen, 0);
			fillbitmap (box_alpha, alpha, 0);
			merge_cmy (artwork_overlay, box, box_alpha, ae->box.min_x, ae->box.min_y);
			bitmap_free(box);
			bitmap_free(box_alpha);
		}
		ae++;
	}

	if (Machine->color_depth == 16)
	{
		artwork_overlay->start_pen = start_pen;
		init_palette(start_pen);
	}
	else
		artwork_overlay->start_pen = 0;

	artwork_remap(artwork_overlay);
	overlay_init(artwork_overlay);
}

int artwork_get_size_info(const char *file_name, struct artwork_size_info *a)
{
	void *fp;
	struct png_info p;
	int file_name_len;
	char file_name2[256];

	/* If the user turned artwork off, bail */
	if (!options.use_artwork) return 0;

	/* check for .png */
	strcpy(file_name2, file_name);
	file_name_len = strlen(file_name2);
	if ((file_name_len < 4) || my_stricmp(&file_name2[file_name_len - 4], ".png"))
	{
		strcat(file_name2, ".png");
	}

	if (!(fp = osd_fopen(Machine->gamedrv->name, file_name2, OSD_FILETYPE_ARTWORK, 0)))
	{
		logerror("Unable to open PNG %s\n", file_name);
		return 0;
	}

	if (!png_read_info(fp, &p))
	{
		osd_fclose (fp);
		return 0;
	}
	osd_fclose (fp);

	a->width = p.width;
	a->height = p.height;
	a->screen = p.screen;

	return 1;
}

