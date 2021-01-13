#ifndef LISY_W_H
#define LISY_W_H

void lisy_w_display_handler( void );
void lisy_w_throttle(void);
int  lisymini_get_gamename(char *gamename);
int  lisyapc_get_gamename(char *gamename);
void lisy_w_solenoid_handler( void );
void lisy_w_switch_handler( void );
unsigned char lisy_w_switch_reader( unsigned char *action );
void lisy_w_lamp_handler(void);
void lisy_w_sound_handler( unsigned char board, unsigned char data);

#define LISYW_TYPE_NONE 0
#define LISYW_TYPE_SYS9 1
#define LISYW_TYPE_SYS11A 2
#define LISYW_TYPE_SYS7 3
#define LISYW_TYPE_SYS11 4
#define LISYW_TYPE_SYS11RK 5

#endif  /* LISY_W_H */
