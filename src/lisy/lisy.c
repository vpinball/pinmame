/*
 LISY.c
 October 2019
 bontango
*/

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "xmame.h"
#include "lisy1.h"
#include "lisy35.h"
#include "lisy80.h"
#include "lisy_w.h"
#include "lisy_home.h"
#include "utils.h"
#include "hw_lib.h"
#include "fadecandy.h"
#include "lisyversion.h"
#include "lisy.h"

//global strcut, the lisy environment
t_stru_lisy_env lisy_env;

//global var for handling different hardware revisions
//set in lisy_hwlib_init
int lisy_hardware_revision;
int lisy_hardware_ID; //only for Software PIC >= 4
unsigned char lisy_K3_value = 3;  //default no jumpers

//global var for handling different eeprom handling
//set in lisy_hwlib_init
int lisy_has24c04 = 1;

//global var for fadecandy routines
//set in lisy_hwlib_init
int lisy_has_fadecandy = 0;
//flag if lamp is mapped to LED, set in fileio
t_lisy_lamp_to_led_map lisy_lamp_to_led_map[52];
//table for coil lamp mapping fileio
t_lisy_home_coil_map lisy_home_coil_map[10];
t_lisy_home_lamp_map lisy_home_lamp_map[49];

//global var for debugging
//typedef is defined in hw_lib.h
ls80dbg_t ls80dbg;

//global var for additional options
//typedef is defined in hw_lib.h
ls80opt_t ls80opt;

//handle time_to quit from usrintf.c
int lisy_time_to_quit_flag = 0;

//global var for timing
struct timeval lisy_start_t;

//do gracefull shutdown
extern void lisy1_shutdown();
extern void lisy80_shutdown();
extern void lisy35_shutdown();
extern void lisy_mini_shutdown();
void lisy_shutdown(void)
{
 if ( lisy_hardware_revision == 100 )
    lisy1_shutdown( );
 else if ( lisy_hardware_revision == 350 )
    lisy35_shutdown( );
 else if ( lisy_hardware_revision == LISY_HW_LISY_W )
    lisy_mini_shutdown( );
 else
    lisy80_shutdown( );
}

//init the Hardware
//lisy_variant decides on HW Init
//valid variants:
//- lisy1 -> lisy_variant: 2 - Gottlieb System1
//- lisy35 -> lisy_variant: 3 - Bally
//- lisy80 -> lisy_variant: 1 - Gottlieb System80 & LISY Home Games based on System80
//- lisy_m -> lisy_variant: 4 - LISY Mini e.g. Williams System 3 to System11 via APC
//- lisy_apc -> lisy_variant: 5 - LISY APC based on LISY Mini
void lisy_hw_init(int lisy_variant)
{

unsigned char dum;

//init LISY environment struct
  strcpy(lisy_env.variant,"unknown");  // string LISY variant 1,80,35,williams,mini,atari ...
  lisy_env.selection = 0; //game selection, usally via S2
  strcpy(lisy_env.gamename,"unknown");  // game name mame fromat (8 chars)
  strcpy(lisy_env.long_name,"unknown");  // game name from csv
  lisy_env.throttle = 0;  // throttle value
  lisy_env.clockscale = 0;  // clockscale value
  lisy_env.disp_sw_ver = 0; //Display PIC Software version
  lisy_env.coil_sw_ver = 0; //Coil PIC Software version
  lisy_env.switch_sw_ver = 0; //Coil PIC Software version
  strcpy(lisy_env.gitversion,GITVERSION);    //LISY Softwareversion git format
  lisy_env.has_soundcard = 0; //do we have a soundcard?
  lisy_env.has_own_sounds = 0;  //do we want to play pinmame sounds?


//store start time in global var
gettimeofday(&lisy_start_t,(struct timezone *)0);

//init th wiringPI library first
lisy80_hwlib_wiringPI_init();

 if ( lisy80_dip1_debug_option() )
 {
  fprintf(stderr,"LISY basic DEBUG activ\n");
  ls80dbg.bitv.basic = 1;
  lisy80_debug("LISY DEBUG timer set"); //first message sets print timer to zero
  //do int the udp switch reader server
  if  (lisy_udp_switch_reader( &dum, 1  ) < 0)
    lisy80_debug("Info: could not start udp switch reader server for debug mode");
  else
    lisy80_debug("Info: udp switch reader server for debug mode succesfully started");

 }
 else ls80dbg.bitv.basic = 0;

//do init the hardware
//this also sets the debug options by reading jumpers via switch pic
if( lisy_variant == 4)
 { 
   //for williams we use special board, minilisy only
   lisymini_hwlib_init();
 }
else if( lisy_variant == 5)
 { 
   //for williams we use special board, minilisy only
   lisyapc_hwlib_init();
 }
else
 {
   lisy_hwlib_init();
 }

   //now look for the other dips and for extended debug options
   lisy80_get_dips();

}


