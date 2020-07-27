#ifndef INC_PLAYMATIC
#define INC_PLAYMATIC
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

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
    /* Switch Column 1 */ \
    COREPORT_BITDEF(0x0008, IPT_START1,    IP_KEY_DEFAULT) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0040, "Ball Tilt",   KEYCODE_INSERT) \
    COREPORT_BIT(   0x0020, "Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0080, "Reset",       KEYCODE_8) \
    /* Switch Column 0 */ \
    COREPORT_BIT(   0x0100, DEF_STR(Unknown), KEYCODE_0)

/*-- Inports for Spain '82 --*/
#define PLAYMATIC2_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(0x0080, IPT_START1,    IP_KEY_DEFAULT) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0010, "Ball Tilt",   KEYCODE_INSERT) \
    /* Switch Column 0 */ \
    COREPORT_BIT(   0x8000, "Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0100, "Reset",       KEYCODE_0)

/*-- Inports for series III games --*/
#define PLAYMATIC3_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(0x0040, IPT_START1,    IP_KEY_DEFAULT) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x1010, "Tilt / Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0020, "Advance Digit", KEYCODE_2) \
    COREPORT_BIT(   0x0080, "Advance Step", KEYCODE_8) \
    /* Switch Column 0 */ \
    COREPORT_BIT(   0x0100, "Reset",       KEYCODE_0)

/*-- Inports for series IV games --*/
#define PLAYMATIC4_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(0x0040, IPT_START1,    IP_KEY_DEFAULT) \
    COREPORT_BIT(   0x0001, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0002, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0004, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0808, "Tilt / Diagnostics", KEYCODE_7) \
    COREPORT_BIT(   0x0080, "Advance Digit", KEYCODE_2) \
    COREPORT_BITTOG(0x0010, "Advance Step / Coin door", KEYCODE_8) \
    /* Switch Column 0 */ \
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
      COREPORT_DIPSET(0x0004, "Extra ball" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x00ff, 0x0002, "Credits for small coin") \
      COREPORT_DIPSET(0x0001, "1/2" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0004, "3/2" ) \
      COREPORT_DIPSET(0x0008, "2" ) \
      COREPORT_DIPSET(0x0010, "3" ) \
      COREPORT_DIPSET(0x0020, "4" ) \
      COREPORT_DIPSET(0x0040, "5" ) \
      COREPORT_DIPSET(0x0080, "6" ) \
    COREPORT_DIPNAME( 0xff00, 0x0400, "Credits for big coin") \
      COREPORT_DIPSET(0x0100, "3" ) \
      COREPORT_DIPSET(0x0200, "4" ) \
      COREPORT_DIPSET(0x0400, "5" ) \
      COREPORT_DIPSET(0x0800, "6" ) \
      COREPORT_DIPSET(0x1000, "7" ) \
      COREPORT_DIPSET(0x2000, "8" ) \
      COREPORT_DIPSET(0x4000, "9" ) \
      COREPORT_DIPSET(0x8000, "10" )

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
    ROM_LOAD(n2, 0x0400, 0x0400, chk2)

#define PLAYMATIC_ROMSTART884(name, n1, chk1, n2, chk2, n3, chk3) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x0400, chk1) \
    ROM_LOAD(n2, 0x0400, 0x0400, chk2) \
    ROM_LOAD(n3, 0x0800, 0x0200, chk3)

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

#define PLAYMATIC_ROMSTART00(name, n1, chk1, n2, chk2) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x0800, chk1) \
      ROM_RELOAD(0x4000, 0x0800) \
      ROM_RELOAD(0x8000, 0x0800) \
      ROM_RELOAD(0xc000, 0x0800) \
    ROM_LOAD(n2, 0x0800, 0x0800, chk2) \
      ROM_RELOAD(0x4800, 0x0800) \
      ROM_RELOAD(0x8800, 0x0800) \
      ROM_RELOAD(0xc800, 0x0800)

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

#define PLAYMATIC_ROMSTART320(name, n1, chk1, n2, chk2) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x1000, chk1) \
      ROM_RELOAD(0x4000, 0x1000) \
      ROM_RELOAD(0x8000, 0x1000) \
      ROM_RELOAD(0xc000, 0x1000) \
    ROM_LOAD(n2, 0x1000, 0x0800, chk2) \
      ROM_RELOAD(0x5000, 0x0800) \
      ROM_RELOAD(0x9000, 0x0800) \
      ROM_RELOAD(0xd000, 0x0800)

#define PLAYMATIC_ROMSTART3232(name, n1, chk1, n2, chk2) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x1000, chk1) \
      ROM_RELOAD(0x4000, 0x1000) \
      ROM_RELOAD(0x8000, 0x1000) \
      ROM_RELOAD(0xc000, 0x1000) \
    ROM_LOAD(n2, 0x1000, 0x1000, chk2) \
      ROM_RELOAD(0x5000, 0x1000) \
      ROM_RELOAD(0x9000, 0x1000) \
      ROM_RELOAD(0xd000, 0x1000)

#define PLAYMATIC_ROMSTART64(name, n1, chk1) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x4000, 0x2000) \
      ROM_RELOAD(0x8000, 0x2000) \
      ROM_RELOAD(0xc000, 0x2000)

#define PLAYMATIC_ROMSTART64_2(name, n1, chk1, n2, chk2) \
ROM_START(name) \
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x4000, 0x2000) \
      ROM_RELOAD(0x8000, 0x2000) \
      ROM_RELOAD(0xc000, 0x2000) \
    ROM_LOAD(n2, 0x2000, 0x2000, chk2) \
      ROM_RELOAD(0x6000, 0x2000) \
      ROM_RELOAD(0xa000, 0x2000) \
      ROM_RELOAD(0xe000, 0x2000)

#define PLAYMATIC_SOUNDROMZ(n1, chk1) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x0800, chk1)

#define PLAYMATIC_SOUNDROM64(n1, chk1) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
      ROM_RELOAD(0x2000, 0x2000)

#define PLAYMATIC_SOUNDROM6416(n1, chk1, n2, chk2) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
    ROM_LOAD(n2, 0x2000, 0x0800, chk2) \
      ROM_RELOAD(0x2800, 0x0800) \
      ROM_RELOAD(0x3000, 0x0800) \
      ROM_RELOAD(0x3800, 0x0800)

#define PLAYMATIC_SOUNDROM6432(n1, chk1, n2, chk2) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
    ROM_LOAD(n2, 0x2000, 0x1000, chk2) \
      ROM_RELOAD(0x3000, 0x1000)

#define PLAYMATIC_SOUNDROM256(n1, chk1) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
  SOUNDREGION(0x20000, REGION_USER1)

#define PLAYMATIC_SOUNDROM256x4(n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
  SOUNDREGION(0x10000, PLAYMATIC_MEMREG_SCPU) \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
  SOUNDREGION(0x20000, REGION_USER1) \
    ROM_LOAD(n2, 0x00000, 0x8000, chk2) \
    ROM_LOAD(n3, 0x08000, 0x8000, chk3) \
    ROM_LOAD(n4, 0x10000, 0x8000, chk4)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(PLAYMATIC1);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC1A);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2SZ);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2S3);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC2S4);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC3S3);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC3S4);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC4);
