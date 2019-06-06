#ifndef _SOUND_H
#define _SOUND_H

int lisy80_sound_stream_init(void);
void lisy80_play_wav(int sound_no);
int lisy_adjust_volume(void);
int lisy1_sound_stream_init(void);
void lisy1_play_wav(int sound_no);
//for mpf
int mpf_sound_stream_init(int debug);
int mpf_play_mp3(Mix_Music *music);

#define LISY80_SOUND_STATUS_IDLE 1
#define LISY80_SOUND_STATUS_RUNNING 2
#define LISY80_SOUND_STATUS_RUNNING_PROTECTED 3

//#define LISY80_SOUND_PATH "/boot/lisy80/sounds/"

//#define MP3_SOUND_PATH "./hardware_sounds"

#define PCM_DEVICE "default"

#endif  // _SOUND_H






