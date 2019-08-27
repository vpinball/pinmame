/**********************************************************************************************
 *
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *	 HJB 08/31/98
 *	 modified to use an automatically selected oversampling factor
 *	 for the current Machine->sample_rate
 *
 *   Mish 21/7/99
 *   Updated to allow multiple OKI chips with different sample rates
 * 
 *   OKI ADPCM emulation:
 *
 *   See the following patents/applications:
 *   (Note: if not registered, the application below was not actually granted, and is effectively abandoned)
 *   Application JP,1980-109800 (Unexamined Publication JP,S57-035434,A) (Examined Publication JP,S61-024850,B) (Registration number JP,1356613,B) https://patents.google.com/patent/JPS5735434A/en
 *   Application JP,1981-185490 (Unexamined Publication JP,S58-088926,A) (Not examined or registered) https://patents.google.com/patent/JPS5888926A/en
 *   Application JP,1982-213971 (Unexamined Publication JP,S59-104699,A) (Not examined or registered) https://patents.google.com/patent/JPS59104699A/en <- this one goes into a bit more detail/better arranged diagrams, and shows a Q table with entries 0-63 rather than 0-48 of the real msm5205
 *
 *   Application JP,1987-184421 (Unexamined Publication JP,S64-028700,A) (Not examined) (Registration number JP,2581696,B) https://patents.google.com/patent/JPS6428700A/en <- quad band coding system for adpcm?
 *   Application JP,1994-039523 (Unexamined Publication JP,H07-248798,A) (Not examined) (Registration number JP,3398457,B) https://patents.google.com/patent/JP3398457B2/en <- this may cover the 'adpcm2' method
 *   Application JP,1995-104333 (Unexamined Publication JP,H08-307371,A) (Not examined or registered) https://patents.google.com/patent/JPH08307371A/en <- something unrelated to adpcm, wireless transmission error detection related?
 *   Application JP,1995-162009 (Unexamined Publication JP,H09-018425,A) (Not examined or registered) https://patents.google.com/patent/JPH0918425A/en <- looks like ADPCM2 maybe?
 *   Application JP,1988-176215 (Unexamined Publication JP,H02-026426,A) (Not examined or registered) https://patents.google.com/patent/JPH0226426A/en <- Fujitsu variant on (G.726/727?) SB-ADPCM, cited by above
 *
 *
 **********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "state.h"
#include "adpcm.h"


//#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//define LOG(x)	printf x
#else
#define LOG(x)
#endif

/* struct describing a single playing ADPCM voice */
struct ADPCMVoice
{
	int stream;				/* which stream are we playing on? */
	UINT8 playing;			/* 1 if we are actively playing */

	UINT8 *region_base;		/* pointer to the base of the region */
	UINT8 *base;			/* pointer to the base memory location */
	UINT32 sample;			/* current sample number */
	UINT32 count;			/* total samples to play */

	UINT32 signal;			/* current ADPCM signal */
	UINT32 step;			/* current ADPCM step */
	UINT32 volume;			/* output volume */
#ifdef PINMAME
	int is6376;
#endif
};

/* array of ADPCM voices */
static UINT8 num_voices;
static struct ADPCMVoice adpcm[MAX_ADPCM];

/* step size index shift table */
static int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

/* volume lookup table */
//static UINT32 volume_table[16];

