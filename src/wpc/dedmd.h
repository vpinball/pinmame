#ifndef INC_DEDMD
#define INC_DEDMD

extern void de_dmd128x16_refresh(struct mame_bitmap *bitmap,int full_refresh);
extern void de2_dmd128x16_refresh(struct mame_bitmap *bitmap,int full_refresh);
extern void de_dmd128x32_refresh(struct mame_bitmap *bitmap,int full_refresh);
extern void de_dmd192x64_refresh(struct mame_bitmap *bitmap,int full_refresh);

/*--------------------- DMD 128x32 -------------------*/
extern const struct Memory_ReadAddress  de_dmd32readmem[];
extern const struct Memory_WriteAddress de_dmd32writemem[];

#define DE_DMD32CPUNO     2
#define DE_DMD32CPUREGION (REGION_CPU1  + DE_DMD32CPUNO)
#define DE_DMD32ROMREGION (REGION_USER1 + DE_DMD32CPUNO)

#define DE_DMD32CPU { \
  CPU_M6809, 4000000, /* 4 Mhz*/ \
  de_dmd32readmem, de_dmd32writemem, \
}
#define DE_DMD32VIDEO  NULL, NULL, de_dmd128x32_refresh

#define DE_DMD32ROM44(n1,chk1,n2,chk2) \
  NORMALREGION(0x10000, DE_DMD32CPUREGION) \
  NORMALREGION(0x80000, DE_DMD32ROMREGION) \
    ROM_LOAD(n1, 0x00000, 0x40000, chk1) \
    ROM_LOAD(n2, 0x40000, 0x40000, chk2)

#define DE_DMD32ROM8x(n1,chk1) \
  NORMALREGION(0x10000, DE_DMD32CPUREGION) \
  NORMALREGION(0x80000, DE_DMD32ROMREGION) \
    ROM_LOAD(n1, 0x00000, 0x80000, chk1) \

/*--------------------- DMD 192x64 -------------------*/
extern const struct Memory_ReadAddress16  de_dmd64readmem[];
extern const struct Memory_WriteAddress16 de_dmd64writemem[];

#define DE_DMD64CPUNO     2
#define DE_DMD64CPUREGION (REGION_CPU1  + DE_DMD64CPUNO)

#define DE_DMD64CPU { \
  CPU_M68000, 6000000, /* 12 Mhz*/ \
  de_dmd64readmem, de_dmd64writemem \
}
#define DE_DMD64VIDEO NULL, NULL, de_dmd192x64_refresh

#define DE_DMD64ROM88(n1,chk1,n2,chk2) \
  NORMALREGION(0x01000000, DE_DMD64CPUREGION) \
    ROM_LOAD16_BYTE(n1, 0x00000001, 0x00080000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000000, 0x00080000, chk2)

/*--------------------- DMD 128x16 -------------------*/
extern const struct Memory_ReadAddress  de_dmd16readmem[];
extern const struct Memory_WriteAddress de_dmd16writemem[];
extern const struct IO_ReadPort         de_dmd16readport[];
extern const struct IO_WritePort        de_dmd16writeport[];
#define DE_DMD16CPUNO 2
#define DE_DMD16CPUREGION (REGION_CPU1  + DE_DMD16CPUNO)
#define DE_DMD16ROMREGION (REGION_USER1 + DE_DMD16CPUNO)

#define DE_DMD16CPU { \
  CPU_Z80, 2000000, /* 4 Mhz */ \
  de_dmd16readmem, de_dmd16writemem, de_dmd16readport, de_dmd16writeport \
}
#define DE_DMD16VIDEO  NULL, NULL, de2_dmd128x16_refresh

#define DE_DMD16ROM1(n1,chk1) \
  NORMALREGION(0x10000, DE_DMD16CPUREGION) \
  NORMALREGION(0x20000, DE_DMD16ROMREGION) \
    ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
      ROM_RELOAD(0x10000, 0x10000)

#define DE_DMD16ROM2(n1,chk1) \
  NORMALREGION(0x10000, DE_DMD16CPUREGION) \
  NORMALREGION(0x20000, DE_DMD16ROMREGION) \
    ROM_LOAD(n1, 0x00000, 0x20000, chk1)

#endif /* INC_DEDMD */

