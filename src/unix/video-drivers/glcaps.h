#ifndef _GLCAPS_H
	/**
	 * glcaps.h
	 *
	 * Copyright (C) 2001  Sven Goethel
	 *
	 * GNU Library General Public License 
	 * as published by the Free Software Foundation
	 *
	 * http://www.gnu.org/copyleft/lgpl.html
	 * General dynamical loading OpenGL (GL/GLU) support for:
	 */

	#define _GLCAPS_H

	#ifndef LIBAPIENTRY
                #define LIBAPIENTRY
        #endif
        #ifndef LIBAPI
                #define LIBAPI
        #endif

	#define BUFFER_SINGLE 0
	#define BUFFER_DOUBLE 1
	 
	#define COLOR_INDEX 0
	#define COLOR_RGBA  1
	 
	#define STEREO_OFF 0
	#define STEREO_ON  1

	typedef struct {
	  int buffer;
	  int color;
	  int stereo;
	  int depthBits;
	  int stencilBits;

	  int redBits;
	  int greenBits;
	  int blueBits;
	  int alphaBits;
	  int accumRedBits;
	  int accumGreenBits;
	  int accumBlueBits;
	  int accumAlphaBits;

	  /* internal use only */
	  int  gl_supported;
	  long nativeVisualID;
	} GLCapabilities;

	/**
	 * prints the contents of the GLCapabilities to stdout !
	 */
	LIBAPI void LIBAPIENTRY printGLCapabilities ( GLCapabilities *glCaps );
#endif
