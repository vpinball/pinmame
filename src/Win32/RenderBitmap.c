/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include "osdepend.h"
#include "dirty.h"
#include "RenderBitmap.h"
#include "MAME32.h"
#include "Display.h"

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static void RenderBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderDirtyBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleHScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleVScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleHScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDoubleVScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderDirtyBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleHScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderDirtyDoubleVScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderVDoubleBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderVDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderVDoubleDirtyBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderVDoubleDirtyHScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderVDoubleBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderVDoubleHScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderVDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderVDoubleHScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderHDoubleBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderHDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderHDoubleDirtyBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderHDoubleDirtyVScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderHDoubleBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderHDoubleVScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static void RenderHDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);
static void RenderHDoubleVScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth);

static __inline void    DoubleLine(BYTE* pSrc, UINT nSrcQuarterWidth, BYTE* pDst);
static __inline void    DoubleDirtyLine(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax, BYTE* pDst);
static __inline void    ExpandLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst, BYTE bg);
static __inline void    ExpandDirtyLine(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax, BYTE* pDst, BYTE bg);
static __inline void    DoubleDirty2Lines(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax, BYTE* pDst, UINT nDstWidth);

static __inline void    DoubleLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst);
static __inline void    ExpandLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst, WORD bg);

static __inline void    LinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst);
static __inline void    DoubleLinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst);
static __inline void    ExpandLinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst, WORD bg);

#define RenderDirtyBitmap16                     RenderBitmap16
#define RenderDirtyDoubleBitmap16               RenderDoubleBitmap16
#define RenderDirtyDoubleHScanlinesBitmap16     RenderDoubleHScanlinesBitmap16
#define RenderDirtyDoubleVScanlinesBitmap16     RenderDoubleVScanlinesBitmap16
#define RenderDirtyHDoubleBitmap16              RenderHDoubleBitmap16
#define RenderDirtyHDoubleVScanlinesBitmap16    RenderHDoubleVScanlinesBitmap16
#define RenderDirtyVDoubleBitmap16              RenderVDoubleBitmap16
#define RenderDirtyVDoubleHScanlinesBitmap16    RenderVDoubleHScanlinesBitmap16

#define RenderDirtyBitmapPalette16                     RenderBitmapPalette16
#define RenderDirtyDoubleBitmapPalette16               RenderDoubleBitmapPalette16
#define RenderDirtyDoubleHScanlinesBitmapPalette16     RenderDoubleHScanlinesBitmapPalette16
#define RenderDirtyDoubleVScanlinesBitmapPalette16     RenderDoubleVScanlinesBitmapPalette16
#define RenderDirtyHDoubleBitmapPalette16              RenderHDoubleBitmapPalette16
#define RenderDirtyHDoubleVScanlinesBitmapPalette16    RenderHDoubleVScanlinesBitmapPalette16
#define RenderDirtyVDoubleBitmapPalette16              RenderVDoubleBitmapPalette16
#define RenderDirtyVDoubleHScanlinesBitmapPalette16    RenderVDoubleHScanlinesBitmapPalette16

/***************************************************************************
    External variables
 ***************************************************************************/


/***************************************************************************
    Internal structures
 ***************************************************************************/

static const UINT32*  m_p16BitLookup;

/***************************************************************************
    Internal variables
 ***************************************************************************/


/***************************************************************************
    External function definitions
 ***************************************************************************/