// volume lookup table. The manual lists only 9 steps, ~3dB per step. Given the dB values,
// that seems to map to a 5-bit volume control. Any volume parameter beyond the 9th index
// results in silent playback.
const UINT8 okim6295_volume_table[16] =
{
	0x20,   //   0 dB
	0x16,   //  -3.2 dB
	0x10,   //  -6.0 dB
	0x0b,   //  -9.2 dB
	0x08,   // -12.0 dB
	0x06,   // -14.5 dB
	0x04,   // -18.0 dB
	0x03,   // -20.5 dB
	0x02,   // -24.0 dB
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

/**********************************************************************************************

     compute_tables -- compute the difference tables

***********************************************************************************************/

static void compute_tables(void)
{
	/* nibble to bit map */
	static int nbl2bit[16][4] =
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
		int stepval = (int)floor(16.0 * pow(11.0 / 10.0, (double)step));

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

	/* generate the OKI6295 volume table */
	//for (step = 0; step < 16; step++)
	//{
	//	double out = 256.0;
	//	int vol = step;
	//
	//	/* 3dB per step */
	//	while (vol-- > 0)
	//		out /= 1.412537545;	/* = 10 ^ (3/20) = 3dB */
	//	volume_table[step] = (UINT32)out;
	//}
}

/**********************************************************************************************

     clock_adpcm -- clock the next ADPCM byte

***********************************************************************************************/

static INT16 clock_adpcm(struct ADPCMVoice *voice, UINT8 nibble)
{
	int signal = voice->signal;
	int step = voice->step;

	signal += diff_lookup[step * 16 + (nibble & 15)];

	/* clamp to the maximum 12bit */
	if (signal > 2047)
		signal = 2047;
	else if (signal < -2048)
		signal = -2048;

	/* adjust the step size and clamp */
	step += index_shift[nibble & 7];
	if (step > 48)
		step = 48;
	else if (step < 0)
		step = 0;

	voice->signal = signal;
	voice->step = step;

	return signal;
}

/**********************************************************************************************

     generate_adpcm -- general ADPCM decoding routine

***********************************************************************************************/

static void generate_adpcm(struct ADPCMVoice *voice, INT16 *buffer, int samples)
{
	float last_sample = 0;

	/* if this voice is active */
	if (voice->playing)
	{
		UINT8 *base = voice->base;
		int sample = voice->sample;
		int count = voice->count;

		/* loop while we still have samples to generate */
		while (samples)
		{
			/* compute the new amplitude and update the current step */
			int nibble = base[sample / 2] >> (((sample & 1) << 2) ^ 4);

			/* output to the buffer, scaling by the volume */
			*buffer++ = (INT32)clock_adpcm(voice, nibble) * (INT32)voice->volume / 2;
			samples--;

			/* next! */
			if (++sample >= count)
			{
				last_sample = *(buffer-1);
				voice->playing = 0;
				break;
			}
		}

		/* update the parameters */
		voice->sample = sample;
	}

	/* for the rest: fade to silence */ //!! correct??
	while (samples--)
	{
		last_sample *= 0.95f;
		*buffer++ = (INT16)last_sample;
	}
}

#ifdef PINMAME
static void generate_adpcm_6376(struct ADPCMVoice *voice, INT16 *buffer, int samples)
{
	float last_sample = 0;

	/* if this voice is active */
	if (voice->playing)
	{
		UINT8 *base = voice->base;
		int sample = voice->sample;
		int count = voice->count;

		/* loop while we still have samples to generate */
		while (samples)
		{
			int nibble;

			if (count == 0)
			{
				/* get the number of samples to play */
				count = (base[sample / 2] & 0x7f) << 1;

				/* end of voice marker */
				if (count == 0)
				{
					last_sample = *(buffer-1);
					voice->playing = 0;
					break;
				}
				else
				{
					/* step past the count byte */
					sample += 2;
				}
			}

			/* compute the new amplitude and update the current step */
			nibble = base[sample / 2] >> (((sample & 1) << 2) ^ 4);

			/* output to the buffer, scaling by the volume */
			/* signal in range -2048..2047, volume in range 2..32 => signal * volume / 2 in range -32768..32767 */
			*buffer++ = (INT32)clock_adpcm(voice, nibble) * (INT32)voice->volume / 2;

			++sample;
			--count;
			--samples;
		}

		/* update the parameters */
		voice->sample = sample;
		voice->count = count;
	}

	/* for the rest: fade to silence */ //!! correct??
	while (samples--)
	{
		last_sample *= 0.95f;
		*buffer++ = (INT16)last_sample;
	}
}
#endif

/**********************************************************************************************

     adpcm_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void adpcm_update(int num, INT16 *buffer, int length)
{
	struct ADPCMVoice *voice = &adpcm[num];

	/* generate them into our buffer */
#ifdef PINMAME
	if (voice->is6376)
		generate_adpcm_6376(voice, buffer, length);
	else
#endif
	generate_adpcm(voice, buffer, length);
}



