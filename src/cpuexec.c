/***************************************************************************

	cpuexec.c

	Core multi-CPU execution engine.

***************************************************************************/

#include <signal.h>
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"
#include "hiscore.h"

#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
#include "cpu/m68000/m68000.h"
#endif


/*************************************
 *
 *	Debug logging
 *
 *************************************/

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *	Macros to help verify active CPU
 *
 *************************************/

#define VERIFY_ACTIVECPU(retval, name)						\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return retval;										\
	}

#define VERIFY_ACTIVECPU_VOID(name)							\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		logerror(#name "() called with no active cpu!\n");	\
		return;												\
	}



/*************************************
 *
 *	Triggers for the timer system
 *
 *************************************/

enum
{
	TRIGGER_TIMESLICE 	= -1000,
	TRIGGER_INT 		= -2000,
	TRIGGER_YIELDTIME 	= -3000,
	TRIGGER_SUSPENDTIME = -4000
};



/*************************************
 *
 *	Internal CPU info structure
 *
 *************************************/

struct cpuinfo
{
	int 	iloops; 				/* number of interrupts remaining this frame */
	int 	totalcycles;			/* total CPU cycles executed */
	int 	vblankint_countdown;	/* number of vblank callbacks left until we interrupt */
	int 	vblankint_multiplier;	/* number of vblank callbacks per interrupt */
	void *	vblankint_timer;		/* reference to elapsed time counter */
	double	vblankint_period;		/* timing period of the VBLANK interrupt */
	void *	timedint_timer;			/* reference to this CPU's timer */
	double	timedint_period; 		/* timing period of the timed interrupt */
};



/*************************************
 *
 *	General CPU variables
 *
 *************************************/

static struct cpuinfo cpu[MAX_CPU];

static int time_to_reset;
static int time_to_quit;

static int vblank;
static int current_frame;
static INT32 watchdog_counter;

static int cycles_running;



/*************************************
 *
 *	Timer variables
 *
 *************************************/

static void *vblank_timer;
static int vblank_countdown;
static int vblank_multiplier;
static double vblank_period;

static void *refresh_timer;
static double refresh_period;
static double refresh_period_inv;

static void *timeslice_timer;
static double timeslice_period;

static double scanline_period;
static double scanline_period_inv;



/*************************************
 *
 *	Save/load variables
 *
 *************************************/

static int loadsave_schedule;
static char loadsave_schedule_id;



/*************************************
 *
 *	Static prototypes
 *
 *************************************/

static void cpu_inittimers(void);
static void cpu_vblankreset(void);
static void cpu_vblankcallback(int param);
static void cpu_updatecallback(int param);

static void handle_loadsave(void);



#if 0
#pragma mark CORE CPU
#endif

/*************************************
 *
 *	Initialize all the CPUs
 *
 *************************************/

int cpu_init(void)
{
	int cpunum;

	/* initialize the interfaces first */
	if (cpuintrf_init())
		return 1;

	/* loop over all our CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type & ~CPU_FLAGS_MASK;

		/* if this is a dummy, stop looking */
		if (cputype == CPU_DUMMY)
			break;

		/* set the save state tag */
		state_save_set_current_tag(cpunum + 1);

		/* initialize this CPU */
		if (cpuintrf_init_cpu(cpunum, cputype))
			return 1;
	}

	/* save some stuff in tag 0 */
	state_save_set_current_tag(0);
	state_save_register_INT32("cpu", 0, "watchdog count", &watchdog_counter, 1);

	/* reset the IRQ lines and save those */
	if (cpuint_init())
		return 1;

	return 0;
}



/*************************************
 *
 *	Prepare the system for execution
 *
 *************************************/