extern MACHINE_DRIVER_EXTERN(PLAYMATIC4SZSU);
extern MACHINE_DRIVER_EXTERN(PLAYMATICBINGO);

#define gl_mPLAYMATIC1     PLAYMATIC1
#define gl_mPLAYMATIC1A    PLAYMATIC1A
#define gl_mPLAYMATIC2     PLAYMATIC2
#define gl_mPLAYMATIC2SZ   PLAYMATIC2SZ
#define gl_mPLAYMATIC2S3   PLAYMATIC2S3
#define gl_mPLAYMATIC2S4   PLAYMATIC2S4
#define gl_mPLAYMATIC3S3   PLAYMATIC3S3
#define gl_mPLAYMATIC3S4   PLAYMATIC3S4
#define gl_mPLAYMATIC4     PLAYMATIC4
#define gl_mPLAYMATIC4SZSU PLAYMATIC4SZSU
#define gl_mPLAYMATICBINGO PLAYMATICBINGO

extern MACHINE_DRIVER_EXTERN(PLAYMATICS1);
extern MACHINE_DRIVER_EXTERN(PLAYMATICS2);
extern MACHINE_DRIVER_EXTERN(PLAYMATICS3);
extern MACHINE_DRIVER_EXTERN(PLAYMATICS4);
extern MACHINE_DRIVER_EXTERN(PLAYMATICSZ);

#endif /* INC_PLAYMATIC */
