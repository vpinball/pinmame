/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

  Changes 2/27/99:
  	- added some rounding to the sorting of timers so that two timers
  		allocated to go off at the same time will go off in the order
  		they were allocated, without concern for floating point rounding
  		errors (thanks Juergen!)
  	- fixed a bug where the base_time was not updated when a CPU was
  		suspended, making subsequent calls to getabsolutetime() return an
  		incorrect time (thanks Nicola!)
  	- changed suspended CPUs so that they don't eat their timeslice until
  		all other CPUs have used up theirs; this allows a slave CPU to
  		trigger a higher priority CPU in the middle of the timeslice
  	- added the ability to call timer_reset() on a oneshot or pulse timer
  		from within that timer's callback; in this case, the timer won't
  		get removed (oneshot) or won't get reprimed (pulse)

  Changes 12/17/99 (HJB):
	- added overclocking factor and functions to set/get it at runtime.

  Changes 12/23/99 (HJB):
	- added burn() function pointer to tell CPU cores when we want to
	  burn cycles, because the cores might need to adjust internal
	  counters or timers.

***************************************************************************/

#include "cpuintrf.h"
#include "driver.h"
#include "timer.h"

#include <stdarg.h>


#define VERBOSE 0

#define MAX_TIMERS 256


/*
 *		internal timer structures
 */
typedef struct timer_entry
{
	struct timer_entry *next;
	struct timer_entry *prev;
	void (*callback)(int);
	int callback_param;
	int enabled;
	double period;
	double start;
	double expire;
} timer_entry;

typedef struct
{
	int *icount;
	void (*burn)(int cycles);
    int index;
	int suspended;
	int trigger;
	int nocount;
	int lost;
	double time;
	double sec_to_cycles;
	double cycles_to_sec;
	double overclock;
} cpu_entry;


/* conversion constants */
double cycles_to_sec[MAX_CPU];
double sec_to_cycles[MAX_CPU];

/* list of per-CPU timer data */
static cpu_entry cpudata[MAX_CPU+1];
static cpu_entry *lastcpu;
static cpu_entry *activecpu;
static cpu_entry *last_activecpu;

/* list of active timers */
static timer_entry timers[MAX_TIMERS];
static timer_entry *timer_head;
static timer_entry *timer_free_head;

/* other internal states */
static double base_time;
static double global_offset;
static timer_entry *callback_timer;
static int callback_timer_modified;

/* prototypes */
static int pick_cpu(int *cpu, int *cycles, double expire);

#if VERBOSE
static void verbose_print(char *s, ...);
#endif

/*
 *		return the current absolute time
 */
INLINE double getabsolutetime(void)
{
	if (activecpu && (*activecpu->icount + activecpu->lost) > 0)
		return base_time - ((double)(*activecpu->icount + activecpu->lost) * activecpu->cycles_to_sec);
	else
		return base_time;
}


/*
 *		adjust the current CPU's timer so that a new event will fire at the right time
 */
INLINE void timer_adjust(timer_entry *timer, double time, double period)
{
	int newicount, diff;

	/* compute a new icount for the current CPU */
	if (period == TIME_NOW)
		newicount = 0;
	else
		newicount = (int)((timer->expire - time) * activecpu->sec_to_cycles) + 1;

	/* determine if we're scheduled to run more cycles */
	diff = *activecpu->icount - newicount;

	/* if so, set the new icount and compute the amount of "lost" time */
	if (diff > 0)
	{
		activecpu->lost += diff;
		if (activecpu->burn)
			(*activecpu->burn)(diff);  /* let the CPU burn the cycles */
		else
			*activecpu->icount = newicount;  /* CPU doesn't care */
	}
}


/*
 *		allocate a new timer
 */
INLINE timer_entry *timer_new(void)
{
	timer_entry *timer;

	/* remove an empty entry */
	if (!timer_free_head)
		return NULL;
	timer = timer_free_head;
	timer_free_head = timer->next;

	return timer;
}


/*
 *		insert a new timer into the list at the appropriate location
 */
