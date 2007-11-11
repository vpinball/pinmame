#ifndef INC_STSND
#define INC_STSND

/*-- St300 interface --*/
struct sndbrdst300 {
  int c0;
  int ax[9];
  int axb[9];
  UINT16 timer1,timer2,timer3;
};
extern MACHINE_DRIVER_EXTERN(st100);
extern MACHINE_DRIVER_EXTERN(st300);
extern MACHINE_DRIVER_EXTERN(st300v);

extern struct sndbrdst300 snddatst300;

#define VSU100_ROMREGION (REGION_CPU1+1)

#define VSU100_SOUNDROM_U9(u9,chk9) \
  SOUNDREGION(0x10000, VSU100_ROMREGION) \
    ROM_LOAD(u9, 0x0000, 0x0800, chk9)

#define VSU100_SOUNDROM_U9U10(u9,chk9,u10,chk10) \
  SOUNDREGION(0x10000, VSU100_ROMREGION) \
    ROM_LOAD(u9, 0x0000, 0x0800, chk9) \
    ROM_LOAD(u10,0x0800, 0x0800, chk10)

#endif /* INC_STSND */


