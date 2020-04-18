// license:BSD-3-Clause
//
// HC55516/HC55536/HC55564 - a CVSD voice decoder chip
//

// ---------------------------------------------------------------------------
//
// Overview
//
// The HC55516 is a CVSD (continuously variable slope delta) modulation 
// encoder/decoder chip.  CVSD modulation was popular in the 1980s for digital
// voice encoding, because it provides decent fidelity for voice audio at low
// bit rates.  CVSD happens to work particularly well with the spectrum
// properties of human voice signals, so it was mostly used for that, and in
// fact you still see CVSD encoding (although not this particular chip) used
// in some modern applications, especially military communications devices.
// It's also one of the codec types included in the Bluetooth specs.
//
// The core of a CVSD encoder/decoder is a "syllabic filter".  This isn't the
// sort of filter that removes unwanted frequency components from the output
// signal; rather, it's essentially an analog computing element that helps
// compute the output signal.  The syllabic filter is characterized by a
// time constant (4ms in the case of the HC55516), which is one of the key
// parameters controlling the CVSD's behavior, along with a pair of voltage
// levels feeding into the filter (the "high" and "low" syllabic filter
// inputs).  The data sheet for the HC55564 can still be found on the Web
// as of this writing, although the chip has been out of production for a
// very long time.
//
// The HC55516 was part of a family of nearly identical chips that also 
// included the HC55536 and HC55564.  Apparently the only difference among
// these variations was the maximum sample clock rate supported, so they're 
// essentially interchangeable.  There's an unrelated family of CVSD decoders
// used in some older pinball machines, the MC3417 and MC3418, which have
// similar properties (in that they're also based on CVSD modulation) but
// aren't compatible with the HC555xx chips and had a somewhat different
// electrical setup (in particular, the MC341x chips used external analog
// components for the syllabic filter and estimator filter, whereas the
// HC555xx uses internal components and a purely digital implementation).
// The software emulations for the two chips are quite similar, and earlier
// versions of PinMAME used the same code for both, but we've separated
// them into their own modules now so that we can use the optimal numerical
// parameters and logic for each chip type.
//
// The chief difficulty in implementing a good HC555xx decoder for PinMAME
// comes from the way the sample rate is handled in the original pinball
// machine hardware.  In particular, the original sound boards didn't use
// a fixed clock to generate the sample data.  Instead, they sent bits to
// the hardware on an ad hoc basis from the software.  That means that,
// first, the sample rate can't be determined by studying the hardware
// schematics, as the clock rate is purely at the whim of the software,
// and second, that the clock rate isn't perfectly uniform from bit to bit,
// as the software source can't be relied upon to generate bits with equal
// timing.  In practice, the actual elapsed time from bit to bit in the
// same clip can vary by as much as a factor of two.  A corollary to the
// first point - that the rate is controlled by the software - is that the
// rate on one pinball machine can vary from clip to clip, and in practice
// this actually happens on many games (e.g., most System 11 games have
// two different sample rates).
//
// Our strategy for handling the sample rate clock is to delay the decoding
// step as long as possible.  The MAME audio stream mechanism gives us some
// inherent backlogging, so we take advantage of that.  Rather than decoding
// the input bits as we receive them from the pinball sound board emulation,
// we collect them in a buffer, with timestamps.  When MAME calls upon us
// to fill its PCM stream buffer, we use the buffered samples to determine
// the HC55516 clock rate for that run of samples retrospectively.  That
// lets us average the clock rate over a large number of bits, smoothing
// out the highly variable per-bit timing.  In practice, we *can* rely on
// the average timing over a large number of bits being stable, because the
// original machines would have sounded terrible if they didn't accomplish
// that.
//
// Another aspect of the ad hoc sample rate that makes this difficult to
// implement in MAME is that MAME further processes our output in the 
// digital domain.  On the original pinball hardware, the output from the
// HC55516 chip was an analog signal that was processed from that point on 
// in the analog domain, so the sample rate that went into generating that 
// signal was irrelevant from that point forward.  But for our setup, we
// can't emulate the rest of the pipeline as though it were analog; we
// produce a digital PCM sample and have to hand off a PCM sample to our 
// next stage, the MAME stream.  The snag is that variable timing again!
// The MAME stream has its own clock rate, and we have an ad hoc clock
// rate from sample to sample.  We have to match the rates when handing
// the samples off to MAME by digitally resampling at the MAME rate. 
// Converting between dissimilar PCM rates is a tricky problem, but
// fortunately MAME already incorporates libsamplerate, which exists
// specifically to solve the resampling problem.  The solution to our
// clock rate mismatch is to use libamplerate as an intermediary.  We
// take each sample from our HC55516 output, feed it to a libsamplerate
// converter, and pass the resulting resampled signal to MAME.

