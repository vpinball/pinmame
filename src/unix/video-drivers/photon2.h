/* MAME Photon 2 Header
 *
 * Writen By: Travis Coady
 * Origional Code By: David Rempel
 *
 * web: http://www.classicgaming.com/phmame/
 * e-mail: smallfri@bigfoot.com
 *
 * Copyright (C) 2000-2001, The PhMAME Developement Team.
*/

/* There is wayyyy... to much X11 crap left
   over here, remove later */

#ifndef __MAME_PHOTON_H_
#define __MAME_PHOTON_H_

#include <Ph.h>
#include <Pt.h>
#include "effect.h"

/* Commented... */
enum { PH_WINDOW, PH_OVR };
#define PH_MODE_COUNT 2

#ifdef __PH_C__
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int 		ph_video_mode;
EXTERN int 		mode_available[PH_MODE_COUNT];
EXTERN int 		ph_grab_mouse;
EXTERN int 		depth;
EXTERN int		show_cursor;
EXTERN unsigned char    *scaled_buffer_ptr;
EXTERN PtWidget_t	*P_mainWindow;
EXTERN struct _Ph_ctrl  *ph_ctx;
extern struct rc_option ph_window_opts[];
extern struct rc_option ph_ovr_opts[];
extern struct rc_option ph_input_opts[];
EXTERN char		phkey[128];


#if 0

EXTERN Display 		*display;
EXTERN Window		window;
EXTERN char	 	xkey[128];
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
EXTERN int		run_in_root_window;
EXTERN int		show_cursor;
EXTERN int		use_private_cmap;
EXTERN int		use_xil;
EXTERN int		use_mt_xil;
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
#endif


/* Normal photon window functions */
int  ph_window_create_display(int depth);
void ph_window_close_display(void);
int  ph_window_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void ph_window_update_display(struct mame_bitmap *bitmap);
int  ph_window_alloc_palette(int writable_colors);
void ph_window_refresh_screen(void);
int  ph_window_16bpp_capable(void);

/* photon video overlay functions */
int  ph_ovr_init(void);
int  ph_ovr_create_display(int depth);
void ph_ovr_close_display(void);
int  ph_ovr_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue);
void ph_ovr_update_display(struct mame_bitmap *bitmap);
int  ph_ovr_alloc_palette(int writable_colors);
int  ph_ovr_16bpp_capable(void);

#if 0
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
#endif

/* generic helper functions */
int ph_init_palette_info(void);

#endif /* ifndef __MAME_PHOTON_H */
