#ifndef GTSS80S_H
#define GTSS80S_H

/*
    Gottlieb System 80 Sound Boards

    - System 80/80A Sound Board

    - System 80/80A Sound & Speech Board

    - System 80A Sound Board with a PiggyPack installed
	  (thanks goes to Peter Hall for providing some very usefull information)

    - System 80B Sound Board (3 generations)

*/

/* Gottlieb System 80/80A Sound Board */

/*-- Sound Board, 1 X 1K Sound Rom, 6530 System sound rom --*/
#define GTS80S1K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x0400,  0x0400, chk1) \
    ROM_RELOAD(  0x0800,  0x0400) \
	ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

/*-- Sound Board, 1 X 2K Sound Rom --*/
#define GTS80S2K_ROMSTART(n1,chk1) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x0800,  0x0800, chk1) \
    ROM_RELOAD(  0xf800,  0x0800)

extern MACHINE_DRIVER_EXTERN(gts80s_s);


/* Gottlieb System 80/80A Sound & Speech Board */

/*-- Sound & Speeh Board, 2 X 2K Voice/Sound Roms --*/
#define GTS80SS22_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
	ROM_LOAD(n1, 0x7000,  0x0800, chk1) \
	ROM_LOAD(n2, 0x7800,  0x0800, chk2)

extern MACHINE_DRIVER_EXTERN(gts80s_ss);


/* Gottlieb System 80B Sound board Hardware Versions:
   -----------------------------------------
   ALL   - CPU: 2x(6502): DAC: 2x(AD7528)
   --------------------------------------
   Gen 1 - DSP: 2x(AY8913): OTHER: SP0-250 (SPEECH GENERATOR)
   Gen 2 - DSP: 2x(AY8913): OTHER: Programmable Capacitor Filter (Not Emulated)
   Gen 3 - DSP: 1x(YM2151): OTHER: None
*/

/*-- Sound rom macros --*/

/* Load 32k Rom Space */
#define GTS80BS_ROMLOAD32(start, n, chk) \
  ROM_LOAD(n, start,  0x8000, chk)

/*-- Gen 1: 3 x 8K Sound CPU Roms --*/
#define GTS80BSSOUND888(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)\
    ROM_LOAD(n3,0xc000,0x2000, chk3)

/*-- Gen 2 : 2 x 8K Sound CPU Roms --*/
#define GTS80BSSOUND88(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
	ROM_LOAD(n1,0xe000,0x2000, chk1)\
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    ROM_LOAD(n2,0xe000,0x2000, chk2)

/*-- Gen 2 & 3: 2 x 32K Sound CPU Roms --*/
#define GTS80BSSOUND3232(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    GTS80BS_ROMLOAD32(0x8000, n1, chk1) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    GTS80BS_ROMLOAD32(0x8000, n2, chk2)

/*-- Gen 2 & 3: 2 x 32K Sound CPU Roms --*/
#define GTS80BSSOUND3x32(n1,chk1,n2,chk2,n3,chk3) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU3) \
    GTS80BS_ROMLOAD32(0x8000, n1, chk1) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU2) \
    GTS80BS_ROMLOAD32(0x8000, n2, chk2) \
  SOUNDREGION(0x10000, GTS80_MEMREG_SCPU1) \
    GTS80BS_ROMLOAD32(0x8000, n3, chk3)

extern MACHINE_DRIVER_EXTERN(gts80s_b1);
extern MACHINE_DRIVER_EXTERN(gts80s_b2);
extern MACHINE_DRIVER_EXTERN(gts80s_b3);
extern MACHINE_DRIVER_EXTERN(gts80s_b3a);

/*-- GTS3 --*/

/* Load 1Mb Rom(128K) to fit into 4Mb Rom Space */
#define GTS3S_ROMLOAD1(start, n, chk) \
  ROM_LOAD(n, start,  0x20000, chk) \
  ROM_RELOAD(start+0x40000, 0x20000) \
  ROM_RELOAD(start+0x80000, 0x20000) \
  ROM_RELOAD(start+0xc0000, 0x20000)

