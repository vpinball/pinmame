/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Debug.h

 ***************************************************************************/

#ifndef MAMEDEBUG_H
#define MAMEDEBUG_H

#if defined(MAME_DEBUG)

extern int  MAME32Debug_init(options_type *options);
extern void MAME32Debug_exit(void);
extern void MAME32Debug_close_display(void);
extern void MAME32Debug_update_display(struct osd_bitmap *debug_bitmap);
extern int  MAME32Debug_create_display(int width, int height, int depth, int fps, int attributes, int orientation);
extern int  MAME32Debug_allocate_colors(int modifiable, const UINT8* debug_palette, UINT32* debug_pens);
extern void MAME32Debug_set_debugger_focus(int debugger_has_focus);

#else

#define MAME32Debug_init(options) 0
#define MAME32Debug_exit()
#define MAME32Debug_close_display()
#define MAME32Debug_update_display(debug_bitmap)
#define MAME32Debug_create_display(width, height, depth, fps, attributes, orientation) 0
#define MAME32Debug_allocate_colors(modifiable, debug_palette, debug_pens) 0
#define MAME32Debug_set_debugger_focus(debugger_has_focus)

#endif

#endif /* MAMEDEBUG_H */
