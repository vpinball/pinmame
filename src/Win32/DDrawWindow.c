/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

    DDrawWindow.c

    Remapping when in non-8 bit color depth stuff:

    If drawing in a window and screen is in 8 bit color depth, things are
    easy.  We set bitmap->lines[] to point to the lines in the back buffer, and
    just do a fast BitBlt onto video memory.  Palette changes
    are passed along to the hardware.  Simple.

    If drawing in a window and screen is 16 bit color depth--let's get back to this.

    If drawing in a window and screen is 24+ bit color depth, this is easy.  Make
    an array of what to write to video memory for each of the 256 colors in
    bitmap->lines[].  Obviously very slow, because for each pixel we write
    3 or more bytes to video RAM.  Life's tough.

    Ok, back to 16 bit color depth.  We could handle it just like 24+ bit color
    depth.  However, this means there's one write to RAM for each pixel (two bytes).
    Now, Pentium is much better at handling writes of four bytes at a time.  So
    we build an array of what to write to video memory for any combination of
    TWO consecutive pixels.  This is array of 65536 elements, each entry being
    a dword to write to video ram for this two-pixel combination.  Takes a lot
    of ram, but if the top left of the game is dword aligned, this makes
    it a lot faster--about 50% faster, since there are 50% fewer memory writes.
    This 50% speedup really does happen, and I just spent the last 5 hours getting
    this to work, just so you people who want to run mame in a window AND must
    have 16 bit color depth can have decent performance.

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
#include "display.h"
#include "DDrawWindow.h"
#include "M32Util.h"
#include "status.h"
#include "dirty.h"
#include "DirectDraw.h"
#include "avi.h"

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
    int    width, height;
    int    pitch;        /* Distance between addresses of two vertically adjacent pixels */
                         /* (may not equal width in some cases). */
    char   *base_bits;   /* Raw bits of surface */
    char   *bits;        /* Raw bits of surface to draw at (top left of window if windowed) */
} Surface;

typedef struct
{
    PALETTEENTRY entries[OSD_NUMPENS];
} CKPALETTE;

typedef struct
{
    int     m_nShift;
    int     m_nSize;
    DWORD   m_dwMask;
} PIXELINFO;

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static BOOL             DirectDrawSetupDrawing(void);
static void             DirectDrawCalcDepth(int *bytes_per_pixel, PIXELINFO *r, PIXELINFO *g, PIXELINFO *b);
static BOOL             DirectDrawCreateSurfaces(void);
static BOOL             DirectDrawLock(BOOL front,Surface *s);
static BOOL             DirectDrawUnlock(BOOL front,Surface *s);
static BOOL             DirectDrawClipToWindow(void);
static void             DirectDrawClearFrontBuffer(void);
static void             DirectDrawClearBuffer(LPDIRECTDRAWSURFACE ddbuffer, DWORD fill_color);
static BOOL             DirectDrawRestoreSurfaces(void);
static BOOL             DirectDrawCreatePalette(CKPALETTE *palette);
static void             DirectDrawSetPaletteColors(LPPALETTEENTRY ppe);
static int              FindBlackPen(void);
static BOOL             DirectDrawSetPalette(void);
static BOOL             DirectDrawUpdateScreen(void);

static void             DrawGame(void);
static void Render8Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                         enum DirtyMode dirty_this_frame);
static void Render8DepthVDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                                enum DirtyMode dirty_this_frame);
static void Render8DepthDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                               enum DirtyMode dirty_this_frame);
static void Render16Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                          enum DirtyMode dirty_this_frame);
static void Render16DepthVDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                                 enum DirtyMode dirty_this_frame);
static void Render16to16Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                              enum DirtyMode dirty_this_frame);
static void RenderGenericDepth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                               enum DirtyMode dirty_this_frame);
static void RenderGenericDepthVDouble(Surface surface,int start_x,int start_y,int length_x,
                                      int length_y,enum DirtyMode dirty_this_frame);
static BOOL IsWindowObscured(void);
static void             __inline__ CalcDepthRemapColor(DWORD *rgb, int index);
static void             osd_win32_create_palette(void);
static void             osd_win32_change_color(int index);
static void             AdjustVisibleRect(int xmin, int ymin, int xmax, int ymax);
static int              FindNearestColor(int min_entry,int max_entry,int r,int g,int b);
static void             AdjustPalette(void);

static void             OnActivateApp(HWND hWnd, BOOL fActivate, DWORD dwThreadId);
static void             OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo);
static void             OnPaint(HWND hWnd);
static void             OnSize(HWND hwnd,UINT state,int cx,int cy);
static void             OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem);
static void             OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);

