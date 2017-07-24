/* this routine is the generic blit routine used in many cases, through a number
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

   These routines use long copies so everything should always be long aligned.
*/

#ifdef PACK_BITS
/* scale destptr delta's by 3/4 since we're using 32 bits ptr's for a 24 bits
   dest */
#define DEST_SCALE 3/4
#define DEST_PIXEL_SIZE 3
#define CORRECTED_DEST_WIDTH ((DEST_WIDTH*3)/4)
#else
#define DEST_PIXEL_SIZE sizeof(DEST_PIXEL)
#define CORRECTED_DEST_WIDTH DEST_WIDTH
#endif

if (yarbsize) /* using arbitrary Y-scaling (Adam D. Moss <adam@gimp.org>) */
{
  switch(widthscale)
    {
#ifdef INDIRECT

#ifdef BLIT_16BPP_HACK
#define COPY_LINE2(SRC, END, DST) \
      {\
   unsigned short *src = (unsigned short *)(SRC); \
   unsigned short *end = (unsigned short *)(END); \
   unsigned int   *dst = (unsigned int   *)(DST); \
   for(;src<end;src+=4,dst+=4) \
   { \
      *(dst  ) = INDIRECT[*(src  )]; \
      *(dst+1) = INDIRECT[*(src+1)]; \
      *(dst+2) = INDIRECT[*(src+2)]; \
      *(dst+3) = INDIRECT[*(src+3)]; \
   }\
      }
#elif defined PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
      {\
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;dst+=3,src+=4) \
   { \
      *(dst  ) = (INDIRECT[*(src  )]    ) | (INDIRECT[*(src+1)]<<24); \
      *(dst+1) = (INDIRECT[*(src+1)]>> 8) | (INDIRECT[*(src+2)]<<16); \
      *(dst+2) = (INDIRECT[*(src+2)]>>16) | (INDIRECT[*(src+3)]<< 8); \
   }\
      }
#elif defined BLIT_HWSCALE_YUY2
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned long *dst = (unsigned long *)DST; \
      unsigned int r,y,y2,u,v; \
      for(;src<end;) \
      { \
         r=INDIRECT[*src++]; \
         y=r&255; \
         u=(r>>8)&255; \
         v=(r>>24); \
         r=INDIRECT[*src++]; \
         u=(u+((r>>8)&255))/2; \
         v=(v+(r>>24))/2; \
         y2=r&255; \
         *dst++=(y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24); \
      } \
   }
#else /* normal indirect */
#define COPY_LINE2(SRC, END, DST) \
      {\
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
   }\
      }
#endif /* dga_16bpp_hack / packed / normal indirect */

#else  /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
   memcpy(DST, SRC, ((END)-(SRC))*DEST_PIXEL_SIZE);
#endif /* indirect */

#define SCALE_Y(Y) (((Y)*yarbsize)/visual_height)
#define REPS_FOR_Y(N,YV,YMAX) ((N)* ( (((YV)+1)*yarbsize)/(YMAX) - ((YV)*yarbsize)/(YMAX)))

    case 1:
#define SCALE_X(X) (X)
#ifdef DOUBLEBUFFER

#ifdef INDIRECT
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   if (reps >0) COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer); \
   while (reps-- >0) { memcpy((DST)+(reps*(CORRECTED_DEST_WIDTH)), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*SCALE_X(1)); } \
}
#else
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   while (reps-- >0) COPY_LINE2(SRC, END, (DST)+(reps*(CORRECTED_DEST_WIDTH)));\
}
#endif

#else
#define COPY_LINE_FOR_Y(YV, YMAX, SRC, END, DST) \
{ \
   int reps = REPS_FOR_Y(1, YV, YMAX); \
   if (reps >0) COPY_LINE2(SRC, END, DST); \
   while (reps-- >1) memcpy((DST)+(reps*(CORRECTED_DEST_WIDTH)), DST, ((END)-(SRC))*DEST_PIXEL_SIZE*SCALE_X(1)); \
}
#endif
/*#define COPY_LINE(SRC, END, DST) { COPY_LINE2(SRC, END, DST) }*/
#include "blit_core.h"
break;

#undef SCALE_X
#undef COPY_LINE2




case 2:
#define SCALE_X(X) ((X)*2)
#ifdef INDIRECT

#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end; src+=2, dst+=3) \
   { \
      *(dst  ) = (INDIRECT[*(src  )]    ) | (INDIRECT[*(src  )]<<24); \
      *(dst+1) = (INDIRECT[*(src  )]>> 8) | (INDIRECT[*(src+1)]<<16); \
      *(dst+2) = (INDIRECT[*(src+1)]>>16) | (INDIRECT[*(src+1)]<<8); \
   } \
}
#else /* not pack bits */
#define COPY_LINE2(SRC, END, DST) \
{ \
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
   } \
}
#endif /* pack bits */

