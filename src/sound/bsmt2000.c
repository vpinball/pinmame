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
 *             -       2)Remove DE Rom loading flag & fix in the drivers themselves
 *             -       3)Command 0x77 could be the sample rate/'pitch' for ADPCM? Or volume like it is now implemented (e.g. sound played when BSMT DMD animation comes on in stereo games: https://www.youtube.com/watch?v=2FtzLzbapZs)
 *             -       4)Alvin G.(Pistol Poker, Worldtour) does not reset/setup-the-mode correctly yet, thus both vgmwrite and the start in here feature hacks for that to setup/init Mode 5.
 *             - DONE: 5)Monopoly and RCT do never set the right volume (as these are mono only), thus a special hack is necessary to make up for that (right_volume_set)
 *
 *   Low level emulation (LLE, BSMT2000_LLE below), after MAME's bsmt2000.cpp by Aaron Giles:
 *   the chip's real program (the TMS320C15 mask ROM, bsmt2000.bin, 4K words, CRC c2a265af)
 *   runs on a TMS320C1x core (src/cpu/tms320c1x) in lock-step with the sound stream. All the
 *   voice/mode/ADPCM logic and the TODO items above then come from the chip itself, and the
 *   writer sees the real write-pending/ready handshake (BSMT2000_status_0_r).
 *   It is more expensive than the HLE, so it is opt-in: the bsmt2000_lle option (pmoptions).
 *   The firmware is looked up like a MAME device ROM: bsmt2000.bin in bsmt2000.zip (or in a
 *   bsmt2000/ folder) in the ROM path; it is also accepted inside the game's own zip.
 *   Without the option, or without the firmware, the high level emulation below is used, unchanged.
 **********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "driver.h"
#include "../ext/vgm/vgmwrite.h"

#ifndef BSMT2000_LLE
#define BSMT2000_LLE 1 // 1: the bsmt2000_lle option can run the real chip program (when bsmt2000.bin is found), 0: HLE only
#endif

#if BSMT2000_LLE
#include "../cpu/tms320c1x/tms320c1x.h"
#include <stdarg.h>
#include <zlib.h>
#ifdef LIBPINMAME
extern void libpinmame_log_info(const char* format, ...);
#endif
#endif

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
// Note that for example Tales from the Crypt partially relies on having interpolation off, otherwise some sounds sound weird
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
    double		sample_rate;			/* output sample rate */
    INT8 *		region_base;			/* pointer to the base of the region */
    int			total_banks;			/* number of total banks in the region */
    double		clock;					/* original clock on the chip */
    bool		stereo;					/* stereo output? */
    UINT8		voices;					/* number of voices */
    bool		adpcm;					/* adpcm enabled? */
    UINT8		last_register;			/* remember last register written before rest */
    UINT8		mode;					/* current mode */
    INT32		adpcm_current;			/* current ADPCM sample */
    INT32		adpcm_delta_n;			/* current ADPCM scale factor */

    struct BSMT2000Voice voice[MAX_VOICES];	/* the voices */

#ifdef PINMAME
    UINT16      adpcm_77;
    bool        use_de_rom_banking;     /* Flag to turn on Rom Banking support for Data East Games */
    //int         shift_data;             /* Shift integer to apply to samples for changing volume - this is most likely done external to the bsmt chip in the real hardware */
    bool        right_volume_set;       /* Monopoly, RCT do never set right volume although its supposed to be stereo */
#endif
};

static uint16_t m_vgm_idx[MAX_BSMT2000];
static uint16_t m_reg_vgm[MAX_BSMT2000];
static INT8 * m_vgm_de_rom[MAX_BSMT2000]; // De-scrambled sample-ROM-copy handed to vgm_write_large_data(). vgmwrite only stores a pointer and writes the contents later


/**********************************************************************************************

     GLOBALS

***********************************************************************************************/

static struct BSMT2000Chip bsmt2000[MAX_BSMT2000];
static INT64 *scratch;

