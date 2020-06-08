// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    Intel 8155/8156 - 2048-Bit Static MOS RAM with I/O Ports and Timer

**********************************************************************
                            _____   _____
                   PC3   1 |*    \_/     | 40  Vcc
                   PC4   2 |             | 39  PC2
              TIMER IN   3 |             | 38  PC1
                 RESET   4 |             | 37  PC0
                   PC5   5 |             | 36  PB7
            _TIMER OUT   6 |             | 35  PB6
                 IO/_M   7 |             | 34  PB5
             CE or _CE   8 |             | 33  PB4
                   _RD   9 |             | 32  PB3
                   _WR  10 |    8155     | 31  PB2
                   ALE  11 |    8156     | 30  PB1
                   AD0  12 |             | 29  PB0
                   AD1  13 |             | 28  PA7
                   AD2  14 |             | 27  PA6
                   AD3  15 |             | 26  PA5
                   AD4  16 |             | 25  PA4
                   AD5  17 |             | 24  PA3
                   AD6  18 |             | 23  PA2
                   AD7  19 |             | 22  PA1
                   Vss  20 |_____________| 21  PA0

**********************************************************************/

#ifndef __I8155__
#define __I8155__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_8155 4

typedef struct
{
	int num;							 /* number of chips to emulate */

	mem_read_handler				in_pa_func[MAX_8155];
	mem_read_handler				in_pb_func[MAX_8155];
	mem_read_handler				in_pc_func[MAX_8155];

	mem_write_handler			out_pa_func[MAX_8155];
	mem_write_handler			out_pb_func[MAX_8155];
	mem_write_handler			out_pc_func[MAX_8155];

	/* this gets called for each change of the TIMER OUT pin (pin 6) */
	mem_write_handler		out_to_func[MAX_8155];
} i8155_interface;

void i8155_init( i8155_interface *intfce);
void i8155_reset( int which );

int i8155_r ( int which, int offset );
void i8155_w( int which, int offset, UINT8 data );

READ_HANDLER( i8155_0_r );
READ_HANDLER( i8155_1_r );
READ_HANDLER( i8155_2_r );
READ_HANDLER( i8155_3_r );

WRITE_HANDLER( i8155_0_w );
WRITE_HANDLER( i8155_1_w );
WRITE_HANDLER( i8155_2_w );
WRITE_HANDLER( i8155_3_w );

#endif
