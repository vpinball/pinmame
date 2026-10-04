// license:BSD-3-Clause

#include "pic32mx.h"
#include <string.h>

#define SFR(p, off)     ((p)->sfr[(off) >> 2])
#define OFF_INTCON      0x81000u
#define OFF_INTSTAT     0x81010u
#define OFF_IFS0        0x81030u
#define OFF_IEC0        0x81060u
#define OFF_IPC0        0x81090u
#define OFF_OSCCON      0x0F000u
#define OFF_RCON        0x0F600u
#define OFF_TRISA       0x86000u
#define OFF_AD1PCFG     0x09060u
#define INTCON_MVEC     (1u << 12)
#define CON_ON          (1u << 15)

static const uint8_t irq_vector[PIC32MX_IRQS] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 23, 23,
	24, 24, 24, 25, 25, 25, 26, 27, 28, 29, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 35, 36, 37,
	38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 5, 9, 13, 17, 21, 28, 49, 49, 49, 50, 50, 50, 51, 51, 51
};

static const uint32_t uart_base[PIC32MX_UARTS] = { 0x6000, 0x6800, 0x6400, 0x6200, 0x6A00, 0x6600 };
static const uint8_t uart_irq[PIC32MX_UARTS] = { 26, 40, 37, 67, 73, 70 };
static const uint32_t i2c_base[PIC32MX_I2CS] = { 0x5300, 0x5400, 0x5000, 0x5100, 0x5200 };
static const uint8_t i2c_irq[PIC32MX_I2CS] = { 29, 43, 26, 37, 40 };
static const uint8_t timer_irq[5] = { 4, 8, 12, 16, 20 };
static const uint16_t tckps_a[4] = { 1, 8, 64, 256 };
static const uint16_t tckps_b[8] = { 1, 2, 4, 8, 16, 32, 64, 256 };

static const struct { uint32_t lo, hi; } known[] = {
	{ 0x00000, 0x00010 }, { 0x00600, 0x01000 }, { 0x05000, 0x05500 }, { 0x06000, 0x06C00 },
	{ 0x09000, 0x09100 }, { 0x0F000, 0x0F800 }, { 0x81000, 0x82000 }, { 0x82000, 0x82100 },
	{ 0x84000, 0x84100 }, { 0x86000, 0x86200 }
};

int pic32mx_irq_vector(int irq) { return irq >= 0 && irq < PIC32MX_IRQS ? irq_vector[irq] : -1; }

static void update_eic(pic32mx *p)
{
	int irq, best_pri = 0, best_sub = -1, best_vec = 0, mvec = (SFR(p, OFF_INTCON) & INTCON_MVEC) != 0;

	for (irq = 0; irq < PIC32MX_IRQS; irq++) {
		uint32_t bit = 1u << (irq & 31), w = (uint32_t)(irq >> 5) * 0x10;
		if (SFR(p, OFF_IFS0 + w) & SFR(p, OFF_IEC0 + w) & bit) {
			int vec = irq_vector[irq];
			uint32_t ipc = SFR(p, OFF_IPC0 + (uint32_t)(vec >> 2) * 0x10) >> ((vec & 3) * 8);
			int pri = (int)((ipc >> 2) & 7), sub = (int)(ipc & 3);
			if (pri > best_pri || (pri == best_pri && pri && sub > best_sub)) {
				best_pri = pri;
				best_sub = sub;
				best_vec = vec;
			}
		}
	}
	mips32_set_eic(&p->cpu, best_pri, mvec ? best_vec : 0, best_pri == 7 ? 1 : 0);
}

void pic32mx_set_irq(pic32mx *p, int irq)
{
	SFR(p, OFF_IFS0 + (uint32_t)(irq >> 5) * 0x10) |= 1u << (irq & 31);
	update_eic(p);
}

static uint32_t pbdiv(const pic32mx *p) { return 1u << ((SFR(p, OFF_OSCCON) >> 19) & 3); }

static int timer_t32(const pic32mx *p, int i)
{
	return (i == 1 || i == 3) && (SFR(p, 0x0600u + (uint32_t)i * 0x200) & 8u);
}