static void ReleaseDDrawObjects(void);
static int                DDrawWindow_init(options_type *options);
static void               DDrawWindow_exit(void);
static struct osd_bitmap* DDrawWindow_alloc_bitmap(int width, int height, int depth);
static void               DDrawWindow_free_bitmap(struct osd_bitmap* bitmap);
static int                DDrawWindow_create_display(int width, int height, int depth, int fps, int attributes, int orientation);
static void               DDrawWindow_close_display(void);
static void               DDrawWindow_set_visible_area(int min_x, int max_x, int min_y, int max_y);
static void               DDrawWindow_set_debugger_focus(int debugger_has_focus);
static int                DDrawWindow_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
static void               DDrawWindow_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
static void               DDrawWindow_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
static void               DDrawWindow_mark_dirty(int x1, int y1, int x2, int y2);
static void               DDrawWindow_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap);
static void               DDrawWindow_led_w(int leds_status);
static void               DDrawWindow_set_gamma(float gamma);
static void               DDrawWindow_set_brightness(int brightness);
static void               DDrawWindow_save_snapshot(struct osd_bitmap *bitmap);
static BOOL               DDrawWindow_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
static void               DDrawWindow_Refresh(void);
static int                DDrawWindow_GetBlackPen(void);
static void               DDrawWindow_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDDisplay DDrawWindowDisplay =
{
    DDrawWindow_init,               /* init              */
    DDrawWindow_exit,               /* exit              */
    DDrawWindow_alloc_bitmap,       /* alloc_bitmap      */
    DDrawWindow_free_bitmap,        /* free_bitmap       */
    DDrawWindow_create_display,     /* create_display    */
    DDrawWindow_close_display,      /* close_display     */
    DDrawWindow_set_visible_area,   /* set_visible_area  */
    DDrawWindow_set_debugger_focus, /* set_debugger_focus*/
    DDrawWindow_allocate_colors,    /* allocate_colors   */
    DDrawWindow_modify_pen,         /* modify_pen        */
    DDrawWindow_get_pen,            /* get_pen           */
    DDrawWindow_mark_dirty,         /* mark_dirty        */
    0,                              /* skip_this_frame   */
    DDrawWindow_update_display,     /* update_display    */
    DDrawWindow_led_w,              /* led_w             */
    DDrawWindow_set_gamma,          /* set_gamma         */
    Display_get_gamma,              /* get_gamma         */
    DDrawWindow_set_brightness,     /* set_brightness    */
    Display_get_brightness,         /* get_brightness    */
    DDrawWindow_save_snapshot,      /* save_snapshot     */

    DDrawWindow_OnMessage,          /* OnMessage         */
    DDrawWindow_Refresh,            /* Refresh           */
    DDrawWindow_GetBlackPen,        /* GetBlackPen       */
    DDrawWindow_UpdateFPS,          /* UpdateFPS         */
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

extern int win32_debug;

static struct osd_bitmap*  pMAMEBitmap;

static Surface surface; /* keeps track of current surface data while locked */

static int scanlines;

/* keep track of how we move hMain when a game starts (to center it),
   to restore when done */
static int window_moved_x;
static int window_moved_y;

static int palette_offset; /* to work in window mode, must be 10 (because of 10 system colors),
                              when desktop is in 8 bit color depth */

static int vector_double; /* to double the recommended x & y */

static BOOL vdouble;         /* for 1:2 aspect ratio */

static int scale;

static CKPALETTE pal;
static CKPALETTE adjusted_pal;
static BOOLEAN   palette_created;
static BOOLEAN   palette_changed;

static int black_pen;
static int ui_pen;
static BOOL ui_pen_shared;

static int      bytes_per_pixel; /* if it's 1, then rest of this is ignored--it's palette */
static DWORD    *depth_remap; /* converting from an 8 bit pixel value to RGB to write to screen */
static int      depth_size;

static PIXELINFO infoRed, infoGrn, infoBlu;

static BOOL in_paint;

static LPDIRECTDRAWSURFACE front_buffer;  /* Screen Direct Draw object */
static LPDIRECTDRAWSURFACE back_buffer;   /* offscreen Direct Draw object */
static LPDIRECTDRAWCLIPPER ddclipper;     /* Clipper object; used to clip to window */
static LPDIRECTDRAWPALETTE ddpalette;

static int wndWidth, wndHeight;
static tRect visible_rect;

static BOOL use_dirty; /* just used to store the user's option, until we can set eDirtyMode */
enum DirtyMode eDirtyMode; /* what kind of dirty mode to use, if any */
static BOOL fast_8bit; /* whether to have main mame code render directly to back buffer */
static BOOL fast_16bit; /* whether to have lookup table based on next 2 bytes */

static BOOL bAviCapture;
static BOOL bAviRun;
static BOOL nAviShowMessage;

#define MAKECOL(r, g, b) \
    ((r & 0x1F) << (infoRed.m_nSize - 5) << infoRed.m_nShift | \
     (g & 0x1F) << (infoGrn.m_nSize - 5) << infoGrn.m_nShift | \
     (b & 0x1F) << (infoBlu.m_nSize - 5) << infoBlu.m_nShift)

/***************************************************************************
    External OSD function definitions
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if
    initialization was successful, nonzero otherwise.
*/
static int DDrawWindow_init(options_type *options)
{
    OSDDisplay.init(options);

    vector_double   = 0;
    vdouble         = FALSE;
    pMAMEBitmap     = NULL;
    scanlines       = FALSE;
    scale           = 2;
    palette_changed = 1;
    palette_created = FALSE;
    in_paint        = FALSE;
    depth_remap     = NULL;
    depth_size      = 0;
    front_buffer    = NULL;
    back_buffer     = NULL;
    ddpalette       = NULL;
    wndWidth        = 0;
    wndHeight       = 0;
    use_dirty       = FALSE;
    ui_pen_shared   = FALSE;

    vector_double = options->double_vector;
    scanlines = options->hscan_lines;
    scale = options->scale;
    use_dirty = options->use_dirty;

    memset(&infoRed, 0, sizeof(PIXELINFO));
    memset(&infoGrn, 0, sizeof(PIXELINFO));
    memset(&infoBlu, 0, sizeof(PIXELINFO));

    /* Check for inconsistent parameters. */
    if (scanlines == TRUE && scale == 1)
    {
        scanlines = FALSE;
    }

    /* Avi capture variables */
    bAviCapture     = GetAviCapture();
    bAviRun         = FALSE;
    nAviShowMessage = (bAviCapture) ? 10 : 0;

    return 0;
}

static void ReleaseDDrawObjects(void)
{
    if (dd != NULL)
    {
        if (ddpalette != NULL)
        {
            IDirectDrawPalette_Release(ddpalette);
            ddpalette = NULL;
        }

        if (ddclipper != NULL)
        {
            IDirectDrawClipper_Release(ddclipper);
            ddclipper = NULL;
        }

        if (back_buffer != NULL)
        {
            IDirectDrawSurface_Release(back_buffer);
            back_buffer = NULL;
        }

        if (front_buffer != NULL)
        {
            IDirectDrawSurface_Release(front_buffer);
            front_buffer = NULL;
        }
    }
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void DDrawWindow_exit(void)
{
    OSDDisplay.exit();
    ReleaseDDrawObjects();
}

static struct osd_bitmap* DDrawWindow_alloc_bitmap(int width, int height, int depth)
{
    assert(OSDDisplay.alloc_bitmap != 0);

    return OSDDisplay.alloc_bitmap(width, height, depth);
}

static void DDrawWindow_free_bitmap(struct osd_bitmap* pBitmap)
{
    assert(OSDDisplay.free_bitmap != 0);

    OSDDisplay.free_bitmap(pBitmap);
}

/*
    Create a display surface large enough to accomodate a bitmap
    of the given dimensions.
    Return a osd_bitmap pointer or 0 in case of error.
*/
static int DDrawWindow_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    int     i;
    RECT    rect;
    char    TitleBuf[256];

    if (dd == NULL)
        return 0;

    if (!DirectDrawSetupDrawing())
        return 0;

    if (use_dirty == TRUE)
    {
        if (attributes & VIDEO_SUPPORTS_DIRTY
        ||  attributes & VIDEO_TYPE_VECTOR)
        {
            eDirtyMode = USE_DIRTYRECT;
        }
        else
        {
            /* Don't use dirty if game doesn't support it. */
            eDirtyMode = NO_DIRTY;
            use_dirty = FALSE;
        }
    }
    else
    {
        eDirtyMode = NO_DIRTY;
    }

    if ((attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) == VIDEO_PIXEL_ASPECT_RATIO_1_2)
    {
        vdouble = TRUE;
        scale = 1;
        scanlines = FALSE;
    }

    /*
        Modify the main window to suit our needs.
    */
    SetWindowLong(MAME32App.m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME | WS_BORDER);

    sprintf(TitleBuf, "%s - %s", Machine->gamedrv->description, MAME32App.m_Name);
    SetWindowText(MAME32App.m_hWnd, TitleBuf);

    if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    {
        if (vector_double)
        {
            width  *= 2;
            height *= 2;
        }
        /* padding to a DWORD value */
        width  -= width  % 4;
        height -= height % 4;


        AdjustVisibleRect(0, 0, width - 1, height - 1);
    }
    else
    {
        AdjustVisibleRect(Machine->visible_area.min_x,
                          Machine->visible_area.min_y,
                          Machine->visible_area.max_x,
                          Machine->visible_area.max_y);
    }

    /* ErrorMsg("visible rect (%i,%i) %ix%i\n",visible_rect.m_Left,visible_rect.m_Top,
             visible_rect.m_Width,visible_rect.m_Height); */

    DirectDrawCalcDepth(&bytes_per_pixel, &infoRed, &infoGrn, &infoBlu);

    if (bytes_per_pixel == 1)
        palette_offset = 10;
    else
        palette_offset = 0;

    /* now allocate display memory stuff for mame core */

    if (bytes_per_pixel == 2)
        pMAMEBitmap = osd_alloc_bitmap(width,height,16);
    else
        pMAMEBitmap = osd_alloc_bitmap(width,height,8);

    /* swap x & y *after* allocating the bitmap */
    if (Machine->orientation & ORIENTATION_SWAP_XY)
    {
        int temp;

        temp   = width;
        width  = height;
        height = temp;
    }

    if (!pMAMEBitmap)
        return 0;

    /* now finish up window/screen stuff */

    if (!DirectDrawCreateSurfaces())
    {
        ErrorMsg("Error creating direct draw surfaces\n");
        return 0;
    }

    InitDirty(width,height,eDirtyMode);

    fast_8bit = (bytes_per_pixel == 1) && eDirtyMode == NO_DIRTY && vdouble == FALSE &&
       (visible_rect.m_Left == 0 && (int)visible_rect.m_Width == width &&
        visible_rect.m_Top == 0 && (int)visible_rect.m_Height == height);

    fast_16bit = (bytes_per_pixel == 2);
    /* && !(Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE); */

#if 0
    printf("dirty mode = %s\n",eDirtyMode == NO_DIRTY ? "none" : "dirty rect");
    printf("fast 8bit = %i\n",fast_8bit);
    printf("fast 16bit = %i\n",fast_16bit);
#endif

    if (fast_8bit)
    {
        /* in 256 color case, have the game code write into our direct draw back buffer,
        so we just blit/flip it each frame */
        if (!DirectDrawLock(FALSE,&surface))
        {
            ErrorMsg("Error locking\n");
            return 0;
        }
        for (i = 0; i < pMAMEBitmap->height; i++)
            pMAMEBitmap->line[i] = surface.bits + i * surface.pitch;
    }

    if (!DirectDrawClipToWindow())
        ErrorMsg("Error creating direct draw clipper on a window--trying to proceed without it.\n");

    /* resize the top level window */

    StatusCreate();

    wndWidth  = (visible_rect.m_Width) * scale + 2 * GetSystemMetrics(SM_CXFIXEDFRAME);
    wndHeight = (visible_rect.m_Height) * scale * (vdouble ? 2 : 1) +
       2 * GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION) + GetStatusHeight();

    GetWindowRect(MAME32App.m_hwndUI, &rect);

    /* move the window to center it over the gui location */
    window_moved_x = ((rect.right  - rect.left) - wndWidth)  / 2;
    window_moved_y = ((rect.bottom - rect.top)  - wndHeight) / 2;

    if ((rect.left + window_moved_x) < 0)
        window_moved_x = -rect.left;
    if ((rect.top + window_moved_y) < 0)
        window_moved_y = -rect.top;

    SetWindowPos(MAME32App.m_hWnd, 0,
                 rect.left + window_moved_x,
                 rect.top  + window_moved_y,
                 wndWidth,
                 wndHeight, SWP_NOZORDER | SWP_SHOWWINDOW);

    SetForegroundWindow(MAME32App.m_hWnd);

    set_ui_visarea(visible_rect.m_Left,visible_rect.m_Top,
                   visible_rect.m_Left + visible_rect.m_Width  - 1,
                   visible_rect.m_Top  + visible_rect.m_Height - 1);

    return 1;
}

