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
#include "lisy_home.h"
#include "externals.h"


//initial value
//static int lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;

//our pointers to preloaded sounds
Mix_Chunk *lisysound[257];   
Mix_Chunk *lisysound_alt[LISY35_SOUND_MAX_ALTERNATIVE_FILES][257]; // bank for alternative sounds (used by lisy35)


/*
 * open sound device and set parameters LISY80 Version
 */
int lisy80_sound_stream_init(void)
{

  //int audio_rate = 44100;                 //Frequency of audio playback
  int audio_rate = 48000;                 //Frequency of audio playback
  Uint16 audio_format = MIX_DEFAULT_FORMAT;       //Format of the audio we're playing
  int audio_channels = 2;                 //2 channels = stereo
  //int audio_buffers = 2048;               //Size of the audio buffers in memory
  int audio_buffers = 1024;               //bytes used per outpur sample

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

  // allocate 64 possible mixing channels
  Mix_AllocateChannels(64);

    //try to preload all sounds
    for( i=1; i<=63; i++)
     {
       //only for sounds with volume not equal 0
	if(lisy80_sound_stru[i].volume != 0)
        {
          //construct the filename, according to game_nr
          sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,i);
          //and load the file
          lisysound[i] = Mix_LoadWAV(wav_file_name);
          if(lisysound[i] == NULL) {
                fprintf(stderr,"Unable to load WAV file: %s\n", Mix_GetError());
           }
 	   else if ( ls80dbg.bitv.sound )
  	   {
   	     sprintf(debugbuf,"preload file:%s as sound number %d (Volume:%d)",wav_file_name,i,lisy80_sound_stru[i].volume);
   	     lisy80_debug(debugbuf);
  	   }
	//set volume for each channel ( channel == soundnumber )
	Mix_Volume( i, lisy80_sound_stru[i].volume);
      } //volume not zero
     } // for i


 return 0;
}

/*
 * open sound device and set parameters LISY35 Version
 * RTH fix number of mappings at the moment
 */
int lisy35_sound_stream_init(void)
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
  Mix_AllocateChannels(256);

  // set volume to lisy_volume for all allocated channels
  Mix_Volume(-1, lisy_volume);

    //try to preload all sounds
	for( i=1; i<=255; i++)
	{
		if ( lisy35_sound_stru[i].soundnumber != 0)
		{
			//construct the filename, according to options red from csv file
			sprintf(wav_file_name,"/boot/%s/%s.wav",lisy35_sound_stru[i].path,lisy35_sound_stru[i].name);
			lisysound[i] = Mix_LoadWAV(wav_file_name);
			if(lisysound[i] == NULL) 
			{
				fprintf(stderr,"Unable to load WAV file: %s - %s\n",wav_file_name, Mix_GetError());
			}
			else 
			{
				lisy35_sound_stru[i].how_many_versions = 1;
				lisy35_sound_stru[i].last_version_played = 0;
				if ( ls80dbg.bitv.sound )
				{
					sprintf(debugbuf,"preloaded file:%s as sound number %d\n",wav_file_name,i);
					lisy80_debug(debugbuf);
				}
				// see if there are alternative sounds (suffix "-2", "-3", "-4"... at the end of file name, example "sound-3.wav")
				for ( int alt = 2 ; alt <= LISY35_SOUND_MAX_ALTERNATIVE_FILES + 1; alt++)
				{
					sprintf(wav_file_name,"/boot/%s/%s-%d.wav",lisy35_sound_stru[i].path,lisy35_sound_stru[i].name,alt);
					lisysound_alt[alt - 2][i] = Mix_LoadWAV(wav_file_name);
					if(lisysound_alt[alt - 2][i] != NULL) 
					{
						lisy35_sound_stru[i].how_many_versions++;
						if (ls80dbg.bitv.sound)
						{
							sprintf(debugbuf,"found alternative #%d for sound %d\n",alt,i);
							lisy80_debug(debugbuf);
						}
					}
					else
					{
						break;
					}
				}
			}
		}// if soundnumber != 0
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

 int i,ret;
 int loopcount = 0;
 static int extended = 0;

 if ( ls80dbg.bitv.sound )
  {
   sprintf(debugbuf,"lisy80_play_wav: want to play sound number: %d (loop:%d not_int_loops:%d)",sound_no,lisy80_sound_stru[sound_no].loop,lisy80_sound_stru[sound_no].not_int_loops);
   lisy80_debug(debugbuf);
  }

 //does this sound mark an extended sound set?
 if ( lisy80_sound_stru[sound_no].loop == 2 )
 {
   extended = 31; //second soundset has a shift of 31
   if ( ls80dbg.bitv.sound )
    {
     sprintf(debugbuf,"lisy80_play_wav: found extended set: %d",sound_no);
     lisy80_debug(debugbuf);
    }
   return; //nothing else todo
 }

 //sound_number plus extended flag is real soundnumber
 sound_no += extended;

 if ( lisy80_sound_stru[sound_no].not_int_loops == 0 ) //check if this sound can stop loop sounds
  {
   //yes, check running state for all loop sounds and cancel them  if running
   for(i=1; i<=31; i++)
    {
     if (lisy80_sound_stru[i].loop & (Mix_Playing(i) != 0) )
     {
      if ( ls80dbg.bitv.sound ) lisy80_debug("found running loop sound, we cancel it\n");
      Mix_HaltChannel(i);
     }
    }//all loop files
  }

 //just play sound on sepearate channel
   if (  lisy80_sound_stru[sound_no].loop > 0) loopcount = -1; //infinitiv loops
   ret = Mix_PlayChannel( sound_no, lisysound[sound_no], loopcount);
   if(ret == -1) {
         fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
     }

 extended = 0; //set back flag
}

