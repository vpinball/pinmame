#ifndef INC_WMSSND
#define INC_WMSSND
/* DCS sound needs this one */
#include "cpu/adsp2100/adsp2100.h"

extern const struct Memory_ReadAddress  s67s_readmem[];
extern const struct Memory_WriteAddress s67s_writemem[];
extern struct DACinterface      s67s_dacInt;
extern struct hc55516_interface s67s_hc55516Int;

#define S67S_CPUNO        1
#define S67S_MEMREG_SCPU  REGION_CPU2


#define S67S_SOUND { SOUND_DAC,     &s67s_dacInt }, \
                   { SOUND_HC55516, &s67s_hc55516Int }, \
                   SAMPLESINTERFACE

#define S67S_SOUNDCPU { \
  CPU_M6808 | CPU_AUDIO_CPU, 3579000/4, \
  s67s_readmem,s67s_writemem,0,0, \
  ignore_interrupt,1 \
}

#define S67S_SOUNDROMS0(ic12, chk12) \
  SOUNDREGION(0x10000, S67S_MEMREG_SCPU) \
    ROM_LOAD(ic12, 0x7000, 0x1000, chk12) \
    ROM_RELOAD(    0xf000, 0x1000)

#define S67S_SOUNDROMS8(ic12, chk12) \
  SOUNDREGION(0x10000, S67S_MEMREG_SCPU) \
    ROM_LOAD(ic12, 0x7800, 0x0800, chk12) \
    ROM_RELOAD(    0xf800, 0x0800)

#define S67S_SPEECHROMS0000(ic7,chk7, ic5,chk5, ic6,chk6, ic4, chk4) \
    ROM_LOAD(ic7, 0x3000, 0x1000, chk7) \
    ROM_RELOAD(   0xb000, 0x1000) \
    ROM_LOAD(ic5, 0x4000, 0x1000, chk5) \
    ROM_RELOAD(   0xc000, 0x1000) \
    ROM_LOAD(ic6, 0x5000, 0x1000, chk6) \
    ROM_RELOAD(   0xd000, 0x1000) \
    ROM_LOAD(ic4, 0x6000, 0x1000, chk4) \
    ROM_RELOAD(   0xe000, 0x1000)

#define S67S_SPEECHROMS000x(ic7,chk7, ic5,chk5, ic6,chk6) \
    ROM_LOAD(ic7, 0x3000, 0x1000, chk7) \
    ROM_RELOAD(   0xb000, 0x1000) \
    ROM_LOAD(ic5, 0x4000, 0x1000, chk5) \
    ROM_RELOAD(   0xc000, 0x1000) \
    ROM_LOAD(ic6, 0x5000, 0x1000, chk6) \
    ROM_RELOAD(   0xd000, 0x1000)


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

#define WPCS_CPUNO 1
#define WPCS_CPUREGION (REGION_CPU1+WPCS_CPUNO)
#define WPCS_ROMREGION (REGION_SOUND1)

/*-- Sound rom macros --*/
#define WPCS_STDREG \
  SOUNDREGION(0x010000, WPCS_CPUREGION) \
  SOUNDREGION(0x180000, WPCS_ROMREGION)

#define WPCS_ROMLOAD2(start, n, chk) \
  ROM_LOAD(n, start,  0x20000, chk) \
    ROM_RELOAD( start + 0x20000, 0x20000) \
    ROM_RELOAD( start + 0x40000, 0x20000) \
    ROM_RELOAD( start + 0x60000, 0x20000)

#define WPCS_ROMLOAD4(start, n, chk) \
  ROM_LOAD(n, start,  0x40000, chk) \
    ROM_RELOAD( start + 0x40000, 0x40000)

#define WPCS_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start, 0x80000, chk)

#define WPCS_SOUNDROM882(u18,chk18,u15,chk15,u14,chk14) \
  WPCS_STDREG \
  WPCS_ROMLOAD8(0x000000, u18, chk18) \
  WPCS_ROMLOAD8(0x080000, u15, chk15) \
  WPCS_ROMLOAD2(0x100000, u14, chk14)
#define WPCS_SOUNDROM288(u18,chk18,u15,chk15,u14,chk14) \
   WPCS_STDREG \
   WPCS_ROMLOAD2(0x000000, u18, chk18) \
   WPCS_ROMLOAD8(0x080000, u15, chk15) \
   WPCS_ROMLOAD8(0x100000, u14, chk14)
#define WPCS_SOUNDROM222(u18,chk18,u15,chk15,u14,chk14) \
   WPCS_STDREG \
   WPCS_ROMLOAD2(0x000000, u18, chk18) \
   WPCS_ROMLOAD2(0x080000, u15, chk15) \
   WPCS_ROMLOAD2(0x100000, u14, chk14)
#define WPCS_SOUNDROM224(u18,chk18,u15,chk15,u14,chk14) \
   WPCS_STDREG \
   WPCS_ROMLOAD2(0x000000, u18, chk18) \
   WPCS_ROMLOAD2(0x080000, u15, chk15) \
   WPCS_ROMLOAD4(0x100000, u14, chk14)
