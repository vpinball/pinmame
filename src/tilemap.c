/* tilemap.c

	When the videoram for a tile changes, call tilemap_mark_tile_dirty
	with the appropriate memory offset.

	In the video driver, follow these steps:

	1)	Set each tilemap's scroll registers.

	2)	Call tilemap_update( ALL_TILEMAPS ).

	3)	Call palette_init_used_colors().
		Mark the colors used by sprites.
		Call palette recalc().

	4)	Call tilemap_draw to draw the tilemaps to the screen, from back to front.

	Notes:
	-	You can currently configure a tilemap as xscroll + scrolling columns or
		yscroll + scrolling rows, but not both types of scrolling simultaneously.

	To Do:
	-	Screenwise scrolling (orthagonal to row/colscroll)

	-	ROZ blitting support

	-	Dynamically resizable tilemaps (num rows/cols, tile size) are used by several games.

	-	modulus to next line of pen data should be settable.

	-	Scroll registers should be automatically recomputed when screen flips.

	-	Logical handling of TILEMAP_FRONT|TILEMAP_BACK (but see below).

	-	Consider alternate priority buffer implementations.  It would be nice to be able to render each
		layer in a single pass, handling multiple tile priorities, split tiles, etc. all seamlessly,
		then combine the layers and sprites appropriately as a final step.
*/
#ifndef DECLARE

#include "driver.h"
#include "tilemap.h"
#include "state.h"

#define SWAP(X,Y) { UINT32 temp=X; X=Y; Y=temp; }
#define MAX_TILESIZE 32
#define MASKROWBYTES(W) (((W)+7)/8)

struct cached_tile_info
{
	const UINT8 *pen_data;
	const UINT32 *pal_data;
	UINT32 pen_usage;
	UINT32 flags;
	int skip;
};

struct tilemap_mask
{
	struct osd_bitmap *bitmask;
	int line_offset;
	UINT8 *data;
	UINT8 **data_row;
};

struct tilemap
{
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows );
	int *memory_offset_to_cached_indx;
	UINT32 *cached_indx_to_memory_offset;
	int logical_flip_to_cached_flip[4];

	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int memory_offset );

	UINT32 max_memory_offset;
	UINT32 num_tiles;
	UINT32 num_logical_rows, num_logical_cols;
	UINT32 num_cached_rows, num_cached_cols;
	UINT32 tile_size;
	UINT32 num_pens;
	UINT32 cached_width, cached_height; /* size in pixels of tilemap as a whole */

	struct cached_tile_info *cached_tile_info;

	int dx, dx_if_flipped;
	int dy, dy_if_flipped;
	int scrollx_delta, scrolly_delta;

	int enable;
	int attributes;

	int type;
	int transparent_pen;
	UINT32 fgmask[4];
	UINT32 bgmask[4];

	int bNeedRender;

	UINT32 *pPenToPixel[8];

	void (*draw_tile)( struct tilemap *tilemap, UINT32 cached_indx, UINT32 col, UINT32 row );

	void (*draw)( int, int );
	void (*draw_opaque)( int, int );
	void (*draw_alpha)( int, int );

	UINT8 *priority,	/* priority for each tile */
		**priority_row;

	UINT8 *visible; /* boolean flag for each tile */

	UINT8 *dirty_vram; /* boolean flag for each tile */

	UINT8 *dirty_pixels;

	int scroll_rows, scroll_cols;
	int *rowscroll, *colscroll;

	int orientation;
	int clip_left,clip_right,clip_top,clip_bottom;

	UINT16 tile_depth, tile_granularity;
	UINT8 *tile_dirty_map;

	/* cached color data */
	struct osd_bitmap *pixmap;
	int pixmap_line_offset;

	struct tilemap_mask *foreground;
	/* for transparent layers, or the front half of a split layer */

	struct tilemap_mask *background;
	/* for the back half of a split layer */

	struct tilemap *next; /* resource tracking */
};

struct osd_bitmap *priority_bitmap; /* priority buffer (corresponds to screen bitmap) */
int priority_bitmap_line_offset;

static UINT8 flip_bit_table[0x100]; /* horizontal flip for 8 pixels */
static struct tilemap *first_tilemap; /* resource tracking */
static int screen_width, screen_height;
struct tile_info tile_info;

enum
{
	TILE_TRANSPARENT,
	TILE_MASKED,
	TILE_OPAQUE
};

/* the following parameters are constant across tilemap_draw calls */
static struct
{
	int clip_left, clip_top, clip_right, clip_bottom;
	int source_width, source_height;
	int dest_line_offset,source_line_offset,mask_line_offset;
	int dest_row_offset,source_row_offset,mask_row_offset;
	struct osd_bitmap *screen, *pixmap, *bitmask;
	UINT8 **mask_data_row;
	UINT8 **priority_data_row;
	int tile_priority;
	int tilemap_priority_code;
} blit;

int PenToPixel_Init( struct tilemap *tilemap )
{
	/*
		Construct a table for all tile orientations in advance.
		This simplifies drawing tiles and masks tremendously.
		If performance is an issue, we can always (re)introduce
		customized code for each case and forgo tables.
	*/
	int i,x,y,tx,ty;
	int tile_size = tilemap->tile_size;
	UINT32 *pPenToPixel;
	int lError;

	lError = 0;
	for( i=0; i<8; i++ )
	{
		pPenToPixel = malloc( tilemap->num_pens*sizeof(UINT32) );
		if( pPenToPixel==NULL )
		{
			lError = 1;
		}
		else
		{
			tilemap->pPenToPixel[i] = pPenToPixel;
			for( ty=0; ty<tile_size; ty++ )
			{
				for( tx=0; tx<tile_size; tx++ )
				{
					if( i&TILE_SWAPXY )
					{
						x = ty;
						y = tx;
					}
					else
					{
						x = tx;
						y = ty;
					}
					if( i&TILE_FLIPX ) x = tile_size-1-x;
					if( i&TILE_FLIPY ) y = tile_size-1-y;
					*pPenToPixel++ = x+y*MAX_TILESIZE;
				}
			}
		}
	}
	return lError;
}

void PenToPixel_Term( struct tilemap *tilemap )
{
	int i;
	for( i=0; i<8; i++ )
	{
		free( tilemap->pPenToPixel[i] );
	}
}

static void tmap_render( struct tilemap *tilemap )
{
	if( tilemap->bNeedRender ){
		tilemap->bNeedRender = 0;
		if( tilemap->enable ){
			UINT8 *dirty_pixels = tilemap->dirty_pixels;
			const UINT8 *visible = tilemap->visible;
			UINT32 cached_indx = 0;
			UINT32 row,col;

			/* walk over cached rows/cols (better to walk screen coords) */
			for( row=0; row<tilemap->num_cached_rows; row++ ){
				for( col=0; col<tilemap->num_cached_cols; col++ ){
					if( visible[cached_indx] && dirty_pixels[cached_indx] ){
						tilemap->draw_tile( tilemap, cached_indx, col, row );
						dirty_pixels[cached_indx] = 0;
					}
					cached_indx++;
				} /* next col */
			} /* next row */
		}
	}
}

struct osd_bitmap *tilemap_get_pixmap( struct tilemap * tilemap )
{
profiler_mark(PROFILER_TILEMAP_DRAW);
	tmap_render( tilemap );
profiler_mark(PROFILER_END);
	return tilemap->pixmap;
}

void tilemap_set_transparent_pen( struct tilemap *tilemap, int pen )
{
	tilemap->transparent_pen = pen;
}

void tilemap_set_transmask( struct tilemap *tilemap, int which, UINT32 fgmask, UINT32 bgmask )
{
	tilemap->fgmask[which] = fgmask;
	tilemap->bgmask[which] = bgmask;
}

void tilemap_set_depth( struct tilemap *tilemap, int tile_depth, int tile_granularity )
{
	if( tilemap->tile_dirty_map )
	{
		free( tilemap->tile_dirty_map);
	}
	tilemap->tile_dirty_map = malloc( Machine->drv->total_colors >> tile_granularity);
	if( tilemap->tile_dirty_map )
	{
		tilemap->tile_depth = tile_depth;
		tilemap->tile_granularity = tile_granularity;
	}
}

/***********************************************************************************/
/* some common mappings */

UINT32 tilemap_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + col;
}
UINT32 tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return col*num_rows + row;
}

/*********************************************************************************/

static struct osd_bitmap *create_tmpbitmap( int width, int height, int depth )
{
	return osd_alloc_bitmap( width,height,depth );
}

static struct osd_bitmap *create_bitmask( int width, int height )
{
	width = (width+7)/8; /* 8 bits per byte */
	return osd_alloc_bitmap( width,height, 8 );
}

/***********************************************************************************/

