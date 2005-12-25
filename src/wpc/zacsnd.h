#ifndef INC_ZACSND
#define INC_ZACSND

#define ZACSND_CPUA       1
#define ZACSND_CPUB       2
#define ZACSND_CPUC       3
#define ZACSND_CPUAREGION (REGION_CPU1+ZACSND_CPUA)
#define ZACSND_CPUBREGION (REGION_CPU1+ZACSND_CPUB)
#define ZACSND_CPUCREGION (REGION_CPU1+ZACSND_CPUC)

extern MACHINE_DRIVER_EXTERN(zac1311);
extern MACHINE_DRIVER_EXTERN(zac1125);
extern MACHINE_DRIVER_EXTERN(zac1346);
extern MACHINE_DRIVER_EXTERN(zac1146);
extern MACHINE_DRIVER_EXTERN(zac1370);
extern MACHINE_DRIVER_EXTERN(zac13136);
extern MACHINE_DRIVER_EXTERN(zac11178);
extern MACHINE_DRIVER_EXTERN(zac11178_13181);
extern MACHINE_DRIVER_EXTERN(zac11178_11181);
extern MACHINE_DRIVER_EXTERN(zac11183);
extern MACHINE_DRIVER_EXTERN(techno);

//Board 1346 configuration (1 x 2K ROM)
#define ZAC_SOUNDROM_0(u1, chk1) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(u1, 0x0000, 0x0800, chk1)

//Board 1370 configuration (3 x 4K ROMS, 1 x 2K ROM)
#define ZAC_SOUNDROM_cefg0(uc,chkc,ue,chke,uf,chkf,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(uc, 0xf000, 0x1000, chkc) \
    ROM_LOAD(ue, 0xe000, 0x1000, chke) \
    ROM_LOAD(uf, 0xd000, 0x1000, chkf) \
    ROM_LOAD(ug, 0xc000, 0x0800, chkg)

//Board 1370 configuration (4 x 4K ROMS)
#define ZAC_SOUNDROM_cefg1(uc,chkc,ue,chke,uf,chkf,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(uc, 0xf000, 0x1000, chkc) \
    ROM_LOAD(ue, 0xe000, 0x1000, chke) \
    ROM_LOAD(uf, 0xd000, 0x1000, chkf) \
    ROM_LOAD(ug, 0xc000, 0x1000, chkg)

//Board 1370 configuration (5 x 4K ROMS)
#define ZAC_SOUNDROM_cefgh(uc,chkc,ue,chke,uf,chkf,ug,chkg,uh,chkh) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(uc, 0xf000, 0x1000, chkc) \
    ROM_LOAD(ue, 0xe000, 0x1000, chke) \
    ROM_LOAD(uf, 0xd000, 0x1000, chkf) \
    ROM_LOAD(ug, 0xc000, 0x1000, chkg) \
    ROM_LOAD(uh, 0x7000, 0x1000, chkh)

#define ZAC_SOUNDROM_de1g(ud,chkd,ue,chke,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ud, 0xe000, 0x2000, chkd) \
    ROM_LOAD(ue, 0xc000, 0x1000, chke) \
      ROM_RELOAD(0xd000, 0x1000) \
    ROM_LOAD(ug, 0xa000, 0x2000, chkg)

#define ZAC_SOUNDROM_de2g(ud,chkd,ue,chke,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ud, 0xe000, 0x2000, chkd) \
    ROM_LOAD(ue, 0xc000, 0x2000, chke) \
    ROM_LOAD(ug, 0xa000, 0x2000, chkg)

#define ZAC_SOUNDROM_e2f2(ue,chke,uf,chkf) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ue, 0xc000, 0x2000, chke) \
    ROM_LOAD(uf, 0xe000, 0x2000, chkf)

#define ZAC_SOUNDROM_e2f4(ue,chke,uf,chkf) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ue, 0xa000, 0x2000, chke) \
    ROM_LOAD(uf, 0xc000, 0x4000, chkf)

#define ZAC_SOUNDROM_e4f4(ue,chke,uf,chkf) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ue, 0x8000, 0x4000, chke) \
    ROM_LOAD(uf, 0xc000, 0x4000, chkf)

#define ZAC_SOUNDROM_f(uf,chkf) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(uf, 0xc000, 0x4000, chkf)

#define ZAC_SOUNDROM_456(u4,chk4,u5,chk5,u6,chk6) \
  SOUNDREGION(0x10000, ZACSND_CPUBREGION) \
    ROM_LOAD(u4, 0x0000, 0x4000, chk4) \
    ROM_LOAD(u5, 0x4000, 0x4000, chk5) \
    ROM_LOAD(u6, 0x8000, 0x4000, chk6)

#define ZAC_SOUNDROM_46(u4,chk4,u6,chk6) \
  SOUNDREGION(0x10000, ZACSND_CPUBREGION) \
    ROM_LOAD(u4, 0x0000, 0x8000, chk4) \
    ROM_LOAD(u6, 0x8000, 0x8000, chk6)

#define ZAC_SOUNDROM_5x256(u05,chk1,u06,chk2,u24,chk3,u25,chk4,u40,chk5) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(u24, 0x0000, 0x8000, chk3) \
    ROM_LOAD(u25, 0x8000, 0x8000, chk4) \
  SOUNDREGION(0x10000, ZACSND_CPUBREGION) \
    ROM_LOAD(u05, 0x0000, 0x8000, chk1) \
    ROM_LOAD(u06, 0x8000, 0x8000, chk2) \
  SOUNDREGION(0x10000, ZACSND_CPUCREGION) \
    ROM_LOAD(u40, 0x0000, 0x8000, chk5) \

#define TECHNO_SOUNDROM1(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(n1, 0xe000, 0x2000, chk1) \
  SOUNDREGION(0x10000, ZACSND_CPUBREGION) \
    ROM_LOAD(n2, 0xe000, 0x2000, chk2)

#define TECHNO_SOUNDROM2(n3,chk3,n4,chk4,n5,chk5) \
  SOUNDREGION(0x20000, ZACSND_CPUCREGION) \
    ROM_LOAD(n5, 0x8000, 0x8000, chk5) \
    ROM_LOAD(n4, 0x10000, 0x8000, chk4) \
    ROM_LOAD(n3, 0x18000, 0x8000, chk3)
#endif /* INC_ZACSND */
