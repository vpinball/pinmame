/***************************************************************************

  mixer.c

  Manage audio channels allocation, with volume and panning control

***************************************************************************/

#include "driver.h"
#include "filter.h"

#include <math.h>
#include <limits.h>
#include <assert.h>

/***************************************************************************/
/* Options */

/* Undefine to use builtin lowpass filter/resampling code, way less quality though, and also not faster anymore */
/* Also, undefining it misses the new reverb filter (Centaur,..) */
#define USE_LIBSAMPLERATE

/* Undefine it to turn off clipping (helpful to find cases where we max out) */
#define MIXER_USE_CLIPPING

/* Define it to enable the logerror output */
/* #define MIXER_USE_LOGERROR */

/***************************************************************************/
/* Config */

#ifdef USE_LIBSAMPLERATE
 #include "../../ext/libsamplerate/samplerate.h"
 #include "../../ext/libsamplerate/samplerate.c"
 #include "../../ext/libsamplerate/src_linear.c"
 #include "../../ext/libsamplerate/src_sinc_opt.c"
 #include "../../ext/libsamplerate/src_zoh.c"
#endif

/* Internal log */
#ifdef MIXER_USE_LOGERROR
#define mixerlogerror(a) logerror a
#else
#define mixerlogerror(a) do { } while (0)
#endif

/* accumulators have ACCUMULATOR_SAMPLES samples (must be a power of 2) */
#define ACCUMULATOR_SAMPLES		8192
#define ACCUMULATOR_MASK		(ACCUMULATOR_SAMPLES - 1)

/* fractional numbers have FRACTION_BITS bits of resolution */
#define FRACTION_BITS			16
#define FRACTION_ONE			(1 << FRACTION_BITS)
#define FRACTION_MASK			(FRACTION_ONE - 1)

#ifndef MIN
 #define MIN(x,y) ((x)<(y)?(x):(y))
#endif

/***************************************************************************/
/* Static data */

static int mixer_sound_enabled;

/* holds all the data for the a mixer channel */
struct mixer_channel_data
{
	char name[40];

	/* current volume, gain and pan */
	int left_volume;
	int right_volume;
	int gain;
	int pan;

	/* mixing levels */
	unsigned mixing_level;
	unsigned default_mixing_level;
	unsigned config_mixing_level;
	unsigned config_default_mixing_level;

	/* current playback positions */
	unsigned samples_available;

	/* resample state */
	unsigned from_frequency; /* current source frequency */
	unsigned to_frequency; /* current destination frequency */
	int is_reset_requested; /* state reset requested */

	/* state of non-streamed playback */
	int is_stream;
	int is_playing;
	int is_looping;
	int is_16bit;
	void* data_start;
	void* data_end;
	void* data_current;

	int frac; /* resample fixed point state (used if filter is not active or in case of USE_LIBSAMPLERATE for the samples playback) */

#ifdef USE_LIBSAMPLERATE
	SRC_STATE* src_left;
	SRC_STATE* src_right;

	int lr_silent_value[2]; // detect complete silence of a channel
	int lr_silence[2];

	int legacy_resample; // fallback to old legacy samples playback
#else
	int pivot; /* resample brehesnam state (used if filter is active) */
	int step; /* fixed point increment */

	/* lowpass filter request */
	unsigned request_lowpass_frequency; /* request for the lowpass arbitrary cut frequency, 0 if default */
	unsigned lowpass_frequency; /* current lowpass arbitrary cut frequency, 0 if default */

	filter* filter; /* filter used, ==0 if none */
	filter_state* left; /* state of the filter for the left/mono channel */
	filter_state* right; /* state of the filter for the right channel */
#endif

#ifdef PINMAME
	int reverbPos[2];
	float reverbDelay[2];
	float reverbForce[2];
#define REVERB_LENGTH 100000
	float reverbBuffer[2][REVERB_LENGTH];
#endif
};

/* channel data */
static struct mixer_channel_data mixer_channel[MIXER_MAX_CHANNELS];
static unsigned config_mixing_level[MIXER_MAX_CHANNELS];
static unsigned config_default_mixing_level[MIXER_MAX_CHANNELS];
static int first_free_channel = 0;
static int is_config_invalid;
static int is_stereo;

/* 32-bit accumulators */
static unsigned accum_base;

#ifdef USE_LIBSAMPLERATE
static float left_accum[ACCUMULATOR_SAMPLES];
static float right_accum[ACCUMULATOR_SAMPLES];
static float in_f[ACCUMULATOR_SAMPLES*25]; //!! 25=magic, should be able to handle all cases where src sample rate is far far larger than dst sample rate (e.g. 4x48000 -> 8000), if changing also change asserts and overflow check in below code (search for ACCUMULATOR_MASK*25)
static float out_f[ACCUMULATOR_SAMPLES];
#else
static int left_accum[ACCUMULATOR_SAMPLES];
static int right_accum[ACCUMULATOR_SAMPLES];
#endif

/* 16-bit mix buffers */
static INT16 mix_buffer[ACCUMULATOR_SAMPLES*2]; /* *2 for stereo */

/* global sample tracking */
static unsigned samples_this_frame;

#ifdef PINMAME
static void mixer_apply_reverb_filter(struct mixer_channel_data* const channel, float * const __restrict buf, const int len, const unsigned left_right)
{
	if (channel->reverbDelay[left_right] != 0.f && len) {
		float * const __restrict rev_buf = channel->reverbBuffer[left_right];
		int i;
		const float rev_force = channel->reverbForce[left_right] * (left_right ? 1.f : 1.04f); // magic: slightly different parameters for left and right makes reverb sound a bit more natural
		int rev_pos = channel->reverbPos[left_right];
		int newPos = rev_pos - (int)(channel->reverbDelay[left_right] * channel->to_frequency * (left_right ? 1.04f : 1.f)); // magic: slightly different parameters for left and right makes reverb sound a bit more natural
		if (newPos < 0) newPos += REVERB_LENGTH;
		for (i = 0; i < len; i++) {
			buf[i] = buf[i] + (rev_buf[newPos] - buf[i]) * rev_force;
			rev_buf[rev_pos] = buf[i];
			rev_pos++; if (rev_pos > REVERB_LENGTH) rev_pos = 0;
			newPos++; if (newPos > REVERB_LENGTH) newPos = 0;
		}
		channel->reverbPos[left_right] = rev_pos;
	}
}
#endif