INLINE void timer_list_insert(timer_entry *timer)
{
	double expire = timer->enabled ? timer->expire : TIME_NEVER;
	timer_entry *t, *lt = NULL;

	/* loop over the timer list */
	for (t = timer_head; t; lt = t, t = t->next)
	{
		/* if the current list entry expires after us, we should be inserted before it */
		/* note that due to floating point rounding, we need to allow a bit of slop here */
		/* because two equal entries -- within rounding precision -- need to sort in */
		/* the order they were inserted into the list */
		if ((t->expire - expire) > TIME_IN_NSEC(1))
		{
			/* link the new guy in before the current list entry */
			timer->prev = t->prev;
			timer->next = t;

			if (t->prev)
				t->prev->next = timer;
			else
				timer_head = timer;
			t->prev = timer;
			return;
		}
	}

	/* need to insert after the last one */
	if (lt)
		lt->next = timer;
	else
		timer_head = timer;
	timer->prev = lt;
	timer->next = NULL;
}


/*
 *		remove a timer from the linked list
 */
INLINE void timer_list_remove(timer_entry *timer)
{
	/* remove it from the list */
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
}


/*
 *		initialize the timer system
 */
void timer_init(void)
{
	cpu_entry *cpu;
	int i;

	/* keep a local copy of how many total CPU's */
	lastcpu = cpudata + cpu_gettotalcpu() - 1;

	/* we need to wait until the first call to timer_cyclestorun before using real CPU times */
	base_time = 0.0;
	global_offset = 0.0;
	callback_timer = NULL;
	callback_timer_modified = 0;

	/* reset the timers */
	memset(timers, 0, sizeof(timers));

	/* initialize the lists */
	timer_head = NULL;
	timer_free_head = &timers[0];
	for (i = 0; i < MAX_TIMERS-1; i++)
		timers[i].next = &timers[i+1];

	/* reset the CPU timers */
	memset(cpudata, 0, sizeof(cpudata));
	activecpu = NULL;
	last_activecpu = lastcpu;

	/* compute the cycle times */
	for (cpu = cpudata, i = 0; cpu <= lastcpu; cpu++, i++)
	{
		/* make a pointer to this CPU's interface functions */
		cpu->icount = cpuintf[Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK].icount;
		cpu->burn = cpuintf[Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK].burn;

		/* get the CPU's overclocking factor */
		cpu->overclock = cpuintf[Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK].overclock;

        /* everyone is active but suspended by the reset line until further notice */
		cpu->suspended = SUSPEND_REASON_RESET;

		/* set the CPU index */
		cpu->index = i;

		/* compute the cycle times */
		cpu->sec_to_cycles = sec_to_cycles[i] = cpu->overclock * Machine->drv->cpu[i].cpu_clock;
		cpu->cycles_to_sec = cycles_to_sec[i] = 1.0 / sec_to_cycles[i];
	}
}

/*
 *		get overclocking factor for a CPU
 */
double timer_get_overclock(int cpunum)
{
	cpu_entry *cpu = &cpudata[cpunum];
	return cpu->overclock;
}

/*
 *		set overclocking factor for a CPU
 */
void timer_set_overclock(int cpunum, double overclock)
{
	cpu_entry *cpu = &cpudata[cpunum];
	cpu->overclock = overclock;
	cpu->sec_to_cycles = sec_to_cycles[cpunum] = cpu->overclock * Machine->drv->cpu[cpunum].cpu_clock;
	cpu->cycles_to_sec = cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
}

/*
 *		allocate a pulse timer, which repeatedly calls the callback using the given period
 */
void *timer_pulse(double period, int param, void (*callback)(int))
{
	double time = getabsolutetime();
	timer_entry *timer;

	/* allocate a new entry */
	timer = timer_new();
	if (!timer)
		return NULL;

	/* fill in the record */
	timer->callback = callback;
	timer->callback_param = param;
	timer->enabled = 1;
	timer->period = period;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = time + period;
	timer_list_insert(timer);

	/* if we're supposed to fire before the end of this cycle, adjust the counter */
	if (activecpu && timer->expire < base_time)
		timer_adjust(timer, time, period);

	#if VERBOSE
		verbose_print("T=%.6g: New pulse=%08X, period=%.6g\n", time + global_offset, timer, period);
	#endif

	/* return a handle */
	return timer;
}


/*
 *		allocate a one-shot timer, which calls the callback after the given duration
 */
