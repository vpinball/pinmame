#ifndef ALTSOUND_DATA_HPP
  #define ALTSOUND_DATA_HPP
  #if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
    #pragma once
  #endif

#include <stdio.h>
#include "..\ext\bass\bass.h"
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
#else
  #ifdef LOG
    #undef LOG
  #endif
  #define LOG(x)
#endif // VPinMAME DEBUG

// ---------------------------------------------------------------------------
// Data Structures
// ---------------------------------------------------------------------------

// For command parsing
#define ALT_MAX_CMDS 4
#define BASS_NO_STREAM 0
#define ALT_MAX_CHANNELS 16

// Structure for CSV parsing
typedef struct _csv_reader {
	FILE* f;
	int delimiter;
	int n_header_fields;
	char** header_fields;	// header split in fields
	int n_fields;
	char** fields;			// current row split in fields
} CsvReader;

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

// Structure for command data
typedef struct _cmd_data {
	unsigned int cmd_counter;
	int stored_command;
	unsigned int cmd_filter;
	unsigned int cmd_buffer[ALT_MAX_CMDS];
} CmdData;

#ifdef __cplusplus
typedef struct _stream_info {
	_stream_info() : handle(0), stream_type(static_cast<AltsoundSampleType>(0)), channel_idx(0){}
	unsigned long handle;
	enum AltsoundSampleType stream_type;
	unsigned int channel_idx;
} AltsoundStreamInfo;

enum AltsoundSampleType {
	UNDEFINED,
	MUSIC,
	JINGLE,
	SFX,
};
#endif

// ---------------------------------------------------------------------------
// Helper function to translate BASS error codes to printable strings
// ---------------------------------------------------------------------------

static const char * bass_err_names[] = {
	"BASS_OK",
	"BASS_ERROR_MEM",
	"BASS_ERROR_FILEOPEN",
	"BASS_ERROR_DRIVER",
	"BASS_ERROR_BUFLOST",
	"BASS_ERROR_HANDLE",
	"BASS_ERROR_FORMAT",
	"BASS_ERROR_POSITION",
	"BASS_ERROR_INIT",
	"BASS_ERROR_START",
	"BASS_ERROR_SSL",
	"BASS_ERROR_REINIT",
	"BASS_ERROR_ALREADY",
	"BASS_ERROR_NOTAUDIO",
	"BASS_ERROR_NOCHAN",
	"BASS_ERROR_ILLTYPE",
	"BASS_ERROR_ILLPARAM",
	"BASS_ERROR_NO3D",
	"BASS_ERROR_NOEAX",
	"BASS_ERROR_DEVICE",
	"BASS_ERROR_NOPLAY",
	"BASS_ERROR_FREQ",
	"BASS_ERROR_NOTFILE",
	"BASS_ERROR_NOHW",
	"BASS_ERROR_EMPTY",
	"BASS_ERROR_NONET",
	"BASS_ERROR_CREATE",
	"BASS_ERROR_NOFX",
	"BASS_ERROR_NOTAVAIL",
	"BASS_ERROR_DECODE",
	"BASS_ERROR_DX",
	"BASS_ERROR_TIMEOUT",
	"BASS_ERROR_FILEFORM",
	"BASS_ERROR_SPEAKER",
	"BASS_ERROR_VERSION",
	"BASS_ERROR_CODEC",
	"BASS_ERROR_ENDED",
	"BASS_ERROR_BUSY",
	"BASS_ERROR_UNSTREAMABLE",
	"BASS_ERROR_PROTOCOL",
	"BASS_ERROR_DENIED"
	//   [BASS_ERROR_UNKNOWN]      = ""
};

static const char* get_bass_err() {
	// returns string representation of BASS error codes
	int err = BASS_ErrorGetCode();
	if (err < 0) {
		return "BASS_ERROR_UNKNOWN";
	}
	else {
		return bass_err_names[err];
	}
}

#endif //ALTSOUND_DATA_HPP