#ifndef GTSS80S_H
#define GTSS80S_H

/*-- Sound Only Board, 1 X 1K Sound Rom, 6530 System sound rom --*/
#define GTS80S1K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x0400,  0x0400, chk1) \
    ROM_RELOAD(  0x0800,  0x0400) \
	ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

/*-- Sound Only Board, 1 X 2K Sound Rom --*/
#define GTS80S2K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x0000,  0x0800, chk1) \
    ROM_RELOAD(  0xf000,  0x0800) \
    ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

#define SOUND_ROMS_NOT_AVAILABLE \
	SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  GTS80S_readmem[];
extern const struct Memory_WriteAddress GTS80S_writemem[];

#define GTS80S_SOUNDCPU { \
  CPU_M6502 | CPU_AUDIO_CPU, /* actually, it is an 6503 (6502 with only 12 address lines) */ \
  1000000, /* 1 MHz ??? */ \
  GTS80S_readmem, GTS80S_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

void GTS80S_init(int num);
void GTS80S_exit(void);
extern void GTS80S_sound_latch(int data);

#endif /* GTSS80S_H */
