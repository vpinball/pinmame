/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Display.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include <math.h>
#include "osdepend.h"
#include "Display.h"
#include "M32Util.h"
#include "driver.h"
#include "uclock.h"
#include "file.h"
#include "png.h"
#include "artwork.h"

#define MAKECOL(r, g, b) ((((r & 0x000000F8) << 7)) | \
                          (((g & 0x000000F8) << 2)) | \
                          (((b & 0x000000F8) >> 3)))

#define GETR(col)  (col >> 7) & 0x000000F8;
#define GETG(col)  (col >> 2) & 0x000000F8;
#define GETB(col)  (col << 3) & 0x000000F8;

#define FRAMESKIP_LEVELS 12
#define INTENSITY_LEVELS 256

extern HINSTANCE	g_hInst;
extern HWND			g_hMainWnd;
extern BOOL			g_fActivateWindow;

/***************************************************************************
    function prototypes
 ***************************************************************************/

static void				  OnPaint(HWND hWnd);
static void               OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange);
static void               OnQueryNewPalette(HWND hWnd);

static int                Display_init(PCONTROLLEROPTIONS pControllerOptions);
static void               Display_exit(void);
static struct mame_bitmap* Display_alloc_bitmap(int width,int height,int depth);
static void               Display_free_bitmap(struct mame_bitmap* bitmap);
static int                Display_create_display(int width, int height, int depth, int fps, int attributes, int orientation);
static void               Display_close_display(void);
static void               Display_set_visible_area(int min_x, int max_x, int min_y, int max_y);
#if MAMEVER < 3709
static int                Display_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT16 *pens, int modifiable, const UINT8 *debug_palette, UINT16 *debug_pens);
#else
static int                Display_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
#endif
static void               Display_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void               Display_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
static void               Display_mark_dirty(int x1, int y1, int x2, int y2);
static int                Display_skip_this_frame(void);
static void               Display_update_display(struct mame_bitmap *game_bitmap, struct mame_bitmap *debug_bitmap);
static BOOL               Display_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void               Display_set_gamma(float gamma);
static void               Display_set_brightness(int brightness);
static void               Display_Refresh(void);
static int                Display_GetBlackPen(void);

static void               Throttle(void);
static void               AdjustColorMap(void);
static void               SetPaletteColors(void);
static void               AdjustVisibleRect(int xmin, int ymin, int xmax, int ymax);
static void			      AdjustPalette(void);
static void               SetPen(int pen, unsigned char red, unsigned char green, unsigned char blue);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDDisplay Display = 
{
    { Display_init },               /* init              */
    { Display_exit },               /* exit              */
    { Display_alloc_bitmap },       /* alloc_bitmap      */
    { Display_free_bitmap },        /* free_bitmap       */
    { Display_create_display },     /* create_display    */
    { Display_close_display },      /* close_display     */
    { Display_set_visible_area },   /* set_visible_area  */
    { 0 },							/* set_debugger_focus*/
    { Display_allocate_colors },    /* allocate_colors   */
    { Display_modify_pen },         /* modify_pen        */
    { Display_get_pen },            /* get_pen           */
    { Display_mark_dirty },         /* mark_dirty        */
    { Display_skip_this_frame },    /* skip_this_frame   */
    { Display_update_display },     /* update_display    */
    { 0 },                          /* led_w             */
    { Display_set_gamma },          /* set_gamma         */
    { Display_get_gamma },          /* get_gamma         */
    { Display_set_brightness },     /* set_brightness    */
    { Display_get_brightness },     /* get_brightness    */
    { 0 },                          /* save_snapshot     */

    { Display_OnMessage },          /* OnMessage         */
    { Display_Refresh },            /* Refresh           */
    { Display_GetBlackPen },        /* GetBlackPen       */
    { 0 },                          /* UpdateFPS         */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tDisplay_private
{
    /* Gamma/Brightness */
    unsigned char   m_pColorMap[INTENSITY_LEVELS];
    double          m_dGamma;
    int             m_nBrightness;

    /* Speed throttling / frame skipping */
    BOOL            m_bThrottled;
    BOOL            m_bAutoFrameskip;
    int             m_nFrameSkip;
    int             m_nFrameskipCounter;
    int             m_nFrameskipAdjust;
    int             m_nSpeed;
    int             m_bInitTime;
    uclock_t        m_PrevMeasure;
    uclock_t        m_ThisFrameBase;
    uclock_t        m_Prev;

    /* vector game stats */
    int             m_nVecUPS;
    int             m_nVecFrameCount;

    /* average frame rate data */
    int             frames_displayed;
    uclock_t        start_time;
    uclock_t        end_time;  /* to calculate fps average on exit */

    struct mame_bitmap*  m_pBitmap;
    struct mame_bitmap*  m_pTempBitmap;
    BITMAPINFO*         m_pInfo;

    tRect               m_VisibleRect;
    int                 m_nClientWidth;
    int                 m_nClientHeight;
    RECT                m_ClientRect;
    int                 m_nWindowWidth;
    int                 m_nWindowHeight;

    BOOL                m_bDouble;
    BOOL                m_bVectorDouble;
    int                 m_nDepth;

    BOOL                m_bModifiablePalette;
    BOOL                m_bUpdatePalette;
    HPALETTE            m_hPalette;
    PALETTEENTRY*       m_PalEntries;
    PALETTEENTRY*       m_AdjustedPalette;
    UINT                m_nUIPen;
    UINT32*             m_p16BitLookup;
    UINT32              m_nTotalColors;
};


#define FRAMES_TO_SKIP 20   /* skip the first few frames from the FPS calculation */
                            /* to avoid counting the copyright and info screens */

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDisplay_private      This;
static int                          snapno = 0;
static const int                    safety = 16;

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT     ps;
    HPALETTE		hOldPalette = 0;

    BeginPaint(hWnd, &ps);

    if (This.m_pBitmap == NULL) {
        EndPaint(hWnd, &ps); 
        return;
    }

	hOldPalette = SelectPalette(ps.hdc, This.m_hPalette, FALSE);
    RealizePalette(ps.hdc);

	StretchDIBits(ps.hdc,
				  g_pControllerOptions->nBorderSizeX*(g_pControllerOptions->fDoubleSize?2:1), 				  
				  g_pControllerOptions->nBorderSizeY*(g_pControllerOptions->fDoubleSize?2:1),
				  This.m_nClientWidth,
				  This.m_nClientHeight,

				  This.m_VisibleRect.m_Left,
				  This.m_pBitmap->height - This.m_VisibleRect.m_Height - This.m_VisibleRect.m_Top,
				  This.m_VisibleRect.m_Width, 
				  This.m_VisibleRect.m_Height,

				  This.m_pBitmap->line[0],
				  This.m_pInfo,
				  DIB_PAL_COLORS,
				  SRCCOPY);

    if (hOldPalette != NULL)
        SelectPalette(ps.hdc, hOldPalette, FALSE);

    EndPaint(hWnd, &ps); 
}

