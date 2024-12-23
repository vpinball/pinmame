// license:BSD-3-Clause
//
// HC55516/HC55532/HC55536/HC55564 - a CVSD voice decoder chip
//

// ---------------------------------------------------------------------------
//
// Overview
//
// The HC555XX is a CVSD (continuously variable slope delta) modulation 
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
// Syllabic filters are defined in analog terms, but like any analog
// (that is, continuous-time) filter, they can be translated into the
// digital (discrete-time) domain.  The HC555xx chip was in fact a
// discrete-time implementation of the CVSD filter system.  A recent (2020)
// decap analysis of the HC55516 provided us with the exact algorithm and
// arithmetic parameters, which we now implement here.  The old MAME
// implementation was also a discrete-time implementation, implementing
// a digital version of the analog filter system specified in the data
// sheet.  The MAME version used IEEE floating-point types to implement
// the filter using the standard discrete-time algorithm for this type
// of filter.  Based on the decap, we can see that the HC555xx used the
// same mathematical model, but used 10- and 12-bit fixed-point arithmetic
// registers to perform the same calculations.  The arithmetic works out
// to be the same after taking into consideration the lower precision of
// the original hardware's registers, which shouldn't be surprising given
// that it would have to be essentially identical to work at all.  The
// old MAME version is arguably the "better" of the two implementations,
// in that it simulates the reference filter with greater mathematical
// precision, thanks to the higher precision of its intermediate results
// and accumulators.  The extremely limited precision of the original
// hardware results in greater rounding errors.  However, replicating
// the bit-for-bit identical arithmetic produces a more exact replica
// of the original output.  The difference in precision is probably too
// small to be audible even in theory, and certainly doesn't seem to be
// in practice.  The one significant difference between the two
// implementations is that we know from the decap what the clipping
// behavior is at the limits of the accumulator register ranges, which
// the old MAME implementation wasn't able to replicate, as it's not
// something we could have inferred from the reference filter spec in
// the data sheet.  The lack of proper clipping in the old PinMAME
// implementation motivated the addition of a loudness compression
// adjustment to handle the excessive dynamic range that resulted.  That
// might actually have been detrimentally audible.
//
// The HC55516 was part of a family of nearly identical chips that also
// included the HC55532, HC55536 and HC55564.  Apparently the only difference among
// these variations was the maximum sample clock rate supported (and some
// bugs, on the earlier revisions) and internal parameters, so they're
// essentially interchangeable (except for some very early machines, due to
// potential software timing bugs that the HC55516 could still cope with though).
// There's an unrelated family of CVSD decoders
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
// Note that apparently Williams/Bally themselves used a daughter board on
// late WPC releases (with the MC341X) if they couldn't get hold of enough HC555XX chips.
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


/* Lord Nightmares additional comments on this chip emu:
The decap my research is based off of is an HC55516 (from https://seanriddle.com/55516die.html) however
this HC55516 die has the spaces on the die for two different sets of silicon vias (only one is populated
but changing this requires changing only one mask layer) and if you populate the 'missing' vias and remove
the others, the behavior changes to match the HC55532 datasheet (mostly).

I'm ASSUMING this is the correct behavior for HC55532, as we don't have a decap of an HC55532
(they're very rare, only made for 2 or 3 years).

We also do not (yet) have a decap of an HC55564 (and HC55536 which should be a simplified, cheaper,
decode-only HC55564), although the datasheet and apocryphal evidence leads me to believe that it behaves
like an HC55516, but possibly with 2 bugs fixed:

1) The HC55516 datasheet (at least the early ones, and some later ones) claims the /FZ input resets both
   the syllabic and integrator filter. The silicon tracing proves it doesn't, it just forces a constant
   alternating input, which takes a lot longer to zero the signal than the datasheet implies. I'm GUESSING
   the HC55564 fixed this so it actually zeroes the two digital filters.

   (additional comment from MJR: Those are interesting details about the /FZ behavior in his remarks -
    It doesn't look like it'll affect VPM, since VPM's version of the chip emulator doesn't even implement
    the /FZ line. It must have never been needed, presumably because it's not wired to a controlled input
    on any of the pinball sound boards. I spot-checked a couple of System 11 schematics just now,
    and /FZ is indeed just pulled high on the ones I checked)

2) The HC55516 and HC55532 have a bug with the way the signed integrator filter decays toward zero by 1/16
   (or 1/32 for HC55532) of its current value each clock cycle, where, if the current value is positive,
   the filter will get stuck with a value of 0xF or 0x1F (for 55516 and 55532 respectively) in the integrator
   and never decay to truly zero. I believe this bug was fixed in the HC55564 and HC55536, which presumably
   allowed for eliminating a weird DC offset hack circuit which takes up a significant portion of the die.
*/


