/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

#if defined(MAME_AVI)

/***************************************************************************

  Avi.c

  AVI capture code contributed by John Zissopoulos
                <zissop@theseas.softlab.ece.ntua.gr>

  How it works
  ==================================================

    AviStartCapture(filename, width, height, depth);

    do {
        AviAddBitmap(tBitmap, pPalEntries)
    } while (!done);

    AviEndCapture();

***************************************************************************/

#include "driver.h"

#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <strings.h>
#include <sys/stat.h>
#include <wingdi.h>
#include <tchar.h>
/* to shutup the warnings about functions were NOT calling in vfw.h */
#ifdef CDECL
#undef CDECL
#define CDECL CLIB_DECL
#endif
#include <vfw.h>

#include "Mame32.h"
#include "Display.h"
#include "Avi.h"


/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL     AVI_Check_Version(void);
static BOOL     AVI_Init(char* filename, int width, int height, int depth);
static void     AVI_Close(char* filename);
static char*    AVI_Convert_Bitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries);
static void     AVI_Message_Error(void);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static PAVIFILE     pfile        = NULL;
static PAVISTREAM   psCompressed = NULL;
static PAVISTREAM   ps           = NULL;
static BOOL         bAviError    = FALSE;
static int          nAviFrames   = 0;
static BOOL         bAviCapture  = FALSE;

static DWORD        nAviFPS = 0;
static char         aviFilename[MAX_PATH * 2];

/***************************************************************************
    External variables
 ***************************************************************************/

/* Set frames per second */
void SetAviFPS(int fps)
{
    nAviFPS = fps;
}

/* Is AVI capturing initialized? */
BOOL GetAviCapture(void)
{
    return bAviCapture;
}

/* Start capture, initialize */
BOOL AviStartCapture(char* filename, int width, int height, int depth)
{
    if (bAviCapture || (!AVI_Check_Version()))
        return FALSE;

    AVI_Init(filename, width, height, depth);
    bAviCapture = TRUE;
    strcpy(aviFilename, filename);

    return TRUE;
}

/* End capture, close file */
void AviEndCapture(void)
{
    if (!bAviCapture)
        return;

    AVI_Close(aviFilename);
    bAviCapture = FALSE;
    strcpy(aviFilename, "");
}

/* Add a frame to an open AVI */
void AviAddBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries)
{
    HRESULT hr;
    LPBITMAPINFOHEADER lpbit;
    char* bitmap;

    if (!bAviCapture)
        return;

    bitmap = AVI_Convert_Bitmap(tBitmap, pPalEntries);
    lpbit = (LPBITMAPINFOHEADER)bitmap;

    if (!nAviFrames)
    {
        hr = AVIStreamSetFormat(psCompressed, 0,
                                lpbit,              /* stream format */
                                lpbit->biSize +     /* format size */
                                lpbit->biClrUsed * sizeof(RGBQUAD));

        if (hr != AVIERR_OK)
        {
            bAviError = TRUE;
        }
    }
    
    if (!bAviError)
    {
        hr = AVIStreamWrite(psCompressed,           /* stream pointer */
                            nAviFrames,             /* time of this frame */
                            1,                      /* number to write */
                            (LPBYTE) lpbit +        /* pointer to data */
                            lpbit->biSize +
                            lpbit->biClrUsed * sizeof(RGBQUAD),
                            lpbit->biSizeImage,     /* size of this frame */
                            AVIIF_KEYFRAME,         /* flags.... */
                            NULL,
                            NULL);
    }
        
    free(bitmap);
    nAviFrames++;
}

/***************************************************************************
    External variables
 ***************************************************************************/

static BOOL AVI_Init(char* filename, int width, int height, int depth)
{
    /* Must pass the following parameters */
    /* width  = drivers[index]->drv->screen_width */
    /* height = drivers[index]->drv->screen_height */
    /* depth  = bits per pixel */

    HRESULT hr;
    AVISTREAMINFO strhdr;
    DWORD dwSize;
    DWORD wColSize;
    DWORD biSizeImage;
    UINT wLineLen;
    AVICOMPRESSOPTIONS opts;
    AVICOMPRESSOPTIONS* aopts[1];

    aopts[0] = &opts;

    AVIFileInit();

    hr = AVIFileOpen(&pfile,                      /* returned file pointer */
                     filename,                    /* file name */
                     OF_WRITE | OF_CREATE,        /* mode to open file with */
                     NULL);                       /* use handler determined */
                                                  /* from file extension.... */
    if (hr != AVIERR_OK)
    {
        AVI_Message_Error();
        return FALSE;
    }

    wLineLen = (width * depth + 31) / 32 * 4;

    wColSize = sizeof(RGBQUAD) * ((depth <= 8) ? OSD_NUMPENS : 0);

    dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
                (DWORD)(UINT)wLineLen * (DWORD)(UINT)height;

    biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize;

    /*
       Fill in the header for the video stream....
       The video stream will run in 60ths of a second....To be changed
    */

    _fmemset(&strhdr, 0, sizeof(strhdr));
    strhdr.fccType                = streamtypeVIDEO;
    strhdr.fccHandler             = 0;
    strhdr.dwScale                = 1;
    strhdr.dwRate                 = 60;         /* 60 fps */
    strhdr.dwSuggestedBufferSize  = biSizeImage;

    SetRect(&strhdr.rcFrame, 0, 0,          /* rectangle for stream */
            (int) width,
            (int) height);

    /* And create the stream */
    hr = AVIFileCreateStream(pfile,         /* file pointer */
                             &ps,           /* returned stream pointer */
                             &strhdr);      /* stream header */
    if (hr != AVIERR_OK)
    {
        AVI_Message_Error();
        return FALSE;
    }

    _fmemset(&opts, 0, sizeof(opts));
    if (!AVISaveOptions(NULL, 0, 1, &ps, aopts))
    {
        AVI_Message_Error();
        return FALSE;
    }

    hr = AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL);
    if (hr != AVIERR_OK)
    {
        AVI_Message_Error();
        return FALSE;
    }

    return TRUE;
}