/***************************************************************************************************

     reset_compression_flags -- reset decompression flags to halt processing of the compressed data

****************************************************************************************************/
static void reset_compression_flags(struct BSMT2000Chip * const chip)
{
    struct BSMT2000Voice *voice = &chip->voice[ADPCM_VOICE];
    voice->reg[REG_RATE] = 0;
    voice->reg[REG_BANK] = 0xFE;
}

#ifdef PINMAME
// Data East games load the sample ROMs in a permuted bank order, so the bank register is remapped on the fly during playback (see use_de_rom_banking)
INLINE UINT16 de_rom_bank_remap(UINT16 bank)
{
    return (bank & 0x07) | ((bank & 0x18) << 1) | ((bank & 0x20) >> 2);
}
#endif

#if BSMT2000_LLE
/**********************************************************************************************

     LOW LEVEL EMULATION: the chip's own program on a TMS320C15
     (after MAME's src/devices/sound/bsmt2000.cpp by Aaron Giles)

     DSP I/O map:  port 0  read: register select    write: sample ROM address (byte within the bank)
                   port 1  read: data word (and clears write pending)    write: sample ROM bank
                   port 2  read: sample ROM byte, in the high byte
                   port 3  write: left DAC          port 7  write: right DAC
                   BIO pin: a data word is pending

***********************************************************************************************/

#define BSMT2000_FW_WORDS    0x1000
#define BSMT2000_FW_CRC      0xc2a265afu

struct BSMT2000LLE
{
    struct tms320c1x_state dsp;
    UINT16      register_select;        /* latched register number, read by the DSP on port 0 */
    UINT16      write_data;             /* latched data word, read by the DSP on port 1 */
    UINT16      rom_address;
    UINT16      rom_bank;
    INT16       left, right;            /* DAC outputs */
    bool        write_pending;          /* drives BIO; cleared when the DSP reads the data word */
    double      cycles_per_sample;      /* DSP cycles per output sample, on average */
    bool        mono;                   /* mode 0: the program only writes the left DAC */
    bool        tick;                   /* the program wrote the left DAC, i.e. completed a sample */
    bool        right_seen;             /* the right DAC has carried sound (sticky, see bsmt2000_lle_update) */
    /* output cadence measurement, to check the stream rate against what the program really does */
    UINT64      cycles_run;
    UINT64      cycles_at_first_dac;
    UINT32      dac_writes;
    UINT32      missed_writes;
    void        (*ready_cb)(void);
};

static bool lle_enabled;                /* the firmware was found and LLE is used */
static UINT16 lle_firmware[BSMT2000_FW_WORDS]; /* host-endian words */
static struct BSMT2000LLE lle[MAX_BSMT2000];

/* user visible messages (the choice between LLE and HLE): libpinmame's log, else the error log */
static void lle_info(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef LIBPINMAME
    libpinmame_log_info("%s", buf);
#else
    logerror("%s\n", buf);
#endif
}

/* bsmt2000.bin is MAME's device ROM: 0x2000 bytes, big-endian words */
static bool lle_try_load(const char *set)
{
    UINT8 raw[BSMT2000_FW_WORDS * 2];
    UINT32 n, crc;
    int i;
    mame_file *f = mame_fopen(set, "bsmt2000.bin", FILETYPE_ROM, 0);
    if (!f)
        return false;
    n = mame_fread(f, raw, sizeof(raw));
    mame_fclose(f);
    crc = (n == sizeof(raw)) ? (UINT32)crc32(0, raw, sizeof(raw)) : 0;
    if (crc != BSMT2000_FW_CRC)
    {
        lle_info("BSMT2000: bsmt2000.bin in '%s' has the wrong size or CRC (%u bytes, CRC %08x, expected c2a265af), ignored", set, n, crc);
        return false;
    }
    for (i = 0; i < BSMT2000_FW_WORDS; i++)
        lle_firmware[i] = (UINT16)((raw[2 * i] << 8) | raw[2 * i + 1]);
    return true;
}

