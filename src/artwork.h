/*********************************************************************

	artwork.h

	Second generation artwork implementation.

*********************************************************************/

#ifndef ARTWORK_H
#define ARTWORK_H


/***************************************************************************

	Constants

***************************************************************************/

/* the various types of built-in overlay primitives */
#define OVERLAY_TYPE_END			0
#define OVERLAY_TYPE_RECTANGLE		1
#define OVERLAY_TYPE_DISK			2

/* flags for the primitives */
#define OVERLAY_FLAG_NOBLEND		0x10
#define OVERLAY_FLAG_MASK			(OVERLAY_FLAG_NOBLEND)

/* the tag assigned to all the internal overlays */
#define OVERLAY_TAG					"overlay"



/***************************************************************************

	Macros

***************************************************************************/

#define OVERLAY_START(name)	\
	static const struct overlay_piece name[] = {

#define OVERLAY_END \
	{ OVERLAY_TYPE_END } };

#define OVERLAY_RECT(l,t,r,b,c) \
	{ OVERLAY_TYPE_RECTANGLE, (c), (l), (t), (r), (b) },

#define OVERLAY_DISK(x,y,r,c) \
	{ OVERLAY_TYPE_DISK, (c), (x), (y), (r), 0 },

#define OVERLAY_DISK_NOBLEND(x,y,r,c) \
	{ OVERLAY_TYPE_DISK | OVERLAY_FLAG_NOBLEND, (c), (x), (y), (r), 0 },



/***************************************************************************

	Type definitions

***************************************************************************/

struct artwork_callbacks
{
	/* provides an additional way to activate artwork system; can be NULL */
	int (*activate_artwork)(struct osd_create_params *params);

	/* function to load an artwork file for a particular driver */
	mame_file *(*load_artwork)(const struct GameDriver **driver);
};

struct overlay_piece
{
	UINT8 type;
	rgb_t color;
	float left, top, right, bottom;
};



/***************************************************************************

	Prototypes

***************************************************************************/

int artwork_create_display(struct osd_create_params *params, UINT32 *rgb_components, const struct artwork_callbacks *callbacks);
void artwork_update_video_and_audio(struct mame_display *display);
void artwork_override_screenshot_params(struct mame_bitmap **bitmap, struct rectangle *rect, UINT32 *rgb_components);

struct mame_bitmap *artwork_get_ui_bitmap(void);
void artwork_mark_ui_dirty(int minx, int miny, int maxx, int maxy);
void artwork_get_screensize(int *width, int *height);
void artwork_enable(int enable);

void artwork_set_overlay(const struct overlay_piece *overlist);
void artwork_show(const char *tag, int show);

mame_file *artwork_load_artwork_file(const struct GameDriver **driver);

#endif /* ARTWORK_H */

