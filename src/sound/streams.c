// license:BSD-3-Clause

/***************************************************************************

  streams.c

  Handle general purpose audio streams

***************************************************************************/

#include "driver.h"
#include <assert.h>
#include <stdbool.h>

#define BUFFER_LEN 16384

#define SAMPLES_THIS_FRAME(channel) \
	mixer_need_samples_this_frame((channel),stream_sample_rate[(channel)])

static int stream_joined_channels[MIXER_MAX_CHANNELS];
static void *stream_buffer[MIXER_MAX_CHANNELS];
static double stream_sample_rate[MIXER_MAX_CHANNELS];
static int stream_buffer_pos[MIXER_MAX_CHANNELS];
static int stream_sample_length[MIXER_MAX_CHANNELS];	/* in usec */
static int stream_param[MIXER_MAX_CHANNELS];
static bool stream_is_float[MIXER_MAX_CHANNELS];
static void (*stream_callback[MIXER_MAX_CHANNELS])(int param,INT16 *buffer,int length);
static void (*stream_callback_multi[MIXER_MAX_CHANNELS])(int param,INT16 **buffer,int length);

/* last sample handed to the mixer on each channel, to pad a joined channel that needs more than was rendered */
static float stream_last_f[MIXER_MAX_CHANNELS];
static INT16 stream_last_16[MIXER_MAX_CHANNELS];

static void stream_keep_last(int channel, int len)
{
	if (stream_is_float[channel])
		stream_last_f[channel] = ((const float*)stream_buffer[channel])[len-1];
	else
		stream_last_16[channel] = ((const INT16*)stream_buffer[channel])[len-1];
}

static void stream_pad(int channel, int from, int to)
{
	int i;
	if (stream_is_float[channel])
	{
		float * const buf = (float*)stream_buffer[channel];
		const float v = from > 0 ? buf[from-1] : stream_last_f[channel];
		for (i = from; i < to; i++)
			buf[i] = v;
	}
	else
	{
		INT16 * const buf = (INT16*)stream_buffer[channel];
		const INT16 v = from > 0 ? buf[from-1] : stream_last_16[channel];
		for (i = from; i < to; i++)
			buf[i] = v;
	}
}

int streams_sh_start(void)
{
	int i;
	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		stream_joined_channels[i] = 1;
		stream_buffer[i] = 0;
		stream_last_f[i] = 0.f;
		stream_last_16[i] = 0;
	}

	return 0;
}


void streams_sh_stop(void)
{
	int i;
	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		if(stream_buffer[i])
			free(stream_buffer[i]);
		stream_buffer[i] = 0;
	}
}


void streams_sh_update(void)
{
	int channel;
	int frame_len[MIXER_MAX_CHANNELS];	/* samples rendered this frame, for each stream */
	int stream_need[MIXER_MAX_CHANNELS];	/* samples each joined channel needs this frame */
	int stream_resampled[MIXER_MAX_CHANNELS];	/* the mixer resamples this joined channel exactly */

	if (Machine->sample_rate == 0) return;

	/* update all the output buffers */
	for (channel = 0;channel < MIXER_MAX_CHANNELS;channel += stream_joined_channels[channel])
	{
		if (stream_buffer[channel])
		{
			int newpos = SAMPLES_THIS_FRAME(channel);
			int buflen;

			/* the joined channels of a stream are resampled separately by the mixer and can need a few
			   more or less samples than the first one: a channel that is silent so far (or at zero volume)
			   takes the mixer's stepping path, which has no latency and takes a slightly different number of
			   samples over time than libsamplerate. So render what the most demanding resampled channel needs
			   and give all of it to the resampled channels (they all take samples at the same rate, so what
			   they get ahead stays bounded), while the others only take what they need of it (below) */
			if (stream_joined_channels[channel] > 1)
			{
				int i, found = 0;
				for (i = 0;i < stream_joined_channels[channel];i++)
				{
					stream_need[channel+i] = SAMPLES_THIS_FRAME(channel+i);
					stream_resampled[channel+i] = mixer_channel_is_resampled(channel+i);
					if (stream_resampled[channel+i] && (!found || stream_need[channel+i] > newpos))
					{
						newpos = stream_need[channel+i];
						found = 1;
					}
				}
			}
			buflen = newpos - stream_buffer_pos[channel];

			assert(buflen + stream_buffer_pos[channel] < BUFFER_LEN);

			if (buflen + stream_buffer_pos[channel] >= BUFFER_LEN)
				buflen = BUFFER_LEN-1 - stream_buffer_pos[channel];

			if (stream_joined_channels[channel] > 1)
			{
				int i;
				if (buflen > 0)
				{
					const void *buf[MIXER_MAX_CHANNELS];

					for (i = 0;i < stream_joined_channels[channel];i++)
					{
						assert(buflen + stream_buffer_pos[channel+i] < BUFFER_LEN);

						buf[i] = (UINT8*)(stream_buffer[channel+i]) + stream_buffer_pos[channel+i]*(stream_is_float[channel+i] ? sizeof(float) : sizeof(INT16));
					}

					(*stream_callback_multi[channel])(stream_param[channel],(INT16**)buf,buflen);
				}

				frame_len[channel] = stream_buffer_pos[channel] + (buflen > 0 ? buflen : 0);
				for (i = 0;i < stream_joined_channels[channel];i++)
					stream_buffer_pos[channel+i] = 0;
			}
			else
			{
				if (buflen > 0)
				{
					void *buf = (UINT8*)(stream_buffer[channel]) + stream_buffer_pos[channel] * (stream_is_float[channel] ? sizeof(float) : sizeof(INT16));

					(*stream_callback[channel])(stream_param[channel],buf,buflen);
				}

				stream_buffer_pos[channel] = 0;
			}
		}
	}

	for (channel = 0;channel < MIXER_MAX_CHANNELS;channel += stream_joined_channels[channel])
	{
		if (stream_buffer[channel])
		{
			int i;
			if (stream_joined_channels[channel] > 1)
			{
				for (i = 0;i < stream_joined_channels[channel];i++)
				{
					int len = frame_len[channel];
					if (!stream_resampled[channel+i])
					{
						/* silent so far or muted: hand it what it needs, repeating its last sample if
						   the frame is short, so it doesn't drift away from the others */
						len = stream_need[channel+i];
						if (len > BUFFER_LEN-1)
							len = BUFFER_LEN-1;
						if (len > frame_len[channel])
							stream_pad(channel+i, frame_len[channel], len);
					}
					if (len > 0)
						stream_keep_last(channel+i, len);
					mixer_play_streamed_sample_16(channel+i,
							stream_buffer[channel+i],len,
							stream_sample_rate[channel]);
				}
			}
			else
				mixer_play_streamed_sample_16(channel,
						stream_buffer[channel],SAMPLES_THIS_FRAME(channel),
						stream_sample_rate[channel]);
		}
	}
}