static int mappings_create( struct tilemap *tilemap )
{
	int max_memory_offset = 0;
	UINT32 col,row;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	/* count offsets (might be larger than num_tiles) */
	for( row=0; row<num_logical_rows; row++ )
	{
		for( col=0; col<num_logical_cols; col++ )
		{
			UINT32 memory_offset = tilemap->get_memory_offset( col, row, num_logical_cols, num_logical_rows );
			if( memory_offset>max_memory_offset ) max_memory_offset = memory_offset;
		}
	}
	max_memory_offset++;
	tilemap->max_memory_offset = max_memory_offset;
	/* logical to cached (tilemap_mark_dirty) */
	tilemap->memory_offset_to_cached_indx = malloc( sizeof(int)*max_memory_offset );
	if( tilemap->memory_offset_to_cached_indx )
	{
		/* cached to logical (get_tile_info) */
		tilemap->cached_indx_to_memory_offset = malloc( sizeof(UINT32)*tilemap->num_tiles );
		if( tilemap->cached_indx_to_memory_offset ) return 0; /* no error */
		free( tilemap->memory_offset_to_cached_indx );
	}
	return -1; /* error */
}

static void mappings_dispose( struct tilemap *tilemap )
{
	free( tilemap->cached_indx_to_memory_offset );
	free( tilemap->memory_offset_to_cached_indx );
}

static void mappings_update( struct tilemap *tilemap )
{
	int logical_flip;
	UINT32 logical_indx, cached_indx;
	UINT32 num_cached_rows = tilemap->num_cached_rows;
	UINT32 num_cached_cols = tilemap->num_cached_cols;
	UINT32 num_logical_rows = tilemap->num_logical_rows;
	UINT32 num_logical_cols = tilemap->num_logical_cols;
	for( logical_indx=0; logical_indx<tilemap->max_memory_offset; logical_indx++ )
	{
		tilemap->memory_offset_to_cached_indx[logical_indx] = -1;
	}

	for( logical_indx=0; logical_indx<tilemap->num_tiles; logical_indx++ )
	{
		UINT32 logical_col = logical_indx%num_logical_cols;
		UINT32 logical_row = logical_indx/num_logical_cols;
		int memory_offset = tilemap->get_memory_offset( logical_col, logical_row, num_logical_cols, num_logical_rows );
		UINT32 cached_col = logical_col;
		UINT32 cached_row = logical_row;
		if( tilemap->orientation & ORIENTATION_SWAP_XY ) SWAP(cached_col,cached_row)
		if( tilemap->orientation & ORIENTATION_FLIP_X ) cached_col = (num_cached_cols-1)-cached_col;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) cached_row = (num_cached_rows-1)-cached_row;
		cached_indx = cached_row*num_cached_cols+cached_col;
		tilemap->memory_offset_to_cached_indx[memory_offset] = cached_indx;
		tilemap->cached_indx_to_memory_offset[cached_indx] = memory_offset;
	}
	for( logical_flip = 0; logical_flip<4; logical_flip++ )
	{
		int cached_flip = logical_flip;
		if( tilemap->attributes&TILEMAP_FLIPX ) cached_flip ^= TILE_FLIPX;
		if( tilemap->attributes&TILEMAP_FLIPY ) cached_flip ^= TILE_FLIPY;
#ifndef PREROTATE_GFX
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPY;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPX;
		}
		else
		{
			if( Machine->orientation & ORIENTATION_FLIP_X ) cached_flip ^= TILE_FLIPX;
			if( Machine->orientation & ORIENTATION_FLIP_Y ) cached_flip ^= TILE_FLIPY;
		}
#endif
		if( tilemap->orientation & ORIENTATION_SWAP_XY )
		{
			cached_flip = ((cached_flip&1)<<1) | ((cached_flip&2)>>1);
		}
		tilemap->logical_flip_to_cached_flip[logical_flip] = cached_flip;
	}
}

/***********************************************************************************/

static void memsetbitmask8( UINT8 *dest, int value, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] |= value;
		if( data&0x40 ) dest[1] |= value;
		if( data&0x20 ) dest[2] |= value;
		if( data&0x10 ) dest[3] |= value;
		if( data&0x08 ) dest[4] |= value;
		if( data&0x04 ) dest[5] |= value;
		if( data&0x02 ) dest[6] |= value;
		if( data&0x01 ) dest[7] |= value;
		if( --count == 0 ) break;
		dest+=8;
	}
}

