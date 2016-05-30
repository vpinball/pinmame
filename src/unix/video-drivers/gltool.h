#ifndef _GLTOOL_H
	#define _GLTOOL_H

	/**
	 * gltool.h
	 *
	 * Copyright (C) 2001  Sven Goethel
	 *
	 * GNU Library General Public License 
	 * as published by the Free Software Foundation
	 *
	 * http://www.gnu.org/copyleft/lgpl.html
	 * General dynamical loading OpenGL (GL/GLU) support for:
	 *
	 *
	 * <OS - System>          <#define>  commentary
	 * -----------------------------------------------
	 * GNU/Linux, Unices/X11  _X11_      (loads glx also)
	 * Macinstosh OS9         _MAC_OS9_
	 * Macinstosh OSX         _MAC_OSX_
	 * Win32                  _WIN32_
	 *
	 */

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <string.h>

	#ifdef _WIN32_
		#include <windows.h>

		#ifdef LIBAPIENTRY
			#undef LIBAPIENTRY
		#endif
		#ifdef LIBAPI
			#undef LIBAPI
		#endif
	 
		#define LIBAPI          __declspec(dllexport)
		#define LIBAPIENTRY    __stdcall
	#else
		#include <ctype.h>
		#include <math.h>
		#define CALLBACK
	#endif

	#ifdef _X11_
		#include <dlfcn.h>
		#include <X11/Xlib.h>
		#include <X11/Xutil.h>
		#include <X11/Xatom.h>
		#include <GL/glx.h>
	#endif

	#ifdef _MAC_OS9_
		#include <agl.h>
		#include <CodeFragments.h>
		#include <Errors.h>
		#include <TextUtils.h>
		#include <StringCompare.h>
	 
		#define fragNoErr 0
	#endif

	#include <GL/gl.h>
	#include <GL/glu.h>

	#ifndef LIBAPIENTRY
                #define LIBAPIENTRY
        #endif
        #ifndef LIBAPI
                #define LIBAPI extern
        #endif

	LIBAPI const char * GLTOOL_USE_GLLIB ;
	LIBAPI const char * GLTOOL_USE_GLULIB ;

	#include "glcaps.h"

	#include "gl-disp-var.h"
	#include "glu-disp-var.h"

	#ifndef GLDEBUG
		#ifndef NDEBUG
			#define NDEBUG
		#endif
	#else
		#ifdef NDEBUG
			#undef NDEBUG
		#endif
	#endif

	#ifndef USE_64BIT_POINTER
		typedef int  PointerHolder;
	#else
		typedef long PointerHolder;
	#endif

	#ifdef _WIN32_
		#ifndef NDEBUG
			#define CHECK_WGL_ERROR(a,b,c) check_wgl_error(a,b,c)
		#else
			#define CHECK_WGL_ERROR(a,b,c)
		#endif
	#else
		#define CHECK_WGL_ERROR(a,b,c)
	#endif

	#ifndef NDEBUG
		#define PRINT_GL_ERROR(a, b)	print_gl_error((a), __FILE__, __LINE__, (b))
		#define CHECK_GL_ERROR()  	check_gl_error(__FILE__,__LINE__)
		#define GL_BEGIN(m) 		__sglBegin(__FILE__, __LINE__, (m))
		#define GL_END()    		__sglEnd  (__FILE__, __LINE__)
		#define SHOW_GL_BEGINEND()	showGlBeginEndBalance(__FILE__, __LINE__)
		#define CHECK_GL_BEGINEND()	checkGlBeginEndBalance(__FILE__, __LINE__)
	#else
		#define PRINT_GL_ERROR(a, b)	
		#define CHECK_GL_ERROR()  
		#define GL_BEGIN(m) 		disp__glBegin(m)
		#define GL_END()    		disp__glEnd  ()
		#define SHOW_GL_BEGINEND()	
		#define CHECK_GL_BEGINEND()	
	#endif

	#ifdef _WIN32_
		LIBAPI void LIBAPIENTRY check_wgl_error 
			(HWND wnd, const char *file, int line);
	#endif

	LIBAPI void LIBAPIENTRY print_gl_error 
		(const char *msg, const char *file, int line, GLenum errorcode);

	LIBAPI void LIBAPIENTRY check_gl_error 
		(const char *file, int line);

	LIBAPI void LIBAPIENTRY showGlBeginEndBalance
		(const char *file, int line);

	LIBAPI void LIBAPIENTRY checkGlBeginEndBalance
		(const char *file, int line);

	LIBAPI void LIBAPIENTRY __sglBegin
		(const char * file, int line, GLenum mode);

	LIBAPI void LIBAPIENTRY __sglEnd
		(const char * file, int line);

	LIBAPI int LIBAPIENTRY unloadGLLibrary (void);

	LIBAPI int LIBAPIENTRY loadGLLibrary 
        	(const char * libGLName, const char * libGLUName);

	LIBAPI void * LIBAPIENTRY getGLProcAddressHelper 
		(const char * libGLName, const char * libGLUName,
		 const char *func, int *method, int debug, int verbose);

        LIBAPI void LIBAPIENTRY fetch_GL_FUNCS 
		(const char * libGLName, const char * libGLUName, int force);
#endif
