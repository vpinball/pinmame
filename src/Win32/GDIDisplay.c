/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  GDIDisplay.c

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <assert.h>
#include <math.h>
#include "MAME32.h"
#include "resource.h"
#include "driver.h"
#include "GDIDisplay.h"
#include "M32Util.h"
#include "status.h"
#include "dirty.h"
#include "avi.h"
#include "Debug.h"

#define MAKECOL(r, g, b) ((((r & 0x000000F8) << 7)) | \
                          (((g & 0x000000F8) << 2)) | \
                          (((b & 0x000000F8) >> 3)))

#define GETR(col)  (col >> 7) & 0x000000F8;
#define GETG(col)  (col >> 2) & 0x000000F8;
#define GETB(col)  (col << 3) & 0x000000F8;

/***************************************************************************
    function prototypes
 ***************************************************************************/

static void             OnCommand(HWND hWnd, int CmdID, HWND hWndCtl, UINT codeNotify);
static void             OnPaint(HWND hWnd);
static void             OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange);
static BOOL             OnQueryNewPalette(HWND hWnd);
static void             OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo);
static BOOL             OnEraseBkgnd(HWND hWnd, HDC hDC);
static void             OnSize(HWND hwnd,UINT state,int cx,int cy);
static void             OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem);
static void             SetPen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void             SetPaletteColors(void);
static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void             AdjustVisibleRect(int xmin, int ymin, int xmax, int ymax);
static void             AdjustPalette(void);