#include "driver.h"
#include "filter.h"
#include "streams.h"
#include "core.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../ext/libsamplerate/common.h"
#include "../../ext/libsamplerate/samplerate.h"
#ifdef __MINGW32__
#include <windef.h>
#endif

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(a[0]))
#endif


// ---------------------------------------------------------------------------
//
// Implementation selection
//
// There are now two implementations of the HC555xx family:
//
// * The original PinMAME implementation, which uses the standard discrete-
//   time filter algorithms to create a model of the ideal syllabic filter
//   per the published specifications in the HC55516 data sheet
//
// * A new HC555xx logic simulation, which re-creates the exact bit logic
//   implemented in the HC555xx chips, as determined by a 2020 decap
//   analysis of a specimen of the physical chip.
//
// The two implementations perform essentially identical math, since they're
// both implementing the same discrete-time filter system.  The old PinMAME
// version implements the math using native 'double' types, whereas the chip
// logic uses 10- and 12-bit 2's complement integer registers (which aren't
// actually integer values mathematically throughout the calculation: the
// chip is also doing fractional arithmetic, but doing so using explicit
// bit shifts rather than implicitly using C operators with floating-point
// types).  The old PinMAME version executes the intermediate calculations
// at higher precision than the chip, due to the wider registers it uses,
// but at the cost of some discrepancy from the exact bit values produced
// by the chip, due to the greater rounding error with the chip's narrow
// registers.
//
// You can select which implementation you want here.  The selection can
// be changed at run-time, although at the moment this isn't exposed as a
// user-configuration parameter.  The selection must be made prior to
// initialization, since it's fixed into the chip state structures once
// those are created.
//
// The two versions sound identical subjectively.  The old PinMAME version
// is a more accurate model of the abstract mathematical filter that the
// chip purports to implement, but the hardware logic version is a more
// accurate re-creation of the chip, and should exactly match its output
// as far as the chip's internal DAC stage.  (We obviously can't truly
// replicate anything beyond that, since the rest of the hardware system
// is in the analog domain.)
//
//   0 -> use the original PinMAME implementation
//   1 -> use the chip logic simulation
//
static const int hc55516_use_chip_logic = 1;


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
// but the initial gain calculation targets the wider range before compression.
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
//
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
// Note: these apply to the old PinMAME implementation of the filter,
// which uses IEEE doubles to hold intermediate results.  The original
// HC555xx implementation (as revealed by a decap analysis) used 10-
// and 12-bit fixed-point arithmetic.  The corresponding parameters
// are mathematically equivalent but are expressed in the C code as
// small integer values.
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

	// syllabic shift register
	UINT8	shiftreg;

	// Syllabic filter status and parameters
	struct
	{
		// Integer arithmetic, as used in HC555XX hardware
		struct
		{
			int syl_reg;           // syllabic filter register - 12-bit signed int
			int integrator;        // integrator register - 10-bit signed int

			// Syllabic filter parameters.  The specific values come from the
			// decap analysis, and differ for HC55516 vs HC55532/36/64.
			int charge_mask;
			int charge_shift;
			int charge_add;
			int decay_shift;

			double gain;
		} intg;

		// Floating-point arithmetic, used in original MAME implementation
		struct
		{
			double syl_level;      // simulated voltage on the syllabic filter
			double integrator;     // simulated voltage on integrator output

			// Output gain.  This is the conversion factor from the simulated voltage
			// level on the integrator to an INT16 PCM sample for the MAME stream.
			double gain;

		} dbl;
	} filter;

	// Bit processor function
	void (*process_bit)(struct hc55516_data *chip, const UINT8 bit, const double output_rate_ratio);

	// Loudness compression function
	float (*compress_loudness)(struct hc55516_data *chip, double sample);

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
	// generation-specific parameters during initialization, using the type 
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
		int nsamples;
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
	const double freq = stream_get_sample_rate(chip->channel);

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

	// When using the src_process or src_callback_process APIs and updating the src_ratio field of the SRC_STATE struct,
	// the library will try to smoothly transition between the conversion ratio of the last call and the conversion ratio of the current call.
	// BUT we can disable this via:
	src_set_ratio(chip->resample_state, sd.src_ratio);

	if (src_process(chip->resample_state, &sd) != SRC_ERR_NO_ERROR)
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
// This version uses a fixed-point fraction implementation, based on the 2020
// decap analysis of the HC55516's internal circuit traces.   The calculations
// here are done using 10- and 12-bit 2's complement integer registers, which 
// are interpreted in some of the intermediate steps as fixed-point fraction
// values.  The arithmetic is essentially the same as the original PinMAME
// implementation - not surprising, since both are implementing the same
// system of discrete-time filters.  But the correspondence is hard to see
// because the arithmetic here looks like integer arithmetic (which isn't
// entirely true; it uses native integer types, but it's actually performing
// fixed-point fraction arithmetic in some of the intermediate steps).  It's
// also difficult to see what's really going on arithmetically because some
// of the arithmetic primitives that a modern C program would express in
// terms of C arithmetic operators are decomposed into their constituent
// bit-twiddling operations, which obscures what they're really computing.
//
// One thing that's helpful in understanding the corresponding to the real
// arithmetic going on is to recognize is that (~N + 1) == -N for 2's
// complement representation, and thus ~N = -N - 1.  The chip's pipeline
// uses this idiom because it's more economical in a hardware layout to
// use a bunch of bit inverters than to implement a 2's-complement "negate"
// operation, since that requires a separate adder step.  This idiom is
// also used to express a subtraction as a bitwise NOT combined with an
// add.  Another obfuscating factor is that the syllabic filter is
// represented in the original hardware as a 12-bit signed integer register,
// and the integrator is a 10-bit signed integer register, so many of the
// operations in the C conversion need additional masking steps to replicate
// the behavior of the hardware, where bits would just fall off the high
// end of the register after an add or shift.  I've cleaned things up a
// little bit (from the baseline MAME implementation) to make the intent 
// clearer.  But I've also deliberately maintained fidelity to the chip's
// exact series of bit operations, so it's still pretty hard to follow 
// what's really going on.  The overall calculation, though, maps very
// directly to the floating-point version, so you can use that as a
// reference to see what the abstract mathematical formulas really are.
//

