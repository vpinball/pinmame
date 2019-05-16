#ifndef LISY35_H
#define LISY35_H

// Generic LISY35 functions.
void lisy35_shutdown(void);
int  lisy35_get_gamename(char *gamename);
void lisy35_display_handler( int index, int value );
unsigned char lisy35_switch_handler( int sys35col );
void lisy35_solenoid_handler(unsigned char data);
void lisy35_sound_handler(unsigned char type, unsigned char data);
void lisy35_lamp_handler( int blanking, int board, int lampadr, int lampdata);
void lisy35_throttle(void);
int lisy35_get_mpudips( int switch_nr );
int lisy35_nvram_handler(int read_or_write, unsigned char *by35_CMOS);
int lisy35_get_SW_Selftest(void);
int lisy35_get_SW_S33(void);

void lisy35_set_variant(void);
void lisy35_set_soundboard_variant( void );

//the IDs for the sound handler (called from by35.c)
#define LISY35_SOUND_HANDLER_IS_DATA 0
#define LISY35_SOUND_HANDLER_IS_CTRL 1

#endif  /* LISY35_H */

