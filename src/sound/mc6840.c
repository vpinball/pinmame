// license:BSD-3-Clause
// copyright-holders:Aaron Giles, James Wallace
/***************************************************************************

    Motorola MC6840 programmable timer module (PTM), clocked from a sound stream

    C port for PinMAME of the 6840 logic in MAME's src/mame/shared/exidysound.cpp
    (sh6840_*, Aaron Giles) and src/devices/machine/6840ptm.cpp (James Wallace,
    based on previous work by Aaron Giles, 'Re-Animator' and Mathis Rosenhauer),
    extended to all counting modes and to box filtered outputs. See mc6840.h.

    Counting follows the data sheet: a counter loaded with N times out on the
    (N+1)th clock and reloads from its latch.
    - continuous, 16 bit: the output toggles at each time-out (period 2(N+1)).
    - continuous, dual 8 bit (latch MSB M, LSB L): the LSB counts L+1 clocks, then
      the MSB decrements; the output is high while the MSB is 0, i.e. low for
      M(L+1) clocks and high for L+1 clocks (period (M+1)(L+1)).
    - single shot: same counting, the output goes high on the first clock after
      initialization and low at the first time-out, then stays low until the next
      initialization; the gate does not hold the counter.
    - comparison modes: the counter runs and flags its time-outs, output low.
    Counter initialization: write to the latch (if CRx4 = 0), gate falling edge,
    or release of the internal reset (CR10).

    This file is compiled through src/wpc/stsnd.c (as tms320c1x.c is through
    bsmt2000.c), so the build scripts don't need to know about it.

***************************************************************************/

#include "driver.h"
#include "mc6840.h"

#define CR_PRESCALE   0x01 /* CR3 */
#define CR_RESET      0x01 /* CR1 */
#define CR_SELECT     0x01 /* CR2: offset 0 writes CR1 (1) or CR3 (0) */
#define CR_INTCLK     0x02
#define CR_DUAL8      0x04
#define CR_NOINIT     0x10 /* CRx4: no initialization on a latch write (as MAME, also in the comparison modes) */
#define CR_IRQEN      0x40
#define CR_OUTEN      0x80

#define MODE(t)        (((t)->cr >> 3) & 7)
#define SINGLE_SHOT(t) (MODE(t) == 4 || MODE(t) == 6)
#define COMPARE(t)     (MODE(t) & 1)

static void update_irq(struct mc6840 *p)
{
	int i, irq = 0;
	const UINT8 old = p->status & 0x80;
	for (i = 0; i < 3; i++)
		if ((p->status & (1 << i)) && (p->t[i].cr & CR_IRQEN))
			irq = 1;
	p->status = (p->status & 0x07) | (irq ? 0x80 : 0);
	if ((p->status & 0x80) != old && p->irq_cb)
		p->irq_cb(irq);
}

static void set_flag(struct mc6840 *p, int idx)
{
	p->status_read &= ~(1 << idx);
	if (p->status & (1 << idx))
		return;
	p->status |= 1 << idx;
	if (p->t[idx].cr & CR_IRQEN)
		update_irq(p);
}

static void clear_flag(struct mc6840 *p, int idx)
{
	p->status &= ~(1 << idx);
	update_irq(p);
}

/* counter initialization: latch to counter, flag cleared */
static void init_counter(struct mc6840 *p, int idx, int from_gate_or_reset)
{
	struct mc6840_timer *t = &p->t[idx];
	t->counter = t->latch;
	if (SINGLE_SHOT(t)) {
		t->armed = 1;
		t->first = !(t->cr & CR_DUAL8);
		t->out = 0;
	}
	else if (from_gate_or_reset || (t->cr & CR_DUAL8))
		t->out = 0; /* dual 8 bit restarts with its low phase, as the counter does */
	clear_flag(p, idx);
}

void mc6840_reset(struct mc6840 *p, void (*irq_cb)(int state))
{
	int i;
	memset(p, 0, sizeof(*p));
	p->irq_cb = irq_cb;
	for (i = 0; i < 3; i++) {
		p->t[i].latch = 0xffff;
		p->t[i].counter = 0xffff;
	}
	p->t[0].cr = CR_RESET; /* the chip comes out of power on reset with the timers held */
	if (irq_cb)
		irq_cb(0);
}

int mc6840_output(const struct mc6840 *p, int idx)
{
	return (p->t[idx].cr & CR_OUTEN) ? p->t[idx].out : 0;
}

void mc6840_w(struct mc6840 *p, int offset, int data)
{
	switch (offset & 7) {
		case 0:
		case 1: {
			const int idx = (offset & 1) ? 1 : ((p->t[1].cr & CR_SELECT) ? 0 : 2);
			const UINT8 diffs = data ^ p->t[idx].cr;
			p->t[idx].cr = data;
			if (diffs & CR_IRQEN)
				update_irq(p);
			if (idx == 0 && (diffs & CR_RESET)) {
				int i;
				if (data & CR_RESET) {
					/* hold all the timers, preset to their latches */
					for (i = 0; i < 3; i++) {
						p->t[i].counter = p->t[i].latch;
						p->t[i].out = 0;
						p->t[i].armed = 0;
						p->t[i].first = 0;
					}
					p->prescale = 0;
				}
				else {
					for (i = 0; i < 3; i++)
						init_counter(p, i, 1);
				}
				p->status = 0;
				p->status_read = 0;
				update_irq(p);
			}
			break;
		}
		case 2: case 4: case 6:
			p->msb_buf = data;
			break;
		default: { /* 3, 5, 7 */
			const int idx = ((offset & 7) - 3) >> 1;
			struct mc6840_timer *t = &p->t[idx];
			t->latch = (p->msb_buf << 8) | (data & 0xff);
			clear_flag(p, idx);
			if (p->t[0].cr & CR_RESET)
				t->counter = t->latch;
			else if (!(t->cr & CR_NOINIT))
				init_counter(p, idx, 0);
			break;
		}
	}
}

