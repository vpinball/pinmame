/*
  lisy80_sound.c
  part of lisy80
  bontango 02.2017
  code mostly taken from http://alsa.opensrc.org/HowTo_Asynchronous_Playback
*/



#include <alsa/asoundlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "fileio.h"
#include "hw_lib.h"
#include "sound.h"
#include "utils.h"

 const char *device_name = "default";
 unsigned int rrate = 44100;
 int channels = 2;
 /* These values are pretty small, might be useful in
   situations where latency is a dirty word. */
 snd_pcm_uframes_t buffer_size = 1024;
 snd_pcm_uframes_t period_size = 64;

//initial value
static int lisy80_sound_stream_status = LISY80_SOUND_STATUS_IDLE;

/* Our device handle */
int err;
snd_pcm_t *pcm_handle = NULL;

//callback function vars
snd_async_handler_t *pcm_callback;


/*
 * sound stream update
 *
 *  done via callback function
 *
 */
void lisy80_sound_callback(snd_async_handler_t *pcm_callback)
{
        snd_pcm_t *pcm_handle = snd_async_handler_get_pcm(pcm_callback);
        snd_pcm_sframes_t avail;
        int err;

        avail = snd_pcm_avail_update(pcm_handle);
        while (avail >= period_size) {
                snd_pcm_writei(pcm_handle, MyBuffer, period_size);
                avail = snd_pcm_avail_update(pcm_handle);
        }
}

/*
 * open sound device and set parameters
 */
int lisy80_sound_stream_init(void)
{
 snd_pcm_hw_params_t *hw_params;
 snd_pcm_sw_params_t *sw_params;

 /* Open the device */
 err = snd_pcm_open (&pcm_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0);

 /* Error check */
 if (err < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n", 
        device_name, snd_strerror (err));
    pcm_handle = NULL;
    return err;
 }

 //Hardware parameters
 snd_pcm_hw_params_malloc (&hw_params);
 snd_pcm_hw_params_any (pcm_handle, hw_params);
 snd_pcm_hw_params_set_access (pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
 snd_pcm_hw_params_set_format (pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
 snd_pcm_hw_params_set_rate_near (pcm_handle, hw_params, &rrate, NULL);
 snd_pcm_hw_params_set_channels (pcm_handle, hw_params, channels);
 snd_pcm_hw_params_set_buffer_size_near (pcm_handle, hw_params, &buffer_size);
 snd_pcm_hw_params_set_period_size_near (pcm_handle, hw_params, &period_size, NULL);
 //set them
 err = snd_pcm_hw_params (pcm_handle, hw_params);
 /* Error check */
 if (err < 0) {
    fprintf (stderr, "cannot set hw parameter for audio device (%s)\n", snd_strerror (err));
    return err;
 }
 //and free mem up
 snd_pcm_hw_params_free (hw_params);

 //software parameters
 snd_pcm_sw_params_malloc (&sw_params);
 snd_pcm_sw_params_current (pcm_handle, sw_params); 
 snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, buffer_size - period_size);
 snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, period_size);
 //set them
 err = snd_pcm_sw_params (pcm_handle, sw_params);
 /* Error check */
 if (err < 0) {
    fprintf (stderr, "cannot set sw parameter for audio device (%s)\n", snd_strerror (err));
    return err;
 }
 //and free mem up
 snd_pcm_sw_params_free (sw_params);

 //So now it's time to actually get asynchronous! 
 snd_async_add_pcm_handler(&pcm_callback, pcm_handle, lisy80_sound_callback, NULL);



/*
 * close sound device
 */
void lisy80_sound_stream_destroy(void)
{
   snd_pcm_close (pcm_handle);
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
    //fflush(lisy80_sound_stream);
    //fclose(lisy80_sound_stream);
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
