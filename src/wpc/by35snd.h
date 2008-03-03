#ifndef INC_BY35SND
#define INC_BY35SND

/*Bally Sound Hardware Info:

  Sound module -32 & -50
  ---------------------
  Custom Chips

  Sounds Plus -51
  ---------------
  M6802/M6808
  AY8910 Sound Generator
  ?NO DAC?

  Sounds Plus -56, Vocalizer -57
  ------------------------------
  M6802/M6808
  AY8910 Sound Generator
  HC55516 DAC

  Squalk N Talk -61
  -------------
  M6809 CPU
  TMS5220 Speach Generator
  AY8910 Sound Generator
  DAC

  Cheap Squeak  -45
  ------------
  M6803 CPU
  DAC
*/

/* Squawk n Talk */
#define BY61_CPUNO     1
#define BY61_CPUREGION (REGION_CPU1+BY61_CPUNO)

extern MACHINE_DRIVER_EXTERN(by61);

#define BY61_SOUNDROMxxx0(u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROM0xx0(u2,chk2,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u2, 0xc000, 0x1000, chk2) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROMxx80(u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u4, 0xe000, 0x0800, chk4) \
      ROM_RELOAD(0xe800, 0x0800) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROMxx00(u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u4, 0xe000, 0x1000, chk4) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROMx080(u3,chk3,u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u3, 0xd000, 0x1000, chk3) \
    ROM_LOAD(u4, 0xe000, 0x0800, chk4) \
      ROM_RELOAD(0xe800, 0x0800) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROMx008(u3,chk3,u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u3, 0xd000, 0x1000, chk3) \
    ROM_LOAD(u4, 0xe000, 0x1000, chk4) \
    ROM_LOAD(u5, 0xf000, 0x0800, chk5) \
      ROM_RELOAD(0xf800, 0x0800)

#define BY61_SOUNDROMx8x8(u3,chk3,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u3, 0xd000, 0x0800, chk3) \
      ROM_RELOAD(0xd800, 0x0800) \
    ROM_LOAD(u5, 0xf000, 0x0800, chk5) \
      ROM_RELOAD(0xf800, 0x0800)

#define BY61_SOUNDROMx000(u3,chk3,u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u3, 0xd000, 0x1000, chk3) \
    ROM_LOAD(u4, 0xe000, 0x1000, chk4) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

#define BY61_SOUNDROM0000(u2,chk2,u3,chk3,u4,chk4,u5,chk5) \
  SOUNDREGION(0x10000, BY61_CPUREGION) \
    ROM_LOAD(u2, 0xc000, 0x1000, chk2) \
    ROM_LOAD(u3, 0xd000, 0x1000, chk3) \
    ROM_LOAD(u4, 0xe000, 0x1000, chk4) \
    ROM_LOAD(u5, 0xf000, 0x1000, chk5)

/* -32, -50 Sound module */
#define BY32_ROMREGION REGION_SOUND1
extern MACHINE_DRIVER_EXTERN(by32);

#define BY32_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x0020, BY32_ROMREGION) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

#define BY50_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x0020, BY32_ROMREGION) \
    ROM_LOAD( n1, 0x0000, 0x0020, chk1)

/* Sounds Plus -51, -56 */
#define BY51_CPUNO     1
#define BY51_CPUREGION (REGION_CPU1+BY51_CPUNO)
extern MACHINE_DRIVER_EXTERN(by51);
extern MACHINE_DRIVER_EXTERN(by56);

#define BY51_SOUNDROM8(n1,chk1) \
  SOUNDREGION(0x10000, BY51_CPUREGION) \
    ROM_LOAD(n1, 0xf000, 0x0800, chk1) \
      ROM_RELOAD(0xf800, 0x0800) \
      ROM_RELOAD(0x1000, 0x0800) \
      ROM_RELOAD(0x1800, 0x0800)

#define BY51_SOUNDROM0(n1,chk1) \
  SOUNDREGION(0x10000, BY51_CPUREGION) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1) \
      ROM_RELOAD(0x1000, 0x1000)

#define BY56_SOUNDROM(n1,chk1) \
  SOUNDREGION(0x10000, BY51_CPUREGION) \
    ROM_LOAD(n1, 0xf000, 0x1000, chk1)

