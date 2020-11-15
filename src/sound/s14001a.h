// license:BSD-3-Clause
// copyright-holders:Ed Bernard, Jonathan Gevaryahu, hap
// thanks-to:Kevin Horton
/*
    SSi TSI S14001A speech IC emulator
*/

#ifndef __S14001A_H__
#define __S14001A_H__

struct S14001A_interface
{
	int region;						/* memory region where the sample ROM lives */
};

int s14001a_sh_start(const struct MachineSound *msound);
void s14001a_sh_stop(void);

int S14001A_bsy_0_r(void);			/* read BUSY pin (pin 40) */
void S14001A_reg_0_w(int data);		/* write to input latch (6-bit word) */
void S14001A_rst_0_w(int data);		/* write to RESET/START pin (pin 10) */
void S14001A_set_rate(double newrate); /* set VSU-1000 clock divider */
//!! DECLARE_READ_LINE_MEMBER(romen_r); // ROM /EN (pin 9)

#endif /* __S14001A_H__ */
