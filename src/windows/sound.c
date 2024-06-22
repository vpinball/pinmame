//============================================================
//
//	sound.c - Win32 implementation of MAME sound routines
//
//============================================================

// standard windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#if _MSC_VER >= 1800
 // Windows 2000 _WIN32_WINNT_WIN2K
 #define _WIN32_WINNT 0x0500
#elif _MSC_VER < 1600
 #define _WIN32_WINNT 0x0400
#else
 #define _WIN32_WINNT 0x0403
#endif
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>
#include <mmsystem.h>

// undef WINNT for dsound.h to prevent duplicate definition
#undef WINNT
#include <dsound.h>

// MAME headers
#include "driver.h"
#include "osdepend.h"
#include "window.h"
#include "video.h"
#include "rc.h"

#include "../../ext/libsamplerate/config.h"
#ifdef RESAMPLER_SSE_OPT
 #if (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || defined(__ia64__) || defined(__x86_64__)
  #include <xmmintrin.h>
  #include <emmintrin.h>
 #else // Arm Neon
  #include "../sse2neon.h"
 #endif
#endif


//============================================================
//	IMPORTS
//============================================================

extern int verbose;



//============================================================
//	DEBUGGING
//============================================================

#define LOG_SOUND				0
#define DISPLAY_UNDEROVERFLOW	0



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
//	GLOBAL VARIABLES
//============================================================

// global parameters
int							attenuation = 0;
double						volume_gain = 1.;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectSound objects
static LPDIRECTSOUND		dsound;
static DSCAPS				dsound_caps;

// sound buffers
static LPDIRECTSOUNDBUFFER	primary_buffer;
static LPDIRECTSOUNDBUFFER	stream_buffer;
static UINT32				stream_buffer_size;
static UINT32				stream_buffer_in;

// descriptors and formats
static DSBUFFERDESC			primary_desc;
static DSBUFFERDESC			stream_desc;
static WAVEFORMATEX			primary_format;
static WAVEFORMATEX			stream_format;

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

static int					is_stereo;
static int					force_mono_to_stereo;

static int					consecutive_lows = 0;
static int					consecutive_mids = 0;
static int					consecutive_highs = 0;

// for mono -> stereo conversion
static INT16 mix_buffer[ACCUMULATOR_SAMPLES * 2]; /* *2 for stereo */

// debugging
#if LOG_SOUND
static FILE *				sound_log;
#endif