#include "driver.h"
#include "filter.h"
#include "streams.h"
#include "core.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../ext/libsamplerate/samplerate.h"
#ifdef __MINGW32__
#include <windef.h>
#endif

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(a[0]))
#endif


// ---------------------------------------------------------------------------
//
// ***** CLOCK RATE MEASUREMENT AND LOGGING *****
//
// The HC55516 doesn't have a fixed sample data rate; the rate is simply
// the speed at which the host sends samples.  If you want to determine the
// rate for a given pinball machine, this code has a rate logging feature
// you can enable.  This requires a special debug build:
//
// 1. Un-comment the line beneath this comment block
// 2. Rebuild PinMAME
// 3. Run the game you want to measure
// 4. Look through the file <ROM>.vpm_hc55516_bitrate.log
// 5. Add a call to hc55516_set_sample_clock() in init_GAME()
//
// The file <ROM>.vpm.hc55516.log will contain information on the clock
// rate as samples are played.  WPC89 games all use the same fixed clock
// rate (22372 Hz).  Most System 11 games use two different clock rates,
// with some clips playing at one rate and some at the other.  In
// principle, each clip could even have a unique rate.
//
// In the first iteration of the 2019 revamp of this code, we thought it 
// was going to be necessary (or would at least improve playback) to 
// pre-set the clock rate per game, so we added a routine to do this
// (hc55516_set_sample_clock()) and added corresponding calls to this
// routine to some init_GAME().  This didn't turn out to be needed after
// reworking the hc55516 code some more, but we left the routine and
// calls intact to preserve the sample rate measurements, in case they
// become useful again in the future.  At the moment, there's no need to
// add more of these, as the information is ignored if provided at all.
//
//#define LOG_SAMPLE_RATE

#ifdef LOG_SAMPLE_RATE

// Log file for sample rate data
static FILE *sample_log_fp;

#endif // LOG_SAMPLE_RATE

// ---------------------------------------------------------------------------
// Gain Selection
//
// The HC55516 emulation works in terms of a simulated analog voltage level 
// at the HC55516 analog output pin (pin 3, AO, on the real chips).  MAME,
// in contrast, works in terms of 16-bit PCM samples.  So we have to convert
// from the simulated analog voltage to a 16-bit PCM level.  
//
// Both formats represent linear wave amplitudes, so the general-purpose way 
// to select the gain is to choose the value that preserves the identical 
// dynamic range:  that is, the gain that maps the maximum analog voltage to 
// the maximum PCM value.  We use loudness compression curve with an input
// dynamic range of -DYN_RANGE_MAX to +DYN_RANGE_MAX (see below).  We 
// ultimately convert this to the INT16 range in the range compression step,
// but the initial gain calculation targes the wider range before compression.
// So the default gain is DYN_RANGE_MAX/V_HIGH.
//
// In practice, some games don't use all of the available dynamic range on
// the CVSD side.  It can be more pleasing in these cases to use a higher 
// than default gain that converts all of the actually used dynamic range on
// the CVSD side to the full scale on the PCM side, or (DYN_RANGE_MAX /
// highest actual voltage level).
//
// To aid in selecting an ideal per-game gain, you can enable dynamic range
// logging by #define'ing LOG_DYN_RANGE below.  That will generate a log file
// (<ROM NAME>.vpm_hc55516_dynrange.log) containing periodic updates with
// the maximum sample level seen so far, and the computed "max non-clipping 
// gain" (the gain that maps the maximum sample level to PCM full scale).
// 
//#define LOG_DYN_RANGE

#ifdef LOG_DYN_RANGE

// log file for dynamic range data
static FILE *dynrange_log_fp;

#endif // LOG_DYN_RANGE

// ---------------------------------------------------------------------------
// Syllabic filter constants.  For an RC filter, the charge factor per
// clock period is given by:
//
//   exp(-clock_period/time_constant)
//
// where clock_period = 1/frequency and time_constant is the filter's RC
// time constant.  For normal analog filters, this would make the charge
// factor dependent upon the data clock frequency.  However, the HC55516
// data sheet has a note claiming that both of its filter time constants
// (syllabic filter and signal estimator, aka integrator) are inversely
// proportional to the data clock frequency.  That effectively makes the
// charge factors independent of the clock frequency, because:
//
//    time_constant = P / frequency  [where P is some constant value]
//    -> clock_period / time_constant = 1/frequency / (P/frequency) = 1/P
//    -> charge factor = exp(-1/P)
//
// The data sheet quotes the time constant values for a 16kHz rate, so we
// can recover the P value for each filter by plugging the quoted time 
// constants and 16kHz into the proportionality formula above, yielding 
// the following fixed charging factors:
//
#define CHARGE  0.984496437005408453   // syllabic filter - 4ms at 16kHz
#define DECAY   0.939413062813475808   // estimator filter - 1ms at 16kHz