static void OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange)
{
    if (hWnd == hWndPaletteChange) 
        return; 
    
    OnQueryNewPalette(hWnd);
}

static void OnQueryNewPalette(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, TRUE); 
    UpdateWindow(hWnd); 
}


/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static BOOL Display_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg) {
	case WM_PAINT:
		OnPaint(hWnd);
		return FALSE;
	
	case WM_PALETTECHANGED:
		OnPaletteChanged(hWnd, (HWND) wParam);
		return TRUE;

	case WM_QUERYNEWPALETTE:
		OnQueryNewPalette(hWnd);
		return TRUE;

    }
    return FALSE;
}

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int Display_init(PCONTROLLEROPTIONS pControllerOptions)
{
    /* Reset the Snapshot number */
    snapno = 0;

    This.m_dGamma            = 1.0; // max(0, options->gamma);
    This.m_nBrightness       = 100; // max(0, options->brightness);
    This.m_bThrottled        = TRUE;
    This.m_bAutoFrameskip    = 1; // options->auto_frame_skip;
    This.m_nFrameSkip        = 1; // options->frame_skip;
    This.m_nFrameskipCounter = 0;
    This.m_nFrameskipAdjust  = 0;
    This.m_nSpeed            = 100;
    This.m_nVecUPS           = 0;
    This.m_nVecFrameCount    = 0;
    This.m_bInitTime         = TRUE;
    This.m_PrevMeasure       = uclock();
    This.m_ThisFrameBase     = uclock();
    This.m_Prev              = uclock();
    This.frames_displayed    = 0;
    This.start_time          = uclock();
    This.end_time            = uclock();

    if (This.m_bAutoFrameskip == TRUE)
        This.m_nFrameSkip = 0;

    /* Initialize map. */
    AdjustColorMap();

    This.m_pBitmap          = NULL;
    This.m_pTempBitmap      = NULL;
    This.m_pInfo            = NULL;

    memset(&This.m_VisibleRect, 0, sizeof(tRect));

    This.m_nDepth           = Machine->color_depth;

    This.m_bModifiablePalette	= FALSE;
    This.m_bUpdatePalette		= FALSE;
    This.m_hPalette				= NULL;
    This.m_PalEntries			= NULL;
    This.m_AdjustedPalette		= NULL;
    This.m_nUIPen				= 0;
    This.m_p16BitLookup			= NULL;
    This.m_nTotalColors			= 0;
	This.m_bDouble			    = pControllerOptions->fDoubleSize;

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void Display_exit(void)
{
   if (This.frames_displayed > FRAMES_TO_SKIP)
      printf("Average FPS: %f\n",
             (double)UCLOCKS_PER_SEC / (This.end_time - This.start_time) *
             (This.frames_displayed - FRAMES_TO_SKIP));
}

/*
    Create a bitmap.
*/
/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */

static struct mame_bitmap* Display_alloc_bitmap(int width, int height, int depth)
{
#if MAMEVER >= 5900
  return bitmap_alloc_depth(width,height,depth);
#else /* MAMEVER */
    struct mame_bitmap*  pBitmap;

    pBitmap = (struct mame_bitmap*)malloc(sizeof(struct mame_bitmap));

    if (pBitmap != NULL)
    {
        unsigned char*  bm;
        int     i;
        int     rdwidth;
        int     rowlen;

        pBitmap->width  = width;
        pBitmap->height = height;
        pBitmap->depth  = depth;

        rdwidth = (width + 7) & ~7;     /* round width to a quadword */

        if (depth == 16)
            rowlen = 2 * (rdwidth + 2 * safety) * sizeof(unsigned char);
        else
            rowlen =     (rdwidth + 2 * safety) * sizeof(unsigned char);

        bm = (unsigned char*)malloc((height + 2 * safety) * rowlen);
        if (bm == NULL)
        {
            free(pBitmap);
            return NULL;
        }

        /* clear ALL bitmap, including safety area, to avoid garbage on right */
        /* side of screen is width is not a multiple of 4 */
        memset(bm, 0, (height + 2 * safety) * rowlen);

        if ((pBitmap->line = malloc((height + 2 * safety) * sizeof(unsigned char *))) == 0)
        {
            free(bm);
            free(pBitmap);
            return 0;
        }

        for (i = 0; i < height + 2 * safety; i++)
        {
			if (depth == 16)
				pBitmap->line[i] = &bm[i * rowlen + safety * 2];
			else
				pBitmap->line[i] = &bm[i * rowlen + safety];
        }

        pBitmap->line += safety;

        pBitmap->_private = bm;
    }

    return pBitmap;
#endif /* MAMEVER */
}

/*
    Free memory allocated in create_bitmap.
*/
static void Display_free_bitmap(struct mame_bitmap* pBitmap)
{
#if MAMEVER >= 5900
  bitmap_free(pBitmap);
#else /* MAMEVER */
    if (pBitmap != NULL)
    {
        pBitmap->line -= safety;
        free(pBitmap->line);
        free(pBitmap->_private);
        free(pBitmap);
    }
#endif /* MAMEVER */
}

/*
	Sets the size of the client area of the real window
*/
int AdjustClientRect(int fActivateWindow)
{
    RECT Rect;

    This.m_nClientWidth  = This.m_VisibleRect.m_Width;
    This.m_nClientHeight = This.m_VisibleRect.m_Height;

    if (g_pControllerOptions->fDoubleSize)
    {
        This.m_nClientWidth  *= 2;
        This.m_nClientHeight *= 2;
    }

    This.m_ClientRect.left   = 0;
    This.m_ClientRect.top    = 0;
    This.m_ClientRect.right  = This.m_nClientWidth  + 2*g_pControllerOptions->nBorderSizeX*(g_pControllerOptions->fDoubleSize?2:1);
    This.m_ClientRect.bottom = This.m_nClientHeight + 2*g_pControllerOptions->nBorderSizeY*(g_pControllerOptions->fDoubleSize?2:1);

    /* Calculate size of window based on desired client area. */
    Rect.left   = This.m_ClientRect.left;
    Rect.top    = This.m_ClientRect.top;
    Rect.right  = This.m_ClientRect.right;
    Rect.bottom = This.m_ClientRect.bottom;
    AdjustWindowRect(&Rect, GetWindowLong(g_hMainWnd, GWL_STYLE), FALSE);

    This.m_nWindowWidth  = RECT_WIDTH(Rect);
    This.m_nWindowHeight = RECT_HEIGHT(Rect);

	/* Show the window and either activate it if flag is set, or leave it as an inactive window */
	if ( fActivateWindow ) {
		SetWindowPos(g_hMainWnd,
					 HWND_TOPMOST,
					 0, 0,
					 This.m_nWindowWidth,
					 This.m_nWindowHeight,
					 SWP_NOMOVE);

		ShowWindow(g_hMainWnd, SW_SHOW);
		SetForegroundWindow(g_hMainWnd);
	}
	else {	
		SetWindowPos(g_hMainWnd,
					 HWND_TOPMOST,
					 0, 0,
					 This.m_nWindowWidth,
					 This.m_nWindowHeight,
					 SWP_NOMOVE | SWP_NOACTIVATE);

		ShowWindow(g_hMainWnd, SW_SHOWNA);
	}

    return 0;
}


/*
    Create a display screen, or window, of the given dimensions (or larger).
    Attributes are the ones defined in driver.h.
    Returns 0 on success.
*/
static int Display_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    unsigned int    i;
    LOGPALETTE      LogPalette;
    char            TitleBuf[256];
    int             bmwidth;
    int             bmheight;

    This.m_nDepth = depth;

    if (attributes & VIDEO_TYPE_VECTOR)
    {
        if (This.m_bVectorDouble == TRUE)
        {
            width  *= 2;
            height *= 2;
        }
        /* padding to a DWORD value */
        width  -= width  % 4;
        height -= height % 4;
    }

    AdjustVisibleRect(0, 0, width - 1, height - 1);

    This.m_nClientWidth  = This.m_VisibleRect.m_Width;
    This.m_nClientHeight = This.m_VisibleRect.m_Height;

    if ((attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) == VIDEO_PIXEL_ASPECT_RATIO_1_2)
    {
        if (orientation & ORIENTATION_SWAP_XY)
            This.m_nClientWidth  *= 2;
        else
            This.m_nClientHeight *= 2;

        This.m_bDouble = FALSE;
    }

    if ((attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) == VIDEO_PIXEL_ASPECT_RATIO_2_1)
    {
        if (orientation & ORIENTATION_SWAP_XY)
            This.m_nClientHeight *= 2;
        else
            This.m_nClientWidth  *= 2;

        This.m_bDouble = FALSE;
    }

    if (This.m_bDouble == TRUE)
    {
        This.m_nClientWidth  *= 2;
        This.m_nClientHeight *= 2;
    }

    /*
        Crap. The scrbitmap is no longer created before osd_create_display() is called.
        The following code is to figure out how big the actual scrbitmap will be.
    */
    if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    {
        bmwidth  = width;
	    bmheight = height;
    }
    else
    {
        bmwidth  = Machine->drv->screen_width;
	    bmheight = Machine->drv->screen_height;

        if (orientation & ORIENTATION_SWAP_XY)
	    {
		    int temp;
		    temp     = bmwidth;
            bmwidth  = bmheight;
            bmheight = temp;
	    }
    }

    /* Make a copy of the scrbitmap for 16 bit palette lookup. */
#if MAMEVER >= 5900
    This.m_pTempBitmap = bitmap_alloc_depth(bmwidth, bmheight, This.m_nDepth);
#else
    This.m_pTempBitmap = osd_alloc_bitmap(bmwidth, bmheight, This.m_nDepth);
#endif /* MAMEVER */
    /* Palette */
    if (This.m_nDepth == 8)
    {
        LogPalette.palVersion    = 0x0300;
        LogPalette.palNumEntries = 1;
        This.m_hPalette = CreatePalette(&LogPalette);
        ResizePalette(This.m_hPalette, OSD_NUMPENS);
    }

    /* Create BitmapInfo */
    This.m_pInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) +
                                       sizeof(RGBQUAD) * OSD_NUMPENS);

    This.m_pInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER); 
    This.m_pInfo->bmiHeader.biWidth         =  (((UINT8 *)(This.m_pTempBitmap->line[1]) - (UINT8 *)(This.m_pTempBitmap->line[0]))) / (This.m_nDepth / 8);
    This.m_pInfo->bmiHeader.biHeight        = -(int)(This.m_VisibleRect.m_Height); /* Negative means "top down" */
    This.m_pInfo->bmiHeader.biPlanes        = 1;
    This.m_pInfo->bmiHeader.biBitCount      = This.m_nDepth;
    This.m_pInfo->bmiHeader.biCompression   = BI_RGB;
    This.m_pInfo->bmiHeader.biSizeImage     = 0;
    This.m_pInfo->bmiHeader.biXPelsPerMeter = 0;
    This.m_pInfo->bmiHeader.biYPelsPerMeter = 0;
    This.m_pInfo->bmiHeader.biClrUsed       = 0;
    This.m_pInfo->bmiHeader.biClrImportant  = 0;

    if (This.m_nDepth == 8)
    {
        /* Map image values to palette index */
        for (i = 0; i < OSD_NUMPENS; i++)
            ((WORD*)(This.m_pInfo->bmiColors))[i] = i;
    }

    sprintf(TitleBuf, "%s", Machine->gamedrv->description);
    SetWindowText(g_hMainWnd, TitleBuf);

	AdjustClientRect(g_fActivateWindow);

    return 0;
}

