#ifndef INC_ZAC
#define INC_ZAC

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define ZAC_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define ZAC_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define ZAC_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for ZAC Games --*/
#define ZAC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1*/ \
    COREPORT_BIT(     0x0001, "Advance(Coin Door)", KEYCODE_8)  \
	COREPORT_BIT(     0x0002, "Return(Coin Door)",  KEYCODE_7)  \
    COREPORT_BIT(     0x0004, "Slam Tilt?",         KEYCODE_HOME)  \
	COREPORT_BIT(     0x0008, "Credit(Service)",    KEYCODE_6)  \
    COREPORT_BITDEF(  0x0010, IPT_COIN1,            KEYCODE_3) \
    COREPORT_BITDEF(  0x0020, IPT_COIN2,            KEYCODE_4) \
    COREPORT_BITDEF(  0x0040, IPT_COIN3,            KEYCODE_5) \
	/* Switch Column 2 */ \
	COREPORT_BITDEF(  0x0080, IPT_START1,			IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0100, "Ball Tilt",			KEYCODE_2)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0200, "Sound Diagnostic", KEYCODE_0) \
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
      COREPORT_DIPSET(0x0008, "1" ) 

/*-- Standard input ports --*/
#define ZAC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    ZAC_COMPORTS

#define ZAC_INPUT_PORTS_END INPUT_PORTS_END

#define ZAC_COMINPORT       CORE_COREINPORT

/*-- ZAC switch numbers --*/
#define ZAC_SWSOUNDDIAG  -7

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define ZAC_CPUNO   0
#define ZAC_SCPU1NO 1

/*-- Memory regions --*/
#define ZAC_MEMREG_CPU		REGION_CPU1
#define ZAC_MEMREG_S1CPU	REGION_CPU2
#define ZAC_MEMREG_SROM		REGION_SOUND1
#define ZAC_MEMREG_CROM1	REGION_USER1 /*CPU ROMS Loaded Here*/

/*-- Main CPU regions and ROM --*/

/* 3 X 2532 ROMS */
#define ZAC_ROMSTART(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
    NORMALREGION(0x8000, ZAC_MEMREG_CROM1) \
      ROM_LOAD( n1, 0x0000, 0x1000, chk1) \
	  ROM_RELOAD(0x1000,0x1000)\
      ROM_LOAD( n2, 0x2000, 0x1000, chk2) \
	  ROM_RELOAD(0x3000,0x1000)\
      ROM_LOAD( n3, 0x4000, 0x1000, chk3) \
	  ROM_RELOAD(0x5000,0x1000)

/* 2 X 2764 ROMS */
#define ZAC_ROMSTART2(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
    NORMALREGION(0x8000, ZAC_MEMREG_CROM1) \
      ROM_LOAD( n1, 0x0000, 0x2000, chk1) \
      ROM_LOAD( n2, 0x2000, 0x2000, chk2)
      
#define ZAC_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_ZAC1;
extern struct MachineDriver machine_driver_ZAC2;
#define mZAC1     ZAC1
#define mZAC2     ZAC2
#endif /* INC_ZAC */