int stream_init(const char *name,int default_mixing_level,
		double sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length))
{
	return stream_init_float(name, default_mixing_level,
		sample_rate,
		param, callback, 0);
}

int stream_init_float(const char *name,int default_mixing_level,
		double sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length),int is_float)
{
	const int channel = mixer_allocate_channel_float(default_mixing_level,is_float);

	stream_joined_channels[channel] = 1;

	mixer_set_channel_legacy_resample(channel,0);

	mixer_set_name(channel,name);

	if ((stream_buffer[channel] = malloc((is_float ? sizeof(float) : sizeof(INT16))*BUFFER_LEN)) == 0)
		return -1;

	stream_is_float[channel] = is_float;

	stream_sample_rate[channel] = sample_rate;
	stream_buffer_pos[channel] = 0;
	if (sample_rate != 0.)
		stream_sample_length[channel] = (int)(1000000. / sample_rate + 0.5);
	else
		stream_sample_length[channel] = 0;
	stream_param[channel] = param;
	stream_callback[channel] = callback;

	return channel;
}

void stream_set_sample_rate(int channel, double sample_rate)
{
	if (stream_sample_rate[channel] == sample_rate)
		return;

	stream_sample_rate[channel] = sample_rate;
	if (sample_rate != 0.)
		stream_sample_length[channel] = (int)(1000000. / sample_rate + 0.5);
	else
		stream_sample_length[channel] = 0;
	if (stream_buffer_pos[channel] >= stream_sample_length[channel])
		stream_buffer_pos[channel] = 0;
}

double stream_get_sample_rate(int channel)
{
	return stream_sample_rate[channel];
}

void stream_free(int channel)
{
	if(stream_buffer[channel])
		free(stream_buffer[channel]);
	stream_buffer[channel] = 0;
}

int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		double sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length))
{
	return stream_init_multi_float(channels, names, default_mixing_levels,
		sample_rate,
		param, callback, 0);
}

int stream_init_multi_float(int channels,const char **names,const int *default_mixing_levels,
		double sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length),int is_float)
{
	int i;
	const int channel = mixer_allocate_channels_float(channels,default_mixing_levels,is_float);

	stream_joined_channels[channel] = channels;

	for (i = 0;i < channels;i++)
	{
		mixer_set_channel_legacy_resample(channel+i,0);

		mixer_set_name(channel+i,names[i]);

		if ((stream_buffer[channel+i] = malloc((is_float ? sizeof(float) : sizeof(INT16))*BUFFER_LEN)) == 0)
			return -1;

		stream_is_float[channel+i] = is_float;

		stream_sample_rate[channel+i] = sample_rate;
		stream_buffer_pos[channel+i] = 0;
		if (sample_rate != 0.)
			stream_sample_length[channel+i] = (int)(1000000. / sample_rate + 0.5);
		else
			stream_sample_length[channel+i] = 0;
	}

	stream_param[channel] = param;
	stream_callback_multi[channel] = callback;

	return channel;
}


/* min_interval is in usec */
void stream_update(int channel,int min_interval)
{
	int newpos;
	int buflen;

	if (Machine->sample_rate == 0 || stream_buffer[channel] == 0)
		return;

	/* get current position based on the timer */
	newpos = sound_scalebufferpos(SAMPLES_THIS_FRAME(channel));

	buflen = newpos - stream_buffer_pos[channel];

	if (buflen * stream_sample_length[channel] > min_interval)
	{
		if (stream_joined_channels[channel] > 1)
		{
			const void *buf[MIXER_MAX_CHANNELS];
			int i;
			for (i = 0;i < stream_joined_channels[channel];i++)
				buf[i] = (UINT8*)(stream_buffer[channel+i]) + stream_buffer_pos[channel+i] * (stream_is_float[channel+i] ? sizeof(float) : sizeof(INT16));

			profiler_mark(PROFILER_SOUND);
			(*stream_callback_multi[channel])(stream_param[channel],(INT16**)buf,buflen);
			profiler_mark(PROFILER_END);

			for (i = 0;i < stream_joined_channels[channel];i++)
				stream_buffer_pos[channel+i] += buflen;
		}
		else
		{
			void *buf = (UINT8*)(stream_buffer[channel]) + stream_buffer_pos[channel] * (stream_is_float[channel] ? sizeof(float) : sizeof(INT16));

			profiler_mark(PROFILER_SOUND);
			(*stream_callback[channel])(stream_param[channel],buf,buflen);
			profiler_mark(PROFILER_END);

			stream_buffer_pos[channel] += buflen;
		}
	}
}
