/*
  utils.c
  part of lisy80
  bontango 01.2019
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
#include "fadecandy.h"
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
//caled by mame.c routine 'run_game'
void lisy_init( void )
{
 unsigned char system;

  //no LISY HW detected?
  if (  lisy_hardware_revision == 0 ) return;
  
//do system specific init
  if ( lisy_hardware_revision == 100 ) system=1;
  else if ( lisy_hardware_revision == 350 ) system=35;
  else if ( lisy_hardware_revision == LISY_HW_LISY_W ) system=121; //RTH where is System used ?? fadecandy only?
     else system=80;

 //init the fadecandy HW ( if told present via K3 )
 if ( lisy_K3_value <= 1 )
 {
  if ( lisy_fadecandy_init(system) )
   {
    lisy_has_fadecandy = 0;
    if (ls80dbg.bitv.basic == 1 ) fprintf(stderr,"Info: No fadecandy HW or no LED configured\n");
   }
  else
   {
    lisy_has_fadecandy = 1;
    if (ls80dbg.bitv.basic == 1 ) fprintf(stderr,"Info: We have a fadecandy with config\n");
   }
}//K3 value


  //do system specific init
  if ( lisy_hardware_revision == 100 ) lisy1_init( );
  else if ( lisy_hardware_revision == 350 ) lisy35_init( );
  else if ( lisy_hardware_revision == 121 ) lisy_w_init();
     else lisy80_init( );
}

//debug output with timestamp
void lisy80_debug(char *message)
{
  struct timeval now;
  static struct timeval last;
  long seconds, useconds;
  static int first = 1;
  char str_seconds[20];
  char my_str_seconds[20];

if (first)
 {
  first = 0;
  //store start time in local static  var
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

 // store time we had last time
 //gettimeofday(&last,(struct timezone *)0);
 last = now;

 //we want only the last three digits of seconds
 sprintf(str_seconds,"%ld",now.tv_sec);
 sprintf(my_str_seconds,"%s",&str_seconds[strlen(str_seconds)]-3);
 
 //we print seconds plus MICROseconds, first elapsed time, second time elapsed since last debug output
 fprintf(stderr,"[%s.%06ld][%ld.%06ld] %s\n\r", my_str_seconds,now.tv_usec,seconds,useconds,message);


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
//command is  0->start or ask timerstatus
//            1->reset timer
//index 0..19, we do handle 19 timers here
// 0..9 for internal ( e.g. freeplay )
// 10..19 for coils lisy80
//return is 0 when init and timer is 'in range'
//return is 1 when time is over
int lisy_timer( unsigned int duration, int command, int index)
{
  unsigned int now;
  int millis_to_go;
  static unsigned int timerstatus[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  static unsigned int timer_last_on[20];
  static unsigned int timer_duration[20];

  //range check
  if ( index > 19 ) return (1);

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

/**
	
 * Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C":
	
 */
	
void strreverse(char* begin, char* end) {
	
	char aux;
	
	while(end>begin)
	
		aux=*end, *end--=*begin, *begin++=aux;
	
}
	
void my_itoa(int value, char* str, int base) {
	
	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	
	char* wstr=str;
	
	int sign;
	


	
	// Validate base
	
	if (base<2 || base>35){ *wstr='\0'; return; }
	

	
	// Take care of sign
	
	if ((sign=value) < 0) value = -value;
	


	
	// Conversion. Number is reversed.
	
	do *wstr++ = num[value%base]; while(value/=base);
	
	if(sign<0) *wstr++='-';
	
	*wstr='\0';
	

	
	// Reverse string

	
	strreverse(str,wstr-1);
	
}

//LISY udp server
//used to receive bytes which are interpreted as switch settings
//start in debug mode only
int lisy_udp_switch_reader( unsigned char *action, unsigned char do_only_init  )
{

  static int s; 
  int rc, n, flags;
  socklen_t len;
  struct sockaddr_in cliAddr, servAddr;
  unsigned char data;
  const int y = 1;


 if (!do_only_init) //this is with do_only_init==0, and needs to be polled by normal switchreader
  {
   // try to receive messages
    len = sizeof (cliAddr);
    n = recvfrom ( s, &data, 1, 0, (struct sockaddr *) &cliAddr, &len );
    if (n < 0) {
       if ( errno == EWOULDBLOCK ) return (80);  //return 80 indicates no data
       printf ("could not receive data ... %d\n", n );
       return (-1);
      }

   //only switchnumbers here, we interprete numbers <100 as OFF
   // and >100 as put to ON
   if ( data < 100) *action=0;  
   else { data -= 100; *action=1;}

   return data;
  }
 else //we create the socket and set it on nonblocking; port is lISY standard port( birthday of bontanto) 5963
  {
  //create socket
  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
     fprintf (stderr,"cannot create socket for udp switch server ...(%s)\n",strerror(errno));
     return (-1);
  }
  //bind the local port
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servAddr.sin_port = htons (5963);
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
  //set to nonblocking
  flags = fcntl(s,F_GETFL,0);
  fcntl(s, F_SETFL, flags | O_NONBLOCK);

  rc = bind ( s, (struct sockaddr *) &servAddr,
              sizeof (servAddr));
  if (rc < 0) {
     fprintf (stderr,"cannot bind port for udp switch server ...(%s)\n",strerror(errno));
     return (-1);
  }
 }//init

 return 0;
}
