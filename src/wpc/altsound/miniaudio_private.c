// ---------------------------------------------------------------------------
// miniaudio_private.c
//
// Single translation unit that compiles the miniaudio implementation and exposes a small set of wrappers
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// ---------------------------------------------------------------------------

#define MA_API static

// stb_vorbis is needed so miniaudio can decode Ogg Vorbis altsound samples
#define STB_VORBIS_HEADER_ONLY
#include <miniaudio/extras/stb_vorbis_static.c>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#undef STB_VORBIS_HEADER_ONLY
#include <miniaudio/extras/stb_vorbis_static.c>

ma_result altsound_ma_decoder_init_file(const char* pFilePath, const ma_decoder_config* pConfig, ma_decoder* pDecoder)
{
    return ma_decoder_init_file(pFilePath, pConfig, pDecoder);
}

ma_result altsound_ma_decoder_init_memory(const void* pData, size_t dataSize, const ma_decoder_config* pConfig, ma_decoder* pDecoder)
{
    return ma_decoder_init_memory(pData, dataSize, pConfig, pDecoder);
}

ma_result altsound_ma_decoder_read_pcm_frames(ma_decoder* pDecoder, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
    return ma_decoder_read_pcm_frames(pDecoder, pFramesOut, frameCount, pFramesRead);
}

ma_result altsound_ma_decoder_seek_to_pcm_frame(ma_decoder* pDecoder, ma_uint64 frameIndex)
{
    return ma_decoder_seek_to_pcm_frame(pDecoder, frameIndex);
}

ma_result altsound_ma_decoder_get_length_in_pcm_frames(ma_decoder* pDecoder, ma_uint64* pLength)
{
    return ma_decoder_get_length_in_pcm_frames(pDecoder, pLength);
}

void altsound_ma_decoder_uninit(ma_decoder* pDecoder)
{
    ma_decoder_uninit(pDecoder);
}

ma_decoder_config altsound_ma_decoder_config_init(ma_format outputFormat, ma_uint32 outputChannels, ma_uint32 outputSampleRate)
{
    return ma_decoder_config_init(outputFormat, outputChannels, outputSampleRate);
}

ma_result altsound_ma_engine_init_device(ma_uint32 channels, ma_uint32 sampleRate,
    ma_engine_process_proc onProcess, void* pProcessUserData, ma_engine* pEngine)
{
    ma_engine_config config = ma_engine_config_init();
    config.channels = channels;
    config.sampleRate = sampleRate;
    config.onProcess = onProcess;
    config.pProcessUserData = pProcessUserData;

    return ma_engine_init(&config, pEngine);
}

void altsound_ma_engine_uninit(ma_engine* pEngine)
{
    ma_engine_uninit(pEngine);
}

ma_result altsound_ma_engine_start(ma_engine* pEngine)
{
    return ma_engine_start(pEngine);
}

ma_result altsound_ma_engine_stop(ma_engine* pEngine)
{
    return ma_engine_stop(pEngine);
}

ma_result altsound_ma_sound_init_from_decoder(ma_engine* pEngine, ma_decoder* pDecoder, ma_uint32 flags, ma_sound* pSound)
{
    return ma_sound_init_from_data_source(pEngine, (ma_data_source*)pDecoder, flags, NULL, pSound);
}

void altsound_ma_sound_uninit(ma_sound* pSound)
{
    ma_sound_uninit(pSound);
}

ma_result altsound_ma_sound_start(ma_sound* pSound)
{
    return ma_sound_start(pSound);
}

ma_result altsound_ma_sound_stop(ma_sound* pSound)
{
    return ma_sound_stop(pSound);
}

void altsound_ma_sound_set_volume(ma_sound* pSound, float volume)
{
    ma_sound_set_volume(pSound, volume);
}

void altsound_ma_sound_set_looping(ma_sound* pSound, ma_bool32 loop)
{
    ma_sound_set_looping(pSound, loop);
}

ma_result altsound_ma_sound_seek_to_pcm_frame(ma_sound* pSound, ma_uint64 frameIndex)
{
    return ma_sound_seek_to_pcm_frame(pSound, frameIndex);
}

void altsound_ma_sound_set_end_callback(ma_sound* pSound, ma_sound_end_proc callback, void* pUserData)
{
    ma_sound_set_end_callback(pSound, callback, pUserData);
}
