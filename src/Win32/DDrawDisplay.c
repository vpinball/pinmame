/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  DDrawDisplay.c

 ***************************************************************************/

#include "driver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <ddraw.h>
#include <assert.h>
#include <math.h>
#include "MAME32.h"
#include "resource.h"
#include "DDrawDisplay.h"
#include "dirty.h"
#include "RenderBitmap.h"
#include "M32Util.h"
#include "DirectDraw.h"
#include "led.h"
#include "avi.h"
#include "dxdecode.h"

#define MAKECOL(r, g, b) (((r & 0xFF) >> (8 - This.m_Red.m_nSize))   << This.m_Red.m_nShift   | \
                          ((g & 0xFF) >> (8 - This.m_Green.m_nSize)) << This.m_Green.m_nShift | \
                          ((b & 0xFF) >> (8 - This.m_Blue.m_nSize))  << This.m_Blue.m_nShift)

#define GETC(col, pi)    (((col & (pi)->m_dwMask) >> (pi)->m_nShift)) << (8 - (pi)->m_nSize);

#define VERBOSE 0

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tPixelInfo
{
    int     m_nShift;
    int     m_nSize;
    DWORD   m_dwMask;
};

struct tDisplay_private
{
    struct osd_bitmap*  m_pBitmap;
    tRect               m_GameRect;         /* possibly doubled and clipped */
    UINT                m_nDisplayLines;    /* # of visible lines of bitmap */
    UINT                m_nDisplayColumns;  /* # of visible columns of bitmap */
    UINT                m_nSkipLinesMin;
    UINT                m_nSkipLinesMax;
    UINT                m_nSkipColumnsMax;
    UINT                m_nSkipColumnsMin;
    BOOL                m_bPanScreen;
    BOOL                m_bUseBackBuffer;
    BOOL                m_bVDouble;         /* for 1:2 aspect ratio */
    BOOL                m_bHDouble;         /* for 2:1 aspect ratio */
    enum DirtyMode      m_eDirtyMode;

    /* Options/Parameters */
    UINT                m_nDisplayIndex;
    BOOL                m_bBestFit;
    UINT                m_nScreenWidth;
    UINT                m_nScreenHeight;
    UINT                m_nSkipLines;
    UINT                m_nSkipColumns;
    BOOL                m_bHScanLines;
    BOOL                m_bVScanLines;
    BOOL                m_bDouble;
    BOOL                m_bStretchX;
    BOOL                m_bStretchY;
    BOOL                m_bUseDirty;
    UINT                m_bBltDouble;
    BOOL                m_bDisableMMX;

    /* AVI Capture params */
    BOOL                m_bAviCapture;      /* Capture AVI mode */
    BOOL                m_bAviRun;          /* Capturing frames */
    int                 m_nAviShowMessage;  /* Show status if !0 */

    /* Palette/Color */
    BOOL                m_bModifiablePalette;
    BOOL                m_bUpdatePalette;
    BOOL                m_bUpdateBackground;
    int                 m_nBlackPen;
    UINT                m_nUIPen;
    PALETTEENTRY*       m_pPalEntries;
    PALETTEENTRY*       m_pAdjustedPalette;
    UINT32*             m_p16BitLookup;
    UINT32              m_nTotalColors;
    int                 m_nDepth;
    struct tPixelInfo   m_Red;
    struct tPixelInfo   m_Green;
    struct tPixelInfo   m_Blue;

    /* DirectDraw */
    IDirectDraw2*       m_pDDraw;
    IDirectDrawSurface* m_pDDSPrimary;
    IDirectDrawSurface* m_pDDSBack;
    IDirectDrawPalette* m_pDDPal;

    BOOL                m_triple_buffer;

    RenderMethod        Render;

#ifdef MAME_DEBUG
    RenderMethod        DebugRender;
    int                 debugger_has_focus;
    unsigned char       oldpalette_red[DEBUGGER_TOTAL_COLORS];
    unsigned char       oldpalette_green[DEBUGGER_TOTAL_COLORS];
    unsigned char       oldpalette_blue[DEBUGGER_TOTAL_COLORS];
    const UINT8         *debug_palette;
#endif /* MAME_DEBUG */
};

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static void     ReleaseDDrawObjects(void);
static void     ClearSurface(IDirectDrawSurface* pddSurface);
static void     DrawSurface(IDirectDrawSurface* pddSurface);
static BOOL     BuildPalette(IDirectDraw2* pDDraw, PALETTEENTRY* pPalEntries, IDirectDrawPalette** ppDDPalette);
static void     SetPen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void     SetPaletteColors(void);
static int      FindBlackPen(void);
static BOOL     SurfaceLockable(IDirectDrawSurface* pddSurface);
static void     GetPixelInfo(DWORD dwMask, struct tPixelInfo* pPixelInfo);
static void     PanDisplay(void);
static void     AdjustDisplay(int xmin, int ymin, int xmax, int ymax);
static void     SelectDisplayMode(int width, int height, int depth);
static BOOL     FindDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwDepth);
static BOOL     FindBestDisplayMode(DWORD  dwWidthIn,   DWORD  dwHeightIn, DWORD dwDepth,
                                DWORD* pdwWidthOut, DWORD* pdwHeightOut);
static void     AdjustPalette(void);
static BOOL     OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);

