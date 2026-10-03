// license:BSD-3-Clause

#include "bootldr.h"
#include <stdio.h>
#include <string.h>

#define PAGE 4096u

static void logf1(pic32_boot *b, const char *fmt, unsigned long v)
{
	char msg[80];
	if (!b->log) return;
	sprintf(msg, fmt, v);
	b->log(b->log_ctx, msg);
}

static void tx_level(pic32_boot *b, uint64_t t, int level)
{
	if (level == b->tx_level) return;
	b->tx_level = level;
	if (b->tx) b->tx(b->tx_ctx, t, level);
}

void boot_init(pic32_boot *b, uint8_t *flash, uint32_t flash_size, boot_tx_fn tx, void *tx_ctx)
{
	memset(b, 0, sizeof(*b));
	b->flash = flash;
	b->flash_size = flash_size;
	b->tx = tx;
	b->tx_ctx = tx_ctx;
	b->window = BOOT_WINDOW;
	boot_reset(b, 0);
}

void boot_set_window(pic32_boot *b, uint64_t pic_cycles)
{
	b->window = pic_cycles;
}

void boot_set_log(pic32_boot *b, boot_log_fn fn, void *ctx)
{
	b->log = fn;
	b->log_ctx = ctx;
}

void boot_reset(pic32_boot *b, uint64_t t)
{
	b->state = BOOT_WAIT;
	b->window_end = t + b->window;
	b->app_at = 0;
	b->tx_free = t;
	b->line = 1;
	b->nedge = 0;
	b->scan_from = t;
	b->flen = 0;
	b->addr = 0;
	memset(b->erased, 0, sizeof(b->erased));
	b->tx_level = -1;
	tx_level(b, t, 1);
}

void boot_rx(pic32_boot *b, uint64_t t, int level)
{
	level = level != 0;
	if (level == b->line) return;
	b->line = level;
	if (b->nedge == BOOT_EDGES) { b->errors++; return; }
	b->edge_t[b->nedge] = t;
	b->edge_l[b->nedge] = (uint8_t)level;
	b->nedge++;
}

static int level_at(const pic32_boot *b, uint64_t t, int base)
{
	int k, v = base;
	for (k = 0; k < b->nedge && b->edge_t[k] <= t; k++) v = b->edge_l[k];
	return v;
}

static void send(pic32_boot *b, uint64_t t, const uint8_t *body, int n, uint8_t seq)
{
	uint8_t f[BOOT_FRAME + 8];
	int i, k, len = 0;
	uint8_t x = 0;
	f[len++] = 0x1B; f[len++] = seq; f[len++] = (uint8_t)(n >> 8); f[len++] = (uint8_t)n; f[len++] = 0x0E;
	for (i = 0; i < n; i++) f[len++] = body[i];
	for (i = 0; i < len; i++) x ^= f[i];
	f[len++] = x;
	if (t < b->tx_free) t = b->tx_free;
	for (i = 0; i < len; i++) {
		tx_level(b, t, 0);
		for (k = 0; k < 8; k++) tx_level(b, t + BOOT_BIT * (uint64_t)(k + 1), f[i] >> k & 1);
		tx_level(b, t + BOOT_BIT * 9u, 1);
		t += BOOT_BIT * 10u;
	}
	b->tx_free = t;
}

static void program(pic32_boot *b, uint32_t a, uint8_t v)
{
	uint32_t page = a / PAGE;
	if (a >= b->flash_size) return;
	if (!(b->erased[page >> 3] & (1u << (page & 7)))) {
		b->erased[page >> 3] |= (uint8_t)(1u << (page & 7));
		memset(b->flash + page * PAGE, 0xFF, PAGE);
	}
	b->flash[a] = v;
}

static void handle(pic32_boot *b, uint64_t t, uint64_t now)
{
	uint8_t *f = b->frame, out[BOOT_FRAME + 8];
	uint8_t cmd = f[5];
	uint32_t cnt, i;
	uint64_t delay = BOOT_LATENCY;
	int len = 2, leave = 0;

	out[0] = cmd;
	out[1] = 0x00;
	switch (cmd) {
	case 0x01:
		out[2] = 8;
		memcpy(out + 3, "STK500_2", 8);
		len = 11;
		if (b->state == BOOT_WAIT) logf1(b, "boot: sign-on %lu", 0);
		b->state = BOOT_HOST;
		break;
	case 0x06:
		b->addr = (uint32_t)f[6] << 24 | (uint32_t)f[7] << 16 | (uint32_t)f[8] << 8 | f[9];
		break;
	case 0x13:
		cnt = (uint32_t)f[6] << 8 | f[7];
		for (i = 0; i < cnt; i++) program(b, b->addr + i, f[15 + i]);
		b->addr += cnt;
		b->programmed += cnt;
		delay += BOOT_PROGRAM;
		break;
	case 0x14:
		cnt = (uint32_t)f[6] << 8 | f[7];
		if (cnt + 3 > BOOT_FRAME) cnt = BOOT_FRAME - 3;
		for (i = 0; i < cnt; i++) out[2 + i] = b->addr + i < b->flash_size ? b->flash[b->addr + i] : 0xFF;
		out[2 + cnt] = 0x00;
		len = (int)cnt + 3;
		b->addr += cnt;
		b->read += cnt;
		break;
	case 0x11:
		leave = 1;
		break;
	default:
		out[1] = 0xC0;
		if (!(b->unknown_logged[cmd >> 5] & (1u << (cmd & 31)))) {
			b->unknown_logged[cmd >> 5] |= 1u << (cmd & 31);
			logf1(b, "boot: unsupported STK500v2 command %02lx", cmd);
		}
		break;
	}
	b->frames++;
	t += delay;
	send(b, t > now ? t : now, out, len, f[1]);
	if (leave) {
		b->app_at = b->tx_free;
		logf1(b, "boot: leave, %lu bytes programmed", b->programmed);
	}
}

