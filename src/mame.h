#ifndef MACHINE_H
#define MACHINE_H

#include "osdepend.h"
#include "drawgfx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MESS
#include "mess.h"
#endif

extern char build_version[];

#define MAX_GFX_ELEMENTS 32
#define MAX_MEMORY_REGIONS 32

struct RegionInfo
{
	UINT8 *		base;
	size_t		length;
	UINT32		type;
	UINT32		flags;
};

struct RunningMachine
{
	struct RegionInfo memory_region[MAX_MEMORY_REGIONS];
	struct GfxElement *gfx[MAX_GFX_ELEMENTS];	/* graphic sets (chars, sprites) */
	struct osd_bitmap *scrbitmap;	/* bitmap to draw into */
	struct rectangle visible_area;
	UINT32 *pens;	/* remapped palette pen numbers. When you write */
					/* directly to a bitmap, never use absolute values, */
					/* use this array to get the pen number. For example, */
					/* if you want to use color #6 in the palette, use */
					/* pens[6] instead of just 6. */
	UINT16 *game_colortable;	/* lookup table used to map gfx pen numbers */
								/* to color numbers */
	UINT32 *remapped_colortable;	/* the above, already remapped through */
									/* Machine->pens */
	const struct GameDriver *gamedrv;	/* contains the definition of the game machine */
	const struct MachineDriver *drv;	/* same as gamedrv->drv */
	int color_depth;	/* video color depth: 8, 16, 15 or 32 */
	int sample_rate;	/* the digital audio sample rate; 0 if sound is disabled. */
						/* This is set to a default value, or a value specified by */
						/* the user; osd_init() is allowed to change it to the actual */
						/* sample rate supported by the audio card. */
	struct GameSamples *samples;	/* samples loaded from disk */
	struct InputPort *input_ports;	/* the input ports definition from the driver */
								/* is copied here and modified (load settings from disk, */
								/* remove cheat commands, and so on) */
	struct InputPort *input_ports_default; /* original input_ports without modifications */
	int orientation;	/* see #defines in driver.h */
	struct GfxElement *uifont;	/* font used by the user interface */
	int uifontwidth,uifontheight;
	int uixmin,uiymin;
	int uiwidth,uiheight;
	int ui_orientation;
	struct rectangle absolute_visible_area;	/* as passed to osd_set_visible_area() */

	/* stuff for the debugger */
	struct osd_bitmap *debug_bitmap;
	UINT32 *debug_pens;
	UINT32 *debug_remapped_colortable;
	struct GfxElement *debugger_font;
};

#ifdef MESS
#define MAX_IMAGES	32
/*
 * This is a filename and it's associated peripheral type
 * The types are defined in mess.h (IO_...)
 */
struct ImageFile {
	const char *name;
	int type;
};
#endif

/* The host platform should fill these fields with the preferences specified in the GUI */
/* or on the commandline. */
struct GameOptions {
	void *record;
	void *playback;
	void *language_file; /* LBO 042400 */

	int mame_debug;
	int cheat;
	int gui_host;

	int samplerate;
	int use_samples;
	int use_emulated_ym3812;
	int use_filter;

	int color_depth;	/* 8 or 16, any other value means auto */
	int vector_width;	/* requested width for vector games; 0 means default (640) */
	int vector_height;	/* requested height for vector games; 0 means default (480) */
	int debug_width;	/* initial size of the debug_bitmap */
	int debug_height;
	int norotate;
	int ror;
	int rol;
	int flipx;
	int flipy;
	int beam;
	float vector_flicker;
	int translucency;
	int antialias;
	int use_artwork;
	char savegame;

	#ifdef MESS
	int append_no_file_extension;

	struct ImageFile image_files[MAX_IMAGES];
	int image_count;
	#endif
};

extern struct GameOptions options;
extern struct RunningMachine *Machine;

int run_game (int game);
int updatescreen(void);
void draw_screen(void);

/* next time vh_screenrefresh is called, full_refresh will be true,
   thus requesting a redraw of the entire screen */
void schedule_full_refresh(void);

void update_video_and_audio(void);
/* osd_fopen() must use this to know if high score files can be used */
int mame_highscore_enabled(void);
void set_led_status(int num,int on);

#endif
