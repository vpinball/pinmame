/*
 LISY_HOME.c
 February 2019
 bontango
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "lisy35.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "lisy_home.h"
#include "coils.h"
#include "switches.h"
#include "utils.h"
#include "eeprom.h"
#include "sound.h"
#include "lisy_home.h"
#include "fadecandy.h"
#include "wheels.h"
#include "externals.h"
#include "lisy.h"


//our pointers to preloaded sounds
Mix_Chunk *lisy_H_sound[32];

#define LISYH_SOUND_PATH "/boot/lisy/lisyH/sounds/"
//init
int lisy_home_init_event(void)
{

 int ret;

//check for fadecandy config -> do the mapping
ret = lisy_file_get_home_mappings();
 if ( ret < 0 )
  {
    fprintf(stderr,"Error: mapping for LISY Home error:%d\n",ret);
    return -1;
  }

//we init sound, may be separate later

/*
RTH new, sound_init done by lisy80 with dip2 == ON

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory

 int i,ret;
 //RTH soundfile names for LISY_HOME are fixed for now
 char lisyH_wav_file_name[6][80]= { "Introducing.wav", "Drain.wav" };
 char wav_file_name[80];



 // Initialize only SDL Audio on default device 
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        return -1;
    }

 //Initialize SDL_mixer with our chosen audio settings
  if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
            printf("Unable to initialize audio: %s\n", Mix_GetError());
            return(-1);
        }

  // allocate only 5 mixing channels now system1
  Mix_AllocateChannels(5);

  // set volume to lisy_volume for all allocated channels
  Mix_Volume(-1, lisy_volume);
  if ( ls80dbg.bitv.sound )
  {
    sprintf(debugbuf,"Info: lisy_volume is %d",lisy_volume);
    lisy80_debug(debugbuf);
  }

 //try to preload all sounds
 ret=-1; //we set it to 0 if at least one wav file could be loaded
 for ( i=1; i<=2; i++)
 {
  sprintf(wav_file_name,"%s/%s",LISYH_SOUND_PATH,lisyH_wav_file_name[i-1]);
  lisy_H_sound[i] = Mix_LoadWAV(wav_file_name);
  if(lisy_H_sound[i] == NULL) {
         fprintf(stderr,"Unable to load WAV file: %s\n", Mix_GetError());
        }
  else {
       ret=0;
       if ( ls80dbg.bitv.sound )
        {
          sprintf(debugbuf,"preload file:%s",lisyH_wav_file_name[i-1]);
          lisy80_debug(debugbuf);
        }
       }
 }

  //RTH fix for now -> Test: play Introducing sound
  ret = Mix_PlayChannel( 1, lisy_H_sound[1], 0);
  if(ret == -1) {
         fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
        }
*/
// return(ret);

//RTH test, play sound 7 after starting
lisy80_play_wav(7);
return ret;
}