static int timer_slave(const pic32mx *p, int i) { return (i == 2 || i == 4) && timer_t32(p, i - 1); }

static void timer_cfg(const pic32mx *p, int i, uint32_t *div, uint32_t *pr, uint32_t *max)
{
	uint32_t base = 0x0600u + (uint32_t)i * 0x200, con = SFR(p, base);
	*div = (i == 0 ? tckps_a[(con >> 4) & 3] : tckps_b[(con >> 4) & 7]) * pbdiv(p);
	if (timer_t32(p, i)) {
		*pr = (SFR(p, base + 0x20) & 0xFFFFu) | (SFR(p, base + 0x220) << 16);
		*max = 0xFFFFFFFFu;
	} else {
		*pr = SFR(p, base + 0x20) & 0xFFFFu;
		*max = 0xFFFFu;
	}
}

static uint32_t steps_to_match(uint32_t t, uint32_t pr, uint32_t max)
{
	return t <= pr ? pr - t + 1 : (max - t) + pr + 2;
}

static void timer_sync(pic32mx *p, int i)
{
	pic32mx_timer *t = &p->timer[i];
	uint32_t div, pr, max, s;
	uint64_t n = p->cpu.cycles - t->last, ticks, events = 0;

	t->last = p->cpu.cycles;
	if (!(SFR(p, 0x0600u + (uint32_t)i * 0x200) & CON_ON) || timer_slave(p, i) || !n) return;
	timer_cfg(p, i, &div, &pr, &max);
	ticks = (n + t->frac) / div;
	t->frac = (uint32_t)((n + t->frac) % div);
	s = steps_to_match(t->tmr, pr, max);
	if (ticks >= s) {
		ticks -= s;
		t->tmr = 0;
		events = 1 + ticks / ((uint64_t)pr + 1);
		ticks %= (uint64_t)pr + 1;
	}
	t->tmr = (uint32_t)((t->tmr + ticks) & max);
	if (events) pic32mx_set_irq(p, timer_irq[timer_t32(p, i) ? i + 1 : i]);
}

static void timers_sync(pic32mx *p)
{
	int i;
	for (i = 0; i < 5; i++) timer_sync(p, i);
}

static uint64_t next_timer_event(const pic32mx *p)
{
	uint64_t best = ~(uint64_t)0;
	uint32_t div, pr, max;
	int i;

	for (i = 0; i < 5; i++) {
		const pic32mx_timer *t = &p->timer[i];
		uint64_t c;
		if (!(SFR(p, 0x0600u + (uint32_t)i * 0x200) & CON_ON) || timer_slave(p, i)) continue;
		timer_cfg(p, i, &div, &pr, &max);
		c = (uint64_t)steps_to_match(t->tmr, pr, max) * div - t->frac;
		if (c < best) best = c;
	}
	return best;
}

static int timer_reg(uint32_t off, int *i, uint32_t *reg)
{
	if (off < 0x0600u || off >= 0x1000u) return 0;
	*i = (int)((off - 0x0600u) >> 9);
	*reg = (off - 0x0600u) & 0x1F0u;
	return 1;
}

static uint32_t timer_read(pic32mx *p, int i, uint32_t reg)
{
	if (reg == 0x10) {
		timers_sync(p);
		if (timer_slave(p, i)) return p->timer[i - 1].tmr >> 16;
		return p->timer[i].tmr & 0xFFFFu;
	}
	return SFR(p, 0x0600u + (uint32_t)i * 0x200 + reg);
}

static void uart_update_rx_irq(pic32mx *p, int u)
{
	if (p->uart[u].rx_count) pic32mx_set_irq(p, uart_irq[u] + 1);
}

void pic32mx_uart_rx(pic32mx *p, int uart, uint8_t byte)
{
	pic32mx_uart *u = &p->uart[uart];
	uint32_t base = uart_base[uart];

	if (!(SFR(p, base) & CON_ON) || !(SFR(p, base + 0x10) & (1u << 12))) return;
	if (u->rx_count == PIC32MX_RXFIFO) { SFR(p, base + 0x10) |= 2u; return; }
	u->rx[(u->rx_head + u->rx_count) % PIC32MX_RXFIFO] = byte;
	u->rx_count++;
	uart_update_rx_irq(p, uart);
}

