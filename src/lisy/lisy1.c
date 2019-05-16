/*
 LISY1.c
 May 2018
 bontango
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "xmame.h"
#include "driver.h"
#include "wpc/core.h"
#include "wpc/sndbrd.h"
#include "lisy1.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "switches.h"
#include "utils.h"
#include "eeprom.h"
#include "sound.h"
#include "lisy.h"
#include "lisy_mame.h"
#include "lisyversion.h"


//global var for internal game_name structure,
//set by  lisy80_get_gamename in unix/main
t_stru_lisy1_games_csv lisy1_game;

//global var for timing and speed
int g_lisy1_throttle_val = 3000;

//internal switch Matrix for system1, we need 7 elements
//as pinmame internal starts with 1
//swMatrix 6 for SLAM and other special switches
unsigned char swMatrixLISY1[7] = { 0,0,0,0,0,0,0 };

//global flag for nvram write
static int lisy1_nvram_delayed_write = 0;

//global var for coil min pulse time option, always activated for lisy1
int lisy1_coil_min_pulse_time[8] = { 0,0,0,0,0,0,0,0};


//all the lamps (36)
//initial value is 7 to indicate first chnage in routine
//int will do set to zero independent

unsigned char lisy1_lamp[37] =
     { 7,7,7,7,7,7,7,7,7,7,
       7,7,7,7,7,7,7,7,7,7,
       7,7,7,7,7,7,7,7,7,7,
       7,7,7,7,7,7,7, };

//global var for sound options
unsigned char lisy1_has_soundcard = 0;  //there is a pHat soundcard installed
unsigned char lisy1_has_own_sounds = 0;  //play own sounds rather then usinig piname sound emulation


//init SW portion of lisy1
void lisy1_init( void )
{
 int i;
 char s_lisy_software_version[16];


 //do the init on vars
 for (i=0; i<=36; i++) lisy1_lamp[i]=0;

 //set signal handler
 lisy80_set_sighandler();

 //show up on calling terminal
 sprintf(s_lisy_software_version,"%02d.%03d ",LISY_SOFTWARE_MAIN,LISY_SOFTWARE_SUB);
 fprintf(stderr,"This is LISY (Lisy1) by bontango, Version %s\n",s_lisy_software_version);

 //show the 'boot' message
 display_show_boot_message_lisy1(s_lisy_software_version,lisy1_game.rom_id,lisy1_game.gamename);

 //check sound options
 if ( ls80opt.bitv.JustBoom_sound )
   {
     lisy1_has_soundcard = 1;
     if ( ls80dbg.bitv.sound) lisy80_debug("internal soundcard to be activated");
     //do we want to use pinamme sounds?
     if ( ls80opt.bitv.test )
      {
       if ( ls80dbg.bitv.sound) lisy80_debug("we try to use pinmame sounds");
      }
      else
      {
        lisy1_has_own_sounds = 1;
        if ( ls80dbg.bitv.sound) lisy80_debug("we try to use our own sounds");
      }
   }

 // try say something about LISY80 if soundcard is installed
 if ( lisy1_has_soundcard )
 {
  //set volume according to poti
  lisy_adjust_volume();
  sprintf(debugbuf,"/bin/echo \"Welcome to LISY 1 Version %s\" | /usr/bin/festival --tts",s_lisy_software_version);
  system(debugbuf);
 }

 //show green ligth for now, lisy1 is running
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(1);

 //check for coil min_pulse parameter
 //option is active if value is either 0 or 1
  fprintf(stderr,"Info: checking for Coil min pulse time extension config\n");
  //first try to read coil opts, for current game
  if ( lisy1_file_get_coilopts() < 0 )
   {
     fprintf(stderr,"Info: no coil opts file; or error occured, using defaults\n");
   }
  else
   {
     fprintf(stderr,"Info: coil opt file read OK, min pulse time set\n");

     if ( ls80dbg.bitv.coils) {
     int i;
     for(i=0; i<=7; i++)
       fprintf(stderr,"coil No [%d]: %d msec minimum pulse time\n",i,lisy1_coil_min_pulse_time[i]);
    }
   }


 if ( lisy1_has_own_sounds )
 {
  //now open soundcard, and init soundstream
  if ( lisy1_sound_stream_init() < 0 )
   {
     fprintf(stderr,"sound init failed, sound emulation disabled\n");
     lisy1_has_own_sounds = 0;
   }
 else
   fprintf(stderr,"info: sound init done\n");
 }

}

//read the csv file on /lisy partition and the DIP switch setting
//give back gamename accordently and line number
// -1 in case we had an error
//this is called early from unix/main.c
int lisy1_get_gamename(char *gamename)
{

 int ret;

 //use function from fileio to get more details about the gamne
 ret =  lisy1_file_get_gamename( &lisy1_game);


  //give back the name and the number of the game
  strcpy(gamename,lisy1_game.gamename);

  //store throttle value from gamelist to global var
  g_lisy1_throttle_val = lisy1_game.throttle;
  //give this info on screen
  fprintf(stderr,"LISY1: Throttle value is %d\n",g_lisy1_throttle_val);

  //other infos are stored in global var

  return ret;
}

//help function
//we may later delete this

char my_datchar( int dat)
{
char datchar;

switch (dat) {
      case 0: datchar = '0'; break;
      case 1: datchar = '1'; break;
      case 2: datchar = '2'; break;
      case 3: datchar = '3'; break;
      case 4: datchar = '4'; break;
      case 5: datchar = '5'; break;
      case 6: datchar = '6'; break;
      case 7: datchar = '7'; break;
      case 8: datchar = '8'; break;
      case 9: datchar = '9'; break;
      case 15: datchar = ' '; break;
      default: datchar = '-'; break;
        }

return datchar;
}

//display handler
void lisy1_display_handler( int index, int value )
{

static int myvalue[48]
= { 80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80,
    80, 80, 80, 80, 80, 80 };

int i;
int has_changed = 0;


if ( index > 47 ) 
  {
    fprintf(stderr,"LISY1_DISP_HANDLER: index out of range index:%d  value:%d\n",index,value);
    return;
  }

if ( value != myvalue[index] )
  {
    has_changed = 1; //for possible debugging only
    //remember
    myvalue[index] = value;

  switch(index) //we need to recalculate index each time as it has the reverse order for LISY1 compared with LISY80
   {
     case 12 ... 17: //system1 player 1 
	index = 29 - index; //new order 17 ... 12
	display1_show_int( 1, index-12, value);
	break;
     case 18 ... 23: //system1 player 2 
	index = 41 - index; //new order 23 ... 18
	display1_show_int( 2, index-18, value);
	break;
     case 0 ... 5: //system1 player 3 
	index = 5 - index; //new order 5 ... 0
	display1_show_int( 3, index, value);
	break;
     case 6 ... 11: //system1 player 4 
	index = 17 - index; //new order 11 ... 6
	display1_show_int( 4, index-6, value);
	break;
     case 32: //system1 status display credits -> 'digits 3&4'
	display1_show_int( 0, 3, value);
	break;
     case 33: //system1 status display credits -> 'digits 3&4'
	display1_show_int( 0, 2, value);
	break;
     case 40: //system1 status display  'ball in play' -> 'digits 0&1'
	display1_show_int( 0, 1, value);
	break;
     case 41: //system1 status display  'ball in play' -> 'digits 0&1'
	display1_show_int( 0, 0, value);
	break;
   }//switch

  }


/* now in displays.c
if ( ls80dbg.bitv.displays )
{
  if ( has_changed )
  {
    printf("----- Index %d changed to %d -----\n",index,value);

    printf("Player1:");
    //for (i=17; i>=12; i--) printf("%c",my_datchar(myvalue[i]));
    for (i=12; i<=17; i++) printf("%c",my_datchar(myvalue[i]));
    printf("\n");
    printf("Player2:");
    for (i=23; i>=18; i--) printf("%c",my_datchar(myvalue[i]));
    printf("\n");
    printf("Player3:");
    for (i=5; i>=0; i--) printf("%c",my_datchar(myvalue[i]));
    printf("\n");
    printf("Player4:");
    for (i=11; i>=6; i--) printf("%c",my_datchar(myvalue[i]));
    printf("\n");

    printf("Ball in Play:");
    printf("%c",my_datchar(myvalue[40]));
    printf("%c",my_datchar(myvalue[41]));
    printf("\n");

    printf("Credits:");
    printf("%c",my_datchar(myvalue[32]));
    printf("%c",my_datchar(myvalue[33]));
    printf("\n");
    printf("\n");
  }

 }
*/
}


