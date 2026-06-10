// ---------------------------------------------------------------------------
// miniaudio_bass_compat.cpp
//
// Implementation of the BASS-style channel/stream API on top of miniaudio
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// ---------------------------------------------------------------------------
#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

#include <miniaudio/miniaudio.h>

#include "miniaudio_bass_compat.hpp"
#include "miniaudio_private.h"
#include "altsound_data.hpp"
#include "altsound_logger.hpp"

#include <unordered_map>
#include <mutex>

// Defined by the integration layer
extern StreamArray channel_stream;

// Engine/stream bookkeeping
static ma_engine* g_engine = nullptr;
static std::unordered_map<unsigned int, _internal_stream_data> g_streamMap;
static std::mutex g_streamMapMutex;
static uint32_t g_nextStreamId = 1;

uint32_t g_channels = 2;
uint32_t g_sampleRate = 44100;
int g_last_ma_err = 0;

std::vector<EndedStream> g_endedStreams;
std::mutex g_endedMutex;

// Fired by miniAudio (audio thread) the moment a non-looping sound reaches its
// end. We only mark the stream and queue its SYNCPROC here; the actual firing
// (which frees the sound) happens later from the engine's onProcess, as the
// sound must not be uninitialized from within this callback
static void MiniAudio_StreamEndCallback(void* pUserData, ma_sound* pSound)
{
	const unsigned int hstream = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pUserData));

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end())
		return;

	it->second.playing = false;
	if (it->second.sync_callback) {
		std::lock_guard<std::mutex> endLock(g_endedMutex);
		g_endedStreams.push_back({ it->second.sync_callback, it->second.hsync, hstream, it->second.sync_userdata });
	}
}

unsigned int MiniAudio_StreamCreateFile(bool mem, const std::string& file, unsigned long long length, bool loop)
{
	if (file.empty()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return MINIAUDIO_NO_STREAM;
	}

	ma_decoder_config config = altsound_ma_decoder_config_init(ma_format_f32, g_channels, g_sampleRate);
	ma_decoder* decoder = new ma_decoder();
	ma_result result = altsound_ma_decoder_init_file(file.c_str(), &config, decoder);
	if (result != MA_SUCCESS) {
		MiniAudio_ErrorSetCode(result);
		delete decoder;
		return MINIAUDIO_NO_STREAM;
	}

	ma_sound* sound = new ma_sound();
	result = altsound_ma_sound_init_from_decoder(g_engine, decoder, MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_PITCH, sound);
	if (result != MA_SUCCESS) {
		MiniAudio_ErrorSetCode(result);
		altsound_ma_decoder_uninit(decoder);
		delete decoder;
		delete sound;
		return MINIAUDIO_NO_STREAM;
	}

	altsound_ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);

	unsigned int hstream = g_nextStreamId++;

	altsound_ma_sound_set_end_callback(sound, MiniAudio_StreamEndCallback, reinterpret_cast<void*>(static_cast<uintptr_t>(hstream)));

	_internal_stream_data data;
	data.decoder = decoder;
	data.sound = sound;
	data.playing = false;
	data.paused = false;
	data.looping = loop;
	data.sample_rate = decoder->outputSampleRate;
	data.channels = decoder->outputChannels;
	data.sync_callback = nullptr;
	data.sync_userdata = nullptr;

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	g_streamMap[hstream] = data;

	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return hstream;
}

bool MiniAudio_ChannelSetVolume(unsigned int hstream, float value)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return 0;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return 0;
	}

	it->second.volume = value;
	if (it->second.sound)
		altsound_ma_sound_set_volume(it->second.sound, value);
	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return 1;
}

bool MiniAudio_ChannelGetVolume(unsigned int hstream, float& value)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	const auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	value = it->second.volume;
	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return true;
}

unsigned int MiniAudio_ChannelSetSync(unsigned int hstream, unsigned int type, void* proc, void* user)
{
	if (hstream == MINIAUDIO_NO_STREAM || !proc) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return 0;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return 0;
	}

	if (type & MINIAUDIO_SYNC_END) {
		it->second.sync_callback = reinterpret_cast<SYNCPROC>(proc);
		it->second.sync_userdata = user;

		static unsigned int sync_id = 1;
		unsigned int hsync = sync_id++;
		it->second.hsync = hsync;
		MiniAudio_ErrorSetCode(MA_SUCCESS);
		return hsync;
	}
	MiniAudio_ErrorSetCode(MA_ERROR);
	return 0;
}

bool MiniAudio_ChannelPlay(unsigned int hstream, bool restart)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	if (restart && it->second.sound) {
		altsound_ma_sound_seek_to_pcm_frame(it->second.sound, 0);
	}

	if (it->second.sound) {
		altsound_ma_sound_set_volume(it->second.sound, it->second.volume);
		altsound_ma_sound_start(it->second.sound);
	}

	it->second.playing = true;
	it->second.paused = false;
	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return true;
}

bool MiniAudio_ChannelPause(unsigned int hstream)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	if (it->second.sound)
		altsound_ma_sound_stop(it->second.sound);

	it->second.paused = true;
	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return true;
}

bool MiniAudio_ChannelStop(unsigned int hstream)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	if (it->second.sound) {
		altsound_ma_sound_stop(it->second.sound);
		altsound_ma_sound_seek_to_pcm_frame(it->second.sound, 0);
	}

	it->second.playing = false;
	it->second.paused = false;
	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return true;
}

bool MiniAudio_StreamFree(unsigned int hstream)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it == g_streamMap.end()) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return false;
	}

	if (it->second.sound) {
		altsound_ma_sound_uninit(it->second.sound);
		delete it->second.sound;
	}

	if (it->second.decoder) {
		altsound_ma_decoder_uninit(it->second.decoder);
		delete it->second.decoder;
	}

	g_streamMap.erase(it);

	for (int i = 0; i < ALT_MAX_CHANNELS; ++i) {
		if (channel_stream[i] && channel_stream[i]->hstream == hstream) {
			delete channel_stream[i];
			channel_stream[i] = nullptr;
			break;
		}
	}

	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return true;
}

unsigned int MiniAudio_ChannelIsActive(unsigned int hstream)
{
	if (hstream == MINIAUDIO_NO_STREAM) {
		MiniAudio_ErrorSetCode(MA_INVALID_ARGS);
		return MINIAUDIO_ACTIVE_STOPPED;
	}

	std::lock_guard<std::mutex> lock(g_streamMapMutex);
	auto it = g_streamMap.find(hstream);
	if (it != g_streamMap.end()) {
		const _internal_stream_data& internal = it->second;
		if (internal.paused) {
			MiniAudio_ErrorSetCode(MA_SUCCESS);
			return MINIAUDIO_ACTIVE_PAUSED;
		}
		else if (internal.playing) {
			MiniAudio_ErrorSetCode(MA_SUCCESS);
			return MINIAUDIO_ACTIVE_PLAYING;
		}
	}

	MiniAudio_ErrorSetCode(MA_SUCCESS);
	return MINIAUDIO_ACTIVE_STOPPED;
}
