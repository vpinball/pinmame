/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

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
#include <assert.h>
#include <math.h>
#include "osdepend.h"
#include "MAME32.h"
#include "Display.h"
#include "M32Util.h"
#include "driver.h"
#include "uclock.h"
#include "file.h"
#include "dirty.h"
#include "avi.h"
#include "png.h"
#include "artwork.h"

#define FRAMESKIP_LEVELS 12
#define INTENSITY_LEVELS 256

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int                Display_init(options_type *options);
static void               Display_exit(void);
static struct osd_bitmap* Display_alloc_bitmap(int width,int height,int depth);
static void               Display_free_bitmap(struct osd_bitmap* bitmap);
static int                Display_skip_this_frame(void);
static void               Display_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap);
static void               Display_set_gamma(float gamma);
static void               Display_set_brightness(int brightness);

static void               Throttle(void);
static void               AdjustColorMap(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDDisplay OSDDisplay = 
{
    Display_init,               /* init              */
    Display_exit,               /* exit              */
    Display_alloc_bitmap,       /* alloc_bitmap      */
    Display_free_bitmap,        /* free_bitmap       */
    0,                          /* create_display    */
    0,                          /* close_display     */
    0,                          /* set_visible_area  */
    0,                          /*        */
    0,                          /* allocate_colors   */
    0,                          /* modify_pen        */
    0,                          /* get_pen           */
    0,                          /* mark_dirty        */
    Display_skip_this_frame,    /* skip_this_frame   */
    Display_update_display,     /* update_display    */
    0,                          /* led_w             */
    Display_set_gamma,          /* set_gamma         */
    Display_get_gamma,          /* get_gamma         */
    Display_set_brightness,     /* set_brightness    */
    Display_get_brightness,     /* get_brightness    */
    0,                          /* save_snapshot     */

    0,                          /* OnMessage         */
    0,                          /* Refresh           */
    0,                          /* GetBlackPen       */
    0,                          /* UpdateFPS         */
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

    /* Frames per second (FPS) info */
    BOOL            m_bShowFPS;
    int             m_nShowFPSTemp;

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

    /* set true if capturing avi */
    BOOL            m_bAviCapture;
};


#define FRAMES_TO_SKIP 20   /* skip the first few frames from the FPS calculation */
                            /* to avoid counting the copyright and info screens */

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct tDisplay_private      This;
static int                          snapno = 0;
static const int                    safety = 16;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

/*
    put here anything you need to do when the program is started. Return 0 if 
    initialization was successful, nonzero otherwise.
*/
static int Display_init(options_type *options)
{
    /* Reset the Snapshot number */
    snapno = 0;

    This.m_dGamma            = max(0, options->gamma);
    This.m_nBrightness       = max(0, options->brightness);
    This.m_bShowFPS          = FALSE;
    This.m_nShowFPSTemp      = 0;
    This.m_bThrottled        = TRUE;
    This.m_bAutoFrameskip    = options->auto_frame_skip;
    This.m_nFrameSkip        = options->frame_skip;
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
    This.m_bAviCapture       = GetAviCapture();

    if (This.m_bAutoFrameskip == TRUE)
        This.m_nFrameSkip = 0;

    /* Initialize map. */
    AdjustColorMap();

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

static struct osd_bitmap* Display_alloc_bitmap(int width, int height, int depth)
{
    struct osd_bitmap*  pBitmap;

    if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
    {
        logerror("osd_alloc_bitmap() unknown depth %d\n", depth);
        return NULL;
    }

    pBitmap = (struct osd_bitmap*)malloc(sizeof(struct osd_bitmap));

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

        rowlen = (rdwidth + 2 * safety) * sizeof(unsigned char);
        if (depth == 32)
            rowlen *= 4;
        else
        if (depth == 15 ||  depth == 16)
            rowlen *= 2;

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
            if (depth == 32)
                pBitmap->line[i] = &bm[i * rowlen + safety * 4];
            else
            if (depth == 15 || depth == 16)
                pBitmap->line[i] = &bm[i * rowlen + safety * 2];
            else
                pBitmap->line[i] = &bm[i * rowlen + safety];
        }

        pBitmap->line += safety;

        pBitmap->_private = bm;
    }

    return pBitmap;
}

