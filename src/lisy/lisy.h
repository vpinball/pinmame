#ifndef LISY_H
#define LISY_H

// here we have all the xternals for lisy1.c and lisy80.c
extern int lisy_hardware_revision;
extern int lisy_hardware_ID;
extern ls80dbg_t ls80dbg;
extern ls80opt_t ls80opt;
extern int lisy_time_to_quit_flag;
extern struct timeval lisy_start_t;
extern t_stru_lisy_env lisy_env;

int lisy_time_to_quit(void);
void lisy_shutdown(void);
void lisy_hw_init(int lisy_variant);
int lisy_set_gamename( char *arg_from_main, char *lisy_gamename);
void lisy_nvram_write_to_file( void );
void lisy_get_sw_version( unsigned char *sw_main, unsigned char *sw_sub, unsigned char *commit);
void lisy_sound_handler( unsigned char board, unsigned char data );


#define NVRAM_DELAY 200  //delay counter for nvram write ( * 15ms )

//the LISY HW revisions
#define LISY_HW_LISY1   100             //LISY1
#define LISY_HW_LISY80_311  311         //LISY80, old HW Version 3.11 (discontinue?)
#define LISY_HW_LISY35  350             //LISY35 Bally
#define LISY_HW_LISY80  320             //LISY80 HW320 & LISY_Home
#define LISY_HW_LISY_W  121             //LISYx_W (Williams) based on LISY_MINI1

//the LISY HW ID, one revision can have multiple hw IDS
#define LISY_HW_ID_NONE 0  //old lisy versions without HW ID
#define LISY_HW_ID_HOME 21  //lisy HOME

#endif  /* LISY_H */

