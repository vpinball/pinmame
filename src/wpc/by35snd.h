#ifndef INC_BY35SND
#define INC_BY35SND

/*Bally Sound Hardware Info:

  Sound module -32 & -50 
  ---------------------
  Custom Chips

  Sounds Plus -51
  ---------------
  M6802/M6808
  AY8910 Sound Generator
  ?NO DAC?

  Sounds Plus -56, Vocalizer -57
  ------------------------------
  M6802/M6808
  AY8910 Sound Generator
  HC55516 DAC

  Squalk N Talk -61
  -------------
  M6809 CPU
  TMS5220 Speach Generator
  AY8910 Sound Generator
  DAC

  Cheap Squeak  -45
  ------------
  M6803 CPU
  DAC
*/

/* Squawk n Talk */
extern const struct Memory_ReadAddress snt_readmem[];
extern const struct Memory_WriteAddress snt_writemem[];
extern struct TMS5220interface snt_tms5220Int;
extern struct DACinterface     snt_dacInt;
extern struct AY8910interface  snt_ay8910Int;

#define SnT_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  snt_readmem, snt_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define SnT_SOUND { SOUND_TMS5220, &snt_tms5220Int },\
                  { SOUND_DAC,     &snt_dacInt }, \
                  { SOUND_AY8910,  &snt_ay8910Int }

#define BY35_SOUND61ROMxxx0(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1)

#define BY35_SOUND61ROM0xx0(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY35_SOUND61ROMxx80(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xe000, 0x0800, chk1) \
    ROM_RELOAD(  0xe800, 0x0800) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY35_SOUND61ROMxx00(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xe000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY35_SOUND61ROMx080(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x0800, chk2) \
    ROM_RELOAD(  0xe800, 0x0800) \
    ROM_LOAD(n3, 0xf000, 0x1000, chk3)

#define BY35_SOUND61ROMx008(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xf000, 0x0800, chk3) \
    ROM_RELOAD(  0xf800, 0x0800)

#define BY35_SOUND61ROMx000(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xe000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xf000, 0x1000, chk3)

#define BY35_SOUND61ROM0000(n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
    ROM_LOAD(n2, 0xd000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xe000, 0x1000, chk3) \
    ROM_LOAD(n4, 0xf000, 0x1000, chk4)

/* -32, -50 Sound module */
extern struct CustomSound_interface s32_custInt;

#define S32_SOUND {SOUND_CUSTOM,&s32_custInt}

#define BY35_SOUND32ROM(n1,chk1) \
  SOUNDREGION(0x0020, BY35_MEMREG_SROM) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

#define BY35_SOUND50ROM(n1,chk1) \
  SOUNDREGION(0x0020, BY35_MEMREG_SROM) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

/* Sounds Plus -51 */
extern const struct Memory_ReadAddress sp51_readmem[];
#define SP51_SOUND { SOUND_AY8910, &sp_ay8910Int }

#define SP51_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  sp51_readmem, sp_writemem, 0, 0, \
  ignore_interrupt,1 \
}
/* Sounds Plus -56, Vocalizer -57 */
extern const struct Memory_ReadAddress sp56_readmem[];
extern struct hc55516_interface sp_hc55516Int;

#define SP56_SOUND { SOUND_AY8910, &sp_ay8910Int },\
                   { SOUND_HC55516, &sp_hc55516Int }
#define SP56_SOUND_CPU { \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  sp56_readmem, sp_writemem, 0, 0, \
  ignore_interrupt,1 \
}

/* Common -51, -56/-57 */
extern const struct Memory_WriteAddress sp_writemem[];
extern struct AY8910interface  sp_ay8910Int;

#define BY35_SOUND51ROM(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x0800, chk1) \
    ROM_RELOAD(  0xf800, 0x0800) \
    ROM_RELOAD(  0x1000, 0x0800) \
    ROM_RELOAD(  0x1800, 0x0800)

#define BY35_SOUND51ROM0(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1) \
    ROM_RELOAD(  0x1000, 0x1000)

#define BY35_SOUND56ROM(n1,chk1) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1)

#define BY35_SOUND57ROM(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
    ROM_LOAD(n1, 0x8000, 0x1000, chk1) \
    ROM_LOAD(n2, 0x9000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xa000, 0x1000, chk3) \
    ROM_LOAD(n4, 0xb000, 0x1000, chk4) \
    ROM_LOAD(n5, 0xc000, 0x1000, chk5) \
    ROM_LOAD(n6, 0xd000, 0x1000, chk6) \
    ROM_LOAD(n7, 0xe000, 0x1000, chk7)

/* Cheap Squeak -45 */
extern const struct Memory_ReadAddress sp45_readmem[];
extern const struct Memory_WriteAddress sp45_writemem[];
extern const struct IO_ReadPort by35_45_readport[];
extern const struct IO_WritePort by35_45_writeport[];

#define SP45_SOUND { SOUND_DAC,     &snt_dacInt }

#define SP45_SOUND_CPU { \
  CPU_M6803 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */  			  \
  sp45_readmem, sp45_writemem, by35_45_readport, by35_45_writeport, \
  ignore_interrupt,1 \
}

#define BY35_SOUND45ROMx0(n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n2, 0xf000, 0x1000, chk2)

#define BY35_SOUND45ROM00(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, BY35_MEMREG_S1CPU) \
    ROM_LOAD(n2, 0xb000, 0x1000, chk2) \
	ROM_RELOAD(  0xc000, 0x1000) \
    ROM_LOAD(n1, 0xe000, 0x1000, chk1) \
	ROM_RELOAD(  0xf000, 0x1000) 

/* generic handler */
void by35_soundInit(void);
void by35_soundExit(void);
WRITE_HANDLER(by35_soundCmd);
#endif /* INC_BY35SND */


