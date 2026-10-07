// license:BSD-3-Clause

#include "display.h"
#include <stdio.h>
#include <string.h>

#define PINS (DISPLAY_P17 | DISPLAY_P20 | DISPLAY_P21 | DISPLAY_P22)

static void say(display *d, int *once, const char *msg)
{
	if (*once) return;
	*once = 1;
	if (d->log) d->log(d->ctx, msg);
}

static void latch(display *d, uint64_t t, int cfg)
{
	char msg[80];
	long n = d->nbits / 8;

	if (d->nbits == 0) return;
	if (d->nbits % 8) {
		sprintf(msg, "display: latch after %ld bits, discarded", d->nbits);
		say(d, &d->logged_bits, msg);
	} else if (d->mode != cfg) {
		sprintf(msg, "display: mode changed during a %ld-byte transfer, discarded", n);
		say(d, &d->logged_mixed, msg);
	} else if (cfg) {
		if (n > DISPLAY_CFG_MAX) {
			sprintf(msg, "display: %ld-byte config packet, first %d kept", n, DISPLAY_CFG_MAX);
			say(d, &d->logged_cfg, msg);
			n = DISPLAY_CFG_MAX;
		}
		if (d->on_config) d->on_config(d->ctx, d->buf, (int)n, t);
	} else if (n != d->frame) {
		sprintf(msg, "display: frame of %ld bytes discarded", n);
		if (d->logged_frame < DISPLAY_LOG_FRAMES && d->log) d->log(d->ctx, msg);
		else if (d->logged_frame == DISPLAY_LOG_FRAMES && d->log) d->log(d->ctx, "display: further discarded frames not logged");
		if (d->logged_frame <= DISPLAY_LOG_FRAMES) d->logged_frame++;
	} else if (d->on_frame)
		d->on_frame(d->ctx, d->buf, t);
	d->nbits = 0;
}

void pinheck_display_init(display *d, void *ctx, display_frame_fn on_frame, display_config_fn on_config, display_log_fn log)
{
	memset(d, 0, sizeof(*d));
	d->ctx = ctx;
	d->on_frame = on_frame;
	d->on_config = on_config;
	d->log = log;
	d->frame = DISPLAY_FRAME;
}

/* the 128 x 32 or the 128 x 64 module: frames of w * h bytes; 0 (frames stay 128 x 32) for any other size */
int pinheck_display_size(display *d, int w, int h)
{
	if (!DISPLAY_SIZE_OK(w, h)) return 0;
	d->frame = (long)w * h;
	return 1;
}

void pinheck_display_pins(display *d, uint64_t t, uint32_t out, uint32_t dir)
{
	uint32_t now = out & dir & PINS, old = d->level;

	d->level = now;
	if ((now & DISPLAY_P20) && !(old & DISPLAY_P20)) {
		latch(d, t, (now & DISPLAY_P17) != 0);
		return;
	}
	if ((now & DISPLAY_P22) && !(old & DISPLAY_P22) && !(now & DISPLAY_P20)) {
		long i = d->nbits >> 3;
		if (d->nbits == 0) d->mode = (now & DISPLAY_P17) != 0;
		if (i < d->frame) {
			if ((d->nbits & 7) == 0) d->buf[i] = 0;
			if (now & DISPLAY_P21) d->buf[i] |= (uint8_t)(0x80 >> (d->nbits & 7));
		}
		d->nbits++;
	}
}

static int word(const uint8_t *b, int i)
{
	return b[2 * i] << 8 | b[2 * i + 1];
}

/* Big-endian words: ?, POSITION, PIXEL SHAPE, BRIGHTNESS, width, height, then BAR BRIGHT in the 128 x 32 module's 14
   bytes; the 128 x 64 module's 12 bytes end after the height (The Jetsons: 1, 55, 0, 255, 128, 64), so its bar brightness
   keeps the default/guess. Anything else keeps the exact look (square dots, as sent) and returns 0 */
