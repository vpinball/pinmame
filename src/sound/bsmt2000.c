/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 *   Chip is actually a TMS320C15 DSP with embedded mask rom
 *   Trivia: BSMT stands for "Brian Schmidt's Mouse Trap"
 *
 *   Modifications for PINMAME by Steve Ellenoff & Martin Adrian & Carsten Waechter
 *
 *             - TODO: 1)Batman,ST25th and Hook set Mode 0 (after setting Mode 1), but still use reg mapping of Mode 1
 *             -       2)Remove DE Rom loading flag & fix in the drivers themselves.
 *             -       3)Fix bsmt interface to handle reverse left/right stereo channels rather than in emulation here
 *             -       4)Command 0x77 could be the sample rate/'pitch' for ADPCM? Or volume like it is now implemented (e.g. sound played when BSMT DMD animation comes on in stereo games: https://www.youtube.com/watch?v=2FtzLzbapZs)
 *             - DONE: 5)Monopoly and RCT do never set the right volume (as these are mono only), thus a special hack is necessary to make up for that (right_volume_set)
 **********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"

#ifndef MIN
 #define MIN(x,y) ((x)<(y)?(x):(y))
#endif

/**********************************************************************************************

     CONSTANTS

***********************************************************************************************/

#define LOG_COMMANDS			0

#define MAX_VOICES				(12+1) // last one is adpcm
#define ADPCM_VOICE				12

