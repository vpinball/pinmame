/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997 Michael Soderstrom and Chris Kirmse
    
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

#if defined(MAME_MMX)

#pragma warning(disable:4799) /* function has no emms instruction */

#define RENDERARGS struct osd_bitmap* pSrcBitmap, UINT nSrcStartLine, UINT nSrcStartColumn, UINT nNumLines, UINT nNumColumns, BYTE* pDst, UINT nDstWidth

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static void RenderDoubleBitmap(RENDERARGS);
static void RenderDoubleHScanlinesBitmap(RENDERARGS);
static void RenderDoubleVScanlinesBitmap(RENDERARGS); 

static void RenderDirtyBitmap(RENDERARGS); 
static void RenderDirtyDoubleBitmap(RENDERARGS); 
static void RenderDirtyDoubleHScanlinesBitmap(RENDERARGS);

static void RenderDoubleBitmap16(RENDERARGS); 
static void RenderDoubleHScanlinesBitmap16(RENDERARGS); 
static void RenderDoubleVScanlinesBitmap16(RENDERARGS); 

static void RenderDirtyBitmap16(RENDERARGS); 

static __inline void    DoubleLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst);
static __inline void    ExpandLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst, INT64 bg);

static __inline void    DoubleLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst);
static __inline void    ExpandLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst);

static __inline INT64   mmx_or(INT64 m1, INT64 m2);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static const UINT32*  m_p16BitLookup;

/***************************************************************************
    External function definitions
 ***************************************************************************/

