#ifndef INC_BY6803SND
#define INC_BY6803SND

#define BY6803_SOUND_S1_x111(n2, chk2, n3, chk3, n4,chk4) \
	NORMALREGION(0x10000, BY6803_MEMREG_SCPU) \
	ROM_LOAD(n2, 0xd000, 0x1000, chk2) \
	ROM_LOAD(n3, 0xe000, 0x1000, chk3) \
	ROM_LOAD(n4, 0xf000, 0x1000, chk4) 

#define BY6803_SOUND_S1_1111(n1,chk1, n2, chk2, n3,chk3, n4, chk4) \
	NORMALREGION(0x10000, BY6803_MEMREG_SCPU) \
	ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
	ROM_LOAD(n2, 0xd000, 0x1000, chk2) \
	ROM_LOAD(n3, 0xe000, 0x1000, chk3) \
	ROM_LOAD(n4, 0xf000, 0x1000, chk4) 

/* Turbo Cheak Squalk - 1 x 16K ROM */
#define BY6803_SOUND_S2_4(n1,chk1) \
    NORMALREGION(0x10000, BY6803_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x8000, 0x4000, chk1) \
	ROM_RELOAD(  0xc000, 0x4000)

/* Turbo Cheak Squalk - 1 x 32K ROM */
#define BY6803_SOUND_S2_8(n1,chk1) \
    NORMALREGION(0x10000, BY6803_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x8000, 0x8000, chk1) 

/* Turbo Cheak Squalk - 1 x 64K ROM */
#define BY6803_SOUND_S2_0(n1,chk1) \
    NORMALREGION(0x10000, BY6803_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x10000, chk1) 

/* Sounds Deluxe - 2 x 64K ROM */
#if defined(MAMEVER) && (MAMEVER > 3709)
#  define BY6803_SOUND_S3_00xx(n1,chk1, n2, chk2) \
    NORMALREGION(0x01000000, BY6803_MEMREG_SCPU) \
    ROM_LOAD16_BYTE(n1, 0x00001, 0x10000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000, 0x10000, chk2) 
#else /* MAMEVER */
#  define BY6803_SOUND_S3_00xx(n1,chk1, n2, chk2) \
    NORMALREGION(0x01000000, BY6803_MEMREG_SCPU) \
      ROM_LOAD_ODD( n1, 0x00000, 0x10000, chk1) \
      ROM_LOAD_EVEN(n2, 0x00000, 0x10000, chk2)
#endif /* MAMEVER */

/* Sounds Deluxe - 4 x 64K ROM */
#if defined(MAMEVER) && (MAMEVER > 3709)
#  define BY6803_SOUND_S3_0000(n1,chk1, n2, chk2, n3,chk3, n4, chk4) \
    NORMALREGION(0x01000000, BY6803_MEMREG_SCPU) \
    ROM_LOAD16_BYTE(n1, 0x00001, 0x10000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000, 0x10000, chk2) \
	ROM_LOAD16_BYTE(n3, 0x20001, 0x10000, chk3) \
    ROM_LOAD16_BYTE(n4, 0x20000, 0x10000, chk4) 
#else /* MAMEVER */
#  define BY6803_SOUND_S3_0000(n1,chk1, n2, chk2, n3,chk3, n4, chk4) \
    NORMALREGION(0x01000000, BY6803_MEMREG_SCPU) \
      ROM_LOAD_ODD( n1, 0x00000, 0x10000, chk1) \
      ROM_LOAD_EVEN(n2, 0x00000, 0x10000, chk2) \
	  ROM_LOAD_ODD( n3, 0x20000, 0x10000, chk3) \
      ROM_LOAD_EVEN(n4, 0x20000, 0x10000, chk4) 
#endif /* MAMEVER */

/* Williams System 11C - 3 x 32K ROM */
#define BY6803_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start, 0x8000, chk) \
  ROM_RELOAD(start+0x8000, 0x8000)

#define BY6803_SOUND_S4_888(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, BY6803_MEMREG_SCPU) \
  SOUNDREGION(0x30000, BY6803_MEMREG_SROM) \
  BY6803_ROMLOAD8(0x00000, n1, chk1) \
  BY6803_ROMLOAD8(0x10000, n2, chk2) \
  BY6803_ROMLOAD8(0x20000, n3, chk3)

