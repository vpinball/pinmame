#ifndef __SP0250_H__
#define __SP0250_H__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct sp0250_interface {
	int volume;
	void (*drq_callback)(int state);
};

int  sp0250_sh_start( const struct MachineSound *msound );
void sp0250_sh_stop( void );

WRITE_HANDLER( sp0250_w );

#endif