/*
     Shut down the display
*/

static void DDrawWindow_close_display(void)
{
   ExitDirty();

    StatusDelete();

    osd_free_bitmap(pMAMEBitmap);

    if (depth_remap != NULL)
        free(depth_remap);
    depth_remap = NULL;
}

static void DDrawWindow_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{

}

static void DDrawWindow_set_debugger_focus(int debugger_has_focus)
{

}

static int DDrawWindow_allocate_colors(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens)
{
    unsigned int    i;

    if (pMAMEBitmap->depth == 16)
    {
       int r,g,b;

       for (r = 0; r < 32; r++)
           for (g = 0; g < 32; g++)
               for (b = 0; b < 32; b++)
               {
                    int r1, g1, b1;

                    r1 = (int)(31.0 * Display_get_brightness() * pow(r / 31.0, 1.0 / Display_get_gamma()) / 100.0);
                    g1 = (int)(31.0 * Display_get_brightness() * pow(g / 31.0, 1.0 / Display_get_gamma()) / 100.0);
                    b1 = (int)(31.0 * Display_get_brightness() * pow(b / 31.0, 1.0 / Display_get_gamma()) / 100.0);

                    *pens++ = MAKECOL(r1, g1, b1);
               }
        Machine->uifont->colortable[0] = 0;
        Machine->uifont->colortable[1] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[2] = MAKECOL(0xFF, 0xFF, 0xFF);
        Machine->uifont->colortable[3] = 0;

    }
    else
    {
        int final_colors = totalcolors;

        if (totalcolors >= 256)
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

                r = palette[3 * i];
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

            /* share color pens[bestwhite] for the user interface text */
            ui_pen_shared = TRUE;
            ui_pen = (pens[bestwhite] + palette_offset) % 256;
            Machine->uifont->colortable[0] = (pens[bestblack] + palette_offset) % 256;
            Machine->uifont->colortable[1] = ui_pen;
            Machine->uifont->colortable[2] = ui_pen;
            Machine->uifont->colortable[3] = (pens[bestblack] + palette_offset) % 256;
        }
        else
        {
            /* reserve color totalcolors for the user interface text */

            ui_pen = (totalcolors + palette_offset) % 256;
            pal.entries[totalcolors].peRed = 0xff;
            pal.entries[totalcolors].peGreen = 0xff;
            pal.entries[totalcolors].peBlue = 0xff;
            pal.entries[totalcolors].peFlags = PC_NOCOLLAPSE;

            Machine->uifont->colortable[0] = 0;
            Machine->uifont->colortable[1] = ui_pen;
            Machine->uifont->colortable[2] = ui_pen;
            Machine->uifont->colortable[3] = 0;

            final_colors++;
        }

        for (i = 0;i < totalcolors;i++)
        {
            pal.entries[i].peRed   = palette[3*i];
            pal.entries[i].peGreen = palette[3*i+1];
            pal.entries[i].peBlue  = palette[3*i+2];
            pal.entries[i].peFlags = PC_NOCOLLAPSE;
        }

        for (i = 0;i < (unsigned int)final_colors;i++)
        {
            memcpy(&adjusted_pal.entries[i], &pal.entries[i], sizeof(PALETTEENTRY));
            Display_MapColor(&adjusted_pal.entries[i].peRed,
                             &adjusted_pal.entries[i].peGreen,
                             &adjusted_pal.entries[i].peBlue);

            pens[i] = (palette_offset + i) % 256;
            if (bytes_per_pixel == 1)
            {
                if (pens[i] < 10 || pens[i] > 245)
                {
                    /* we don't set the windows colors, so we need to remap to nearest other color */
                    pens[i] = FindNearestColor(10,min(245,10+totalcolors),pal.entries[i].peRed,
                                               pal.entries[i].peGreen,pal.entries[i].peBlue);
                    /*
                    ErrorMsg("Looking for %i (%i,%i,%i), got %i (%i,%i,%i)",
                             (palette_offset + i) % 256,pal.entries[i].peRed,pal.entries[i].peGreen,
                             pal.entries[i].peBlue,pens[i],pal.entries[pens[i]].peRed,
                             pal.entries[pens[i]].peGreen,pal.entries[pens[i]].peBlue);
                             */
                }
            }
        }
    }

    osd_win32_create_palette();

    black_pen = FindBlackPen();

    return 0;
}

/*
    Change the color of the pen.
*/
static void DDrawWindow_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    if (pMAMEBitmap->depth == 16)
    {
       ErrorMsg("Shouldn't modify pen in 16 bit color depth game mode!\n");
    }

    pen -= palette_offset;
    if (pen < 0)
        pen += 256;

    /* ignore useless writes--they happen a lot! */
    if (pal.entries[pen].peRed == red && pal.entries[pen].peGreen == green &&
        pal.entries[pen].peBlue == blue)
        return;

    /*
        save up the change to the palette, then change the palette at most
        once per frame.  Speeds things up that way when lots of colors are changed.  CMK
    */
    pal.entries[pen].peRed   = red;
    pal.entries[pen].peGreen = green;
    pal.entries[pen].peBlue  = blue;
    pal.entries[pen].peFlags = PC_NOCOLLAPSE;

    memcpy(&adjusted_pal.entries[pen], &pal.entries[pen], sizeof(PALETTEENTRY));
    Display_MapColor(&adjusted_pal.entries[pen].peRed,
                     &adjusted_pal.entries[pen].peGreen,
                     &adjusted_pal.entries[pen].peBlue);

    osd_win32_change_color(pen);
}

/*
    Get the color of a pen.
*/
static void DDrawWindow_get_pen(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue)
{
    if (pMAMEBitmap->depth == 8)
    {
        pen -= palette_offset;
        if (pen < 0)
            pen += 256;

        if (OSD_NUMPENS <= pen)
            pen = 0;

        *pRed   = pal.entries[pen].peRed;
        *pGreen = pal.entries[pen].peGreen;
        *pBlue  = pal.entries[pen].peBlue;
    }
    else
    {
        PIXELINFO * r = &infoRed;
        PIXELINFO * g = &infoGrn;
        PIXELINFO * b = &infoBlu;

        *pRed   = ((pen & r->m_dwMask) >> r->m_nShift) << (8 - r->m_nSize);
        *pGreen = ((pen & g->m_dwMask) >> g->m_nShift) << (8 - g->m_nSize);
        *pBlue  = ((pen & b->m_dwMask) >> b->m_nShift) << (8 - b->m_nSize);
    }
}