/*
     Shut down the display
*/
static void Display_close_display(void)
{
    if (This.m_nDepth == 8)
    {
        if (This.m_hPalette != NULL)
        {
            DeletePalette(This.m_hPalette);
            This.m_hPalette = NULL;
        }
    }
#if MAMEVER >= 5900
    bitmap_free(This.m_pTempBitmap);
#else
    osd_free_bitmap(This.m_pTempBitmap);
#endif /* MAMEVER */
    if (This.m_pInfo != NULL)
    {
        free(This.m_pInfo);
        This.m_pInfo = NULL;
    }

    if (This.m_PalEntries != NULL)
    {
        free(This.m_PalEntries);
        This.m_PalEntries = NULL;
    }

    if (This.m_AdjustedPalette != NULL)
    {
        free(This.m_AdjustedPalette);
        This.m_AdjustedPalette = NULL;
    }

    if (This.m_p16BitLookup != NULL)
    {
        free(This.m_p16BitLookup);
        This.m_p16BitLookup = NULL;
    }
}

/*
    center image inside the display based on the visual area
*/
static void Display_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{

//This screws up Extra Long displays, like Elvira, Harley Davidson.. (SJE)
#if 0
	if ( max_x>255 )
		max_x = 255;

	if ( max_y>255 )
		max_y = 255;
#endif

    AdjustVisibleRect(min_x, min_y, max_x, max_y);

    set_ui_visarea(This.m_VisibleRect.m_Left,
                   This.m_VisibleRect.m_Top,
                   This.m_VisibleRect.m_Left + This.m_VisibleRect.m_Width - 1,
                   This.m_VisibleRect.m_Top  + This.m_VisibleRect.m_Height - 1);

	AdjustClientRect(0);
}
#if MAMEVER >= 3709
static int Display_allocate_colors(unsigned int totalcolors,
                               const UINT8* palette,
                               UINT32*      pens,
                               int          modifiable,
                               const UINT8* debug_palette,
                               UINT32*      debug_pens)
