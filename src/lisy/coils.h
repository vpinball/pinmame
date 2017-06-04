#ifndef _COILS_H_
#define _COILS_H_

void coil_set_str( char *str, int action);
//void coil_set( int coil, int action );
void coil_test ( void );
void lisy80_coil_init ( void );
void lisy1_coil_init ( void );

int coil_get_sw_version(void);
void coil_coil_set( int coil, int action);
void lisy80_coil_sound_set( int sound );
void lisy1_coil_sound_set( int sound );
void coil_cmd2pic_noansw(unsigned char command);
int coil_cmd2pic(unsigned char command);
int coil_get_k3(void);
void coil_led_set( int action);

/* pulse time for coils in u sec */
#define COIL_PULSE_TIME 80000

//the commands
#define LS80COILCMD_GET_SW_VERSION_MAIN 1 
#define LS80COILCMD_GET_SW_VERSION_SUB 2
#define LS80COILCMD_INIT 3
#define LS80COILCMD_SETSOUND 4
#define LS80COILCMD_LED_ON 5
#define LS80COILCMD_LED_OFF 6
#define LS80COILCMD_GET_K3 7

#endif  // _COILS_H_

