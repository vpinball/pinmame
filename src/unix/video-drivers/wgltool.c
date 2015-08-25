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

#include "wgltool.h"

#include "wgl-disp-var.hc"

/**
 * do not call this one directly,
 * use fetch_GL_FUNCS (gltool.c) instead
 */
void LIBAPIENTRY fetch_WGL_FUNCS (const char * libGLName, 
			          const char * libGLUName, int force)
{
  static int _firstRun = 1;

  if(force)
        _firstRun = 1;

  if(!_firstRun)
  	return;

  if(!loadGLLibrary (libGLName, libGLUName))
  	return;

  #define GET_GL_PROCADDRESS(a) getGLProcAddressHelper (libGLName, libGLUName, (a), NULL, 1, 0);


  #include "wgl-disp-fetch.hc"

  _firstRun=0;
}


HGLRC LIBAPIENTRY get_GC( HDC * hDC, GLCapabilities *glCaps,
		HGLRC shareWith, 
		int offScreenRenderer,
		int width, int height, HBITMAP *pix,
		int verbose)

{
	const char * text=0;
	HDC hDCOrig = 0;

	// Color Palette handle
	HPALETTE hPalette = NULL;
	HGLRC tempRC=0;

    if( *hDC == 0 && !offScreenRenderer)
		printf( "get_GC: Error, HDC is zero\n");

	// Select the pixel format
	if(offScreenRenderer)
	{
		hDCOrig = *hDC;
		*hDC = CreateCompatibleDC(hDCOrig);
		// setupDIB(*hDC, pix, width, height);
		setupDIB(hDCOrig, *hDC, pix, width, height);
		/* SetDCPixelFormat(hDCOffScr, doubleBuffer, stereo, stencilBits, offScreenRenderer); */
		/* setupPalette(hDC); USE MY PROC */
	}

	SetDCPixelFormat(*hDC, glCaps, offScreenRenderer, verbose);

	// Create palette if needed
	hPalette = GetOpenGLPalette(*hDC);

    tempRC = disp__wglCreateContext( *hDC );

	if(verbose)
	{
		fprintf(stderr,"\n\nPIXELFORMAT OF GL-Context SETTINGS:\n");
		text=GetTextualPixelFormatByHDC(*hDC);
		fprintf(stderr,text);
	}

    /* check if the context could be created */
    if( tempRC == NULL ) {
        fprintf(stderr, "getGC context could NOT be created \n");
        return( 0 );
    }

    /* associated the context with the X window */
    if( disp__wglMakeCurrent( *hDC, tempRC ) == FALSE) {
		fprintf(stderr,"wglMakeCurrent(%p,%p) failed on new context!!!\n", *hDC,tempRC);
		fprintf(stderr,"Error code = %d\n",(int)GetLastError());
		disp__wglMakeCurrent(NULL, NULL);
		disp__wglDeleteContext( tempRC );
        return( 0 );
    }

	if(shareWith!=NULL && disp__wglShareLists(shareWith, tempRC)==FALSE)
	{
		fprintf(stderr,"\nERROR: Could not share lists between the new and the given GLContext (Win32Native)!\n");
		fprintf(stderr,"Error code = %d\n",(int)GetLastError());
		disp__wglMakeCurrent(NULL, NULL);
		disp__wglDeleteContext( tempRC );
        return( 0 );
	}

    if(verbose)
			printf( "HGLRC (glContext) created: %p\n", tempRC );

    return tempRC;
}