static int                DDraw_init(options_type *options);
static void               DDraw_exit(void);
static struct osd_bitmap* DDraw_alloc_bitmap(int width, int height, int depth);
static void               DDraw_free_bitmap(struct osd_bitmap* bitmap);
static int                DDraw_create_display(int width, int height, int depth, int fps, int attributes, int orientation);
static void               DDraw_close_display(void);
static void               DDraw_set_visible_area(int min_x, int max_x, int min_y, int max_y);
static void               DDraw_set_debugger_focus(int debugger_has_focus);
static int                DDraw_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
static void               DDraw_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void               DDraw_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
static void               DDraw_mark_dirty(int x1, int y1, int x2, int y2);
static void               DDraw_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap);
static void               DDraw_led_w(int leds_status);
static void               DDraw_set_gamma(float gamma);
static void               DDraw_set_brightness(int brightness);
static void               DDraw_save_snapshot(struct osd_bitmap *bitmap);
static BOOL               DDraw_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void               DDraw_Refresh(void);
static int                DDraw_GetBlackPen(void);
static void               DDraw_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDDisplay DDrawDisplay = 
{
    DDraw_init,                 /* init              */
    DDraw_exit,                 /* exit              */
    DDraw_alloc_bitmap,         /* alloc_bitmap      */
    DDraw_free_bitmap,          /* free_bitmap       */
    DDraw_create_display,       /* create_display    */
    DDraw_close_display,        /* close_display     */
    DDraw_set_visible_area,     /* set_visible_area  */
    DDraw_set_debugger_focus,   /* set_debugger_focus*/
    DDraw_allocate_colors,      /* allocate_colors   */
    DDraw_modify_pen,           /* modify_pen        */
    DDraw_get_pen,              /* get_pen           */
    DDraw_mark_dirty,           /* mark_dirty        */
    0,                          /* skip_this_frame   */
    DDraw_update_display,       /* update_display    */
    DDraw_led_w,                /* led_w             */
    DDraw_set_gamma,            /* set_gamma         */
    Display_get_gamma,          /* get_gamma         */
    DDraw_set_brightness,       /* set_brightness    */
    Display_get_brightness,     /* get_brightness    */
    DDraw_save_snapshot,        /* save_snapshot     */
    
    DDraw_OnMessage,            /* OnMessage         */
    DDraw_Refresh,              /* Refresh           */
    DDraw_GetBlackPen,          /* GetBlackPen       */
    DDraw_UpdateFPS,            /* UpdateFPS         */
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDisplay_private This;

/***************************************************************************
    External OSD function definitions
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int DDraw_init(options_type *options)
{
    OSDDisplay.init(options);

    This.m_pBitmap           = NULL;
    This.m_GameRect.m_Top    = 0;
    This.m_GameRect.m_Left   = 0;
    This.m_GameRect.m_Width  = 0;
    This.m_GameRect.m_Height = 0;
    This.m_nDisplayLines     = 0;
    This.m_nDisplayColumns   = 0;
    This.m_bPanScreen        = FALSE;
    This.m_nSkipLinesMin     = 0;
    This.m_nSkipLinesMax     = 0;
    This.m_nSkipColumnsMax   = 0;
    This.m_nSkipColumnsMin   = 0;
    This.m_bUseBackBuffer    = FALSE;
    This.m_bVDouble          = FALSE;
    This.m_bHDouble          = FALSE;
    This.m_eDirtyMode        = NO_DIRTY;

    This.m_nDisplayIndex     = options->display_monitor;
    This.m_bBestFit          = options->display_best_fit;
    This.m_nScreenWidth      = options->width;
    This.m_nScreenHeight     = options->height;
    This.m_nSkipColumns      = max(0, options->skip_columns);
    This.m_nSkipLines        = max(0, options->skip_lines);
    This.m_bHScanLines       = options->hscan_lines;
    This.m_bVScanLines       = options->vscan_lines;
    This.m_bDouble           = (options->scale == 2);
    This.m_bStretchX         = FALSE;
    This.m_bStretchY         = FALSE;
    This.m_bUseDirty         = options->use_dirty;
    This.m_bBltDouble        = options->use_blit;
    This.m_bDisableMMX       = options->disable_mmx;

    This.m_bAviCapture       = GetAviCapture();
    This.m_bAviRun           = FALSE;
    This.m_nAviShowMessage   = (This.m_bAviCapture) ? 10: 0;

    This.m_bModifiablePalette = FALSE;
    This.m_bUpdatePalette     = FALSE;
    This.m_bUpdateBackground  = FALSE;
    This.m_nBlackPen          = 0;
    This.m_nUIPen             = 0;
    This.m_pPalEntries        = NULL;
    This.m_pAdjustedPalette   = NULL;
    This.m_p16BitLookup       = NULL;
    This.m_nTotalColors       = 0;
    This.m_nDepth             = 0;

    memset(&This.m_Red,   0, sizeof(struct tPixelInfo));
    memset(&This.m_Green, 0, sizeof(struct tPixelInfo));
    memset(&This.m_Blue,  0, sizeof(struct tPixelInfo));

    This.m_pDDraw            = NULL;
    This.m_pDDSPrimary       = NULL;
    This.m_pDDSBack          = NULL;
    This.m_pDDPal            = NULL;

    This.Render              = NULL;

    /* Check for inconsistent parameters. */
    if (This.m_bDouble == FALSE
    && (This.m_bHScanLines == FALSE || This.m_bVScanLines == FALSE))
    {
        This.m_bHScanLines = FALSE;
        This.m_bVScanLines = FALSE;
    }
    
    This.m_bBltDouble = (This.m_bBltDouble  == TRUE  &&
                         This.m_bDouble     == TRUE  && 
                         This.m_bHScanLines == FALSE && 
                         This.m_bVScanLines == FALSE);
    if (This.m_bBltDouble == TRUE)
    {
        This.m_bUseBackBuffer = TRUE;
    }

    This.m_triple_buffer = options->triple_buffer;

    if (This.m_triple_buffer == TRUE)
    {
       This.m_bUseDirty = FALSE;
       This.m_bUseBackBuffer = FALSE;
    }

    LED_init();

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void DDraw_exit(void)
{
    OSDDisplay.exit();
    ReleaseDDrawObjects();
    LED_exit();
}

static struct osd_bitmap* DDraw_alloc_bitmap(int width, int height, int depth)
{
    assert(OSDDisplay.alloc_bitmap != 0);

    return OSDDisplay.alloc_bitmap(width, height, depth);
}

static void DDraw_free_bitmap(struct osd_bitmap* pBitmap)
{
    assert(OSDDisplay.free_bitmap != 0);

    OSDDisplay.free_bitmap(pBitmap);
}

/*
    Create a display screen, or window, of the given dimensions (or larger).
    Attributes are the ones defined in driver.h.
    Returns 0 on success.
*/
static int DDraw_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    DDSURFACEDESC   ddSurfaceDesc;
    HRESULT         hResult = DD_OK;
    int             bmwidth;
    int             bmheight;

    This.m_nDepth = depth;

    /*
        Set up the DirectDraw object.
    */

    DirectDraw_CreateByIndex(This.m_nDisplayIndex);

    This.m_pDDraw = dd;
    if (This.m_pDDraw == NULL)
    {
        ErrorMsg("No DirectDraw object has been created");
        return 0;
    }

    if (This.m_bUseDirty == TRUE)
    {
        if (attributes & VIDEO_SUPPORTS_DIRTY)
        {
            This.m_eDirtyMode = USE_DIRTYRECT;
        }
        else
        {
            /* Don't use dirty if game doesn't support it. */
            This.m_eDirtyMode = NO_DIRTY;
        }
    }
    else
    {
        This.m_eDirtyMode = NO_DIRTY;
    }

    if ((attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) == VIDEO_PIXEL_ASPECT_RATIO_1_2)
    {
        if (orientation & ORIENTATION_SWAP_XY)
        {
            This.m_bHDouble    = TRUE;
            This.m_bVDouble    = FALSE;
            This.m_bDouble     = FALSE;
            This.m_bHScanLines = FALSE;
        }
        else
        {
            This.m_bHDouble    = FALSE;
            This.m_bVDouble    = TRUE;
            This.m_bDouble     = FALSE;
            This.m_bVScanLines = FALSE;
        }
    }

    if ((attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) == VIDEO_PIXEL_ASPECT_RATIO_2_1)
    {
        if (orientation & ORIENTATION_SWAP_XY)
        {
            This.m_bHDouble    = FALSE;
            This.m_bVDouble    = TRUE;
            This.m_bDouble     = FALSE;
            This.m_bVScanLines = FALSE;
        }
        else
        {
            This.m_bHDouble    = TRUE;
            This.m_bVDouble    = FALSE;
            This.m_bDouble     = FALSE;
            This.m_bHScanLines = FALSE;
        }
    }

    if (This.m_bBestFit == TRUE)
    {
        SelectDisplayMode(width, height, depth);
    }
    else
    {
        BOOL bFound = FALSE;
        /*
            Check to see if display mode exists since user
            can specify any arbitrary size.
        */
        bFound = FindDisplayMode(This.m_nScreenWidth, This.m_nScreenHeight, depth);
        if (bFound == FALSE)
        {
            ErrorMsg("DirectDraw does not support the requested mode %dx%d (%d bpp)",
                     This.m_nScreenWidth,
                     This.m_nScreenHeight,
                     depth);
            goto error;
        }
    }

    if (attributes & VIDEO_TYPE_VECTOR)
    {
        /* Vector games are always non-doubling. */
        This.m_bDouble = FALSE;

        /* Vector monitors have no scanlines. */
        This.m_bHScanLines = FALSE;
        This.m_bVScanLines = FALSE;

        if (This.m_bUseDirty == TRUE)
            This.m_eDirtyMode = USE_DIRTYRECT;
        else
            This.m_eDirtyMode = NO_DIRTY;
    }

    /* Center display. */
    AdjustDisplay(0, 0, width - 1, height - 1);

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

        if (Machine->orientation & ORIENTATION_SWAP_XY)
        {
            int temp;
            temp     = bmwidth;
            bmwidth  = bmheight;
            bmheight = temp;
        }
    }

    InitDirty(bmwidth, bmheight, This.m_eDirtyMode);

    /*
        Modify the main window to suit our needs.
    */
    SetWindowLong(MAME32App.m_hWnd, GWL_STYLE, WS_POPUP);
    SetWindowPos(MAME32App.m_hWnd,
                 HWND_TOPMOST,
                 0, 0,
                 GetSystemMetrics(SM_CXSCREEN) / 4,
                 GetSystemMetrics(SM_CYSCREEN) / 4,
                 0);

    ShowWindow(MAME32App.m_hWnd, SW_SHOW);
    UpdateWindow(MAME32App.m_hWnd);
    MAME32App.ProcessMessages();

    /*
        Get exclusive mode.
        "Windows does not support Mode X modes; therefore, when your application
        is in a Mode X mode, you cannot use the IDirectDrawSurface2::Lock or 
        IDirectDrawSurface2::Blt methods to lock or blit the primary surface."
    */
    hResult = IDirectDraw_SetCooperativeLevel(This.m_pDDraw,
                                              MAME32App.m_hWnd,
                                              /*DDSCL_ALLOWMODEX |*/
                                              DDSCL_EXCLUSIVE |
                                              DDSCL_FULLSCREEN);
    if (FAILED(hResult))
    {
        ErrorMsg("IDirectDraw.SetCooperativeLevel failed: %s", DirectXDecodeError(hResult));
        goto error;
    }

    {
        int frame_rate = (int)fps;
        /* most monitors can't go this low */
        if (frame_rate < 50)
            frame_rate *= 2;

        if (This.m_triple_buffer == FALSE)
            frame_rate = 0; /* old behavior--monitor default when not triple buffering */

    try_set_display_mode:
        hResult = IDirectDraw2_SetDisplayMode(This.m_pDDraw,
                                              This.m_nScreenWidth,
                                              This.m_nScreenHeight,
                                              depth, frame_rate, 0);
        
        if (FAILED(hResult))
        {
            if (frame_rate == 0)
            {
                ErrorMsg("IDirectDraw.SetDisplayMode failed: %s", DirectXDecodeError(hResult));
                goto error;
            }
            
            frame_rate = 0; /* tried to get the correct rate, now try anything */
            goto try_set_display_mode;
        }
    }

    /*
        Create Surface(s).
    */
    if (This.m_triple_buffer)
    {
        /* Create the primary surface with 2 back buffers. */
        memset(&ddSurfaceDesc, 0, sizeof(ddSurfaceDesc));
        ddSurfaceDesc.dwSize         = sizeof(ddSurfaceDesc);
        ddSurfaceDesc.dwFlags        = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
                                       DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
        ddSurfaceDesc.dwBackBufferCount = 2;

        hResult = IDirectDraw_CreateSurface(This.m_pDDraw,
                                            &ddSurfaceDesc,
                                            &This.m_pDDSPrimary,
                                            NULL);

        if (FAILED(hResult))
        {
            ErrorMsg("IDirectDraw.CreateSurface failed: %s", DirectXDecodeError(hResult));
            goto error;
        }

        /* If the surface can't be locked, clean up and use the backbuffer scheme. */
        if (SurfaceLockable(This.m_pDDSPrimary) == FALSE)
        {
            IDirectDraw_Release(This.m_pDDSPrimary);

            This.m_triple_buffer = FALSE;
            This.m_bUseBackBuffer = TRUE;
        }
        else
        {
           DDSCAPS ddscaps;
           ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
           
           hResult = IDirectDrawSurface_GetAttachedSurface(This.m_pDDSPrimary,
                                                           &ddscaps,
                                                           &This.m_pDDSBack);
           if (FAILED(hResult))
           {
              ErrorMsg("IDirectDraw.GetAttachedSurface failed: %s", DirectXDecodeError(hResult));
              goto error;
           }
        }
    }

    if (This.m_triple_buffer == FALSE)
    {
        if (This.m_bUseBackBuffer == FALSE)
        {
            /* Create the primary surface with no back buffers. */
            memset(&ddSurfaceDesc, 0, sizeof(ddSurfaceDesc));
            ddSurfaceDesc.dwSize         = sizeof(ddSurfaceDesc);
            ddSurfaceDesc.dwFlags        = DDSD_CAPS;
            ddSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
            
            hResult = IDirectDraw_CreateSurface(This.m_pDDraw,
                                                &ddSurfaceDesc,
                                                &This.m_pDDSPrimary,
                                                NULL);
            
            if (FAILED(hResult))
            {
                ErrorMsg("IDirectDraw.CreateSurface failed: %s", DirectXDecodeError(hResult));
                goto error;
            }
            
            /*
              This will cause the update_display to write directly to
              the primary display surface.
            */
            This.m_pDDSBack = This.m_pDDSPrimary;
            
            /* If the surface can't be locked, clean up and use the backbuffer scheme. */
            if (SurfaceLockable(This.m_pDDSPrimary) == FALSE)
            {
                IDirectDraw_Release(This.m_pDDSPrimary);
                
                This.m_bUseBackBuffer = TRUE;
            }
        }
        
        if (This.m_bUseBackBuffer == TRUE)
        {
            /* Create the primary surface with 1 back buffer. */
            memset(&ddSurfaceDesc, 0, sizeof(ddSurfaceDesc));
            ddSurfaceDesc.dwSize            = sizeof(ddSurfaceDesc);
            ddSurfaceDesc.dwFlags           = DDSD_CAPS;
            ddSurfaceDesc.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE;
            
            hResult = IDirectDraw_CreateSurface(This.m_pDDraw,
                                                &ddSurfaceDesc,
                                                &This.m_pDDSPrimary,
                                                NULL);
            
            if (FAILED(hResult))
            {
                ErrorMsg("IDirectDraw.CreateSurface failed: %s", DirectXDecodeError(hResult));
                goto error;
            }
            
            
            /* CMK 10/16/97 */
            /* Create a separate back buffer to blit with */
            
            ddSurfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
            
            if (This.m_bBltDouble == TRUE)
            {
                /* the backbuffer is now game sized when blitting 1/30/98 MR */
                ddSurfaceDesc.dwWidth  = width;
                ddSurfaceDesc.dwHeight = height;
            }
            else
            {
                /*
                  this doesn't really need to be this big, but the code is setup
                  to assume the backbuffer is screen sized, not game sized.  
                  Until that's changed, this works.
                */
                ddSurfaceDesc.dwWidth  = This.m_nScreenWidth;
                ddSurfaceDesc.dwHeight = This.m_nScreenHeight;
            }
            
            ddSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            ddSurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            hResult = IDirectDraw2_CreateSurface(This.m_pDDraw, &ddSurfaceDesc, &This.m_pDDSBack, NULL);
            
            if (FAILED(hResult))
            {
                ErrorMsg("IDirectDrawSurface.CreateSurface back buffer failed: %s", DirectXDecodeError(hResult));
                goto error;
            }
            
        }
    }

    UpdateWindow(MAME32App.m_hWnd);

    return 0;

