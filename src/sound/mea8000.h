/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Philips MEA 8000 emulation.

**********************************************************************/

#pragma once

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
