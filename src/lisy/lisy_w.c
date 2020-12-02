/*
 LISY_W.c
 April 2019
 bontango
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "xmame.h"
#include "driver.h"
#include "wpc/core.h"
#include "wpc/s11.h"
#include "wpc/s7.h"
#include "lisy_w.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "switches.h"
#include "utils.h"
#include "eeprom.h"
#include "sound.h"
#include "lisy.h"
#include "usbserial.h"
#include "lisy_api_com.h"

//typedefs

/*


********   Position in segment array in general ****************
from core.c
#define DISP_SEG_7(row,col,type) {4*row,16*col,row*20+col*8+1,7,type}
*
core.h:#define DISP_SEG_BALLS(no1,no2,type)  {2,8,no1,1,type},{2,10,no2,1,type}
*
core.h:#define DISP_SEG_CREDIT(no1,no2,type) {2,2,no1,1,type},{2,4,no2,1,type}
*
*******************************************************************


********   SYSTEM 11  *************
const struct core_dispLayout s11_dispS11[] = {
  DISP_SEG_7(0,0,CORE_SEG16),DISP_SEG_7(0,1,CORE_SEG16),
  DISP_SEG_7(1,0,CORE_SEG8), DISP_SEG_7(1,1,CORE_SEG8),
  {2,8,0,1,CORE_SEG7S},{2,10,8,1,CORE_SEG7S}, {2,2,20,1,CORE_SEG7S},{2,4,28,1,CORE_SEG7S}, {0}
};
*
results in:
Player 1: 0
Player 2: 8
Player 3: 20
Player 4: 28
Credits: 20 28 Balls: 0 8
*/
 typedef union {
       core_tSeg segments;
       //assigment accoring to s11games.c & core.c/.h see above
       struct {
          UINT16 balls1; //0
          UINT16 player1[7]; //1..7
          UINT16 balls2;  //8
          UINT16 player2[7]; //9..15
          UINT16 dum1[4]; //16..19
          UINT16 credits1; //20
          UINT16 player3[7]; //21..27
          UINT16 credits2; //28
          UINT16 player4[7]; //29..35
          UINT16 dum2[CORE_SEGCOUNT-36]; //the rest
      } disp;
  } t_mysegments_s11;

/*
********   SYSTEM 7  *************
from core.h
struct core_dispLayout {
  UINT16 top, left, start, length, type;
*
from s7games.c
const core_tLCDLayout s7_dispS7[] = {
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_BALLS(0,8,CORE_SEG7S),DISP_SEG_CREDIT(20,28,CORE_SEG7S),{0}
};
results in:
Player 1: 21
Player 2: 29
Player 3: 1
Player 4: 9
Credits: 20 28 Balls: 0 8
*/
 typedef union {
       core_tSeg segments;
       //assigment accoring to s7games.c & core.c/.h see above
       struct {
	  UINT16 balls1; //0
          UINT16 player1[7]; //1..7
	  UINT16 balls2;  //8
          UINT16 player2[7]; //9..15
	  UINT16 dum1[4]; //16..19
	  UINT16 credits1; //20
          UINT16 player3[7]; //21..27
	  UINT16 credits2; //28
          UINT16 player4[7]; //29..35
	  UINT16 dum2[CORE_SEGCOUNT-36]; //the rest
      } disp;
  } t_mysegments_s7;

/*
********   SYSTEM 9  *************
const struct core_dispLayout s11_dispS9[] = {
  {4, 0, 1,7, CORE_SEG87}, {4,16, 9,7, CORE_SEG87},
  {0, 0,21,7, CORE_SEG87}, {0,16,29,7, CORE_SEG87},
  DISP_SEG_CREDIT(0,8,CORE_SEG7S),DISP_SEG_BALLS(20,28,CORE_SEG7S),{0}
results in:
Player 1: 1
Player 2: 9
Player 3: 21
Player 4: 29
Credits: 0 8 Balls: 20 28
*/
 typedef union {
       core_tSeg segments;
       //assigment accoring to s11games.c & core.c/.h see above
       struct {
	  UINT16 credits1; //0
          UINT16 player3[7]; //1..7
	  UINT16 credits2;  //8
          UINT16 player4[7]; //9..15
	  UINT16 dum1[4]; //16..19
	  UINT16 balls1; //20
          UINT16 player1[7]; //21..27
	  UINT16 balls2; //28
          UINT16 player2[7]; //29..35
	  UINT16 dum2[CORE_SEGCOUNT-36]; //the rest
      } disp;
  } t_mysegments_s9;


//global var for internal game_name structure,
//set by  lisy_set_gamename in unix/main
t_stru_lisymini_games_csv lisymini_game;

//global var for timing and speed
int g_lisymini_throttle_val = 300;

//internal switch Matrix for Williams system1, we need 9 elements
//as pinmame internal starts with 1
unsigned char swMatrixLISY_W[9] = { 0,0,0,0,0,0,0,0,0 };

//internal flag fo AC Relais, default 0 ->not present
unsigned char lisy_has_AC_Relais = 0;


