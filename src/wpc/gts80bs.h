#ifndef INC_GTS80BS
#define INC_GTS80BS

/* System 80B Sound board Hardware Versions:
   -----------------------------------------
   ALL   - CPU: 2x(6502): DAC: 2x(AD7528)
   --------------------------------------
   Gen 1 - DSP: 2x(AY8913): OTHER: SP0-250 (SPEECH GENERATOR)    (Not Emulated)
   Gen 2 - DSP: 2x(AY8913): OTHER: Programmable Capacitor Filter (Not Emulated)
   Gen 3 - DSP: 1x(YM2151): OTHER: None
*/   

/*-- Sound rom macros --*/

/* Load 32k Rom Space */
#define GTS80BS_ROMLOAD32(start, n, chk) \
  ROM_LOAD(n, start,  0x8000, chk) 

/*-- Gen 1: 3 x 8K Sound CPU Roms --*/
#define GTS80BSSOUND888(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)\
    ROM_LOAD(n3,0xc000,0x2000, chk3)

/*-- Gen 2 : 2 x 8K Sound CPU Roms --*/
#define GTS80BSSOUND88(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
	ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)

/*-- Gen 2 & 3: 2 x 32K Sound CPU Roms --*/
#define GTS80BSSOUND3232(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    GTS80BS_ROMLOAD32(0x8000, n1, chk1) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    GTS80BS_ROMLOAD32(0x8000, n2, chk2)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  GTS80BS1_readmem[];
extern const struct Memory_WriteAddress GTS80BS1_writemem[];
extern const struct Memory_ReadAddress  GTS80BS1_readmem2[];
extern const struct Memory_WriteAddress GTS80BS1_writemem2[];
extern const struct Memory_ReadAddress  GTS80BS2_readmem[];
extern const struct Memory_WriteAddress GTS80BS2_writemem[];
extern const struct Memory_ReadAddress  GTS80BS2_readmem2[];
extern const struct Memory_WriteAddress GTS80BS2_writemem2[];
extern const struct Memory_ReadAddress  GTS80BS3_readmem[];
extern const struct Memory_WriteAddress GTS80BS3_writemem[];
extern const struct Memory_ReadAddress  GTS80BS3_readmem2[];
extern const struct Memory_WriteAddress GTS80BS3_writemem2[];

extern struct DACinterface      GTS80BS_dacInt;
extern struct AY8910interface   GTS80BS_ay8910Int;
extern struct YM2151interface   GTS80BS_ym2151Int;
extern struct Samplesinterface	samples_interface;

/*-- Sound interface communications --*/
extern void GTS80BS_init(void);
extern void GTS80BS_exit(void);
extern void GTS80BS_sound_latch(int data);

/****************/
/* GENERATION 1 */
/****************/
#define GTS80BS1_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS1_readmem, GTS80BS1_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define GTS80BS1_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS1_readmem2, GTS80BS1_writemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define GTS80BS1_SOUND \
{ SOUND_DAC,     &GTS80BS_dacInt }, \
{ SOUND_AY8910,  &GTS80BS_ay8910Int }, \
{ SOUND_SAMPLES, &samples_interface}

/****************/
/* GENERATION 2 */
/****************/
#define GTS80BS2_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS2_readmem, GTS80BS2_writemem, 0, 0, \
  ignore_interrupt, 0 \
}
#define GTS80BS2_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS2_readmem2, GTS80BS2_writemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define GTS80BS2_SOUND \
{ SOUND_DAC,     &GTS80BS_dacInt }, \
{ SOUND_AY8910,  &GTS80BS_ay8910Int }, \
{ SOUND_SAMPLES, &samples_interface}

/****************/
/* GENERATION 3 */
/****************/
#define GTS80BS3_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS3_readmem, GTS80BS3_writemem, 0, 0, \
  ignore_interrupt, 0 \
}
#define GTS80BS3_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS80BS3_readmem2, GTS80BS3_writemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define GTS80BS3_SOUND \
{ SOUND_DAC,     &GTS80BS_dacInt }, \
{ SOUND_YM2151,  &GTS80BS_ym2151Int }, \
{ SOUND_SAMPLES, &samples_interface}


#endif /* INC_GTS80BS */