// Syllabic filter shift mask.  The HC55516 uses a 3-bit shift register
// (mask binary 111 = 0x07).
#define SHIFTMASK 0x07 

// Syllabic filter high/low inputs.  These aren't documented in the HC55516
// data sheet, so we're guessing based on the audio signal specs.  The data
// sheet says that the audio signal output voltage at 0dB is 1.2V RMS, which
// implies a peak DC voltage of 1.7VDC.  The "low" input to the filter is
// usually 0V.  So we'll use 1.7 and 0 as the syllabic filter voltage charge 
// range.
#define V_HIGH  1.7
#define V_LOW   0.0

// Maximum value for INT16 dynamic range after applying the gain.  This is 
// the maximum value for our compression curve (see below), which is to say,
// the input value to the compression function that yields INT16_MAX as the
// result.
#define DYN_RANGE_MAX 69790.0

// Loudness compression function.  This takes a sample in linear space form
// -DYN_RANGE_MAX to +DYN_RANGE_MAX and compresses it to fit -1..1.
// The curve is linear at low volumes and flattens at higher volumes, which
// tends to make the overall signal sound louder.
INLINE float compress_loudness(const double sample)
{
	// apply compression
	return (float)(sample / (32768.0 + fabs(sample)) + sample * (0.15 * (1.0 / 32768.0)));
}

// Default gain.  This is the scaling factor to convert from the simulated
// analog output voltage from the HC55516 to a signed 16-bit PCM level.  Both
// representations are linear in wave amplitude, so preserving the identical
// dynamic range is simply a matter of multiplying by the PCM maximum value
// divided by the maximum analog voltage.  The maximum analog output voltage
// from a CVSD decoder is the same as the "high" input to the syllabic filter.
// (The physical HC55516 technically doesn't quite work that way, as it uses
// a digital implementation of the signal estimator filter, just like we do,
// and uses an internal DAC to convert that modeled filter to a true analog
// signal.  But our model omits that stage, as it's an implementation detail
// of the physical chip, so for the purposes of our mathematical model, the
// estimator filter is analog.)
//
// This is only the default gain.  The actual gain can be set at run-time
// per chip.  That allows tweaking the gain to take better advantage of the
// full dynamic range for games that don't use the full analog voltage range
// of the chip for actual output data.
//
#define DEFAULT_GAIN    (DYN_RANGE_MAX/V_HIGH)


//
// HC55536 chip state
//
struct hc55516_data
{
	// mixer channel
	INT8 	channel;

	// Syllabic filter status
	UINT8	shiftreg;           // last few data bits (determined by SHIFTMASK)
	double 	syl_level;          // simulated voltage on the syllabic filter
	double	integrator;         // simulated voltage on integrator output

	// Output gain.  This is the conversion factor from the simulated voltage
	// level on the integrator to an INT16 PCM sample for the MAME stream.
	double  gain;

	// last clock state
	UINT8   last_clock;

	// last data bit from the host
	UINT8   databit;

	// Final output filter.
	//
	// In hardware, the HC555xx chip requires a low-pass filter on the analog 
	// output to remove quantization noise.  All of the original sound boards 
	// that used this chip (that I'm aware of) had two stages of active low-pass 
	// filters, so we can model them all with two IIR filter stages.  The 
	// analog parameters vary by system, so we'll set up the appropriate 
	// generation-specific parameters during nitialization, using the type 
	// information passed in from the sound board host code.
	struct
	{
		// filter type - one of the HC55516_FILTER_xxx constants
		int type;
		
		// first filter stage
		filter2_context f1;

		// second filter stage
		filter2_context f2;
	} output_filter;

	// Input bit ring buffer.  When the pinball sound board emulator clocks a
	// data bit out to the HC55516, we add it to this buffer for later decoding.
	// We read bits back out of this ring when MAME calls upon us to fill its
	// stream with PCM samples.
	struct
	{
		// bit buffer
		struct
		{
			UINT8 bit;     // CVSD stream bit
			double t;      // time of rising edge of HC55516 clock for this bit
		} bits[1000];

		// Read/write indices.  The buffer is empty when read == write.
		int read;
		int write;
	} bits_in;

	// Last output stream update time.  We use this to determine where the
	// next buffered input bit goes in the stream timeline.
	double stream_update_time;