//init SW portion of lisy_w
void lisy_w_init( void )
{
 int i,sb;
 char s_lisy_software_version[16];
 unsigned char sw_main,sw_sub,commit;


 //do the init on vars
 //for (i=0; i<=36; i++) lisy1_lamp[i]=0;

 //set signal handler
 lisy80_set_sighandler();

//set the internal type
if (strcmp(lisymini_game.type,"SYS7") == 0) lisymini_game.typeno = LISYW_TYPE_SYS7;
else if (strcmp(lisymini_game.type,"SYS9") == 0) lisymini_game.typeno = LISYW_TYPE_SYS9;
else if (strcmp(lisymini_game.type,"SYS11A") == 0) lisymini_game.typeno = LISYW_TYPE_SYS11A;
else lisymini_game.typeno = LISYW_TYPE_NONE;

//set internal flags based on system type
switch(lisymini_game.typeno)
{
	case LISYW_TYPE_SYS7: 
	case LISYW_TYPE_SYS9: 
		lisy_has_AC_Relais = 0;	
		break;
	case LISYW_TYPE_SYS11A: 
		lisy_has_AC_Relais = 1;	
		break;
	default : 
		lisy_has_AC_Relais = 0;	
}

 //show up on calling terminal
 lisy_get_sw_version( &sw_main, &sw_sub, &commit);
 sprintf(s_lisy_software_version,"%d%02d %02d",sw_main,sw_sub,commit);
 fprintf(stderr,"This is LISY (Lisy Mini) by bontango, Version %s\n",s_lisy_software_version);

  
 //set displays initial to ASCII with dot (6)  for boot message
 for(i=0; i<5; i++) lisy_api_display_set_prot( i, 6);
//convert gamename to uppercase for display
for(i=0; i<strlen(lisymini_game.long_name); i++) lisymini_game.long_name[i] = toupper(lisymini_game.long_name[i]);
 //show the 'boot' message
 lisy_api_show_boot_message(s_lisy_software_version,lisymini_game.type,lisymini_game.gamenr,lisymini_game.long_name);
  
 //set HW rules for solenoids, let do APC this (faster)
 //we do it per default for all 6 special solenoids
 //and ignore 'special switches' for pinmamem in switch_handler
 lisy_api_sol_set_hwrule( 17, 65 ); 
 lisy_api_sol_set_hwrule( 18, 66 );
 lisy_api_sol_set_hwrule( 19, 67 ); 
 lisy_api_sol_set_hwrule( 20, 68 ); 
 lisy_api_sol_set_hwrule( 21, 69 ); 
 lisy_api_sol_set_hwrule( 22, 70 ); 

 //show green ligth for now, lisy mini is running
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(1);

}

/*
 throttle routine as with sound disabled
 lisy_w runs faster than the org game :-0
*/
void lisy_w_throttle(void)
{
static int first = 1;
unsigned int now;
int sleeptime;
static unsigned int last;


if (first)
 {
  first = 0;
  //store start time first, which is number of microseconds since wiringPiSetup (wiringPI lib)
  last = micros();
 }

 // if we are faster than throttle value which 
 //is per default 3000 usec (3 msec)
 //we need to slow down a bit

 //do that in a while loop with updating soudn_stream in between

 //do update the soundstream if enabled (pinmame internal sound only)
 if (sound_stream && sound_enabled)
 {
  do{
    //do update the soundstream if enabled (pinmame internal sound only)
    sound_stream_update(sound_stream);

   //see how many micros passed
   now = micros();
   //beware of 'wrap' which happens each 71 minutes
   if ( now < last) now = last; //we had a wrap

   //calculate if we are above minimum sleep time
   sleeptime = g_lisymini_throttle_val  - ( now - last);
  } while ( sleeptime > 0);
 }
 else
 //if no sound enabled use sleep routine
  {
   //see how many micros passed
    now = micros();
    //beware of 'wrap' which happens each 71 minutes
    if ( now < last) now = last; //we had a wrap

    //calculate if we are above minimum sleep time
    sleeptime = g_lisymini_throttle_val - ( now - last);
    if ( sleeptime > 0)
          delayMicroseconds( sleeptime );
  }

 //store current time for next round with speed limitc
 last = micros();

}



/* from core.c
const int core_bcd2seg7[16] = {
// 0    1    2    3    4    5    6    7    8    9  
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
};
if 'comma' is active we add 0x80
*/