/***************************************************************************
	mixer_channel_resample
***************************************************************************/

/* Window size of the FIR filter in samples (must be odd) */
/* Greater values are more precise, lesser values are faster. */
#define FILTER_WIDTH FILTER_ORDER_MAX

/* The number of samples that need to be played to flush the filter state */
/* For the FIR filters it's equal to the filter width */
#define FILTER_FLUSH FILTER_WIDTH

/* Setup the resample information
	from_frequency - input frequency
	lowpass_frequency - lowpass frequency, use 0 to automatically compute it from the resample operation
	restart - restart the resample state
*/
static void mixer_channel_resample_set(struct mixer_channel_data *channel, unsigned from_frequency,
#ifndef USE_LIBSAMPLERATE
	unsigned lowpass_frequency,
#endif
	int restart)
{
	unsigned to_frequency;
	to_frequency = Machine->sample_rate;

#ifdef USE_LIBSAMPLERATE
	mixerlogerror(("Mixer:mixer_channel_resample_set(%s,%d,%d)\n", channel->name, from_frequency, restart));

	if (restart)
		channel->frac = 0;
#else
	mixerlogerror(("Mixer:mixer_channel_resample_set(%s,%d,%d,%d)\n", channel->name, from_frequency, lowpass_frequency, restart));

	if (restart)
	{
		mixerlogerror(("\tpivot=0\n"));
		channel->pivot = 0;
		channel->frac = 0;
	}

	/* only if the filter change */
	if (from_frequency != channel->from_frequency
		|| to_frequency != channel->to_frequency
		|| lowpass_frequency != channel->lowpass_frequency)
	{
		/* delete the previous filter */
		if (channel->filter)
		{
			filter_free(channel->filter);
			channel->filter = 0;
		}

		/* make a new filter */
		//if (options.use_filter)
		if ((from_frequency != 0 && to_frequency != 0 && (from_frequency != to_frequency || lowpass_frequency != 0)))
		{
			double cut;
			unsigned cut_frequency;

			if (from_frequency < to_frequency) {
				/* upsampling */
				cut_frequency = from_frequency / 2;
				if (lowpass_frequency != 0 && cut_frequency > lowpass_frequency)
					cut_frequency = lowpass_frequency;
				cut = (double)cut_frequency / to_frequency;
			} else {
				/* downsampling */
				cut_frequency = to_frequency / 2;
				if (lowpass_frequency != 0 && cut_frequency > lowpass_frequency)
					cut_frequency = lowpass_frequency;
				cut = (double)cut_frequency / from_frequency;
			}

			channel->filter = filter_lp_fir_alloc(cut, FILTER_WIDTH);

			mixerlogerror(("\tfilter from %d Hz, to %d Hz, cut %f, cut %d Hz\n",from_frequency,to_frequency,cut,cut_frequency));
		}
	}

	channel->lowpass_frequency = lowpass_frequency;
	channel->step = ((unsigned long long)from_frequency << FRACTION_BITS) / to_frequency;
#endif

	channel->from_frequency = from_frequency;
	channel->to_frequency = to_frequency;

	/* reset the filter state */
	if (
#ifndef USE_LIBSAMPLERATE
		channel->filter &&
#endif
		channel->is_reset_requested)
	{
		mixerlogerror(("\tstate clear\n"));
		channel->is_reset_requested = 0;
#ifdef USE_LIBSAMPLERATE
		src_reset(channel->src_left);
		src_reset(channel->src_right);
#else
		filter_state_reset(channel->filter,channel->left);
		filter_state_reset(channel->filter,channel->right);
#endif
	}
}

