/*****************************************************************

  Xmame glide driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
/* pretend we're x11.c otherwise display and a few other crucial things don't
   get declared */
#define __X11_C_   
#define __XFX_C_


#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "xmame.h"
#include "x11.h"

int  InitVScreen(void);
void CloseVScreen(void);
int  InitGlide(void);

extern int fxwidth;
extern int fxheight;
extern struct rc_option fx_opts[];

static Cursor        cursor;
static XVisualInfo   myvisual;

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL, 		NULL,			rc_link,	fx_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL, 		NULL,			rc_link,	x11_input_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

typedef struct {
#define MWM_HINTS_DECORATIONS   2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;

int sysdep_init(void)
{
  fprintf(stderr,
     "info: using FXmame v0.5 driver for xmame, written by Mike Oliphant\n");
  
  /* Open the display. */
  display=XOpenDisplay(NULL);

  if(!display) {
	fprintf (stderr,"OSD ERROR: failed to open the display.\n");
	return OSD_NOT_OK; 
  }

  return InitGlide();
}

void sysdep_close(void)
{
   XCloseDisplay(display);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  XSetWindowAttributes winattr;
  int 		 myscreen;
  XEvent	 event;
  XSizeHints 	 hints;
  XWMHints 	 wm_hints;
  MotifWmHints mwmhints;
  Atom mwmatom;
  
  screen=DefaultScreenOfDisplay(display);
  myscreen=DefaultScreen(display);
  cursor=XCreateFontCursor(display,XC_trek);
  
  if(!XMatchVisualInfo(display,myscreen,8,PseudoColor,&myvisual)) {
	fprintf(stderr,"8bit depth PseudoColor X-Visual not available :-( \n");
	/* test for a 8bpp environment */
	if      (XMatchVisualInfo(display,myscreen,8,TrueColor,&myvisual))
	  fprintf(stderr,"Using 8bpp TrueColor X-Visual Resource\n");
	/* test for a 15bpp environment */
	else if (XMatchVisualInfo(display,myscreen,15,TrueColor,&myvisual))
	  fprintf(stderr,"Using 15bpp TrueColor X-Visual Resource\n");
	/* test for a 16bpp environment */
	else if (XMatchVisualInfo(display,myscreen,16,TrueColor,&myvisual))
	  fprintf(stderr,"Using 16bpp TrueColor X-Visual Resource\n");
	/* test for a 24bpp environment */
	else if (XMatchVisualInfo(display,myscreen,24,TrueColor,&myvisual))
	  fprintf(stderr,"Using 24bpp TrueColor X-Visual Resource\n");
	/* test for a 32bpp environment */
	else if (XMatchVisualInfo(display,myscreen,32,TrueColor,&myvisual))
	  fprintf(stderr,"Using 32bpp TrueColor X-Visual Resource\n");
	/* if arrives here means an error :-( */
	else
	  {
		fprintf(stderr,"Cannot find any supported X-Visual resource\n");
		sysdep_display_close(); /* this will clean up for us */
		return OSD_NOT_OK; 
	  }
  } else fprintf(stderr,"Using 8bpp PseudoColor Visual. Good!\n");

  colormap=XCreateColormap(display,
						   RootWindow(display,myvisual.screen),
						   myvisual.visual,AllocNone);

  winattr.background_pixel=0;
  winattr.border_pixel=WhitePixelOfScreen(screen);
  winattr.bit_gravity=ForgetGravity;
  winattr.win_gravity=NorthWestGravity;
  winattr.backing_store=NotUseful;
  winattr.override_redirect=False;
  winattr.save_under=False;
  winattr.event_mask=0;
  winattr.do_not_propagate_mask=0;
  winattr.colormap=colormap;
  winattr.cursor=None;

  window=XCreateWindow(display,RootWindowOfScreen(screen),0,0,fxwidth,fxheight,
					   0,myvisual.depth,
					   InputOutput,myvisual.visual,
					   CWBorderPixel | CWBackPixel |
					   CWEventMask | CWDontPropagate |
					   CWColormap | CWCursor,&winattr);
  
  if (!window) {
	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  /*  Placement hints etc. */

  hints.flags=PMinSize|PMaxSize|USPosition|USSize;
  
  hints.min_width=hints.max_width=hints.base_width=screen->width;
  hints.min_height=hints.max_height=hints.base_height=screen->height;

  hints.x=hints.y=0;
  
  wm_hints.input=TRUE;
  wm_hints.flags=InputHint;
  
  XSetWMHints(display,window,&wm_hints);
  XSetWMNormalHints(display,window,&hints);
  XStoreName(display,window,title);
  
  XDefineCursor(display,window,cursor);

  /* Hack to get rid of window title bar */
  
  mwmhints.flags=MWM_HINTS_DECORATIONS;
  mwmhints.decorations=0;
  mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);
  
  XChangeProperty(display,window,mwmatom,mwmatom,32,
				  PropModeReplace,(unsigned char *)&mwmhints,4);
  
  /* Map and expose the window. */

  if(use_mouse) {
	/* grab the pointer and query MotionNotify events */

	XSelectInput(display, 
				 window, 
				 FocusChangeMask   | ExposureMask | 
				 KeyPressMask      | KeyReleaseMask | 
				 EnterWindowMask   | LeaveWindowMask |
				 PointerMotionMask | ButtonMotionMask |
				 ButtonPressMask   | ButtonReleaseMask
				 );
	
	XGrabPointer(display,
				 window, /* RootWindow(display,DefaultScreen(display)), */
				 False,
				 PointerMotionMask | ButtonMotionMask |
				 ButtonPressMask   | ButtonReleaseMask | 
				 EnterWindowMask   | LeaveWindowMask ,
				 GrabModeAsync, GrabModeAsync,
				 None, cursor, CurrentTime );
  }
  else {
	XSelectInput(display, 
				 window, 
				 FocusChangeMask | ExposureMask | 
				 KeyPressMask | KeyReleaseMask
				 );
  }
  
  XMapRaised(display,window);
  XClearWindow(display,window);
  XWindowEvent(display,window,ExposureMask,&event);
  
  if (InitVScreen() != OSD_OK)
     return OSD_NOT_OK;
  
  return OSD_OK;
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void sysdep_display_close (void)
{
   XFreeColormap(display, colormap);
     
   if(window) {
     /* ungrab the pointer */

     if(use_mouse) XUngrabPointer(display,CurrentTime);

     CloseVScreen();  /* Shut down glide stuff */
   }

   XSync(display,False); /* send all events to sync; */
}
