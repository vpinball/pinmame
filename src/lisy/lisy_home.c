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
#include "lisy35.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "switches.h"
#include "utils.h"
#include "eeprom.h"
#include "sound.h"
#include "lisy_home.h"
#include "fadecandy.h"
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
static unsigned int current_status = LISYH_EXT_CMD_FIRST_SOLBOARD; //RTH we need to add second board

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
    if (  current_status != LISYH_EXT_CMD_LED_ROW_1)
    {  //RTH need to be extended
      lisyh_coil_select_lamp_driver();
      current_status = LISYH_EXT_CMD_LED_ROW_1;
    }
   }
 else //the mapped device is a coil
  {
    //do we talk to the solenoiddriver currently?
    if (  current_status != LISYH_EXT_CMD_FIRST_SOLBOARD)
    {  //RTH need to be extended
      lisyh_coil_select_solenoid_driver();
      current_status = LISYH_EXT_CMD_FIRST_SOLBOARD;
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
