#ifndef VOTRAX_H
#define VOTRAX_H

extern struct CustomSound_interface votrax_custInt;

#define VOTRAXINTERFACE {SOUND_CUSTOM, &votrax_custInt}

void votrax_w(int data);
int votrax_status_r(void);
void votrax_repeat_w(int dummy);
void votrax_set_busy_func(void (*busy_func)(int state));
void votrax_set_base_freqency(int baseFrequency);

#endif