static const UINT8 regmap[8][7] = {
    { 0x00, 0x18, 0x24, 0x30, 0x3c, 0x48, 0xff }, // last one (stereo/leftvol) unused, set to max for mapping
    { 0x00, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // mode 2 only a testmode left channel
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // mode 3 only a testmode right channel
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // mode 4 only a testmode left channel
    { 0x00, 0x18, 0x24, 0x30, 0x3c, 0x54, 0x60 },
    { 0x00, 0x10, 0x18, 0x20, 0x28, 0x38, 0x40 },
    { 0x00, 0x12, 0x1b, 0x24, 0x2d, 0x3f, 0x48 } };

/* NOTE: the original chip did not support interpolation, but music usually sounds */
/* nicer if you enable it. For accuracy's sake, we leave it off by default, also sound effects sound clearer without it. */
#define ENABLE_INTERPOLATION 0

// Whacky ADPCM interpolation, better leave off for now
#define ENABLE_ADPCM_INTERPOLATION 0

#ifdef PINMAME
#define	LOG_COMPRESSED_ONLY	0
#endif

//#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//#define LOG(x) printf x
#else
#define LOG(x)
#endif

#define MAX_SAMPLE_CHUNK		10000

#define REG_CURRPOS				0
#define REG_RATE				1
#define REG_LOOPEND				2
#define REG_LOOPSTART			3
#define REG_BANK				4
#define REG_RIGHTVOL			5
#define REG_LEFTVOL				6
#define REG_TOTAL				7

/**********************************************************************************************

     INTERNAL DATA STRUCTURES

***********************************************************************************************/

/* struct describing a single playing voice */
struct BSMT2000Voice
{
	/* external state */
    UINT16      reg[REG_TOTAL];         /* 7 registers */

    UINT16      fraction;               /* just for temporary sample calc */
};

struct BSMT2000Chip
{
	int			stream;					/* which stream are we using */
    int			sample_rate;			/* output sample rate */
    INT8 *		region_base;			/* pointer to the base of the region */
	int			total_banks;			/* number of total banks in the region */
    UINT32		clock;					/* original clock on the chip */
    UINT8		stereo;					/* stereo output? */
    UINT8		voices;					/* number of voices */
    UINT8		adpcm;					/* adpcm enabled? */
    UINT8		last_register;			/* remember last register written before rest */
    UINT8       mode;                   /* current mode */
    INT32		adpcm_current;			/* current ADPCM sample */
    INT32		adpcm_delta_n;			/* current ADPCM scale factor */

    struct BSMT2000Voice voice[MAX_VOICES];	/* the voices */

#ifdef PINMAME
    UINT16      adpcm_77;
    int         use_de_rom_banking;     /* Flag to turn on Rom Banking support for Data East Games */
    //int         shift_data;             /* Shift integer to apply to samples for changing volume - this is most likely done external to the bsmt chip in the real hardware */
    UINT8       right_volume_set;       /* Monopoly, RCT do never set right volume although its supposed to be stereo */
#endif
};


/**********************************************************************************************

     GLOBALS

***********************************************************************************************/

static struct BSMT2000Chip bsmt2000[MAX_BSMT2000];
static INT64 *scratch;

/***************************************************************************************************

     reset_compression_flags -- reset decompression flags to halt processing of the compressed data

****************************************************************************************************/
static void reset_compression_flags(struct BSMT2000Chip *chip)
{
    struct BSMT2000Voice *voice = &chip->voice[ADPCM_VOICE];
    voice->reg[REG_RATE] = 0;
    voice->reg[REG_BANK] = 0xFE;
}

/**************************************************
    set_mode - set the mode after reset
***************************************************/
static void set_mode(struct BSMT2000Chip *chip, int i)
{
    /* force an update */
    stream_update(chip->stream,0);

    /* the mode comes from the address of the last register accessed */
    switch (chip->last_register)
    {
    default: // 119 happens sometimes
        break;

		//BATMAN,ST25TH,Hook trigger sequence: mode 1 set, 0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,..,0x6D, then mode 0 set but definetly use reg mapping of mode 1
#ifndef PINMAME
        /* mode 0: 24kHz, 12 channel PCM, 1 channel ADPCM, mono */
    case 0:
        chip->sample_rate = chip->clock / 1000;
        chip->stereo = 0;
        chip->voices = 12;
        chip->adpcm = 1;
        chip->mode = 0;
        break;
#endif

        /* mode 1: 24kHz, 11 channel PCM, 1 channel ADPCM, stereo */
    case 1:
        chip->sample_rate = chip->clock / 1000;
        chip->stereo = 1;
        chip->voices = 11;
        chip->adpcm = 1;
        chip->mode = 1;
        break;

    // mode 2+4 test left channel output
    // mode 3 tests right channel output (similar to mode 2)

        /* mode 5: 24kHz, 12 channel PCM, stereo */
    case 5:
        chip->sample_rate = chip->clock / 1000;
        chip->stereo = 1;
        chip->voices = 12;
        chip->adpcm = 0;
        chip->mode = 5;
        break;

        /* mode 6: 34kHz, 8 channel PCM, stereo */
    case 6:
        chip->sample_rate = chip->clock / 706;
        chip->stereo = 1;
        chip->voices = 8;
        chip->adpcm = 0;
        chip->mode = 6;
        break;

        /* mode 7: 32kHz, 9 channel PCM, stereo */
    case 7:
        chip->sample_rate = chip->clock / 750;
        chip->stereo = 1;
        chip->voices = 9;
        chip->adpcm = 0;
        chip->mode = 7;
        break;
    }

    /* update the sample rate */
    stream_set_sample_rate(chip->stream, chip->sample_rate);
    stream_set_sample_rate(chip->stream+1, chip->sample_rate);

	LOG(("BSMT#%d last reg. prior to reset: %d\n", i, chip->last_register));
}

/**********************************************************************************************

     bsmt2000_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void bsmt2000_update(int num, INT16 **buffer, int length)
{
	struct BSMT2000Chip *chip = &bsmt2000[num];
	INT64 *left, *right;
	INT16 *ldest = buffer[0];
	INT16 *rdest = buffer[1];
    struct BSMT2000Voice *voice;
    int samp, voicenum;

    length = (length > MAX_SAMPLE_CHUNK) ? MAX_SAMPLE_CHUNK : length;
	/* determine left/right source data */
	left = scratch;
	right = scratch + length;
				
	/* clear out the accumulator */
	memset(left, 0, length * sizeof(left[0]));
	memset(right, 0, length * sizeof(right[0]));

	/* compressed voice */
	voice = &chip->voice[ADPCM_VOICE];
	if (chip->adpcm && voice->reg[REG_BANK] < chip->total_banks && voice->reg[REG_RATE])
	{
		INT8 *base = &chip->region_base[voice->reg[REG_BANK] * 0x10000];
		INT32 rvol = voice->reg[REG_RIGHTVOL];
		INT32 lvol = chip->stereo ? voice->reg[REG_LEFTVOL] : rvol;
		UINT32 pos = voice->reg[REG_CURRPOS];
		UINT32 frac = voice->fraction;
#ifdef PINMAME
		if (chip->stereo && !chip->right_volume_set) // Monopoly and RCT feature stereo hardware, but only ever set the left volume
			rvol = lvol;
#endif
		/* loop while we still have samples to generate & decompressed samples to play */
		for (samp = 0; samp < length; samp++)
		{
			/* apply volumes and add */
			left[samp]  = chip->adpcm_current * (lvol * 2); // ADPCM voice gets added twice to ACC (2x APAC)
			right[samp] = chip->adpcm_current * (rvol * 2);

			/* update position */
			frac++;
			if (frac == 6)
			{
				pos++;
				frac = 0;
			}

			/* every 3 samples, we update the ADPCM state */
			if (frac == 1 || frac == 4)
			{
				static const INT32 delta_tab[16] = { 154, 154, 128, 102, 77, 58, 58, 58, 58, 58, 58, 58, 77, 102, 128, 154 };
				INT32 delta, value;

				if(pos >= voice->reg[REG_LOOPEND])
					break;

				// extract corresponding nibble and convert to full integer
				value = base[pos];
				if (frac == 1)
					value >>= 4;
				value &= 0xF;
				if (value & 0x8)
					value |= 0xFFFFFFF0;

				/* compute the delta for this sample */
				delta = chip->adpcm_delta_n * value;
				if (value > 0)
					delta += chip->adpcm_delta_n >> 1;
				else
					delta -= chip->adpcm_delta_n >> 1;

				/* add and clamp against the sample */
				chip->adpcm_current += delta;
				if (chip->adpcm_current > 32767) //!! ??
					chip->adpcm_current = 32767;
				else if (chip->adpcm_current < -32768)
					chip->adpcm_current = -32768;

				/* adjust the delta multiplier */
				chip->adpcm_delta_n = (chip->adpcm_delta_n * delta_tab[value+8]) >> 6;
				if (chip->adpcm_delta_n > 2000) //!! ??
					chip->adpcm_delta_n = 2000;
				else if (chip->adpcm_delta_n < 1)
					chip->adpcm_delta_n = 1;
			}
		}

#if ENABLE_ADPCM_INTERPOLATION
		pos = voice->reg[REG_CURRPOS];
		frac = voice->fraction;
		// interpolate each package of three same samples that the ADPCM generated
		for (samp = 0; samp < length && pos < voice->reg[REG_LOOPEND]; samp++)
		{
			int samp_c;
			switch (frac)
			{
			case 0:
			case 3:
				samp_c = MIN(samp + 1, length-1);
				left[samp]  = (left [samp] * (INT64)3334 + left [samp_c] * (INT64)6666) / (INT64)10000;
				right[samp] = (right[samp] * (INT64)3334 + right[samp_c] * (INT64)6666) / (INT64)10000;
				break;
			case 1:
			case 4:
				break;
			case 2:
			case 5:
				samp_c = MIN(samp + 2, length-1);
				left[samp]  = (left [samp] * (INT64)6666 + left [samp_c] * (INT64)3334) / (INT64)10000;
				right[samp] = (right[samp] * (INT64)6666 + right[samp_c] * (INT64)3334) / (INT64)10000;
				break;
			}

			/* update position */
			frac++;
			if (frac == 6)
			{
				pos++;
				frac = 0;
			}
		}
#endif
		/* update the position */
		voice->reg[REG_CURRPOS] = (UINT16)pos;
		voice->fraction = (UINT16)frac;

		/* "rate" is a control register; clear it to 0 when done */
		if (pos >= voice->reg[REG_LOOPEND])
			voice->reg[REG_RATE] = 0;
	}

	/* loop over normal voices (8bit, 8kHz mono samples) */
	for (voicenum = 0; voicenum < chip->voices; voicenum++)
	{
		voice = &chip->voice[voicenum];

		/* compute the region base */
		if (voice->reg[REG_BANK] < chip->total_banks)
		{
			INT8 *base = &chip->region_base[voice->reg[REG_BANK] * 0x10000];
			UINT32 rate = voice->reg[REG_RATE];
			INT32 rvol = voice->reg[REG_RIGHTVOL];
			INT32 lvol = chip->stereo ? voice->reg[REG_LEFTVOL] : rvol;
			UINT32 pos = voice->reg[REG_CURRPOS];
			UINT32 frac = voice->fraction;
#ifdef PINMAME
			if (chip->stereo && !chip->right_volume_set) // Monopoly and RCT feature stereo hardware, but only ever set the left volume
				rvol = lvol;
#endif
			if (chip->adpcm_77 > 0 && rvol == 0 && lvol == 0) //!! is this really correct?
			{
				rvol = lvol = chip->adpcm_77;
			}

			/* loop while we still have samples to generate */
            for (samp = 0; samp < length; samp++)
            {
                INT32 val1 = base[pos];
                // sample is shifted by 8, as accumulator expects everything in 16bit*16bit (sampledata*volume), and then cuts that down to 16bit in the end
#if ENABLE_INTERPOLATION
                INT32 val2 = base[MIN(pos + 1, (UINT32)voice->reg[REG_LOOPEND]-1)];
                INT32 sample = (val1 * (INT32)(0x800 - frac) + (val2 * (INT32)frac)) >> 3; // (... >> 11) << 8
#else
                INT32 sample = val1 << 8;
#endif
				/* apply volumes and add */
				left[samp]  += sample * lvol;
				right[samp] += sample * rvol;

                /* update position */
                frac += rate;
                pos += frac >> 11;
                frac &= 0x7ff;

				/* check for loop end */
                if (pos >= voice->reg[REG_LOOPEND])
                {
                    pos += voice->reg[REG_LOOPSTART] - voice->reg[REG_LOOPEND]; // looks whacky, but it seems to be correct like this
                    frac = 0;
                }
			}

			/* update the position */
            voice->reg[REG_CURRPOS] = (UINT16)pos;
            voice->fraction = (UINT16)frac;
        }
	}

    /* reduce the overall gain */
    /* which is a simple SACH opcode on the TMS, so this copies the upper 16bit to the output, thus a shift by 16 */

#if 0//def PINMAME // different shifts to 'simulate' external amplifiers and other hardware
    INT32 bsmt_shift_data = 16 - chip->shift_data;
#else
    #define bsmt_shift_data 16
#endif

    for (samp = 0; samp < length; samp++)
    {
        INT64 l = (left[samp] >> bsmt_shift_data);
        INT64 r = (right[samp] >> bsmt_shift_data);
        if (l > 32767) // this overflow check happens after each voices accumulation on the TMS (as it only has a 32bit ACC with saturation enabled), but this way we are more accurately mixing (not with respect to the actual HW though)
            l = 32767;
        else if (l < -32768)
            l = -32768;
        if (r > 32767)
            r = 32767;
        else if (r < -32768)
            r = -32768;
        ldest[samp] = (INT16)l;
        rdest[samp] = (INT16)r;
    }
}