/**********************************************************************************************

     state save support for MAME

***********************************************************************************************/

static UINT32 voice_base_offset[MAX_ADPCM]; /*we cannot save the pointer - this is a workaround*/
static void adpcm_state_save_base_store (void)
{
	int i;
	struct ADPCMVoice *voice;

	for (i=0; i<num_voices; i++)
	{
		voice = &adpcm[i];
		voice_base_offset[i] = (UINT32)(voice->base - voice->region_base);
	}
}

static void adpcm_state_save_base_refresh (void)
{
	int i;
	struct ADPCMVoice *voice;

	for (i=0; i<num_voices; i++)
	{
		voice = &adpcm[i];
		voice->base = &voice->region_base[ voice_base_offset[i] ];
	}
}

static void adpcm_state_save_register( void )
{
	int i;
	char buf[20];
	struct ADPCMVoice *voice;


	sprintf(buf,"ADPCM");

	for (i=0; i<num_voices; i++)
	{
		voice = &adpcm[i];

		state_save_register_UINT8  (buf, i, "playing", &voice->playing, 1);
		state_save_register_UINT32 (buf, i, "base_offset" , &voice_base_offset[i],  1);
		state_save_register_UINT32 (buf, i, "sample" , &voice->sample,  1);
		state_save_register_UINT32 (buf, i, "count"  , &voice->count,   1);
		state_save_register_UINT32 (buf, i, "signal" , &voice->signal,  1);
		state_save_register_UINT32 (buf, i, "step"   , &voice->step,    1);
		state_save_register_UINT32 (buf, i, "volume" , &voice->volume,  1);
	}
	state_save_register_func_presave(adpcm_state_save_base_store);
	state_save_register_func_postload(adpcm_state_save_base_refresh);
}

/**********************************************************************************************

     ADPCM_sh_start -- start emulation of several ADPCM output streams

***********************************************************************************************/

int ADPCM_sh_start(const struct MachineSound *msound)
{
	const struct ADPCMinterface *intf = msound->sound_interface;
	char stream_name[40];
	int i;

	/* reset the ADPCM system */
	num_voices = intf->num;
	compute_tables();

	/* initialize the voices */
	memset(adpcm, 0, sizeof(adpcm));
	for (i = 0; i < num_voices; i++)
	{
		/* generate the name and create the stream */
		sprintf(stream_name, "%s #%d", sound_name(msound), i);
		adpcm[i].stream = stream_init(stream_name, intf->mixing_level[i], intf->frequency, i, adpcm_update);
		if (adpcm[i].stream == -1)
			return 1;

		/* initialize the rest of the structure */
		adpcm[i].region_base = memory_region(intf->region);
		adpcm[i].volume = 0x20;
		adpcm[i].signal = -2;
	}

	adpcm_state_save_register();

	/* success */
	return 0;
}



/**********************************************************************************************

     ADPCM_sh_stop -- stop emulation of several ADPCM output streams

***********************************************************************************************/

void ADPCM_sh_stop(void)
{
}



/**********************************************************************************************

     ADPCM_sh_update -- update ADPCM streams

***********************************************************************************************/

void ADPCM_sh_update(void)
{
}



/**********************************************************************************************

     ADPCM_play -- play data from a specific offset for a specific length

***********************************************************************************************/

void ADPCM_play(int num, int offset, int length)
{
	struct ADPCMVoice *voice = &adpcm[num];

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= num_voices)
	{
		LOG(("error: ADPCM_trigger() called with channel = %d, but only %d channels allocated\n", num, num_voices));
		return;
	}

	/* update the ADPCM voice */
	stream_update(voice->stream, 0);

	/* set up the voice to play this sample */
	voice->playing = 1;
	voice->base = &voice->region_base[offset];
	voice->sample = 0;
	voice->count = length;

	/* also reset the ADPCM parameters */
	voice->signal = -2;
	voice->step = 0;
}