static void feed(pic32_boot *b, uint8_t v, uint64_t t, uint64_t now)
{
	uint32_t size;
	int i;
	uint8_t x = 0;
	if (b->flen == 0 && v != 0x1B) { b->errors++; return; }
	b->frame[b->flen++] = v;
	if (b->flen < 5) return;
	size = (uint32_t)b->frame[2] << 8 | b->frame[3];
	if (size + 6 > BOOT_FRAME || b->frame[4] != 0x0E) { b->errors++; b->flen = 0; return; }
	if ((uint32_t)b->flen < size + 6) return;
	for (i = 0; i < b->flen; i++) x ^= b->frame[i];
	b->flen = 0;
	if (x || size == 0) { b->errors++; return; }
	if (b->state == BOOT_APP) return;
	handle(b, t, now);
}

void boot_advance(pic32_boot *b, uint64_t now)
{
	for (;;) {
		int k, base, s = -1;
		uint64_t start;
		uint32_t v = 0;
		for (k = 0; k < b->nedge; k++)
			if (!b->edge_l[k] && b->edge_t[k] >= b->scan_from) { s = k; break; }
		if (s < 0) {
			if (b->nedge) { b->edge_l[0] = b->edge_l[b->nedge - 1]; b->edge_t[0] = b->edge_t[b->nedge - 1]; b->nedge = 1; }
			break;
		}
		start = b->edge_t[s];
		if (now < start + BOOT_BIT * 9u + BOOT_BIT / 2u) break;
		base = s ? b->edge_l[s - 1] : 1;
		for (k = 0; k < 8; k++)
			if (level_at(b, start + BOOT_BIT * (uint64_t)k + BOOT_BIT * 3u / 2u, base)) v |= 1u << k;
		b->scan_from = start + BOOT_BIT * 9u + BOOT_BIT / 2u;
		if (!level_at(b, b->scan_from, base)) b->errors++;
		else feed(b, (uint8_t)v, start + BOOT_BIT * 10u, now);
		for (k = 0; k < b->nedge && b->edge_t[k] < b->scan_from; k++);
		if (k > 1) {
			memmove(b->edge_t, b->edge_t + k - 1, sizeof(b->edge_t[0]) * (size_t)(b->nedge - k + 1));
			memmove(b->edge_l, b->edge_l + k - 1, (size_t)(b->nedge - k + 1));
			b->nedge -= k - 1;
		}
	}
	if (b->state == BOOT_WAIT && now >= b->window_end) {
		b->state = BOOT_APP;
		tx_level(b, b->window_end, 0);
	} else if (b->state == BOOT_HOST && b->app_at && now >= b->app_at) {
		b->state = BOOT_APP;
		tx_level(b, b->app_at, 0);
	}
}

/* the machine stops (a restart or the end of a run) with the PIC32 held for programming */
void boot_stop(pic32_boot *b)
{
	if (b->state == BOOT_HOST && !b->app_at) logf1(b, "boot: stopped in programming mode, %lu bytes programmed", b->programmed);
}

uint64_t boot_hold(const pic32_boot *b, uint64_t now)
{
	uint64_t end, byte = now + BOOT_BIT * 9u + BOOT_BIT / 2u;
	int k;
	if (b->state == BOOT_APP) return 0;
	/* never step past a byte's decode point, so replies keep their exact latency */
	for (k = 0; k < b->nedge; k++)
		if (!b->edge_l[k] && b->edge_t[k] >= b->scan_from) { byte = b->edge_t[k] + BOOT_BIT * 9u + BOOT_BIT / 2u; break; }
	end = b->state == BOOT_WAIT ? b->window_end : b->app_at ? b->app_at : now + BOOT_POLL;
	if (byte < end) end = byte;
	if (end <= now) return 1;
	return end - now < BOOT_POLL ? end - now : BOOT_POLL;
}