/*
  switch handler
  give back the value of the pinmame Matrix byte 
  swMatrix[0] is pinmame internal (sound?)
  swMatrix[1..6] is system1, where 6 is not used?!
  swMatrix[7] is  'special switches' bit7:SLAM; bit6:Outhole; bit5:Reset
  Note: SLAM is reverse, which means closed with bit cleared (because of handling in gts1.c)
*/
unsigned char lisy1_switch_handler( int sys1strobe )
{
int ret;
unsigned char strobe,returnval,action;
static int simulate_coin_flag = 0;

//read values from pic
//check if there is an update first
ret = lisy1_switch_reader( &action );

//SLAM handling is reverse in lisy1, meaning 0 is CLOSED
//we suppress processing of SLAM Switch actions with option slam active
if ( (ret == 76) && (ls80opt.bitv.slam == 1)) return(swMatrixLISY1[sys1strobe-1]);


//NOTE: system has has 8*5==40 switches in maximum, counting 00..04;10...14; ...
//we use 'internal strobe 6' to handle special switches in the same way ( SLAM=06,OUTHOLE=16,RESET=26 )
if (ret < 80) //ret is switchnumber
      {

	//calculate strobe & return
	//Note: this is different from system80
        strobe = ret % 10;
        returnval = ret / 10;

        //set the bit in the Matrix var according to action
        // action 0 means set the bit
        // any other means delete the bit
        if (action == 0) //set bit 
          	   SET_BIT(swMatrixLISY1[strobe],returnval);
        else  //delete bit
          	   CLEAR_BIT(swMatrixLISY1[strobe],returnval);

	if ( ls80dbg.bitv.switches )
  	{
           sprintf(debugbuf,"LISY1_SWITCH_READER Switch#:%d strobe:%d return:%d action:%d\n",ret,strobe,returnval,action);
           lisy80_debug(debugbuf);
  	}
  } //if ret < 80 => update internal matrix

//do we need a 'special' routine to handle that switch?
//system1 Test switch strobe:0 ret:0
 if ( CHECK_BIT(swMatrixLISY1[0],0)) //is bit set?
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
//system1 Replay switch strobe:3 ret:0
 if ( CHECK_BIT(swMatrixLISY1[3],0)) //is bit set?
 {
    //after 3 secs we simulate coin insert via Chute#1; internal timer 1
    //def lisy_timer( unsigned int duration, int command, int index)
    if ( lisy_timer( 3000, 0, 1)) { simulate_coin_flag = 1;  lisy_timer( 0, 1, 1); }
 }
 else // bit is zero, reset timer index 0
 {
    lisy_timer( 0, 1, 1);
 }

//do we need to simalte coin insert?
 if ( simulate_coin_flag )
 {
    //simulate coin insert for 50 millisecs via timer 2
    // chute#1 is switch1 ; strobe 1 ret 0
    //def lisy_timer( unsigned int duration, int command, int index)
     SET_BIT(swMatrixLISY1[1],0);
     if ( lisy_timer( 50, 0, 2)) { CLEAR_BIT(swMatrixLISY1[1],0); simulate_coin_flag = 0; }
 }
 else // bit is zero, reset timer index 0
 {
    lisy_timer( 0, 1, 2);
 }
} //freeplay option set

