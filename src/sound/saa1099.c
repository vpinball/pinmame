// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller, Manuel Abadia

// mixture of MAME and VGM

/***************************************************************************

	Philips SAA1099 Sound driver

	By Juergen Buchmueller and Manuel Abadia

	SAA1099 register layout:
	========================

	offs | 7654 3210 | description
	-----+-----------+---------------------------
	0x00 | ---- xxxx | Amplitude channel 0 (left)
	0x00 | xxxx ---- | Amplitude channel 0 (right)
	0x01 | ---- xxxx | Amplitude channel 1 (left)
	0x01 | xxxx ---- | Amplitude channel 1 (right)
	0x02 | ---- xxxx | Amplitude channel 2 (left)
	0x02 | xxxx ---- | Amplitude channel 2 (right)
	0x03 | ---- xxxx | Amplitude channel 3 (left)
	0x03 | xxxx ---- | Amplitude channel 3 (right)
	0x04 | ---- xxxx | Amplitude channel 4 (left)
	0x04 | xxxx ---- | Amplitude channel 4 (right)
	0x05 | ---- xxxx | Amplitude channel 5 (left)
	0x05 | xxxx ---- | Amplitude channel 5 (right)
		 |			 |
	0x08 | xxxx xxxx | Frequency channel 0
	0x09 | xxxx xxxx | Frequency channel 1
	0x0a | xxxx xxxx | Frequency channel 2
	0x0b | xxxx xxxx | Frequency channel 3
	0x0c | xxxx xxxx | Frequency channel 4
	0x0d | xxxx xxxx | Frequency channel 5
		 |			 |
	0x10 | ---- -xxx | Channel 0 octave select
	0x10 | -xxx ---- | Channel 1 octave select
	0x11 | ---- -xxx | Channel 2 octave select
	0x11 | -xxx ---- | Channel 3 octave select
	0x12 | ---- -xxx | Channel 4 octave select
	0x12 | -xxx ---- | Channel 5 octave select
		 |			 |
	0x14 | ---- ---x | Channel 0 frequency enable (0 = off, 1 = on)
	0x14 | ---- --x- | Channel 1 frequency enable (0 = off, 1 = on)
	0x14 | ---- -x-- | Channel 2 frequency enable (0 = off, 1 = on)
	0x14 | ---- x--- | Channel 3 frequency enable (0 = off, 1 = on)
	0x14 | ---x ---- | Channel 4 frequency enable (0 = off, 1 = on)
	0x14 | --x- ---- | Channel 5 frequency enable (0 = off, 1 = on)
		 |			 |
	0x15 | ---- ---x | Channel 0 noise enable (0 = off, 1 = on)
	0x15 | ---- --x- | Channel 1 noise enable (0 = off, 1 = on)
	0x15 | ---- -x-- | Channel 2 noise enable (0 = off, 1 = on)
	0x15 | ---- x--- | Channel 3 noise enable (0 = off, 1 = on)
	0x15 | ---x ---- | Channel 4 noise enable (0 = off, 1 = on)
	0x15 | --x- ---- | Channel 5 noise enable (0 = off, 1 = on)
		 |			 |
	0x16 | ---- --xx | Noise generator parameters 0
	0x16 | --xx ---- | Noise generator parameters 1
		 |			 |
	0x18 | --xx xxxx | Envelope generator 0 parameters
	0x18 | x--- ---- | Envelope generator 0 control enable (0 = off, 1 = on)
	0x19 | --xx xxxx | Envelope generator 1 parameters
	0x19 | x--- ---- | Envelope generator 1 control enable (0 = off, 1 = on)
		 |			 |
	0x1c | ---- ---x | All channels enable (0 = off, 1 = on)
	0x1c | ---- --x- | Synch & Reset generators

    Unspecified bits should be written as zero.

***************************************************************************/

#include "driver.h"
#include "saa1099.h"
#include <math.h>

#include "../ext/vgm/vgmwrite.h"

static const int clock_divider = 256;

#define LEFT	0x00
#define RIGHT	0x01

#define BOOL UINT8

/* this structure defines a channel */
struct saa1099_channel
{
	UINT8 frequency;        /* frequency (0x00..0xff) */
	BOOL freq_enable;       /* frequency enable */
	BOOL noise_enable;      /* noise enable */
	UINT8 octave;           /* octave (0x00..0x07) */
	UINT16 amplitude[2];    /* amplitude (0x00..0x0f) */
	UINT8 envelope[2];      /* envelope (0x00..0x0f or 0x10 == off) */