/**********************************************************************************************

     ADPCM_play -- stop playback on an ADPCM data channel

***********************************************************************************************/

void ADPCM_stop(int num)
{
	struct ADPCMVoice *voice = &adpcm[num];

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= num_voices)
	{
		LOG(("error: ADPCM_stop() called with channel = %d, but only %d channels allocated\n", num, num_voices));
		return;
	}

	/* update the ADPCM voice */
	stream_update(voice->stream, 0);

	/* stop playback */
	voice->playing = 0;
}



/**********************************************************************************************

     ADPCM_setvol -- change volume on an ADPCM data channel

***********************************************************************************************/

void ADPCM_setvol(int num, int vol)
{
	struct ADPCMVoice *voice = &adpcm[num];

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= num_voices)
	{
		LOG(("error: ADPCM_setvol() called with channel = %d, but only %d channels allocated\n", num, num_voices));
		return;
	}

	/* update the ADPCM voice */
	stream_update(voice->stream, 0);
	voice->volume = vol;
}



/**********************************************************************************************

     ADPCM_playing -- returns true if an ADPCM data channel is still playing

***********************************************************************************************/

int ADPCM_playing(int num)
{
	struct ADPCMVoice *voice = &adpcm[num];

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return 0;

	/* range check the numbers */
	if (num >= num_voices)
	{
		LOG(("error: ADPCM_playing() called with channel = %d, but only %d channels allocated\n", num, num_voices));
		return 0;
	}

	/* update the ADPCM voice */
	stream_update(voice->stream, 0);
	return voice->playing;
}



/**********************************************************************************************
 *
    OKIM 6295 ADPCM chip:

    Command bytes are sent:

        1xxx xxxx = start of 2-byte command sequence, xxxxxxx is the sample
                    number to trigger
        abcd vvvv = second half of command; one of the abcd bits is set to
                    indicate which voice the v bits seem to be volumed

        0abc d000 = stop playing; one or more of the abcd bits is set to
                    indicate which voice(s)

    Status is read:

        ???? abcd = one bit per voice, set to 0 if nothing is playing, or
                    1 if it is active
 *
***********************************************************************************************/

#ifdef PINMAME
static int OKIM6295_VOICES = 4;

static INT32 okim6295_command[MAX_OKIM6295];
static INT32 okim6295_base[MAX_OKIM6295][4];
#else
#define OKIM6295_VOICES		4

static INT32 okim6295_command[MAX_OKIM6295];
static INT32 okim6295_base[MAX_OKIM6295][OKIM6295_VOICES];
#endif


/**********************************************************************************************

     state save support for MAME

***********************************************************************************************/

static void okim6295_state_save_register(void)
{
	int i,j;
	int chips;
	char buf[20];
	char buf2[20];

	adpcm_state_save_register();
	sprintf(buf,"OKIM6295");

	chips = num_voices / OKIM6295_VOICES;
	for (i = 0; i < chips; i++)
	{
		state_save_register_INT32  (buf, i, "command", &okim6295_command[i], 1);
		for (j = 0; j < OKIM6295_VOICES; j++)
		{
			sprintf(buf2,"base_voice_%1i",j);
			state_save_register_INT32  (buf, i, buf2, &okim6295_base[i][j], 1);
		}
	}
}



/**********************************************************************************************

     OKIM6295_sh_start -- start emulation of an OKIM6295-compatible chip

***********************************************************************************************/

