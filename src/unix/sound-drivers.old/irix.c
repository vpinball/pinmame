/*
 * IRIX sound code
 * 
 * Addition of new al stuff by Tristram Scott 23/9/98
 * 
 */
#include "xmame.h"
#include "sound.h"
#include <sys/stropts.h>
#include <dmedia/audio.h>
#include <errno.h>

static ALport   devAudio;
static ALconfig devAudioConfig;
	
int sysdep_audio_init(void) {
#ifdef IRIX_HAVE_al
	ALpv pvs[4];
	int alIsBusy = 0;
#else
	long parambuf[4];
#endif
	sound_8bit	= TRUE;
	sound_stereo	= FALSE;
	
	if (play_sound) {

	    fprintf(stderr_file, "IRIX sound system initializing ...");

#ifdef IRIX_HAVE_al
	    /* first of all, look for anyone using driver */
	    pvs[0].param = AL_MAX_PORTS;
	    pvs[1].param = AL_UNUSED_PORTS;
	    if( alGetParams( AL_SYSTEM,pvs,2) < 0) {
		fprintf(stderr_file, "alGetParams failed: %s\n", alGetErrorString(oserror()));
		exit(1);
	    }
	    fprintf(stderr_file, "Maximum audio port count is %d, current unused count is %d\n",
	    	pvs[0].value.i,pvs[1].value.i);
	    if( pvs[1].value.i < 1) {
	        fprintf(stderr_file, "No unused audio ports, try using -nosound option\n");
		exit(1); 
	    }
	    if( pvs[1].value.i < pvs[0].value.i) {
	        fprintf(stderr_file, "Looks like someone else is using the audio device already.\n");
		pvs[0].param = AL_RATE;
		if( alGetParams( AL_DEFAULT_OUTPUT,pvs,1) < 0) {
		    fprintf(stderr_file, "alGetParams failed: %s\n", alGetErrorString(oserror()));
		    exit(1);
		}
		fprintf(stderr_file, "Current sample rate is %d, we will use that.\n", 
		    pvs[0].value.i);
		options.samplerate = pvs[0].value.i;
		alIsBusy = 1;
	    }

	    /* try to get a configuration descriptor */
	    if ( ( devAudioConfig=alNewConfig() ) == (ALconfig) NULL ){
		fprintf(stderr_file, "Cannot get a config Descriptor.Exiting\n");
		exit(1);
	    }

	    /* channel-specific parameters */	
	    alSetChannels(devAudioConfig,AL_MONO); 		/* mono */
	    alSetQueueSize(devAudioConfig,(int)AUDIO_BUFF_SIZE); /* buffer size */
	    alSetSampFmt(devAudioConfig,AL_SAMPFMT_TWOSCOMP);	/* linear signed */
	    alSetWidth(devAudioConfig,AL_SAMPLE_8);   	/* 8 bits */

	    /* global audio-device parameters */

	    pvs[0].param = AL_MASTER_CLOCK;
	    pvs[0].value.i = AL_CRYSTAL_MCLK_TYPE;
	    pvs[1].param = AL_RATE;
	    pvs[1].value.i = options.samplerate; 
 
	    if( !alIsBusy) {
		if( alSetParams( AL_DEFAULT_OUTPUT,pvs,2) < 0) {
		    fprintf(stderr_file, "Cannot Configure the sound system.Exiting...\n");
		    exit(1);
		}
	    }

	    /* try to get a Audio channel descriptor with pre-calculated params */
	    if ( ( devAudio=alOpenPort("audio_fd","w",devAudioConfig) ) == (ALport) NULL ){
		fprintf(stderr_file, "Cannot get an Audio Channel Descriptor.Exiting\n");
		exit(1);
	    }
#else
	    /* first of all, look for anyone using driver */
	    parambuf[0] = AL_INPUT_COUNT;
	    parambuf[2] = AL_OUTPUT_COUNT;
	    ALgetparams( AL_DEFAULT_DEVICE,parambuf,4);
	    if (parambuf[1] || parambuf[3]) {
		fprintf(stderr_file, "Someone is already using the sound system.Exiting..\n");
		exit(1);
	    }

	    /* try to get a configuration descriptor */
	    if ( ( devAudioConfig=ALnewconfig() ) == (ALconfig) NULL ){
		fprintf(stderr_file, "Cannot get a config Descriptor.Exiting\n");
		exit(1);
	    }

	    /* channel-specific parameters */	
	    ALsetchannels(devAudioConfig,AL_MONO); 		/* mono */
	    ALsetqueuesize(devAudioConfig,(long)AUDIO_BUFF_SIZE); /* buffer size */
	    ALsetsampfmt(devAudioConfig,AL_SAMPFMT_TWOSCOMP);	/* linear signed */
	    ALsetwidth(devAudioConfig,AL_SAMPLE_8);   	/* 8 bits */

	    /* global audio-device parameters */
	    parambuf[0]	= AL_OUTPUT_RATE;	parambuf[1] =	options.samplerate;
	    parambuf[2]	= AL_INPUT_RATE;	parambuf[3] =	options.samplerate;
	    if( ALsetparams( AL_DEFAULT_DEVICE,parambuf,4) < 0) {
		fprintf(stderr_file, "Cannot Configure the sound system.Exiting...\n");
		exit(1);
	    }

	    /* try to get a Audio channel descriptor with pre-calculated params */
	    if ( ( devAudio=ALopenport("audio_fd","w",devAudioConfig) ) == (ALport) NULL ){
		fprintf(stderr_file, "Cannot get an Audio Channel Descriptor.Exiting\n");
		exit(1);
	    }

#endif 
	} /* if (play_sound) */
	return OSD_OK;
}

void sysdep_audio_close(void) {
#ifdef IRIX_HAVE_al
	if(play_sound){
		alClosePort(devAudio);
		alFreeConfig(devAudioConfig);
	}
#else
	if(play_sound){
		ALcloseport(devAudio);
		ALfreeconfig(devAudioConfig);
	}
#endif
}	

long sysdep_audio_get_freespace() {
#ifdef IRIX_HAVE_al
	return alGetFillable(devAudio);
#else
	return ALgetfillable(devAudio);
#endif
}

/* routine to dump audio samples into audio device */
int sysdep_audio_play(unsigned char *buff, int size) {
#ifdef IRIX_HAVE_al
	unsigned char *pt=buff;
	/* try only to send samples that no cause blocking */
        long maxsize=alGetFillable(devAudio);
	/* new mixer code works in a unsigned char; driver is signed.... */
	for (;pt<(buff+size);pt++) *pt ^=0x80;
	return alWriteFrames(devAudio,(void *)buff,(size<=maxsize)?size:maxsize);	
#else
	unsigned char *pt=buff;
	/* try only to send samples that no cause blocking */
        long maxsize=ALgetfillable(devAudio);
	/* new mixer code works in a unsigned char; driver is signed.... */
	for (;pt<(buff+size);pt++) *pt ^=0x80;
	return ALwritesamps(devAudio,(void *)buff,(size<=maxsize)?size:maxsize);	
#endif
}