static int uart_reg(uint32_t off, int *u, uint32_t *reg)
{
	int i;
	for (i = 0; i < PIC32MX_UARTS; i++)
		if (off >= uart_base[i] && off < uart_base[i] + 0x50) { *u = i; *reg = (off - uart_base[i]) & 0xF0u; return 1; }
	return 0;
}

static uint32_t uart_read(pic32mx *p, int u, uint32_t reg)
{
	uint32_t base = uart_base[u];
	pic32mx_uart *q = &p->uart[u];

	if (reg == 0x10)
		return (SFR(p, base + 0x10) & 0xFCE2u) | (1u << 8) | (1u << 4) | (q->rx_count ? 1u : 0u);
	if (reg == 0x30) {
		uint32_t v = 0;
		if (q->rx_count) {
			v = q->rx[q->rx_head];
			q->rx_head = (q->rx_head + 1) % PIC32MX_RXFIFO;
			q->rx_count--;
			uart_update_rx_irq(p, u);
		}
		return v;
	}
	return SFR(p, base + reg);
}

/* the host's cycle counter is current inside board callbacks; lowering it ends the run there */
static void host_enter(pic32mx *p)
{
	if (p->icount) *p->icount = (int)((int64_t)p->run_end - (int64_t)p->cpu.cycles);
}

static void host_leave(pic32mx *p)
{
	int left;
	if (!p->icount) return;
	left = (int)((int64_t)p->run_end - (int64_t)p->cpu.cycles);
	if (*p->icount >= left) return;
	p->run_end = p->cpu.cycles + (uint64_t)(*p->icount > 0 ? *p->icount : 0);
	p->cpu.stop = 1;
}

static void uart_write(pic32mx *p, int u, uint32_t reg, uint32_t old, uint32_t v)
{
	uint32_t base = uart_base[u];

	if (reg == 0x20) {
		if ((SFR(p, base) & CON_ON) && (SFR(p, base + 0x10) & (1u << 10))) {
			if (p->board.uart_tx) { host_enter(p); p->board.uart_tx(p->board.ctx, u + 1, (uint8_t)v, p->cpu.cycles); host_leave(p); }
			pic32mx_set_irq(p, uart_irq[u] + 2);
		}
	} else if (reg == 0x10 && !(old & (1u << 10)) && (v & (1u << 10)))
		pic32mx_set_irq(p, uart_irq[u] + 2);
}

static int i2c_pins(pic32mx *p, int m, int scl, int sda)
{
	int r;
	if (!p->board.i2c_pins) return sda;
	host_enter(p);
	r = p->board.i2c_pins(p->board.ctx, m + 1, scl, sda, p->cpu.cycles);
	host_leave(p);
	return r;
}

static int i2c_reg(uint32_t off, int *m, uint32_t *reg)
{
	int i;
	for (i = 0; i < PIC32MX_I2CS; i++)
		if (off >= i2c_base[i] && off < i2c_base[i] + 0x70) { *m = i; *reg = (off - i2c_base[i]) & 0xF0u; return 1; }
	return 0;
}

static void i2c_finish_later(pic32mx *p, int m, int bits, uint32_t con_clear, uint32_t stat_set, uint32_t stat_clear)
{
	pic32mx_i2c *q = &p->i2c[m];
	uint32_t brg = SFR(p, i2c_base[m] + 0x40) & 0xFFFu;
	q->pending = 1;
	q->done_at = p->cpu.cycles + (uint64_t)bits * 2 * (brg + 2) * pbdiv(p);
	q->con_clear = con_clear;
	q->stat_set = stat_set;
	q->stat_clear = stat_clear;
	p->cpu.stop = 1;
}

static void i2c_complete(pic32mx *p, int m)
{
	pic32mx_i2c *q = &p->i2c[m];
	uint32_t base = i2c_base[m];
	q->pending = 0;
	SFR(p, base) &= ~q->con_clear;
	SFR(p, base + 0x10) = (SFR(p, base + 0x10) & ~q->stat_clear) | q->stat_set;
	if (q->stat_set & 2u) SFR(p, base + 0x60) = q->rcv;
	pic32mx_set_irq(p, i2c_irq[m] + 2);
}