#else /* not indirect */

#define COPY_LINE2(SRC, END, DST) \
{ \
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
   } \
}
#endif
#include "blit_core.h"
break;

#undef SCALE_X
#undef COPY_LINE2




case 3:
#define SCALE_X(X)   ((X)*3)
#ifdef INDIRECT

#define COPY_LINE2(SRC, END, DST) \
{ \
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
   } \
}
#else /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
{ \
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
   } \
}
#endif
#include "blit_core.h"
break;

#undef SCALE_X
#undef COPY_LINE2




default:
#define SCALE_X(X)   ((X)*widthscale)

#ifdef INDIRECT

#ifdef PACK_BITS
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   DEST_PIXEL pixel; \
   int i, step=0; \
   for(;src<end;src++) \
   { \
      pixel = INDIRECT[*src]; \
      for(i=0; i<widthscale; i++,step=(step+1)&3) \
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
   } \
}
#else
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   int i; \
   for(;src<end;src++) \
   { \
      const DEST_PIXEL v = INDIRECT[*(src)]; \
      i=(widthscale+7)/8; \
      dst+=widthscale&7; \
      switch (widthscale&7) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = v; \
         case 7:      *(dst-7) = v; \
         case 6:      *(dst-6) = v; \
         case 5:      *(dst-5) = v; \
         case 4:      *(dst-4) = v; \
         case 3:      *(dst-3) = v; \
         case 2:      *(dst-2) = v; \
         case 1:      *(dst-1) = v; \
                 } while(--i>0); \
      } \
   } \
}
#endif

#else
#define COPY_LINE2(SRC, END, DST) \
{ \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   int i; \
   for(;src<end;src++) \
   { \
      const DEST_PIXEL v = *(src); \
      i=(widthscale+7)/8; \
      dst+=widthscale&7; \
      switch (widthscale&7) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = v; \
         case 7:      *(dst-7) = v; \
         case 6:      *(dst-6) = v; \
         case 5:      *(dst-5) = v; \
         case 4:      *(dst-4) = v; \
         case 3:      *(dst-3) = v; \
         case 2:      *(dst-2) = v; \
         case 1:      *(dst-1) = v; \
                 } while(--i>0); \
      } \
   } \
}
#endif
#include "blit_core.h"
break;

#undef SCALE_X
#undef COPY_LINE2















#undef COPY_LINE

#undef SCALE_Y
#undef REPS_FOR_Y
#undef COPY_LINE_FOR_Y

#define COPY_LINE_FOR_Y(YV,YMAX, SRC,END,DST) COPY_LINE(SRC,END,DST)
#define REPS_FOR_Y(N,YV,YMAX) SCALE_Y(N)
    }
}
else
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
#elif defined BLIT_HWSCALE_YUY2
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned long *dst = (unsigned long *)DST; \
      unsigned int r,r2,y,y2,uv1,uv2; \
      for(;src<end;) \
      { \
         r=INDIRECT[*src++]; \
         r2=INDIRECT[*src++]; \
         y=r&0xff; \
         y2=r2&0xff0000; \
         uv1=(r&0xff00ff00)>>1; \
         uv2=(r2&0xff00ff00)>>1; \
         uv1=(uv1+uv2)&0xff00ff00; \
         *dst++=y|y2|uv1; \
      } \
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
#ifdef BLIT_HWSCALE_YUY2
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned char *dst = (unsigned char *)DST; \
      int r,g,b,r2,g2,b2,y,y2,u,v; \
      for(;src<end;) \
      { \
         r=g=b=*src++; \
         r&=RMASK;  r>>=16; \
         g&=GMASK;  g>>=8; \
         b&=BMASK;  b>>=0; \
         r2=g2=b2=*src++; \
         r2&=RMASK;  r2>>=16; \
         g2&=GMASK;  g2>>=8; \
         b2&=BMASK;  b2>>=0; \
         y = (( 9897*r + 19235*g + 3736*b ) >> 15); \
         y2 = (( 9897*r2 + 19235*g2 + 3736*b2 ) >> 15); \
         r+=r2; g+=g2; b+=b2; \
         *dst++=y; \
         u = (( -(5537/2)*r - (10878/2)*g + (16384/2)*b ) >> 15) + 128; \
         *dst++=u; \
         v = (( (16384/2)*r - (13730/2)*g -(2664/2)*b ) >> 15 ) + 128; \
         *dst++=y2; \
         *dst++=v; \
      }\
   }
#else
#define COPY_LINE2(SRC, END, DST) \
   memcpy(DST, SRC, ((END)-(SRC))*DEST_PIXEL_SIZE);