	// MAME output stream sample time (1/sample rate)
	double output_dt;

	// libresample stream.  We use this to convert from the current clip's
	// sample rate to the output stream rate.
	SRC_STATE *resample_state;

	// Buffered PCM output samples.  The HC55516 data clock runs slower than
	// the PC sound sample rate (22kHz or less for the HC55516 clock, vs 44.1
	// or 48 kHz for the PC rate), so each HC55516 bit will generate more than
	// one PCM sample as we translate the slower HC55516 samples through
	// libsamplerate to generate PC samples.  We buffer the samples coming
	// out of libsamplerate here.  This is a circular buffer.
	struct
	{
		// PCM samples, scaled to HC55516 simulated voltage output level
		float pcm[1000];

		// Read/write indices.  The buffer is empty when read == write.
		int read;
		int write;
	} pcm_out;

#ifdef LOG_SAMPLE_RATE
	// Sample clock statistics.  This keeps track of the time between clock
	// signals from the host so that we can estimate the overall sample rate.
	// To do a proper sample rate conversion in the digital domain from the
	// original data stream to the PC's PCM stream, we have to know the clock
	// rate we're converting FROM.
	struct
	{
		// last clock rise time
		double t_rise;

		// number of samples collected
		INT64 n_samples;

		// sum of measured sample times over the whole run
		double t_sum;

		// sum of measured sample times since the last checkpoint
		int n_samples_cur;
		double t_sum_cur;

	} clock_history;
#endif // LOG_SAMPLE_RATE

#ifdef LOG_DYN_RANGE
	// dynamic range measurements, if desired
	struct
	{
		// min and max measures seen so far
		double vmin, vmax;

		// number of samples collected since the last log file update
		long nsamples;
	} dynrange;
#endif
};

// chip state structures
static struct hc55516_data hc55516[MAX_HC55516];

// ---------------------------------------------------------------------------
//
// Initialize the output filter
//
static void init_output_filter(struct hc55516_data *chip)
{
	// we process samples on the way to the MAME output stream, so use
	// the MAME stream rate
	const int freq = stream_get_sample_rate(chip->channel);

	// set up the filter according to the sound board type
	switch (chip->output_filter.type)
	{
	case HC55516_FILTER_C8228:
		// Williams Speech Board Type 2 (part number C-8228), used in System 3
		// and optionally (otherwise C-8226 with MC3417) in System 6/7 games.
		// This used a 2-stage multiple feedback
		// filter.  Note that there's a third op-amp stage as well, but that's
		// the actual output amplifier rather than another filter, so we don't
		// include it here.
		// [Note: I'm assuming they're meant to be MF filters, even though the
		// schematics I found are marked with the op-amp inputs at the opposite
		// polarities of what's used in a standard MF filter.  I'm assuming 
		// this is an error in the schematics and not some other type of
		// filter by intention; I'm pretty sure a positive feedback topology 
		// like that marked on the schematics would just blow up rather than
		// work as a filter. The schematics I found are second-hand,
		// from www.firepowerpinball.com, not original Williams schematics,
		// so I'm guessing this was just a transcription error. --mjr]
		filter_mf_lp_setup(43000, 36000, 180000, 1800e-12, 180e-12, &chip->output_filter.f1, freq);
		filter_mf_lp_setup(27000, 15000, 27000, 4700e-12, 1200e-12, &chip->output_filter.f2, freq);
		break;

	case HC55516_FILTER_SYS11:
		// System 11 sound board.  This uses a single-pole active low-pass filter
		// as the first stage, and a multiple feedback filter as the second stage.
		filter_active_lp_setup(43000, 36000, 180000, 180e-12, &chip->output_filter.f1, freq);
		filter_mf_lp_setup(47000, 22000, 15000, 1800e-12, 330e-12, &chip->output_filter.f2, freq);
		break;

	case HC55516_FILTER_WPC89:
	default:
		// Pre-DCS WPC sound boards.  Use the two-stage multiple feedback filter
		// found in the WPC89 schematics.
		filter_mf_lp_setup(150000, 22000, 150000, 1800e-12, 330e-12, &chip->output_filter.f1, freq);
		filter_mf_lp_setup(47000, 22000, 150000, 1800e-12, 330e-12, &chip->output_filter.f2, freq);
		break;
	}
}

