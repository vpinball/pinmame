#ifndef S80SOUND0_H
#define S80SOUND0_H

/*-- Sound Only Board, 1 X 1K Sound Rom, 6530 System sound rom --*/
#define S80SOUND1K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x0400,  0x0400, chk1) \
    ROM_RELOAD(  0x0800,  0x0400) \
	ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

/*-- Sound Only Board, 1 X 2K Sound Rom --*/
#define S80SOUND2K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x0000,  0x0800, chk1) \
    ROM_RELOAD(  0xf000,  0x0800) \
    ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

#define SOUND_ROMS_NOT_AVAILABLE \
	SOUNDREGION(0x10000, S80_MEMREG_SCPU1)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  S80S_sreadmem[];
extern const struct Memory_WriteAddress S80S_swritemem[];

#define S80S_SOUNDCPU { \
  CPU_M6502 | CPU_AUDIO_CPU, /* actually, it is an 6503 (6502 with only 12 address lines) */ \
  1000000, /* 1 MHz ??? */ \
  S80S_sreadmem, S80S_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}

void S80S_sinit(int num);
extern void sys80_sound_latch_s(int data);

#endif /* S80SOUND0_H */