#endif
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

#define SCALE_Y(Y)   ((Y)<<1)

/* 1x2 no scanlines */
case 0x00102:

#ifdef DOUBLEBUFFER

#ifdef INDIRECT
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST),                        doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE); \
   memcpy((DST)+(CORRECTED_DEST_WIDTH), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DST)); \
   COPY_LINE2(SRC, END, (DST)+(CORRECTED_DEST_WIDTH)); \
}
#endif

#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
   memcpy((DST)+(CORRECTED_DEST_WIDTH), DST, ((END)-(SRC))*DEST_PIXEL_SIZE); \
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
#elif defined BLIT_HWSCALE_YUY2
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned int *dst = (unsigned int *)DST; \
      int r; \
      for(;src<end;) \
      { \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
          r=INDIRECT[*src++]; \
          *dst++=r; \
      } \
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

#ifdef BLIT_HWSCALE_YUY2
#define COPY_LINE2(SRC, END, DST) \
   {\
      SRC_PIXEL  *src = SRC; \
      SRC_PIXEL  *end = END; \
      unsigned char *dst = (unsigned char *)DST; \
      int r,g,b,y,u,v; \
      for(;src<end;) \
      { \
         r=g=b=*src++; \
         r&=RMASK;  r>>=16; \
         g&=GMASK;  g>>=8; \
         b&=BMASK;  b>>=0; \
         y = (( 9897*r + 19235*g + 3736*b ) >> 15); \
         *dst++=y; \
         u = (( -(5537)*r - (10878)*g + (16384)*b ) >> 15) + 128; \
         *dst++=u; \
         v = (( (16384)*r - (13730)*g -(2664)*b ) >> 15 ) + 128; \
         *dst++=y; \
         *dst++=v; \
      }\
   }
#else
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
#endif

#define SCALE_X(X)   ((X)<<1)
#define SCALE_Y(Y)   ((Y))

/* 2x1 no scanlines */
case 0x00201:

#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*2); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

#undef SCALE_X
#undef SCALE_Y

#define SCALE_X(X)   ((X)<<1)
#define SCALE_Y(Y)   ((Y)<<1)

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
#undef SCALE_Y

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
#define SCALE_Y(Y)   ((Y))

/* 3x1 no scanlines */
case 0x00301:

#ifdef DOUBLEBUFFER
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, (DEST_PIXEL *)doublebuffer_buffer) \
   memcpy((DST), doublebuffer_buffer, ((END)-(SRC))*DEST_PIXEL_SIZE*3); \
}
#else
#define COPY_LINE(SRC, END, DST) \
{ \
   COPY_LINE2(SRC, END, DST) \
}
#endif

#include "blit_core.h"
#undef COPY_LINE
break;

#undef SCALE_X
#undef SCALE_Y

#define SCALE_X(X)   ((X)*3)
#define SCALE_Y(Y)   ((Y)*3)

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
#undef SCALE_Y

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
#define SCALE_Y(Y)   ((Y)<<2)

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
#undef SCALE_Y

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
      for(i=0; i<widthscale; i++,step=(step+1)&3) \
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
      const DEST_PIXEL v = INDIRECT[*(src)]; \
      i=(widthscale+7)/8; \
      dst+=widthscale&7; \
      switch (widthscale&7) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = v; \
         case 7:      *(dst-7) = v; \
         case 6:      *(dst-6) = v; \
         case 5:      *(dst-5) = v; \
         case 4:      *(dst-4) = v; \
         case 3:      *(dst-3) = v; \
         case 2:      *(dst-2) = v; \
         case 1:      *(dst-1) = v; \
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
      const DEST_PIXEL v = *(src); \
      i=(widthscale+7)/8; \
      dst+=widthscale&7; \
      switch (widthscale&7) \
      { \
         case 0: do{  dst+=8; \
                      *(dst-8) = v; \
         case 7:      *(dst-7) = v; \
         case 6:      *(dst-6) = v; \
         case 5:      *(dst-5) = v; \
         case 4:      *(dst-4) = v; \
         case 3:      *(dst-3) = v; \
         case 2:      *(dst-2) = v; \
         case 1:      *(dst-1) = v; \
                 } while(--i>0); \
      } \
   }
#endif

#define SCALE_X(X)   ((X)*widthscale)
#define SCALE_Y(Y)   ((Y)*heightscale)

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
#undef SCALE_Y

break;      
}

#ifdef DEST_SCALE
#undef DEST_SCALE
#endif
#undef DEST_PIXEL_SIZE
#undef CORRECTED_DEST_WIDTH


#undef REPS_FOR_Y
#undef COPY_LINE_FOR_Y
