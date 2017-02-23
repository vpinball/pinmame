/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Philips MEA 8000 emulation.

**********************************************************************/

#ifndef __MEA8000__
#define __MEA8000__

#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct MEA8000interface
{
	int mixing_level;
	mem_write_handler req_out_func;
};

int MEA8000_sh_start(const struct MachineSound *msound);
void MEA8000_sh_stop(void);

/* reset by external signal */
void mea8000_reset ( void );
void mea8000_config ( int channel, mem_write_handler req_out_func );

/* interface to CPU via address/data bus*/
READ_HANDLER  ( mea8000_r );
WRITE_HANDLER ( mea8000_w );

#endif /* __MEA8000__ */
