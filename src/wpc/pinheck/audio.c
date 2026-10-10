// license:BSD-3-Clause

#include "audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MODE(c)  (((c) >> 26) & 31)
#define APIN(c)  ((c) & 31)
#define BPIN(c)  (((c) >> 9) & 31)
#define DIFF(m)  ((m) == 5 || (m) == 7)
#define DUTY1    6
#define LOG_MODE 1
#define LOG_TWO  2
#define LOG_FULL 4

static const int chan_pin[2] = { AUDIO_PIN_L, AUDIO_PIN_R };

/* callers format the message only when this is true: pin_level runs at every counter write */
static int log_due(const audio * const a, int ch, unsigned bit) { return !(a->ch[ch].logged & bit); }

static void log_once(audio * const a, int ch, unsigned bit, const char *msg)
{
	if (a->ch[ch].logged & bit) return;
	a->ch[ch].logged |= bit;
	if (a->log) a->log(a->log_ctx, msg);
}

static double pin_level(audio * const a, int ch)
{
	int pin = chan_pin[ch], n, k, drivers = 0;
	double sum = 0.0;
	char msg[64];
	if (!((a->dir >> pin) & 1)) return 0.0;
	if ((a->out >> pin) & 1) return 1.0;
	for (n = 0; n < 8; n++)
		for (k = 0; k < 2; k++) {
			uint32_t c = a->ctr[n][k];
			if (MODE(c) == 0 || (APIN(c) != (uint32_t)pin && !(DIFF(MODE(c)) && BPIN(c) == (uint32_t)pin))) continue;
			if (MODE(c) != DUTY1 || APIN(c) != (uint32_t)pin) {
				if (log_due(a, ch, LOG_MODE)) {
					sprintf(msg, "audio: counter mode %u on P%d not modelled, silence", (unsigned)MODE(c), pin);
					log_once(a, ch, LOG_MODE, msg);
				}
				return 0.0;
			}
			sum += a->frq[n][k] / 4294967296.0;
			drivers++;
		}
	if (drivers > 1 && log_due(a, ch, LOG_TWO)) {
		sprintf(msg, "audio: %d counters drive P%d, combined as OR", drivers, pin);
		log_once(a, ch, LOG_TWO, msg);
	}
	return sum > 1.0 ? 1.0 : sum;
}

static int on_audio_pin(uint32_t c)
{
	uint32_t m = MODE(c);
	if (!m) return 0;
	if (APIN(c) == AUDIO_PIN_L || APIN(c) == AUDIO_PIN_R) return 1;
	return DIFF(m) && (BPIN(c) == AUDIO_PIN_L || BPIN(c) == AUDIO_PIN_R);
}

static void push(audio * const a, int ch, uint64_t t, double level)
{
	audio_chan * const c = &a->ch[ch];
	audio_point *last;
	if (c->count) {
		last = &c->pt[(c->head + c->count - 1) % AUDIO_POINTS];
		if (last->level == level) return;
		if (last->t == t) { last->level = level; return; }
	} else if (c->level == level) return;
	if (c->count == AUDIO_POINTS) {
		log_once(a, ch, LOG_FULL, "audio: level history full, oldest change folded");
		c->level = c->pt[c->head].level;
		c->head = (c->head + 1) % AUDIO_POINTS;
		c->count--;
	}
	c->pt[(c->head + c->count) % AUDIO_POINTS].t = t;
	c->pt[(c->head + c->count) % AUDIO_POINTS].level = level;
	c->count++;
}

static void update(audio * const a, uint64_t t)
{
	int ch;
	if (t < a->last_t) t = a->last_t;
	a->last_t = t;
	for (ch = 0; ch < 2; ch++) push(a, ch, t, pin_level(a, ch));
}

void audio_init(audio * const a, double sample_rate, audio_log_fn log, void *ctx)
{
	memset(a, 0, sizeof(*a));
	a->r = exp(-2.0 * 3.14159265358979323846 * 10.0 / sample_rate);
	a->log = log;
	a->log_ctx = ctx;
}

void audio_reset(audio * const a, uint64_t t)
{
	memset(a->ctr, 0, sizeof(a->ctr));
	memset(a->frq, 0, sizeof(a->frq));
	a->out = a->dir = 0;
	if (t > a->last_t) a->last_t = t;
	update(a, t);
}

void audio_ctr(audio * const a, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq)
{
	int was = on_audio_pin(a->ctr[cog & 7][ctr & 1]);
	a->ctr[cog & 7][ctr & 1] = ctr_reg;
	a->frq[cog & 7][ctr & 1] = frq;
	if (was || on_audio_pin(ctr_reg)) update(a, t);
}

void audio_pins(audio * const a, uint64_t t, uint32_t out, uint32_t dir)
{
	uint32_t m = (1u << AUDIO_PIN_L) | (1u << AUDIO_PIN_R);
	if ((out & m) == (a->out & m) && (dir & m) == (a->dir & m)) return;
	a->out = out;
	a->dir = dir;
	update(a, t);
}

double audio_level(audio * const a, int ch, uint64_t t0, uint64_t t1)
{
	audio_chan * const c = &a->ch[ch];
	double acc = 0.0, level = c->level;
	uint64_t t = t0;
	while (c->count && c->pt[c->head].t <= t0) {
		c->level = level = c->pt[c->head].level;
		c->head = (c->head + 1) % AUDIO_POINTS;
		c->count--;
	}
	while (c->count && c->pt[c->head].t < t1) {
		acc += level * (double)(c->pt[c->head].t - t);
		t = c->pt[c->head].t;
		c->level = level = c->pt[c->head].level;
		c->head = (c->head + 1) % AUDIO_POINTS;
		c->count--;
	}
	acc += level * (double)(t1 - t);
	return t1 > t0 ? acc / (double)(t1 - t0) : level;
}

/* Each sample is the mean level over its interval, then a 10 Hz DC blocker.
   TODO: the mean is a crude anti-aliasing filter; content above half the sample rate aliases into the top of the
   audible range. A windowed-sinc or polyphase resampler would remove it (at maybe 5-10x this path's ~0.1% CPU).
   TODO: the board's RC low-pass (and amplifier) after the Propeller pins is not modelled; with its R/C values from
   the schematic it would match the real machine more closely and also reduce the aliasing */
void audio_render(audio * const a, float * const out_stereo, int n, uint64_t t_end)
{
	const uint64_t t0 = a->t_render, span = t_end > t0 ? t_end - t0 : 0;
	uint64_t s0 = t0;
	int j;
	for (j = 0; j < n; j++) {
		const uint64_t s1 = t0 + span * (uint64_t)(j + 1) / (uint64_t)n;
		int ch;
		for (ch = 0; ch < 2; ch++) {
			audio_chan * const c = &a->ch[ch];
			double x = audio_level(a, ch, s0, s1), y = x - c->x1 + a->r * c->y1;
			c->x1 = x;
			c->y1 = y;
			out_stereo[2 * j + ch] = (float)y;
		}
		s0 = s1;
	}
	if (t_end > a->t_render) a->t_render = t_end;
}