// sound options
struct rc_option sound_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "audio_latency", NULL, rc_int, &audio_latency, "1", 1, 4, NULL, "set audio latency (increase to reduce glitches)" },
	{ "force_stereo", NULL, rc_bool, &force_mono_to_stereo, "0", 0, 0, NULL, "always force stereo output (e.g. to better support multi channel sound systems)" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//	PROTOTYPES
//============================================================

static int			dsound_init(void);
static void			dsound_kill(void);
static int			dsound_create_buffers(void);
static void			dsound_destroy_buffers(void);



//============================================================
//	bytes_in_stream_buffer
//============================================================

INLINE int bytes_in_stream_buffer(void)
{
	DWORD play_position, write_position;
	IDirectSoundBuffer_GetCurrentPosition(stream_buffer, &play_position, &write_position);
	if (stream_buffer_in > play_position)
		return stream_buffer_in - play_position;
	else
		return stream_buffer_size + stream_buffer_in - play_position;
}



//============================================================
//	osd_start_audio_stream
//============================================================

int osd_start_audio_stream(int stereo)
{
#if LOG_SOUND
	sound_log = fopen("sound.log", "w");
#endif

	consecutive_lows = 0;
	consecutive_mids = 0;
	consecutive_highs = 0;

	is_stereo = stereo;

	force_mono_to_stereo = pmoptions.force_mono_to_stereo; //!! how to use/set the sound_opts in here?

	// skip if sound disabled
	if (Machine->sample_rate != 0)
	{
		// attempt to initialize directsound
		if (dsound_init())
			return 1;

		// set the startup volume
		osd_set_mastervolume(attenuation);
	}

	// determine the number of samples per frame
	samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;

	// compute how many samples to generate the first frame
	samples_left_over = samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	// return the samples to play the first frame
	return samples_this_frame;
}



//============================================================
//	osd_stop_audio_stream
//============================================================

void osd_stop_audio_stream(void)
{
	// if nothing to do, don't do it
	if (Machine->sample_rate == 0)
		return;

	// kill the buffers and dsound
	dsound_destroy_buffers();
	dsound_kill();

	// print out over/underflow stats
	if (verbose && (buffer_overflows || buffer_underflows))
		fprintf(stderr, "Sound: buffer overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);

#if LOG_SOUND
	if (sound_log)
		fprintf(sound_log, "Sound buffer: overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
	fclose(sound_log);
#endif
}



//============================================================
//	update_sample_adjustment
//============================================================

static void update_sample_adjustment(int buffered)
{
	// if we're not throttled don't bother
	if (!throttle)
	{
		consecutive_lows = 0;
		consecutive_mids = 0;
		consecutive_highs = 0;
		current_adjustment = 0;
		return;
	}

#if LOG_SOUND
	fprintf(sound_log, "update_sample_adjustment: %d\n", buffered);
#endif

	// do we have too few samples in the buffer?
	if (buffered < lower_thresh)
	{
		// keep track of how many consecutive times we get this condition
		consecutive_lows++;
		consecutive_mids = 0;
		consecutive_highs = 0;

		// adjust so that we generate more samples per frame to compensate
		current_adjustment = (consecutive_lows < MAX_SAMPLE_ADJUST) ? consecutive_lows : MAX_SAMPLE_ADJUST;

#if LOG_SOUND
		fprintf(sound_log, "  (too low - adjusting to %d)\n", current_adjustment);
#endif
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

#if LOG_SOUND
		fprintf(sound_log, "  (too high - adjusting to %d)\n", current_adjustment);
#endif
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

#if LOG_SOUND
			fprintf(sound_log, "  (%d consecutive_mids - adjusting to %d)\n", consecutive_mids, current_adjustment);
#endif
		}
	}
}



//============================================================
//	copy_sample_data
//============================================================

static void copy_sample_data(INT16 *data, int bytes_to_copy)	// adopted from MAME 0.105
{
	void *buffer1, *buffer2;
	DWORD length1, length2;
	int cur_bytes;

	// attempt to lock the stream buffer
	HRESULT result = IDirectSoundBuffer_Lock(stream_buffer, stream_buffer_in, bytes_to_copy, &buffer1, &length1, &buffer2, &length2, 0);

	// if we failed, assume it was an underflow (i.e.,
	if (result != DS_OK)
	{
		buffer_underflows++;
		return;
	}

	// adjust the input pointer
	stream_buffer_in = (stream_buffer_in + bytes_to_copy) % stream_buffer_size;

	// copy the first chunk
	cur_bytes = (bytes_to_copy > length1) ? length1 : bytes_to_copy;
	memcpy(buffer1, data, cur_bytes);

	// adjust for the number of bytes
	bytes_to_copy -= cur_bytes;
	data = (INT16 *)((UINT8 *)data + cur_bytes);	// adopted from MAME 0.105

	// copy the second chunk (2 pointers due to circular dsound buffer)
	if (bytes_to_copy != 0 && buffer2)
	{
		cur_bytes = (bytes_to_copy > length2) ? length2 : bytes_to_copy;
		memcpy(buffer2, data, cur_bytes);
	}

	// unlock
	result = IDirectSoundBuffer_Unlock(stream_buffer, buffer1, length1, buffer2, length2);
}



//============================================================
//	osd_update_audio_stream
//============================================================

int osd_update_audio_stream(INT16 *buffer)
{
	// if nothing to do, don't do it
	if (Machine->sample_rate != 0 && stream_buffer)
	{
		const int original_bytes = bytes_in_stream_buffer();
		const int input_bytes = samples_this_frame * stream_format.nBlockAlign;
		int final_bytes;

		// update the sample adjustment
		update_sample_adjustment(original_bytes);

		// copy mono to stereo buffer if requested
		if(!is_stereo && force_mono_to_stereo)
		{
			int i;
			for (i = 0; i < input_bytes/2; ++i)
				mix_buffer[i*2] = mix_buffer[i*2+1] = buffer[i];
			buffer = mix_buffer;
		}

		if(volume_gain > 1.)
		{
			const float vgf = (float)volume_gain;
			int i;
			for (i = 0; i < input_bytes; ++i)
			{
#if defined(RESAMPLER_SSE_OPT)
				const INT16 samplei = (INT16)_mm_cvtss_si32(_mm_max_ss(_mm_min_ss(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), buffer[i]), _mm_set_ss(vgf)), _mm_set_ss(32767.f)), _mm_set_ss(-32768.f)));
#else
				const float sample = (float)buffer[i] * vgf;
				INT16 samplei;
				if (sample <= -32768.f)
					samplei = -32768;
				else if (sample >= 32767.f)
					samplei = 32767;
				else
#ifdef __GNUC__
					samplei = (INT16)(sample + .5f);
#else
					samplei = (INT16)(lrintf(sample));
#endif
#endif
				buffer[i] = samplei;
			}
		}

		// copy data into the sound buffer
		copy_sample_data(buffer, input_bytes);

		// check for overflows
		final_bytes = bytes_in_stream_buffer();
		if (final_bytes < original_bytes)
			buffer_overflows++;
	}

	// reset underflow/overflow tracking
	if (++total_frames == INGORE_UNDERFLOW_FRAMES)
		buffer_overflows = buffer_underflows = 0;

	// update underflow/overflow logging