static int                GDI_init(options_type *options);
static void               GDI_exit(void);
static struct osd_bitmap* GDI_alloc_bitmap(int width, int height,int depth);
static void               GDI_free_bitmap(struct osd_bitmap* bitmap);
static int                GDI_create_display(int width, int height, int depth, int fps, int attributes, int orientation);
static void               GDI_close_display(void);
static void               GDI_set_visible_area(int min_x, int max_x, int min_y, int max_y);
static void               GDI_set_debugger_focus(int debugger_has_focus);
static int                GDI_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
static void               GDI_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void               GDI_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
static void               GDI_mark_dirty(int x1, int y1, int x2, int y2);
static void               GDI_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap);
static void               GDI_led_w(int leds_status);
static void               GDI_set_gamma(float gamma);
static void               GDI_set_brightness(int brightness);
static void               GDI_save_snapshot(struct osd_bitmap *bitmap);
static BOOL               GDI_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void               GDI_Refresh(void);
static int                GDI_GetBlackPen(void);
static void               GDI_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDDisplay GDIDisplay = 
{
    GDI_init,               /* init              */
    GDI_exit,               /* exit              */
    GDI_alloc_bitmap,       /* aloc_bitmap       */
    GDI_free_bitmap,        /* free_bitmap       */
    GDI_create_display,     /* create_display    */
    GDI_close_display,      /* close_display     */
    GDI_set_visible_area,   /* set_visible_area  */
    GDI_set_debugger_focus, /* set_debugger_focus*/
    GDI_allocate_colors,    /* allocate_colors   */
    GDI_modify_pen,         /* modify_pen        */
    GDI_get_pen,            /* get_pen           */
    GDI_mark_dirty,         /* mark_dirty        */
    0,                      /* skip_this_frame   */
    GDI_update_display,     /* update_display    */
    GDI_led_w,              /* led_w             */
    GDI_set_gamma,          /* set_gamma         */
    Display_get_gamma,      /* get_gamma         */
    GDI_set_brightness,     /* set_brightness    */
    Display_get_brightness, /* get_brightness    */
    GDI_save_snapshot,      /* save_snapshot     */

    GDI_OnMessage,          /* OnMessage         */
    GDI_Refresh,            /* Refresh           */
    GDI_GetBlackPen,        /* GetBlackPen       */
    GDI_UpdateFPS,          /* UpdateFPS         */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

struct tDisplay_private
{
    struct osd_bitmap*  m_pBitmap;
    struct osd_bitmap*  m_pTempBitmap;
    BITMAPINFO*         m_pInfo;

    tRect               m_VisibleRect;
    int                 m_nClientWidth;
    int                 m_nClientHeight;
    RECT                m_ClientRect;
    int                 m_nWindowWidth;
    int                 m_nWindowHeight;

    BOOL                m_bScanlines;
    BOOL                m_bDouble;
    BOOL                m_bVectorDouble;
    BOOL                m_bUseDirty;
    int                 m_nDepth;

    /* AVI Capture params */
    BOOL                m_bAviCapture;      /* Capture AVI mode */
    BOOL                m_bAviRun;          /* Capturing frames */
    int                 m_nAviShowMessage;  /* Show status if !0 */

    BOOL                m_bModifiablePalette;
    BOOL                m_bUpdatePalette;
    HPALETTE            m_hPalette;
    PALETTEENTRY*       m_PalEntries;
    PALETTEENTRY*       m_AdjustedPalette;
    UINT                m_nUIPen;
    UINT32*             m_p16BitLookup;
    UINT32              m_nTotalColors;

    HWND                m_hWndAbout;
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDisplay_private      This;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int GDI_init(options_type *options)
{
    OSDDisplay.init(options);

    This.m_pBitmap          = NULL;
    This.m_pTempBitmap      = NULL;
    This.m_pInfo            = NULL;

    memset(&This.m_VisibleRect, 0, sizeof(tRect));
    This.m_nClientWidth     = 0;
    This.m_nClientHeight    = 0;
    This.m_nWindowWidth     = 0;
    This.m_nWindowHeight    = 0;

    This.m_bScanlines       = options->hscan_lines;
    This.m_bDouble          = (options->scale == 2);
    This.m_bVectorDouble    = options->double_vector;
    This.m_bUseDirty        = FALSE; /* options->use_dirty; */
    This.m_nDepth           = Machine->color_depth;

    This.m_bAviCapture      = GetAviCapture();
    This.m_bAviRun          = FALSE;
    This.m_nAviShowMessage  = (This.m_bAviCapture) ? 10: 0;

    This.m_bModifiablePalette = FALSE;
    This.m_bUpdatePalette   = FALSE;
    This.m_hPalette         = NULL;
    This.m_hWndAbout        = NULL;
    This.m_PalEntries       = NULL;
    This.m_AdjustedPalette  = NULL;
    This.m_nUIPen           = 0;
    This.m_p16BitLookup     = NULL;
    This.m_nTotalColors     = 0;

    /* Scanlines don't work. */
    This.m_bScanlines = FALSE;

    /* Check for inconsistent parameters. */
    if (This.m_bScanlines == TRUE
    &&  This.m_bDouble    == FALSE)
    {
        This.m_bScanlines = FALSE;
    }

    MAME32Debug_init(options);

    return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void GDI_exit(void)
{
    OSDDisplay.exit();

    MAME32Debug_exit();
}

static struct osd_bitmap* GDI_alloc_bitmap(int width, int height,int depth)
{
    assert(OSDDisplay.alloc_bitmap != 0);

    return OSDDisplay.alloc_bitmap(width, height, depth);
}

static void GDI_free_bitmap(struct osd_bitmap* pBitmap)
{
    assert(OSDDisplay.free_bitmap != 0);

    OSDDisplay.free_bitmap(pBitmap);
}

/*
    Create a display screen, or window, of the given dimensions (or larger).
    Attributes are the ones defined in driver.h.
    Returns 0 on success.
*/
static int GDI_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    unsigned int    i;
    RECT            Rect;
    HMENU           hMenu;
    LOGPALETTE      LogPalette;
    char            TitleBuf[256];
    int             bmwidth;
    int             bmheight;

    This.m_nDepth = depth;
    if (This.m_nDepth == 15)
        This.m_nDepth = 16;

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

    if (This.m_bUseDirty == TRUE)
    {
        InitDirty(bmwidth, bmheight, NO_DIRTY);
    }

    /* Make a copy of the scrbitmap for 16 bit palette lookup. */
    This.m_pTempBitmap = osd_alloc_bitmap(bmwidth, bmheight, This.m_nDepth);

    /* Create BitmapInfo */
    This.m_pInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) +
                                       sizeof(RGBQUAD) * OSD_NUMPENS);

    This.m_pInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER); 
    This.m_pInfo->bmiHeader.biWidth         =  (This.m_pTempBitmap->line[1] - This.m_pTempBitmap->line[0]) / (This.m_nDepth / 8);
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
        /* Palette */
        LogPalette.palVersion    = 0x0300;
        LogPalette.palNumEntries = 1;
        This.m_hPalette = CreatePalette(&LogPalette);
        ResizePalette(This.m_hPalette, OSD_NUMPENS);

        /* Map image values to palette index */
        for (i = 0; i < OSD_NUMPENS; i++)
            ((WORD*)(This.m_pInfo->bmiColors))[i] = i;
    }

    /*
        Modify the main window to suit our needs.
    */
    hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAIN_MENU));
    SetMenu(MAME32App.m_hWnd, hMenu);

    SetWindowLong(MAME32App.m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME | WS_BORDER);

    sprintf(TitleBuf, "%s - %s", Machine->gamedrv->description, MAME32App.m_Name);
    SetWindowText(MAME32App.m_hWnd, TitleBuf);

    StatusCreate();

    This.m_ClientRect.left   = 0;
    This.m_ClientRect.top    = 0;
    This.m_ClientRect.right  = This.m_nClientWidth;
    This.m_ClientRect.bottom = This.m_nClientHeight;

    /* Calculate size of window based on desired client area. */
    Rect.left   = This.m_ClientRect.left;
    Rect.top    = This.m_ClientRect.top;
    Rect.right  = This.m_ClientRect.right;
    Rect.bottom = This.m_ClientRect.bottom + GetStatusHeight();
    AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME | WS_BORDER, hMenu != NULL);

    This.m_nWindowWidth  = RECT_WIDTH(Rect);
    This.m_nWindowHeight = RECT_HEIGHT(Rect);

    SetWindowPos(MAME32App.m_hWnd,
                 HWND_NOTOPMOST,
                 0, 0,
                 This.m_nWindowWidth,
                 This.m_nWindowHeight,
                 SWP_NOMOVE);

    This.m_hWndAbout = CreateDialog(GetModuleHandle(NULL),
                                    MAKEINTRESOURCE(IDD_ABOUT),
                                    MAME32App.m_hWnd,
                                    AboutDialogProc);

    ShowWindow(MAME32App.m_hWnd, SW_SHOW);
    SetForegroundWindow(MAME32App.m_hWnd);


    MAME32Debug_create_display(options.debug_width,
                               options.debug_height,
                               depth, fps, attributes, orientation);

    return 0;
}