void *timer_set(double duration, int param, void (*callback)(int))
{
	double time = getabsolutetime();
	timer_entry *timer;

	/* allocate a new entry */
	timer = timer_new();
	if (!timer)
		return NULL;

	/* fill in the record */
	timer->callback = callback;
	timer->callback_param = param;
	timer->enabled = 1;
	timer->period = 0;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = time + duration;
	timer_list_insert(timer);

	/* if we're supposed to fire before the end of this cycle, adjust the counter */
	if (activecpu && timer->expire < base_time)
		timer_adjust(timer, time, duration);

	#if VERBOSE
		verbose_print("T=%.6g: New oneshot=%08X, duration=%.6g\n", time + global_offset, timer, duration);
	#endif

	/* return a handle */
	return timer;
}


/*
 *		reset the timing on a timer
 */
void timer_reset(void *which, double duration)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	/* compute the time of the next firing */
	timer->start = time;
	timer->expire = time + duration;

	/* remove the timer and insert back into the list */
	timer_list_remove(timer);
	timer_list_insert(timer);

	/* if we're supposed to fire before the end of this cycle, adjust the counter */
	if (activecpu && timer->expire < base_time)
		timer_adjust(timer, time, duration);

	/* if this is the callback timer, mark it modified */
	if (timer == callback_timer)
		callback_timer_modified = 1;

	#if VERBOSE
		verbose_print("T=%.6g: Reset %08X, duration=%.6g\n", time + global_offset, timer, duration);
	#endif
}


/*
 *		remove a timer from the system
 */
void timer_remove(void *which)
{
	timer_entry *timer = which;

	/* remove it from the list */
	timer_list_remove(timer);

	/* free it up by adding it back to the free list */
	timer->next = timer_free_head;
	timer_free_head = timer;

	#if VERBOSE
		verbose_print("T=%.6g: Removed %08X\n", getabsolutetime() + global_offset, timer);
	#endif
}


/*
 *		enable/disable a timer
 */
int timer_enable(void *which, int enable)
{
	timer_entry *timer = which;
	int old;

	#if VERBOSE
		if (enable) verbose_print("T=%.6g: Enabled %08X\n", getabsolutetime() + global_offset, timer);
		else verbose_print("T=%.6g: Disabled %08X\n", getabsolutetime() + global_offset, timer);
	#endif

	/* set the enable flag */
	old = timer->enabled;
	timer->enabled = enable;

	/* remove the timer and insert back into the list */
	timer_list_remove(timer);
	timer_list_insert(timer);

	return old;
}


/*
 *		return the time since the last trigger
 */
double timer_timeelapsed(void *which)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	return time - timer->start;
}


/*
 *		return the time until the next trigger
 */
double timer_timeleft(void *which)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	return timer->expire - time;
}


/*
 *		return the current time
 */
double timer_get_time(void)
{
	return global_offset + getabsolutetime();
}


/*
 *		return the time when this timer started counting
 */
double timer_starttime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->start;
}


/*
 *		return the time when this timer will fire next
 */
double timer_firetime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->expire;
}


/*
 *		begin CPU execution by determining how many cycles the CPU should run
 */
int timer_schedule_cpu(int *cpu, int *cycles)
{
	double end;

	/* then see if there are any CPUs that aren't suspended and haven't yet been updated */
	if (pick_cpu(cpu, cycles, timer_head->expire))
		return 1;

	/* everyone is up-to-date; expire any timers now */
	end = timer_head->expire;
	while (timer_head->expire <= end)
	{
		timer_entry *timer = timer_head;

		/* the base time is now the time of the timer */
		base_time = timer->expire;

		#if VERBOSE
			verbose_print("T=%.6g: %08X fired (exp time=%.6g)\n", getabsolutetime() + global_offset, timer, timer->expire + global_offset);
		#endif

		/* set the global state of which callback we're in */
		callback_timer_modified = 0;
		callback_timer = timer;

		/* call the callback */
		if (timer->callback)
		{
			profiler_mark(PROFILER_TIMER_CALLBACK);
			(*timer->callback)(timer->callback_param);
			profiler_mark(PROFILER_END);
		}

		/* clear the callback timer global */
		callback_timer = NULL;

		/* reset or remove the timer, but only if it wasn't modified during the callback */
		if (!callback_timer_modified)
		{
			if (timer->period)
			{
				timer->start = timer->expire;
				timer->expire += timer->period;

				timer_list_remove(timer);
				timer_list_insert(timer);
			}
			else
				timer_remove(timer);
		}
	}

	/* reset scheduling so it starts with CPU 0 */
	last_activecpu = lastcpu;

#ifdef MAME_DEBUG
{
	extern int debug_key_delay;
	debug_key_delay = 0x7ffe;
}
#endif

	/* go back to scheduling */
	return pick_cpu(cpu, cycles, timer_head->expire);
}


