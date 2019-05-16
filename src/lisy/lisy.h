#ifndef LISY_H
#define LISY_H

// here we have all the xternals for lisy1.c and lisy80.c
extern int lisy_hardware_revision;
extern int lisy_hardware_ID;
extern ls80dbg_t ls80dbg;
extern ls80opt_t ls80opt;
extern int lisy_time_to_quit_flag;
extern struct timeval lisy_start_t;

int lisy_time_to_quit(void);
void lisy_shutdown(void);
void lisy_hw_init(int lisy_variant);
int lisy_set_gamename( char *arg_from_main, char *lisy_gamename);
void lisy_nvram_write_to_file( void );

#endif  /* LISY_H */

