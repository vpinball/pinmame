#ifndef INC_DESOUND
#define INC_DESOUND

/*-- Sound rom macros --*/

/* Load 1Mb Rom(128K) to fit into 1Meg Rom Space */
#define DES_ROMLOAD2(start, n, chk) \
  ROM_LOAD(n, start,  0x20000, chk) \
  ROM_RELOAD( start + 0x20000, 0x20000) \
  ROM_RELOAD( start + 0x40000, 0x20000) \
  ROM_RELOAD( start + 0x60000, 0x20000) \
  ROM_RELOAD( start + 0x80000, 0x20000) \
  ROM_RELOAD( start + 0xa0000, 0x20000) \
  ROM_RELOAD( start + 0xc0000, 0x20000) \
  ROM_RELOAD( start + 0xe0000, 0x20000) 

/* Load 2Mb Rom(256K) to fit into 1Meg Rom Space  */
#define DES_ROMLOAD4(start, n, chk) \
  ROM_LOAD(n, start,  0x40000, chk) \
  ROM_RELOAD( start + 0x40000, 0x40000)\
  ROM_RELOAD( start + 0x80000, 0x40000)\
  ROM_RELOAD( start + 0xc0000, 0x40000)

/* Load 4Mb Rom(512K) to fit into 1Meg Rom Space */
#define DES_ROMLOAD8(start, n, chk) \
  ROM_LOAD(n, start, 0x80000, chk) \
  ROM_RELOAD( start + 0x80000, 0x80000)

/* 1 X 256Kb(32K) CPU ROM & 2 X 1Mb(128K) SOUND ROM @ U17, U21 */
#define DES_SOUNDROM011(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x8000,0x8000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD2(0x000000,u17,chk17) \
    DES_ROMLOAD2(0x100000,u21,chk21) \

/* 1 X 256Kb(32K) CPU ROM & 1 X 2Mb(256K) SOUND ROM @ U17 &
                            1 X 1Mb(128K) SOUND ROM @ U21    */
#define DES_SOUNDROM021(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x8000,0x8000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD4(0x000000,u17,chk17) \
    DES_ROMLOAD2(0x100000,u21,chk21) \

/* 1 X 256Kb(32K) CPU ROM & 2 X 2Mb(256K) SOUNDS ROM @ U17, U21 */
#define DES_SOUNDROM022(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x8000,0x8000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD4(0x000000,u17,chk17) \
    DES_ROMLOAD4(0x100000,u21,chk21) \

/* 1 X 256Kb(32K) CPU ROM & 1 X 4Mb(512K) SOUND ROM @ U17 &
                            1 X 2Mb(256K) SOUND ROM @ U21    */
#define DES_SOUNDROM042(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x8000,0x8000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD8(0x000000,u17,chk17) \
    DES_ROMLOAD4(0x100000,u21,chk21) \

/* 1 X 512Kb(64K) CPU ROM & 1 X 4Mb(512K) SOUND ROM @ U17 &
                            1 X 2Mb(256K) SOUND ROM @ U21    */
#define DES_SOUNDROM142(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD8(0x000000,u17,chk17) \
    DES_ROMLOAD4(0x100000,u21,chk21) 

/* 1 X 512Kb(64K) CPU ROM & 2 X 4Mb(512K) SOUND ROMS @ U17,U21 */
#define DES_SOUNDROM144(u7,chk7, u17,chk17,u21,chk21) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x200000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD8(0x000000,u17,chk17) \
    DES_ROMLOAD8(0x100000,u21,chk21) 

/* 1 X 512Kb(64K) CPU ROM & 3 X 4Mb(512K) SOUND ROMS @ U17,U21,U36 */
#define DES_SOUNDROM1444(u7,chk7, u17,chk17,u21,chk21,u36,chk36) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD8(0x000000,u17,chk17) \
    DES_ROMLOAD8(0x100000,u21,chk21) \
    DES_ROMLOAD8(0x200000,u36,chk36) 


/* 1 X 512Kb(64K) CPU ROM & 4 X 4Mb(512K) SOUND ROMS @ U17,U21,U36,U37 */
#define DES_SOUNDROM14444(u7,chk7, u17,chk17,u21,chk21,u36,chk36,u37,chk37) \
  SOUNDREGION(0x010000, DE_MEMREG_SDCPU1) \
    ROM_LOAD(u7,0x0000,0x10000,chk7) \
  SOUNDREGION(0x400000, DE_MEMREG_SDROM1) \
    DES_ROMLOAD8(0x000000,u17,chk17) \
    DES_ROMLOAD8(0x100000,u21,chk21) \
    DES_ROMLOAD8(0x200000,u36,chk36) \
    DES_ROMLOAD8(0x300000,u37,chk37)
    
/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress  des_readmem[];
extern const struct Memory_WriteAddress des_writemem[];

extern struct BSMT2000interface des_bsmt2000Int;

/*-- Sound interface communications --*/
extern READ_HANDLER(des_status_r);
extern WRITE_HANDLER(des_soundCmd_w);

extern void DES_init(void);
extern int des_irq(void);

#define DES_SOUNDCPU ,{ \
  CPU_M6809 | CPU_AUDIO_CPU, \
  2000000, /* 2 MHz */ \
  des_readmem, des_writemem, 0, 0, \
  ignore_interrupt, 0, \
  des_irq, 489 /*Fixed FIRQ of 489Hz as measured on real machine*/\
}

#define DES_SOUND \
  { SOUND_BSMT2000,  &des_bsmt2000Int }
#endif /* INC_DESSOUND */
