/***************************************************************************

  sn76496.c
  by Nicola Salmoria

  Routines to emulate the:
  Texas Instruments SN76489, SN76489A, SN76494/SN76496
  ( Also known as, or at least compatible with, the TMS9919.)
  and the Sega 'PSG' used on the Master System, Game Gear, and Megadrive/Genesis
  This chip is known as the Programmable Sound Generator, or PSG, and is a 4
  channel sound generator, with three squarewave channels and a noise/arbitrary
  duty cycle channel.

  Noise emulation for all chips should be accurate:
  SN76489 uses a 15-bit shift register with taps on bits D and E, output on /E
  * It uses a 15-bit ring buffer for periodic noise/arbitrary duty cycle.
  SN76489A uses a 16-bit shift register with taps on bits D and F, output on F
  * It uses a 16-bit ring buffer for periodic noise/arbitrary duty cycle.
  SN76494 and SN76496 are PROBABLY identical in operation to the SN76489A
  * They have an audio input line which is mixed with the 4 channels of output.
  Sega Master System III/MD/Genesis PSG uses a 16-bit shift register with taps
  on bits C and F, output on F
  * It uses a 16-bit ring buffer for periodic noise/arbitrary duty cycle.
  Sega Game Gear PSG is identical to the SMS3/MD/Genesis one except it has an
  extra register for mapping which channels go to which speaker.

  28/03/2005 : Sebastien Chevalier
  Update th SN76496Write func, according to SN76489 doc found on SMSPower.
   - On write with 0x80 set to 0, when LastRegister is other then TONE,
   the function is similar than update with 0x80 set to 1

  23/04/2007 : Lord Nightmare
  Major update, implement all three different noise generation algorithms plus
  the game gear stereo output, and a set_variant call to discern among them.
***************************************************************************/

#include "driver.h"

#include "../ext/vgm/vgmwrite.h"

#define MAX_OUTPUT 0x7fff

#define STEP 0x10000

struct SN76496
{
	int Channel;
	int VolTable[16];	    /* volume table         */
	INT32 Register[8];	    /* registers */
	INT32 LastRegister;	    /* last register written */
	INT32 Volume[4];		    /* volume of voice 0-2 and noise */
	UINT32 RNG;		/* noise generator      */
    INT32 NoiseMode;	    /* active noise mode */
    INT32 FeedbackMask;     /* mask for feedback */
    INT32 WhitenoiseTap1;   /* first white noise tap  */
    INT32 WhitenoiseTap2;   /* second white noise tap */
    INT32 Negate;           /* output is inverted (SN76489, SN94624) */
    INT32 ClockDivider;     /* 8 for most, 1 for the SN76494/SN94624 */
    INT32 Period[4];
	INT32 Count[4];
	INT32 Output[4];

	unsigned short vgm_idx;
};


static struct SN76496 sn[MAX_76496];



static void SN76496Write(int chip,int data)
{
	struct SN76496 *R = &sn[chip];
	int r, c;

	/* update the output buffer before changing the registers */
	stream_update(R->Channel,0);

	vgm_write(R->vgm_idx, 0x00, data, 0x00);

	if (data & 0x80)
	{
		r = (data & 0x70) >> 4;

		R->LastRegister = r;
		R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
	}
	else
        r = R->LastRegister;

    c = r / 2;
	
    switch (r)
	{
		case 0:	/* tone 0 : frequency */
		case 2:	/* tone 1 : frequency */
		case 4:	/* tone 2 : frequency */
		    if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x0f) | ((data & 0x3f) << 4);
			/* A frequency register of 0 means 0x400, not 1: the counter is 10 bit and
			   wraps. This used to clamp to STEP, i.e. period 1, turning what should be
			   the lowest tone the chip can make into an ultrasonic one */
			R->Period[c] = STEP * (R->Register[r] ? R->Register[r] : 0x400);
			if (r == 4)
			{
				/* update noise shift frequency */
				if ((R->Register[6] & 0x03) == 0x03)
					R->Period[3] = 2 * R->Period[2];
			}
			break;
		case 1:	/* tone 0 : volume */
		case 3:	/* tone 1 : volume */
		case 5:	/* tone 2 : volume */
		case 7:	/* noise  : volume */
			R->Volume[c] = R->VolTable[data & 0x0f];
			if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
			break;
		case 6:	/* noise  : frequency, mode */
			{
                int n;
			    if ((data & 0x80) == 0) R->Register[r] = (R->Register[r] & 0x3f0) | (data & 0x0f);
				n = R->Register[6];
				R->NoiseMode = (n & 4) ? 1 : 0;
				/* N/512,N/1024,N/2048,Tone #3 output */
				R->Period[3] = ((n&3) == 3) ? 2 * R->Period[2] : (STEP << (5+(n&3)));
			        /* Reset noise shifter */
				R->RNG = R->FeedbackMask; /* this is correct according to the smspower document */
				//R->RNG = 0xF35; /* this is not, but sounds better in do run run */
				R->Output[3] = R->RNG & 1;
			}
			break;
	}
}

