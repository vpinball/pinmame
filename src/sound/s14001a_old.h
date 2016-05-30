#ifndef _S14001A_H_
#define _S14001A_H_
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct S14001A_interface
{
	int region;			/* memory region where the sample ROM lives */
};

int s14001a_sh_start(const struct MachineSound *msound);
void s14001a_sh_stop(void);

int S14001A_bsy_0_r(void);     		/* read BUSY pin */
void S14001A_reg_0_w(int data);		/* write to input latch */
void S14001A_rst_0_w(int data);		/* write to RESET pin */
void S14001A_set_rate(int newrate);     /* set VSU-1000 clock divider */
void S14001A_set_volume(int volume);    /* set VSU-1000 volume control */

#endif

