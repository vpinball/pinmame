/***************************************************************************

	Philips SAA1099 Sound driver - Valley Bell's core

	Ported from libVGM, emu/cores/saa1099_vb.c (Valley Bell, 2018), commit a7215f49 (2021-11-06).

	Kept deliberately close to the original so it can be re-diffed later. The five
	generator functions below - saa_freq_gen_reset/_step, saa_advance, saa_envgen_load
	and saa_envgen_step - plus the envelope tables and both register write handlers are
	verbatim apart from libVGM's typedefs (SAA_CHIP* and friends) being spelled out, and
	the chip being addressed by index rather than by a void* handle.

	saa1099vb_update() is the one that had to be restructured, all of it forced by the
	interface, none of it by the algorithm:
	 - libVGM accumulates into DEV_SMPL (INT32) buffers, PinMAME hands us INT16 ones, so
	   we sum into per sample locals and clamp on store
	 - RC_STEP/RC_GET_VAL/RC_MASK are inlined as three statements instead of pulling in
	   RatioCntr.h
	 - declarations moved to suit the C style used here
	The mixing itself - the tone+noise state logic and the three noDCremove branches - is
	line for line the original.

	Not ported: the per channel mute mask (saaCh->muted, saa1099v_set_mute_mask), as
	PinMAME has no hook for it. If it is ever wanted, note that upstream tests it *after*
	the generators have run, so muting cannot desync anything. saa1099v_reset() is folded
	into saa1099vb_init(); create/destroy/device_start have no analogue here.

	This is a second, independent implementation of the chip, selectable at
	compile time via HAS_SAA1099_VB in pinmame.h - see saa1099.c for the
	dispatch, and 2151intf.c for the same pattern applied to the YM2151.

	Unlike the MAME derived core in saa1099.c, this one was written against
	NewRisingSun's test cases recorded from real hardware, and passes most of
	them. Valley Bell's own list of remaining issues:
	 - test 10 "Repeated triangle, continuously resetting frequency counters":
	   the envelope runs slower here than in the recording (this may equally be
	   a DOSBox vs. real machine timing difference)
	 - enabling the envelope generator produces tiny clicks on real hardware
	   (0 -> 1 -> 0 spikes); not emulated
	 - frequency/octave writes are cached in a particular way that is not
	   reproduced - SAASound documents it, and the Philips datasheet recommends
	   an ordering that avoids the issue
	 - at the start of NRS' test cases the first phase of the square tone is
	   often much longer than on real hardware, probably the same root cause

	The libVGM original is driven by a RATIO_CNTR so it can run at any output
	rate. We run the stream at clock/128, the chip's own step rate, which makes
	the ratio exactly 1:1 - one chip step per sample, no counter aliasing.

***************************************************************************/

#include "driver.h"
#include "saa1099.h"
#include "saa1099_vb.h"

#define LEFT	0x00
#define RIGHT	0x01

/*-- one frequency generator (tone, or a noise generator's own divider) --*/
struct saa_freq_gen
{
	INT16  cntr;
	INT16  limit;
	UINT8  state;
	UINT8  trigger;
};

struct saa_noise_gen
{
	UINT8  mode;
	UINT32 state;
	struct saa_freq_gen fgen;
};

struct saa_env_gen
{
	UINT8 enable;
	UINT8 reload;
	UINT8 extClock;
	UINT8 step;
	UINT8 wave;
	UINT8 invert;

	UINT8 pos;      /* position in the envelope array */
	UINT8 flags;
	UINT8 volL;
	UINT8 volR;
};

struct saa_vb_channel
{
	UINT8 volL, volR;
	UINT8 freq;
	UINT8 oct;
	UINT8 toneOn;
	UINT8 noiseOn;

	struct saa_freq_gen fgen;
	UINT8 state;    /* tone state */
};

struct SAA1099_VB
{
	struct saa_vb_channel channels[6];
	struct saa_noise_gen  noise[2];
	struct saa_env_gen    env[2];

	UINT8 allOn;
	UINT8 fgReset;  /* frequency generator reset */
	UINT8 curAddr;
	UINT8 regs[0x20];

