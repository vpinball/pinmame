// license:BSD-3-Clause

#ifndef HC55516_H
#define HC55516_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_HC55516		4

// Set the known sample clock rate for an HC55516 chip.  'num' is the chip
// number - most games only use one HC55516 chip, in which case this should
// be 0.  If the game uses multiple chips, set the frequency for each chip
// individually.  This can be called from the init_GAME() routine for an 
// individual game driver to set that game's clock rate.
//
// This information isn't currently used for anything, but we're keeping
// the function (and the calls to it that were added for all WPC89 games)
// in case it turns out to be useful in the future.  During the 2019 revamp
// of the HC55516 code, we initially thought that it would be necessary (or
// at least would improve playback) to have pre-set clock data per game.
// Further work made that unnecessary; the emulator simply adapts in real
// time to the current playback rate.  If you want to determine the clock
// rate or rates used in a particular game, you can make a special build
// of hc55516.c with rate logging enabled - see hc55516.c for details.
//
void hc55516_set_sample_clock(int num, int frequency);


// start the hc55516 subsystem
int hc55516_sh_start(const struct MachineSound *msound);

// Low-pass output filter types
#define HC55516_FILTER_C8228   1        // Williams speech board Type 2 (C-8228), system 3 through system 9
#define HC55516_FILTER_SYS11   2        // Williams System 11 
#define HC55516_FILTER_WPC89   3        // Pre-DCS WPC sound boards

struct hc55516_interface
{
	int num;
	int volume[MAX_HC55516];
	int output_filter_type;
};

#ifdef PINMAME
/* 
 *  Set the gain.  The default gain maps the full dynamic range of the HC55516
 *  output to the full dynamic range of the 16-bit PCM MAME stream.  This can
 *  be overridden per game as needed to adjust the gain according to the 
 *  dynamic range actually used in the game's audio clips.  See the loudness
 *  metering comments in hc55516.c for more details and tips on selecting the
 *  gain for a given game.
 */
void hc55516_set_gain(int num, double gain);
#endif

/* sets the databit (0 or 1) */
void hc55516_digit_w(int num, int data);

/* sets the clock state (0 or 1, clocked on the rising edge) */
void hc55516_clock_w(int num, int state);

/* clears or sets the clock state */
void hc55516_clock_clear_w(int num, int data);
void hc55516_clock_set_w(int num, int data);

/* clears the clock state and sets the databit */
void hc55516_digit_clock_clear_w(int num, int data);

WRITE_HANDLER( hc55516_0_digit_w );
WRITE_HANDLER( hc55516_0_clock_w );
WRITE_HANDLER( hc55516_0_clock_clear_w );
WRITE_HANDLER( hc55516_0_clock_set_w );
WRITE_HANDLER( hc55516_0_digit_clock_clear_w );

WRITE_HANDLER( hc55516_1_digit_w );
WRITE_HANDLER( hc55516_1_clock_w );
WRITE_HANDLER( hc55516_1_clock_clear_w );
WRITE_HANDLER( hc55516_1_clock_set_w );
WRITE_HANDLER( hc55516_1_digit_clock_clear_w );


#endif