static bool lle_load_firmware(void)
{
    const struct GameDriver *drv;
    if (!pmoptions.bsmt2000_lle)
        return false;
    if (Machine->sample_rate == 0) // no sound: the DSP would never run (it runs with the stream), and the writer would wait for it forever
        return false;
    if (lle_try_load("bsmt2000"))
        return true;
    for (drv = Machine->gamedrv; drv; drv = drv->clone_of)
        if (drv->name && *drv->name && lle_try_load(drv->name))
            return true;
    lle_info("BSMT2000: bsmt2000_lle is set, but bsmt2000.bin (CRC c2a265af) was not found in bsmt2000.zip or the game's zip: using the HLE");
    return false;
}

static UINT16 lle_io_r(void *param, int port)
{
    struct BSMT2000LLE * const l = (struct BSMT2000LLE *)param;
    switch (port)
    {
        case 0:
            return l->register_select;
        case 1:
            l->write_pending = false; // reading the data implicitly clears the write pending flag
            if (l->ready_cb)
                l->ready_cb();
            return l->write_data;
        case 2:
        {
            /* the program expects the sample byte in the high byte */
            const struct BSMT2000Chip * const chip = &bsmt2000[l - lle];
            UINT16 bank = l->rom_bank;
#ifdef PINMAME
            if (chip->use_de_rom_banking)
                bank = de_rom_bank_remap(bank);
#endif
            if (bank >= chip->total_banks)
                return 0;
            return (UINT16)((UINT8)chip->region_base[((UINT32)bank << 16) + l->rom_address] << 8);
        }
    }
    return 0;
}

static void lle_io_w(void *param, int port, UINT16 data)
{
    struct BSMT2000LLE * const l = (struct BSMT2000LLE *)param;
    switch (port)
    {
        case 0: l->rom_address = data; break;
        case 1: l->rom_bank = data; break;
        case 3:
            /* the program writes the right DAC, then the left one, once per sample period: the left write completes a sample */
            l->left = (INT16)data;
            if (l->dac_writes++ == 0)
                l->cycles_at_first_dac = l->cycles_run;
            l->tick = true;
            tms320c1x_abort(&l->dsp);
            break;
        case 7:
            l->right = (INT16)data;
            if (l->right > 64 || l->right < -64)
                l->right_seen = true;
            break;
    }
}

static int lle_bio_r(void *param)
{
    return ((struct BSMT2000LLE *)param)->write_pending ? 1 : 0;
}

/* The sample rate the program outputs at in a mode: the length of its sample loop, in DSP cycles,
   measured on the program itself while idle (with the cycle counts of the core; the loop gets a
   fraction of a cycle longer on average while it takes register writes). The HLE uses the same
   rates (set_mode). Modes 2-4 are test modes. 0: unknown mode, keep the current rate. */
static double lle_mode_cycles(int mode)
{
    switch (mode)
    {
        case 0: return 249.5;
        case 1: return 250.5;
        case 5: return 254.;
        case 6: return 193.;
        case 7: return 208.;
    }
    return 0.;
}

static void lle_set_mode(int i, int mode, bool update_stream)
{
    struct BSMT2000Chip * const chip = &bsmt2000[i];
    const double cycles = lle_mode_cycles(mode);
    if (cycles <= 0.)
        return;
    lle[i].cycles_per_sample = cycles;
    lle[i].mono = (mode == 0);
    chip->sample_rate = chip->clock / 4. / cycles; // the TMS320C1x runs one cycle per 4 input clocks
    if (update_stream)
    {
        stream_set_sample_rate(chip->stream, chip->sample_rate);
        stream_set_sample_rate(chip->stream + 1, chip->sample_rate);
    }
}

/* the program picks its mode at reset from the register select latch (it reads it first thing) */
static void lle_reset(int i)
{
    struct BSMT2000LLE * const l = &lle[i];
    stream_update(bsmt2000[i].stream, 0);
    lle_set_mode(i, l->register_select, true);
    tms320c1x_reset(&l->dsp);
    logerror("BSMT2000: reset, register select %u, %.1f Hz\n", l->register_select, bsmt2000[i].sample_rate);
}

