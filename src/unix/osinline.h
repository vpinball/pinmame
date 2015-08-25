#ifndef __OSINLINE__
#define __OSINLINE__

/* for uclock() */
#include "sysdep/misc.h"

/* #define osd_cycles() uclock() */

#if defined svgalib || defined x11 || defined ggi || defined openstep || defined SDL
extern unsigned char *dirty_lines;
extern unsigned char **dirty_blocks;

#define osd_mark_vector_dirty(x,y) \
{ \
   dirty_lines[(y)>>3] = 1; \
   dirty_blocks[(y)>>3][(x)>>3] = 1; \
}

#else
#define osd_mark_vector_dirty(x,y)
#endif

#ifdef X86_ASM
#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	int result;
	asm (
			"movl  %1    , %0    ; "
			"imull %2            ; "    /* do the multiply */
			"movl  %%edx , %%eax ; "
			:  "=&a" (result)           /* the result has to go in eax */
			:  "mr" (x),                /* x and y can be regs or mem */
			   "mr" (y)
			:  "%edx", "%cc"            /* clobbers edx and flags */
		);
	return result;
}
#endif

#endif /* __OSINLINE__ */
