#ifndef INC_ALVGSOUND
#define INC_ALVGSOUND

#define ALVGS_CPUNO 1
#define ALVGS_CPUREGION (REGION_CPU1+ALVGS_CPUNO)
#define ALVGS_ROMREGION (REGION_SOUND1)

extern MACHINE_DRIVER_EXTERN(alvg_s1);
extern MACHINE_DRIVER_EXTERN(alvg_s2);

/*-- Sound rom macros --*/
/*-- 64K Sound CPU Rom, 4 X 256K Voice Roms --*/

/* Load 2Mb Rom(256K) to fit into 1Meg Rom Space  */
#define ALVGS_ROMLOAD4(start, n, chk) \
  ROM_LOAD(n,  start,  0x40000, chk) \
    ROM_RELOAD(start + 0x40000, 0x40000)\
    ROM_RELOAD(start + 0x80000, 0x40000)\
    ROM_RELOAD(start + 0xc0000, 0x40000)

#define ALVGS_SOUNDROM(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  SOUNDREGION(0x10000, ALVGS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x10000, chk1) \
	SOUNDREGION(0x400000, ALVGS_ROMREGION) \
	ALVGS_ROMLOAD4(0x000000, n2, chk2) \
	ALVGS_ROMLOAD4(0x100000, n3, chk3) \
	ALVGS_ROMLOAD4(0x200000, n4, chk4) \
	ALVGS_ROMLOAD4(0x300000, n5, chk5)

/*-- 64K Sound CPU Rom, 1 X 256K Voice Roms --*/
#define ALVGS_SOUNDROM11(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, ALVGS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x10000, chk1) \
	SOUNDREGION(0x400000, ALVGS_ROMREGION) \
	ALVGS_ROMLOAD4(0x000000, n2, chk2)

#endif /* INC_ALVGSOUND */
