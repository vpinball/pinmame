/***************************************************************************

  streams.c

  Handle general purpose audio streams

***************************************************************************/

#include "driver.h"
#include <math.h>
#include <assert.h>

#define BUFFER_LEN 16384

#define SAMPLES_THIS_FRAME(channel) \
	mixer_need_samples_this_frame((channel),stream_sample_rate[(channel)])

static int stream_joined_channels[MIXER_MAX_CHANNELS];
static INT16 *stream_buffer[MIXER_MAX_CHANNELS];
static int stream_sample_rate[MIXER_MAX_CHANNELS];
static int stream_buffer_pos[MIXER_MAX_CHANNELS];
static int stream_sample_length[MIXER_MAX_CHANNELS];	/* in usec */
static int stream_param[MIXER_MAX_CHANNELS];
static void (*stream_callback[MIXER_MAX_CHANNELS])(int param,INT16 *buffer,int length);
static void (*stream_callback_multi[MIXER_MAX_CHANNELS])(int param,INT16 **buffer,int length);

static int memory[MIXER_MAX_CHANNELS];
static int K[MIXER_MAX_CHANNELS];
static double K_tmp[MIXER_MAX_CHANNELS];

/*
signal >--R1--+--R2--+
              |      |
              C      R3---> amp
              |      |
             GND    GND
*/

/* R1, R2, R3 in Ohm; C in pF */
/* set C = 0 to disable the filter */
void set_RC_filter(int channel, int R1, int R2, int R3, int C, int sample_rate)
{
	double Req;

	memory[channel] = 0;

	if (C == 0)
	{
		K[channel] = 0x10000;
		K_tmp[channel] = 0.;

		return;
	}

	/* Cut Frequency = 1/(2*Pi*Req*C) */

	Req = (R2 + R3 > 0) ? ((long long)R1*(long long)(R2 + R3)) / (double)(R1 + R2 + R3) : R1;

	K_tmp[channel] = Req * (C * 1E-12); /* 1E-12: convert pF to F */

	K[channel] = 0x10000 - (int)(0x10000 * exp(-1. / (K_tmp[channel] * sample_rate)));
}

static void set_RC_filter_sample_rate(int channel, int sample_rate)
{
	if (K_tmp[channel] == 0.)
		return;

	K[channel] = (int)(0x10000 - 0x10000 * exp(-1. / (K_tmp[channel] * sample_rate)));
}

static void apply_RC_filter(int channel,INT16 *buf,int len)
{
	int i;

	if (K[channel] == 0x10000) return;	/* filter disabled */

	for (i = 0; i < len; i++)
	{
		memory[channel] += (buf[i] - memory[channel]) * K[channel] / 0x10000;
		buf[i] = memory[channel];
	}
}

