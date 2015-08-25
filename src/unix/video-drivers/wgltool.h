#ifndef _WGLTOOL_H
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

	#define _WGLTOOL_H

	#include "gltool.h"

	#include "glcaps.h"

	#include <windows.h>
	#include <wingdi.h>

	#include "wgl-disp-var.h"

	/**
	 * do not call this one directly,
	 * use fetch_GL_FUNCS (gltool.h) instead
	 */
	LIBAPI void LIBAPIENTRY fetch_WGL_FUNCS 
				(const char * libGLName, 
					 const char * libGLUName, int force);


	LIBAPI void LIBAPIENTRY setPixelFormatByGLCapabilities( 
						PIXELFORMATDESCRIPTOR *pfd,
						GLCapabilities *glCaps,
						int offScreenRenderer,
						HDC hdc);

	// Set Pixel Format function - forward declaration
	LIBAPI void LIBAPIENTRY SetDCPixelFormat(HDC hDC, GLCapabilities *glCaps,
			int offScreenRenderer, int verbose);

	LIBAPI HPALETTE LIBAPIENTRY GetOpenGLPalette(HDC hDC);

	LIBAPI HGLRC LIBAPIENTRY get_GC( HDC *hDC, GLCapabilities *glCaps,
			HGLRC shareWith, int offScreenRenderer,
			int width, int height, HBITMAP *pix,
			int verbose);

	LIBAPI int LIBAPIENTRY PixelFormatDescriptorFromDc( HDC Dc, 
			PIXELFORMATDESCRIPTOR *Pfd );

	const char * LIBAPIENTRY GetTextualPixelFormatByHDC(HDC hdc);

	const char * LIBAPIENTRY GetTextualPixelFormatByPFD(
			PIXELFORMATDESCRIPTOR *ppfd, int format);

	LIBAPI void LIBAPIENTRY setupDIB(HDC hDCOrig, HDC hDC, HBITMAP * hBitmap, 
			int width, int height);

	LIBAPI void LIBAPIENTRY resizeDIB(HDC hDC, HBITMAP *hOldBitmap, 
			HBITMAP *hBitmap);

	LIBAPI HPALETTE LIBAPIENTRY setupPalette(HDC hDC);

	LIBAPI int LIBAPIENTRY setGLCapabilities ( HDC hdc, int nPixelFormat,
								GLCapabilities *glCaps );

#endif