RenderMethod SelectRenderMethodMMX(BOOL bDouble, BOOL bHScanLines, BOOL bVScanLines,
                                   enum DirtyMode eDirtyMode, BOOL b16bit, BOOL bPalette16,
                                   const UINT32* p16BitLookup)
{
    RenderMethod Render = NULL;

    if (bDouble == TRUE)
    {
        if (bHScanLines == TRUE)
        {
            if (b16bit == TRUE)
            {
                if (eDirtyMode == USE_DIRTYRECT)
                    Render = NULL;
                else
                    Render = RenderDoubleHScanlinesBitmap16;
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
                if (eDirtyMode == USE_DIRTYRECT)
                    Render = NULL;
                else
                    Render = RenderDoubleVScanlinesBitmap16;
            }
            else
            {
                if (eDirtyMode == USE_DIRTYRECT)
                    Render = NULL;
                else
                    Render = RenderDoubleVScanlinesBitmap;
            }
        }
        else
        {
            if (b16bit == TRUE)
            {
                if (eDirtyMode == USE_DIRTYRECT)
/*
                    Render = RenderDirtyDoubleBitmap16;
*/
                    Render = RenderDoubleBitmap16;
                else
                    Render = RenderDoubleBitmap16;
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
        if (b16bit == TRUE)
        {
            if (eDirtyMode == USE_DIRTYRECT)
                Render = RenderDirtyBitmap16;
            else
                Render = NULL;
        }
        else
        {
            if (eDirtyMode == USE_DIRTYRECT)
                Render = RenderDirtyBitmap;
            else
                Render = NULL ;
        }
    }

    /* Set up the 16 bit palette lookup table. */
    if (bPalette16 == TRUE)
        m_p16BitLookup = p16BitLookup;

    return Render;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

#define DO_PIXELS8                                                  \
{                                                                   \
    *pdwDst = *(INT64 *)(pSrcBitmap->line[y] + x +nSrcStartColumn); \
}                                                                   \
x += 8;                                                             \
pdwDst++;                                                           \
if (x >= nNumColumns)                                               \
    goto RDBEnd;

static void RenderDirtyBitmap(RENDERARGS)
{
    UINT y;

    if (nSrcStartColumn & 0x7)
    {
        nNumColumns     -= 8 - (nSrcStartColumn & 0x7);
        nSrcStartColumn += 8 - (nSrcStartColumn & 0x7);
    }
    nNumColumns -= nNumColumns & 0x7;

    for (y = nSrcStartLine; y < nNumLines + nSrcStartLine; y++)
    {
        if (IsDirtyLine(y))
        {
            INT64 *dirty1 = (INT64 *) ((BYTE *)(dirty_buffer + y * dirty_width) + (nSrcStartColumn >> 3));
            INT64 *dirty;
            char sixtyfour[8];
            UINT x = 0;
            INT64 *pdwDst = (INT64 *)(pDst + (y - nSrcStartLine) * nDstWidth);
            
            dirty = (INT64 *) sixtyfour;

            while (x < nNumColumns)
            {
                *dirty = *dirty1;

                dirty1++;

                if (((int *)sixtyfour)[0])
                {
                    if (sixtyfour[0])
                        DO_PIXELS8;
                    if (sixtyfour[1])
                        DO_PIXELS8;
                    if (sixtyfour[2])
                        DO_PIXELS8;
                    if (sixtyfour[3])
                        DO_PIXELS8;
                }
                else
                {
                    x += 32;
                    pdwDst +=4;
                }

                if (((int *)sixtyfour)[1])
                {
                    if (sixtyfour[4])
                        DO_PIXELS8;
                    if (sixtyfour[5])
                        DO_PIXELS8;
                    if (sixtyfour[6])
                        DO_PIXELS8;
                    if (sixtyfour[7])
                    {
                        *pdwDst = *(INT64 *)(pSrcBitmap->line[y] + x + nSrcStartColumn);
                    }
                    x += 8;
                    pdwDst++;
                }
                else
                {
                    x += 32;
                    pdwDst +=4;
                }
           }
        }
RDBEnd:
        ;
    }
}

#define DO_PIXELS16      \
{                        \
    *pdwDst++ = *pSrc++; \
    *pdwDst++ = *pSrc++; \
}                        \
else                     \
{                        \
    pdwDst += 2;         \
    pSrc   += 2;         \
}                        \
x += 8;                  \
if (x >= nNumColumns)    \
    goto RDBEnd16;

static void RenderDirtyBitmap16(RENDERARGS)
{
    UINT y;
    if (nSrcStartColumn & 0x7)
    {
        nNumColumns     -= 8 - (nSrcStartColumn & 0x7);
        nSrcStartColumn += 8 - (nSrcStartColumn & 0x7);
    }
    nNumColumns -= nNumColumns & 0x7;
    
    for (y = nSrcStartLine; y < nNumLines + nSrcStartLine; y++)
    {
        if (IsDirtyLine(y))
        {       
            INT64 *dirty1 = (INT64 *) (((BYTE*)(dirty_buffer + y * dirty_width)) + (nSrcStartColumn >> 3));
            INT64 *dirty;
            char sixtyfour[8];
            UINT x  = 0;
            INT64 *pdwDst = (INT64 *)(pDst + (y - nSrcStartLine) * nDstWidth);
            INT64 *pSrc = (INT64 *)(pSrcBitmap->line[y] + nSrcStartColumn * 2);
            
            dirty = (INT64 *) sixtyfour;

            while (x < nNumColumns)
            {
                *dirty = *dirty1;

                dirty1++;

                if (((int *)sixtyfour)[0])
                {
                    if (sixtyfour[0])
                        DO_PIXELS16;
                    if (sixtyfour[1])
                        DO_PIXELS16;
                    if (sixtyfour[2])
                        DO_PIXELS16;
                    if (sixtyfour[3])
                        DO_PIXELS16;
                }
                else
                {
                    x += 32;
                    pdwDst +=8;
                    pSrc +=8;
                }

                if (((int *)sixtyfour)[1])
                {
                    if (sixtyfour[4])
                        DO_PIXELS16;
                    if (sixtyfour[5])
                        DO_PIXELS16;
                    if (sixtyfour[6])
                        DO_PIXELS16;
                    if (sixtyfour[7])
                    {
                        *pdwDst++ = *pSrc++; 
                        *pdwDst++ = *pSrc++; 
                    }
                    else
                    {
                        pdwDst +=2;
                        pSrc   +=2;
                    }
                    x += 8;
                }
                else
                {
                    x += 32;
                    pdwDst += 8;
                    pSrc   += 8;
                }
           }
        }
RDBEnd16:
        ;
    }
}

static void RenderDoubleBitmap(RENDERARGS)
{
    UINT    nDoubleDstWidth = nDstWidth << 1;
    int     nSrcQrtWidth    = nNumColumns >> 3;
    UINT    nDstQrtWidth    = nDstWidth >> 3;

    while (nNumLines--)
    {
        DoubleLine(pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nSrcQrtWidth, pDst);
        pDst += nDstWidth;

        DoubleLine(pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nSrcQrtWidth, pDst);
        pDst += nDstWidth;
    
        nSrcStartLine++;
    }

    __asm emms
}

static void RenderDoubleBitmap16(RENDERARGS)
{
    int nQrtWidth = nNumColumns >> 2;

    while (nNumLines--)
    {
        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nQrtWidth, pDst);
        pDst += nDstWidth;

        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nQrtWidth, pDst);
        pDst += nDstWidth;

        nSrcStartLine++;
    }
    __asm emms
}


static void RenderDirtyDoubleBitmap(RENDERARGS)
{
    UINT nDstDoubleWidth = nDstWidth << 1;

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
        {
            INT64   mSrc;
            INT64*  pDDst = (INT64 *) pDst;
            BYTE*   pSrc = pSrcBitmap->line[nSrcStartLine];
            UINT    x = nSrcStartColumn;
            UINT    nSrcXMax = nSrcStartColumn + nNumColumns;

            while (x < nSrcXMax)
            {
                if (((x % (32)) == 0) && !IsDirtyDword(x, nSrcStartLine))
                {
                    x += 32;
                    pDDst += 8; /* 32 pixels * (1 DWORD / 4 pixels) * 2 */
                }
                else
                {
                    if (IsDirty8(x, nSrcStartLine))
                    {
                        mSrc = *((INT64 *)(pSrc + x));
                        __asm
                        {
                            mov         edi, pDDst

                            movq        mm0, mSrc

                            movq        mm1, mSrc
                            punpcklbw   mm0, mm0

                            movq        [edi], mm0
                            punpckhbw   mm1, mm1

                            movq        [edi + 8], mm1

                            add         edi, nDstWidth

                            movq        [edi], mm0
                            add         edi, 8

                            movq        [edi], mm1
                        }
                    }
                    pDDst += 2;           
                    x += 8;
                }
            }               
        }
        nSrcStartLine++;
        pDst += nDstDoubleWidth;
    }
    __asm emms
}

