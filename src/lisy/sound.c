/*
  lisy80_sound.c
  part of lisy80
  bontango 02.2017
  we use SDL2 mixer library here
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "fileio.h"
#include "hw_lib.h"
#include "sound.h"
#include "utils.h"
#include "fadecandy.h"
#include "externals.h"


//initial value
static int lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;

//our pointers to preloaded sounds
Mix_Chunk *lisysound[32];   


/*
 * open sound device and set parameters
 */
int lisy80_sound_stream_init(void)
{

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory

 int i;
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

  // allocate 31 mixing channels
  Mix_AllocateChannels(31);

  // set volume to lisy_volume for all allocated channels
  Mix_Volume(-1, lisy_volume);

    //try to preload all sounds
    for( i=1; i<=31; i++)
     {
       //there is no sound file 16
       if ( i==16) continue;
       //construct the filename, according to game_nr
       sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,i);
       //put 'loop' fix to zero for now
       lisysound[i] = Mix_LoadWAV(wav_file_name);
       if(lisysound[i] == NULL) {
                fprintf(stderr,"Unable to load WAV file: %s\n", Mix_GetError());
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
 * open sound device and set parameters
 */
int lisy1_sound_stream_init(void)
{

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory

 int i,ret;
 //RTH soundfile names for LISY1 are fixed for now
 char lisy1_wav_file_name[6][80]= { "10.wav", "100.wav", "1000.wav", "gameover.wav", "tilt.wav" };
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

  // allocate only 5 mixing channels for system1
  Mix_AllocateChannels(5);

  // set volume to lisy_volume for all allocated channels
  Mix_Volume(-1, lisy_volume);

 //try to preload all sounds
 ret=-1; //we set it to 0 if at least one wav file could be loaded 
 for ( i=1; i<=5; i++)
 {
  sprintf(wav_file_name,"%s%03d/%s",LISY1_SOUND_PATH,lisy1_game.gamenr,lisy1_wav_file_name[i-1]);
  lisysound[i] = Mix_LoadWAV(wav_file_name);
  if(lisysound[i] == NULL) {
         fprintf(stderr,"Unable to load WAV file: %s\n", Mix_GetError());
        }
  else {
       ret=0;
       if ( ls80dbg.bitv.sound )
  	{
   	  sprintf(debugbuf,"preload file:%s",lisy1_wav_file_name[i-1]);
   	  lisy80_debug(debugbuf);
  	}
       }
 }

 return(ret);
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
 * LISY80 new sound request
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
  ret = Mix_PlayChannel( sound_no, lisysound[sound_no], 0);
  if(ret == -1) {
         fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
        }

 //update the state  and store last sound played
 if (  lisy80_sound_stru[sound_no].can_be_interrupted ) lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING;
   else lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING_PROTECTED;
 last_sound_played = sound_no;

}

/*
 * LISY1 new sound request
 */
void lisy1_play_wav(int sound_no)
{

 int ret;
 static int last_sound_played = 0;

 if ( ls80dbg.bitv.sound )
  {
   sprintf(debugbuf,"lisy1_play_wav: want to play sound number: %d",sound_no);
   lisy80_debug(debugbuf);
  }

 // no protection in lisy1 yet, each sound will be canecelled by next on
 //check if old sound is still running
 /*
    if (Mix_Playing(last_sound_played) != 0)
    {
      if ( ls80dbg.bitv.sound ) lisy80_debug("lisy1_sound: old sound still playing, we cancel it\n");
      Mix_HaltChannel(last_sound_played);
    }
 */

  //RTH test: lets play new sound on separate channel in parallel

  //Play our (pre loaded) sound file on separate channel
  ret = Mix_PlayChannel( sound_no, lisysound[sound_no], 0);
  if(ret == -1) {
         fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
        }

 //store last sound played
 last_sound_played = sound_no;

}



//
//sound handling
//


//get postion of poti and give it back
//RTH: number of times to read to adjust
int lisy_get_position(void)
{
  
  int i,pos;
  long poti_val;

  //first time fake read
  poti_val = lisy80_get_poti_val();
  poti_val = 0;

  //read 100 times
  for(i=1; i<=10; i++) poti_val = poti_val + lisy80_get_poti_val();
 
  //divide result
  pos = poti_val / 10;

  if ( ls80dbg.bitv.sound)
    {
     sprintf(debugbuf,"get_position called, it reports: %d\n",pos);
     lisy80_debug(debugbuf);
    }

  return( pos );
}



//set new volume in case postion of poti have changed
int lisy_adjust_volume(void)
{
  static int first = 1;
  static int old_position;
  int position,diff,sdl_volume,amix_volume;

  //no poti for hardware 3.11
  if ( lisy_hardware_revision == 311) return(0);


  //read position
  position = lisy_get_position();

  //we assume pos is in range 600 ... 7500
  //and translate that to the SDL range of 0..128
  //we use steps of 54
  sdl_volume =  ( position / 54 ) - 10;
  if ( sdl_volume > 128 ) sdl_volume = 128; //limit

  if ( first)  //first time called, set volume
  {
    first = 0;
    old_position = position;
    if ( ls80dbg.bitv.sound)
    {
     sprintf(debugbuf,"Volume first setting: position of poti is:%d",position);
     lisy80_debug(debugbuf);
    }
    lisy_volume = sdl_volume; //set global var for SDL
 
    // first setting, we do it with amixer for now; range here is 0..100
     amix_volume = (sdl_volume*100) / 128;
     sprintf(debugbuf,"/usr/bin/amixer sset Digital %d\%%",amix_volume);
     system(debugbuf);
    // first setting, we announce here the volume setting
     sprintf(debugbuf,"/bin/echo \"Volume set to %d percent\" | /usr/bin/festival --tts",amix_volume);
     system(debugbuf);
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"first Volume setting initiated via amixer: %d percent",amix_volume);
      lisy80_debug(debugbuf);
     }
  }
  else
  {

    //is there a significantge change? otherwise it is just because linux is not soo accurate
    if( (diff = abs(old_position - position )) > 300)
    {
     lisy_volume = sdl_volume; //set global var for SDL
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"new Volume setting initiated: %d",sdl_volume);
      lisy80_debug(debugbuf);
     }
    }
    else
    {
     if ( ls80dbg.bitv.sound)
     {
      sprintf(debugbuf,"new Volume setting initiated, but no significant change:%d",diff);
      lisy80_debug(debugbuf);
     }
    }

    old_position = position;

  }

  return(1);

}



