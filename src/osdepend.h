#ifndef OSDEPEND_H
#define OSDEPEND_H

#include "osd_cpu.h"
#include "inptport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The Win32 port requires this constant for variable arg routines. */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

#ifdef __LP64__
#define FPTR long   /* 64bit: sizeof(void *) is sizeof(long)  */
#else
#define FPTR int
#endif


int osd_init(void);
void osd_exit(void);


/******************************************************************************

  Display

******************************************************************************/

struct osd_bitmap
{
	int width,height;	/* width and height of the bitmap */
	int depth;			/* bits per pixel */
	void *_private;		/* don't touch! - reserved for osdepend use */
	UINT8 **line;		/* pointers to the start of each line */
};

/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */
struct osd_bitmap *osd_alloc_bitmap(int width,int height,int depth);
void osd_free_bitmap(struct osd_bitmap *bitmap);

/*
  Create a display screen, or window, of the given dimensions (or larger). It is
  acceptable to create a smaller display if necessary, in that case the user must
  have a way to move the visibility window around.
  Attributes are the ones defined in driver.h, they can be used to perform
  optimizations, e.g. dirty rectangle handling if the game supports it, or faster
  blitting routines with fixed palette if the game doesn't change the palette at
  run time. The VIDEO_PIXEL_ASPECT_RATIO flags should be honored to produce a
  display of correct proportions.
  Orientation is the screen orientation (as defined in driver.h) which will be done
  by the core. This can be used to select thinner screen modes for vertical games
  (ORIENTATION_SWAP_XY set), or even to ask the user to rotate the monitor if it's
  a pivot model. Note that the OS dependant code must NOT perform any rotation,
  this is done entirely in the core.
  Depth can be 8 or 16 for palettized modes, meaning that the core will store in the
  bitmaps logical pens which will have to be remapped through a palette at blit time,
  and 15 or 32 for direct mapped modes, meaning that the bitmaps will contain RGB
  triplets (555 or 888). For direct mapped modes, the VIDEO_RGB_DIRECT flag is set
  in the attributes field.

  Returns 0 on success.
*/
int osd_create_display(int width,int height,int depth,int fps,int attributes,int orientation);
void osd_close_display(void);


/*
  Set the portion of the screen bitmap that has to be drawn on screen. The OS
  dependant code is allowed to display a smaller portion of the bitmap if
  necessary, in that case the user must have a way to move the visibility
  window around.
  Parts of the bitmap outside the specified rectangle must never be drawn
  because they might contain garbage.
  The function must call set_ui_visarea() to tell the core the portion of the
  bitmap actually visible (which might be smaller than requested), so the user
  interface can be drawn accordingly. If the visible area is ssmaller than
  requested, set_ui_visarea() must also be called whenever the user moves the
  visibility window, so the user interface will remain at a fixed position on
  screen while the game display moves around.
*/
void osd_set_visible_area(int min_x,int max_x,int min_y,int max_y);



/*
  osd_allocate_colors() is called after osd_create_display(), to create and
  initialize the palette.

  palette is an array of 'totalcolors' R,G,B triplets. The function returns
  in *pens the pen values corresponding to the requested colors.
  When modifiable is not 0, the palette will be modified later via calls to
  osd_modify_pen(). Otherwise, the code can assume that the palette will not
  change, and activate special optimizations (e.g. direct copy for a 16-bit
  display).

  For direct mapped modes, the palette contains just three entries, a pure red,
  pure green and pure blue. Of course this is not the game palette, it is only
  used by the core to determine the layout (RGB, BGR etc.) so the OS code can
  do a straight copy of the bitmap without having to remap it. RGB 565 modes
  are NOT supported yet by the core, only 555.

  The function must also initialize Machine->uifont->colortable[] to get proper
  white-on-black and black-on-white text.

  The debug_* parameters are for the debugger display, and may be NULL if the
  debugger is not enabled. The debugger always uses DEBUGGER_TOTAL_COLORS
  colors and the palette doesn't change at run time.

  Return 0 for success.
*/
int osd_allocate_colors(unsigned int totalcolors,
		const UINT8 *palette,UINT32 *pens,int modifiable,
		const UINT8 *debug_palette,UINT32 *debug_pens);
void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue);
void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue);

void osd_mark_dirty(int xmin,int ymin,int xmax,int ymax);

