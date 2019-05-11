// license:BSD-3-Clause

// Un-comment the following line to create a test build that writes the
// measured sample rate to a log file (vpm_hc55516.log) while the game
// runs.  This lets you collect the actual sample rate for a game whose
// clock rate isn't yet known.  You can then patch up the game's
// init_GAME() routine (e.g., init_taf() for The Addams Family) with
// a call to hc55516_set_sample_clock() that sets the correct initial
// rate for the game.
// #define LOG_SAMPLE_RATE

#include "driver.h"
#include "filter.h"
#include <math.h>
#include <stdio.h>
#ifdef __MINGW32__
 #include <windef.h>
#endif

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(a[0]))
#endif

// Log file for sample rate data, for testing builds
static FILE *logfp;

// Final low-pass output filter type.  A CVSD decoder pretty much requires 
// a low-pass filter to reduce quantization noise above the Nyquist limit
// (which is half the sampling rate).  The code can be configured with a
// couple of different filters here.  In principle, the best option should
// be a FIR filter, as that should have a nice steep wall at the cutoff
// frequency.  A simple single-pole low-pass filter actually sounds a
// little better to my ear.
#define NO_OUTPUT_FILTER   0   // no filtering - not recommended
#define FIR_OUTPUT_FILTER  1   // low-pass FIR filter
#define SP_OUTPUT_FILTER   2   // simple single-pole low-pass filter
#define OUTPUT_FILTER_TYPE FIR_OUTPUT_FILTER

// Syllabic filter shift mask.  The HC55516 and MC3417 use a 3-bit shift
// register (mask binary 111 = 0x07).
#define SHIFTMASK 0x07 

// Syllabic filter high/low inputs.  These aren't documented in the HC55516
// data sheet, so we're guessing based on the audio signal specs.  The data
// sheet says that the audio signal output voltage at 0dB is 1.2V RMS, which
// implies a peak DC voltage of 1.7VDC.  The "low" input to the filter is
// usually 0V.  So we'll use 1.7 and 0 as the syllabic filter voltage charge 
// range.
#define V_HIGH  1.7
#define V_LOW   0

#ifdef PINMAME

#define SAMPLE_GAIN			6500.0
#else
 #define V_HIGH              1.7
 #define V_LOW				0.0416 // idle voltage (0/1 alternating input on each clock) from MC3417 datasheet
 #define SAMPLE_GAIN		10000.0
#endif

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

struct hc55516_data
{
	// mixer channel
	INT8 	channel;
	
	// last clock level and data bit input from host
	UINT8	last_clock;
	UINT8	databit;

	// Syllabic filter parameters.  The syllabic filter is nominally specified
	// in terms of its time constant, which is independent of the data clock 
	// rate.  (The HC55516 data sheet actually has a note to the effect that 
	// the time constant is inversely proportional to the clock, but this does
	// not appear to be true in practice: we get audibly better results by
	// assuming an invariant time constant.)  Our internal parameters, on the
	// other hand, are functions of the data clock. For optimal playback, we
	// we have to observe the actual data clock rate and then calculate the
	// filter parameters based on that.
	double   charge;            // syllabic filter charge rate
	double   decay;             // syllabic filter decay rate
	
	// Syllabic filter status
	UINT8	shiftreg;           // last few data bits (determined by SHIFTMASK)
	double 	syl_level;          // simulated voltage on the syllabic filter
	double	integrator;         // simulated voltage on integrator output

	// Gain - conversion factor from a simulated voltage level on the
	// integrator to an INT16 PCM sample
	double  gain; 

