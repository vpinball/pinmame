#ifndef INC_PLAYMATIC
#define INC_PLAYMATIC

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define PLAYMATIC_ROMEND    ROM_END

/*-- Inports for early games --*/
#define PLAYMATIC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 7 */ \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BITDEF(0x0008, IPT_START1,    IP_KEY_DEFAULT)  \
    COREPORT_BIT(   0x0020, "Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0080, "Reset",       KEYCODE_8) \
    COREPORT_BIT(   0x0040, "Ball Tilt",   KEYCODE_INSERT) \
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
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
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

/*-- Inports for series III games --*/
#define PLAYMATIC3_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 7 */ \
    COREPORT_BITDEF(0x0040, IPT_START1,    IP_KEY_DEFAULT)  \
    COREPORT_BIT(   0x0020, "Advance Digit", KEYCODE_2) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0010, "Ball Tilt / Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0080, "Advance Step", KEYCODE_8) \
    COREPORT_BIT(   0x0100, "Reset",       KEYCODE_0)

/*-- Inports for series IV games --*/
#define PLAYMATIC4_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 7 */ \
    COREPORT_BITDEF(0x0040, IPT_START1,    IP_KEY_DEFAULT)  \
    COREPORT_BIT(   0x0080, "Advance Digit", KEYCODE_2) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0010, "Key 7",       KEYCODE_7) \
    COREPORT_BIT(   0x0020, "Key 8",       KEYCODE_8) \
    COREPORT_BIT(   0x0100, "Reset",       KEYCODE_0)

/*-- Standard input ports --*/
#define PLAYMATIC1_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    /* Switch Column 5 */ \
    COREPORT_BIT(   0x2000, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x4000, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x8000, "Coin 3",      KEYCODE_5) \
    /* Switch Column 7 */ \
    COREPORT_BIT(   0x0001, "Test",        KEYCODE_7) \
    COREPORT_BIT(   0x0002, "Reset",       KEYCODE_6) \
    COREPORT_BIT(   0x0004, "(Re)set HSTD",KEYCODE_END) \
    COREPORT_BIT(   0x0008, "Set Replay #1",KEYCODE_8) \
    COREPORT_BIT(   0x0010, "Set Replay #2",KEYCODE_9) \
    COREPORT_BIT(   0x0020, "Set Replay #3",KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "HSTD award") \
      COREPORT_DIPSET(0x0000, "3 games" ) \
      COREPORT_DIPSET(0x0001, "1 game" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "Balls per game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0002, "5" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Special award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0004, "Extra ball" )

#define PLAYMATIC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PLAYMATIC_COMPORTS

#define PLAYMATIC3_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PLAYMATIC3_COMPORTS

#define PLAYMATIC4_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PLAYMATIC4_COMPORTS

#define PLAYMATIC_INPUT_PORTS_END INPUT_PORTS_END

#define PLAYMATIC_COMINPORT       CORE_COREINPORT

#define PLAYMATIC_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define PLAYMATIC_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define PLAYMATIC_SOLSMOOTH       6 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define PLAYMATIC_MEMREG_CPU      REGION_CPU1
#define PLAYMATIC_MEMREG_SCPU     REGION_CPU2

/* CPUs */
#define PLAYMATIC_CPU             0
#define PLAYMATIC_SCPU            1

#define PLAYMATIC_ROMSTART88(name, n1, chk1, n2, chk2) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x0400, chk1) \
      ROM_RELOAD(0x0c00, 0x0400) \
    ROM_LOAD(n2, 0x0400, 0x0400, chk2) \
      ROM_RELOAD(0x0800, 0x0400) \

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
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2S);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC3S);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC4S);

#define gl_mPLAYMATIC      PLAYMATIC
#define gl_mPLAYMATIC2     PLAYMATIC2
#define gl_mPLAYMATIC2S    PLAYMATIC2S
#define gl_mPLAYMATIC3S    PLAYMATIC3S
#define gl_mPLAYMATIC4S    PLAYMATIC4S

#endif /* INC_PLAYMATIC */