/*
 *		end CPU execution by updating the number of cycles the CPU actually ran
 */
void timer_update_cpu(int cpunum, int ran)
{
	cpu_entry *cpu = cpudata + cpunum;

	/* update the time if we haven't been suspended */
	if (!cpu->suspended)
	{
		cpu->time += (double)(ran - cpu->lost) * cpu->cycles_to_sec;
		cpu->lost = 0;
	}

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d finished (net=%d)\n", cpu->time + global_offset, cpunum, ran - cpu->lost);
	#endif

	/* time to renormalize? */
	if (cpu->time >= 1.0)
	{
		timer_entry *timer;
		double one = 1.0;
		cpu_entry *c;

		#if VERBOSE
			verbose_print("T=%.6g: Renormalizing\n", cpu->time + global_offset);
		#endif

		/* renormalize all the CPU timers */
		for (c = cpudata; c <= lastcpu; c++)
			c->time -= one;

		/* renormalize all the timers' times */
		for (timer = timer_head; timer; timer = timer->next)
		{
			timer->start -= one;
			timer->expire -= one;
		}

		/* renormalize the global timers */
		global_offset += one;
	}

	/* now stop counting cycles */
	base_time = cpu->time;
	activecpu = NULL;
}


/*
 *		suspend a CPU but continue to count time for it
 */
void timer_suspendcpu(int cpunum, int suspend, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	int nocount = cpu->nocount;
	int old = cpu->suspended;

	#if VERBOSE
		if (suspend) verbose_print("T=%.6g: Suspending CPU %d\n", getabsolutetime() + global_offset, cpunum);
		else verbose_print("T=%.6g: Resuming CPU %d\n", getabsolutetime() + global_offset, cpunum);
	#endif

	/* mark the CPU */
	if (suspend)
		cpu->suspended |= reason;
	else
		cpu->suspended &= ~reason;
	cpu->nocount = 0;

	/* if this is the active CPU and we're halting, stop immediately */
	if (activecpu && cpu == activecpu && !old && cpu->suspended)
	{
		#if VERBOSE
			verbose_print("T=%.6g: Reset ICount\n", getabsolutetime() + global_offset);
		#endif

		/* set the CPU's time to the current time */
		cpu->time = base_time = getabsolutetime();	/* ASG 990225 - also set base_time */
		cpu->lost = 0;

		/* no more instructions */
		if (cpu->burn)
			(*cpu->burn)(*cpu->icount); /* let the CPU burn the cycles */
		else
			*cpu->icount = 0;	/* CPU doesn't care */
    }

	/* else if we're unsuspending a CPU, reset its time */
	else if (old && !cpu->suspended && !nocount)
	{
		double time = getabsolutetime();

		/* only update the time if it's later than the CPU's time */
		if (time > cpu->time)
			cpu->time = time;
		cpu->lost = 0;

		#if VERBOSE
			verbose_print("T=%.6g: Resume time\n", cpu->time + global_offset);
		#endif
	}
}



/*
 *		hold a CPU and don't count time for it
 */
void timer_holdcpu(int cpunum, int hold, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;

	/* same as suspend */
	timer_suspendcpu(cpunum, hold, reason);

	/* except that we don't count time */
	if (hold)
		cpu->nocount = 1;
}



/*
 *		query if a CPU is suspended or not
 */
int timer_iscpususpended(int cpunum, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	return (cpu->suspended & reason) && !cpu->nocount;
}



/*
 *		query if a CPU is held or not
 */
int timer_iscpuheld(int cpunum, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	return (cpu->suspended & reason) && cpu->nocount;
}