/*
 * LISY35 new sound request
 * RTH minimal implementation yet
 */
void lisy35_play_wav(int sound_no)
{

	int ret;

	if ( ls80dbg.bitv.sound )
	{
		sprintf(debugbuf,"lisy35_play_wav: want to play sound number: %d mapped to sound:%d, for which %d versions are loaded",sound_no,lisy35_sound_stru[sound_no].soundnumber, lisy35_sound_stru[sound_no].how_many_versions);
		lisy80_debug(debugbuf);
	}

	//Play our (pre loaded) sound file on separate channel
	if ( lisy35_sound_stru[sound_no].soundnumber != 0)
	{
		Mix_Chunk *soundtoplay;
		
		// check if the regular sound must be played, or if there is an alternative sound to play
		lisy35_sound_stru[sound_no].last_version_played += 1;
		if (lisy35_sound_stru[sound_no].last_version_played > lisy35_sound_stru[sound_no].how_many_versions)
		{
			// if we're at the end of the alternative sound list, go back to sound 1
			lisy35_sound_stru[sound_no].last_version_played = 1;
		}
		if(lisy35_sound_stru[sound_no].last_version_played == 1)
		{
			// play from the regular bank of sounds
			soundtoplay = lisysound[sound_no];
		}
		else
		{
			// play from the alternative bank of sounds
			soundtoplay = lisysound_alt[lisy35_sound_stru[sound_no].last_version_played - 2][sound_no];
		}

		if ( ls80dbg.bitv.sound )
		{
			sprintf(debugbuf,"sound version %d playing...", lisy35_sound_stru[sound_no].last_version_played);
			lisy80_debug(debugbuf);
		}
		
		// decode option
		// 0=normal play, 1=loop, 2=stop loops, 3=speech

		// is it sound supposed to be played as a loop ?
		int loop = 0;
		if (lisy35_sound_stru[sound_no].option == LISY35_SOUND_OPTION_LOOP) 
		{
			loop = -1;
		}
		
		// is this sound a "loop stopper" ?
		// if yes, check if a sound loop is running and stop it
		if (lisy35_sound_stru[sound_no].option == LISY35_SOUND_OPTION_STOP_LOOP) 
		{
			//yes, check running state for all loop sounds and cancel them if running
			for(int i=1; i<=257; i++)
			{
				if (Mix_Playing(i) != 0 && lisy35_sound_stru[i].soundnumber != 0 && lisy35_sound_stru[i].option == LISY35_SOUND_OPTION_LOOP) 
				{
					if ( ls80dbg.bitv.sound ) lisy80_debug("found running loop, we cancel it\n");
					Mix_HaltChannel(i);
				}
			}
		}

		// is this sound a speech sound ?
		// we don't want speech sounds to go over other speech sounds, so stop any other ongoing speech sound
		if (lisy35_sound_stru[sound_no].option == LISY35_SOUND_OPTION_SPEECH) 
		{
			for(int i=1; i<=257; i++)
			{
				if (Mix_Playing(i) != 0 && lisy35_sound_stru[i].soundnumber != 0 && lisy35_sound_stru[i].option == LISY35_SOUND_OPTION_SPEECH) 
				{			
					Mix_HaltChannel(i);
				}
			}
		}

		// play sound
		ret = Mix_PlayChannel( sound_no, soundtoplay, loop);
		
		if(ret == -1) 
		{
			fprintf(stderr,"Unable to play WAV file: %s\n", Mix_GetError());
		}
	}
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
//give back setting made (in percent)
//for HW_ID 2 set fix to 180 71%  RTH Test
int lisy_adjust_volume(void)
{
  static int first = 1;
  static int old_position;
  int position,diff,sdl_volume,amix_volume;

  //no poti for hardware 3.11
  if ( lisy_hardware_revision == 311) return(0);

  //hardware 3.20 plus hardware_ID == 2 fix setting
  if ( ( lisy_hardware_revision == 320) & ( lisy_hardware_ID == 2) )
  {

     system("/usr/bin/amixer sset PCM 180");

     if ( ls80dbg.bitv.sound)
     {
      lisy80_debug("LISY80 with HW_ID 2: fix volume set to 180(71%)");
     }

    return(180);
  }


  //read position
  position = lisy_get_position();

  //we assume pos is in range 600 ... 5000
  //and translate that to the SDL range of 0..128
  //we use steps of 36
  sdl_volume =  ( position / 36 ) - 10;
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
     //set both PCM AND Digital
     //which is for Hifiberry and Justboom
     sprintf(debugbuf,"/usr/bin/amixer sset PCM %d\%%",amix_volume);
     system(debugbuf);
     sprintf(debugbuf,"/usr/bin/amixer sset Digital %d\%%",amix_volume);
     system(debugbuf);
    // first setting, we announce here the volume setting
    // sprintf(debugbuf,"/bin/echo \"Volume set to %d percent\" | /usr/bin/festival --tts",amix_volume);
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
     amix_volume = (sdl_volume*100) / 128;
     sprintf(debugbuf,"/usr/bin/amixer sset PCM %d\%%",amix_volume);
     system(debugbuf);
     sprintf(debugbuf,"/usr/bin/amixer sset Digital %d\%%",amix_volume);
     system(debugbuf);
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

  return(sdl_volume*100) / 128;

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

