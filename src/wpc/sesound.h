#ifndef INC_SESOUND
#define INC_SESOUND

#define SES_MEMREG_SCPU
#define SES_MEMREG_SROM
/*-- Sound rom macros --*/
#define SES_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start,  0x80000, chk) \
  ROM_RELOAD( start + 0x80000, 0x80000)

#define SES_ROMLOAD0(start, n, chk) \
  ROM_LOAD(n, start, 0x100000, chk)

#define SES_SOUNDROM08888(u7,chk7, u17,chk17,u21,chk21,u36,chk36,u37,chk37) \
  SOUNDREGION(0x010000, SE_MEMREG_SCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, SE_MEMREG_SROM1) \
    SES_ROMLOAD8(0x000000,u17,chk17) \
    SES_ROMLOAD8(0x100000,u21,chk21) \
    SES_ROMLOAD8(0x200000,u36,chk36) \
    SES_ROMLOAD8(0x300000,u37,chk37)

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

extern struct BSMT2000interface ses_bsmt2000Int;

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
  ses_irq, 976 \
}

#define SES_SOUND \
  { SOUND_BSMT2000,  &ses_bsmt2000Int }
#endif /* INC_SESSOUND */
