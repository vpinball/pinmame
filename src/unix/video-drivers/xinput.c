/*
 * X-Mame x11 input code
 *
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "xmame.h"
#include "devices.h"
#include "x11.h"
#include "xkeyboard.h"
#include "keyboard.h"

#ifdef xgl
#include "glmame.h"
static int xgl_aspect_resize_action = 0;
#endif

static int current_mouse[MOUSE_AXIS] = {0,0,0,0,0,0,0,0};
static int x11_use_winkeys = 0;

static int x11_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option x11_input_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "X11-input related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "grabmouse", "gm", rc_bool, &x11_grab_mouse, "0", 0, 0, NULL, "Enable/disable mousegrabbing (also alt + pagedown)" },
	{ "grabkeyboard", "gkb", rc_bool, &x11_grab_keyboard, "0", 0, 0, NULL, "Enable/disable keyboardgrabbing (also alt + pageup)" },
	{ "winkeys", "wk", rc_bool, &x11_use_winkeys, "0", 0, 0, NULL, "Enable/disable mapping of windowskeys under X" },
	{ "mapkey", "mk", rc_use_function, NULL, NULL, 0, 0, x11_mapkey, "Set a specific key mapping, see xmamerc.dist" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

#if defined(__sgi) && !defined(MESS)
/* Under Xmame, track if the game is paused due to window iconification */
static unsigned char game_paused_by_unmap = FALSE;
#endif

/*
 * Parse keyboard events
 */
