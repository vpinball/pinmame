/*
 LISY.c
 April 2017
 bontango
*/

#include "hw_lib.h"


//global var for handling different hardware revisions
//set in lisy80_hwlib_init
int lisy_hardware_revision;

//global var for debugging
//typedef is defined in hw_lib.h
ls80dbg_t ls80dbg;

//global var for additional options
//typedef is defined in hw_lib.h
ls80opt_t ls80opt;

//handle time_to quit from usrintf.c
int lisy_time_to_quit_flag = 0;

//do gracefull shutdown
extern void lisy1_shutdown();
extern void lisy80_shutdown();
void lisy_shutdown(void)
{
 if ( lisy_hardware_revision == 100 )
    lisy1_shutdown( );
 else
    lisy80_shutdown( );
}
