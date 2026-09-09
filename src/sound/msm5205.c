// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *	 HJB 08/31/98
 *	 modified to use an automatically selected oversampling factor
 *   for the current sample rate
 *
 *	 01/06/99
 *	separate MSM5205 emulator form adpcm.c and some fix
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*

    MSM 5205 ADPCM chip:

    Data is streamed from a CPU by means of a clock generated on the chip.

    Holding the rate selector lines (S1 and S2) both high places the MSM5205 in an undocumented
    mode which disables the sampling clock generator and makes VCK an input line.

    A reset signal is set high or low to determine whether playback (and interrupts) are occurring.

  MSM6585: is an upgraded MSM5205 voice synth IC.
   Improvements:
    More precise internal DA converter
    Built in low-pass filter
    Expanded sampling frequency

   Differences between MSM6585 & MSM5205:

                              MSM6585                      MSM5205
    Master clock frequency    640kHz                       384k/768kHz
    Sampling frequency        4k/8k/16k/32kHz at 640kHz    4k/6k/8kHz at 384kHz
    ADPCM bit length          4-bit                        3-bit/4-bit
    Data capture timing       3µsec at 640kHz              15.6µsec at 384kHz
    DA converter              12-bit                       10-bit
    Low-pass filter           -40dB/oct                    N/A
    Overflow prevent circuit  Included                     N/A
    Cutoff Frequency          (Sampling Frequency/2.5)kHz  N/A

    Data capture follows VCK falling edge on MSM5205 (VCK rising edge on MSM6585)

   TODO:
   - lowpass filter for MSM6585

 */

#include "driver.h"
#include "msm5205.h"
#include "../ext/vgm/vgmwrite.h"

/*
 * ADPCM lookup table
 */

/* step size index shift table */
static const int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

/*
 *   Compute the difference table
 */

static void ComputeTables (void)
{
	/* nibble to bit map */
	static const int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};

	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= 48; step++)
	{
		/* compute the step value */
		int stepval = (int)floor(16.0 * pow (11.0 / 10.0, (double)step));

		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
		}
	}
}

/*
 *
 *	MSM 5205 ADPCM chip:
 *
 *	Data is streamed from a CPU by means of a clock generated on the chip.
 *
 *	A reset signal is set high or low to determine whether playback (and interrupts) are occuring
 *
 */

struct MSM5205Voice
{
	int stream;             /* number of stream system      */
	void *timer;            /* VCLK callback timer          */
	int data;               /* next adpcm data              */
	int vclk;               /* vclk signal (external mode)  */
	int reset;              /* reset pin signal             */
	int prescaler;          /* prescaler selector S1 and S2 */
	int bitwidth;           /* bit width selector -3B/4B    */
	int signal;             /* current ADPCM signal         */
	int step;               /* current ADPCM step           */
	int dac_bits;           /* msm6585: 12, msm5205: 10     */
	unsigned short vgm_idx;
};

/* Rate select as libvgm (currently) wants it in a VGM, which is not 100% what we do.
   libvgm's own S1/S2 decode is right for the 6585 and transposed for the 5205,
   and per libvgm issue #159 it may(!) not be changed - so an exporter
   has to pre-compensate or playback comes out at the wrong rate. Straight through for
   the 6585, swapped for the 5205. This is exactly what ValleyBell's VGM-logging fork of
   MAME emits, too, (m_s2<<1)|m_s1 versus (m_s1<<1)|m_s2 */
#define MSM5205_VGM_RATESEL(sel) (((sel) & 8) ? ((sel) & 3) : ((((sel) & 1) << 1) | (((sel) >> 1) & 1)))

static const struct MSM5205interface *msm5205_intf;
static struct MSM5205Voice msm5205[MAX_MSM5205];

/* stream update callbacks */
static void MSM5205_update(int chip,INT16 *buffer,int length)
{
	struct MSM5205Voice *voice = &msm5205[chip];

	/* if this voice is active */
	if (voice->signal)
	{
		const int dac_mask = (voice->dac_bits >= 12) ? 0 : (1 << (12 - voice->dac_bits)) - 1;
		const INT16 val = (voice->signal & ~dac_mask) * 16;
		int i;
		for (i = 0; i < length; i++)
			buffer[i] = val;
	}
	else
		memset (buffer,0,length*sizeof(INT16));
}

// timer callback at VCLK low edge
static void MSM5205_vclk_callback(int num)
{
	struct MSM5205Voice *voice = &msm5205[num];
	int val;
	int new_signal;

	// callback user handler and latch next data
	if(msm5205_intf->vclk_callback[num]) (*msm5205_intf->vclk_callback[num])(num);

	// reset check at last hiedge of VCLK
	if (voice->reset)
	{
		new_signal = 0;
		voice->step = 0;
	}
	else
	{
		/* update signal */
		/* !! MSM5205 has internal 12bit decoding, signal width is 0 to 8191 !! */
		val = voice->data;
		new_signal = (voice->signal * 245 + (diff_lookup[voice->step * 16 + (val & 15)] << 8)) >> 8;

		if (new_signal > 2047) new_signal = 2047;
		else if (new_signal < -2048) new_signal = -2048;

		voice->step += index_shift[val & 7];

		if (voice->step > 48) voice->step = 48;
		else if (voice->step < 0) voice->step = 0;
	}

	/* update when signal changed */
	if(voice->signal != new_signal)
	{
		stream_update(voice->stream,0);
		voice->signal = new_signal;
	}
}
/*
 *    Start emulation of an MSM5205-compatible chip
 */

