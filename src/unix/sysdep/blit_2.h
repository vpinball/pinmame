/* this routine is the generic blit routine used in many cases, trough a number
   of defines it can be customised for specific cases.
   Currently recognised defines:
   DEST		ptr of type DEST_PIXEL to which should be blitted, if this is
		not defined only PUT_IMAGE is called if defined.
   DEST_PIXEL	type of the buffer to which is blitted, only needed if
                DEST is defined.
   DEST_WIDTH   Width of the destination buffer in pixels! Only needed if
                DEST is defined.
   SRC_PIXEL    type of the buffer from which is blitted, currently
                8 bpp (unsigned char) and 16 bpp (unsigned short) are supported.
   PUT_IMAGE    This function is called to update the parts of the screen
		which need updating. This is only called if defined.
   INDIRECT     This needs to be defined if DEST_PIXEL != unsigned char,
                this is a ptr to a list of pixels/colormappings for the
                colordepth conversion.
   BLIT_16BPP_HACK This one speaks for itself, it's a speedup hack for 8bpp
                to 16bpp blits.
   PACK_BITS    Write to packed 24bit pixels, DEST_PIXEL must be 32bits and
                INDIRECT must be on.
   DOUBLEBUFFER First copy each line to a buffer called doublebuffer_buffer,
                then do a memcpy to the real destination. This speeds up
                scaling when writing directly to framebuffer since it
                tremendously speeds up the reads done to copy one line to
                the next.
   
   This routines use long copy's so everything should always be long aligned.
*/

#error unused

#ifdef PACK_BITS
/* scale destptr delta's by 3/4 since we're using 32 bits ptr's for a 24 bits
   dest */
#define DEST_SCALE(X) ((X * 3) / 4)
#define DEST_PIXEL_SIZE 3
#else
#define DEST_SCALE(X) (X)
#define DEST_PIXEL_SIZE sizeof(DEST_PIXEL)
#endif

switch (heightscale | (widthscale << 8) | (use_scanlines << 16))
{
/* 1x1 */

#ifdef INDIRECT

#ifdef BLIT_16BPP_HACK
#define COPY_LINE2(SRC, END, DST) \
   unsigned short *src = (unsigned short *)(SRC); \
   unsigned short *end = (unsigned short *)(END); \
   unsigned int   *dst = (unsigned int   *)(DST); \
   for(;src<end;src+=4,dst+=4) \
   { \
      *(dst  ) = INDIRECT[*(src  )]; \
      *(dst+1) = INDIRECT[*(src+1)]; \
      *(dst+2) = INDIRECT[*(src+2)]; \
      *(dst+3) = INDIRECT[*(src+3)]; \
   }
#elif defined PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;dst+=3,src+=4) \
   { \
      *(dst  ) = (INDIRECT[*(src  )]    ) | (INDIRECT[*(src+1)]<<24); \
      *(dst+1) = (INDIRECT[*(src+1)]>> 8) | (INDIRECT[*(src+2)]<<16); \
      *(dst+2) = (INDIRECT[*(src+2)]>>16) | (INDIRECT[*(src+3)]<< 8); \
   }
#else /* normal indirect */
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;src+=8,dst+=8) \
   { \
      *(dst  ) = INDIRECT[*(src  )]; \
      *(dst+1) = INDIRECT[*(src+1)]; \
      *(dst+2) = INDIRECT[*(src+2)]; \
      *(dst+3) = INDIRECT[*(src+3)]; \
      *(dst+4) = INDIRECT[*(src+4)]; \
      *(dst+5) = INDIRECT[*(src+5)]; \
      *(dst+6) = INDIRECT[*(src+6)]; \
      *(dst+7) = INDIRECT[*(src+7)]; \
   }
#endif /* dga_16bpp_hack / packed / normal indirect */

#else  /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
   memcpy(DST, SRC, ((END)-(SRC))*DEST_PIXEL_SIZE);
#endif /* indirect */

#define SCALE_X(X) (X)
#define SCALE_Y(Y) (Y)

/* 1x1 we don't do scanlines with 1x1 */
case 0x00101:
case 0x10101:
#define COPY_LINE(SRC, END, DST) { COPY_LINE2(SRC, END, DST) }
#include "blit_core.h"
#undef COPY_LINE
break;

