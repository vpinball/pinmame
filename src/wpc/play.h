#ifndef INC_PLAYMATIC
#define INC_PLAYMATIC

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define PLAYMATIC_ROMEND    ROM_END

/*-- Common Inports for PLAYMATIC Games --*/
#define PLAYMATIC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0040, "Key 1",     KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Key 2",     KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Key 3",     KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Key 4",     KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Key 5",     KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Key 6",     KEYCODE_6) \
    COREPORT_BIT(     0x0001, "Key 7",     KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Key 8",     KEYCODE_8) \

/*-- Standard input ports --*/
#define PLAYMATIC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PLAYMATIC_COMPORTS

#define PLAYMATIC_INPUT_PORTS_END INPUT_PORTS_END

#define PLAYMATIC_COMINPORT       CORE_COREINPORT

#define PLAYMATIC_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define PLAYMATIC_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define PLAYMATIC_SOLSMOOTH       1 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define PLAYMATIC_MEMREG_CPU      REGION_CPU1
#define PLAYMATIC_MEMREG_SCPU     REGION_CPU2

/* CPUs */
#define PLAYMATIC_CPU             0
#define PLAYMATIC_SCPU            1

#define PLAYMATIC_ROMSTART8888(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x0400, chk1) \
      ROM_RELOAD(0x4000, 0x0400) \
      ROM_RELOAD(0x8000, 0x0400) \
      ROM_RELOAD(0xc000, 0x0400) \
    ROM_LOAD(n2, 0x0400, 0x0400, chk2) \
      ROM_RELOAD(0x4400, 0x0400) \
      ROM_RELOAD(0x8400, 0x0400) \
      ROM_RELOAD(0xc400, 0x0400) \
    ROM_LOAD(n3, 0x0800, 0x0400, chk3) \
      ROM_RELOAD(0x4800, 0x0400) \
      ROM_RELOAD(0x8800, 0x0400) \
      ROM_RELOAD(0xc800, 0x0400) \
    ROM_LOAD(n4, 0x0c00, 0x0400, chk4) \
      ROM_RELOAD(0x4c00, 0x0400) \
      ROM_RELOAD(0x8c00, 0x0400) \
      ROM_RELOAD(0xcc00, 0x0400)

#define PLAYMATIC_ROMSTART000(name, n1, chk1, n2, chk2, n3, chk3) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x0800, chk1) \
      ROM_RELOAD(0x4000, 0x0800) \
      ROM_RELOAD(0x8000, 0x0800) \
      ROM_RELOAD(0xc000, 0x0800) \
    ROM_LOAD(n2, 0x0800, 0x0800, chk2) \
      ROM_RELOAD(0x4800, 0x0800) \
      ROM_RELOAD(0x8800, 0x0800) \
      ROM_RELOAD(0xc800, 0x0800) \
    ROM_LOAD(n3, 0x1000, 0x0800, chk3) \
      ROM_RELOAD(0x5000, 0x0800) \
      ROM_RELOAD(0x9000, 0x0800) \
      ROM_RELOAD(0xd000, 0x0800)

#define PLAYMATIC_ROMSTART64(name, n1, chk1) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x4000, 0x2000) \
      ROM_RELOAD(0x8000, 0x2000) \
      ROM_RELOAD(0xc000, 0x2000)

#define PLAYMATIC_SOUNDROM6416(n1, chk1, n2, chk2) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x4000, 0x2000) \
      ROM_RELOAD(0x8000, 0x2000) \
      ROM_RELOAD(0xc000, 0x2000) \
    ROM_LOAD(n2, 0x2000, 0x0800, chk2) \
      ROM_RELOAD(0x6000, 0x0800) \
      ROM_RELOAD(0xa000, 0x0800) \
      ROM_RELOAD(0xe000, 0x0800)

#define PLAYMATIC_SOUNDROM6432(n1, chk1, n2, chk2) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x4000, 0x2000) \
      ROM_RELOAD(0x8000, 0x2000) \
      ROM_RELOAD(0xc000, 0x2000) \
    ROM_LOAD(n2, 0x2000, 0x1000, chk2) \
      ROM_RELOAD(0x6000, 0x1000) \
      ROM_RELOAD(0xa000, 0x1000) \
      ROM_RELOAD(0xe000, 0x1000)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(PLAYMATIC);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2);

#define gl_mPLAYMATIC      PLAYMATIC
#define gl_mPLAYMATIC2     PLAYMATIC2

#endif /* INC_PLAYMATIC */
