#ifndef INC_HNK
#define INC_HNK

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define HNK_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define HNK_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define HNK_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for HNK Games --*/
#define HNK_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #2 & #3) */ \
    COREPORT_BIT(     0x0001, "Ball Tilt",        KEYCODE_INSERT)  \
    COREPORT_BITDEF(  0x0002, IPT_START1,         IP_KEY_DEFAULT)  \
    /* Switch Column 2 (Switches #9 - #10) */ \
    COREPORT_BIT(     0x0004, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT)  \
    COREPORT_BITTOG(  0x0010, "Outhole",          KEYCODE_7)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0020, "Self Test",        KEYCODE_8) \
    COREPORT_BIT(     0x0040, "CPU Diagnostic",   KEYCODE_9) \
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
    COREPORT_DIPNAME( 0x0010, 0x0010, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S8") \
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
#define HNK_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    HNK_COMPORTS

#define HNK_INPUT_PORTS_END INPUT_PORTS_END

#define HNK_COMINPORT       CORE_COREINPORT

/*-- HNK switch numbers --*/
#define HNK_SWSELFTEST   -7
#define HNK_SWCPUDIAG    -6

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define HNK_CPU   0
#define HNK_SCPU1 1

/*-- Memory regions --*/
#define HNK_MEMREG_CPU		REGION_CPU1
#define HNK_MEMREG_SCPU		REGION_CPU2

/*-- Main CPU regions and ROM --*/
#define HNK_ROMSTART(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, HNK_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2) \
	  ROM_RELOAD (0xf800,0x0800)			//For IRQ & Reset Vectors
      
#define HNK_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_HNK;
#define mHNK     HNK
#endif /* INC_HNK */