/**********************************************************************************************

     BSMT2000_sh_start -- start emulation of the BSMT2000

***********************************************************************************************/

INLINE void init_voice(struct BSMT2000Voice *voice)
{
    memset(voice, 0, sizeof(*voice));
 	voice->reg[REG_LEFTVOL] = 0x7fff;
 	voice->reg[REG_RIGHTVOL] = 0x7fff;
}


INLINE void init_all_voices(struct BSMT2000Chip *chip)
 {
 	int i;
 	/* init the voices */
    for (i = 0; i < MAX_VOICES; i++)
 		init_voice(&chip->voice[i]);
}

int BSMT2000_sh_start(const struct MachineSound *msound)
{
	const struct BSMT2000interface *intf = msound->sound_interface;
	char stream_name[2][40];
	const char *stream_name_ptrs[2];
	int vol[2];
	int i;

	/* initialize the chips */
	memset(&bsmt2000, 0, sizeof(bsmt2000));
	for (i = 0; i < intf->num; i++)
	{
		bsmt2000[i].voices = intf->voices[i];

		/* generate the name and create the stream */
		sprintf(stream_name[0], "%s #%d Ch1", sound_name(msound), i);
		sprintf(stream_name[1], "%s #%d Ch2", sound_name(msound), i);
		stream_name_ptrs[0] = stream_name[0];
		stream_name_ptrs[1] = stream_name[1];

		/* set the volumes */
#ifdef PINMAME
		//Handle Reverse Stereo flag from interface here
		vol[intf->reverse_stereo] = MIXER(intf->mixing_level[i], MIXER_PAN_LEFT);
		vol[1 - intf->reverse_stereo] = MIXER(intf->mixing_level[i], MIXER_PAN_RIGHT);
		//Capture other interface flags we need later
		bsmt2000[i].use_de_rom_banking = intf->use_de_rom_banking;
		//bsmt2000[i].shift_data = intf->shift_data;
        bsmt2000[i].right_volume_set = 0;
		bsmt2000[i].adpcm_77 = 0;
#else
		vol[0] = MIXER(intf->mixing_level[i], MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->mixing_level[i], MIXER_PAN_RIGHT);
#endif /* PINMAME */

        bsmt2000[i].sample_rate = intf->baseclock[i] / 1000;
        bsmt2000[i].clock = intf->baseclock[i];

        // guess initial mode from the parameters, should be not necessary, but f.e. Alvin G. reset is not wired yet, thus also no mode set!
        bsmt2000[i].last_register = (bsmt2000[i].voices == 11) ? 1 : 5;
        bsmt2000[i].mode = bsmt2000[i].last_register;

		/* create the stream */
		bsmt2000[i].stream = stream_init_multi(2, stream_name_ptrs, vol, bsmt2000[i].sample_rate, i, bsmt2000_update);
		if (bsmt2000[i].stream == -1)
			return 1;

		/* initialize the regions */
		bsmt2000[i].region_base = (INT8 *)memory_region(intf->region[i]);
		bsmt2000[i].total_banks = (int)(memory_region_length(intf->region[i]) / 0x10000);

		/* init the voices */
		init_all_voices(&bsmt2000[i]);
		reset_compression_flags(&bsmt2000[i]);
        set_mode(&bsmt2000[i], i);
	}

	/* allocate memory */
	scratch = malloc(sizeof(scratch[0]) * 2 * MAX_SAMPLE_CHUNK);
	if (!scratch)
		return 1;

	/* success */
	return 0;
}


