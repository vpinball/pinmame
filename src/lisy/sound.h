#ifndef _SOUND_H
#define _SOUND_H

int lisy80_sound_stream_init(void);
void lisy80_play_wav(int sound_no);
int lisy_adjust_volume(void);
int lisy1_sound_stream_init(void);
void lisy1_play_wav(int sound_no);
int lisy35_sound_stream_init(void);
void lisy35_play_wav(int sound_no);
//for mpf
int mpf_sound_stream_init(int debug);
int mpf_play_mp3(Mix_Music *music);

#define LISY80_SOUND_STATUS_IDLE 1
#define LISY80_SOUND_STATUS_RUNNING 2
#define LISY80_SOUND_STATUS_RUNNING_PROTECTED 3

#define MP3_SOUND_PATH "./hardware_sounds"

#define PCM_DEVICE "default"

#define LISY35_SOUND_MAX_ALTERNATIVE_FILES 3
#define LISY35_SOUND_OPTION_NORMAL 0
#define LISY35_SOUND_OPTION_LOOP 1
#define LISY35_SOUND_OPTION_STOP_LOOP 2
#define LISY35_SOUND_OPTION_SPEECH 3

#endif  // _SOUND_H