	INT32 volTbl[0x10];

	/* 32.32 fixed point step counter, libVGM's RATIO_CNTR inlined */
	UINT64 stepInc;
	UINT64 stepVal;
};

static struct SAA1099_VB saa1099vb[MAX_SAA1099];

#define ENV_LOAD	0x80	/* load the buffered commands after processing this entry */
#define ENV_STAY	0x40	/* stay at this value, do not advance */

static const UINT8 saa_envelopes[8][32] =
{
	{	/* zero amplitude */
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0|ENV_LOAD|ENV_STAY,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0|ENV_LOAD|ENV_STAY,
	},
	{	/* maximum amplitude */
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF|ENV_LOAD,
	0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF|ENV_LOAD,
	},
	{	/* single decay */
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0|ENV_LOAD|ENV_STAY,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0|ENV_LOAD|ENV_STAY,
	},
	{	/* repetitive decay */
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0|ENV_LOAD,
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0|ENV_LOAD,
	},
	{	/* single triangular */
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0|ENV_LOAD|ENV_STAY,
	},
	{	/* repetitive triangular */
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,
	0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0|ENV_LOAD,
	},
	{	/* single attack */
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF|ENV_LOAD,
	0x0|ENV_LOAD|ENV_STAY, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0|ENV_LOAD|ENV_STAY,
	},
	{	/* repetitive attack */
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF|ENV_LOAD,
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF|ENV_LOAD,
	}
};

static void saa_envgen_load(struct SAA1099_VB *saa, UINT8 genID);
static void saa_envgen_step(struct saa_env_gen *saaEGen);

INLINE void saa_freq_gen_reset(struct saa_freq_gen *fgen)
{
	fgen->cntr = 0x000;
	/* keep fgen->limit */
	fgen->state = 0;
	fgen->trigger = 0;
}

INLINE void saa_freq_gen_step(struct saa_freq_gen *fgen, UINT16 inc)
{
	fgen->cntr -= inc;
	fgen->trigger = (fgen->cntr < 0x000);	/* trigger on underflow */
	if (fgen->trigger)
	{
		fgen->cntr += fgen->limit;
		fgen->state ^= 1;
	}
}

#define FREQGEN_FALL_EDGE(fgen)	( (fgen).trigger && ! (fgen).state )

static void saa_advance(struct SAA1099_VB *saa, UINT32 steps)
{
	struct saa_vb_channel *saaCh;
	struct saa_noise_gen *saaNGen;
	struct saa_env_gen *saaEGen;
	UINT8 curChn;

	for (; steps > 0; steps --)
	{
		/* run the frequency generators, process the tone channels */
		if (! saa->fgReset)	/* skipped while the "frequency generator reset" flag is on */
		{
			for (curChn = 0; curChn < 6; curChn ++)
			{
				saaCh = &saa->channels[curChn];

				saa_freq_gen_step(&saaCh->fgen, 1 << saaCh->oct);
				if (FREQGEN_FALL_EDGE(saaCh->fgen))
					saaCh->state ^= 1;
			}
		}

		/* run the noise generators */
		for (curChn = 0; curChn < 2; curChn ++)
		{
			UINT16 inc;

			saaNGen = &saa->noise[curChn];
			saaCh = &saa->channels[curChn * 3 + 0];

			/* modes 0..2: master clock divided by 128 / 256 / 512
			   mode 3: clocked by frequency generator 0/3 */
			if (saaNGen->mode == 0x03)
				inc = FREQGEN_FALL_EDGE(saaCh->fgen);
			else
				inc = 1;
			saa_freq_gen_step(&saaNGen->fgen, inc);
			if (saaNGen->fgen.trigger)
			{
				/* polynomial is x^18 + x^11 + x (i.e. 0x20400)
				   see http://www.vogons.org/viewtopic.php?f=9&t=51695 */
				if ( (! (saaNGen->state & 0x20000)) != (! (saaNGen->state & 0x00400)) )
					saaNGen->state = (saaNGen->state << 1) | 0x01;
				else
					saaNGen->state <<= 1;
			}
		}

		/* run the envelope generators */
		for (curChn = 0; curChn < 2; curChn ++)
		{
			saaEGen = &saa->env[curChn];
			saaCh = &saa->channels[curChn * 3 + 1];

			if (! saaEGen->extClock && FREQGEN_FALL_EDGE(saaCh->fgen))
			{
				if ((saaEGen->flags & ENV_LOAD) && saaEGen->reload)
					saa_envgen_load(saa, curChn);
				saa_envgen_step(saaEGen);
			}
		}
	}
}

