/*
  lisy80_sound.c
  part of lisy80
  bontango 02.2017
  code partly taken from Alessandro
*/

/*
 * Copyright (C) 2009 Alessandro Ghedini <alessandro@ghedini.me>
 * --------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Alessandro Ghedini wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we
 * meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * --------------------------------------------------------------
 */


#include <alsa/asoundlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "fileio.h"
#include "hw_lib.h"
#include "sound.h"
#include "utils.h"


//initial value
static int lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;

//local vars
static FILE *lisy80_sound_stream;
static char *lisy80_sound_buff;
static int lisy80_buff_size;
static snd_pcm_t *lisy80_pcm_handle;
static snd_pcm_uframes_t lisy80_frames;

/*
 * open sound device and set parameters
 */
int lisy80_sound_stream_init(void)
{
 unsigned int pcm, rate, tmp;
 int channels;
 snd_pcm_hw_params_t *params;

 //code from play.c, modified a little bit by bontango ;-)
  rate     = 44100; //fixed rate for now
  channels = 2; //we use stereo, even we need mono

 /* Open the PCM device in playback mode */
 if ((pcm = snd_pcm_open(&lisy80_pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
        {
           printf("ERROR: Can't open \"%s\" PCM device. %s\n", PCM_DEVICE, snd_strerror(pcm));
	   return -1;
         }
 /* Allocate parameters object and fill it with default values*/
 snd_pcm_hw_params_alloca(&params);
 snd_pcm_hw_params_any(lisy80_pcm_handle, params);

 /* Set parameters */
 if ((pcm = snd_pcm_hw_params_set_access(lisy80_pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        {
                printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));
	   return -1;
         }

 if ((pcm = snd_pcm_hw_params_set_format(lisy80_pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
        {
                printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));
	   return -1;
         }

 if ((pcm = snd_pcm_hw_params_set_channels(lisy80_pcm_handle, params, channels)) < 0)
        {
                printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));
	   return -1;
         }

 if ((pcm = snd_pcm_hw_params_set_rate_near(lisy80_pcm_handle, params, &rate, 0)) < 0)
        {
                printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));
	   return -1;
         }

 /* Write parameters */
 if ((pcm = snd_pcm_hw_params(lisy80_pcm_handle, params)) < 0)
        {
                printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));
	   return -1;
         }

 //get no of channels and set rate
 snd_pcm_hw_params_get_channels(params, &tmp);

 snd_pcm_hw_params_get_rate(params, &tmp, 0);

 /* Allocate buffer to hold single period */
        snd_pcm_hw_params_get_period_size(params, &lisy80_frames, 0);

        lisy80_buff_size = lisy80_frames * channels * 2 /* 2 -> sample size */;
        lisy80_sound_buff = (char *) malloc(lisy80_buff_size);

        snd_pcm_hw_params_get_period_time(params, &tmp, NULL);
 
 return 0;
}

/*
 * close sound device
 */
void lisy80_sound_stream_destroy(void)
{
        snd_pcm_drain(lisy80_pcm_handle);
        snd_pcm_close(lisy80_pcm_handle);
        free(lisy80_sound_buff);
}

/*
 * sound stream update
 *
 * this is regulary called by lisy80
 *
 */
void lisy80_sound_stream_update(void)
{
 unsigned int pcm;
 snd_pcm_uframes_t avail;

 //if we are idle, there is nothing to do
 if ( lisy80_sound_stream_status == LISY80_SOUND_STATUS_IDLE) return;

 //check if we can send the frames
 avail = snd_pcm_avail_update(lisy80_pcm_handle);

 if ( avail >= lisy80_frames )
 {
  //now read file and send one soundbuffer to soundlib if available
  if( ( fread( lisy80_sound_buff, sizeof(char), lisy80_buff_size,  lisy80_sound_stream)) > 0 )
  {
    if ((pcm = snd_pcm_writei(lisy80_pcm_handle, lisy80_sound_buff, lisy80_frames)) == -EPIPE) {
                        printf("XRUN.\n");
                        snd_pcm_prepare(lisy80_pcm_handle);
                } else if (pcm < 0) {
                        printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
                }
  }
  else //close stream and set state to idle
  {
   lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;
   fclose(lisy80_sound_stream);
  }
 }//if avail
}



/*
 * new sound request
 */
void lisy80_play_wav(int sound_no)
{

 char wav_file_name[80];
 char wav_file_header[44];

 if ( ls80dbg.bitv.sound )
  {
   sprintf(debugbuf,"lisy80_play_wav: want to play sound number: %d",sound_no);
   lisy80_debug(debugbuf);
  }

 //if we are in running state, but current sound is protected ignore request
 if ( lisy80_sound_stream_status == LISY80_SOUND_STATUS_RUNNING_PROTECTED)
  {
    if ( ls80dbg.bitv.sound ) lisy80_debug("lisy80_sound: in running PROTECTED state, new request ignored\n");
    return;
  }

 //if we are in running state, there is still a sound file open
 if ( lisy80_sound_stream_status == LISY80_SOUND_STATUS_RUNNING)
  {
    if ( ls80dbg.bitv.sound ) lisy80_debug("lisy80_sound: still in running state, current stream cancelled\n");
    fflush(lisy80_sound_stream);
    fclose(lisy80_sound_stream);
  }


 //construct the filename, according to game_nr
 sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,sound_no);

 //try to open the file
 if ( ( lisy80_sound_stream = fopen(wav_file_name,"r")) == NULL )
        {
           printf("lisy80_sound: Can't open \"%s\"\n", wav_file_name);
	   return;
         }

 //RTH: ugly: we assume 44 bytes of wav header, and discard them
 fread( wav_file_header, sizeof(char), 44,  lisy80_sound_stream);

 //update the state 
 if (  lisy80_sound_stru[sound_no].can_be_interrupted ) lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING;
   else lisy80_sound_stream_status = LISY80_SOUND_STATUS_RUNNING_PROTECTED;


}
