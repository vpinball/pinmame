/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include "osdepend.h"

typedef struct
{
    DWORD   m_Top;
    DWORD   m_Left;
    DWORD   m_Width;
    DWORD   m_Height;
} tRect;

#define OSD_NUMPENS (256)

struct OSDDisplay
{
    int                 (*init)(options_type *options);
    void                (*exit)(void);
    struct osd_bitmap*  (*alloc_bitmap)(int width, int height, int depth);
    void                (*free_bitmap)(struct osd_bitmap* bitmap);
    int                 (*create_display)(int width, int height, int depth, int fps, int attributes, int orientation);
    void                (*close_display)(void);
    void                (*set_visible_area)(int min_x, int max_x, int min_y, int max_y);
    void                (*set_debugger_focus)(int debugger_has_focus);
    int                 (*allocate_colors)(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
    void                (*modify_pen)(int pen, unsigned char red, unsigned char green, unsigned char blue);
    void                (*get_pen)(int pen, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
    void                (*mark_dirty)(int x1, int y1, int x2, int y2);
    int                 (*skip_this_frame)(void);
    void                (*update_display)(struct osd_bitmap *game_bitmap, struct osd_bitmap *debug_bitmap);
    void                (*led_w)(int leds_status);
    void                (*set_gamma)(float gamma);
    float               (*get_gamma)(void);
    void                (*set_brightness)(int brightness);
    int                 (*get_brightness)(void);
    void                (*save_snapshot)(struct osd_bitmap *bitmap);

    BOOL                (*OnMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    void                (*Refresh)(void);
    int                 (*GetBlackPen)(void);
    void                (*UpdateFPS)(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS);
};

extern float Display_get_gamma(void);
extern int   Display_get_brightness(void);
extern void  Display_MapColor(unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
extern void  Display_WriteBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries);
extern BOOL  Display_Throttled(void);

extern struct OSDDisplay OSDDisplay;

#endif /* DISPLAY_H */
