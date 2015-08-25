/* MAME Photon 2 (Neutrino) Input Code
 *
 * Writen By: Travis Coady
 * Origional Code By: David Rempel
 *
 * web: http://www.classicgaming.com/phmame/
 * e-mail: smallfri@bigfoot.com
 *
 * Copyright (C) 2000-2001, The PhMAME Developement Team.
*/

#include <Ph.h>
#include <Pt.h>
#include "xmame.h"
#include "devices.h"
#include "photon2.h"
#include "phkeyboard.h"

int current_mouse[MOUSE_AXIS] = {0,0,0,0,0,0,0,0};
int update_mouse=FALSE;

static int ph_use_winkeys = 0;

static int ph_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option ph_input_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Photon-input related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "grabmouse",	"nograbmouse",		rc_bool,	&ph_grab_mouse,
     "0",		0,			0,		NULL,
     "Enable/disable mousegrabbing (also alt + pagedown)" },
   { "winkeys",		"nowinkeys",		rc_bool,	&ph_use_winkeys,
     "0",		0,			0,		NULL,
     "Enable/disable mapping of windowskeys under Photon" },
   { "mapkey",		NULL,			rc_use_function, NULL,
     NULL,		0,			0,		ph_mapkey,
     "Set a specific key mapping, see phmamerc.dist" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

/*
 * Parse keyboard events
 */

#define EVENT_SIZE      sizeof(PhEvent_t) + 1000
unsigned char cevent[EVENT_SIZE];

void sysdep_update_keyboard (void)
{
	PhEvent_t *event=&cevent;

	while ( PhEventPeek (event,EVENT_SIZE) == Ph_EVENT_MSG)
	{
		PtEventHandler(event);
	}
	

// This is where the photon event handling code goes
	  
#if 0
  XEvent 		E;
  int	 		keycode,code;
  int			mask;
  int 			*pt;
  static int		old_grab_mouse = FALSE;
  /* grrr some windowmanagers send multiple focus events, this is used to
     filter them. */
  static int            focus = FALSE;
  
  /* handle winkey mappings */
  if (x11_use_winkeys)
  {
    extended_code_table[XK_Meta_L&0x1FF] = KEY_LWIN; 
    extended_code_table[XK_Meta_R&0x1FF] = KEY_RWIN; 
  }

#ifdef x11
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
      case FocusIn:
        /* check for multiple events and ignore them */
        if (focus) break;
        focus = TRUE;
	/* to avoid some meta-keys to get locked when wm iconify xmame, we must
	perform a key reset whenever we retrieve keyboard focus */
	memset((void *)&xkey[0], FALSE, 128*sizeof(unsigned char) );
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
        mask = TRUE;
      case KeyRelease:
	keycode = XLookupKeysym ((XKeyEvent *) &E, 0);
	/* fprintf(stderr, "Keyevent key:%04X\n", keycode); */
	/* look which table should be used */
	pt=code_table;
        if ( (keycode & 0xfe00) == 0xfe00 )
        {
          pt=extended_code_table;
	  code=keycode&0x01ff;
	}
	else
	  code=keycode&0x00ff;
	/* if unnasigned key ignore it */
	if ( *(pt+code) ) xkey [ *(pt+code) ] = mask;
	break;
#ifdef X11_JOYSTICK
      /* grrr we can't use case here since the event types for XInput devices
         aren't hardcoded, since we should have caught anything else above,
         just asume it's an XInput event */
      default:
	  process_x11_joy_event(&E);
	  break;
#endif
    } /* switch */
  } /* while */
#endif
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int ph_mapkey(struct rc_option *option, const char *arg, int priority)
{
   int from,to;
   /* ultrix sscanf() requires explicit leading of 0x for hex numbers */
   if ( sscanf(arg,"0x%x,0x%x",&from,&to) == 2)
   {
      /* perform tests */
      /* fprintf(stderr_file,"trying to map %x to%x\n",from,to); */
      if ( (to>=0) || (to<=127) )
      {
         if ( (from>=0) && (from<=0x00ff) ) 
         {
            code_table[from]=to; return OSD_OK;
         }
         if ( (from>=0xfe00) && (from<=0xffff) ) 
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
#if 1
	int i;
	PhCursorInfo_t buf;
	PhPoint_t	windowpos;
	int ig;

	ig=PhInputGroup(NULL);
	
	if (PhQueryCursor(ig,&buf) != 0 )
	{
		fprintf(stderr,"error: mouse Error\n");
		mouse_data[0].deltas[0] = 0;
		mouse_data[0].deltas[1] = 0;
		return;
	}

	if ( ph_grab_mouse )
	{
//		fprintf(stderr,"grabbing mouse\n");
		PtGetAbsPosition(P_mainWindow,&windowpos.x, &windowpos.y);
		PhMoveCursorAbs(ig, windowpos.x+(visual_width/2), windowpos.y+(visual_height/2));
		mouse_data[0].deltas[0] = buf.pos.x - (windowpos.x+(visual_width/2));
		mouse_data[0].deltas[1] = buf.pos.y - (windowpos.y+(visual_height/2));

	}
	else
	{
		if (update_mouse==FALSE)
		{
			mouse_data[0].deltas[0]=0;
			mouse_data[0].deltas[0]=0;
		}
		update_mouse=FALSE;
	}
#endif
}

void sysdep_set_leds(int leds)
{
}