void sysdep_update_keyboard (void)
{
	XEvent				E;
	KeySym				keysym;
	char				keyname[16+1];
	int				mask;
	struct xmame_keyboard_event	event;
	static int			old_grab_mouse = FALSE;
	/* grrr some windowmanagers send multiple focus events, this is used to
	   filter them. */
	static int			focus = FALSE;

	/* handle winkey mappings */
	if (x11_use_winkeys)
	{
		extended_code_table[XK_Meta_L&0x1FF] = KEY_LWIN; 
		extended_code_table[XK_Meta_R&0x1FF] = KEY_RWIN; 
	}

#ifdef NOT_DEFINED /* x11 */
	if(run_in_root_window && x11_video_mode == X11_WINDOW)
	{
		static int i=0;
		i = ++i % 3;
		switch (i)
		{
			case 0:
				xkey[KEY_O] = 0;
				xkey[KEY_K] = 0;
				break;
			case 1:
				xkey[KEY_O] = 1;
				xkey[KEY_K] = 0;
				break;
			case 2:
				xkey[KEY_O] = 0;
				xkey[KEY_K] = 1;
				break;
		}
	}
	else
#endif

		/* query all events that we have previously requested */
		while ( XPending(display) )
		{
			mask = FALSE;
			event.press = FALSE;

			XNextEvent(display,&E);
			/*  fprintf(stderr_file,"Event: %d\n",E.type); */

			/* we don't have to check x11_video_mode or extensions like xil here,
			   since our eventmask should make sure that we only get the event's matching
			   the current update method */
			switch (E.type)
			{
				/* display events */
#ifdef x11
				case Expose:
					if ( E.xexpose.count == 0 ) x11_window_refresh_screen();
					break;
#endif
#ifdef xgl
				case ConfigureNotify:
					if(E.xany.window == window)
					{
						if( xgl_aspect_resize_action == 0 &&
								(
								 abs(winwidth - E.xconfigure.width) > 50 ||
								 abs(winheight - E.xconfigure.height) > 50
								)
						  )
						{
							xgl_aspect_resize_action = 1;

							winwidth  = E.xconfigure.width;
							winheight = E.xconfigure.height; 

							xgl_fixaspectratio(&winwidth, &winheight);

							XResizeWindow(display,window,winwidth, winheight);

							xgl_resize(winwidth, winheight, 0);
						} else {
							xgl_aspect_resize_action = 0;
						}
					}
					break;
#endif
				case FocusIn:
					/* check for multiple events and ignore them */
					if (focus) break;
					focus = TRUE;
					/* to avoid some meta-keys to get locked when wm iconify xmame, we must
					   perform a key reset whenever we retrieve keyboard focus */
					xmame_keyboard_clear();
					if (old_grab_mouse)
					{
						if (!XGrabPointer(display, window, True, 0, GrabModeAsync,
									GrabModeAsync, window, None, CurrentTime))
						{
							if (show_cursor) XDefineCursor(display,window,invisible_cursor);
							x11_grab_mouse = TRUE;
						}
					}
					break;
				case FocusOut:
					/* check for multiple events and ignore them */
					if (!focus) break;
					focus = FALSE;
					old_grab_mouse = x11_grab_mouse;
					if (x11_grab_mouse)
					{
						XUngrabPointer(display, CurrentTime);
						if (show_cursor) XDefineCursor(display,window,normal_cursor);
						x11_grab_mouse = FALSE;
					}
					break;
				case EnterNotify:
					if (use_private_cmap) XInstallColormap(display,colormap);
					break;	
				case LeaveNotify:
					if (use_private_cmap) XInstallColormap(display,DefaultColormapOfScreen(screen));
					break;	
#ifdef USE_XIL
				case ConfigureNotify:
					update_xil_window_size( E.xconfigure.width, E.xconfigure.height );
					break;
#endif
					/* input events */    
				case MotionNotify:
					current_mouse[0] += E.xmotion.x_root;
					current_mouse[1] += E.xmotion.y_root;
					break;
				case ButtonPress:
					mask = TRUE;
#ifdef USE_DGA
					/* Some buggy combination of XFree and virge screwup the viewport
					   on the first mouseclick */
					if(xf86_dga_first_click) { xf86_dga_first_click = 0; xf86_dga_fix_viewport = 1; }
#endif          
				case ButtonRelease:
					mouse_data[0].buttons[E.xbutton.button-1] = mask;
					break;
				case KeyPress:
					event.press = TRUE;
				case KeyRelease:
					/* get bare keysym, for the scancode */
					keysym = XLookupKeysym ((XKeyEvent *) &E, 0);
					/* get key name, using modifiers for the unicode */
					XLookupString ((XKeyEvent *) &E, keyname, 16, NULL, NULL);

					/*	fprintf(stderr, "Keyevent keycode: %04X, keysym: %04X, unicode: %02X\n",
						E.xkey.keycode, (unsigned int)keysym, (unsigned int)keyname[0]); */

					/* look which table should be used */
					if ( (keysym & ~0x1ff) == 0xfe00 )
						event.scancode = extended_code_table[keysym & 0x01ff];
					else if (keysym < 0xff)
						event.scancode = code_table[keysym & 0x00ff];
					else
						event.scancode = 0;

					event.unicode = keyname[0];

					xmame_keyboard_register_event(&event);
					break;
#if defined USE_XINPUT_DEVICES || defined X11_JOYSTICK
				default:
#endif
#ifdef USE_XINPUT_DEVICES
					if (XInputProcessEvent(&E)) break;
#endif

#if defined(__sgi) && !defined(MESS)
					/*
					 * Push the pause keycode in the Xmame keyboard queue, accordingly to
					 * to the three rules explained below. This should pause/run the game if
					 * Xmame window is unmapped/mapped.
					 *
					 * Rules:
					 * - mapped with game paused by unmap -> restart the game
					 * - unmapped with game running -> pause the game and flag the condition
					 * - unmapped with game paused  -> no action (already paused by the user)
					 */
				case MapNotify:
					game_paused_by_unmap = FALSE;
					break;

				case UnmapNotify:
					if (! is_game_paused())
						game_paused_by_unmap = TRUE;
					break;
#endif

#ifdef X11_JOYSTICK
					/* grrr we can't use case here since the event types for XInput devices
					   aren't hardcoded, since we should have caught anything else above,
					   just asume it's an XInput event */
					process_x11_joy_event(&E);
					break;
#endif
			} /* switch */
		} /* while */
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int x11_mapkey(struct rc_option *option, const char *arg, int priority)
{
	unsigned int from,to;
	/* ultrix sscanf() requires explicit leading of 0x for hex numbers */
	if ( sscanf(arg,"0x%x,0x%x",&from,&to) == 2)
	{
		/* perform tests */
		/* fprintf(stderr,"trying to map %x to %x\n",from,to); */
		if ( to <= 127 )
		{
			if ( from <= 0x00ff ) 
			{
				code_table[from]=to; return OSD_OK;
			}
			else if ( (from>=0xfe00) && (from<=0xffff) ) 
			{
				extended_code_table[from&0x01ff]=to; return OSD_OK;
			}
		}
		/* stderr_file isn't defined yet when we're called. */
		fprintf(stderr,"Invalid keymapping %s. Ignoring...\n", arg);
	}
	return OSD_NOT_OK;
}

void sysdep_mouse_poll (void)
{
	int i;
	if(x11_video_mode == X11_DGA)
	{
		/* 2 should be MOUSE_AXIS but we don't support more
		   then 2 axis at the moment so this is faster */
		for (i=0; i<2; i++)
		{
			mouse_data[0].deltas[i] = current_mouse[i];
			current_mouse[i] = 0;
		}
	}
	else
	{
		Window root,child;
		int root_x, root_y, pos_x, pos_y;
		unsigned int keys_buttons;

		if (!XQueryPointer(display,window, &root,&child, &root_x,&root_y,
					&pos_x,&pos_y,&keys_buttons) )
		{
			mouse_data[0].deltas[0] = 0;
			mouse_data[0].deltas[1] = 0;
			return;
		}

		if ( x11_grab_mouse )
		{
			XWarpPointer(display, None, window, 0, 0, 0, 0,
					visual_width/2, visual_height/2);
			mouse_data[0].deltas[0] = pos_x - visual_width/2;
			mouse_data[0].deltas[1] = pos_y - visual_height/2;
		}
		else
		{
			mouse_data[0].deltas[0] = pos_x - current_mouse[0];
			mouse_data[0].deltas[1] = pos_y - current_mouse[1];
			current_mouse[0] = pos_x;
			current_mouse[1] = pos_y;
		}
	}
}

void sysdep_set_leds(int leds)
{
}