void LIBAPIENTRY setPixelFormatByGLCapabilities( 
					PIXELFORMATDESCRIPTOR *pfd,
				        GLCapabilities *glCaps,
					int offScreenRenderer,
					HDC hdc)
{

	int colorBits = glCaps->redBits + glCaps->greenBits + glCaps->blueBits;

	pfd->nSize=sizeof(PIXELFORMATDESCRIPTOR); 
	pfd->nVersion=1; 
	pfd->dwFlags=PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED; /* refined later */
	pfd->iPixelType=0; 
	pfd->cColorBits=0; 
	pfd->cRedBits=0; 
	pfd->cRedShift=0; 
	pfd->cGreenBits=0; 
	pfd->cGreenShift=0; 
	pfd->cBlueBits=0; 
	pfd->cBlueShift=0; 
	pfd->cAlphaBits=0; 
	pfd->cAlphaShift=0; 
	pfd->cAccumBits=0; 
	pfd->cAccumRedBits=0; 
	pfd->cAccumGreenBits=0; 
	pfd->cAccumBlueBits=0; 
	pfd->cAccumAlphaBits=0; 
	pfd->cDepthBits=32; 
	pfd->cStencilBits=0; 
	pfd->cAuxBuffers=0; 
	pfd->iLayerType=PFD_MAIN_PLANE; 
	pfd->bReserved=0; 
	pfd->dwLayerMask=0; 
	pfd->dwVisibleMask=0; 
	pfd->dwDamageMask=0; 
 
    if(COLOR_RGBA == glCaps->color)
		pfd->iPixelType=PFD_TYPE_RGBA; 
	else
		pfd->iPixelType=PFD_TYPE_COLORINDEX; 

    if(offScreenRenderer)
		pfd->dwFlags |= PFD_DRAW_TO_BITMAP;           // Draw to Bitmap
	else
		pfd->dwFlags |= PFD_DRAW_TO_WINDOW;           // Draw to Window (not to bitmap)


    if(BUFFER_DOUBLE==glCaps->buffer)
		pfd->dwFlags |= PFD_DOUBLEBUFFER ;  // Double buffered is optional

    if(STEREO_ON==glCaps->stereo)
		pfd->dwFlags |= PFD_STEREO ;        // Stereo is optional

    if(hdc!=NULL && GetDeviceCaps(hdc, BITSPIXEL)<colorBits)
    	    pfd->cColorBits = GetDeviceCaps(hdc, BITSPIXEL);
        else
    pfd->cColorBits = (BYTE)colorBits;

	pfd->cStencilBits = (BYTE) glCaps->stencilBits;
}


void LIBAPIENTRY SetDCPixelFormat(HDC hDC, GLCapabilities *glCaps,
		                  int offScreenRenderer, int verbose)
{
    int nPixelFormat=-1;
	const char * text=0;

    PIXELFORMATDESCRIPTOR pfd ;

    if(verbose)
    {
	fprintf(stdout, "GL4Java SetDCPixelFormat: input capabilities:\n");
	printGLCapabilities ( glCaps );
    }

    if(glCaps->nativeVisualID>=0)
    {
            if ( 0 < DescribePixelFormat( hDC, (int)(glCaps->nativeVisualID), 
	                                  sizeof(pfd), &pfd ) )
	    {
	        nPixelFormat=(int)(glCaps->nativeVisualID);
	        if(verbose)
		{
		  fprintf(stderr,"\n\nUSER found stored PIXELFORMAT number: %ld\n",
		  	nPixelFormat);
		  fflush(stderr);
		}	
	    } else {
		  fprintf(stderr,"\n\nUSER no stored PIXELFORMAT number found !!\n");
           nPixelFormat = -1;
		  fflush(stderr);
	    }
	}

    if(nPixelFormat<0)
        setPixelFormatByGLCapabilities( &pfd, glCaps, offScreenRenderer, hDC);

    if(verbose)
    {
		fprintf(stderr,"\n\nUSER CHOOSED PIXELFORMAT (TRYING):\n");
		text=GetTextualPixelFormatByPFD(&pfd, nPixelFormat);			
		fprintf(stderr,text);
		fflush(stderr);
    }

    // Choose a pixel format that best matches that described in pfd
    if( hDC == 0 )
	    printf( "SetDCPixelFormat: Error, no HDC-Contex is given\n");
    else if(nPixelFormat<0)
	    nPixelFormat = ChoosePixelFormat(hDC, &pfd);

    if( nPixelFormat == 0 )
	    printf( "SetDCPixelFormat: Error with PixelFormat\n" );

    // Set the pixel format for the device context
    if( SetPixelFormat(hDC, nPixelFormat, &pfd) == FALSE)
	    printf( "setpixel failed\n" );
    else {
            (void) setGLCapabilities ( hDC, nPixelFormat, glCaps );
	    if(verbose)
	    {
	        fprintf(stdout, "GL4Java SetDCPixelFormat: used capabilities:\n");
	        printGLCapabilities ( glCaps );
	    }
    }
	fflush(stdout);
	fflush(stderr);
}