// Helper for the 10-bit integrator register: clip an integer to 10 bits
// (signed) -> -512..+511
INLINE int clip10bits(int v)
{
	// Saturate to 10-bit signed range
	//
	// Note: the MAME code, which doggedly reproduces the gate logic
	// from the chip decap, expresses this operation in the most obtuse
	// possible way, but MAME's version accomplishes the same thing.
	// Here's how MAME expresses it:
	//
	//  v = v & 0x3ff;
	//  return (v & 0x200) != 0 ? v | ~0x3FF : v;
	//
	// That's the hardware designer's way of thinking about arithmetic,
	// as something you have to decompose into inverts and NAND/NOR
	// gates.  We C programmers have the luxury of working with human-
	// readable numbers and algebraic expressions, so let's do that
	// instead.  It's not at all obvious that the MAME code expresses
	// the same operation as this nice simple math formula, so let's
	// look at what's going on in the bit logic.  The first line in the
	// MAME code masks the value to the low-order 10 bits (setting all
	// of the higher bits to zero, however many bits there are in the
	// native machine type representing the local variable v after the
	// C compiler turns it into machine code - probably 32 bits, but
	// it could be 16 or 64, or who knows, 36 if we ever get ported
	// to a PDP-10).  But the point is, whatever size 'v' is locally,
	// we now have just a 10-bit number in it, with all of the higher-
	// order bits zeroed.  On the original HC55516 hardware, this
	// number is *physically* stored in a 10-bit register, so there
	// actually are no higher-order bits to store - that's why MAME
	// has to zero them.  But the HC hardware thinks about this 10-bit
	// register as a signed 2's-complement value.  What does that mean
	// for the C translation?  It means that if the high-order bit of
	// this 10-bit value is '1', the 10-bit value is negative, so we
	// need our wider native C value to ALSO be negative.  How do we
	// make it negative?  By filling all of the higher-order bits
	// with '1' bits.  That's what that second line is doing: it's
	// testing the sign bit of the 10-bit value, at bit 0x200, and
	// if it's set (meaning the 10-bit value is negative), it fills
	// in all of the higher-order bits in the native C 'v' with '1'
	// bits.  If bit 0x200 is zero, it does nothing, leaving the
	// higher-order bits as zeros (which we guaranteed that they
	// already are via the first line).
	return v < -512 ? -512 : v > 511 ? 511 : v;
}