WRITE_HANDLER( SN76489_0_w ) {	SN76496Write(0,data); }
WRITE_HANDLER( SN76489_1_w ) {	SN76496Write(1,data); }
WRITE_HANDLER( SN76489_2_w ) {	SN76496Write(2,data); }
WRITE_HANDLER( SN76489_3_w ) {	SN76496Write(3,data); }

WRITE_HANDLER( SN76489A_0_w ) { SN76496Write(0, data); }
WRITE_HANDLER( SN76489A_1_w ) { SN76496Write(1, data); }
WRITE_HANDLER( SN76489A_2_w ) { SN76496Write(2, data); }
WRITE_HANDLER( SN76489A_3_w ) { SN76496Write(3, data); }

WRITE_HANDLER( SN76494_0_w ) {	SN76496Write(0,data); }
WRITE_HANDLER( SN76494_1_w ) {	SN76496Write(1,data); }
WRITE_HANDLER( SN76494_2_w ) {	SN76496Write(2,data); }
WRITE_HANDLER( SN76494_3_w ) {	SN76496Write(3,data); }

WRITE_HANDLER( SN76496_0_w ) {	SN76496Write(0,data); }
WRITE_HANDLER( SN76496_1_w ) {	SN76496Write(1,data); }
WRITE_HANDLER( SN76496_2_w ) {	SN76496Write(2,data); }
WRITE_HANDLER( SN76496_3_w ) {	SN76496Write(3,data); }



