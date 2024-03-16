#ifndef __SP0256_H__
#define __SP0256_H__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// license:BSD-3-Clause
// copyright-holders:Joseph Zbiciak,Tim Lindner
/**********************************************************************

    SP0256 Narrator Speech Processor emulation

**********************************************************************
                            _____   _____
                   Vss   1 |*    \_/     | 28  OSC 2
                _RESET   2 |             | 27  OSC 1
           ROM DISABLE   3 |             | 26  ROM CLOCK
                    C1   4 |             | 25  _SBY RESET
                    C2   5 |             | 24  DIGITAL OUT
                    C3   6 |             | 23  Vdi
                   Vdd   7 |    SP0256   | 22  TEST
                   SBY   8 |             | 21  SER IN
                  _LRQ   9 |             | 20  _ALD
                    A8  10 |             | 19  SE
                    A7  11 |             | 18  A1
               SER OUT  12 |             | 17  A2
                    A6  13 |             | 16  A3
                    A5  14 |_____________| 15  A4

**********************************************************************/

/*
   GI SP0256 Narrator Speech Processor

   By Joe Zbiciak. Ported to MAME by tim lindner.

*/

struct sp0256_interface {
	int volume;
	int clock;
	void (*lrq_callback)(int state);
	void (*sby_callback)(int state);
	int	memory_region;
};

int  sp0256_sh_start( const struct MachineSound *msound );
void sp0256_sh_stop( void );
void sp0256_reset( void );

void sp0256_bitrevbuff(UINT8 *buffer, unsigned int start, unsigned int length);

WRITE_HANDLER( sp0256_ALD_w );

READ16_HANDLER( spb640_r );
WRITE16_HANDLER( spb640_w );

#endif
