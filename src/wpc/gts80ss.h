#ifndef GTS80SS_H
#define GTS80SS_H

/*-- Sound/Speak Board, 2 X 2K Voice/Sound Roms --*/
#define GTS80SS22_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x7000,  0x0800, chk1) \
	ROM_LOAD(n2, 0x7800,  0x0800, chk2) 

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  GTS80SS_readmem[];
extern const struct Memory_WriteAddress GTS80SS_writemem[];

#define GTS80SS_SOUNDCPU { \
  CPU_M6502 | CPU_AUDIO_CPU, \
  1000000, /* 1 MHz */ \
  GTS80SS_readmem, GTS80SS_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

void GTS80SS_init(int num);
void GTS80SS_exit(void);
void GTS80SS_sound_latch(int data);

#endif /* GTS80SS_H */
