#ifndef INC_PEYPER
#define INC_PEYPER

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define PEYPER_ROMEND	ROM_END

/*-- Common Inports for PEYPER Games --*/
#define PEYPER_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 0 */ \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0080, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0040, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BIT(     0x0008, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0004, "Reset",            KEYCODE_0) \
    COREPORT_BIT(     0x0002, "Extra 1",          KEYCODE_8) \
    COREPORT_BIT(     0x0001, "Extra 2",          KEYCODE_9) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0020, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x2000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

/*-- Standard input ports --*/
#define PEYPER_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PEYPER_COMPORTS

#define PEYPER_INPUT_PORTS_END INPUT_PORTS_END

#define PEYPER_COMINPORT       CORE_COREINPORT

#define PEYPER_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define PEYPER_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define PEYPER_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define PEYPER_MEMREG_CPU	REGION_CPU1

/* CPUs */
#define PEYPER_CPU	0

#define PEYPER_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3) \
   ROM_START(name) \
     NORMALREGION(0x10000, PEYPER_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2) \
       ROM_LOAD(n3, 0x4000, 0x2000, chk3)

#define PEYPER_ROMSTART2(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, PEYPER_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(PEYPER);

#define gl_mPEYPER		PEYPER

#endif /* INC_PEYPER */