RenderMethod SelectRenderMethod(BOOL bDouble, BOOL bHDouble, BOOL bVDouble, BOOL bHScanLines, BOOL bVScanLines,
                                enum DirtyMode eDirtyMode, BOOL b16bit, BOOL bPalette16,
                                const UINT32* p16BitLookup, BOOL bMMX)
{
    RenderMethod Render = NULL;

    /* remove '&& bPalette16 == FALSE' after implementation in RenderMMX.c */
    if (bMMX == TRUE && bHDouble == FALSE && bVDouble == FALSE && bPalette16 == FALSE)
    {
        Render = SelectRenderMethodMMX(bDouble, bHScanLines, bVScanLines,
                                       eDirtyMode, b16bit, bPalette16, p16BitLookup);
    }

    if (Render == NULL)
    {
        if (bDouble == TRUE)
        {
            if (bHScanLines == TRUE)
            {
                if (b16bit == TRUE)
                {
                    if (bPalette16 == TRUE)
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleHScanlinesBitmapPalette16;
                        else
                            Render = RenderDoubleHScanlinesBitmapPalette16;
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleHScanlinesBitmap16;
                        else
                            Render = RenderDoubleHScanlinesBitmap16;
                    }
                }
                else
                {
                    if (eDirtyMode == USE_DIRTYRECT)
                        Render = RenderDirtyDoubleHScanlinesBitmap;
                    else
                        Render = RenderDoubleHScanlinesBitmap;
                }
            }
            else
            if (bVScanLines == TRUE)
            {
                if (b16bit == TRUE)
                {
                    if (bPalette16 == TRUE)
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleVScanlinesBitmapPalette16;
                        else
                            Render = RenderDoubleVScanlinesBitmapPalette16;
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleVScanlinesBitmap16;
                        else
                            Render = RenderDoubleVScanlinesBitmap16;
                    }
                }
                else
                {
                    if (eDirtyMode == USE_DIRTYRECT)
                        Render = RenderDirtyDoubleVScanlinesBitmap;
                    else
                        Render = RenderDoubleVScanlinesBitmap;
                }
            }
            else
            {
                if (b16bit == TRUE)
                {
                    if (bPalette16 == TRUE)
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleBitmapPalette16;
                        else
                            Render = RenderDoubleBitmapPalette16;
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyDoubleBitmap16;
                        else
                            Render = RenderDoubleBitmap16;
                    }
                }
                else
                {
                    if (eDirtyMode == USE_DIRTYRECT)
                        Render = RenderDirtyDoubleBitmap;
                    else
                        Render = RenderDoubleBitmap;
                }
            }
        }
        else
        {
            if (bHDouble == TRUE)
            {
                if (bVScanLines == TRUE)
                {
                    if (b16bit == TRUE)
                    {
                        if (bPalette16 == TRUE)
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyHDoubleVScanlinesBitmapPalette16;
                            else
                                Render = RenderHDoubleVScanlinesBitmapPalette16;
                        }
                        else
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyHDoubleVScanlinesBitmap16;
                            else
                                Render = RenderHDoubleVScanlinesBitmap16;
                        }
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderHDoubleDirtyVScanlinesBitmap;
                        else
                            Render = RenderHDoubleVScanlinesBitmap;
                    }
                }
                else
                if (bHScanLines == TRUE)
                {
                    assert(FALSE);
                }
                else
                {
                    if (b16bit == TRUE)
                    {
                        if (bPalette16 == TRUE)
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyHDoubleBitmapPalette16;
                            else
                                Render = RenderHDoubleBitmapPalette16;
                        }
                        else
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyHDoubleBitmap16;
                            else
                                Render = RenderHDoubleBitmap16;
                        }
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderHDoubleDirtyBitmap;
                        else
                            Render = RenderHDoubleBitmap;
                    }
                }
            }
            else
            if (bVDouble == TRUE)
            {
                if (bHScanLines == TRUE)
                {
                    if (b16bit == TRUE)
                    {
                        if (bPalette16 == TRUE)
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyVDoubleHScanlinesBitmapPalette16;
                            else
                                Render = RenderVDoubleHScanlinesBitmapPalette16;
                        }
                        else
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyVDoubleHScanlinesBitmap16;
                            else
                                Render = RenderVDoubleHScanlinesBitmap16;
                        }
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderVDoubleDirtyHScanlinesBitmap;
                        else
                            Render = RenderVDoubleHScanlinesBitmap;
                    }
                }
                else
                if (bVScanLines == TRUE)
                {
                    assert(FALSE);
                }
                else
                {
                    if (b16bit == TRUE)
                    {
                        if (bPalette16 == TRUE)
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyVDoubleBitmapPalette16;
                            else
                                Render = RenderVDoubleBitmapPalette16;
                        }
                        else
                        {
                            if (eDirtyMode == USE_DIRTYRECT)
                                Render = RenderDirtyVDoubleBitmap16;
                            else
                                Render = RenderVDoubleBitmap16;
                        }
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderVDoubleDirtyBitmap;
                        else
                            Render = RenderVDoubleBitmap;
                    }
                }
            }
            else
            {
                if (b16bit == TRUE)
                {
                    if (bPalette16 == TRUE)
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyBitmapPalette16;
                        else
                            Render = RenderBitmapPalette16;
                    }
                    else
                    {
                        if (eDirtyMode == USE_DIRTYRECT)
                            Render = RenderDirtyBitmap16;
                        else
                            Render = RenderBitmap16;
                    }
                }
                else
                {
                    if (eDirtyMode == USE_DIRTYRECT)
                        Render = RenderDirtyBitmap;
                    else
                        Render = RenderBitmap;
                }
            }
        }
    }

    assert(Render != NULL);

    /* Set up the 16 bit palette lookup table. */
    if (bPalette16 == TRUE)
        m_p16BitLookup = p16BitLookup;

    return Render;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void RenderBitmap(struct osd_bitmap* pSrcBitmap,
                         UINT nSrcStartLine, UINT nSrcStartColumn,
                         UINT nNumLines, UINT nNumColumns,
                         BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        memcpy(pDst, pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns);
        
        pDst += nDstWidth;
    }
}

