/*
  utils.c
  part of lisy80
  bontango 04.2016
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<errno.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "utils.h"
#include "externals.h"

struct stru_lisy80_error
{
  int lisy80_err_number;  // the lisy80 error number
  int doexit;  //do we want to exit LISY80 or running anyway?
  int LED;  //do we want to put display PIC LED on?
  int display;  // should we try to put message to Gottlieb display?
  char message_short[8]; //short message for 80 & 80A
  char message_long[17]; //longer message for 80B; first 3 chars for error number & space
  char message_con[80]; //message for console
} ;

struct stru_lisy80_error lisy80_error_struct[12] = {
  { 0, 1, 0, 0, "      ", "                 ", "This is just a placeholder as zero is no error" },
  { 1, 1, 0, 0, "      ", "                 ", "Failed to initialize the wiringPi library" },
  { 2, 1, 1, 0, "      ", "                 ", "Failed to open the I2C bus for displays" },
  { 3, 1, 1, 0, "      ", "                 ", "Unable to get bus access to talk to display slave" },
  { 4, 1, 0, 1, "i2coil", "I2C COIL PIC PROB", "Failed to open the I2C bus for coils" },
  { 5, 1, 0, 1, "i2coil", "I2C COIL PIC PROB", "Unable to get bus access to talk to display slave"},
  { 6, 0, 0, 0, "i2cbus", "I2C BUS PROB WRIT", "Failed to write to the I2C bus display pic" }, //RTH: no exit, need to be monitored for repeat
  { 7, 0, 0, 0, "i2cbus", "I2C BUS PROB WRIT", "Failed to write to the I2C bus coil pic" },  //RTH: no exit, need to be monitored for repeat
  { 8, 1, 0, 0, "i2cbus", "I2C BUS PROB READ", "Failed to read from the I2C bus display pic" },
  { 9, 1, 0, 1, "i2cbus", "I2C BUS PROB READ", "Failed to read from the I2C bus coil pic" },
  {10, 0, 0, 1, "romiss", "ROM MISSING      ", "Failed to read ROM data for selected game" }, //mame will do exit for us
  {11, 1, 0, 1, "inv hw", "INVALID HARDWARE ", "Could not determine Hardware revision" } //unable to detect eeprom at valid address
 };


//lisy80_error, based on index
//gives error message on console
//indication via LED 
//and via Display if possible
//error 1..9 means we cannot put errormessage to Gottlieb display
//error 10..19 means we have trouble with display PIC
//for errors 20 and higher we assume to be able to put error message on Gottlieb display
void lisy80_error(int error_num)
{

 //put Error LED on 
 lisy80_set_red_led(1);
 //put display LED to on if requested
 if ( lisy80_error_struct[error_num].LED) lisy80_set_dp_led(1);

 //try to give error message at Gottlieb display if requested
 if ( lisy80_error_struct[error_num].display)
	 display_show_error_message(lisy80_error_struct[error_num].lisy80_err_number,lisy80_error_struct[error_num].message_short, lisy80_error_struct[error_num].message_long );

 //now put error message on console and exit if needed
 syserr(lisy80_error_struct[error_num].message_con, error_num,lisy80_error_struct[error_num].doexit);


}


// syserr
// print error message and exit
// new version with strerr_r instead of sys_errlist
void syserr( char *msg, int number, int doexit)
{

char info[128];
strerror_r(errno, info, sizeof(info));

fprintf(stderr,"LISY80_SYS_ERROR:%d >%s (%d:%s)\n\r",number,msg,errno,info);

if (doexit) exit(number);
/* else continue
else
  {
   fprintf(stderr,"press ENTER to continue\n\r");
   fgets(info, 10, stdin);
   return;
  }
*/
}

//init SW portion of lisy80 or LISY1
void lisy_init( void )
{
  if ( lisy_hardware_revision == 100 ) lisy1_init( );
     else lisy80_init( );
}


//debug output with timestamp
void lisy80_debug(char *message)
{
  struct timeval now;
  static struct timeval last;
  long seconds, useconds;
  static int first = 1;

if (first)
 {
  first = 0;
  //store start time in global var
  gettimeofday(&last,(struct timezone *)0);
 }

 //see what time is now
 gettimeofday(&now,(struct timezone *)0);

 seconds = now.tv_sec - last.tv_sec;
  useconds = now.tv_usec - last.tv_usec;
  if(useconds < 0) {
        useconds += 1000000;
        seconds--;
  }   

 fprintf(stderr,"[%ld.%03ld] %s\n\r",seconds,useconds,message);

 // store time we had last time
 gettimeofday(&last,(struct timezone *)0);

}