static void i2c_sync(pic32mx *p)
{
	int m;
	for (m = 0; m < PIC32MX_I2CS; m++)
		if (p->i2c[m].pending && p->cpu.cycles >= p->i2c[m].done_at) i2c_complete(p, m);
}

static uint64_t next_i2c_event(const pic32mx *p)
{
	uint64_t best = ~(uint64_t)0;
	int m;
	for (m = 0; m < PIC32MX_I2CS; m++)
		if (p->i2c[m].pending) {
			uint64_t c = p->i2c[m].done_at > p->cpu.cycles ? p->i2c[m].done_at - p->cpu.cycles : 0;
			if (c < best) best = c;
		}
	return best;
}

static void i2c_write(pic32mx *p, int m, uint32_t reg, uint32_t v)
{
	uint32_t base = i2c_base[m], con = SFR(p, base);
	int i, b, ack;

	if (!(con & CON_ON)) return;
	if (p->i2c[m].pending) {
		uint32_t col = reg == 0x00 ? v & 0x1Fu & ~p->i2c[m].con_clear : reg == 0x50 ? 1u : 0;
		if (reg == 0x00) SFR(p, base) &= ~col;
		if (col) SFR(p, base + 0x10) |= 1u << 7;
		return;
	}
	if (reg == 0x50) {
		for (i = 7; i >= 0; i--) {
			b = (int)((v >> i) & 1);
			i2c_pins(p, m, 0, b); i2c_pins(p, m, 1, b); i2c_pins(p, m, 0, b);
		}
		i2c_pins(p, m, 0, 1);
		ack = i2c_pins(p, m, 1, 1);
		i2c_pins(p, m, 0, 1);
		SFR(p, base + 0x10) |= (1u << 14) | 1u;
		i2c_finish_later(p, m, 9, 0, ack ? (1u << 15) : 0, (1u << 14) | 1u | (ack ? 0 : (1u << 15)));
		return;
	}
	if (reg != 0x00) return;
	if (con & 1u) {
		i2c_pins(p, m, 1, 1); i2c_pins(p, m, 1, 0); i2c_pins(p, m, 0, 0);
		i2c_finish_later(p, m, 1, 1u, 1u << 3, 1u << 4);
	} else if (con & 2u) {
		i2c_pins(p, m, 0, 1); i2c_pins(p, m, 1, 1); i2c_pins(p, m, 1, 0); i2c_pins(p, m, 0, 0);
		i2c_finish_later(p, m, 1, 2u, 1u << 3, 0);
	} else if (con & 4u) {
		i2c_pins(p, m, 0, 0); i2c_pins(p, m, 1, 0); i2c_pins(p, m, 1, 1);
		i2c_finish_later(p, m, 1, 4u, 1u << 4, 1u << 3);
	} else if (con & 8u) {
		uint32_t byte = 0;
		for (i = 0; i < 8; i++) {
			i2c_pins(p, m, 0, 1);
			byte = (byte << 1) | (uint32_t)i2c_pins(p, m, 1, 1);
		}
		i2c_pins(p, m, 0, 1);
		p->i2c[m].rcv = byte;
		i2c_finish_later(p, m, 8, 8u, 2u, 0);
	} else if (con & 16u) {
		b = (con & 32u) ? 1 : 0;
		i2c_pins(p, m, 0, b); i2c_pins(p, m, 1, b); i2c_pins(p, m, 0, b);
		i2c_finish_later(p, m, 1, 16u, 0, 0);
	}
}

static uint32_t port_read(pic32mx *p, uint32_t off)
{
	int port = (int)((off - OFF_TRISA) >> 6);
	uint32_t reg = (off - OFF_TRISA) & 0x30u, tris = SFR(p, OFF_TRISA + (uint32_t)port * 0x40);
	if (reg == 0x10) {
		uint32_t in = 0xFFFFu;
		p->unc = 0;
		if (p->board.port_read) { host_enter(p); in = p->board.port_read(p->board.ctx, port, p->cpu.cycles); host_leave(p); }
		if (port == PIC32MX_PORTB) { in &= SFR(p, OFF_AD1PCFG); p->unc &= SFR(p, OFF_AD1PCFG); }
		p->unc &= tris;
		return (SFR(p, OFF_TRISA + (uint32_t)port * 0x40 + 0x20) & ~tris) | (in & tris);
	}
	return SFR(p, off & ~0xFu);
}

