/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "osdepend.h"
#include "ControllerOptions.h"

typedef struct
{
    DWORD       m_Top;
    DWORD       m_Left;
    DWORD       m_Width;
    DWORD       m_Height;
} tRect;

#define OSD_NUMPENS     (256)

struct OSDDisplay
{
    int                 (*init)(PCONTROLLEROPTIONS pControllerOptions);
    void                (*exit)(void);
    struct osd_bitmap*  (*alloc_bitmap)(int width, int height, int depth);
    void                (*free_bitmap)(struct osd_bitmap* bitmap);
    int                 (*create_display)(int width, int height, int depth, int fps, int attributes, int orientation);
    void                (*close_display)(void);
    void                (*set_visible_area)(int min_x, int max_x, int min_y, int max_y);
    void                (*set_debugger_focus)(int debugger_has_focus);
#if MAMEVER < 3709
    int                 (*allocate_colors)(unsigned int totalcolors, const UINT8 *palette, UINT16 *pens, int modifiable, const UINT8 *debug_palette, UINT16 *debug_pens);
#else
    int                 (*allocate_colors)(unsigned int totalcolors, const UINT8 *palette, UINT32 *pens, int modifiable, const UINT8 *debug_palette, UINT32 *debug_pens);
#endif
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

#ifdef __cplusplus
extern "C" {
#endif

extern float Display_get_gamma(void);
extern int   Display_get_brightness(void);
extern void  Display_MapColor(unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue);
extern void  Display_WriteBitmap(struct osd_bitmap* tBitmap, PALETTEENTRY* pPalEntries);
extern BOOL  Display_Throttled(void);

extern struct OSDDisplay Display;

#ifdef __cplusplus
}
#endif

#endif