static void RenderBitmap16(struct osd_bitmap* pSrcBitmap,
                           UINT nSrcStartLine, UINT nSrcStartColumn,
                           UINT nNumLines, UINT nNumColumns,
                           BYTE* pDst, UINT nDstWidth)
{
    UINT    nLen = nNumColumns * 2;

    while (nNumLines--)
    {
        memcpy(pDst, (WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nLen);
        
        pDst += nDstWidth;
    }
}

static void RenderBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                  UINT nSrcStartLine, UINT nSrcStartColumn,
                                  UINT nNumLines, UINT nNumColumns,
                                  BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        LinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns / 2, (DWORD*)pDst);
        
        pDst += nDstWidth;
    }
}

static void RenderDirtyBitmap(struct osd_bitmap* pSrcBitmap,
                              UINT nSrcStartLine, UINT nSrcStartColumn,
                              UINT nNumLines, UINT nNumColumns,
                              BYTE* pDst, UINT nDstWidth)
{
    UINT    y, x;
    DWORD*  pdwDst;
    
    for (y = nSrcStartLine; y < nNumLines + nSrcStartLine; y++)
    {
        if (IsDirtyLine(y))
        {
            x  = nSrcStartColumn;
            pdwDst = (DWORD*)(pDst + (y - nSrcStartLine) * nDstWidth);
        
            while (x < nNumColumns + nSrcStartColumn)
            {
                if (((x % (32)) == 0) && !IsDirtyDword(x, y))
                {
                    x      += 32;
                    pdwDst += 8; /* 32 pixels * (1 DWORD / 4 pixels) */
                }
                else
                {
                    if (IsDirty4(x, y))
                        *pdwDst = *(DWORD*)(pSrcBitmap->line[y] + x);
             
                    x += 4;
                    pdwDst++;
                }
            }
        }
    }
}

static void RenderDoubleBitmap(struct osd_bitmap* pSrcBitmap,
                               UINT nSrcStartLine, UINT nSrcStartColumn,
                               UINT nNumLines, UINT nNumColumns,
                               BYTE* pDst, UINT nDstWidth)
{
    UINT nSrcQuarterWidth = nNumColumns >> 2;

    while (nNumLines--)
    {
        DoubleLine(pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nSrcQuarterWidth, pDst);
        pDst += nDstWidth;

        DoubleLine(pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nSrcQuarterWidth, pDst);
        pDst += nDstWidth;
    }
}

static void RenderDoubleBitmap16(struct osd_bitmap* pSrcBitmap,
                                 UINT nSrcStartLine, UINT nSrcStartColumn,
                                 UINT nNumLines, UINT nNumColumns,
                                 BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nNumColumns, pDst);
        pDst += nDstWidth;

        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, pDst);
        pDst += nDstWidth;
    }
}

static void RenderDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                        UINT nSrcStartLine, UINT nSrcStartColumn,
                                        UINT nNumLines, UINT nNumColumns,
                                        BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        DoubleLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nNumColumns, (DWORD*)pDst);
        pDst += nDstWidth;

        DoubleLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, (DWORD*)pDst);
        pDst += nDstWidth;
    }
}

static void RenderDirtyDoubleBitmap(struct osd_bitmap* pSrcBitmap,
                                    UINT nSrcStartLine, UINT nSrcStartColumn,
                                    UINT nNumLines, UINT nNumColumns,
                                    BYTE* pDst, UINT nDstWidth)
{
    UINT    nDstQrtWidth    = nDstWidth >> 2;
    UINT    nDstDoubleWidth = nDstWidth * 2;

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
            DoubleDirty2Lines(pSrcBitmap->line[nSrcStartLine],
                              nSrcStartColumn, nSrcStartLine, 
                              nSrcStartColumn + nNumColumns,
                              pDst, nDstQrtWidth);
        pDst += nDstDoubleWidth;
        nSrcStartLine++;
    }
}

