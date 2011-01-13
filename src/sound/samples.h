#ifndef SAMPLES_H
#define SAMPLES_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct Samplesinterface
{
	int channels;	/* number of discrete audio channels needed */
	int volume;		/* global volume for all samples */
	const char **samplenames;
#ifdef PINMAME
	const char *prefix;
#endif
};


/* Start one of the samples loaded from disk. Note: channel must be in the range */
/* 0 .. Samplesinterface->channels-1. It is NOT the discrete channel to pass to */
/* mixer_play_sample() */
void sample_start(int channel,int samplenum,int loop);
void sample_set_freq(int channel,int freq);
void sample_set_volume(int channel,int volume);
void sample_stop(int channel);
int sample_playing(int channel);

int samples_sh_start(const struct MachineSound *msound);
#ifdef PINMAME
void samples_sh_stop(void);
#endif

#endif
