#ifndef INC_S11CSOUN
#define INC_S11CSOUN

/*-- Sound rom macros --*/
/* 150401 Added SOUNDROM008 */
#define S11CS_STDREG \
  SOUNDREGION(0x10000, S11_MEMREG_SCPU1) \
  SOUNDREGION(0x30000, S11_MEMREG_SROM1)

#define S11CS_STDREGWPC \
  SOUNDREGION(0x10000, WPC_MEMREG_SCPU) \
  SOUNDREGION(0x30000, WPC_MEMREG_SROM)

#define S11CS_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start, 0x8000, chk) \
  ROM_RELOAD(start+0x8000, 0x8000)

#define S11CS_ROMLOAD0(start, n, chk) \
  ROM_LOAD(n, start, 0x10000, chk)

#define S11CS_SOUNDROM000(n1,chk1,n2,chk2,n3,chk3) \
  S11CS_STDREG \
  S11CS_ROMLOAD0(0x00000, n1, chk1) \
  S11CS_ROMLOAD0(0x10000, n2, chk2) \
  S11CS_ROMLOAD0(0x20000, n3, chk3)

#define S11CS_SOUNDROMWPC000(n1,chk1,n2,chk2,n3,chk3) \
  S11CS_STDREGWPC \
  S11CS_ROMLOAD0(0x00000, n1, chk1) \
  S11CS_ROMLOAD0(0x10000, n2, chk2) \
  S11CS_ROMLOAD0(0x20000, n3, chk3)

#define S11CS_SOUNDROM008(n1,chk1,n2,chk2,n3,chk3) \
  S11CS_STDREG \
  S11CS_ROMLOAD0(0x00000, n1, chk1) \
  S11CS_ROMLOAD0(0x10000, n2, chk2) \
  S11CS_ROMLOAD8(0x20000, n3, chk3)

#define S11CS_SOUNDROM888(n1,chk1,n2,chk2,n3,chk3) \
  S11CS_STDREG \
  S11CS_ROMLOAD8(0x00000, n1, chk1) \
  S11CS_ROMLOAD8(0x10000, n2, chk2) \
  S11CS_ROMLOAD8(0x20000, n3, chk3)

#define S11CS_SOUNDROM88(n1,chk1,n2,chk2) \
  S11CS_STDREG \
  S11CS_ROMLOAD8(0x00000, n1, chk1) \
  S11CS_ROMLOAD8(0x10000, n2, chk2)

#define S11CS_SOUNDROM8(n1,chk1) \
  S11CS_STDREG \
  S11CS_ROMLOAD8(0x00000, n1, chk1)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  s11cs_readmem[];
extern const struct Memory_WriteAddress s11cs_writemem[];
extern const struct Memory_ReadAddress  s11s_readmem[];
extern const struct Memory_WriteAddress s11s_writemem[];
extern const struct Memory_ReadAddress  s9s_readmem[];
extern const struct Memory_WriteAddress s9s_writemem[];

extern struct DACinterface      s11_dacInt, s11_dacInt2;
extern struct YM2151interface   s11cs_ym2151Int;
extern struct hc55516_interface s11_hc55516Int, s11_hc55516Int2;

/*-- Sound interface communications --*/
#define S11C_SOUNDCPU { \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000, /* 2 MHz ? */ \
  s11cs_readmem, s11cs_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define S11_SOUNDCPU { \
  CPU_M6808 | CPU_AUDIO_CPU, 1000000, /* 1 Mhz */ \
  s11s_readmem, s11s_writemem, NULL, NULL, \
  NULL, 0, NULL, 0 \
}
#define S9_SOUNDCPU { \
  CPU_M6808 | CPU_AUDIO_CPU, 1000000, /* 1 Mhz */ \
  s9s_readmem, s9s_writemem, NULL, NULL, \
  NULL, 0, NULL, 0 \
}

#define S11C_SOUND \
  { SOUND_YM2151,  &s11cs_ym2151Int }, \
  { SOUND_DAC,     &s11_dacInt }, \
  { SOUND_HC55516, &s11_hc55516Int }, \
  SAMPLESINTERFACE

#define S11_SOUND \
  { SOUND_YM2151,  &s11cs_ym2151Int }, \
  { SOUND_DAC,     &s11_dacInt2 }, \
  { SOUND_HC55516, &s11_hc55516Int2 }, \
  SAMPLESINTERFACE

#define S9_SOUND \
  { SOUND_DAC,     &s11_dacInt }, \
  { SOUND_HC55516, &s11_hc55516Int }, \
  SAMPLESINTERFACE

#endif /* INC_S11CSOUN */