//check SLAM switch if SLAM option is not set
//system1 SLAM switch strobe:6 ret:7
//reverse logic, but set means open
if ( (ls80opt.bitv.slam == 0) && CHECK_BIT(swMatrixLISY1[6],7)) //is bit set?
{
   fprintf(stderr," SLAM switch open!!\n");
   display_show_str( 1, "SLAM  ");
   display_show_str( 2, "OPEN  ");
   display_show_str( 3, "OPTION");
   display_show_str( 4, "AVAIL!");
   sleep(1);
}


  //just give back Matrix-Byte, should be updated by now or next cycle
  return(swMatrixLISY1[sys1strobe-1]);
}


//solenoid handler
//solenoids 0..8, this also includes sound
void lisy1_solenoid_handler(int ioport,int enable)
{

//we only cover first 8 solenoids, system1 games should not have more
if ( ioport > 7 )
  {
    if ( ls80dbg.bitv.coils )
      {
        sprintf(debugbuf,"LISY1_SOLENOID_HANDLER_A: ioport out of range:%d",ioport);
        lisy80_debug(debugbuf);
      }
    return;
   }

switch(ioport)
 {
   case 0: lisy1_coil_set( Q_OUTH, !enable); break;
   case 1: lisy1_coil_set( Q_KNOCK, !enable); break;
   case 2: lisy1_coil_set( Q_TENS, !enable);
           if ( ( lisy1_has_own_sounds ) && ( enable == 0)) lisy1_play_wav(1);
           break;
   case 3: lisy1_coil_set( Q_HUND, !enable);
           if ( ( lisy1_has_own_sounds ) && ( enable == 0)) lisy1_play_wav(2);
           break;
   case 4: lisy1_coil_set( Q_TOUS, !enable);
           if ( ( lisy1_has_own_sounds ) && ( enable == 0)) lisy1_play_wav(3);
           break;
   case 5: lisy1_coil_set( Q_SYS1_SOL6, !enable); break;
   case 6: lisy1_coil_set( Q_SYS1_SOL7, !enable); break;
   case 7: lisy1_coil_set( Q_SYS1_SOL8, !enable); break;
 }
  
if ( ls80dbg.bitv.coils )
{
switch(ioport)
 {
   case 0: sprintf(debugbuf," 0-0-Outhole set to %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 1: sprintf(debugbuf," 0-1-Knocker set to %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 2: sprintf(debugbuf," 0-2-Tens Chime set to %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 3: sprintf(debugbuf," 0-3-Hundred Chime set to %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 4: sprintf(debugbuf," 0-4-Thousend Chime set to %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 5: sprintf(debugbuf," 0-5-Solenoid 6 %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 6: sprintf(debugbuf," 0-6-Solenoid 7 %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
   case 7: sprintf(debugbuf," 0-7-Solenoid 8 %s",enable ? "OFF" : "ON");
	   lisy80_debug(debugbuf);
           break;
 }
}
  
}

//lamp handler for system1
void lisy1_lamp_handler( int data, int isld)
{

  unsigned char new_lamp[4];
  int offset,i,flip_flop_no;
  static int current_LD = 15;
  static unsigned char Q1_first_time = 1;
  static unsigned char Q2_first_time = 1;

  if(isld) //just store th LD value, wie will set the lamp with the DS
    {
      current_LD = data;
      return;
    }
  else //we have a DS value now
    {
     flip_flop_no = data; 
     //do a range check, also return with data==0
     if (( flip_flop_no < 1 ) ||( flip_flop_no > 9 ))  return;

     //OK, assume we have valid data now, compute which lamps need to be changed (4 max)
     //we have to negate it, becuase of inverters on the Org MPU
     new_lamp[0] =  !CHECK_BIT(current_LD, 0);
     new_lamp[1] =  !CHECK_BIT(current_LD, 1);
     new_lamp[2] =  !CHECK_BIT(current_LD, 2);
     new_lamp[3] =  !CHECK_BIT(current_LD, 3);

     //which of the 9 * 74175 we have adressed? compute the offset
     offset = (flip_flop_no - 1) *4;

    for (i=0; i<=3; i++) {
      if ( lisy1_lamp[ i + offset] != new_lamp[i] ) {
      if ( ls80dbg.bitv.lamps)
       {
           if ( new_lamp[i] ) sprintf(debugbuf,"LISY1_LAMP_HANDLER: Transistor_Q:%d ON",i+1+offset);
             else sprintf(debugbuf,"LISY1_LAMP_HANDLER: Transistor_Q:%d OFF",i+1+offset);
        lisy80_debug(debugbuf);
       }

       //RTH: we may add nvram write later on here, like with system80

       //remember
       lisy1_lamp[i+offset] = new_lamp[i];
       //check special cases RTH: will need to moved ot an event handler
       //Q1==Game Over; Q2==Tilt; Q3==HGTD; Q4==Shoot Again fix for all system1
       switch( i+1+offset) //This is the transistor number
        {
         case 1: if ( Q1_first_time ) { Q1_first_time = 0; break; }
                 if ( new_lamp[i] && lisy1_has_own_sounds ) lisy1_play_wav(4);
	         break;
         case 2: if ( Q2_first_time ) { Q2_first_time = 0; break; }
                 if ( new_lamp[i] && lisy1_has_own_sounds ) lisy1_play_wav(5);
	         break;
        }


       //and set the lamp/coil
       if ( new_lamp[i] ) lisy1_coil_set(i+1+offset, 1); else  lisy1_coil_set(i+1+offset, 0);
    }
   }

 }
}

//watchdog, this one is called every 15 milliseconds or so
//depending on spee of PI
//we do some usefull stuff in here
void lisy1TickleWatchdog( void )
{

//do we need to do a delayed nvram write?
 if ( lisy1_nvram_delayed_write )
 {
    //do it via timer 3; 0,5 secs after buffered write
     if ( lisy_timer( 500, 0, 3))
       {
	//lisy1_nvram_handler( 2, 0, 0);  
	lisy_nvram_write_to_file();
	lisy1_nvram_delayed_write = 0;
       }
 }
 else // reset timer index 3
 {
    lisy_timer( 0, 1, 3);
 }

}

/*
 throttle routine as with sound disabled
 lisy1 runs faster than the org game :-0
*/
void lisy1_throttle(void)
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

 // if we are faster than throttle value which is per default 3000 usec (3 msec)
 //we need to slow down a bit

 //see how many micros passed
 now = micros();
 //beware of 'wrap' which happens each 71 minutes
 if ( now < last) now = last; //we had a wrap

 //calculate difference and sleep a bit
 sleeptime = g_lisy1_throttle_val - ( now - last); 
 if (sleeptime > 0) delayMicroseconds ( sleeptime );

 //store current time for next round with speed limitc
 last = micros();

}

//read dipswitchsettings for specific game/mpu
//and give back settings or -1 in case of error
//switch_nr is 0..2
int lisy1_get_mpudips( int switch_nr )
{

 int ret;
 char dip_file_name[80];


 //use function from fileio.c
 ret = lisy1_file_get_mpudips( switch_nr, ls80dbg.bitv.basic, dip_file_name );
 //debug?
 if ( ls80dbg.bitv.basic )
  {
   sprintf(debugbuf,"LISY1: switch_no:%d ret:%d DIP switch settings according to %s",switch_nr,ret,dip_file_name);
   lisy80_debug(debugbuf);
  }

 //return setting, this will be -1 if 'first_time' failed
 return ret;

}

//shutdown lisy1
void lisy1_shutdown(void)
{
 struct timeval now;
 long seconds_passed;
 int tickles_per_second;

 //see what time is now
 //gettimeofday(&now,(struct timezone *)0);
 //calculate how many seconds passed
 //seconds_passed = now.tv_sec - lisy80_start_t.tv_sec;
 //and calculate how many tickles per second we had
 //tickles_per_second = no_of_tickles / seconds_passed;

 fprintf(stderr,"LISY1 graceful shutdown initiated\n");
 //fprintf(stderr,"run %ld seconds in total\n",seconds_passed);
 //fprintf(stderr,"we had appr. %d tickles per second\n",tickles_per_second);
 //fprintf(stderr,"and %ld throttles\n",no_of_throttles);
 //show the 'shutdown' message
 display_show_shut_message();
 //put all coils and lamps to zero
 lisy1_coil_init();
 lisy80_hwlib_shutdown();
}

//handling of nvram via eeprom
//read_or_write = 0 means read
// 1 means buffered write
// 2 means write to eeprom (usually delayed)
unsigned char lisy1_nvram_handler(int read_or_write, unsigned char ramAddr, unsigned char accu)
{
  //printf("lisy1_nvram_handler %s accu:%d ramAddr:%d\n",read_or_write ? "WRITE" : "READ",accu,ramAddr);

 static unsigned char first_time = 1;
 static eeprom_block_t nvram_block;
 static eeprom_block_t lisy1_block;
 int i,ret;

 //check content at first time
 if (first_time)
 {

  first_time = 0;
  //do we have a valid signature in block 1? -> 1 =yes
  //if not init eeprom note:gane nr is 80 here
  if ( !lisy_eeprom_checksignature())
     {
       ret = lisy_eeprom_init();
 	if ( ls80dbg.bitv.basic )
  	{
        	sprintf(debugbuf,"write nvram INIT done:%d",ret);
        	lisy80_debug(debugbuf);
  	}

     }
  //read lisy80 block
  lisy_eeprom_256byte_read( lisy1_block.byte, 1);

  //if the stored game number is not the current one
  //initialize nvram block with zeros and write to eeprom
  if(lisy1_block.content.gamenr != lisy1_game.gamenr)
   {
    for(i=0;i<=255; i++) nvram_block.byte[i] = '\0';
    ret = lisy_eeprom_256byte_write( nvram_block.byte, 0);
    if ( ls80dbg.bitv.basic )
     {
        sprintf(debugbuf,"stored game nr not current one, we init with zero:%d",ret);
        lisy80_debug(debugbuf);
      }

    //prepare new gane number to write
    lisy1_block.content.gamenr = lisy1_game.gamenr;
   }

   //now update statistics
   lisy1_block.content.starts++;
   if(ls80dbg.bitv.basic) lisy1_block.content.debugs++;
   lisy1_block.content.counts[lisy1_game.gamenr]++;
   lisy1_block.content.Software_Main = LISY_SOFTWARE_MAIN;
   lisy1_block.content.Software_Sub = LISY_SOFTWARE_SUB;
   ret = lisy_eeprom_256byte_write( lisy1_block.byte, 1);
   if ( ls80dbg.bitv.basic )
   {
        sprintf(debugbuf,"nvram statistics updated for game:%d",lisy1_block.content.gamenr);
        lisy80_debug(debugbuf);
   }

   //read the eeprom into the internal buffer (will be used for following reads)
   ret = lisy_eeprom_256byte_read( nvram_block.byte, 0);

 } //first time only

 switch(read_or_write)
   {
	case 0:  //read
	  return nvram_block.byte[ramAddr];
	  break;	  	
	case 1:  //delayed write
	  nvram_block.byte[ramAddr] = accu;
	  lisy1_nvram_delayed_write = 1;
	  return 0;
	  break;	  	
	case 2:  //delayed write
  	  ret = lisy_eeprom_256byte_write( nvram_block.byte, 0);
	  if ( ls80dbg.bitv.basic )
	  {
	          sprintf(debugbuf,"LISY80 write nvram done:%d",ret);
	          lisy80_debug(debugbuf);
	  }
	  return 0;
	  break;	  	
    }
   return 0;
}

