#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "driver.h"
//#include "misc.h"
#include "video.h"
#include "rc.h"
#include "fileio.h"

#include "dllsound.h"

//============================================================
//	GLOBAL VARIABLES
//============================================================

// global parameters
int							attenuation = 0;


//============================================================
//	PARAMETERS
//============================================================

#define INGORE_UNDERFLOW_FRAMES	100

// the local buffer is what the stream buffer feeds from
// note that this needs to be large enough to buffer at frameskip 11
// for 30fps games like Tapper; we will scale the value down based
// on the actual framerate of the game
#define MAX_BUFFER_SIZE			(128 * 1024)

// this is the maximum number of extra samples we will ask for
// per frame (I know this looks like a lot, but most of the
// time it will generally be nowhere close to this)
#define MAX_SAMPLE_ADJUST		16


//============================================================
//	LOCAL VARIABLES
//============================================================

// buffer
int channels = 1;

// buffer over/underflow counts
static int					total_frames;
static int					buffer_underflows;
static int					buffer_overflows;

// global sample tracking
static double				samples_per_frame;
static double				samples_left_over;
static UINT32				samples_this_frame;

// sample rate adjustments
static int					current_adjustment = 0;
static int					audio_latency;
static int					lower_thresh;
static int					upper_thresh;

// enabled state
static int					is_enabled = 1;

// sound options (none at this time)
struct rc_option sound_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "audio_latency", NULL, rc_int, &audio_latency, "1", 1, 4, NULL, "set audio latency (increase to reduce glitches)" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



/******************************************************************************

Sound

******************************************************************************/

/*
osd_start_audio_stream() is called at the start of the emulation to initialize
the output stream, then osd_update_audio_stream() is called every frame to
feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
When the stream is stereo, left and right samples are alternated in the
stream.

osd_start_audio_stream() and osd_update_audio_stream() must return the number
of samples (or couples of samples, when using stereo) required for next frame.
This will be around Machine->sample_rate / Machine->drv->frames_per_second,
the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
maintain audio and video in sync when using vsync. Note that sound emulation,
especially when DACs are involved, greatly depends on the number of samples
per frame to be roughly constant, so the returned value must always stay close
to the reference value of Machine->sample_rate / Machine->drv->frames_per_second.
Of course that value is not necessarily an integer so at least a +/- 1
adjustment is necessary to avoid drifting over time.
*/

int currentWritingBufferPos = 0;
int currentStreamingBufferPos = -1; // -1 means not ready
int streamingBufSize = -1;

INT16* streamingBuf = NULL;
int streamingBufferInitialized = 0;

int osd_start_audio_stream(int stereo)
{
	printf("osd_start_audio_stream SampleRate:%d stereo:%d\n", Machine->sample_rate,stereo);
	channels = stereo ? 2 : 1;

	streamingBuf = (INT16*)malloc(MAX_BUFFER_SIZE * sizeof(INT16));
	streamingBufferInitialized = 1;
	currentStreamingBufferPos = 0;
	currentWritingBufferPos = 0;

	// determine the number of samples per frame
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;

	// compute how many samples to generate the first frame
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	// return the samples to play the first frame
	return samples_this_frame;
}

cycles_t lastUpdate = 0;
int osd_update_audio_stream(INT16 *buf)
{
	if (!streamingBufferInitialized)
		return samples_this_frame;
	//cycles_t cyc = osd_cycles();
	//printf("Updt: %d\n", (cyc - lastUpdate));
	//lastUpdate = cyc;

	for (int i = 0; i < samples_this_frame; i++)
	{
		streamingBuf[currentWritingBufferPos] = buf[i];
		currentWritingBufferPos++;
		if (currentWritingBufferPos >  MAX_BUFFER_SIZE)
			currentWritingBufferPos = 0;
	}

	//printf("streamingBufSize : %d, currentWritingBuffer:%d, currentWritingBufferPos:%d\n", streamingBufSize, currentWritingBuffer, currentWritingBufferPos);
	//int toWrite = samples_this_frame * channels;
	//printf("end: %d\n",end);
	
	
	// compute how many samples to generate next frame
	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	samples_this_frame += current_adjustment;

	//samples_this_frame = streamingBufSize;

	// return the samples to play this next frame
	return samples_this_frame;
}