static void DDrawWindow_mark_dirty(int x1, int y1, int x2, int y2)
{
   /*printf("%3i,%3i to %3i,%3i\n",x1,y1,x2,y2); */

    if (!fast_8bit)
        MarkDirty(x1,y1,x2,y2);
}

/*
    Update the display.
*/
static void DDrawWindow_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap)
{
    DrawGame();

    StatusUpdate();
}

/* control keyboard leds or other indicators */
static void DDrawWindow_led_w(int leds_status)
{
    StatusWrite(leds_status);
}

static void DDrawWindow_set_gamma(float gamma)
{
    OSDDisplay.set_gamma(gamma);

    AdjustPalette();
}

static void DDrawWindow_set_brightness(int brightness)
{
    OSDDisplay.set_brightness(brightness);

    AdjustPalette();
}

static void DDrawWindow_save_snapshot(struct osd_bitmap *bitmap)
{
    if (bAviCapture)
    {
        char buf[40];

        bAviRun = !bAviRun;     /* toggle capture on/off */
        sprintf(buf, "AVI Capture %s", (bAviRun) ? "ON" : "OFF");
        StatusSetString(buf);
    }
    else
        Display_WriteBitmap(bitmap, pal.entries);
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void CalcClippedRect(RECT *rect)
{
    RECT scr_rect;
    POINT p;
    int height = pMAMEBitmap->height;
    int width = pMAMEBitmap->width;

    p.x = p.y = 0;
    ClientToScreen(MAME32App.m_hWnd, &p);
    if (p.x >= 0)
        p.x = 0;
    else
        p.x = -p.x;

    if (p.y >= 0)
        p.y = 0;
    else
        p.y = -p.y;

    rect->left = p.x;
    rect->top = p.y;

    GetWindowRect(GetDesktopWindow(),&scr_rect);

    p.x = width;
    p.y = height;

    ClientToScreen(MAME32App.m_hWnd, &p);

    if (p.x > scr_rect.right)
        rect->right = width - (p.x-scr_rect.right);
    else
        rect->right = width;


    if (p.y > scr_rect.bottom)
        rect->bottom = height - (p.y-scr_rect.bottom);
    else
        rect->bottom = height;
}

static void DrawGame()
{
    int i, start_x, start_y, length_x, length_y;
    RECT rect;
    enum DirtyMode dirty_this_frame;
    BOOL draw_foreground;

    dirty_this_frame = eDirtyMode;
    if (in_paint || palette_changed)
       dirty_this_frame = NO_DIRTY;

    if (palette_changed)
    {
        if (bytes_per_pixel == 1)
            DirectDrawSetPaletteColors(adjusted_pal.entries);
        for (i = 0; i < OSD_NUMPENS; i++)
           osd_win32_change_color(i);
    }
    palette_changed = FALSE;

    /* ErrorMsg("Updating display\n"); */

    if (fast_8bit)
    {
        /* game already wrote the data to the back buffer, and we're holding it locked. */

        if (in_paint)
        {
            DirectDrawUnlock(FALSE,&surface); /* can fail depending on when called; that's ok */
            DirectDrawUpdateScreen();
            if (DirectDrawLock(FALSE,&surface)) /* can fail depending on when called; that's ok */
            {
                for (i = 0;i < pMAMEBitmap->height; i++)
                    pMAMEBitmap->line[i] = surface.bits + i * surface.pitch;
            }
        }
        else
        {
            if (!DirectDrawUnlock(FALSE,&surface))
            {
                ErrorMsg("error unlocking\n");
                return;
            }

            DirectDrawUpdateScreen();

            if (DirectDrawLock(FALSE,&surface))
            {
                for (i = 0; i < pMAMEBitmap->height; i++)
                    pMAMEBitmap->line[i] = surface.bits + i * surface.pitch;
            }
            MAME32App.ProcessMessages();

        }
        return;
    }

    MAME32App.ProcessMessages();
    /*if (MAME32App.m_bIsPaused && !in_paint) */
    /*    return; */

    draw_foreground = (!in_paint) && !IsWindowObscured() && ((scale == 1) || scanlines || vdouble);
    /* lock our back buffersurface */
    if (!DirectDrawLock(draw_foreground,&surface))
        return;

    if (draw_foreground)
    {
        CalcClippedRect(&rect);

        start_x = rect.left;
        start_y = rect.top;
        length_y = rect.bottom - rect.top;
        length_x = rect.right - rect.left;
    }
    else
    {
        /* drawing to background buffer, and the blit will clip to our window */
        start_x = 0;
        start_y = 0;
        length_y = pMAMEBitmap->height;
        length_x = pMAMEBitmap->width;
    }

    /* adjust to only draw visible area */
    if (start_x < (int)visible_rect.m_Left)
    {
       length_x -= (visible_rect.m_Left - start_x);
       start_x = visible_rect.m_Left;
    }
    if (start_x + length_x > (int)(visible_rect.m_Left + visible_rect.m_Width))
    {
       length_x = visible_rect.m_Left + visible_rect.m_Width - start_x;
    }
    if (start_y < (int)visible_rect.m_Top)
    {
       length_y -= (visible_rect.m_Top - start_y);
       start_y = visible_rect.m_Top;
    }
    if (start_y + length_y > (int)(visible_rect.m_Top + visible_rect.m_Height))
    {
       length_y = visible_rect.m_Top + visible_rect.m_Height - start_y;
    }

    if (bAviRun)
        AviAddBitmap(pMAMEBitmap, pal.entries);
    else if (nAviShowMessage > 0)
    {
        char buf[40];

        nAviShowMessage--;

        sprintf(buf, "AVI Capture OFF");
        StatusSetString(buf);
    }

    switch (bytes_per_pixel)
    {
    case 1:
        /* this case usually handled faster up above--here's the slow double buffer way */
        if (scanlines == FALSE)
        {
            if (vdouble)
                Render8DepthVDouble(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
            else
                Render8Depth(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
        }
        else
            Render8DepthDouble(surface,start_x,start_y,length_x,length_y,dirty_this_frame);

        break;

    case 2:
        if (pMAMEBitmap->depth == 16)
        {
            Render16to16Depth(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
            break;
        }
        if (fast_16bit)
        {
            if (vdouble)
                Render16DepthVDouble(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
            else
                Render16Depth(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
            break;
        }

        /* if not fast_16bit, fall through to generic... */
    default:
        if (vdouble)
            RenderGenericDepthVDouble(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
        else
            RenderGenericDepth(surface,start_x,start_y,length_x,length_y,dirty_this_frame);
        break;
    }

    /* Unlock surface */
    if (!DirectDrawUnlock(draw_foreground,&surface))
    {
        ErrorMsg("Failed to unlock surface!\n");
        return;
    }

    if (!draw_foreground)
        DirectDrawUpdateScreen();
}

/* renders into surface, assumes 8 bit color depth */

static void Render8Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                         enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;

    for (i = start_y; i < start_y+length_y; i++)
    {
        ptr = surface.bits + ((i-start_y)*surface.pitch);
        source_ptr = pMAMEBitmap->line[i] + start_x;

        j = start_x;

        if (dirty_this_frame == USE_DIRTYRECT)
        {
            while (j < start_x + length_x)
            {
                if (((j % 32) == 0) && !IsDirtyDword(j,i))
                {
                    j += 32;
                    ptr += 32;
                    source_ptr += 32;
                }
                else
                {
                    if (IsDirty(j,i))
                        *ptr = *source_ptr;

                    j++;
                    ptr++;
                    source_ptr++;
                }
            }
        }
        else
        {
            memcpy(ptr,source_ptr,length_x);
        }
    }
}

/* renders into surface, assumes 8 bit color depth, draws each horizontal line twice */
static void Render8DepthVDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                                enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;
    int row; /* 0 or 1 for first or second of doubled rows */

    for (i = start_y; i < start_y+length_y; i++)
    {
        row = 0;
        while (row < 2)
        {
            ptr = surface.bits + ((2*(i-start_y) + row)*surface.pitch);
            source_ptr = pMAMEBitmap->line[i] + start_x;

            j = start_x;

            if (dirty_this_frame == USE_DIRTYRECT)
            {
                while (j < start_x + length_x)
                {
                    if (((j % 32) == 0) && !IsDirtyDword(j,i))
                    {
                        j += 32;
                        ptr += 32;
                        source_ptr += 32;
                    }
                    else
                    {
                        if (IsDirty(j,i))
                            *ptr = *source_ptr;

                        j++;
                        ptr++;
                        source_ptr++;
                    }
                }
            }
            else
            {
                memcpy(ptr,source_ptr,length_x);
            }

            row++;
        }
    }
}

/* derived from Render8Depth */
static void Render8DepthDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                               enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;

    int row; /* 0 or 1 for first or second of doubled rows */
    BYTE b;

    for (i = start_y; i < start_y+length_y; i++)
    {
        row = 0;
        while (row < 2)
        {
            ptr = surface.bits + ((2*(i-start_y) + row)*surface.pitch);
            source_ptr = pMAMEBitmap->line[i] + start_x;

            j = start_x;

            if (dirty_this_frame == USE_DIRTYRECT)
            {
                while (j < start_x + length_x)
                {
                    if (((j % 32) == 0) && !IsDirtyDword(j,i))
                    {
                        j += 32;
                        ptr += 64;
                        source_ptr += 32;
                    }
                    else
                    {
                        if (IsDirty(j,i))
                        {
                            b = *source_ptr;
                            *(WORD *)ptr = (b | (((WORD)b) << 8));
                        }

                        j++;
                        ptr += 2;
                        source_ptr++;
                    }
                }
            }
            else
            {
                while (j < start_x + length_x)
                {
                    b = *source_ptr;
                    *(WORD *)ptr = (b | (((WORD)b) << 8));
                    j++;
                    ptr += 2;
                    source_ptr++;
                }
            }
            row++;
            if (scanlines)
                break;
        }
    }


}

/* renders into surface, assumes 16 bit color depth */
static void Render16Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                          enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;

    /* STILL TO ADD--support for pixel doubling, scanlines */
    for (i = start_y; i < start_y+length_y; i++)
    {
        ptr = surface.bits + ((i-start_y)*surface.pitch);
        source_ptr = pMAMEBitmap->line[i] + start_x;

        j = start_x;


        /* MAIN LOOP PER ROW */
        if (dirty_this_frame == USE_DIRTYRECT)
        {
            while (j < start_x + length_x - 1)
            {
                if (((j % 32) == 0) && !IsDirtyDword(j,i))
                {
                    j += 32;
                    ptr += 64;
                    source_ptr += 32;
                }
                else
                {
                    if (IsDirty(j,i) || IsDirty(j+1,i))
                    {
                        /* lookup based on next TWO bytes */
                        *(DWORD *)ptr = depth_remap[*(WORD *)source_ptr];
                    }
                    j += 2;
                    ptr += 4;
                    source_ptr += 2;
                }
            }

        }
        else
        {
            for (; j < start_x + length_x - 1; j += 2)
            {
                /* lookup based on next TWO bytes */
                *(DWORD *)ptr = depth_remap[*(WORD *)source_ptr];
                ptr += 4;
                source_ptr += 2;
            }
        }

        /* last pixel if wasn't aligned */
        if (j == start_x + length_x - 1)
        {
            if (dirty_this_frame == NO_DIRTY ||
                (dirty_this_frame == USE_DIRTYRECT && IsDirty(j,i)))
            {
                *(WORD *)ptr = (WORD)depth_remap[*source_ptr];
            }
        }

    }
}

/* renders into surface, assumes 16 bit color depth, draws each horizontal line twice */
static void Render16DepthVDouble(Surface surface,int start_x,int start_y,int length_x,int length_y,
                                 enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;
    int row; /* 0 or 1 for first or second of doubled rows */

    /* STILL TO ADD--support for pixel doubling, scanlines */
    for (i = start_y; i < start_y+length_y; i++)
    {
       row = 0;
       while (row < 2)
       {
            ptr = surface.bits + ((2*(i-start_y) + row)*surface.pitch);
            source_ptr = pMAMEBitmap->line[i] + start_x;

            j = start_x;


            /* MAIN LOOP PER ROW */
            if (dirty_this_frame == USE_DIRTYRECT)
            {
               while (j < start_x + length_x - 1)
               {
                  if (((j % 32) == 0) && !IsDirtyDword(j,i))
                  {
                     j += 32;
                     ptr += 64;
                     source_ptr += 32;
                  }
                  else
                  {
                     if (IsDirty(j,i) || IsDirty(j+1,i))
                     {
                        /* lookup based on next TWO bytes */
                        *(DWORD *)ptr = depth_remap[*(WORD *)source_ptr];
                     }
                     j += 2;
                     ptr += 4;
                     source_ptr += 2;
                  }
               }

            }
            else
            {
               for (; j < start_x + length_x - 1; j += 2)
               {
                  /* lookup based on next TWO bytes */
                  *(DWORD *)ptr = depth_remap[*(WORD *)source_ptr];
                  ptr += 4;
                  source_ptr += 2;
               }
            }

            /* last pixel if wasn't aligned */
            if (j == start_x + length_x - 1)
            {
               if (dirty_this_frame == NO_DIRTY ||
                   (dirty_this_frame == USE_DIRTYRECT && IsDirty(j,i)))
               {
                  *(WORD *)ptr = (WORD)depth_remap[*source_ptr];
               }
            }
            row++;
       }

    }
}

/* renders 16 bit bitmap onto 16 bit surface */
static void Render16to16Depth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                              enum DirtyMode dirty_this_frame)
{
    int i;
    BYTE *ptr;

    for (i = start_y; i < start_y+length_y; i++)
    {
        ptr = surface.bits + ((i-start_y)*surface.pitch);

        memcpy(ptr, pMAMEBitmap->line[i] + start_x*2, length_x*2);
    }


}

/* renders into surface, assumes nothing about color depth */
static void RenderGenericDepth(Surface surface,int start_x,int start_y,int length_x,int length_y,
                               enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;

    /* 24/32/?? bit color */

    /* STILL TO ADD--support for pixel doubling, scanlines */
    for (i = start_y; i < start_y+length_y; i++)
    {
        ptr = surface.bits + ((i-start_y)*surface.pitch);
        source_ptr = pMAMEBitmap->line[i] + start_x;

        j = start_x;

        if (dirty_this_frame == USE_DIRTYRECT)
        {
            while (j < start_x + length_x)
            {
                if (((j % 32) == 0) && !IsDirtyDword(j,i))
                {
                    j += 32;
                    ptr += 32*bytes_per_pixel;
                    source_ptr += 32;
                }
                else
                {
                    if (IsDirty(j,i))
                        memcpy(ptr,&depth_remap[*source_ptr], bytes_per_pixel);

                    j++;
                    ptr += bytes_per_pixel;
                    source_ptr ++;
                }
            }
        }
        else
        {
            while (j < start_x + length_x)
            {
                memcpy(ptr,&depth_remap[*source_ptr], bytes_per_pixel);
                j++;
                ptr += bytes_per_pixel;
                source_ptr++;
            }
        }
    }
}

static void RenderGenericDepthVDouble(Surface surface,int start_x,int start_y,int length_x,
                                      int length_y,enum DirtyMode dirty_this_frame)
{
    int i,j;
    BYTE *ptr,*source_ptr;

    /* 24/32/?? bit color */
    int row; /* 0 or 1 for first or second of doubled rows */

    /* STILL TO ADD--support for pixel doubling, scanlines */
    for (i = start_y; i < start_y+length_y; i++)
    {
       row = 0;
       while (row < 2)
       {
            ptr = surface.bits + ((2*(i-start_y) + row)*surface.pitch);
            source_ptr = pMAMEBitmap->line[i] + start_x;

            j = start_x;

            if (dirty_this_frame == USE_DIRTYRECT)
            {
                while (j < start_x + length_x)
                {
                    if (((j % 32) == 0) && !IsDirtyDword(j,i))
                    {
                        j += 32;
                        ptr += 32*bytes_per_pixel;
                         source_ptr += 32;
                    }
                    else
                    {
                        if (IsDirty(j,i))
                            memcpy(ptr,&depth_remap[*source_ptr], bytes_per_pixel);

                        j++;
                        ptr += bytes_per_pixel;
                        source_ptr ++;
                    }
                }
            }
            else
            {
                while (j < start_x + length_x)
                {
                    memcpy(ptr,&depth_remap[*source_ptr], bytes_per_pixel);
                    j++;
                    ptr += bytes_per_pixel;
                    source_ptr++;
                }
            }
            row++;
       }
    }
}

static BOOL IsWindowObscured(void)
{
    BOOL is_obscured;
    HWND hwnd;
    RECT rect,rect2,rect3;
    POINT p;

    is_obscured = FALSE;

    p.x = 0;
    p.y = 0;
    ClientToScreen(MAME32App.m_hWnd, &p);
    rect.left = p.x;
    rect.top = p.y;

    p.x = pMAMEBitmap->width;
    p.y = pMAMEBitmap->height;
    ClientToScreen(MAME32App.m_hWnd, &p);
    rect.right = p.x;
    rect.bottom = p.y;

    hwnd = MAME32App.m_hWnd;
    while (1)
    {
        hwnd = GetNextWindow(hwnd,GW_HWNDPREV);
        if (hwnd == NULL)
            break;

        if (IsWindowVisible(hwnd))
        {
            GetWindowRect(hwnd,&rect2);

            if (IntersectRect(&rect3,&rect,&rect2))
            {
                /*
                char buf[100];
                GetWindowText(hwnd,buf,sizeof(buf)-1);
                printf("obscured by %08x %i %i %i %i %s\n",hwnd,rect2.left,rect2.top,rect2.right,
                rect2.bottom,buf);
                */
                is_obscured = TRUE;
                break;
            }
        }
    }
    return is_obscured;
}

static void osd_win32_create_palette()
{
    int i,j;

    DirectDrawCreatePalette(&pal);
    DirectDrawSetPalette();

    switch (bytes_per_pixel)
    {
    case 1 :
        break;
    case 2 :
        if (fast_16bit)
        {
            /* making a TWO byte lookup table.  (256K bytes) Crazy, but at
               render time allows us to lookup 2 bytes, write one dword.  Fast */
            depth_size = 256 * 256;
            depth_remap = (DWORD *)malloc(depth_size * (sizeof(DWORD)));

            for (i = 0; i < 256; i++)
                for (j = 0; j < 256; j++)
                {
                    CalcDepthRemapColor(&depth_remap[256 * i + j], j);
                    CalcDepthRemapColor((DWORD *)((char *)&depth_remap[256 * i + j] + 2), i);
                }
            break;
        }

       /* if not fast 16 bit, fall through to generic... */
    default :
        depth_size = 256;
        depth_remap = (DWORD *)malloc(depth_size * (sizeof(DWORD)));

        for (i = 0; i < depth_size; i++)
            CalcDepthRemapColor(&depth_remap[i], i);

        break;
    }
}

static void __inline__ CalcDepthRemapColor(DWORD *rgb, int index)
{
    DWORD color =
        (((DWORD)(adjusted_pal.entries[index].peRed   >> (8 - infoRed.m_nSize))) << infoRed.m_nShift) |
        (((DWORD)(adjusted_pal.entries[index].peGreen >> (8 - infoGrn.m_nSize))) << infoGrn.m_nShift) |
        (((DWORD)(adjusted_pal.entries[index].peBlue  >> (8 - infoBlu.m_nSize))) << infoBlu.m_nShift);

    memcpy(rgb, &color, bytes_per_pixel);

    /*
    ErrorMsg("%04x %i %i %i \n",depth_remap[index],pal.entries[index].peRed,
       pal.entries[index].peGreen,pal.entries[index].peBlue);
       */
}

static void osd_win32_change_color(int index)
{
    int i;

    palette_changed = TRUE;

    switch (bytes_per_pixel)
    {
    case 1 :
        /* if 8 bit color, change the hardware palette .
        don't need to do any color depth remapping */
        break;

    case 2 :
        if (fast_16bit)
        {
            /* change all entries with index as the second pixel */
            for (i = 0; i < 256; i++)
                CalcDepthRemapColor((DWORD *)((char *)&depth_remap[256 * i + index]), index);

            /* change all entries with index as the first pixel */
            for (i = 0; i < 256; i++)
                CalcDepthRemapColor((DWORD *)((char *)&depth_remap[256 * index + i] + 2), index);
            break;
        }

       /* if not fast_16bit, fall through to generic... */
    default :
        CalcDepthRemapColor(&depth_remap[index], index);
        break;
    }
}

static void DDrawWindow_Refresh()
{
    InvalidateRect(MAME32App.m_hWnd, NULL, FALSE);
}

static int DDrawWindow_GetBlackPen(void)
{
    return black_pen;
}

static void DDrawWindow_UpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS)
{
    StatusUpdateFPS(bShow, nSpeed, nFPS, nMachineFPS, nFrameskip, nVecUPS);
}

/***************************************************************************
    Message handlers
 ***************************************************************************/

static BOOL DDrawWindow_OnMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (Msg)
    {
        PEEK_MESSAGE(hWnd, WM_ACTIVATEAPP,          OnActivateApp);
        HANDLE_MESSAGE(hWnd, WM_GETMINMAXINFO,      OnGetMinMaxInfo);
        HANDLE_MESSAGE(hWnd, WM_PAINT,              OnPaint);
        HANDLE_MESSAGE(hWnd, WM_SIZE,               OnSize);
        HANDLE_MESSAGE(hWnd, WM_DRAWITEM,           OnDrawItem);
        HANDLE_MESSAGE(hWnd, WM_LBUTTONDOWN,        OnLButtonDown);
    }
    return FALSE;
}

