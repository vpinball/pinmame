#ifndef STREAMS_H
#define STREAMS_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

void set_RC_filter(int channel,int R1,int R2,int R3,int C,int sample_rate);

int streams_sh_start(void);
void streams_sh_stop(void);
void streams_sh_update(void);

int stream_init(const char *name,int default_mixing_level,
		int sample_rate,
		int param,void (*callback)(int param,INT16 *buffer,int length));
int stream_init_multi(int channels,const char **names,const int *default_mixing_levels,
		int sample_rate,
		int param,void (*callback)(int param,INT16 **buffer,int length));
void stream_update(int channel,int min_interval);	/* min_interval is in usec */

#ifdef PINMAME
void stream_set_sample_rate(int channel, int sample_rate);
int stream_get_sample_rate(int channel);
#endif /* PINMAME */

#ifdef __cplusplus
}
#endif

#endif
