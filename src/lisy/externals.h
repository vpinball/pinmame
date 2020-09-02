#ifndef EXTERNALS_H
#define EXTERNALS_H


// all the extenal global vars & routines
//from lisy80.c or lisy1.c or lisy.c or lisy_w.c

extern int lisy_has24c04;
extern int lisy_hardware_revision;
extern int lisy_hardware_ID;
extern unsigned char lisy_K3_value;
extern t_stru_lisy80_sounds_csv lisy80_sound_stru[64];
extern t_stru_lisy35_sounds_csv lisy35_sound_stru[256];
extern int lisy80_coil_min_pulse_time[10];
extern int lisy80_coil_min_pulse_mod;
extern int lisy1_coil_min_pulse_time[8];
extern ls80dbg_t ls80dbg;
extern ls80opt_t ls80opt;
extern unsigned char swMatrix[9];
extern int lisy_time_to_quit_flag;
extern int lisy80_game_nr;
extern int lisy_volume;
extern int lisy_has_fadecandy;
extern t_lisy_lamp_to_led_map lisy_lamp_to_led_map[52];
extern t_lisy_home_lamp_map lisy_home_lamp_map[49];
extern t_lisy_home_coil_map lisy_home_coil_map[10];

extern int fd_api;  //file descriptor for lisy api ( eg APC )

extern t_stru_lisy80_games_csv lisy80_game;
extern t_stru_lisy1_games_csv lisy1_game;
extern t_stru_lisy35_games_csv lisy35_game;
extern t_stru_lisymini_games_csv lisymini_game;

void lisy1_init( void );
void lisy80_init( void );
void lisy35_init( void );
void lisy_w_init( void );


#endif  /* EXTERNALS_H */