// helper for the 10-bit integrator register: sign-extend a 10-bit value
// to the width of the local native int type, so that we can do a signed
// arithmetic intermediate operation with int variables
INLINE int signext10bits(int v)
{
	return (v & 0x200) != 0 ? v | ~0x3FF : v;
}

// bit processor
static void process_bit_HC555XX(struct hc55516_data *chip, const UINT8 bit, const double output_rate_ratio)
{
	// shift the bit into the shift register
	chip->shiftreg = ((chip->shiftreg << 1) | bit) & SHIFTMASK;

	// Apply the syllabic filter charge update (in floating-point terms,
	// this is calculating syl *= 31/32 or syl *= 63/64, depending upon
	// the charge_mask/charge_shift parameters).  This part is common to
	// coincidence (last 3 bits were the same) or non-coincidence.
	chip->filter.intg.syl_reg += (~chip->filter.intg.syl_reg & chip->filter.intg.charge_mask) >> chip->filter.intg.charge_shift;

	// add extra charge on non-coincidence
	if (chip->shiftreg != 0 && chip->shiftreg != SHIFTMASK)
		chip->filter.intg.syl_reg += chip->filter.intg.charge_add;

	// mask the syllabic filter register to 12 bits, per the HC555XX hardware
	chip->filter.intg.syl_reg &= 0xFFF;

	// apply the integrator filter decay update (in floating-point terms,
	// this is calculating integrator *= 15/16 or 31/32, depending upon
	// the INTSHIFT parameter)
	int sum = signext10bits(((~chip->filter.intg.integrator >> chip->filter.intg.decay_shift) + 1) & 0x3FF);
	chip->filter.intg.integrator = clip10bits(chip->filter.intg.integrator + sum);

	// scale the sample from 10-bit signed (-512..511) to 16-bit signed (-32768..32767)
	int sample = (chip->filter.intg.integrator << 6) | (((chip->filter.intg.integrator & 0x3FF) ^ 0x200) >> 4);

	// buffer the sample
	add_sample_out(chip, sample / 32768.0, output_rate_ratio);

	// Charge the integrator from the syllabic filter according to the 
	// current data bit.
	//
	// Note: the MAME version of this code, which is a doggedly literal
	// translation of the HC55516 gate logic (from a decap analysis) into
	// C, expresses the negation of 'sum' as a somewhat obtuse bit-
	// twiddling operation.  We C programmers have the luxury of a more
	// abstract mathematical expression of our intent, so let's take
	// advantage of that and not resort to unreadable bit twiddling.
	// MAME expresses the negation like this:
	// 
	//    sum = (~sum) + 1;
	//    sum = signext10bits(sum & 0x3FF);
	// 
	// That bit-twiddling formula is the canonical bit-logic decomposition
	// of the 2's complement negation operation - you invert all the bits
	// and add 1.  The physical HC chip does it that way because that's
	// the way you do it in logic gates.  For a C program, it will work
	// on 2's-complement platforms to do the same decomposition, but it's
	// actually not 100% portable to do so, because there do exist machines
	// with other integer representations (although none that anyone would
	// use today, admittedly).  The math operation that we're trying to
	// achieve is a negation, so it's more strictly correct to write it
	// that way and let the compiler translate it into the appropriate
	// machine operation for us.
	sum = chip->filter.intg.syl_reg >> 6;
	if (sum < 2)
		sum = 2;
	if ((chip->shiftreg & 1) != 0)
		sum = -sum;
	chip->filter.intg.integrator = clip10bits(chip->filter.intg.integrator + sum);
}

// We don't need any loudness compression for the integer math implementation,
// because this implementation has an inherently limited 10-bit dynamic range
// (limited by the precision of the 10-bit integrator register).  This is
// narrower than the MAME stream's 16-bit dynamic range and thus doesn't need
// any range compression.
static float flat_loudness(struct hc55516_data *chip, double sample)
{
	return (float)(sample * chip->filter.intg.gain);
}