static void cpu_pre_run(void)
{
	int cpunum;

	logerror("Machine reset\n");

	begin_resource_tracking();

	/* read hi scores information from hiscore.dat */
	hs_open(Machine->gamedrv->name);
	hs_init();

	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers();
	watchdog_counter = -1;

	/* reset sound chips */
	sound_reset();

	/* first pass over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* enable all CPUs (except for audio CPUs if the sound is off) */
		if (!(Machine->drv->cpu[cpunum].cpu_type & CPU_AUDIO_CPU) || Machine->sample_rate != 0)
			timer_suspendcpu(cpunum, 0, SUSPEND_ANY_REASON);
		else
			timer_suspendcpu(cpunum, 1, SUSPEND_REASON_DISABLE);

		/* reset the interrupt state */
		cpuint_reset_cpu(cpunum);

		/* reset the total number of cycles */
		cpu[cpunum].totalcycles = 0;
	}

	vblank = 0;

	/* do this AFTER the above so machine_init() can use cpu_halt() to hold the */
	/* execution of some CPUs, or disable interrupts */
	if (Machine->drv->machine_init)
		(*Machine->drv->machine_init)();

	/* now reset each CPU */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
#ifdef PINMAME
        	if (!(Machine->drv->cpu[cpunum].cpu_type & CPU_AUDIO_CPU) || Machine->sample_rate != 0)
#endif /* PINMAME */
		cpunum_reset(cpunum, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]);

	/* reset the globals */
	cpu_vblankreset();
	current_frame = 0;
	state_save_dump_registry();
}



/*************************************
 *
 *	Finish up execution
 *
 *************************************/

static void cpu_post_run(void)
{
	/* write hi scores to disk - No scores saving if cheat */
	hs_close();

	/* stop the machine */
	if (Machine->drv->machine_stop)
		(*Machine->drv->machine_stop)();

	end_resource_tracking();
}



/*************************************
 *
 *	Execute until done
 *
 *************************************/

void cpu_run(void)
{
	int cpunum;

#ifdef MAME_DEBUG
	/* initialize the debugger */
	if (mame_debug)
		mame_debug_init();
#endif

	/* loop over multiple resets, until the user quits */
	time_to_quit = 0;
	while (!time_to_quit)
	{
		/* prepare everything to run */
		cpu_pre_run();

		/* loop until the user quits or resets */
		time_to_reset = 0;
		while (!time_to_quit && !time_to_reset)
		{
			profiler_mark(PROFILER_EXTRA);

			/* if we have a load/save scheduled, handle it */
			if (loadsave_schedule != LOADSAVE_NONE)
				handle_loadsave();

			/* ask the timer system to schedule */
			if (timer_schedule_cpu(&cpunum, &cycles_running))
			{
				int ran;

				/* run for the requested number of cycles */
				profiler_mark(PROFILER_CPU1 + cpunum);
				ran = cpunum_execute(cpunum, cycles_running);
				profiler_mark(PROFILER_END);

				/* update based on how many cycles we really ran */
				cpu[cpunum].totalcycles += ran;

				/* update the timer with how long we actually ran */
				timer_update_cpu(cpunum, ran);
			}

			profiler_mark(PROFILER_END);
		}

		/* finish up this iteration */
		cpu_post_run();
	}

#ifdef MAME_DEBUG
	/* shut down the debugger */
	if (mame_debug)
		mame_debug_exit();
#endif
}



/*************************************
 *
 *	Deinitialize all the CPUs
 *
 *************************************/

void cpu_exit(void)
{
	int cpunum;

	/* shut down the CPU cores */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		cpuintrf_exit_cpu(cpunum);
}



/*************************************
 *
 *	Force a reset at the end of this
 *	timeslice
 *
 *************************************/

void machine_reset(void)
{
	time_to_reset = 1;
}




#if 0
#pragma mark -
#pragma mark SAVE/RESTORE
#endif

/*************************************
 *
 *	Handle saves at runtime
 *
 *************************************/

static void handle_save(void)
{
	char name[2] = { 0 };
	void *file;
	int cpunum;

	/* open the file */
	name[0] = loadsave_schedule_id;
	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_STATE, 1);

	/* write the save state */
	state_save_save_begin(file);

	/* write tag 0 */
	state_save_set_current_tag(0);
	state_save_save_continue();

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		cpuintrf_push_context(cpunum);

		/* make sure banking is set */
		activecpu_reset_banking();

		/* save the CPU data */
		state_save_set_current_tag(cpunum + 1);
		state_save_save_continue();

		cpuintrf_pop_context();
	}

	/* finish and close */
	state_save_save_finish();
	osd_fclose(file);

	/* unschedule the save */
	loadsave_schedule = LOADSAVE_NONE;
}