static void saa_envgen_load(struct SAA1099_VB *saa, UINT8 genID)
{
	struct saa_env_gen *saaEGen = &saa->env[genID];
	const UINT8 data = saa->regs[0x18 | genID];

	saaEGen->extClock = (data >> 5) & 0x01;
	saaEGen->wave     = (data >> 1) & 0x07;
	saaEGen->invert   = (data >> 0) & 0x01;
	saaEGen->pos      = 0x00;
	saaEGen->reload   = 0;

	if (! saaEGen->enable)
	{
		saaEGen->flags = ENV_LOAD | ENV_STAY;
		saaEGen->volL = saaEGen->volR = 0x10;
	}
}

static void saa_envgen_step(struct saa_env_gen *saaEGen)
{
	UINT8 wdata;

	if (! saaEGen->enable)
		return;

	wdata = saa_envelopes[saaEGen->wave][saaEGen->pos];
	saaEGen->flags = wdata & 0xF0;
	saaEGen->volL = saaEGen->volR = wdata & 0x0F;
	if (saaEGen->invert)
		saaEGen->volR ^= 0x0F;

	if (! (saaEGen->flags & ENV_STAY))
	{
		saaEGen->pos ++;
		saaEGen->pos &= 0x1F;
	}
	if (saaEGen->step)
	{
		saaEGen->flags |= (saa_envelopes[saaEGen->wave][saaEGen->pos] & 0xF0);
		saaEGen->volL &= ~0x01;
		saaEGen->volR &= ~0x01;
		if (! (saaEGen->flags & ENV_STAY))
		{
			saaEGen->pos ++;
			saaEGen->pos &= 0x1F;
		}
	}
}

/*-------------------------------------------------
	interface used by saa1099.c
-------------------------------------------------*/

void saa1099vb_init(int chip, double clock, double sample_rate)
{
	struct SAA1099_VB * const saa = &saa1099vb[chip];
	UINT8 curVol;
	UINT8 curChn;

	memset(saa, 0, sizeof(struct SAA1099_VB));

	/* libVGM's RC_SET_RATIO(&stepCntr, clock, sample_rate * 128): how many chip steps
	   pass per output sample, 32.32 fixed point. saa1099.c picks sample_rate = clock/128
	   so this comes out exactly 1.0 */
	{
		const double div = sample_rate * 128.0;
		saa->stepInc = (UINT64)(4294967296.0 /* 2^32 */ * clock / div + 0.5);
	}
	saa->stepVal = 0;

	for (curVol = 0x00; curVol < 0x10; curVol ++)
		saa->volTbl[curVol] = curVol * 0x4000 / 16 / 6;

	saa->allOn = 0x00;
	saa->fgReset = 0x00;
	saa->curAddr = 0x00;

	for (curChn = 0; curChn < 6; curChn ++)
	{
		struct saa_vb_channel * const saaCh = &saa->channels[curChn];

		saaCh->volL = saaCh->volR = 0x00;
		saaCh->freq = 0x00;
		saaCh->oct = 0x00;
		saaCh->toneOn = saaCh->noiseOn = 0x00;
		saa_freq_gen_reset(&saaCh->fgen);
		saaCh->state = 0;
		saaCh->fgen.limit = saaCh->freq ^ 0x1FF;
	}
	for (curChn = 0; curChn < 2; curChn ++)
	{
		saa_freq_gen_reset(&saa->noise[curChn].fgen);
		saa->noise[curChn].mode = 0x00;
		saa->noise[curChn].state = ~0u;

		saa->env[curChn].enable = 0;
		saa->env[curChn].reload = 0;
		saa->env[curChn].pos = 0x00;
		saa->env[curChn].flags = ENV_LOAD | ENV_STAY;
		saa->env[curChn].volL = saa->env[curChn].volR = 0x10;
	}
}