int OKIM6295_sh_start(const struct MachineSound *msound)
{
	const struct OKIM6295interface *intf = msound->sound_interface;
	char stream_name[40];
	int i;

	/* reset the ADPCM system */
#ifdef PINMAME // OKI6376 has only 2 voices per chip, activated by num <= 0!
  if (intf->num < 1) { OKIM6295_VOICES = 2; num_voices = 2; } else
#endif
	num_voices = intf->num * OKIM6295_VOICES;
	compute_tables();

	/* initialize the voices */
	memset(adpcm, 0, sizeof(adpcm));
	for (i = 0; i < num_voices; i++)
	{
		int chip = i / OKIM6295_VOICES;
		int voice = i % OKIM6295_VOICES;

		/* reset the OKI-specific parameters */
		okim6295_command[chip] = -1;
		okim6295_base[chip][voice] = 0;

		/* generate the name and create the stream */
#ifdef PINMAME
		if (intf->num < 1) {
			adpcm[i].is6376 = 1;
			sprintf(stream_name, "MSM6376 #%d (voice %d)", chip, voice);
		}
		else
#endif
		sprintf(stream_name, "%s #%d (voice %d)", sound_name(msound), chip, voice);
		adpcm[i].stream = stream_init(stream_name, intf->mixing_level[chip], (int)intf->frequency[chip], i, adpcm_update);
		if (adpcm[i].stream == -1)
			return 1;

		/* initialize the rest of the structure */
		adpcm[i].region_base = memory_region(intf->region[chip]);
		adpcm[i].volume = 0x20;
		adpcm[i].signal = -2;
	}

	okim6295_state_save_register();

	/* success */
	return 0;
}



/**********************************************************************************************

     OKIM6295_sh_stop -- stop emulation of an OKIM6295-compatible chip

***********************************************************************************************/

void OKIM6295_sh_stop(void)
{
}



/**********************************************************************************************

     OKIM6295_sh_update -- update emulation of an OKIM6295-compatible chip

***********************************************************************************************/

void OKIM6295_sh_update(void)
{
}



/**********************************************************************************************

     OKIM6295_set_bank_base -- set the base of the bank for a given voice on a given chip

***********************************************************************************************/

void OKIM6295_set_bank_base(int which, int base)
{
	int channel;

	for (channel = 0; channel < OKIM6295_VOICES; channel++)
	{
		struct ADPCMVoice *voice = &adpcm[which * OKIM6295_VOICES + channel];

		/* update the stream and set the new base */
		stream_update(voice->stream, 0);
		okim6295_base[which][channel] = base;
	}
}



/**********************************************************************************************

     OKIM6295_set_frequency -- dynamically adjusts the frequency of a given ADPCM voice

***********************************************************************************************/

void OKIM6295_set_frequency(int which, double frequency)
{
	int channel;

	for (channel = 0; channel < OKIM6295_VOICES; channel++)
	{
		struct ADPCMVoice *voice = &adpcm[which * OKIM6295_VOICES + channel];

		/* update the stream and set the new base */
		stream_update(voice->stream, 0);
		stream_set_sample_rate(voice->stream, (int)frequency);
	}
}


/**********************************************************************************************

     OKIM6295_status_r -- read the status port of an OKIM6295-compatible chip

***********************************************************************************************/

static int OKIM6295_status_r(int num)
{
	int i, result;

	/* range check the numbers */
	if (num >= num_voices / OKIM6295_VOICES)
	{
		LOG(("error: OKIM6295_status_r() called with chip = %d, but only %d chips allocated\n",num, num_voices / OKIM6295_VOICES));
		return 0xff;
	}

	result = 0xf0;	/* naname expects bits 4-7 to be 1 */
	/* set the bit to 1 if something is playing on a given channel */
	for (i = 0; i < OKIM6295_VOICES; i++)
	{
		struct ADPCMVoice *voice = &adpcm[num * OKIM6295_VOICES + i];

		/* update the stream */
		stream_update(voice->stream, 0);

		/* set the bit if it's playing */
		if (voice->playing)
			result |= 1 << i;
	}

	return result;
}



/**********************************************************************************************

     OKIM6295_data_w -- write to the data port of an OKIM6295-compatible chip

***********************************************************************************************/