/* Resample a channel
	channel - channel info
	state - filter state
	volume - volume (0-255)
	dst - destination vector
	dst_len - max number of destination samples
	src - source vector, (updated at the exit)
	src_len - max number of source samples
*/
static unsigned mixer_channel_resample_16(struct mixer_channel_data* channel,
#ifdef USE_LIBSAMPLERATE
	SRC_STATE* src_state,
#else
	filter_state* state,
#endif
	int volume,
#ifdef USE_LIBSAMPLERATE
	float* const __restrict dst,
#else
	int* const __restrict dst,
#endif
	unsigned dst_len, INT16** psrc, unsigned src_len, unsigned left_right)
{
	unsigned dst_base = (accum_base + channel->samples_available) & ACCUMULATOR_MASK;
	unsigned dst_pos = dst_base;

	const INT16* __restrict src = *psrc;

#ifdef USE_LIBSAMPLERATE
	SRC_DATA data;
	long i;
	const float scale_copy = (float)(volume / 256.0 / 0x8000);
	const float scale_volume = (float)(volume / 256.0);

	//limit src_len input length, roughly same as old code did basically:
	src_len = MIN(src_len, MAX((unsigned int)(dst_len*1.2*((double)channel->from_frequency / channel->to_frequency)),1)); //1.2=magic, limit incoming input, so that not all is immediately processed

	if (src_len == 0 || dst_len == 0)
		return 0;

	assert(src_len <= ACCUMULATOR_MASK*25); //!! magic see in_f

	src_len = MIN(src_len, ACCUMULATOR_MASK*25);
#endif
	assert( dst_len <= ACCUMULATOR_MASK );

	if (channel->from_frequency == channel->to_frequency) // raw copy, no filtering
	{
		const unsigned len = (src_len > dst_len) ? dst_len : src_len;
		const INT16* const __restrict src_end = src + len;
		while (src != src_end)
		{
#ifdef USE_LIBSAMPLERATE
			dst[dst_pos] += (float)*src * scale_copy;
#else
			dst[dst_pos] += ((int)*src * volume) >> 8;
#endif
			dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			++src;
		}

		*psrc = src;
		return (dst_pos - dst_base) & ACCUMULATOR_MASK;
	}

#ifdef USE_LIBSAMPLERATE
	// Detect complete silence to skip the resampling of such an unused channel
	if (!channel->legacy_resample && channel->lr_silence[left_right])
	{
		// first sample? -> init
		if (channel->lr_silent_value[left_right] == INT_MAX)
			channel->lr_silent_value[left_right] = src[0];

		for (i = 0; (unsigned int)i < src_len; ++i)
			if (src[i] != channel->lr_silent_value[left_right])
			{
				channel->lr_silence[left_right] = 0;
				break;
			}
	}

	// Special/Legacy samples playback code-path:

	if (channel->legacy_resample || channel->lr_silence[left_right])
	{
		/* end address */
		const INT16* const __restrict src_end = src + src_len;
		const unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		const int step = ((unsigned long long)channel->from_frequency << FRACTION_BITS) / channel->to_frequency;
		int frac = channel->frac;
		src += frac >> FRACTION_BITS;
		frac &= FRACTION_MASK;

		while (src < src_end && dst_pos != dst_pos_end)
		{
			dst[dst_pos] += (float)*src * scale_copy;
			frac += step;
			dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;
		}

		/* adjust the end if it's too big */
		if (src > src_end) {
			frac += (int)(src - src_end) << FRACTION_BITS;
			src = src_end;
		}

		channel->frac = frac;

		*psrc = src;
		return (dst_pos - dst_base) & ACCUMULATOR_MASK;
	}

	// Normal libsamplerate code-path:

	src_short_to_float_array(src, in_f, src_len);

	data.data_in = in_f;
	data.data_out = out_f;
	data.input_frames = src_len;
	data.output_frames = dst_len;
	data.end_of_input = 0;
	data.src_ratio = (double)channel->to_frequency / (double)channel->from_frequency;

	src_process(src_state, &data);

	mixer_apply_reverb_filter(channel, out_f, data.output_frames_gen, left_right);

	for (i = 0; i < data.output_frames_gen; ++i)
	{
		dst[dst_pos] += out_f[i] * scale_volume;
		dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
	}

	*psrc = src+data.input_frames_used;
	return (dst_pos - dst_base) & ACCUMULATOR_MASK;

#else

	if (!channel->filter)
	{
			/* end address */
			INT16* src_end = src + src_len;
			unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

			int step = channel->step;
			int frac = channel->frac;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;

			while (src < src_end && dst_pos != dst_pos_end)
			{
				dst[dst_pos] += (*src * volume) >> 8;
				frac += step;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				src += frac >> FRACTION_BITS;
				frac &= FRACTION_MASK;
			}

			/* adjust the end if it's too big */
			if (src > src_end) {
				frac += (int)(src - src_end) << FRACTION_BITS;
				src = src_end;
			}

			channel->frac = frac;
	} else if (!channel->from_frequency) {
		dst_pos = (dst_pos + dst_len) & ACCUMULATOR_MASK;
	} else {
		int pivot = channel->pivot;

		/* end address */
		INT16* src_end = src + src_len;
		unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		/* volume */
		filter_real v = volume;

		if (channel->from_frequency < channel->to_frequency)
		{
			/* upsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,
#ifdef FILTER_USE_INT
					(*src * v) >> 8);
#else
					(*src * v) * (1./256.));
#endif
				pivot += channel->from_frequency;
				if (pivot > 0)
				{
					pivot -= channel->to_frequency;
					++src;
				}
				/* dest */
				dst[dst_pos] += filter_compute(channel->filter,state);
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			}
		} else {
			/* downsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,
#ifdef FILTER_USE_INT
					(*src * v) >> 8);
#else
					(*src * v) * (1./256.));
#endif
				pivot -= channel->to_frequency;
				++src;
				/* dest */
				if (pivot < 0)
				{
					pivot += channel->from_frequency;
					dst[dst_pos] += filter_compute(channel->filter,state);
					dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				}
			}
		}

		channel->pivot = pivot;
	}

	*psrc = src;
	return (dst_pos - dst_base) & ACCUMULATOR_MASK;
#endif
}

