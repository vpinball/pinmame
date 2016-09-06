#include "driver.h"
#include "filter.h"
//#include <math.h>

//#ifndef M_E
// #define M_E 2.7182818284590452353602874713527
//#endif

#define SAMPLE_RATE (4*48000) // 4x oversampling of standard output rate

#define SHIFTMASK 0x07 // = hc55516 and mc3417 //!! At least Xenon and Flash Gordon, and early Williams' (C-8226, but NOT the C-8228 so maybe does not matter overall??) had a MC3417
//#define SHIFTMASK 0x0F // = mc3418 //!! also features a more advanced syllabic filter (fancier lowpass filter for the step adaption) than the simpler chips above!

#define	INTEGRATOR_LEAK_TC		0.001
 #define leak   /*=pow(1.0/M_E, 1.0/(INTEGRATOR_LEAK_TC * 16000.0));*/ 0.939413062813475786119710824622305084524680890549441822009
#define	FILTER_DECAY_TC			0.004
 #define decay  /*=pow(1.0/M_E, 1.0/(FILTER_DECAY_TC * 16000.0));   */ 0.984496437005408405986988829697020369707861003180350567476
#define	FILTER_CHARGE_TC		0.004
 #define charge /*=pow(1.0/M_E, 1.0/(FILTER_CHARGE_TC * 16000.0));  */ 0.984496437005408405986988829697020369707861003180350567476

#define	FILTER_MAX				1.0954 // 0 dbmo sine wave peak value volts from MC3417 datasheet
#ifdef PINMAME
 #define ENABLE_LOWPASS_ESTIMATE 1
 #define SAMPLE_GAIN			6500.0
#else
 #define FILTER_MIN				0.0416 // idle voltage (0/1 alternating input on each clock) from MC3417 datasheet
 #define SAMPLE_GAIN			10000.0
#endif

struct hc55516_data
{
	INT8 	channel;
	UINT8	last_clock;
	UINT8	databit;
	UINT8	shiftreg;

	INT16	curr_value;
	INT16	next_value;

	UINT32	update_count;

	double 	filter;
	double	integrator;
	double  gain;

#ifdef PINMAME // add low pass filtering like chip spec suggests, and like real machines also had (=extreme filtering networks, f.e. Flash Gordon has (multiple) Sallen-Key Active Low-pass at the end (at least ~3Khz), TZ has (multiple) Multiple Feedback Active Low-pass at the end (~3.5KHz))
#if ENABLE_LOWPASS_ESTIMATE
	filter* filter_f;           /* filter used, ==0 if none */
	filter_state* filter_state; /* state of the filter */

	UINT32  length_estimate;    // for estimating the clock rate/'default' sample playback rate of the machine
	UINT32  length_estimate_runs;
#endif
#endif
};


static struct hc55516_data hc55516[MAX_HC55516];

static void hc55516_update(int num, INT16 *buffer, int length);