int pinheck_display_look(display_look *look, const uint8_t *cfg, int n)
{
	look->shape = DISPLAY_SQUARE;
	look->brightness = 255;
	look->position = DISPLAY_ALIGNED;
	look->bar = 62;
	if (!cfg || (n != 14 && n != 12) || word(cfg, 4) != DISPLAY_W || word(cfg, 5) != (n == 14 ? DISPLAY_H : 2 * DISPLAY_H))
		return 0;
	look->position = word(cfg, 1);
	look->shape = word(cfg, 2) <= DISPLAY_HIGHREZ ? word(cfg, 2) : DISPLAY_SQUARE;
	look->brightness = word(cfg, 3) < 255 ? word(cfg, 3) : 255;
	if (n == 14)
		look->bar = word(cfg, 6) < 62 ? word(cfg, 6) : 62;
	return 1;
}

static void rgb332(uint8_t v, int *c)
{
	c[0] = ((v >> 5) & 7) * 255 / 7;
	c[1] = ((v >> 2) & 7) * 255 / 7;
	c[2] = (v & 3) * 255 / 3;
}

/* adds dot (x, y)'s colour to c; outside the frame (h rows) is black */
static void add_dot(const uint8_t *f, int h, int x, int y, int *c)
{
	int d[3];
	if (x >= DISPLAY_W || y >= h) return;
	rgb332(f[y * DISPLAY_W + x], d);
	c[0] += d[0];
	c[1] += d[1];
	c[2] += d[2];
}

/* Scale2x: sub-pixel (sx, sy) of dot (x, y), neighbours clamped at the edges.
   TODO: find out what the module's HIGH REZ really does (photos, videos or its firmware); Scale2x is a stand-in */
static uint8_t scale2x(const uint8_t *f, int h, int x, int y, int sx, int sy)
{
	uint8_t p = f[y * DISPLAY_W + x];
	uint8_t up = y > 0 ? f[(y - 1) * DISPLAY_W + x] : p, down = y < h - 1 ? f[(y + 1) * DISPLAY_W + x] : p;
	uint8_t left = x > 0 ? f[y * DISPLAY_W + x - 1] : p, right = x < DISPLAY_W - 1 ? f[y * DISPLAY_W + x + 1] : p;
	uint8_t v = sy ? down : up, hz = sx ? right : left, v2 = sy ? up : down, hz2 = sx ? left : right;
	return v == hz && hz != v2 && v != hz2 ? v : p;
}

/* frame (128 x h RGB332, h = 32 or 64) -> rgb (256 x 2h, 3 bytes per pixel) in the look */
void pinheck_display_render(const display_look *look, const uint8_t *frame, int h, uint8_t *rgb)
{
	int x, y, k, dy = look->position - DISPLAY_ALIGNED;
	dy = dy >= 0 ? dy / 4 : -((3 - dy) / 4);
	memset(rgb, 0, (size_t)DISPLAY_LOOK_W * 2 * h * 3);
	for (y = 0; y < 2 * h; y++) {
		const int dotY = y >> 1, sy = y & 1;
		uint8_t *out;
		if (y + dy < 0 || y + dy >= 2 * h) continue;
		out = rgb + (y + dy) * (DISPLAY_LOOK_W * 3);
		for (x = 0; x < DISPLAY_LOOK_W; x++) {
			const int dotX = x >> 1, sx = x & 1;
			int c[3] = { 0, 0, 0 };
			if (look->shape == DISPLAY_HIGHREZ)
				rgb332(scale2x(frame, h, dotX, dotY, sx, sy), c);
			else if (look->shape == DISPLAY_SQUARE || !(sx | sy))
				add_dot(frame, h, dotX, dotY, c);
			else {
				/* a gap between round dots: the mean of the dots around it, times bar / 124 */
				const int m = (1 + sx) * (1 + sy);
				add_dot(frame, h, dotX, dotY, c);
				if (sx) add_dot(frame, h, dotX + 1, dotY, c);
				if (sy) add_dot(frame, h, dotX, dotY + 1, c);
				if (sx && sy) add_dot(frame, h, dotX + 1, dotY + 1, c);
				for (k = 0; k < 3; k++) c[k] = c[k] * look->bar / (124 * m);
			}
			for (k = 0; k < 3; k++) *out++ = (uint8_t)(c[k] * look->brightness / 255);
		}
	}
}