error:
    DDraw_close_display();

    return 1;
}

/*
     Shut down the display
*/
static void DDraw_close_display(void)
{
    if (This.m_pDDraw != NULL)
    {
        IDirectDraw_RestoreDisplayMode(This.m_pDDraw);
        IDirectDraw_SetCooperativeLevel(This.m_pDDraw, MAME32App.m_hWnd, DDSCL_NORMAL);
    }

    ReleaseDDrawObjects();

    ExitDirty();

    if (This.m_pPalEntries != NULL)
    {
        free(This.m_pPalEntries);
        This.m_pPalEntries = NULL;
    }

    if (This.m_pAdjustedPalette != NULL)
    {
        free(This.m_pAdjustedPalette);
        This.m_pAdjustedPalette = NULL;
    }

    if (This.m_p16BitLookup != NULL)
    {
        free(This.m_p16BitLookup);
        This.m_p16BitLookup = NULL;
    }
}

static void DDraw_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
    AdjustDisplay(min_x, min_y, max_x, max_y);
}

static void DDraw_set_debugger_focus(int debugger_has_focus)
{
#ifdef MAME_DEBUG
    int i;

    if (!debugger_has_focus && This.debugger_has_focus) {
        /* Debugger losing focus */
        This.m_bUpdateBackground = 1;
        for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++) {
            DDraw_modify_pen(i+2, This.oldpalette_red[i], This.oldpalette_green[i], This.oldpalette_blue[i]);
        }
    }
    else if (debugger_has_focus && !This.debugger_has_focus) {
        /* Debugger gaining focus */
        for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++) {
            DDraw_get_pen(i+2, &This.oldpalette_red[i], &This.oldpalette_green[i], &This.oldpalette_blue[i]);
            DDraw_modify_pen(i+2, This.debug_palette[i*3+0], This.debug_palette[i*3+1], This.debug_palette[i*3+2]);
        }
    }
    This.debugger_has_focus = debugger_has_focus;
