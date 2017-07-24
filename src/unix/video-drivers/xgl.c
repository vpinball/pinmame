/*****************************************************************

  Xmame OpenGL driver

  Written based on the x11 driver by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/
/* pretend we're x11.c otherwise display and a few other crucial things don't
   get declared */
#define __X11_C_   
#define __XOPENGL_C_

#define RRand(range) (random()%range)

#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/cursorfont.h>

#include "glxtool.h"

#include "xmame.h"
#include "glmame.h"
#include "x11.h"

#include "driver.h"
#include "xmame.h"

static Cursor        cursor;
static XVisualInfo   *myvisual;

typedef struct {
#define MWM_HINTS_DECORATIONS   2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;

int winwidth = 0;
int winheight = 0;
int fullscreen_width = 0;
int fullscreen_height = 0;
int orig_width = 0;
int orig_height = 0;
int visual_orientated_width = 0;
int visual_orientated_height = 0;
static int is_fullscreen=0;
int bilinear=1; /* Do binlinear filtering? */
int alphablending=0; /* alphablending */

int fullscreen = 0;
int antialias=0;
int antialiasvec=1;
int translucency = 0;

static int drawbitmapvec;

XSetWindowAttributes window_attr;
GLXContext glContext=NULL;

const GLCapabilities glCapsDef = { BUFFER_DOUBLE, COLOR_RGBA, STEREO_OFF,
			           1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1,
			           -1
			         };

GLCapabilities glCaps;

char * libGLName=0;
char * libGLUName=0;

static char *gl_res = NULL;

static const char * xgl_version_str = 
	"\nGLmame v0.94 - the_peace_version , by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com,\nbased upon GLmame v0.6 driver for xmame, written by Mike Oliphant\n\n";

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "OpenGL Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "fullscreen",   NULL,    rc_bool,       &fullscreen,
      "0",           0,       0,             NULL,
      "Start at fullscreen (default: false)" },
   { "gldblbuffer",	NULL,			rc_bool,	&doublebuffer,
     "1",		0,			0,		NULL,
     "Enable/disable double buffering (default: true)" },
   { "gltexture_size",	NULL,			rc_int,	&force_text_width_height,
     "0",		0,			0,		NULL,
     "Force the max width and height of one texture segment (default: autosize)" },
   { "glforceblitmode", "glblit",		rc_bool,	&useColorBlitter,
     "0",		0,			0,		NULL,
     "Force blitter for true color modes 15/32bpp (default: true)" },
#ifndef NOGLEXT78
   { "glext78",	        "glext",		rc_bool,	&useGLEXT78,
     "1",		0,			0,		NULL,
     "Force the usage of the gl extension #78, if available (paletted texture, default: true)" },
