#ifndef INC_SESOUND
#define INC_SESOUND

#define SES_MEMREG_SCPU
#define SES_MEMREG_SROM
/*-- Sound rom macros --*/

/* Load 4Mb Rom(512K) to fit into 1Meg Rom Space*/
#define SES_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start,  0x80000, chk) \
  ROM_RELOAD( start + 0x80000, 0x80000)

/* Load 8Mb Rom(1024K) */
#define SES_ROMLOAD0(start, n, chk) \
  ROM_LOAD(n, start, 0x100000, chk)

/* 1 X 512Kb(64K) CPU ROM & 2 X 4Mb(512K) SOUNDS ROM @ U17, U21 */
#define SES_SOUNDROM088(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD8(0x000000,u17,chk17) \
    SES_ROMLOAD8(0x100000,u21,chk21) 
    
/* 1 X 512Kb(64K) CPU ROM & 3 X 4Mb(512K) SOUNDS ROM @ U17, U21, U36 */
#define SES_SOUNDROM0888(u7,chk7, u17,chk17,u21,chk21,u36,chk36) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD8(0x000000,u17,chk17) \
    SES_ROMLOAD8(0x100000,u21,chk21) \
    SES_ROMLOAD8(0x200000,u36,chk36) 


/* 1 X 512Kb(64K) CPU ROM & 4 X 4Mb(512K) SOUNDS ROM @ U17, U21, U36, U37 */
#define SES_SOUNDROM08888(u7,chk7, u17,chk17,u21,chk21,u36,chk36,u37,chk37) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD8(0x000000,u17,chk17) \
    SES_ROMLOAD8(0x100000,u21,chk21) \
    SES_ROMLOAD8(0x200000,u36,chk36) \
    SES_ROMLOAD8(0x300000,u37,chk37)

/* 1 X 512Kb(64K) CPU ROM & 3 X 8Mb(1024K) SOUNDS ROM @ U17, U21, U36 */
#define SES_SOUNDROM0000(u7,chk7, u17,chk17,u21,chk21,u36,chk36) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD0(0x000000,u17,chk17) \
    SES_ROMLOAD0(0x100000,u21,chk21) \
    SES_ROMLOAD0(0x200000,u36,chk36) 

/* 1 X 512Kb(64K) CPU ROM & 3 X 8Mb(1024K) SOUNDS ROM @ U17, U21, U36 & */
/*                          1 X 4Mb(512K) SOUNDS ROM @ U37 */
#define SES_SOUNDROM00008(u7,chk7, u17,chk17,u21,chk21,u36,chk36,u37,chk37) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD0(0x000000,u17,chk17) \
    SES_ROMLOAD0(0x100000,u21,chk21) \
    SES_ROMLOAD0(0x200000,u36,chk36) \
    SES_ROMLOAD8(0x300000,u37,chk37)

/* 1 X 512Kb(64K) CPU ROM & 4 X 8Mb(1024K) SOUNDS ROM @ U17, U21, U36, U37 */
#define SES_SOUNDROM00000(u7,chk7, u17,chk17,u21,chk21,u36,chk36,u37,chk37) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD0(0x000000,u17,chk17) \
    SES_ROMLOAD0(0x100000,u21,chk21) \
    SES_ROMLOAD0(0x200000,u36,chk36) \
    SES_ROMLOAD0(0x300000,u37,chk37)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  ses_readmem[];
extern const struct Memory_WriteAddress ses_writemem[];

extern struct BSMT2000interface ses_bsmt2000Int1;
extern struct BSMT2000interface ses_bsmt2000Int2;
extern struct BSMT2000interface ses_bsmt2000Int3;

/*-- Sound interface communications --*/
extern READ_HANDLER(ses_status_r);
extern WRITE_HANDLER(ses_soundCmd_w);

extern void SES_init(void);
extern int ses_irq(void);

#define SES_SOUNDCPU ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  ses_readmem, ses_writemem, 0, 0, \
  ignore_interrupt, 0, \
  ses_irq, 489 /*Fixed FIRQ of 489Hz as measured on real machine*/\
}

//11 Voice Interface
#define SES_SOUND1 \
  { SOUND_BSMT2000,  &ses_bsmt2000Int1 }
//12 Voice Interface
#define SES_SOUND2 \
  { SOUND_BSMT2000,  &ses_bsmt2000Int2 }
//11 Voice Interface with large volume adjustment
#define SES_SOUND3 \
  { SOUND_BSMT2000,  &ses_bsmt2000Int3 }

#endif /* INC_SESSOUND */