int mc6840_r(struct mc6840 *p, int offset)
{
	switch (offset & 7) {
		case 0:
			return 0;
		case 1:
			p->status_read |= p->status & 0x07;
			return p->status;
		case 2: case 4: case 6: {
			const int idx = ((offset & 7) - 2) >> 1;
			if (p->status_read & (1 << idx)) {
				p->status_read &= ~(1 << idx);
				clear_flag(p, idx);
			}
			p->lsb_buf = p->t[idx].counter & 0xff;
			return p->t[idx].counter >> 8;
		}
		default:
			return p->lsb_buf;
	}
}

void mc6840_set_gate(struct mc6840 *p, int idx, int state)
{
	struct mc6840_timer *t = &p->t[idx];
	state = state ? 1 : 0;
	if (t->gate && !state && !(p->t[0].cr & CR_RESET))
		init_counter(p, idx, 1);
	t->gate = state;
}

/* advance one timer by n of its clocks; returns the clocks its output (before CRx7) was high */
static int count_clocks(struct mc6840 *p, int idx, int n, int *rises)
{
	struct mc6840_timer *t = &p->t[idx];
	const int single = SINGLE_SHOT(t);
	const int compare = COMPARE(t);
	const int outen = t->cr & CR_OUTEN;
	int high = 0;

	if (t->cr & CR_DUAL8) {
		while (n > 0) {
			const int lsb = t->counter & 0xff;
			int msb = t->counter >> 8;
			if (n <= lsb) {
				t->counter -= n;
				if (t->out) high += n;
				break;
			}
			n -= lsb + 1;
			if (t->out) high += lsb + 1;
			if (msb == 0) {
				/* end of the cycle: reload, output low */
				t->counter = t->latch;
				t->out = 0;
				if (single) t->armed = 0;
				set_flag(p, idx);
			}
			else {
				msb--;
				t->counter = (msb << 8) | (t->latch & 0xff);
				if (msb == 0 && !compare && (!single || t->armed) && !t->out) {
					t->out = 1;
					if (outen) (*rises)++;
				}
			}
		}
	}
	else {
		while (n > 0) {
			if (n <= t->counter) {
				t->counter -= n;
				if (t->out) high += n;
				break;
			}
			n -= t->counter + 1;
			if (t->out) high += t->counter + 1;
			t->counter = t->latch;
			set_flag(p, idx);
			if (compare)
				continue;
			if (single) {
				t->out = 0;
				t->armed = 0;
			}
			else {
				t->out ^= 1;
				if (t->out && outen) (*rises)++;
			}
		}
	}
	return high;
}

static int run_timer(struct mc6840 *p, int idx, int n, int *rises)
{
	struct mc6840_timer *t = &p->t[idx];
	int high = 0;
	if (n <= 0)
		return 0;
	/* 16 bit single shot: the output goes high after the first clock following
	   initialization (unless that clock is already the time-out, N = 0) */
	if (t->first) {
		t->first = 0;
		high = count_clocks(p, idx, 1, rises);
		n--;
		if (t->armed && !t->out) {
			t->out = 1;
			if (t->cr & CR_OUTEN) (*rises)++;
		}
	}
	return high + count_clocks(p, idx, n, rises);
}

void mc6840_clock(struct mc6840 *p, int e_clocks, const int *ext_clocks, struct mc6840_clock_result *r)
{
	int i;
	const int held = p->t[0].cr & CR_RESET;
	for (i = 0; i < 3; i++) {
		struct mc6840_timer *t = &p->t[i];
		const int n = (t->cr & CR_INTCLK) ? e_clocks : (ext_clocks ? ext_clocks[i] : 0);
		int high;
		r->rises[i] = 0;
		if (!held && !t->gate && !t->first && !(t->cr & (CR_DUAL8 | (i == 2 ? CR_PRESCALE : 0))) && n <= t->counter) {
			/* fast path: 16 bit, no time-out during this sample */
			t->counter -= n;
			r->high[i] = (t->out && (t->cr & CR_OUTEN)) ? n : 0;
			r->level[i] = mc6840_output(p, i);
			continue;
		}
		if (held || (t->gate && !SINGLE_SHOT(t)))
			high = t->out ? n : 0; /* not counting: the output keeps its level */
		else if (i == 2 && (t->cr & CR_PRESCALE)) {
			/* divide by 8 prescaler in front of timer 3 */
			const int total = p->prescale + n;
			const int out0 = t->out;
			p->prescale = total & 7;
			if (total >> 3) {
				high = run_timer(p, i, total >> 3, &r->rises[i]) * 8;
				if (high > n) high = n;
			}
			else
				high = out0 ? n : 0;
		}
		else
			high = run_timer(p, i, n, &r->rises[i]);
		r->high[i] = (t->cr & CR_OUTEN) ? high : 0;
		r->level[i] = mc6840_output(p, i);
	}
}