// ---------------------------------------------------------------------------
//
// Process an input bit using the original PinMAME double-precision float
// implementation of the filter.  This implements the same standard discrete-
// time filter math as the decap version above, but it uses double-precision
// floats instead of fixed-point fractions.  This performs the calculations
// with higher precision than the original chip, which is perhaps both a plus
// and minus: it's more accurate mathematically than the original hardware,
// but that very thing makes it a less than perfect re-creation.
//
// This implementation also lacks the integrator filter clipping that the
// original hardware does.  The chip's 10-bit integrator register saturates
// at its extremes (-512..+511), whereas this implementation allows the
// integrator register to grow to whatever a double can hold, which for
// our purposes is effectively unlimited.  This actually matters: many of
// the original System 11 sound clips actually do saturate the integrator in
// the 10-bit version.  As with the higher arithmetic precision of the
// 'double' version, the greater dynamic range of the 'double' version
// probably results in better quality sound, but makes it a less accurate
// re-creation.  It also has the downside that it motivated the addition of
// a loudness compression curve to deal with the observation that the
// double implementation can exceed the 16-bit dynamic range of the MAME
// stream and thus needs range compression.  The loudness compression might
// be more detrimental to the sound quality than the precision and range
// upgrades are positives.
//
static void process_bit_dbl(struct hc55516_data *chip, const UINT8 bit, const double output_rate_ratio)
{
	// add/subtract the syllabic filter output to/from the integrator
	const double di = (1.0 - DECAY) * chip->filter.dbl.syl_level;
	if (bit != 0)
		chip->filter.dbl.integrator += di;
	else
		chip->filter.dbl.integrator -= di;

	// simulate leakage
	chip->filter.dbl.integrator *= DECAY;

	// shift the new data bit into the syllabic filter's shift register
	chip->shiftreg = ((chip->shiftreg << 1) | bit) & SHIFTMASK;

	// figure the new syllabic filter output level
	chip->filter.dbl.syl_level *= CHARGE;
	chip->filter.dbl.syl_level += (1.0 - CHARGE) * ((chip->shiftreg == 0 || chip->shiftreg == SHIFTMASK) ? V_HIGH : V_LOW);

	// buffer the sample
	add_sample_out(chip, chip->filter.dbl.integrator, output_rate_ratio);
}

// Loudness compression function.  This takes a sample in linear space form
// -DYN_RANGE_MAX to +DYN_RANGE_MAX and compresses it to fit -1..1.
// The curve is linear at low volumes and flattens at higher volumes, which
// tends to make the overall signal sound louder.
static float compress_loudness(struct hc55516_data *chip, double sample)
{
	// apply gain
	sample *= chip->filter.dbl.gain;

	// apply compression
	return (float)(sample / (32768.0 + fabs(sample)) + sample * (0.15 * (1.0 / 32768.0)));
}