static void AVI_Close(char* filename)
{
    FILE* fp;

    if (ps)
        AVIStreamClose(ps);

    if (psCompressed)
        AVIStreamClose(psCompressed);

    if (pfile)
        AVIFileClose(pfile);

    AVIFileExit();

    pfile        = NULL;
    ps           = NULL;
    psCompressed = NULL;
    nAviFrames   = 0;
    bAviError    = FALSE;

    /* Directly edit the frame rate information */
    if ((fp = fopen(filename, "rb+")) != NULL)
    {
        fseek(fp, 132, SEEK_SET);
        fwrite(&nAviFPS, 1, sizeof(DWORD), fp);
        fclose(fp);
    }
}


static BOOL AVI_Check_Version(void)
{
    WORD wVer;

    /* first let's make sure we are running on 1.1 */
    wVer = HIWORD(VideoForWindowsVersion());
    if (wVer < 0x010a)
    {
        return FALSE;
    }
    return TRUE;
}

static void AVI_Message_Error(void)
{
    MessageBox(NULL, "AVI File Error!", "Error", MB_OK);
}

static char* AVI_Convert_Bitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries)
{
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[OSD_NUMPENS];
    char *  bitmap;
    int     i;
    DWORD   dwSize;
    DWORD   wColSize;
    UINT    wLineLen;

    wLineLen = (tBitmap->width * tBitmap->depth + 31) / 32 * 4;

    wColSize = sizeof(RGBQUAD) * ((tBitmap->depth <= 8) ? OSD_NUMPENS * sizeof(RGBQUAD) : 0);

    dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
             (DWORD)(UINT)wLineLen * (DWORD)(UINT)tBitmap->height;

    if (tBitmap->depth <= 8)
    {
        bitmap = (char*)malloc(sizeof(BITMAPINFOHEADER) + OSD_NUMPENS * sizeof(RGBQUAD) + tBitmap->height * wLineLen * tBitmap->depth / 8);
    }
    else
    {
        bitmap = (char*)malloc(sizeof(BITMAPINFOHEADER) + tBitmap->height * wLineLen * tBitmap->depth / 8);
    }

    /* This is the RGBQUAD structure */
    if (tBitmap->depth <= 8)
    {
        for (i = 0; i < OSD_NUMPENS; i++)
        {
            bmiColors[i].rgbRed      = pPalEntries->peRed;
            bmiColors[i].rgbGreen    = pPalEntries->peGreen;
            bmiColors[i].rgbBlue     = pPalEntries->peBlue;
            bmiColors[i].rgbReserved = 0;
            pPalEntries++;
        }
    }

    /* This is the BITMAPINFOHEADER structure */
    bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth           = tBitmap->width;
    bmiHeader.biHeight          = tBitmap->height;
    bmiHeader.biPlanes          = 1;
    bmiHeader.biBitCount        = (tBitmap->depth <= 8) ? 8 : 16;
    bmiHeader.biCompression     = BI_RGB;
    bmiHeader.biSizeImage       = dwSize - sizeof(BITMAPINFOHEADER) - wColSize;
    bmiHeader.biXPelsPerMeter   = 0;
    bmiHeader.biYPelsPerMeter   = 0;
    bmiHeader.biClrUsed         = (tBitmap->depth == 8) ? OSD_NUMPENS : 0;
    bmiHeader.biClrImportant    = 0;

    /* Write the BITMAPINFOHEADER */
    memcpy(bitmap, (void*)&bmiHeader, sizeof(BITMAPINFOHEADER));

    /* Write the Color Table */
    if (tBitmap->depth <= 8)
    {
        memcpy(bitmap + sizeof(BITMAPINFOHEADER), (void*)&bmiColors, OSD_NUMPENS * sizeof(RGBQUAD));
    }

    /* Write the actual Bitmap Data */
    for (i = tBitmap->height - 1; 0 <= i; i--)
    {
        if (tBitmap->depth <= 8)
        {
            memcpy(bitmap + sizeof(BITMAPINFOHEADER) + OSD_NUMPENS * sizeof(RGBQUAD) + (tBitmap->height - i - 1) *
                   (tBitmap->width * tBitmap->depth / 8), (void*)tBitmap->line[i],
                   (tBitmap->width * tBitmap->depth / 8));
        }
        else
        {
            unsigned short  pLine[1024];
            unsigned short* ptr = (unsigned short*)tBitmap->line[i];
            unsigned char   r, g, b;
            int x;

            /* convert bitmap data to RGB555 */
            for (x = 0; x < tBitmap->width; x++)
            {
                osd_get_pen(*ptr++, &r, &g, &b);
                pLine[x] = ((r & 0xF8) << 7) |
                           ((g & 0xF8) << 2) |
                           ((b & 0xF8) >> 3);
            }

            memcpy(bitmap + sizeof(BITMAPINFOHEADER) + (tBitmap->height - i - 1) *
                   (tBitmap->width * tBitmap->depth / 8),
                   (void*)pLine, (tBitmap->width * tBitmap->depth / 8));
        }
    }

    return bitmap;
}

#endif /* MAME_AVI */
