#ifndef INC_GTS3SOUND
#define INC_GTS3SOUND

/*-- Sound rom macros --*/

/* Load 1Mb Rom(128K) to fit into 4Meg Rom Space */
#define GTS3S_ROMLOAD1(start, n, chk) \
  ROM_LOAD(n, start,  0x20000, chk) \
  ROM_RELOAD( start + 0x20000, 0x20000) \
  ROM_RELOAD( start + 0x40000, 0x20000) \
  ROM_RELOAD( start + 0x60000, 0x20000) 

/* Load 2Mb Rom(256K) to fit into 4Meg Rom Space */
#define GTS3S_ROMLOAD2(start, n, chk) \
  ROM_LOAD(n, start,  0x40000, chk) \
  ROM_RELOAD( start + 0x40000, 0x20000) 

/* Load 4Mb Rom(512K) */
#define GTS3S_ROMLOAD4(start, n, chk) \
  ROM_LOAD(n, start,  0x80000, chk)

/*-- 2 x 32K Sound CPU Roms, 2 x 128K Voice Roms --*/
#define GTS3SOUND32128(n1,chk1,n2,chk2,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x80000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD1(0x0000, n3, chk3) \
	GTS3S_ROMLOAD1(0x0000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2) 

/*-- 2 x 32K Sound CPU Roms, 2 x 256K Voice Roms --*/
#define GTS3SOUND32256(n1,chk1,n2,chk2,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x80000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD2(0x0000, n3, chk3) \
	GTS3S_ROMLOAD2(0x0000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2) 

/*-- 2 x 32K Sound CPU Roms, 1 x 256K, 1 x 512K Voice Roms --*/
#define GTS3SOUND32256512(n1,chk1,n2,chk2,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x80000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD2(0x0000, n3, chk3) \
	GTS3S_ROMLOAD4(0x0000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2) 

/*-- 2 x 32K Sound CPU Roms, 2 x 512K Voice Roms --*/
#define GTS3SOUND32512(n1,chk1,n2,chk2,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x80000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD4(0x0000, n3, chk3) \
	GTS3S_ROMLOAD4(0x0000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2) 

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  GTS3_sreadmem[];
extern const struct Memory_WriteAddress GTS3_swritemem[];
extern const struct Memory_ReadAddress  GTS3_sreadmem2[];
extern const struct Memory_WriteAddress GTS3_swritemem2[];

extern struct DACinterface      GTS3_dacInt;
extern struct YM2151interface   GTS3_ym2151Int;
extern struct OKIM6295interface GTS3_okim6295_interface;
extern struct Samplesinterface	samples_interface;

/*-- Sound interface communications --*/
extern void GTS3_sinit(int num);

#define GTS3_SOUNDCPU1 ,{ \
  CPU_M65C02 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS3_sreadmem, GTS3_swritemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define GTS3_SOUNDCPU2 ,{ \
  CPU_M65C02 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  GTS3_sreadmem2, GTS3_swritemem2, 0, 0, \
  ignore_interrupt, 0 \
}

#define GTS3_SOUND \
{ SOUND_YM2151,  &GTS3_ym2151Int }, \
{ SOUND_DAC,     &GTS3_dacInt }, \
{ SOUND_OKIM6295,&GTS3_okim6295_interface },\
{ SOUND_SAMPLES, &samples_interface}

#endif /* INC_GTS3SOUND */