//internal routine
//gets a (7digit) segment value and give back char value
char my_seg14_2char( UINT16 segvalue )
{

 char retchar;

 switch(segvalue)
   {
        case 0x0877: retchar = 'A'; break;
        case 0x2a0f: retchar = 'B'; break;
        case 0x0039: retchar = 'C'; break;
        case 0x220f: retchar = 'D'; break;
        case 0x0079: retchar = 'E'; break;
        case 0x0071: retchar = 'F'; break;
        case 0x083d: retchar = 'G'; break;
        case 0x0876: retchar = 'H'; break;
        case 0x2209: retchar = 'I'; break;
        //case : retchar = 'J'; break;
        //case : retchar = 'K'; break;
        case 0x0038: retchar = 'L'; break;
        case 0x0536: retchar = 'M'; break;
        case 0x1136: retchar = 'N'; break;
        //case 0x003f: retchar = 'O'; break; same as '0'
        case 0x0873: retchar = 'P'; break;
        //case : retchar = 'Q'; break;
        case 0x1873: retchar = 'R'; break;
        //case 0x086d: retchar = 'S'; break; same as '5'
        case 0x2201: retchar = 'T'; break;
        case 0x003e: retchar = 'U'; break;
        case 0xc430: retchar = 'V'; break;
        case 0x5036: retchar = 'W'; break;
        case 0x5500: retchar = 'X'; break;
        case 0x2500: retchar = 'Y'; break;
        //case : retchar = 'Z'; break;
        case 0x003f: retchar = '0'; break;
        case 0x0006: retchar = '1'; break;
        case 0x085f: retchar = '2'; break;
        case 0x084f: retchar = '3'; break;
        case 0x0866: retchar = '4'; break;
        case 0x086d: retchar = '5'; break;
        case 0x087d: retchar = '6'; break;
        case 0x0007: retchar = '7'; break;
        case 0x087f: retchar = '8'; break;
        case 0x086f: retchar = '9'; break;

        case 0: retchar = ' '; break;
        default: retchar = '?';
   }

 return (retchar);;

}

//internal routine
//gets a (7digit) segment value and give back char value 
char my_seg2char( UINT16 segvalue ) 
{

 UINT8 hascomma = 0;
 char retchar;

 //handle comma
 if ( segvalue >= 0x80 ) 
 { 
  segvalue = segvalue - 0x80;
  hascomma = 1;
 }

 switch(segvalue)
   {
	case 0x3f: retchar = '0'; break;
	case 0x06: retchar = '1'; break;
	case 0x5b: retchar = '2'; break;
	case 0x4f: retchar = '3'; break;
	case 0x66: retchar = '4'; break;
	case 0x6d: retchar = '5'; break;
	case 0x7d: retchar = '6'; break;
	case 0x07: retchar = '7'; break;
	case 0x7f: retchar = '8'; break;
	case 0x6f: retchar = '9'; break;
	case 0: retchar = ' '; break;
	default: retchar = '?';
   }

 if(hascomma) retchar = retchar + 0x80;
 return (retchar);;
}

//send ASCII characters o display
//use my_seg2char routine to translate williams 7segment to ASCII
void send_ASCII_to_display( int no, UINT16 *dispval)
{
 int i;
 char str[8]; //include null termination

 if (no) { for(i=0; i<=6; i++) str[i] = my_seg2char( dispval[i]); str[7] = '\0'; }
 else { for(i=0; i<=3; i++) str[i] = my_seg2char( dispval[i]); str[5] = '\0'; } //status display

 lisy_api_send_str_to_disp(no, str);

}

//send SEG7 data to display
void send_SEG7_to_display( int no, UINT16 *dispval)
{

 if (no) { lisy_api_send_SEG7_to_disp(no, 7, dispval);  }
 else { lisy_api_send_SEG7_to_disp(no, 4, dispval); } //status display

}

//send SEG14 data to display
void send_SEG14_to_display( int no, UINT16 *dispval)
{

 if (no) { lisy_api_send_SEG14_to_disp(no, 7, dispval);  }
 else { lisy_api_send_SEG14_to_disp(no, 4, dispval); } //status display

}