/*************************************
 *
 *	Handle loads at runtime
 *
 *************************************/

static void handle_load(void)
{
	char name[2] = { 0 };
	void *file;
	int cpunum;

	/* open the file */
	name[0] = loadsave_schedule_id;
	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_STATE, 0);

	/* if successful, load it */
	if (file)
	{
		/* start loading */
		if (!state_save_load_begin(file))
		{
			/* read tag 0 */
			state_save_set_current_tag(0);
			state_save_load_continue();

			/* loop over CPUs */
			for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
			{
				cpuintrf_push_context(cpunum);

				/* make sure banking is set */
				activecpu_reset_banking();

				/* load the CPU data */
				state_save_set_current_tag(cpunum + 1);
				state_save_load_continue();

				cpuintrf_pop_context();
			}

			/* finish and close */
			state_save_load_finish();
		}
		osd_fclose(file);
	}

	/* unschedule the load */
	loadsave_schedule = LOADSAVE_NONE;
}



/*************************************
 *
 *	Handle saves & loads at runtime
 *
 *************************************/

static void handle_loadsave(void)
{
	/* it's one or the other */
	if (loadsave_schedule == LOADSAVE_SAVE)
		handle_save();
	else if (loadsave_schedule == LOADSAVE_LOAD)
		handle_load();

	/* reset the schedule */
	loadsave_schedule = LOADSAVE_NONE;
}



/*************************************
 *
 *	Schedules a save/load for later
 *
 *************************************/

void cpu_loadsave_schedule(int type, char id)
{
	loadsave_schedule = type;
	loadsave_schedule_id = id;
}



/*************************************
 *
 *	Unschedules any saves or loads
 *
 *************************************/

void cpu_loadsave_reset(void)
{
	loadsave_schedule = LOADSAVE_NONE;
}


#if 0
#pragma mark -
#pragma mark WATCHDOG
#endif

/*************************************
 *
 *	Watchdog routines
 *
 *************************************/

/*--------------------------------------------------------------

	Use these functions to initialize, and later maintain, the
	watchdog. For convenience, when the machine is reset, the
	watchdog is disabled. If you call this function, the
	watchdog is initialized, and from that point onwards, if you
	don't call it at least once every 3 seconds, the machine
	will be reset.

	The 3 seconds delay is targeted at qzshowby, which otherwise
	would reset at the start of a game.

--------------------------------------------------------------*/

static void watchdog_reset(void)
{
	if (watchdog_counter == -1)
		logerror("watchdog armed\n");
	watchdog_counter = 3 * Machine->drv->frames_per_second;
}


WRITE_HANDLER( watchdog_reset_w )
{
	watchdog_reset();
}


READ_HANDLER( watchdog_reset_r )
{
	watchdog_reset();
	return 0xff;
}


WRITE16_HANDLER( watchdog_reset16_w )
{
	watchdog_reset();
}


READ16_HANDLER( watchdog_reset16_r )
{
	watchdog_reset();
	return 0xffff;
}


WRITE32_HANDLER( watchdog_reset32_w )
{
	watchdog_reset();
}


READ32_HANDLER( watchdog_reset32_r )
{
	watchdog_reset();
	return 0xffffffff;
}



#if 0
#pragma mark -
#pragma mark HALT/RESET
#endif

/*************************************
 *
 *	Handle reset line changes
 *
 *************************************/

static void reset_callback(int param)
{
	int cpunum = param & 0xff;
	int state = param >> 8;

	/* if we're asserting the line, just halt the CPU */
	if (state == ASSERT_LINE)
	{
		timer_suspendcpu(cpunum, 1, SUSPEND_REASON_RESET);
		return;
	}

	/* if we're clearing the line that was previously asserted, or if we're just */
	/* pulsing the line, reset the CPU */
	if ((state == CLEAR_LINE && timer_iscpususpended(cpunum, SUSPEND_REASON_RESET)) || state == PULSE_LINE)
		cpunum_reset(cpunum, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]);

	/* if we're clearing the line, make sure the CPU is not halted */
	timer_suspendcpu(cpunum, 0, SUSPEND_REASON_RESET);
}


void cpu_set_reset_line(int cpunum, int state)
{
	timer_set(TIME_NOW, (cpunum & 0xff) | (state << 8), reset_callback);
}