static void SN76496Update(int chip,INT16 *buffer,int length)
{
	int i;
	struct SN76496 *R = &sn[chip];

	/* If the volume is 0, increase the counter */
	for (i = 0;i < 4;i++)
	{
		if (R->Volume[i] == 0)
		{
			/* note that I do count += length, NOT count = length + 1. You might think */
			/* it's the same since the volume is 0, but doing the latter could cause */
			/* interferencies when the program is rapidly modulating the volume. */
			if (R->Count[i] <= length*STEP) R->Count[i] += length*STEP;
		}
	}

	while (length > 0)
	{
		int vol[4];
		INT32 out;
		int left;

		/* vol[] keeps track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vol[0] = vol[1] = vol[2] = vol[3] = 0;

		for (i = 0;i < 3;i++)
		{
			if (R->Output[i]) vol[i] += R->Count[i];
			R->Count[i] -= STEP;
			/* Period[i] is the half period of the square wave. Here, in each */
			/* loop I add Period[i] twice, so that at the end of the loop the */
			/* square wave is in the same status (0 or 1) it was at the start. */
			/* vol[i] is also incremented by Period[i], since the wave has been 1 */
			/* exactly half of the time, regardless of the initial position. */
			/* If we exit the loop in the middle, Output[i] has to be inverted */
			/* and vol[i] incremented only if the exit status of the square */
			/* wave is 1. */
			while (R->Count[i] <= 0)
			{
				R->Count[i] += R->Period[i];
				if (R->Count[i] > 0)
				{
					R->Output[i] ^= 1;
					if (R->Output[i]) vol[i] += R->Period[i];
					break;
				}
				R->Count[i] += R->Period[i];
				vol[i] += R->Period[i];
			}
			if (R->Output[i]) vol[i] -= R->Count[i];
		}

		left = STEP;
		do
		{
			int nextevent;

			if (R->Count[3] < left) nextevent = R->Count[3];
			else nextevent = left;

			if (R->Output[3]) vol[3] += R->Count[3];
			R->Count[3] -= nextevent;
			if (R->Count[3] <= 0)
			{
                /* One condition for both noise modes: The two
                   taps are separate there rather than one combined mask, and the second
                   is gated on the white-noise bit - so in periodic mode this reduces to
                   feedback from tap1 alone */
                if (((R->RNG & R->WhitenoiseTap1) != 0) != (((R->RNG & R->WhitenoiseTap2) != 0) && (R->NoiseMode == 1)))
                {
                    R->RNG >>= 1;
                    R->RNG |= R->FeedbackMask;
                }
                else
                {
                    R->RNG >>= 1;
                }
                R->Output[3] = R->RNG & 1;
                
                R->Count[3] += R->Period[3];
				if (R->Output[3]) vol[3] += R->Period[3];
			}
			if (R->Output[3]) vol[3] -= R->Count[3];

			left -= nextevent;
		} while (left > 0);

		/* PINMAME: Centre on 0 rather than swinging 0..MAX: vol[] is the on-time out of STEP, so subtracting Volume[i]/2
		   takes out the pedestal. The three tone channels and the noise channel are separate, never ANDed together, so every
		   active channel is at 50% duty and the removal is exact. Peak-to-peak is unchanged, so this is not a volume change */
		out = (2*vol[0] - STEP) * R->Volume[0] + (2*vol[1] - STEP) * R->Volume[1] +
				(2*vol[2] - STEP) * R->Volume[2] + (2*vol[3] - STEP) * R->Volume[3];

		/* the SN76489/SN94624 output stage inverts */
		if (R->Negate) out = -out;

		if (out > MAX_OUTPUT * STEP) out = MAX_OUTPUT * STEP;
		else if (out < -(MAX_OUTPUT * STEP)) out = -(MAX_OUTPUT * STEP);

		*(buffer++) = out / (2*STEP);

		length--;
	}
}

static void SN76496_set_gain(int chip,int gain)
{
	struct SN76496 *R = &sn[chip];
	int i;
	double out;

	gain &= 0xff;

	/* increase max output basing on gain (0.2 dB per step) */
	out = (double)(MAX_OUTPUT / 4);
	while (gain-- > 0)
		out *= 1.023292992;	/* = (10 ^ (0.2/20)) */

	/* build volume table (2dB per step) */
	for (i = 0;i < 15;i++)
	{
		/* limit volume to avoid clipping */
		if (out > (double)(MAX_OUTPUT / 4)) R->VolTable[i] = MAX_OUTPUT / 4;
		else R->VolTable[i] = (int)out;

		out /= 1.258925412;	/* = 10 ^ (2/20) = 2dB */
	}
	R->VolTable[15] = 0;
}



static int SN76496_init(const struct MachineSound *msound,int chip,double clock,int volume,int clockdivider)
{
	int i;
	struct SN76496 *R = &sn[chip];
	char name[40];

	/* MAME runs the stream at clock/2 and then divides by the per-variant clock
	   divider, so the generators see clock/(2*divider): clock/16 for most parts,
	   but clock/2 for the SN76494 and SN94624, which divide by 1 */
	double sample_rate = clock/(2.*clockdivider);

	sprintf(name,"SN76496 #%d",chip);
	R->Channel = stream_init(name,volume,sample_rate,chip,SN76496Update);

	if (R->Channel == -1)
		return 1;

	/* Silence on reset. MAME instead leaves the volume registers at 0 - which on this
	   chip means MAX - and derives the volumes from them, so a non-Sega part hums at
	   cold boot until the game initialises it; MAME lists that under BTANB. Deliberate
	   difference: authentic, but it would make Inder and Wico beep on every start */
	for (i = 0;i < 4;i++) R->Volume[i] = 0;

	R->LastRegister = /*m_sega_style_psg?3:*/0; // Sega VDP PSG defaults to selected period reg for 2nd channel //!! m_sega_style_psg true/false?
	for (i = 0;i < 8;i+=2)
	{
		R->Register[i] = 0;
		R->Register[i + 1] = 0x0;   /* 0 = max volume. MAME resets this to 0xf only for the Sega VDP PSG, and every part used here is non-Sega, so 0x0 is right - see the note at Volume[] above for why we do not then play it */
	}

	for (i = 0;i < 4;i++)
	{
		R->Output[i] = 0;
		/* tone periods reset to 0x400 as on the chip (MAME); the noise one stays at STEP
		   rather than MAME's 0, because the period is a divisor in the loop below and 0 would spin it */
		R->Period[i] = R->Count[i] = (i < 3) ? (STEP * 0x400) : STEP;
	}

    /* Default is SN76489 non-A */
    R->FeedbackMask = 0x4000;   /* mask for feedback */
    R->WhitenoiseTap1 = 0x01;
    R->WhitenoiseTap2 = 0x02;
    R->Negate = 1;
    R->ClockDivider = clockdivider;

    R->RNG = R->FeedbackMask;
    R->Output[3] = R->RNG & 1;

	return 0;
}