static void OKIM6295_data_w(int num, int data)
{
	/* range check the numbers */
	if (num >= num_voices / OKIM6295_VOICES)
	{
		LOG(("error: OKIM6295_data_w() called with chip = %d, but only %d chips allocated\n", num, num_voices / OKIM6295_VOICES));
		return;
	}

	/* if a command is pending, process the second half */
	if (okim6295_command[num] != -1)
	{
		// the manual explicitly says that it's not possible to start multiple voices at the same time
		int voicemask = data >> 4, i;

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte) */
		for (i = 0; i < OKIM6295_VOICES; i++, voicemask >>= 1)
		{
			if (voicemask & 1)
			{
				struct ADPCMVoice *voice = &adpcm[num * OKIM6295_VOICES + i];
				unsigned char *base;
				int start, stop;

				/* update the stream */
				stream_update(voice->stream, 0);

				if (Machine->sample_rate == 0) return;

				/* determine the start/stop positions */
				base = &voice->region_base[okim6295_base[num][i] + okim6295_command[num] * 8];
				/*if ((int)base < 0x400) { // avoid access violations // this cannot work like this, was supposed to "fix" Caribbean Cruise
					start = stop = 0x40000;
				} else {*/
					start = (base[0] << 16) + (base[1] << 8) + base[2];
					stop = (base[3] << 16) + (base[4] << 8) + base[5];
					//start &= 0x3ffff;
					//stop &= 0x3ffff;
				//}
				LOG(("OKIM6295:%d playing sample %02x [%05x:%05x] on voice #%x\n", num, okim6295_command[num], start, stop, i+1));
				/* set up the voice to play this sample */
				if (start >= stop) {
					LOG(("OKIM6295:%d empty data - ignore\n", num));
					//voice->playing = 0; // needed?
				} else if (start < 0x40000 && stop < 0x40000) {
					if (!voice->playing) /* fixes Got-cha and Steel Force */
					{
						voice->playing = 1;
						voice->base = &voice->region_base[okim6295_base[num][i] + start];
						voice->sample = 0;
						voice->count = 2 * (stop - start + 1);

						/* also reset the ADPCM parameters */
						voice->signal = -2;
						voice->step = 0;
						voice->volume = okim6295_volume_table[data & 0x0f];
					}
					else
					{
						LOG(("OKIM6295:%d requested to play sample %02x on non-stopped voice #%x\n",num,okim6295_command[num],i+1));
					}
				}
				/* invalid samples go here */
				else
				{
					LOG(("OKIM6295:%d requested to play invalid sample %02x on voice #%x\n",num,okim6295_command[num],i+1));
					voice->playing = 0;
				}
			}
		}

		/* reset the command */
		okim6295_command[num] = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		okim6295_command[num] = data & 0x7f;
	}

	/* otherwise, this is a silence command */
	else
	{
		int voicemask = data >> 3, i;
		// TODO either mute all channels or only one - fixes some sound in GTS3 games!?
		if (voicemask == 0x0f || voicemask == 0x08 || voicemask == 0x04 || voicemask == 0x02 || voicemask == 0x01) {
			/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
			for (i = 0; i < 4; i++, voicemask >>= 1)
			{
				if (voicemask & 1)
				{
					struct ADPCMVoice *voice = &adpcm[num * OKIM6295_VOICES + i];
					LOG(("OKIM6295:%d mute voice #%x\n", num, i+1));
					if (voice->playing) {

						/* update the stream, then turn it off */
						stream_update(voice->stream, 0);
						voice->playing = 0;
					}
				}
			}
		} else {
			LOG(("OKIM6295:%d ignoring mute command 0x%02x\n", num, data));
		}
	}
}

#ifdef PINMAME
/**********************************************************************************************

     OKIM6376_data_w -- write to the data port of an OKIM6376-compatible chip

***********************************************************************************************/