static void OnActivateApp(HWND hWnd, BOOL fActivate, DWORD dwThreadId)
{
    if (fActivate && MAME32App.m_hWnd == hWnd)
    {
        if (palette_created)
            DirectDrawSetPalette();
    }
}

static void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO* pMinMaxInfo)
{
    pMinMaxInfo->ptMaxSize.x      = wndWidth;
    pMinMaxInfo->ptMaxSize.y      = wndHeight;
    pMinMaxInfo->ptMaxTrackSize.x = wndWidth;
    pMinMaxInfo->ptMaxTrackSize.y = wndHeight;
    pMinMaxInfo->ptMinTrackSize.x = wndWidth;
    pMinMaxInfo->ptMinTrackSize.y = wndHeight;
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC         hdc;

    in_paint = TRUE;

    hdc = BeginPaint(hWnd, &ps);

    DrawGame();

    EndPaint(hWnd, &ps);
    in_paint = FALSE;
}

static void OnSize(HWND hwnd,UINT state,int cx,int cy)
{
    StatusWindowSize(state,cx,cy);
}

static void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem)
{
    StatusDrawItem(lpDrawItem);
}

static void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
   /* ErrorMsg("at %i,%i color %i\n",x,y,pMAMEBitmap->line[y][x]); */
}


/***************************************************************************
    DirectDraw functions
 ***************************************************************************/