	/* vars to simulate the square wave */
	int counter;
	UINT8 level;
};
#define freq(x) ((511 - x.frequency) << (8 - x.octave)) // clock / ((511 - frequency) * 2^(8 - octave))

/* this structure defines a noise channel */
struct saa1099_noise
{
	/* vars to simulate the noise generator output */
	int counter;
	int freq;
	UINT32 level;                   /* noise polynomal shifter */
};

/* this structure defines a SAA1099 chip */
struct SAA1099
{
	int stream;                     /* our stream */
	UINT8 noise_params[2];          /* noise generators parameters */
	BOOL env_enable[2];             /* envelope generators enable */
	BOOL env_reverse_right[2];      /* envelope reversed for right channel */
	UINT8 env_mode[2];              /* envelope generators mode */
	BOOL env_bits[2];               /* non zero = 3 bits resolution */
	BOOL env_clock[2];              /* envelope clock mode (non-zero external) */
	UINT8 env_step[2];              /* current envelope step */
	BOOL all_ch_enable;             /* all channels enable */
	BOOL sync_state;                /* sync all channels */
	UINT8 selected_reg;             /* selected register */
	struct saa1099_channel channels[6]; /* channels */
	struct saa1099_noise noise[2];  /* noise generators */
	double master_clock;

	unsigned short vgm_idx;
};

/* saa1099 chips */
static struct SAA1099 saa1099[MAX_SAA1099];

static const UINT16 amplitude_lookup[16] = {
	 0*32768u/16,  1*32768u/16,  2*32768u/16,  3*32768u/16,
	 4*32768u/16,  5*32768u/16,  6*32768u/16,  7*32768u/16,
	 8*32768u/16,  9*32768u/16, 10*32768u/16, 11*32768u/16,
	12*32768u/16, 13*32768u/16, 14*32768u/16, 15*32768u/16
};

static const UINT8 envelope[8][64] = {
	/* zero amplitude */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* maximum amplitude */
	{15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15, },
	/* single decay */
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* repetitive decay */
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	/* single triangular */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* repetitive triangular */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
	/* single attack */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* repetitive attack */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 }
};

static void saa1099_envelope(int chip, int ch)
{
	struct SAA1099 * const saa = &saa1099[chip];
	if (saa->env_enable[ch])
	{
		const UINT8 mode = saa->env_mode[ch];
		/* step from 0..63 and then loop in steps 32..63 */
		const UINT8 step = saa->env_step[ch] =
			((saa->env_step[ch] + 1) & 0x3f) | (saa->env_step[ch] & 0x20);

		const UINT8 mask = (saa->env_bits[ch]) ? 14 /* 3 bit resolution, mask LSB */
		                                       : 15;

		UINT8 envval = envelope[mode][step] & mask;
		saa->channels[ch*3+0].envelope[ LEFT] =
		saa->channels[ch*3+1].envelope[ LEFT] =
		saa->channels[ch*3+2].envelope[ LEFT] = envval;
		if (saa->env_reverse_right[ch] & 0x01)
			envval = (15 - envval) & mask;
		saa->channels[ch*3+0].envelope[RIGHT] =
		saa->channels[ch*3+1].envelope[RIGHT] =
		saa->channels[ch*3+2].envelope[RIGHT] = envval;
	}
	else
	{
		/* envelope mode off, set all envelope factors to 16 */
		saa->channels[ch*3+0].envelope[ LEFT] =
		saa->channels[ch*3+1].envelope[ LEFT] =
		saa->channels[ch*3+2].envelope[ LEFT] =
		saa->channels[ch*3+0].envelope[RIGHT] =
		saa->channels[ch*3+1].envelope[RIGHT] =
		saa->channels[ch*3+2].envelope[RIGHT] = 16;
	}
}