#define BY57_SOUNDROM(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
    ROM_LOAD(n1, 0x8000, 0x1000, chk1) \
    ROM_LOAD(n2, 0x9000, 0x1000, chk2) \
    ROM_LOAD(n3, 0xa000, 0x1000, chk3) \
    ROM_LOAD(n4, 0xb000, 0x1000, chk4) \
    ROM_LOAD(n5, 0xc000, 0x1000, chk5) \
    ROM_LOAD(n6, 0xd000, 0x1000, chk6) \
    ROM_LOAD(n7, 0xe000, 0x1000, chk7)

/* Cheap Squeak -45 */
#define BY45_CPUNO     1
#define BY45_CPUREGION (REGION_CPU1+BY45_CPUNO)
extern MACHINE_DRIVER_EXTERN(by45);

#define BY45_SOUNDROMx2(n1,chk1) \
  SOUNDREGION(0x10000, BY45_CPUREGION) \
    ROM_LOAD(n1, 0xc000, 0x2000, chk1) \
      ROM_RELOAD(0xe000, 0x2000)

/* 2 x 4K ROMS */
#define BY45_SOUNDROM11(n2,chk2,n1,chk1) \
  SOUNDREGION(0x10000, BY45_CPUREGION) \
    ROM_LOAD(n1, 0x8000, 0x1000, chk1) \
      ROM_RELOAD(0x9000, 0x1000) \
      ROM_RELOAD(0xa000, 0x1000) \
      ROM_RELOAD(0xb000, 0x1000) \
    ROM_LOAD(n2, 0xc000, 0x1000, chk2) \
      ROM_RELOAD(0xd000, 0x1000) \
      ROM_RELOAD(0xe000, 0x1000) \
      ROM_RELOAD(0xf000, 0x1000)

/* 2 x 8K ROMS */
#define BY45_SOUNDROM22(n2,chk2,n1,chk1) \
  SOUNDREGION(0x10000, BY45_CPUREGION) \
    ROM_LOAD(n1, 0x8000, 0x2000, chk1) \
      ROM_RELOAD(0xa000, 0x2000) \
    ROM_LOAD(n2, 0xc000, 0x2000, chk2) \
      ROM_RELOAD(0xe000, 0x2000)

/* Turbo Cheap Squeak */
#define BYTCS_CPUNO     1
#define BYTCS_CPUREGION (REGION_CPU1+BYTCS_CPUNO)
extern MACHINE_DRIVER_EXTERN(byTCS);
extern MACHINE_DRIVER_EXTERN(byTCS2);

/* Turbo Cheak Squalk - 1 x 16K ROM */
#define BYTCS_SOUNDROM4(n1,chk1) \
  SOUNDREGION(0x10000, BYTCS_CPUREGION) \
    ROM_LOAD(n1, 0x8000, 0x4000, chk1) \
      ROM_RELOAD(0xc000, 0x4000)

/* Turbo Cheak Squalk - 1 x 32K ROM */
#define BYTCS_SOUNDROM8(n1,chk1) \
  SOUNDREGION(0x10000, BYTCS_CPUREGION) \
    ROM_LOAD(n1, 0x8000, 0x8000, chk1)

/* Turbo Cheak Squalk - 1 x 64K ROM */
#define BYTCS_SOUNDROM0(n1,chk1) \
  SOUNDREGION(0x10000, BYTCS_CPUREGION) \
    ROM_LOAD(n1, 0x0000, 0x10000, chk1)

/* Sounds Delux */
#define BYSD_CPUNO     1
#define BYSD_CPUREGION (REGION_CPU1+BYSD_CPUNO)
extern MACHINE_DRIVER_EXTERN(bySD);

#define BYSD_SOUNDROM0000(n1,chk1, n2, chk2, n3,chk3, n4, chk4) \
  SOUNDREGION(0x01000000, BYSD_CPUREGION) \
    ROM_LOAD16_BYTE(n1, 0x00001, 0x10000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000, 0x10000, chk2) \
    ROM_LOAD16_BYTE(n3, 0x20001, 0x10000, chk3) \
    ROM_LOAD16_BYTE(n4, 0x20000, 0x10000, chk4)

#define BYSD_SOUNDROM00xx(n1,chk1, n2, chk2) \
  SOUNDREGION(0x01000000, BYSD_CPUREGION) \
    ROM_LOAD16_BYTE(n1, 0x00001, 0x10000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000, 0x10000, chk2)

#endif /* INC_BY35SND */