static void RenderDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                         UINT nSrcStartLine, UINT nSrcStartColumn,
                                         UINT nNumLines, UINT nNumColumns,
                                         BYTE* pDst, UINT nDstWidth)
{
    UINT    nDstDoubleWidth  = nDstWidth * 2;
    UINT    nSrcQuarterWidth = nNumColumns >> 2;

    while (nNumLines--)
    {
        DoubleLine(pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nSrcQuarterWidth, pDst);
        pDst += nDstDoubleWidth;
    }
}

static void RenderDoubleHScanlinesBitmap16(struct osd_bitmap* pSrcBitmap,
                                           UINT nSrcStartLine, UINT nSrcStartColumn,
                                           UINT nNumLines, UINT nNumColumns,
                                           BYTE* pDst, UINT nDstWidth)
{
    UINT    nDstDoubleWidth = nDstWidth * 2;

    while (nNumLines--)
    {
        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, pDst);
        pDst += nDstDoubleWidth;
    }
}

static void RenderDoubleHScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                                  UINT nSrcStartLine, UINT nSrcStartColumn,
                                                  UINT nNumLines, UINT nNumColumns,
                                                  BYTE* pDst, UINT nDstWidth)
{
    UINT    nDstDoubleWidth = nDstWidth * 2;

    while (nNumLines--)
    {
        DoubleLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, (DWORD*)pDst);
        pDst += nDstDoubleWidth;
    }
}

static void RenderDirtyDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                              UINT nSrcStartLine, UINT nSrcStartColumn,
                                              UINT nNumLines, UINT nNumColumns,
                                              BYTE* pDst, UINT nDstWidth)
{
    UINT    nDstDoubleWidth = nDstWidth * 2;

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
            DoubleDirtyLine(pSrcBitmap->line[nSrcStartLine],
                            nSrcStartColumn, nSrcStartLine,
                            nSrcStartColumn + nNumColumns, pDst);
        pDst += nDstDoubleWidth;
        nSrcStartLine++;
    }
}

static void RenderDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                         UINT nSrcStartLine, UINT nSrcStartColumn,
                                         UINT nNumLines, UINT nNumColumns,
                                         BYTE* pDst, UINT nDstWidth)
{
    UINT    nSrcHalfWidth   = nNumColumns >> 1;
    BYTE    nBlackPen       = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLine(pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn,
                   nSrcHalfWidth, pDst, nBlackPen);
        pDst += nDstWidth;

        ExpandLine(pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                   nSrcHalfWidth, pDst, nBlackPen);
        pDst += nDstWidth;
    }
}

static void RenderDoubleVScanlinesBitmap16(struct osd_bitmap* pSrcBitmap,
                                           UINT nSrcStartLine, UINT nSrcStartColumn,
                                           UINT nNumLines, UINT nNumColumns,
                                           BYTE* pDst, UINT nDstWidth)
{
    WORD    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLine16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn,
                     nNumColumns, pDst, nBlackPen);
        pDst += nDstWidth;

        ExpandLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                     nNumColumns, pDst, nBlackPen);
        pDst += nDstWidth;
    }
}

static void RenderDoubleVScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                                  UINT nSrcStartLine, UINT nSrcStartColumn,
                                                  UINT nNumLines, UINT nNumColumns,
                                                  BYTE* pDst, UINT nDstWidth)
{
    WORD    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn,
                            nNumColumns, (DWORD*)pDst, nBlackPen);
        pDst += nDstWidth;

        ExpandLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                            nNumColumns, (DWORD*)pDst, nBlackPen);
        pDst += nDstWidth;
    }
}

static void RenderDirtyDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                              UINT nSrcStartLine, UINT nSrcStartColumn,
                                              UINT nNumLines, UINT nNumColumns,
                                              BYTE* pDst, UINT nDstWidth)
{
    BYTE    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
        {
            ExpandDirtyLine(pSrcBitmap->line[nSrcStartLine],
                            nSrcStartColumn, nSrcStartLine,
                            nSrcStartColumn + nNumColumns,
                            pDst, nBlackPen);
            pDst += nDstWidth;

            ExpandDirtyLine(pSrcBitmap->line[nSrcStartLine],
                            nSrcStartColumn, nSrcStartLine,
                            nSrcStartColumn + nNumColumns,
                            pDst, nBlackPen);
            pDst += nDstWidth;
        }
        else
            pDst += nDstWidth * 2;

        nSrcStartLine++;
    }
}