/*
  osd_skip_this_frame() must return 0 if the current frame will be displayed.
  This can be used by drivers to skip cpu intensive processing for skipped
  frames, so the function must return a consistent result throughout the
  current frame. The function MUST NOT check timers and dynamically determine
  whether to display the frame: such calculations must be done in
  osd_update_video_and_audio(), and they must affect the FOLLOWING frames, not
  the current one. At the end of osd_update_video_and_audio(), the code must
  already know exactly whether the next frame will be skipped or not.
*/
int osd_skip_this_frame(void);

/*
  Update video and audio. game_bitmap contains the game display, while
  debug_bitmap an image of the debugger window (if the debugger is active; NULL
  otherwise). They can be shown one at a time, or in two separate windows,
  depending on the OS limitations. If only one is shown, the user must be able
  to toggle between the two by pressing IPT_UI_TOGGLE_DEBUG; moreover,
  osd_debugger_focus() will be used by the core to force the display of a
  specific bitmap, e.g. the debugger one when the debugger becomes active.

  leds_status is a bitmask of lit LEDs, usually player start lamps. They can be
  simulated using the keyboard LEDs, or in other ways e.g. by placing graphics
  on the window title bar.
*/
void osd_update_video_and_audio(
		struct osd_bitmap *game_bitmap,struct osd_bitmap *debug_bitmap,int leds_status);

void osd_debugger_focus(int debugger_has_focus);

void osd_set_gamma(float _gamma);
float osd_get_gamma(void);
void osd_set_brightness(int brightness);
int osd_get_brightness(void);

/*
  Save a screen shot of the game display. It is suggested to use the core
  function save_screen_snapshot() or save_screen_snapshot_as(), so the format
  of the screen shots will be consistent across ports. This hook is provided
  only to allow the display of a file requester to let the user choose the
  file name. This isn't scrictly necessary, so you can just call
  save_screen_snapshot() to let the core automatically pick a default name.
*/
void osd_save_snapshot(struct osd_bitmap *bitmap);


/******************************************************************************

  Sound

******************************************************************************/

/*
  osd_start_audio_stream() is called at the start of the emulation to initialize
  the output stream, then osd_update_audio_stream() is called every frame to
  feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

  The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
  When the stream is stereo, left and right samples are alternated in the
  stream.

  osd_start_audio_stream() and osd_update_audio_stream() must return the number
  of samples (or couples of samples, when using stereo) required for next frame.
  This will be around Machine->sample_rate / Machine->drv->frames_per_second,
  the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
  maintain audio and video in sync when using vsync. Note that sound emulation,
  especially when DACs are involved, greatly depends on the number of samples
  per frame to be roughly constant, so the returned value must always stay close
  to the reference value of Machine->sample_rate / Machine->drv->frames_per_second.
  Of course that value is not necessarily an integer so at least a +/- 1
  adjustment is necessary to avoid drifting over time.
*/
int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

/*
  control master volume. attenuation is the attenuation in dB (a negative
  number). To convert from dB to a linear volume scale do the following:
	volume = MAX_VOLUME;
	while (attenuation++ < 0)
		volume /= 1.122018454;		//	= (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation);
int osd_get_mastervolume(void);

void osd_sound_enable(int enable);

/* direct access to the Sound Blaster OPL chip */
void osd_opl_control(int chip,int reg);
void osd_opl_write(int chip,int data);


/******************************************************************************

  Keyboard

******************************************************************************/

/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo *osd_get_key_list(void);

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_get_key_list().
*/
int osd_is_key_pressed(int keycode);

/*
  Return the Unicode value of the most recently pressed key. This
  function is used only by text-entry routines in the user interface and should
  not be used by drivers. The value returned is in the range of the first 256
  bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

  Set flush to 1 to clear the buffer before entering text. This will avoid
  having prior UI and game keys leak into the text entry.
*/
int osd_readkey_unicode(int flush);


/******************************************************************************

  Joystick & Mouse/Trackball

******************************************************************************/

/*
  return a list of all available joystick inputs (see input.h)
*/
const struct JoystickInfo *osd_get_joy_list(void);

/*
  tell whether the specified joystick direction/button is pressed or not.
  joycode is the OS dependant code specified in the list returned by
  osd_get_joy_list().
*/
int osd_is_joy_pressed(int joycode);


/* We support 4 players for each analog control / trackball */
#define OSD_MAX_JOY_ANALOG	4
#define X_AXIS          1
#define Y_AXIS          2

