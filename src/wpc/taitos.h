#ifndef INC_TAITOSND
#define INC_TAITOSND

/* Taito Sound Hardware Info:

  Sound System
  ------------
  M6802
  PIA 6821

*/

extern MACHINE_DRIVER_EXTERN(taitos_sintetizador);
extern MACHINE_DRIVER_EXTERN(taitos_sintetizadorpp);
extern MACHINE_DRIVER_EXTERN(taitos_sintevox);
extern MACHINE_DRIVER_EXTERN(taitos_sintevoxpp);

#define TAITO_SOUNDROMS11(rom1,chk1,rom2,chk2) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom2, 0x7800, 0x0400, chk2) \
      ROM_RELOAD(  0x0800, 0x0400) \
    ROM_LOAD(rom1, 0x7c00, 0x0400, chk1) \
      ROM_RELOAD(  0x0c00, 0x0400) \
      ROM_RELOAD(  0xfc00, 0x0400)

#define TAITO_SOUNDROMS2(rom1,chk1) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x1000, 0x0800, chk1) \
      ROM_RELOAD(  0x1800, 0x0800) \
      ROM_RELOAD(  0xf800, 0x0800)

#define TAITO_SOUNDROMS22(rom1,chk1,rom2,chk2) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom2, 0x1000, 0x0800, chk2) \
    ROM_LOAD(rom1, 0x1800, 0x0800, chk1) \
      ROM_RELOAD(  0xf800, 0x0800)

#define TAITO_SOUNDROMS222(rom1,chk1,rom2,chk2,rom3,chk3) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x0800, 0x0800, chk1) \
    ROM_LOAD(rom3, 0x1000, 0x0800, chk3) \
    ROM_LOAD(rom2, 0x1800, 0x0800, chk2) \
      ROM_RELOAD(  0xf800, 0x0800)

#define TAITO_SOUNDROMS4(rom1,chk1) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x3000, 0x1000, chk1) \
      ROM_RELOAD(  0xf000, 0x1000)

#define TAITO_SOUNDROMS44(rom1,chk1,rom2,chk2) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x2000, 0x1000, chk1) \
    ROM_LOAD(rom2, 0x3000, 0x1000, chk2) \
      ROM_RELOAD(  0xf000, 0x1000)

#define TAITO_SOUNDROMS444(rom1,chk1,rom2,chk2,rom3,chk3) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x5000, 0x1000, chk1) \
    ROM_LOAD(rom2, 0x6000, 0x1000, chk2) \
    ROM_LOAD(rom3, 0x7000, 0x1000, chk3) \
      ROM_RELOAD(  0xf000, 0x1000)

#endif /* INC_TAITOSND */


