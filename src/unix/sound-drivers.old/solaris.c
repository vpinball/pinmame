/*
* Solaris dependent code
*
* Preliminary audio support by jantonio@dit.upm.es
* no control on mono/stereo output channel and so  (yet)
*
*Major rewrite by Keith Hargrove condor@sun.com
*       This is my first pass at this
*       I use the EOF counter to keep track of how
*       data has been played
*
*       The whole sound system needs to be rewiten as a thread
*       This would give better performace and perfect sound
*       This code is more of a band-aid
*       I did not add support for the old ss1 and ss2 audio
*       I don't think that a ss1 or ss2 can run MAME
*       unless somone is using an ss2 with the turbo chip??
*       if there is a need it can be added
*
* 990326 merged in Solaris x86 sound. Mathis Rosenhauer
*
*/

#include <sys/stropts.h>
#include <sys/audioio.h>
#include "xmame.h"
#include "sound.h"

static int audioctl_fd;     /* audio control device for solaris     */
static int audio_fd;        /* audio device for solaris             */
static int written = 0;     /* number of bytes written              */
static int sample_size;
static int buffer_size;

static audio_info_t	a_info;   /* info about audio settings            */
static audio_device_t a_dev;  /* info about audio hardware            */

int sysdep_audio_init(void)
{
	int error;

	if (play_sound)
	{
		fprintf(stderr_file, "Solaris sound device initialization...\n");
		/* try to open audio device */
		if ( (audio_fd = open("/dev/audio", O_WRONLY | O_NDELAY)) < 0)
		{
			fprintf(stderr_file, "couldn't open audio device\n");
			return OSD_NOT_OK;
		}
		/* try to open audioctl device */
		if ( (audioctl_fd = open("/dev/audioctl", O_RDWR )) < 0)
		{
			fprintf(stderr_file, "couldn't open audioctl device\n");
			close(audio_fd);
			return OSD_NOT_OK;
		}
		sleep(2);
		
		/* empty buffers before change config */
		ioctl(audio_fd, AUDIO_DRAIN, 0);    /* drain everything out */
		ioctl(audio_fd, I_FLUSH, FLUSHRW);  /* flush everything     */
		ioctl(audioctl_fd, I_FLUSH, FLUSHRW);   /* flush everything */
		
		/* identify audio device. if AMD chipset, bad, luck :-( */
		if(ioctl(audio_fd, AUDIO_GETDEV, &a_dev) < 0)
		{
			fprintf(stderr_file, "Cannot get sound device type\n");
			close(audio_fd);
			close(audioctl_fd);
			return OSD_NOT_OK;
		}

		fprintf(stderr_file, "Sound device is a %s %s version %s\n",a_dev.config,a_dev.name,a_dev.version);
		
		/* get audio parameters. */
		if (ioctl(audioctl_fd, AUDIO_GETINFO, &a_info) < 0)
		{
			fprintf(stderr_file, "AUDIO_GETINFO failed!\nRun with -nosound\n");
			close(audio_fd);
			close(audioctl_fd);
			return OSD_NOT_OK;
		}

		AUDIO_INITINFO(&a_info);
		a_info.play.precision   = sound_8bit? 8: 16;
		a_info.play.channels    = sound_stereo? 2: 1;
		a_info.play.sample_rate = options.samplerate;
		a_info.play.encoding    = AUDIO_ENCODING_LINEAR;
		sample_size = a_info.play.precision / 8;
		a_info.play.buffer_size = buffer_size = sample_size*frag_size*num_frags;

		if ((error = ioctl(audio_fd,AUDIO_SETINFO,&a_info)) < 0 )
		{
			fprintf(stderr_file, "Error %d: audio device parameters not supported\n", error);
			fprintf(stderr_file, "Enter 'man %s' to see parameters supported by your audio device.\n", (strchr(a_dev.name,','))+1);
			sleep(2);
			close(audio_fd);
			close(audioctl_fd);
			return OSD_NOT_OK;
		}
	}
	return OSD_OK;
}

void sysdep_audio_close(void)
{
	if (play_sound)
	{
		close(audio_fd);
		close(audioctl_fd);
	}
}

int sysdep_audio_play(unsigned char *buf, int bufsize)
{
	static unsigned char tmp_buf[512];
	static int  j = 0;
    int i;

	for (i = 0; i < bufsize; i++)
	{
		tmp_buf[j++] = buf[i];
		if(j == 512 )
		{
			write(audio_fd, tmp_buf, j);
			written += j;
			/* write eof */
			write(audio_fd, tmp_buf, 0);
			j = 0;
		}
	}
	return bufsize;
}

long sysdep_audio_get_freespace(void)
{
	ioctl (audioctl_fd, AUDIO_GETINFO, &a_info);
	return ((buffer_size - (written - a_info.play.eof * 512))/sample_size);
}
