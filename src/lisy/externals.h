#ifndef EXTERNALS_H
#define EXTERNALS_H


// all the extenal global vars & routines
//from lisy80.c or lisy1.c or lisy.c

extern int lisy_hardware_revision;
extern t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
extern ls80dbg_t ls80dbg;
extern ls80opt_t ls80opt;
extern t_stru_lisy80_games_csv lisy80_game;
extern unsigned char swMatrix[9];
extern int lisy_hardware_revision;
extern int lisy_time_to_quit_flag;
extern int lisy80_game_nr;
extern int lisy80_volume;

extern t_stru_lisy1_games_csv lisy1_game;

void lisy1_init( void );
void lisy80_init( void );


#endif  /* EXTERNALS_H */