/*************************************
 *
 *	Handle halt line changes
 *
 *************************************/

static void halt_callback(int param)
{
	int cpunum = param & 0xff;
	int state = param >> 8;

	/* if asserting, halt the CPU */
	if (state == ASSERT_LINE)
		timer_suspendcpu(cpunum, 1, SUSPEND_REASON_HALT);

	/* if clearing, unhalt the CPU */
	else if (state == CLEAR_LINE)
		timer_suspendcpu(cpunum, 0, SUSPEND_REASON_HALT);
}


void cpu_set_halt_line(int cpunum, int state)
{
	timer_set(TIME_NOW, (cpunum & 0xff) | (state << 8), halt_callback);
}



/*************************************
 *
 *	Return suspended status of CPU
 *
 *************************************/

int cpu_getstatus(int cpunum)
{
	if (cpunum < cpu_gettotalcpu())
		return !timer_iscpususpended(cpunum, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE);
	return 0;
}



#if 0
#pragma mark -
#pragma mark TIMING HELPERS
#endif

/*************************************
 *
 *	Return cycles ran this iteration
 *
 *************************************/

int cycles_currently_ran(void)
{
	VERIFY_ACTIVECPU(0, cycles_currently_ran);
	return cycles_running - activecpu_get_icount();
}



/*************************************
 *
 *	Return cycles remaining in this
 *	iteration
 *
 *************************************/

int cycles_left_to_run(void)
{
	VERIFY_ACTIVECPU(0, cycles_left_to_run);
	return activecpu_get_icount();
}



/*************************************
 *
 *	Return total number of CPU cycles
 *	for the active CPU.
 *
 *************************************/

/*--------------------------------------------------------------

	IMPORTANT: this value wraps around in a relatively short
	time. For example, for a 6MHz CPU, it will wrap around in
	2^32/6000000 = 716 seconds = 12 minutes.
	Make sure you don't do comparisons between values returned
	by this function, but only use the difference (which will
	be correct regardless of wraparound).

--------------------------------------------------------------*/

int cpu_gettotalcycles(void)
{
	VERIFY_ACTIVECPU(0, cpu_gettotalcycles);
	return cpu[activecpu].totalcycles + cycles_currently_ran();
}



/*************************************
 *
 *	Return cycles until next interrupt
 *	handler call
 *
 *************************************/

int cpu_geticount(void)
{
	int result;

/* remove me - only used by mamedbg, m92 */
	VERIFY_ACTIVECPU(0, cpu_geticount);
	result = TIME_TO_CYCLES(activecpu, cpu[activecpu].vblankint_period - timer_timeelapsed(cpu[activecpu].vblankint_timer));
	return (result < 0) ? 0 : result;
}



/*************************************
 *
 *	Scales a given value by the fraction
 *	of time elapsed between refreshes
 *
 *************************************/

int cpu_scalebyfcount(int value)
{
	int result = (int)((double)value * timer_timeelapsed(refresh_timer) * refresh_period_inv);
	if (value >= 0)
		return (result < value) ? result : value;
	else
		return (result > value) ? result : value;
}



#if 0
#pragma mark -
#pragma mark VIDEO TIMING
#endif

/*************************************
 *
 *	Creates the refresh timer
 *
 *************************************/

void cpu_init_refresh_timer(void)
{
	/* allocate an infinite timer to track elapsed time since the last refresh */
	refresh_period = TIME_IN_HZ(Machine->drv->frames_per_second);
	refresh_period_inv = 1.0 / refresh_period;
	refresh_timer = timer_alloc(NULL);

	/* while we're at it, compute the scanline times */
	if (Machine->drv->vblank_duration)
		scanline_period = (refresh_period - TIME_IN_USEC(Machine->drv->vblank_duration)) /
				(double)(Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1);
	else
		scanline_period = refresh_period / (double)Machine->drv->screen_height;
	scanline_period_inv = 1.0 / scanline_period;
}



/*************************************
 *
 *	Returns the current scanline
 *
 *************************************/

/*--------------------------------------------------------------

	Note: cpu_getscanline() counts from 0, 0 being the first
	visible line. You might have to adjust this value to match
	the hardware, since in many cases the first visible line
	is >0.

--------------------------------------------------------------*/