static void port_write(pic32mx *p, uint32_t off, uint32_t old, uint32_t v)
{
	int port = (int)((off - OFF_TRISA) >> 6);
	uint32_t base = OFF_TRISA + (uint32_t)port * 0x40;

	if (((off - OFF_TRISA) & 0x30u) == 0x30 || old == v) return;
	if (!p->board.port_write) return;
	host_enter(p);
	p->board.port_write(p->board.ctx, port, SFR(p, base + 0x20), SFR(p, base), p->cpu.cycles);
	host_leave(p);
}

static int is_known(uint32_t off)
{
	unsigned i;
	for (i = 0; i < sizeof(known) / sizeof(known[0]); i++)
		if (off >= known[i].lo && off < known[i].hi) return 1;
	return 0;
}

static void log_unmapped(pic32mx *p, uint32_t off, int write)
{
	if (p->logged[off >> 4]) return;
	p->logged[off >> 4] = 1;
	if (p->board.unmapped) p->board.unmapped(p->board.ctx, PIC32MX_SFR_BASE + off, write);
}

static uint32_t sfr_read(pic32mx *p, uint32_t off)
{
	int i;
	uint32_t reg;

	if (!is_known(off)) { log_unmapped(p, off, 0); return 0; }
	if (off & 0xCu) return 0;
	if (timer_reg(off, &i, &reg)) return timer_read(p, i, reg);
	if (uart_reg(off, &i, &reg)) return uart_read(p, i, reg);
	if (i2c_reg(off, &i, &reg) && reg == 0x60) SFR(p, i2c_base[i] + 0x10) &= ~2u;
	if (off >= OFF_TRISA && off < OFF_TRISA + PIC32MX_PORTS * 0x40) return port_read(p, off);
	return SFR(p, off);
}

static void sfr_write(pic32mx *p, uint32_t off, uint32_t v)
{
	uint32_t reg = off & ~0xFu, old, nv;
	int i;
	uint32_t sub;

	if (!is_known(off)) { log_unmapped(p, off, 1); return; }
	if (timer_reg(reg, &i, &sub)) { timers_sync(p); p->cpu.stop = 1; }
	if (reg >= OFF_TRISA && reg < OFF_TRISA + PIC32MX_PORTS * 0x40 && (reg & 0x30u) == 0x10) reg += 0x10;
	old = SFR(p, reg);
	switch (off & 0xCu) {
	case 0x0: nv = v; break;
	case 0x4: nv = old & ~v; break;
	case 0x8: nv = old | v; break;
	default:  nv = old ^ v; break;
	}
	if (reg >= OFF_TRISA && reg < OFF_TRISA + PIC32MX_PORTS * 0x40) {
		SFR(p, reg) = nv;
		port_write(p, reg, old, nv);
		return;
	}
	if (reg == OFF_INTSTAT || reg == OFF_RCON + 0x0) { SFR(p, reg) = nv; return; }
	SFR(p, reg) = nv;
	if (timer_reg(reg, &i, &sub)) {
		if (sub == 0x10) {
			if (timer_slave(p, i)) p->timer[i - 1].tmr = (p->timer[i - 1].tmr & 0xFFFFu) | (nv << 16);
			else p->timer[i].tmr = timer_t32(p, i) ? (p->timer[i].tmr & 0xFFFF0000u) | (nv & 0xFFFFu) : nv & 0xFFFFu;
		}
		return;
	}
	if (uart_reg(reg, &i, &sub)) { uart_write(p, i, sub, old, nv); return; }
	if (i2c_reg(reg, &i, &sub)) { i2c_write(p, i, sub, nv); return; }
	if (reg >= OFF_INTCON && reg < OFF_INTCON + 0x1000) update_eic(p);
}

