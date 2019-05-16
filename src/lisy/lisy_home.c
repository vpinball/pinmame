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
#include "lisyversion.h"


//our pointers to preloaded sounds
Mix_Chunk *lisy_H_sound[32];

#define LISYH_SOUND_PATH "/boot/lisy/lisyH/sounds/"
//init
int lisy_home_init_event(void)
{

//we init sound, may be separate later

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory

 int i,ret;
 //RTH soundfile names for LISY_HOME are fixed for now
 char lisyH_wav_file_name[6][80]= { "Introducing.wav", "Drain.wav" };
 char wav_file_name[80];


 /* Initialize only SDL Audio on default device */
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

 return(ret);
}

//lamp
void lisy_home_lamp_event(int coil, int action)
{

int ret;

if ( coil == 1)
 {
   printf(" -- RTH --- coil 1 action: %d\n",action);
 //Play our (pre loaded) sound file on separate channel
  ret = Mix_PlayChannel( 1, lisy_H_sound[1], 0);
  if(ret == -1) {
         fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
        }
 }
}



//the eventhandler
void lisy_home_event_handler( int id, int arg1, int arg2, char *str)
{

//give this info on screen
  if ( 0 )
  //if ( ls80dbg.bitv.basic )
  {
    char str_event_id[40];
    switch(id)
     {
	case LISY_HOME_EVENT_INIT: sprintf(str_event_id,"LISY_HOME_EVENT_INIT"); break;
	case LISY_HOME_EVENT_SOUND: sprintf(str_event_id,"LISY_HOME_EVENT_SOUND"); break;
	case LISY_HOME_EVENT_SOLENOID: sprintf(str_event_id,"LISY_HOME_EVENT_SOLENOID"); break;
	case LISY_HOME_EVENT_SWITCH: sprintf(str_event_id,"LISY_HOME_EVENT_SWITCH"); break;
	case LISY_HOME_EVENT_LAMP: sprintf(str_event_id,"LISY_HOME_EVENT_LAMP"); break;
	case LISY_HOME_EVENT_DISPLAY: sprintf(str_event_id,"LISY_HOME_EVENT_DISPLAY"); break;
	default: sprintf(str_event_id,"unknown event: %d",id); break;
     }
    sprintf(debugbuf,"LISY HOME Event handler: %s",str_event_id);
    lisy80_debug(debugbuf);
  }

    switch(id)
     {
	case LISY_HOME_EVENT_INIT: lisy_home_init_event(); break;
	//case LISY_HOME_EVENT_SOUND: sprintf(str_event_id,"LISY_HOME_EVENT_SOUND"); break;
	//case LISY_HOME_EVENT_SOLENOID: sprintf(str_event_id,"LISY_HOME_EVENT_SOLENOID"); break;
	//case LISY_HOME_EVENT_SWITCH: sprintf(str_event_id,"LISY_HOME_EVENT_SWITCH"); break;
	case LISY_HOME_EVENT_LAMP: lisy_home_lamp_event(arg1, arg2); break;
	//case LISY_HOME_EVENT_DISPLAY: sprintf(str_event_id,"LISY_HOME_EVENT_DISPLAY"); break;
     }

}