static void saa1099_update(int chip, INT16 **buffer, int length)
{
	struct SAA1099 * const saa = &saa1099[chip];
	int j;
	UINT32 ch;
	//INT32 clk2div512;

	/* if the channels are disabled we're done */
	if (!saa->all_ch_enable)
	{
		/* init output data */
		memset(buffer[ LEFT],0,length*sizeof(INT16));
		memset(buffer[RIGHT],0,length*sizeof(INT16));
		return;
	}

	for (ch = 0; ch < 2; ch++)
	{
		switch (saa->noise_params[ch])
		{
		case 0:
		case 1:
		case 2: saa->noise[ch].freq = 256 << saa->noise_params[ch]; break;
		case 3: saa->noise[ch].freq = freq(saa->channels[ch * 3]); break;
		}
	}

	// clock fix thanks to http://www.vogons.org/viewtopic.php?p=344227#p344227
	//old: clk2div512 = 2 * saa->master_clock / 512;
	//new/fixed: clk2div512 = (saa->master_clock + 128) / clock_divider;

	/* fill all data needed */
	for( j = 0; j < length; j++ )
	{
		int output_l = 0, output_r = 0;

		/* for each channel */
		for (ch = 0; ch < 6; ch++)
		{
			struct saa1099_channel* const saach = &saa->channels[ch];
			int outlvl;

			/* check the actual position in the square wave */
			while (saach->counter <= 0)
			{
				saach->counter += freq(saa->channels[ch]);
				saach->level ^= 1;

				/* eventually clock the envelope counters */
				if (ch == 1 && saa->env_clock[0] == 0)
					saa1099_envelope(chip, 0);
				else if (ch == 4 && saa->env_clock[1] == 0)
					saa1099_envelope(chip, 1);
			}
			saach->counter -= clock_divider;

			// Now with bipolar output. -Valley Bell
			outlvl = 0;
			if (saach->noise_enable)
				outlvl += (saa->noise[ch/3].level & 1) ? +1 : -1;
			if (saach->freq_enable)
				outlvl += (saach->level & 1) ? +1 : -1;
			output_l += outlvl * saach->amplitude[ LEFT] * saach->envelope[ LEFT] / 32;
			output_r += outlvl * saach->amplitude[RIGHT] * saach->envelope[RIGHT] / 32;
		}

		for (ch = 0; ch < 2; ch++)
		{
			/* update the state of the noise generator
			 * polynomial is x^18 + x^11 + x (i.e. 0x20400) and is a plain XOR, initial state is probably all 1s
			 * see http://www.vogons.org/viewtopic.php?f=9&t=51695 */
			while (saa->noise[ch].counter <= 0)
			{
				saa->noise[ch].counter += saa->noise[ch].freq; // clock / ((511 - frequency) * 2^(8 - octave)) or clock / 2^(8 + noise period)
				if( ((saa->noise[ch].level & 0x20000) == 0) != ((saa->noise[ch].level & 0x0400) == 0) )
					saa->noise[ch].level = (saa->noise[ch].level << 1) | 1;
				else
					saa->noise[ch].level <<= 1;
			}
			saa->noise[ch].counter -= clock_divider;
		}
		/* write sound data to the buffer */
		buffer[LEFT][j] = output_l / 6;
		buffer[RIGHT][j] = output_r / 6;
	}
}



int saa1099_sh_start(const struct MachineSound *msound)
{
	int i;
	const struct SAA1099_interface *intf = msound->sound_interface;

	/* for each chip allocate one stream */
	for (i = 0; i < intf->numchips; i++)
	{
		int j;
		int vol[2];
		double sample_rate;
		char buf[2][64];
		const char *name[2];
		struct SAA1099 *saa = &saa1099[i];

		memset(saa, 0, sizeof(struct SAA1099));

		saa->master_clock = 7159090.5; // = XTAL(14'318'181) / 2 //!! 4000000 8000000 6000000 ?
		sample_rate = saa->master_clock / clock_divider; // current MAME
		//sample_rate = saa->master_clock / 128.0 * 8; // previously / VGMPlay

		for (j = 0; j < 2; j++)
		{
			sprintf(buf[j], "SAA1099 #%d", i);
			name[j] = buf[j];
			vol[j] = MIXER(intf->volume[i][j], j ? MIXER_PAN_RIGHT : MIXER_PAN_LEFT);
		}
		saa->stream = stream_init_multi(2, name, vol, sample_rate, i, saa1099_update);

		saa->vgm_idx = vgm_open(VGMC_SAA1099, saa->master_clock);
	}

	return 0;
}

void saa1099_sh_stop(void)
{
}

static void saa1099_control_port_w( int chip, int reg, int data )
{
	struct SAA1099 * const saa = &saa1099[chip];

	if ((data & 0xff) > 0x1c)
	{
		/* Error! */
		logerror("%04x: (SAA1099 #%d) Unknown register selected\n",activecpu_get_pc(), chip);
	}

	saa->selected_reg = data & 0x1f;
	if (saa->selected_reg == 0x18 || saa->selected_reg == 0x19)
	{
		/* clock the envelope channels */
		if (saa->env_clock[0])
			saa1099_envelope(chip,0);
		if (saa->env_clock[1])
			saa1099_envelope(chip,1);
	}
}