	// Output sample ring buffer.  With a real HC55516 chip, the output is an
	// analog signal, so the sound boards using this chip handled the rest of
	// the audio chain in the analog domain, feeding the signal into op amps,
	// mixing with other analog signals, and ultimately sending it out to the
	// speakers.  For our simulation, the output from our integrator goes into
	// a PCM stream, to be mixed with other PCM streams and ultimately to go
	// out to a DAC.  So our output chain is all in the digital domain.  We
	// *could* try to simulate an analog level coming out of our chip and
	// resample that, and indeed, that's what earlier versions of this code
	// did.  But the results were terrible, since that's not the right way to
	// resample a signal that's already in the digital domain.  The right way
	// is to keep everything in the digital domain.  The complication is that
	// the HC55516 doesn't have a fixed clock rate: instead, the host sets
	// the clock.  The data clock rate is simply how fast the host sends us
	// samples.  So we can't know in advance what kind of stream to set up.
	// More problematically, the actual pinball machine hosts that we're
	// using in the emulation don't have perfectly consistent clocks, as they
	// generate their samples from software.  The time between samples can be
	// slightly variable as a result.  That's a huge problem for MAME's design
	// in that MAME wants a fixed-rate stream coming out of each device.  To
	// bridge the gap, we use a ring buffer for our outputs.  Each time the
	// host clocks out an output, we put it in the ring.  Each time the MAME 
	// stream asks for an input, we pull it from the ring.  As long as the
	// MAME stream and host are generating and consuming samples at nearly
	// the same rate, this evens out any small variations in the sample-to-
	// sample timing of individual bits coming out of the host.
	struct
	{
		// note: the buffer is empty when read == write
		INT16   buf[1000];      // ring buffer
		int     read;           // read pointer - index of next sample to read
		int     write;          // write pointer - index of next free slot
	}
	samples_out;

	// Sample clock statistics.  This keeps track of the time between clock
	// signals from the host so that we can estimate the overall sample rate.
	// To do a proper sample rate conversion in the digital domain from the
	// original data stream to the PC's PCM stream, we have to know the clock
	// rate we're converting FROM.
	struct
	{
		// observed sample rate in Hz, and its inverse (the sample time)
		int freq;
		double T;

		// global time of last clock rising edge
		double t_rise;

		// number of samples collected
		INT64 n_samples;

		// sample count for next filter adjustment 
		INT64 next_filter_adjustment;

		// sum of measured sample times over the whole run
		double t_sum;

		// sum of measured sample times since the last checkpoint (to check
		// for convergence)
		int n_samples_cur;
		double t_sum_cur;
	} clock;


#ifdef PINMAME
#if OUTPUT_FILTER_TYPE == FIR_OUTPUT_FILTER
	// Low-pass filter.  This is absolutely required for a CVSD decoder, since
	// these chips are invariably used with low sample rates, which results in
	// a Nyquist limit that's well within the audible range and thus a LOT of
	// audible quantization noise.
	filter* filter_f;
	filter_state* filter_state;
#elif OUTPUT_FILTER_TYPE == SP_OUTPUT_FILTER
	// Filter state and single-pole low-pass filter alpha parameter
	INT32   out_filter_sample;
	double  out_filter_alpha;
#endif
#endif
};

// chip structures
static struct hc55516_data hc55516[MAX_HC55516];

// pre-set sample rates
static int hc55516_clock_rate[MAX_HC55516];

// foward declarations
static void hc55516_update(int num, INT16 *buffer, int length);

static void calc_syllabic_filter_params(struct hc55516_data *chip, double sample_time)
{
	// Save the new frequency value
	chip->clock.T = sample_time;
	chip->clock.freq = (int)(1.0 / sample_time);

	// Figure the syllabic filter parameters.
	//
	// The charge constant is e^(-2*pi*T/Tc), where T is the 
	// time per sample (1 / the bit clock frequency) and Tc 
	// is the syllabic filter time constant (4ms for the 
	// HC55516, per the data sheet).
	//
	// Note that the HC55516 data sheet has a note saying that
	// the syllabic filter time constant is inversely 
	// proportional to the bit clock, which suggests that 4ms
	// is only the value for the "typical" 16 kHz clock rate
	// used as the reference point in the data sheet table.
	// However, empirically this does not seem to be the case;
	// we seem to get better results if we treat this as a
	// fixed 4ms value.  And information elsewhere in the data
	// sheet suggests that the filter really is fixed at 4ms.
	// 
	// Sample values for common pinball machine clock rates
	//
	// 2500Hz/4ms = .5334881     (early voice machines - Black Knight, Xenon)
	// 8kHz/4ms   = .85463600    (System 11 machines)
	// 12kHz/4ms  = .87730577    (Twilight Zone)
	// 20kHz/4ms  = .92446525    (Addams Family)
	//
	const double SYLLABIC_FILTER_TIME = .004;
	const int ROLLOFF_FREQ = 1000;
	chip->charge = exp(-2.0 * PI * chip->clock.T * (1.0 / SYLLABIC_FILTER_TIME));

	// figure the integrator decay factor
	chip->decay = exp(-2.0 * PI * chip->clock.T * ROLLOFF_FREQ);

	// Calculate the low-pass filter alpha.  Nominally, we have to
	// filter out noise above the Nyquist limit, which is 1/2 the
	// sample frequency.  But this is a simple low-pass filter, not
	// a brick-wall filter, so we need to start attenuating below
	// the desired cutoff.  There's a sweet spot here.  If we make
	// the cutoff too low, we'll filter out too much of the original
	// signal's high-frequency component, and it'll sound muffled.
	// If the cutoff is too high, the filter will pass through too
	// much of the quantization noise.  The cutoff has to be 
	// chosen empirically to make the result "sound right".
#if OUTPUT_FILTER_TYPE == SP_OUTPUT_FILTER
	{
		double cutoff = chip->clock.freq * 0.4;
		double RC = 1.0 / (2 * PI * cutoff);
		chip->out_filter_alpha = chip->clock.T / (RC + chip->clock.T);
	}
#endif
}