/*
 * open sound device,
 * set parameters
 *
 * return 0 on success
 */
int mpf_sound_stream_init(int debug)
{

  int audio_rate = 44100;                 //Frequency of audio playback
  Uint16 audio_format = AUDIO_S16SYS;     //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  int audio_buffers = 2048;               //Size of the audio buffers in memory
  int flags = MIX_INIT_MP3;		  //we expect mp3 files here

  int result;


 /* Initialize only SDL Audio on default device */
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        return 1;
    }

    if (flags != (result = Mix_Init(flags))) {
        printf("Could not initialize mixer (result: %d).\n", result);
        printf("Mix_Init: %s\n", Mix_GetError());
        return(2);
    }


  //Initialize SDL_mixer with our chosen audio settings
  if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
            printf("Unable to initialize audio: %s\n", Mix_GetError());
            return(3);
        }

  // set volume to 50% at the moment, need to check potentiometer RTH
  //Mix_VolumeMusic(MIX_MAX_VOLUME/2);
  Mix_VolumeMusic(10);

 return 0;
}

/*
 * close sound device
 */
/*
void lisy80_sound_stream_destroy(void)
{

   //Need to make sure that SDL_mixer and SDL have a chance to clean up
   Mix_CloseAudio();
   SDL_Quit();

    Mix_PlayMusic(music, 1);
    Mix_FreeMusic(music);
    SDL_Quit();
}

 */
 
// new sound request
int mpf_play_mp3(Mix_Music *music)
{

 int ret;
 //static int last_sound_played = 0;


 ret = ( Mix_PlayMusic(music, 1));
 if ( ret == -1) printf("Mix_PlayMusic: %s\n", Mix_GetError());
 return ret;

}

  /*
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

*/
