#ifndef INC_HANKINSND
#define INC_HANKINSND

/* Hankin Sound Hardware Info:

  Sound System
  ------------
  M6802
  WAVEFORM ROM

*/

extern const struct Memory_ReadAddress  hnks_readmem[];
extern const struct Memory_WriteAddress hnks_writemem[];

#define HNK_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 327680,	/* 0.327680 MHz */					\
  hnks_readmem, hnks_writemem, 0, 0, \
  0,1 \
}

extern struct CustomSound_interface hnks_custInt;

#define HNK_SOUND {SOUND_CUSTOM,&hnks_custInt}, SAMPLESINTERFACE

#define HNK_SOUNDROMS(ic14,chk14,ic3,chk3) \
  SOUNDREGION(0x10000, HNK_MEMREG_SCPU) \
    ROM_LOAD(ic14, 0x1000, 0x0800, chk14) \
	    ROM_RELOAD(0xf800, 0x0800) \
    ROM_LOAD(ic3,  0xf000, 0x0200, chk3)

#endif /* INC_HANKINSND */


