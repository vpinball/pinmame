#ifndef INC_S80SOUND2
#define INC_S80SOUND2

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
#define S80BS_ROMLOAD32(start, n, chk) \
  ROM_LOAD(n, start,  0x8000, chk) 

/*-- Gen 1: 3 x 8K Sound CPU Roms --*/
#define S80BSSOUND888(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
    ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, S80_MEMREG_SCPU2) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)\
    ROM_LOAD(n3,0xc000,0x2000, chk3)

/*-- Gen 2 : 2 x 8K Sound CPU Roms --*/
#define S80BSSOUND88(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
	ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, S80_MEMREG_SCPU2) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)

/*-- Gen 2 & 3: 2 x 32K Sound CPU Roms --*/
#define S80BSSOUND3232(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
    S80BS_ROMLOAD32(0x8000, n1, chk1) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU2) \
    S80BS_ROMLOAD32(0x8000, n2, chk2)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  S80Bs1_sreadmem[];
extern const struct Memory_WriteAddress S80Bs1_swritemem[];
extern const struct Memory_ReadAddress  S80Bs1_sreadmem2[];
extern const struct Memory_WriteAddress S80Bs1_swritemem2[];
extern const struct Memory_ReadAddress  S80Bs2_sreadmem[];
extern const struct Memory_WriteAddress S80Bs2_swritemem[];
extern const struct Memory_ReadAddress  S80Bs2_sreadmem2[];
extern const struct Memory_WriteAddress S80Bs2_swritemem2[];
extern const struct Memory_ReadAddress  S80Bs3_sreadmem[];
extern const struct Memory_WriteAddress S80Bs3_swritemem[];
extern const struct Memory_ReadAddress  S80Bs3_sreadmem2[];
extern const struct Memory_WriteAddress S80Bs3_swritemem2[];

extern struct DACinterface      S80Bs_dacInt;
extern struct AY8910interface   S80Bs_ay8910Int;
extern struct YM2151interface   S80Bs_ym2151Int;
extern struct Samplesinterface	samples_interface;

/*-- Sound interface communications --*/
extern void S80Bs_sound_init(void);
extern void S80Bs_sound_exit(void);
extern void S80Bs_soundlatch(int data);

/****************/
/* GENERATION 1 */
/****************/
#define S80BS1_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs1_sreadmem, S80Bs1_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define S80BS1_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs1_sreadmem2, S80Bs1_swritemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define S80BS1_SOUND \
{ SOUND_DAC,     &S80Bs_dacInt }, \
{ SOUND_AY8910,  &S80Bs_ay8910Int }, \
{ SOUND_SAMPLES, &samples_interface}

/****************/
/* GENERATION 2 */
/****************/
#define S80BS2_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs2_sreadmem, S80Bs2_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}
#define S80BS2_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs2_sreadmem2, S80Bs2_swritemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define S80BS2_SOUND \
{ SOUND_DAC,     &S80Bs_dacInt }, \
{ SOUND_AY8910,  &S80Bs_ay8910Int }, \
{ SOUND_SAMPLES, &samples_interface}

/****************/
/* GENERATION 3 */
/****************/
#define S80BS3_SOUNDCPU1 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs3_sreadmem, S80Bs3_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}
#define S80BS3_SOUNDCPU2 ,{ \
  CPU_M6502 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  S80Bs3_sreadmem2, S80Bs3_swritemem2, 0, 0, \
  ignore_interrupt, 0 \
}
#define S80BS3_SOUND \
{ SOUND_DAC,     &S80Bs_dacInt }, \
{ SOUND_YM2151,  &S80Bs_ym2151Int }, \
{ SOUND_SAMPLES, &samples_interface}


#endif /* INC_S80SOUND2 */