static unsigned mixer_channel_resample_8(struct mixer_channel_data *channel,
#ifdef USE_LIBSAMPLERATE
	SRC_STATE* src_state,
#else
	filter_state* state,
#endif
	int volume,
#ifdef USE_LIBSAMPLERATE
	float* const __restrict dst,
#else
	int* const __restrict dst,
#endif
	unsigned dst_len, INT8** psrc, unsigned src_len, unsigned left_right)
{
	unsigned dst_base = (accum_base + channel->samples_available) & ACCUMULATOR_MASK;
	unsigned dst_pos = dst_base;

	const INT8* __restrict src = *psrc;

#ifdef USE_LIBSAMPLERATE
	SRC_DATA data;
	long i;
	const float scale_copy = (float)(volume / 256.0 / 0x80);
	const float scale_volume = (float)(volume / 256.0);

	//limit src_len input length, roughly same as old code did basically:
	src_len = MIN(src_len, MAX((unsigned int)(dst_len*1.2*((double)channel->from_frequency / channel->to_frequency)),1)); //1.2=magic, limit incoming input, so that not all is immediately processed

	if (src_len == 0 || dst_len == 0)
		return 0;

	assert(src_len <= ACCUMULATOR_MASK*25); //!! magic see in_f

	src_len = MIN(src_len, ACCUMULATOR_MASK*25);
#endif
	assert( dst_len <= ACCUMULATOR_MASK );

	if (channel->from_frequency == channel->to_frequency) // raw copy, no filtering
	{
		/* copy */
		const unsigned len = (src_len > dst_len) ? dst_len : src_len;
		const INT8* const __restrict src_end = src + len;
		while (src != src_end)
		{
#ifdef USE_LIBSAMPLERATE
			dst[dst_pos] += (float)*src * scale_copy;
#else
			dst[dst_pos] += (int)*src * volume;
#endif
			dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			++src;
		}

		*psrc = src;
		return (dst_pos - dst_base) & ACCUMULATOR_MASK;
	}

#ifdef USE_LIBSAMPLERATE
	// Special/Legacy samples playback code-path:

	if (channel->legacy_resample)
	{
		/* end address */
		const INT8* const __restrict src_end = src + src_len;
		const unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		const int step = ((unsigned long long)channel->from_frequency << FRACTION_BITS) / channel->to_frequency;
		int frac = channel->frac;
		src += frac >> FRACTION_BITS;
		frac &= FRACTION_MASK;

		while (src < src_end && dst_pos != dst_pos_end)
		{
			dst[dst_pos] += (float)*src * scale_copy;
			dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			frac += step;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;
		}

		/* adjust the end if it's too big */
		if (src > src_end) {
			frac += (int)(src - src_end) << FRACTION_BITS;
			src = src_end;
		}

		channel->frac = frac;

		*psrc = src;
		return (dst_pos - dst_base) & ACCUMULATOR_MASK;
	}

	// Normal libsamplerate code-path:
	
	src_char_to_float_array(src, in_f, src_len);

	data.data_in = in_f;
	data.data_out = out_f;
	data.input_frames = src_len;
	data.output_frames = dst_len;
	data.end_of_input = 0;
	data.src_ratio = (double)channel->to_frequency / (double)channel->from_frequency;

	src_process(src_state, &data);

	mixer_apply_reverb_filter(channel, out_f, data.output_frames_gen, left_right);

	for (i = 0; i < data.output_frames_gen; ++i)
	{
		dst[dst_pos] += out_f[i] * scale_volume;
		dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
	}

	*psrc = src + data.input_frames_used;
	return (dst_pos - dst_base) & ACCUMULATOR_MASK;

#else

	if (!channel->filter)
	{
		 	/* end address */
			INT8* src_end = src + src_len;
			unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

			int step = channel->step;
			int frac = channel->frac;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;

			while (src < src_end && dst_pos != dst_pos_end)
			{
				dst[dst_pos] += *src * volume;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				frac += step;
				src += frac >> FRACTION_BITS;
				frac &= FRACTION_MASK;
			}

			/* adjust the end if it's too big */
			if (src > src_end) {
				frac += (int)(src - src_end) << FRACTION_BITS;
				src = src_end;
			}

			channel->frac = frac;
	} else if (!channel->from_frequency) {
		dst_pos = (dst_pos + dst_len) & ACCUMULATOR_MASK;
	} else {
		int pivot = channel->pivot;

		/* end address */
		INT8* src_end = src + src_len;
		unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		/* volume */
		int v = volume;

		if (channel->from_frequency < channel->to_frequency)
		{
			/* upsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v);
				pivot += channel->from_frequency;
				if (pivot > 0)
				{
					pivot -= channel->to_frequency;
					++src;
				}
				/* dest */
				dst[dst_pos] += filter_compute(channel->filter,state);
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			}
		} else {
			/* downsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v);
				pivot -= channel->to_frequency;
				++src;
				/* dest */
				if (pivot < 0)
				{
					pivot += channel->from_frequency;
					dst[dst_pos] += filter_compute(channel->filter,state);
					dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				}
			}
		}

		channel->pivot = pivot;
	}

	*psrc = src;
	return (dst_pos - dst_base) & ACCUMULATOR_MASK;
#endif
}

/* Mix a 8 bit channel */
static unsigned mixer_channel_resample_8_pan(struct mixer_channel_data *channel, int* volume, unsigned dst_len, INT8** src, unsigned src_len)
{
	unsigned count;

#ifdef MMSND
	if( mmsnd_stereomono ){
	  /**** all sound mono mode ****/
	  /* save */
	  unsigned save_pivot = channel->pivot;
	  unsigned save_frac = channel->frac;
	  INT8* save_src = *src;
	  count = mixer_channel_resample_8(channel, channel->left, volume[0], left_accum, dst_len, src, src_len, 0);
	  /* restore */
	  channel->pivot = save_pivot;
	  channel->frac = save_frac;
	  *src = save_src;
	  mixer_channel_resample_8(channel, channel->right, volume[1], right_accum, dst_len, src, src_len, 1);
	  channel->samples_available += count;
	  return count;
	}
#endif

#ifdef USE_LIBSAMPLERATE
	SRC_STATE *cl = channel->src_left;
	SRC_STATE *cr = channel->src_right;
#else
	filter_state *cl = channel->left;
	filter_state *cr = channel->right;
#endif

	if (!is_stereo || channel->pan == MIXER_PAN_LEFT) {
		count = mixer_channel_resample_8(channel, cl, volume[0], left_accum, dst_len, src, src_len, 0);
	} else if (channel->pan == MIXER_PAN_RIGHT) {
		count = mixer_channel_resample_8(channel, cr, volume[1], right_accum, dst_len, src, src_len, 1);
	} else {
		/* save */
#ifndef USE_LIBSAMPLERATE
		unsigned save_pivot = channel->pivot;
#endif
		unsigned save_frac = channel->frac;
		INT8* save_src = *src;
		count = mixer_channel_resample_8(channel, cl, volume[0], left_accum, dst_len, src, src_len, 0);
		/* restore */
#ifndef USE_LIBSAMPLERATE
		channel->pivot = save_pivot;
#endif
		channel->frac = save_frac;
		*src = save_src;
		mixer_channel_resample_8(channel, cr, volume[1], right_accum, dst_len, src, src_len, 1);
	}

	channel->samples_available += count;
	return count;
}

/* Mix a 16 bit channel */
static unsigned mixer_channel_resample_16_pan(struct mixer_channel_data *channel, int* volume, unsigned dst_len, INT16** src, unsigned src_len)
{
	unsigned count;

#ifdef MMSND
	if( mmsnd_stereomono ){
	  /**** all sound mono mode ****/
	  /* save */
	  unsigned save_pivot = channel->pivot;
	  unsigned save_frac = channel->frac;
	  INT16* save_src = *src;
	  count = mixer_channel_resample_16(channel, channel->left, volume[0], left_accum, dst_len, src, src_len, 0);
	  /* restore */
	  channel->pivot = save_pivot;
	  channel->frac = save_frac;
	  *src = save_src;
	  mixer_channel_resample_16(channel, channel->right, volume[1], right_accum, dst_len, src, src_len, 1);
	  channel->samples_available += count;
	  return count;
	}
#endif

#ifdef USE_LIBSAMPLERATE
	SRC_STATE *cl = channel->src_left;
	SRC_STATE *cr = channel->src_right;
#else
	filter_state *cl = channel->left;
	filter_state *cr = channel->right;
#endif

	if (!is_stereo || channel->pan == MIXER_PAN_LEFT) {
		count = mixer_channel_resample_16(channel, cl, volume[0], left_accum, dst_len, src, src_len, 0);
	} else if (channel->pan == MIXER_PAN_RIGHT) {
		count = mixer_channel_resample_16(channel, cr, volume[1], right_accum, dst_len, src, src_len, 1);
	} else {
		/* save */
#ifndef USE_LIBSAMPLERATE
		unsigned save_pivot = channel->pivot;
#endif
		unsigned save_frac = channel->frac;
		INT16* save_src = *src;
		count = mixer_channel_resample_16(channel, cl, volume[0], left_accum, dst_len, src, src_len, 0);
		/* restore */
#ifndef USE_LIBSAMPLERATE
		channel->pivot = save_pivot;
#endif
		channel->frac = save_frac;
		*src = save_src;
		mixer_channel_resample_16(channel, cr, volume[1], right_accum, dst_len, src, src_len, 1);
	}

	channel->samples_available += count;
	return count;
}

