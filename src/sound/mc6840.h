// license:BSD-3-Clause
// copyright-holders:Aaron Giles, James Wallace

#pragma once

/*
    Motorola MC6840 programmable timer module (PTM), clocked from a sound stream.

    Written for PinMAME after MAME's BSD-3-Clause sources: the sh6840 code of
    src/mame/shared/exidysound.cpp (Aaron Giles) and src/devices/machine/6840ptm.cpp
    (James Wallace, after Aaron Giles, 'Re-Animator' and Mathis Rosenhauer).

    The chip has no timer of its own here: the owner calls mc6840_clock() from its
    stream update with the number of E (and external) clock edges of each output
    sample, and gets back how long each output stayed high during that sample, so
    the output can be box filtered instead of point sampled.
    Supported: continuous and single shot modes, 16 bit and dual 8 bit counting,
    internal (E) or external clock per timer, timer 3 prescaler, gate inputs,
    internal reset, status register and interrupt flags, counter read back.
    In the frequency/pulse width comparison modes the counters run and flag their
    time-outs, but the gate measurement itself is not done (the outputs stay low,
    as in MAME).
*/

// 1: the Stern SB-300 (MP-200 games) and Game Plan MSU-1 sound boards run this 6840
// in the sound stream (src/wpc/stsnd.c, gpsnd.c), 0: the old sample based emulation
#define MC6840_SOUND 0

struct mc6840_timer {
	UINT8  cr;       /* control register */
	UINT16 latch;    /* timer latch */
	UINT16 counter;
	UINT8  out;      /* output level before the CRx7 output enable */
	UINT8  armed;    /* single shot: the output pulse is still to come */
	UINT8  first;    /* single shot: first clock after initialization still to come */
	UINT8  gate;     /* gate input level (1 = high) */
};

struct mc6840 {
	struct mc6840_timer t[3];
	UINT8 status;       /* bits 0-2: interrupt flags, bit 7: composite interrupt */
	UINT8 status_read;  /* flags that were set when the status register was last read */
	UINT8 msb_buf;      /* MSB buffer (writes) */
	UINT8 lsb_buf;      /* LSB buffer (reads) */
	UINT8 prescale;     /* timer 3 divide by 8 prescaler state */
	void (*irq_cb)(int state);
};

/* Result of mc6840_clock() for one sample. high[] is in input clocks of each
   timer's source (E or external; before the timer 3 prescaler), so
   high[i] / clocks of that source = average output level over the sample.
   level[] is the (output enabled) level at the end of the sample, rises[] the
   number of low to high edges on the output pin during the sample. */
struct mc6840_clock_result {
	int high[3];
	int level[3];
	int rises[3];
};

void mc6840_reset(struct mc6840 *p, void (*irq_cb)(int state));
void mc6840_w(struct mc6840 *p, int offset, int data);
int  mc6840_r(struct mc6840 *p, int offset);
void mc6840_set_gate(struct mc6840 *p, int idx, int state);
/* advance by e_clocks E clocks and ext_clocks[i] external clock edges on C1..C3 (ext_clocks may be NULL) */
void mc6840_clock(struct mc6840 *p, int e_clocks, const int *ext_clocks, struct mc6840_clock_result *r);
/* output pin level of a timer (0/1, with the CRx7 output enable) */
int  mc6840_output(const struct mc6840 *p, int idx);
