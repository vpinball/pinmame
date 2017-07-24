/*
 * NetBSD audio code by entropy@zippy.bernstein.com
 * Audio code is based on the solaris driver, by jantonio@dit.upm.es
 *
 * fixed for 16bit & stereo sound by cpg@aladdin.de, 01-Aug-1999
 * (tested on NetBSD/alpha and NetBSD/i386)
 */

#include "xmame.h"
#include "sound.h"
#include "devices.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/audioio.h>
static int audio_fd;
static char msg_buf[80];
static audio_info_t a_info;
static audio_device_t a_dev;
static int card_stereo; /* card is a stereo card */
static int setfragment(int,int *);
static void setblocksize(int,struct audio_info *);

int sysdep_audio_init(void)
{
  int log_2_frag, frag;
  int corrected_frag_size;

  if (play_sound)
  {
    fprintf(stderr_file, "NetBSD sound device initialization...\n");
    /* try to open audio device */
    if ((audio_fd = open("/dev/audio", O_WRONLY /* | O_NDELAY*/)) < 0)
    {
      perror("Cannot open audio device");
      play_sound = FALSE;
    }
    else
    {
      /* empty buffers before change config */
      ioctl(audio_fd, AUDIO_FLUSH, 0);        /* flush everything */
      
      /* identify audio device. */
      if(ioctl(audio_fd, AUDIO_GETDEV, &a_dev) < 0)
      {
	perror("Cannot get sound device type");
	close(audio_fd);
	play_sound = FALSE;
      }
      else
      {
	fprintf(stderr_file, "Sound device is a %s %s version %s\n",
	       a_dev.config, a_dev.name, a_dev.version);
	    
	/* initialize audio parameters. */ 
	AUDIO_INITINFO(&a_info);
        if (ioctl(audio_fd, AUDIO_GETINFO, &a_info) < 0) {
          fprintf(stderr_file,"cannot query audio device parameters: %s\n",strerror(errno));
        err_ret:
          close(audio_fd);
          play_sound = FALSE;
          return OSD_OK;
        }

        if (a_info.play.channels == 1) {
          a_info.play.channels = 2; /* try to set stereo */
          if (ioctl(audio_fd, AUDIO_SETINFO, &a_info) < 0) {
            a_info.play.channels = 1;
          }
        }

        if (a_info.play.channels == 2) {
          card_stereo = TRUE;
        }
        else {
          card_stereo = FALSE;
        }

        if (!sound_8bit) { /* force from command line */
          AUDIO_INITINFO(&a_info);
          a_info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
          a_info.play.precision = 16;
          sound_8bit = FALSE;
          if (ioctl(audio_fd, AUDIO_SETINFO, &a_info) < 0) {
            sound_8bit = TRUE;
          }
        }
        if (sound_8bit) {
          a_info.play.encoding = (uint) AUDIO_ENCODING_ULINEAR;
          a_info.play.precision = (uint) 8;
        }

	AUDIO_INITINFO(&a_info);
        if (sound_8bit) {
          a_info.play.encoding = (uint) AUDIO_ENCODING_ULINEAR;
          a_info.play.precision = (uint) 8;
        }
        else {
          a_info.play.encoding = (uint) AUDIO_ENCODING_SLINEAR_LE;
          a_info.play.precision = (uint) 16;
        }
        if (sound_stereo && card_stereo) {
          a_info.play.channels = 2;
        }
        else {
          a_info.play.channels = 1;
          sound_stereo = FALSE;
        }
	a_info.play.sample_rate = (uint) options.samplerate;
	a_info.blocksize = options.samplerate / AUDIO_TIMER_FREQ;
	a_info.mode = AUMODE_PLAY | AUMODE_PLAY_ALL;

        if (ioctl(audio_fd, AUDIO_SETINFO, &a_info) < 0) {
          fprintf(stderr_file,"cannot set audio device parameters: %s\n",strerror(errno));
          goto err_ret;
        }

	corrected_frag_size = (options.samplerate * frag_size) / 22050;
	if (!sound_8bit)  corrected_frag_size *= 2;
	if (sound_stereo) corrected_frag_size *= 2;
	for (log_2_frag=0; (1 << log_2_frag) < corrected_frag_size; log_2_frag++) {}
	frag = log_2_frag + (num_frags << 16);

        snprintf(msg_buf,sizeof(msg_buf)-1,"Setting fragsize to %d, numfrags to %d",
                 1 << (frag&0x0000FFFF), frag >> 16);

	if (setfragment(audio_fd,&frag) < 0) {
          fprintf(stderr_file,"%s\nSNDCTL_DSP_SETFRAGMENT: %s\n",msg_buf,strerror(errno));
          goto err_ret;
	}

#ifndef USE_TIMER
        if (ioctl(audio_fd, AUDIO_GETINFO, &a_info) < 0) {
          perror("AUDIO_GETINFO");
          goto err_ret;
        }
        setblocksize(audio_fd, &a_info);
        frag_size = a_info.blocksize;
        num_frags = a_info.play.buffer_size / frag_size;

        /* hmm, tests have shown that num_frags > 6 are no good,
         * so we force them here into this limit...
         */
        if (num_frags > 6) {
          fprintf(stderr_file,"Warning: frags reduced from %d to 6\n",num_frags);
          num_frags = 6;

          frag = log_2_frag + (num_frags << 16);
          snprintf(msg_buf,sizeof(msg_buf)-1,"Setting fragsize to %d, numfrags to %d",
                   1 << (frag&0x0000FFFF), frag >> 16);
          setfragment(audio_fd,&frag);
        }
        /* fprintf(stderr_file,"Fragsize = %d, Numfrags = %d\n", frag_size, num_frags); */
#endif
        fprintf(stderr_file,"%s\n",msg_buf);
	fprintf(stderr_file,"Audio device %s set to %dbit linear %s %dHz\n",
                audiodevice,(sound_8bit)? 8:16,
                (sound_stereo)? "stereo":"mono",
                options.samplerate);
      }
    }
  }
  return OSD_OK;
}