/*
 *		suspend a CPU until a specified trigger condition is met
 */
void timer_suspendcpu_trigger(int cpunum, int trigger)
{
	cpu_entry *cpu = cpudata + cpunum;

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d suspended until %d\n", getabsolutetime() + global_offset, cpunum, trigger);
	#endif

	/* suspend the CPU immediately if it's not already */
	timer_suspendcpu(cpunum, 1, SUSPEND_REASON_TRIGGER);

	/* set the trigger */
	cpu->trigger = trigger;
}



/*
 *		hold a CPU and don't count time for it
 */
void timer_holdcpu_trigger(int cpunum, int trigger)
{
	cpu_entry *cpu = cpudata + cpunum;

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d held until %d\n", getabsolutetime() + global_offset, cpunum, trigger);
	#endif

	/* suspend the CPU immediately if it's not already */
	timer_holdcpu(cpunum, 1, SUSPEND_REASON_TRIGGER);

	/* set the trigger */
	cpu->trigger = trigger;
}



/*
 *		generates a trigger to unsuspend any CPUs waiting for it
 */
void timer_trigger(int trigger)
{
	cpu_entry *cpu;

	/* cause an immediate resynchronization */
	if (activecpu)
	{
		int left = *activecpu->icount;
		if (left > 0)
		{
			activecpu->lost += left;
			if (activecpu->burn)
				(*activecpu->burn)(left); /* let the CPU burn the cycles */
			else
				*activecpu->icount = 0; /* CPU doesn't care */
		}
	}

	/* look for suspended CPUs waiting for this trigger and unsuspend them */
	for (cpu = cpudata; cpu <= lastcpu; cpu++)
	{
		if (cpu->suspended && cpu->trigger == trigger)
		{
			#if VERBOSE
				verbose_print("T=%.6g: CPU %d triggered\n", getabsolutetime() + global_offset, cpu->index);
			#endif

			timer_suspendcpu(cpu->index, 0, SUSPEND_REASON_TRIGGER);
			cpu->trigger = 0;
		}
	}
}


/*
 *		pick the next CPU to run
 */
static int pick_cpu(int *cpunum, int *cycles, double end)
{
	cpu_entry *cpu = last_activecpu;

	/* look for a CPU that isn't suspended and hasn't run its full timeslice yet */
	do
	{
		/* wrap around */
		cpu += 1;
		if (cpu > lastcpu)
			cpu = cpudata;

		/* if this CPU is suspended, just bump its time */
		if (cpu->suspended)
		{
			/* ASG 990225 - defer this update until the slice has finished */
/*			if (!cpu->nocount)
			{
				cpu->time = end;
				cpu->lost = 0;
			}*/
		}

		/* if this CPU isn't suspended and has time left.... */
		else if (cpu->time < end)
		{
			/* mark the CPU active, and remember the CPU number locally */
			activecpu = last_activecpu = cpu;

			/* return the number of cycles to execute and the CPU number */
			*cpunum = cpu->index;
			*cycles = (int)((double)(end - cpu->time) * cpu->sec_to_cycles);

			if (*cycles > 0)
			{
				#if VERBOSE
					verbose_print("T=%.6g: CPU %d runs %d cycles\n", cpu->time + global_offset, *cpunum, *cycles);
				#endif

				/* remember the base time for this CPU */
				base_time = cpu->time + ((double)*cycles * cpu->cycles_to_sec);

				/* success */
				return 1;
			}
		}
	}
	while (cpu != last_activecpu);

	/* ASG 990225 - bump all suspended CPU times after the slice has finished */
	for (cpu = cpudata; cpu <= lastcpu; cpu++)
		if (cpu->suspended && !cpu->nocount)
		{
			cpu->time = end;
			cpu->lost = 0;
		}

	/* failure */
	return 0;
}



/*
 *		debugging
 */
#if VERBOSE

#ifdef macintosh
#undef printf
#endif

static void verbose_print(char *s, ...)
{
	va_list ap;

	va_start(ap, s);

	#if (VERBOSE == 1)
		if (errorlog) vfprintf(errorlog, s, ap);
	#else
		vprintf(s, ap);
		fflush(NULL);
	#endif

	va_end(ap);
}

#endif