/*
     Shut down the display
*/
static void GDI_close_display(void)
{
    ExitDirty();
    
    if (This.m_nDepth == 8)
    {
        if (This.m_hPalette != NULL)
        {
            DeletePalette(This.m_hPalette);
            This.m_hPalette = NULL;
        }
    }

    if (IsWindow(This.m_hWndAbout))
    {
        DestroyWindow(This.m_hWndAbout);
        This.m_hWndAbout = NULL;
    }

    StatusDelete();
   
    osd_free_bitmap(This.m_pTempBitmap);

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

    MAME32Debug_close_display();
}

/*
    center image inside the display based on the visual area
*/
static void GDI_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
    AdjustVisibleRect(min_x, min_y, max_x, max_y);

    set_ui_visarea(This.m_VisibleRect.m_Left,
                   This.m_VisibleRect.m_Top,
                   This.m_VisibleRect.m_Left + This.m_VisibleRect.m_Width  - 1,
                   This.m_VisibleRect.m_Top  + This.m_VisibleRect.m_Height - 1);
}

static void GDI_set_debugger_focus(int debugger_has_focus)
{
    if (!debugger_has_focus)
    {
        SetForegroundWindow(MAME32App.m_hWnd);
    }

    MAME32Debug_set_debugger_focus(debugger_has_focus);
}

static int GDI_allocate_colors(unsigned int totalcolors,
                               const UINT8* palette,
                               UINT32*      pens,
                               int          modifiable,
                               const UINT8* debug_palette,
                               UINT32*      debug_pens)
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
            *pens++ = MAKECOL(r, g, b);
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

    MAME32Debug_allocate_colors(modifiable, debug_palette, debug_pens);

    return 0;
}

/*
    Change the color of the pen.
*/
static void GDI_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
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
static void GDI_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue)
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