int hc55516_sh_start(const struct MachineSound *msound)
{
	const struct hc55516_interface *intf = msound->sound_interface;
	int i;

	/* loop over HC55516 chips */
	for (i = 0; i < intf->num; i++)
	{
		struct hc55516_data *chip = &hc55516[i];
		char name[40];

		/* reset the channel */
		memset(chip, 0, sizeof(*chip));

		/* create the stream */
		sprintf(name, "HC55516 #%d", i);
		chip->channel = stream_init(name, intf->volume[i], SAMPLE_RATE, i, hc55516_update);
		chip->gain = SAMPLE_GAIN;
		/* bail on fail */
		if (chip->channel == -1)
			return 1;

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
	struct hc55516_data *chip = &hc55516[num];
	INT32 data, slope;
	int i;

	/* zero-length? bail */
	if (length == 0)
		return;

#ifdef PINMAME
#if ENABLE_LOWPASS_ESTIMATE
	// 'detect' clock rate and guesstimate low pass filtering from this
	// not perfect, as some machines vary the clock rate, depending on the sample played  :/
 #define LOWPASS_ESTIMATE_CYCLES 666
	if (chip->update_count == 0 // is update coming from clock?
		&& length < SAMPLE_RATE/12000 // and no outlier?
		&& chip->length_estimate_runs < LOWPASS_ESTIMATE_CYCLES) // and we are still tracking the clock?
	{
		chip->length_estimate += length;
		chip->length_estimate_runs++;
	}
	else if (chip->length_estimate_runs == LOWPASS_ESTIMATE_CYCLES) // enough tracking of the clock -> enable filter and estimate for which cut frequency
	{
		double freq_scale;
		
		chip->length_estimate /= LOWPASS_ESTIMATE_CYCLES;

		freq_scale = ((INT32)chip->length_estimate - SAMPLE_RATE/48000) / (double)(SAMPLE_RATE/12000-1 - SAMPLE_RATE/48000); // estimate to end up within 0..1 (with all tested machines)
		freq_scale = max(min(freq_scale, 1.), 0.); // make sure to be in 0..1
		freq_scale = 1.-sqrt(sqrt(freq_scale));    // penalty for low clock rates -> even more of the lower frequencies removed then

		if (freq_scale < 0.45) // assume that high clock rates/most modern machines (that would end up at ~12000Hz filtering, see below) do not need to be filtered at all (improves clarity at the price of some noise)
		{
			chip->filter_f = filter_lp_fir_alloc((2000 + 22000*freq_scale)/SAMPLE_RATE, 51); // magic, majority of modern machines up to TZ = ~12000Hz then, older/low sampling rates = ~7000Hz, down to ~2500Hz for Black Knight //!! Xenon should actually end up at lower Hz, so maybe handle that one specifically?
			chip->filter_state = filter_state_alloc(); //!! leaks
			filter_state_reset(chip->filter_f, chip->filter_state);
		}

		chip->length_estimate_runs++;
	}
#endif
#endif

	/* track how many samples we've updated without a clock, e.g. if its too many, then chip got no data = silence */
	chip->update_count += length;
	if (chip->update_count > SAMPLE_RATE / 32)
	{
		chip->update_count = SAMPLE_RATE; // prevent overflow
		chip->next_value /= 2; // PINMAME: fade out

		chip->integrator = 0.; // PINMAME: reset all state
		chip->filter = 0.;
		chip->shiftreg = 0;
	}

	/* compute the interpolation slope */
	// as the clock drives the update (99% of the time), we can interpolate only within the current update phase
	// for the remaining cases where the output drives the update, length is rather small (1 or very low 2 digit range): then the last sample will simply be repeated
	data = chip->curr_value;

	slope = (((INT64)chip->next_value - data) << 16) / length; // PINMAME: increase/fix precision issue! //!! requires length to be at least 2, otherwise overflow can happen!
	data <<= 16;
	chip->curr_value = chip->next_value;

#ifdef PINMAME
#if ENABLE_LOWPASS_ESTIMATE
	if (chip->filter_f)
		for (i = 0; i < length; i++, data += slope)
		{
			filter_insert(chip->filter_f, chip->filter_state, data >> 16);
			*buffer++ = filter_compute(chip->filter_f, chip->filter_state);
		}
	else
#endif
#endif
		for (i = 0; i < length; i++, data += slope)
			*buffer++ = data >> 16;
}


void hc55516_clock_w(int num, int state)
{
	struct hc55516_data *chip = &hc55516[num];
	int clock = state & 1, diffclock;

	/* update the clock */
	diffclock = clock ^ chip->last_clock;
	chip->last_clock = clock;

	/* speech clock changing (active on rising edge) */
	if (diffclock && clock) //!! mc341x would need !clock
	{
		double temp;

		/* clear the update count */
		chip->update_count = 0;

		chip->shiftreg = ((chip->shiftreg << 1) | chip->databit) & SHIFTMASK;

		/* move the estimator up or down a step based on the bit */
		if (chip->databit)
			chip->integrator += chip->filter;
		else
			chip->integrator -= chip->filter;

		/* simulate leakage */
		chip->integrator *= leak;

		/* if we got all 0's or all 1's in the last n bits, bump the step up by charging the filter */
		if (chip->shiftreg == 0 || chip->shiftreg == SHIFTMASK)
		{
			chip->filter = (1.-charge) * FILTER_MAX + chip->filter * charge;
#ifndef PINMAME // cannot happen
			if (chip->filter > FILTER_MAX)
				chip->filter = FILTER_MAX;
#endif
		}
		/* simulate decay */
		else
		{
			chip->filter *= decay;
#ifndef PINMAME //!! should not be needed from chip spec, as it will either alternate 0/1 databits or output 'real' silence
			if (chip->filter < FILTER_MIN)
				chip->filter = FILTER_MIN;
#endif
		}

		/* compute the sample as a 16-bit word */
		temp = chip->integrator * chip->gain;

#ifdef PINMAME
#if 1
		/* compress the sample range to fit better in a 16-bit word */
		// Pharaoh: up to 109000, 'normal' max around 45000-50000, so find a balance between compression and clipping
		if (temp < 0.)
			temp = temp / (temp * -(1.0 / 32768.0) + 1.0) + temp*0.15;
		else
			temp = temp / (temp *  (1.0 / 32768.0) + 1.0) + temp*0.15;

		if(temp <= -32768.)
			chip->next_value = -32768;
		else if(temp >= 32767.)
			chip->next_value = 32767;
		else
			chip->next_value = (INT16)temp;
#else
		/* Cut off extreme peaks produced by bad speech data (eg. Pharaoh) */
		if (temp < -80000.) temp = -80000.;
		else if (temp > 80000.) temp = 80000.;
		/* Just wrap to prevent clipping */
		if (temp < -32768.) chip->next_value = (INT16)(-65536. - temp);
		else if (temp > 32767.) chip->next_value = (INT16)(65535. - temp);
		else chip->next_value = (INT16)temp;
#endif
#else
		/* compress the sample range to fit better in a 16-bit word */
		if (temp < 0.)
			chip->next_value = (int)(temp / (temp * -(1.0 / 32768.0) + 1.0));
		else
			chip->next_value = (int)(temp / (temp *  (1.0 / 32768.0) + 1.0));
#endif
		/* update the output buffer before changing the registers */
		stream_update(chip->channel, 0);
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
