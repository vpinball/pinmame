/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"



/**********************************************************************************************

     CONSTANTS

***********************************************************************************************/

#define BACKEND_INTERPOLATE		1
#define LOG_COMMANDS			1
#define MAKE_WAVS				0

#if MAKE_WAVS
#include "wavwrite.h"
#endif


#define MAX_SAMPLE_CHUNK		10000

#define FRAC_BITS				14
#define FRAC_ONE				(1 << FRAC_BITS)
#define FRAC_MASK				(FRAC_ONE - 1)

#define REG_CURRPOS				0
#define REG_UNUSED1				1
#define REG_RATE				2
#define REG_LOOPEND				3
#define REG_LOOPSTART			4
#define REG_BANK				5
#define REG_UNUSED6				6
#define REG_LEFTVOL				7
#define REG_RIGHTVOL			8
#define REG_TOTAL				9



/**********************************************************************************************

     INTERNAL DATA STRUCTURES

***********************************************************************************************/

/* struct describing a single playing voice */
struct BSMT2000Voice
{
	/* external state */
	UINT16		reg[REG_TOTAL];			/* 9 registers */
	UINT32		position;				/* current position */
	UINT32		loop_start_position;	/* loop start position */
	UINT32		loop_stop_position;		/* loop stop position */
	UINT32		adjusted_rate;			/* adjusted rate */
	UINT32          bank;
};

struct BSMT2000Chip
{
	int			stream;					/* which stream are we using */
	INT8 *		region_base;			/* pointer to the base of the region */
	int			total_banks;			/* number of total banks in the region */
	double 		master_clock;			/* master clock frequency */

	INT32		output_step;			/* step value for frequency conversion */
	INT32		output_pos;				/* current fractional position */
	INT32		last_lsample;			/* last sample output */
	INT32		last_rsample;			/* last sample output */
	INT32		curr_lsample;			/* current sample target */
	INT32		curr_rsample;			/* current sample target */

	struct 		BSMT2000Voice voice[12];/* the 12 voices */

#if MAKE_WAVS
	void *		wavraw;					/* raw waveform */
	void *		wavresample;			/* resampled waveform */
#endif
};



/**********************************************************************************************

     GLOBALS

***********************************************************************************************/

static struct BSMT2000Chip bsmt2000[MAX_BSMT2000];
static INT32 *accumulator;
static INT32 *scratch;



/**********************************************************************************************

     interpolate
     backend_interpolate -- interpolate between two samples

***********************************************************************************************/

#define interpolate(sample1, sample2, accum)										\
		(sample1 * (INT32)(0x10000 - (accum & 0xffff)) + 							\
		 sample2 * (INT32)(accum & 0xffff)) >> 16;

#if BACKEND_INTERPOLATE
#define backend_interpolate(sample1, sample2, position)								\
		(sample1 * (INT32)(FRAC_ONE - position) + 									\
		 sample2 * (INT32)position) >> FRAC_BITS;
#else
#define backend_interpolate(sample1, sample2, position)	sample1
#endif



/**********************************************************************************************

     generate_samples -- generate samples for all voices at the chip's frequency

***********************************************************************************************/

static void generate_samples(struct BSMT2000Chip *chip, INT32 *left, INT32 *right, int samples)
{
	int v;
	logerror("BSMT generate samples = %d\n",samples);
	/* skip if nothing to do */
	if (!samples)
		return;

	/* clear out the accumulator */
	memset(left, 0, samples * sizeof(left[0]));
	memset(right, 0, samples * sizeof(right[0]));

	/* loop over voices */
	for (v = 0; v < 12; v++)
	{
		struct BSMT2000Voice *voice = &chip->voice[v];
		
		/* compute the region base */
		if ((voice->bank < chip->total_banks) && (voice->adjusted_rate > 0))
		{
			INT8 *base = &chip->region_base[voice->bank * 0x10000];
			INT32 *lbuffer = left, *rbuffer = right;
			UINT32 rate = voice->adjusted_rate;
			UINT32 pos = voice->position;
			INT32 lvol = voice->reg[REG_LEFTVOL];
			INT32 rvol = voice->reg[REG_RIGHTVOL];
			int remaining = samples;
			logerror("voice%02d rate=%8x pos=%8x, lvol=%4x, rvol=%4x\n",v,rate,pos,lvol,rvol);
			/* loop while we still have samples to generate */
			while (remaining--)
			{
				/* fetch two samples */
				INT32 val1 = base[pos >> 16];
				INT32 val2 = base[(pos >> 16) + 1];
				pos += rate;
//				logerror("val1=%8x val2=%8x\n",val1,val2);
				/* interpolate */
				val1 = interpolate(val1, val2, pos);
				
				/* apply volumes and add */
				*lbuffer++ += val1 * lvol;
				*rbuffer++ += val1 * rvol;

				/* check for loop end */
				if (pos >= voice->loop_stop_position)
					pos += voice->loop_start_position - voice->loop_stop_position;
			}

			/* update the position */
			voice->position = pos;
		}
	}
}



