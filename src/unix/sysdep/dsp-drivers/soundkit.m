/*
 * Driver for the NeXT / Apple soundkit objects for use on OpenStep
 * systems. Based on the older sound drivers, converted into the new
 * style framework by taking a long hard look at esound.c.
 *
 * -bat. 06/09/2000
 */

#import <stdio.h>
#import <stdlib.h>
#import <unistd.h>
#import <SoundKit/SoundKit.h>

#import "sysdep/sysdep_dsp.h"
#import "sysdep/sysdep_dsp_priv.h"
#import "sysdep/plugin_manager.h"

/*
 * Destroy function - we release the playStream and free up
 * the dsp structure.
 */

static void
soundkit_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
	NXPlayStream *playStream = (NXPlayStream*)dsp->_priv;

	[playStream release];

	free(dsp);
}

/*
 * Write a block of data to the sound object. We take pains to make sure it
 * in the correct format by copying it around the place quite a lot - this
 * makes 8 bit sound rather slow as we cannot write it to the device
 * directly due to an intel bug.
 */
   
static int
soundkit_dsp_write(struct sysdep_dsp_struct *dsp,
		unsigned char *data, int count)
{
	static int theTag = 1;
	static long buff_size = 0;
	static unsigned char *out_buff = NULL;

	BOOL isStereo = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? YES : NO;
	NXPlayStream *playStream = (NXPlayStream*)dsp->_priv;
	long bytes = sizeof(short) * count * (isStereo ? 2 : 1);

	/* do a malloc if necessary */
	if(bytes > buff_size) {
		if(out_buff)
			free(out_buff);
		out_buff = malloc(bytes);
		if(!out_buff)
			return 0;
		buff_size = bytes;
	}

	/* do the byte swap and write the block */
	SNDSwapHostToSound(out_buff, data, count,
			isStereo ? 2: 1, SND_FORMAT_LINEAR_16);
	[playStream playBuffer:out_buff size:bytes tag:theTag++];

	return count;
}

/*
 * Creation function.
 */

static void*
soundkit_dsp_create(const void *flags)
{
	NXPlayStream *playStream = nil;
	NXSoundOut *soundOut = nil;
	NXSoundParameters *soundParams = nil;
	SNDSoundStruct *soundStruct = NULL;
	char *soundHost = getenv("SOUNDHOST");
	id priv = nil;
	struct sysdep_dsp_struct *dsp = NULL;
	const struct sysdep_dsp_create_params *params = flags;

	/* allocate the dsp struct */
	if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
	{
		perror("error malloc failed for struct sysdep_dsp_struct\n");
		return NULL;
	}

	/* create sound object (possibly remotely) */
	if(soundHost) {
		soundOut = [[NXSoundOut alloc]
			initOnHost:[NSString stringWithCString:soundHost]];
		if(!soundOut) {
			fprintf(stderr,
				"could not initialise sound object on %s\n",
				soundHost);
	      		return NULL;
	    	} else
			fprintf(stderr, "info: soundkit: send sound to %s\n",
					soundHost);
	} else
		soundOut = [NXSoundOut new];

	if(!soundOut) {
		fprintf(stderr,"could not initialise sound object\n");
		return NULL;
	}

	/* create a parameters object - always 16 bit please ! */
	dsp->hw_info.type |= SYSDEP_DSP_16BIT;
	if(params->type & SYSDEP_DSP_STEREO) {
		SNDAlloc(&soundStruct, 0, SND_FORMAT_LINEAR_16,
				params->samplerate, 2, 0);
		dsp->hw_info.type |= SYSDEP_DSP_STEREO;
	} else {
		SNDAlloc(&soundStruct, 0, SND_FORMAT_LINEAR_16,
				params->samplerate, 1, 0);
	}
	soundParams = [[NXSoundParameters alloc]
			initFromSoundStruct:soundStruct];
	if(!soundParams) {
		fprintf(stderr,"could not create sound parameters\n");
		return NULL;
	}

	/* print the info */
	fprintf(stderr, "info: soundkit: %s sound at %d hz\n",
			(dsp->hw_info.type & SYSDEP_DSP_STEREO) ?
			"stereo" : "mono", params->samplerate);

	/* create the stream object */
	playStream = [[NXPlayStream alloc]
		initOnDevice:soundOut withParameters:soundParams];
	if(!playStream) {
		fprintf(stderr,"could not create play stream\n");
		return NULL;
	}

	/* fill in the functions and some data */
	dsp->_priv = playStream;
	dsp->write = soundkit_dsp_write;
	dsp->destroy = soundkit_dsp_destroy;
	dsp->hw_info.type = params->type;
	dsp->hw_info.samplerate = params->samplerate;

	/* activate and return */
   
	[playStream activate];
	return dsp;
}

/*
 * The public variables structure
 */

const struct plugin_struct sysdep_dsp_soundkit = {
	"soundkit",
	"sysdep_dsp",
	"NeXT SoundKit plugin",
	NULL, 				/* no options */
	NULL,				/* no init */
	NULL,				/* no exit */
	soundkit_dsp_create,
	3				/* high priority */
};
