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



//============================================================
//	GLOBAL VARIABLES
//============================================================

// global cycle_counter function and divider
cycles_t		(*cycle_counter)(void) = init_cycle_counter;
cycles_t		cycles_per_sec;


//============================================================
//	STATIC VARIABLES
//============================================================

static cycles_t suspend_adjustment;
static cycles_t suspend_time;


//============================================================
//	init_cycle_counter
//============================================================

static cycles_t init_cycle_counter(void)
{
	LARGE_INTEGER frequency;

	suspend_adjustment = 0;
	suspend_time = 0;

	if (QueryPerformanceFrequency( &frequency ))
	{
		cycle_counter = performance_cycle_counter;
		logerror("using performance counter for timing ... ");
		cycles_per_sec = frequency.QuadPart;
		logerror("cycles/second = %llu\n", cycles_per_sec);
	}
	else
	{
		logerror("NO QueryPerformanceFrequency available");
	}

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

#ifndef __LP64__
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
	return __rdtsc();
}
#endif

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
	return rdtsc_cycle_counter(); //!! meh, but only used for profiling
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

//

static unsigned int sTimerInit = 0;
static LARGE_INTEGER TimerFreq;
static LARGE_INTEGER sTimerStart;

static void wintimer_init(void)
{
	sTimerInit = 1;

	QueryPerformanceFrequency(&TimerFreq);
	QueryPerformanceCounter(&sTimerStart);
}

// tries(!) to be as exact as possible at the cost of potentially causing trouble with other threads/cores due to OS madness
// needs timeBeginPeriod(1) before calling 1st time to make the Sleep(1) in here behave more or less accurately (and timeEndPeriod(1) after not needing that precision anymore)
// but MAME code does this already
void uSleep(const UINT64 u)
{
	LARGE_INTEGER TimerEnd;
	LARGE_INTEGER TimerNow;
	LONGLONG TwoMSTimerTicks;

	if (sTimerInit == 0)
		wintimer_init();

	QueryPerformanceCounter(&TimerNow);
	TimerEnd.QuadPart = TimerNow.QuadPart + ((u * TimerFreq.QuadPart) / 1000000ull);
	TwoMSTimerTicks = (2000 * TimerFreq.QuadPart) / 1000000ull;

	while (TimerNow.QuadPart < TimerEnd.QuadPart)
	{
		if ((TimerEnd.QuadPart - TimerNow.QuadPart) > TwoMSTimerTicks)
			Sleep(1); // really pause thread for 1-2ms (depending on OS)
		else
#ifdef __MINGW32__
			{__asm__ __volatile__("pause");}
#else
			YieldProcessor(); // was: "SwitchToThread() let other threads on same core run" //!! could also try Sleep(0) or directly use _mm_pause() instead of YieldProcessor() here
#endif

		QueryPerformanceCounter(&TimerNow);
	}
}

// can sleep too long by 1000 to 2000 (=1 to 2ms)
// needs timeBeginPeriod(1) before calling 1st time to make the Sleep(1) in here behave more or less accurately (and timeEndPeriod(1) after not needing that precision anymore)
// but MAME code does this already
void uOverSleep(const UINT64 u)
{
	LARGE_INTEGER TimerEnd;
	LARGE_INTEGER TimerNow;

	if (sTimerInit == 0)
		wintimer_init();

	QueryPerformanceCounter(&TimerNow);
	TimerEnd.QuadPart = TimerNow.QuadPart + ((u * TimerFreq.QuadPart) / 1000000ull);

	while (TimerNow.QuadPart < TimerEnd.QuadPart)
	{
		Sleep(1); // really pause thread for 1-2ms (depending on OS)
		QueryPerformanceCounter(&TimerNow);
	}
}

// skips sleeping completely if u < 4000 (=4ms), otherwise will undersleep by -3000 to -2000 (=-3 to -2ms)
// needs timeBeginPeriod(1) before calling 1st time to make the Sleep(1) in here behave more or less accurately (and timeEndPeriod(1) after not needing that precision anymore)
// but MAME code does this already
void uUnderSleep(const UINT64 u)
{
	LARGE_INTEGER TimerEndSleep;
	LARGE_INTEGER TimerNow;

	if (sTimerInit == 0)
		wintimer_init();

	if (u < 4000) // Sleep < 4ms? -> exit
		return;

	QueryPerformanceCounter(&TimerNow);
	TimerEndSleep.QuadPart = TimerNow.QuadPart + (((u - 4000ull) * TimerFreq.QuadPart) / 1000000ull);

	while (TimerNow.QuadPart < TimerEndSleep.QuadPart)
	{
		Sleep(1); // really pause thread for 1-2ms (depending on OS)
		QueryPerformanceCounter(&TimerNow);
	}
}

//

typedef LONG(CALLBACK* NTSETTIMERRESOLUTION)(IN ULONG DesiredTime,
	IN BOOLEAN SetResolution,
	OUT PULONG ActualTime);
static NTSETTIMERRESOLUTION NtSetTimerResolution;

typedef LONG(CALLBACK* NTQUERYTIMERRESOLUTION)(OUT PULONG MaximumTime,
	OUT PULONG MinimumTime,
	OUT PULONG CurrentTime);
static NTQUERYTIMERRESOLUTION NtQueryTimerResolution;

static HMODULE hNtDll = NULL;
static ULONG win_timer_old_period = -1;

static TIMECAPS win_timer_caps;
static MMRESULT win_timer_result = TIMERR_NOCANDO;

void set_lowest_possible_win_timer_resolution()
{
	// First crank up the multimedia timer resolution to its max
	// this gives the system much finer timeslices (usually 1-2ms)
	win_timer_result = timeGetDevCaps(&win_timer_caps, sizeof(win_timer_caps));
	if (win_timer_result == TIMERR_NOERROR)
		timeBeginPeriod(win_timer_caps.wPeriodMin);

	// Then try the even finer sliced (usually 0.5ms) low level variant
	hNtDll = LoadLibrary("NtDll.dll");
	if (hNtDll) {
		NtQueryTimerResolution = (NTQUERYTIMERRESOLUTION)GetProcAddress(hNtDll, "NtQueryTimerResolution");
		NtSetTimerResolution = (NTSETTIMERRESOLUTION)GetProcAddress(hNtDll, "NtSetTimerResolution");
		if (NtQueryTimerResolution && NtSetTimerResolution) {
			ULONG min_period, tmp;
			NtQueryTimerResolution(&tmp, &min_period, &win_timer_old_period);
			if (min_period < 4500) // just to not screw around too much with the time (i.e. potential timer improvements in future HW/OSs), limit timer period to 0.45ms (picked 0.45 here instead of 0.5 as apparently some current setups can feature values just slightly below 0.5, so just leave them at this native rate then)
				min_period = 5000;
			if (min_period < 10000) // only set this if smaller 1ms, cause otherwise timeBeginPeriod already did the job
				NtSetTimerResolution(min_period, TRUE, &tmp);
			else
				win_timer_old_period = -1;
		}
	}
}

void restore_win_timer_resolution()
{
	// restore both timer resolutions

	if (hNtDll) {
		if (win_timer_old_period != -1)
		{
			ULONG tmp;
			NtSetTimerResolution(win_timer_old_period, FALSE, &tmp);
			win_timer_old_period = -1;
		}
		FreeLibrary(hNtDll);
		hNtDll = NULL;
	}

	if (win_timer_result == TIMERR_NOERROR)
	{
		timeEndPeriod(win_timer_caps.wPeriodMin);
		win_timer_result = TIMERR_NOCANDO;
	}
}