// If necessary, creates a 3-3-2 palette for the device context listed.
HPALETTE LIBAPIENTRY GetOpenGLPalette(HDC hDC)
{
    HPALETTE hRetPal = NULL;	// Handle to palette to be created
    PIXELFORMATDESCRIPTOR pfd;	// Pixel Format Descriptor
    LOGPALETTE *pPal=0;			// Pointer to memory for logical palette
    int nPixelFormat=0;			// Pixel format index
    int nColors=0;				// Number of entries in palette
    int i=0;						// Counting variable
    BYTE RedRange=0,GreenRange=0,BlueRange=0;
							    // Range for each color entry (7,7,and 3)


    // Get the pixel format index and retrieve the pixel format description
    nPixelFormat = GetPixelFormat(hDC);
    DescribePixelFormat(hDC, nPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    // Does this pixel format require a palette?  If not, do not create a
    // palette and just return NULL
    if(!(pfd.dwFlags & PFD_NEED_PALETTE))
	    return NULL;

    // Number of entries in palette.  8 bits yeilds 256 entries
    nColors = 1 << pfd.cColorBits;	

    // Allocate space for a logical palette structure plus all the palette entries
    pPal = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + nColors*sizeof(PALETTEENTRY));

    // Fill in palette header
    pPal->palVersion = 0x300;		// Windows 3.0
    pPal->palNumEntries = nColors; // table size

    // Build mask of all 1's.  This creates a number represented by having
    // the low order x bits set, where x = pfd.cRedBits, pfd.cGreenBits, and
    // pfd.cBlueBits.  
    RedRange = (1 << pfd.cRedBits) -1;
    GreenRange = (1 << pfd.cGreenBits) - 1;
    BlueRange = (1 << pfd.cBlueBits) -1;

    // Loop through all the palette entries
    for(i = 0; i < nColors; i++)
	    {
	    // Fill in the 8-bit equivalents for each component
	    pPal->palPalEntry[i].peRed = (i >> pfd.cRedShift) & RedRange;
	    pPal->palPalEntry[i].peRed = (unsigned char)(
		    (double) pPal->palPalEntry[i].peRed * 255.0 / RedRange);

	    pPal->palPalEntry[i].peGreen = (i >> pfd.cGreenShift) & GreenRange;
	    pPal->palPalEntry[i].peGreen = (unsigned char)(
		    (double)pPal->palPalEntry[i].peGreen * 255.0 / GreenRange);

	    pPal->palPalEntry[i].peBlue = (i >> pfd.cBlueShift) & BlueRange;
	    pPal->palPalEntry[i].peBlue = (unsigned char)(
		    (double)pPal->palPalEntry[i].peBlue * 255.0 / BlueRange);

	    pPal->palPalEntry[i].peFlags = (unsigned char) NULL;
	    }
	    

    // Create the palette
    hRetPal = CreatePalette(pPal);

    // Go ahead and select and realize the palette for this device context
    SelectPalette(hDC,hRetPal,FALSE);
    RealizePalette(hDC);

    // Free the memory used for the logical palette structure
    free(pPal);

    // Return the handle to the new palette
    return hRetPal;
}


static void
PrintMessage( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

	fprintf(stderr, Buffer);
}


int LIBAPIENTRY
PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd )
{
    int PfdIndex;

    if ( 0 < (PfdIndex = GetPixelFormat( Dc )) )
    {
        if ( 0 < DescribePixelFormat( Dc, PfdIndex, sizeof(*Pfd), Pfd ) )
        {
            return(PfdIndex);
        }
        else
        {
            PrintMessage("Could not get a description of pixel format %d\n",
                PfdIndex );
        }
    }
    else
    {
        PrintMessage("Could not get pixel format for Dc 0x%08lX\n", Dc );
    }
    return( 0 );
}


