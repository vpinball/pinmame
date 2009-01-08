#ifndef INC_JP
#define INC_JP

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define JP_ROMEND	ROM_END

/*-- Common Inports for JP Games --*/
#define JP_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 4 */ \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0008, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BIT(     0x0040, "Pendulum Tilt",    KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0020, "Door Tilt",        KEYCODE_HOME) \
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
    COREPORT_DIPNAME( 0x0001, 0x0000, "SA") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "SB") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "SC") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "SD") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" )

/*-- Standard input ports --*/
#define JP_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    JP_COMPORTS

#define JP_INPUT_PORTS_END INPUT_PORTS_END

#define JP_COMINPORT       CORE_COREINPORT

#define JP_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define JP_DISPLAYSMOOTH   3 /* Smooth the display over this number of VBLANKS */
#define JP_SOLSMOOTH       2 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define JP_MEMREG_CPU	REGION_CPU1
#define JP_MEMREG_SND	REGION_CPU2

/* CPUs */
#define JP_CPU	0

#define JP_ROMSTART1(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, JP_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1)

#define JP_ROMSTART2(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, JP_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- SOUND ROMS --*/
#define JP_SNDROM8(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7, n8, chk8) \
  NORMALREGION(0x10000, JP_MEMREG_SND) \
    ROM_LOAD(n1, 0x00000, 0x4000, chk1) \
  NORMALREGION(0x40000, REGION_SOUND1) \
    ROM_LOAD(n2, 0x0000, 0x8000, chk2) \
    ROM_LOAD(n3, 0x8000, 0x8000, chk3) \
    ROM_LOAD(n4, 0x10000, 0x8000, chk4) \
    ROM_LOAD(n5, 0x18000, 0x8000, chk5) \
    ROM_LOAD(n6, 0x20000, 0x8000, chk6) \
    ROM_LOAD(n7, 0x28000, 0x8000, chk7) \
    ROM_LOAD(n8, 0x30000, 0x8000, chk8)

/*-- SOUND ROMS --*/
#define JP_SNDROM15(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7, n8, chk8, n9, chk9, n10, chk10, n11, chk11, n12, chk12, n13, chk13, n14, chk14, n15, chk15) \
  NORMALREGION(0x10000, JP_MEMREG_SND) \
    ROM_LOAD(n1, 0x00000, 0x4000, chk1) \
  NORMALREGION(0x80000, REGION_SOUND1) \
    ROM_LOAD(n2, 0x0000, 0x8000, chk2) \
    ROM_LOAD(n3, 0x8000, 0x8000, chk3) \
    ROM_LOAD(n4, 0x10000, 0x8000, chk4) \
    ROM_LOAD(n5, 0x18000, 0x8000, chk5) \
    ROM_LOAD(n6, 0x20000, 0x8000, chk6) \
    ROM_LOAD(n7, 0x28000, 0x8000, chk7) \
    ROM_LOAD(n8, 0x30000, 0x8000, chk8) \
    ROM_LOAD(n9, 0x38000, 0x8000, chk9) \
    ROM_LOAD(n10,0x40000, 0x8000, chk10) \
    ROM_LOAD(n11,0x48000, 0x8000, chk11) \
    ROM_LOAD(n12,0x50000, 0x8000, chk12) \
    ROM_LOAD(n13,0x58000, 0x8000, chk13) \
    ROM_LOAD(n14,0x60000, 0x8000, chk14) \
    ROM_LOAD(n15,0x68000, 0x8000, chk15)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(JP);
extern MACHINE_DRIVER_EXTERN(JP2);
extern MACHINE_DRIVER_EXTERN(JPS);

#define gl_mJP		JP
#define gl_mJP2		JP2
#define gl_mJPS		JPS

#endif /* INC_JP */