static void GDI_mark_dirty(int x1, int y1, int x2, int y2)
{
}

/*
    Update the display.
*/
static void GDI_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap)
{
    This.m_pBitmap = game_bitmap;

    if (This.m_bUpdatePalette == TRUE)
    {
        SetPaletteColors();
        This.m_bUpdatePalette = FALSE;
    }

    InvalidateRect(MAME32App.m_hWnd, &This.m_ClientRect, FALSE);

    UpdateWindow(MAME32App.m_hWnd);

    StatusUpdate();

    MAME32Debug_update_display(debug_bitmap);

    MAME32App.ProcessMessages();
}

/* control keyboard leds or other indicators */
static void GDI_led_w(int leds_status)
{
    StatusWrite(leds_status);
}

static void GDI_set_gamma(float gamma)
{
    assert(OSDDisplay.set_gamma != 0);

    OSDDisplay.set_gamma(gamma);

    AdjustPalette();
}

static void GDI_set_brightness(int brightness)
{
    assert(OSDDisplay.set_brightness != 0);

    OSDDisplay.set_brightness(brightness);

    AdjustPalette();
}

static void GDI_save_snapshot(struct osd_bitmap *bitmap)
{
    if (This.m_bAviCapture)
    {
        This.m_nAviShowMessage = 10;        /* show message for 10 frames */
        This.m_bAviRun = !This.m_bAviRun;   /* toggle capture on/off */
    }
    else
        Display_WriteBitmap(bitmap, This.m_PalEntries);
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

static BOOL GDI_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg)
    {
        HANDLE_MESSAGE(hWnd, WM_COMMAND,            OnCommand);
        HANDLE_MESSAGE(hWnd, WM_PAINT,              OnPaint);
        HANDLE_MESSAGE(hWnd, WM_PALETTECHANGED,     OnPaletteChanged);
        HANDLE_MESSAGE(hWnd, WM_QUERYNEWPALETTE,    OnQueryNewPalette);
        HANDLE_MESSAGE(hWnd, WM_GETMINMAXINFO,      OnGetMinMaxInfo);
        HANDLE_MESSAGE(hWnd, WM_ERASEBKGND,         OnEraseBkgnd);
        HANDLE_MESSAGE(hWnd, WM_SIZE,               OnSize);
        HANDLE_MESSAGE(hWnd, WM_DRAWITEM,           OnDrawItem);
    }
    return FALSE;
}

static void GDI_Refresh()
{
    InvalidateRect(MAME32App.m_hWnd, &This.m_ClientRect, FALSE); 
    UpdateWindow(MAME32App.m_hWnd);
}

static int GDI_GetBlackPen(void)
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

static void GDI_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS)
{
    StatusUpdateFPS(bShow, nSpeed, nFPS, nMachineFPS, nFrameskip, nVecUPS);
}

