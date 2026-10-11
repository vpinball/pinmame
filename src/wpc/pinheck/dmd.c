// license:BSD-3-Clause

#include "dmd.h"
#include <string.h>

static void restart(pinheck_dmd * const d)
{
	d->rows = d->row = d->nsum = d->sublevel = -1;
	d->count = 0;
	d->nbits = 0;
	memset(d->shift, 0, sizeof(d->shift));
	memset(d->latched, 0, sizeof(d->latched));
	memset(d->sub, 0, sizeof(d->sub));
}

void pinheck_dmd_init(pinheck_dmd * const d, int full, const uint8_t *hub, uint32_t buf, void *ctx, dmd_sub_fn on_sub, dmd_frame_fn on_frame)
{
	memset(d, 0, sizeof(*d));
	d->full = full;
	d->hub = hub;
	d->buf = buf;
	d->ctx = ctx;
	d->on_sub = on_sub;
	d->on_frame = on_frame;
	restart(d);
}

/* P18 rising: the row's dots go to the output latches; P20 marks a subframe's first row */
static void latch(pinheck_dmd * const d, uint32_t now, uint64_t t)
{
	int x;
	if (now & DMD_P20) {
		d->rows = 0;
		d->count++;
	} else
		d->rows = d->rows >= 0 && d->rows < DMD_H - 1 ? d->rows + 1 : -1;
	if (d->full) {
		const uint32_t start = d->nbits % DMD_W; /* the oldest of the last 128 bits */
		if (!start) memcpy(d->latched, d->shift, DMD_ROW);
		else {
			memset(d->latched, 0, DMD_ROW);
			for (x = 0; x < DMD_W; x++) {
				const uint32_t b = (start + (uint32_t)x) % DMD_W;
				if (d->shift[b >> 3] & (0x80 >> (b & 7))) d->latched[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
			}
			/* the register in shifting order from bit 0: the next latches take the copy */
			memcpy(d->shift, d->latched, DMD_ROW);
			d->nbits = 0;
		}
	} else {
		/* the cog shows subframe k after its start at level k mod 16: a dot is lit while its value is above the level */
		const int level = (d->count - 1) % DMD_LEVELS;
		memset(d->latched, 0, DMD_ROW);
		if (d->rows < 0 || !d->count) return;
		for (x = 0; x < DMD_W; x += 2) {
			const uint32_t a = (d->buf + (uint32_t)d->rows * (DMD_W / 2) + (uint32_t)(x >> 1)) & 0xFFFFu;
			const uint8_t v = d->hub_at ? d->hub_at(d->ctx, a, t) : d->hub[a];
			if ((v >> 4) > level) d->latched[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
			if ((v & 15) > level) d->latched[x >> 3] |= (uint8_t)(0x40 >> (x & 7));
		}
	}
}

static void subframe(pinheck_dmd * const d, uint64_t t)
{
	int i, k;
	if (d->on_sub) d->on_sub(d->ctx, d->sub, d->sublevel, t);
	if (!d->on_frame) return;
	if (d->sublevel == 0) {
		memset(d->sum, 0, sizeof(d->sum));
		d->nsum = 0;
	}
	if (d->nsum < 0 || d->sublevel != d->nsum) { d->nsum = -1; return; }
	for (i = 0; i < DMD_SUB; i++)
		if (d->sub[i])
			for (k = 0; k < 8; k++) d->sum[8 * i + k] += (uint8_t)(d->sub[i] >> (7 - k) & 1);
	if (++d->nsum == DMD_LEVELS) {
		d->on_frame(d->ctx, d->sum, t);
		d->nsum = -1;
	}
}

/* P19 rising: the row driver shifts, starting over at the first row while P20 is high */
static void row_clock(pinheck_dmd * const d, uint32_t now, uint64_t t)
{
	if (now & DMD_P20) {
		d->row = 0;
		d->sublevel = d->count > 0 ? (d->count - 1) % DMD_LEVELS : -1;
	} else
		d->row = d->row >= 0 && d->row < DMD_H - 1 ? d->row + 1 : -1;
	if (d->row < 0) return;
	memcpy(d->sub + d->row * DMD_ROW, d->latched, DMD_ROW);
	if (d->row == DMD_H - 1) subframe(d, t);
}

void pinheck_dmd_pins(pinheck_dmd * const d, uint64_t t, uint32_t out, uint32_t dir)
{
	const uint32_t now = out & dir & (d->full ? DMD_ALL_PINS : DMD_ROW_PINS), rise = now & ~d->level;
	const int on = (dir & DMD_ROW_PINS) == DMD_ROW_PINS;

	d->level = now;
	if (on != d->on) {
		d->on = on;
		restart(d);
	}
	if (!on) return;
	if (rise & DMD_P17) {
		const uint32_t b = d->nbits % DMD_W;
		if (now & DMD_P16) d->shift[b >> 3] |= (uint8_t)(0x80 >> (b & 7));
		else d->shift[b >> 3] &= (uint8_t)~(0x80 >> (b & 7));
		d->nbits++;
	}
	if (rise & DMD_P18) latch(d, now, t);
	if (rise & DMD_P19) row_clock(d, now, t);
}