int MSM5205_sh_start (const struct MachineSound *msound)
{
	int i;

	/* save a global pointer to our interface */
	msm5205_intf = msound->sound_interface;

	/* compute the difference tables */
	ComputeTables ();

	/* initialize the voices */
	memset (msm5205, 0, sizeof (msm5205));

	/* stream system initialize */
	for (i = 0;i < msm5205_intf->num;i++)
	{
		struct MSM5205Voice *voice = &msm5205[i];
		char name[20];
		sprintf(name,"MSM5205 #%d",i);
		voice->stream = stream_init(name,msm5205_intf->mixing_level[i],
		                        Machine->sample_rate,i,
		                        MSM5205_update);
		voice->timer = timer_alloc(MSM5205_vclk_callback);

		voice->vgm_idx = vgm_open(VGMC_MSM5205, msm5205_intf->baseclock);
		vgm_header_set(voice->vgm_idx, 0x00, (msm5205_intf->select[i] & 8) ? 1 : 0); /* 0 = MSM5205, 1 = MSM6585 */
		vgm_header_set(voice->vgm_idx, 0x01, MSM5205_VGM_RATESEL(msm5205_intf->select[i]));
		vgm_header_set(voice->vgm_idx, 0x02, (msm5205_intf->select[i] & 4) ? 1 : 0); /* 4 bit ADPCM */
	}
	/* initialize */
	MSM5205_sh_reset();
	/* success */
	return 0;
}

/*
 *    Stop emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_stop (void)
{
}

/*
 *    Update emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_update (void)
{
}


/*
 *    Reset emulation of an MSM5205-compatible chip
 */
void MSM5205_sh_reset(void)
{
	int i;

	/* bail if we're not emulating sound */
	if (Machine->sample_rate == 0)
		return;

	for (i = 0; i < msm5205_intf->num; i++)
	{
		struct MSM5205Voice *voice = &msm5205[i];
		/* initialize work */
		voice->data    = 0;
		voice->vclk    = 0;
		voice->reset   = 0;
		voice->signal  = 0;
		voice->step    = 0;
		voice->dac_bits = (msm5205_intf->variant[i] == 0) ? 10 : 12;
		/* timer and bitwidth set */
		MSM5205_playmode_w(i,msm5205_intf->select[i]);
	}
}

/*
 *    Handle an update of the vclk status of a chip (1 is reset ON, 0 is reset OFF)
 *    This function can use selector = MSM5205_SEX only
 */
void MSM5205_vclk_w (int num, int vclk)
{
	/* range check the numbers */
	if (num >= msm5205_intf->num)
	{
		logerror("error: MSM5205_vclk_w() called with chip = %d, but only %d chips allocated\n", num, msm5205_intf->num);
		return;
	}
	if (msm5205[num].prescaler != 0)
	{
		logerror("error: MSM5205_vclk_w() called with chip = %d, but VCLK selected master mode\n", num);
	}
	else
	{
		if (msm5205[num].vclk != vclk)
		{
			msm5205[num].vclk = vclk;
			vgm_write(msm5205[num].vgm_idx, 0x00, 0x02, vclk ? 1 : 0);
			if( !vclk ) MSM5205_vclk_callback(num);
		}
	}
}

/*
 *    Handle an update of the reset status of a chip (1 is reset ON, 0 is reset OFF)
 */

void MSM5205_reset_w (int num, int reset)
{
	/* range check the numbers */
	if (num >= msm5205_intf->num)
	{
		logerror("error: MSM5205_reset_w() called with chip = %d, but only %d chips allocated\n", num, msm5205_intf->num);
		return;
	}
	msm5205[num].reset = reset;
	vgm_write(msm5205[num].vgm_idx, 0x00, 0x00, reset ? 1 : 0);
}

/*
 *    Handle an update of the data to the chip
 */

void MSM5205_data_w (int num, int data)
{
	if (msm5205[num].bitwidth == 4)
		msm5205[num].data = data & 0x0f;
	else
		msm5205[num].data = (data & 0x07) << 1; /* unknown */
	vgm_write(msm5205[num].vgm_idx, 0x00, 0x01, (UINT8)msm5205[num].data);
}

/*
 *    Handle a change of the selector
 */

void MSM5205_playmode_w(int num,int select)
{
	struct MSM5205Voice *voice = &msm5205[num];
	static const int prescaler_table[2][4] =
	{
		{ 96, 48, 64,  0},
		{160, 80, 40, 20} // msm6585 - see the note in msm5205.h
	};
	int prescaler = prescaler_table[select >> 3 & 1][select & 3];
	int bitwidth = (select & 4) ? 4 : 3;

	if (voice->prescaler != prescaler)
	{
		stream_update(voice->stream,0);

		voice->prescaler = prescaler;
		/* timer set */
		if( prescaler )
		{
			double period = TIME_IN_HZ(msm5205_intf->baseclock / prescaler);
			timer_adjust(voice->timer, period, num, period);
		}
		else
			timer_adjust(voice->timer, TIME_NEVER, 0, 0);

		/* as ValleyBell's VGM fork of MAME does, both on any rate change */
		vgm_write(voice->vgm_idx, 0x00, 0x04, (UINT8)MSM5205_VGM_RATESEL(select));
		vgm_write(voice->vgm_idx, 0x00, 0x05, (select & 4) ? 1 : 0);
	}

	if (voice->bitwidth != bitwidth)
	{
		stream_update(voice->stream,0);

		voice->bitwidth = bitwidth;
		vgm_write(voice->vgm_idx, 0x00, 0x05, (select & 4) ? 1 : 0);
	}
}


void MSM5205_set_volume(int num,int volume)
{
	struct MSM5205Voice *voice = &msm5205[num];

	mixer_set_volume(voice->stream,volume);
}
