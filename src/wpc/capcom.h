#ifndef INC_CAPCOM
#define INC_CAPCOM

#include "core.h"
#include "sim.h"

extern PINMAME_VIDEO_UPDATE(cc_dmd128x32);
extern PINMAME_VIDEO_UPDATE(cc_dmd256x64);
extern void cap_UpdateSoundLEDS(int data);

#define CC_COMPORTS \
  PORT_START /* 0 */ \
    /* These are put in switch column 9 */ \
    COREPORT_BIT(     0x0040, "Start Game",KEYCODE_1) \
    COREPORT_BIT(     0x0001, "Coin #1",   KEYCODE_3) \
    COREPORT_BIT(     0x0002, "Coin #2",   KEYCODE_4) \
    COREPORT_BIT(     0x0004, "Coin #3",   KEYCODE_5) \
    COREPORT_BIT(     0x0008, "Coin #4",   KEYCODE_6) \
    COREPORT_BITTOG(  0x0080, "Coin Door", KEYCODE_7) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0400, "Buy In",    KEYCODE_2) \
    COREPORT_BIT(     0x0200, "Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT(     0x0100, "Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT(     0x0800, "Unused #1", KEYCODE_8) \
    COREPORT_BIT(     0x1000, "Unused #2", KEYCODE_9) \
    COREPORT_BIT(     0x2000, "Unused #3", KEYCODE_0) \
    COREPORT_BIT(     0x4000, "Unused #4", KEYCODE_PGUP) \
    COREPORT_BIT(     0x8000, "Unused #5", KEYCODE_PGDN)

#define CC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    CC_COMPORTS

#define CC_INPUT_PORTS_END INPUT_PORTS_END

#define CC_ROMSTART_2(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
    ROM_REGION16_BE(0x00100000, REGION_USER1,0) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
	NORMALREGION(0x00100000, REGION_CPU1)

#define CC_ROMSTART_4(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    ROM_REGION16_BE(0x00500000, REGION_USER1,0) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
      ROM_LOAD16_BYTE(n3, 0x400001, 0x80000, chk3) \
      ROM_LOAD16_BYTE(n4, 0x400000, 0x80000, chk4) \
    NORMALREGION(0x00500000, REGION_CPU1) 

#define CC_ROMSTART_8(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8) \
  ROM_START(name) \
    ROM_REGION16_BE(0x00d00000, REGION_USER1,0) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
      ROM_LOAD16_BYTE(n3, 0x400001, 0x80000, chk3) \
      ROM_LOAD16_BYTE(n4, 0x400000, 0x80000, chk4) \
      ROM_LOAD16_BYTE(n5, 0x800001, 0x80000, chk5) \
      ROM_LOAD16_BYTE(n6, 0x800000, 0x80000, chk6) \
      ROM_LOAD16_BYTE(n7, 0xc00001, 0x80000, chk7) \
      ROM_LOAD16_BYTE(n8, 0xc00000, 0x80000, chk8) \
	NORMALREGION(0x00d00000, REGION_CPU1) 

#define CC_ROMSTART_2X(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     ROM_REGION16_BE(0x00500000, REGION_USER1,0) \
       ROM_LOAD(n1, 0x000000, 0x100000, chk1) \
       ROM_LOAD(n2, 0x400000, 0x100000, chk2) \
     NORMALREGION(0x00500000, REGION_CPU1)

#define CC_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(cc);
extern MACHINE_DRIVER_EXTERN(cc1);
extern MACHINE_DRIVER_EXTERN(cc2);

#endif /* INC_CAPCOM */
