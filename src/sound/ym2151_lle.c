/***************************************************************************

	YM2151-LLE glue

	Wraps nukeykt's YM2151-LLE (fmopm.c/.h, https://github.com/nukeykt/YM2151-LLE),
	a die-shot derived, pin level emulation of the OPM, so that 2151intf.c can drive
	it like the other cores. Selected at compile time with HAS_YM2151_LLE.

	*** fmopm.c/.h are GPLv2, unlike the rest of PinMAME. Building with
	*** HAS_YM2151_LLE makes the resulting binary GPLv2. It is off by default.

	fmopm.c exports exactly one function, FMOPM_Clock(chip, clk) - there is no
	sample output and no register interface. Everything goes through the pins:
	set chip->input.*, call FMOPM_Clock twice (clk 0 then 1) per master clock, and
	read chip->o_*. So this file has to provide, in order:

	  - a reset that holds /IC low for long enough,
	  - bus write cycles that hold /CS, /WR, A0 and the data long enough to latch,
	  - a way to get samples out (see lle_sample() for why that is not o_so),
	  - a status read (see ym2151lle_read_status()),
	  - IRQ and CT1/CT2 forwarding to the handlers 2151intf.c installs.

	Cost: the FSM is 32 slots at the internal clock, which is the master clock divided
	by two, so one output sample is 64 master clocks. FMOPM_Clock takes one clock
	*phase*, so that is 128 calls per sample, ~7.2M/s at 3.579545MHz - but half of them
	hit the early-out at the top of FMOPM_Clock (the internal clock only moves on one
	phase), leaving ~3.6M real evaluations.

	Measured against Nuked on the same machine, one chip, 200k samples, x64 /O2:
	Nuked 0.39s (9.1x realtime, ~11% of a core), this 0.82s (4.4x realtime, ~23%).
	So roughly twice the cost of Nuked, not the order of magnitude the "low level"
	label suggests

***************************************************************************/

#include "driver.h"
#include "ym2151_lle.h"
#include "fmopm.h"
#include "fmopm.c"

#define LLE_LEFT  0
#define LLE_RIGHT 1

/* Master clocks to hold a write on the bus. The chip latches wr0/wr1 through write*_l[0..2],
   two internal clocks, i.e. four master clocks; this is double that for margin. It also sets
   how fast the queue drains, since one write occupies the bus for this long - at 8 that is up
   to 8 writes per 64 clock sample, comfortably ahead of any sound CPU */
#define LLE_WRITE_HOLD 8

#define LLE_WRITEBUF 256 /* a full patch load is ~60 entries; drained up to 8 per sample */

static struct {
	fmopm_t chip;

	struct { UINT8 a0, data; } wbuf[LLE_WRITEBUF];
	int wbuf_rd, wbuf_wr;
	int write_hold;		/* master clocks left with the write pins asserted */

	INT16  smp[2];		/* latest L/R sample, see lle_sample() */

	void (*irqhandler)(int irq);
	mem_write_handler porthandler;
	int prev_irq, prev_ct1, prev_ct2;
} lle[MAX_2151];

/*-------------------------------------------------
	pin level helpers
-------------------------------------------------*/

/* one master clock: the core wants both phases */
INLINE void lle_clock(int num)
{
	FMOPM_Clock(&lle[num].chip, 0);
	FMOPM_Clock(&lle[num].chip, 1);
}

INLINE void lle_idle_pins(int num)
{
	lle[num].chip.input.cs = 1;
	lle[num].chip.input.wr = 1;
	lle[num].chip.input.rd = 1;
}

void ym2151lle_init(int num, int is_ym2164)
{
	int i;

	memset(&lle[num], 0, sizeof(lle[num]));

	lle[num].chip.input.ym2164 = is_ym2164 ? 1 : 0;
	lle_idle_pins(num);
	lle[num].chip.input.a0 = 0;
	lle[num].chip.input.data = 0;

	/* /IC low resets the chip. It is active low and also forces the register write
	   strobe (see wr0 in FMOPM_Clock), so it has to be held for a full FSM pass to
	   clear everything - 64 master clocks is one pass, give it several */
	lle[num].chip.input.ic = 0;
	for (i = 0; i < 64 * 8; i++)
		lle_clock(num);
	lle[num].chip.input.ic = 1;
	for (i = 0; i < 64; i++)
		lle_clock(num);

	lle[num].prev_irq = 0;
	lle[num].prev_ct1 = 0;
	lle[num].prev_ct2 = 0;
}

void ym2151lle_reset(int num)
{
	void (*irq)(int) = lle[num].irqhandler;
	mem_write_handler port = lle[num].porthandler;
	const int is_opp = lle[num].chip.input.ym2164;

	ym2151lle_init(num, is_opp);

	lle[num].irqhandler = irq;
	lle[num].porthandler = port;
}

void ym2151lle_set_handlers(int num, void (*irqhandler)(int irq), mem_write_handler porthandler)
{
	lle[num].irqhandler = irqhandler;
	lle[num].porthandler = porthandler;
}

/* Queued rather than applied here: a write has to be held on the bus across several
   master clocks, and the only place the chip is clocked is the generate loop below */
void ym2151lle_write(int num, UINT8 a0, UINT8 data)
{
	const int next = (lle[num].wbuf_wr + 1) % LLE_WRITEBUF;

	if (next == lle[num].wbuf_rd)
		return; /* full - the CPU is writing faster than we can clock the chip out */

	lle[num].wbuf[lle[num].wbuf_wr].a0 = a0 & 1;
	lle[num].wbuf[lle[num].wbuf_wr].data = data;
	lle[num].wbuf_wr = next;
}