static void memcpybitmask8( UINT8 *dest, const UINT8 *source, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

static void memcpybitmask16( UINT16 *dest, const UINT16 *source, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

static void memcpybitmask32( UINT32 *dest, const UINT32 *source, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = source[0];
		if( data&0x40 ) dest[1] = source[1];
		if( data&0x20 ) dest[2] = source[2];
		if( data&0x10 ) dest[3] = source[3];
		if( data&0x08 ) dest[4] = source[4];
		if( data&0x04 ) dest[5] = source[5];
		if( data&0x02 ) dest[6] = source[6];
		if( data&0x01 ) dest[7] = source[7];
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

static void blend16( UINT16 *dest, const UINT16 *source, int count )
{
	for(;;)
	{
		*dest = alpha_blend16(*dest, *source);
		if( --count == 0 ) break;
		source++;
		dest++;
	}
}

static void blendbitmask16( UINT16 *dest, const UINT16 *source, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = alpha_blend16(dest[0], source[0]);
		if( data&0x40 ) dest[1] = alpha_blend16(dest[1], source[1]);
		if( data&0x20 ) dest[2] = alpha_blend16(dest[2], source[2]);
		if( data&0x10 ) dest[3] = alpha_blend16(dest[3], source[3]);
		if( data&0x08 ) dest[4] = alpha_blend16(dest[4], source[4]);
		if( data&0x04 ) dest[5] = alpha_blend16(dest[5], source[5]);
		if( data&0x02 ) dest[6] = alpha_blend16(dest[6], source[6]);
		if( data&0x01 ) dest[7] = alpha_blend16(dest[7], source[7]);
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

static void blend32( UINT32 *dest, const UINT32 *source, int count )
{
	for(;;)
	{
		*dest = alpha_blend32(*dest, *source);
		if( --count == 0 ) break;
		source++;
		dest++;
	}
}

static void blendbitmask32( UINT32 *dest, const UINT32 *source, const UINT8 *bitmask, int count )
{
	for(;;)
	{
		UINT32 data = *bitmask++;
		if( data&0x80 ) dest[0] = alpha_blend32(dest[0], source[0]);
		if( data&0x40 ) dest[1] = alpha_blend32(dest[1], source[1]);
		if( data&0x20 ) dest[2] = alpha_blend32(dest[2], source[2]);
		if( data&0x10 ) dest[3] = alpha_blend32(dest[3], source[3]);
		if( data&0x08 ) dest[4] = alpha_blend32(dest[4], source[4]);
		if( data&0x04 ) dest[5] = alpha_blend32(dest[5], source[5]);
		if( data&0x02 ) dest[6] = alpha_blend32(dest[6], source[6]);
		if( data&0x01 ) dest[7] = alpha_blend32(dest[7], source[7]);
		if( --count == 0 ) break;
		source+=8;
		dest+=8;
	}
}

/***********************************************************************************/

/**** DEPTH == 8 ****/

#define DEPTH 8
#define TILE_SIZE	8
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##8x8x8BPP args body
#include "tilemap.c"

#define TILE_SIZE	16
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##16x16x8BPP args body
#include "tilemap.c"

#define TILE_SIZE	32
#define DATA_TYPE UINT8
#define memcpybitmask memcpybitmask8
#define DECLARE(function,args,body) static void function##32x32x8BPP args body
#include "tilemap.c"
#undef DEPTH

/**** DEPTH == 16 ****/

#define DEPTH 16
#define TILE_SIZE	8
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define blend blend16
#define blendbitmask blendbitmask16
#define DECLARE(function,args,body) static void function##8x8x16BPP args body
#include "tilemap.c"

#define TILE_SIZE	16
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define DECLARE(function,args,body) static void function##16x16x16BPP args body
#include "tilemap.c"

#define TILE_SIZE	32
#define DATA_TYPE UINT16
#define memcpybitmask memcpybitmask16
#define DECLARE(function,args,body) static void function##32x32x16BPP args body
#include "tilemap.c"
#undef DEPTH
#undef blend
#undef blendbitmask

/**** DEPTH == 32 ****/

#define DEPTH 32
#define TILE_SIZE	8
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define blend blend32
#define blendbitmask blendbitmask32
#define DECLARE(function,args,body) static void function##8x8x32BPP args body
#include "tilemap.c"

#define TILE_SIZE	16
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define DECLARE(function,args,body) static void function##16x16x32BPP args body
#include "tilemap.c"

#define TILE_SIZE	32
#define DATA_TYPE UINT32
#define memcpybitmask memcpybitmask32
#define DECLARE(function,args,body) static void function##32x32x32BPP args body
#include "tilemap.c"
#undef DEPTH
#undef blend
#undef blendbitmask

/*********************************************************************************/

static void mask_dispose( struct tilemap_mask *mask )
{
	if( mask )
	{
		free( mask->data_row );
		free( mask->data );
		osd_free_bitmap( mask->bitmask );
		free( mask );
	}
}

static struct tilemap_mask *mask_create( struct tilemap *tilemap )
{
	int row;
	struct tilemap_mask *mask = malloc( sizeof(struct tilemap_mask) );
	if( mask )
	{
		mask->data = malloc( tilemap->num_tiles );
		mask->data_row = malloc( tilemap->num_cached_rows * sizeof(UINT8 *) );
		mask->bitmask = create_bitmask( tilemap->cached_width, tilemap->cached_height );
		if( mask->data && mask->data_row && mask->bitmask )
		{
			for( row=0; row<tilemap->num_cached_rows; row++ )
			{
				mask->data_row[row] = mask->data + row*tilemap->num_cached_cols;
			}
			mask->line_offset = mask->bitmask->line[1] - mask->bitmask->line[0];
			return mask;
		}
	}
	mask_dispose( mask );
	return NULL;
}

/***********************************************************************************/

static void install_draw_handlers( struct tilemap *tilemap )
{
	int tile_size = tilemap->tile_size;
	tilemap->draw = tilemap->draw_opaque = tilemap->draw_alpha = NULL;
	switch( Machine->scrbitmap->depth )
	{
	case 32:
		tilemap->draw_tile = draw_tile8x8x32BPP;

		if( tile_size==8 )
		{
			tilemap->draw = draw8x8x32BPP;
			tilemap->draw_opaque = draw_opaque8x8x32BPP;
			tilemap->draw_alpha = draw_alpha8x8x32BPP;
		}
		else if( tile_size==16 )
		{
			tilemap->draw = draw16x16x32BPP;
			tilemap->draw_opaque = draw_opaque16x16x32BPP;
			tilemap->draw_alpha = draw_alpha16x16x32BPP;
		}
		else if( tile_size==32 )
		{
			tilemap->draw = draw32x32x32BPP;
			tilemap->draw_opaque = draw_opaque32x32x32BPP;
			tilemap->draw_alpha = draw_alpha32x32x32BPP;
		}
		break;

	case 15:
	case 16:
		tilemap->draw_tile = draw_tile8x8x16BPP;

		if( tile_size==8 )
		{
			tilemap->draw = draw8x8x16BPP;
			tilemap->draw_opaque = draw_opaque8x8x16BPP;
			tilemap->draw_alpha = draw_alpha8x8x16BPP;
		}
		else if( tile_size==16 )
		{
			tilemap->draw = draw16x16x16BPP;
			tilemap->draw_opaque = draw_opaque16x16x16BPP;
			tilemap->draw_alpha = draw_alpha16x16x16BPP;
		}
		else if( tile_size==32 )
		{
			tilemap->draw = draw32x32x16BPP;
			tilemap->draw_opaque = draw_opaque32x32x16BPP;
			tilemap->draw_alpha = draw_alpha32x32x16BPP;
		}
		break;

	case 8:
		tilemap->draw_tile = draw_tile8x8x8BPP;

		if( tile_size==8 )
		{
			tilemap->draw = draw8x8x8BPP;
			tilemap->draw_opaque = draw_opaque8x8x8BPP;
			tilemap->draw_alpha = draw8x8x8BPP;
		}
		else if( tile_size==16 )
		{
			tilemap->draw = draw16x16x8BPP;
			tilemap->draw_opaque = draw_opaque16x16x8BPP;
			tilemap->draw_alpha = draw16x16x8BPP;
		}
		else if( tile_size==32 )
		{
			tilemap->draw = draw32x32x8BPP;
			tilemap->draw_opaque = draw_opaque32x32x8BPP;
			tilemap->draw_alpha = draw32x32x8BPP;
		}
		break;
	}
}

/***********************************************************************************/

static void tilemap_reset(void)
{
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

int tilemap_init( void )
{
	UINT32 value, data, bit;
	for( value=0; value<0x100; value++ )
	{
		data = 0;
		for( bit=0; bit<8; bit++ ) if( (value>>bit)&1 ) data |= 0x80>>bit;
		flip_bit_table[value] = data;
	}
	screen_width = Machine->scrbitmap->width;
	screen_height = Machine->scrbitmap->height;
	first_tilemap = 0;
	state_save_register_func_postload(tilemap_reset);
	priority_bitmap = create_tmpbitmap( screen_width, screen_height, 8 );
	if( priority_bitmap ){
		priority_bitmap_line_offset = priority_bitmap->line[1] - priority_bitmap->line[0];
		return 0;
	}
	return -1;
}

void tilemap_close( void )
{
	while( first_tilemap )
	{
		struct tilemap *next = first_tilemap->next;
		tilemap_dispose( first_tilemap );
		first_tilemap = next;
	}
	osd_free_bitmap( priority_bitmap );
}

/***********************************************************************************/

struct tilemap *tilemap_create(
	void (*tile_get_info)( int memory_offset ),
	UINT32 (*get_memory_offset)( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows ),
	int type,
	int tile_width, int tile_height,
	int num_cols, int num_rows )
{
	struct tilemap *tilemap;

	if( tile_width != tile_height )
	{
		logerror( "tilemap_create: tile_width must be equal to tile_height\n" );
		return 0;
	}

	tilemap = calloc( 1,sizeof( struct tilemap ) );
	if( tilemap )
	{
		int num_tiles = num_cols*num_rows;
		tilemap->num_logical_cols = num_cols;
		tilemap->num_logical_rows = num_rows;
		if( Machine->orientation & ORIENTATION_SWAP_XY )
		{
			SWAP( num_cols,num_rows )
		}
		tilemap->num_cached_cols = num_cols;
		tilemap->num_cached_rows = num_rows;
		tilemap->num_tiles = num_tiles;
		tilemap->num_pens = tile_width*tile_height;
		tilemap->tile_size = tile_width; /* tile_width and tile_height are equal */
		tilemap->cached_width = tile_width*num_cols;
		tilemap->cached_height = tile_height*num_rows;
		tilemap->tile_get_info = tile_get_info;
		tilemap->get_memory_offset = get_memory_offset;
		tilemap->orientation = Machine->orientation;

		/* various defaults */
		tilemap->enable = 1;
		tilemap->type = type;
		tilemap->scroll_rows = 1;
		tilemap->scroll_cols = 1;
		tilemap->transparent_pen = -1;
		tilemap->tile_depth = 0;
		tilemap->tile_granularity = 0;
		tilemap->tile_dirty_map = 0;

		tilemap->cached_tile_info = calloc( num_tiles, sizeof(struct cached_tile_info) );
		tilemap->priority = calloc( num_tiles,1 );
		tilemap->visible = calloc( num_tiles,1 );
		tilemap->dirty_vram = malloc( num_tiles );
		tilemap->dirty_pixels = malloc( num_tiles );
		tilemap->rowscroll = calloc(tilemap->cached_height,sizeof(int));
		tilemap->colscroll = calloc(tilemap->cached_width,sizeof(int));
		tilemap->priority_row = malloc( sizeof(UINT8 *)*num_rows );
		tilemap->pixmap = create_tmpbitmap( tilemap->cached_width, tilemap->cached_height, Machine->scrbitmap->depth );
		tilemap->foreground = mask_create( tilemap );
		tilemap->background = (type & TILEMAP_SPLIT)?mask_create( tilemap ):NULL;

		if( tilemap->cached_tile_info &&
			tilemap->priority && tilemap->visible &&
			tilemap->dirty_vram && tilemap->dirty_pixels &&
			tilemap->rowscroll && tilemap->colscroll &&
			tilemap->priority_row &&
			tilemap->pixmap && tilemap->foreground &&
			((type&TILEMAP_SPLIT)==0 || tilemap->background) &&
			(mappings_create( tilemap )==0) )
		{
			UINT32 row;
			for( row=0; row<num_rows; row++ ){
				tilemap->priority_row[row] = tilemap->priority+num_cols*row;
			}
			install_draw_handlers( tilemap );
			mappings_update( tilemap );
			tilemap_set_clip( tilemap, &Machine->visible_area );
			memset( tilemap->dirty_vram, 1, num_tiles );
			memset( tilemap->dirty_pixels, 1, num_tiles );
			tilemap->pixmap_line_offset = tilemap->pixmap->line[1] - tilemap->pixmap->line[0];
			tilemap->next = first_tilemap;
			first_tilemap = tilemap;
			if( PenToPixel_Init( tilemap ) == 0 )
			{
				return tilemap;
			}
		}
		tilemap_dispose( tilemap );
	}
	return 0;
}

void tilemap_dispose( struct tilemap *tilemap )
{
	struct tilemap *prev;

	if( tilemap==first_tilemap )
	{
		first_tilemap = tilemap->next;
	}
	else
	{
		prev = first_tilemap;
		while( prev->next != tilemap ) prev = prev->next;
		prev->next =tilemap->next;
	}
	PenToPixel_Term( tilemap );
	free( tilemap->cached_tile_info );
	free( tilemap->priority );
	free( tilemap->visible );
	free( tilemap->dirty_vram );
	free( tilemap->dirty_pixels );
	free( tilemap->rowscroll );
	free( tilemap->colscroll );
	free( tilemap->priority_row );
	osd_free_bitmap( tilemap->pixmap );
	mask_dispose( tilemap->foreground );
	mask_dispose( tilemap->background );
	mappings_dispose( tilemap );
	free( tilemap );
}

/***********************************************************************************/

static void unregister_pens( struct cached_tile_info *cached_tile_info, int num_pens )
{
	if( palette_used_colors )
	{
		const UINT32 *pal_data = cached_tile_info->pal_data;
		if( pal_data )
		{
			UINT32 pen_usage = cached_tile_info->pen_usage;
			if( pen_usage )
			{
				palette_decrease_usage_count(
					pal_data-Machine->remapped_colortable,
					pen_usage,
					PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
			}
			else
			{
				palette_decrease_usage_countx(
					pal_data-Machine->remapped_colortable,
					num_pens,
					cached_tile_info->pen_data,
					PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
			}
			cached_tile_info->pal_data = NULL;
		}
	}
}

static void register_pens( struct cached_tile_info *cached_tile_info, int num_pens )
{
	if (palette_used_colors)
	{
		UINT32 pen_usage = cached_tile_info->pen_usage;
		if( pen_usage )
		{
			palette_increase_usage_count(
				cached_tile_info->pal_data-Machine->remapped_colortable,
				pen_usage,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
		else
		{
			palette_increase_usage_countx(
				cached_tile_info->pal_data-Machine->remapped_colortable,
				num_pens,
				cached_tile_info->pen_data,
				PALETTE_COLOR_VISIBLE|PALETTE_COLOR_CACHED );
		}
	}
}

/***********************************************************************************/

void tilemap_set_enable( struct tilemap *tilemap, int enable )
{
	tilemap->enable = enable?1:0;
}

void tilemap_set_flip( struct tilemap *tilemap, int attributes )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_set_flip( tilemap, attributes );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->attributes!=attributes )
	{
		tilemap->attributes = attributes;
		tilemap->orientation = Machine->orientation;
		if( attributes&TILEMAP_FLIPY )
		{
			tilemap->orientation ^= ORIENTATION_FLIP_Y;
			tilemap->scrolly_delta = tilemap->dy_if_flipped;
		}
		else
		{
			tilemap->scrolly_delta = tilemap->dy;
		}
		if( attributes&TILEMAP_FLIPX )
		{
			tilemap->orientation ^= ORIENTATION_FLIP_X;
			tilemap->scrollx_delta = tilemap->dx_if_flipped;
		}
		else
		{
			tilemap->scrollx_delta = tilemap->dx;
		}

		mappings_update( tilemap );
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip )
{
	int left,top,right,bottom;
	if( clip )
	{
		left = clip->min_x;
		top = clip->min_y;
		right = clip->max_x+1;
		bottom = clip->max_y+1;
		if( tilemap->orientation & ORIENTATION_SWAP_XY )
		{
			SWAP(left,top)
			SWAP(right,bottom)
		}
		if( tilemap->orientation & ORIENTATION_FLIP_X )
		{
			SWAP(left,right)
			left = screen_width-left;
			right = screen_width-right;
		}
		if( tilemap->orientation & ORIENTATION_FLIP_Y )
		{
			SWAP(top,bottom)
			top = screen_height-top;
			bottom = screen_height-bottom;
		}
	}
	else
	{
		left = 0;
		top = 0;
		right = tilemap->cached_width;
		bottom = tilemap->cached_height;
	}
	tilemap->clip_left = left;
	tilemap->clip_right = right;
	tilemap->clip_top = top;
	tilemap->clip_bottom = bottom;
}

/***********************************************************************************/

void tilemap_set_scroll_cols( struct tilemap *tilemap, int n )
{
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if (tilemap->scroll_rows != n)
		{
			tilemap->scroll_rows = n;
		}
	}
	else
	{
		if (tilemap->scroll_cols != n)
		{
			tilemap->scroll_cols = n;
		}
	}
}

void tilemap_set_scroll_rows( struct tilemap *tilemap, int n )
{
	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if (tilemap->scroll_cols != n)
		{
			tilemap->scroll_cols = n;
		}
	}
	else
	{
		if (tilemap->scroll_rows != n)
		{
			tilemap->scroll_rows = n;
		}
	}
}

/***********************************************************************************/

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int memory_offset )
{
	if( memory_offset<tilemap->max_memory_offset )
	{
		int cached_indx = tilemap->memory_offset_to_cached_indx[memory_offset];
		if( cached_indx>=0 )
		{
			tilemap->dirty_vram[cached_indx] = 1;
		}
	}
}

void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_mark_all_tiles_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else
	{
		memset( tilemap->dirty_vram, 1, tilemap->num_tiles );
	}
}

static void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap )
{
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_mark_all_pixels_dirty( tilemap );
			tilemap = tilemap->next;
		}
	}
	else
	{
		/* invalidate all offscreen tiles */
		UINT32 cached_tile_indx;
		UINT32 num_pens = tilemap->tile_size*tilemap->tile_size;
		for( cached_tile_indx=0; cached_tile_indx<tilemap->num_tiles; cached_tile_indx++ )
		{
			if( !tilemap->visible[cached_tile_indx] )
			{
				unregister_pens( &tilemap->cached_tile_info[cached_tile_indx], num_pens );
				tilemap->dirty_vram[cached_tile_indx] = 1;
			}
		}
		memset( tilemap->dirty_pixels, 1, tilemap->num_tiles );
	}
}

void tilemap_dirty_palette( const UINT8 *dirty_pens )
{
	UINT32 *color_base = Machine->remapped_colortable;
	struct tilemap *tilemap = first_tilemap;
	while( tilemap )
	{
		if( !tilemap->tile_dirty_map)
			tilemap_mark_all_pixels_dirty( tilemap );
		else
		{
			UINT8 *dirty_map = tilemap->tile_dirty_map;
			int i, j, pen, row, col;
			int step = 1 << tilemap->tile_granularity;
			int count = 1 << tilemap->tile_depth;
			int limit = Machine->drv->total_colors - count;
			pen = 0;
			for( i=0; i<limit; i+=step )
			{
				for( j=0; j<count; j++ )
					if( dirty_pens[i+j] )
					{
						dirty_map[pen++] = 1;
						goto next;
					}
				dirty_map[pen++] = 0;
			next:
				;
			}

			i = 0;
			for( row=0; row<tilemap->num_cached_rows; row++ )
			{
				for( col=0; col<tilemap->num_cached_cols; col++ )
				{
					if (!tilemap->dirty_vram[i] && !tilemap->dirty_pixels[i])
					{
						struct cached_tile_info *cached_tile = tilemap->cached_tile_info+i;
						j = (cached_tile->pal_data - color_base) >> tilemap->tile_granularity;
						if( dirty_map[j] )
						{
							if( tilemap->visible[i] )
							{
								tilemap->draw_tile( tilemap, i, col, row );
							}
							else
							{
								tilemap->dirty_pixels[i] = 1;
							}
						}
					}
					i++;
				}
			}
		}
		tilemap = tilemap->next;
	}
}

/***********************************************************************************/

static void draw_bitmask(
		struct osd_bitmap *mask,
		UINT32 x0, UINT32 y0,
		UINT32 tile_size,
		const UINT8 *maskdata,
		UINT32 flags )
{
	UINT8 data;
	UINT8 *pDest;
	int x,sy,y1,y2,dy;

	if( flags&TILE_FLIPY )
	{
		y1 = y0+tile_size-1;
		y2 = y1-tile_size;
 		dy = -1;
 	}
 	else
 	{
		y1 = y0;
		y2 = y1+tile_size;
 		dy = 1;
 	}
	/* to do:
	 * 	support screen orientation here, so pre-rotate code can be removed from
	 *	namcos1,namcos2,namconb1
	 */
	if( flags&TILE_FLIPX )
	{
		tile_size--;
		for( sy=y1; sy!=y2; sy+=dy )
		{
			pDest = mask->line[sy]+x0/8;
			for( x=tile_size/8; x>=0; x-- )
			{
				data = flip_bit_table[*maskdata++];
				pDest[x] = data;
			}
		}
	}
	else
	{
		for( sy=y1; sy!=y2; sy+=dy )
		{
			pDest = mask->line[sy]+x0/8;
			for( x=0; x<tile_size/8; x++ )
			{
				data = *maskdata++;
				pDest[x] = data;
			}
		}
	}
}

static void draw_color_mask(
	struct tilemap *tilemap,
	struct osd_bitmap *mask,
	UINT32 x0, UINT32 y0,
	UINT32 tile_size,
	const UINT8 *pPenData,
	const UINT16 *clut,
	int transparent_color,
	UINT32 flags,
	int pitch )
{
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_SWAPXY|TILE_FLIPY|TILE_FLIPX)];
	int tx,ty;
	const UINT8 *pSource;
	UINT8 data;
	UINT32 yx;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size/2; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( clut[data&0xf]!=transparent_color )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
				yx = *pPenToPixel++;
				if( clut[data>>4]!=transparent_color )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( clut[data]!=transparent_color )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch;
		}
	}
}

