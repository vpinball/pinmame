#ifndef S80SOUND1_H
#define S80SOUND1_H

/*-- Sound/Speak Board, 2 X 2K Voice/Sound Roms --*/
#define S80SOUND22_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x7000,  0x0800, chk1) \
    ROM_RELOAD(  0xf000,  0x0800) \
	ROM_LOAD(n2, 0x7800,  0x0800, chk2) \
    ROM_RELOAD(  0xf800,  0x0800) 

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  S80SS_sreadmem[];
extern const struct Memory_WriteAddress S80SS_swritemem[];

#define S80SS_SOUNDCPU { \
  CPU_M6502 | CPU_AUDIO_CPU, \
  1000000, /* 1 MHz */ \
  S80SS_sreadmem, S80SS_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}

void S80SS_sinit(int num);
void S80SS_sexit();
void sys80_sound_latch_ss(int data);

#endif /* S80SOUND1_H */
