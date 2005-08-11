#ifndef INC_MRGAME
#define INC_MRGAME

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define MRGAME_CPUNO	0

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define MRGAME_ROMEND	ROM_END

/*-- Common Inports for MRGAME Games --*/
#define MRGAME_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0001, "Test Advance",	  KEYCODE_8)\
	COREPORT_BIT(     0x0002, "Test Return",	  KEYCODE_7)\
	COREPORT_BIT(     0x0004, "Slam Tilt",        KEYCODE_HOME)\
	COREPORT_BIT(     0x0008, "Credit Service",	  KEYCODE_9)\
    COREPORT_BITDEF(  0x0010, IPT_COIN1,          KEYCODE_3)\
	COREPORT_BITDEF(  0x0020, IPT_COIN2,          KEYCODE_4)\
	COREPORT_BITDEF(  0x0040, IPT_COIN3,          KEYCODE_5)\
	/* Switch Column 2 */ \
    COREPORT_BITDEF(  0x0080, IPT_START1,         IP_KEY_DEFAULT)\
    COREPORT_BIT(     0x0100, "Ball Tilt",        KEYCODE_INSERT)\
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "CPU SW1-1")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0001, "1" )\
    COREPORT_DIPNAME( 0x0002, 0x0000, "CPU SW1-2")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0002, "1" )\
    COREPORT_DIPNAME( 0x0004, 0x0000, "CPU SW1-3")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0004, "1" )\
    COREPORT_DIPNAME( 0x0008, 0x0000, "CPU SW1-4")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0008, "1" )\
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "VID SW1-1")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0001, "1" )\
    COREPORT_DIPNAME( 0x0002, 0x0000, "VID SW1-2")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0002, "1" )\
    COREPORT_DIPNAME( 0x0004, 0x0000, "VID SW1-3")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0004, "1" )\
    COREPORT_DIPNAME( 0x0008, 0x0000, "VID SW1-4")\
	  COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0008, "1" )

/*-- Standard input ports --*/
#define MRGAME_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    MRGAME_COMPORTS

#define MRGAME_INPUT_PORTS_END INPUT_PORTS_END

#define MRGAME_COMINPORT       CORE_COREINPORT

#define MRGAME_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define MRGAME_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define MRGAME_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- switch numbers --*/
// no switches outside switchmatrix

/*-- Memory regions --*/
#define MRGAME_MEMREG_CPU		REGION_CPU1
#define MRGAME_MEMREG_VID       REGION_CPU2
#define MRGAME_MEMREG_SND1      REGION_CPU3
#define MRGAME_MEMREG_SND2      REGION_CPU4

/*-- Main CPU regions and ROM --*/

/*-- 2 X CPU ROM --*/
#define MRGAME_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
   NORMALREGION(0x100000, MRGAME_MEMREG_CPU) \
       ROM_LOAD16_BYTE(n1, 0x000000, 0x8000, chk1) \
	   ROM_LOAD16_BYTE(n2, 0x000001, 0x8000, chk2)

/*-- VIDEO ROMS --*/
 
//Generation 1 
#define MRGAME_VIDEOROM1(n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
  NORMALREGION(0x10000, MRGAME_MEMREG_VID) \
   ROM_LOAD(n1, 0x00000, 0x8000, chk1) \
  ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) \
   ROM_LOAD(n2, 0x0000, 0x8000, chk2) \
   ROM_LOAD(n3, 0x8000, 0x8000, chk3) \
  ROM_REGION( 0x0020, REGION_PROMS, 0 ) \
   ROM_LOAD(n4, 0x0000, 0x0020, chk4)

//Generation 2 
#define MRGAME_VIDEOROM2(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7) \
  NORMALREGION(0x10000, MRGAME_MEMREG_VID) \
   ROM_LOAD(n1, 0x00000, 0x8000, chk1) \
  ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) \
   ROM_LOAD(n2, 0x0000, 0x8000, chk2) \
   ROM_LOAD(n3, 0x8000, 0x8000, chk3) \
   ROM_LOAD(n4, 0x10000, 0x8000, chk4) \
   ROM_LOAD(n5, 0x18000, 0x8000, chk5) \
   ROM_LOAD(n6, 0x20000, 0x8000, chk6) \
  ROM_REGION( 0x0020, REGION_PROMS, 0 ) \
   ROM_LOAD(n7, 0x0000, 0x0020, chk7)

/*-- SOUND ROMS --*/

//Generation 1 - 4 Rom Chips ( 1 x 16K, 3 x 32K )
#define MRGAME_SOUNDROM14(n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND1) \
   ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
  NORMALREGION(0x4000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x4000, chk2) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND2) \
   ROM_LOAD(n3, 0x0000, 0x8000, chk3) \
   ROM_LOAD(n4, 0x8000, 0x8000, chk4)

//Generation 1 - 5 Rom Chips ( 1 x 16K, 4 x 32K )
#define MRGAME_SOUNDROM15(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND1) \
   ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
   ROM_LOAD(n2, 0x8000, 0x8000, chk2) \
  NORMALREGION(0x4000, REGION_USER1) \
   ROM_LOAD(n3, 0x0000, 0x4000, chk3) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND2) \
   ROM_LOAD(n4, 0x0000, 0x8000, chk4) \
   ROM_LOAD(n5, 0x8000, 0x8000, chk5)

//Generation 2 - 5 Rom Chips ( 1 x 16K, 2 x 32K, 2 x 64K )
#define MRGAME_SOUNDROM25(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND1) \
   ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
  NORMALREGION(0x4000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x4000, chk2) \
  NORMALREGION(0x30000, REGION_USER2) \
   ROM_LOAD(n4, 0x00000, 0x10000, chk4) \
   ROM_LOAD(n5, 0x10000, 0x10000, chk5) \
  NORMALREGION(0x10000, MRGAME_MEMREG_SND2) \
   ROM_LOAD(n3, 0x00000, 0x8000, chk3)    

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(mrgame_g1);
extern MACHINE_DRIVER_EXTERN(mrgame_vg2_sg1);
extern MACHINE_DRIVER_EXTERN(mrgame_g2);

#define mMRGAME1        mrgame_g1
#define mMRGAME2		mrgame_vg2_sg1
#define mMRGAME3		mrgame_g2

extern PINMAME_VIDEO_UPDATE(mrgame_update_g1);
extern PINMAME_VIDEO_UPDATE(mrgame_update_g2);

#endif /* INC_MRGAME */
