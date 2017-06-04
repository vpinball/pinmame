#ifndef _SOUND_H
#define _SOUND_H

int lisy80_sound_stream_init(void);
void lisy80_play_wav(int sound_no);

#define LISY80_SOUND_STATUS_IDLE 1
#define LISY80_SOUND_STATUS_RUNNING 2
#define LISY80_SOUND_STATUS_RUNNING_PROTECTED 3

#define LISY80_SOUND_PATH "/boot/lisy80/sounds/"

#define PCM_DEVICE "default"

#endif  // _SOUND_H