//set the new sig handler
void lisy80_set_sighandler(void)
{

if (signal(SIGUSR1, lisy80_sig_handler) == SIG_ERR)
        fprintf(stderr,"LISY80_SIG_HANDLER: can't catch SIGUSR1\n");
else
        fprintf(stderr,"LISY80_SIG_HANDLER: SIGUSR1 catched\n");

}

//signal handler
void lisy80_sig_handler(int signo)
{
    if (signo == SIGUSR1)
     {
	lisy_time_to_quit_flag = 1;
        fprintf(stderr,"LISY80_SIG_HANDLER: received SIGUSR1\n");
        fprintf(stderr,"LISY80_SIG_HANDLER: initiated shutdown of xpinmame\n");
     }

}


// FIFO Handling
// used for switches 

/* FIRST BUFFER  */
#define LISY80_BUFFER_SIZE 128

struct Buffer {
  unsigned char data[LISY80_BUFFER_SIZE];
  unsigned char read; // zeigt auf das Feld mit dem äesten Inhalt
  unsigned char write; // zeigt immer auf leeres Feld
} LISY80_buffer;


//
// initialisiert den Buffer
//
void LISY80_BufferInit(void)
{
  LISY80_buffer.read = LISY80_buffer.write = 0;
}

//
// Stellt 1 Byte in den Ringbuffer
//
// Returns:
//     LISY80_BUFFER_FAIL       der Ringbuffer ist voll. Es kann kein weiteres Byte gespeichert werden
//     LISY80_BUFFER_SUCCESS    das Byte wurde gespeichert
//
unsigned char LISY80_BufferIn(unsigned char byte)
{

  if ( ( LISY80_buffer.write + 1 == LISY80_buffer.read ) ||
       ( LISY80_buffer.read == 0 && LISY80_buffer.write + 1 == LISY80_BUFFER_SIZE ) )
    return LISY80_BUFFER_FAIL; // voll

  LISY80_buffer.data[LISY80_buffer.write] = byte;

  LISY80_buffer.write++;
  if (LISY80_buffer.write >= LISY80_BUFFER_SIZE)
    LISY80_buffer.write = 0;

  return LISY80_BUFFER_SUCCESS;
}
//
// Holt 1 Byte aus dem Ringbuffer, sofern mindestens eines abholbereit ist
//
// Returns:
//     LISY80_BUFFER_FAIL       der Ringbuffer ist leer. Es kann kein Byte geliefert werden.
//     LISY80_BUFFER_SUCCESS    1 Byte wurde geliefert
//
unsigned char LISY80_BufferOut(unsigned char *pByte)
{
  if (LISY80_buffer.read == LISY80_buffer.write)
    return LISY80_BUFFER_FAIL;

  *pByte = LISY80_buffer.data[LISY80_buffer.read];

  LISY80_buffer.read++;
  if (LISY80_buffer.read >= LISY80_BUFFER_SIZE)
    LISY80_buffer.read = 0;

  return LISY80_BUFFER_SUCCESS;
}

//calculates parity for 8 bit
unsigned char parity( unsigned char val )
{
  val ^= val >> 4;
  val ^= val >> 2;
  val ^= val >> 1;
  val &= 0x01;
  return val;
}

//handle internal timer
//duration in millisecs
//command is  0->start or ask timerstatus 1->reset timer
//index 0..9, we do handle 10 timers here
//return is 0 when init and timer is 'in range'
//return is 1 when time is over
int lisy_timer( unsigned int duration, int command, int index)
{
  unsigned int now;
  int millis_to_go;
  static unsigned int timerstatus[10] = { 0,0,0,0,0,0,0,0,0,0 };
  static unsigned int timer_last_on[10];
  static unsigned int timer_duration[10];

  if (command == 0)
   {
    if ( timerstatus[index] == 0) //need to start timer
     {
	timerstatus[index] = 1;
	timer_duration[index] = duration;
	timer_last_on[index] = millis();
	return 0;
     }
    else //give back status of timer
     {
        //see what time is now
        now = millis();
        //beware of 'wrap' which happens each 49 days
        if ( now < timer_last_on[index] ) now = timer_last_on[index]; //we had a wrap
        //calculate if timmer is over and give back status
        millis_to_go = timer_duration[index] - (now - timer_last_on[index]);
	if (millis_to_go > 0) return 0; else return 1;
     }
   }
  else //command !=0 reset timer
   {
    timerstatus[index] = 0;
    return 0;
   }
}