#if (DISPLAY_UNDEROVERFLOW || LOG_SOUND)
{
	static int prev_overflows, prev_underflows;
	if (total_frames > INGORE_UNDERFLOW_FRAMES && (buffer_overflows != prev_overflows || buffer_underflows != prev_underflows))
	{
		prev_overflows = buffer_overflows;
		prev_underflows = buffer_underflows;
#if DISPLAY_UNDEROVERFLOW
		usrintf_showmessage("overflows=%d underflows=%d", buffer_overflows, buffer_underflows);
#endif
#if LOG_SOUND
		fprintf(sound_log, "************************ overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
#endif
	}
}
#endif

	// compute how many samples to generate next frame
	samples_left_over += samples_per_frame;
	samples_this_frame = (UINT32)samples_left_over;
	samples_left_over -= (double)samples_this_frame;

	samples_this_frame += current_adjustment;

	// return the samples to play this next frame
	return samples_this_frame;
}



//============================================================
//	osd_set_mastervolume
//============================================================

void osd_set_mastervolume(int _attenuation)
{
	volume_gain = 1.;

	// clamp the attenuation to -32 - 32 range
	if (_attenuation < -32)
		_attenuation = -32;
	if (_attenuation > 32)
		_attenuation = 32;
	attenuation = _attenuation;

	while (_attenuation-- > 0)
		volume_gain *= 1.1220184543019634355910389464779; // = (10 ^ (1/20)) = 1dB

	// set the master volume
	if (stream_buffer && is_enabled)
		IDirectSoundBuffer_SetVolume(stream_buffer, (attenuation == -32) ? DSBVOLUME_MIN : min(attenuation,0) * 100);
}



//============================================================
//	osd_get_mastervolume
//============================================================

int osd_get_mastervolume(void)
{
	return attenuation;
}



//============================================================
//	osd_sound_enable
//============================================================

void osd_sound_enable(int enable_it)
{
	if (stream_buffer)
	{
		if (enable_it)
			IDirectSoundBuffer_SetVolume(stream_buffer, min(attenuation,0) * 100);
		else
			IDirectSoundBuffer_SetVolume(stream_buffer, DSBVOLUME_MIN);

		is_enabled = enable_it;
	}
}

// Structure of Audio Device informations
typedef struct
{
	LPGUID guid;
	char description[1024];
	char module[1024];
}AudioDevice;

// maximum number of handled devices
#define MAX_HANDLED_DEVICES 10

// AudioDevices information
AudioDevice audio_devices[MAX_HANDLED_DEVICES];

// Number of enumerated audio devices
int audio_devices_number = 0;

// Number of current audio device
int current_audio_device = -1;

// Audio Devices enumeration callback
BOOL CALLBACK EnumCallBack (LPGUID guid, LPCSTR desc,
	LPCSTR mod, LPVOID list)
{
	AudioDevice *ad;

	if(audio_devices_number>=MAX_HANDLED_DEVICES-1)
		return FALSE;	// Hardcoded Max reached (TODO: realloc)

	ad = &(audio_devices[audio_devices_number]);
	if (guid == NULL)
		ad->guid = NULL;
	else{
		ad->guid = (LPGUID)malloc(sizeof (GUID));
		memcpy (ad->guid, guid, sizeof (GUID));
	}

	strcpy(ad->description,desc);
	strcpy(ad->module,mod);

	audio_devices_number++;
	return TRUE;
}