//lamp and coils injkl. mapping
void lisy_home_coil_event(int coil, int action)
{

int real_coil;
int mycoil;
int org_is_coil = 0; //is the org deivce a coil?
int map_is_coil = 0; //is the lamp/coil mapped to a coil?
//static unsigned int current_status = LISYH_EXT_CMD_FIRST_SOLBOARD; //RTH we need to add second board
static unsigned int current_status = LISY35_EXT_CMD_AUX_BOARD_1; //RTH we need to add second board

union two {
    unsigned char byte;
    struct {
    unsigned COIL:6, ACTION:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
}data;


 //look for possible mapping
 if (coil <= 48) //org is a lamp
 {
    real_coil = lisy_home_lamp_map[coil].mapped_to_no;
    map_is_coil = lisy_home_lamp_map[coil].mapped_is_coil;
    org_is_coil = 0;
 }
 else //org is coil
 {
  org_is_coil = 1;
  //determine which coil we use
  switch( coil )
   {
    case Q_SOL1: mycoil = 1;
                break;
    case Q_SOL2: mycoil = 2;
                break;
    case Q_SOL3: mycoil = 3;
                break;
    case Q_SOL4: mycoil = 4;
                break;
    case Q_SOL5: mycoil = 5;
                break;
    case Q_SOL6: mycoil = 6;
                break;
    case Q_SOL7: mycoil = 7;
                break;
    case Q_SOL8: mycoil = 8;
                break;
    case Q_SOL9: mycoil = 9;
                break;
    default: mycoil = 0;
                break;
   }
    real_coil = lisy_home_coil_map[mycoil].mapped_to_no;
    map_is_coil = lisy_home_coil_map[mycoil].mapped_is_coil;
  }


 if (!map_is_coil) //the mapped device is a lamp
  {
    //do we talk to the lampdriver currently?
    if (  current_status != LISY35_EXT_CMD_AUX_BOARD_1)
    {  //RTH need to be extended
      //RTH need to be lisyh_coil_select_lamp_driver();
      current_status = LISY35_EXT_CMD_AUX_BOARD_1;
    }
   }
 else //the mapped device is a coil
  {
    //do we talk to the solenoiddriver currently?
    if (  current_status != LISY35_EXT_CMD_AUX_BOARD_1)
    {  //RTH need to be extended
      //RTH need to be lisyh_coil_select_solenoid_driver();
      current_status = LISY35_EXT_CMD_AUX_BOARD_1;
    }
  }


  //debug
  if ( ls80dbg.bitv.lamps | ls80dbg.bitv.coils )
  {
    sprintf(debugbuf,"LISY HOME:  map %s number:%d TO %s number:%d",org_is_coil ? "coil" : "lamp", coil ,map_is_coil ? "coil" : "lamp", real_coil);
    lisy80_debug(debugbuf);
  }


        //now do the setting
        --real_coil; //we have only 6 bit, so we start at zero for coil 1

        // build control byte
        data.bitv.COIL = real_coil;
        data.bitv.ACTION = action;
        data.bitv.IS_CMD = 0;        //this is a coil setting

        //write to PIC
        lisy80_write_byte_coil_pic(  data.byte );

}



//the eventhandler
void lisy_home_event_handler( int id, int arg1, int arg2, char *str)
{

    switch(id)
     {
	case LISY_HOME_EVENT_INIT: lisy_home_init_event(); break;
	//case LISY_HOME_EVENT_SOUND: sprintf(str_event_id,"LISY_HOME_EVENT_SOUND"); break;
	//case LISY_HOME_EVENT_COIL: sprintf(str_event_id,"LISY_HOME_EVENT_COIL"); break;
	//case LISY_HOME_EVENT_SWITCH: sprintf(str_event_id,"LISY_HOME_EVENT_SWITCH"); break;
	case LISY_HOME_EVENT_COIL: lisy_home_coil_event(arg1, arg2); break;
	//case LISY_HOME_EVENT_DISPLAY: sprintf(str_event_id,"LISY_HOME_EVENT_DISPLAY"); break;
     }

}


//do the led setting on starship
//aware of mapping
void lisy_home_ss_lamp_set( int lamp, int action)
{
  int i;

  //debug?
  if (  ls80dbg.bitv.lamps )
  {
     sprintf(debugbuf,"lisy_home_ss_lamp_set: lamp:%d action:%d\n",lamp,action);
     lisy80_debug(debugbuf);
  }//debug

  //we may set other actions because of lamp status
  lisy_home_ss_event_handler( LISY_HOME_SS_EVENT_LAMP, lamp, action);

  //how many mappings?
  for ( i=0; i<lisy_home_ss_lamp_map[lamp].no_of_maps; i++)
  {
   if(lisy_home_ss_lamp_map[lamp].mapped_to_line[i] == 7) //coil mapping
      lisyh_coil_set( lisy_home_ss_lamp_map[lamp].mapped_to_led[i], action);
   else if(lisy_home_ss_lamp_map[lamp].mapped_to_line[i] <= 6) //LED mapping
      lisyh_led_set( lisy_home_ss_lamp_map[lamp].mapped_to_led[i], lisy_home_ss_lamp_map[lamp].mapped_to_line[i], action);
  }
}

//do the led setting on starship
//special lamps
//aware of mapping
void lisy_home_ss_special_lamp_set( int lamp, int action)
{
  int i;

  //debug?
  if (  ls80dbg.bitv.lamps )
  {
     sprintf(debugbuf,"lisy_home_ss_special_lamp_set: lamp:%d action:%d\n",lamp,action);
     lisy80_debug(debugbuf);
  }//debug

  //how many mappings?
  for ( i=0; i<lisy_home_ss_special_lamp_map[lamp].no_of_maps; i++)
  {
   if(lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i] == 7) //coil mapping
      lisyh_coil_set( lisy_home_ss_special_lamp_map[lamp].mapped_to_led[i], action);
   if(lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i] <= 6) //LED mapping
      lisyh_led_set( lisy_home_ss_special_lamp_map[lamp].mapped_to_led[i], lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i], action);
  }
}

