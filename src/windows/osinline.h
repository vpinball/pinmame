//============================================================
//
//	osinline.h - Win32 inline functions
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

#define osd_pend	osd_pend
#define pdo16		osd_pdo16
#define pdt16		osd_pdt16
#define pdt16np		osd_pdt16np


//============================================================
//	PROTOTYPES
//============================================================

void osd_pend(void);
void osd_pdo16( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode );
void osd_pdt16( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );
void osd_pdt16np( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );


//============================================================
//	INLINE FUNCTIONS
//============================================================

#ifdef _MSC_VER

#define vec_mult _vec_mult
INLINE int _vec_mult(int x, int y)
{
    int result;

    __asm {
        mov eax, x
        imul y
        mov result, edx
    }

    return result;
}

INLINE unsigned int osd_cycles(void)
{
	int result;

	__asm {
        rdtsc
        mov result, eax
    }

    return result;
}

#else

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

#endif /* _MSC_VER */

#endif /* __OSINLINE__ */
