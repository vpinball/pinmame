#include "mamalleg.h"
#include "driver.h"
#include "ticker.h"
#include <time.h>



static int use_rdtsc;
TICKER ticks_per_sec;

TICKER ticker(void)
{
	if (use_rdtsc)
	{
		INT64 result;

		__asm__ __volatile__ (
			"rdtsc"
			: "=A" (result)
		);

		return result;
	}
	else
	{
		/* this assumes that uclock_t is 64-bit (which it is) */
		return uclock();
	}
}

int cpu_rdtsc(void)
{
	int result;

	__asm__ (
		"movl $1,%%eax     ; "
		"xorl %%ebx,%%ebx  ; "
		"xorl %%ecx,%%ecx  ; "
		"xorl %%edx,%%edx  ; "
		"cpuid             ; "
		"testl $0x10,%%edx ; "
		"setne %%al        ; "
		"andl $1,%%eax     ; "
	:  "=&a" (result)   /* the result has to go in eax */
	:       /* no inputs */
	:  "%ebx", "%ecx", "%edx" /* clobbers ebx ecx edx */
	);
	return result;
}

void init_ticker(void)
{
	/* if the RDTSC instruction is available use it because */
	/* it is more precise and has less overhead than uclock() */
	if (cpu_cpuid && cpu_rdtsc())
	{
		uclock_t a,b;
		TICKER start,end;

		use_rdtsc = 1;	/* must set this before calling ticker() */

		a = uclock();
		/* wait some time to let everything stabilize */
		do
		{
			b = uclock();
		} while (b-a < UCLOCKS_PER_SEC/10);

		a = uclock();
		start = ticker();
		do
		{
			b = uclock();
		} while (b-a < UCLOCKS_PER_SEC/4);
		end = ticker();
		ticks_per_sec = (end - start)*4;

logerror("using RDTSC for timing, CPU speed = %d.%06d MHz\n",
		(int)(ticks_per_sec/1000000),(int)(ticks_per_sec%1000000));
	}
	else
	{
		use_rdtsc = 0;
		ticks_per_sec = UCLOCKS_PER_SEC;

logerror("using uclock() for timing\n");
	}
}