static void bsmt2000_lle_update(int num, INT16 **buffer, int length)
{
    struct BSMT2000LLE * const l = &lle[num];
    INT16 * const ldest = buffer[0];
    INT16 * const rdest = buffer[1];
    int samp;

    /* One output sample per sample of the program: run it until it completes the next one (its loop
       length varies by a few cycles from sample to sample, so sampling its DACs at fixed times would now
       and then repeat or skip one). The stream runs at the program's average rate (lle_set_mode), and
       the time the program runs is capped, in case it does not output anything (yet). */
    const int cap = (int)(2. * l->cycles_per_sample);
    for (samp = 0; samp < length; samp++)
    {
        int spent = 0;
        l->tick = false;
        while (!l->tick && spent < cap)
            spent += tms320c1x_execute(&l->dsp, cap - spent);
        l->cycles_run += spent;
        ldest[samp] = l->left;
        /* Mode 0 is mono (only the left DAC is written). Some stereo-mode games never set a right volume
           (Monopoly, RollerCoaster Tycoon: the HLE's right_volume_set): the chip then really outputs
           silence on the right, and those mono machines use the left output only. So, as the HLE does,
           the left output goes to both channels until the right one has carried any sound. */
        rdest[samp] = (l->mono || !l->right_seen) ? l->left : l->right;
    }
}

static void lle_start(int i, const struct BSMT2000interface *intf)
{
    struct BSMT2000LLE * const l = &lle[i];
    memset(l, 0, sizeof(*l));
    tms320c1x_init(&l->dsp, lle_firmware, 12, l, lle_io_r, lle_io_w, lle_bio_r);
    // power-on mode: the HLE's guess from the voice count; the Data East/Sega/Stern boards set it themselves
    // with a reset, Alvin G. does not wire the reset (yet), so the program starts in this mode there
    l->register_select = bsmt2000[i].last_register;
    lle_set_mode(i, l->register_select, true);
    tms320c1x_reset(&l->dsp);
}

static void lle_stop(int i)
{
    const struct BSMT2000LLE * const l = &lle[i];
    const UINT64 span = l->cycles_run - l->cycles_at_first_dac;
    logerror("BSMT2000: LLE stopped: %u DAC writes in %llu cycles (%.3f cycles per write, stream: %.3f), %u missed write(s)\n",
        l->dac_writes, (unsigned long long)l->cycles_run,
        (l->dac_writes > 1) ? (double)span / (double)(l->dac_writes - 1) : 0., l->cycles_per_sample, l->missed_writes);
}
#endif /* BSMT2000_LLE */

int BSMT2000_lle_active(void)
{
#if BSMT2000_LLE
    return lle_enabled;
#else
    return 0;
#endif
}

void BSMT2000_set_ready_callback(int num, void (*cb)(void))
{
#if BSMT2000_LLE
    if (num >= 0 && num < MAX_BSMT2000)
        lle[num].ready_cb = cb;
#endif
}

/* 1: the chip can take the next register write; the HLE is always ready */
int BSMT2000_status_0_r(void)
{
#if BSMT2000_LLE
    if (lle_enabled)
    {
        stream_update(bsmt2000[0].stream, 0);
        return lle[0].write_pending ? 0 : 1;
    }
#endif
    return 1;
}

