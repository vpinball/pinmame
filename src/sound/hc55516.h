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
// individually.
//
// This should be called from the init_GAME() function for each game (e.g.,
// init_taf() for The Addams Family).  If this isn't called, we'll infer the
// sample rate by observing the actual data stream coming from the host, but
// this can cause some ugly wow/flutter glitching during startup as we adjust
// the stream from the default initial rate to the actual rate.  It makes for
// much smoother startup to provide the known frequency.
//
// For anyone encountering a game that hasn't had its clock rate added yet,
// build hc55516.c with the LOG_SAMPLE_RATE symbol defined.  That'll make
// the code write a log file with its measurements on the rate.  You can use
// that to get a reading of the actual rate for the game, and then patch up
// the init_GAME() function for the game to put that rate into effect.
//
void hc55516_set_sample_clock(int num, int frequency);

// start the hc55516 subsystem
int hc55516_sh_start(const struct MachineSound *msound);

struct hc55516_interface
{
	int num;
	int volume[MAX_HC55516];
};

#ifdef PINMAME
/* sets the gain (10000 is normal) */
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