int hc55516_sh_start(const struct MachineSound *msound)
{
	const struct hc55516_interface *intf = msound->sound_interface;
	int i;

	// if desired, create a log file
#ifdef LOG_SAMPLE_RATE
	char tmp[_MAX_PATH];
#ifdef strcpy_s
	strcpy_s(tmp, sizeof(tmp), Machine->gamedrv->name);
	strcat_s(tmp, sizeof(tmp), ".vpm_hc55516.log");
#else
	sprintf(tmp, "vpm_hc55516.log");
#endif
	logfp = fopen(tmp, "a");
#endif

	// loop over HC55516 chips
	for (i = 0; i < intf->num; i++)
	{
		struct hc55516_data *chip = &hc55516[i];
		char name[40];
		int freq;

		// reset the chip struct
		memset(chip, 0, sizeof(*chip));

		// assume that the game driver pre-set the sample rate so that we don't have
		// to figure it out ourselves
		chip->clock.next_filter_adjustment = 1000000000000LL;

		// Set the initial frequency.  If the game driver set a known  frequency via
		// hc55516_set_clock_rate(), use that, otherwise use a default.
		freq = hc55516_clock_rate[i];
		if (freq == 0)
		{
			// set a default sample rate
			freq = 12000;

			// we need to collect sample rate data and adjust the filter once we have
			// enough data to determine the actual rate
			chip->clock.next_filter_adjustment = 250;
		}

		// set the initial syllabic filter parameters
		calc_syllabic_filter_params(chip, 1.0 / freq);

		// create the output stream
		sprintf(name, "HC55516 #%d", i);
		chip->channel = stream_init(name, intf->volume[i], freq, i, hc55516_update);
		chip->gain = SAMPLE_GAIN;
		/* bail on fail */
		if (chip->channel == -1)
			return 1;

		// Set up a low-pass FIR filter, to filter out quantization noise
		// above the Nyquist limit.  The FIR filter implementation works
		// inherently in terms of the sampling rate, so the filter will
		// naturally adjust to our sample rate updates as we collect clock
		// data.  That means we can set up the filter up front, before we
		// even know the sample rate.  Note that older versions tried to
		// guess whether a filter was needed based on sampling rate, but
		// in reality the filter is ALWAYS needed, since CVSD bit rates
		// are always low enough that there's a ton of audible quantization
		// noise in the decoded signal.  This is true for every historical
		// game using this chip, so there's no good reason to make the
		// existence of the filter conditional on the rate.
#if OUTPUT_FILTER_TYPE == FIR_OUTPUT_FILTER
		chip->filter_f = filter_lp_fir_alloc(0.21, FILTER_ORDER_MAX);
		chip->filter_state = filter_state_alloc();
		filter_state_reset(chip->filter_f, chip->filter_state);
#endif

#ifdef PINMAME
#if ENABLE_LOWPASS_ESTIMATE
		chip->filter_f = 0;
		chip->length_estimate = 0;
		chip->length_estimate_runs = 0;
#endif
#endif
	}

	/* success */
	return 0;
}


