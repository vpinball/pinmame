// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*
   GI SP0250 digital LPC sound synthesizer

   By O. Galibert.

   Unimplemented:
   - Direct Data test mode (pin 7)
*/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"

/*
standard external clock is 3.12MHz
the chip provides a 445.7kHz output clock, which is = 3.12MHz / 7
therefore I expect the clock divider to be a multiple of 7
Also there are 6 cascading filter stages so I expect the divider to be a multiple of 6.

The SP0250 manual states that the original speech is sampled at 10kHz, so the divider
should be 312, but 312 = 39*8 so it doesn't look right because a divider by 39 is unlikely.

7*6*8 = 336 gives a 9.286kHz sample rate and matches the samples from the Sega boards.
*/
#ifdef PINMAME
#define CLOCK_DIVIDER 312
#else
#define CLOCK_DIVIDER (7*6*8)
#endif

static struct {
	INT16 amp;
	UINT8 pitch;
	UINT8 repeat;
	int pcount, rcount;
	UINT16 lfsr;
	int stream;
	int voiced;
	UINT8 fifo[15];
	int fifo_pos;

	void (*drq)(int state);

	struct {
		INT16 F, B;
		INT16 z1, z2;
	} filter[6];
} sp0250;

static void sp0250_timer_tick(int param);
static void sp0250_update(int num, INT16* output, int length);

int sp0250_sh_start( const struct MachineSound *msound )
{
	struct sp0250_interface *intf = msound->sound_interface;
	memset(&sp0250, 0, sizeof(sp0250));
	sp0250.lfsr = 0x7fff;
	sp0250.drq = intf->drq_callback;
	sp0250.drq(ASSERT_LINE);
	timer_pulse(TIME_IN_HZ(3120000 / CLOCK_DIVIDER), 0, sp0250_timer_tick);

	sp0250.stream = stream_init("SP0250", intf->volume, 3120000. / CLOCK_DIVIDER, 0, sp0250_update);

	return 0;
}

static UINT16 sp0250_ga(UINT8 v)
{
	return (v & 0x1f) << (v>>5);
}

// Internal ROM to the chip, cf. manual
static const UINT16 coefs[128] = {
	  0,   9,  17,  25,  33,  41,  49,  57,  65,  73,  81,  89,  97, 105, 113, 121,
	129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 203, 217, 225, 233, 241, 249,
	257, 265, 273, 281, 289, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337,
	341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381, 385, 389, 393, 397, 401,
	405, 409, 413, 417, 421, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445,
	447, 449, 451, 453, 455, 457, 459, 461, 463, 465, 467, 469, 471, 473, 475, 477,
	479, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495,
	496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511
};

static INT16 sp0250_gc(UINT8 v)
{
	INT16 res = coefs[v & 0x7f];
	if(!(v & 0x80))
		res = -res;
	return res;
}

static void sp0250_load_values(void)
{
    int f;

	sp0250.filter[0].B = sp0250_gc(sp0250.fifo[ 0]);
	sp0250.filter[0].F = sp0250_gc(sp0250.fifo[ 1]);
	sp0250.amp         = sp0250_ga(sp0250.fifo[ 2]);
	sp0250.filter[1].B = sp0250_gc(sp0250.fifo[ 3]);
	sp0250.filter[1].F = sp0250_gc(sp0250.fifo[ 4]);
	sp0250.pitch       =           sp0250.fifo[ 5];
	sp0250.filter[2].B = sp0250_gc(sp0250.fifo[ 6]);
	sp0250.filter[2].F = sp0250_gc(sp0250.fifo[ 7]);
	sp0250.repeat      =           sp0250.fifo[ 8] & 0x3f;
	sp0250.voiced      =           sp0250.fifo[ 8] & 0x40;
	sp0250.filter[3].B = sp0250_gc(sp0250.fifo[ 9]);
	sp0250.filter[3].F = sp0250_gc(sp0250.fifo[10]);
	sp0250.filter[4].B = sp0250_gc(sp0250.fifo[11]);
	sp0250.filter[4].F = sp0250_gc(sp0250.fifo[12]);
	sp0250.filter[5].B = sp0250_gc(sp0250.fifo[13]);
	sp0250.filter[5].F = sp0250_gc(sp0250.fifo[14]);
	sp0250.fifo_pos = 0;
	sp0250.drq(ASSERT_LINE);
	sp0250.pcount = 0;
	sp0250.rcount = 0;

	for (f = 0; f < 6; f++)
		sp0250.filter[f].z1 = sp0250.filter[f].z2 = 0;
}

WRITE_HANDLER( sp0250_w )
{
	stream_update(sp0250.stream, 0);
	if(sp0250.fifo_pos != 15)
	{
		sp0250.fifo[sp0250.fifo_pos++] = data;
		if(sp0250.fifo_pos == 15)
			sp0250.drq(CLEAR_LINE);
	}
}

static void sp0250_timer_tick(int param)
{
	stream_update(sp0250.stream, 0);
}

static void sp0250_update(int num, INT16 *output, int length)
{
	int i;
	for(i=0; i<length; i++) {
		if (sp0250.rcount >= sp0250.repeat)
		{
			if(sp0250.fifo_pos == 15)
				sp0250_load_values();
			else
			{
				// According to http://www.cpcwiki.eu/index.php/SP0256_Measured_Timings
				// the SP0250 executes "NOPs" with a repeat count of 1 and unchanged
				// pitch while waiting for input
				sp0250.repeat = 1;
				sp0250.pcount = 0;
				sp0250.rcount = 0;
			}
		}

		// 15-bit LFSR algorithm verified by dump from actual hardware
		// clocks every cycle regardless of voiced/unvoiced setting
		sp0250.lfsr ^= (sp0250.lfsr ^ (sp0250.lfsr >> 1)) << 15;
		sp0250.lfsr >>= 1;

		INT16 z0;
		int f;

		if (sp0250.voiced)
			z0 = (sp0250.pcount == 0) ? sp0250.amp : 0;
		else
			z0 = (sp0250.lfsr & 1) ? sp0250.amp : -sp0250.amp;

		for (f = 0; f < 6; f++) {
			z0 += ((sp0250.filter[f].z1 * sp0250.filter[f].F) >> 8)
				+ ((sp0250.filter[f].z2 * sp0250.filter[f].B) >> 9);
			sp0250.filter[f].z2 = sp0250.filter[f].z1;
			sp0250.filter[f].z1 = z0;
		}

#if 0
		// maximum amp value is effectively 13 bits
		// reduce to 7 bits; due to filter effects it
		// may occasionally clip
		int dac = z0 >> 6;
		if (dac < -64)
			dac = -64;
		if (dac > 63)
			dac = 63;
		output[i] = dac << 9;
#else
		// Physical resolution is only 7 bits, but heh
		// max amplitude is 0x0f80 so we have margin to push up the output
		int dac = z0 << 3;
		if (dac < -32768)
			dac = -32768;
		if (dac > 32767)
			dac = 32767;
		output[i] = dac;
#endif

		if(sp0250.pcount++ == sp0250.pitch)
		{
			sp0250.pcount = 0;
			sp0250.rcount++;
		}
	}
}

void sp0250_sh_stop( void )
{
}