const char * LIBAPIENTRY
GetTextualPixelFormatByPFD(PIXELFORMATDESCRIPTOR *ppfd, int format)
{
	static char buffer[2000];
	char line[200];

    sprintf(buffer,"Pixel format %d\n", format);
    sprintf(line,"  dwFlags - 0x%x\n", ppfd->dwFlags); 
	if (ppfd->dwFlags & PFD_DOUBLEBUFFER) { strcat(buffer, line); sprintf(line,"\tPFD_DOUBLEBUFFER\n "); }
	if (ppfd->dwFlags & PFD_STEREO) { strcat(buffer, line); sprintf(line,"\tPFD_STEREO\n "); }
	if (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) { strcat(buffer, line); sprintf(line,"\tPFD_DRAW_TO_WINDOW\n "); }
    if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) { strcat(buffer, line); sprintf(line,"\tPFD_DRAW_TO_BITMAP\n "); }
    if (ppfd->dwFlags & PFD_SUPPORT_GDI) { strcat(buffer, line); sprintf(line,"\tPFD_SUPPORT_GDI\n "); }
    if (ppfd->dwFlags & PFD_SUPPORT_OPENGL) { strcat(buffer, line); sprintf(line,"\tPFD_SUPPORT_OPENGL\n "); }
    if (ppfd->dwFlags & PFD_GENERIC_ACCELERATED) { strcat(buffer, line); sprintf(line,"\tPFD_GENERIC_ACCELERATED\n "); }
    if (ppfd->dwFlags & PFD_GENERIC_FORMAT) { strcat(buffer, line); sprintf(line,"\tPFD_GENERIC_FORMAT\n "); }
    if (ppfd->dwFlags & PFD_NEED_PALETTE) { strcat(buffer, line); sprintf(line,"\tPFD_NEED_PALETTE\n "); }
    if (ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE) { strcat(buffer, line); sprintf(line,"\tPFD_NEED_SYSTEM_PALETTE\n "); }
    strcat(buffer, line); sprintf(line,"\n");
    strcat(buffer, line); sprintf(line,"  iPixelType - %d\n", ppfd->iPixelType); 
    if (ppfd->iPixelType == PFD_TYPE_RGBA) { strcat(buffer, line); sprintf(line,"\tPGD_TYPE_RGBA\n"); }
    if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) { strcat(buffer, line); sprintf(line,"\tPGD_TYPE_COLORINDEX\n"); }
    strcat(buffer, line); sprintf(line,"  cColorBits - %d\n", ppfd->cColorBits);
    strcat(buffer, line); sprintf(line,"  cRedBits - %d\n", ppfd->cRedBits);
    strcat(buffer, line); sprintf(line,"  cRedShift - %d\n", ppfd->cRedShift);
    strcat(buffer, line); sprintf(line,"  cGreenBits - %d\n", ppfd->cGreenBits);
    strcat(buffer, line); sprintf(line,"  cGreenShift - %d\n", ppfd->cGreenShift);
    strcat(buffer, line); sprintf(line,"  cBlueBits - %d\n", ppfd->cBlueBits);
    strcat(buffer, line); sprintf(line,"  cBlueShift - %d\n", ppfd->cBlueShift);
    strcat(buffer, line); sprintf(line,"  cAlphaBits - %d (N.A.)\n", ppfd->cAlphaBits);
    strcat(buffer, line); sprintf(line,"  cAlphaShift - 0x%x (N.A.)\n", ppfd->cAlphaShift);
    strcat(buffer, line); sprintf(line,"  cAccumBits - %d\n", ppfd->cAccumBits);
    strcat(buffer, line); sprintf(line,"  cAccumRedBits - %d\n", ppfd->cAccumRedBits);
    strcat(buffer, line); sprintf(line,"  cAccumGreenBits - %d\n", ppfd->cAccumGreenBits);
    strcat(buffer, line); sprintf(line,"  cAccumBlueBits - %d\n", ppfd->cAccumBlueBits);
    strcat(buffer, line); sprintf(line,"  cAccumAlphaBits - %d\n", ppfd->cAccumAlphaBits);
    strcat(buffer, line); sprintf(line,"  cDepthBits - %d\n", ppfd->cDepthBits);
    strcat(buffer, line); sprintf(line,"  cStencilBits - %d\n", ppfd->cStencilBits);
    strcat(buffer, line); sprintf(line,"  cAuxBuffers - %d\n", ppfd->cAuxBuffers);
    strcat(buffer, line); sprintf(line,"  iLayerType - %d\n", ppfd->iLayerType);
    strcat(buffer, line); sprintf(line,"  bReserved - %d\n", ppfd->bReserved);
    strcat(buffer, line); sprintf(line,"  dwLayerMask - 0x%x\n", ppfd->dwLayerMask);
    strcat(buffer, line); sprintf(line,"  dwVisibleMask - 0x%x\n", ppfd->dwVisibleMask);
    strcat(buffer, line); sprintf(line,"  dwDamageMask - 0x%x\n", ppfd->dwDamageMask);
	strcat(buffer, line); 
	return buffer;
}

