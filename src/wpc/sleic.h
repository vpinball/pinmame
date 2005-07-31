#ifndef INC_SLEIC
#define INC_SLEIC

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SLEIC_ROMEND    ROM_END

/*-- Common Inports for SLEIC Games --*/
#define SLEIC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0001, "Key 1",     KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Key 2",     KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Key 3",     KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Key 4",     KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Key 5",     KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Key 6",     KEYCODE_6) \
    COREPORT_BIT(     0x0040, "Key 7",     KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Key 8",     KEYCODE_8) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" )

/*-- Standard input ports --*/
#define SLEIC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SLEIC_COMPORTS

#define SLEIC_INPUT_PORTS_END INPUT_PORTS_END

#define SLEIC_COMINPORT       CORE_COREINPORT

#define SLEIC_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define SLEIC_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define SLEIC_SOLSMOOTH       1 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define SLEIC_MEMREG_CPU     REGION_CPU1
#define SLEIC_MEMREG_DISPLAY REGION_CPU2
#define SLEIC_MEMREG_IO      REGION_CPU3

/* CPUs */
#define SLEIC_MAIN_CPU    0
#define SLEIC_DISPLAY_CPU 1
#define SLEIC_IO_CPU      2

#define SLEIC_ROMSTART4(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
ROM_START(name) \
  NORMALREGION(0x100000, SLEIC_MEMREG_CPU) \
    ROM_LOAD(n3, 0xe0000, 0x20000, chk3) \
  NORMALREGION(0x10000, SLEIC_MEMREG_DISPLAY) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
  NORMALREGION(0x10000, SLEIC_MEMREG_IO) \
    ROM_LOAD(n4, 0x0000, 0x8000, chk4) \
  NORMALREGION(0x100000, REGION_USER1) \
    ROM_LOAD(n2, 0x00000, 0x80000, chk2)

/*-- These are only here so the game structure can be in the game file --*/
extern PINMAME_VIDEO_UPDATE(sleic_dmd_update);

extern MACHINE_DRIVER_EXTERN(SLEIC);

#define gl_mSLEIC      SLEIC

#endif /* INC_SLEIC */