static void OnCommand(HWND hWnd, int CmdID, HWND hWndCtl, UINT codeNotify)
{
    switch (CmdID)
    {
        case ID_FILE_EXIT:
            /* same as double-clicking on main window close box */
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;

        case ID_ABOUT:
            ShowWindow(This.m_hWndAbout, SW_SHOW);
        break;
    }
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT     ps;

    BeginPaint(hWnd, &ps);

    if (This.m_pBitmap == NULL)
    {
        EndPaint(hWnd, &ps); 
        return;
    }

    if (This.m_bAviRun) /* add avi frame */
        AviAddBitmap(This.m_pBitmap, This.m_PalEntries);
    
    if (This.m_nAviShowMessage > 0)
    {
        char    buf[20];
        
        This.m_nAviShowMessage--;
        sprintf(buf, "AVI Capture %s", (This.m_bAviRun) ? "ON" : "OFF");
        StatusSetString(buf);
    }

    if (This.m_nDepth == 8)
    {
        /* 8 bit uses windows palette. */
        HPALETTE hOldPalette;

        hOldPalette = SelectPalette(ps.hdc, This.m_hPalette, FALSE);
        RealizePalette(ps.hdc);

        StretchDIBits(ps.hdc,
                      0, 0,
                      This.m_nClientWidth,
                      This.m_nClientHeight,
                      0,
                      0,
                      This.m_VisibleRect.m_Width,
                      This.m_VisibleRect.m_Height,                      
                      This.m_pBitmap->line[This.m_VisibleRect.m_Top] + This.m_VisibleRect.m_Left,
                      This.m_pInfo,
                      DIB_PAL_COLORS,
                      SRCCOPY);

        if (hOldPalette != NULL)
            SelectPalette(ps.hdc, hOldPalette, FALSE);
    }
    else
    {
        if (This.m_bModifiablePalette)
        {
            int x, y;

            /* ugh, map indexes to 555 rgb */
            for (y = 0; y < This.m_VisibleRect.m_Height; y++)
            {
                UINT16* pSrc = (UINT16*)(This.m_pBitmap->line[This.m_VisibleRect.m_Top + y]
                             + (This.m_VisibleRect.m_Left * 2));
                UINT32* pDst = (UINT32*)(This.m_pTempBitmap->line[This.m_VisibleRect.m_Top + y]
                             + (This.m_VisibleRect.m_Left * 2));

                for (x = 0; x < This.m_VisibleRect.m_Width / 2; x++)
                {
                    UINT32 d1, d2;
                    d1 = This.m_p16BitLookup[*(pSrc++)];
                    d2 = This.m_p16BitLookup[*(pSrc++)];

                    *pDst++ = (d1 & 0x0000ffff) | (d2 & 0xffff0000);
                }
            }

            StretchDIBits(ps.hdc,
                          0, 0,
                          This.m_nClientWidth,
                          This.m_nClientHeight,
                          0,
                          0,
                          This.m_VisibleRect.m_Width,
                          This.m_VisibleRect.m_Height,
                          This.m_pTempBitmap->line[This.m_VisibleRect.m_Top] + (This.m_VisibleRect.m_Left * 2),
                          This.m_pInfo,
                          DIB_RGB_COLORS,
                          SRCCOPY);
        }
        else
        {
            StretchDIBits(ps.hdc,
                          0, 0,
                          This.m_nClientWidth,
                          This.m_nClientHeight,
                          0,
                          0,
                          This.m_VisibleRect.m_Width,
                          This.m_VisibleRect.m_Height,
                          This.m_pBitmap->line[This.m_VisibleRect.m_Top] + (This.m_VisibleRect.m_Left * 2),
                          This.m_pInfo,
                          DIB_RGB_COLORS,
                          SRCCOPY);
        }
    }

    EndPaint(hWnd, &ps); 
}

static void OnPaletteChanged(HWND hWnd, HWND hWndPaletteChange)
{
    if (hWnd == hWndPaletteChange) 
        return; 
    
    OnQueryNewPalette(hWnd);
}

static BOOL OnQueryNewPalette(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, TRUE); 
    UpdateWindow(hWnd); 
    return TRUE;
}

static void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo)
{
    pMinMaxInfo->ptMaxSize.x      = This.m_nWindowWidth;
    pMinMaxInfo->ptMaxSize.y      = This.m_nWindowHeight;
    pMinMaxInfo->ptMaxTrackSize.x = This.m_nWindowWidth;
    pMinMaxInfo->ptMaxTrackSize.y = This.m_nWindowHeight;
    pMinMaxInfo->ptMinTrackSize.x = This.m_nWindowWidth;
    pMinMaxInfo->ptMinTrackSize.y = This.m_nWindowHeight;
}

static BOOL OnEraseBkgnd(HWND hWnd, HDC hDC)
{
    /* No background erasing is required. */
    return TRUE;
}

static void OnSize(HWND hwnd,UINT state,int cx,int cy)
{
    StatusWindowSize(state,cx,cy);
}

static void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem)
{
    StatusDrawItem(lpDrawItem);
}

static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
        {
            HBITMAP hBitmap;
            hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
                                         MAKEINTRESOURCE(IDB_ABOUT),
                                         IMAGE_BITMAP, 0, 0, LR_SHARED);
            SendMessage(GetDlgItem(hDlg, IDC_ABOUT), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
            Static_SetText(GetDlgItem(hDlg, IDC_VERSION), GetVersionString());
            return 1;
        }

        case WM_COMMAND:
            ShowWindow(hDlg, SW_HIDE);
            return 1;
    }
    return 0;
}