#endif
}

static int DDraw_allocate_colors(unsigned int totalcolors,
                                 const UINT8* palette,
                                 UINT32*      pens,
                                 int          modifiable,
                                 const UINT8* debug_palette,
                                 UINT32*      debug_pens)
{
    unsigned int    i;
    BOOL            bResult = TRUE;
    HRESULT         hResult = DD_OK;
    BOOL            bPalette16;

#ifdef MAME_DEBUG
    if (debug_pens) {
        modifiable = TRUE;
        This.debug_palette = debug_palette;
        for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++) {
            debug_pens[i] = i+2;
        }
    }
    else {
        This.debug_palette = NULL;
    }
#endif /* MAME_DEBUG */

    This.m_bModifiablePalette = modifiable ? TRUE : FALSE;

    This.m_nTotalColors = totalcolors;
    if (This.m_nDepth != 8)
        This.m_nTotalColors += 2;
    else
        This.m_nTotalColors = OSD_NUMPENS;

#ifdef MAME_DEBUG
    if (debug_pens &&This.m_nTotalColors < (DEBUGGER_TOTAL_COLORS+2))
        This.m_nTotalColors = DEBUGGER_TOTAL_COLORS+2;
#endif /* MAME_DEBUG */

    if (This.m_nDepth             == 16
    &&  This.m_bModifiablePalette == TRUE)
        bPalette16 = TRUE;
    else
        bPalette16 = FALSE;

    This.m_p16BitLookup     = (UINT32*)       malloc(This.m_nTotalColors * sizeof(UINT32));
    This.m_pPalEntries      = (PALETTEENTRY*) malloc(This.m_nTotalColors * sizeof(PALETTEENTRY));
    This.m_pAdjustedPalette = (PALETTEENTRY*) malloc(This.m_nTotalColors * sizeof(PALETTEENTRY));
    if (This.m_p16BitLookup == NULL || This.m_pPalEntries == NULL || This.m_pAdjustedPalette == NULL)
        return 1;

    if (This.m_nDepth != 8)
    {
        DDPIXELFORMAT   ddPixelFormat;

        memset(&ddPixelFormat, 0, sizeof(DDPIXELFORMAT));
        ddPixelFormat.dwSize  = sizeof(DDPIXELFORMAT);
        ddPixelFormat.dwFlags = DDPF_RGB;
        hResult = IDirectDrawSurface_GetPixelFormat(This.m_pDDSPrimary, &ddPixelFormat);
        if (FAILED(hResult))
        {
            ErrorMsg("IDirectDrawSurface.GetPixelFormat failed: %s", DirectXDecodeError(hResult));
            return 1;
        }

        GetPixelInfo(ddPixelFormat.dwRBitMask, &This.m_Red);       
        GetPixelInfo(ddPixelFormat.dwGBitMask, &This.m_Green);
        GetPixelInfo(ddPixelFormat.dwBBitMask, &This.m_Blue);
    }

    if (This.m_nDepth != 8 && This.m_bModifiablePalette == FALSE)
    {
        int r, g, b;
        /* 16 bit static palette. */

        for (i = 0; i < totalcolors; i++)
        {
            r = 255.0 * Display_get_brightness() * pow(palette[3 * i + 0] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            g = 255.0 * Display_get_brightness() * pow(palette[3 * i + 1] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            b = 255.0 * Display_get_brightness() * pow(palette[3 * i + 2] / 255.0, 1.0 / Display_get_gamma()) / 100.0;
            *pens++ = MAKECOL(r, g, b);
        }

        Machine->uifont->colortable[0] = 0;
        Machine->uifont->colortable[1] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[2] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[3] = 0;
    }
    else
    {
        /*
            Set the palette Entries.
        */

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
            /* 16 bit palette, or 8 bit palette with extra palette space left over. */

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

        /*
            Create the DirectDrawPalette.
        */

        if (This.m_nDepth == 8)
        {
            bResult = BuildPalette(This.m_pDDraw, This.m_pPalEntries, &This.m_pDDPal);
            if (bResult == FALSE)
                return 1;

            hResult = IDirectDrawSurface_SetPalette(This.m_pDDSPrimary, This.m_pDDPal);
            if (FAILED(hResult))
            {
                ErrorMsg("IDirectDrawSurface.SetPalette failed: %s", DirectXDecodeError(hResult));
                return 1;
            }
        }
        
        This.m_bUpdatePalette = TRUE;        
    }      


    /*
        Setup the rendering method.
    */

    if (This.m_bBltDouble == TRUE)
    {
        This.Render = SelectRenderMethod(FALSE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         This.m_eDirtyMode,
                                         This.m_nDepth == 16 ? TRUE : FALSE,
                                         bPalette16,
                                         This.m_p16BitLookup,
                                         MAME32App.m_bMMXDetected && !This.m_bDisableMMX ? TRUE: FALSE);
    }
    else
    {
        This.Render = SelectRenderMethod(This.m_bDouble,
                                         This.m_bHDouble,
                                         This.m_bVDouble,
                                         This.m_bHScanLines,
                                         This.m_bVScanLines,
                                         This.m_eDirtyMode,
                                         This.m_nDepth == 16 ? TRUE : FALSE,
                                         bPalette16,
                                         This.m_p16BitLookup,
                                         MAME32App.m_bMMXDetected && !This.m_bDisableMMX ? TRUE: FALSE);
    }

#ifdef MAME_DEBUG
    This.DebugRender = SelectRenderMethod(FALSE,
                                     FALSE,
                                     FALSE,
                                     FALSE,
                                     FALSE,
                                     NO_DIRTY,
                                     This.m_nDepth == 16 ? TRUE : FALSE,
                                     bPalette16,
                                     This.m_p16BitLookup,
                                     FALSE);
#endif

    /* Find the black pen to use for background. It may not be zero. */
    This.m_nBlackPen = FindBlackPen();

    /* Clear buffers. */
    UpdateWindow(MAME32App.m_hWnd);
    ClearSurface(This.m_pDDSPrimary);
    ClearSurface(This.m_pDDSBack);

    return 0;
}

/*
    Change the color of the pen.
*/
static void DDraw_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    assert(This.m_bModifiablePalette == TRUE); /* Shouldn't modify pen if flag is not set! */

    assert(0 <= pen && pen < This.m_nTotalColors);

    if (This.m_pPalEntries[pen].peRed    == red
    &&  This.m_pPalEntries[pen].peGreen  == green
    &&  This.m_pPalEntries[pen].peBlue   == blue)
        return;

    if (pen == This.m_nBlackPen)
        /* This.m_nBlackPen will be updated on update_display() */
        This.m_bUpdateBackground = TRUE;

    SetPen(pen, red, green, blue);

    This.m_bUpdatePalette = TRUE;
}

/*
    Get the color of a pen.
*/
static void DDraw_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue)
{
    assert(pRed   != NULL);
    assert(pGreen != NULL);
    assert(pBlue  != NULL);

    if (This.m_nDepth != 8 && This.m_bModifiablePalette == FALSE)
    {
        *pRed   = GETC(pen, &This.m_Red);
        *pGreen = GETC(pen, &This.m_Green);
        *pBlue  = GETC(pen, &This.m_Blue);        
    }
    else
    {
        assert(0 <= pen && pen < This.m_nTotalColors);

        if (This.m_nTotalColors <= pen)
            pen = 0;

        *pRed   = This.m_pPalEntries[pen].peRed;
        *pGreen = This.m_pPalEntries[pen].peGreen;
        *pBlue  = This.m_pPalEntries[pen].peBlue;
    }   
}

static void DDraw_mark_dirty(int x1, int y1, int x2, int y2)
{
    MarkDirty(x1, y1, x2, y2);
}