static uint8_t *mem_ptr(pic32mx *p, uint32_t pa, int size, int write)
{
	if (pa < PIC32MX_RAM_SIZE && PIC32MX_RAM_SIZE - pa >= (uint32_t)size) return p->ram + pa;
	if (write) return NULL;
	if (pa >= 0x1D000000u && pa - 0x1D000000u < PIC32MX_FLASH_SIZE) {
		uint32_t o = pa - 0x1D000000u;
		return o + (uint32_t)size <= p->flash_size ? (uint8_t *)p->flash + o : NULL;
	}
	if (pa >= 0x1FC00000u && pa - 0x1FC00000u + (uint32_t)size <= PIC32MX_BOOT_SIZE) return p->boot + (pa - 0x1FC00000u);
	return NULL;
}

static uint32_t bus_read(void *ctx, uint32_t pa, int size, int fetch, int *err)
{
	pic32mx *p = (pic32mx *)ctx;
	uint8_t *m;
	uint32_t v = 0;
	int i;

	(void)fetch;
	if (pa >= PIC32MX_SFR_BASE && pa - PIC32MX_SFR_BASE < PIC32MX_SFR_SIZE) {
		uint32_t off = pa - PIC32MX_SFR_BASE, sm = size == 4 ? 0xFFFFFFFFu : (1u << (size * 8)) - 1;
		v = (sfr_read(p, off & ~3u) >> ((off & 3) * 8)) & sm;
		if (p->unc) {
			uint32_t m = (p->unc >> ((off & 3) * 8)) & sm;
			p->unc = 0;
			if (m) mips32_uncertain(&p->cpu, m, p->unc_tok << 2 | (off & 3));
		}
		return v;
	}
	m = mem_ptr(p, pa, size, 0);
	if (!m) {
		if (pa >= 0x1D000000u && pa - 0x1D000000u < PIC32MX_FLASH_SIZE) return 0xFFFFFFFFu;
		*err = 1;
		return 0;
	}
	for (i = 0; i < size; i++) v |= (uint32_t)m[i] << (8 * i);
	return v;
}

static uint32_t merge_base(pic32mx *p, uint32_t reg)
{
	int i;
	uint32_t sub;
	if (reg >= OFF_TRISA && reg < OFF_TRISA + PIC32MX_PORTS * 0x40 && (reg & 0x30u) == 0x10) return SFR(p, reg + 0x10);
	if (timer_reg(reg, &i, &sub) && sub == 0x10) return timer_read(p, i, sub);
	return SFR(p, reg);
}

static void bus_write(void *ctx, uint32_t pa, uint32_t data, int size, int *err)
{
	pic32mx *p = (pic32mx *)ctx;
	uint8_t *m;
	int i;

	if (pa >= PIC32MX_SFR_BASE && pa - PIC32MX_SFR_BASE < PIC32MX_SFR_SIZE) {
		uint32_t off = pa - PIC32MX_SFR_BASE, sh = (off & 3) * 8;
		if (size < 4) {
			uint32_t mask = ((1u << (size * 8)) - 1) << sh;
			if (!(off & 0xCu)) data = (merge_base(p, off & ~0xFu) & ~mask) | ((data << sh) & mask);
			else data = (data << sh) & mask;
		}
		sfr_write(p, off & ~3u, data);
		return;
	}
	m = mem_ptr(p, pa, size, 1);
	if (!m) { *err = 1; return; }
	for (i = 0; i < size; i++) m[i] = (uint8_t)(data >> (8 * i));
}

static int exc_hook(void *ctx, mips32_state *s, int code)
{
	pic32mx *p = (pic32mx *)ctx;
	if (code != MIPS32_EXC_INT) {
		p->exc_count++;
		if (p->board.exception) p->board.exception(p->board.ctx, code, s->cur_pc);
	}
	return MIPS32_HOOK_DELIVER;
}

static void irq_taken(void *ctx, int vector)
{
	pic32mx *p = (pic32mx *)ctx;
	if (vector >= 0 && vector < PIC32MX_VECTORS) p->vec_count[vector]++;
	SFR(p, OFF_INTSTAT) = ((uint32_t)p->cpu.eic_ripl << 8) | (uint32_t)vector;
}