/***************************************************************************
	mix_sample_8
***************************************************************************/

void mix_sample_8(struct mixer_channel_data *channel, int samples_to_generate)
{
	INT8 *source, *source_end;
	int mixing_volume[2];

	/* compute the overall mixing volume */
	if (mixer_sound_enabled)
	{
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}
	/* get the initial state */
	source = channel->data_current;
	source_end = channel->data_end;

	/* an outer loop to handle looping samples */
	while (samples_to_generate > 0)
	{
		samples_to_generate -= mixer_channel_resample_8_pan(channel,mixing_volume,samples_to_generate,&source,(unsigned int)(source_end - source));

		assert( source <= source_end );

		/* handle the end case */
		if (source >= source_end)
		{
			/* if we're done, stop playing */
			if (!channel->is_looping)
			{
				channel->is_playing = 0;
				break;
			}

			/* if we're looping, wrap to the beginning */
			else
				source -= (INT8 *)source_end - (INT8 *)channel->data_start;
		}
	}

	/* update the final positions */
	channel->data_current = source;
}

/***************************************************************************
	mix_sample_16
***************************************************************************/

void mix_sample_16(struct mixer_channel_data *channel, int samples_to_generate)
{
	INT16 *source, *source_end;
	int mixing_volume[2];

	/* compute the overall mixing volume */
	if (mixer_sound_enabled)
	{
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}
	/* get the initial state */
	source = channel->data_current;
	source_end = channel->data_end;

	/* an outer loop to handle looping samples */
	while (samples_to_generate > 0)
	{
		samples_to_generate -= mixer_channel_resample_16_pan(channel,mixing_volume,samples_to_generate,&source,(unsigned int)(source_end - source));

		assert( source <= source_end );

		/* handle the end case */
		if (source >= source_end)
		{
			/* if we're done, stop playing */
			if (!channel->is_looping)
			{
				channel->is_playing = 0;
				break;
			}

			/* if we're looping, wrap to the beginning */
			else
				source -= (INT16 *)source_end - (INT16 *)channel->data_start;
		}
	}

	/* update the final positions */
	channel->data_current = source;
}

/***************************************************************************
	mixer_flush
***************************************************************************/

/* Silence samples */
static INT8 silence_data[FILTER_FLUSH];

/* Flush the state of the filter playing some 0 samples */
static void mixer_flush(struct mixer_channel_data *channel)
{
	INT8 *source_begin, *source_end;
	int mixing_volume[2];
	unsigned save_available;

	mixerlogerror(("Mixer:mixer_flush(%s)\n",channel->name));

	/* filter reset request */
	channel->is_reset_requested = 1;

	/* null volume */
	mixing_volume[0] = 0;
	mixing_volume[1] = 0;

	/* null data */
	source_begin = silence_data;
	source_end = silence_data + FILTER_FLUSH;

	/* save the number of samples availables */
	save_available = channel->samples_available;

	/* mix the silence */
	mixer_channel_resample_8_pan(channel,mixing_volume,ACCUMULATOR_MASK,&source_begin,(unsigned int)(source_end - source_begin));

	/* restore the number of samples availables */
	channel->samples_available = save_available;
}

/***************************************************************************
	mixer_sh_start
***************************************************************************/

int mixer_sh_start(void)
{
	struct mixer_channel_data *channel;
	int i;
	int r;
#ifdef USE_LIBSAMPLERATE
	int error;
#endif
	memset(silence_data, 0, sizeof(silence_data));

	/* reset all channels to their defaults */
	memset(mixer_channel, 0, sizeof(mixer_channel));
	for (i = 0, channel = mixer_channel; i < MIXER_MAX_CHANNELS; i++, channel++)
	{
		channel->mixing_level 					= 0xff;
		channel->default_mixing_level 			= 0xff;
		channel->config_mixing_level 			= config_mixing_level[i];
		channel->config_default_mixing_level 	= config_default_mixing_level[i];

#ifdef USE_LIBSAMPLERATE
		channel->src_left  = src_new((pmoptions.resampling_quality == 0) ? SRC_SINC_FASTEST : SRC_SINC_MEDIUM_QUALITY, 1, &error); //!! if changing quality, change src_sinc_opt again to include the other table (search for //!! there)
		channel->src_right = src_new((pmoptions.resampling_quality == 0) ? SRC_SINC_FASTEST : SRC_SINC_MEDIUM_QUALITY, 1, &error);

		channel->lr_silent_value[0] = INT_MAX;
		channel->lr_silence[0] = 1;
		channel->lr_silent_value[1] = INT_MAX;
		channel->lr_silence[1] = 1;
#else
		channel->left = filter_state_alloc();
		channel->right = filter_state_alloc();
#endif
	}

	/* determine if we're playing in stereo or not */
	first_free_channel = 0;
	is_stereo = ((Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO) != 0);

	/* clear the accumulators */
	accum_base = 0;
	memset(left_accum, 0, sizeof(left_accum));
	memset(right_accum, 0, sizeof(right_accum));

	r = osd_start_audio_stream(is_stereo);
	if (r < 0)
		return -1;

	samples_this_frame = r;

	mixer_sound_enabled = 1;

	return 0;
}


/***************************************************************************
	mixer_sh_stop
***************************************************************************/