/**********************************************************************************************

     bsmt2000_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void bsmt2000_update(int num, INT16 **buffer, int length)
{
	struct BSMT2000Chip *chip = &bsmt2000[num];
	INT32 *lsrc = scratch, *rsrc = scratch;
	INT32 lprev = chip->last_lsample;
	INT32 rprev = chip->last_rsample;
	INT32 lcurr = chip->curr_lsample;
	INT32 rcurr = chip->curr_rsample;
	INT16 *ldest = buffer[0];
	INT16 *rdest = buffer[1];
	INT32 interp;
	int remaining = length;
	int samples_left = 0;

#if MAKE_WAVS
	/* start the logging once we have a sample rate */
	if (chip->output_step)
	{
		if (!chip->wavraw)
		{
			int sample_rate = (int)((double)Machine->sample_rate / (double)(1 << FRAC_BITS) * (double)chip->output_step);
			chip->wavraw = wav_open("raw.wav", sample_rate, 2);
		}
		if (!chip->wavresample)
			chip->wavresample = wav_open("resamp.wav", Machine->sample_rate, 2);
	}
#endif

	/* then sample-rate convert with linear interpolation */
	while (remaining > 0)
	{
		/* if we're over, grab the next samples */
		while (chip->output_pos >= FRAC_ONE)
		{
			/* do we have any samples available? */
			if (samples_left == 0)
			{
				/* compute how many new samples we need */
				UINT32 final_pos = chip->output_pos + (remaining - 1) * chip->output_step;
				samples_left = final_pos >> FRAC_BITS;
				if (samples_left > MAX_SAMPLE_CHUNK)
					samples_left = MAX_SAMPLE_CHUNK;

				/* determine left/right source data */
				lsrc = scratch;
				rsrc = scratch + samples_left;
				generate_samples(chip, lsrc, rsrc, samples_left);

#if MAKE_WAVS
				/* log the raw data */
				if (chip->wavraw)
					wav_add_data_32lr(chip->wavraw, lsrc, rsrc, samples_left, 4);
#endif
			}

			/* adjust the positions */
			chip->output_pos -= FRAC_ONE;
			lprev = lcurr;
			rprev = rcurr;

			/* fetch new samples */
			lcurr = *lsrc++ >> 9;
			rcurr = *rsrc++ >> 9;
			samples_left--;
		}

		/* interpolate between the two current samples */
		while (remaining > 0 && chip->output_pos < FRAC_ONE)
		{
			/* left channel */
			interp = backend_interpolate(lprev, lcurr, chip->output_pos);
			*ldest++ = (interp < -32768) ? -32768 : (interp > 32767) ? 32767 : interp;

			/* right channel */
			interp = backend_interpolate(rprev, rcurr, chip->output_pos);
			*rdest++ = (interp < -32768) ? -32768 : (interp > 32767) ? 32767 : interp;

			/* advance */
			chip->output_pos += chip->output_step;
			remaining--;
		}
	}

	/* remember the last samples */
	chip->last_lsample = lprev;
	chip->last_rsample = rprev;
	chip->curr_lsample = lcurr;
	chip->curr_rsample = rcurr;

#if MAKE_WAVS
	/* log the resampled data */
	if (chip->wavresample)
		wav_add_data_16lr(chip->wavresample, buffer[0], buffer[1], length);
#endif
}



/**********************************************************************************************

     BSMT2000_sh_start -- start emulation of the BSMT2000

***********************************************************************************************/