/* The status register is read through the pins too, but driving a /RD cycle costs as many
   clocks as a write and the CPU polls this hard while waiting for BUSY. So read the same
   flip-flops the chip's own read path uses - see the read_bus assembly in fmopm.c, which
   takes the [0] stage of each, not [1]. Bits 2..6 only carry anything in test mode */
UINT8 ym2151lle_read_status(int num)
{
	const fmopm_t * const c = &lle[num].chip;

	return (UINT8)((c->busy_cnt_en[0]    << 7)
	             | (c->timer_b_status[0] << 1)
	             |  c->timer_a_status[0]);
}

/*-------------------------------------------------
	output
-------------------------------------------------*/

/* Where the sample is taken from, and why it is not o_so.

   The chip does emit the finished sample serially on o_so, framed by o_sh1/o_sh2, in the
   YM3012 float format. Decoding that from the pins turned out not to be a single flat
   frame: the 16 bit value goes through accm_l_shifter -> accm_l_bit -> accm_lrbit into a
   second, 21 bit shifter that applies its own clamping, and only then is the sign/exponent/
   mantissa word assembled. A frame decoder written against the obvious reading of that does
   not round trip - it was tried and checked against the chip, and did not reproduce the
   value the chip had just loaded.

   So take the sample where the chip itself computes it, at the load strobe into the output
   shifter (fmopm.c, the accm_clear_l/r blocks). That is the exact digital sample the DAC is
   handed, in offset binary with an inverted MSB - positive values land in the upper half -
   hence the -0x8000. The only thing given up is the YM3012's own float quantisation, which
   neither the Nuked nor the ymfm core models either, so this stays comparable with them.

   If the serial path is ever wanted, decode it against this value as ground truth.

   Checked with a harness that drives this same pin sequence and keys on one channel with
   KC=0x4A, i.e. octave 4 note A: output rises from silence through the attack, is symmetric
   at +/-8168, and crosses zero 125 times in 7980 samples = 438Hz against the 440Hz the note
   should be. So the reset, the write cycles and this conversion are all doing the right thing */
INLINE INT16 lle_sample(INT32 accm)
{
	INT32 v = accm & 0x7fff;
	if ((accm & 0x20000) == 0)
		v |= 0x8000;
	v -= 0x8000;

	if (v >  32767) v =  32767;
	if (v < -32768) v = -32768;
	return (INT16)v;
}

void ym2151lle_generate(int num, INT16 **buffers, int length)
{
	int j;

	for (j = 0; j < length; j++)
	{
		int k;

		for (k = 0; k < 64; k++) /* 64 master clocks make one output sample */
		{
			/* Start the next queued write as soon as the bus goes free, rather than once per
			   sample: a patch load is 30-odd register pairs, which the sound CPU emits far
			   faster than 60 samples, so one per sample would silently drop most of it. A
			   write occupies LLE_WRITE_HOLD clocks, so this drains up to 64/8 per sample */
			if (!lle[num].write_hold && lle[num].wbuf_rd != lle[num].wbuf_wr)
			{
				lle[num].chip.input.a0   = lle[num].wbuf[lle[num].wbuf_rd].a0;
				lle[num].chip.input.data = lle[num].wbuf[lle[num].wbuf_rd].data;
				lle[num].chip.input.cs   = 0;
				lle[num].chip.input.wr   = 0;
				lle[num].wbuf_rd = (lle[num].wbuf_rd + 1) % LLE_WRITEBUF;
				lle[num].write_hold = LLE_WRITE_HOLD;
			}

			/* the load strobes are asserted going into this clock, so read the accumulators
			   before advancing, exactly as the chip's own shifter load does */
			const int load_l = lle[num].chip.accm_clear_l[1] & 1;
			const int load_r = lle[num].chip.accm_clear_r[1] & 1;
			const INT32 accm_l = lle[num].chip.accm_l[2];
			const INT32 accm_r = lle[num].chip.accm_r[2];

			lle_clock(num);

			if (load_l) lle[num].smp[LLE_LEFT ] = lle_sample(accm_l);
			if (load_r) lle[num].smp[LLE_RIGHT] = lle_sample(accm_r);

			if (lle[num].write_hold && --lle[num].write_hold == 0)
				lle_idle_pins(num);

			/* IRQ and CT1/CT2 are pins too, so watch them for changes */
			if (lle[num].chip.o_irq_pull != lle[num].prev_irq)
			{
				lle[num].prev_irq = lle[num].chip.o_irq_pull;
				if (lle[num].irqhandler)
					lle[num].irqhandler(lle[num].prev_irq ? 1 : 0);
			}
			if (lle[num].chip.o_ct1 != lle[num].prev_ct1 || lle[num].chip.o_ct2 != lle[num].prev_ct2)
			{
				lle[num].prev_ct1 = lle[num].chip.o_ct1;
				lle[num].prev_ct2 = lle[num].chip.o_ct2;
				if (lle[num].porthandler) /* same packing as Nuked: bit 0 = CT1, bit 1 = CT2 */
					lle[num].porthandler(0, (UINT8)((lle[num].prev_ct1 & 1) | ((lle[num].prev_ct2 & 1) << 1)));
			}
		}

		buffers[LLE_LEFT ][j] = lle[num].smp[LLE_LEFT];
		buffers[LLE_RIGHT][j] = lle[num].smp[LLE_RIGHT];
	}
}