/*
    Update the display.
*/
static void DDraw_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap)
{
    HRESULT     hResult;

    if (This.m_pDDSBack    == NULL
    ||  This.m_pDDSPrimary == NULL)
        return;

#ifdef MAME_DEBUG
    if (debug_bitmap && input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
        osd_debugger_focus(This.debugger_has_focus ^ 1);

    This.m_pBitmap = This.debugger_has_focus ? debug_bitmap : game_bitmap;
#else /* !MAME_DEBUG */
    This.m_pBitmap = game_bitmap;
#endif /* MAME_DEBUG */

    if (This.m_triple_buffer)
        ClearSurface(This.m_pDDSBack);

    if (This.m_bUpdateBackground == TRUE)
    {
        This.m_nBlackPen = FindBlackPen();
        ClearSurface(This.m_pDDSPrimary);
        MarkAllDirty();

        This.m_bUpdateBackground = FALSE;
    }

    if (This.m_bUpdatePalette == TRUE)
    {
        SetPaletteColors();
        This.m_bUpdatePalette = FALSE;
    }

    profiler_mark(PROFILER_BLIT);
    DrawSurface(This.m_pDDSBack);

    if (This.m_triple_buffer == TRUE)
    {
        while (1)
        {
            hResult = IDirectDrawSurface_Flip(This.m_pDDSPrimary, NULL, DDFLIP_WAIT);
                
            if (hResult == DD_OK)
                break;

            if (hResult == DDERR_SURFACELOST)
            {
                hResult = IDirectDrawSurface_Restore(This.m_pDDSPrimary);
                if (FAILED(hResult))
                    break;
            }
            else
            if (hResult != DDERR_WASSTILLDRAWING)
            {
                ErrorMsg("IDirectDrawSurface.Flip failed: %s", DirectXDecodeError(hResult));
                break;
            }
        }
       
    }

    if (This.m_bUseBackBuffer == TRUE)
    {
        /* CMK 10/16/97 */
        RECT    rectSrc, rectDest;

        rectDest.top    = This.m_GameRect.m_Top;
        rectDest.left   = This.m_GameRect.m_Left;
        rectDest.bottom = This.m_GameRect.m_Height + This.m_GameRect.m_Top;
        rectDest.right  = This.m_GameRect.m_Width  + This.m_GameRect.m_Left;

        if (This.m_bBltDouble)
        {
            rectSrc.top    = This.m_nSkipLines;
            rectSrc.left   = This.m_nSkipColumns;
            rectSrc.bottom = This.m_nSkipLines   + This.m_nDisplayLines;
            rectSrc.right  = This.m_nSkipColumns + This.m_nDisplayColumns;
        }
        else
        {
            rectSrc = rectDest;
        }

        if (This.m_bStretchX)
        {
            rectDest.left   = 0;
            rectDest.right  = This.m_nScreenWidth;
        }
        if (This.m_bStretchY)
        {
            rectDest.top    = 0;
            rectDest.bottom = This.m_nScreenHeight;
        }

        while (1)
        {
            hResult = IDirectDrawSurface_Blt(This.m_pDDSPrimary,
                                             &rectDest,
                                             This.m_pDDSBack,
                                             &rectSrc,
                                             DDBLT_WAIT,
                                             NULL);
                
            if (hResult == DD_OK)
                break;

            if (hResult == DDERR_SURFACELOST)
            {
                hResult = IDirectDrawSurface_Restore(This.m_pDDSPrimary);
                if (FAILED(hResult))
                    break;
            }
            else
            if (hResult != DDERR_WASSTILLDRAWING)
            {
                ErrorMsg("IDirectDrawSurface.Blt failed: %s", DirectXDecodeError(hResult));
                break;
            }
        }
    }

    profiler_mark(PROFILER_END);

    /* Check for PGUP, PGDN and pan screen */
    if (This.m_bPanScreen == TRUE)
    {
        PanDisplay();
    }
}

/* control keyboard leds or other indicators */
static void DDraw_led_w(int leds_status)
{
    LED_StatusWrite(leds_status);
}

static void DDraw_set_gamma(float gamma)
{
    assert(OSDDisplay.set_gamma != 0);

    OSDDisplay.set_gamma(gamma);

    AdjustPalette();
}

static void DDraw_set_brightness(int brightness)
{
    assert(OSDDisplay.set_brightness != 0);

    OSDDisplay.set_brightness(brightness);

    AdjustPalette();
}

static void DDraw_save_snapshot(struct osd_bitmap *bitmap)
{
    if (This.m_bAviCapture)
    {
        This.m_nAviShowMessage = 10;        /* show message for 10 frames */
        This.m_bAviRun = !This.m_bAviRun;   /* toggle capture on/off */
    }
    else
        Display_WriteBitmap(bitmap, This.m_pPalEntries);
}

/***************************************************************************
    External functions
 ***************************************************************************/

/***************************************************************************
    Internal functions
 ***************************************************************************/

/*
    Draw the entire MAMEBitmap on the DirectDraw surface;
*/
static void DrawSurface(IDirectDrawSurface* pddSurface)
{
    BYTE*           pbScreen;
    HRESULT         hResult;
    DDSURFACEDESC   ddSurfaceDesc;
    UINT            nDisplayLines, nDisplayColumns, nSkipLines, nSkipColumns;
    UINT            nRenderSkipLines, nRenderSkipColumns;
    RenderMethod    Render;
    tRect           *pGameRect;
#ifdef MAME_DEBUG
    tRect           DebugRect;
#endif

    if (pddSurface == NULL)
        return;

    assert(This.m_pBitmap != NULL);

#ifdef MAME_DEBUG
    if (This.debugger_has_focus)
    {
        Render = This.DebugRender;
        nRenderSkipLines = nRenderSkipColumns = 0;
        nSkipLines = (This.m_nScreenHeight - This.m_pBitmap->height) / 2;
        nSkipColumns = (This.m_nScreenWidth - This.m_pBitmap->width) / 2;

        nDisplayLines = This.m_pBitmap->height;
        nDisplayColumns = This.m_pBitmap->width;
        pGameRect = &DebugRect;
        DebugRect.m_Top = nSkipLines;
        DebugRect.m_Left = nSkipColumns;
        DebugRect.m_Height = nDisplayLines;
        DebugRect.m_Width = nDisplayColumns;

#if VERBOSE
        logerror("DrawSurface(): Render=This.DebugRender This.m_nScreenHeight=%i This.m_nScreenWidth=%i nSkipLines=%i nSkipColumns=%i\n",
                 This.m_nScreenHeight, This.m_nScreenWidth, nSkipLines, nSkipColumns);
#endif
    }
    else
#endif
    {
        Render = This.Render;
        nRenderSkipLines = nSkipLines = This.m_nSkipLines;
        nRenderSkipColumns = nSkipColumns = This.m_nSkipColumns;

        nDisplayLines = This.m_nDisplayLines;
        nDisplayColumns = This.m_nDisplayColumns;
        pGameRect = &This.m_GameRect;
    }

    ddSurfaceDesc.dwSize = sizeof(ddSurfaceDesc);

    while (1)
    {
        hResult = IDirectDrawSurface_Lock(pddSurface, NULL, &ddSurfaceDesc, DDLOCK_WAIT, NULL);

        if (hResult == DD_OK)
            break;

        if (hResult == DDERR_SURFACELOST)
        {
            hResult = IDirectDrawSurface_Restore(pddSurface);
            if (FAILED(hResult))
                return;
        }
        else
        if (hResult != DDERR_WASSTILLDRAWING)
        {
            ErrorMsg("IDirectDrawSurface.Lock failed: %s", DirectXDecodeError(hResult));
            return;
        }
    }

    if (This.m_bBltDouble == TRUE)
    {


        if (This.m_nDepth == 8)
        {
            pbScreen = &((BYTE*)(ddSurfaceDesc.lpSurface))
                        [nSkipLines * ddSurfaceDesc.lPitch +
                         nSkipColumns];
        }
        else
        {
            pbScreen = (BYTE*)&((unsigned short*)(ddSurfaceDesc.lpSurface))
                        [nSkipLines * ddSurfaceDesc.lPitch / 2 +
                         nSkipColumns];
        }
    }
    else
    {
        if (This.m_nDepth == 8)
        {
            pbScreen = &((BYTE*)(ddSurfaceDesc.lpSurface))
                        [pGameRect->m_Top * ddSurfaceDesc.lPitch +
                         pGameRect->m_Left];
        }
        else
        {
            pbScreen = (BYTE*)&((unsigned short*)(ddSurfaceDesc.lpSurface))
                        [pGameRect->m_Top * ddSurfaceDesc.lPitch / 2 +
                         pGameRect->m_Left];
        }
    }

    if (This.m_bAviRun) /* add avi frame */
        AviAddBitmap(This.m_pBitmap, This.m_pPalEntries);
    
    if (This.m_nAviShowMessage > 0)
    {
        char    buf[32];
        
        This.m_nAviShowMessage--;
               
        sprintf(buf, "AVI Capture %s", (This.m_bAviRun) ? "ON" : "OFF");
        ui_text(This.m_pBitmap, buf, Machine->uiwidth - strlen(buf) * Machine->uifontwidth, 0);        
    }

    assert(Render != NULL);

    Render(This.m_pBitmap,
                nRenderSkipLines,
                nRenderSkipColumns,
                nDisplayLines,
                nDisplayColumns,
                pbScreen,
                ddSurfaceDesc.lPitch);

    hResult = IDirectDrawSurface_Unlock(pddSurface, NULL);
    if (FAILED(hResult))
    {
        ErrorMsg("IDirectDrawSurface.Unlock failed: %s", DirectXDecodeError(hResult));
        return;
    }
}

/*
    Set the entire DirectDraw surface to black;
*/
static void ClearSurface(IDirectDrawSurface* pddSurface)
{
    HRESULT     hResult;
    DDBLTFX     ddBltFX;

    memset(&ddBltFX, 0, sizeof(DDBLTFX));
    ddBltFX.dwSize      = sizeof(DDBLTFX);
    ddBltFX.dwFillColor = This.m_nBlackPen;

    while (1)
    {
        hResult = IDirectDrawSurface_Blt(pddSurface,
                                         NULL,
                                         NULL,
                                         NULL,
                                         DDBLT_WAIT | DDBLT_COLORFILL,
                                         &ddBltFX);
            
        if (hResult == DD_OK)
            break;

        if (hResult == DDERR_SURFACELOST)
        {
            hResult = IDirectDrawSurface_Restore(pddSurface);
            if (FAILED(hResult))
                break;
        }
        else
        if (hResult != DDERR_WASSTILLDRAWING)
        {
            break;
        }
    }

    return;
}

/*
    Build a palette.
*/
static BOOL BuildPalette(IDirectDraw2* pDDraw, PALETTEENTRY* pPalEntries, IDirectDrawPalette** ppDDPalette)
{
    HRESULT hResult;

    assert(This.m_nDepth == 8);

    hResult = IDirectDraw_CreatePalette(pDDraw,
                                        DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                                        pPalEntries,
                                        ppDDPalette,
                                        NULL);

    if (*ppDDPalette == NULL || FAILED(hResult))
    {
        ErrorMsg("IDirectDraw.CreatePalette failed: %s", DirectXDecodeError(hResult));
        return FALSE;
    }
    return TRUE;
}

/*
    Finished with all DirectDraw objects we use; release them.
*/
static void ReleaseDDrawObjects(void)
{
    if (This.m_pDDraw != NULL)
    {
        if (This.m_pDDPal != NULL)
        {
            IDirectDrawPalette_Release(This.m_pDDPal);
            This.m_pDDPal = NULL;
        }

        if (This.m_pDDSPrimary != NULL)
        {
            if (This.m_pDDSBack != NULL && This.m_pDDSBack != This.m_pDDSPrimary)
                IDirectDrawSurface_Release(This.m_pDDSBack);
            This.m_pDDSBack    = NULL;
              
            IDirectDrawSurface_Release(This.m_pDDSPrimary);
            This.m_pDDSPrimary = NULL;
        }

        This.m_pDDraw = NULL;
    }
}

static void SetPen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    assert(0 <= pen && pen < This.m_nTotalColors);
    assert(This.m_pPalEntries      != NULL);
    assert(This.m_pAdjustedPalette != NULL);

    This.m_pPalEntries[pen].peRed    = red;
    This.m_pPalEntries[pen].peGreen  = green;
    This.m_pPalEntries[pen].peBlue   = blue;
    This.m_pPalEntries[pen].peFlags  = PC_NOCOLLAPSE;

    memcpy(&This.m_pAdjustedPalette[pen], &This.m_pPalEntries[pen], sizeof(PALETTEENTRY));
    Display_MapColor(&This.m_pAdjustedPalette[pen].peRed,
                     &This.m_pAdjustedPalette[pen].peGreen,
                     &This.m_pAdjustedPalette[pen].peBlue);
}

