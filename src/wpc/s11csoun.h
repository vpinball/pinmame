#ifndef INC_S11CSOUN
#define INC_S11CSOUN

#define S11CS_CPUNO     1
#define S11CS_CPUREGION (REGION_CPU1+S11CS_CPUNO)
#define S11CS_ROMREGION (REGION_SOUND1)

/*-- Sound rom macros --*/
/* 150401 Added SOUNDROM008 */
#define S11CS_STDREG \
  SOUNDREGION(0x10000, S11CS_CPUREGION) \
  SOUNDREGION(0x30000, S11CS_ROMREGION)

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

/*-- Sound on CPU board --*/
#define S11S_CPUNO 2
#define S11S_CPUREGION (REGION_CPU1+S11S_CPUNO)
#define S11S_ROMREGION (REGION_SOUND2)

#define S11S_STDREG \
  SOUNDREGION(0x10000, S11S_CPUREGION) \
  SOUNDREGION(0x10000, S11S_ROMREGION)

#define S11S_SOUNDROM44(n1, chk1, n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n1, 0x4000, 0x4000, chk1) \
    ROM_LOAD(n2, 0xc000, 0x4000, chk2) \
      ROM_RELOAD(  0x8000, 0x4000)

#define S11S_SOUNDROMx8(n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n2, 0x4000, 0x4000, chk2) \
      ROM_CONTINUE(0xc000, 0x4000)

#define S11S_SOUNDROM88(n1, chk1, n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
    ROM_LOAD(n2, 0x8000, 0x8000, chk2)

/*-- S9 Sound on CPU board --*/
#define S9S_CPUNO     1
#define S9S_CPUREGION (REGION_CPU1+S9S_CPUNO)

#define S9S_STDREG SOUNDREGION(0x10000, S9S_CPUREGION)

#define S9S_SOUNDROM41111(u49,chk49, u4,chk4, u5,chk5, u6,chk6, u7,chk7) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49)  \
     ROM_LOAD(u7,  0x8000, 0x1000, chk7)  \
     ROM_LOAD(u5,  0x9000, 0x1000, chk5)  \
     ROM_LOAD(u6,  0xa000, 0x1000, chk6)  \
     ROM_LOAD(u4,  0xb000, 0x1000, chk4)

#define S9S_SOUNDROM4111(u49,chk49, u4,chk4, u5,chk5, u6,chk6) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49)  \
     ROM_LOAD(u5,  0x9000, 0x1000, chk5) \
     ROM_LOAD(u6,  0xa000, 0x1000, chk6) \
     ROM_LOAD(u4,  0xb000, 0x1000, chk4)

#define S9S_SOUNDROM4(u49,chk49) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49) \
     ROM_RELOAD(0x8000, 0x4000) \
     ROM_RELOAD(0x4000, 0x4000) \
     ROM_RELOAD(0x0000, 0x4000)

/*-- Jokerz sound on CPU board --*/
#define S11B3S_CPUNO 1
#define S11B3S_CPUREGION (REGION_CPU1+S11B3S_CPUNO)
#define S11B3S_ROMREGION (REGION_SOUND1)

#define S11B3S_STDREG \
  SOUNDREGION(0x10000, S11B3S_CPUREGION) \
  SOUNDREGION(0x10000, S11B3S_ROMREGION)

#define S11B3S_SOUNDROM881(n1, chk1, n2, chk2, n3, chk3) \
  S11B3S_STDREG \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
    ROM_LOAD(n2, 0x8000, 0x8000, chk2)

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

#define S11B3_SOUND S9_SOUND

#endif /* INC_S11CSOUN */
