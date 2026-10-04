// license:BSD-3-Clause

#include "rtc.h"
#include <string.h>

enum { IDLE, CTRL, PTR, WRITE, READ, IGNORE };

static uint8_t bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static int unbcd(uint8_t v) { return (v >> 4) * 10 + (v & 15); }

static int64_t days_from_civil(int y, int m, int d)
{
	int64_t era, yoe, doy, doe;
	y -= m <= 2;
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

static void civil_from_days(int64_t z, int *y, int *m, int *d)
{
	int64_t era, doe, yoe, doy, mp;
	z += 719468;
	era = (z >= 0 ? z : z - 146096) / 146097;
	doe = z - era * 146097;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	*d = (int)(doy - (153 * mp + 2) / 5 + 1);
	*m = (int)(mp < 10 ? mp + 3 : mp - 9);
	*y = (int)(yoe + era * 400 + (*m <= 2));
}

static int64_t now(const ds1340 *d)
{
	return d->base + (int64_t)(d->ticks / (d->ticks_per_s ? d->ticks_per_s : 1));
}

static void latch(ds1340 *d)
{
	int64_t t = now(d), days = t >= 0 ? t / 86400 : (t - 86399) / 86400, s = t - days * 86400;
	int y, m, dd;
	civil_from_days(days, &y, &m, &dd);
	d->reg[0] = (uint8_t)((d->reg[0] & 0x80) | bcd((int)(s % 60)));
	d->reg[1] = bcd((int)(s / 60 % 60));
	d->reg[2] = (uint8_t)((d->reg[2] & 0xC0) | bcd((int)(s / 3600)));
	d->reg[3] = (uint8_t)(((days + 4) % 7 + 7) % 7 + 1);
	d->reg[4] = bcd(dd);
	d->reg[5] = bcd(m);
	d->reg[6] = bcd(y % 100);
}

static void store_time(ds1340 *d)
{
	int y = 2000 + unbcd(d->reg[6]);
	int64_t t = days_from_civil(y, unbcd(d->reg[5] & 0x1F), unbcd(d->reg[4] & 0x3F)) * 86400 +
		unbcd(d->reg[2] & 0x3F) * 3600 + unbcd(d->reg[1] & 0x7F) * 60 + unbcd(d->reg[0] & 0x7F);
	d->base = t - (int64_t)(d->ticks / (d->ticks_per_s ? d->ticks_per_s : 1));
}

void ds1340_init(ds1340 *d, int64_t unix_seconds, uint64_t ticks_per_second)
{
	memset(d, 0, sizeof(*d));
	d->base = unix_seconds;
	d->ticks_per_s = ticks_per_second;
	d->scl = d->sda = 1;
	d->out = 1;
	d->reg[7] = 0x80;
	latch(d);
}

void ds1340_tick(ds1340 *d, uint64_t ticks) { d->ticks += ticks; }

static void byte_done(ds1340 *d)
{
	uint8_t b = d->shift;
	switch (d->state) {
	case CTRL:
		if ((b >> 1) != 0x68) { d->ack = 0; d->state = IGNORE; return; }
		d->ack = 1;
		d->state = (b & 1) ? READ : PTR;
		if (d->state == READ) latch(d);
		return;
	case PTR:
		d->ptr = b % 10;
		d->ack = 1;
		d->state = WRITE;
		latch(d);
		return;
	case WRITE:
		d->reg[d->ptr] = b;
		if (d->ptr <= 6) store_time(d);
		d->ptr = (d->ptr + 1) % 10;
		d->ack = 1;
		return;
	}
	d->ack = 0;
}

int ds1340_update(ds1340 *d, int scl, int sda)
{
	int pscl = d->scl, psda = d->sda;
	d->scl = scl;
	d->sda = sda;
	if (pscl && scl && psda != sda) {
		if (!sda) { d->state = CTRL; d->bit = 0; d->shift = 0; d->out = 1; d->sent = 0; }
		else { d->state = IDLE; d->out = 1; }
		return d->out;
	}
	if (d->state == IDLE || d->state == IGNORE) return d->out = 1;
	if (!pscl && scl) {
		if (d->bit < 8) {
			if (d->state != READ) d->shift = (uint8_t)(d->shift << 1 | (sda & 1));
			d->bit++;
		} else {
			if (d->sent && sda) { d->state = IGNORE; d->out = 1; return d->out; }
			d->bit = 9;
		}
		return d->out;
	}
	if (pscl && !scl) {
		if (d->bit == 8) {
			if (d->sent) d->out = 1;
			else { byte_done(d); d->out = d->ack ? 0 : 1; }
		} else if (d->bit == 9) {
			d->bit = 0;
			d->shift = 0;
			if (d->state == READ) { d->shift = d->reg[d->ptr]; d->ptr = (d->ptr + 1) % 10; d->out = d->shift >> 7 & 1; d->sent = 1; }
			else d->out = 1;
		} else if (d->sent && d->bit > 0) d->out = d->shift >> (7 - d->bit) & 1;
	}
	return d->out;
}