int streams_sh_start(void)
{
	int i;

	for (i = 0;i < MIXER_MAX_CHANNELS;i++)
	{
		stream_joined_channels[i] = 1;
		stream_buffer[i] = 0;

		K[i] = 0x10000;
		K_tmp[i] = 0.;
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
	int channel,i;

	if (Machine->sample_rate == 0) return;

	/* update all the output buffers */
	for (channel = 0;channel < MIXER_MAX_CHANNELS;channel += stream_joined_channels[channel])
	{
		if (stream_buffer[channel])
		{
			int newpos;
			int buflen;

			newpos = SAMPLES_THIS_FRAME(channel);

			buflen = newpos - stream_buffer_pos[channel];

			assert(buflen + stream_buffer_pos[channel] < BUFFER_LEN);

			if (buflen + stream_buffer_pos[channel] >= BUFFER_LEN)
				buflen = BUFFER_LEN-1 - stream_buffer_pos[channel];

			if (stream_joined_channels[channel] > 1)
			{
				INT16 *buf[MIXER_MAX_CHANNELS];

				if (buflen > 0)
				{
					for (i = 0;i < stream_joined_channels[channel];i++)
					{
						assert(buflen + stream_buffer_pos[channel+i] < BUFFER_LEN);

						buf[i] = stream_buffer[channel+i] + stream_buffer_pos[channel+i];
					}

					(*stream_callback_multi[channel])(stream_param[channel],buf,buflen);
				}

				for (i = 0;i < stream_joined_channels[channel];i++)
					stream_buffer_pos[channel+i] = 0;

				for (i = 0;i < stream_joined_channels[channel];i++)
					apply_RC_filter(channel+i,stream_buffer[channel+i],buflen);
			}
			else
			{
				if (buflen > 0)
				{
					INT16 *buf = stream_buffer[channel] + stream_buffer_pos[channel];

					(*stream_callback[channel])(stream_param[channel],buf,buflen);
				}

				stream_buffer_pos[channel] = 0;

				apply_RC_filter(channel,stream_buffer[channel],buflen);
			}
		}
	}

	for (channel = 0;channel < MIXER_MAX_CHANNELS;channel += stream_joined_channels[channel])
	{
		if (stream_buffer[channel])
		{
			for (i = 0;i < stream_joined_channels[channel];i++)
				mixer_play_streamed_sample_16(channel+i,
						stream_buffer[channel+i],sizeof(INT16)*SAMPLES_THIS_FRAME(channel+i),
						stream_sample_rate[channel]);
		}
	}
}

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length))
{
	int channel;

	channel = mixer_allocate_channel(default_mixing_level);

	stream_joined_channels[channel] = 1;

	mixer_set_channel_legacy_resample(channel,0);

	mixer_set_name(channel,name);

	if ((stream_buffer[channel] = malloc(sizeof(INT16)*BUFFER_LEN)) == 0)
		return -1;

	stream_sample_rate[channel] = sample_rate;
	stream_buffer_pos[channel] = 0;
	if (sample_rate)
		stream_sample_length[channel] = 1000000 / sample_rate;
	else
		stream_sample_length[channel] = 0;
	stream_param[channel] = param;
	stream_callback[channel] = callback;
	set_RC_filter(channel,0,0,0,0,sample_rate);

	return channel;
}

#ifdef PINMAME
void stream_set_sample_rate(int channel, int sample_rate) {
	if (stream_sample_rate[channel] == sample_rate)
		return;

	stream_sample_rate[channel] = sample_rate;
	if (sample_rate)
		stream_sample_length[channel] = 1000000 / sample_rate;
	else
		stream_sample_length[channel] = 0;
 	if (stream_buffer_pos[channel] >= stream_sample_length[channel]) {
		stream_buffer_pos[channel] = 0;
	}

	set_RC_filter_sample_rate(channel,sample_rate);
}

int stream_get_sample_rate(int channel) {
	return stream_sample_rate[channel];
}

void stream_free(int channel) {
	free(stream_buffer[channel]);
	stream_buffer[channel] = 0;
}
#endif /* PINMAME */

int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length))
{
	int channel,i;

	channel = mixer_allocate_channels(channels,default_mixing_levels);

	stream_joined_channels[channel] = channels;

	for (i = 0;i < channels;i++)
	{
		mixer_set_channel_legacy_resample(channel+i,0);

		mixer_set_name(channel+i,names[i]);

		if ((stream_buffer[channel+i] = malloc(sizeof(INT16)*BUFFER_LEN)) == 0)
			return -1;

		stream_sample_rate[channel+i] = sample_rate;
		stream_buffer_pos[channel+i] = 0;
		if (sample_rate)
			stream_sample_length[channel+i] = 1000000 / sample_rate;
		else
			stream_sample_length[channel+i] = 0;

		set_RC_filter(channel+i,0,0,0,0,sample_rate);
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
			INT16 *buf[MIXER_MAX_CHANNELS];
			int i;

			for (i = 0;i < stream_joined_channels[channel];i++)
				buf[i] = stream_buffer[channel+i] + stream_buffer_pos[channel+i];

			profiler_mark(PROFILER_SOUND);
			(*stream_callback_multi[channel])(stream_param[channel],buf,buflen);
			profiler_mark(PROFILER_END);

			for (i = 0;i < stream_joined_channels[channel];i++)
				stream_buffer_pos[channel+i] += buflen;
		}
		else
		{
			INT16 *buf = stream_buffer[channel] + stream_buffer_pos[channel];

			profiler_mark(PROFILER_SOUND);
			(*stream_callback[channel])(stream_param[channel],buf,buflen);
			profiler_mark(PROFILER_END);

			stream_buffer_pos[channel] += buflen;
		}
	}
}
