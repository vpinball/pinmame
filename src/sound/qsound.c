// license:BSD-3-Clause
// copyright-holders:Paul Leaman, Miguel Angel Horna

//!! wire up VGM output support

//!! use latest MAME HLE core
//!! or use ctr version in libvgm/vgmplay (which seems to be roughly the same as the new MAME HLE though)

/***************************************************************************

  Capcom System QSound™ (HLE)
  ===========================

  Driver by Paul Leaman and Miguel Angel Horna

  A 16 channel stereo sample player.

  QSpace position is simulated by panning the sound in the stereo space.

  Many thanks to CAB (the author of Amuse), without whom this probably would
  never have been finished.

  TODO:
  - hook up the DSP!
  - is master volume really linear?
  - understand higher bits of reg 0
  - understand reg 9
  - understand other writes to $90-$ff area

  Links:
  https://siliconpr0n.org/map/capcom/dl-1425

***************************************************************************/

#include <math.h>
#include "driver.h"

/*
Debug defines
*/
#define LOG_WAVE	0
#define LOG_QSOUND  0

/* 8 bit source ROM samples */
typedef unsigned char QSOUND_SRC_SAMPLE;

#define QSOUND_CLOCKDIV (2*1248)  // DSP program uses 1248 machine cycles per iteration
#define QSOUND_CHANNELS 16

typedef struct QSOUND_CHANNEL
{
	unsigned short reg[8]; // channel control registers

	// work variables
	int lvol;           // left volume
	int rvol;           // right volume
} qsound_channel;


/* Private variables */
static struct QSound_interface *intf;	/* Interface  */
static int qsound_stream;				/* Audio stream */
static qsound_channel channel[QSOUND_CHANNELS];
static unsigned short qsound_data;		/* register latch data */
QSOUND_SRC_SAMPLE *qsound_sample_rom;	/* Q sound sample ROM */
static unsigned int qsound_sample_rom_length;
static unsigned int qsound_sample_rom_mask;

static int qsound_pan_table[33];		/* Pan volume table */

#if LOG_WAVE
static FILE *fpRawDataL;
static FILE *fpRawDataR;
#endif

/* Function prototypes */
void qsound_update( int num, INT16 **buffer, int length );
void qsound_write_data(UINT8 address, UINT16 data);

INLINE UINT32 pow2_mask(UINT32 v)
{
	if (v == 0)
		return 0;
	v --;
	v |= (v >>  1);
	v |= (v >>  2);
	v |= (v >>  4);
	v |= (v >>  8);
	v |= (v >> 16);
	return v;
}

INLINE unsigned int saturate(long long val)
{
	return (int)min(max(val, (long long)INT_MIN), (long long)INT_MAX);
}

int qsound_sh_start(const struct MachineSound *msound)
{
	int i,adr;

	intf = msound->sound_interface;

	qsound_sample_rom = (QSOUND_SRC_SAMPLE *)memory_region(intf->region);
	qsound_sample_rom_length = memory_region_length(intf->region);
	qsound_sample_rom_mask = pow2_mask(qsound_sample_rom_length);

	memset(channel, 0, sizeof(channel));

	// create pan table
	for (i = 0; i < 33; i++)
		qsound_pan_table[i] = (int)((256 / sqrt(32.0)) * sqrt((double)i));

#if LOG_QSOUND
	logerror("Pan table\n");
	for (i=0; i<33; i++)
		logerror("%02x ", qsound_pan_table[i]);
#endif
	{
		/* Allocate stream */
#define CHANNELS 2
		char buf[CHANNELS][40];
		const char *name[CHANNELS];
		int  vol[2];
		name[0] = buf[0];
		name[1] = buf[1];
		sprintf( buf[0], "%s L", sound_name(msound) );
		sprintf( buf[1], "%s R", sound_name(msound) );
		vol[0] = MIXER(intf->mixing_level[0], MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->mixing_level[1], MIXER_PAN_RIGHT);
		qsound_stream = stream_init_multi(
			CHANNELS,
			name,
			vol,
			intf->clock / QSOUND_CLOCKDIV,
			0,
			qsound_update );
	}

	for (adr = 0x80; adr < 0x90; adr++)
		qsound_write_data(adr, 0x120);

#if LOG_WAVE
	fpRawDataR=fopen("qsoundr.raw", "w+b");
	fpRawDataL=fopen("qsoundl.raw", "w+b");
	if (!fpRawDataR || !fpRawDataL)
	{
		return 1;
	}
#endif

	return 0x00;
}

