#ifndef INC_TAITOSND
#define INC_TAITOSND

/* Taito Sound Hardware Info:

  Sound System
  ------------
  M6802
  PIA 6821

*/

extern MACHINE_DRIVER_EXTERN(taitos_sintevox);
extern MACHINE_DRIVER_EXTERN(taitos_sintetizador);
extern MACHINE_DRIVER_EXTERN(taitos_sintevoxpp);

#define TAITO_SOUNDROMS22(rom1,chk1,rom2,chk2) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x1800, 0x0800, chk1) \
    ROM_LOAD(rom2, 0x1000, 0x0800, chk2) \

#define TAITO_SOUNDROMS444(rom1,chk1,rom2,chk2,rom3,chk3) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x5000, 0x1000, chk1) \
    ROM_LOAD(rom2, 0x6000, 0x1000, chk2) \
    ROM_LOAD(rom3, 0x7000, 0x1000, chk3) \

#endif /* INC_TAITOSND */


