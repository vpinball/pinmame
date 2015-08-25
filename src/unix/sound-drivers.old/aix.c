/*
* AIX dependent code
*
* Audio support by Chris Sharp <sharp@hursley.ibm.com>
*
*/

#include "xmame.h"
#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stropts.h>
#include <UMS/UMSAudioDevice.h>
#include <UMS/UMSBAUDDevice.h>

static int channels;
static UMSAudioDeviceMClass audio_device_class;
static UMSAudioDevice_ReturnCode rc;
static UMSBAUDDevice audio_device;

static Environment *ev;
static UMSAudioTypes_Buffer buffer;
static long samples_per_sec, bytes_per_sample;
static long read_cnt, transferred_cnt;
static long samples_read;
static long samples_written;
static long out_rate;
static long left_gain, right_gain;

static long flags;
static UMSAudioDeviceMClass_ErrorCode audio_device_class_error;
static char* error_string;
static char* audio_formats_alias;
static char* audio_inputs_alias;
static char* audio_outputs_alias;
static char* obyte_order;

int sysdep_audio_init(void)
{
	struct itimerval        timer_value;
	int 			i;
	sound_8bit   = TRUE;
	sound_stereo = FALSE;
	
	if (play_sound)
	{
		int supported=FALSE;
		channels = 1;
                fprintf(stderr_file, "AIX sound device initialization...\n");
                /* try to open audio device */
        	ev = somGetGlobalEnvironment();
        	audio_device = UMSBAUDDeviceNew();

        	rc = UMSAudioDevice_open(audio_device,ev,"/dev/paud0","PLAY", UMSAudioDevice_BlockingIO);
        	if (audio_device == NULL)
        	{
        	fprintf(stderr_file,"can't create audio device object\nError: %s\n",error_string);
        	exit(1);
        	}

    		rc = UMSAudioDevice_set_sample_rate(audio_device, ev, options.samplerate, &out_rate);
    		rc = UMSAudioDevice_set_bits_per_sample(audio_device, ev, AUDIO_SAMPLE_BITS);
    		rc = UMSAudioDevice_set_number_of_channels(audio_device, ev, channels);
    		rc = UMSAudioDevice_set_audio_format_type(audio_device, ev, "PCM");
    		rc = UMSAudioDevice_set_number_format(audio_device, ev, "TWOS_COMPLEMENT");
    		rc = UMSAudioDevice_set_volume(audio_device, ev, 100);
    		rc = UMSAudioDevice_set_balance(audio_device, ev, 0);

    		rc = UMSAudioDevice_set_time_format(audio_device,ev,UMSAudioTypes_Bytes);

    		if (obyte_order) free(obyte_order);
    		rc = UMSAudioDevice_set_byte_order(audio_device, ev, "LSB");
    		left_gain = 100;
    		right_gain = 100;

    		rc = UMSAudioDevice_enable_output(audio_device, ev, "LINE_OUT", &left_gain, &right_gain);
    		rc = UMSAudioDevice_initialize(audio_device, ev);
#ifdef USE_TIMER
    		buffer._maximum = AUDIO_BUFF_SIZE;
#else
		buffer._maximum = sysdep_get_audio_freespace();
#endif
    		buffer._buffer  = (char *) malloc(AUDIO_BUFF_SIZE);
    		buffer._length = 0;

		/*
    		bytes_per_sample = (AUDIO_SAMPLE_BITS / 8) * channels;
    		samples_per_sec  = bytes_per_sample * out_rate;
    		buffer._buffer  = (char *) malloc(samples_per_sec);
    		buffer._length = 0;
    		buffer._maximum = samples_per_sec;
    		bbuf_size = buffer._maximum;
		*/
    		rc = UMSAudioDevice_start(audio_device, ev);
        }
        return OSD_OK;
}

void sysdep_audio_close(void) {
	if (play_sound)
	{
          rc = UMSAudioDevice_play_remaining_data(audio_device, ev, TRUE);
          UMSAudioDevice_stop(audio_device, ev);
          UMSAudioDevice_close(audio_device, ev);
          _somFree(audio_device);
          free(buffer._buffer);
	}
}	

int sysdep_audio_play(unsigned char *buf, int bufsize)
{
/* 
 * not sure about if audio buffers in aix are signed or unsigned char.
 * please, tester needed...
 */
	int i;
        buffer._length = bufsize;
#if 1
	memcpy(buffer._buffer,buf,bufsize); /* unsigned buffer */
#else
	for (i=0;i<bufsize;i++) buffer._buffer[i] = buf[i]^0x80; /* signed one */
#endif
        rc = UMSAudioDevice_write(audio_device, ev, &buffer, bufsize, &samples_written);
        return rc;
}

long sysdep_audio_get_freespace() {
	int i;
	rc = UMSAudioDevice_write_buff_remain(audio_device,ev,&i);
	i = i/96; /* ??? */
	if (i<0) return (0);
	/* fprintf(stderr_file,"Audio Buffer remains: %d\n blocks",i); */
	return (long)(i);
}
