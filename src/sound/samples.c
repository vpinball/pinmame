#include "driver.h"



static int firstchannel,numchannels;


/* Start one of the samples loaded from disk. Note: channel must be in the range */
/* 0 .. Samplesinterface->channels-1. It is NOT the discrete channel to pass to */
/* mixer_play_sample() */
void sample_start(int channel,int samplenum,int loop)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		logerror("error: sample_start() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}
	if (samplenum >= Machine->samples->total)
	{
		logerror("error: sample_start() called with samplenum = %d, but only %d samples available\n",samplenum,Machine->samples->total);
		return;
	}
	if (Machine->samples->sample[samplenum] == 0) return;

	if ( Machine->samples->sample[samplenum]->resolution == 8 )
	{
		logerror("play 8 bit sample %d, channel %d\n",samplenum,channel);
		mixer_play_sample(firstchannel + channel,
				Machine->samples->sample[samplenum]->data,
				Machine->samples->sample[samplenum]->length,
				Machine->samples->sample[samplenum]->smpfreq,
				loop);
	}
	else
	{
		logerror("play 16 bit sample %d, channel %d\n",samplenum,channel);
		mixer_play_sample_16(firstchannel + channel,
				(short *) Machine->samples->sample[samplenum]->data,
				Machine->samples->sample[samplenum]->length,
				Machine->samples->sample[samplenum]->smpfreq,
				loop);
	}
}

void sample_set_freq(int channel,int freq)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		logerror("error: sample_set_freq() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_sample_frequency(channel + firstchannel,freq);
}

void sample_set_volume(int channel,int volume)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		logerror("error: sample_set_volume() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_volume(channel + firstchannel,volume * 100 / 255);
}

void sample_stop(int channel)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		logerror("error: sample_stop() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_stop_sample(channel + firstchannel);
}

int sample_playing(int channel)
{
	if (Machine->sample_rate == 0) return 0;
	if (Machine->samples == 0) return 0;
	if (channel >= numchannels)
	{
		logerror("error: sample_playing() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return 0;
	}

	return mixer_is_sample_playing(channel + firstchannel);
}

#ifdef PINMAME

/* UPDATED gv 10/12/04:
   Now you can add more that one SAMPLES interface to the game driver.
   As the maximum number of samples is kept in static variables,
   and not organized in sound interface definitions as used on other
   sound hardware, it's necessary to always call samples_sh_stop() on reset!
*/
#define MAX_SAMPLES 4
static int hi_samples;
static struct GameSamples *game_samples[MAX_SAMPLES];

int samples_sh_start(const struct MachineSound *msound)
{
	int hi_sample = 0;
	int i, channelno;
	int vol[MIXER_MAX_CHANNELS];
	const struct Samplesinterface *intf = msound->sound_interface;

	if (hi_samples >= MAX_SAMPLES)
	{
		logerror("error: too many sample sounds. Only %d allowed.\n",MAX_SAMPLES);
		return 1;
	}
	/* read audio samples if available */
	game_samples[hi_samples] = readsamples(intf->samplenames,Machine->gamedrv->name);
	if (!game_samples[hi_samples]) return 0;

	/* alloc memory for all samples read so far */
	Machine->samples = auto_malloc(hi_samples * sizeof(struct GameSamples)
	 + ((hi_samples ? Machine->samples->total : 0) + game_samples[hi_samples]->total) * sizeof(struct GameSample));
	hi_samples++;
	Machine->samples->total = 0;
	for (i=0; i < hi_samples; i++) {
		int j;
		Machine->samples->total += game_samples[i]->total;
		for (j=0; j < game_samples[i]->total; j++)
			Machine->samples->sample[hi_sample++] = game_samples[i]->sample[j];
	}

	for (i = 0; i < intf->channels; i++)
		vol[i] = intf->volume;
	channelno = mixer_allocate_channels(intf->channels,vol);
	if (!firstchannel) firstchannel = channelno;
	for (i = 0; i < intf->channels; i++)
	{
		char buf[40];
		/* set a name for the sample channel if available */
		if (intf->prefix) sprintf(buf, "%s #%d", intf->prefix, i);
		else sprintf(buf, "Sample #%d", numchannels + i);
		mixer_set_name(firstchannel + numchannels + i,buf);
	}
	numchannels += intf->channels;
	return 0;
}

void samples_sh_stop(void) {
  firstchannel = 0;
  numchannels = 0;
  hi_samples = 0;
}

#else /*PINMAME*/

int samples_sh_start(const struct MachineSound *msound)
{
	int i;
	int vol[MIXER_MAX_CHANNELS];
	const struct Samplesinterface *intf = msound->sound_interface;

	/* read audio samples if available */
	Machine->samples = readsamples(intf->samplenames,Machine->gamedrv->name);

	numchannels = intf->channels;
	for (i = 0;i < numchannels;i++)
		vol[i] = intf->volume;
	firstchannel = mixer_allocate_channels(numchannels,vol);
	for (i = 0;i < numchannels;i++)
	{
		char buf[40];

		sprintf(buf,"Sample #%d",i);
		mixer_set_name(firstchannel + i,buf);
	}
	return 0;
}

#endif
