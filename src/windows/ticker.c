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



//============================================================
//	PROTOTYPES
//============================================================

static cycles_t init_cycle_counter(void);
static cycles_t performance_cycle_counter(void);
static cycles_t rdtsc_cycle_counter(void);
static cycles_t time_cycle_counter(void);
static cycles_t nop_cycle_counter(void);



//============================================================
//	GLOBAL VARIABLES
//============================================================

// global cycle_counter function and divider
cycles_t		(*cycle_counter)(void) = init_cycle_counter;
cycles_t		(*ticks_counter)(void) = init_cycle_counter;
cycles_t		cycles_per_sec;
int				win_force_rdtsc;
int				win_high_priority;



//============================================================
//	STATIC VARIABLES
//============================================================

static cycles_t suspend_adjustment;
static cycles_t suspend_time;


//============================================================
//	init_cycle_counter
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
//	init_cycle_counter
//============================================================

static cycles_t init_cycle_counter(void)
{
	cycles_t start, end;
	DWORD a, b;
	int priority;
	LARGE_INTEGER frequency;

	suspend_adjustment = 0;
	suspend_time = 0;

	if (!win_force_rdtsc && QueryPerformanceFrequency( &frequency ))
	{
		// use performance counter if available as it is constant
		cycle_counter = performance_cycle_counter;
		logerror("using performance counter for timing ... ");
		cycles_per_sec = frequency.QuadPart;

		if (has_rdtsc())
		{
			ticks_counter = rdtsc_cycle_counter;
		}
		else
		{
			ticks_counter = nop_cycle_counter;
		}
	}
	else
	{
		if (has_rdtsc())
		{
			// if the RDTSC instruction is available use it because
			// it is more precise and has less overhead than timeGetTime()
			cycle_counter = rdtsc_cycle_counter;
			ticks_counter = rdtsc_cycle_counter;
			logerror("using RDTSC for timing ... ");
		}
		else
		{
			cycle_counter = time_cycle_counter;
			ticks_counter = nop_cycle_counter;
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

		// get the starting cycle count
		start = (*cycle_counter)();

		// now wait for 1/4 second total
		do
		{
			a = timeGetTime();
		} while (a - b < 250);

		// get the ending cycle count
		end = (*cycle_counter)();

		// compute ticks_per_sec
		cycles_per_sec = (end - start) * 4;

		// restore our priority
		// raise it if the config option is set and the debugger is not active
		if (win_high_priority && !options.mame_debug && priority == THREAD_PRIORITY_NORMAL)
			priority = THREAD_PRIORITY_ABOVE_NORMAL;
		SetThreadPriority(GetCurrentThread(), priority);
	}

	// log the results
	logerror("cycles/second = %u\n", (int)cycles_per_sec);

	// return the current cycle count
	return (*cycle_counter)();
}



//============================================================
//	performance_cycle_counter
//============================================================

static cycles_t performance_cycle_counter(void)
{
	LARGE_INTEGER performance_count;
	QueryPerformanceCounter( &performance_count );
	return (cycles_t)performance_count.QuadPart;
}



//============================================================
//	rdtsc_cycle_counter
//============================================================

#ifdef _MSC_VER

static cycles_t rdtsc_cycle_counter(void)
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

static cycles_t rdtsc_cycle_counter(void)
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
//	time_cycle_counter
//============================================================

static cycles_t time_cycle_counter(void)
{
	// use timeGetTime
	return (cycles_t)timeGetTime();
}



//============================================================
//	nop_cycle_counter
//============================================================

static cycles_t nop_cycle_counter(void)
{
	return 0;
}



//============================================================
//	osd_cycles
//============================================================

cycles_t osd_cycles(void)
{
	return suspend_time ? suspend_time : (*cycle_counter)() - suspend_adjustment;
}



//============================================================
//	osd_cycles_per_second
//============================================================

cycles_t osd_cycles_per_second(void)
{
	return cycles_per_sec;
}



//============================================================
//	osd_profiling_ticks
//============================================================

cycles_t osd_profiling_ticks(void)
{
	return (*ticks_counter)();
}



//============================================================
//	win_timer_enable
//============================================================

void win_timer_enable(int enabled)
{
	cycles_t actual_cycles;

	actual_cycles = (*cycle_counter)();
	if (!enabled)
	{
		suspend_time = actual_cycles;
	}
	else if (suspend_time > 0)
	{
		suspend_adjustment += actual_cycles - suspend_time;
		suspend_time = 0;
	}
}

