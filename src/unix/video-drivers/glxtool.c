/**
 * glxtool.c
 *
 * Copyright (C) 2001  Sven Goethel
 *
 * GNU Library General Public License 
 * as published by the Free Software Foundation
 *
 * http://www.gnu.org/copyleft/lgpl.html
 * General dynamical loading OpenGL (GL/GLU) support for:
 */

#include "glxtool.h"

#include "glx-disp-var.hc"

/**
 * do not call this one directly,
 * use fetch_GL_FUNCS (gltool.c) instead
 */
void LIBAPIENTRY fetch_GLX_FUNCS (const char * libGLName, 
			          const char * libGLUName, int force)
{
  static int _firstRun = 1;

  if(force)
        _firstRun = 1;

  if(!_firstRun)
  	return;

  #define GET_GL_PROCADDRESS(a) getGLProcAddressHelper (libGLName, libGLUName, ( SYMBOL_PREFIX a), NULL, 1, 0);

  #include "glx-disp-fetch.hc"

  _firstRun=0;
}

/*
 * Name      : get_GC
 *
 * Parameters: win    - the X window use to the OpenGL context with
 *             visual - The visual to create the context for
 *             gc     - a pointer to a GLXContext structure. This is how
 *                      the created context will be returned to the caller
 *
 * Returns   : a pointer to a created GLXContext is returned through the
 *             gc argument.
 *             int - an error code: 0 means everything was fine
 *                                 -1 context creation failed
 *                                 -2 context/window association failed
 *
 * Purpose   : create an X window Graphics context and assocaite it with
 *             the window. It returns 0 if everything was fine, -1 if the
 *             context could not be created, -2 if the context could not
 *             be associated with the window
 */
int LIBAPIENTRY get_GC( Display *display, Window win, XVisualInfo *visual,
            GLXContext *gc, GLXContext gc_share,
	    int verbose )
{
    int trial = 2;

    while(trial>0)
    {
    	switch(trial)
	{
	  case 2:
	    *gc = disp__glXCreateContext( display, visual, gc_share, GL_TRUE );
	    break;
	  case 1:
	    *gc = disp__glXCreateContext( display, visual, gc_share, GL_FALSE );
	    break;
	}
    	trial--;

        /* check if the context could be created */
        if( *gc == NULL ) {
	    continue;
        }

        /* associated the context with the X window */
        if( disp__glXMakeCurrent( display, win, *gc ) == False) {
	    disp__glXDestroyContext( display, *gc );
            if(verbose)
	    {
	      fprintf(stderr, "GLINFO: glXCreateContext/glXDestroyContext trial(%d) %p\n", trial, (void *)*gc);
	    }
	    continue;
        } else {
	    if(verbose)
		fprintf(stderr, "GLINFO: glXCreateContext trial (%d) sure %p\n", trial, (void *)*gc);
	    return 0;
        }
    }

    return -2;
}


int LIBAPIENTRY setVisualAttribListByGLCapabilities( 
					int visualAttribList[/*>=32*/],
				        GLCapabilities *glCaps )
{
    int i=0;
    visualAttribList[i++] = GLX_USE_GL;    /* paranoia .. */
    visualAttribList[i++] = GLX_RED_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_GREEN_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_BLUE_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_DEPTH_SIZE;
    visualAttribList[i++] = 1;
    visualAttribList[i++] = GLX_ACCUM_RED_SIZE;
    visualAttribList[i++] = (glCaps->accumRedBits>0)?1:0;
    visualAttribList[i++] = GLX_ACCUM_GREEN_SIZE;
    visualAttribList[i++] = (glCaps->accumGreenBits>0)?1:0;
    visualAttribList[i++] = GLX_ACCUM_BLUE_SIZE;
    visualAttribList[i++] = (glCaps->accumBlueBits>0)?1:0;

    if(COLOR_RGBA == glCaps->color)
    {
	    visualAttribList[i++] = GLX_RGBA;
	    visualAttribList[i++] = GLX_ALPHA_SIZE;
	    visualAttribList[i++] = (glCaps->alphaBits>0)?1:0;
	    visualAttribList[i++] = GLX_ACCUM_ALPHA_SIZE;
	    visualAttribList[i++] = (glCaps->accumAlphaBits>0)?1:0;
    }
    if(BUFFER_DOUBLE==glCaps->buffer)
	    visualAttribList[i++] = GLX_DOUBLEBUFFER;

    if(STEREO_ON==glCaps->stereo)
	    visualAttribList[i++] = GLX_STEREO;

    visualAttribList[i++] = GLX_STENCIL_SIZE;
    visualAttribList[i++] = glCaps->stencilBits;
    visualAttribList[i] = None;
    return i;
}