int BSMT2000_sh_start(const struct MachineSound *msound)
{
	const struct BSMT2000interface *intf = msound->sound_interface;
	char stream_name[2][40];
	const char *stream_name_ptrs[2];
	int vol[2];
	int i, j;

	/* initialize the voices */
	memset(&bsmt2000, 0, sizeof(bsmt2000));
	for (i = 0; i < intf->num; i++)
	{
		/* generate the name and create the stream */
		sprintf(stream_name[0], "%s #%d Ch1", sound_name(msound), i);
		sprintf(stream_name[1], "%s #%d Ch2", sound_name(msound), i);
		stream_name_ptrs[0] = stream_name[0];
		stream_name_ptrs[1] = stream_name[1];

		/* set the volumes */
		vol[0] = MIXER(intf->mixing_level[i],MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->mixing_level[i],MIXER_PAN_RIGHT);

		/* create the stream */
		bsmt2000[i].stream = stream_init_multi(2, stream_name_ptrs, vol, Machine->sample_rate, i, bsmt2000_update);
		if (bsmt2000[i].stream == -1)
			return 1;

		/* initialize the regions */
		bsmt2000[i].region_base = (INT8 *)memory_region(intf->region[i]);
		bsmt2000[i].total_banks = memory_region_length(intf->region[i]) / 0x10000;

		/* initialize the rest of the structure */
		bsmt2000[i].master_clock = (double)intf->baseclock[i];
		bsmt2000[i].output_step = (int)((double)intf->baseclock[i] / 1024.0 * (double)(1 << FRAC_BITS) / (double)Machine->sample_rate);

		/* init the voices */
		for (j = 0; j < 12; j++)
		{
			bsmt2000[i].voice[j].position = 0;
			bsmt2000[i].voice[j].adjusted_rate = 0;
			bsmt2000[i].voice[j].reg[REG_LEFTVOL] = 0x3fff;
			bsmt2000[i].voice[j].reg[REG_RIGHTVOL] = 0x3fff;
			bsmt2000[i].voice[j].bank = 0;
		}
	}

	/* allocate memory */
	accumulator = malloc(sizeof(accumulator[0]) * 2 * MAX_SAMPLE_CHUNK);
	scratch = malloc(sizeof(scratch[0]) * 2 * MAX_SAMPLE_CHUNK);
	if (!accumulator || !scratch)
		return 1;

	/* success */
	logerror("BSMT_init OK\n");
	return 0;
}



/**********************************************************************************************

     BSMT2000_sh_stop -- stop emulation of the BSMT2000

***********************************************************************************************/

void BSMT2000_sh_stop(void)
{
	/* free memory */
	if (accumulator)
		free(accumulator);
	accumulator = NULL;

	if (scratch)
		free(scratch);
	scratch = NULL;

#if MAKE_WAVS
{
	int i;

	for (i = 0; i < MAX_BSMT2000; i++)
	{
		if (bsmt2000[i].wavraw)
			wav_close(bsmt2000[i].wavraw);
		if (bsmt2000[i].wavresample)
			wav_close(bsmt2000[i].wavresample);
	}
}
#endif
}



/**********************************************************************************************

     bsmt2000_reg_write -- handle a write to the selected BSMT2000 register

***********************************************************************************************/

static void bsmt2000_reg_write(struct BSMT2000Chip *chip, offs_t offset, data16_t data, data16_t mem_mask)
{
	struct BSMT2000Voice *voice = &chip->voice[offset % 12];
	int regindex = offset / 12;

	logerror("BSMT_w: voice=%2d reg=%2d value=%4x\n",offset%12,regindex,data);
	/* update the register */
	if (regindex < REG_TOTAL)
		COMBINE_DATA(&voice->reg[regindex]);

	/* force an update */
	stream_update(chip->stream, 0);
	
	/* update parameters */
	switch (regindex)
	{
		case REG_RATE:
			voice->adjusted_rate = voice->reg[REG_RATE] << 5;
			break;
		
		case REG_CURRPOS:
			voice->position = voice->reg[REG_CURRPOS] << 16;
			break;
		
		case REG_LOOPSTART:
			voice->loop_start_position = voice->reg[REG_LOOPSTART] << 16;
			break;

		case REG_LOOPEND:
			voice->loop_stop_position = voice->reg[REG_LOOPEND] << 16;
			break;
		case REG_BANK:
			voice->bank = (voice->reg[REG_BANK] & 0x07) | 
			              ((voice->reg[REG_BANK] & 0x18)<<1) |
                                      ((voice->reg[REG_BANK] & 0x20)>>2);
			break;
	}
}


static data16_t bsmt2000_reg_read(struct BSMT2000Chip *chip, offs_t offset)
{
	/* no idea if you can read or not */
	return 0;
}



/**********************************************************************************************

     BSMT2000_data_0_r -- handle a read from the status register

***********************************************************************************************/

READ16_HANDLER( BSMT2000_data_0_r )
{
	return bsmt2000_reg_read(&bsmt2000[0], offset);
}



/**********************************************************************************************

     BSMT2000_data_0_w -- handle a write to the current register

***********************************************************************************************/

WRITE16_HANDLER( BSMT2000_data_0_w )
{
	bsmt2000_reg_write(&bsmt2000[0], offset, data, mem_mask);
}