static void setblocksize(int fd, struct audio_info *info) /* from netbsd's ossaudio lib */
{
  struct audio_info set;
  int s;

  if (info->blocksize & (info->blocksize-1)) {
    for(s = 32; s < info->blocksize; s <<= 1)
      ;
    AUDIO_INITINFO(&set);
    set.blocksize = s;
    ioctl(fd, AUDIO_SETINFO, &set);
    ioctl(fd, AUDIO_GETINFO, info);
  }
}


static int setfragment(int fd,int *frag) /* from netbsd's ossaudio lib */
{
  audio_info_t tmpinfo;
  unsigned int u;
  int idat;
  int retval;

  AUDIO_INITINFO(&tmpinfo);
  idat = *frag;
  if ((idat & 0xffff) < 4 || (idat & 0xffff) > 17)
    return (EINVAL);
  tmpinfo.blocksize = 1 << (idat & 0xffff);
  tmpinfo.hiwat = (idat >> 16) & 0x7fff;
  if (tmpinfo.hiwat == 0) /* 0 means set to max */
    tmpinfo.hiwat = 65536;
  (void) ioctl(fd, AUDIO_SETINFO, &tmpinfo);
  retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
  if (retval < 0)
    return (retval);
  u = tmpinfo.blocksize;
  for(idat = 0; u > 1; idat++, u >>= 1)
                        ;
  idat |= (tmpinfo.hiwat & 0x7fff) << 16;
  *frag = idat;
  return(0);
}

void sysdep_audio_close(void)
{
  if (play_sound) close(audio_fd);
}

int sysdep_audio_play(unsigned char *buf, int bufsize)
{
  return write(audio_fd, buf, bufsize);
}

long sysdep_audio_get_freespace()
{
#ifndef USE_TIMER
       audio_info_t info;
       int i;
       long l;
        AUDIO_INITINFO(&info);
        i = ioctl(audio_fd,AUDIO_GETINFO,&info);
       if (i<0) { perror("AUDIO_GETINFO"); return -1; }
       l = AUDIO_BUFF_SIZE - info.play.seek;
       return (l < 0) ? 0 : l;
#else 
       return 0;
#endif   
}