void mixer_sh_stop(void)
{
	struct mixer_channel_data *channel;
	int i;

	osd_stop_audio_stream();

	for (i = 0, channel = mixer_channel; i < MIXER_MAX_CHANNELS; i++, channel++)
	{
#ifdef USE_LIBSAMPLERATE
		src_delete(channel->src_left);
		src_delete(channel->src_right);
#else
		if (channel->filter)
			filter_free(channel->filter);
		filter_state_free(channel->left);
		filter_state_free(channel->right);
#endif
	}
}

/***************************************************************************
	mixer_update_channel
***************************************************************************/

void mixer_update_channel(struct mixer_channel_data *channel, int total_sample_count)
{
	int samples_to_generate = total_sample_count - channel->samples_available;

	/* don't do anything for streaming channels */
	if (channel->is_stream)
		return;

	/* if we're all caught up, just return */
	if (samples_to_generate <= 0)
		return;

	/* if we're playing, mix in the data */
	if (channel->is_playing)
	{
		if (channel->is_16bit)
			mix_sample_16(channel, samples_to_generate);
		else
			mix_sample_8(channel, samples_to_generate);

		if (!channel->is_playing)
			mixer_flush(channel);
	}
}

/***************************************************************************
	mixer_sh_update
***************************************************************************/

void mixer_sh_update(void)
{
	struct mixer_channel_data* channel;
	unsigned int accum_pos = accum_base;
	int i;

	profiler_mark(PROFILER_MIXER);

#ifdef MMSND
	WaveDataOutStart();
#endif

	/* update all channels (for streams this is a no-op) */
	for (i = 0, channel = mixer_channel; i < first_free_channel; i++, channel++)
	{
		mixer_update_channel(channel, samples_this_frame);

		/* if we needed more than they could give, adjust their pointers */
		if (samples_this_frame > channel->samples_available)
			channel->samples_available = 0;
		else
			channel->samples_available -= samples_this_frame;
	}

	/* copy the mono 32-bit data to a 16-bit buffer, clipping along the way */
	if (!is_stereo)
	{
		INT16* __restrict mix = mix_buffer;
		for (i = 0; (unsigned int)i < samples_this_frame; i++)
		{
			/* fetch and clip the sample */
#ifdef USE_LIBSAMPLERATE
			INT16 samplei;
#if defined(RESAMPLER_SSE_OPT) && defined(MIXER_USE_CLIPPING)
			samplei = (INT16)_mm_cvtss_si32(_mm_max_ss(_mm_min_ss(_mm_set_ss(left_accum[accum_pos] * 32768.f), _mm_set_ss(32767.f)), _mm_set_ss(-32768.f)));
#else
			const float sample = left_accum[accum_pos] * 32768.f;
#ifdef MIXER_USE_CLIPPING
			if (sample <= -32768.f)
				samplei = -32768;
			else if (sample >= 32767.f)
				samplei = 32767;
			else
#endif
			samplei = (INT16)(lrintf(sample));
#endif
#else
			int samplei = left_accum[accum_pos];
#ifdef MIXER_USE_CLIPPING
			if (samplei < -32768)
				samplei = -32768;
			else if (samplei > 32767)
				samplei = 32767;
#endif
#endif
			/* store and zero out behind us */
			*mix++ = samplei;
			left_accum[accum_pos] = 0;

			/* advance to the next sample */
			accum_pos = (accum_pos + 1) & ACCUMULATOR_MASK;
		}
	}

	/* copy the stereo 32-bit data to a 16-bit buffer, clipping along the way */
	else
	{
		INT16* __restrict mix = mix_buffer;
		for (i = 0; (unsigned int)i < samples_this_frame; i++)
		{
			/* fetch and clip the left sample */
#ifdef USE_LIBSAMPLERATE
			INT16 samplei;
#if defined(RESAMPLER_SSE_OPT) && defined(MIXER_USE_CLIPPING)
			samplei = (INT16)_mm_cvtss_si32(_mm_max_ss(_mm_min_ss(_mm_set_ss(left_accum[accum_pos] * 32768.f), _mm_set_ss(32767.f)), _mm_set_ss(-32768.f)));
#else
			float sample = left_accum[accum_pos] * 32768.f;
#ifdef MIXER_USE_CLIPPING
			if (sample <= -32768.f)
				samplei = -32768;
			else if (sample >= 32767.f)
				samplei = 32767;
			else
#endif
			samplei = (INT16)(lrintf(sample));
#endif
#else
			int samplei = left_accum[accum_pos];
#ifdef MIXER_USE_CLIPPING
			if (samplei < -32768)
				samplei = -32768;
			else if (samplei > 32767)
				samplei = 32767;
#endif
#endif
			/* store and zero out behind us */
			*mix++ = samplei;
			left_accum[accum_pos] = 0;

			/* fetch and clip the right sample */
#ifdef USE_LIBSAMPLERATE
#if defined(RESAMPLER_SSE_OPT) && defined(MIXER_USE_CLIPPING)
			samplei = (INT16)_mm_cvtss_si32(_mm_max_ss(_mm_min_ss(_mm_set_ss(right_accum[accum_pos] * 32768.f), _mm_set_ss(32767.f)), _mm_set_ss(-32768.f)));
#else
			sample = right_accum[accum_pos] * 32768.f;
#ifdef MIXER_USE_CLIPPING
			if (sample <= -32768.f)
				samplei = -32768;
			else if (sample >= 32767.f)
				samplei = 32767;
			else
#endif
			samplei = (INT16)(lrintf(sample));
#endif
#else
			samplei = right_accum[accum_pos];
#ifdef MIXER_USE_CLIPPING
			if (samplei < -32768)
				samplei = -32768;
			else if (samplei > 32767)
				samplei = 32767;
#endif
#endif
			/* store and zero out behind us */
			*mix++ = samplei;
			right_accum[accum_pos] = 0;

			/* advance to the next sample */
			accum_pos = (accum_pos + 1) & ACCUMULATOR_MASK;
		}
	}

	/* play the result */
#ifdef MMSND
	WaveDataOutEnd( mix_buffer, samples_this_frame, is_stereo );
#endif
#ifdef PINMAME
    {
    extern void pm_wave_record(INT16 *buffer, int samples);
    pm_wave_record(mix_buffer, samples_this_frame);
    }
#endif /* PINMAME */
	samples_this_frame = osd_update_audio_stream(mix_buffer);

	accum_base = accum_pos;

	profiler_mark(PROFILER_END);
}