void saa1099vb_write_addr(int chip, UINT8 data)
{
	struct SAA1099_VB * const saa = &saa1099vb[chip];

	saa->curAddr = data & 0x1F;
	if (saa->curAddr == 0x18 || saa->curAddr == 0x19)
	{
		struct saa_env_gen * const saaEGen = &saa->env[saa->curAddr & 0x01];

		/* an address write is the external envelope clock */
		if (saaEGen->extClock)
		{
			if ((saaEGen->flags & ENV_LOAD) && saaEGen->reload)
				saa_envgen_load(saa, saa->curAddr & 0x01);
			saa_envgen_step(saaEGen);
		}
	}
}

void saa1099vb_write_data(int chip, UINT8 data)
{
	struct SAA1099_VB * const saa = &saa1099vb[chip];
	UINT8 curChn;
	UINT8 prevState;

	saa->regs[saa->curAddr] = data;
	switch (saa->curAddr)
	{
	case 0x00:	/* channel 0..5 volume (low nibble left, high nibble right) */
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
		curChn = saa->curAddr & 0x07;
		saa->channels[curChn].volL = (data >> 0) & 0x0F;
		saa->channels[curChn].volR = (data >> 4) & 0x0F;
		break;
	case 0x08:	/* channel 0..5 frequency */
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
		curChn = saa->curAddr & 0x07;
		saa->channels[curChn].freq = data;
		saa->channels[curChn].fgen.limit = data ^ 0x1FF;
		break;
	case 0x10:	/* channel 0+1 / 2+3 / 4+5 octave */
	case 0x11:
	case 0x12:
		curChn = (saa->curAddr & 0x03) << 1;
		saa->channels[curChn | 0x00].oct = (data >> 0) & 0x07;
		saa->channels[curChn | 0x01].oct = (data >> 4) & 0x07;
		break;
	case 0x14:	/* tone enable */
		for (curChn = 0; curChn < 6; curChn ++)
			saa->channels[curChn].toneOn = (data >> curChn) & 0x01;
		break;
	case 0x15:	/* noise enable */
		for (curChn = 0; curChn < 6; curChn ++)
			saa->channels[curChn].noiseOn = (data >> curChn) & 0x01;
		break;
	case 0x16:	/* noise generator frequencies */
		saa->noise[0x00].mode = (data >> 0) & 0x03;
		saa->noise[0x01].mode = (data >> 4) & 0x03;
		for (curChn = 0; curChn < 2; curChn ++)
		{
			struct saa_noise_gen * const saaNGen = &saa->noise[curChn];
			if (saaNGen->mode == 0x03)
				saaNGen->fgen.limit = 1;	/* clocked by the frequency generator */
			else
				saaNGen->fgen.limit = (1 << saaNGen->mode);
			saaNGen->fgen.cntr = 0x000;
		}
		break;
	case 0x18:	/* envelope generator 0 parameters */
	case 0x19:	/* envelope generator 1 parameters */
		curChn = saa->curAddr & 0x01;
		prevState = saa->env[curChn].enable;
		saa->env[curChn].enable = (data >> 7) & 0x01;
		saa->env[curChn].step   = (data >> 4) & 0x01;
		saa->env[curChn].reload = 1;
		if (! saa->env[curChn].enable || ! prevState)
		{
			saa_envgen_load(saa, curChn);
			saa_envgen_step(&saa->env[curChn]);
		}
		break;
	case 0x1C:	/* chip enable / sync & reset */
		saa->allOn = (data >> 0) & 0x01;
		prevState = saa->fgReset;
		saa->fgReset = (data >> 1) & 0x01;
		if (saa->fgReset)
		{
			for (curChn = 0; curChn < 6; curChn ++)
			{
				saa_freq_gen_reset(&saa->channels[curChn].fgen);
				/* the reset forces the state to 0, per the real hardware recordings */
				saa->channels[curChn].state = 0;
			}
		}
		else if (prevState)
		{
			/* releasing "reset" switches to state 1 immediately, and that "1" phase
			   is longer than usual in some cases */
			for (curChn = 0; curChn < 6; curChn ++)
				saa->channels[curChn].state = 1;
		}
		break;
	}
}