static BOOL DirectDrawSetupDrawing()
{
    HRESULT ddrval;

    ddrval = IDirectDraw2_SetCooperativeLevel(dd, MAME32App.m_hWnd, DDSCL_NORMAL);

    if (ddrval != DD_OK)
    {
        ErrorMsg("SetCooperativeLevel failed!\n");
        return FALSE;
    }

    /* Create clipper, for use when not screen */
    ddrval = IDirectDraw2_CreateClipper(dd, 0, &ddclipper, NULL);
    if (ddrval != DD_OK)
    {
        ErrorMsg("CreateClipper failed!\n");
        return FALSE;
    }

    return TRUE;
}

/* for non-8 bit color depth, return the layout of video memory */

static void DirectDrawCalcDepth(int *bytes_per_pixel, PIXELINFO *r,
                                PIXELINFO *g, PIXELINFO *b)
{
    DDSURFACEDESC format;
    HRESULT ddrval;

    assert(dd != NULL);

    format.dwSize = sizeof(format);

    ddrval = IDirectDraw2_GetDisplayMode(dd, &format);
    if (ddrval != DD_OK)
    {
        ErrorMsg("Error in directdrawsetup %i %i %i %i \n", ddrval,
                DDERR_INVALIDOBJECT,
                DDERR_INVALIDPARAMS,
                DDERR_UNSUPPORTEDMODE);
        *bytes_per_pixel = 1;
        return;
    }

    if ((format.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) ||
        (format.ddpfPixelFormat.dwRBitMask == 0 &&
         format.ddpfPixelFormat.dwGBitMask == 0 &&
         format.ddpfPixelFormat.dwBBitMask == 0))
    {
        /* ErrorMsg("8 bit color depth ok\n"); */
        *bytes_per_pixel = 1;
    }
    else
    {
        unsigned long m;
        int s;

        r->m_dwMask = format.ddpfPixelFormat.dwRBitMask;
        g->m_dwMask = format.ddpfPixelFormat.dwGBitMask;
        b->m_dwMask = format.ddpfPixelFormat.dwBBitMask;

        /*
         * Determine the red, green and blue masks' shift and bit count
         */
        for (s = 0, m = r->m_dwMask; !(m & 1); s++, m >>= 1);
        r->m_nShift = s;

        for (s = 0; (r->m_dwMask >> r->m_nShift) & (1 << s); s++);
        r->m_nSize = s;

        for (s = 0, m = g->m_dwMask; !(m & 1); s++, m >>= 1);
        g->m_nShift = s;

        for (s = 0; (g->m_dwMask >> g->m_nShift) & (1 << s) ; s++);
        g->m_nSize = s;

        for (s = 0, m = b->m_dwMask; !(m & 1); s++, m >>= 1);
        b->m_nShift = s;

        for (s = 0; (b->m_dwMask >> b->m_nShift) & (1 << s) ; s++);
        b->m_nSize = s;

        *bytes_per_pixel = format.ddpfPixelFormat.dwRGBBitCount / 8;

        /*
        ErrorMsg("Red   Mask 0x%08x - Shift(%i), Bits(%i)\n", r->m_dwMask, r->m_nShift, r->m_nSize);
        ErrorMsg("Green Mask 0x%08x - Shift(%i), Bits(%i)\n", g->m_dwMask, g->m_nShift, g->m_nSize);
        ErrorMsg("Blue  Mask 0x%08x - Shift(%i), Bits(%i)\n", b->m_dwMask, b->m_nShift, b->m_nSize);

        ErrorMsg("bits per pixel %i\n",format.ddpfPixelFormat.dwRGBBitCount);
        */
    }
}