#define WPCS_SOUNDROM248(u18,chk18,u15,chk15,u14,chk14) \
   WPCS_STDREG \
   WPCS_ROMLOAD2(0x000000, u18, chk18) \
   WPCS_ROMLOAD4(0x080000, u15, chk15) \
   WPCS_ROMLOAD8(0x100000, u14, chk14)
#define WPCS_SOUNDROM84x(u18,chk18,u15,chk15) \
   WPCS_STDREG \
   WPCS_ROMLOAD8(0x000000, u18, chk18) \
   WPCS_ROMLOAD4(0x080000, u15, chk15)
#define WPCS_SOUNDROM22x(u18,chk18,u15,chk15) \
   WPCS_STDREG \
   WPCS_ROMLOAD2(0x000000, u18, chk18) \
   WPCS_ROMLOAD2(0x080000, u15, chk15)
#define WPCS_SOUNDROM888(u18,chk18,u15,chk15,u14,chk14) \
   WPCS_STDREG \
   WPCS_ROMLOAD8(0x000000, u18, chk18) \
   WPCS_ROMLOAD8(0x080000, u15, chk15) \
   WPCS_ROMLOAD8(0x100000, u14, chk14)
#define WPCS_SOUNDROM8xx(u18,chk18) \
   WPCS_STDREG \
   WPCS_ROMLOAD8(0x000000, u18, chk18)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  wpcs_readmem[];
extern const struct Memory_WriteAddress wpcs_writemem[];

extern struct DACinterface      wpcs_dacInt;
extern struct YM2151interface   wpcs_ym2151Int;
extern struct hc55516_interface wpcs_hc55516Int;

#define WPCS_SOUNDCPU ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  wpcs_readmem, wpcs_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define WPCS_SOUND \
  { SOUND_YM2151,  &wpcs_ym2151Int }, { SOUND_DAC,     &wpcs_dacInt }, \
  { SOUND_HC55516, &wpcs_hc55516Int }, SAMPLESINTERFACE


/*-- Sound ROM macros --*/
/*-- standard regions --*/
#define DCS_STDREG(size) \
   SOUNDREGION(ADSP2100_SIZE, WPC_MEMREG_SCPU) \
   SOUNDREGION(0x1000*2,      WPC_MEMREG_SBANK) \
   SOUNDREGION(size,          WPC_MEMREG_SROM)
#define DCS_ROMLOADx(start, n, chk) \
   ROM_LOAD(n, start, 0x080000, chk) ROM_RELOAD(start+0x080000, 0x080000)
#define DCS_ROMLOADm(start, n,chk) \
   ROM_LOAD(n, start, 0x100000, chk)

/*-- Games use different number of ROMS and different sizes --*/
#define DCS_SOUNDROM1x(n2,chk2) \
   DCS_STDREG(0x100000) \
   DCS_ROMLOADx(0x000000,n2,chk2)

#define DCS_SOUNDROM1m(n2,chk2) \
   DCS_STDREG(0x100000) \
   DCS_ROMLOADm(0x000000,n2,chk2)

#define DCS_SOUNDROM3m(n2,chk2,n3,chk3,n4,chk4) \
   DCS_STDREG(0x300000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4)

#define DCS_SOUNDROM4xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
   DCS_STDREG(0x400000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5)

#define DCS_SOUNDROM4mx(n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
   DCS_STDREG(0x400000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5)

#define DCS_SOUNDROM5xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6)

#define DCS_SOUNDROM5x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6)

#define DCS_SOUNDROM5m(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6)

#define DCS_SOUNDROM6x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7)

#define DCS_SOUNDROM6m(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6) \
   DCS_ROMLOADm(0x500000,n7,chk7)

#define DCS_SOUNDROM6xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6) \
   DCS_ROMLOADm(0x500000,n7,chk7)

#define DCS_SOUNDROM7x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8) \
   DCS_STDREG(0x700000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7) \
   DCS_ROMLOADx(0x600000,n8,chk8)

#define DCS_SOUNDROM8x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8,n9,chk9) \
   DCS_STDREG(0x800000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7) \
   DCS_ROMLOADx(0x600000,n8,chk8) \
   DCS_ROMLOADx(0x700000,n9,chk9)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress16  dcs2_readmem[];
extern const struct Memory_WriteAddress16 dcs2_writemem[];
extern const struct Memory_ReadAddress16  dcs1_readmem[];
extern const struct Memory_WriteAddress16 dcs1_writemem[];

extern struct CustomSound_interface dcs_custInt;

#define DCS1_SOUNDCPU ,{ \
  CPU_ADSP2105 | CPU_AUDIO_CPU,	\
  10240000, /* 10.24 MHz */ \
  dcs1_readmem, dcs1_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define DCS2_SOUNDCPU ,{ \
  CPU_ADSP2105 | CPU_AUDIO_CPU,	\
  10240000, /* 10.24 MHz */ \
  dcs2_readmem, dcs2_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define DCS_SOUND { SOUND_CUSTOM, &dcs_custInt }, SAMPLESINTERFACE

#endif /* INC_WMSSND */