void saa1099vb_update(int chip, INT16 **buffer, int length)
{
	struct SAA1099_VB * const saa = &saa1099vb[chip];
	int curSmpl;

	if (! saa->allOn)
	{
		memset(buffer[ LEFT], 0, length * sizeof(INT16));
		memset(buffer[RIGHT], 0, length * sizeof(INT16));
		return;
	}

	for (curSmpl = 0; curSmpl < length; curSmpl ++)
	{
		INT32 outL = 0, outR = 0;
		UINT8 curChn;

		for (curChn = 0; curChn < 6; curChn ++)
		{
			struct saa_vb_channel * const saaCh = &saa->channels[curChn];
			const UINT8 neChn = curChn / 3;	/* noise/envelope generator for this channel */
			UINT8 noDCremove = 0;
			INT32 outState;
			INT32 outVolL, outVolR;

			if (saaCh->toneOn && saaCh->noiseOn)
			{
				/* this is how mixing tone+noise actually behaves */
				outState = saaCh->state;
				if (saaCh->state)
					outState += (saa->noise[neChn].state & 0x01);
			}
			else
			{
				outState  = (saaCh->toneOn) ? saaCh->state : 1;
				outState &= (saaCh->noiseOn) ? (saa->noise[neChn].state & 0x01) : 1;
				outState *= 2;	/* same amplitude as the tone+noise case */
			}

			if ((curChn == 2 || curChn == 5) && saa->env[neChn].enable)
			{
				/* enabling the envelope generator disables amplitude bit 0 */
				outVolL = (saa->volTbl[saaCh->volL & ~0x01] * saa->env[neChn].volL) >> 4;
				outVolR = (saa->volTbl[saaCh->volR & ~0x01] * saa->env[neChn].volR) >> 4;
				if (! saaCh->toneOn && ! saaCh->noiseOn)
					noDCremove = 2;
			}
			else
			{
				outVolL = saa->volTbl[saaCh->volL];
				outVolR = saa->volTbl[saaCh->volR];
				if (! saaCh->toneOn && ! saaCh->noiseOn)
					noDCremove = 1;
			}

			/* tone XOR noise enabled: outState is 0 [off] or 2 [on]
			   tone AND noise enabled: outState is 0 [both off], 1 [tone only] or 2 [both] */
			if (! noDCremove)
			{
				outState = outState - 1;	/* remove the DC offset, make it bipolar */
				outL += outState * outVolL;
				outR += outState * outVolR;
			}
			else if (noDCremove == 1)
			{
				/* skip the offset removal, which is what makes software PCM work */
				outL += outState * outVolL;
				outR += outState * outVolR;
			}
			else /* noDCremove == 2 */
			{
				/* centre on the current volume level, for envelope generator based sounds */
				outL += outState * outVolL - saa->volTbl[saaCh->volL & ~0x01];
				outR += outState * outVolR - saa->volTbl[saaCh->volR & ~0x01];
			}
		}

		/* volTbl already divides by the 6 channels, so the sum cannot leave INT16 range;
		   clamp anyway rather than wrap if that ever stops being true */
		buffer[ LEFT][curSmpl] = (outL >  32767) ?  32767 : (outL < -32768) ? -32768 : (INT16)outL;
		buffer[RIGHT][curSmpl] = (outR >  32767) ?  32767 : (outR < -32768) ? -32768 : (INT16)outR;

		/* advancing after the sample is calculated reproduces a few artifacts better,
		   such as the dip after a frequency generator reset */
		saa->stepVal += saa->stepInc;
		saa_advance(saa, (UINT32)(saa->stepVal >> 32));
		saa->stepVal &= (((UINT64)1 << 32) - 1);
	}
}