static void draw_pen_mask(
	struct tilemap *tilemap,
	struct osd_bitmap *mask,
	UINT32 x0, UINT32 y0,
	UINT32 tile_size,
	const UINT8 *pPenData,
	int transparent_pen,
	UINT32 flags,
	int pitch )
{
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_SWAPXY|TILE_FLIPY|TILE_FLIPX)];
	int tx,ty;
	const UINT8 *pSource;
	UINT8 data;
	UINT32 yx;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size/2; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( (data&0xf)!=transparent_pen )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
				yx = *pPenToPixel++;
				if( (data>>4)!=transparent_pen )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( data!=transparent_pen )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch;
		}
	}
}

static void draw_mask(
	struct tilemap *tilemap,
	struct osd_bitmap *mask,
	UINT32 x0, UINT32 y0,
	UINT32 tile_size,
	const UINT8 *pPenData,
	UINT32 transmask,
	UINT32 flags,
	int pitch )
{
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_SWAPXY|TILE_FLIPY|TILE_FLIPX)];
	int tx,ty;
	const UINT8 *pSource;
	UINT8 data;
	UINT32 yx;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size/2; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( !((1<<(data&0xf))&transmask) )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
				yx = *pPenToPixel++;
				if( !((1<<(data>>4))&transmask) )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				if( !((1<<data)&transmask) )
				{
					mask->line[y0+yx/MAX_TILESIZE][(x0+(yx%MAX_TILESIZE))/8] |= 0x80>>(yx%8);
				}
			}
			pPenData += pitch;
		}
	}
}