void osd_stop_audio_stream(void)
{
}

cycles_t last = 0;
int lastStreamingBuffer = -1;
int fillAudioBuffer(float *dest, int maxSamples)
{
	int i = 0;
	if (!streamingBufferInitialized)
		return 0;

	if (streamingBufSize < 0)
		streamingBufSize = maxSamples;

	//cycles_t cyc = osd_cycles();
	//printf("Fill: %d\n",(cyc-last));
	//last = cyc;

	// Small adjustment for next frames. This wil help synchronizing
	// TODO: handle the modulo (just started over the buff start)
	if (currentStreamingBufferPos > currentWritingBufferPos)
		current_adjustment = 1;
	else
	if (currentStreamingBufferPos < currentWritingBufferPos)
		current_adjustment = -1;
	else
		current_adjustment = 0;

	//if(current_adjustment!=0)
	//	printf("syncing sound with output:%d \n", current_adjustment);

	int nb = streamingBufSize;
	if (nb > maxSamples)
		nb = maxSamples;
//	for (int i = readingPos; i < readingPos+ maxSamples; i++)
	for (int i = 0; i < nb; i++)
	{
		dest[i] = (float)streamingBuf[currentStreamingBufferPos] / 32768;
		currentStreamingBufferPos++;
		if (currentStreamingBufferPos > MAX_BUFFER_SIZE)
			currentStreamingBufferPos = 0;
	}

	return nb;
}

//============================================================
//	update_sample_adjustment
//============================================================

static void update_sample_adjustment(int buffered)
{
	static int consecutive_lows = 0;
	static int consecutive_mids = 0;
	static int consecutive_highs = 0;

	// if we're not throttled don't bother
	if (!throttle)
	{
		consecutive_lows = 0;
		consecutive_mids = 0;
		consecutive_highs = 0;
		current_adjustment = 0;
		return;
	}

	// do we have too few samples in the buffer?
	if (buffered < lower_thresh)
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows++;
		consecutive_mids = 0;
		consecutive_highs = 0;

		// adjust so that we generate more samples per frame to compensate
		current_adjustment = (consecutive_lows < MAX_SAMPLE_ADJUST) ? consecutive_lows : MAX_SAMPLE_ADJUST;
	}

	// do we have too many samples in the buffer?
	else if (buffered > upper_thresh)
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows = 0;
		consecutive_mids = 0;
		consecutive_highs++;

		// adjust so that we generate more samples per frame to compensate
		current_adjustment = (consecutive_highs < MAX_SAMPLE_ADJUST) ? -consecutive_highs : -MAX_SAMPLE_ADJUST;

	}

	// otherwise, we're in the sweet spot
	else
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows = 0;
		consecutive_mids++;
		consecutive_highs = 0;

		// after 10 or so of these, revert back to no adjustment
		if (consecutive_mids > 10 && current_adjustment != 0)
		{
			current_adjustment = 0;
		}
	}
}


/*
control master volume. attenuation is the attenuation in dB (a negative
number). To convert from dB to a linear volume scale do the following:
volume = MAX_VOLUME;
while (attenuation++ < 0)
volume /= 1.122018454;		//	= (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation)
{
}

int osd_get_mastervolume(void)
{
	return 0;
}

void osd_sound_enable(int enable)
{
}


/******************************************************************************

Audio device related functions

******************************************************************************/

/*
Enumerate audio devices using DirectSound and return the number of found devices
*/
int osd_enum_audio_devices(void)
{
	printf("osd_enum_audio_devices. \n");
	return 1;
}
/*
Return the number of found devices (by previous call to osd_enum_audio_devices())
*/
int osd_get_audio_devices_count(void)
{
	return 1;
}

/*
Return the audio device description (null char ended string) of the "num" device
*/
char* osd_get_audio_device_description(int num)
{
	return "Dummy audio device";
}

/*
Return the audio device module (null char ended string) of the "num" device
*/
char* osd_get_audio_device_module(int num)
{
	return "Dummy audio module";
}

/*
Set the audio device number to be used for the next call to dsound_init
*/
int osd_set_audio_device(int num)
{
	return 0;
}

/*
Get the current audio device number
*/
int osd_get_current_audio_device(void)
{
	return 0;
	//return -1;
}