/*
 * SoundKit support for xmame. This code is held here rather than in the
 * OpenStep display file because it is not strictly OpenStep code and thus
 * is only really applicable to NeXT/Apple operating systems. Not that I really
 * expect anyone to try and run it elsewhere, but you should try and do these
 * things right after all !
 *
 * -bat. 11/1/1999
 */

#import <SoundKit/SoundKit.h>
#import <libc.h>
#import "xmame.h"
#import "sound.h"
#import "osdepend.h"
#import "driver.h"

/*
 * Variables used by the sound code.
 */

static NXSoundOut *sound = nil;
static NXPlayStream *stream = nil;
static NSLock *queue_lock = nil;
static int queued = 0;

/*
 * This is the delegate object for the sound kit. It is used to keep
 * track of the number of queued packets for the sound device by collecting
 * the "complete" messages. Sometimes these get lost though for some reason.
 */

@interface Delegate : NSObject
- soundStream:sender didCompleteBuffer:(int)tag;
- soundStreamDidAbort:sender deviceReserved:(BOOL)flag;
@end

@implementation Delegate

/*
 * Decrement the number of queued buffers
 */

- soundStream:sender didCompleteBuffer:(int)tag
{
	[queue_lock lock];
	queued--;
	[queue_lock unlock];
	return self;
}

/*
 * Deal with an aborted sound stream
 */

- soundStreamDidAbort:sender deviceReserved:(BOOL)flag;
{
	fprintf(stderr,"Sound stream aborted: ");
	if(flag!=YES) 
		fprintf(stderr,"Device reserved\n");
	else
		fprintf(stderr,"%s\n",
		[[NXSoundDevice textForError:[sender lastError]] cString]);
	play_sound = FALSE;
	return self;
}

@end

/*
 * Initialise the sound stream if we are playing sounds. We set up
 * the soundkit objects and the delegate necessary to track what has
 * been completed. A sound object may be placed on a remote host by the
 * use of an environment variable.
 */

int
sysdep_audio_init(void)
{
	NXSoundParameters *params = nil;
	SNDSoundStruct *sptr = NULL;
	char *host = getenv("SOUNDHOST");

	if(!play_sound)
		return OSD_OK;

	/* make the queue lock */
	queue_lock = [NSLock new];

	/* create sound object (possibly remotely) */
	if(host) {
		sound = [[NXSoundOut alloc]
			initOnHost:[NSString stringWithCString:host]];
		if(sound==nil) {
			fprintf(stderr,
			   "could not initialise sound object on %s\n", host);
	      		return OSD_NOT_OK;
	    	} else
	      		printf("Sending sound output to %s\n",host);
	} else
		sound=[[NXSoundOut alloc] init];

	if(!sound) {
		fprintf(stderr,"could not initialise sound object\n");
		return OSD_NOT_OK;
	}

	/* create a parameters object */
	printf("Sound is %d Hz %s %d bit samples\n", options.samplerate,
		sound_stereo ? "stereo" : "mono", sound_8bit ? 8 : 16);
	SNDAlloc(&sptr,0,SND_FORMAT_LINEAR_16,options.samplerate,
		sound_stereo ? 2 : 1 ,0);
	params = [[NXSoundParameters alloc] initFromSoundStruct:sptr];
	if(!params) {
		fprintf(stderr,"could not create sound parameters\n");
		return OSD_NOT_OK;
	}

	/* create the stream object */
	stream = [[NXPlayStream alloc]
		initOnDevice:sound withParameters:params];
	if(!stream) {
		fprintf(stderr,"could not create play stream\n");
		return OSD_NOT_OK;
	}

	/* set the delegate and activate the whole lot */
	[stream setDelegate:[Delegate new]];
	[stream activate];

	return OSD_OK;
}

/*
 * Wait for all sounds to finish and return.
 */

void
sysdep_audio_close(void)
{
	int i;

	/* return if we are not playing sounds at all */
	if(!play_sound)
		return;

	/*
	 * Wait for all the sounds to finish - but if
	 * they aren't all done in 4 seconds then quit anyway.
	 * (sometimes the delegate stops getting the messages!)
	 */

	for(i=0;i<4;i++) {
		int left;
		[queue_lock lock];
		left = queued;
		[queue_lock unlock];
		if(left==0)
			break;
			sleep(1);
	}
	[stream release];
	[sound release];
	[queue_lock release];
}

/*
 * o.k.- this is ugly, the idea of this function was to return the amount of
 * freespace in the audio output buffer. however, all attempts on my part
 * to do this "properly"  have either resulted in no sound of a horrible
 * unlistenable mess. This is not great either, but it does work after
 * a fashion. What I do is keep a count of the number of queued buffers.
 * As more are queued I return less free space - thus MAME queues smaller
 * buffers as the sound queue starts to fill up. Don't ask me for a rational
 * behind this, and feel free to rip it out and try to do better...
 */

long
sysdep_audio_get_freespace(void)
{
	int how_many;

	if(!play_sound)	/* in case of abort */
		return 0;

	[queue_lock lock];
	how_many = queued;
	[queue_lock unlock];

	if(how_many<8) return AUDIO_BUFF_SIZE;
	if(how_many<18) return (AUDIO_BUFF_SIZE>>1);
	if(how_many<30) return (AUDIO_BUFF_SIZE>>2);
	return 0;
}

/*
 * Here we play a block of audio.
 */

int
sysdep_audio_play(unsigned char *buff, int size)
{
	short buf16[AUDIO_BUFF_SIZE];
	short *pt16=buf16;
	int samples = sound_8bit ? size : size / 2;
	int bytes = sound_8bit ? size * 2 : size;

	/* make sure we have native 16 bit block */
	if(sound_8bit) {
		int i;
		unsigned char *pt8=buff;
		for(i=0;i<size;i++)
			*pt16++ = (*pt8++ - 0x80) * 256;
		pt16=buf16;
	} else
		pt16 = (short*)buff;

	/* Do the byte swapping and play the stream */
	SNDSwapHostToSound(pt16, pt16, samples,
		sound_stereo ? 2 : 1, SND_FORMAT_LINEAR_16);
	[stream playBuffer:pt16 size:bytes tag:(int)1];

	[queue_lock lock];
	queued++;
	[queue_lock unlock];
	return size;
}