static void SetPaletteColors()
{
    assert(This.m_pPalEntries      != NULL);
    assert(This.m_pAdjustedPalette != NULL);

    if (This.m_nDepth == 8)
    {
        HRESULT hResult;

        assert(This.m_pDDPal != NULL);

        hResult = IDirectDrawPalette_SetEntries(This.m_pDDPal,
                                                0,
                                                0,
                                                This.m_nTotalColors,                                                
                                                This.m_pAdjustedPalette);
        if (FAILED(hResult))
        {
            ErrorMsg("IDirectDrawPalette.SetEntries failed: %s", DirectXDecodeError(hResult));
        }
    }

    if (This.m_nDepth == 16 && This.m_bModifiablePalette == TRUE)
    {
        UINT i;

        for (i = 0; i < This.m_nTotalColors; i++)
        {
            This.m_p16BitLookup[i] = MAKECOL(This.m_pAdjustedPalette[i].peRed,
                                             This.m_pAdjustedPalette[i].peGreen,
                                             This.m_pAdjustedPalette[i].peBlue) * 0x10001;
        }
    }
}

static int FindBlackPen(void)
{    
    if (This.m_nDepth == 8 || This.m_bModifiablePalette == TRUE)
    {
        int i;

        assert(This.m_pPalEntries != NULL);

        for (i = 0; i < This.m_nTotalColors; i++)
        {
            if (This.m_pPalEntries[i].peRed   == 0
            &&  This.m_pPalEntries[i].peGreen == 0
            &&  This.m_pPalEntries[i].peBlue  == 0)
            {
                return i;
            }
        }
    }

    return 0;
}

static BOOL SurfaceLockable(IDirectDrawSurface* pddSurface)
{
    HRESULT         hResult;
    DDSURFACEDESC   ddSurfaceDesc;

    if (pddSurface == NULL)
        return FALSE;

    memset(&ddSurfaceDesc, 0, sizeof(DDSURFACEDESC));
    ddSurfaceDesc.dwSize = sizeof(ddSurfaceDesc);

    while (1)
    {
        hResult = IDirectDrawSurface_Lock(pddSurface, NULL, &ddSurfaceDesc, DDLOCK_WAIT, NULL);

        if (hResult == DD_OK)
        {
            hResult = IDirectDrawSurface_Unlock(pddSurface, NULL);
            return TRUE;
        }

        /*
            "Access to this surface is being refused because no driver exists
            which can supply a pointer to the surface.
            This is most likely to happen when attempting to lock the primary
            surface when no DCI provider is present.
            Will also happen on attempts to lock an optimized surface."
        */
        if (hResult == DDERR_CANTLOCKSURFACE)
        {
            return FALSE;
        }

        if (hResult == DDERR_SURFACELOST)
        {
            hResult = IDirectDrawSurface_Restore(pddSurface);
            if (FAILED(hResult))
                return FALSE;
        }
        else
        if (hResult != DDERR_WASSTILLDRAWING)
        {
            ErrorMsg("IDirectDrawSurface.Lock failed: %s", DirectXDecodeError(hResult));
            return FALSE;
        }
    }
}