//identify gamename we need to emulate
int lisy_set_gamename( char *arg_from_main, char *lisy_gamename)
{
  int res;

	//we support LISY80, LISY1 and LISY35 at the moment
        //LISY80
        //get the gamename from DIP Switch on LISY80 in case gamename (last arg) is 'lisy80'
        if ( strcmp(arg_from_main,"lisy80") == 0)
        {
            //do init of LISY80 hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy_hw_init(1); //Variant 1
                if ( (res=lisy80_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(arg_from_main,lisy_gamename);
                   fprintf(stderr,"LISY80: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
		{
                   fprintf(stderr,"LISY80: no matching game or other error\n\r");
		   return (-1);
		}
        }

	//LISY1
        //get the gamename from DIP Switch on LISY1 in case gamename (last arg) is 'lisy1'
        else if ( strcmp(arg_from_main,"lisy1") == 0)
        {
            //do init of LISY1 hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy_hw_init(2); //Variant 2
                if ( (res=lisy1_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(arg_from_main,lisy_gamename);
                   fprintf(stderr,"LISY1: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
		{
                   fprintf(stderr,"LISY1: no matching game or other error\n\r");
		   return (-1);
		}
        }

	//LISY35
        //get the gamename from DIP Switch on LISY35 in case gamename (last arg) is 'lisy35'
        else if ( strcmp(arg_from_main,"lisy35") == 0)
        {
            //do init of LISY35 hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy_hw_init(3); //Variant 3
                if ( (res=lisy35_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(arg_from_main,lisy_gamename);
                   fprintf(stderr,"LISY35: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
		{
                   fprintf(stderr,"LISY35: no matching game or other error\n\r");
		   return (-1);
		}
	    //for LISY35 we need to initialize the variant ( PIC in/out config )
            lisy35_set_variant();
        }

	//LISY_Mini special board for e.g. Williams via APC
        //get the gamename from DIP Switch on LISY_mini in case gamename (last arg) is 'lisy_m'
        else if ( strcmp(arg_from_main,"lisy_m") == 0)
        {
            //do init of LISY_Mini hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy_hw_init(4); //Variant 4
                if ( (res=lisymini_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(arg_from_main,lisy_gamename);
                   fprintf(stderr,"LISYMINI: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
                   fprintf(stderr,"LISYMINI: no matching game or other error\n\r");
        }

 	//LISY APC based on LISY_Mini 
        //get the gamename from DIP Switch on LISY_mini in case gamename (last arg) is 'lisy_apc'
        else if ( strcmp(arg_from_main,"lisy_apc") == 0)
        {
            //do init of LISY_Mini hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy_hw_init(5); //Variant 5
                if ( (res=lisyapc_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(arg_from_main,lisy_gamename);
                   fprintf(stderr,"LISY_APC: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
                   fprintf(stderr,"LISY_APC: no matching game or other error\n\r");
        }


	//no match so far, return 1, lets decide main what to do
	else
        {
            fprintf(stderr,"LISY: found LISY support compiled in, but not activated yet \n\r");
            fprintf(stderr,"LISY: argument was: >%s<\n",arg_from_main);
	    return (1);
        }

 return(0);
}

//get lisyversion ( from GITVERSION in lisyversion.h )
void lisy_get_sw_version( unsigned char *sw_main, unsigned char *sw_sub, unsigned char *commit)
{

 char gitversion[40] = GITVERSION;

 //gitversion has format:  "5.20-1-g6f15813"
 *sw_main = atoi(strtok(gitversion, "."));
 *sw_sub = atoi(strtok(NULL, "-"));
 *commit = atoi(strtok(NULL, "-"));


}

//this give back lisy_time_to_quit to cpuexec
//set by signalhandler
int lisy_time_to_quit(void)
{
  return lisy_time_to_quit_flag;
}


//sound handler
void lisy_sound_handler( unsigned char board, unsigned char data )
{

//first tests
if ( lisy_hardware_revision == LISY_HW_LISY_W)
  lisy_w_sound_handler(board, data);
}
      
