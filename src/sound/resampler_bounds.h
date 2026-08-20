#ifndef PINMAME_RESAMPLER_BOUNDS_H
#define PINMAME_RESAMPLER_BOUNDS_H

#include <stddef.h>
#include <stdint.h>

/**
 * Bounds checking and clamp macros for audio resamplers in PinMAME.
 * Protects against ring-buffer overruns during dynamic sample rate transitions.
 */

#define PINMAME_AUDIO_CLAMP(val, min_val, max_val) \
    (((val) < (min_val)) ? (min_val) : (((val) > (max_val)) ? (max_val) : (val)))

#define PINMAME_CHECK_RING_BUFFER_FIT(write_offset, chunk_size, buffer_capacity) \
    (((size_t)(write_offset) + (size_t)(chunk_size)) <= (size_t)(buffer_capacity))

static inline int16_t pinmame_clamp_sample16(int32_t sample) {
    if (sample > 32767) return 32767;
    if (sample < -32768) return -32768;
    return (int16_t)sample;
}

#endif /* PINMAME_RESAMPLER_BOUNDS_H */
