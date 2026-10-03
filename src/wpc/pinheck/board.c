// license:BSD-3-Clause

#include "board.h"
#include <string.h>

enum { PA, PB, PC, PD, PE, PF, PG };

static const uint8_t sol_port[BOARD_SOLS] = { PC, PC, PE, PF, PE, PE, PE, PC, PE, PC, PA, PA, PA, PA, PA, PC, PG, PG, PA, PA, PG, PG, PG, PA };
static const uint8_t sol_bit[BOARD_SOLS]  = {  2,  3,  5,  3,  6,  7,  8, 14,  9,  4,  2,  0,  1,  4,  5, 13,  1,  0,  6,  7, 14, 12, 13,  9 };
static const uint8_t servo_port[BOARD_SERVOS] = { PF, PA, PF, PE, PE };
static const uint8_t servo_bit[BOARD_SERVOS]  = {  1, 10,  4,  3,  4 };

#define LVL(b, p, n) ((int)((((b)->lat[p] & ~(b)->tris[p]) >> (n)) & 1u))

static void ws_latch(pinheck_board *b, int chain, uint64_t t)
{
	pinheck_board_ws *w = &b->ws[chain];
	int i, n = w->bits / 24;
	if (n > BOARD_RGB_MAX) n = BOARD_RGB_MAX;
	for (i = 0; i < n; i++)
		if (b->io.rgb) b->io.rgb(b->io.ctx, t, chain, i, w->buf[3 * i], w->buf[3 * i + 1], w->buf[3 * i + 2]);
	b->ws_leds[chain] = n;
	w->bits = 0;
	memset(w->buf, 0, sizeof(w->buf));
}

static void ws_edge(pinheck_board *b, int chain, int data, uint64_t t)
{
	pinheck_board_ws *w = &b->ws[chain];
	if (w->bits && t - w->last > b->hz / 2000) ws_latch(b, chain, w->last + b->hz / 2000);
	if (w->bits < BOARD_RGB_MAX * 24 && data) w->buf[w->bits >> 3] |= (uint8_t)(0x80 >> (w->bits & 7));
	w->bits++;
	w->last = t;
}

static void update_sols(pinheck_board *b, uint64_t t)
{
	uint32_t s = b->wd_on ? b->sol_pins : 0;
	if (s == b->sols) return;
	b->sols = s;
	if (b->io.sols) b->io.sols(b->io.ctx, t, s);
}

void pinheck_board_tick(pinheck_board *b, uint64_t t)
{
	int c;
	if (b->wd_on && t >= b->wd_until) { b->wd_on = 0; update_sols(b, b->wd_until); }
	for (c = 0; c < BOARD_RGB_CHAINS; c++)
		if (b->ws[c].bits && t - b->ws[c].last > b->hz / 2000) ws_latch(b, c, b->ws[c].last + b->hz / 2000);
	for (c = 0; c < BOARD_SERVOS; c++)
		if (b->servo_on[c] && t - b->servo_rise[c] >= (uint64_t)b->hz * 3 / 50) {
			b->servo_on[c] = 0;
			if (b->io.servo) b->io.servo(b->io.ctx, b->servo_rise[c] + (uint64_t)b->hz * 3 / 50, c, 0);
		}
}

void pinheck_board_init(pinheck_board *b, const pinheck_board_io *io, uint32_t hz)
{
	memset(b, 0, sizeof(*b));
	b->io = *io;
	b->hz = hz;
	b->sr165 = 0xFFFFu;
	memset(b->tris, 0xFF, sizeof(b->tris));
}

void pinheck_board_port(pinheck_board *b, int port, uint32_t lat, uint32_t tris, uint64_t t)
{
	uint32_t old = b->lat[port] & ~b->tris[port], now = lat & ~tris, ch = old ^ now;
	int i;

	b->lat[port] = lat;
	b->tris[port] = tris;
	if (!ch) return;
	pinheck_board_tick(b, t);
	if (port == PB && (b->lamp_cols != (uint8_t)now || b->lamp_rows != (uint8_t)(now >> 8))) {
		b->lamp_cols = (uint8_t)now;
		b->lamp_rows = (uint8_t)(now >> 8);
		if (b->io.lamps) b->io.lamps(b->io.ctx, t, b->lamp_cols, b->lamp_rows);
	}
	for (i = 0; i < BOARD_SOLS; i++)
		if (sol_port[i] == port && (ch >> sol_bit[i] & 1u))
			b->sol_pins = (b->sol_pins & ~(1u << i)) | ((now >> sol_bit[i] & 1u) << i);
	if (port == PG && (ch >> 15 & 1u) && !(now >> 15 & 1u)) {
		b->wd_until = t + (uint64_t)b->hz * 9 / 8;
		b->wd_on = 1;
	}
	update_sols(b, t);
	if (port == PA && (ch >> 3 & 1u)) {
		b->start = (int)(now >> 3 & 1u);
		if (b->io.start_lamp) b->io.start_lamp(b->io.ctx, t, b->start);
	}
	if (port == PE && (ch & 1u) && (now & 1u)) {
		b->sr595 = (uint16_t)(b->sr595 << 1 | (uint16_t)LVL(b, PG, 7));
		if (b->cab_lat) b->sr165 = (uint16_t)(b->sr165 << 1 | 1u);
	}
	if (port == PG && (ch >> 8 & 1u)) {
		b->cab_lat = (int)(now >> 8 & 1u);
		if (!b->cab_lat && b->sr595 != b->gi) {
			b->gi = b->sr595;
			if (b->io.gi) b->io.gi(b->io.ctx, t, b->gi);
		}
		if (b->cab_lat) b->sr165 = (uint16_t)~(b->io.cabinet ? b->io.cabinet(b->io.ctx) : 0);
	}
	if (port == PE && (ch >> 2 & 1u) && (now >> 2 & 1u)) ws_edge(b, BOARD_RGB_ONBOARD, LVL(b, PE, 1), t);
	if (port == PC && (ch >> 1 & 1u) && (now >> 1 & 1u)) ws_edge(b, BOARD_RGB_EXTERNAL, LVL(b, PG, 6), t);
	for (i = 0; i < BOARD_SERVOS; i++) {
		if (servo_port[i] != port || !(ch >> servo_bit[i] & 1u)) continue;
		if (now >> servo_bit[i] & 1u) { b->servo_rise[i] = t; b->servo_on[i] = 1; }
		else if (b->servo_on[i] && b->io.servo) b->io.servo(b->io.ctx, t, i, (uint32_t)(t - b->servo_rise[i]));
	}
}

uint32_t pinheck_board_read(pinheck_board *b, int port, uint64_t t)
{
	uint32_t v = 0xFFFFu;
	int c;
	(void)t;
	if (port == PD && b->io.sw_col)
		for (c = 0; c < 8; c++)
			if (!(b->tris[PD] >> (8 + c) & 1u) && !(b->lat[PD] >> (8 + c) & 1u))
				v &= ~(uint32_t)b->io.sw_col(b->io.ctx, c);
	if (port == PF) {
		uint16_t q = b->cab_lat ? b->sr165 : (uint16_t)~(b->io.cabinet ? b->io.cabinet(b->io.ctx) : 0);
		if (!(q & 0x8000u)) v &= ~1u;
	}
	return v;
}
