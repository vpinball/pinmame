//============================================================
//
//	ticker.c - Win32 timing code
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

// MAME headers
#include "driver.h"
#include "ticker.h"



//============================================================
//	PROTOTYPES
//============================================================

static TICKER init_ticker(void);
static TICKER rdtsc_ticker(void);
static TICKER time_ticker(void);



//============================================================
//	GLOBAL VARIABLES
//============================================================

// global ticker function and divider
TICKER			(*ticker)(void) = init_ticker;
TICKER			ticks_per_sec;



//============================================================
//	init_ticker
//============================================================

#ifdef _MSC_VER

static int has_rdtsc(void)
{
	int nFeatures;

	__asm {

		mov eax, 1
		cpuid
		mov nFeatures, edx
	}

	return ((nFeatures & 0x10) == 0x10) ? TRUE : FALSE;
}

#else

static int has_rdtsc(void)
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

#endif

//============================================================
//	init_ticker
//============================================================

static TICKER init_ticker(void)
{
	TICKER start, end;
	DWORD a, b;
	int priority;

	// if the RDTSC instruction is available use it because
	// it is more precise and has less overhead than timeGetTime()
	if (has_rdtsc())
	{
		ticker = rdtsc_ticker;
		logerror("using RDTSC for timing ... ");
	}
	else
	{
		ticker = time_ticker;
		logerror("using timeGetTime for timing ... ");
	}

	// temporarily set our priority higher
	priority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// wait for an edge on the timeGetTime call
	a = timeGetTime();
	do
	{
		b = timeGetTime();
	} while (a == b);

	// get the starting ticker
	start = ticker();

	// now wait for 1/4 second total
	do
	{
		a = timeGetTime();
	} while (a - b < 250);

	// get the ending ticker
	end = ticker();

	// compute ticks_per_sec
	ticks_per_sec = (end - start) * 4;

	// reduce our priority
	SetThreadPriority(GetCurrentThread(), priority);

	// log the results
	logerror("ticks/second = %d\n", (int)ticks_per_sec);

	// return the current ticker value
	return ticker();
}



//============================================================
//	rdtsc_ticker
//============================================================


#ifdef _MSC_VER

static TICKER rdtsc_ticker(void)
{
	INT64 result;
	INT64 *presult = &result;

	__asm {

		rdtsc
		mov ebx, presult
		mov [ebx],eax
		mov [ebx+4],edx
	}

	return result;
}

#else

static TICKER rdtsc_ticker(void)
{
	INT64 result;

	// use RDTSC
	__asm__ __volatile__ (
		"rdtsc"
		: "=A" (result)
	);

	return result;
}

#endif

//============================================================
//	time_ticker
//============================================================

static TICKER time_ticker(void)
{
	// use timeGetTime
	return (TICKER)timeGetTime();
}
