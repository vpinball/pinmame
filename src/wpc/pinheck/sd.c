// license:BSD-3-Clause

#include "sd.h"
#include <string.h>

static uint8_t crc7(const uint8_t *d, int n)
{
	uint8_t c = 0;
	int i, b;
	for (i = 0; i < n; i++)
		for (b = 7; b >= 0; b--) {
			int x = ((c >> 6) ^ (d[i] >> b)) & 1;
			c = (uint8_t)((c << 1) & 0x7F);
			if (x) c ^= 0x09;
		}
	return c;
}

static uint16_t crc16(const uint8_t *d, int n)
{
	uint16_t c = 0;
	int i, b;
	for (i = 0; i < n; i++) {
		c ^= (uint16_t)(d[i] << 8);
		for (b = 0; b < 8; b++) c = (uint16_t)((c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1);
	}
	return c;
}

static void bits(uint8_t *r, int hi, int width, uint32_t v)
{
	int k;
	for (k = 0; k < width; k++) {
		int bit = hi - width + 1 + k, byte = 15 - bit / 8;
		if ((v >> k) & 1) r[byte] |= (uint8_t)(1 << (bit % 8));
		else r[byte] &= (uint8_t)~(1 << (bit % 8));
	}
}

static void push(sd_card *s, uint8_t b)
{
	if (s->olen == (int)sizeof(s->out)) {
		memmove(s->out, s->out + s->opos, (size_t)(s->olen - s->opos));
		s->olen -= s->opos;
		s->opos = 0;
	}
	if (s->olen < (int)sizeof(s->out)) s->out[s->olen++] = b;
}

static void r1(sd_card *s, uint8_t v)
{
	push(s, 0xFF);
	push(s, (uint8_t)(v | (s->idle ? 0x01 : 0x00)));
}

static void data_block(sd_card *s, const uint8_t *d, int n)
{
	uint16_t c = crc16(d, n);
	int k;
	push(s, 0xFF);
	push(s, 0xFE);
	for (k = 0; k < n; k++) push(s, d[k]);
	push(s, (uint8_t)(c >> 8));
	push(s, (uint8_t)c);
}

static int read_block(sd_card *s, uint32_t lba)
{
	uint8_t b[512];
	if (lba >= s->dev.sectors) { push(s, 0xFF); push(s, 0x08); return -1; }
	if (s->dev.read(s->dev.ctx, lba, b)) { push(s, 0xFF); push(s, 0x01); return -1; }
	s->reads++;
	data_block(s, b, 512);
	return 0;
}

static void reg16(sd_card *s, int cid)
{
	uint8_t r[16];
	memset(r, 0, sizeof(r));
	if (cid) {
		bits(r, 127, 8, 0x03);
		bits(r, 119, 16, ('S' << 8) | 'D');
		bits(r, 103, 32, ('P' << 24) | ('I' << 16) | ('N' << 8) | 'H');
		bits(r, 71, 8, 'K');
		bits(r, 63, 8, 0x10);
		bits(r, 55, 32, 0x20160701u);
		bits(r, 19, 12, (16 << 4) | 7);
	} else if (s->hc) {
		bits(r, 127, 2, 1);
		bits(r, 119, 8, 0x0E); bits(r, 103, 8, 0x32); bits(r, 95, 12, 0x5B5); bits(r, 83, 4, 9);
		bits(r, 69, 22, s->dev.sectors / 1024 ? s->dev.sectors / 1024 - 1 : 0);
		bits(r, 46, 1, 1); bits(r, 45, 7, 0x7F); bits(r, 25, 4, 9);
	} else {
		uint32_t c = s->dev.sectors / 512 ? s->dev.sectors / 512 - 1 : 0;
		bits(r, 119, 8, 0x0E); bits(r, 103, 8, 0x32); bits(r, 95, 12, 0x5B5); bits(r, 83, 4, 9);
		bits(r, 79, 1, 1); bits(r, 73, 12, c > 4095 ? 4095 : c); bits(r, 49, 3, 7);
		bits(r, 46, 1, 1); bits(r, 45, 7, 0x7F); bits(r, 25, 4, 9);
	}
	r[15] = (uint8_t)(crc7(r, 15) << 1 | 1);
	r1(s, 0x00);
	data_block(s, r, 16);
}

static void command(sd_card *s)
{
	uint32_t arg = (uint32_t)s->cmd[1] << 24 | (uint32_t)s->cmd[2] << 16 | (uint32_t)s->cmd[3] << 8 | s->cmd[4];
	int idx = s->cmd[0] & 0x3F, acmd = s->acmd;

	s->acmd = 0;
	if (acmd && idx == 41) {
		if (s->v2 && (arg & 0x40000000u)) s->hc = 1;
		s->idle = 0;
		r1(s, 0x00);
		return;
	}
	switch (idx) {
	case 0: s->idle = 1; s->v2 = s->hc = s->multi = s->wstate = 0; r1(s, 0x00); break;
	case 1: s->idle = 0; r1(s, 0x00); break;
	case 8: s->v2 = 1; r1(s, 0x00); push(s, 0x00); push(s, 0x00); push(s, (uint8_t)((arg >> 8) & 0x0F)); push(s, (uint8_t)arg); break;
	case 9: reg16(s, 0); break;
	case 10: reg16(s, 1); break;
	case 12: s->multi = 0; s->olen = s->opos = 0; r1(s, 0x00); break;
	case 13: r1(s, 0x00); push(s, 0x00); break;
	case 16: r1(s, arg == 512 ? 0x00 : 0x40); break;
	case 17: case 18: {
		uint32_t lba = s->hc ? arg : arg >> 9;
		if (!s->hc && (arg & 511)) { r1(s, 0x20); break; }
		r1(s, 0x00);
		if (read_block(s, lba) == 0 && idx == 18) { s->multi = 1; s->next_lba = lba + 1; }
		break;
	}
	case 24: case 25: r1(s, 0x00); s->wstate = idx == 25 ? 3 : 1; break;
	case 55: r1(s, 0x00); s->acmd = 1; break;
	case 58: r1(s, 0x00); push(s, (uint8_t)((s->idle ? 0 : 0x80) | (s->hc ? 0x40 : 0))); push(s, 0xFF); push(s, 0x80); push(s, 0x00); break;
	case 59: r1(s, 0x00); break;
	default: r1(s, 0x04); break;
	}
}

static void write_byte(sd_card *s, uint8_t b)
{
	int multi = s->wstate >= 3;
	int st = multi ? s->wstate - 2 : s->wstate;
	if (st == 1) {
		if (multi && b == 0xFD) { push(s, 0xFF); push(s, 0x00); push(s, 0xFF); s->wstate = 0; return; }
		if (b == (multi ? 0xFC : 0xFE)) { s->wcount = 0; s->wstate = multi ? 4 : 2; }
		return;
	}
	if (++s->wcount < 514) return;
	s->writes++;
	push(s, 0x05); push(s, 0x00); push(s, 0x00); push(s, 0xFF);
	s->wstate = multi ? 3 : 0;
}

static void in_byte(sd_card *s, uint8_t b)
{
	if (s->wstate) { write_byte(s, b); return; }
	if (!s->ncmd && (b & 0xC0) != 0x40) return;
	s->cmd[s->ncmd++] = b;
	if (s->ncmd == 6) { s->ncmd = 0; command(s); }
}

static uint8_t pop(sd_card *s)
{
	if (s->opos >= s->olen && s->multi) {
		s->olen = s->opos = 0;
		if (read_block(s, s->next_lba++)) s->multi = 0;
	}
	if (s->opos >= s->olen) { s->olen = s->opos = 0; return 0xFF; }
	return s->out[s->opos++];
}

void sd_init(sd_card *s, const sd_blockdev *dev)
{
	memset(s, 0, sizeof(*s));
	s->dev = *dev;
	s->cs = 1;
	s->dout = 0xFF;
	s->idle = 1;
}

int sd_update(sd_card *s, int cs, int sclk, int mosi)
{
	cs = cs != 0; sclk = sclk != 0; mosi = mosi != 0;
	if (cs) {
		if (!s->cs) { s->inbits = 0; s->ncmd = 0; s->olen = s->opos = 0; s->obit = 0; s->dout = 0xFF; s->multi = 0; }
		s->cs = 1;
		s->sclk = sclk;
		return 1;
	}
	if (s->cs) { s->cs = 0; s->inbits = 0; s->obit = 0; s->dout = 0xFF; s->risen = 0; }
	if (sclk && !s->sclk) {
		s->risen = 1;
		s->in = (uint8_t)(s->in << 1 | mosi);
		if (++s->inbits == 8) { s->inbits = 0; in_byte(s, s->in); }
	} else if (!sclk && s->sclk && s->risen) {
		s->risen = 0;
		if (++s->obit == 8) { s->obit = 0; s->dout = pop(s); }
	}
	s->sclk = sclk;
	return (s->dout >> (7 - s->obit)) & 1;
}
