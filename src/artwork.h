/*********************************************************************

  artwork.h

  Generic backdrop/overlay functions.

  Be sure to include driver.h before including this file.

  Created by Mike Balfour - 10/01/1998
*********************************************************************/

#ifndef ARTWORK_H

#define ARTWORK_H 1

/*********************************************************************
  artwork

  This structure is a generic structure used to hold both backdrops
  and overlays.
*********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

struct artwork_info
{
	struct mame_bitmap *artwork;
	struct mame_bitmap *artwork1;
	struct mame_bitmap *alpha;
	struct mame_bitmap *orig_artwork;   /* needed for palette recalcs */
	int start_pen;
	UINT32 *rgb;
};


struct artwork_element
{
	struct rectangle box;
	UINT8 red,green,blue;
	UINT16 alpha;   /* 0x00-0xff or OVERLAY_DEFAULT_OPACITY */
};

struct artwork_size_info
{
	int width, height;         /* widht and height of the artwork */
	struct rectangle screen;   /* location of the screen relative to the artwork */
};

#define OVERLAY_DEFAULT_OPACITY         0xffff

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

extern struct artwork_info *artwork_backdrop;
extern struct artwork_info *artwork_overlay;
extern struct mame_bitmap *artwork_real_scrbitmap;

/*********************************************************************
  functions that apply to backdrops AND overlays
*********************************************************************/
void overlay_load(const char *filename, unsigned int start_pen);
void overlay_create(const struct artwork_element *ae, unsigned int start_pen);
void backdrop_load(const char *filename, unsigned int start_pen);
/*void backdrop_refresh(struct artwork_info *a); */
void artwork_load(struct artwork_info **a,const char *filename, unsigned int start_pen);
void artwork_load_size(struct artwork_info **a,const char *filename, unsigned int start_pen, int width, int height);
void artwork_elements_scale(struct artwork_element *ae, int width, int height);
void artwork_free(struct artwork_info **a);
int artwork_get_size_info(const char *file_name, struct artwork_size_info *a);

/* called by mame.c */
void artwork_kill(void);
void artwork_draw(struct mame_bitmap *dest,struct mame_bitmap *source, int full_refresh);

#ifdef __cplusplus
}
#endif

#endif