/**
 * Description for pWin:
 *   if ownwin:
 *      input: 	the parent window
 *      output: the newly created window
 *   else if ! ownwin:
 *      i/o:    the window itself
 */
VisualGC LIBAPIENTRY findVisualGlX( Display *display, 
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
			     )
{
    Window newWin = 0;
    int visualAttribList[64];
    int j=0;
    int done=0;
    VisualGC vgc = { NULL, 0, 0 };
    int tryChooseVisual = 1;
    int gc_ret = 0;
    int ownwin = 0;
    GLCapabilities _glCaps;
    int accumTestDone=0;

    /** 
     * The Visual seeked by Function: findVisualIdByFeature !
     */
    XVisualInfo * visualList=NULL; /* the visual list, to be XFree-ed */

    /* paranoia .. */
    glCaps->gl_supported = 1;

    /* backup ... */
    _glCaps = *glCaps;

    if(pOwnWin) ownwin=*pOwnWin;

    do {
            if(verbose)
	    {
	         fprintf(stderr, "GLINFO: seeking visual loop# %d\n", j);
	         fprintf(stderr, "GLINFO: seeking gl capabilities:\n");
		 printGLCapabilities ( glCaps );
	    }

            if(glCaps->nativeVisualID>=0 && j==0)
	    {
	        /**
		 * if somebody passes us a nativeVisualID,
		 * we will try to use it .. the 1st time only
		 */
	    	vgc.visual = findVisualIdByID(&visualList, 
		                              (int)(glCaps->nativeVisualID), 
					       display, *pWin, verbose);
	    }

	    if(vgc.visual==NULL)
	    {
                glCaps->nativeVisualID=0;
                (void) setVisualAttribListByGLCapabilities( 
						visualAttribList, glCaps);
	    }

            if(tryChooseVisual && vgc.visual==NULL)
	    {
		    vgc.visual = disp__glXChooseVisual( display,
						  DefaultScreen( display ),
						  visualAttribList );
		    if(verbose)
		    {
			if(vgc.visual!=NULL)
			{
			    fprintf(stdout, "findVisualGlX.glXChooseVisual: found visual(ID:%d(0x%X))\n", 
				(int) vgc.visual->visualid,
				(int) vgc.visual->visualid);
			        printVisualInfo ( display, vgc.visual );
			} else {
			    fprintf(stdout, "findVisualGlX.glXChooseVisual: no visual\n");
			}
			fflush(stdout);
		    }
	    }

	    if(vgc.visual==NULL)
	    {
	        vgc.visual = findVisualIdByFeature( &visualList, 
				                      display, *pWin,
						      glCaps,
						      verbose);
	    }

	    if( ownwin && vgc.visual!=NULL)
	    {
		if(verbose)
		{
		    fprintf(stdout, "findVisualGlX: CREATING OWN WINDOW !\n");
		    fflush(stdout);
		}
		newWin = createOwnOverlayWin(display, 
					     rootWin, 
					     *pWin /* the parent */,
			       		     pOwnWinAttr,
			                     ownWinmask,
					     vgc.visual, width, height);
	    }

	    if( offscreen && vgc.visual!=NULL)
	    {
	        if(pix!=NULL && *pix!=0)
		{
			XFreePixmap(display, *pix);
		}
		if(pix!=NULL && vgc.visual !=NULL)
	    		*pix = XCreatePixmap( display, rootWin, width, height, 
		                              vgc.visual->depth); 
		if(pix!=NULL && *pix!=0)
		{
	           *pWin = disp__glXCreateGLXPixmap( display,  vgc.visual, *pix );
		   if(*pWin==0)
		   {
		   	XFreePixmap(display, *pix);
			*pix=0;
			fprintf(stderr, "GLINFO(%d): glXCreateGLXPixmap failed\n", j);
			fflush(stderr);
		   }
		} else {
		   fprintf(stderr, "GLINFO(%d): XCreatePixmap failed\n", j);
		   fflush(stderr);
		   *pWin = 0;
		}
                if(verbose)
		{
			if(*pWin!=0)
			{
			   fprintf(stderr, "GLINFO(%d): pixmap ok\n", j);
			   fflush(stderr);
			}
		}
	    }

	    gc_ret = -100;

	    if( ownwin && newWin!=0 && 
	        gc_ret==-100 /* just a poss. test stage */ &&
	        (gc_ret=get_GC( display, newWin,
			        vgc.visual, &(vgc.gc), shareWith, verbose)) == 0
              )
	    {
		    vgc.success=1;
		    *pWin = newWin ;
	    }
	    else if( vgc.visual!=NULL &&  !ownwin && *pWin!=0 &&
	             (gc_ret=get_GC( display, *pWin,
			             vgc.visual, &(vgc.gc), shareWith, verbose)) == 0
	           )    
	    {
		    vgc.success=1;
            } else 
	    {
	    	    j++; /* trial counter */
		    if(verbose)
		    {
		        fprintf(stderr, "GLINFO(%d): Visual fetching failed (gc_get=%d)\n", j, gc_ret);
			fflush(stderr);
		    }

		    if(pix!=NULL && *pix!=0)
		    {
		   	XFreePixmap(display, *pix);
			*pix=0;
		    }

		    if(ownwin && newWin!=0)
		    {
		        if(verbose)
		        {
		          fprintf(stdout, "findVisualGlX: FREE OWN WINDOW !\n");
		          fflush(stdout);
		        }
		        destroyOwnOverlayWin(display, &newWin,
			                     pOwnWinAttr);
		    }

		    if(visualList!=NULL)
		    {
			XFree(visualList);
			visualList=NULL;
	    		vgc.visual=NULL;
		    } else if(vgc.visual!=NULL)
		    {
			XFree(vgc.visual);
	    		vgc.visual=NULL;
		    }

                    glCaps->nativeVisualID=-1;

		    /**
		     * Falling-Back the exact (min. requirement) parameters ..
		     */
		    if(!ownwin && !offscreen) {
		        *glCaps=_glCaps;
			ownwin=1;
			if(pOwnWin) *pOwnWin=ownwin;
		    } else if(glCaps->stereo==STEREO_ON) {
			glCaps->stereo=STEREO_OFF;
		    } else if(glCaps->stencilBits>32) {
		        glCaps->stencilBits=32;
		    } else if(glCaps->stencilBits>16) {
		        glCaps->stencilBits=16;
		    } else if(glCaps->stencilBits>8) {
		        glCaps->stencilBits=8;
		    } else if(glCaps->stencilBits>0) {
		        glCaps->stencilBits=0;
		    } else if( glCaps->alphaBits>0 ||
		               glCaps->accumAlphaBits>0
			     )
	            {
			glCaps->alphaBits=0;
			glCaps->accumAlphaBits=0;
		    } else if( accumTestDone==0 &&
		               ( glCaps->accumRedBits==0 ||
		                 glCaps->accumGreenBits==0 ||
		                 glCaps->accumBlueBits==0
			       )
			     ) 
	            {
		        glCaps->accumRedBits=1;
			glCaps->accumGreenBits=1;
			glCaps->accumBlueBits=1;
		    } else if( glCaps->accumRedBits>0 ||
		               glCaps->accumGreenBits>0 ||
		               glCaps->accumBlueBits>0
			     ) 
	            {
		        glCaps->accumRedBits=0;
			glCaps->accumGreenBits=0;
			glCaps->accumBlueBits=0;
			accumTestDone=1;
		    } else if(glCaps->buffer==BUFFER_SINGLE && 
		              offscreen) 
		    {
		        glCaps->buffer=BUFFER_DOUBLE;
		    } else if(glCaps->buffer==BUFFER_DOUBLE) {
		        glCaps->buffer=BUFFER_SINGLE;
		    } else if(tryChooseVisual) {
		        *glCaps=_glCaps;
			tryChooseVisual=0;
			accumTestDone=0;
		    } else done=1;
	    }
    } while (vgc.success==0 && done==0) ;

    if(vgc.success==1 && verbose)
    {
	    (void) setGLCapabilities (display, vgc.visual, glCaps);

	    fprintf(stderr, "\nfindVisualGlX results vi(ID:%d): \n screen %d, depth %d, class %d,\n clrmapsz %d, bitsPerRGB %d, shared with %d\n",
		(int)vgc.visual->visualid,
		(int)vgc.visual->screen,
		(int)vgc.visual->depth,
		(int)vgc.visual->class,
		(int)vgc.visual->colormap_size,
		(int)vgc.visual->bits_per_rgb,
		(int)shareWith);
    }

    return vgc;
}