/*
 * DirectDrawCreateSurfaces:  Create main screen buffer
 *   Returns TRUE on success.
 */
static BOOL DirectDrawCreateSurfaces()
{
    DDSURFACEDESC   ddsd;
    HRESULT         ddrval;

    assert(dd != NULL);

    /* Create primary surface */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddrval = IDirectDraw2_CreateSurface(dd, &ddsd, &front_buffer, NULL);
    if (ddrval != DD_OK)
    {
        ErrorMsg("CreateSurface failed to create front buffer, error = %i\n", ddrval);
        return FALSE;
    }

    /* Create a separate back buffer to blit with */

    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

    ddsd.dwWidth  = pMAMEBitmap->width * bytes_per_pixel * scale;
    ddsd.dwHeight = pMAMEBitmap->height * scale * (vdouble ? 2 : 1);

    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

    ddrval = IDirectDraw2_CreateSurface(dd, &ddsd, &back_buffer, NULL);
    if (ddrval != DD_OK)
    {
        ErrorMsg("CreateSurface failed to create back buffer, error = %i\n", ddrval);
        return FALSE;
    }

    return TRUE;
}

static BOOL DirectDrawLock(BOOL front,Surface *s)
{
    DDSURFACEDESC ddsd;
    HRESULT ddrval;

    LPDIRECTDRAWSURFACE buffer; /* could potentially be either front_buffer or back_buffer */

    if (front)
       buffer = front_buffer;
    else
       buffer = back_buffer;

    ddsd.dwSize = sizeof(ddsd);
    while (1)
    {
        ddrval = IDirectDrawSurface2_Lock(buffer, NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (ddrval == DD_OK)
        break;

        if (ddrval == DDERR_SURFACELOST)
        {
            ddrval = IDirectDrawSurface2_Restore(buffer);
            if (ddrval != DD_OK)
            {
                ErrorMsg("DirectDrawRestoreSurfaces failed to restore surfaces\n");
                return FALSE;
            }
            continue;
        }
        /*
        ErrorMsg("Error locking %iXX %i %i %i %i %i %i\n", ddrval,
          DDERR_INVALIDOBJECT,DDERR_INVALIDPARAMS,DDERR_OUTOFMEMORY,
          DDERR_SURFACEBUSY,DDERR_SURFACELOST,DDERR_WASSTILLDRAWING);
          */
        return FALSE;
    }

    s->height = ddsd.dwHeight;
    s->width  = ddsd.dwWidth;
    s->base_bits = ddsd.lpSurface;
    s->bits   = ddsd.lpSurface;
    s->pitch  = ddsd.lPitch;

    if (front)
    {
        POINT p;

        p.x = p.y = 0;
        ClientToScreen(MAME32App.m_hWnd, &p);

        if (p.x < 0)
            p.x = 0;

        if (p.y < 0)
            p.y = 0;

        s->bits += p.y*s->pitch + p.x*bytes_per_pixel;
    }

    return TRUE;
}

static BOOL DirectDrawClipToWindow()
{
    HRESULT     ddrval;

    ddrval = IDirectDrawClipper_SetHWnd(ddclipper, 0, MAME32App.m_hWnd);
    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawClipToWindow couldn't set window\n");
        return FALSE;
    }

    ddrval = IDirectDrawSurface2_SetClipper(front_buffer, ddclipper);
    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawClipToWindow couldn't set clipper\n");
        return FALSE;
    }

    return TRUE;
}

static void DirectDrawClearFrontBuffer()
{
    DirectDrawClearBuffer(front_buffer, 0);
}

/*
 * DirectDrawClearBuffer:  Clear given Direct Draw buffer with given RGB color.
 */
static void DirectDrawClearBuffer(LPDIRECTDRAWSURFACE ddbuffer, DWORD fill_color)
{
    DDBLTFX     ddbltfx;
    HRESULT     ddrval;

    assert(dd != NULL);
    assert(ddbuffer != NULL);

    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = fill_color;

    while (1)
    {
        ddrval = IDirectDrawSurface2_Blt(ddbuffer, NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);

        if (ddrval == DD_OK )
        break;

        if (ddrval == DDERR_SURFACELOST )
        {
            if (!DirectDrawRestoreSurfaces())
                return;
        }
        else
        if (ddrval != DDERR_WASSTILLDRAWING)
        {
            ErrorMsg("DirectDrawClearBuffer got unexpected error %i\n", ddrval);
            return;
        }
    }
}