const char * LIBAPIENTRY GetTextualPixelFormatByHDC(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd=0;
    int format=0;

    ppfd   = &pfd;
    format = PixelFormatDescriptorFromDc( hdc, ppfd );

	return GetTextualPixelFormatByPFD(ppfd, format);
}

/*****************************************************************/

/* Struct used to manage color ramps */
typedef struct {
    GLfloat amb[3];	/* ambient color / bottom of ramp */
    GLfloat diff[3];	/* diffuse color / middle of ramp */
    GLfloat spec[3];	/* specular color / top of ramp */
    GLfloat ratio;	/* ratio of diffuse to specular in ramp */
    GLint indexes[3];	/* where ramp was placed in palette */
} colorIndexState ;

#define NUM_COLORS (sizeof(colors) / sizeof(colors[0]))

void LIBAPIENTRY
setupDIB(HDC hDCOrig, HDC hDC, HBITMAP * hBitmap, int width, int height)
{
    BITMAPINFO *bmInfo=0;
    BITMAPINFOHEADER *bmHeader=0;
    UINT usage=0;
    VOID *base=0;
    int bmiSize=0;
    int bitsPerPixel=0;
	HBITMAP hOldBitmap=0;

    bmiSize = sizeof(*bmInfo);
    bitsPerPixel = GetDeviceCaps(hDC, BITSPIXEL);

    switch (bitsPerPixel) {
    case 8:
	// bmiColors is 256 WORD palette indices 
	bmiSize += (256 * sizeof(WORD)) - sizeof(RGBQUAD);
	break;
    case 16:
	// bmiColors is 3 WORD component masks 
	bmiSize += (3 * sizeof(DWORD)) - sizeof(RGBQUAD);
	break;
    case 24:
    case 32:
    default:
	// bmiColors not used 
	break;
    }

    bmInfo = (BITMAPINFO *) calloc(1, bmiSize);
    bmHeader = &bmInfo->bmiHeader;

    bmHeader->biSize = sizeof(*bmHeader);
    bmHeader->biWidth = width;
    bmHeader->biHeight = height;
    bmHeader->biPlanes = 1;			// must be 1 
    bmHeader->biBitCount = bitsPerPixel;
    bmHeader->biXPelsPerMeter = 0;
    bmHeader->biYPelsPerMeter = 0;
    bmHeader->biClrUsed = 0;			// all are used 
    bmHeader->biClrImportant = 0;		// all are important 

    switch (bitsPerPixel) {
    case 8:
	bmHeader->biCompression = BI_RGB;
	bmHeader->biSizeImage = 0;
	usage = DIB_PAL_COLORS;
	// bmiColors is 256 WORD palette indices 
	{
	    WORD *palIndex = (WORD *) &bmInfo->bmiColors[0];
	    int i;

	    for (i=0; i<256; i++) {
		palIndex[i] = i;
	    }
	}
	break;
    case 16:
	bmHeader->biCompression = BI_RGB;
	bmHeader->biSizeImage = 0;
	usage = DIB_RGB_COLORS;
	// bmiColors is 3 WORD component masks 
	{
	    DWORD *compMask = (DWORD *) &bmInfo->bmiColors[0];

	    compMask[0] = 0xF800;
	    compMask[1] = 0x07E0;
	    compMask[2] = 0x001F;
	}
	break;
    case 24:
    case 32:
    default:
	bmHeader->biCompression = BI_RGB;
	bmHeader->biSizeImage = 0;
	usage = DIB_RGB_COLORS;
	// bmiColors not used 
	break;
    }

    *hBitmap = CreateDIBSection(hDC, bmInfo, usage, &base, NULL, 0);
    if (*hBitmap == NULL) {
	(void) MessageBox(WindowFromDC(hDC),
		"Failed to create DIBSection.",
		"OpenGL application error",
		MB_ICONERROR | MB_OK);
	exit(1);
    }

    hOldBitmap = SelectObject(hDC, *hBitmap);
	if(hOldBitmap!=0)
		DeleteObject(hOldBitmap);

    free(bmInfo);
}


