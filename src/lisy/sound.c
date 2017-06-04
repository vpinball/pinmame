/*
  lisy80_sound.c
  part of lisy80
  bontango 02.2017
  we use SDL2 mixet library here
*/

#include <SDL2/SDL.h>
#include "SDL2/SDL_mixer.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "fileio.h"
#include "hw_lib.h"
#include "sound.h"
#include "utils.h"
#include "externals.h"


//initial value
static int lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;

//our pointers to preloaded sounds
Mix_Chunk *sound[32];   


/*
 * open sound device and set parameters
 */
int lisy80_sound_stream_init(void)
{

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory

 int ret,i;
 char wav_file_name[80];


 /* Initialize only SDL Audio on default device */
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        return -1;
    }

  //Initialize SDL_mixer with our chosen audio settings
  if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
            printf("Unable to initialize audio: %s\n", Mix_GetError());
            exit(1);
        }

  // allocate 31 mixing channels
  Mix_AllocateChannels(31);

  // set volume to lisy80_volume for all allocated channels
  Mix_Volume(-1, lisy80_volume);

    //try to preload all sounds
    for( i=1; i<=31; i++)
     {
       //there is no sound file 16
       if ( i==16) continue;
       //construct the filename, according to game_nr
       sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,i);
       //put 'loop' fix to zero for now
       sound[i] = Mix_LoadWAV(wav_file_name);
       if(sound[i] == NULL) {
                printf("Unable to load WAV file: %s\n", Mix_GetError());
        }
 	else if ( ls80dbg.bitv.sound )
  	{
   	  sprintf(debugbuf,"preload file:%s as sound number %d",wav_file_name,i);
   	  lisy80_debug(debugbuf);
  	}
     } // for i


 return 0;
}

/*
 * close sound device
 */
void lisy80_sound_stream_destroy(void)
{

   //Need to make sure that SDL_mixer and SDL have a chance to clean up
   Mix_CloseAudio();
   SDL_Quit();

}

/*
 * new sound request
 */
void lisy80_play_wav(int sound_no)
{

 int ret;
 static int last_sound_played = 0;

 if ( ls80dbg.bitv.sound )
  {
   sprintf(debugbuf,"lisy80_play_wav: want to play sound number: %d",sound_no);
   lisy80_debug(debugbuf);
  }

 //if we are in running state, but current sound is protected and still playing ignore request
 if ( lisy80_sound_stream_status == LISY80_SOUND_STATUS_RUNNING_PROTECTED)
  {
    if (Mix_Playing(last_sound_played) != 0)
      {
        if ( ls80dbg.bitv.sound ) lisy80_debug("lisy80_sound: in running PROTECTED state, new request ignored\n");
        return;
      }
  }

 //if we are in running state, check if old sound is still running
 if ( lisy80_sound_stream_status == LISY80_SOUND_STATUS_RUNNING)
  {
    if (Mix_Playing(last_sound_played) != 0)
    {
      if ( ls80dbg.bitv.sound ) lisy80_debug("lisy80_sound: old sound still in running state, we cancel it\n");
      Mix_HaltChannel(last_sound_played);
    }
  }

  //Play our (pre loaded) sound file on separate channel
  ret = Mix_PlayChannel( sound_no, sound[sound_no], 0);
  if(ret == -1) {
         printf("Unable to play WAV file: %s\n", Mix_GetError());
        }

 //update the state  and store last sound played
 if (  lisy80_sound_stru[sound_no].can_be_interrupted ) lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING;
   else lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING_PROTECTED;
 last_sound_played = sound_no;

}