/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration (void);
/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration (void);
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next (void);
/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate (void);
/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration (void);

void osd_trak_read(int player,int *deltax,int *deltay);

/* return values in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y);


/*
  inptport.c defines some general purpose defaults for key and joystick bindings.
  They may be further adjusted by the OS dependant code to better match the
  available keyboard, e.g. one could map pause to the Pause key instead of P, or
  snapshot to PrtScr instead of F12. Of course the user can further change the
  settings to anything he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_defaults(struct ipd *defaults);


/******************************************************************************

  File I/O

******************************************************************************/

/* inp header */
typedef struct
{
	char name[9];      /* 8 bytes for game->name + NUL */
	char version[3];   /* byte[0] = 0, byte[1] = version byte[2] = beta_version */
	char reserved[20]; /* for future use, possible store game options? */
} INP_HEADER;


/* file handling routines */
enum
{
	OSD_FILETYPE_ROM = 1,
	OSD_FILETYPE_SAMPLE,
	OSD_FILETYPE_NVRAM,
	OSD_FILETYPE_HIGHSCORE,
	OSD_FILETYPE_HIGHSCORE_DB, /* LBO 040400 */
	OSD_FILETYPE_CONFIG,
	OSD_FILETYPE_INPUTLOG,
	OSD_FILETYPE_STATE,
	OSD_FILETYPE_ARTWORK,
	OSD_FILETYPE_MEMCARD,
	OSD_FILETYPE_SCREENSHOT,
	OSD_FILETYPE_HISTORY,  /* LBO 040400 */
	OSD_FILETYPE_CHEAT,  /* LBO 040400 */
	OSD_FILETYPE_LANGUAGE, /* LBO 042400 */
#ifdef MESS
	OSD_FILETYPE_IMAGE_R,
	OSD_FILETYPE_IMAGE_RW,
#endif
	OSD_FILETYPE_end /* dummy last entry */
};

/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

int osd_faccess(const char *filename, int filetype);
void *osd_fopen(const char *gamename,const char *filename,int filetype,int read_or_write);
int osd_fread(void *file,void *buffer,int length);
int osd_fwrite(void *file,const void *buffer,int length);
int osd_fread_swap(void *file,void *buffer,int length);
int osd_fwrite_swap(void *file,const void *buffer,int length);
#ifdef LSB_FIRST
#define osd_fread_msbfirst osd_fread_swap
#define osd_fwrite_msbfirst osd_fwrite_swap
#define osd_fread_lsbfirst osd_fread
#define osd_fwrite_lsbfirst osd_fwrite
#else
#define osd_fread_msbfirst osd_fread
#define osd_fwrite_msbfirst osd_fwrite
#define osd_fread_lsbfirst osd_fread_swap
#define osd_fwrite_lsbfirst osd_fwrite_swap
#endif
int osd_fread_scatter(void *file,void *buffer,int length,int increment);
int osd_fseek(void *file,int offset,int whence);
void osd_fclose(void *file);
int osd_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum);
int osd_fsize(void *file);
unsigned int osd_fcrc(void *file);
/* LBO 040400 - start */
int osd_fgetc(void *file);
int osd_ungetc(int c, void *file);
char *osd_fgets(char *s, int n, void *file);
int osd_feof(void *file);
int osd_ftell(void *file);
/* LBO 040400 - end */

/******************************************************************************

  Miscellaneous

******************************************************************************/

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,int current,int total);

/* called when the game is paused/unpaused, so the OS dependant code can do special */
/* things like changing the title bar or darkening the display. */
/* Note that the OS dependant code must NOT stop processing input, since the user */
/* interface is still active while the game is paused. */
void osd_pause(int paused);



#ifdef MAME_NET
/* network */
int osd_net_init(void);
int osd_net_send(int player, unsigned char buf[], int *size);
int osd_net_recv(int player, unsigned char buf[], int *size);
int osd_net_sync(void);
int osd_net_input_sync(void);
int osd_net_exit(void);
int osd_net_add_player(void);
int osd_net_remove_player(int player);
int osd_net_game_init(void);
int osd_net_game_exit(void);
#endif /* MAME_NET */

#ifdef MESS
/* this is here to follow the current mame file hierachi style */
#include "osd_dir.h"
#endif

void CLIB_DECL logerror(const char *text,...);

#ifdef __cplusplus
}
#endif

#endif