static void RenderDirtyDoubleHScanlinesBitmap(RENDERARGS)
{
    UINT nDstDoubleWidth = nDstWidth << 1;

    while (nNumLines--)
    {
        if (IsDirtyLine(nSrcStartLine))
        {
            INT64   mSrc;
            INT64*  pDDst = (INT64 *) pDst;
            BYTE*   pSrc = pSrcBitmap->line[nSrcStartLine];
            UINT    x = nSrcStartColumn;
            UINT    nSrcXMax = nSrcStartColumn + nNumColumns;

            while (x < nSrcXMax)
            {
                if (((x % (32)) == 0) && !IsDirtyDword(x, nSrcStartLine))
                {
                    x += 32;
                    pDDst += 8; /* 32 pixels * (1 DWORD / 4 pixels) * 2 */
                }
                else
                {
                    if (IsDirty8(x, nSrcStartLine))
                    {
                        mSrc = *((INT64 *)(pSrc + x));
                        __asm
                        {
                            mov         edi, pDDst

                            movq        mm0, mSrc

                            movq        mm1, mSrc
                            punpcklbw   mm0, mm0

                            movq        [edi], mm0
                            punpckhbw   mm1, mm1

                            movq        [edi + 8], mm1
                        }
                    }
                    pDDst += 2;           
                    x += 8;
                }
            }               
        }
        nSrcStartLine++;
        pDst += nDstDoubleWidth;
    }
    __asm emms
}

static void RenderDoubleHScanlinesBitmap(RENDERARGS)
{
    int nDstDoubleWidth = nDstWidth << 1;
    int nSrcHalfWidth   = nNumColumns >> 3;

    while (nNumLines--)
    {
        DoubleLine(pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn, nSrcHalfWidth, pDst);
        nSrcStartLine++;
        pDst += nDstDoubleWidth;
    }
    __asm emms
}

static void RenderDoubleHScanlinesBitmap16(RENDERARGS)
{
    int nDstDoubleWidth = nDstWidth << 1;
    int nQrtWidth       = nNumColumns >> 2;

    while (nNumLines--)
    {
        DoubleLine16((WORD*)pSrcBitmap->line[nSrcStartLine++] + nSrcStartColumn, nQrtWidth, pDst);
        pDst += nDstDoubleWidth;
    }
    __asm emms
}

