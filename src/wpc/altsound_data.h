#ifndef ALTSOUND_DATA_HPP
  #define ALTSOUND_DATA_HPP
  #if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
    #pragma once
  #endif

#include <stdio.h>

// ---------------------------------------------------------------------------
// Logging support
// ---------------------------------------------------------------------------

#if (defined(_DEBUG) && defined(VPINMAME))
  #ifdef __cplusplus
	extern "C" {
  #endif

  extern void logerror(const char *text, ...);
  
  #ifdef __cplusplus
    }
  #endif

  #ifdef LOG
    #undef LOG
  #endif
  #define LOG(x) logerror x;
  #define FORMAT_OUTPUT 1 //0 DAR_DEBUG
#else
  #ifdef LOG
    #undef LOG
  #endif
  #define LOG(x)
#endif // VPinMAME DEBUG

#if FORMAT_OUTPUT
  #define INDENT indent_level++; indent[0] = '\0'; \
    for (int i=0; i < indent_level; i++) {\
      indent[i] = '\t';\
      indent[i+1] = '\0';\
    }

  #define OUTDENT indent_level--; indent_level = indent_level < 0?0:indent_level;\
    indent[0] = '\0'; \
    for (int i=0; i < indent_level; i++) {\
      indent[i] = '\t';\
      indent[i + 1] = '\0';\
    }

  static int indent_level = 0;
  static int indent_idx = 0;
#else
  #define INDENT
  #define OUTDENT
#endif

static char indent[100] = { '\0' }; // ensure null termination for empty string

// ---------------------------------------------------------------------------
// Data Structures
// ---------------------------------------------------------------------------

// Structure for holding parsed sample data
typedef struct _pin_samples {
	int * ID;
	char ** files_with_subpath;
	signed char * channel;
	float * gain;
	float * ducking;
	unsigned char * loop;
	unsigned char * stop;
	unsigned int num_files;
} PinSamples;

#endif //ALTSOUND_DATA_HPP