static void RenderVDoubleBitmap(struct osd_bitmap* pSrcBitmap,
                                UINT nSrcStartLine, UINT nSrcStartColumn,
                                UINT nNumLines, UINT nNumColumns,
                                BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        memcpy(pDst, pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nNumColumns);
        pDst += nDstWidth;

        memcpy(pDst, pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns);
        pDst += nDstWidth;
    }
}

static void RenderVDoubleDirtyBitmap(struct osd_bitmap* pSrcBitmap,
                                     UINT nSrcStartLine, UINT nSrcStartColumn,
                                     UINT nNumLines, UINT nNumColumns,
                                     BYTE* pDst, UINT nDstWidth)
{
    UINT    y, x;
    DWORD*  pdwDst;
    UINT    nLineOffset = nDstWidth / sizeof(DWORD);

    for (y = nSrcStartLine; y < nNumLines + nSrcStartLine; y++)
    {
        if (IsDirtyLine(y))
        {
            x  = nSrcStartColumn;
            pdwDst = (DWORD*)(pDst + (y - nSrcStartLine) * nDstWidth * 2); /* skip every 2 lines */

            while (x < nNumColumns + nSrcStartColumn)
            {
                if (((x % (32)) == 0) && !IsDirtyDword(x, y))
                {
                    x      += 32;
                    pdwDst += 8; /* 32 pixels * (1 DWORD / 4 pixels) */
                }
                else
                {
                    if (IsDirty4(x, y))
                        *pdwDst = *(pdwDst + nLineOffset) = *(DWORD*)(pSrcBitmap->line[y] + x);
             
                    x += 4;
                    pdwDst++;
                }
            }
        }
    }
}

static void RenderVDoubleHScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                          UINT nSrcStartLine, UINT nSrcStartColumn,
                                          UINT nNumLines, UINT nNumColumns,
                                          BYTE* pDst, UINT nDstWidth)
{
    UINT nDstDoubleWidth = nDstWidth * 2;

    while (nNumLines--)
    {
        memcpy(pDst, pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns);
        pDst += nDstDoubleWidth;
    }
}

static void RenderVDoubleDirtyHScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                               UINT nSrcStartLine, UINT nSrcStartColumn,
                                               UINT nNumLines, UINT nNumColumns,
                                               BYTE* pDst, UINT nDstWidth)
{
    UINT    y, x;
    DWORD*  pdwDst;
    
    for (y = nSrcStartLine; y < nNumLines + nSrcStartLine; y++)
    {
        if (IsDirtyLine(y))
        {
            x  = nSrcStartColumn;
            pdwDst = (DWORD*)(pDst + (y - nSrcStartLine) * nDstWidth * 2); /* skip every 2 lines */

            while (x < nNumColumns + nSrcStartColumn)
            {
                if (((x % (32)) == 0) && !IsDirtyDword(x, y))
                {
                    x      += 32;
                    pdwDst += 8; /* 32 pixels * (1 DWORD / 4 pixels) */
                }
                else
                {
                    if (IsDirty4(x, y))
                        *pdwDst = *(DWORD*)(pSrcBitmap->line[y] + x);
             
                    x += 4;
                    pdwDst++;
                }
            }
        }
    }
}

static void RenderVDoubleBitmap16(struct osd_bitmap* pSrcBitmap,
                                  UINT nSrcStartLine, UINT nSrcStartColumn,
                                  UINT nNumLines, UINT nNumColumns,
                                  BYTE* pDst, UINT nDstWidth)
{
    UINT nLen = nNumColumns * 2;

    while (nNumLines--)
    {
        memcpy(pDst, (WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nLen);       
        pDst += nDstWidth;

        memcpy(pDst, (WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nLen);        
        pDst += nDstWidth;

        nSrcStartLine++;
    }
}

static void RenderVDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                         UINT nSrcStartLine, UINT nSrcStartColumn,
                                         UINT nNumLines, UINT nNumColumns,
                                         BYTE* pDst, UINT nDstWidth)
{
    UINT nLen = nNumColumns / 2;

    while (nNumLines--)
    {
        LinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nLen, (DWORD*)pDst);

        pDst += nDstWidth;

        LinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nLen, (DWORD*)pDst);
        pDst += nDstWidth;

        nSrcStartLine++;
    }
}

