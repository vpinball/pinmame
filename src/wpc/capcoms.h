#ifndef INC_CAPCOMSOUND
#define INC_CAPCOMSOUND

#define CAPCOMS_CPUNO 2
#define CAPCOMS_CPUREGION (REGION_CPU1+CAPCOMS_CPUNO)
#define CAPCOMS_ROMREGION (REGION_SOUND1)

extern MACHINE_DRIVER_EXTERN(capcoms);

/*-- Sound rom macros --*/

//ADD THIS TO TOP OF ALL MACROS ONCE WE GET 87c52 images
#if 0
  SOUNDREGION(0x10000, CAPCOMS_CPUREGION) \
    ROM_LOAD(n1, 0x0000,  0x10000, chk1) \

#endif

/*-- 64K Sound CPU Rom, 1 X 512K, 1 X 128K MPG Roms --*/
#define CAPCOMS_SOUNDROM2(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x300000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0      ,  0x80000, chk2) \
	ROM_LOAD(n3,  0x80000,  0x20000, chk3)  

/*-- 64K Sound CPU Rom, 2 X 1024K, 1 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM3(n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  SOUNDREGION(0x300000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x100000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) 

/*-- 64K Sound CPU Rom, 1 X 1024K, 3 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM4a(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  SOUNDREGION(0x300000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x80000, chk3) \
	ROM_LOAD(n4,  0x180000,  0x80000, chk4) \
	ROM_LOAD(n5,  0x200000,  0x80000, chk5) 

/*-- 64K Sound CPU Rom, 2 X 1024K, 2 X 512K MPG Roms --*/
#define CAPCOMS_SOUNDROM4b(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  SOUNDREGION(0x300000, CAPCOMS_ROMREGION) \
    ROM_LOAD(n2,  0       ,  0x100000, chk2) \
	ROM_LOAD(n3,  0x100000,  0x100000, chk3) \
	ROM_LOAD(n4,  0x200000,  0x80000, chk4) \
	ROM_LOAD(n5,  0x280000,  0x80000, chk5) 

#endif /* INC_CAPCOMSOUND */
