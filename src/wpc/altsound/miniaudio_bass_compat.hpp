// ---------------------------------------------------------------------------
// miniaudio_bass_compat.hpp
//
// BASS-style channel/stream API implemented on top of miniaudio
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// ---------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include "altsound_data.hpp"

#define MINIAUDIO_ATTRIB_VOL 1
#define MINIAUDIO_SYNC_END 2
#define MINIAUDIO_SYNC_ONETIME 0x80000000

#define MINIAUDIO_ACTIVE_STOPPED 0
#define MINIAUDIO_ACTIVE_PLAYING 1
#define MINIAUDIO_ACTIVE_PAUSED 3

#define MINIAUDIO_SAMPLE_LOOP 0x4

struct ma_decoder;
struct ma_sound;
typedef void (ALTSOUNDCALLBACK *SYNCPROC)(unsigned int hsync, unsigned int hstream, unsigned int data, void *user);

struct _internal_stream_data {
	ma_decoder* decoder = nullptr;
	ma_sound* sound = nullptr;
	bool playing = false;
	bool paused = false;
	bool looping = false;
	uint32_t sample_rate = 44100;
	uint32_t channels = 2;
	float volume = 1.0f;
	SYNCPROC sync_callback = nullptr;
	void* sync_userdata = nullptr;
	unsigned int hsync = 0;
};

// An ended (non-looping) stream queued by the miniAudio end callback (audio
// thread) to have its SYNCPROC fired from a safe point (the engine's onProcess,
// after the read), since the SYNCPROC frees the sound, which must not happen
// from within the end callback
struct EndedStream {
	SYNCPROC callback;
	unsigned int hsync;
	unsigned int hstream;
	void* userdata;
};

extern std::vector<EndedStream> g_endedStreams;
extern std::mutex g_endedMutex;

extern uint32_t g_sampleRate;
extern uint32_t g_channels;
extern int g_last_ma_err;

inline int MiniAudio_ErrorGetCode()
{
	return g_last_ma_err;
}

inline void MiniAudio_ErrorSetCode(int ma_err)
{
	g_last_ma_err = ma_err;
}

unsigned int MiniAudio_StreamCreateFile(bool mem, const char* file, unsigned long long offset, unsigned long long length, unsigned int flags);
int MiniAudio_ChannelSetAttribute(unsigned int hstream, unsigned int attrib, float value);
bool MiniAudio_ChannelGetAttribute(unsigned int hstream, unsigned int attrib, float* value);
unsigned int MiniAudio_ChannelSetSync(unsigned int hstream, unsigned int type, unsigned long long param, void* proc, void* user);
int MiniAudio_ChannelPlay(unsigned int hstream, bool restart);
int MiniAudio_ChannelPause(unsigned int hstream);
int MiniAudio_ChannelStop(unsigned int hstream);
unsigned int MiniAudio_ChannelIsActive(unsigned int hstream);
int MiniAudio_StreamFree(unsigned int hstream);