static void ClearMask( struct osd_bitmap *bitmap, int tile_size, int x0, int y0 )
{
	UINT8 *pDest;
	int ty,tx;
	for( ty=0; ty<tile_size; ty++ )
	{
		pDest = bitmap->line[y0+ty]+x0/8;
		for( tx=tile_size/8; tx!=0; tx-- )
		{
			*pDest++ = 0x00;
		}
	}
}

static int InspectMask( struct osd_bitmap *bitmap, int tile_size, int x0, int y0 )
{
	const UINT8 *pSource;
	int ty,tx;

	switch( bitmap->line[y0][x0/8] )
	{
	case 0xff: /* possibly opaque */
		for( ty=0; ty<tile_size; ty++ )
		{
			pSource = bitmap->line[y0+ty]+x0/8;
			for( tx=tile_size/8; tx!=0; tx-- )
			{
				if( *pSource++ != 0xff ) return TILE_MASKED;
			}
		}
		return TILE_OPAQUE;

	case 0x00: /* possibly transparent */
		for( ty=0; ty<tile_size; ty++ )
		{
			pSource = bitmap->line[y0+ty]+x0/8;
			for( tx=tile_size/8; tx!=0; tx-- )
			{
				if( *pSource++ != 0x00 ) return TILE_MASKED;
			}
		}
		return TILE_TRANSPARENT;

	default:
		return TILE_MASKED;
	}
}

static void render_mask( struct tilemap *tilemap, UINT32 cached_indx )
{
	const struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
	UINT32 col = cached_indx%tilemap->num_cached_cols;
	UINT32 row = cached_indx/tilemap->num_cached_cols;
	UINT32 type = tilemap->type;
	UINT32 tile_size = tilemap->tile_size;
	UINT32 y0 = tile_size*row;
	UINT32 x0 = tile_size*col;
	int pitch = tile_size + cached_tile_info->skip;
	UINT32 pen_usage = cached_tile_info->pen_usage;
	const UINT8 *pen_data = cached_tile_info->pen_data;
	UINT32 flags = cached_tile_info->flags;

	if( type & TILEMAP_BITMASK )
	{
		/* hack; games using TILEMAP_BITMASK may pass in NULL or (~0) to indicate
		 * tiles that are wholly transparent or opaque.
		 */
		if( tile_info.mask_data == TILEMAP_BITMASK_TRANSPARENT )
		{
			tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
		}
		else if( tile_info.mask_data == TILEMAP_BITMASK_OPAQUE )
		{
			tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
		}
		else
		{
			/* We still inspect the tile data, since not all games
			 * using TILEMAP_BITMASK use the above hack.
			 */
			draw_bitmask( tilemap->foreground->bitmask,
				x0, y0, tile_size, tile_info.mask_data, flags );

			tilemap->foreground->data_row[row][col] =
				InspectMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
		}
	}
	else if( type & TILEMAP_SPLIT )
	{
		UINT32 fgmask = tilemap->fgmask[(flags>>TILE_SPLIT_OFFSET)&3];
		UINT32 bgmask = tilemap->bgmask[(flags>>TILE_SPLIT_OFFSET)&3];

		if( (pen_usage & fgmask)==0 || (flags&TILE_IGNORE_TRANSPARENCY) )
		{ /* foreground totally opaque */
			tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
		}
		else if( (pen_usage & ~fgmask)==0 )
		{ /* foreground transparent */
			ClearMask( tilemap->background->bitmask, tile_size, x0, y0 );
			draw_mask( tilemap,tilemap->background->bitmask,
				x0, y0, tile_size, pen_data, bgmask, flags, pitch );
			tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
		}
		else
		{ /* masked tile */
			ClearMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
			draw_mask( tilemap,tilemap->foreground->bitmask,
				x0, y0, tile_size, pen_data, fgmask, flags, pitch );
			tilemap->foreground->data_row[row][col] = TILE_MASKED;
		}

		if( (pen_usage & bgmask)==0 || (flags&TILE_IGNORE_TRANSPARENCY) )
		{ /* background totally opaque */
			tilemap->background->data_row[row][col] = TILE_OPAQUE;
		}
		else if( (pen_usage & ~bgmask)==0 )
		{ /* background transparent */
			ClearMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
			draw_mask( tilemap,tilemap->foreground->bitmask,
				x0, y0, tile_size, pen_data, fgmask, flags, pitch );
				tilemap->foreground->data_row[row][col] = TILE_MASKED;
			tilemap->background->data_row[row][col] = TILE_TRANSPARENT;
		}
		else
		{ /* masked tile */
			ClearMask( tilemap->background->bitmask, tile_size, x0, y0 );
			draw_mask( tilemap,tilemap->background->bitmask,
				x0, y0, tile_size, pen_data, bgmask, flags, pitch );
			tilemap->background->data_row[row][col] = TILE_MASKED;
		}
	}
	else if( type==TILEMAP_TRANSPARENT )
	{
		if( pen_usage )
		{
			UINT32 fgmask = 1 << tilemap->transparent_pen;
		 	if( flags&TILE_IGNORE_TRANSPARENCY ) fgmask = 0;
			if( pen_usage == fgmask )
			{
				tilemap->foreground->data_row[row][col] = TILE_TRANSPARENT;
			}
			else if( pen_usage & fgmask )
			{
				ClearMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
				draw_mask( tilemap,tilemap->foreground->bitmask,
					x0, y0, tile_size, pen_data, fgmask, flags, pitch );
				tilemap->foreground->data_row[row][col] = TILE_MASKED;
			}
			else
			{
				tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
			}
		}
		else
		{
			ClearMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
			draw_pen_mask(
					tilemap,tilemap->foreground->bitmask,
					x0, y0, tile_size, pen_data, tilemap->transparent_pen, flags, pitch );
			tilemap->foreground->data_row[row][col] =
				InspectMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
		}
	}
	else if( type==TILEMAP_TRANSPARENT_COLOR )
	{
		ClearMask( tilemap->foreground->bitmask, tile_size, x0, y0 );

		draw_color_mask(
				tilemap,tilemap->foreground->bitmask,
				x0, y0, tile_size, pen_data,
				Machine->game_colortable + (cached_tile_info->pal_data - Machine->remapped_colortable),
				tilemap->transparent_pen, flags, pitch );

		tilemap->foreground->data_row[row][col] =
				InspectMask( tilemap->foreground->bitmask, tile_size, x0, y0 );
	}
	else
	{
		tilemap->foreground->data_row[row][col] = TILE_OPAQUE;
	}
}

