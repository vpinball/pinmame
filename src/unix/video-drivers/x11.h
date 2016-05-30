#ifndef __X11_H_
#define __X11_H_

#include <X11/Xlib.h>
#include "effect.h"

#ifdef __X11_C_
#define EXTERN
#else
#define EXTERN extern
#endif

enum { X11_WINDOW, X11_DGA, X11_XV_WINDOW, X11_XV_FULLSCREEN };
#define X11_MODE_COUNT 4

EXTERN Display 		*display;
EXTERN Window		window;
EXTERN Screen 		*screen;
EXTERN Colormap		colormap;
EXTERN Visual		*xvisual;
EXTERN int		depth;
EXTERN unsigned char	*scaled_buffer_ptr;
EXTERN int		mode_available[X11_MODE_COUNT];
EXTERN Cursor		normal_cursor;
EXTERN Cursor		invisible_cursor;
EXTERN int		x11_video_mode;
EXTERN int		x11_grab_mouse;
EXTERN int		x11_grab_keyboard;
EXTERN int		run_in_root_window;
EXTERN int		show_cursor;
EXTERN int		use_private_cmap;
EXTERN int		use_xil;
EXTERN int		use_mt_xil;
#ifdef USE_HWSCALE
EXTERN int		use_hwscale;
EXTERN int		use_xv;
EXTERN long		hwscale_redmask;
EXTERN long		hwscale_greenmask;
EXTERN long		hwscale_bluemask;
#endif
extern struct rc_option xf86_dga_opts[];
extern struct rc_option x11_window_opts[];
extern struct rc_option	x11_input_opts[];

#if defined x11 && defined USE_DGA
EXTERN int		xf86_dga_fix_viewport;
EXTERN int		xf86_dga_first_click;
#endif

#ifdef X11_JOYSTICK
EXTERN int devicebuttonpress;
EXTERN int devicebuttonrelease;
EXTERN int devicemotionnotify;
EXTERN int devicebuttonmotion;
#endif

/*** prototypes ***/

/* device related */
void process_x11_joy_event(XEvent *event);

#ifdef x11

/* Normal x11_window functions */
int  x11_window_create_display(int depth);
void x11_window_close_display(void);
int  x11_window_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void x11_window_update_display(struct mame_bitmap *bitmap);
int  x11_window_alloc_palette(int writable_colors);
void x11_window_refresh_screen(void);
int  x11_window_16bpp_capable(void);

/* Xf86_dga functions */
int  xf86_dga_init(void);
int  xf86_dga_create_display(int depth);
void xf86_dga_close_display(void);
int  xf86_dga_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void xf86_dga_update_display(struct mame_bitmap *bitmap);
int  xf86_dga_alloc_palette(int writable_colors);
int  xf86_dga_16bpp_capable(void);
int  xf86_dga1_init(void);
int  xf86_dga1_create_display(int depth);
void xf86_dga1_close_display(void);
int  xf86_dga1_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void xf86_dga1_update_display(struct mame_bitmap *bitmap);
int  xf86_dga1_alloc_palette(int writable_colors);
int  xf86_dga1_16bpp_capable(void);
int  xf86_dga2_init(void);
int  xf86_dga2_create_display(int depth);
void xf86_dga2_close_display(void);
int  xf86_dga2_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void xf86_dga2_update_display(struct mame_bitmap *bitmap);
int  xf86_dga2_alloc_palette(int writable_colors);
int  xf86_dga2_16bpp_capable(void);

/* XIL functions */
#ifdef USE_XIL
void init_xil( void );
void setup_xil_images( int, int );
void refresh_xil_screen( void );
#endif

/* DBE functions */
#ifdef USE_DBE
void setup_dbe( void );
void swap_dbe_buffers( void );
#endif

/* generic helper functions */
int x11_init_palette_info(void);

#endif /* ifdef x11 */

#undef EXTERN
#endif /* ifndef __X11_H_ */
