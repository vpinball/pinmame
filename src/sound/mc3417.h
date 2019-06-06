// license:BSD-3-Clause

#ifndef MC3417_H
#define MC3417_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_MC3417		2

struct mc3417_interface
{
	int num;
	int volume[MAX_MC3417];
};

int mc3417_sh_start(const struct MachineSound *msound);

/* sets the gain (10000 is normal) */
void mc3417_set_gain(int num, double gain);

/* sets the databit (0 or 1) */
void mc3417_digit_w(int num, int data);

/* sets the clock state (0 or 1, clocked on the rising edge) */
void mc3417_clock_w(int num, int state);

/* clears or sets the clock state */
void mc3417_clock_clear_w(int num, int data);
void mc3417_clock_set_w(int num, int data);

/* clears the clock state and sets the databit */
void mc3417_digit_clock_clear_w(int num, int data);

WRITE_HANDLER( mc3417_0_digit_w );
WRITE_HANDLER( mc3417_0_clock_w );
WRITE_HANDLER( mc3417_0_clock_clear_w );
WRITE_HANDLER( mc3417_0_clock_set_w );
WRITE_HANDLER( mc3417_0_digit_clock_clear_w );

WRITE_HANDLER( mc3417_1_digit_w );
WRITE_HANDLER( mc3417_1_clock_w );
WRITE_HANDLER( mc3417_1_clock_clear_w );
WRITE_HANDLER( mc3417_1_clock_set_w );
WRITE_HANDLER( mc3417_1_digit_clock_clear_w );


#endif