static void update_tile_info( struct tilemap *tilemap )
{
	int *logical_flip_to_cached_flip = tilemap->logical_flip_to_cached_flip;
	UINT32 num_pens = tilemap->tile_size * tilemap->tile_size;
	UINT32 num_tiles = tilemap->num_tiles;
	UINT32 cached_indx;
	UINT8 *visible = tilemap->visible;
	UINT8 *dirty_vram = tilemap->dirty_vram;
	UINT8 *dirty_pixels = tilemap->dirty_pixels;

	memset( &tile_info, 0x00, sizeof(tile_info) ); /* initialize defaults */

	for( cached_indx=0; cached_indx<num_tiles; cached_indx++ )
	{
		if( visible[cached_indx] && dirty_vram[cached_indx] )
		{
			struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
			UINT32 memory_offset = tilemap->cached_indx_to_memory_offset[cached_indx];
			unregister_pens( cached_tile_info, num_pens );
			tilemap->tile_get_info( memory_offset );
			{
				UINT32 flags = tile_info.flags;
				cached_tile_info->flags = (flags&0xfc)|logical_flip_to_cached_flip[flags&0x3];
			}
			cached_tile_info->pen_usage = tile_info.pen_usage;
			cached_tile_info->pen_data = tile_info.pen_data;
			cached_tile_info->pal_data = tile_info.pal_data;
			cached_tile_info->skip = tile_info.skip;
			tilemap->priority[cached_indx] = tile_info.priority;
			register_pens( cached_tile_info, num_pens );
			dirty_pixels[cached_indx] = 1;
			dirty_vram[cached_indx] = 0;
			render_mask( tilemap, cached_indx );
		}
	}
}

static void update_visible( struct tilemap *tilemap )
{
	// visibility marking is not currently implemented
	memset( tilemap->visible, 1, tilemap->num_tiles );
}

void tilemap_update( struct tilemap *tilemap )
{
profiler_mark(PROFILER_TILEMAP_UPDATE);
	if( tilemap==ALL_TILEMAPS )
	{
		tilemap = first_tilemap;
		while( tilemap )
		{
			tilemap_update( tilemap );
			tilemap = tilemap->next;
		}
	}
	else if( tilemap->enable )
	{
		tilemap->bNeedRender = 1;
		update_visible( tilemap );
		update_tile_info( tilemap );
	}
profiler_mark(PROFILER_END);
}

/***********************************************************************************/

void tilemap_set_scrolldx( struct tilemap *tilemap, int dx, int dx_if_flipped )
{
	tilemap->dx = dx;
	tilemap->dx_if_flipped = dx_if_flipped;
	tilemap->scrollx_delta = ( tilemap->attributes & TILEMAP_FLIPX )?dx_if_flipped:dx;
}

void tilemap_set_scrolldy( struct tilemap *tilemap, int dy, int dy_if_flipped )
{
	tilemap->dy = dy;
	tilemap->dy_if_flipped = dy_if_flipped;
	tilemap->scrolly_delta = ( tilemap->attributes & TILEMAP_FLIPY )?dy_if_flipped:dy;
}

void tilemap_set_scrollx( struct tilemap *tilemap, int which, int value )
{
	value = tilemap->scrollx_delta-value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value )
		{
			tilemap->colscroll[which] = value;
		}
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value )
		{
			tilemap->rowscroll[which] = value;
		}
	}
}

void tilemap_set_scrolly( struct tilemap *tilemap, int which, int value )
{
	value = tilemap->scrolly_delta - value;

	if( tilemap->orientation & ORIENTATION_SWAP_XY )
	{
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) which = tilemap->scroll_rows-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_X ) value = screen_width-tilemap->cached_width-value;
		if( tilemap->rowscroll[which]!=value )
		{
			tilemap->rowscroll[which] = value;
		}
	}
	else
	{
		if( tilemap->orientation & ORIENTATION_FLIP_X ) which = tilemap->scroll_cols-1 - which;
		if( tilemap->orientation & ORIENTATION_FLIP_Y ) value = screen_height-tilemap->cached_height-value;
		if( tilemap->colscroll[which]!=value )
		{
			tilemap->colscroll[which] = value;
		}
	}
}
/***********************************************************************************/

void tilemap_draw( struct osd_bitmap *dest, struct tilemap *tilemap, UINT32 flags, UINT32 priority )
{
	int xpos,ypos;
profiler_mark(PROFILER_TILEMAP_DRAW);
	tmap_render( tilemap );

	if( tilemap->enable )
	{
		void (*draw)( int, int );

		int rows = tilemap->scroll_rows;
		const int *rowscroll = tilemap->rowscroll;
		int cols = tilemap->scroll_cols;
		const int *colscroll = tilemap->colscroll;

		int left = tilemap->clip_left;
		int right = tilemap->clip_right;
		int top = tilemap->clip_top;
		int bottom = tilemap->clip_bottom;

		int tile_size = tilemap->tile_size;

		blit.screen = dest;
		if( dest )
		{
			blit.pixmap = tilemap->pixmap;
			blit.source_line_offset = tilemap->pixmap_line_offset;

			blit.dest_line_offset = dest->line[1] - dest->line[0];

			switch( dest->depth )
			{
			case 15:
			case 16:
				blit.dest_line_offset /= 2;
				blit.source_line_offset /= 2;
				break;
			case 32:
				blit.dest_line_offset /= 4;
				blit.source_line_offset /= 4;
				break;
			}
			blit.dest_row_offset = tile_size*blit.dest_line_offset;
		}


		if( tilemap->type==TILEMAP_OPAQUE || (flags&TILEMAP_IGNORE_TRANSPARENCY) )
		{
			draw = tilemap->draw_opaque;
		}
		else
		{
			if( flags & TILEMAP_ALPHA )
				draw = tilemap->draw_alpha;
			else
				draw = tilemap->draw;

			if( flags&TILEMAP_BACK )
			{
				blit.bitmask = tilemap->background->bitmask;
				blit.mask_line_offset = tilemap->background->line_offset;
				blit.mask_data_row = tilemap->background->data_row;
			}
			else
			{
				blit.bitmask = tilemap->foreground->bitmask;
				blit.mask_line_offset = tilemap->foreground->line_offset;
				blit.mask_data_row = tilemap->foreground->data_row;
			}

			blit.mask_row_offset = tile_size*blit.mask_line_offset;
		}

		blit.source_row_offset = tile_size*blit.source_line_offset;

		blit.priority_data_row = tilemap->priority_row;
		blit.source_width = tilemap->cached_width;
		blit.source_height = tilemap->cached_height;
		blit.tile_priority = flags&0xf;
		blit.tilemap_priority_code = priority;

		if( rows == 1 && cols == 1 )
		{ /* XY scrolling playfield */
			int scrollx = rowscroll[0];
			int scrolly = colscroll[0];

			if( scrollx < 0 )
			{
				scrollx = blit.source_width - (-scrollx) % blit.source_width;
			}
			else
			{
				scrollx = scrollx % blit.source_width;
			}

			if( scrolly < 0 )
			{
				scrolly = blit.source_height - (-scrolly) % blit.source_height;
			}
			else
			{
				scrolly = scrolly % blit.source_height;
			}

	 		blit.clip_left = left;
	 		blit.clip_top = top;
	 		blit.clip_right = right;
	 		blit.clip_bottom = bottom;

			for(
				ypos = scrolly - blit.source_height;
				ypos < blit.clip_bottom;
				ypos += blit.source_height )
			{
				for(
					xpos = scrollx - blit.source_width;
					xpos < blit.clip_right;
					xpos += blit.source_width )
				{
					draw( xpos,ypos );
				}
			}
		}
		else if( rows == 1 )
		{ /* scrolling columns + horizontal scroll */
			int col = 0;
			int colwidth = blit.source_width / cols;
			int scrollx = rowscroll[0];

			if( scrollx < 0 )
			{
				scrollx = blit.source_width - (-scrollx) % blit.source_width;
			}
			else
			{
				scrollx = scrollx % blit.source_width;
			}

			blit.clip_top = top;
			blit.clip_bottom = bottom;

			while( col < cols )
			{
				int cons = 1;
				int scrolly = colscroll[col];

	 			/* count consecutive columns scrolled by the same amount */
				if( scrolly != TILE_LINE_DISABLED )
				{
					while( col + cons < cols &&	colscroll[col + cons] == scrolly ) cons++;

					if( scrolly < 0 )
					{
						scrolly = blit.source_height - (-scrolly) % blit.source_height;
					}
					else
					{
						scrolly %= blit.source_height;
					}

					blit.clip_left = col * colwidth + scrollx;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - blit.source_height;
						ypos < blit.clip_bottom;
						ypos += blit.source_height )
					{
						draw( scrollx,ypos );
					}

					blit.clip_left = col * colwidth + scrollx - blit.source_width;
					if (blit.clip_left < left) blit.clip_left = left;
					blit.clip_right = (col + cons) * colwidth + scrollx - blit.source_width;
					if (blit.clip_right > right) blit.clip_right = right;

					for(
						ypos = scrolly - blit.source_height;
						ypos < blit.clip_bottom;
						ypos += blit.source_height )
					{
						draw( scrollx - blit.source_width,ypos );
					}
				}
				col += cons;
			}
		}
		else if( cols == 1 )
		{ /* scrolling rows + vertical scroll */
			int row = 0;
			int rowheight = blit.source_height / rows;
			int scrolly = colscroll[0];
			if( scrolly < 0 )
			{
				scrolly = blit.source_height - (-scrolly) % blit.source_height;
			}
			else
			{
				scrolly = scrolly % blit.source_height;
			}
			blit.clip_left = left;
			blit.clip_right = right;
			while( row < rows )
			{
				int cons = 1;
				int scrollx = rowscroll[row];
				/* count consecutive rows scrolled by the same amount */
				if( scrollx != TILE_LINE_DISABLED )
				{
					while( row + cons < rows &&	rowscroll[row + cons] == scrollx ) cons++;
					if( scrollx < 0)
					{
						scrollx = blit.source_width - (-scrollx) % blit.source_width;
					}
					else
					{
						scrollx %= blit.source_width;
					}
					blit.clip_top = row * rowheight + scrolly;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - blit.source_width;
						xpos < blit.clip_right;
						xpos += blit.source_width )
					{
						draw( xpos,scrolly );
					}
					blit.clip_top = row * rowheight + scrolly - blit.source_height;
					if (blit.clip_top < top) blit.clip_top = top;
					blit.clip_bottom = (row + cons) * rowheight + scrolly - blit.source_height;
					if (blit.clip_bottom > bottom) blit.clip_bottom = bottom;
					for(
						xpos = scrollx - blit.source_width;
						xpos < blit.clip_right;
						xpos += blit.source_width )
					{
						draw( xpos,scrolly - blit.source_height );
					}
				}
				row += cons;
			}
		}
	}