static void saa1099_write_port_w( int chip, int offset, int data )
{
	struct SAA1099 * const saa = &saa1099[chip];
	const int reg = saa->selected_reg;
	int ch;

	/* first update the stream to this point in time */
	stream_update(saa->stream, 0);

	vgm_write(saa->vgm_idx, 0x00, reg & 0x7F, data);

	switch (reg)
	{
	/* channel i amplitude */
	case 0x00:  case 0x01:  case 0x02:  case 0x03:  case 0x04:  case 0x05:
		ch = reg & 7;
		saa->channels[ch].amplitude[LEFT] = amplitude_lookup[data & 0x0f];
		saa->channels[ch].amplitude[RIGHT] = amplitude_lookup[(data >> 4) & 0x0f];
		break;
	/* channel i frequency */
	case 0x08:  case 0x09:  case 0x0a:  case 0x0b:  case 0x0c:  case 0x0d:
		ch = reg & 7;
		saa->channels[ch].frequency = data & 0xff;
		break;
	/* channel i octave */
	case 0x10:  case 0x11:  case 0x12:
		ch = (reg - 0x10) << 1; // (reg & 0x03) << 1;
		saa->channels[ch + 0].octave = data & 0x07;
		saa->channels[ch + 1].octave = (data >> 4) & 0x07;
		break;
	/* channel i frequency enable */
	case 0x14:
		saa->channels[0].freq_enable =  data & 0x01;
		saa->channels[1].freq_enable = (data & 0x02) ? 1 : 0;
		saa->channels[2].freq_enable = (data & 0x04) ? 1 : 0;
		saa->channels[3].freq_enable = (data & 0x08) ? 1 : 0;
		saa->channels[4].freq_enable = (data & 0x10) ? 1 : 0;
		saa->channels[5].freq_enable = (data & 0x20) ? 1 : 0;
		break;
	/* channel i noise enable */
	case 0x15:
		saa->channels[0].noise_enable =  data & 0x01;
		saa->channels[1].noise_enable = (data & 0x02) ? 1 : 0;
		saa->channels[2].noise_enable = (data & 0x04) ? 1 : 0;
		saa->channels[3].noise_enable = (data & 0x08) ? 1 : 0;
		saa->channels[4].noise_enable = (data & 0x10) ? 1 : 0;
		saa->channels[5].noise_enable = (data & 0x20) ? 1 : 0;
		break;
	/* noise generators parameters */
	case 0x16:
		saa->noise_params[0] = data & 0x03;
		saa->noise_params[1] = (data >> 4) & 0x03;
		break;
	/* envelope generators parameters */
	case 0x18:  case 0x19:
		ch = reg - 0x18; // reg & 0x01;
		saa->env_reverse_right[ch] = data & 0x01;
		saa->env_mode[ch] = (data >> 1) & 0x07;
		saa->env_bits[ch] = (data & 0x10) ? 1 : 0;
		saa->env_clock[ch] = (data & 0x20) ? 1 : 0;
		saa->env_enable[ch] = (data & 0x80) ? 1 : 0;
		/* reset the envelope */
		saa->env_step[ch] = 0;
		break;
	/* channels enable & reset generators */
	case 0x1c:
		saa->all_ch_enable = data & 0x01;
		saa->sync_state = (data & 0x02) ? 1 : 0;
		if (data & 0x02)
		{
			int i;

			/* Synch & Reset generators */
			logerror("%04x: (SAA1099 #%d) -reg 0x1c- Chip reset\n",activecpu_get_pc(), chip);
			for (i = 0; i < 6; i++)
			{
				saa->channels[i].level = 0;
				saa->channels[i].counter = freq(saa->channels[i]);
			}
		}
		break;
	default:    /* Error! */
		logerror("%04x: (SAA1099 #%d) Unknown operation (reg:%02x, data:%02x)\n",activecpu_get_pc(), chip, reg, data);
		break;
	}
}


/*******************************************
	SAA1099 interface functions
*******************************************/

WRITE_HANDLER( saa1099_control_port_0_w )
{
	saa1099_control_port_w(0, offset, data);
}

WRITE_HANDLER( saa1099_write_port_0_w )
{
	saa1099_write_port_w(0, offset, data);
}

WRITE_HANDLER( saa1099_control_port_1_w )
{
	saa1099_control_port_w(1, offset, data);
}

WRITE_HANDLER( saa1099_write_port_1_w )
{
	saa1099_write_port_w(1, offset, data);
}

WRITE16_HANDLER( saa1099_control_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		saa1099_control_port_w(0, offset, data & 0xff);
}

WRITE16_HANDLER( saa1099_write_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		saa1099_write_port_w(0, offset, data & 0xff);
}

WRITE16_HANDLER( saa1099_control_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		saa1099_control_port_w(1, offset, data & 0xff);
}

WRITE16_HANDLER( saa1099_write_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		saa1099_write_port_w(1, offset, data & 0xff);
}