XVisualInfo * LIBAPIENTRY findVisualIdByID( XVisualInfo ** visualList, 
			        int visualID, Display *disp,
			        Window win, int verbose)
{
    XVisualInfo    *    vi=0;
    XVisualInfo    	viTemplate;
    int 		i, numReturns;
    XWindowAttributes 	xwa;
    int 		done=0;

    if(XGetWindowAttributes(disp, win, &xwa) == 0)
    {
	fprintf(stderr, "\nERROR while fetching XWindowAttributes\n");
	fflush(stderr);
	return 0;
    }

    viTemplate.screen = DefaultScreen( disp );
    viTemplate.class = (xwa.visual)->class ;
    viTemplate.depth = xwa.depth;

    *visualList = XGetVisualInfo( disp, VisualScreenMask,
	                          &viTemplate, &numReturns ); 

    for(i=0; done==0 && i<numReturns; i++)
    {
        vi = &((*visualList)[i]);

	if(vi->visualid==visualID)
	{
                if(verbose)
		{
		    fprintf(stderr, "findVisualIdByID: Found matching Visual:\n");
		    printVisualInfo ( disp, vi);
		}
		return vi;
	}
    }

    if(verbose)
    {
	    if( numReturns==0 )
		fprintf(stderr, "findVisualIdByID: No available visuals. Exiting...\n" );
	    else if( i>=numReturns )
		fprintf(stderr, "findVisualIdByID: No matching visualID found ...\n" );
	    fflush(stderr);
    }
     
    XFree(*visualList);
    *visualList=NULL;
    return NULL;
}

