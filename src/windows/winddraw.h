//============================================================
//
//	winddraw.h - Win32 DirectDraw code
//
//============================================================

#ifndef __WIN32_DDRAW__
#define __WIN32_DDRAW__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "window.h"


//============================================================
//	PROTOTYPES
//============================================================

int win_ddraw_init(int width, int height, int depth, int attributes, const struct win_effect_data *effect);
void win_ddraw_kill(void);
int win_ddraw_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, void *vector_dirty_pixels, int update);
void win_ddraw_wait_vsync(void);
void win_ddraw_fullscreen_margins(DWORD desc_width, DWORD desc_height, RECT *margins);



#endif