profiler_mark(PROFILER_END);
}

/***********************************************************************************/

#else // DECLARE
/*
	The following procedure body is #included several times by
	tilemap.c to implement a suite of tilemap_draw subroutines.

	The constant TILE_SIZE is different in each instance of this
	code, allowing arithmetic shifts to be used by the compiler
	instead of multiplies/divides.

	This routine should be fairly optimal, for C code, though of
	course there is room for improvement.

	It renders pixels one row at a time, skipping over runs of totally
	transparent tiles, and calling custom blitters to handle runs of
	masked/totally opaque tiles.
*/
DECLARE( draw, (int xpos, int ypos),
{
	struct osd_bitmap *screen = blit.screen;
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;
	DATA_TYPE *dest_baseaddr = NULL;
	DATA_TYPE *dest_next;
	int dy;
	int count;
	const DATA_TYPE *source0;
	DATA_TYPE *dest0;
	UINT8 *pmap0;
	int i;
	int row;
	UINT8 *priority_data;
	int tile_type;
	int prev_tile_type;
	int x_start;
	int x_end;
	int column;
	int c1;
	int c2; /* leftmost and rightmost visible columns in source tilemap */
	int y; /* current screen line to render */
	int y_next;
	int num_pixels;
	int priority_bitmap_row_offset;
	UINT8 *priority_bitmap_baseaddr;
	UINT8 *priority_bitmap_next;
	int priority;
	const DATA_TYPE *source_baseaddr;
	const DATA_TYPE *source_next;
	const UINT8 *mask0;
	UINT8 *mask_data;
	const UINT8 *mask_baseaddr;
	const UINT8 *mask_next;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 )
	{ /* do nothing if totally clipped */
		priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_SIZE;
		priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		priority = blit.tile_priority;
		if( screen )
		{
			dest_baseaddr = xpos + (DATA_TYPE *)screen->line[y1];
		}

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];
		mask_baseaddr = blit.bitmask->line[y1];

		c1 = x1/TILE_SIZE; /* round down */
		c2 = (x2+TILE_SIZE-1)/TILE_SIZE; /* round up */

		y = y1;
		y_next = TILE_SIZE*(y1/TILE_SIZE) + TILE_SIZE;
		if( y_next>y2 ) y_next = y2;

		dy = y_next-y;
		dest_next = dest_baseaddr + dy*blit.dest_line_offset;
		priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_line_offset;
		source_next = source_baseaddr + dy*blit.source_line_offset;
		mask_next = mask_baseaddr + dy*blit.mask_line_offset;
		for(;;)
		{
			row = y/TILE_SIZE;
			mask_data = blit.mask_data_row[row];
			priority_data = blit.priority_data_row[row];
			prev_tile_type = TILE_TRANSPARENT;
			x_start = x1;

			for( column=c1; column<=c2; column++ )
			{
				if( column==c2 || priority_data[column]!=priority )
				{
					tile_type = TILE_TRANSPARENT;
				}
				else
				{
					tile_type = mask_data[column];
				}

				if( tile_type!=prev_tile_type )
				{
					x_end = column*TILE_SIZE;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT )
					{
						if( prev_tile_type == TILE_MASKED )
						{
							count = (x_end+7)/8 - x_start/8;
							mask0 = mask_baseaddr + x_start/8;
							source0 = source_baseaddr + (x_start&0xfff8);
							dest0 = dest_baseaddr + (x_start&0xfff8);
							pmap0 = priority_bitmap_baseaddr + (x_start&0xfff8);
							i = y;
							for(;;)
							{
								if( screen ) memcpybitmask( dest0, source0, mask0, count );
								memsetbitmask8( pmap0, tilemap_priority_code, mask0, count );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								mask0 += blit.mask_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
						else
						{ /* TILE_OPAQUE */
							num_pixels = x_end - x_start;
							dest0 = dest_baseaddr+x_start;
							source0 = source_baseaddr+x_start;
							pmap0 = priority_bitmap_baseaddr + x_start;
							i = y;
							for(;;)
							{
								if( screen ) memcpy( dest0, source0, num_pixels*sizeof(DATA_TYPE) );
								memset( pmap0, tilemap_priority_code, num_pixels );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;
			y = y_next;
			y_next += TILE_SIZE;

			if( y_next>=y2 )
			{
				y_next = y2;
			}
			else
			{
				dest_next += blit.dest_row_offset;
				priority_bitmap_next += priority_bitmap_row_offset;
				source_next += blit.source_row_offset;
				mask_next += blit.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})

DECLARE( draw_opaque, (int xpos, int ypos),
{
	struct osd_bitmap *screen = blit.screen;
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;
	DATA_TYPE *dest_baseaddr = NULL;
	DATA_TYPE *dest_next;
	int dy;
//	int count;
	const DATA_TYPE *source0;
	DATA_TYPE *dest0;
	UINT8 *pmap0;
	int i;
	int row;
	UINT8 *priority_data;
	int tile_type;
	int prev_tile_type;
	int x_start;
	int x_end;
	int column;
	int c1;
	int c2; /* leftmost and rightmost visible columns in source tilemap */
	int y; /* current screen line to render */
	int y_next;
	int num_pixels;
	int priority_bitmap_row_offset;
	UINT8 *priority_bitmap_baseaddr;
	UINT8 *priority_bitmap_next;
	int priority;
	const DATA_TYPE *source_baseaddr;
	const DATA_TYPE *source_next;

//	const UINT8 *mask0;
//	UINT8 *mask_data;
//	const UINT8 *mask_baseaddr;
//	const UINT8 *mask_next;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 )
	{ /* do nothing if totally clipped */
		priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_SIZE;
		priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		priority = blit.tile_priority;
		if( screen )
		{
			dest_baseaddr = xpos + (DATA_TYPE *)screen->line[y1];
		}

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];
//		mask_baseaddr = blit.bitmask->line[y1];

		c1 = x1/TILE_SIZE; /* round down */
		c2 = (x2+TILE_SIZE-1)/TILE_SIZE; /* round up */

		y = y1;
		y_next = TILE_SIZE*(y1/TILE_SIZE) + TILE_SIZE;
		if( y_next>y2 ) y_next = y2;

		dy = y_next-y;
		dest_next = dest_baseaddr + dy*blit.dest_line_offset;
		priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_line_offset;
		source_next = source_baseaddr + dy*blit.source_line_offset;
//		mask_next = mask_baseaddr + dy*blit.mask_line_offset;
		for(;;)
		{
			row = y/TILE_SIZE;
//			mask_data = blit.mask_data_row[row];
			priority_data = blit.priority_data_row[row];
			prev_tile_type = TILE_TRANSPARENT;
			x_start = x1;

			for( column=c1; column<=c2; column++ )
			{
				if( column==c2 || priority_data[column]!=priority )
				{
					tile_type = TILE_TRANSPARENT;
				}
				else
				{
//					tile_type = mask_data[column];
					tile_type = TILE_OPAQUE;
				}

				if( tile_type!=prev_tile_type )
				{
					x_end = column*TILE_SIZE;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT )
					{
//						if( prev_tile_type == TILE_MASKED )
//						{
//							count = (x_end+7)/8 - x_start/8;
//							mask0 = mask_baseaddr + x_start/8;
//							source0 = source_baseaddr + (x_start&0xfff8);
//							dest0 = dest_baseaddr + (x_start&0xfff8);
//							pmap0 = priority_bitmap_baseaddr + (x_start&0xfff8);
//							i = y;
//							for(;;)
//							{
//								if( screen ) memcpybitmask( dest0, source0, mask0, count );
//								memsetbitmask8( pmap0, tilemap_priority_code, mask0, count );
//								if( ++i == y_next ) break;
//
//								dest0 += blit.dest_line_offset;
//								source0 += blit.source_line_offset;
//								mask0 += blit.mask_line_offset;
//								pmap0 += priority_bitmap_line_offset;
//							}
//						}
//						else
						{ /* TILE_OPAQUE */
							num_pixels = x_end - x_start;
							dest0 = dest_baseaddr+x_start;
							source0 = source_baseaddr+x_start;
							pmap0 = priority_bitmap_baseaddr + x_start;
							i = y;
							for(;;)
							{
								if( screen ) memcpy( dest0, source0, num_pixels*sizeof(DATA_TYPE) );
								memset( pmap0, tilemap_priority_code, num_pixels );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
//			mask_baseaddr = mask_next;
			y = y_next;
			y_next += TILE_SIZE;

			if( y_next>=y2 )
			{
				y_next = y2;
			}
			else
			{
				dest_next += blit.dest_row_offset;
				priority_bitmap_next += priority_bitmap_row_offset;
				source_next += blit.source_row_offset;
//				mask_next += blit.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})
#undef IGNORE_TRANSPARENCY

#if DEPTH >= 16
DECLARE( draw_alpha, (int xpos, int ypos),
{
	int tilemap_priority_code = blit.tilemap_priority_code;
	int x1 = xpos;
	int y1 = ypos;
	int x2 = xpos+blit.source_width;
	int y2 = ypos+blit.source_height;

	/* clip source coordinates */
	if( x1<blit.clip_left ) x1 = blit.clip_left;
	if( x2>blit.clip_right ) x2 = blit.clip_right;
	if( y1<blit.clip_top ) y1 = blit.clip_top;
	if( y2>blit.clip_bottom ) y2 = blit.clip_bottom;

	if( x1<x2 && y1<y2 )
	{ /* do nothing if totally clipped */
		DATA_TYPE *dest_baseaddr = xpos + (DATA_TYPE *)blit.screen->line[y1];
		DATA_TYPE *dest_next;

		int priority_bitmap_row_offset = priority_bitmap_line_offset*TILE_SIZE;
		UINT8 *priority_bitmap_baseaddr = xpos + (UINT8 *)priority_bitmap->line[y1];
		UINT8 *priority_bitmap_next;

		int priority = blit.tile_priority;
		const DATA_TYPE *source_baseaddr;
		const DATA_TYPE *source_next;
		const UINT8 *mask_baseaddr;
		const UINT8 *mask_next;

		int c1;
		int c2; /* leftmost and rightmost visible columns in source tilemap */
		int y; /* current screen line to render */
		int y_next;

		/* convert screen coordinates to source tilemap coordinates */
		x1 -= xpos;
		y1 -= ypos;
		x2 -= xpos;
		y2 -= ypos;

		source_baseaddr = (DATA_TYPE *)blit.pixmap->line[y1];
		mask_baseaddr = blit.bitmask->line[y1];

		c1 = x1/TILE_SIZE; /* round down */
		c2 = (x2+TILE_SIZE-1)/TILE_SIZE; /* round up */

		y = y1;
		y_next = TILE_SIZE*(y1/TILE_SIZE) + TILE_SIZE;
		if( y_next>y2 ) y_next = y2;

		{
			int dy = y_next-y;
			dest_next = dest_baseaddr + dy*blit.dest_line_offset;
			priority_bitmap_next = priority_bitmap_baseaddr + dy*priority_bitmap_line_offset;
			source_next = source_baseaddr + dy*blit.source_line_offset;
			mask_next = mask_baseaddr + dy*blit.mask_line_offset;
		}

		for(;;)
		{
			int row = y/TILE_SIZE;
			UINT8 *mask_data = blit.mask_data_row[row];
			UINT8 *priority_data = blit.priority_data_row[row];

			int tile_type;
			int prev_tile_type = TILE_TRANSPARENT;

			int x_start = x1;
			int x_end;

			int column;
			for( column=c1; column<=c2; column++ )
			{
				if( column==c2 || priority_data[column]!=priority )
					tile_type = TILE_TRANSPARENT;
				else
					tile_type = mask_data[column];

				if( tile_type!=prev_tile_type )
				{
					x_end = column*TILE_SIZE;
					if( x_end<x1 ) x_end = x1;
					if( x_end>x2 ) x_end = x2;

					if( prev_tile_type != TILE_TRANSPARENT )
					{
						if( prev_tile_type == TILE_MASKED )
						{
							int count = (x_end+7)/8 - x_start/8;
							const UINT8 *mask0 = mask_baseaddr + x_start/8;
							const DATA_TYPE *source0 = source_baseaddr + (x_start&0xfff8);
							DATA_TYPE *dest0 = dest_baseaddr + (x_start&0xfff8);
							UINT8 *pmap0 = priority_bitmap_baseaddr + (x_start&0xfff8);
							int i = y;
							for(;;)
							{
								blendbitmask( dest0, source0, mask0, count );
								memsetbitmask8( pmap0, tilemap_priority_code, mask0, count );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								mask0 += blit.mask_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
						else
						{ /* TILE_OPAQUE */
							int num_pixels = x_end - x_start;
							DATA_TYPE *dest0 = dest_baseaddr+x_start;
							const DATA_TYPE *source0 = source_baseaddr+x_start;
							UINT8 *pmap0 = priority_bitmap_baseaddr + x_start;
							int i = y;
							for(;;)
							{
								blend( dest0, source0, num_pixels );
								memset( pmap0, tilemap_priority_code, num_pixels );
								if( ++i == y_next ) break;

								dest0 += blit.dest_line_offset;
								source0 += blit.source_line_offset;
								pmap0 += priority_bitmap_line_offset;
							}
						}
					}
					x_start = x_end;
				}

				prev_tile_type = tile_type;
			}

			if( y_next==y2 ) break; /* we are done! */

			priority_bitmap_baseaddr = priority_bitmap_next;
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;

			y = y_next;
			y_next += TILE_SIZE;

			if( y_next>=y2 )
			{
				y_next = y2;
			}
			else
			{
				dest_next += blit.dest_row_offset;
				priority_bitmap_next += priority_bitmap_row_offset;
				source_next += blit.source_row_offset;
				mask_next += blit.mask_row_offset;
			}
		} /* process next row */
	} /* not totally clipped */
})
#endif

#if (TILE_SIZE == 8) /* only construct once for each depth */
DECLARE( draw_tile, (struct tilemap *tilemap, UINT32 cached_indx, UINT32 col, UINT32 row ),
{
	struct cached_tile_info *cached_tile_info = &tilemap->cached_tile_info[cached_indx];
	UINT32 tile_size = tilemap->tile_size;
	const UINT8 *pPenData = cached_tile_info->pen_data;
	int pitch = tile_size + cached_tile_info->skip;
	const UINT32 *pPalData = cached_tile_info->pal_data;
	UINT32 flags = cached_tile_info->flags;
	UINT32 y0 = tile_size*row;
	UINT32 x0 = tile_size*col;
	struct osd_bitmap *pPixmap = tilemap->pixmap;
	UINT32 *pPenToPixel = tilemap->pPenToPixel[flags&(TILE_SWAPXY|TILE_FLIPY|TILE_FLIPX)];
	int tx;
	int ty;
	const UINT8 *pSource;
	UINT8 data;
	UINT32 yx;

	if( flags&TILE_4BPP )
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size/2; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				*(x0+(yx%MAX_TILESIZE)+(DATA_TYPE *)pPixmap->line[y0+yx/MAX_TILESIZE]) = pPalData[data&0xf];
				yx = *pPenToPixel++;
				*(x0+(yx%MAX_TILESIZE)+(DATA_TYPE *)pPixmap->line[y0+yx/MAX_TILESIZE]) = pPalData[data>>4];
			}
			pPenData += pitch/2;
		}
	}
	else
	{
		for( ty=tile_size; ty!=0; ty-- )
		{
			pSource = pPenData;
			for( tx=tile_size; tx!=0; tx-- )
			{
				data = *pSource++;
				yx = *pPenToPixel++;
				*(x0+(yx%MAX_TILESIZE)+(DATA_TYPE *)pPixmap->line[y0+yx/MAX_TILESIZE]) = pPalData[data];
			}
			pPenData += pitch;
		}
	}
})

#endif /* draw_tile */

#undef TILE_SIZE
#undef DATA_TYPE
#undef memcpybitmask
#undef DECLARE

#endif /* DECLARE */
