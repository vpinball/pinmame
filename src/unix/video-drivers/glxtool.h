#ifndef _GLXTOOL_H
	/**
	 * glxtool.h
	 *
	 * Copyright (C) 2001  Sven Goethel
	 *
	 * GNU Library General Public License 
	 * as published by the Free Software Foundation
	 *
	 * http://www.gnu.org/copyleft/lgpl.html
	 * General dynamical loading OpenGL (GL/GLU) support for:
	 */

	#define _GLXTOOL_H

    #include <X11/Xlib.h>
  	#include <X11/Xutil.h>
  	#include <X11/Xatom.h>

	#ifndef SYMBOL_PREFIX
		#ifndef __ELF__
			#ifdef  __sgi
				#define SYMBOL_PREFIX   ""
			#else
				#define SYMBOL_PREFIX   "_"
			#endif
		#else
			#define SYMBOL_PREFIX	""
		#endif
	#endif
  
	#include "gltool.h"
	  
	#include "glx-disp-var.h"

	typedef struct {
		XVisualInfo * visual;
		GLXContext    gc;
		int           success;  /* 1: OK, 0: ERROR */
	} VisualGC;
  
	LIBAPI int LIBAPIENTRY get_GC
		( Display *display, Window win, XVisualInfo *visual,
		    GLXContext *gc, GLXContext gc_share,
		    int verbose );

        LIBAPI int LIBAPIENTRY setVisualAttribListByGLCapabilities( 
					int visualAttribList[/*>=32*/],
				        GLCapabilities *glCaps );

	LIBAPI VisualGC LIBAPIENTRY findVisualGlX( Display *display, 
				       Window rootWin,
				       Window * pWin, 
				       int width, int height,
				       GLCapabilities * glCaps,
				       int * pOwnWin,
			               XSetWindowAttributes * pOwnWinAttr,
				       unsigned long ownWinmask,
				       GLXContext shareWith,
				       int offscreen,
				       Pixmap *pix,
				       int verbose
				     );

	LIBAPI XVisualInfo * LIBAPIENTRY findVisualIdByID
				( XVisualInfo ** visualList, 
					int visualID, Display *disp,
					Window win, int verbose);

	LIBAPI XVisualInfo * LIBAPIENTRY findVisualIdByFeature
					( XVisualInfo ** visualList, 
					     Display *disp, Window win,
					     GLCapabilities *glCaps,
					     int verbose);

	LIBAPI int LIBAPIENTRY testVisualInfo 
			( Display *display, XVisualInfo * vi, 
			     GLCapabilities *glCaps, int verbose);

	LIBAPI int LIBAPIENTRY setGLCapabilities 
				( Display * disp, 
				XVisualInfo * visual, GLCapabilities *glCaps);

	LIBAPI void LIBAPIENTRY printAllVisualInfo 
				( Display *disp, Window win, int verbose);
	LIBAPI void LIBAPIENTRY printVisualInfo 
				( Display *display, XVisualInfo * vi);

	LIBAPI Window LIBAPIENTRY createOwnOverlayWin
			          (Display *display, 
				   Window rootwini, Window parentWin,
			           XSetWindowAttributes * pOwnWinAttr,
				   unsigned long ownWinmask,
				   XVisualInfo *visual, int width, int height);

	LIBAPI void LIBAPIENTRY destroyOwnOverlayWin
				 (Display *display, Window *newWin,
				  XSetWindowAttributes * pOwnWinAttr);

	/**
	 * do not call this one directly,
	 * use fetch_GL_FUNCS (gltool.h) instead
	 */
	LIBAPI void LIBAPIENTRY fetch_GLX_FUNCS 
				(const char * libGLName, 
					 const char * libGLUName, int force);

	LIBAPI int LIBAPIENTRY x11gl_myErrorHandler(
				  Display *pDisp, XErrorEvent *p_error);

	LIBAPI int LIBAPIENTRY x11gl_myIOErrorHandler(Display *pDisp);

#endif
