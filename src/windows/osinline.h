//============================================================
//
//	osinline.c - Win32 inline functions
//
//============================================================

#ifndef __OSINLINE__
#define __OSINLINE__

#include "osd_cpu.h"
#include "video.h"

//============================================================
//	MACROS
//============================================================

#define osd_mark_vector_dirty MARK_DIRTY


//============================================================
//	INLINE FUNCTIONS
//============================================================

#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
	int result;
	__asm__ (
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

INLINE unsigned int osd_cycles(void)
{
	int result;

	__asm__ __volatile__ (
		"rdtsc                 \n"	/* load clock cycle counter in eax and edx */
		:  "=&a" (result)			/* the result has to go in eax */
		:							/* no inputs */
		:  "%edx"					/* clobbers edx */
	);

	return result;
}

#endif /* __OSINLINE__ */
