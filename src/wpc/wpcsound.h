#ifndef INC_WPCSOUND
#define INC_WPCSOUND

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

#endif /* INC_WPCSOUND */
