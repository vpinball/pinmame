#include "xmame.h"
#include "driver.h"

#ifdef xgl
        extern void gl_dirty_init(void);
        extern void gl_dirty_close(void);
	extern void gl_mark_dirty(int x1, int y1, int x2, int y2);
#endif

/* hmm no more way to find out what the width and height of the screenbitmap
   are, so just define WIDTH and HEIGHT to be 2048 */

#define WIDTH  (2048 / 8)
#define HEIGHT (2048 / 8)

int osd_dirty_init(void)
{
   dirty_lines      = NULL;
   dirty_blocks     = NULL;
   
   /* vector games always need a dirty array */
   if (use_dirty || (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
   {
      int i;
      
      dirty_lines = malloc(HEIGHT);
      if (!dirty_lines)
      {
         fprintf(stderr_file, "Error: couldn't allocate mem\n");
         return OSD_NOT_OK;
      }
      memset(dirty_lines, 0, HEIGHT);
   	
      dirty_blocks = malloc(HEIGHT * sizeof(char *));
      if (!dirty_blocks)
      {
         free(dirty_lines); dirty_lines = NULL;
         fprintf(stderr_file, "Error: couldn't allocate mem\n");
         return OSD_NOT_OK;
      }
   	   
      for (i=0; i< HEIGHT; i++)
      {
         dirty_blocks[i] = malloc(WIDTH);
         if (!dirty_blocks[i]) break;
         memset(dirty_blocks[i], 0, WIDTH);
      }
      if (i!=HEIGHT)
      { 
         fprintf(stderr_file, "Error: couldn't allocate mem\n");
         for(;i>=0;i--) free(dirty_blocks[i]);
         free(dirty_blocks); dirty_blocks = NULL;
         free(dirty_lines);  dirty_lines  = NULL;
         return OSD_NOT_OK;
      }
   }
   #ifdef xgl
      if(use_dirty) gl_dirty_init();
   #endif
   
   return OSD_OK;
}

void osd_dirty_close(void)
{
   /* vector games always need a dirty array */
   if (use_dirty || (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
   {
      int i;
      
      if (!dirty_blocks) return;
      
      for (i=0; i< HEIGHT; i++) free(dirty_blocks[i]);
      free (dirty_blocks);
      free (dirty_lines);
   }
   #ifdef xgl
      if(use_dirty) gl_dirty_close();
   #endif
}

void osd_mark_dirty(int x1, int y1, int x2, int y2)
{
	int y,x;
	if (use_dirty)
	{
	   if (x1 < visual.min_x) x1=visual.min_x;
	   if (y1 < visual.min_y) y1=visual.min_y;
	   if (x2 > visual.max_x) x2=visual.max_x;
	   if (y2 > visual.max_y) y2=visual.max_y;
	   x1 >>= 3;
	   y1 >>= 3;
	   x2 = (x2 + 8) >> 3;
	   y2 = (y2 + 8) >> 3;
 	   for (y=y1; y<y2; y++)
	   {
	      dirty_lines[y] = 1;
	      for(x=x1; x<x2; x++)
              {
	         dirty_blocks[y][x] = 1;
	      }
	   }
           #ifdef xgl
	       gl_mark_dirty(x1, y1, x2, y2);
           #endif
	}
}
