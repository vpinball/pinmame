/* MAME headers */
#include "osdepend.h"
#include "sysdep/misc.h"

/*============================================================ */
/*	osd_cycles */
/*============================================================ */

cycles_t osd_cycles(void)
{
	return uclock();
}



/*============================================================ */
/*	osd_cycles_per_second */
/*============================================================ */

cycles_t osd_cycles_per_second(void)
{
	return UCLOCKS_PER_SEC;
}



/*============================================================ */
/*	osd_profiling_ticks */
/*============================================================ */

cycles_t osd_profiling_ticks(void)
{
	return 0;
}