XVisualInfo * LIBAPIENTRY findVisualIdByFeature( XVisualInfo ** visualList, 
                                     Display *disp, Window win,
				     GLCapabilities *glCaps,
				     int verbose)
{
    XVisualInfo    *    vi=0;
    XVisualInfo    	viTemplate;
    int 		i, numReturns;
    XWindowAttributes 	xwa;
    int 		done=0;

    if(XGetWindowAttributes(disp, win, &xwa) == 0)
    {
	fprintf(stderr, "\nERROR while fetching XWindowAttributes\n");
	fflush(stderr);
	return 0;
    }

    viTemplate.screen = DefaultScreen( disp );
    viTemplate.class = (xwa.visual)->class ;
    viTemplate.depth = xwa.depth;

    *visualList = XGetVisualInfo( disp, VisualScreenMask,
	                          &viTemplate, &numReturns ); 

    for(i=0; done==0 && i<numReturns; i++)
    {
        vi = &((*visualList)[i]);
	if ( testVisualInfo ( disp, vi, glCaps, verbose ) )
	{
                if(verbose)
		{
		    fprintf(stderr, "findVisualIdByFeature: Found matching Visual:\n");
		    printVisualInfo ( disp, vi);
		}
		return vi;
	}
    }

    if(verbose)
    {
	    if( numReturns==0 )
		fprintf(stderr, "findVisualIdByFeature: No available visuals. Exiting...\n" );
	    else if( i>=numReturns )
		fprintf(stderr, "findVisualIdByFeature: No matching visual found ...\n" );
	    fflush(stderr);
    }
     
    XFree(*visualList);
    *visualList=NULL;
    return NULL;
}

