#ifndef INC_MIDWAY
#define INC_MIDWAY

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define MIDWAY_ROMEND	ROM_END

/*-- Common Inports for MIDWAY Games --*/
#define MIDWAY_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 0 */ \
    COREPORT_BIT(     0x0100, "Player #1 Start",   KEYCODE_1) \
    COREPORT_BIT(     0x0400, "Player #2 Start",   KEYCODE_2) \
    COREPORT_BIT(     0x0800, "Player #3 Start",   KEYCODE_3) \
    COREPORT_BIT(     0x0200, "Player #4 Start",   KEYCODE_4) \
    COREPORT_BIT(     0x0008, "Solenoid Test",     KEYCODE_7) \
    COREPORT_BIT(     0x0010, "Switch Test",       KEYCODE_8) \
    COREPORT_BIT(     0x0020, "Lamp/Display Test", KEYCODE_9) \
    COREPORT_BIT(     0x0040, "Reset",             KEYCODE_0) \
    /* switch column 3 */ \
    COREPORT_BIT(     0x1000, "Slam Tilt",         KEYCODE_HOME) \
    COREPORT_BIT(     0x8000, "Ball Tilt",         KEYCODE_INSERT) \
    COREPORT_BITDEF(  0x4000, IPT_COIN1, IP_KEY_DEFAULT) \
    /* switch column 4 */ \
    COREPORT_BITDEF(  0x2000, IPT_COIN2, KEYCODE_6)

#define MIDWAYP_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 5 */ \
    COREPORT_BITIMP(  0x0002, "Slam Tilt",         KEYCODE_HOME) \
    COREPORT_BITIMP(  0x0020, "1 Credit",          KEYCODE_5) \
    COREPORT_BITIMP(  0x0040, "2 Credits",         KEYCODE_3) \
    COREPORT_BITIMP(  0x0080, "3 Credits",         KEYCODE_4) \
    /* switch column 6 */ \
    COREPORT_BITIMP(  0x0100, "4 Credits",         KEYCODE_6) \
    COREPORT_BITIMP(  0x0200, "5 Credits",         KEYCODE_7) \
    COREPORT_BITIMP(  0x0400, "6 Credits",         KEYCODE_8) \
    COREPORT_BIT(     0x0800, "Ball Tilt",         KEYCODE_INSERT) \
    COREPORT_BITIMP(  0x1000, "Game Start",        KEYCODE_1) \
    COREPORT_BIT(     0x8000, "Test",              KEYCODE_0) \
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
    COREPORT_DIPNAME( 0x0010, 0x0010, "Balls per Game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0010, "5" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Straight (no specials)") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0100, "0" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "Add-a-ball award") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "Replay award") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Match feature") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "15K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "20K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "25K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "30K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "35K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "40K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "45K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "50K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "55K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "60K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "65K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "70K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "75K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "80K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "85K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "90K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "95K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "100K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "105K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "110K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

/*-- Standard input ports --*/
#define MIDWAY_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    MIDWAY_COMPORTS

#define MIDWAYP_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    MIDWAYP_COMPORTS

#define MIDWAY_INPUT_PORTS_END INPUT_PORTS_END

#define MIDWAY_COMINPORT       CORE_COREINPORT

#define MIDWAY_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
#define MIDWAY_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define MIDWAY_DISPLAYSMOOTH   3 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define MIDWAY_CSOL(x) ((x)+24)

/*-- MIDWAY switch numbers --*/

/*-- Memory regions --*/
#define MIDWAY_MEMREG_CPU	REGION_CPU1

/* CPUs */
#define MIDWAY_CPU	0

/*-- MIDWAY CPU regions and ROM --*/
#define MIDWAY_1_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, MIDWAY_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x0400, chk1)

#define MIDWAY_3_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3) \
   ROM_START(name) \
     NORMALREGION(0x10000, MIDWAY_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x0800, 0x0800, chk2) \
       ROM_LOAD(n3, 0x1000, 0x0800, chk3)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(MIDWAY);
extern MACHINE_DRIVER_EXTERN(MIDWAYProto);

#define gl_mMIDWAY		MIDWAY
#define gl_mMIDWAYP		MIDWAYProto

#endif /* INC_MIDWAY */
