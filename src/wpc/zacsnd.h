#ifndef INC_ZACSND
#define INC_ZACSND

#define ZACSND_CPUA       1
#define ZACSND_CPUAREGION (REGION_CPU1+ZACSND_CPUA)
#define ZACSND_CPUB       2
#define ZACSND_CPUBREGION (REGION_CPU1+ZACSND_CPUB)

extern MACHINE_DRIVER_EXTERN(zac1346);
extern MACHINE_DRIVER_EXTERN(zac1370);
extern MACHINE_DRIVER_EXTERN(zac13136);

#define ZAC_SOUNDROM_0(u1, chk1) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(u1, 0x0000, 0x0800, chk1)

//Board 1370 configuration (3 x 4K ROMS, 1 x 2K ROM)
#define ZAC_SOUNDROM_cefg0(uc,chkc,ue,chke,uf,chkf,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ug, 0x8000, 0x0800, chkg) \
    ROM_RELOAD(  0xc000, 0x0800) \
    ROM_LOAD(uf, 0x9000, 0x1000, chkf) \
	ROM_RELOAD(  0xd000, 0x1000) \
    ROM_LOAD(ue, 0xa000, 0x1000, chke) \
	ROM_RELOAD(  0xe000, 0x1000) \
    ROM_LOAD(uc, 0xb000, 0x1000, chkc) \
	ROM_RELOAD(  0xf000, 0x1000)

//Board 1370 configuration (4 x 4K ROMS)
#define ZAC_SOUNDROM_cefg1(uc,chkc,ue,chke,uf,chkf,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ug, 0x8000, 0x1000, chkg) \
    ROM_RELOAD(  0xc000, 0x1000) \
    ROM_LOAD(uf, 0x9000, 0x1000, chkf) \
	ROM_RELOAD(  0xd000, 0x1000) \
    ROM_LOAD(ue, 0xa000, 0x1000, chke) \
	ROM_RELOAD(  0xe000, 0x1000) \
    ROM_LOAD(uc, 0xb000, 0x1000, chkc) \
	ROM_RELOAD(  0xf000, 0x1000)

#define ZAC_SOUNDROM_de1g(ud,chkd,ue,chke,ug,chkg) \
  SOUNDREGION(0x10000, ZACSND_CPUAREGION) \
    ROM_LOAD(ud, 0xe000, 0x2000, chkd) \
    ROM_LOAD(ue, 0xd000, 0x1000, chke) \
    ROM_LOAD(ug, 0xb000, 0x2000, chkg)

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
    ROM_LOAD(u6, 0x8000, 0x4000, chk6) \
      ROM_RELOAD(0xc000, 0x4000)

#define ZAC_SOUNDROM_46(u4,chk4,u6,chk6) \
  SOUNDREGION(0x10000, ZACSND_CPUBREGION) \
    ROM_LOAD(u4, 0x0000, 0x8000, chk4) \
    ROM_LOAD(u6, 0x8000, 0x8000, chk6)

#endif /* INC_ZACSND */
