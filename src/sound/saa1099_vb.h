#pragma once

/**********************************************
	Philips SAA1099 Sound driver - Valley Bell/libvgm's core

	Internal interface, used by saa1099.c only. The public API (the
	SAA1099_interface struct and the port write handlers) stays in saa1099.h
	and is the same whichever core is compiled in
**********************************************/

void saa1099vb_init(int chip, double clock, double sample_rate);
void saa1099vb_write_addr(int chip, UINT8 data);
void saa1099vb_write_data(int chip, UINT8 data);
void saa1099vb_update(int chip, INT16 **buffer, int length);