/*
static void
setupDIB(HDC hDCOrig, HDC hDC, HBITMAP * hBitmap, int width, int height)
{
	HBITMAP hOldBitmap=0;

	*hBitmap = CreateCompatibleBitmap(  hDCOrig, width, height );
    if (*hBitmap == NULL) {
        fprintf(stderr,"Failed to create CreateCompatibleBitmap! \n");
		fflush(stderr);
		return;
    }

    hOldBitmap = SelectObject(hDC, *hBitmap);
	if(hOldBitmap!=0)
		DeleteObject(hOldBitmap);
}
*/


void LIBAPIENTRY resizeDIB(HDC hDC, HBITMAP *hOldBitmap, HBITMAP *hBitmap)
{
	/*
    SelectObject(hDC, *hOldBitmap);
    DeleteObject(*hBitmap);
    setupDIB(hDC, hBitmap);
	*/
}

HPALETTE LIBAPIENTRY setupPalette(HDC hDC)
{
	HPALETTE hPalette = NULL;
    PIXELFORMATDESCRIPTOR pfd;
    LOGPALETTE* pPal=0;
    int pixelFormat = GetPixelFormat(hDC);
    int paletteSize=0;
	colorIndexState colors[] = {
		{
			{ 0.0F, 0.0F, 0.0F },
			{ 0.1F, 0.6F, 0.3F },
			{ 1.0F, 1.0F, 1.0F },
			0.75F, { 0, 0, 0 },
	    },
		{
			{ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 0.2F, 0.5F },
			{ 1.0F, 1.0F, 1.0F },
			0.75F, { 0, 0, 0 },
		},
		{
			{ 0.0F, 0.05F, 0.05F },
			{ 0.6F, 0.0F, 0.8F },
			{ 1.0F, 1.0F, 1.0F },
			0.75F, { 0, 0, 0 },
		},
	};


    DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    /*
    ** Determine if a palette is needed and if so what size.
    */
    if (pfd.dwFlags & PFD_NEED_PALETTE) {
		paletteSize = 1 << pfd.cColorBits;
    } else if (pfd.iPixelType == PFD_TYPE_COLORINDEX) {
		paletteSize = 4096;
    } else {
		return NULL;
    }

    pPal = (LOGPALETTE*)
	malloc(sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY));
    pPal->palVersion = 0x300;
    pPal->palNumEntries = paletteSize;

    if (pfd.iPixelType == PFD_TYPE_RGBA) {
	/*
	** Fill the logical paletee with RGB color ramps
	*/
	int redMask = (1 << pfd.cRedBits) - 1;
	int greenMask = (1 << pfd.cGreenBits) - 1;
	int blueMask = (1 << pfd.cBlueBits) - 1;
	int i;



	for (i=0; i<paletteSize; ++i) {
	    pPal->palPalEntry[i].peRed =
		    (((i >> pfd.cRedShift) & redMask) * 255) / redMask;
	    pPal->palPalEntry[i].peGreen =
		    (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
	    pPal->palPalEntry[i].peBlue =
		    (((i >> pfd.cBlueShift) & blueMask) * 255) / blueMask;
	    pPal->palPalEntry[i].peFlags = 0;
	}
    } else {
	/*
	** Fill the logical palette with color ramps.
	**
	** Set up the logical palette so that it can be realized
	** into the system palette as an identity palette.
	**
	** 1) The default static entries should be present and at the right
	**    location.  The easiest way to do this is to grab them from
	**    the current system palette.
	**
	** 2) All non-static entries should be initialized to unique values.
	**    The easiest way to do this is to ensure that all of the non-static
	**    entries have the PC_NOCOLLAPSE flag bit set.
	*/
	int numRamps = NUM_COLORS;
	int rampSize = (paletteSize - 20) / numRamps;
	int extra = (paletteSize - 20) - (numRamps * rampSize);
	int i, r;

	/*
	** Initialize static entries by copying them from the
	** current system palette.
	*/
	GetSystemPaletteEntries(hDC, 0, paletteSize, &pPal->palPalEntry[0]);

	/*
	** Fill in non-static entries with desired colors.
	*/
	for (r=0; r<numRamps; ++r) {
	    int rampBase = r * rampSize + 10;
	    PALETTEENTRY *pe = &pPal->palPalEntry[rampBase];
	    int diffSize = (int) (rampSize * colors[r].ratio);
	    int specSize = rampSize - diffSize;

	    for (i=0; i<rampSize; ++i) {
		GLfloat *c0, *c1;
		GLint a;

		if (i < diffSize) {
		    c0 = colors[r].amb;
		    c1 = colors[r].diff;
		    a = (i * 255) / (diffSize - 1);
		} else {
		    c0 = colors[r].diff;
		    c1 = colors[r].spec;
		    a = ((i - diffSize) * 255) / (specSize - 1);
		}

		pe[i].peRed = (BYTE) (a * (c1[0] - c0[0]) + 255 * c0[0]);
		pe[i].peGreen = (BYTE) (a * (c1[1] - c0[1]) + 255 * c0[1]);
		pe[i].peBlue = (BYTE) (a * (c1[2] - c0[2]) + 255 * c0[2]);
		pe[i].peFlags = PC_NOCOLLAPSE;
	    }

	    colors[r].indexes[0] = rampBase;
	    colors[r].indexes[1] = rampBase + (diffSize-1);
	    colors[r].indexes[2] = rampBase + (rampSize-1);
	}

	/*
	** Initialize any remaining non-static entries.
	*/
	for (i=0; i<extra; ++i) {
	    int index = numRamps*rampSize+10+i;
	    PALETTEENTRY *pe = &pPal->palPalEntry[index];

	    pe->peRed = (BYTE) 0;
	    pe->peGreen = (BYTE) 0;
	    pe->peBlue = (BYTE) 0;
	    pe->peFlags = PC_NOCOLLAPSE;
	}
    }

    hPalette = CreatePalette(pPal);
    free(pPal);

    if (hPalette) {
	SelectPalette(hDC, hPalette, FALSE);
	RealizePalette(hDC);
    }

	return hPalette;
}


