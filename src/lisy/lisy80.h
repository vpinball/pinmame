#ifndef LISY80_H
#define LISY80_H

#define LISY80_SOFTWARE_MAIN 	3
#define LISY80_SOFTWARE_SUB 	43

// Generic LISY80 functions.
int  lisy80_get_gamename(char *gamename);
void  lisy80_init(void );
void lisy80_shutdown(void);
void lisy80TickleWatchdog( void );
void lisy80_display_handler_bcd( int riot1a, int riot1b );
void lisy80_display_handler_alpha( int a_or_b, int riot_port );
int lisy80_switch_handler( int riot0b );
int lisy80_switch_handler_old( int *switch_value );
void lisy80_throttle(int riot0b);
void lisy80_coil_handler_a( int data);
void lisy80_coil_handler_b( int data);
int lisy80_get_mpudips( int switch_nr );
void lisy80_simulate_switch( int myswitch, int action );
int lisy80_special_function(int myswitch, int action);
//int lisy80_simulated_switch_reader( unsigned char *action );
int lisy80_nvram_handler(int read_or_write, UINT8 *GTS80_pRAM);


#define LISY80_LEFTADV_SWITCH	6
#define LISY80_TEST_SWITCH	7
#define LISY80_RIGHTADV_SWITCH	16
#define LISY80_LEFTCOIN_SWITCH	17
#define LISY80_RIGHTCOIN_SWITCH	27
#define LISY80_CENTERCOIN_SWITCH	37
#define LISY80_REPLAY_SWITCH	47
#define LISY80_TILT_SWITCH	57

#endif  /* LISY80_H */