#endif
   { "glbilinear",	"glbilin",		rc_bool,	&bilinear,
     "1",		0,			0,		NULL,
     "Enable/disable bilinear filtering (default: true)" },
   { "gldrawbitmap",	"glbitmap",		rc_bool,	&drawbitmap,
     "1",		0,			0,		NULL,
     "Enable/Disable the drawing of the bitmap - e.g. disable it within vector games for a speedup (default: true)" },
   { "gldrawbitmapvec",	"glbitmapv",		rc_bool,	&drawbitmapvec,
     "1",		0,			0,		NULL,
     "Enable/Disable the drawing of the bitmap only for vector games - speedup (default: true)" },
   { "glcolormod",	"glcmod",		rc_bool,	&use_mod_ctable,
     "1",		0,			0,		NULL,
     "Enable/Disable color modulation (intensity,gamma) (default: true)" },
   { "glbeam",		NULL,			rc_float,	&gl_beam,
     "1.0",		1.0,			16.0,		NULL,
     "Set the beam size for vector games (default: 1.0)" },
   { "glalphablending",	"glalpha",		rc_bool,	&alphablending,
     "1",		0,			0,		NULL,
     "Enable/disable alphablending if available (default: true)" },
   { "glantialias",	"glaa",			rc_bool,	&antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing (default: true)" },
   { "glantialiasvec",	"glaav",		rc_bool,	&antialiasvec,
     "1",		0,			0,		NULL,
     "Enable/disable vector antialiasing (default: true)" },
   { "gllibname",	"gllib",		rc_string,	&libGLName,
     "libGL.so",	0,			0,		NULL,
     "Choose the dynamically loaded OpenGL Library (default libGL.so)" },
   { "glulibname",	"glulib",		rc_string,	&libGLUName,
     "libGLU.so",	0,			0,		NULL,
     "Choose the dynamically loaded GLU Library (default libGLU.so)" },
   { "cabview",		NULL,			rc_bool,	&cabview,
     "0",		0,			0,		NULL,
     "Start/Don't start in cabinet view mode (default: false)" },
   { "cabinet",		NULL,			rc_string,	&cabname,
     "glmamejau",	0,			0,		NULL,
     "Specify which cabinet model to use (default: glmamejau)" },
   { "glres",	        NULL,			rc_string,	&gl_res,
     NULL,		0,			0,		NULL,
     "Always scale games to XresxYres, keeping their aspect ratio. This overrides the scale options" },
   { NULL,		NULL,			rc_link,	x11_input_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static Cursor create_invisible_cursor (Display * display, Window win)
{
	Pixmap cursormask;
	XGCValues xgc;
	XColor dummycolour;
	Cursor cursor;
	GC gc;

	cursormask = XCreatePixmap (display, win, 1, 1, 1 /*depth */ );
	xgc.function = GXclear;
	gc = XCreateGC (display, cursormask, GCFunction, &xgc);
	XFillRectangle (display, cursormask, gc, 0, 0, 1, 1);
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = XCreatePixmapCursor (display, cursormask, cursormask,
			&dummycolour, &dummycolour, 0, 0);
	XFreeGC (display, gc);
	XFreePixmap (display, cursormask);
	return cursor;
}


int sysdep_init(void)
{
   /* Open the display. */
   display=XOpenDisplay(NULL);

   if(!display) {
      fprintf (stderr,"OSD ERROR: failed to open the display.\n");
      return OSD_NOT_OK; 
   }
  
   gl_bootstrap_resources();

#ifndef NDEBUG
   printf("GLINFO: xgl init !\n");
#endif

   return OSD_OK;
}

void sysdep_close(void)
{
   XCloseDisplay(display);

   fprintf(stderr, xgl_version_str);
  
#ifndef NDEBUG
   printf("GLINFO: xgl closed !\n");
#endif
}


/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  int 		myscreen;
  XEvent	event;
  XSizeHints 	hints;
  XWMHints 	wm_hints;
  MotifWmHints	mwmhints;
  unsigned long winmask;
  Atom mwmatom;
  char *glxfx;
  int resizeEvtMask = 0;
  VisualGC vgc;
  int ownwin = 1;
  int customSize=0;

  /* If using 3Dfx, Resize the window to fullscreen so we don't lose focus
     We have to do this after glXMakeCurrent(), or else the voodoo driver
     will give us the wrong resolution */
  if((glxfx=getenv("MESA_GLX_FX")) && (glxfx[0]=='f') )
  	fullscreen=1;

  if(gl_res!=NULL)
  {
     if( sscanf(gl_res, "%dx%d", &fullscreen_width, &fullscreen_height) != 2)
     {
        fprintf(stderr, "error: invalid value for glres: %s\n", gl_res);
     } else {
        customSize=1;
	orig_width = fullscreen_width;
	orig_height = fullscreen_height;
     }
  }
  if(!fullscreen) 
	  resizeEvtMask = StructureNotifyMask ;

  fprintf(stderr, xgl_version_str);
  
  screen=DefaultScreenOfDisplay(display);
  myscreen=DefaultScreen(display);
  cursor=XCreateFontCursor(display,XC_trek);
  
  if(!customSize)
  {
  	fullscreen_width = screen->width;
  	fullscreen_height = screen->height;

	if( ! blit_swapxy )
	{
		  visual_orientated_width  = visual_width;
		  visual_orientated_height = visual_height;
		  orig_width  = visual_width*widthscale;
		  orig_height = visual_height*heightscale;
	} else {
		  visual_orientated_width  = visual_height;
		  visual_orientated_height = visual_width;
		  orig_width  = visual_height*heightscale;
		  orig_height = visual_width*widthscale;
	}
  }

  if(fullscreen)
  {
	winwidth = fullscreen_width;
	winheight = fullscreen_height;
	is_fullscreen=1;
  } else {
	winwidth = orig_width;
	winheight = orig_height;
	is_fullscreen=0;
  }

  window_attr.background_pixel=0;
  window_attr.border_pixel=WhitePixelOfScreen(screen);
  window_attr.bit_gravity=ForgetGravity;
  window_attr.win_gravity=NorthWestGravity;
  window_attr.backing_store=NotUseful;
  window_attr.override_redirect=False;
  window_attr.save_under=False;
  window_attr.event_mask=0; 
  window_attr.do_not_propagate_mask=0;
  window_attr.colormap=0; /* done later, within findVisualGlX .. */
  window_attr.cursor=None;

  if(fullscreen) 
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWDontPropagate | 
                CWColormap    | CWCursor
	        ;
  else
      winmask = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap    
	        ;

  fetch_GL_FUNCS (libGLName, libGLUName, 1 /* force refetch ... */);

  glCaps = glCapsDef;

  glCaps.alphaBits=(alphablending>0)?1:0;
  glCaps.buffer   =(doublebuffer>0)?BUFFER_DOUBLE:BUFFER_SINGLE;
  glCaps.gl_supported = 1;

  window = RootWindow(display,DefaultScreen( display ));
  vgc = findVisualGlX( display, window,
                       &window, winwidth, winheight, &glCaps, 
		       &ownwin, &window_attr, winmask,
		       NULL, 0, NULL, 
#ifndef NDEBUG
		       1);
#else
		       0);
#endif

  if(vgc.success==0)
  {
	fprintf(stderr,"OSD ERROR: failed to obtain visual.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }

  myvisual =vgc.visual;
  glContext=vgc.gc;
  colormap=window_attr.colormap;

  if (!window) {
	fprintf(stderr,"OSD ERROR: failed in XCreateWindow().\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  if(!glContext) {
	fprintf(stderr,"OSD ERROR: failed to create OpenGL context.\n");
	sysdep_display_close();
	return OSD_NOT_OK; 
  }
  
  setGLCapabilities ( display, myvisual, &glCaps);

  alphablending= (glCaps.alphaBits>0)?1:0;
  doublebuffer = (glCaps.buffer==BUFFER_DOUBLE)?1:0;
  useColorIndex= (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)==0;

  /*  Placement hints etc. */
  
  hints.flags=PMinSize|PMaxSize;
  
  if(fullscreen) 
  { 
  	hints.flags|=USPosition|USSize;
  } else {
  	hints.flags|=PSize;
  }

  hints.min_width	= hints.max_width = hints.base_width = winwidth;
  hints.min_height= hints.max_height = hints.base_height = winheight;
  
  wm_hints.input=TRUE;
  wm_hints.flags=InputHint;
  
  XSetWMHints(display,window,&wm_hints);

  if(fullscreen) 
	  XSetWMNormalHints(display,window,&hints);

  XStoreName(display,window,title);
  
  if(fullscreen) 
	  XDefineCursor(display,window,create_invisible_cursor (display, window));
  else
	  XDefineCursor(display,window,cursor);

  /* Hack to get rid of window title bar */
  if(fullscreen) 
  {
	mwmhints.flags=MWM_HINTS_DECORATIONS;
	mwmhints.decorations=0;
	mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);
  
	XChangeProperty(display,window,mwmatom,mwmatom,32,
			PropModeReplace,(unsigned char *)&mwmhints,4);
  }
  
  /* Map and expose the window. */

  if(use_mouse) {
	/* grab the pointer and query MotionNotify events */

	XSelectInput(display, 
				 window, 
				 FocusChangeMask   | ExposureMask | 
				 KeyPressMask      | KeyReleaseMask | 
				 EnterWindowMask   | LeaveWindowMask |
				 PointerMotionMask | ButtonMotionMask |
				 ButtonPressMask   | ButtonReleaseMask |
				 resizeEvtMask 
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
				 KeyPressMask | KeyReleaseMask  |
				 resizeEvtMask 
				 );
  }
  
  XMapRaised(display,window);
  XClearWindow(display,window);
  XWindowEvent(display,window,ExposureMask,&event);
  
  XResizeWindow(display,window,winwidth,winheight);

  if(fullscreen) {
	hints.min_width	= hints.max_width = hints.base_width = screen->width;
	hints.min_height= hints.max_height = hints.base_height = screen->height;

	XSetWMNormalHints(display,window,&hints);
  }

  if ( (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) 
       && drawbitmap ) drawbitmap=drawbitmapvec;

  InitVScreen(depth);

#ifndef NDEBUG
  printf("GLINFO: xgl display created !\n");
#endif

  effect_init2(depth, depth, visual_width*widthscale);

  return OSD_OK;
}

/*
 * Shut down the display, also used to clean up if any error happens
 * when creating the display
 */

void sysdep_display_close (void)
{
   disp__glXMakeCurrent(display, None, NULL);
   disp__glXDestroyContext(display,glContext);
   glContext=0;

   CloseVScreen();  /* Shut down GL stuff */

   if(window) {
     /* ungrab the pointer */

     if(use_mouse) XUngrabPointer(display,CurrentTime);

     destroyOwnOverlayWin(display, &window, &window_attr);
     colormap=window_attr.colormap;
   }

   XSync(display,False); /* send all events to sync; */

#ifndef NDEBUG
   printf("GLINFO: xgl display closed !\n");
#endif
}

/* Swap GL video buffers */

void SwapBuffers(void)
{
  disp__glXSwapBuffers(display,window);
}

void toggleFullscreen()
{
   if(is_fullscreen)
   {
	XMoveResizeWindow(display, window, 
			  (fullscreen_width-orig_width)/2, 
			  (fullscreen_height-orig_height)/2,
			  orig_width, orig_height);
        XSync(display,False); /* send all events to sync; */
   } else {
	Window rootwin, childwin;
	int rx, ry, x, y, dx, dy;
	int w, h;
	unsigned int mask;
	Bool ok;

	/* sync with root window to coord 0/0 */
	XMoveWindow(display,window, 0, 0);
        XSync(display,False); /* send all events to sync; */

	/* get the diff of both orig. coord */
	ok = XQueryPointer(display, window, &rootwin, &childwin,
			    &rx, &ry, &x, &y, &mask);
	dx = rx-x;
	dy = ry-y;
	
	/* get the aspect future full screen size */
	w = fullscreen_width;
	h = fullscreen_height;
	xgl_fixaspectratio(&w, &h);

	/* the new coords .. */
	x = ( fullscreen_width  - w  ) / 2 - dx;
	y = ( fullscreen_height - h ) / 2 - dy;

#ifndef NDEBUG
	printf("GLINFO: switching to max aspect %d/%d, %dx%d\n", x, y, w, h);
#endif

	XMoveResizeWindow(display, window, x, y, w, h);
        XSync(display,False); /* send all events to sync; */
    }
    is_fullscreen = !is_fullscreen;
}