int cpu_getscanline(void)
{
	return (int)(timer_timeelapsed(refresh_timer) * scanline_period_inv);
}



/*************************************
 *
 *	Returns time until given scanline
 *
 *************************************/

double cpu_getscanlinetime(int scanline)
{
	double scantime = timer_starttime(refresh_timer) + (double)scanline * scanline_period;
	double abstime = timer_get_time();
	double result;

	/* if we're already past the computed time, count it for the next frame */
	if (abstime >= scantime)
		scantime += TIME_IN_HZ(Machine->drv->frames_per_second);

	/* compute how long from now until that time */
	result = scantime - abstime;

	/* if it's small, just count a whole frame */
	if (result < TIME_IN_NSEC(1))
		result = TIME_IN_HZ(Machine->drv->frames_per_second);
	return result;
}



/*************************************
 *
 *	Returns time for one scanline
 *
 *************************************/

double cpu_getscanlineperiod(void)
{
	return scanline_period;
}



/*************************************
 *
 *	Returns a crude approximation
 *	of the horizontal position of the
 *	bream
 *
 *************************************/

int cpu_gethorzbeampos(void)
{
	double elapsed_time = timer_timeelapsed(refresh_timer);
	int scanline = (int)(elapsed_time * scanline_period_inv);
	double time_since_scanline = elapsed_time - (double)scanline * scanline_period;
	return (int)(time_since_scanline * scanline_period_inv * (double)Machine->drv->screen_width);
}



/*************************************
 *
 *	Returns the VBLANK state
 *
 *************************************/

int cpu_getvblank(void)
{
	return vblank;
}



/*************************************
 *
 *	Returns the current frame count
 *
 *************************************/

int cpu_getcurrentframe(void)
{
	return current_frame;
}



#if 0
#pragma mark -
#pragma mark SYNCHRONIZATION
#endif

/*************************************
 *
 *	Generate a specific trigger
 *
 *************************************/

void cpu_trigger(int trigger)
{
	timer_trigger(trigger);
}



/*************************************
 *
 *	Generate a trigger in the future
 *
 *************************************/

void cpu_triggertime(double duration, int trigger)
{
	timer_set(duration, trigger, cpu_trigger);
}



/*************************************
 *
 *	Generate a trigger for an int
 *
 *************************************/

void cpu_triggerint(int cpunum)
{
	timer_trigger(TRIGGER_INT + cpunum);
}



/*************************************
 *
 *	Burn/yield CPU cycles until a trigger
 *
 *************************************/

void cpu_spinuntil_trigger(int trigger)
{
	VERIFY_ACTIVECPU_VOID(cpu_spinuntil_trigger);
	timer_suspendcpu_trigger(activecpu, trigger);
}


void cpu_yielduntil_trigger(int trigger)
{
	VERIFY_ACTIVECPU_VOID(cpu_yielduntil_trigger);
	timer_holdcpu_trigger(activecpu, trigger);
}



/*************************************
 *
 *	Burn/yield CPU cycles until an
 *	interrupt
 *
 *************************************/

void cpu_spinuntil_int(void)
{
	VERIFY_ACTIVECPU_VOID(cpu_spinuntil_int);
	cpu_spinuntil_trigger(TRIGGER_INT + activecpu);
}


void cpu_yielduntil_int(void)
{
	VERIFY_ACTIVECPU_VOID(cpu_yielduntil_int);
	cpu_yielduntil_trigger(TRIGGER_INT + activecpu);
}



/*************************************
 *
 *	Burn/yield CPU cycles until the
 *	end of the current timeslice
 *
 *************************************/

void cpu_spin(void)
{
	cpu_spinuntil_trigger(TRIGGER_TIMESLICE);
}