//display handler System7
void lisy_w_display_handler_SYS7(void)
{
  static UINT8 first = 1;
  UINT8 i,k;
  UINT16 status[4];
  UINT16 sum1,sum2;
  int len;


  static t_mysegments_s7 mysegments;
  t_mysegments_s7 tmp_segments;

  if(first)
  {
        memset(mysegments.segments,0,sizeof(mysegments.segments));
	first=0;
  }
 
  //something changed?
  if  ( memcmp(mysegments.segments,coreGlobals.segments,sizeof(mysegments)) != 0)
  {
    //store it
    memcpy(tmp_segments.segments,coreGlobals.segments,sizeof(mysegments));
    //check it display per display
    len = sizeof(mysegments.disp.player1);
    if( memcmp( tmp_segments.disp.player1,mysegments.disp.player1,len) != 0) send_ASCII_to_display(1, tmp_segments.disp.player1);
    if( memcmp( tmp_segments.disp.player2,mysegments.disp.player2,len) != 0) send_ASCII_to_display(2, tmp_segments.disp.player2);
    if( memcmp( tmp_segments.disp.player3,mysegments.disp.player3,len) != 0) send_ASCII_to_display(3, tmp_segments.disp.player3);
    if( memcmp( tmp_segments.disp.player4,mysegments.disp.player4,len) != 0) send_ASCII_to_display(4, tmp_segments.disp.player4);
    //status display
    sum1 = tmp_segments.disp.balls1 + tmp_segments.disp.balls2 + tmp_segments.disp.credits1 + tmp_segments.disp.credits2;
    sum2 = mysegments.disp.balls1 + mysegments.disp.balls2 + mysegments.disp.credits1 + mysegments.disp.credits2;
    if (sum1 != sum2 )
	{
	 status[0]=tmp_segments.disp.credits1;
	 status[1]=tmp_segments.disp.credits2;
	 status[2]=tmp_segments.disp.balls1;
	 status[3]=tmp_segments.disp.balls2;
	 send_ASCII_to_display(0, status);
	}
    //remember it
    memcpy(mysegments.segments,coreGlobals.segments,sizeof(mysegments));

    //and print out if debug display
    if ( ls80dbg.bitv.displays ) 
    {
    char c;
    lisy80_debug("display change detected");
    fprintf(stderr,"\nPlayer1: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player1[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }
    fprintf(stderr,"\nPlayer2: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player2[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }

    fprintf(stderr,"\nPlayer3: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player3[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }
    fprintf(stderr,"\nPlayer4: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player4[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }

    fprintf(stderr,"\nCredits: %c%c",my_seg2char(mysegments.disp.credits1),my_seg2char(mysegments.disp.credits2));
    fprintf(stderr,"\nBalls: %c%c",my_seg2char(mysegments.disp.balls1),my_seg2char(mysegments.disp.balls2));
    fprintf(stderr,"\n\n ");
    }

    //check which line has changed

  }
}

//display handler System9
void lisy_w_display_handler_SYS9(void)
{
  static UINT8 first = 1;
  UINT8 i,k;
  UINT16 status[4];
  UINT16 sum1,sum2;
  int len;


  static t_mysegments_s9 mysegments;
  t_mysegments_s9 tmp_segments;

  if(first)
  {
        memset(mysegments.segments,0,sizeof(mysegments.segments));
	first=0;
  }
 
  //something changed?
  if  ( memcmp(mysegments.segments,coreGlobals.segments,sizeof(mysegments)) != 0)
  {
    //store it
    memcpy(tmp_segments.segments,coreGlobals.segments,sizeof(mysegments));
    //check it display per display
    len = sizeof(mysegments.disp.player1);
    if( memcmp( tmp_segments.disp.player1,mysegments.disp.player1,len) != 0) send_ASCII_to_display(1, tmp_segments.disp.player1);
    if( memcmp( tmp_segments.disp.player2,mysegments.disp.player2,len) != 0) send_ASCII_to_display(2, tmp_segments.disp.player2);
    if( memcmp( tmp_segments.disp.player3,mysegments.disp.player3,len) != 0) send_ASCII_to_display(3, tmp_segments.disp.player3);
    if( memcmp( tmp_segments.disp.player4,mysegments.disp.player4,len) != 0) send_ASCII_to_display(4, tmp_segments.disp.player4);
    //status display
    sum1 = tmp_segments.disp.balls1 + tmp_segments.disp.balls2 + tmp_segments.disp.credits1 + tmp_segments.disp.credits2;
    sum2 = mysegments.disp.balls1 + mysegments.disp.balls2 + mysegments.disp.credits1 + mysegments.disp.credits2;
    if (sum1 != sum2 )
	{
	 status[0]=tmp_segments.disp.credits1;
	 status[1]=tmp_segments.disp.credits2;
	 status[2]=tmp_segments.disp.balls1;
	 status[3]=tmp_segments.disp.balls2;
	 send_ASCII_to_display(0, status);
	}
    //remember it
    memcpy(mysegments.segments,coreGlobals.segments,sizeof(mysegments));

    //and print out if debug display
    if ( ls80dbg.bitv.displays ) 
    {
    char c;
    lisy80_debug("display change detected");
    fprintf(stderr,"\nPlayer1: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player1[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }
    fprintf(stderr,"\nPlayer2: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player2[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }

    fprintf(stderr,"\nPlayer3: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player3[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }
    fprintf(stderr,"\nPlayer4: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player4[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }

    fprintf(stderr,"\nCredits: %c%c",my_seg2char(mysegments.disp.credits1),my_seg2char(mysegments.disp.credits2));
    fprintf(stderr,"\nBalls: %c%c",my_seg2char(mysegments.disp.balls1),my_seg2char(mysegments.disp.balls2));
    fprintf(stderr,"\n\n ");
    }

    //check which line has changed

  }
}

//display handler System11A
void lisy_w_display_handler_SYS11A(void)
{
  static UINT8 first = 1;
  UINT8 i,k;
  UINT16 status[4];
  UINT16 sum1,sum2;
  int len;


  static t_mysegments_s11 mysegments;
  t_mysegments_s11 tmp_segments;

  if(first)
  {
        memset(mysegments.segments,0,sizeof(mysegments.segments));
	//for system11A display 1&2 are SEG14, 3&4 SEG7, we keep status display as ASCII
	lisy_api_display_set_prot(1,4);
	lisy_api_display_set_prot(2,4);
	lisy_api_display_set_prot(3,3);
	lisy_api_display_set_prot(4,3);
	first=0;
  }
 
  //something changed?
  if  ( memcmp(mysegments.segments,coreGlobals.segments,sizeof(mysegments)) != 0)
  {
    //store it
    memcpy(tmp_segments.segments,coreGlobals.segments,sizeof(mysegments));
    //check it display per display
    len = sizeof(mysegments.disp.player1);
    if( memcmp( tmp_segments.disp.player1,mysegments.disp.player1,len) != 0) send_SEG14_to_display(1, tmp_segments.disp.player1);
    if( memcmp( tmp_segments.disp.player2,mysegments.disp.player2,len) != 0) send_SEG14_to_display(2, tmp_segments.disp.player2);
    //if( memcmp( tmp_segments.disp.player3,mysegments.disp.player3,len) != 0) send_ASCII_to_display(3, tmp_segments.disp.player3);
    //if( memcmp( tmp_segments.disp.player4,mysegments.disp.player4,len) != 0) send_ASCII_to_display(4, tmp_segments.disp.player4);
    if( memcmp( tmp_segments.disp.player3,mysegments.disp.player3,len) != 0) send_SEG7_to_display(3, tmp_segments.disp.player3);
    if( memcmp( tmp_segments.disp.player4,mysegments.disp.player4,len) != 0) send_SEG7_to_display(4, tmp_segments.disp.player4);
    //status display
    sum1 = tmp_segments.disp.balls1 + tmp_segments.disp.balls2 + tmp_segments.disp.credits1 + tmp_segments.disp.credits2;
    sum2 = mysegments.disp.balls1 + mysegments.disp.balls2 + mysegments.disp.credits1 + mysegments.disp.credits2;
    if (sum1 != sum2 )
	{
	 status[0]=tmp_segments.disp.credits1;
	 status[1]=tmp_segments.disp.credits2;
	 status[2]=tmp_segments.disp.balls1;
	 status[3]=tmp_segments.disp.balls2;
	 send_ASCII_to_display(0, status);
	}
    //remember it
    memcpy(mysegments.segments,coreGlobals.segments,sizeof(mysegments));

    //and print out if debug display
    if ( ls80dbg.bitv.displays ) 
    {
char c;
UINT16 rt;
    fprintf(stderr,"\nPlayer1: ");
    for(i=0; i<=6; i++) 
    {
      rt = mysegments.disp.player1[i];
      fprintf(stderr,"0x%04x ",rt);
    }
//test RTH print chars
    fprintf(stderr,"\nPlayer1: ");
    for(i=0; i<=6; i++)
    {
      c=my_seg14_2char(mysegments.disp.player1[i]);
      fprintf(stderr,"%c",c);
    }



    fprintf(stderr,"\nPlayer2: ");
    for(i=0; i<=6; i++) 
    {
      rt = mysegments.disp.player2[i];
      fprintf(stderr,"0x%04x ",rt);
    }
//test RTH print chars
    fprintf(stderr,"\nPlayer2: ");
    for(i=0; i<=6; i++)
    {
      c=my_seg14_2char(mysegments.disp.player2[i]);
      fprintf(stderr,"%c",c);
    }

  
    fprintf(stderr,"\nPlayer3: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player3[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }
    fprintf(stderr,"\nPlayer4: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player4[i]); 
      if ( c>=0x80 ) { c=c-0x80; fprintf(stderr,"%c",c); fprintf(stderr,"."); }
      else fprintf(stderr,"%c",c);
    }

    fprintf(stderr,"\nCredits: %c%c",my_seg2char(mysegments.disp.credits1),my_seg2char(mysegments.disp.credits2));
    fprintf(stderr,"\nBalls: %c%c",my_seg2char(mysegments.disp.balls1),my_seg2char(mysegments.disp.balls2));
    fprintf(stderr,"\n\n ");
    }

    //check which line has changed

  }
}


//display handler
//we switch here according to system type
void lisy_w_display_handler(void)
{

 switch(lisymini_game.typeno)
 {
  case LISYW_TYPE_SYS7: 
	lisy_w_display_handler_SYS7();
       break;
  case LISYW_TYPE_SYS9: 
	lisy_w_display_handler_SYS9();
       break;
  case LISYW_TYPE_SYS11A: 
	lisy_w_display_handler_SYS11A();
       break;
 }
}

/*
  switch handler
  we use core_setSw and let pinmame
  read core switches
*/
void lisy_w_switch_handler( void )
{
int i,ret;
unsigned char action;
static unsigned char first=1;
static int simulate_coin_flag = 0;

//on first call read alls wicthes from APC
if (first)
{
 //get the switch status from APC andf set internal pinmame matrix
 for(i=1; i<=64; i++) core_setSw( i, lisy_api_get_switch_status(i) );
 
 //special handling for switches in coin door
 //goes via IRQ on Williams MPU
 //in APC advance is #72 and up/down switch #73

 switch(lisymini_game.typeno)
 {
        case LISYW_TYPE_SYS7:
        case LISYW_TYPE_SYS9:
 		core_setSw( S7_SWADVANCE, lisy_api_get_switch_status(72) );
 		core_setSw( S7_SWUPDN, lisy_api_get_switch_status(73) );
                break;
        case LISYW_TYPE_SYS11A:
 		core_setSw( S11_SWADVANCE, lisy_api_get_switch_status(72) );
 		core_setSw( S11_SWUPDN, lisy_api_get_switch_status(73) );
                break;
 }

 first=0;
}



//read values over usbserial
//check if there is an update first
ret = lisy_w_switch_reader( &action );

//if debug mode is set we get our reedings from udp switchreader in additon
//but do not overwrite real switches
if ( ( ls80dbg.bitv.basic ) & ( ret == 80))
 {
   if ( ( ret = lisy_udp_switch_reader( &action, 0 )) != 80)
   {
     sprintf(debugbuf,"LISY_W_SWITCH_HANDLER (UDP Server Data received: %d",ret);
     lisy80_debug(debugbuf);
   }
 }

//jump back in case no switch pressed
if ( ret == 80) return;

//NOTE: system has has 8*8==64 switches in maximum, counting 1...64; ...
if (ret <= 64 ) //ret is switchnumber
      {

        //set the switch
        core_setSw( ret, action );

        if ( ls80dbg.bitv.switches )
        {
           sprintf(debugbuf,"LISY_W_SWITCH_HANDLER Switch#:%d action:%d\n",ret,action);
           lisy80_debug(debugbuf);
        }
  return;
  } //if ret <= 64

//special switches
if ( ret == 71) {
        //memory protect; debugging only, as withon pinmamem this switch is not used
        if ( ls80dbg.bitv.switches )
        {
           sprintf(debugbuf,"LISY_W_SWITCH_HANDLER MEMORY PROTECT(%d) action:%d\n",ret,action);
           lisy80_debug(debugbuf);
        }
    }
//advance
if ( ret == 72) {
 switch(lisymini_game.typeno)
 {
        case LISYW_TYPE_SYS7:
          core_setSw( S7_SWADVANCE, action );
          if ( ls80dbg.bitv.switches )
          {
             sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S7_SWADVANCE(%d) action:%d\n",ret,action);
             lisy80_debug(debugbuf);
          }
	  break;
        case LISYW_TYPE_SYS9:
        case LISYW_TYPE_SYS11A:
          core_setSw( S11_SWADVANCE, action );
          if ( ls80dbg.bitv.switches )
          {
             sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S11_SWADVANCE(%d) action:%d\n",ret,action);
             lisy80_debug(debugbuf);
          }
	  break;
   }
    }

//up down
if ( ret == 73) {
 switch(lisymini_game.typeno)
 {
        case LISYW_TYPE_SYS7:
          core_setSw( S7_SWUPDN, action );
          if ( ls80dbg.bitv.switches )
          {
             sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S7_SWUPDN(%d) action:%d\n",ret,action);
             lisy80_debug(debugbuf);
          }
          break;
        case LISYW_TYPE_SYS9:
        case LISYW_TYPE_SYS11A:
          core_setSw( S11_SWUPDN, action );
          if ( ls80dbg.bitv.switches )
          {
             sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S11_SWUPDN(%d) action:%d\n",ret,action);
             lisy80_debug(debugbuf);
          }
          break;
   }
    }

if ( ret == 78) {  //not from APC but maybe from udb reader for testing
        core_setSw( S11_SWCPUDIAG, action );
        if ( ls80dbg.bitv.switches )
        {
           sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S11_SWCPUDIAG(%d) action:%d\n",ret,action);
           lisy80_debug(debugbuf);
        }
    }
if ( ret == 79) { //not from APC but maybe from udb reader for testing
        core_setSw( S11_SWSOUNDDIAG, action );
        if ( ls80dbg.bitv.switches )
        {
           sprintf(debugbuf,"LISY_W_SWITCH_HANDLER S11_SWSOUNDDIAG(%d) action:%d\n",ret,action);
           lisy80_debug(debugbuf);
        }
    }
/*
//do we need a 'special' routine to handle that switch?
//system35 Test switch is separate but mapped to strobe:6 ret:7
//so matrix 7,7 to check
if ( CHECK_BIT(swMatrixLISY_W[7],7)) //is bit set?
 {
    //after 3 secs we initiate shutdown; internal timer 0
    //def lisy_timer( unsigned int duration, int command, int index)
    if ( lisy_timer( 3000, 0, 0)) lisy_time_to_quit_flag = 1;
 }
 else // bit is zero, reset timer index 0
 {
    lisy_timer( 0, 1, 0);
 }

if (ls80opt.bitv.freeplay == 1) //only if freeplay option is set
{
//Williams system35 (Credit) Replay switch is #3 strobe:0 ret:3, so matrix 0,5 to check
 if ( CHECK_BIT(swMatrixLISY_W[0],5)) //is bit set?
 {
    //after 2 secs we simulate coin insert via Chute#1; internal timer 1
    //def lisy_timer( unsigned int duration, int command, int index)
    if ( lisy_timer( 2000, 0, 1)) { simulate_coin_flag = 1;  lisy_timer( 0, 1, 1); }
 }
 else // bit is zero, reset timer index 0
 {
    lisy_timer( 0, 1, 1);
 }

//do we need to simalte coin insert?
 if ( simulate_coin_flag )
 {
    //simulate coin insert for 50 millisecs via timer 2
    // left coin is switch6; strobe 0 ret 6, so matrix is 0,6 to set
    //def lisy_timer( unsigned int duration, int command, int index)
     SET_BIT(swMatrixLISY_W[0],6);
     if ( lisy_timer( 50, 0, 2)) { CLEAR_BIT(swMatrixLISY_W[0],6); simulate_coin_flag = 0; }
 }
 else // bit is zero, reset timer index 0
 {
    lisy_timer( 0, 1, 2);
 }
} //freeplay option set
*/
  return;
}

//solenoid handler
void lisy_w_solenoid_handler( void )
{

int i,j,sol_no;
static UINT32 mysol=0;
int mux_sol_active = 0;
uint8_t action;
//remember if we need a delayd ac select activation
//this is needed as we sometimes in pinmame have a ac select
//activation too early (see also PROC code in s11.c)
static uint8_t ac_want_to_change = 0; //1== action 0; 2==action 2
static uint8_t current_ac_state = 0;

//did something changed?
if ( mysol != coreGlobals.solenoids)
{
   //check all solenoids 
   for(i=0; i<=31; i++)
    {
      //send to APC in case something changed
      if( CHECK_BIT(mysol,i) != CHECK_BIT(coreGlobals.solenoids,i) )
      {
	//do we activate or do we deactivate
        if ( CHECK_BIT(coreGlobals.solenoids,i)) action = 1; else action = 0;
	//sol number starts with 1
        sol_no = i+1;

	//in case of Solenoid 14 (AC Relais)
        //check if one of the multiplexed sols are still active
        // (RTH: TODO need another routine for Road Kings)
	if ( ( sol_no == 14) & (lisy_has_AC_Relais == 1) )
	{
	  //lets check if any of the muxed solenoids are active now
	  //in pinmame these are 1..8 ( AC-relais 0) and 25..33 ( AC-relais 1)
	  for(j=0; j<=7; j++)  if ( CHECK_BIT(coreGlobals.solenoids,j)) mux_sol_active++;
	  for(j=24; j<=31; j++)  if ( CHECK_BIT(coreGlobals.solenoids,j)) mux_sol_active++;
          //no muxed solenoid active, activate ac relais
	  if (mux_sol_active == 0) 
	    {
	      lisy_api_sol_ctrl(14,action);
	      current_ac_state = action;
	      if ( ls80dbg.bitv.coils )
		{
                  sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: AC-Relais changed to %d",action);
                  lisy80_debug(debugbuf);
	        }
	    }
	  else //we have active solenoids, ac relais needs to be delayd
	   {
	        ac_want_to_change = action + 1; //just remember for next round
	        if ( ls80dbg.bitv.coils )
		{
                  sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: AC-Relais wants change to %d, DELAYED due %d active muxed Solenoids",mux_sol_active,action);
                  lisy80_debug(debugbuf);
	        }
	   }
	}//sol == 14	

        //for ac relais (sol 14) we have special routine (see above)
	//special solenoids, we with HW rules only by ignoring special switches 65 ... 70
	//if the pinball (e.g. pinbot) is using special solenoids 'normal' we do it here
	if (lisy_has_AC_Relais == 1)
	  {
        	if ( ( sol_no != 14) &( sol_no <= 22 )) lisy_api_sol_ctrl(sol_no,action);
	  }
	else
	  {
		//include sol 14 for systems without AC Relais
        	if ( sol_no <= 22 ) lisy_api_sol_ctrl(sol_no,action);
	  }

        //in case we hav solenoid #23, also activate #24 on APC
        //as APC use two solenoids for flipper (left/right)
        if (sol_no == 23 ) { lisy_api_sol_ctrl(23,action); lisy_api_sol_ctrl(24,action); }
	//for games without AC Relais Williams use Sol 25 for flipper enable
        if ( (sol_no == 25 ) & (lisy_has_AC_Relais == 0)) { lisy_api_sol_ctrl(23,action); lisy_api_sol_ctrl(24,action); }

	//with A-C Relais Solenoids 1..8 are muxed, in pinmame we have the 'C-Side' as Solenoids 25..33
        //so we need to substract 24 before sending command to APC
	if (( sol_no >=25) & (lisy_has_AC_Relais == 1)) { lisy_api_sol_ctrl(sol_no-24,action); }

        //debug?
        if ( ( ls80dbg.bitv.coils ) & ( sol_no != 14))
        {
	  if ( ( sol_no < 25)& (lisy_has_AC_Relais == 1))
           { sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: Solenoid:%d, changed to %d ( AC is %d)",sol_no,action,current_ac_state); }
	  else if ( ( sol_no >=  25)& (lisy_has_AC_Relais == 1))
          { sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: Solenoid:%d(%d), changed to %d ( AC is %d)",sol_no-24,sol_no,action,current_ac_state); }
          else
          { sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: Solenoid:%d, changed to %d ( no AC Relais) ",sol_no,action); }

           lisy80_debug(debugbuf);
         }

	//do we have a delayd ac activation from last round?
	if (ac_want_to_change != 0)
	{
	      //is it now save to activate?
	      for(j=0; j<=7; j++)  if ( CHECK_BIT(coreGlobals.solenoids,j)) mux_sol_active++;
	      for(j=24; j<=31; j++)  if ( CHECK_BIT(coreGlobals.solenoids,j)) mux_sol_active++;
	      if (mux_sol_active == 0)
		{
	          lisy_api_sol_ctrl(14,ac_want_to_change-1); 
                  if ( ls80dbg.bitv.coils )
                  {
                    sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: AC-Relais DELAYD change to %d",ac_want_to_change-1);
                    lisy80_debug(debugbuf);
                  }
		  current_ac_state = ac_want_to_change-1;
		  ac_want_to_change = 0; //reset flag
		}
	 mux_sol_active = 0; //reset counter
	}
      }//something changed
     }//for
   //store it for next call
   mysol = coreGlobals.solenoids;
  }

}//solenoid_handler

//read the csv file on /lisy partition and the DIP switch setting
//give back gamename accordently and line number
// -1 in case we had an error
//this is called early from unix/main.c
int lisymini_get_gamename(char *gamename)
{
 int ret;

 //use function from fileio to get more details about the gamne
 ret =  lisymini_file_get_gamename( &lisymini_game);

  //give back the name and the number of the game
  strcpy(gamename,lisymini_game.gamename);

  //store throttle value from gamelist to global var
  g_lisymini_throttle_val = lisymini_game.throttle;

  //give this info on screen
  if ( ls80dbg.bitv.basic )
  {
    sprintf(debugbuf,"Info: LISYMINI Throttle value is %d for this game",g_lisymini_throttle_val);
    lisy80_debug(debugbuf);
    }

  return ret;
}

//get changed switch from usbserial via LISY_API
// if no change, returnvalue is 80
// if change happened , identiyf switch and action and give back
unsigned char lisy_w_switch_reader( unsigned char *action )
{

 unsigned char switch_number;
 static int num = 0;

 //this routine is called every 0,5 msec, which 2000 per second
 num++;
 if ( num > 40)
 { 
  num = 0;
  switch_number  = lisy_api_ask_for_changed_switch();
 }
 else return 80; //no asking this round

 //no change if we get 127, this is 80 LISY internal
 if (switch_number == 127) return 80;

 if ( ls80dbg.bitv.switches )
 {
     sprintf(debugbuf,"LISY_W_SWITCH_READER: changed switch reported: returnbyte:%d",switch_number);
     lisy80_debug(debugbuf);
 }

 //otherwise check action for switch ( ON/OFF )
 if ( CHECK_BIT( switch_number,7))
 {
  *action = 1;
  CLEAR_BIT( switch_number,7);
 }
 else *action = 0;

if ( ls80dbg.bitv.switches )
 {
     sprintf(debugbuf,"LISY_W_SWITCH_READER: return switch: %d, action: %d",switch_number,*action);
     lisy80_debug(debugbuf);
 }
 return switch_number;

}

//lamp handler
void lisy_w_lamp_handler( )
{

 int i,j,lamp_no;
 static UINT8 first = 1;
 uint8_t action;

 static UINT8 mylampMatrix[8]; //8*8 = 64 lamps in maximum

  if(first)
  {
        memset(mylampMatrix,0,sizeof(mylampMatrix));
        first=0;
  }

  //something changed?
  if  ( memcmp(mylampMatrix,coreGlobals.lampMatrix,sizeof(mylampMatrix)) != 0)
  {
   //check all lamps
   for(i=0; i<=7; i++)
    {
     for(j=0; j<=7; j++)
      {
        if( CHECK_BIT(mylampMatrix[i],j) != CHECK_BIT(coreGlobals.lampMatrix[i],j) )
        {
	 //send to APC
	 lamp_no = i*8 + j +1;
	 if ( CHECK_BIT(coreGlobals.lampMatrix[i],j)) action = 1; else action = 0;
	 lisy_api_lamp_ctrl(lamp_no,action);
         if ( ls80dbg.bitv.lamps )
         {
         sprintf(debugbuf,"LISY_W_LAMP_HANDLER: Lamp:%d, changed to %d",lamp_no,action);
         lisy80_debug(debugbuf);
         } //if debug
       }//if changed
    }//j
   }//i
  //store it
  memcpy(mylampMatrix,coreGlobals.lampMatrix,sizeof(mylampMatrix));
 }//changed
}//lamp_handler

//sound handler
void lisy_w_sound_handler(unsigned char board, unsigned char data)
{
  char filename[40];

  if ( ls80dbg.bitv.sound )
  {
    sprintf(debugbuf,"LISY_W sound_handler: board:%d 0x%x (%d)",board,data,data);
    lisy80_debug(debugbuf);
    }


      //use command  play index and let do APC the work ;-)
      lisy_api_sound_play_index(board,data);

}

//shutdown lisy mini
void lisy_mini_shutdown(void)
{

 fprintf(stderr,"LISY Mini graceful shutdown initiated\n");
 //show the 'shutdown' message
 //set displays  one and two to ASCII with dot (6)  for boot message
 lisy_api_display_set_prot( 1, 6);
 lisy_api_display_set_prot( 2, 6);
 lisy_api_send_str_to_disp( 1, "DO SHUT");
 lisy_api_send_str_to_disp( 2, "DOWN   ");

}

//read the csv file on /lisy partition and the DIP switch setting
//give back gamename accordently and line number
// -1 in case we had an error
//this is called early from unix/main.c
int lisyapc_get_gamename(char *gamename)
{
 int ret;

 //use function from fileio to get more details about the gamne
 ret =  lisyapc_file_get_gamename( &lisymini_game);

  //give back the name and the number of the game
  strcpy(gamename,lisymini_game.gamename);

  //store throttle value from gamelist to global var
  g_lisymini_throttle_val = lisymini_game.throttle;

  //give this info on screen
  if ( ls80dbg.bitv.basic )
  {
    sprintf(debugbuf,"Info: LISYAPC Throttle value is %d for this game",g_lisymini_throttle_val);
    lisy80_debug(debugbuf);
    }

  return ret;
}