#else
static int Display_allocate_colors(unsigned int totalcolors,
                               const UINT8* palette,
                               UINT16*      pens,
                               int          modifiable,
                               const UINT8* debug_palette,
                               UINT16*      debug_pens)
#endif
{
    unsigned int i;

    This.m_bModifiablePalette = modifiable ? TRUE : FALSE;

    This.m_nTotalColors = totalcolors;
    if (This.m_nDepth != 8)
        This.m_nTotalColors += 2;
    else
        This.m_nTotalColors = OSD_NUMPENS;

    This.m_p16BitLookup    = (UINT32*) malloc(This.m_nTotalColors * sizeof(UINT32));
    This.m_PalEntries      = (PALETTEENTRY*) malloc(This.m_nTotalColors * sizeof(PALETTEENTRY));
    This.m_AdjustedPalette = (PALETTEENTRY*) malloc(This.m_nTotalColors * sizeof(PALETTEENTRY));
    if (This.m_p16BitLookup == NULL || This.m_PalEntries == NULL || This.m_AdjustedPalette == NULL)
        return 1;

    if (This.m_nDepth != 8 && This.m_bModifiablePalette == FALSE)
    {
        /* 16 bit static palette. */
        int r, g, b;

        for (i = 0; i < totalcolors; i++)
        {
            r = 255.0 * Display_get_brightness() * pow(palette[3 * i + 0] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            g = 255.0 * Display_get_brightness() * pow(palette[3 * i + 1] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            b = 255.0 * Display_get_brightness() * pow(palette[3 * i + 2] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            //*pens++ = MAKECOL(r, g, b);
        }

        Machine->uifont->colortable[0] = 0;
        Machine->uifont->colortable[1] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[2] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[3] = 0;
    }
    else
    {
		if (This.m_nDepth == 8 && 255 <= totalcolors)
        {
            int bestblack;
            int bestwhite;
            int bestblackscore;
            int bestwhitescore;

            bestblack = bestwhite = 0;
            bestblackscore = 3 * 255 * 255;
            bestwhitescore = 0;
            for (i = 0; i < totalcolors; i++)
            {
                int r, g, b, score;

                r = palette[3 * i + 0];
                g = palette[3 * i + 1];
                b = palette[3 * i + 2];
                score = r * r + g * g + b * b;

                if (score < bestblackscore)
                {
                    bestblack      = i;
                    bestblackscore = score;
                }
                if (score > bestwhitescore)
                {
                    bestwhite      = i;
                    bestwhitescore = score;
                }
            }

            for (i = 0; i < totalcolors; i++)
                pens[i] = i;

            /* map black to pen 0, otherwise the screen border will not be black */
            pens[bestblack] = 0;
            pens[0] = bestblack;

            Machine->uifont->colortable[0] = pens[bestblack];
            Machine->uifont->colortable[1] = pens[bestwhite];
            Machine->uifont->colortable[2] = pens[bestwhite];
            Machine->uifont->colortable[3] = pens[bestblack];

            This.m_nUIPen = pens[bestwhite];
        }
        else
        {
            /* 8 or 16 bit palette with extra palette space left over. */

            /*
                If we have free places, fill the palette starting from the end,
                so we don't touch color 0, which is better left black.
            */
            for (i = 0; i < totalcolors; i++)
                pens[i] = (This.m_nTotalColors - 1) - i;

            /* As long as there are free pens, set 0 as black for GetBlackPen. */
            SetPen(0, 0, 0, 0);

            /* reserve color 1 for the user interface text */
            This.m_nUIPen = 1;
            SetPen(This.m_nUIPen, 0xFF, 0xFF, 0xFF);

            Machine->uifont->colortable[0] = 0;
            Machine->uifont->colortable[1] = This.m_nUIPen;
            Machine->uifont->colortable[2] = This.m_nUIPen;
            Machine->uifont->colortable[3] = 0;
        }

        for (i = 0; i < totalcolors; i++)
        {
            SetPen(pens[i],
                   palette[3 * i + 0],
                   palette[3 * i + 1],
                   palette[3 * i + 2]);
        }

        SetPaletteColors();
    }

    return 0;
}

/*
    Change the color of the pen.
*/
static void Display_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    assert(This.m_bModifiablePalette == TRUE); /* Shouldn't modify pen if flag is not set! */

    assert(0 <= pen && pen < This.m_nTotalColors);

    if (This.m_PalEntries[pen].peRed    == red
    &&  This.m_PalEntries[pen].peGreen  == green
    &&  This.m_PalEntries[pen].peBlue   == blue)
        return;

    SetPen(pen, red, green, blue);

    This.m_bUpdatePalette = TRUE;
}

/*
    Get the color of a pen.
*/
static void Display_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue)
{
    if (This.m_nDepth != 8 && This.m_bModifiablePalette == FALSE)
    {
        *pRed   = GETR(pen);
        *pGreen = GETG(pen);
        *pBlue  = GETB(pen);
    }
    else
    {
        assert(0 <= pen && pen < This.m_nTotalColors);

        if (This.m_nTotalColors <= pen)
            pen = 0;

        *pRed   = This.m_PalEntries[pen].peRed;
        *pGreen = This.m_PalEntries[pen].peGreen;
        *pBlue  = This.m_PalEntries[pen].peBlue;
    }
}

static void Display_mark_dirty(int x1, int y1, int x2, int y2)
{
}

static Display_skip_this_frame(void)
{
    static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
    {
        { 0,0,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,1 },
        { 0,0,0,0,0,1,0,0,0,0,0,1 },
        { 0,0,0,1,0,0,0,1,0,0,0,1 },
        { 0,0,1,0,0,1,0,0,1,0,0,1 },
        { 0,1,0,0,1,0,1,0,0,1,0,1 },
        { 0,1,0,1,0,1,0,1,0,1,0,1 },
        { 0,1,0,1,1,0,1,0,1,1,0,1 },
        { 0,1,1,0,1,1,0,1,1,0,1,1 },
        { 0,1,1,1,0,1,1,1,0,1,1,1 },
        { 0,1,1,1,1,1,0,1,1,1,1,1 },
        { 0,1,1,1,1,1,1,1,1,1,1,1 }
    };

    assert(0 <= This.m_nFrameSkip && This.m_nFrameSkip < FRAMESKIP_LEVELS);
    assert(0 <= This.m_nFrameskipCounter && This.m_nFrameskipCounter < FRAMESKIP_LEVELS);

    return skiptable[This.m_nFrameSkip][This.m_nFrameskipCounter];
}

static void ClearFPSDisplay(void)
{
//  MAME32App.m_pDisplay->UpdateFPS(FALSE, 0, 0, 0, 0, 0);
    schedule_full_refresh();
}

/*
    Update the display.
*/
static void Display_update_display(struct mame_bitmap *game_bitmap, struct mame_bitmap *debug_bitmap)
{
    if (osd_skip_this_frame() == 0)
    {   
        /* Wait until it's time to update the screen. */
        Throttle();

		This.m_pBitmap = game_bitmap;

		if (This.m_bUpdatePalette == TRUE) {
			SetPaletteColors();
			This.m_bUpdatePalette = FALSE;
		}

		InvalidateRect(g_hMainWnd, NULL, FALSE); // , &This.m_ClientRect, FALSE);
		UpdateWindow(g_hMainWnd);

        if (This.m_bThrottled
        &&  This.m_bAutoFrameskip
        &&  This.m_nFrameskipCounter == 0)
        {
            if (100 <= This.m_nSpeed)
            {
                /* increase frameskip quickly, decrease it slowly */
                This.m_nFrameskipAdjust++;
                if (This.m_nFrameskipAdjust >= 3)
                {
                    This.m_nFrameskipAdjust = 0;
                    if (This.m_nFrameSkip > 0)
                        This.m_nFrameSkip--;
                }
            }
            else
            {
                if (This.m_nSpeed < 80)
                {
                    This.m_nFrameskipAdjust -= (90 - This.m_nSpeed) / 5;
                }
                else
                {
                    /* don't push frameskip too far if we are close to 100% speed */
                    if (This.m_nFrameSkip < 8)
                        This.m_nFrameskipAdjust--;    
                }
                while (This.m_nFrameskipAdjust <= -2)
                {
                    This.m_nFrameskipAdjust += 2;
                    if (This.m_nFrameSkip < FRAMESKIP_LEVELS - 1)
                        This.m_nFrameSkip++;
                }
            }
        }       
    }

    /*
        Adjust frameskip.
    */
    if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
    {
        if (This.m_bAutoFrameskip == TRUE)
        {
            This.m_bAutoFrameskip = FALSE;
            This.m_nFrameSkip     = 0;
        }
        else
        {
            if (This.m_nFrameSkip == FRAMESKIP_LEVELS - 1)
            {
                This.m_nFrameSkip     = 0;
                This.m_bAutoFrameskip = TRUE;
            }
            else
                This.m_nFrameSkip++;
        }

        /* reset the frame counter every time the frameskip key is pressed, so */
        /* we'll measure the average FPS on a consistent status. */
        This.frames_displayed = 0;
    }

    if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
    {
        if (This.m_bAutoFrameskip == TRUE)
        {
            This.m_bAutoFrameskip = FALSE;
            This.m_nFrameSkip     = FRAMESKIP_LEVELS - 1;
        }
        else
        {
            if (This.m_nFrameSkip == 0)
                This.m_bAutoFrameskip = TRUE;
            else
                This.m_nFrameSkip--;
        }

        /* reset the frame counter every time the frameskip key is pressed, so */
        /* we'll measure the average FPS on a consistent status. */
        This.frames_displayed = 0;
    }

    /*
        Toggle throttle.
    */
    if (input_ui_pressed(IPT_UI_THROTTLE))
    {
        This.m_bThrottled = (This.m_bThrottled == TRUE) ? FALSE : TRUE;

        /* reset the frame counter every time the throttle key is pressed, so */
        /* we'll measure the average FPS on a consistent status. */
        This.frames_displayed = 0;
    }

    This.m_nFrameskipCounter = (This.m_nFrameskipCounter + 1) % FRAMESKIP_LEVELS;
}

static void Display_set_gamma(float gamma)
{
    if (This.m_dGamma < 0.2) This.m_dGamma = 0.2;
    if (This.m_dGamma > 3.0) This.m_dGamma = 3.0;

    This.m_dGamma = gamma;
    AdjustColorMap();
}

float Display_get_gamma()
{
    return (float)This.m_dGamma;
}

static void Display_set_brightness(int brightness)
{
    This.m_nBrightness = brightness;
    AdjustColorMap();
}

int Display_get_brightness()
{
    return This.m_nBrightness;
}

void Display_MapColor(unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue)
{
    *pRed   = This.m_pColorMap[(unsigned char)*pRed];
    *pGreen = This.m_pColorMap[(unsigned char)*pGreen];
    *pBlue  = This.m_pColorMap[(unsigned char)*pBlue];
}

BOOL Display_Throttled()
{
    return This.m_bThrottled;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void Throttle(void)
{
    static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
    {
        { 1,1,1,1,1,1,1,1,1,1,1,1 },
        { 2,1,1,1,1,1,1,1,1,1,1,0 },
        { 2,1,1,1,1,0,2,1,1,1,1,0 },
        { 2,1,1,0,2,1,1,0,2,1,1,0 },
        { 2,1,0,2,1,0,2,1,0,2,1,0 },
        { 2,0,2,1,0,2,0,2,1,0,2,0 },
        { 2,0,2,0,2,0,2,0,2,0,2,0 },
        { 2,0,2,0,0,3,0,2,0,0,3,0 },
        { 3,0,0,3,0,0,3,0,0,3,0,0 },
        { 4,0,0,0,4,0,0,0,4,0,0,0 },
        { 6,0,0,0,0,0,6,0,0,0,0,0 },
        {12,0,0,0,0,0,0,0,0,0,0,0 }
    };
    uclock_t curr;
    int already_synced = 0;

    if (This.m_bInitTime)
    {
        /* first time through, initialize timer */
        This.m_PrevMeasure = uclock() - FRAMESKIP_LEVELS * UCLOCKS_PER_SEC / Machine->drv->frames_per_second;
        This.m_bInitTime = FALSE;
    }

    if (This.m_nFrameskipCounter == 0)
        This.m_ThisFrameBase = This.m_PrevMeasure + FRAMESKIP_LEVELS * UCLOCKS_PER_SEC / Machine->drv->frames_per_second;

    if (This.m_bThrottled == TRUE)
    {
        profiler_mark(PROFILER_IDLE);
#if 0
        if (video_sync)
        {
            static uclock_t last;

			
            do
            {
                vsync();
                curr = uclock();
            } while (UCLOCKS_PER_SEC / (curr - last) > Machine->drv->frames_per_second * 11 / 10);

            last = curr;
        }
        else
#endif
        {
            uclock_t target;

            curr = uclock();

            if (already_synced == 0)
            {
            /* wait only if the audio update hasn't synced us already */

                /* wait until enough time has passed since last frame... */
                target = This.m_ThisFrameBase +
                         This.m_nFrameskipCounter * UCLOCKS_PER_SEC / Machine->drv->frames_per_second;

                if (curr - target < 0)
                {
                    do
                    {
						Sleep((target-curr)/10000);
                        curr = uclock();
                    }
                    while (curr - target < 0);
                }
            }
        }
        profiler_mark(PROFILER_END);
    }
    else
    {
        curr = uclock();
    }


    /* for the FPS average calculation */
    if (++This.frames_displayed == FRAMES_TO_SKIP)
        This.start_time = curr;
    else
        This.end_time = curr;

    if (This.m_nFrameskipCounter == 0)
    {
        uclock_t divdr;

        divdr = Machine->drv->frames_per_second * (curr - This.m_PrevMeasure) / (100 * FRAMESKIP_LEVELS);
        This.m_nSpeed = (int)((UCLOCKS_PER_SEC + divdr / 2) / divdr);

        This.m_PrevMeasure = curr;
    }

    This.m_Prev = curr;

    This.m_nVecFrameCount += waittable[This.m_nFrameSkip][This.m_nFrameskipCounter];
    if (This.m_nVecFrameCount >= Machine->drv->frames_per_second)
    {
        extern int vector_updates; /* avgdvg_go()'s per Mame frame, should be 1 */

        This.m_nVecFrameCount = 0;
        This.m_nVecUPS = vector_updates;
        vector_updates = 0;
    }

}

static void AdjustColorMap(void)
{
    int i;
    
    for (i = 0; i < INTENSITY_LEVELS; i++)
    {
        This.m_pColorMap[i] = (unsigned char)((INTENSITY_LEVELS - 1.0)
                            * (This.m_nBrightness / 100.0)
                            * pow(((double)i / (INTENSITY_LEVELS - 1.0)), 1.0 / This.m_dGamma));
    }
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void SetPen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    assert(0 <= pen && pen < This.m_nTotalColors);
    assert(This.m_PalEntries      != NULL);
    assert(This.m_AdjustedPalette != NULL);

    This.m_PalEntries[pen].peRed    = red;
    This.m_PalEntries[pen].peGreen  = green;
    This.m_PalEntries[pen].peBlue   = blue;
    This.m_PalEntries[pen].peFlags  = PC_NOCOLLAPSE;

    memcpy(&This.m_AdjustedPalette[pen], &This.m_PalEntries[pen], sizeof(PALETTEENTRY));
    Display_MapColor(&This.m_AdjustedPalette[pen].peRed,
                     &This.m_AdjustedPalette[pen].peGreen,
                     &This.m_AdjustedPalette[pen].peBlue);
}

static void SetPaletteColors(void)
{
    assert(This.m_PalEntries      != NULL);
    assert(This.m_AdjustedPalette != NULL);

    if (This.m_nDepth == 8)
    {
        assert(This.m_hPalette != NULL);
        SetPaletteEntries(This.m_hPalette, 0, This.m_nTotalColors, This.m_AdjustedPalette);
    }
    else
    if (This.m_bModifiablePalette == TRUE)
    {
        UINT i;

        for (i = 0; i < This.m_nTotalColors; i++)
        {
	        This.m_p16BitLookup[i] = MAKECOL(This.m_AdjustedPalette[i].peRed,
                                             This.m_AdjustedPalette[i].peGreen,
                                             This.m_AdjustedPalette[i].peBlue) * 0x10001;
        }
    }
}

static void AdjustPalette(void)
{
    if (This.m_nDepth == 8 || This.m_bModifiablePalette == TRUE)
    {
        UINT i;
    
        assert(This.m_PalEntries      != NULL);
        assert(This.m_AdjustedPalette != NULL);

        memcpy(This.m_AdjustedPalette, This.m_PalEntries, This.m_nTotalColors * sizeof(PALETTEENTRY));

        for (i = 0; i < This.m_nTotalColors; i++)
        {
            if (i != This.m_nUIPen) /* Don't map the UI pen. */
                Display_MapColor(&This.m_AdjustedPalette[i].peRed,
                                 &This.m_AdjustedPalette[i].peGreen,
                                 &This.m_AdjustedPalette[i].peBlue);
        }

        This.m_bUpdatePalette = TRUE;
    }
}

static void AdjustVisibleRect(int xmin, int ymin, int xmax, int ymax)
{
    This.m_VisibleRect.m_Left   = xmin;
    This.m_VisibleRect.m_Top    = ymin;
    This.m_VisibleRect.m_Width  = xmax - xmin + 1;
    This.m_VisibleRect.m_Height = ymax - ymin + 1;
}


static void Display_Refresh()
{
	AdjustClientRect(FALSE);
    InvalidateRect(g_hMainWnd, NULL, FALSE); 
    UpdateWindow(g_hMainWnd);
}

static int Display_GetBlackPen(void)
{
    /* 
        NOTE: This method is called by osd_clearbitmap() when creating
        a new bitmap with osd_new_bitmap(). This is before the colors 
        are set in osd_allocate_colors().
        The code returns 0 in this case because m_nTotalColors is initialized
        to 0 which may be a problem.
    */

    if (This.m_nDepth == 8 || This.m_bModifiablePalette == TRUE)
    {
        int i;
        
        for (i = 0; i < This.m_nTotalColors; i++)
        {
            assert(This.m_PalEntries != NULL);

            if (This.m_PalEntries[i].peRed   == 0
            &&  This.m_PalEntries[i].peGreen == 0
            &&  This.m_PalEntries[i].peBlue  == 0)
            {
                return i;
            }
        }
    }

    return 0;
}
