/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DIRECTDRAW_H__
#define __DIRECTDRAW_H__

#include <ddraw.h>

/************************************************************************/
/* for borland */
#if defined(NONAMELESSUNION)

/*
 * DDPIXELFORMAT
 */
#define dwRGBBitCount u1.dwRGBBitCount
#define dwRBitMask    u2.dwRBitMask
#define dwGBitMask    u3.dwGBitMask
#define dwBBitMask    u4.dwBBitMask

/*
 * DDSURFACEDESC
 */
#define lPitch u1.lPitch

/*
 * DDBLTFX
 * Used to pass override information to the DIRECTDRAWSURFACE callback Blt.
 */
#define dwFillColor u5.dwFillColor

#endif
/************************************************************************/

extern LPDIRECTDRAW2 dd;

#define MAXMODES    256 /* Maximum number of DirectDraw Display modes. */

/* Display mode node */
struct tDisplayMode
{
    DWORD           m_dwWidth;
    DWORD           m_dwHeight;
    DWORD           m_dwBPP;
};

/* EnumDisplayMode Context */
struct tDisplayModes
{
    struct tDisplayMode m_Modes[MAXMODES];
    int                 m_nNumModes;
};

extern struct tDisplayModes*    DDraw_GetDisplayModes(void);

extern BOOL           DirectDraw_Initialize(void);
extern void           DirectDraw_CreateByIndex(int num_display);
struct tDisplayModes* DirectDraw_GetDisplayModes(void);
extern void           DirectDraw_Close(void);
extern int            DirectDraw_GetNumDisplays(void);
extern char*          DirectDraw_GetDisplayName(int num_display);

#endif
