#include <stdio.h>
#include "driver.h"
#include "rc.h"

#include "dllsound.h"

//============================================================
//	GLOBAL VARIABLES
//============================================================

// global parameters
int							attenuation = 0;

extern int g_fPause;

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
static int					writing_advance = 0;
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

static int currentWritingBufferPos = 0;
static int currentStreamingBufferPos = -1; // -1 means not ready
static int streamingBufSize = -1; // Hold the data to be out using get pending
static int stream_buffer_size;

static INT16* streamingBuf = NULL;
static int streamingBufferInitialized = 0;
static int writingToStreamDiff = 0;

int osd_start_audio_stream(int stereo)
{
	printf("osd_start_audio_stream SampleRate:%d stereo:%d fps:%.2f\n", Machine->sample_rate,stereo, Machine->drv->frames_per_second);
	channels = stereo ? 2 : 1;

	// compute the buffer sizes
	stream_buffer_size = ((UINT64)MAX_BUFFER_SIZE * (UINT64)(Machine->sample_rate)) / 44100; //!! 44100 ?!
	stream_buffer_size = (int)((double)(stream_buffer_size * 30) / Machine->drv->frames_per_second);
	stream_buffer_size = (stream_buffer_size / 1024) * 1024;
	stream_buffer_size *= channels;
	// compute the upper/lower thresholds
	lower_thresh = audio_latency * stream_buffer_size / 5;
	upper_thresh = (audio_latency + 1) * stream_buffer_size / 5;

	streamingBuf = (INT16*)malloc(stream_buffer_size * sizeof(INT16));
	streamingBufferInitialized = 1;
	currentStreamingBufferPos = 0;
	currentWritingBufferPos = 0;

	// determine the number of samples per frame
	samples_per_frame = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;
	printf("sample_rate: %d frames_per_second: %.1f samples_per_frame: %.1f\n", Machine->sample_rate, Machine->drv->frames_per_second, samples_per_frame);
	// compute how many samples to generate the first frame
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	writing_advance = (int)(samples_per_frame * channels * 3);

	printf("stream_buffer_size: %d lower_thresh:%d upper_thresh: %d\n", stream_buffer_size, lower_thresh, upper_thresh);

	// return the samples to play the first frame
	return samples_this_frame;
}

//static cycles_t lastUpdate = 0;
//static int tmp = 0;

int osd_update_audio_stream(INT16 *buf)
{
	unsigned int i;

	if (!streamingBufferInitialized)
		return samples_this_frame;
	//cycles_t cyc = osd_cycles();
	//printf("Updt: %d\n", (cyc - lastUpdate));
	//lastUpdate = cyc;

	//printf("samples_this_frame: %d\n", samples_this_frame);

	for (i = 0; i < samples_this_frame*channels; i++)
	{
		streamingBuf[currentWritingBufferPos] = buf[i];
		currentWritingBufferPos++;
		if (currentWritingBufferPos >  stream_buffer_size)
			currentWritingBufferPos = 0;
		writingToStreamDiff++;
	}

	// Adjustement
	{
		int potentialDiff = writingToStreamDiff - writing_advance;
		
		//Try to let approx one frame of advance to writing
		if (potentialDiff != 0)
			current_adjustment = potentialDiff > 0 ? -1 : 1;

		//if ((++tmp) % 30 == 0)
		//	printf("potentialDiff: %d current_adjustment: %d samples_this_frame %d\n", potentialDiff, current_adjustment, samples_this_frame);
	}

	//if ((++tmp)  % 30 == 0)
	//	printf("writingToStreamDif: %d current_adjustment: %d samples_this_frame %d\n", writingToStreamDiff, current_adjustment, samples_this_frame);

	// compute how many samples to generate next frame
	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	samples_this_frame += current_adjustment;
	
	//if (streamingBufSize >= 0)
	//	samples_this_frame = streamingBufSize;

	// return the samples to play this next frame
	return samples_this_frame;
}

void osd_stop_audio_stream(void)
{
}

void forceResync()
{
	currentStreamingBufferPos = (int)((long)(currentWritingBufferPos - writing_advance) % (long)stream_buffer_size);
	printf("Resync %d (dropped:%d samples)\n", writingToStreamDiff, (writingToStreamDiff- writing_advance));
	writingToStreamDiff = writing_advance;
}

//static cycles_t last = 0;

int fillAudioBuffer(float *const __restrict dest, const int outChannels, const int maxSamples)
{
	int nbOut = streamingBufSize;

	if (g_fPause)
		return 0;

	if (!streamingBufferInitialized)
		return 0;

	if (streamingBufSize < 0)
	{
		streamingBufSize = maxSamples;
		forceResync();
		return 0;
	}
	//cycles_t cyc = osd_cycles();
	//printf("Fill: %d\n",(cyc-last));
	//last = cyc;

	if (nbOut > maxSamples)
		nbOut = maxSamples;

	if (channels == outChannels) // stereo to stereo or mono to mono
	{
		int i;
		for (i = 0; i < nbOut; i++)
		{
			dest[i] = (float)streamingBuf[currentStreamingBufferPos] * (float)(1./32768.0);
			currentStreamingBufferPos++;
			if (currentStreamingBufferPos > stream_buffer_size)
				currentStreamingBufferPos = 0;
			//writingToStreamDiff--;
		}
		writingToStreamDiff -= nbOut;
	}
	else
	if(channels == 1 && outChannels == 2) // Mono to stereo
	{
		const int nbToRead = nbOut / 2;
		int outPos = 0;
		int i;
		for (i = 0; i < nbToRead; i++)
		{
			dest[outPos    ] = (float)streamingBuf[currentStreamingBufferPos] * (float)(1./32768.0);
			dest[outPos + 1] = dest[outPos]; // Copy the mono on both out left and right
			outPos += 2;
			currentStreamingBufferPos++;
			if (currentStreamingBufferPos > stream_buffer_size)
				currentStreamingBufferPos = 0;
			//writingToStreamDiff--;
		}
		writingToStreamDiff -= nbToRead;
	}
	// TODO: stereo to mono
	
	if (writingToStreamDiff > writing_advance *2 || writingToStreamDiff <0)
		forceResync();

	return nbOut;
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
