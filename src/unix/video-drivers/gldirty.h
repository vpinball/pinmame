#ifndef GLDIRTY_H
	#define GLDIRTY_H

	#include "xmame.h"
	#include "driver.h"

	#include "glmame.h"

	#include "drawgfx.h"

	typedef struct rectangle Rectangle;

	extern const Rectangle nullRect;
	extern       Rectangle firstDirtyRect;
	extern       int       dirtyRectNumber;
	extern       int       screendirty;

	int gl_dirty_init();
	void gl_dirty_close(void);
	void gl_mark_dirty(int x1, int y1, int x2, int y2);
#endif