static void RenderVDoubleHScanlinesBitmap16(struct osd_bitmap* pSrcBitmap,
                                            UINT nSrcStartLine, UINT nSrcStartColumn,
                                            UINT nNumLines, UINT nNumColumns,
                                            BYTE* pDst, UINT nDstWidth)
{
    UINT nDstDoubleWidth = nDstWidth * 2;
    UINT nLen = nNumColumns * 2;

    while (nNumLines--)
    {
        memcpy(pDst, (WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nLen);
        pDst += nDstDoubleWidth;
    }
}

static void RenderVDoubleHScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap,
                                                   UINT nSrcStartLine, UINT nSrcStartColumn,
                                                   UINT nNumLines, UINT nNumColumns,
                                                   BYTE* pDst, UINT nDstWidth)
{
    UINT nDstDoubleWidth = nDstWidth * 2;
    UINT nLen = nNumColumns / 2;

    while (nNumLines--)
    {
        LinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nLen, (DWORD*)pDst);
        pDst += nDstDoubleWidth;
    }
}

static void RenderHDoubleBitmap(struct osd_bitmap* pSrcBitmap,
                                UINT nSrcStartLine, UINT nSrcStartColumn,
                                UINT nNumLines, UINT nNumColumns,
                                BYTE* pDst, UINT nDstWidth)
{
    UINT nSrcQuarterWidth = nNumColumns >> 2;

    while (nNumLines--)
    {
        DoubleLine(pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nSrcQuarterWidth, pDst);
        pDst += nDstWidth;
    }
}

static void RenderHDoubleVScanlinesBitmap(struct osd_bitmap* pSrcBitmap,
                                          UINT nSrcStartLine, UINT nSrcStartColumn,
                                          UINT nNumLines, UINT nNumColumns,
                                          BYTE* pDst, UINT nDstWidth)
{
    UINT    nSrcHalfWidth   = nNumColumns >> 1;
    BYTE    nBlackPen       = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLine(pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                   nSrcHalfWidth, pDst, nBlackPen);
        pDst += nDstWidth;
    }
}

static void RenderHDoubleDirtyBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
        {
            DoubleDirtyLine(pSrcBitmap->line[nSrcStartLine],
                            nSrcStartColumn, nSrcStartLine, 
                            nSrcStartColumn + nNumColumns,
                            pDst);
        }
        pDst += nDstWidth;
        nSrcStartLine++;
    }
}

static void RenderHDoubleDirtyVScanlinesBitmap(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    BYTE    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
        {
            ExpandDirtyLine(pSrcBitmap->line[nSrcStartLine],
                            nSrcStartColumn, nSrcStartLine,
                            nSrcStartColumn + nNumColumns,
                            pDst, nBlackPen);
        }

        pDst += nDstWidth;
        nSrcStartLine++;
    }
}

static void RenderHDoubleBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, pDst);
        pDst += nDstWidth;
    }
}

static void RenderHDoubleVScanlinesBitmap16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    WORD    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                     nNumColumns, pDst, nBlackPen);
        pDst += nDstWidth;
    }
}

static void RenderHDoubleBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    while (nNumLines--)
    {
        DoubleLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nNumColumns, (DWORD*)pDst);
        pDst += nDstWidth;
    }
}

static void RenderHDoubleVScanlinesBitmapPalette16(struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth)
{
    WORD    nBlackPen = MAME32App.m_pDisplay->GetBlackPen();

    while (nNumLines--)
    {
        ExpandLinePalette16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn,
                            nNumColumns, (DWORD*)pDst, nBlackPen);
        pDst += nDstWidth;
    }
}


/***************************************************************************
    inlines
 ***************************************************************************/

static __inline void DoubleLine(BYTE* pSrc, UINT nQuarterWidth, BYTE* pDst)
{
#if defined(_M_IX86) && defined(_MSC_VER)
    __asm
    {
        mov ecx, nQuarterWidth
        mov esi, pSrc
        mov edi, pDst
        
    L1:
        mov   eax, [esi]
        add   esi, 4

        mov   ebx, eax
        bswap eax
        xchg  ax, bx
        rol   eax, 8     
        ror   ebx, 8

        mov   [edi], eax
        add   edi, 4
        mov   [edi], ebx
        add   edi, 4

        dec   ecx
        jnz   L1
    }
#else
    BYTE    pixel1;
    BYTE    pixel2;

    while (nQuarterWidth--)
    {
        pixel1 = *pSrc++;
        pixel2 = *pSrc++;

        *((DWORD*)pDst)++ = (DWORD)((pixel2 << 24) | (pixel2 << 16) | (pixel1 << 8) | pixel1);
    }
#endif
}

