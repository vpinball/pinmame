#ifndef LISY1_H
#define LISY1_H

// Generic LISY1 functions.
void lisy1TickleWatchdog( void );
int  lisy1_get_gamename(char *gamename);
void lisy1_display_handler( int index, int value );
unsigned char lisy1_switch_handler( int sys1strobe );
void lisy1_solenoid_handler(int ioport,int enable);;
void lisy1_lamp_handler( int data, int isld);
void lisy1_throttle(void);
int lisy1_get_mpudips( int switch_nr );
unsigned char lisy1_nvram_handler(int read_or_write, unsigned char ramAddr, unsigned char accu);

#endif  /* LISY1_H */

