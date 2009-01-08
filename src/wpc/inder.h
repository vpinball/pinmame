#ifndef INC_INDER
#define INC_INDER

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define INDER_ROMEND	ROM_END

/*-- Common Inports for INDER Games --*/
#define INDER_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Ball Tilt",        KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0080, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0040, "Economic Adjusts", KEYCODE_8) \
    COREPORT_BIT(     0x0020, "Reset Button",     KEYCODE_0) \
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
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
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
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" )

/*-- Standard input ports --*/
#define INDER_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    INDER_COMPORTS

#define INDER_INPUT_PORTS_END INPUT_PORTS_END

#define INDER_COMINPORT       CORE_COREINPORT

#define INDER_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define INDER_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define INDER_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define INDER_MEMREG_CPU	REGION_CPU1
#define INDER_MEMREG_SND	REGION_CPU2

/* CPUs */
#define INDER_CPU		0
#define INDER_SND_CPU	1

/*-- GAME ROMS --*/
#define INDER_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, INDER_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x1000, chk1) \
       ROM_LOAD(n2, 0x1000, 0x1000, chk2)

#define INDER_ROMSTART1(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, INDER_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1)

#define INDER_ROMSTART2(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, INDER_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- SOUND ROMS --*/
#define INDER_SNDROM(n1, chk1) \
  NORMALREGION(0x10000, INDER_MEMREG_SND) \
    ROM_LOAD(n1, 0x00000, 0x2000, chk1)

#define INDER_SNDROM4(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
  NORMALREGION(0x10000, INDER_MEMREG_SND) \
    ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
  NORMALREGION(0x40000, REGION_USER1) \
    ROM_LOAD(n2, 0x0000, 0x10000, chk2) \
    ROM_LOAD(n3, 0x10000, 0x10000, chk3) \
    ROM_LOAD(n4, 0x20000, 0x10000, chk4) \
    ROM_LOAD(n5, 0x30000, 0x10000, chk5)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(INDER0);
extern MACHINE_DRIVER_EXTERN(INDER1);
extern MACHINE_DRIVER_EXTERN(INDER2);
extern MACHINE_DRIVER_EXTERN(INDERS);

#define gl_mINDER0		INDER0
#define gl_mINDER1		INDER1
#define gl_mINDER2		INDER2
#define gl_mINDERS		INDERS

#endif /* INC_INDER */