extern struct Samplesinterface	samples_interface;

/******************************************************/
/********** GENERATION 1 - SQUALK & TALK **************/
/******************************************************/
extern const struct Memory_ReadAddress snt_readmem[];
extern const struct Memory_WriteAddress snt_writemem[];
extern struct TMS5220interface snt_tms5220Int;
extern struct DACinterface     snt_dacInt;
extern struct AY8910interface  snt_ay8910Int;

#define BY6803_SOUNDCPU1 ,{ \
  CPU_M6802 | CPU_AUDIO_CPU, 3580000/4,	/* .8 MHz */					\
  snt_readmem, snt_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BY6803_GEN1_SOUND \
	{ SOUND_TMS5220, &snt_tms5220Int },\
    { SOUND_DAC,     &snt_dacInt }, \
    { SOUND_AY8910,  &snt_ay8910Int }, \
	{ SOUND_SAMPLES, &samples_interface }

/***********************************************************/
/********** GENERATION 2 - TURBO CHEAP SQUALK **************/
/***********************************************************/
extern const struct Memory_ReadAddress  s2_readmem[];
extern const struct Memory_WriteAddress s2_writemem[];
extern const struct Memory_ReadAddress  s2a_readmem[];
extern const struct Memory_WriteAddress s2a_writemem[];
extern struct DACinterface     s2_dacInt;

#define BY6803_SOUNDCPU2 ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000,	/* 2MHz */					\
  s2_readmem, s2_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BY6803_SOUNDCPU2A ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000,	/* 2MHz */					\
  s2a_readmem, s2a_writemem, 0, 0, \
  ignore_interrupt,1 \
}

#define BY6803_GEN2_SOUND \
    { SOUND_DAC,     &s2_dacInt },  \
	{ SOUND_SAMPLES, &samples_interface }

/***********************************************************/
/********** GENERATION 3 - SOUNDS DELUXE      **************/
/***********************************************************/
extern const struct Memory_ReadAddress16 s3_readmem[];
extern const struct Memory_WriteAddress16 s3_writemem[];
extern struct DACinterface     s3_dacInt;

#define BY6803_SOUNDCPU3 ,{ \
  CPU_M68000 | CPU_AUDIO_CPU, 8000000,	/* 8MHz */					\
  s3_readmem, s3_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BY6803_GEN3_SOUND \
    { SOUND_DAC,     &s3_dacInt },  \
	{ SOUND_SAMPLES, &samples_interface }

/************************************************************/
/********** GENERATION 4 - WILLIAMS SYSTEM 11C **************/
/************************************************************/
extern const struct Memory_ReadAddress s4_readmem[];
extern const struct Memory_WriteAddress s4_writemem[];
extern struct DACinterface      s4_dacInt;
extern struct YM2151interface   s4_ym2151Int;
extern struct hc55516_interface s4_hc55516Int;

#define BY6803_SOUNDCPU4 ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, 2000000,	/* 2MHz */					\
  s4_readmem, s4_writemem, 0, 0, \
  ignore_interrupt,1 \
}
#define BY6803_GEN4_SOUND \
	{ SOUND_YM2151,  &s4_ym2151Int }, \
	{ SOUND_DAC,     &s4_dacInt }, \
	{ SOUND_HC55516, &s4_hc55516Int }, \
	{ SOUND_SAMPLES, &samples_interface }

/*Sound Generation Specific funcs*/
extern void by6803_sndinit1(void);
extern void by6803_sndexit1(void);
extern WRITE_HANDLER(by6803_sndcmd1);
extern void by6803_snddiag1(void);

extern void by6803_sndinit2(void);
extern void by6803_sndexit2(void);
extern WRITE_HANDLER(by6803_sndcmd2);
extern void by6803_snddiag2(void);

extern void by6803_sndinit3(void);
extern void by6803_sndexit3(void);
extern WRITE_HANDLER(by6803_sndcmd3);
extern void by6803_snddiag3(void);

extern void by6803_sndinit4(void);
extern void by6803_sndexit4(void);
extern WRITE_HANDLER(by6803_sndcmd4);
extern void by6803_snddiag4(void);

#endif /* INC_BY6803SND */
