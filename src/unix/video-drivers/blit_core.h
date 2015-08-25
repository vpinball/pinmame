/* this file is used by blit.h -- don't use it directly ! */

#ifdef DEST_SCALE
#define DEST_SCALE_X(X)   (SCALE_X(X) * DEST_SCALE)
#define DEST_SCALE_Y(Y)   (SCALE_Y(Y) * DEST_SCALE)
#else
#define DEST_SCALE_X(X)   SCALE_X(X)
#define DEST_SCALE_Y(Y)   SCALE_Y(Y)
#endif

if (effect) {
  switch(effect) {

  default:
  case EFFECT_SCALE2X:
    {
#  ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) {
	       rotate_func(rotate_dbbuf0, bitmap, y-1);
	       rotate_func(rotate_dbbuf1, bitmap, y);
	       rotate_func(rotate_dbbuf2, bitmap, y+1);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scale2x_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_func(line_dest, line_dest+DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scale2x_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_direct_func(line_dest, line_dest+DEST_WIDTH, rotate_dbbuf0, rotate_dbbuf1, rotate_dbbuf2, visual_width);
#		endif
#	endif
       }
     } else {
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scale2x_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src-src_width, line_src, line_src+src_width, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_func(line_dest, line_dest+DEST_WIDTH, line_src-src_width, line_src, line_src+src_width, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scale2x_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src-src_width, line_src, line_src+src_width, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scale2x_direct_func(line_dest, line_dest+DEST_WIDTH, line_src-src_width, line_src, line_src+src_width, visual_width);
#		endif
#	endif
	}
     }
#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_SCAN2:
      {
#  ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan2_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan2_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
       }
     } else {
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan2_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan2_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_scan2_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
	}
     }
#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_RGBSTRIPE:
      {
#  ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
       }
     } else {
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbstripe_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*2);
#		else
		  effect_rgbstripe_direct_func(line_dest, line_dest+DEST_WIDTH, line_src, visual_width);
#		endif
#	endif
	}
     }
#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

    case EFFECT_RGBSCAN:
      {
#  ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
       }
     } else {
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_rgbscan_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_rgbscan_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
	}
     }
#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
  }
  break;

 case EFFECT_SCAN3:
  {
#  ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=DEST_SCALE_Y(DEST_WIDTH), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan3_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan3_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
       }
     } else {
	int src_width = (((SRC_PIXEL *)bitmap->line[1]) - ((SRC_PIXEL *)bitmap->line[0]));
	SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
	SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
	DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);

	for (;line_src < line_end; line_dest+=DEST_SCALE_Y(DEST_WIDTH), line_src+=src_width) {

#	ifdef INDIRECT
#		ifdef DOUBLEBUFFER
		  effect_scan3_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width, INDIRECT);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width, INDIRECT);
#		endif
#	else
#		ifdef DOUBLEBUFFER
		  effect_scan3_direct_func(effect_dbbuf, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE, effect_dbbuf+DEST_WIDTH*DEST_PIXEL_SIZE*2, line_src, visual_width);
		  memcpy(line_dest, effect_dbbuf, DEST_WIDTH*DEST_PIXEL_SIZE*3);
#		else
		  effect_scan3_direct_func(line_dest, line_dest+DEST_WIDTH, line_dest+DEST_WIDTH*2, line_src, visual_width);
#		endif
#	endif
	}
     }
#  endif
#  ifdef PUT_IMAGE
      PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#  endif      
	}
      break;
    }
  }
else /* no effect */
{
#ifdef DEST
     if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy)) {
       int y;
       SRC_PIXEL *line_src;
       SRC_PIXEL *line_end;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
       for (y = visual.min_y; y <= visual.max_y; line_dest+=REPS_FOR_Y(DEST_WIDTH,y,visual_height), y++) {
	       rotate_func(rotate_dbbuf, bitmap, y);
	       line_src = (SRC_PIXEL *)rotate_dbbuf;
	       line_end = (SRC_PIXEL *)rotate_dbbuf + visual_width;
	       COPY_LINE_FOR_Y(y, visual_height, line_src, line_end, line_dest);
       }
     } else {
       int y = visual.min_y;
       int src_width = (((SRC_PIXEL *)bitmap->line[1]) -
			((SRC_PIXEL *)bitmap->line[0]));
       SRC_PIXEL *line_src = (SRC_PIXEL *)bitmap->line[visual.min_y]   + visual.min_x;
       SRC_PIXEL *line_end = (SRC_PIXEL *)bitmap->line[visual.max_y+1] + visual.min_x;
       DEST_PIXEL *line_dest = (DEST_PIXEL *)(DEST);
     
       for (;line_src < line_end;
	    line_dest+=REPS_FOR_Y(DEST_WIDTH,y,visual_height),
		    line_src+=src_width, y++)
	       COPY_LINE_FOR_Y(y,visual_height,
			       line_src, line_src+visual_width, line_dest);
     }
#endif
#ifdef PUT_IMAGE
     PUT_IMAGE(0, 0, SCALE_X(visual_width), SCALE_Y(visual_height))
#endif
  }

#undef DEST_SCALE_X
#undef DEST_SCALE_Y
