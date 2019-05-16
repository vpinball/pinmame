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
#include "lisyversion.h"

//typedefs

/*
from core.c
#define DISP_SEG_7(row,col,type) {4*row,16*col,row*20+col*8+1,7,type}
*
core.h:#define DISP_SEG_BALLS(no1,no2,type)  {2,8,no1,1,type},{2,10,no2,1,type}
*
core.h:#define DISP_SEG_CREDIT(no1,no2,type) {2,2,no1,1,type},{2,4,no2,1,type}
*
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
  } t_mysegments;


//global var for internal game_name structure,
//set by  lisy80_get_gamename in unix/main
t_stru_lisymini_games_csv lisymini_game;

//global var for timing and speed
int g_lisymini_throttle_val = 300;

//internal switch Matrix for Williams system1, we need 9 elements
//as pinmame internal starts with 1
unsigned char swMatrixLISY_W[9] = { 0,0,0,0,0,0,0,0,0 };


//init SW portion of lisy_w
void lisy_w_init( void )
{
 int i,sb;
 char s_lisy_software_version[16];


 //do the init on vars
 //for (i=0; i<=36; i++) lisy1_lamp[i]=0;

 //set signal handler
 lisy80_set_sighandler();

 //show up on calling terminal
 sprintf(s_lisy_software_version,"%02d.%03d ",LISY_SOFTWARE_MAIN,LISY_SOFTWARE_SUB);
 fprintf(stderr,"This is LISY (Lisy Mini) by bontango, Version %s\n",s_lisy_software_version);

 //show the 'boot' message
 //display_show_boot_message_lisy35(s_lisy_software_version);

 //show green ligth for now, lisy1 is running
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

 //see how many micros passed
 now = micros();
 //beware of 'wrap' which happens each 71 minutes
 if ( now < last) now = last; //we had a wrap

 //calculate difference and sleep a bit
 sleeptime = g_lisymini_throttle_val - ( now - last);
 if (sleeptime > 0) delayMicroseconds ( sleeptime );

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
//gets a segment value and give back char value 
char my_seg2char( UINT16 segvalue ) 
{

 UINT8 hascomma = 0;
 char retchar = ' ';

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
	default: retchar = ' ';
   }

 if(hascomma) retchar = retchar + 0x80;
 return (retchar);;
}

void send_to_display( int no, UINT16 *dispval)
{
 int i;
 char str[8]; //include null termination

 if (no) { for(i=0; i<=6; i++) str[i] = my_seg2char( dispval[i]); str[7] = '\0'; }
 else { for(i=0; i<=3; i++) str[i] = my_seg2char( dispval[i]); str[5] = '\0'; } //status display

 lisy_usb_send_str_to_disp(no, str);


}

//display handler
void lisy_w_display_handler(void)
{
  static UINT8 first = 1;
  UINT8 i,k;
  UINT16 status[4];
  UINT16 sum1,sum2;
  int len;


  static t_mysegments mysegments;
  t_mysegments tmp_segments;

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
    if( memcmp( tmp_segments.disp.player1,mysegments.disp.player1,len) != 0) send_to_display(1, tmp_segments.disp.player1);
    if( memcmp( tmp_segments.disp.player2,mysegments.disp.player2,len) != 0) send_to_display(2, tmp_segments.disp.player2);
    if( memcmp( tmp_segments.disp.player3,mysegments.disp.player3,len) != 0) send_to_display(3, tmp_segments.disp.player3);
    if( memcmp( tmp_segments.disp.player4,mysegments.disp.player4,len) != 0) send_to_display(4, tmp_segments.disp.player4);
    //status display
    sum1 = tmp_segments.disp.balls1 + tmp_segments.disp.balls2 + tmp_segments.disp.credits1 + tmp_segments.disp.credits2;
    sum2 = mysegments.disp.balls1 + mysegments.disp.balls2 + mysegments.disp.credits1 + mysegments.disp.credits2;
    if (sum1 != sum2 )
	{
	 status[0]=tmp_segments.disp.credits1;
	 status[1]=tmp_segments.disp.credits2;
	 status[2]=tmp_segments.disp.balls1;
	 status[3]=tmp_segments.disp.balls2;
	 send_to_display(0, status);
	}
    //remember it
    memcpy(mysegments.segments,coreGlobals.segments,sizeof(mysegments));

    //and print out if debug display
    if ( ls80dbg.bitv.displays ) 
    {
    char c;
    lisy80_debug("display change detected");
    printf("\nPlayer1: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player1[i]); 
      if ( c>=0x80 ) { c=c-0x80; printf("%c",c); printf("."); }
      else printf("%c",c);
    }
    printf("\nPlayer2: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player2[i]); 
      if ( c>=0x80 ) { c=c-0x80; printf("%c",c); printf("."); }
      else printf("%c",c);
    }
    printf("\nPlayer3: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player3[i]); 
      if ( c>=0x80 ) { c=c-0x80; printf("%c",c); printf("."); }
      else printf("%c",c);
    }
    printf("\nPlayer4: ");
    for(i=0; i<=6; i++) 
    {
      c=my_seg2char(mysegments.disp.player4[i]); 
      if ( c>=0x80 ) { c=c-0x80; printf("%c",c); printf("."); }
      else printf("%c",c);
    }

    printf("\nCredits: %c%c",my_seg2char(mysegments.disp.credits1),my_seg2char(mysegments.disp.credits2));
    printf("\nBalls: %c%c",my_seg2char(mysegments.disp.balls1),my_seg2char(mysegments.disp.balls2));
    printf("\n\n ");
    }

    //check which line has changed

  }
}

/*
  switch handler
  give back the value of the pinmame Matrix byte
  swMatrix[0] is pinmame internal 
  swMatrix[1..8] is Williams
*/
unsigned char lisy_w_switch_handler( int col )
{
int ret;
unsigned char strobe,returnval,action;
static int simulate_coin_flag = 0;


// get the truth columni tcol 1..8 (code from core.c)
// col is 1,2,4,8,16,32, ...
  int tcol = 1;
  if (col) {
    while ((col & 0x01) == 0) {
      col >>= 1;
      tcol += 1;
    }
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
     //we start internally with 0, so substract one
     --ret;
   }
 }