/***************************************************************************
	mixer_allocate_channel
***************************************************************************/

int mixer_allocate_channel(int default_mixing_level)
{
	/* this is just a degenerate case of the multi-channel mixer allocate */
	return mixer_allocate_channels(1, &default_mixing_level);
}


/***************************************************************************
	mixer_allocate_channels
***************************************************************************/

int mixer_allocate_channels(int channels, const int *default_mixing_levels)
{
	int i, j;

	mixerlogerror(("Mixer:mixer_allocate_channels(%d)\n",channels));

	/* make sure we didn't overrun the number of available channels */
	if (first_free_channel + channels > MIXER_MAX_CHANNELS)
	{
		logerror("Too many mixer channels (requested %d, available %d)\n", first_free_channel + channels, MIXER_MAX_CHANNELS);
		exit(1);
	}

	/* loop over channels requested */
	for (i = 0; i < channels; i++)
	{
		struct mixer_channel_data *channel = &mixer_channel[first_free_channel + i];

		/* extract the basic data */
		channel->default_mixing_level 	= MIXER_GET_LEVEL(default_mixing_levels[i]);
		channel->pan 					= MIXER_GET_PAN(default_mixing_levels[i]);
		channel->gain 					= MIXER_GET_GAIN(default_mixing_levels[i]);
		/* add by hiro-shi */
		channel->left_volume			= 100;
		channel->right_volume 			= 100;

		/* backwards compatibility with old 0-255 volume range */
		if (channel->default_mixing_level > 100)
			channel->default_mixing_level = channel->default_mixing_level * 25 / 255;

		/* attempt to load in the configuration data for this channel */
		channel->mixing_level = channel->default_mixing_level;
		if (!is_config_invalid)
		{
			/* if the defaults match, set the mixing level from the config */
			if (channel->default_mixing_level == channel->config_default_mixing_level && channel->config_mixing_level <= 100)
				channel->mixing_level = channel->config_mixing_level;
			/* otherwise, invalidate all channels that have been created so far */
			else
			{
				is_config_invalid = 1;
				for (j = 0; j < first_free_channel + i; j++)
					mixer_set_mixing_level(j, mixer_channel[j].default_mixing_level);
			}
		}

#ifdef USE_LIBSAMPLERATE
		channel->legacy_resample = 1;
#endif

		/* set the default name */
		mixer_set_name(first_free_channel + i, 0);
	}

	/* increment the counter and return the first one */
	first_free_channel += channels;
	return first_free_channel - channels;
}


/***************************************************************************
	mixer_set_name
***************************************************************************/

void mixer_set_name(int ch, const char *name)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	/* either copy the name or create a default one */
	if (name != NULL)
		strcpy(channel->name, name);
	else
		sprintf(channel->name, "<channel #%d>", ch);

	/* append left/right onto the channel as appropriate */
	if (channel->pan == MIXER_PAN_LEFT)
		strcat(channel->name, " (Lt)");
	else if (channel->pan == MIXER_PAN_RIGHT)
		strcat(channel->name, " (Rt)");
}


/***************************************************************************
	mixer_get_name
***************************************************************************/

const char *mixer_get_name(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	/* return a pointer to the name or a NULL for an unused channel */
	if (channel->name[0] != 0)
		return channel->name;
	else
		return NULL;
}


/***************************************************************************
	mixer_set_volume
***************************************************************************/

void mixer_set_volume(int ch, int volume)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	if (channel->left_volume == volume && channel->right_volume == volume)
		return;

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->left_volume  = volume;
	channel->right_volume = volume;
}


/***************************************************************************
	mixer_set_mixing_level
***************************************************************************/

void mixer_set_mixing_level(int ch, int level)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	if (channel->mixing_level == level)
		return;

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->mixing_level = level;
}


/***************************************************************************
	mixer_set_stereo_volume
***************************************************************************/
void mixer_set_stereo_volume(int ch, int l_vol, int r_vol )
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	if (channel->left_volume == l_vol && channel->right_volume == r_vol)
		return;

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->left_volume  = l_vol;
	channel->right_volume = r_vol;
}

/***************************************************************************
	mixer_get_mixing_level
***************************************************************************/

int mixer_get_mixing_level(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	return channel->mixing_level;
}


/***************************************************************************
	mixer_get_default_mixing_level
***************************************************************************/

int mixer_get_default_mixing_level(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	return channel->default_mixing_level;
}


/***************************************************************************
	mixer_load_config
***************************************************************************/

void mixer_load_config(const struct mixer_config *config)
{
	int i;

	for (i = 0; i < MIXER_MAX_CHANNELS; i++)
	{
		config_default_mixing_level[i] = config->default_levels[i];
		config_mixing_level[i] = config->mixing_levels[i];
	}
	is_config_invalid = 0;
}


/***************************************************************************
	mixer_save_config
***************************************************************************/

void mixer_save_config(struct mixer_config *config)
{
	int i;

	for (i = 0; i < MIXER_MAX_CHANNELS; i++)
	{
		config->default_levels[i] = mixer_channel[i].default_mixing_level;
		config->mixing_levels[i] = mixer_channel[i].mixing_level;
	}
}


/***************************************************************************
	mixer_read_config
***************************************************************************/

void mixer_read_config(mame_file *f)
{
	struct mixer_config config;

	if (mame_fread(f, config.default_levels, MIXER_MAX_CHANNELS) < MIXER_MAX_CHANNELS ||
	    mame_fread(f, config.mixing_levels, MIXER_MAX_CHANNELS) < MIXER_MAX_CHANNELS)
	{
		memset(config.default_levels, 0xff, sizeof(config.default_levels));
		memset(config.mixing_levels, 0xff, sizeof(config.mixing_levels));
	}

	mixer_load_config(&config);
}


/***************************************************************************
	mixer_write_config
***************************************************************************/

void mixer_write_config(mame_file *f)
{
	struct mixer_config config;

	mixer_save_config(&config);
	mame_fwrite(f, config.default_levels, MIXER_MAX_CHANNELS);
	mame_fwrite(f, config.mixing_levels, MIXER_MAX_CHANNELS);
}


/***************************************************************************
	mixer_play_streamed_sample_16
***************************************************************************/