#undef SCALE_Y

/* 1x2 */

#define SCALE_Y(Y)   ((Y) * 2)

/* 1x2 no scanlines */
case 0x00102:

#ifdef DOUBLEBUFFER

#ifdef INDIRECT
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                          doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE); \
   memcpy((DST) + DEST_SCALE(DEST_WIDTH), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DST)); \
   COPY_LINE2(SRC, END, (DST) + DEST_SCALE(DEST_WIDTH)); \
}
#endif

#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST) + DEST_SCALE(DEST_WIDTH), DST, ((END)-(SRC))*DEST_PIXEL_SIZE); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

/* 1x2 scanlines */
case 0x10102:
#define COPY_LINE(SRC, END, DST) { COPY_LINE2(SRC, END, DST) }
#include "blit_core.h"
#undef COPY_LINE
break;

#undef COPY_LINE2
#undef SCALE_X
#undef SCALE_Y


/* 2x2 */

#ifdef INDIRECT

#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=2, dst+=3) \
   { \
      *(dst  ) = (INDIRECT[*(src  )]    ) | (INDIRECT[*(src  )]<<24); \
      *(dst+1) = (INDIRECT[*(src  )]>> 8) | (INDIRECT[*(src+1)]<<16); \
      *(dst+2) = (INDIRECT[*(src+1)]>>16) | (INDIRECT[*(src+1)]<<8); \
   }
#else /* not pack bits */
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=16) \
   { \
      *(dst   ) = *(dst+ 1) = INDIRECT[*(src  )]; \
      *(dst+ 2) = *(dst+ 3) = INDIRECT[*(src+1)]; \
      *(dst+ 4) = *(dst+ 5) = INDIRECT[*(src+2)]; \
      *(dst+ 6) = *(dst+ 7) = INDIRECT[*(src+3)]; \
      *(dst+ 8) = *(dst+ 9) = INDIRECT[*(src+4)]; \
      *(dst+10) = *(dst+11) = INDIRECT[*(src+5)]; \
      *(dst+12) = *(dst+13) = INDIRECT[*(src+6)]; \
      *(dst+14) = *(dst+15) = INDIRECT[*(src+7)]; \
   }
#endif /* pack bits */

#else /* not indirect */

#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=16) \
   { \
      *(dst   ) = *(dst+ 1) = *(src  ); \
      *(dst+ 2) = *(dst+ 3) = *(src+1); \
      *(dst+ 4) = *(dst+ 5) = *(src+2); \
      *(dst+ 6) = *(dst+ 7) = *(src+3); \
      *(dst+ 8) = *(dst+ 9) = *(src+4); \
      *(dst+10) = *(dst+11) = *(src+5); \
      *(dst+12) = *(dst+13) = *(src+6); \
      *(dst+14) = *(dst+15) = *(src+7); \
   }
#endif

#define SCALE_X(X)   ((X)<<1)
#define SCALE_X_8(X) ((X)<<4)
#define SCALE_Y(Y)   ((Y)<<1)
#define SCALE_Y_8(Y) ((Y)<<4)

/* 2x2 no scanlines */
case 0x00202:

#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                        doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*2); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*2); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH), DST, ((END)-(SRC))*DEST_PIXEL_SIZE*2); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

/* 2x2 scanlines */
case 0x10202:

#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                        doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*2); \
}
#else
#define COPY_LINE(SRC, END, DST) { COPY_LINE2(SRC, END, DST) }
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

#undef COPY_LINE2
#undef SCALE_X
#undef SCALE_X_8
#undef SCALE_Y
#undef SCALE_Y_8

#ifndef PACK_BITS
/* 3x3 */

/* this macro is used to copy a line */
#ifdef INDIRECT

