#pragma once

#define BUILD_YMF262 HAS_YMF262


typedef void (*OPL3_TIMERHANDLER)(int channel,double interval_Sec);
typedef void (*OPL3_IRQHANDLER)(int param,int irq);
typedef void (*OPL3_UPDATEHANDLER)(int param,int min_interval_us);



#if BUILD_YMF262

int  YMF262Init(int num, double clock, double rate);
void YMF262Shutdown(void);
void YMF262ResetChip(int which);
int  YMF262Write(int which, UINT8 a, UINT8 v);
unsigned char YMF262Read(int which, UINT8 a);
int  YMF262TimerOver(int which, UINT8 c);
void YMF262UpdateOne(int which, INT16 **buffers, int length);

void YMF262SetTimerHandler(int which, OPL3_TIMERHANDLER TimerHandler, int channelOffset);
void YMF262SetIRQHandler(int which, OPL3_IRQHANDLER IRQHandler, int param);
void YMF262SetUpdateHandler(int which, OPL3_UPDATEHANDLER UpdateHandler, int param);

void ymf262_set_mute_mask(void* chip, UINT32 MuteMask);
void ymf262_set_volume(void* chip, INT32 volume);
void ymf262_set_vol_lr(void* chip, INT32 volLeft, INT32 volRight);

#endif