void hc55516_update(int num, INT16 *buffer, int length)
{
	/* zero-length? bail */
	if (length == 0)
		return;

	/* add samples until the request is fulfilled */
	struct hc55516_data *chip = &hc55516[num];
	for (; length != 0; --length)
	{
		INT16 sample;

		/* if a sample is available, add it; otherwise decay the last sample */
		if (chip->samples_out.read != chip->samples_out.write)
		{
			/* sample available - pull it */
			sample = chip->samples_out.buf[chip->samples_out.read++];
			if (chip->samples_out.read >= _countof(chip->samples_out.buf))
				chip->samples_out.read = 0;
		}
		else
		{
			/* no more samples available - decay the last sample */
			sample = (chip->samples_out.buf[chip->samples_out.read] /= 2);
		}

		/* apply the low-pass filter (if any) */
#if OUTPUT_FILTER_TYPE == FIR_OUTPUT_FILTER
		filter_insert(chip->filter_f, chip->filter_state, (filter_real)sample);
		*buffer++ = filter_compute_clamp16(chip->filter_f, chip->filter_state);
#elif OUTPUT_FILTER_TYPE == SP_OUTPUT_FILTER
		chip->out_filter_sample = (INT16)(chip->out_filter_sample + chip->out_filter_alpha*(sample - chip->out_filter_sample));
		*buffer++ = (INT16)chip->out_filter_sample;
#else
		*buffer++ = sample;
#endif
	}
}

static void add_sample_out(struct hc55516_data *chip, INT16 sample)
{
	/* add the sample at the write pointer */
	chip->samples_out.buf[chip->samples_out.write++] = sample;
	if (chip->samples_out.write >= _countof(chip->samples_out.buf))
		chip->samples_out.write = 0;

	/* if the write pointer bumped into the read pointer, drop the oldest sample */
	if (chip->samples_out.write == chip->samples_out.read) {
		if (chip->samples_out.read >= _countof(chip->samples_out.buf))
			chip->samples_out.write = 0;
	}
}

// collect clock statistics on each rising clock edge
static void collect_clock_stats(int num)
{
	struct hc55516_data *chip = &hc55516[num];

	// note the time since the last update
	double now = timer_get_time();

	// check if the last clock was recent enough that we can assume that the
	// clock has been running since the previous sample
	double dt = now - chip->clock.t_rise;
	if (dt < .001)
	{
		// The clock is running.  Collect the timing sample.
		chip->clock.t_sum += dt;
		chip->clock.n_samples++;

#ifdef LOG_SAMPLE_RATE
		chip->clock.n_samples_cur++;
		chip->clock.t_sum_cur += dt;
		if (logfp != NULL && chip->clock.n_samples_cur >= 2000)
		{
			// for logging purposes, collect a separate sum since the last log point, to
			// make sure there's not a lot of short-term fluctuation, which might indicate
			// a bug in the emulation or, more problematically, a game that uses a mix of
			// different rates for different clips

			// log the data
			fprintf(logfp, "Chip #%d  # samples: %10I64d   current: %6d Hz   average:  %6d Hz\n",
				num, chip->clock.n_samples,
				(int)(chip->clock.n_samples_cur / chip->clock.t_sum_cur),
				(int)(chip->clock.n_samples / chip->clock.t_sum));
			fflush(logfp);

			// reset the local counters
			chip->clock.n_samples_cur = 0;
			chip->clock.t_sum_cur = 0;
		}
#endif

		// If we've passed the next threshold for adjusting the filter
		// based on the collected clock data, make the adjustment.
		if (chip->clock.n_samples >= chip->clock.next_filter_adjustment)
		{
			// recalculate the syllabic filter parameters based on the
			// new sample rate
			calc_syllabic_filter_params(chip, chip->clock.t_sum / chip->clock.n_samples);

			// change the output stream to match the observed sample rate
			stream_set_sample_rate(chip->channel, chip->clock.freq);

			// Set the next update threshold.  The reading usually converges
			// pretty quickly, so we don't need to keep this up for too many
			// iterations.
			if (chip->clock.next_filter_adjustment < 5000)
				chip->clock.next_filter_adjustment = 5000;
			else if (chip->clock.next_filter_adjustment < 5000)
				chip->clock.next_filter_adjustment = 30000;
			else
				chip->clock.next_filter_adjustment = 1000000000000LL;  // basically "never again"
		}
	}
	else
	{
		int i;
		INT16 s;

		// The previous clock time was too long ago for this to be part of
		// the same clock run.  That means that the clock has newly started,
		// so we've probably just started playing back a new track.  Stuff
		// a few silent samples into the buffer, so that we can keep the
		// simulated sample stream ahead of the actual output sound stream.
		// That will better accommodate slight variations in the clock rate
		// from the sound board host by keeping a small backlog of samples
		// in the buffer for times when the host gets a tiny bit behind.
		s = chip->samples_out.buf[chip->samples_out.read];
		for (i = 0; i < 16; ++i)
			add_sample_out(chip, s /= 2);
	}

	// remember the last time
	chip->clock.t_rise = now;
}