#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=24) \
   { \
      *(dst   ) = *(dst+ 1) = *(dst+ 2) = INDIRECT[*(src  )]; \
      *(dst+ 3) = *(dst+ 4) = *(dst+ 5) = INDIRECT[*(src+1)]; \
      *(dst+ 6) = *(dst+ 7) = *(dst+ 8) = INDIRECT[*(src+2)]; \
      *(dst+ 9) = *(dst+10) = *(dst+11) = INDIRECT[*(src+3)]; \
      *(dst+12) = *(dst+13) = *(dst+14) = INDIRECT[*(src+4)]; \
      *(dst+15) = *(dst+16) = *(dst+17) = INDIRECT[*(src+5)]; \
      *(dst+18) = *(dst+19) = *(dst+20) = INDIRECT[*(src+6)]; \
      *(dst+21) = *(dst+22) = *(dst+23) = INDIRECT[*(src+7)]; \
   }
#else /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=24) \
   { \
      *(dst   ) = *(dst+ 1) = *(dst+ 2) = *(src  ); \
      *(dst+ 3) = *(dst+ 4) = *(dst+ 5) = *(src+1); \
      *(dst+ 6) = *(dst+ 7) = *(dst+ 8) = *(src+2); \
      *(dst+ 9) = *(dst+10) = *(dst+11) = *(src+3); \
      *(dst+12) = *(dst+13) = *(dst+14) = *(src+4); \
      *(dst+15) = *(dst+16) = *(dst+17) = *(src+5); \
      *(dst+18) = *(dst+19) = *(dst+20) = *(src+6); \
      *(dst+21) = *(dst+22) = *(dst+23) = *(src+7); \
   }
#endif

#define SCALE_X(X)   ((X)*3)
#define SCALE_X_8(X) ((X)*24)
#define SCALE_Y(Y)   ((Y)*3)
#define SCALE_Y_8(Y) ((Y)*24)

/* 3x3 no scanlines */
case 0x00303:

/* should we use doublebuffering ? */
#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                          doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   DST, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, DST, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

/* 3x3 scanlines */
case 0x10303:

/* should we use doublebuffering ? */
#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                          doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   DST, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

#undef COPY_LINE2
#undef SCALE_X
#undef SCALE_X_8
#undef SCALE_Y
#undef SCALE_Y_8

/* 4x4 */

#ifdef INDIRECT

#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=32) \
   { \
      *(dst   ) = *(dst+ 1) = *(dst+ 2) = *(dst+ 3) = INDIRECT[*(src  )]; \
      *(dst+ 4) = *(dst+ 5) = *(dst+ 6) = *(dst+ 7) = INDIRECT[*(src+1)]; \
      *(dst+ 8) = *(dst+ 9) = *(dst+10) = *(dst+11) = INDIRECT[*(src+2)]; \
      *(dst+12) = *(dst+13) = *(dst+14) = *(dst+15) = INDIRECT[*(src+3)]; \
      *(dst+16) = *(dst+17) = *(dst+18) = *(dst+19) = INDIRECT[*(src+4)]; \
      *(dst+20) = *(dst+21) = *(dst+22) = *(dst+23) = INDIRECT[*(src+5)]; \
      *(dst+24) = *(dst+25) = *(dst+26) = *(dst+27) = INDIRECT[*(src+6)]; \
      *(dst+28) = *(dst+29) = *(dst+30) = *(dst+31) = INDIRECT[*(src+7)]; \
   }
#else /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=8, dst+=32) \
   { \
      *(dst   ) = *(dst+ 1) = *(dst+ 2) = *(dst+ 3) = *(src  ); \
      *(dst+ 4) = *(dst+ 5) = *(dst+ 6) = *(dst+ 7) = *(src+1); \
      *(dst+ 8) = *(dst+ 9) = *(dst+10) = *(dst+11) = *(src+2); \
      *(dst+12) = *(dst+13) = *(dst+14) = *(dst+15) = *(src+3); \
      *(dst+16) = *(dst+17) = *(dst+18) = *(dst+19) = *(src+4); \
      *(dst+20) = *(dst+21) = *(dst+22) = *(dst+23) = *(src+5); \
      *(dst+24) = *(dst+25) = *(dst+26) = *(dst+27) = *(src+6); \
      *(dst+28) = *(dst+29) = *(dst+30) = *(dst+31) = *(src+7); \
   }
#endif

#define SCALE_X(X)   ((X)<<2)
#define SCALE_X_8(X) ((X)<<5)
#define SCALE_Y(Y)   ((Y)<<2)
#define SCALE_Y_8(Y) ((Y)<<5)

