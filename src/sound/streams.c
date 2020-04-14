// license:BSD-3-Clause

/***************************************************************************

  streams.c

  Handle general purpose audio streams

***************************************************************************/

#include "driver.h"
#include <assert.h>

#define BUFFER_LEN 16384

#define SAMPLES_THIS_FRAME(channel) \
	mixer_need_samples_this_frame((channel),stream_sample_rate[(channel)])

static int stream_joined_channels[MIXER_MAX_CHANNELS];
static void *stream_buffer[MIXER_MAX_CHANNELS];
static int stream_sample_rate[MIXER_MAX_CHANNELS];
static int stream_buffer_pos[MIXER_MAX_CHANNELS];
static int stream_sample_length[MIXER_MAX_CHANNELS];	/* in usec */
static int stream_param[MIXER_MAX_CHANNELS];
static UINT8 stream_is_float[MIXER_MAX_CHANNELS];
static void (*stream_callback[MIXER_MAX_CHANNELS])(int param,INT16 *buffer,int length);
static void (*stream_callback_multi[MIXER_MAX_CHANNELS])(int param,INT16 **buffer,int length);

int streams_sh_start(void)
{
	int i;
	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		stream_joined_channels[i] = 1;
		stream_buffer[i] = 0;
	}

	return 0;
}


void streams_sh_stop(void)
{
	int i;
	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		free(stream_buffer[i]);
		stream_buffer[i] = 0;
	}
}


void streams_sh_update(void)
{
	int channel;

	if (Machine->sample_rate == 0) return;

	/* update all the output buffers */
	for (channel = 0;channel < MIXER_MAX_CHANNELS;channel += stream_joined_channels[channel])
	{
		if (stream_buffer[channel])
		{
			const int newpos = SAMPLES_THIS_FRAME(channel);
			int buflen = newpos - stream_buffer_pos[channel];

			assert(buflen + stream_buffer_pos[channel] < BUFFER_LEN);

			if (buflen + stream_buffer_pos[channel] >= BUFFER_LEN)
				buflen = BUFFER_LEN-1 - stream_buffer_pos[channel];

			if (stream_joined_channels[channel] > 1)
			{
				int i;
				if (buflen > 0)
				{
					void *buf[MIXER_MAX_CHANNELS];

					for (i = 0;i < stream_joined_channels[channel];i++)
					{
						assert(buflen + stream_buffer_pos[channel+i] < BUFFER_LEN);

						buf[i] = (UINT8*)(stream_buffer[channel+i]) + stream_buffer_pos[channel+i]*(stream_is_float[channel+i] ? sizeof(float) : sizeof(INT16));
					}

					(*stream_callback_multi[channel])(stream_param[channel],buf,buflen);
				}

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
			for (i = 0;i < stream_joined_channels[channel];i++)
				mixer_play_streamed_sample_16(channel+i,
						stream_buffer[channel+i],SAMPLES_THIS_FRAME(channel+i),
						stream_sample_rate[channel]);
		}
	}
}

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length))
{
	return stream_init_float(name, default_mixing_level,
		sample_rate,
		param, callback, 0);
}

int stream_init_float(const char *name,int default_mixing_level,
		int sample_rate,
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
	if (sample_rate)
		stream_sample_length[channel] = 1000000 / sample_rate;
	else
		stream_sample_length[channel] = 0;
	stream_param[channel] = param;
	stream_callback[channel] = callback;

	return channel;
}

void stream_set_sample_rate(int channel, int sample_rate)
{
	if (stream_sample_rate[channel] == sample_rate)
		return;

	stream_sample_rate[channel] = sample_rate;
	if (sample_rate)
		stream_sample_length[channel] = 1000000 / sample_rate;
	else
		stream_sample_length[channel] = 0;
	if (stream_buffer_pos[channel] >= stream_sample_length[channel])
		stream_buffer_pos[channel] = 0;
}

int stream_get_sample_rate(int channel)
{
	return stream_sample_rate[channel];
}

void stream_free(int channel)
{
	free(stream_buffer[channel]);
	stream_buffer[channel] = 0;
}

int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length))
{
	int i;
	const int channel = mixer_allocate_channels(channels,default_mixing_levels);

	stream_joined_channels[channel] = channels;

	for (i = 0;i < channels;i++)
	{
		mixer_set_channel_legacy_resample(channel+i,0);

		mixer_set_name(channel+i,names[i]);

		if ((stream_buffer[channel+i] = malloc(sizeof(INT16)*BUFFER_LEN)) == 0)
			return -1;

		stream_is_float[channel] = 0;

		stream_sample_rate[channel+i] = sample_rate;
		stream_buffer_pos[channel+i] = 0;
		if (sample_rate)
			stream_sample_length[channel+i] = 1000000 / sample_rate;
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
			void *buf[MIXER_MAX_CHANNELS];
			int i;
			for (i = 0;i < stream_joined_channels[channel];i++)
				buf[i] = (UINT8*)(stream_buffer[channel+i]) + stream_buffer_pos[channel+i] * (stream_is_float[channel+i] ? sizeof(float) : sizeof(INT16));

			profiler_mark(PROFILER_SOUND);
			(*stream_callback_multi[channel])(stream_param[channel],buf,buflen);
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