static void GetPixelInfo(DWORD dwMask, struct tPixelInfo* pPixelInfo)
{
    int nShift = 0;
    int nCount = 0;
    int i;

    pPixelInfo->m_dwMask = dwMask;

    while ((dwMask & 1) == 0 && nShift < sizeof(DWORD) * 8)
    {
        nShift++;
        dwMask >>= 1;
    }
    pPixelInfo->m_nShift = nShift;

    for (i = 0; i < sizeof(DWORD) * 8; i++)
    {
        if (dwMask & (1 << i))
            nCount++;
    }
    pPixelInfo->m_nSize = nCount;
}

/*
    FindBestDisplayMode will search through the available display modes
    and select the mode which is just large enough for the input dimentions.
    If no display modes are large enough, the largest mode available is returned.
*/
static BOOL FindBestDisplayMode(DWORD  dwWidthIn,   DWORD  dwHeightIn, DWORD dwDepth,
                                DWORD* pdwWidthOut, DWORD* pdwHeightOut)
{
    struct tDisplayModes*   pDisplayModes;
    int     i;
    BOOL    bFound = FALSE;
    DWORD   dwBestWidth  = 10000;
    DWORD   dwBestHeight = 10000;
    DWORD   dwBiggestWidth  = 0;
    DWORD   dwBiggestHeight = 0;

#ifdef MAME_DEBUG
    if (dwWidthIn < options.debug_width)
        dwWidthIn = options.debug_width;
    if (dwHeightIn < options.debug_height)
        dwHeightIn = options.debug_height;
#endif /* MAME_DEBUG */

    pDisplayModes = DirectDraw_GetDisplayModes();

    assert(0 < pDisplayModes->m_nNumModes);

    for (i = 0; i < pDisplayModes->m_nNumModes; i++)
    {
        if (dwDepth != pDisplayModes->m_Modes[i].m_dwBPP)
            continue;

        /* Find a mode big enough for input size. */
        if (dwWidthIn  <= pDisplayModes->m_Modes[i].m_dwWidth
        &&  dwHeightIn <= pDisplayModes->m_Modes[i].m_dwHeight)
        {
            /* Get the smallest display size. */
            if (pDisplayModes->m_Modes[i].m_dwWidth  < dwBestWidth
            ||  pDisplayModes->m_Modes[i].m_dwHeight < dwBestHeight)
            {
                dwBestWidth  = pDisplayModes->m_Modes[i].m_dwWidth;
                dwBestHeight = pDisplayModes->m_Modes[i].m_dwHeight;
                bFound = TRUE;
            }
        }

        /*
            Keep track of the biggest size in case
            we can't find a mode that works.
        */
        if (dwBiggestWidth  < pDisplayModes->m_Modes[i].m_dwWidth
        ||  dwBiggestHeight < pDisplayModes->m_Modes[i].m_dwHeight)
        {
            dwBiggestWidth  = pDisplayModes->m_Modes[i].m_dwWidth;
            dwBiggestHeight = pDisplayModes->m_Modes[i].m_dwHeight;
        }
    }

    assert(pdwWidthOut  != NULL);
    assert(pdwHeightOut != NULL);

    if (bFound == TRUE)
    {
        *pdwWidthOut  = dwBestWidth;
        *pdwHeightOut = dwBestHeight;
    }
    else
    {
        *pdwWidthOut  = dwBiggestWidth;
        *pdwHeightOut = dwBiggestHeight;
    }

#if VERBOSE
    logerror("FindBestDisplayMode: dwWidthIn=%i dwHeightIn=%i dwDepth=%i *pdwWidthOut=%i *pdwHeightOut=%i\n",
             dwWidthIn, dwHeightIn, dwDepth, *pdwWidthOut, *pdwHeightOut);
#endif

    return bFound;
}

static BOOL FindDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwDepth)
{
    int     i;
    BOOL    bModeFound = FALSE;
    struct tDisplayModes*   pDisplayModes;

    pDisplayModes = DirectDraw_GetDisplayModes();

    for (i = 0; i < pDisplayModes->m_nNumModes; i++)
    {
        if (pDisplayModes->m_Modes[i].m_dwWidth  == dwWidth
        &&  pDisplayModes->m_Modes[i].m_dwHeight == dwHeight
        &&  pDisplayModes->m_Modes[i].m_dwBPP    == dwDepth)
        {
            bModeFound = TRUE;
            break;
        }
    }

    return bModeFound;
}

/*
    This function tries to find the best display mode.
*/
static void SelectDisplayMode(int width, int height, int depth)
{
    if (This.m_bBestFit == TRUE)
    {
        /* vector games use 640x480 as default */
        if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
        {
            This.m_nScreenWidth  = 640;
            This.m_nScreenHeight = 480;
        }
        else
        {
            BOOL bResult;

            if (This.m_bHDouble == TRUE)
            {
                width *= 2;
            }
            if (This.m_bVDouble == TRUE)
            {
                height *= 2;
            }

            if (This.m_bDouble == TRUE)
            {
                width  *= 2;
                height *= 2;
            }

            if (This.m_bDouble == TRUE)
            {
                bResult = FindBestDisplayMode(width, height, depth,
                                              (DWORD*)&This.m_nScreenWidth,
                                              (DWORD*)&This.m_nScreenHeight);

                if (bResult == FALSE)
                {
                    /* no pixel doubled modes fit, revert to not doubled */
                    This.m_bDouble = FALSE;
                    width  /= 2;
                    height /= 2;
                }
            }

            if (This.m_bDouble == FALSE)
            {
                FindBestDisplayMode(width, height, depth,
                                    (DWORD*)&This.m_nScreenWidth,
                                    (DWORD*)&This.m_nScreenHeight);
            }
        }
    }
}