/*
    Free memory allocated in create_bitmap.
*/
static void Display_free_bitmap(struct osd_bitmap* pBitmap)
{
    if (pBitmap != NULL)
    {
        pBitmap->line -= safety;
        free(pBitmap->line);
        free(pBitmap->_private);
        free(pBitmap);
    }
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
    MAME32App.m_pDisplay->UpdateFPS(FALSE, 0, 0, 0, 0, 0);
    schedule_full_refresh();
}

/*
    Update the display.
*/
static void Display_update_display(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap)
{
    if (osd_skip_this_frame() == 0)
    {   
        if (This.m_nShowFPSTemp)
        {
            This.m_nShowFPSTemp--;
            if ((This.m_bShowFPS == FALSE)
            &&  (This.m_nShowFPSTemp == 0))
            {
                ClearFPSDisplay();
            }
        }

        /*
            Frames per second.
        */
        if (input_ui_pressed(IPT_UI_SHOW_FPS))
        {
            if (This.m_nShowFPSTemp)
            {
                This.m_nShowFPSTemp = 0;
                ClearFPSDisplay();
            }
            else
            {
                This.m_bShowFPS = (This.m_bShowFPS == TRUE) ? FALSE : TRUE;
                if (This.m_bShowFPS == FALSE)
                    ClearFPSDisplay();
            }
        }

        /* Wait until it's time to update the screen. */
        Throttle();

        MAME32App.m_pDisplay->update_display(game_bitmap, debug_bitmap);

        ClearDirty();

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

        if (This.m_bShowFPS == FALSE)
            This.m_nShowFPSTemp = 2 * Machine->drv->frames_per_second;

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

        if (This.m_bShowFPS == FALSE)
            This.m_nShowFPSTemp = 2 * Machine->drv->frames_per_second;

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

void Display_WriteBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries)
{
    char                sFileName[MAX_PATH];
    int                 nResult;
    void*               pFile;

    /* first of all try with "gamename.png" */
    sprintf(sFileName, "%.8s", Machine->gamedrv->name);

    /* Avoid overwriting of existing files */
    nResult = osd_faccess(sFileName, OSD_FILETYPE_SCREENSHOT);
    if (nResult)
    {
        /* Create a unique PNG file */
        do
        {
            /* otherwise use "nameNNNN" */
            sprintf(sFileName, "%.4s%04d", Machine->gamedrv->name, snapno++);

            nResult = osd_faccess(sFileName, OSD_FILETYPE_SCREENSHOT);
        }
        while (nResult != 0);
    }

    pFile = osd_fopen(sFileName, "", OSD_FILETYPE_SCREENSHOT, TRUE);
    if (pFile != NULL)
    {
        save_screen_snapshot_as(pFile, tBitmap);
        osd_fclose(pFile);
    }
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

    /*
        Show frames per second.
    */
    if (This.m_bShowFPS == TRUE || This.m_nShowFPSTemp != 0 || This.m_bAviCapture)
    {
        int fps;
        int divdr;

        divdr = 100 * FRAMESKIP_LEVELS;
        fps = (Machine->drv->frames_per_second * (FRAMESKIP_LEVELS - This.m_nFrameSkip) * This.m_nSpeed + (divdr / 2)) / divdr;
        
        if (This.m_bAviCapture)
            SetAviFPS(fps);

        if (This.m_bShowFPS == TRUE || This.m_nShowFPSTemp != 0)
            MAME32App.m_pDisplay->UpdateFPS(TRUE, This.m_nSpeed, fps, Machine->drv->frames_per_second, This.m_nFrameSkip, This.m_nVecUPS);
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