// ---------------------------------------------------------------------------
//
// Add a decoded output sample.  This takes a raw sample from the CVSD 
// decoder, resamples it using the PCM sample rate of the MAME output stream,
// and adds it to our output buffer to eventually pass to the MAME stream.
//
static void add_sample_out(struct hc55516_data *chip, const double sample, const double output_rate_ratio)
{
	float fOut[128];
	SRC_DATA sd;
	long i;

	// resample at the MAME stream rate
	const float fIn = (float)sample;
	sd.data_in = &fIn;
	sd.input_frames = 1;
	sd.input_frames_used = 0;
	sd.data_out = fOut;
	sd.output_frames = _countof(fOut);
	sd.output_frames_gen = 0;
	sd.end_of_input = 0;
	sd.src_ratio = output_rate_ratio;
	if (src_process(chip->resample_state, &sd))
	{
		// error processing the sample - not much we can do, so just discard
		// the sample
		return;
	}

	// Add the resampled output(s) to the PCM sample buffer.  Note that the HC55516
	// clock rates are in the 20kHz range (the exact rate varies by game and even
	// clip), whereas the MAME stream will be at the PC sound card hardware rate,
	// typically 44.1 kHz or 48 kHz.  Each HC55516 sample therefore turns into 
	// approximately two MAME samples.  That's the main reason we need this extra
	// buffering step - MAME might not be ready to accept all the PCM samples that
	// convert from the current single HC55516 sample, so we need to be prepared
	// to stash extras for the next MAME buffer refill.
	for (i = 0; i < sd.output_frames_gen; ++i)
	{
		/* add the sample at the write pointer */
		chip->pcm_out.pcm[chip->pcm_out.write++] = fOut[i];
		if (chip->pcm_out.write >= _countof(chip->pcm_out.pcm))
			chip->pcm_out.write = 0;

		/* if the write pointer bumped into the read pointer, drop the oldest sample */
		if (chip->pcm_out.write == chip->pcm_out.read) {
			if (++chip->pcm_out.read >= _countof(chip->pcm_out.pcm))
				chip->pcm_out.read = 0;
		}
	}
}

// ---------------------------------------------------------------------------
//
// Process an input bit to the CVSD decoder.  This applies the CVSD decoding
// algorithm to the bit to produce the next PCM sample.  The PCM sample is at
// the CVSD clock rate, so it must be resampled to the MAME output stream rate
// before being passed to MAME.
//
static void process_bit(struct hc55516_data *chip, const UINT8 bit, const double output_rate_ratio)
{
	// add/subtract the syllabic filter output to/from the integrator
	const double di = (1.0 - DECAY)*chip->syl_level;
	if (bit != 0)
		chip->integrator += di;
	else
		chip->integrator -= di;

	// simulate leakage
	chip->integrator *= DECAY;

	// shift the new data bit into the syllabic filter's shift register
	chip->shiftreg = ((chip->shiftreg << 1) | bit) & SHIFTMASK;

	// figure the new syllabic filter output level
	chip->syl_level *= CHARGE;
	chip->syl_level += (1.0 - CHARGE) * ((chip->shiftreg == 0 || chip->shiftreg == SHIFTMASK) ? V_HIGH : V_LOW);

	// buffer the sample
	add_sample_out(chip, chip->integrator, output_rate_ratio);
}

// ---------------------------------------------------------------------------
//
// Apply the final output filter
//
static float apply_filter(struct hc55516_data *chip, const float sample)
{
	// run the sample through the two-stage filter
	const double filtered = filter2_step_with(&chip->output_filter.f2, filter2_step_with(&chip->output_filter.f1, sample));

	// apply the gain and apply loudness compression
	const float scaled = compress_loudness(filtered * chip->gain);

#ifdef LOG_DYN_RANGE
	// update the min/max range 
	if (sample < chip->dynrange.vmin)
		chip->dynrange.vmin = sample;
	if (sample > chip->dynrange.vmax)
		chip->dynrange.vmax = sample;

	// update the log file periodically
	if (++chip->dynrange.nsamples > 100000 && dynrange_log_fp != NULL)
	{
		// figure the maximum gain that won't clip for the current range
		const double maxgain1 = -DYN_RANGE_MAX / chip->dynrange.vmin;
		const double maxgain2 = DYN_RANGE_MAX / chip->dynrange.vmax;
		const double maxgain = maxgain1 < maxgain2 ? maxgain1 : maxgain2;

		// log the data
		fprintf(dynrange_log_fp, "HC55516 #%d range [%lf..+%lf] -> max gain w/o clipping %d\n",
			(int)(chip - hc55516), chip->dynrange.vmin, chip->dynrange.vmax, (int)maxgain);

		// make sure the new hits the file immediately, in case the program
		// exits before the next update (as we won't have a chance to close
		// the file properly on exit)
		fflush(dynrange_log_fp);

		// reset the log sample counter
		chip->dynrange.nsamples = 0;
	}
#endif // LOG_LOUDNESS

	// return the scaled result
	return scaled;
}

