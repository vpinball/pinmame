// license:BSD-3-Clause

#ifndef PINHECK_AUDIO_H
#define PINHECK_AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PIN_L 15
#define AUDIO_PIN_R 14
#define AUDIO_POINTS 4096

typedef void (*audio_log_fn)(void *ctx, const char *msg);

typedef struct audio_point { uint64_t t; double level; } audio_point;

typedef struct audio_chan {
	audio_point pt[AUDIO_POINTS];
	int head, count;
	double level, x1, y1;
	unsigned logged;
} audio_chan;

typedef struct audio {
	uint32_t ctr[8][2], frq[8][2];
	uint32_t out, dir;
	uint64_t last_t, t_render;
	double r;
	audio_chan ch[2];
	audio_log_fn log;
	void *log_ctx;
} audio;

void audio_init(audio *a, double sample_rate, audio_log_fn log, void *ctx);
void audio_reset(audio *a, uint64_t t);
void audio_ctr(audio *a, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq);
void audio_pins(audio *a, uint64_t t, uint32_t out, uint32_t dir);
double audio_level(audio *a, int ch, uint64_t t0, uint64_t t1);
void audio_render(audio *a, int16_t *out_stereo, int n, uint64_t t_end);

#ifdef __cplusplus
}
#endif

#endif