//NOTE: system has has 8*8==64 switches in maximum, counting 1...64; ...
//we use 'internal strobe 6' to handle special switches in the same way ( TEST=49,S33=50 )
if (ret < 80) //ret is switchnumber
      {

        //calculate strobe & return
        //Note: this is different from system80
        strobe = ret / 8;
        returnval = ret % 8;

        //set the bit in the Matrix var according to action
        // action 1 means set the bit
        // any other means delete the bit
	//add 1 to strobe as we start with 1 not with zero
        if (action ) //set bit
                   SET_BIT(swMatrixLISY_W[strobe+1],returnval);
        else  //delete bit
                   CLEAR_BIT(swMatrixLISY_W[strobe+1],returnval);

        if ( ls80dbg.bitv.switches )
        {
           sprintf(debugbuf,"LISY_W_SWITCH_HANDLER Switch#:%d strobe:%d return:%d action:%d\n",ret,strobe+1,returnval,action);
           lisy80_debug(debugbuf);
        }
  } //if ret < 80 => update internal matrix

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

  return(swMatrixLISY_W[tcol]);
}

//get special switches
UINT8 lisy_w_get_special_switch( UINT8 sw)
{

//lisy80_debug("spec switch");
return 1;

}

//solenoid handler
void lisy_w_solenoid_handler( void )
{

int i,sol_no;
static UINT32 mysol=0;
uint8_t action;


//did something changed?
if ( mysol != coreGlobals.solenoids)
{
   //check all solenoids
   for(i=0; i<=31; i++)
    {
      //send to APC
      if( CHECK_BIT(mysol,i) != CHECK_BIT(coreGlobals.solenoids,i) )
      {
        if ( CHECK_BIT(coreGlobals.solenoids,i)) action = 1; else action = 0;
        sol_no = i+1;
        lisy_usb_sol_ctrl(sol_no,action);
      //debug?
      if ( ls80dbg.bitv.coils )
      {
         sprintf(debugbuf,"LISY_W_SOLENOID_HANDLER: Solenoid:%d, changed to %d",sol_no,action);
         lisy80_debug(debugbuf);
	}
      }
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
 static int pollrate = 100; //fixed pollrate 100 per second  at the moment
 static int num = 0;

 //this routine is called every 0,5 msec, which 2000 per second
 //RTH check throttl value
 num++;
 if ( num > (2000/pollrate))
 { 
  num = 0;
  switch_number  = lisy_usb_ask_for_changed_switch();
 }
 else return 80; //no asking this round

 //no change if we get 127, this is 80 LISY internal
 if (switch_number == 127) return 80;

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

//display handler
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
	 lisy_usb_lamp_ctrl(lamp_no,action);
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
