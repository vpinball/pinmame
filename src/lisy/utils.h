#ifndef _UTILS_H
#define _UTILS_H


//define the debugbuf once here
char debugbuf[256];

void lisy_init( void );

void lisy80_debug(char *message);

void lisy80_error(int error_num);
void syserr( char *msg, int number, int doexit);
void lisy80_set_sighandler(void);
void lisy80_sig_handler(int signo);

void LISY80_BufferInit(void);
unsigned char LISY80_BufferIn(unsigned char byte);
unsigned char LISY80_BufferOut(unsigned char *pByte);
unsigned char parity( unsigned char val );

int lisy_timer( unsigned int duration, int command, int index);

void my_itoa(int value, char* str, int base); 

int lisy_udp_switch_reader( unsigned char *action, unsigned char do_only_init  );

void lisy_logger(void);



#define LISY80_BUFFER_FAIL     0
#define LISY80_BUFFER_SUCCESS  1

//
//some bit settings routines
//postion(pos) goes from 0..7
//
#define CHECK_BIT(var, pos) ((var) & (1<<(pos)))
 
#define SET_BIT(var, pos) var |= (1 << pos)
 
#define CLEAR_BIT(var, pos) var &= (~(1 << pos))
 
#define TOGGLE_BIT(var, pos) var ^= (1 << pos)

#endif  /* UTILS_H */

