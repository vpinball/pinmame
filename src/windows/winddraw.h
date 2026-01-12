//============================================================
//
//	winddraw.h - Win32 DirectDraw code
//
//============================================================
#pragma once

#include "window.h"

//============================================================
//	PROTOTYPES
//============================================================

int win_ddraw_init(int width, int height, int depth, int attributes, const struct win_effect_data *effect);
void win_ddraw_kill(void);
int win_ddraw_draw(struct mame_bitmap *bitmap, const struct rectangle *bounds, int update);
void win_ddraw_wait_vsync(void);
void win_ddraw_fullscreen_margins(DWORD desc_width, DWORD desc_height, RECT *margins);
