/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.
    
***************************************************************************/

/***************************************************************************

  Screenshot.c
  
    Win32 DIB handling.
    
      Created 7/1/98 by Mike Haaland (mhaaland@hypertech.com)
      
***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include "Screenshot.h"
#include "driver.h"
#include "options.h"
#include "zlib.h"
#include "file.h"
#include "osdepend.h"
#include "FilePrivate.h"
#include "png.h"


/***************************************************************************
    global variables
***************************************************************************/

char pic_format[FORMAT_MAX][4] = {
    "png",
    "bmp"
};

/***************************************************************************
    function prototypes
***************************************************************************/

static BOOL     LoadPNG(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *pPal);
static BOOL     LoadBMP(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *Pal);
static BOOL     DrawDIB(HWND hWnd, HDC hDC, HGLOBAL hDIB, HPALETTE hPal);
static LPVOID   ImageIdent(LPCSTR filename, int* filetype, BOOL flyer);

static BOOL     AllocatePNG(struct png_info *p, HGLOBAL *phDIB, HPALETTE* pPal);

/***************************************************************************
    Static global variables
***************************************************************************/

static HGLOBAL   hDIB = 0;
static HPALETTE  hPal = 0;
static int       nLastGame = -1;

#define WIDTHBYTES(width) ((width) / 8)

/* PNG variables */

static int   copy_size = 0;
static char* pixel_ptr = 0;
static int   row = 0;
static int   effWidth;

/***************************************************************************
    Functions
***************************************************************************/

BOOL ScreenShotLoaded(void)
{
    return hDIB != NULL;
}

/* This function will work with both "old" (BITMAPCOREHEADER)
 * and "new" (BITMAPINFOHEADER) bitmap formats, but will always
 * return with the "new" BITMAPINFO header in the phDIB handle.
 *
 * LoadBMP  - Loads a BMP file and creates a logical palette for it.
 * Returns  - TRUE for success
 * sBMPFile - Full path of the BMP file
 * phDIB    - Pointer to a HGLOBAL variable to hold the loaded bitmap
 *            Memory is allocated by this function but should be 
 *            released by the caller.
 * pPal     - Will hold the logical palette
 */