/* Load 2Mb Rom(256K) to fit into 4Mb Rom Space */
#define GTS3S_ROMLOAD2(start, n, chk) \
  ROM_LOAD(n, start,  0x40000, chk) \
  ROM_RELOAD(start+0x40000, 0x40000)

/* Load 4Mb Rom(512K) */
#define GTS3S_ROMLOAD4(start, n, chk) \
  ROM_LOAD(n, start,  0x80000, chk)

/*-- 2 x 32K Sound CPU Roms --*/
//Purposely load in n2 first!
#define GTS3SOUND3232(n2,chk2,n1,chk1) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x10000, REGION_CPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

//This code did not work for the 128K Rom games
#if 0
	/*-- 2 x 32K Sound CPU Roms, 2 x 128K Voice Roms --*/
	//Purposely load in n2 first!
	#define GTS3SOUND32128(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
	  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
		ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
	  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
		GTS3S_ROMLOAD1(0x00000, n3, chk3) \
		GTS3S_ROMLOAD1(0x80000, n4, chk4) \
	  SOUNDREGION(0x10000, REGION_CPU2) \
		ROM_LOAD(n2, 0x8000,  0x8000, chk2)
#endif

/*-- 2 x 32K Sound CPU Roms, 2 x 128K Voice Roms BUT TREATED AS 1x256 ROM --*/
//Purposely load in n2 first!
#define GTS3SOUND32128(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD1(0x00000, n3, chk3) \
	GTS3S_ROMLOAD1(0x20000, n4, chk4) \
  SOUNDREGION(0x10000, REGION_CPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- 2 x 32K Sound CPU Roms, 2 x 256K Voice Roms --*/
//Purposely load in n2 first!
#define GTS3SOUND32256A(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD2(0x00000, n3, chk3) \
	GTS3S_ROMLOAD2(0x80000, n4, chk4) \
  SOUNDREGION(0x10000, REGION_CPU2) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- 2 x 32K Sound CPU Roms, 2 x 256K Voice Roms --*/
//Purposely load in n2 first!
#define GTS3SOUND32256(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD2(0x00000, n3, chk3) \
	GTS3S_ROMLOAD2(0x80000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- 2 x 32K Sound CPU Roms, 1 x 512K Voice Rom --*/
//Purposely load in n2 first!
#define GTS3SOUND32512A(n2,chk2,n1,chk1,n3,chk3) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD4(0x00000, n3, chk3) \
	  ROM_RELOAD(0x80000, 0x80000) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- 2 x 32K Sound CPU Roms, 1 x 512K, 1 x 256K Voice Roms --*/
//Purposely load in n2 first!
#define GTS3SOUND32512256(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD4(0x00000, n3, chk3) \
	GTS3S_ROMLOAD2(0x80000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- 2 x 32K Sound CPU Roms, 2 x 512K Voice Roms --*/
//Purposely load in n2 first!
#define GTS3SOUND32512(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
    ROM_LOAD(n1, 0x8000,  0x8000, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	GTS3S_ROMLOAD4(0x00000, n3, chk3) \
	GTS3S_ROMLOAD4(0x80000, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
	ROM_LOAD(n2, 0x8000,  0x8000, chk2)

/*-- unknown Sound & Voice CPU Roms --*/
#define GTS3SOUNDX(n2,chk2,n1,chk1,n3,chk3, n4, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU2) \
	ROM_LOAD(n1, 0x8000,  0, chk1) \
  SOUNDREGION(0x100000, GTS3_MEMREG_SROM1) \
	ROM_LOAD(n3, 0x00000, 0, chk3) \
	ROM_LOAD(n4, 0x80000, 0, chk4) \
  SOUNDREGION(0x10000, GTS3_MEMREG_SCPU1) \
	ROM_LOAD(n2, 0x8000,  0, chk2)

extern MACHINE_DRIVER_EXTERN(gts80s_s3);
extern MACHINE_DRIVER_EXTERN(gts80s_s3_no);

#endif /* GTSS80S_H */