/* Center image inside the display based on the visual area. */
static void AdjustDisplay(int xmin, int ymin, int xmax, int ymax)
{
    int gfx_xoffset;
    int gfx_yoffset;
    int width_factor=1;
    int xpad = 0; /* # of pad pixels for 8 pixel granularity */
    UINT viswidth;
    UINT visheight;

    viswidth  = xmax - xmin + 1;
    visheight = ymax - ymin + 1;

    if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
    {
        if (This.m_bDouble == TRUE)
            width_factor = 2;

        if (viswidth * width_factor < This.m_nScreenWidth)
        {
            UINT temp_width;
            int  temp_xmin;
            int  temp_xmax;

            temp_xmin  = xmin & (~0x7); /* quad align, 1 lower */
            temp_width = xmax - temp_xmin + 1;
            if (temp_width * width_factor <= This.m_nScreenWidth)
            {
                xpad = (xmax - temp_xmin + 1) - viswidth;
                viswidth += xpad;
                if (temp_width & 0x7)
                {
                    if (((temp_width + 7) & ~0x7 ) * width_factor <= This.m_nScreenWidth)
                    {
                        temp_xmax = xmax + 8 - (temp_width & 0x7);
                        xpad = (temp_xmax - temp_xmin + 1) - viswidth;
                        viswidth += xpad;
                    }
                }
            }
        }
    }

    This.m_nDisplayLines   = visheight;
    This.m_nDisplayColumns = viswidth;
    if (This.m_bDouble == TRUE && This.m_bHDouble == TRUE)
    {
        gfx_xoffset = (int)(This.m_nScreenWidth - viswidth * 4) / 2;
        if (This.m_nDisplayColumns > This.m_nScreenWidth / 4)
            This.m_nDisplayColumns = This.m_nScreenWidth / 4;
    }
    else
    if (This.m_bDouble == TRUE || This.m_bHDouble == TRUE)
    {
        gfx_xoffset = (int)(This.m_nScreenWidth - viswidth * 2) / 2;
        if (This.m_nDisplayColumns > This.m_nScreenWidth / 2)
            This.m_nDisplayColumns = This.m_nScreenWidth / 2;
    }
    else
    {
        gfx_xoffset = (int)(This.m_nScreenWidth - viswidth)  / 2;
        if (This.m_nDisplayColumns > This.m_nScreenWidth)
            This.m_nDisplayColumns = This.m_nScreenWidth;
    }

    if (This.m_bDouble == TRUE && This.m_bVDouble == TRUE)
    {
        gfx_yoffset = (int)(This.m_nScreenHeight - visheight * 4) / 2;
        if (This.m_nDisplayLines > This.m_nScreenHeight / 4)
            This.m_nDisplayLines = This.m_nScreenHeight / 4;
    }
    else
    if (This.m_bDouble == TRUE || This.m_bVDouble == TRUE)
    {
        gfx_yoffset = (int)(This.m_nScreenHeight - visheight * 2) / 2;
        if (This.m_nDisplayLines > This.m_nScreenHeight / 2)
            This.m_nDisplayLines = This.m_nScreenHeight / 2;
    }
    else
    {
        gfx_yoffset = (int)(This.m_nScreenHeight - visheight) / 2;
        if (This.m_nDisplayLines > This.m_nScreenHeight)
            This.m_nDisplayLines = This.m_nScreenHeight;
    }


    This.m_nSkipLinesMin   = ymin;
    This.m_nSkipLinesMax   = visheight - This.m_nDisplayLines   + ymin;
    This.m_nSkipColumnsMin = xmin;
    This.m_nSkipColumnsMax = viswidth  - This.m_nDisplayColumns + xmin;

    /* Align on a quadword !*/
    gfx_xoffset &= ~7;

    /* skipcolumns is relative to the visible area. */
    This.m_nSkipColumns = xmin + This.m_nSkipColumns;
    This.m_nSkipLines   = ymin + This.m_nSkipLines;

    /* Failsafe against silly parameters */
    if (This.m_nSkipLines < This.m_nSkipLinesMin)
        This.m_nSkipLines = This.m_nSkipLinesMin;
    if (This.m_nSkipColumns < This.m_nSkipColumnsMin)
        This.m_nSkipColumns = This.m_nSkipColumnsMin;
    if (This.m_nSkipLines > This.m_nSkipLinesMax)
        This.m_nSkipLines = This.m_nSkipLinesMax;
    if (This.m_nSkipColumns > This.m_nSkipColumnsMax)
        This.m_nSkipColumns = This.m_nSkipColumnsMax;

    /* Just in case the visual area doesn't fit */
    if (gfx_xoffset < 0)
    {
        gfx_xoffset = 0;
    }
    if (gfx_yoffset < 0)
    {
        gfx_yoffset = 0;
    }

    This.m_GameRect.m_Left   = gfx_xoffset;
    This.m_GameRect.m_Top    = gfx_yoffset;
    This.m_GameRect.m_Width  = This.m_nDisplayColumns;
    This.m_GameRect.m_Height = This.m_nDisplayLines;

    if (This.m_bHDouble == TRUE)
    {
        This.m_GameRect.m_Width *= 2;
    }
    if (This.m_bVDouble == TRUE)
    {
        This.m_GameRect.m_Height *= 2;
    }

    if (This.m_bDouble == TRUE)
    {
        This.m_GameRect.m_Width  *= 2;
        This.m_GameRect.m_Height *= 2;
    }
        
    /* Figure out if we need to check for PGUP and PGDN for panning. */
    if (This.m_nDisplayColumns < viswidth
    ||  This.m_nDisplayLines   < visheight) 
        This.m_bPanScreen = TRUE;

    set_ui_visarea(This.m_nSkipColumns, This.m_nSkipLines,
                   This.m_nSkipColumns + This.m_nDisplayColumns - 1 - xpad,
                   This.m_nSkipLines   + This.m_nDisplayLines - 1);

/*
    ErrorMsg("Screen width = %d, height = %d\n"
             "Game xoffset = %d, yoffset  = %d\n"
             "xmin %d xmax %d ymin %d ymax %d\n"
             "viswidth %d visheight %d\n"
             "Skipcolumns Min %d, Max %d\n"
             "Skiplines Min %d, Max%d\n"
             "Skip columns %d, lines %d\n"
             "Display columns %d, lines %d\n",
             This.m_nScreenWidth,
             This.m_nScreenHeight,
             gfx_xoffset, gfx_yoffset,
             xmin, xmax, ymin, ymax,
             viswidth, visheight,
             This.m_nSkipColumnsMin, This.m_nSkipColumnsMax,
             This.m_nSkipLinesMin, This.m_nSkipLinesMax,
             This.m_nSkipColumns, This.m_nSkipLines,
             This.m_nDisplayColumns,
             This.m_nDisplayLines);
*/
}

static void PanDisplay(void)
{
    BOOL bMarkDirty = FALSE;

    /* Horizontal panning. */
    if (input_ui_pressed_repeat(IPT_UI_PAN_LEFT, 1))
    {
        if (This.m_nSkipColumns < This.m_nSkipColumnsMax)
        {
            This.m_nSkipColumns++;
            bMarkDirty = TRUE;
        }
    }

    if (input_ui_pressed_repeat(IPT_UI_PAN_RIGHT, 1))
    {
        if (This.m_nSkipColumns > This.m_nSkipColumnsMin)
        {
            This.m_nSkipColumns--;
            bMarkDirty = TRUE;
        }
    }

    /* Vertical panning. */
    if (input_ui_pressed_repeat(IPT_UI_PAN_DOWN, 1))
    {
        if (This.m_nSkipLines < This.m_nSkipLinesMax)
        {
            This.m_nSkipLines++;
            bMarkDirty = TRUE;
        }
    }

    if (input_ui_pressed_repeat(IPT_UI_PAN_UP, 1))
    {
        if (This.m_nSkipLines > This.m_nSkipLinesMin)
        {
            This.m_nSkipLines--;
            bMarkDirty = TRUE;
        }
    }

    if (bMarkDirty == TRUE)
    {
        MarkAllDirty();

        set_ui_visarea(This.m_nSkipColumns, This.m_nSkipLines,
                       This.m_nDisplayColumns + This.m_nSkipColumns - 1,
                       This.m_nDisplayLines   + This.m_nSkipLines   - 1);
    }
}

static BOOL DDraw_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg)
    {
        HANDLE_MESSAGE(hWnd, WM_SETCURSOR,  OnSetCursor);
    }
    return FALSE;
}

static BOOL OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
    SetCursor(NULL);
 
    return TRUE;
}

static void DDraw_Refresh()
{
    This.m_bUpdateBackground = TRUE;

    DDraw_update_display(This.m_pBitmap, Machine->debug_bitmap);
}

static int DDraw_GetBlackPen(void)
{
    return This.m_nBlackPen;
}

static void DDraw_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS)
{
    char buf[64];

    if (bShow)
    {
        sprintf(buf, "fskp%2d%4d%%%4d/%d fps", nFrameskip, nSpeed, nFPS, nMachineFPS);
        ui_text(This.m_pBitmap, buf, Machine->uiwidth - strlen(buf) * Machine->uifontwidth, 0);

        if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
        {
            sprintf(buf, " %d vector updates", nVecUPS);
            ui_text(This.m_pBitmap, buf,
                    Machine->uiwidth - strlen(buf) * Machine->uifontwidth,
                    Machine->uifontheight);
        }
    }
}

static void AdjustPalette(void)
{
    if (This.m_nDepth == 8 || This.m_bModifiablePalette == TRUE)
    {
        UINT i;
        
        memcpy(This.m_pAdjustedPalette, This.m_pPalEntries, This.m_nTotalColors * sizeof(PALETTEENTRY));

        for (i = 0; i < This.m_nTotalColors; i++)
        {
            if (i != This.m_nUIPen) /* Don't map the UI pen. */
                Display_MapColor(&This.m_pAdjustedPalette[i].peRed,
                                 &This.m_pAdjustedPalette[i].peGreen,
                                 &This.m_pAdjustedPalette[i].peBlue);
        }

        This.m_bUpdatePalette = TRUE;        
    }
}