void qsound_sh_stop (void)
{
#if LOG_WAVE
	if (fpRawDataR)
	{
		fclose(fpRawDataR);
	}
	if (fpRawDataL)
	{
		fclose(fpRawDataL);
	}
#endif
}

void qsound_sh_reset(void)
{
	int i,j;
	for (i = 0; i < QSOUND_CHANNELS; ++i)
		for (j = 0; j < 8; ++j)
			channel[i].reg[j] = 0;
}

WRITE_HANDLER( qsound_data_h_w )
{
	qsound_data = (qsound_data & 0x00ff) | (data << 8);
}

WRITE_HANDLER( qsound_data_l_w )
{
	qsound_data = (qsound_data & 0xff00) | data;
}

WRITE_HANDLER( qsound_cmd_w )
{
	stream_update(qsound_stream, 0);
	qsound_write_data(data, qsound_data);
}

READ_HANDLER( qsound_status_r )
{
	/* Port ready bit (0x80 if ready) */
	return 0x80;
}

static void qsound_write_data(UINT8 address, UINT16 data)
{
	int ch = 0, reg;

	if (address < 0x80)
	{
		ch = address >> 3;
		reg = address & 7;
	}
	else if (address < 0x90)
	{
		ch = address & 0xf;
		reg = 8;
	}
	else if (address >= 0xba && address < 0xca)
	{
		ch = address - 0xba;
		reg = 9;
	}
	else
	{
		// unknown
		reg = address;
	}

	switch (reg)
	{
	case 0: // bank
	case 1: // current sample
	case 2: // playback rate
	case 3: // sample interval counter
	case 4: // loop offset
	case 5: // end sample
	case 6: // channel volume
	case 7: // unused
		channel[ch].reg[reg] = data;
		break;

	case 8:
	{
		// panning (left=0x0110, centre=0x0120, right=0x0130)
		// looks like it doesn't write other values than that
		int pan = (data & 0x3f) - 0x10;
		if (pan > 0x20)
			pan = 0x20;
		if (pan < 0)
			pan = 0;

		channel[ch].rvol = qsound_pan_table[pan];
		channel[ch].lvol = qsound_pan_table[0x20 - pan];
		break;
	}

	case 9:
		// unknown
		break;

	default:
		//logerror("%s: write_data %02x = %04x\n", machine().describe_context(), address, data);
		break;
	}
}

void qsound_update( int num, INT16 **buffer, int length )
{
	unsigned int n;

	memset(buffer[0], 0, length * sizeof(INT16));
	memset(buffer[1], 0, length * sizeof(INT16));
	if (! qsound_sample_rom_length)
		return;

	for (n = 0; n < QSOUND_CHANNELS; ++n)
	{
		qsound_channel *pC = &channel[n];

		// Go through the buffer and add voice contributions
		const unsigned int bank = channel[(n + QSOUND_CHANNELS - 1) & (QSOUND_CHANNELS - 1)].reg[0] & 0x7fff;
		int i;
		for (i = 0; i < length; i++)
		{
			// current sample address (bank comes from previous channel)
			const unsigned int addr = pC->reg[1] | (bank << 16);

			// update based on playback rate
			long long updated = (int)((unsigned int)pC->reg[2] << 4) + (int)(((unsigned int)pC->reg[1] << 16) | pC->reg[3]);
			pC->reg[3] = (unsigned short)saturate(updated);
			if (updated >= (int)((unsigned int)pC->reg[5] << 16))
				updated -= (int)((unsigned int)pC->reg[4] << 16);
			pC->reg[1] = (unsigned short)(saturate(updated) >> 16);

			// get the scaled sample
			const int scaled = (int)((signed short)pC->reg[6]) * (signed short)((unsigned short)qsound_sample_rom[addr & qsound_sample_rom_mask] << 8);

			// apply simple panning
			buffer[0][i] += ((scaled >> 8) * pC->lvol) >> 14;
			buffer[1][i] += ((scaled >> 8) * pC->rvol) >> 14;
		}
	}

#if LOG_WAVE
	fwrite(buffer[0], length*sizeof(INT16), 1, fpRawDataL);
	fwrite(buffer[1], length*sizeof(INT16), 1, fpRawDataR);
#endif
}
