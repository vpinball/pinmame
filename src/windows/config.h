//============================================================
//
//	config.h - Win32 configuration routines
//
//============================================================

#ifndef _WIN_CONFIG__
#define _WIN_CONFIG__

#include "rc.h"

// Initializes and exits the configuration system
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

// Creates an RC object
struct rc_struct *cli_rc_create(void);

#endif // _WIN_CONFIG__
