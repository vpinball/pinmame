/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DIRTY_H__
#define __DIRTY_H__

#include "mame.h"

enum DirtyMode { NO_DIRTY = 0, USE_DIRTYRECT };

extern void InitDirty(int width, int height, enum DirtyMode);
extern void ExitDirty(void);

extern void MarkDirty(int x1, int y1, int x2, int y2);
extern void MarkDirtyPixel(int x, int y);
extern void MarkAllDirty(void);
extern void ClearDirty(void);

extern int dirty_width;
extern int dirty_height;
extern int *dirty_buffer;
extern int *dirty_line;

#define DIRTYDWORD(x, y)    (    (y) * dirty_width + ((x) / 32))
#define DIRTYBYTE(x, y)     (4 * (y) * dirty_width + ((x) / 8))
#define DIRTYBIT(x, y)      (0x01 << ((x) % 32))
#define DIRTYMASK8(x, y)    (0xFF << ((x) % 32))
#define DIRTYMASK4(x, y)    (0x0F << ((x) % 32))
#define DIRTYMASK2(x, y)    (0x03 << ((x) % 32))

#define IsDirtyLine(y)  (dirty_line[y])

#define IsDirty(x, y)   (dirty_buffer[DIRTYDWORD(x, y)] & DIRTYBIT(x, y))
#define IsDirty2(x, y)  (dirty_buffer[DIRTYDWORD(x, y)] & DIRTYMASK2(x, y))
#define IsDirty4(x, y)  (dirty_buffer[DIRTYDWORD(x, y)] & DIRTYMASK4(x, y))
#define IsDirty8(x, y)  (dirty_buffer[DIRTYDWORD(x, y)] & DIRTYMASK8(x, y))

#define IsDirtyDword(x, y) (dirty_buffer[DIRTYDWORD(x, y)])

#endif