//
// Update the MAME output stream.  
//
// This takes the buffered CVSD input bit stream, and converts it to a PCM
// sample stream at the MAME stream rate.
//
static void hc55516_update(int num, INT16 *buffer_ptr, int length)
{
	struct hc55516_data *chip = &hc55516[num];
	const double work_ahead = 0.0025;
	const double max_gap = .0005;
	const double now = timer_get_time();
	double t = chip->stream_update_time;
	float * __restrict buffer = (float*)buffer_ptr;

	// Start a tiny bit early, so that we (hopefully) end a little early,
	// leaving a little CVSD input to carry over to next time.  This helps
	// avoid audible "seams" between adjacent frames.
	t -= work_ahead;

	// generate samples
	for (;;)
	{
		int i, n;
		double tprv;
		double ratio;

		// transfer any buffered PCM samples
		for (; chip->pcm_out.read != chip->pcm_out.write && length != 0; --length, t += chip->output_dt)
		{
			*buffer++ = apply_filter(chip, chip->pcm_out.pcm[chip->pcm_out.read++]);
			if (chip->pcm_out.read >= _countof(chip->pcm_out.pcm))
				chip->pcm_out.read = 0;
		}

		// stop if we're out of buffer space
		if (length == 0)
			break;

		// if the input bit buffer is empty, we have no more data, so satisfy the
		// rest of the buffer request with silence
		if (chip->bits_in.read == chip->bits_in.write)
		{
			// add silence
			for (; length != 0; --length, t += chip->output_dt)
				*buffer++ = apply_filter(chip, 0.0f);

			// done
			break;
		}

		// fill any gap to the next input bit with silence
		for (; length != 0 && t < chip->bits_in.bits[chip->bits_in.read].t - max_gap; --length, t += chip->output_dt)
			*buffer++ = apply_filter(chip, 0.0f);

		// stop if we're out of space
		if (length == 0)
			break;

		// We're at the start of a section of samples.  Find the run of continuously
		// clocked bits, with no breaks in the clock of longer than a few times the
		// clock rate.  We'll assume that any period of clock inactivity longer than
		// a few times the current clock tick represents a quiet period between clips.
		// Also stop when we reach the last sample.
		for (n = 0, i = chip->bits_in.read, tprv = t; i != chip->bits_in.write;)
		{
			// figure the length of time between samples
			const double tcur = chip->bits_in.bits[i].t;
			const double dt = tcur - tprv;

			// if it's too long, consider this the end of the current run
			if (dt > max_gap)
				break;

			// this sample is a keeper - count it
			++n;

			// advance to the next sample
			tprv = tcur;
			if (++i >= _countof(chip->bits_in.bits))
				i = 0;
		}

		// We've found the end of the run of continuous bits.
		//
		// - i points to the next sample after the last one in the current run
		// - tprv is the timestamp of the last bit included in the run
		// - t is the output stream time of the start of the run
		// - n is the number of HC55516 input bits in the run
		//
		// That means we're going to use n bits fill the time interval (tprv - t),
		// hence the effective clock rate of these bits is n/(tprv - t).  The
		// MAME stream needs PCM samples at its own rate, so we have to convert
		// from frequency n/(tprv - t) to the MAME stream frequency.  That will
		// translate from the n HC55516 input bits to the number of samples that
		// will fill the same time in the MAME stream.  Figure the resampling
		// rate ratio.
		ratio = stream_get_sample_rate(chip->channel) * (tprv - t) / n;

		// In rare cases, the ratio can be zero or even negative.  This can happen
		// when the game outputs a very small group of bits (1-3), which has been
		// observed in at least one game (F-14 Tomcat).  The difference in clock 
		// steps between the input and output streams can cause the output clock 
		// to get ahead of the input clock in such cases.  The same clock leapfrog
		// can happen in any group of bits, of course, but it doesn't cause trouble
		// if we have a more typical block of hundreds of bits, since the combined
		// time in the run will overwhelm the leapfrog effect, and the little bit
		// of jitter will get subsumed organically into the rate conversion.
		//
		// So what to do when we encounter this oddball case?  A handful of bits
		// in isolation will just sound like a click or tiny burst of noise, if
		// it's audible at all, so we could probably get away with dropping the
		// samples entirely without anyone missing them.  However, just in case
		// the game really intended to output a little click or short burst of
		// noise, we'll retain them.  We obviously can't output them in negative
		// time, but we can at least pack them into a short time by converting
		// them at the output stream sample rate.  Remember, this only happens
		// when we have a handful of samples in the group, so we're only using
		// a handful of slots in the output stream.  To do this, force the rate
		// ratio to 1:1 by fiat.  This will make the little bit group sound sped
		// up (by a factor of 2 or so for most games), but the loss of fidelity
		// shouldn't be noticeable because a few bits in isolation just sounds
		// like noise anyway.
		if (ratio < 0.5)
			ratio = 1.0;

		// generate these samples
		for (i = chip->bits_in.read; n > 0; --n)
		{
			// process this bit
			process_bit(chip, chip->bits_in.bits[i++].bit, ratio);
			if (i >= _countof(chip->bits_in.bits))
				i = 0;
		}

		// update the read pointer
		chip->bits_in.read = i;
	}

	// set the new stream update time
	chip->stream_update_time = now;
}