/**************************************************
    set_mode - set the mode after reset
***************************************************/
static void set_mode(struct BSMT2000Chip * const chip, int i)
{
    /* force an update */
    stream_update(chip->stream,0);

    /* the mode comes from the address of the last register accessed */
    switch (chip->last_register)
    {
    default: // 119 happens sometimes
        break;

    //!! BATMAN,ST25TH,Hook trigger sequence: mode 1 set, 0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,..,0x6D, then mode 0 set but definetly use reg mapping of mode 1
#ifndef PINMAME
        /* mode 0: 24kHz, 12 channel PCM, 1 channel ADPCM, mono */
    case 0:
        chip->sample_rate = chip->clock / 4. / 249.5;
        chip->stereo = 0;
        chip->voices = 12;
        chip->adpcm = 1;
        chip->mode = 0;
        break;
#endif

        /* mode 1: 24kHz, 11 channel PCM, 1 channel ADPCM, stereo */
    case 1:
        chip->sample_rate = chip->clock / 4. / 250.5;
        chip->stereo = 1;
        chip->voices = 11;
        chip->adpcm = 1;
        chip->mode = 1;
        break;

    // mode 2+4 test left channel output
    // mode 3 tests right channel output (similar to mode 2)

        /* mode 5: 24kHz, 12 channel PCM, stereo */
    case 5:
        chip->sample_rate = chip->clock / 4. / 254.;
        chip->stereo = 1;
        chip->voices = 12;
        chip->adpcm = 0;
        chip->mode = 5;
        break;

        /* mode 6: 34kHz, 8 channel PCM, stereo */
    case 6:
        chip->sample_rate = chip->clock / 4. / 193.;
        chip->stereo = 1;
        chip->voices = 8;
        chip->adpcm = 0;
        chip->mode = 6;
        break;

        /* mode 7: 32kHz, 9 channel PCM, stereo */
    case 7:
        chip->sample_rate = chip->clock / 4. / 208.;
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

static void bsmt2000_update(int num, INT16 **buffer, int _length)
{
	const int length = (_length > MAX_SAMPLE_CHUNK) ? MAX_SAMPLE_CHUNK : _length;
	struct BSMT2000Chip * const chip = &bsmt2000[num];
	/* determine left/right source data */
	INT64 * const left  = scratch;
	INT64 * const right = scratch + length;
	INT16 * const ldest = buffer[0];
	INT16 * const rdest = buffer[1];
	struct BSMT2000Voice *voice;
	int samp, voicenum;

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


INLINE void init_all_voices(struct BSMT2000Chip * const chip)
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
#if BSMT2000_LLE
	lle_enabled = lle_load_firmware();
	if (lle_enabled)
		lle_info("BSMT2000: low level emulation (the chip's own program from bsmt2000.bin)");
#endif
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

		bsmt2000[i].sample_rate = intf->baseclock[i] / 1000.;
		bsmt2000[i].clock = intf->baseclock[i];

		// guess initial mode from the parameters, should be not necessary, but e.g. Alvin G. reset is not wired/emulated yet, thus also no mode set!
		bsmt2000[i].last_register = (bsmt2000[i].voices == 11) ? 1 : 5;
		bsmt2000[i].mode = bsmt2000[i].last_register;

		/* create the stream */
		bsmt2000[i].stream = stream_init_multi(2, stream_name_ptrs, vol, bsmt2000[i].sample_rate, i,
#if BSMT2000_LLE
			lle_enabled ? bsmt2000_lle_update :
#endif
			bsmt2000_update);
		if (bsmt2000[i].stream == -1)
			return 1;

		m_reg_vgm[i] = 0; //!!?
		m_vgm_idx[i] = vgm_open(VGMC_BSMT2000, intf->baseclock[i]);
#ifdef PINMAME
		if (intf->use_de_rom_banking)
		{
			// DE games store the sample ROM in a permuted bank order and remap the bank register on the fly;
			// VGM playback expects raw bank writes to index the ROM directly, so dump a de-scrambled copy to match the
			// (unmodified) bank values recorded via vgm_write below. This matches what libvgm playback expects.
			const UINT32 region_len = memory_region_length(intf->region[i]);
			const int banks = (int)(region_len / 0x10000);
			// vgm_write_large_data() only stores a pointer and writes the data later
			INT8 * const __restrict descrambled = (INT8 *)malloc(region_len);
			m_vgm_de_rom[i] = descrambled;
			if (descrambled)
			{
				const INT8 * const __restrict src = (const INT8 *)memory_region(intf->region[i]);
				int b;
				memset(descrambled, 0, region_len);
				for (b = 0; b < banks; b++)
				{
					const UINT16 srcbank = de_rom_bank_remap((UINT16)b);
					if (srcbank < banks)
						memcpy(descrambled + (size_t)b * 0x10000, src + (size_t)srcbank * 0x10000, 0x10000);
				}
				vgm_write_large_data(m_vgm_idx[i], 0x01, region_len, 0x00, 0x00, descrambled);
			}
#if LOG_COMMANDS
			else // out of memory
			{
				logerror("Unable to descramble ROM for VGM export");
			}
#endif
		}
		else
#endif
			vgm_dump_sample_rom(m_vgm_idx[i], 0x01, intf->region[i]);

		/* initialize the regions */
		bsmt2000[i].region_base = (INT8 *)memory_region(intf->region[i]);
		bsmt2000[i].total_banks = (int)(memory_region_length(intf->region[i]) / 0x10000);

		/* init the voices */
		init_all_voices(&bsmt2000[i]);
		reset_compression_flags(&bsmt2000[i]);
		set_mode(&bsmt2000[i], i);
#if BSMT2000_LLE
		if (lle_enabled)
			lle_start(i, intf);
#endif

		// Record the initial mode into the VGM stream so the later-on used player employs the right
		// register mapping/sample rate from the start. DE games (re-)issue this via BSMT2000_sh_reset(),
		// but Alvin G. games whose reset is not wired/emulated yet (setting mode 5), would default to the
		// wrong default mode and wrongly decode every register write (=garbled exported audio).
		// Also note that this needs a patch in vgmwrite to make time 0 vgm writes work! (search for PinMAME)
		vgm_write(m_vgm_idx[i], 0x01, 0x00, bsmt2000[i].last_register & 0x7f);
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
	int i;

	/* free memory */
	if (scratch)
		free(scratch);
	scratch = NULL;

#if BSMT2000_LLE
	if (lle_enabled)
		for (i = 0; i < MAX_BSMT2000; i++)
			if (bsmt2000[i].clock > 0.)
				lle_stop(i);
	lle_enabled = false;
#endif

	// free the de-scrambled VGM sample ROM copy
	for (i = 0; i < MAX_BSMT2000; i++)
		if (m_vgm_de_rom[i])
		{
			free(m_vgm_de_rom[i]);
			m_vgm_de_rom[i] = NULL;
		}
}

/**********************************************************************************************

     BSMT2000_sh_reset -- reset emulation of the BSMT2000

***********************************************************************************************/

void BSMT2000_sh_reset(void)
{
	int i;
	for (i = 0; i < MAX_BSMT2000; i++)
	{
		vgm_write(m_vgm_idx[i], 0x01, 0x00, m_reg_vgm[i] & 0x7f);
#if BSMT2000_LLE
		if (lle_enabled)
		{
			if (bsmt2000[i].clock > 0.)
				lle_reset(i);
			continue;
		}
#endif
		init_all_voices(&bsmt2000[i]);
		reset_compression_flags(&bsmt2000[i]);
		set_mode(&bsmt2000[i],i);
	}
}


/**********************************************************************************************

     bsmt2000_reg_write -- handle a write to the selected BSMT2000 register

***********************************************************************************************/

static void bsmt2000_reg_write(struct BSMT2000Chip * const chip, offs_t offset, data16_t data, data16_t mem_mask)
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
		    voice->reg[REG_BANK] = de_rom_bank_remap(voice->reg[REG_BANK]);
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
				if(chip->use_de_rom_banking)
					voice->reg[REG_BANK] = de_rom_bank_remap(voice->reg[REG_BANK]);
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
	m_reg_vgm[0] = offset;
	vgm_write(m_vgm_idx[0], 0x00, data, offset & 0x7f);

#if BSMT2000_LLE
	if (lle_enabled)
	{
		struct BSMT2000LLE * const l = &lle[0];
		/* let the program catch up to now, then latch the register number and the data word */
		stream_update(bsmt2000[0].stream, 0);
		l->register_select = (UINT16)offset;
		COMBINE_DATA(&l->write_data);
		if (l->write_pending)
			l->missed_writes++;
		l->write_pending = true;
		return;
	}
#endif
	bsmt2000_reg_write(&bsmt2000[0], offset, data, mem_mask);
}

#if BSMT2000_LLE
/* The TMS320C1x core is only used by the BSMT2000, so it is compiled as part of this file: no
   build script (makefiles, CMake, Visual Studio projects) has to list it. Keep this include last,
   as the core defines short register macros (OV, DP, ARP, ...). */
#include "../cpu/tms320c1x/tms320c1x.c"
#endif