void mixer_play_streamed_sample_16(int ch, INT16 *data, int len, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	int mixing_volume[2];

	mixerlogerror(("Mixer:mixer_play_streamed_sample_16(%s,,%d,%d)\n",channel->name,len/2,freq));

	/* skip if sound is off */
	if (Machine->sample_rate == 0)
		return;
	channel->is_stream = 1;

	profiler_mark(PROFILER_MIXER);

	/* compute the overall mixing volume */
	if (mixer_sound_enabled) {
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}

	mixer_channel_resample_set(channel,freq,
#ifndef USE_LIBSAMPLERATE
		channel->request_lowpass_frequency,
#endif
		0);

	/* compute the length in fractional form */
	len = len / 2; /* convert len from byte to word */

	mixer_channel_resample_16_pan(channel,mixing_volume,ACCUMULATOR_MASK,&data,len);

	profiler_mark(PROFILER_END);
}


/***************************************************************************
	mixer_samples_this_frame
***************************************************************************/

int mixer_samples_this_frame(void)
{
	return samples_this_frame;
}


/***************************************************************************
	mixer_need_samples_this_frame
***************************************************************************/
#define EXTRA_SAMPLES 1    // safety margin for sampling rate conversion
int mixer_need_samples_this_frame(int channel,int freq)
{
	if (mixer_channel[channel].samples_available > samples_this_frame) // check if still enough samples around
		return 0;

	return (int)((samples_this_frame - mixer_channel[channel].samples_available)
			* (INT64)freq / Machine->sample_rate + EXTRA_SAMPLES);
}


/***************************************************************************
	mixer_play_sample
***************************************************************************/

void mixer_play_sample(int ch, INT8 *data, int len, int freq, int loop)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_play_sample_8(%s,,%d,%d,%s)\n",channel->name,len,freq,loop ? "loop" : "single"));

	/* skip if sound is off, or if this channel is a stream */
	if (Machine->sample_rate == 0 || channel->is_stream)
		return;

	/* update the state of this channel */
	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	mixer_channel_resample_set(channel,freq,
#ifndef USE_LIBSAMPLERATE
		channel->request_lowpass_frequency,
#endif
		1);

	/* now determine where to mix it */
	channel->data_start = data;
	channel->data_current = data;
	channel->data_end = (UINT8 *)data + len;
	channel->is_playing = 1;
	channel->is_looping = loop;
	channel->is_16bit = 0;
}


/***************************************************************************
	mixer_play_sample_16
***************************************************************************/

void mixer_play_sample_16(int ch, INT16 *data, int len, int freq, int loop)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_play_sample_16(%s,%d,%d,%s)\n",channel->name,len/2,freq,loop ? "loop" : "single"));

	/* skip if sound is off, or if this channel is a stream */
	if (Machine->sample_rate == 0 || channel->is_stream)
		return;

	/* update the state of this channel */
	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	mixer_channel_resample_set(channel,freq,
#ifndef USE_LIBSAMPLERATE
		channel->request_lowpass_frequency,
#endif
		1);

	/* now determine where to mix it */
	channel->data_start = data;
	channel->data_current = data;
	channel->data_end = (UINT8 *)data + len;
	channel->is_playing = 1;
	channel->is_looping = loop;
	channel->is_16bit = 1;
}


/***************************************************************************
	mixer_stop_sample
***************************************************************************/

void mixer_stop_sample(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_stop_sample(%s)\n",channel->name));

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	if (channel->is_playing) {
		channel->is_playing = 0;
		mixer_flush(channel);
	}
}

/***************************************************************************
	mixer_is_sample_playing
***************************************************************************/

int mixer_is_sample_playing(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	return channel->is_playing;
}


/***************************************************************************
	mixer_set_sample_frequency
***************************************************************************/

void mixer_set_sample_frequency(int ch, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	assert( !channel->is_stream );

	if (channel->is_playing) {
		mixerlogerror(("Mixer:mixer_set_sample_frequency(%s,%d)\n",channel->name,freq));

		if (channel->from_frequency == freq)
			return;

		mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

		mixer_channel_resample_set(channel,freq,
#ifndef USE_LIBSAMPLERATE
			channel->request_lowpass_frequency,
#endif
			0);
	}
}

/***************************************************************************
	mixer_set_lowpass_frequency
***************************************************************************/

/* Set the desidered lowpass cut frequency.
This function should be called immeditially after the mixer_allocate() and
before the first play() call. Otherwise the lowpass frequency may be
unused until the next filter recompute.
	ch - channel
	freq - frequency in Hz. Use 0 to disable
*/
void mixer_set_lowpass_frequency(int ch, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	assert(!channel->is_playing && !channel->is_stream);

#ifdef USE_LIBSAMPLERATE
	mixerlogerror(("Mixer:mixer_set_lowpass_frequency(%s,%d) NOT supported anymore\n", channel->name, freq));
#else
	mixerlogerror(("Mixer:mixer_set_lowpass_frequency(%s,%d)\n", channel->name, freq));
	channel->request_lowpass_frequency = freq;
#endif
}

/***************************************************************************
	mixer_sound_enable_global_w
***************************************************************************/

void mixer_sound_enable_global_w(int enable)
{
	int i;
	struct mixer_channel_data *channel;

	if (mixer_sound_enabled == enable)
		return;

	/* update all channels (for streams this is a no-op) */
	for (i = 0, channel = mixer_channel; i < first_free_channel; i++, channel++)
	{
		mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	}

	mixer_sound_enabled = enable;
}

#ifdef PINMAME
void mixer_set_reverb_filter(int ch, float delay, float force)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	channel->reverbPos[0] = channel->reverbPos[1] = 0;
	channel->reverbDelay[0] = channel->reverbDelay[1] = delay;
	channel->reverbForce[0] = channel->reverbForce[1] = force;
	memset(channel->reverbBuffer, 0, sizeof(channel->reverbBuffer[0][0])*2*REVERB_LENGTH);
}
#endif

#ifdef USE_LIBSAMPLERATE
void mixer_set_channel_legacy_resample(int ch, int enable)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	channel->legacy_resample = enable;
}
#else
void mixer_set_channel_legacy_resample(int ch, int enable)
{
}
#endif
