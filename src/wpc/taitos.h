#ifndef INC_TAITOSND
#define INC_TAITOSND

/* Taito Sound Hardware Info:

  Sound System
  ------------
  M6802
  PIA 6821

*/

extern MACHINE_DRIVER_EXTERN(taitos);

#define TAITO_SOUNDROMS22(rom1,chk1,rom2,chk2) \
  SOUNDREGION(0x10000, TAITO_MEMREG_SCPU) \
    ROM_LOAD(rom1, 0x1800, 0x0800, chk1) \
    ROM_LOAD(rom2, 0x1000, 0x0800, chk2) \

#endif /* INC_TAITOSND */