static BOOL LoadBMP(LPVOID mfile, HGLOBAL *phDIB, HPALETTE  *pPal)
{
    int                 dibSize;
    int                 palEntrySize;
    int                 nColors;
    int                 nFileLen = 0;
    int                 size;
    HGLOBAL             hDIB;
    BITMAPFILEHEADER    bmfHeader;
    BITMAPINFOHEADER    bi;
    LPBITMAPINFO        bmInfo;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPCOREHEADER  bc;
    RGBQUAD*            pRgb;
    LPVOID              lpDIBBits = 0;
    DWORD               dwWidth = 0; 
    DWORD               dwHeight = 0; 
    WORD                wPlanes, wBitCount;

    /* Get the file size */
    if (osd_fseek(mfile, 0L, SEEK_END) != 0)
        return FALSE;

    if ((nFileLen = osd_ftell(mfile)) == -1)
        return FALSE;

    osd_fseek(mfile, 0L, SEEK_SET);

    /* Read file header */
    if (osd_fread(mfile, (LPSTR)&bmfHeader, sizeof(bmfHeader)) != sizeof(bmfHeader))
    {
        return FALSE;
    }
    
    /* File type should be 'BM' */
    if (bmfHeader.bfType != ((WORD) ('M' << 8) | 'B'))
    {
        return FALSE;
    }
    if (osd_fread(mfile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
    {
        return FALSE;
    }

    size = bi.biSize;

    /* Check the nature (BITMAPINFO or BITMAPCORE) of the info. block and
     * extract the field information accordingly. If a BITMAPCOREHEADER, 
     * transfer it's field information to a BITMAPINFOHEADER-style block 
     */ 
    switch (size)
    {
    case sizeof(BITMAPINFOHEADER):
        palEntrySize = sizeof(RGBQUAD);
        if (bi.biClrUsed != 0)
        {
            nColors = bi.biClrUsed;
        }
        else
        {
            nColors = (bi.biBitCount <= 8) ? 1 << bi.biBitCount : 0;
            /* 24 bit images have no color table */
        }
        break;

    case sizeof(BITMAPCOREHEADER):
        bc = (LPBITMAPCOREHEADER)&bi;

        nColors = (bc->bcBitCount <= 8) ? 1 << bc->bcBitCount : 0;        

        dwWidth   = (DWORD)bc->bcWidth; 
        dwHeight  = (DWORD)bc->bcHeight; 
        wPlanes   = bc->bcPlanes; 
        wBitCount = bc->bcBitCount; 

        bi.biSize           = sizeof(BITMAPINFOHEADER); 
        bi.biWidth          = dwWidth; 
        bi.biHeight         = dwHeight; 
        bi.biPlanes         = wPlanes; 
        bi.biBitCount       = wBitCount; 
        
        bi.biCompression    = BI_RGB; 
        bi.biSizeImage      = 0; 
        bi.biXPelsPerMeter  = 0; 
        bi.biYPelsPerMeter  = 0; 
        bi.biClrUsed        = nColors; 
        bi.biClrImportant   = nColors;

        osd_fseek(mfile, sizeof(BITMAPCOREHEADER) - sizeof (BITMAPINFOHEADER), SEEK_CUR);

        palEntrySize = sizeof(RGBTRIPLE);
        break;

    default: 
        /* Not a DIB! */
        osd_fclose(mfile);
        return FALSE;
    }

    if (bi.biSizeImage == 0)
        bi.biSizeImage = WIDTHBYTES((DWORD)bi.biWidth * bi.biBitCount) *
                         bi.biHeight;

    if (bi.biClrUsed == 0)
        bi.biClrUsed = nColors;

    dibSize = nFileLen - (sizeof(BITMAPFILEHEADER) + size + (nColors * palEntrySize));
    hDIB = GlobalAlloc(GMEM_FIXED, bi.biSize + (nColors * sizeof(RGBQUAD)) + dibSize);
    if (! hDIB)
    {
        osd_fclose(mfile);
        return FALSE;
    }
    lpbi = (LPVOID)hDIB;
    memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
    pRgb = (RGBQUAD*)((LPSTR)lpbi + bi.biSize);
    lpDIBBits = (LPVOID)((LPSTR)lpbi + bi.biSize + (nColors * sizeof(RGBQUAD)));
    if (nColors)
    {
        if (osd_fread(mfile, (LPSTR)pRgb, nColors * palEntrySize) !=
            (nColors * palEntrySize))
        {
            GlobalFree(hDIB);
            osd_fclose(mfile);
            return FALSE;
        }

        if (size == sizeof(BITMAPCOREHEADER))
        {
            int i;
            /* Convert an old color table (3 byte RGBTRIPLEs) to a new
             * color table (4 byte RGBQUADs)
             */
            for (i = nColors - 1; i >= 0; i--)
            {
                RGBQUAD rgb;
                
                rgb.rgbRed      = ((RGBTRIPLE *)pRgb)[i].rgbtRed;
                rgb.rgbGreen    = ((RGBTRIPLE *)pRgb)[i].rgbtGreen;
                rgb.rgbBlue     = ((RGBTRIPLE *)pRgb)[i].rgbtBlue;
                rgb.rgbReserved = (BYTE)0;
                
                pRgb[i] = rgb; 
            } 
        } 
    } 
    
    /* Read the remainder of the bitmap file. */
    if (osd_fread(mfile, (LPSTR)lpDIBBits, dibSize) != dibSize)
    {
        GlobalFree(hDIB);
        return FALSE;
    }
    
    bmInfo = (LPBITMAPINFO)hDIB;

    /* Create a halftone palette if colors > 256.  */
    if (0 == nColors)
    {
        HDC hDC = CreateCompatibleDC(0); /* Desktop DC */
        *pPal = CreateHalftonePalette(hDC);
        DeleteDC(hDC);
    }
    else
    {
        UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);
        LOGPALETTE *pLP = (LOGPALETTE *)malloc(nSize);
        int  i;

        pLP->palVersion     = 0x300;
        pLP->palNumEntries  = nColors;

        for (i = 0; i < nColors; i++)
        {
            pLP->palPalEntry[i].peRed   = bmInfo->bmiColors[i].rgbRed;
            pLP->palPalEntry[i].peGreen = bmInfo->bmiColors[i].rgbGreen; 
            pLP->palPalEntry[i].peBlue  = bmInfo->bmiColors[i].rgbBlue;
            pLP->palPalEntry[i].peFlags = 0;
        }
        
        *pPal = CreatePalette(pLP);
        
        free(pLP);
    }
    
    *phDIB = hDIB;
    return TRUE;
}


/* Draw a DIB on the screen. It will scale the bitmap
 * to the area associated with the passed in HWND handle.
 * Returns TRUE for success
 */
static BOOL DrawDIB(HWND hWnd, HDC hDC, HGLOBAL hDIB, HPALETTE hPal)
{
    LPVOID  lpDIBBits;          /* Pointer to DIB bits */
    int     nResults = 0;
    RECT    rect;
    
    LPBITMAPINFO bmInfo = (LPBITMAPINFO)hDIB;
    int nColors = bmInfo->bmiHeader.biClrUsed ? bmInfo->bmiHeader.biClrUsed : 
                    1 << bmInfo->bmiHeader.biBitCount;
    
    if (bmInfo->bmiHeader.biBitCount > 8 )
        lpDIBBits = (LPVOID)((LPDWORD)(bmInfo->bmiColors +
        bmInfo->bmiHeader.biClrUsed) +
        ((bmInfo->bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
    else
        lpDIBBits = (LPVOID)(bmInfo->bmiColors + nColors);
    
    if (hPal && (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE))
    {
        SelectPalette(hDC, hPal, FALSE);
        RealizePalette(hDC);
    }

    GetClientRect(hWnd, &rect);

    {
        RECT RectWindow;
        int x, y;

        GetWindowRect(hWnd, &RectWindow);
        x = ((RectWindow.right  - RectWindow.left) - (rect.right  - rect.left)) / 2;
        y = ((RectWindow.bottom - RectWindow.top)  - (rect.bottom - rect.top))  / 2;

        OffsetRect(&rect, x, y);
    }

    nResults = StretchDIBits(hDC,                        /* hDC */
                             rect.left,                  /* DestX */
                             rect.top,                   /* DestY */
                             rect.right  - rect.left,    /* nDestWidth */
                             rect.bottom - rect.top,     /* nDestHeight */
                             0,                          /* SrcX */
                             0,                          /* SrcY */
                             bmInfo->bmiHeader.biWidth,  /* nStartScan */
                             bmInfo->bmiHeader.biHeight, /* nNumScans */
                             lpDIBBits,                  /* lpBits */
                             (LPBITMAPINFO)hDIB,         /* lpBitsInfo */
                             DIB_RGB_COLORS,             /* wUsage */
                             SRCCOPY );                  /* Flags */

    return (nResults) ? TRUE : FALSE;
}

BOOL GetScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict)
{
    int     destX, destY;
    int     destW, destH;
    RECT    rect;
    /* for scaling */        
    int x, y;
    int rWidth, rHeight;
    double scale;
    LPBITMAPINFO bmInfo = (LPBITMAPINFO)hDIB;
    BOOL bReduce = FALSE;

    if (hDIB == 0)
        return FALSE;
    
    GetClientRect(hWnd, &rect);

    /* Scale the bitmap to the frame specified by the passed in hwnd */
    x = bmInfo->bmiHeader.biWidth;
    y = bmInfo->bmiHeader.biHeight;

    rWidth  = (rect.right  - rect.left);
    rHeight = (rect.bottom - rect.top);

    /* Limit the screen shot to max height of 254 */
    if (restrict == TRUE && rHeight > 264)
    {
        rect.bottom = rect.top + 264;
        rHeight = 264;
    }

    /* If the bitmap does NOT fit in the screenshot area */
    if (x > rWidth - 10 || y > rHeight - 10)
    {
        rect.right  -= 10;
        rect.bottom -= 10;
        rWidth  -= 10;
        rHeight -= 10;
        bReduce = TRUE;
        /* Try to scale it properly */
        /*  assumes square pixels, doesn't consider aspect ratio */
        if (x > y)
            scale = (double)rWidth  / x;
        else
            scale = (double)rHeight / y;

        destW = (int)(x * scale);
        destH = (int)(y * scale);

        /* If it's still to big, scale again */
        if (destW > rWidth || destH > rHeight)
        {
            if (destW > rWidth)
                scale = (double)rWidth  / destW;
            else
                scale = (double)rHeight / destH;

            destW = (int)(destW * scale);
            destH = (int)(destH * scale);
        }
    }
    else
    {
        /* Use the bitmaps size if it fits */
        destW = x;
        destH = y;
    }


    destX = ((rWidth  - destW) / 2);
    destY = ((rHeight - destH) / 2);

    if (bReduce)
    {
        destX += 5;
        destY += 5;
    }

    pRect->left   = destX;
    pRect->top    = destY;
    pRect->right  = destX + destW;
    pRect->bottom = destY + destH;
    
    return TRUE;
}

/* Allow us to pre-load the DIB once for future draws */
BOOL LoadScreenShot(int nGame, int nType)
{
    static int use_flyer = -1;
    BOOL loaded = FALSE;

    /* No need to reload the same one again */
    if (nGame == nLastGame && hDIB != 0 && use_flyer == nType)
    {
        return TRUE;
    }

    /* Delete the last ones */
    FreeScreenShot();

    /* Load the DIB */
    loaded = LoadDIB(drivers[nGame]->name, &hDIB, &hPal, nType);

    /* If not loaded, see if there is a clone and try that */
    if (!loaded
    &&   (drivers[nGame]->clone_of != NULL)
    &&  !(drivers[nGame]->clone_of->flags & NOT_A_DRIVER))
    {
        loaded = LoadDIB(drivers[nGame]->clone_of->name, &hDIB, &hPal, nType);
        if (!loaded && drivers[nGame]->clone_of->clone_of)
            loaded = LoadDIB(drivers[nGame]->clone_of->clone_of->name, &hDIB, &hPal, nType);
    }

    nLastGame = nGame;
    use_flyer = nType;

    return (loaded) ? TRUE : FALSE;
}

/* This will draw the screen shot if it's loaded
 * Returns TRUE for success
 */
BOOL DrawScreenShot(HWND hWnd)
{
    HDC     hDC;
    BOOL    bSuccess = FALSE;
    
    if (hDIB != 0)
    {
        hDC = GetWindowDC(hWnd);
        bSuccess = DrawDIB(hWnd, hDC, hDIB, hPal);
        ReleaseDC(hWnd, hDC);
    }
    return bSuccess;
}

/* Delete the HPALETTE and Free the HDIB memory */
void FreeScreenShot(void)
{
    if (hDIB != 0)
        GlobalFree(hDIB);
    hDIB = 0;

    if (hPal)
        DeleteObject(hPal);
    hPal = 0;
}

BOOL LoadDIB(LPCTSTR filename, HGLOBAL *phDIB, HPALETTE *pPal, BOOL flyer)
{
    LPVOID  mfile;
    int     filetype;
    BOOL    success = FALSE;
    
    if ((mfile = ImageIdent(filename, &filetype, flyer)) != 0)
    {
        switch (filetype)
        {
        case FORMAT_BMP:    /* BMP */
            success = LoadBMP(mfile, phDIB, pPal);
            break;

        case FORMAT_PNG:    /* PNG */
            success = LoadPNG(mfile, phDIB, pPal);
            break;

        case FORMAT_UNKNOWN: /* Not a supported format */
            success = FALSE;
            break;
        }
        osd_fclose(mfile);
    }
    return success;
}

static LPVOID ImageIdent(LPCSTR filename, int *filetype, BOOL flyer)
{
    LPVOID  mfile;
    char    buf[16];
    int     ftype = (flyer) ? OSD_FILETYPE_FLYER : OSD_FILETYPE_SCREENSHOT;

    if ((mfile = osd_fopen(filename, "", ftype, 0)) == NULL)
        return FALSE;
    
    /* Read file header */
    if (osd_fread(mfile, buf, 16) != 16)
        return 0;

    osd_fseek(mfile, 0L, SEEK_SET);

    /* File type should be 'BM' */
    if (buf[0] == 'B'
    &&  buf[1] == 'M')
    {
        *filetype = FORMAT_BMP;
    }
    else
    if (buf[1] == 'P'
    &&  buf[2] == 'N'
    &&  buf[3] == 'G')
    {
        *filetype = FORMAT_PNG;
    }
    else
    {
        *filetype = FORMAT_UNKNOWN;
    }

    return mfile;
}

HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc)
{
    LPBITMAPINFOHEADER  lpbi;
    HBITMAP             hBM;
    int                 nColors;
    BITMAPINFO *        bmInfo = (LPBITMAPINFO)hDIB;
    LPVOID              lpDIBBits;

    if (hDIB == NULL)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)hDIB;
    nColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1 << lpbi->biBitCount;

    if (bmInfo->bmiHeader.biBitCount > 8)
        lpDIBBits = (LPVOID)((LPDWORD)(bmInfo->bmiColors + 
            bmInfo->bmiHeader.biClrUsed) + 
            ((bmInfo->bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
    else
        lpDIBBits = (LPVOID)(bmInfo->bmiColors + nColors);

    if (desc != 0)
    {
        /* Store for easy retrieval later */
        desc->bmWidth  = bmInfo->bmiHeader.biWidth;
        desc->bmHeight = bmInfo->bmiHeader.biHeight;
        desc->bmColors = (nColors <= 256) ? nColors : 0;
    }

    hBM = CreateDIBitmap(hDC,                     /* handle to device context */
                        (LPBITMAPINFOHEADER)lpbi, /* pointer to bitmap info header  */
                        (LONG)CBM_INIT,           /* initialization flag */
                        lpDIBBits,                /* pointer to initialization data  */
                        (LPBITMAPINFO)lpbi,       /* pointer to bitmap info */
                        DIB_RGB_COLORS);          /* color-data usage  */

    return hBM;
}


/***************************************************************************
    PNG graphics handling functions
***************************************************************************/

static void store_pixels(char *buf, int len)
{
    if (pixel_ptr && copy_size)
    {
        memcpy(&pixel_ptr[row * effWidth], buf, len);
        row--;
        copy_size -= len;
    }
}

BOOL AllocatePNG(struct png_info *p, HGLOBAL *phDIB, HPALETTE *pPal)
{
    int                 dibSize;
    HGLOBAL             hDIB;
    BITMAPINFOHEADER    bi;
    LPBITMAPINFOHEADER  lpbi;
    LPBITMAPINFO        bmInfo;
    LPVOID              lpDIBBits = 0;
    int                 lineWidth = 0;
    int                 nColors = 0;
    RGBQUAD*            pRgb;
    copy_size = 0;
    pixel_ptr = 0;
    row       = p->height - 1;
    lineWidth = p->width;
    
    if (p->color_type != 2 && p->num_palette <= 256)
        nColors =  p->num_palette;

    bi.biSize           = sizeof(BITMAPINFOHEADER); 
    bi.biWidth          = p->width;
    bi.biHeight         = p->height;
    bi.biPlanes         = 1;
    bi.biBitCount       = (p->color_type == 3) ? 8 : 24; /* bit_depth; */
    
    bi.biCompression    = BI_RGB; 
    bi.biSizeImage      = 0; 
    bi.biXPelsPerMeter  = 0; 
    bi.biYPelsPerMeter  = 0; 
    bi.biClrUsed        = nColors;
    bi.biClrImportant   = nColors;
    
    effWidth = (long)(((long)lineWidth*bi.biBitCount + 31) / 32) * 4;
    
    dibSize = (effWidth * bi.biHeight);
    hDIB = GlobalAlloc(GMEM_FIXED, bi.biSize + (nColors * sizeof(RGBQUAD)) + dibSize);
    
    if (!hDIB)
    {
        return FALSE;
    }

    lpbi = (LPVOID)hDIB;
    memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
    pRgb = (RGBQUAD*)((LPSTR)lpbi + bi.biSize);
    lpDIBBits = (LPVOID)((LPSTR)lpbi + bi.biSize + (nColors * sizeof(RGBQUAD)));
    if (nColors)
    {
        int i;
        /*
          Convert a PNG palette (3 byte RGBTRIPLEs) to a new
          color table (4 byte RGBQUADs)
        */
        for (i = 0; i < nColors; i++)
        {
            RGBQUAD rgb;
            
            rgb.rgbRed      = p->palette[i * 3 + 0];
            rgb.rgbGreen    = p->palette[i * 3 + 1];
            rgb.rgbBlue     = p->palette[i * 3 + 2];
            rgb.rgbReserved = (BYTE)0;
            
            pRgb[i] = rgb; 
        } 
    } 
    
    bmInfo = (LPBITMAPINFO)hDIB;
    
    /* Create a halftone palette if colors > 256. */
    if (0 == nColors || nColors > 256)
    {
        HDC hDC = CreateCompatibleDC(0); /* Desktop DC */
        *pPal = CreateHalftonePalette(hDC);
        DeleteDC(hDC);
    }
    else
    {
        UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);
        LOGPALETTE *pLP = (LOGPALETTE *)malloc(nSize);
        int  i;
        
        pLP->palVersion     = 0x300;
        pLP->palNumEntries  = nColors;
        
        for (i = 0; i < nColors; i++)
        {
            pLP->palPalEntry[i].peRed   = bmInfo->bmiColors[i].rgbRed;
            pLP->palPalEntry[i].peGreen = bmInfo->bmiColors[i].rgbGreen; 
            pLP->palPalEntry[i].peBlue  = bmInfo->bmiColors[i].rgbBlue;
            pLP->palPalEntry[i].peFlags = 0;
        }
        
        *pPal = CreatePalette(pLP);
        
        free (pLP);
    }
    
    copy_size = dibSize;
    pixel_ptr = lpDIBBits;
    *phDIB = hDIB;
    return TRUE;
}

/* Copied and modified from png.c */
static int png_read_bitmap(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *pPAL)
{
    struct png_info p;
    UINT32 i;
    int bytespp;
    
    if (!png_read_file(mfile, &p))
        return 0;

    if (p.color_type != 3 && p.color_type != 2)
    {
        logerror("Unsupported color type %i (has to be 3)\n", p.color_type);
        free(p.image);
        return 0;
    }
    if (p.interlace_method != 0)
    {
        logerror("Interlace unsupported\n");
        free (p.image);
        return 0;
    }
 
    /* Convert < 8 bit to 8 bit */
    png_expand_buffer_8bit(&p);
    
    if (!AllocatePNG(&p, phDIB, pPAL))
    {
        logerror("Unable to allocate memory for artwork\n");
        free(p.image);
        return 0;
    }

    bytespp = (p.color_type == 2) ? 3 : 1;

    for (i = 0; i < p.height; i++)
    {
        UINT8 *ptr = p.image + i * (p.width * bytespp);

        if (p.color_type == 2) /*(p->bit_depth > 8) */
        {
            int j;
            UINT8 bTmp;

            for (j = 0; j < p.width; j++)
            {
                bTmp = ptr[0];
                ptr[0] = ptr[2];
                ptr[2] = bTmp;
                ptr += 3;
            }
        }   
        store_pixels(p.image + i * (p.width * bytespp), p.width * bytespp);
    }

    free(p.image);

    return 1;
}

/* Load a png image */
static BOOL LoadPNG(LPVOID mfile, HGLOBAL *phDIB, HPALETTE *pPal)
{
    if (!png_read_bitmap(mfile, phDIB, pPal))
        return 0;
    return 1;
}

/* End of source */
