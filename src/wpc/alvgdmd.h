#ifndef INC_ALVGDMD
#define INC_ALVGDMD

/*--------------------- DMD 128x32 -------------------*/
#define ALVGDMD_CPUNO     1
#define ALVGDMD_CPUREGION (REGION_CPU1 + ALVGDMD_CPUNO)
#define ALVGDMD_ROMREGION (REGION_GFX1 + ALVGDMD_CPUNO)

extern MACHINE_DRIVER_EXTERN(alvgdmd);

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
extern MACHINE_DRIVER_EXTERN(test8031);
#endif

extern PINMAME_VIDEO_UPDATE(alvgdmd_update);

//Main CPU 64K (Full Rom), DMD Data Roms (2 X 256K)
#define ALVGDMD_ROM(n1,chk1,n2,chk2,n3,chk3) \
  NORMALREGION(0x10000, ALVGDMD_CPUREGION) \
	ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
  NORMALREGION(0x80000, ALVGDMD_ROMREGION) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3)

//Main CPU 64K (1st 32K Empty), DMD Data Roms (2 X 256K)
//NOTE: We load the 64K CPU after the DMD Data roms, and then copy the upper 32K into the cpu region
#define ALVGDMD_SPLIT_ROM(n1,chk1,n2,chk2,n3,chk3) \
  NORMALREGION(0x90000, ALVGDMD_ROMREGION) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3) \
	ROM_LOAD(n1, 0x80000, 0x10000, chk1)  \
  NORMALREGION(0x10000, ALVGDMD_CPUREGION) \
	ROM_COPY(ALVGDMD_ROMREGION,0x88000,0x0000,0x8000)

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG

#define TEST8031(n1,chk1,n2,chk2,n3,chk3) \
  NORMALREGION(0x10000, REGION_CPU1) \
	ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
  NORMALREGION(0x80000, REGION_GFX1) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3)

#define TEST8031_SPLIT_ROM(n1,chk1,n2,chk2,n3,chk3) \
  NORMALREGION(0x90000, REGION_GFX1) \
    ROM_LOAD(n2, 0x00000, 0x40000, chk2) \
    ROM_LOAD(n3, 0x40000, 0x40000, chk3) \
	ROM_LOAD(n1, 0x80000, 0x10000, chk1)  \
  NORMALREGION(0x10000, REGION_CPU1) \
	ROM_COPY(REGION_GFX1,0x88000,0x0000,0x8000)

#endif 

#endif /* INC_ALVGDMD */

