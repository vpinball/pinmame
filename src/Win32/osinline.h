/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __OSINLINE__
#define __OSINLINE__

/* What goes herein depends heavily on the OS. */

#include "dirty.h"
/* Clean dirty method */
#define osd_mark_vector_dirty(x, y) MarkDirtyPixel(x,y);

#if defined(_M_IX86) && defined(_MSC_VER)
#define vec_mult _vec_mult
inline int _vec_mult(int x, int y)
{
    int result;
    __asm
    {
        mov eax , x
        imul y
        mov result , edx
    }
    return result;
}

#if _MSC_VER < 0x0400
INLINE unsigned int osd_cycles(void)
{
    int result;

#define ASM_RDTSC __asm _emit 0x0f __asm _emit 0x31

    __asm
    {
        xor eax, eax        /* touch eax so compiler will not use it. */
        ASM_RDTSC           /* load clock cycle counter in eax and edx */
        mov result, eax     /* the result has to go in eax (low 32 bits) */
    }

    return result;
}
#else
INLINE unsigned int osd_cycles(void)
{
    int result;

    __asm
    {
        rdtsc               /* load clock cycle counter in eax and edx */
        mov result, eax     /* the result has to go in eax (low 32 bits) */
    }

    return result;
}

#endif /* _MSC_VER */

#else

#include "uclock.h"

INLINE unsigned int osd_cycles(void)
{
    return uclock();
}

#endif

#endif /* __OSINLINE__ */