//map momentary solenoids to lisy home
void lisy_home_ss_mom_coil_set( unsigned char value)
{
  static unsigned char old_coil_active[15] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  unsigned char current_coil_active[15];
  int i;

  //value is 0..14 for Solenoids 1..15, 15 for all OFF
  memset(current_coil_active, 0, sizeof(current_coil_active));
  if ( value < 15 ) current_coil_active[value] = 1;

  for( i=0; i<=14; i++)
	{
	 if ( old_coil_active[i] != current_coil_active[i])
	  {
	   lisyh_coil_set( lisy_home_ss_coil_map[i+1].mapped_to_coil, current_coil_active[i]);
	   old_coil_active[i] = current_coil_active[i];
	  } 
	}
}

//map continous solenoids to lisy home
//coils 16,17,18,19
void lisy_home_ss_cont_coil_set( unsigned char cont_data)
{
	//PB4 cont.2
   	lisyh_coil_set( lisy_home_ss_coil_map[17].mapped_to_coil, !CHECK_BIT( cont_data, 0));
	//PB5 cont.4 (coin lockout)
   	lisyh_coil_set( lisy_home_ss_coil_map[19].mapped_to_coil, !CHECK_BIT( cont_data, 1));
	//PB6 cont.1 (flipper enable)
   	lisyh_coil_set( lisy_home_ss_coil_map[16].mapped_to_coil, !CHECK_BIT( cont_data, 2));
	//PB7 cont.3
   	lisyh_coil_set( lisy_home_ss_coil_map[18].mapped_to_coil, !CHECK_BIT( cont_data, 3));
}

//send LED colorcodes  mappings to led driver
void lisy_home_ss_send_led_colors( void)
{

 int ledline,led;

 for (ledline=0; ledline <=5; ledline++)
 {
	for (led=0; led <=47; led++)
	{
			lisyh_led_set_LED_color(ledline, led,
			led_rgbw_color[ledline][led].red,
			led_rgbw_color[ledline][led].green,
			led_rgbw_color[ledline][led].blue,
			led_rgbw_color[ledline][led].white);
	}
 }
}

//vars for event handler actions
unsigned char lisy_home_ss_lamp_1canplay_status = 0;
unsigned char lisy_home_ss_lamp_2canplay_status = 0;
unsigned char lisy_home_ss_digit_ballinplay_status = 80;
unsigned char lisy35_flipper_disable_status = 1; //default flipper disbaled
unsigned char want_wheel_score_credits_reset = 0;


void lisy_home_ss_cont_sol_event( unsigned char cont_data )
{
  lisy35_flipper_disable_status =  CHECK_BIT( cont_data, 2);
}

