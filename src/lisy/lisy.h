#ifndef LISY_H
#define LISY_H

#define LISY_SOFTWARE_VER     7

// here we have all the xternals for lisy1.c and lisy80.c
extern int lisy_hardware_revision;
extern ls80dbg_t ls80dbg;
extern ls80opt_t ls80opt;
extern int lisy_time_to_quit_flag;

int lisy_time_to_quit(void);
void lisy_shutdown(void);


#endif  /* LISY_H */

