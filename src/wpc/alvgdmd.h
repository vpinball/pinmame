#ifndef INC_ALVGDMD
#define INC_ALVGDMD

/*--------------------- DMD 128x32 -------------------*/
#define ALVGDMD_CPUNO     2
#define ALVGDMD_CPUREGION (REGION_CPU1 + ALVGDMD_CPUNO)
#define ALVGDMD_ROMREGION (REGION_GFX1 + ALVGDMD_CPUNO)

extern MACHINE_DRIVER_EXTERN(alvgdmd);

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
extern MACHINE_DRIVER_EXTERN(test8031);
#endif

extern PINMAME_VIDEO_UPDATE(alvgdmd_update);

/* HELPER MACROS */

//NOTE: DMD CPU requires 128K of region space, since the ROM is mapped in the lower 64K, and the RAM in the upper 64K
#define CPU_REGION(reg)		 NORMALREGION(0x20000, reg)

//256K DMD Data Roms
#define ALVGDMD_256K_DATA_ROM(reg, n2,chk2,n3,chk3) \
  NORMALREGION(0x100000, reg) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
	ROM_RELOAD(  0x80000, 0x40000) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3) \
	ROM_RELOAD(  0xc0000, 0x40000)

//Same as above, but with extra 64K for the dmd cpu to be temporarily stored
#define ALVGDMD_256K_DATA_ROM_SPLIT(reg, n2,chk2,n3,chk3) \
  NORMALREGION(0x110000, reg) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
	ROM_RELOAD(  0x80000, 0x40000) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3) \
	ROM_RELOAD(  0xc0000, 0x40000)

//512K DMD Data Roms
#define ALVGDMD_512K_DATA_ROM(reg, n2,chk2,n3,chk3) \
  NORMALREGION(0x100000, reg) \
    ROM_LOAD(n2, 0x00000, 0x80000, chk2) \
    ROM_LOAD(n3, 0x80000, 0x80000, chk3)

/* END HELPER MACROS */


//Main CPU 64K (Full Rom), DMD Data Roms (2 X 256K) -
#define ALVGDMD_ROM(n1,chk1,n2,chk2,n3,chk3) \
  CPU_REGION(ALVGDMD_CPUREGION) \
	ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
	ALVGDMD_256K_DATA_ROM(ALVGDMD_ROMREGION,n2,chk2,n3,chk3)

//Main CPU 64K (1st 32K Empty), DMD Data Roms (2 X 256K)
//NOTE: We load the 64K CPU after the DMD Data roms, and then copy the upper 32K into the cpu region
#define ALVGDMD_SPLIT_ROM(n1,chk1,n2,chk2,n3,chk3) \
  ALVGDMD_256K_DATA_ROM_SPLIT(ALVGDMD_ROMREGION,n2,chk2,n3,chk3) \
  	ROM_LOAD(n1, 0x100000, 0x10000, chk1) \
  CPU_REGION(ALVGDMD_CPUREGION) \
	ROM_COPY(ALVGDMD_ROMREGION,0x108000,0x0000,0x8000)

//Main CPU 64K (Full Rom), DMD Data Roms (1 X 256K) -
#define ALVGDMD_ROM2R(n1,chk1,n2,chk2) \
  CPU_REGION(ALVGDMD_CPUREGION) \
	ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
    NORMALREGION(0x100000, ALVGDMD_ROMREGION) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
	ROM_RELOAD(  0x40000, 0x40000) \
	ROM_RELOAD(  0x80000, 0x40000) \
	ROM_RELOAD(  0xc0000, 0x40000)

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG

#define TEST8031_ROM(n1,chk1,n2,chk2,n3,chk3) \
  CPU_REGION(REGION_CPU1) \
	ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
    ALVGDMD_256K_DATA_ROM(REGION_GFX1,n2,chk2,n3,chk3)

#define TEST8031_SPLIT_ROM(n1,chk1,n2,chk2,n3,chk3) \
  ALVGDMD_256K_DATA_ROM_SPLIT(REGION_GFX1,n2,chk2,n3,chk3) \
    ROM_LOAD(n1, 0x100000, 0x10000, chk1) \
  CPU_REGION(REGION_CPU1) \
	ROM_COPY(REGION_GFX1,0x108000,0x0000,0x8000)
#endif

#endif /* INC_ALVGDMD */