//============================================================
//	osd_enum_audio_devices
//============================================================
int osd_enum_audio_devices()
{
	audio_devices_number = 0;
	if(DirectSoundEnumerate (EnumCallBack, NULL)!=DS_OK)
		return 0;
	return audio_devices_number;
}

//============================================================
//	osd_get_audio_devices_count
//============================================================
int osd_get_audio_devices_count()
{
	return audio_devices_number;
}

//============================================================
//	osd_get_audio_device_description
//============================================================
char* osd_get_audio_device_description(int num)
{
	return audio_devices[num].description;
}

//============================================================
//	osd_get_audio_device_module
//============================================================
char* osd_get_audio_device_module(int num)
{
	return audio_devices[num].module;
}

//============================================================
//	osd_set_audio_device
//============================================================
int osd_set_audio_device(int num)
{
	if(num<0 || num>=audio_devices_number)
		current_audio_device = -1;
	else
		current_audio_device = num;
	return current_audio_device;
}

//============================================================
//	osd_get_current_audio_device
//============================================================
int osd_get_current_audio_device()
{
	return current_audio_device;
}

//============================================================
//	dsound_init
//============================================================

static int dsound_init(void)
{
	HRESULT result;

	LPGUID guid = NULL;	// Default audio device

	osd_enum_audio_devices(); // (Re-)Enumerate devices

	// Get the guid to the user selected audio device (NULL if no selected)
	if(current_audio_device>= 0 && current_audio_device<audio_devices_number)
		guid = audio_devices[current_audio_device].guid;

	// now attempt to create it
	result = DirectSoundCreate(guid, &dsound, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating DirectSound: %08x\n", (UINT32)result);
		goto cant_create_dsound;
	}

	// get the capabilities
	dsound_caps.dwSize = sizeof(dsound_caps);
	result = IDirectSound_GetCaps(dsound, &dsound_caps);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error getting DirectSound capabilities: %08x\n", (UINT32)result);
		goto cant_get_caps;
	}

	// set the cooperative level
	result = IDirectSound_SetCooperativeLevel(dsound, win_video_window, DSSCL_PRIORITY);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error setting DirectSound cooperative level: %08x\n", (UINT32)result);
		goto cant_set_coop_level;
	}

	// make a format description for what we want
	stream_format.wBitsPerSample	= 16;
	stream_format.wFormatTag		= WAVE_FORMAT_PCM;
	stream_format.nChannels			= (is_stereo || force_mono_to_stereo) ? 2 : 1;
	stream_format.nSamplesPerSec	= (int)(Machine->sample_rate+0.5);
	stream_format.nBlockAlign		= stream_format.wBitsPerSample * stream_format.nChannels / 8;
	stream_format.nAvgBytesPerSec	= stream_format.nSamplesPerSec * stream_format.nBlockAlign;

	// compute the buffer sizes
	stream_buffer_size = ((UINT64)MAX_BUFFER_SIZE * (UINT64)stream_format.nSamplesPerSec) / 44100;
	stream_buffer_size = (stream_buffer_size * stream_format.nBlockAlign) / 4;
	stream_buffer_size = (UINT32)((stream_buffer_size * 30) / Machine->drv->frames_per_second + 0.5);
	stream_buffer_size = (stream_buffer_size / 1024) * 1024;
	if (stream_buffer_size < 1024)
		stream_buffer_size = 1024;

	// compute the upper/lower thresholds
	lower_thresh = audio_latency * stream_buffer_size / 5;
	upper_thresh = (audio_latency + 1) * stream_buffer_size / 5;
#if LOG_SOUND
	fprintf(sound_log, "stream_buffer_size = %d (max %d)\n", stream_buffer_size, MAX_BUFFER_SIZE);
	fprintf(sound_log, "lower_thresh = %d\n", lower_thresh);
	fprintf(sound_log, "upper_thresh = %d\n", upper_thresh);