// ---------------------------------------------------------------------------
#ifdef LOG_SAMPLE_RATE
#define LOG_CLOCK_UPDATE(chip) collect_clock_stats(chip)
//
// collect clock statistics on each rising clock edge
//
static void collect_clock_stats(struct hc55516_data *chip)
{
	// note the time since the last update
	const double now = timer_get_time();

	// check if the last clock was recent enough that we can assume that the
	// clock has been running since the previous sample
	const double dt = now - chip->clock_history.t_rise;
	if (dt < .001)
	{
		// The clock is running.  Collect the timing sample.
		chip->clock_history.t_sum += dt;
		chip->clock_history.n_samples++;

		chip->clock_history.n_samples_cur++;
		chip->clock_history.t_sum_cur += dt;
		if (sample_log_fp != NULL && chip->clock_history.n_samples_cur >= 2000)
		{
			// for logging purposes, collect a separate sum since the last log point, to
			// make sure there's not a lot of short-term fluctuation, which might indicate
			// a bug in the emulation or, more problematically, a game that uses a mix of
			// different rates for different clips

			// log the data
			fprintf(sample_log_fp, "Chip #%d  # samples: %10I64d   current: %6d Hz   average:  %6d Hz\n",
				(int)(chip - hc55516), chip->clock_history.n_samples,
				(int)(chip->clock_history.n_samples_cur / chip->clock_history.t_sum_cur),
				(int)(chip->clock_history.n_samples / chip->clock_history.t_sum));
			fflush(sample_log_fp);

			// reset the local counters
			chip->clock_history.n_samples_cur = 0;
			chip->clock_history.t_sum_cur = 0;
		}
	}

	// remember the last time
	chip->clock_history.t_rise = now;
}
#else // LOG_SAMPLE_RATE
#define LOG_CLOCK_UPDATE(chip)
#endif // LOG_SAMPLE_RATE

// ---------------------------------------------------------------------------
//
// Process a clock edge
//
void hc55516_clock_w(int num, int state)
{
	struct hc55516_data *chip = &hc55516[num];

	// update the clock
	const int clock = state & 1;
	const int diffclock = clock ^ chip->last_clock;
	chip->last_clock = clock;

	// clock out sample bits on the rising edge
	if (diffclock && clock)
	{
		// log the clock update if desired
		LOG_CLOCK_UPDATE(chip);

		// add the bit to the bit buffer
		chip->bits_in.bits[chip->bits_in.write].bit = chip->databit;
		chip->bits_in.bits[chip->bits_in.write].t = timer_get_time();

		// update the write pointer
		if (++chip->bits_in.write >= _countof(chip->bits_in.bits))
			chip->bits_in.write = 0;

		// if we bumped into the read pointer, drop the last sample
		if (chip->bits_in.write == chip->bits_in.read && ++chip->bits_in.read >= _countof(chip->bits_in.bits))
			chip->bits_in.read = 0;
	}
}

// Set the gain, as a mutiple of the default gain.  The default gain
// yields a 1:1 mapping from the full dynamic range of the HC55516 to
// the full dynamic range of the MAME stream.
void hc55516_set_gain(int num, double gain)
{
	hc55516[num].gain = gain * DEFAULT_GAIN;
}

// Set the data bit input.  This just latches the bit for later processing,
// on the next rising clock edge.
void hc55516_digit_w(int num, int data)
{
	hc55516[num].databit = data & 1;
}

// Clear the clock (take it low)
void hc55516_clock_clear_w(int num, int data)
{
	hc55516_clock_w(num, 0);
}

// Set the clock (take it high).  The HC55516 reads the latest data bit
// on the rising clock edge, so this processes the latched bit from the
// last data write.
void hc55516_clock_set_w(int num, int data)
{
	hc55516_clock_w(num, 1);
}

// Clear the clock and set the data bit input.  This is a convenience
// function for the sake of the WPC89 sound board hardware, which is wired
// so that a data port write to the HC55516 also takes the clock low.
// This latches the data bit and updates the clock status.
void hc55516_digit_clock_clear_w(int num, int data)
{
	hc55516[num].databit = data & 1;
	hc55516_clock_w(num, 0);
}

