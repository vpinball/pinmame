#ifdef xgl
#include "gldirty.h"

const Rectangle nullRect = { 0, 0, 0, 0};

Rectangle firstDirtyRect;

int       dirtyRectNumber = 0;  

int 	  screendirty;	/* Has the bitmap been modified since the last frame? */

int gl_dirty_init()
{
   firstDirtyRect = nullRect;
   dirtyRectNumber = 0;

   return OSD_OK;
}

void gl_dirty_close(void)
{
   firstDirtyRect = nullRect;
   dirtyRectNumber = 0;
}

void gl_mark_dirty(int x1, int y1, int x2, int y2)
{
	screendirty = 1;

	if (!use_dirty)
		return;

	if (x1 < visual.min_x) x1=visual.min_x;
	if (y1 < visual.min_y) y1=visual.min_y;
	if (x2 > visual.max_x) x2=visual.max_x;
	if (y2 > visual.max_y) y2=visual.max_y;

	if(dirtyRectNumber==1)
	{
		/**
		 * if the firstDirtyRect is _not_
		 * included by the new one,
		 * increase dirtyRectNumber (to skip the dirty usage)
		 * and exit .. :-(
		 */
		if( !
		    (
		     firstDirtyRect.min_x>=x1 &&
		     firstDirtyRect.min_y>=y1 &&
		     firstDirtyRect.max_x<=x2 &&
		     firstDirtyRect.max_y<=y2
		    )
		  ) 
		{
			dirtyRectNumber++;
			return;
		}
		/**
		 * if the firstDirtyRect includes the new one,
		 * just ignore the new one ;-)
		 *
		 * otherwise, we use the bigger new one ..
		 */
		if(  firstDirtyRect.min_x<=x1 &&
		     firstDirtyRect.min_y<=y1 &&
		     firstDirtyRect.max_x>=x2 &&
		     firstDirtyRect.max_y>=y2
		  )
		  return;

	} else {
		/**
		 * the first dirty rectangle ..
		 */
		dirtyRectNumber++;
	}

	firstDirtyRect.min_x=x1;
	firstDirtyRect.min_y=y1;
	firstDirtyRect.max_x=x2;
	firstDirtyRect.max_y=y2;
}

#endif /* #if !defined xgl */