static void OKIM6376_data_w(int num, int data)
{
	/* if a command is pending, process the second half */
	if (okim6295_command[num] != -1)
	{
		int temp = data >> 4, i, start;
		unsigned char *base;

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte) */
		for (i = 0; i < OKIM6295_VOICES; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &adpcm[num * OKIM6295_VOICES + i];

				/* update the stream */
				stream_update(voice->stream, 0);

				if (Machine->sample_rate == 0) return;

				/* determine the start position, max address space is 16Mbit */
				base = &voice->region_base[ okim6295_base[num][i] + okim6295_command[num] * 4];
				start = ((base[0] << 16) + (base[1] << 8) + base[2]) & 0x1fffff;

				if (start == 0)
				{
					voice->playing = 0;
				}
				else
				{
					/* set up the voice to play this sample */
					if (!voice->playing)
					{
						voice->playing = 1;
						voice->base = &voice->region_base[okim6295_base[num][i] + start];
						voice->sample = 0;
						voice->count = 0;

						/* also reset the ADPCM parameters */
						voice->signal = -2;
						voice->step = 0;
						voice->volume = 0x20;
					}
					else
					{
						LOG(("OKIM6376:%d requested to play sample %02x on non-stopped voice #%x\n",num,okim6295_command[num],i+1));
					}
				}
			}
		}

		/* reset the command */
		okim6295_command[num] = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		// FIX: maximum adpcm words are 111, there are other 8 commands to generate BEEP tone (0x70 to 0x77),
		// and others for internal testing, that manual explicitly says not to use (0x78 to 0x7f)
		okim6295_command[num] = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i;

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < OKIM6295_VOICES; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = &adpcm[num * OKIM6295_VOICES + i];

				voice->playing = 0;
			}
		}
	}
}
#endif

/**********************************************************************************************

     OKIM6295_status_0_r -- generic status read functions
     OKIM6295_status_1_r

***********************************************************************************************/

READ_HANDLER( OKIM6295_status_0_r )
{
	return OKIM6295_status_r(0);
}

READ_HANDLER( OKIM6295_status_1_r )
{
	return OKIM6295_status_r(1);
}

READ_HANDLER( OKIM6295_status_2_r )
{
	return OKIM6295_status_r(2);
}

READ16_HANDLER( OKIM6295_status_0_lsb_r )
{
	return OKIM6295_status_r(0);
}

READ16_HANDLER( OKIM6295_status_1_lsb_r )
{
	return OKIM6295_status_r(1);
}

READ16_HANDLER( OKIM6295_status_2_lsb_r )
{
	return OKIM6295_status_r(2);
}

READ16_HANDLER( OKIM6295_status_0_msb_r )
{
	return OKIM6295_status_r(0) << 8;
}

READ16_HANDLER( OKIM6295_status_1_msb_r )
{
	return OKIM6295_status_r(1) << 8;
}

READ16_HANDLER( OKIM6295_status_2_msb_r )
{
	return OKIM6295_status_r(2) << 8;
}



/**********************************************************************************************

     OKIM6295_data_0_w -- generic data write functions
     OKIM6295_data_1_w

***********************************************************************************************/

WRITE_HANDLER( OKIM6295_data_0_w )
{
	OKIM6295_data_w(0, data);
}

WRITE_HANDLER( OKIM6295_data_1_w )
{
	OKIM6295_data_w(1, data);
}

WRITE_HANDLER( OKIM6295_data_2_w )
{
	OKIM6295_data_w(2, data);
}

WRITE16_HANDLER( OKIM6295_data_0_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(0, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_1_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(1, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_2_lsb_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_w(2, data & 0xff);
}

WRITE16_HANDLER( OKIM6295_data_0_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(0, data >> 8);
}

WRITE16_HANDLER( OKIM6295_data_1_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(1, data >> 8);
}

WRITE16_HANDLER( OKIM6295_data_2_msb_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_w(2, data >> 8);
}
#ifdef PINMAME
WRITE_HANDLER( OKIM6376_data_0_w )
{
	OKIM6376_data_w(0, data);
}
#endif