// MAME port handlers.  The HC55516 ports are write-only.
WRITE_HANDLER(hc55516_0_digit_w)	{ hc55516_digit_w(0, data); }
WRITE_HANDLER(hc55516_0_clock_w)	{ hc55516_clock_w(0, data); }
WRITE_HANDLER(hc55516_0_clock_clear_w)	{ hc55516_clock_clear_w(0, data); }
WRITE_HANDLER(hc55516_0_clock_set_w)		{ hc55516_clock_set_w(0, data); }
WRITE_HANDLER(hc55516_0_digit_clock_clear_w)	{ hc55516_digit_clock_clear_w(0, data); }

WRITE_HANDLER(hc55516_1_digit_w) { hc55516_digit_w(1, data); }
WRITE_HANDLER(hc55516_1_clock_w) { hc55516_clock_w(1, data); }
WRITE_HANDLER(hc55516_1_clock_clear_w) { hc55516_clock_clear_w(1, data); }
WRITE_HANDLER(hc55516_1_clock_set_w)  { hc55516_clock_set_w(1, data); }
WRITE_HANDLER(hc55516_1_digit_clock_clear_w) { hc55516_digit_clock_clear_w(1, data); }


// Set a known sampling rate for the game.  The init_GAME() function for the
// specific game can call this during initialization to preset the clock rate
// for the HC55516 chip, if known.  (This isn't required, and in fact the
// information isn't currently used for anything, as we instead adapt to the
// de facto playback rate at run time.)
void hc55516_set_sample_clock(int chipno, int frequency)
{
	// This information isn't currently used, so we just ignore the call.
	// We're keeping this function around (along with the calls to it made 
	// in various game drivers) in case it becomes useful in the future.
}

// 
// Start the HC55516 emulation
//
int hc55516_sh_start(const struct MachineSound *msound)
{
	const struct hc55516_interface *intf = msound->sound_interface;
	int i;
	int lsrerr;

	// if desired, create a log file
#ifdef LOG_SAMPLE_RATE
	{
		char tmp[_MAX_PATH];
		strcpy_s(tmp, sizeof(tmp), Machine->gamedrv->name);
		strcat_s(tmp, sizeof(tmp), ".vpm_hc55516_bitrate.log");
		sample_log_fp = fopen(tmp, "a");
	}
#endif

	// if desired, initialize dynamic range logging
#ifdef LOG_DYN_RANGE
	{
		// create the log file
		char tmp[_MAX_PATH];
		strcpy_s(tmp, sizeof(tmp), Machine->gamedrv->name);
		strcat_s(tmp, sizeof(tmp), ".vpm_hc55516_dynrange.log");
		dynrange_log_fp = fopen(tmp, "a");
	}
#endif

	// loop over HC55516 chips
	for (i = 0; i < intf->num; i++)
	{
		struct hc55516_data *chip = &hc55516[i];
		char name[40];
		SRC_DATA sd;
		float sd_in[32], sd_out[128];
		int j;

		// reset the chip struct
		memset(chip, 0, sizeof(*chip));

		// create the output stream
		sprintf(name, "HC55516 #%d", i);
		chip->channel = stream_init_float(name, intf->volume[i], Machine->sample_rate, i, hc55516_update, 1); // pick output sample rate for max quality, also saves an additional filtering step in the mixer!
		chip->stream_update_time = timer_get_time();
		chip->output_dt = 1.0 / Machine->sample_rate;
		chip->gain = DEFAULT_GAIN;
		if (chip->channel == -1)
			return 1;

		// set up the output filters
		chip->output_filter.type = intf->output_filter_type;
		init_output_filter(chip);

		// create our libsamplerate stream
		chip->resample_state = src_new(SRC_SINC_MEDIUM_QUALITY, 1, &lsrerr);
		if (lsrerr != 0)
			return 1;

		// Prime the libsamplerate stream with some silence, so that we get
		// samples out of it immediately when we start feeding it live data.
		for (j = 0; j < _countof(sd_in); sd_in[j++] = 0);
		sd.data_in = sd_in;
		sd.input_frames = _countof(sd_in);
		sd.input_frames_used = 0;
		sd.data_out = sd_out;
		sd.output_frames = _countof(sd_out);
		sd.output_frames_gen = 0;
		sd.end_of_input = 0;
		sd.src_ratio = stream_get_sample_rate(chip->channel) / 20000.0;
		src_process(chip->resample_state, &sd);
	}

	/* success */
	return 0;
}
