/*
  special file to include allegro.h without compiler errors
*/

#define inline __inline__	/* inline is not an ANSI keyword so remap it */
#undef INLINE				/* Allegro redefines our INLINE definition */
#define ZERO_SIZE 1			/* avoid zero-length array declarations */

#include <allegro.h>

#undef inline
#undef INLINE
#define INLINE static __inline__
#undef ZERO_SIZE