// Alternative loudness compression: this version simply clips to the 16-bit
// range.
static float compress_clip(struct hc55516_data *chip, double sample)
{
	return (float)(sample < -32768.0 ? -32768.0 : sample > 32767.0 ? 32767.0 : sample);
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
	const float scaled = (*chip->compress_loudness)(chip, filtered);

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
			(*chip->process_bit)(chip, chip->bits_in.bits[i++].bit, ratio);
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

// Set the gain, as a multiple of the default gain.  The default gain
// yields a 1:1 mapping from the full dynamic range of the HC55516 to
// the full dynamic range of the MAME stream.
void hc55516_set_mixing_level(int num, double gain)
{
	if (gain < 1.0)
	{
		mixer_set_mixing_level(hc55516[num].channel, (int)(gain * 100.));
		gain = 1.0;
	}
	else
		mixer_set_mixing_level(hc55516[num].channel, 100);

	hc55516[num].filter.dbl.gain = gain * DEFAULT_GAIN; // for hc55516_use_chip_logic == 0
	hc55516[num].filter.intg.gain = gain; // for hc55516_use_chip_logic == 1
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
// de-facto playback rate at run time.)
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

		// Set up the chip bit processor implementation
		if (hc55516_use_chip_logic)
		{
			// Chip type.  The newer MAME decap implementation distinguishes
			// the chip types because the internal logic uses slightly different
			// arithmetic parameters across the different chips.  PinMAME doesn't
			// have a full notion of which chip we're using more specific
			// than "55516 family" for most machines, most of the pinball
			// sound boards that used these chips used the 55536 variant (the
			// low-cost decoder-only version of the 55532) though, earlier ones the 55516.

			// use the chip logic simulation, with a flat loudness curve
			chip->process_bit = process_bit_HC555XX;
			chip->compress_loudness = flat_loudness;

			// set the default parameters for this version of the filter
			if (intf->volume[i] > 100)
				chip->filter.intg.gain = intf->volume[i]/100.0;
			else
				chip->filter.intg.gain = 1.0;

			// Populate the filter parameters according to the chip type
			if (intf->chip_type == 55516 || intf->chip_type == 55536 || intf->chip_type == 55564)
			{
				// 55516 parameters.  These were obtained from the MAME decap analysis.
				// 
				// The 55536 and 55564 use the same parameters, or so it seems from the 
				// data sheets (this hasn't been verified by decap analysis).  These
				// chips are also believed to include a fix for a bug in the -16 that 
				// prevented it from reaching a constant 0V at the output, which caused 
				// a known noise ("humming") bug with the older chip.  (I'm not sure
				// whether or not the MAME algorithm implemented here includes the fix.)
				chip->filter.intg.charge_mask = 0xFC0;
				chip->filter.intg.charge_shift = 6;
				chip->filter.intg.charge_add = 0xFC1;
				chip->filter.intg.decay_shift = 4;

				chip->filter.intg.syl_reg = 0x3F;
			}
			else if (intf->chip_type == 55532)
			{
				// 55532 parameters.  The MAME decap analysis didn't include this chip,
				// so these are only inferred, based on the chip's optimization for 
				// 32 kHz clocking.
				chip->filter.intg.charge_mask = 0xF80;
				chip->filter.intg.charge_shift = 7;
				chip->filter.intg.charge_add = 0xFE1;
				chip->filter.intg.decay_shift = 5;

				chip->filter.intg.syl_reg = 0x7F;
			}
			else
			{
				// invalid chip selection
				return 1;
			}
		}
		else
		{
			// Use the original PinMAME ideal syllabic filter simulation, with the
			// corresponding loudness compression curve.  (Dynamic range compression
			// is required with this filter simulation because it doesn't have any
			// internal clipping, which allows its effective dynamic range to exceed
			// the 16-bit range of the the MAME stream.)
			//
			// An alternative to the non-linear compression curve that would be truer
			// to the original HC55516 logic would be to clip the sample to the 16-bit
			// range.  That might also require some scaling (i.e., multiply the samples
			// by X, where X > 1.0) to raise the floor level.
			chip->process_bit = process_bit_dbl;
			chip->compress_loudness = compress_loudness;

			// set the default parameters for this version of the filter
			if (intf->volume[i] > 100)
				chip->filter.dbl.gain = DEFAULT_GAIN * intf->volume[i] / 100.0;
			else
				chip->filter.dbl.gain = DEFAULT_GAIN;
		}

		// create the output stream
		switch(intf->chip_type)
		{
		case 55516: sprintf(name, "HC55516 #%d", i); break;
		case 55532: sprintf(name, "HC55532 #%d", i); break;
		case 55536: sprintf(name, "HC55536 #%d", i); break;
		case 55564: sprintf(name, "HC55564 #%d", i); break;
		default:    sprintf(name, "unknown HC555XX #%d", i); break;
		}

		chip->channel = stream_init_float(name, MIN(intf->volume[i],100), Machine->sample_rate, i, hc55516_update, 1); // pick output sample rate for max quality, also saves an additional filtering step in the mixer!
		chip->stream_update_time = timer_get_time();
		chip->output_dt = 1.0 / Machine->sample_rate;
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

		// When using the src_process or src_callback_process APIs and updating the src_ratio field of the SRC_STATE struct,
		// the library will try to smoothly transition between the conversion ratio of the last call and the conversion ratio of the current call.
		// BUT we can disable this via:
		src_set_ratio(chip->resample_state, sd.src_ratio);

		src_process(chip->resample_state, &sd);
	}

	/* success */
	return 0;
}
