#ifndef INC_HNKS
#define INC_HNKS

extern MACHINE_DRIVER_EXTERN(hnks);

#define HNK_SOUNDROMS(ic14,chk14,ic3,chk3) \
  SOUNDREGION(0x10000, HNK_MEMREG_SCPU) \
    ROM_LOAD(ic14, 0x1000, 0x0800, chk14) \
      ROM_RELOAD(  0xf800, 0x0800) \
    ROM_LOAD(ic3,  0xf000, 0x0200, chk3)

#endif /* INC_HNKS */

