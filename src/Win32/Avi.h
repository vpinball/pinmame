/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef MAME32AVI_H
#define MAME32AVI_H

#if defined(MAME_AVI)

BOOL    AviStartCapture(char* filename, int width, int height, int depth);
void    AviEndCapture(void);

void    AviAddBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries);

void    SetAviFPS(int fps);
BOOL    GetAviCapture(void);

#else

#define AviStartCapture(filename, width, height, depth) 0
#define AviEndCapture()

#define AviAddBitmap(tBitmap, pPalEntries)

#define SetAviFPS(fps)
#define GetAviCapture() 0;

#endif /* MAME_AVI */

#endif /* MAME32AVI_H */
