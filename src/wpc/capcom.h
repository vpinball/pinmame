#ifndef INC_CAPCOM
#define INC_CAPCOM

#include "core.h"
#include "sim.h"

#define CC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #6 & #7) */ \
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    /* Switch Column 2 (Switches #9 - #16) */ \
	/* For Stern MPU-200 (Switches #1-3, and #8) */ \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0200, "Slam Tilt",        KEYCODE_HOME)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0400, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0800, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x1000, "Sound Diagnostic", KEYCODE_0)

#define CC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    CC_COMPORTS

#define CC_INPUT_PORTS_END INPUT_PORTS_END

#define CC_ROMSTART_2(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
    NORMALREGION(0x00100000, REGION_CPU1) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2)

#define CC_ROMSTART_4(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    NORMALREGION(0x00200000, REGION_CPU1) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
      ROM_LOAD16_BYTE(n3, 0x100001, 0x80000, chk3) \
      ROM_LOAD16_BYTE(n4, 0x100000, 0x80000, chk4)

#define CC_ROMSTART_8(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8) \
  ROM_START(name) \
    NORMALREGION(0x00400000, REGION_CPU1) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
      ROM_LOAD16_BYTE(n3, 0x100001, 0x80000, chk3) \
      ROM_LOAD16_BYTE(n4, 0x100000, 0x80000, chk4) \
      ROM_LOAD16_BYTE(n5, 0x200001, 0x80000, chk5) \
      ROM_LOAD16_BYTE(n6, 0x200000, 0x80000, chk6) \
      ROM_LOAD16_BYTE(n7, 0x300001, 0x80000, chk7) \
      ROM_LOAD16_BYTE(n8, 0x300000, 0x80000, chk8)

#define CC_ROMSTART_2X(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x00200000, REGION_CPU1) \
       ROM_LOAD(n1, 0x000000, 0x100000, chk1) \
       ROM_LOAD(n2, 0x100000, 0x100000, chk2)

#define CC_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(cc);

#endif /* INC_CAPCOM */