void hc55516_clock_w(int num, int state)
{
	struct hc55516_data *chip = &hc55516[num];

	/* update the clock */
	int clock = state & 1, diffclock;
	diffclock = clock ^ chip->last_clock;
	chip->last_clock = clock;

	/* speech clock changing (active on rising edge) */
	if (diffclock && clock) //!! mc341x would need !clock
	{
		double temp;

		// collect clock timing data on each rising edge
		collect_clock_stats(num);

		/* move the estimator up or down a step based on the bit */
		if (chip->databit)
			chip->integrator += chip->syl_level;
		else
			chip->integrator -= chip->syl_level;

		/* simulate leakage */
		chip->integrator *= chip->decay;

		/* add/subtract the syllabic filter output to/from the integrator */
		double di = (1.0 - chip->decay)*chip->syl_level;
		if (chip->databit != 0)
			chip->integrator += di;
		else
			chip->integrator -= di;

		/* shift the bit into the shift register */
		chip->shiftreg = ((chip->shiftreg << 1) | chip->databit) & SHIFTMASK;

		/* figure the new syllabic filter output */
		chip->syl_level *= chip->charge;
		chip->syl_level += (1.0 - chip->charge) * ((chip->shiftreg == 0 || chip->shiftreg == SHIFTMASK) ? V_HIGH : V_LOW);

		/* compute the sample as a 16-bit word */
		temp = chip->integrator * chip->gain;

#ifdef PINMAME
		/* compress the sample range to fit better in a 16-bit word */
		// Pharaoh: up to 109000, 'normal' max around 45000-50000, so find a balance between compression and clipping
		if (temp < 0.)
			temp = temp / (temp * -(1.0 / 32768.0) + 1.0) + temp*0.15;
		else
			temp = temp / (temp *  (1.0 / 32768.0) + 1.0) + temp*0.15;

		/* clip to INT16 range */
		if(temp <= -32768.)
			temp = -32768;
		else if(temp >= 32767.)
			temp = 32767;
#else
		/* compress the sample range to fit better in a 16-bit word */
		if (temp < 0.)
			temp = (temp / (temp * -(1.0 / 32768.0) + 1.0));
		else
			temp = (temp / (temp *  (1.0 / 32768.0) + 1.0));
#endif

		/* add the sample to the ring buffer */
		add_sample_out(chip, (INT16)temp);
	}
}

#ifdef PINMAME
void hc55516_set_gain(int num, double gain)
{
	hc55516[num].gain = gain;
}
#endif


void hc55516_digit_w(int num, int data)
{
	hc55516[num].databit = data & 1;
}


void hc55516_clock_clear_w(int num, int data)
{
	hc55516_clock_w(num, 0);
}

void hc55516_clock_set_w(int num, int data)
{
	hc55516_clock_w(num, 1);
}


void hc55516_digit_clock_clear_w(int num, int data)
{
	hc55516[num].databit = data & 1;
	hc55516_clock_w(num, 0);
}

// Set a known sampling rate for the game.  The init_GAME() function for the
// specific game can call this during initialization to preset the clock rate
// for the HC55516 chip, if known.  In the absence of this information, we
// have to infer the sample rate by observing the actual data that the game's
// emulated sound board sends us.  It takes a few moments to get a good read
// on the sample rate, so there tends to be some glitching during the first
// couple of seconds of playback.  Initializing the game with the correct
// value makes for a more seamless startup.
void hc55516_set_sample_clock(int num, int frequency)
{
	if (num >= 0 && num < MAX_HC55516)
		hc55516_clock_rate[num] = frequency;
}

WRITE_HANDLER( hc55516_0_digit_w )	{ hc55516_digit_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_w )	{ hc55516_clock_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_clear_w )	{ hc55516_clock_clear_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_set_w )		{ hc55516_clock_set_w(0,data); }
WRITE_HANDLER( hc55516_0_digit_clock_clear_w )	{ hc55516_digit_clock_clear_w(0,data); }

WRITE_HANDLER( hc55516_1_digit_w ) { hc55516_digit_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_w ) { hc55516_clock_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_clear_w ) { hc55516_clock_clear_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_set_w )  { hc55516_clock_set_w(1,data); }
WRITE_HANDLER( hc55516_1_digit_clock_clear_w ) { hc55516_digit_clock_clear_w(1,data); }