/* 4x4 no scanlines */
case 0x00404:

/* should we use doublebuffering ? */
#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                          doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*3, doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   DST, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, DST, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*3, DST, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

/* 4x4 scanlines */
case 0x10404:

/* should we use doublebuffering ? */
#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                          doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH),   DST, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH)*2, DST, ((END)-(SRC))*DEST_PIXEL_SIZE*4); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

#undef COPY_LINE2
#undef SCALE_X
#undef SCALE_X_8
#undef SCALE_Y
#undef SCALE_Y_8

#endif /* #ifndef PACK_BITS */

/* Generic scaling code here (arbitrary values) */

/* This is what happens when you give an assembly-language programmer
   a C compiler.  Thanks to td, of course.                             -JDL */

default:

#ifdef INDIRECT

#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   DEST_PIXEL pixel; \
   int i, step=0; \
   for(;src<end;src++) \
   { \
      pixel = INDIRECT[*src]; \
      for(i=0; i<widthscale; i++,step=(step+1)%4) \
      { \
         switch(step) \
         { \
            case 0: \
               *(dst  )  = pixel; \
               break; \
            case 1: \
               *(dst  ) |= pixel << 24; \
               *(dst+1)  = pixel >> 8; \
               break; \
            case 2: \
               *(dst+1) |= pixel << 16; \
               *(dst+2)  = pixel >> 16; \
               break; \
            case 3: \
               *(dst+2) |= pixel << 8; \
               dst+=3; \
               break; \
         } \
      } \
   }
#else
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   int i; \
   for(;src<end;src++) \
   { \
      i=(widthscale+7)/8; \
      dst+=widthscale%8; \
      switch (widthscale%8) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = INDIRECT[*(src)]; \
         case 7:      *(dst-7) = INDIRECT[*(src)]; \
         case 6:      *(dst-6) = INDIRECT[*(src)]; \
         case 5:      *(dst-5) = INDIRECT[*(src)]; \
         case 4:      *(dst-4) = INDIRECT[*(src)]; \
         case 3:      *(dst-3) = INDIRECT[*(src)]; \
         case 2:      *(dst-2) = INDIRECT[*(src)]; \
         case 1:      *(dst-1) = INDIRECT[*(src)]; \
                 } while(--i>0); \
      } \
   }
#endif

#else
#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   int i; \
   for(;src<end;src++) \
   { \
      i=(widthscale+7)/8; \
      dst+=widthscale%8; \
      switch (widthscale%8) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = *(src); \
         case 7:      *(dst-7) = *(src); \
         case 6:      *(dst-6) = *(src); \
         case 5:      *(dst-5) = *(src); \
         case 4:      *(dst-4) = *(src); \
         case 3:      *(dst-3) = *(src); \
         case 2:      *(dst-2) = *(src); \
         case 1:      *(dst-1) = *(src); \
                 } while(--i>0); \
      } \
   }
#endif

#define SCALE_X(X)   ((X)*widthscale)
#define SCALE_X_8(X) ((X)*widthscale*8)
#define SCALE_Y(Y)   ((Y)*heightscale)
#define SCALE_Y_8(Y) ((Y)*heightscale*8)

/* should we use doublebuffering ? */
#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   int max_i = heightscale-use_scanlines; \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   dst = (DST); \
   if (max_i < 1) max_i = 1; \
   for(i=0; i<max_i; i++, dst+=CORRECTED_DEST_WIDTH) \
      memcpy(dst, doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*widthscale); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   dst = (DST) + (CORRECTED_DEST_WIDTH); \
   for(i=1; i<(heightscale-use_scanlines); i++, dst+=CORRECTED_DEST_WIDTH) \
      memcpy(dst, DST, ((END)-(SRC))*DEST_PIXEL_SIZE*widthscale); \
}
#endif

#include "blit_core.h"
#undef COPY_LINE

#undef COPY_LINE2
#undef SCALE_X
#undef SCALE_X_8
#undef SCALE_Y
#undef SCALE_Y_8

break;      
}

#ifdef DEST_SCALE
#undef DEST_SCALE
#endif
#undef DEST_PIXEL_SIZE
#undef CORRECTED_DEST_WIDTH