int LIBAPIENTRY setGLCapabilities ( Display * disp, 
                        XVisualInfo * visual, GLCapabilities *glCaps)
{
    int iValue=0;
    int iValue1=0;
    int iValue2=0;
    int iValue3=0;

    memset(glCaps, 0, sizeof(GLCapabilities));

    if(disp__glXGetConfig( disp, visual, GLX_USE_GL, &iValue)==0)
    {
	glCaps->gl_supported=(iValue==True)?1:0;
    } else {
	fprintf(stderr,"GLINFO: fetching GLX_USE_GL state failed\n");
	fflush(stderr);
    }

    if( ! glCaps->gl_supported )
    	return 0;

    if(disp__glXGetConfig( disp, visual, GLX_DOUBLEBUFFER, &iValue)==0)
    {
	glCaps->buffer=iValue?BUFFER_DOUBLE:BUFFER_SINGLE;
    } else {
	fprintf(stderr,"GLINFO: fetching doubleBuffer state failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig( disp, visual, GLX_RGBA, &iValue)==0)
    {
	glCaps->color=iValue?COLOR_RGBA:COLOR_INDEX;
    } else {
	fprintf(stderr,"GLINFO: fetching rgba state failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig( disp, visual, GLX_STEREO, &iValue)==0)
    {
	glCaps->stereo=iValue?STEREO_ON:STEREO_OFF;
    } else {
	fprintf(stderr,"GLINFO: fetching stereoView state failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig( disp, visual, GLX_DEPTH_SIZE, &iValue)==0)
    {
        glCaps->depthBits = iValue;
    } else {
	fprintf(stderr,"GLINFO: fetching depthBits state failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig( disp, visual, GLX_STENCIL_SIZE, &iValue)==0)
    {
        glCaps->stencilBits = iValue;
    } else {
	fprintf(stderr,"GLINFO: fetching stencilBits state failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig(disp,visual,GLX_RED_SIZE, &iValue)==0 &&
       disp__glXGetConfig(disp,visual,GLX_GREEN_SIZE, &iValue1)==0 &&
       disp__glXGetConfig(disp,visual,GLX_BLUE_SIZE, &iValue2)==0 &&
       disp__glXGetConfig(disp,visual,GLX_ALPHA_SIZE, &iValue3)==0 )
    {
        glCaps->redBits = iValue;
        glCaps->greenBits= iValue1;
        glCaps->blueBits= iValue2;
        glCaps->alphaBits= iValue3;
    } else {
	fprintf(stderr,"GLINFO: fetching rgba Size states failed\n");
	fflush(stderr);
    }

    if(disp__glXGetConfig(disp,visual,GLX_ACCUM_RED_SIZE, &iValue)==0 &&
       disp__glXGetConfig(disp,visual,GLX_ACCUM_GREEN_SIZE, &iValue1)==0 &&
       disp__glXGetConfig(disp,visual,GLX_ACCUM_BLUE_SIZE, &iValue2)==0 &&
       disp__glXGetConfig(disp,visual,GLX_ACCUM_ALPHA_SIZE, &iValue3)==0 )
    {
        glCaps->accumRedBits = iValue;
        glCaps->accumGreenBits= iValue1;
        glCaps->accumBlueBits= iValue2;
        glCaps->accumAlphaBits= iValue3;
    } else {
	fprintf(stderr,"GLINFO: fetching rgba AccumSize states failed\n");
	fflush(stderr);
    }
    glCaps->nativeVisualID=(long) (visual->visualid);

    return 1;
}

int LIBAPIENTRY testVisualInfo ( Display *display, XVisualInfo * vi, 
		     GLCapabilities *glCaps, int verbose)
{
    GLCapabilities _glCaps;
    setGLCapabilities ( display, vi, &_glCaps);

    /*
    if(verbose)
    {
            fprintf(stderr, "testVisualInfo vi(ID:%d(0x%X)):\n",
	            (int) vi->visualid, (int) vi->visualid);
	    printGLCapabilities ( &_glCaps );
    }
    */

    if(_glCaps.gl_supported != glCaps->gl_supported) return 0;

    if(_glCaps.buffer!=glCaps->buffer) return 0;

    if(_glCaps.color!=glCaps->color) return 0;

    if(_glCaps.stereo!=glCaps->stereo) return 0;

    if( ( glCaps->stencilBits>0 && 
          _glCaps.stencilBits<glCaps->stencilBits
	) ||
        (
	  glCaps->stencilBits==0 &&
	  _glCaps.stencilBits!=glCaps->stencilBits
	)
      )
    	return 0;

    if( ( glCaps->accumRedBits>0 && 
          _glCaps.accumRedBits<glCaps->accumRedBits
	) ||
        (
	  glCaps->accumRedBits==0 &&
          _glCaps.accumRedBits!=glCaps->accumRedBits
	)
      )
    	return 0;

    if( ( glCaps->accumGreenBits>0 && 
          _glCaps.accumGreenBits<glCaps->accumGreenBits
	) ||
        (
	  glCaps->accumGreenBits==0 &&
          _glCaps.accumGreenBits!=glCaps->accumGreenBits
	)
      )
    	return 0;

    if( ( glCaps->accumBlueBits>0 && 
          _glCaps.accumBlueBits<glCaps->accumBlueBits
	) ||
        (
	  glCaps->accumBlueBits==0 &&
          _glCaps.accumBlueBits!=glCaps->accumBlueBits
	)
      )
    	return 0;

    if( ( glCaps->alphaBits>0 && 
	  _glCaps.alphaBits<glCaps->alphaBits
	) ||
        (
	  glCaps->alphaBits==0 &&
	  _glCaps.alphaBits!=glCaps->alphaBits
	)
      )
	return 0;

    if( glCaps->alphaBits>0 )
    {
	    if( ( glCaps->accumAlphaBits>0 && 
		  _glCaps.accumAlphaBits<glCaps->accumAlphaBits
		) ||
                (
	          glCaps->accumAlphaBits==0 &&
		  _glCaps.accumAlphaBits!=glCaps->accumAlphaBits
	        )
	      )
		return 0;
    }

    return 1;
}


void LIBAPIENTRY printVisualInfo ( Display *display, XVisualInfo * vi)
{
    GLCapabilities glCaps;

    setGLCapabilities ( display, vi, &glCaps);

    fprintf(stdout, "\nvi(ID:%d(0x%X)): \n screen %d, depth %d, class %d,\n clrmapsz %d, bitsPerRGB %d\n",
	(int) vi->visualid,
	(int) vi->visualid,
	(int) vi->screen,
	(int) vi->depth,
	(int) vi->class,
	(int) vi->colormap_size,
	(int) vi->bits_per_rgb );

    printGLCapabilities ( &glCaps );
}

void LIBAPIENTRY printAllVisualInfo ( Display *disp, Window win, int verbose)
{
    XVisualInfo    *	visualInfo=0;    
    XVisualInfo    *    vi=0;
    XVisualInfo    	viTemplate;
    int 		i, numReturns;
    XWindowAttributes 	xwa;

    if(XGetWindowAttributes(disp, win, &xwa) == 0)
    {
	fprintf(stderr, "\nERROR while fetching XWindowAttributes\n");
	fflush(stderr);
	return;
    }

    viTemplate.screen = DefaultScreen( disp );
    viTemplate.class = (xwa.visual)->class ;
    viTemplate.depth = xwa.depth;

    visualInfo = XGetVisualInfo( disp, VisualScreenMask,
	&viTemplate, &numReturns ); 

    if(verbose)
    {
	    fprintf(stderr, "\nNum of Visuals : %d\n", numReturns );

	    for(i=0; i<numReturns; i++)
	    {
	    	    vi = &(visualInfo[i]);
		    printVisualInfo ( disp, vi);
	    }
    }

    XFree(visualInfo); 	
}

Window LIBAPIENTRY createOwnOverlayWin
			(Display *display, Window rootwini, Window parentWin,
			   XSetWindowAttributes * pOwnWinAttr,
			   unsigned long ownWinmask,
                           XVisualInfo *visual, int width, int height)
{				  
  /*
  //------------------------------------------------------------------------//
  // Some Systems (SGI) wont give up the window and so we have to create a  //
  // window that fits on top of the Java Canvas.                            //
  //------------------------------------------------------------------------//
  */
  Window window=0;
  XSetWindowAttributes   attribs ;
  unsigned long winmask = 0;

  memset(&attribs, 0, sizeof(XSetWindowAttributes));

  if(pOwnWinAttr) attribs=*pOwnWinAttr;
  if(ownWinmask!=0) winmask=ownWinmask;

  if(winmask==0)
  {
  	winmask= CWBitGravity  | CWColormap | CWBorderPixel | CWBackPixel ;
  }

  /*
    //----------------------------------------------------------------------//
    // now we have the visual with the best depth so lets make a color map  //
    // for it.  we use allocnone because this is a true colour visual and   //
    // the color map is read only anyway. This must be done because we      //
    // cannot call XCreateSimpleWindow.                                     // 
    //----------------------------------------------------------------------//
  */
  if(attribs.colormap==0 && (winmask & CWColormap)>0 )
    attribs.colormap = XCreateColormap ( display
                               , rootwini
                               , visual->visual
                               , AllocNone 
                               );

  /*
    //----------------------------------------------------------------------//
    // Set up the window attributes.                                        //
    //----------------------------------------------------------------------//
  */
  if(pOwnWinAttr==NULL)
  {
    attribs.event_mask       = ExposureMask                              ;
    attribs.border_pixel     = BlackPixel(display, DefaultScreen(display)) ;
    attribs.bit_gravity      = SouthWestGravity ;                          ;
    attribs.background_pixel = 0xFF00FFFF                                  ;  
  }
   
  /*
    //----------------------------------------------------------------------//
    // Create a window.                                                     //
    //----------------------------------------------------------------------//
  */
    window = XCreateWindow( display
                          , parentWin  
                          , 0
                          , 0
                          , width
                          , height
                          , 0
                          , visual->depth
                          , InputOutput
                          , visual->visual
			  , winmask
                          , &attribs);
    
    if(pOwnWinAttr) *pOwnWinAttr=attribs;

    return window;
}

void LIBAPIENTRY destroyOwnOverlayWin(Display *display, Window *newWin,
			   XSetWindowAttributes * pOwnWinAttr)
{				  
  if(newWin!=NULL)
  {
  	XDestroyWindow( display, *newWin );
	*newWin=0;
  }

  if(pOwnWinAttr!=NULL && pOwnWinAttr->colormap!=0)
  {
    XFreeColormap(display, pOwnWinAttr->colormap);
    pOwnWinAttr->colormap=0;
  }
}

int LIBAPIENTRY x11gl_myErrorHandler(Display *pDisp, XErrorEvent *p_error)
{
	char err_msg[80];

	XGetErrorText(pDisp, p_error->error_code, err_msg, 80);
	fprintf(stderr, "X11 Error detected.\n %s\n", err_msg);
	fprintf(stderr, " Protocol request: %d\n", p_error->request_code);
	fprintf(stderr, " Resource ID : 0x%x\n", (int)p_error->resourceid);
	fprintf(stderr, " \ntrying to continue ... \n");
	fflush(stderr);
	return 0;
}

int LIBAPIENTRY x11gl_myIOErrorHandler(Display *pDisp)
{
	fprintf(stderr, "X11 I/O Error detected.\n");
	fprintf(stderr, " \ndo not know what to do ... \n");
	fflush(stderr);
	return 0;
}