static __inline void DoubleLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst)
{
    while (nSrcWidth--)
    {
        *((DWORD*)pDst)++ = (DWORD)((*pSrc << 16) | *pSrc++);
    }
}

static __inline void DoubleDirtyLine(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax, BYTE* pDst)
{
    BYTE    pixel1;
    BYTE    pixel2;
       
    while (x < nSrcXMax)
    {
        if (((x % (32)) == 0) && !IsDirtyDword(x, y))
        {
            x += 32;
            ((DWORD*)pDst) += 16; /* 32 pixels * (1 DWORD / 4 pixels) * 2 */
        }
        else
        {
            if (IsDirty2(x, y))
            {
                pixel1 = *(pSrc + x);
                pixel2 = *(pSrc + x + 1);

                *((DWORD*)pDst)++ = (DWORD)((pixel2 << 24) | (pixel2 << 16) | (pixel1 << 8) | pixel1);
            }
            else
            {
                ((DWORD*)pDst)++;
            }
            x += 2;
        }
    }
}

static __inline void ExpandLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst, BYTE bg)
{
    BYTE    pixel1;

    while (nSrcHalfWidth--)
    {
        pixel1 = *pSrc++;

        *((DWORD*)pDst)++ = (DWORD)((bg << 24) | (*pSrc++ << 16) | (bg << 8) | pixel1);
    }
}

static __inline void ExpandLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst, WORD bg)
{
    while (nSrcWidth--)
    {
        *((DWORD*)pDst)++ = (DWORD)((bg << 16) | *pSrc++);
    }
}

static __inline void ExpandDirtyLine(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax, BYTE* pDst, BYTE bg)
{   
    while (x < nSrcXMax)
    {
        if (((x % (32)) == 0) && !IsDirtyDword(x, y))
        {
            x += 32;
            ((DWORD*)pDst) += 16; /* 32 pixels * (1 DWORD / 4 pixels) * 2 */
        }
        else
        {
            if (IsDirty2(x, y))
            {
                *((DWORD*)pDst)++ = (DWORD)((bg << 24) | (*(pSrc + x + 1) << 16) | (bg << 8) | *(pSrc + x));
            }
            else
            {
                ((DWORD*)pDst)++;
            }
            x += 2;
        }
    }
}

static __inline void DoubleDirty2Lines(BYTE* pSrc, UINT x, UINT y, UINT nSrcXMax,
                                       BYTE* pDst, UINT nDstWidth)
{
    BYTE    pixel1;
    BYTE    pixel2;
    DWORD*  pDDst = (DWORD *) pDst;

    while (x < nSrcXMax)
    {
        if (((x % (32)) == 0) && !IsDirtyDword(x, y))
        {
            x += 32;
            pDDst += 16; /* 32 pixels * (1 DWORD / 4 pixels) * 2 */
        }
        else
        {
            if (IsDirty2(x, y))
            {
                pixel1 = *(pSrc + x);
                pixel2 = *(pSrc + x + 1);

                *pDDst = *(pDDst + nDstWidth) = (DWORD)((pixel2 << 24) | (pixel2 << 16) | (pixel1 << 8) | pixel1);
            }
            pDDst++;
            x += 2;
        }
    }
}

static __inline void LinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst)
{
    while (nSrcWidth--)
    {
        UINT32 d1, d2;
        d1 = m_p16BitLookup[*(pSrc++)];
        d2 = m_p16BitLookup[*(pSrc++)];

        *pDst++ = (d2 & 0xFFFF0000) | (d1 & 0x0000FFFF);
    }
}

static __inline void DoubleLinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst)
{
    while (nSrcWidth--)
        *pDst++ = m_p16BitLookup[*(pSrc++)];
}

static __inline void ExpandLinePalette16(WORD* pSrc, UINT nSrcWidth, DWORD* pDst, WORD bg)
{
    while (nSrcWidth--)
        *pDst++ = (m_p16BitLookup[*(pSrc++)] & 0xFFFF0000) | bg;
}