void cpu_yield(void)
{
	cpu_yielduntil_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *	Burn/yield CPU cycles for a
 *	specific period of time
 *
 *************************************/

void cpu_spinuntil_time(double duration)
{
	static int timetrig = 0;

	cpu_spinuntil_trigger(TRIGGER_SUSPENDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_SUSPENDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}


void cpu_yielduntil_time(double duration)
{
	static int timetrig = 0;

	cpu_yielduntil_trigger(TRIGGER_YIELDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_YIELDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



#if 0
#pragma mark -
#pragma mark CORE TIMING
#endif

/*************************************
 *
 *	Returns the number of times the
 *	interrupt handler will be called
 *	before the end of the current
 *	video frame.
 *
 *************************************/

/*--------------------------------------------------------------

	This can be useful to interrupt handlers to synchronize
	their operation. If you call this from outside an interrupt
	handler, add 1 to the result, i.e. if it returns 0, it means
	that the interrupt handler will be called once.

--------------------------------------------------------------*/

int cpu_getiloops(void)
{
	VERIFY_ACTIVECPU(0, cpu_getiloops);
	return cpu[activecpu].iloops;
}



/*************************************
 *
 *	Hook for updating things on the
 *	real VBLANK (once per frame)
 *
 *************************************/

static void cpu_vblankreset(void)
{
	int cpunum;

	/* read hi scores from disk */
	hs_update();

	/* read keyboard & update the status of the input ports */
	update_input_ports();

	/* reset the cycle counters */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		if (!timer_iscpususpended(cpunum, SUSPEND_REASON_DISABLE))
			cpu[cpunum].iloops = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame - 1;
		else
			cpu[cpunum].iloops = -1;
	}
}



/*************************************
 *
 *	First-run callback for VBLANKs
 *
 *************************************/

static void cpu_firstvblankcallback(int param)
{
	/* now that we're synced up, pulse from here on out */
	timer_adjust(vblank_timer, vblank_period, param, vblank_period);

	/* but we need to call the standard routine as well */
	cpu_vblankcallback(param);
}



/*************************************
 *
 *	VBLANK core handler
 *
 *************************************/

static void cpu_vblankcallback(int param)
{
	int cpunum;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* if the interrupt multiplier is valid */
		if (cpu[cpunum].vblankint_multiplier != -1)
		{
			/* decrement; if we hit zero, generate the interrupt and reset the countdown */
			if (!--cpu[cpunum].vblankint_countdown)
			{
				/* a param of -1 means don't call any callbacks */
				if (param != -1)
				{
					/* if the CPU has a VBLANK handler, call it */
					if (Machine->drv->cpu[cpunum].vblank_interrupt && cpu_getstatus(cpunum))
					{
						cpuintrf_push_context(cpunum);
						(*Machine->drv->cpu[cpunum].vblank_interrupt)();
						cpuintrf_pop_context();
					}

					/* update the counters */
					cpu[cpunum].iloops--;
				}

				/* reset the countdown and timer */
				cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier;
				timer_adjust(cpu[cpunum].vblankint_timer, TIME_NEVER, 0, 0);
			}
		}

		/* else reset the VBLANK timer if this is going to be a real VBLANK */
		else if (vblank_countdown == 1)
			timer_adjust(cpu[cpunum].vblankint_timer, TIME_NEVER, 0, 0);
	}

	/* is it a real VBLANK? */
	if (!--vblank_countdown)
	{
		/* do we update the screen now? */
		if (!(Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK))
			time_to_quit = updatescreen();

		/* Set the timer to update the screen */
		timer_set(TIME_IN_USEC(Machine->drv->vblank_duration), 0, cpu_updatecallback);
		vblank = 1;

		/* reset the globals */
		cpu_vblankreset();

		/* reset the counter */
		vblank_countdown = vblank_multiplier;
	}
}



/*************************************
 *
 *	End-of-VBLANK callback
 *
 *************************************/

static void cpu_updatecallback(int param)
{
	/* update the screen if we didn't before */
	if (Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK)
		time_to_quit = updatescreen();
	vblank = 0;

	/* update IPT_VBLANK input ports */
	inputport_vblank_end();

	/* reset partial updating */
	reset_partial_updates();

	/* check the watchdog */
	if (watchdog_counter > 0)
		if (--watchdog_counter == 0)
		{
			logerror("reset caused by the watchdog\n");
			machine_reset();
		}

	/* track total frames */
	current_frame++;

	/* reset the refresh timer */
	timer_adjust(refresh_timer, TIME_NEVER, 0, 0);
}



/*************************************
 *
 *	Callback for timed interrupts
 *	(not tied to a VBLANK)
 *
 *************************************/

static void cpu_timedintcallback(int param)
{
	/* bail if there is no routine */
	if (Machine->drv->cpu[param].timed_interrupt && cpu_getstatus(param))
	{
		cpuintrf_push_context(param);
		(*Machine->drv->cpu[param].timed_interrupt)();
		cpuintrf_pop_context();
	}
}



/*************************************
 *
 *	Converts an integral timing rate
 *	into a period
 *
 *************************************/

/*--------------------------------------------------------------

	Rates can be specified as follows:

		rate <= 0		-> 0
		rate < 50000	-> 'rate' cycles per frame
		rate >= 50000	-> 'rate' nanoseconds

--------------------------------------------------------------*/

static double cpu_computerate(int value)
{
	/* values equal to zero are zero */
	if (value <= 0)
		return 0.0;

	/* values above between 0 and 50000 are in Hz */
	if (value < 50000)
		return TIME_IN_HZ(value);

	/* values greater than 50000 are in nanoseconds */
	else
		return TIME_IN_NSEC(value);
}



/*************************************
 *
 *	Callback to force a timeslice
 *
 *************************************/

static void cpu_timeslicecallback(int param)
{
	timer_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *	Setup all the core timers
 *
 *************************************/

static void cpu_inittimers(void)
{
	double first_time;
	int cpunum, max, ipf;

	/* allocate a dummy timer at the minimum frequency to break things up */
	ipf = Machine->drv->cpu_slices_per_frame;
	if (ipf <= 0)
		ipf = 1;
	timeslice_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
	timeslice_timer = timer_alloc(cpu_timeslicecallback);
	timer_adjust(timeslice_timer, timeslice_period, 0, timeslice_period);

	/*
	 *	The following code finds all the CPUs that are interrupting in sync with the VBLANK
	 *	and sets up the VBLANK timer to run at the minimum number of cycles per frame in
	 *	order to service all the synced interrupts
	 */

	/* find the CPU with the maximum interrupts per frame */
	max = 1;
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > max)
			max = ipf;
	}

	/* now find the LCD with the rest of the CPUs (brute force - these numbers aren't huge) */
	vblank_multiplier = max;
	while (1)
	{
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
			if (ipf > 0 && (vblank_multiplier % ipf) != 0)
				break;
		}
		if (cpunum == cpu_gettotalcpu())
			break;
		vblank_multiplier += max;
	}

	/* initialize the countdown timers and intervals */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > 0)
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = vblank_multiplier / ipf;
		else
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = -1;
	}

	/* allocate a vblank timer at the frame rate * the LCD number of interrupts per frame */
	vblank_period = TIME_IN_HZ(Machine->drv->frames_per_second * vblank_multiplier);
	vblank_timer = timer_alloc(cpu_vblankcallback);
	vblank_countdown = vblank_multiplier;

	/*
	 *		The following code creates individual timers for each CPU whose interrupts are not
	 *		synced to the VBLANK, and computes the typical number of cycles per interrupt
	 */

	/* start the CPU interrupt timers */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;

		/* compute the average number of cycles per interrupt */
		if (ipf <= 0)
			ipf = 1;
		cpu[cpunum].vblankint_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
		cpu[cpunum].vblankint_timer = timer_alloc(NULL);

		/* see if we need to allocate a CPU timer */
		ipf = Machine->drv->cpu[cpunum].timed_interrupts_per_second;
		if (ipf)
		{
			cpu[cpunum].timedint_period = cpu_computerate(ipf);
			cpu[cpunum].timedint_timer = timer_alloc(cpu_timedintcallback);
			timer_adjust(cpu[cpunum].timedint_timer, cpu[cpunum].timedint_period, cpunum, cpu[cpunum].timedint_period);
		}
	}

	/* note that since we start the first frame on the refresh, we can't pulse starting
	   immediately; instead, we back up one VBLANK period, and inch forward until we hit
	   positive time. That time will be the time of the first VBLANK timer callback */
	first_time = -TIME_IN_USEC(Machine->drv->vblank_duration) + vblank_period;
	while (first_time < 0)
	{
		cpu_vblankcallback(-1);
		first_time += vblank_period;
	}
	timer_set(first_time, 0, cpu_firstvblankcallback);
}

