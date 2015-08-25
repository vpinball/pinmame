/* this file is used by blit.h don't use it directly ! */

#define DEST_SCALE_X(X)   DEST_SCALE(SCALE_X(X))
#define DEST_SCALE_X_8(X) DEST_SCALE(SCALE_X(X) * 8)
#define DEST_SCALE_Y(Y)   DEST_SCALE(SCALE_Y(Y))
#define DEST_SCALE_Y_8(Y) DEST_SCALE(SCALE_Y(Y) * 8)

switch(use_dirty)
{
   case 0: /* non dirty */
   {
#ifdef DEST
      int src_width = (((SRC_PIXEL *)bitmap->line[1]) -
         ((SRC_PIXEL *)bitmap->line[0]));
      SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[src_y] + src_x;
      SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[src_y + src_height] +
         src_x + src_width;
      DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
      
      for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH),
         line_src+=src_width)
         COPY_LINE(line_src, line_src+src_width, line_dest)
#endif
#ifdef PUT_IMAGE
      PUT_IMAGE(src_x, src_y, dest_x, dest_y,
         SCALE_X(src_width), SCALE_Y(src_height))
#endif      
      break;
   }
   case 1: /* normal dirty */
      osd_dirty_merge();
   case 2: /* vector */
   {
      int y, max_y = (src_y + src_width) >> 3;
#ifdef DEST
      DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
      for (y = src_y>>3; y < max_y; y++, line_dest += DEST_SCALE_Y_8(DEST_WIDTH))
#else
      for (y = src_y>>3; y < max_y; y++)
#endif      
      {
         if (bitmap->dirty_lines[y])
         {
            int x, max_x;
            max_x = (src_x + src_width) >> 3;
            for(x = src_x>>3; x < max_x; x++)
            {
               if (bitmap->dirty_blocks[y][x])
               {
                  int min_x;
#ifdef DEST
                  int max_x, h, max_h;
                  DEST_PIXEL *block_dest = line_dest + DEST_SCALE_X_8(x);
#endif
                  min_x = x << 3;
                  do {
                     bitmap->dirty_blocks[y][x]=0;
                     x++;
                  } while (bitmap->dirty_blocks[y][x]);
#ifdef DEST                  
                  max_x = x << 3;
                  h     = y << 3;
                  max_h = h + 8;
                  for (; h<max_h; h++, block_dest += DEST_SCALE_Y(DEST_WIDTH))
                     COPY_LINE((SRC_PIXEL *)bitmap->line[h]+min_x,
                        (SRC_PIXEL *)bitmap->line[h]+max_x, block_dest)
#endif
#ifdef PUT_IMAGE
                  PUT_IMAGE(
                     min_x,
                     y << 3,
                     dest_x + SCALE_X(min_x - src_x),
                     dest_y + SCALE_Y((y << 3) - src_y),
                     SCALE_X((x<<3) - min_x),
                     SCALE_Y(8))
#endif
               }
            }
            bitmap->dirty_lines[y] = 0;
         }
      }
      break;
   }
}

#undef DEST_SCALE_X
#undef DEST_SCALE_X_8
#undef DEST_SCALE_Y
#undef DEST_SCALE_Y_8