void pic32mx_reset(pic32mx *p)
{
	static const uint32_t stub[4] = { 0x3C1A9D00u, 0x375A1000u, 0x03400008u, 0x00000000u };
	int i, j;

	memset(p->sfr, 0, sizeof(p->sfr));
	memset(p->timer, 0, sizeof(p->timer));
	memset(p->uart, 0, sizeof(p->uart));
	memset(p->i2c, 0, sizeof(p->i2c));
	memset(p->boot, 0xFF, sizeof(p->boot));
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) p->boot[i * 4 + j] = (uint8_t)(stub[i] >> (8 * j));
	SFR(p, OFF_OSCCON) = 0x00053320u;
	SFR(p, OFF_RCON) = 0x00000003u;
	for (i = 0; i < PIC32MX_PORTS; i++) SFR(p, OFF_TRISA + (uint32_t)i * 0x40) = 0xFFFFu;
	for (i = 0; i < PIC32MX_UARTS; i++) SFR(p, uart_base[i] + 0x10) = 0x110u;
	mips32_reset(&p->cpu);
}

void pic32mx_uncertain(pic32mx *p, uint32_t mask, uint32_t token)
{
	p->unc = mask;
	p->unc_tok = token;
}

static int soc_settle(void *ctx, uint32_t token, int wait, uint32_t *bits)
{
	pic32mx *p = (pic32mx *)ctx;
	uint32_t in = 0;
	int r;
	if (!p->board.port_settle) return 1;
	/* token = the read's token (its low 30 bits) << 2 | byte: the whole token is the latest one's or before it */
	r = p->board.port_settle(p->board.ctx, p->unc_tok - ((p->unc_tok - (token >> 2)) & 0x3FFFFFFFu), wait, &in);
	*bits = in >> ((token & 3) * 8);
	return r;
}

void pic32mx_init(pic32mx *p, const pic32mx_board *board, const uint8_t *flash, uint32_t flash_size)
{
	mips32_bus bus;

	memset(p, 0, sizeof(*p));
	memset(&bus, 0, sizeof(bus));
	if (board) p->board = *board;
	p->flash = flash;
	p->flash_size = flash_size > PIC32MX_FLASH_SIZE ? PIC32MX_FLASH_SIZE : flash_size;
	bus.ctx = p;
	bus.read = bus_read;
	bus.write = bus_write;
	bus.exc_hook = exc_hook;
	bus.irq_taken = irq_taken;
	bus.settle = soc_settle;
	mips32_init(&p->cpu, &bus, 2, 0x00018700u);
	mips32_direct(&p->cpu, 0, 0x1D000000u, p->flash_size, p->flash, NULL);
	mips32_direct(&p->cpu, 1, 0, PIC32MX_RAM_SIZE, p->ram, p->ram);
	mips32_direct(&p->cpu, 2, 0x1FC00000u, PIC32MX_BOOT_SIZE, p->boot, NULL);
	pic32mx_reset(p);
}

int pic32mx_run(pic32mx *p, int cycles)
{
	uint64_t start = p->cpu.cycles;

	p->run_end = start + (uint64_t)(cycles > 0 ? cycles : 0);
	while (p->cpu.cycles < p->run_end) {
		uint64_t slice = p->run_end - p->cpu.cycles, ev = next_timer_event(p), e2 = next_i2c_event(p);
		uint64_t h = p->board.hold ? p->board.hold(p->board.ctx, p->cpu.cycles) : 0;
		if (h) {
			p->cpu.cycles += h < slice ? h : slice;
			continue;
		}
		if (e2 < ev) ev = e2;
		if (ev < slice) slice = ev ? ev : 1;
		mips32_run(&p->cpu, (int)slice);
		timers_sync(p);
		i2c_sync(p);
		if (mips32_timer_irq(&p->cpu)) pic32mx_set_irq(p, 0);
	}
	host_enter(p);
	return (int)(p->cpu.cycles - start);
}

uint32_t pic32mx_sfr_peek(const pic32mx *p, uint32_t va)
{
	uint32_t off = (va & 0x1FFFFFFFu) - PIC32MX_SFR_BASE;
	return off < PIC32MX_SFR_SIZE ? p->sfr[off >> 2] : 0;
}