/*
 * DirectDrawRestoreSurfaces:  Restore surfaces when they've been lost
 *   (e.g. when the program is minimized and reactivated).
 *   Returns TRUE on success.
 */
static BOOL DirectDrawRestoreSurfaces(void)
{
    HRESULT     ddrval;

    assert(front_buffer != NULL);

    ddrval = IDirectDrawSurface2_Restore(front_buffer);
    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawRestoreSurfaces failed to restore surfaces\n");
        return FALSE;
    }
    ddrval = IDirectDrawSurface2_Restore(back_buffer);
    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawRestoreSurfaces failed to restore back surfaces\n");
        return FALSE;
    }

    return TRUE;
}

static void DirectDrawSetPaletteColors(LPPALETTEENTRY ppe)
{
    HRESULT     ddrval;

    assert(ddpalette != NULL);

    ddrval = IDirectDrawPalette_SetEntries(ddpalette, 0, 0, 256, ppe);
    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawSetColor couldn't set colors, error = %i\n", ddrval);
        return;
    }
}

static int FindBlackPen(void)
{
    int i;

    for (i = 0; i < OSD_NUMPENS; i++)
    {
        if (pal.entries[i].peRed   == 0
        &&  pal.entries[i].peGreen == 0
        &&  pal.entries[i].peBlue  == 0)
        {
            return i;
        }
    }
    return 0;
}

static BOOL DirectDrawUpdateScreen()
{
    HRESULT ddrval;
    RECT rect,source_rect;
    POINT p;

    assert(dd != NULL);
    assert(front_buffer != NULL);

    while (1)
    {
        p.x = p.y = 0;
        ClientToScreen(MAME32App.m_hWnd, &p);
        rect.left = p.x;
        rect.top  = p.y;

        rect.right  = rect.left + visible_rect.m_Width  * scale * bytes_per_pixel;
        rect.bottom = rect.top  + visible_rect.m_Height * scale;

        source_rect.left = 0;
        source_rect.top = 0;
        source_rect.right = source_rect.left + visible_rect.m_Width * bytes_per_pixel;
        source_rect.bottom = source_rect.top + visible_rect.m_Height;

        ddrval = IDirectDrawSurface2_Blt(front_buffer, &rect, back_buffer, &source_rect, 0, NULL);

        if (ddrval == DD_OK)
            break;

        if (ddrval == DDERR_SURFACELOST)
            if (!DirectDrawRestoreSurfaces())
                return TRUE;

        if (ddrval != DDERR_WASSTILLDRAWING)
        {
            ErrorMsg("DirectDrawUpdateScreen got unexpected error %08x\n", ddrval);
            if (ddrval == DDERR_SURFACEBUSY)
                ErrorMsg("surface busy\n");

            return FALSE;
        }
    }
    return TRUE;
}

/*
 * DirectDrawUnlock:  Unlock the given Direct Draw surface, where s->bits
 *   points to the memory of the surface that was locked.
 */
static BOOL DirectDrawUnlock(BOOL front,Surface *s)
{
    HRESULT ddrval;

    LPDIRECTDRAWSURFACE buffer; /* could potentially be either front_buffer or back_buffer */

    if (front)
       buffer = front_buffer;
    else
       buffer = back_buffer;

    ddrval = IDirectDrawSurface2_Unlock(buffer, s->base_bits);
    if (ddrval != DD_OK)
        return FALSE;

    return TRUE;
}

static BOOL DirectDrawCreatePalette(CKPALETTE *palette)
{
    HRESULT     ddrval;

    assert(dd != NULL);

    ddrval = IDirectDraw2_CreatePalette(dd,
                        DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                        (struct tagPALETTEENTRY *)palette, &ddpalette, NULL);

    if (ddrval != DD_OK)
    {
        ErrorMsg("DirectDrawSetPalette couldn't create palette, error = %i\n", ddrval);
        return FALSE;
    }
    return TRUE;
}

/*
 * DirectDrawSetPalette:  Set palette of the display.
 *   Returns TRUE on success.
 */
static BOOL DirectDrawSetPalette()
{
    HRESULT     ddrval;

    assert(dd != NULL);
    assert(ddpalette != NULL);

    while (1)
    {
        ddrval = IDirectDrawSurface2_SetPalette(front_buffer, ddpalette);
        if (ddrval == DD_OK)
            break;

        if (ddrval == DDERR_SURFACELOST)
        {
            if (!DirectDrawRestoreSurfaces())
            {
                ErrorMsg("Direct draw error setting palette/restoring surface\n");
                return FALSE;
            }
            continue;
        }
        /*
        debug(("DirectDrawSetPalette couldn't set palette, error = %i\n", ddrval));
        ErrorMsg("%i %i %i %i %i\n%i %i %i %i %i\n",
          DDERR_GENERIC , DDERR_INVALIDOBJECT , DDERR_INVALIDPARAMS ,
          DDERR_INVALIDSURFACETYPE , DDERR_NOEXCLUSIVEMODE ,
          DDERR_NOPALETTEATTACHED ,DDERR_NOPALETTEHW ,DDERR_NOT8BITCOLOR ,
          DDERR_SURFACELOST , DDERR_UNSUPPORTED);
          */
        return FALSE;
    }

    return TRUE;
}

static void AdjustVisibleRect(int xmin, int ymin, int xmax, int ymax)
{
    int temp;
    int w, h;

    if (Machine->orientation & ORIENTATION_SWAP_XY)
    {
        temp = xmin; xmin = ymin; ymin = temp;
        temp = xmax; xmax = ymax; ymax = temp;
        w = Machine->drv->screen_height;
        h = Machine->drv->screen_width;
    }
    else
    {
        w = Machine->drv->screen_width;
        h = Machine->drv->screen_height;
    }

    if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
    {
        if (Machine->orientation & ORIENTATION_FLIP_X)
        {
            temp = w - xmin - 1;
            xmin = w - xmax - 1;
            xmax = temp;
        }
        if (Machine->orientation & ORIENTATION_FLIP_Y)
        {
            temp = h - ymin - 1;
            ymin = h - ymax - 1;
            ymax = temp;
        }
    }

    visible_rect.m_Left   = xmin;
    visible_rect.m_Top    = ymin;
    visible_rect.m_Width  = xmax - xmin + 1;
    visible_rect.m_Height = ymax - ymin + 1;
}

static int FindNearestColor(int min_entry,int max_entry,int r,int g,int b)
{
    int i;
    int best,mindist;

    mindist = 200000;
    best = 0;

    for (i = min_entry;i < max_entry;i++)
    {
        int d1,d2,d3,dist;

        d1 = (int)pal.entries[i].peRed - r;
        d2 = (int)pal.entries[i].peGreen - g;
        d3 = (int)pal.entries[i].peBlue - b;
        dist = d1*d1 + d2*d2 + d3*d3;

        if (dist < mindist)
        {
            best = i;
            mindist = dist;
        }
    }

    return best;
}

static void AdjustPalette(void)
{
    int i;

    if (pMAMEBitmap->depth == 16)
        return;

    memcpy(adjusted_pal.entries, pal.entries, OSD_NUMPENS * sizeof(PALETTEENTRY));

    for (i = 0; i < OSD_NUMPENS; i++)
    {
        if (i != ui_pen) /* Don't map the UI pen. */
            Display_MapColor(&adjusted_pal.entries[i].peRed,
                             &adjusted_pal.entries[i].peGreen,
                             &adjusted_pal.entries[i].peBlue);
    }
    /* Set the UI pen if it is shared with the game palette. */
    if (ui_pen_shared)
        Display_MapColor(&adjusted_pal.entries[ui_pen].peRed,
                         &adjusted_pal.entries[ui_pen].peGreen,
                         &adjusted_pal.entries[ui_pen].peBlue);

    palette_changed = TRUE;
}

/***************************************************************************

 ***************************************************************************/