#endif

	// create the buffers
	if (dsound_create_buffers())
		goto cant_create_buffers;

	// start playing
	is_enabled = 1;

	result = IDirectSoundBuffer_Play(stream_buffer, 0, 0, DSBPLAY_LOOPING);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error playing: %08x\n", (UINT32)result);
		goto cant_play;
	}
	return 0;

	// error handling
cant_play:
	dsound_destroy_buffers();
cant_create_buffers:
cant_set_coop_level:
cant_get_caps:
	IDirectSound_Release(dsound);
cant_create_dsound:
	dsound = NULL;
	return 0;
}



//============================================================
//	dsound_kill
//============================================================

static void dsound_kill(void)
{
	// release the object
	if (dsound)
		IDirectSound_Release(dsound);
	dsound = NULL;
}



//============================================================
//	dsound_create_buffers
//============================================================

static int dsound_create_buffers(void)
{
	HRESULT result;
	void *buffer;
	DWORD locked;

	// create a buffer desc for the primary buffer
	memset(&primary_desc, 0, sizeof(primary_desc));
	primary_desc.dwSize				= sizeof(primary_desc);
	primary_desc.dwFlags			= DSBCAPS_PRIMARYBUFFER |
									  DSBCAPS_GETCURRENTPOSITION2;
	primary_desc.lpwfxFormat		= NULL;

	// create the primary buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &primary_desc, &primary_buffer, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating primary DirectSound buffer: %08x\n", (UINT32)result);
		goto cant_create_primary;
	}

	// attempt to set the primary format
	result = IDirectSoundBuffer_SetFormat(primary_buffer, &stream_format);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error setting primary DirectSound buffer format: %08x\n", (UINT32)result);
		goto cant_set_primary_format;
	}

	// get the primary format
	result = IDirectSoundBuffer_GetFormat(primary_buffer, &primary_format, sizeof(primary_format), NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error getting primary DirectSound buffer format: %08x\n", (UINT32)result);
		goto cant_get_primary_format;
	}
	if (verbose)
		fprintf(stderr, "DirectSound: Primary buffer: %d Hz, %d bits, %d channels\n",
				(int)primary_format.nSamplesPerSec, (int)primary_format.wBitsPerSample, (int)primary_format.nChannels);

	// create a buffer desc for the stream buffer
	memset(&stream_desc, 0, sizeof(stream_desc));
	stream_desc.dwSize				= sizeof(stream_desc);
	stream_desc.dwFlags				= DSBCAPS_CTRLVOLUME |
									  DSBCAPS_GLOBALFOCUS |
									  DSBCAPS_GETCURRENTPOSITION2;
	stream_desc.dwBufferBytes 		= stream_buffer_size;
	stream_desc.lpwfxFormat			= &stream_format;

	// create the stream buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &stream_desc, &stream_buffer, NULL);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error creating DirectSound stream buffer: %08x\n", (UINT32)result);
		goto cant_create_buffer;
	}

	// lock the buffer
	result = IDirectSoundBuffer_Lock(stream_buffer, 0, stream_buffer_size, &buffer, &locked, NULL, NULL, 0);
	if (result != DS_OK)
	{
		fprintf(stderr, "Error locking DirectSound stream buffer: %08x\n", (UINT32)result);
		goto cant_lock_buffer;
	}

	// clear the buffer and unlock it
	memset(buffer, 0, locked);
	IDirectSoundBuffer_Unlock(stream_buffer, buffer, locked, NULL, 0);
	return 0;

	// error handling
cant_lock_buffer:
	IDirectSoundBuffer_Release(stream_buffer);
cant_create_buffer:
	stream_buffer = NULL;
cant_get_primary_format:
cant_set_primary_format:
	IDirectSoundBuffer_Release(primary_buffer);
cant_create_primary:
	primary_buffer = NULL;
	return 0;
}



//============================================================
//	dsound_destroy_buffers
//============================================================

static void dsound_destroy_buffers(void)
{
	// stop any playback
	if (stream_buffer)
		IDirectSoundBuffer_Stop(stream_buffer);

	// release the stream buffer
	if (stream_buffer)
		IDirectSoundBuffer_Release(stream_buffer);
	stream_buffer = NULL;

	// release the primary buffer
	if (primary_buffer != NULL)
		IDirectSoundBuffer_Release(primary_buffer);
	primary_buffer = NULL;
}
