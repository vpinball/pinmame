#ifndef INC_SPINB
#define INC_SPINB

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define SPINB_CPUNO	0

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SPINB_ROMEND	ROM_END

/*-- Common Inports for Spinball Games --*/
#define SPINB_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
	COREPORT_BITDEF(  0x0001, IPT_COIN1,          KEYCODE_3)\
	COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_4)\
	COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_5)\
	COREPORT_BIT(     0x0008, "Ball Tilt",        KEYCODE_INSERT)\
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)\
    COREPORT_BIT(     0x0020, "Puesta A Cero",    KEYCODE_HOME)\
    COREPORT_BIT(     0x0040, "Test Economico",	  KEYCODE_9)\
    COREPORT_BIT(     0x0080, "Test Tecnico",	  KEYCODE_8)\
	COREPORT_BIT(     0x0100, "Test Contacto",	  KEYCODE_7)\
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1-1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S1-2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S1-3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S1-4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S1-5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S1-6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S1-7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S1-8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S2-1")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0100, "1" )\
    COREPORT_DIPNAME( 0x0200, 0x0000, "S2-2")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0200, "1" )\
    COREPORT_DIPNAME( 0x0400, 0x0400, "S2-3")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0400, "1" )\
    COREPORT_DIPNAME( 0x0800, 0x0000, "S2-4")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0800, "1" )\
    COREPORT_DIPNAME( 0x1000, 0x0000, "S2-5")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x1000, "1" )\
    COREPORT_DIPNAME( 0x2000, 0x0000, "S2-6")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x2000, "1" )\
    COREPORT_DIPNAME( 0x4000, 0x0000, "S2-7")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x4000, "1" )\
    COREPORT_DIPNAME( 0x8000, 0x0000, "S2-8")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x8000, "1" )\
  PORT_START /* 2 */\
    COREPORT_DIPNAME( 0x0001, 0x0000, "S3-1")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0001, "1" )\
    COREPORT_DIPNAME( 0x0002, 0x0000, "S3-2")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0002, "1" )\
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3-3")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0004, "1" )\
    COREPORT_DIPNAME( 0x0008, 0x0000, "S3-4")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0008, "1" )\
    COREPORT_DIPNAME( 0x0010, 0x0000, "S3-5")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0010, "1" )\
    COREPORT_DIPNAME( 0x0020, 0x0000, "S3-6")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0020, "1" )\
    COREPORT_DIPNAME( 0x0040, 0x0000, "S3-7")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0040, "1" )\
    COREPORT_DIPNAME( 0x0080, 0x0000, "S3-8")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0080, "1" )

/*-- Standard input ports --*/
#define SPINB_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SPINB_COMPORTS

#define SPINB_INPUT_PORTS_END INPUT_PORTS_END

#define SPINB_COMINPORT       CORE_COREINPORT

#define SPINB_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define SPINB_LAMPSMOOTH      6 /* Smooth the lamps over this number of VBLANKS */
#define SPINB_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define SPINB_CSOL(x) ((x)+24)

/*-- switch numbers --*/
#define SPINB_SWTEST     -7
#define SPINB_SWENTER    -6

/*-- Memory regions --*/
#define SPINB_MEMREG_CPU		REGION_CPU1
#define SPINB_MEMREG_DMD        REGION_CPU2

/*-- Main CPU regions and ROM --*/

/*-- 64K CPU ROM --*/
#define SPINB_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
   NORMALREGION(0x10000, SPINB_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
	   ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- DMD ROMS --*/
//NOTE: DMD CPU requires a memory region of 204,801 Bytes to allow linear address mapping as follows:
//      ROM0 lives in 1st 64K space, RAM lives in next 2K space, ROM1 lives in next 128K space, and
//      DMD Commands lives in the next byte, totalling 0x32001 of space required
#define SPINB_DMDROM(n1, chk1, n2, chk2) \
  NORMALREGION(0x32001, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
   ROM_LOAD(n2, 0x12000, 0x20000, chk2) 

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(spinb);
extern MACHINE_DRIVER_EXTERN(spinbs1);

#define mSPINB        spinb
#define mSPINBS		  spinbs1

extern PINMAME_VIDEO_UPDATE(SPINBdmd_update);

#endif /* INC_SPINB */