int LIBAPIENTRY setGLCapabilities ( HDC hdc, 
					 int nPixelFormat,
 					 GLCapabilities *glCaps )
{
    PIXELFORMATDESCRIPTOR pfd;

    (void) PixelFormatDescriptorFromDc( hdc, &pfd);

    if (pfd.dwFlags & PFD_DOUBLEBUFFER)
	glCaps->buffer=BUFFER_DOUBLE;
    else
	glCaps->buffer=BUFFER_SINGLE;

    if (pfd.dwFlags & PFD_STEREO)
	glCaps->stereo=STEREO_ON;
    else
	glCaps->stereo=STEREO_OFF;

    if (pfd.iPixelType == PFD_TYPE_RGBA)
	glCaps->color=COLOR_RGBA;

    if (pfd.iPixelType == PFD_TYPE_COLORINDEX)
	glCaps->color=COLOR_INDEX;

    glCaps->depthBits = pfd.cDepthBits;
    glCaps->stencilBits = pfd.cStencilBits;

    glCaps->redBits = pfd.cRedBits;
    glCaps->greenBits= pfd.cGreenBits;
    glCaps->blueBits=  pfd.cBlueBits;
    /* glCaps->alphaBits= pfd.cAlphaBits; N.A. */
    glCaps->accumRedBits = pfd.cAccumRedBits;
    glCaps->accumGreenBits= pfd.cAccumGreenBits;
    glCaps->accumBlueBits= pfd.cAccumBlueBits;
    glCaps->accumAlphaBits= pfd.cAccumAlphaBits;

    glCaps->nativeVisualID=nPixelFormat;

    return 1;
}