static int generic_start(const struct MachineSound *msound, int feedbackmask, int noisetap1, int noisetap2, int negate, int clockdivider)
{
	int chip;
	const struct SN76496interface *intf = msound->sound_interface;
	struct SN76496 *R;

	for (chip = 0;chip < intf->num;chip++)
	{
		if (SN76496_init(msound,chip,intf->baseclock[chip],intf->volume[chip] & 0xff,clockdivider) != 0)
			return 1;

		SN76496_set_gain(chip,(intf->volume[chip] >> 8) & 0xff);

		R = &sn[chip];

		R->FeedbackMask = feedbackmask;
		R->WhitenoiseTap1 = noisetap1;
		R->WhitenoiseTap2 = noisetap2;
		R->Negate = negate;
		R->ClockDivider = clockdivider;
		/* reseed: SN76496_init ran before the real mask was known and used the
		   0x4000 default, so a part with a wider mask started with the wrong RNG */
		R->RNG = R->FeedbackMask;
		R->Output[3] = R->RNG & 1;

		R->vgm_idx = vgm_open(VGMC_SN76496, intf->baseclock[chip]); //!!
		vgm_header_set(R->vgm_idx, 0x01, R->FeedbackMask);
		vgm_header_set(R->vgm_idx, 0x02, R->WhitenoiseTap1);
		vgm_header_set(R->vgm_idx, 0x03, R->WhitenoiseTap2);
		vgm_header_set(R->vgm_idx, 0x04, R->Negate);
		vgm_header_set(R->vgm_idx, 0x05, 0); //!! m_stereo);
		vgm_header_set(R->vgm_idx, 0x06, R->ClockDivider);
		vgm_header_set(R->vgm_idx, 0x07, 0); //!! m_sega_style_psg); // how ironic that it does just the opposite of its name
	}
	return 0;
}

/* feedback mask, noise tap 1, noise tap 2, negate, clock divider - all taken MAME's sn76496.cpp. The old table had one combined tap mask, a
   noise-only invert instead of the output negate, and no divider at all; the masks for everything above the plain SN76489 were 0x8000/0x06 where MAME has 0x10000/0x04+0x08 */
int SN76489_sh_start(const struct MachineSound *msound)
{
    return generic_start(msound, 0x4000, 0x01, 0x02, 1, 8);
}

int SN76489A_sh_start(const struct MachineSound *msound)
{
    return generic_start(msound, 0x10000, 0x04, 0x08, 0, 8);
}

int SN76494_sh_start(const struct MachineSound *msound)
{
    /* divider 1, not 8: the SN76494 clocks its generators at clock/2 where the rest of the family runs at clock/16 */
    return generic_start(msound, 0x10000, 0x04, 0x08, 0, 1);
}

int SN76496_sh_start(const struct MachineSound *msound)
{
    return generic_start(msound, 0x10000, 0x04, 0x08, 0, 8);
}

int gamegear_sh_start(const struct MachineSound *msound)
{
    return generic_start(msound, 0x8000, 0x01, 0x08, 1, 8);
}

int smsiii_sh_start(const struct MachineSound *msound)
{
    return generic_start(msound, 0x8000, 0x01, 0x08, 1, 8);
}