void lisy_home_ss_display_event( int digit, int value)
{
	static int old_ballinplay_status = -1;
	static int old_match_status = -1;
	static int old_credit_status = -1;

	switch(digit)
	{
	 //case LISY_HOME_DIGIT_CREDITS10:
	 //		break;
	 case LISY_HOME_DIGIT_CREDITS:
		if ( old_credit_status < 0 )
		{
		   old_credit_status = value;
		   want_wheel_score_credits_reset = 1;
		}
		break;
	 case LISY_HOME_DIGIT_BALLINPLAY:
		//wheels reset when ballinplay changes from 0 to 1
		lisy_home_ss_digit_ballinplay_status = value;
		if (( lisy_home_ss_digit_ballinplay_status == 1) & ( old_ballinplay_status == 0)) wheel_score_reset();
		//set/unset lamp for ball in play
		if (( lisy_home_ss_digit_ballinplay_status > 0) & ( lisy_home_ss_digit_ballinplay_status <= 5))
			 lisy_home_ss_special_lamp_set ( 9+lisy_home_ss_digit_ballinplay_status, 1); //Lamps 10...14
		if (( old_ballinplay_status > 0)  & ( old_ballinplay_status <= 5 ))
			 lisy_home_ss_special_lamp_set ( 9+old_ballinplay_status, 0); //Lamps 10...14
		//store old value
		old_ballinplay_status = lisy_home_ss_digit_ballinplay_status;
		break;
	 case LISY_HOME_DIGIT_MATCH:
		//set/unset lamp for match
		if (( value >= 0) & ( value <= 9))
			 lisy_home_ss_special_lamp_set ( value, 1); //Lamps 0...9
		if (( old_match_status >= 0) & ( old_match_status <= 9))
			 lisy_home_ss_special_lamp_set ( old_match_status, 0); //Lamps 0...9
		old_match_status = value;
		break;
	}
}

void lisy_home_ss_lamp_event( int lamp, int action)
{
	switch(lamp)
	{
	 case LISY_HOME_SS_LAMP_1CANPLAY: //set light on player1 to ON if 1canplay or 2canplay is ON
		lisy_home_ss_lamp_1canplay_status = action;
		if (( lisy_home_ss_lamp_1canplay_status == 1) | ( lisy_home_ss_lamp_2canplay_status ==1))
		 {
		 lisy_home_ss_special_lamp_set ( 15, 1); 
		 lisy_home_ss_special_lamp_set ( 16, 1); 
		 }
		else
		 {
		 lisy_home_ss_special_lamp_set ( 15, 0); 
		 lisy_home_ss_special_lamp_set ( 16, 0); 
		 }
		break;
	 case LISY_HOME_SS_LAMP_2CANPLAY: 
		 lisy_home_ss_lamp_2canplay_status = action;
		 lisy_home_ss_special_lamp_set ( 17, action); 
		 lisy_home_ss_special_lamp_set ( 18, action); 
		break;
	 case LISY_HOME_SS_LAMP_GAMEOVER: //make sure reset credit wheel does not block solenoid off by waiting for game over
		 if (( action = 1) &( want_wheel_score_credits_reset = 1))
			{
			 want_wheel_score_credits_reset = 0;
			 wheel_score_credits_reset();
			}
		break;
	}
}

void lisy_home_ss_init_event(void)
{
 int i;

 //activate GI lamps for credit, drop targets 3000 and top rollover
 for(i=0; i<=127; i++)
  {
	if ( lisy_home_ss_GI_leds[i].line != 0) 
	{
	 lisyh_led_set( lisy_home_ss_GI_leds[i].led, lisy_home_ss_GI_leds[i].line, 1);
         if ( ls80dbg.bitv.lamps )
          {
          sprintf(debugbuf,"activate GI led:%d line:%d",lisy_home_ss_GI_leds[i].led, lisy_home_ss_GI_leds[i].line);
          lisy80_debug(debugbuf);
          }
	}
  } //for
}


//the Starship eventhandler
void lisy_home_ss_event_handler( int id, int arg1, int arg2)
{
	switch(id)
	{
	 case LISY_HOME_SS_EVENT_INIT: lisy_home_ss_init_event( ); break;
	 case LISY_HOME_SS_EVENT_LAMP: lisy_home_ss_lamp_event( arg1, arg2); break;
	 case LISY_HOME_SS_EVENT_DISPLAY: lisy_home_ss_display_event( arg1, arg2); break;
	 case LISY_HOME_SS_EVENT_CONT_SOL: lisy_home_ss_cont_sol_event( arg1 ); break;
	}

  //2canplay lamp blocks credit switch when ball in play is 1 ( only 2 players on Starship)
  if  (( lisy_home_ss_lamp_2canplay_status == 1)  & ( lisy_home_ss_digit_ballinplay_status == 1))
	lisy_home_ss_ignore_credit = 1; else lisy_home_ss_ignore_credit = 0;
}