/**********************************************************************************************

     BSMT2000_sh_stop -- stop emulation of the BSMT2000

***********************************************************************************************/

void BSMT2000_sh_stop(void)
{
	/* free memory */
	if (scratch)
		free(scratch);
	scratch = NULL;
}

/**********************************************************************************************

     BSMT2000_sh_reset -- reset emulation of the BSMT2000

***********************************************************************************************/

void BSMT2000_sh_reset(void)
{
	int i;
	for (i = 0; i < MAX_BSMT2000; i++)
	{
		init_all_voices(&bsmt2000[i]);
		reset_compression_flags(&bsmt2000[i]);
		set_mode(&bsmt2000[i],i);
	}
}


/**********************************************************************************************

     bsmt2000_reg_write -- handle a write to the selected BSMT2000 register

***********************************************************************************************/

static void bsmt2000_reg_write(struct BSMT2000Chip *chip, offs_t offset, data16_t data, data16_t mem_mask)
{
    struct BSMT2000Voice *voice;

    /*store last register written*/
    chip->last_register = offset;

    if (offset >= 0x80)
        return;
        
    /* force an update */
    stream_update(chip->stream, 0);

    if (offset < 0x6d) /* regular/non-adpcm voice */
    {
        int voice_index;
        int regindex = 6;
        while (offset < regmap[chip->mode][regindex])
            --regindex;

        voice_index = offset - regmap[chip->mode][regindex];
        if (voice_index >= chip->voices)
            return;

        voice = &chip->voice[voice_index];

#if LOG_COMMANDS
	    logerror("BSMT#%d write: V%d R%d = %04X\n", chip - bsmt2000, voice_index, regindex, data);
#endif
        /* update the register */
        COMBINE_DATA(&voice->reg[regindex]);

#ifdef PINMAME
        if (chip->use_de_rom_banking && regindex == REG_BANK)
	    {
		    //DE GAMES HAVE FUNKY ROM LOADING - SO WE MESS WITH ROM BANK DATA TO MAKE IT WORK OUT
		    UINT16 temp = (voice->reg[REG_BANK] & 0x07) |
			             ((voice->reg[REG_BANK] & 0x18)<<1) |
				         ((voice->reg[REG_BANK] & 0x20)>>2);
		    voice->reg[REG_BANK] = temp;
	    }

        if (regindex == REG_RIGHTVOL)
            chip->right_volume_set = 1;

		if (regindex == REG_CURRPOS)
			voice->fraction = 0;
#endif
    }
    else if(chip->adpcm) /* update parameters for compressed voice */
    {
#if LOG_COMPRESSED_ONLY
        logerror("BSMT#%d write: %02x = %04X\n", chip - bsmt2000, offset, data);
#endif

        voice = &chip->voice[ADPCM_VOICE];
 		switch (offset)
 		{
			//LOOP STOP POSITION
 			case 0x6d:
 				COMBINE_DATA(&voice->reg[REG_LOOPEND]);
                LOG(("REG_LOOPEND=%04X\n", voice->reg[REG_LOOPEND]));
 				break;

 			//ROM BANK
 			case 0x6f:
 				COMBINE_DATA(&voice->reg[REG_BANK]);
#ifdef PINMAME
				if(chip->use_de_rom_banking) {
					UINT16 temp = (voice->reg[REG_BANK] & 0x07) |
							     ((voice->reg[REG_BANK] & 0x18)<<1) |
								 ((voice->reg[REG_BANK] & 0x20)>>2);
					voice->reg[REG_BANK] = temp;
				}
#endif
    			break;

			//RATE - USED AS A CONTROL TO TELL CHIP READY TO OUTPUT COMPRESSED DATA (VALUE = 1 FOR VALID DATA)
			case 0x73:
				COMBINE_DATA(&voice->reg[REG_RATE]);
#ifdef PINMAME
				if(voice->reg[REG_RATE] != 0)
#endif
                {
					/* reset adpcm values also */
					chip->adpcm_current = 0;
					chip->adpcm_delta_n = 10;
				}
				break;

			//RIGHT CHANNEL VOLUME
#ifdef PINMAME
            case 0x6e: // main right channel volume control, used when ADPCM is alreay playing
#endif
			case 0x74: // right channel volume control, copied on start to 0x6e
#ifdef PINMAME
				chip->right_volume_set = 1;
#endif
 				COMBINE_DATA(&voice->reg[REG_RIGHTVOL]);
				LOG(("REG_RIGHTVOL=%04X\n", voice->reg[REG_RIGHTVOL]));
 				break;

			//SAMPLE START POSITION
 			case 0x75:
 				COMBINE_DATA(&voice->reg[REG_CURRPOS]);
                LOG(("REG_CURRPOS=%04X voice->loop_stop_position=%08X\n", voice->reg[REG_CURRPOS], voice->reg[REG_LOOPEND]));
				voice->fraction = 0;
				break;

 			case 0x77:
				//for example MONOPOLY & RCT (and no other ADPCM commands it seems), STAR WARS, APOLLO13 trigger a lot of: 0x77 (data = 0)?
				//for example ID4 uses 0x77 also with increasing/decreasing data input (1280 up to 32000), so maybe sample rate/'pitch' for ADPCM? Or really volume?
				//Tommy even uses 0x77 without using any other ADPCM commands
				COMBINE_DATA(&chip->adpcm_77);
				LOG(("REG_ADPCM77=%04X\n", chip->adpcm_77));
				break;

			//LEFT CHANNEL VOLUME
#ifdef PINMAME
			case 0x70: // main left channel volume control, used when ADPCM is alreay playing

			case 0x78: // left channel volume control, copied on start to 0x70
#endif
                if (chip->stereo)
                {
                    COMBINE_DATA(&voice->reg[REG_LEFTVOL]);
                    LOG(("REG_LEFTVOL=%04X\n", voice->reg[REG_LEFTVOL]));
                }
 				break;
        }
	}
}


/**********************************************************************************************

     BSMT2000_data_0_w -- handle a write to the current register

***********************************************************************************************/

WRITE16_HANDLER( BSMT2000_data_0_w )
{
	bsmt2000_reg_write(&bsmt2000[0], offset, data, mem_mask);
}
