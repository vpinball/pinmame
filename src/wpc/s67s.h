#ifndef INC_S67S
#define INC_S67S

extern const struct Memory_ReadAddress  s67s_readmem[];
extern const struct Memory_WriteAddress s67s_writemem[];
extern struct DACinterface      s67s_dacInt;
extern struct hc55516_interface s67s_hc55516Int;
extern struct Samplesinterface  samples_interface;

#define S67S_CPUNO        1
#define S67S_MEMREG_SCPU  REGION_CPU2


#define S67S_SOUND \
  { SOUND_DAC,     &s67s_dacInt }, \
  { SOUND_HC55516, &s67s_hc55516Int }, \
  { SOUND_SAMPLES, &samples_interface}

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

extern WRITE_HANDLER(s67s_cmd);
extern void s67s_init(void);
extern void s67s_exit(void);

#endif /* INC_S67S */