static void RenderDoubleVScanlinesBitmap(RENDERARGS)
{
    int     nSrcHalfWidth = nNumColumns >> 3;
    DWORD   dwBlackPen;
    INT64   mBlackPen;

    dwBlackPen = (DWORD)MAME32App.m_pDisplay->GetBlackPen();
    __asm
    {
        movq        mm2, dwBlackPen
        punpcklbw   mm2, mm2
        punpcklwd   mm2, mm2
        punpckldq   mm2, mm2
    }

    while (nNumLines--)
    {
        BYTE* b = pSrcBitmap->line[nSrcStartLine] + nSrcStartColumn;

        ExpandLine(b, nSrcHalfWidth, pDst, mBlackPen);
        pDst += nDstWidth;

        ExpandLine(b, nSrcHalfWidth, pDst, mBlackPen);
        pDst += nDstWidth;
        nSrcStartLine++;
    }
    __asm emms
}

static void RenderDoubleVScanlinesBitmap16(RENDERARGS)
{
    DWORD   dwBlackPen;

    nNumColumns >>= 2;

    dwBlackPen = (DWORD)MAME32App.m_pDisplay->GetBlackPen();
    __asm
    {
        movq        mm2, dwBlackPen
        punpcklwd   mm2, mm2
        punpckldq   mm2, mm2
    }

    while (nNumLines--)
    {
        WORD* pSrc = ((WORD*)pSrcBitmap->line[nSrcStartLine]) + nSrcStartColumn;

        ExpandLine16(pSrc, nNumColumns, pDst);
        pDst += nDstWidth;

        ExpandLine16(pSrc, nNumColumns, pDst);
        pDst += nDstWidth;

        nSrcStartLine++;
    }
    __asm emms
}

/* support functions */

static __inline void DoubleLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst)
{
    __asm
    {
        mov         esi, pSrc
        mov         edi, pDst

        mov         eax, nSrcHalfWidth
L1:
        movq        mm0, [esi]

        movq        mm1, [esi]
        punpcklbw   mm0, mm0

        movq        [edi], mm0
        add         edi, 8

        punpckhbw   mm1, mm1
        add         esi, 8

        movq        [edi], mm1
        add         edi, 8

        dec         eax
        jnz         L1
    }
}

static __inline void DoubleLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst)
{
    __asm
    {
        mov         esi, pSrc
        mov         edi, pDst

        mov         eax, nSrcWidth
        sub         edi, 16
L1:
        movq        mm0, [esi]
        add         edi, 16

        movq        mm1, [esi]
        punpcklwd   mm0, mm0

        movq        [edi], mm0
        punpckhwd   mm1, mm1

        movq        [edi + 8], mm1
        add         esi, 8

        dec         eax
        jnz         L1
    }
}

static __inline void ExpandLine(BYTE* pSrc, UINT nSrcHalfWidth, BYTE* pDst, INT64 bg)
{
    __asm
    {
        mov         esi, pSrc
        mov         edi, pDst

        mov         eax, nSrcHalfWidth
L1:
        movq        mm0, [esi]

        movq        mm1, [esi]
        punpcklbw   mm0, mm2

        movq        [edi], mm0
        add         esi, 8

        punpckhbw   mm1, mm2
        add         edi, 8

        movq        [edi], mm1
        add         edi, 8

        dec         eax
        jnz         L1
    }
}

static __inline void ExpandLine16(WORD* pSrc, UINT nSrcWidth, BYTE* pDst)
{
    __asm
    {
        mov         esi, pSrc
        mov         edi, pDst

        mov         eax, nSrcWidth
        sub         edi, 16
L1:
        movq        mm0, [esi]
        add         edi, 16

        movq        mm1, [esi]
        punpcklwd   mm0, mm2

        movq        [edi], mm0
        punpckhwd   mm1, mm2

        movq        [edi + 8], mm1
        add         esi, 8

        dec         eax
        jnz         L1
    }
}

static __inline INT64 mmx_or(INT64 m1, INT64 m2)
{
    INT64 m;

    __asm
    {
        movq    mm0, m1
        por     mm0, m2
        movq    m, mm0
    }

    return m;
}

/* MAME_MMX */
#else
/* stub to allow compilation with no MMX */
RenderMethod SelectRenderMethodMMX(BOOL bDouble, BOOL bHScanlines, BOOL bVScanlines,
                                   enum DirtyMode eDirtyMode, BOOL b16bit, BOOL bPalette16,
                                   const UINT32* p16BitLookup)
{
    return NULL;
}
#endif
