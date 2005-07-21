#ifndef INC_SPINB
#define INC_SPINB

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define SPINB_CPU_GAME 0
#define SPINB_CPU_DMD  1
#define SPINB_CPU_SND1 2
#define SPINB_CPU_SND2 3

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SPINB_ROMEND	ROM_END

/*-- Common Inports for Spinball Games --*/
#define SPINB_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
	COREPORT_BITDEF(  0x0001, IPT_COIN1,          KEYCODE_3)\
/*	COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_4)\	commented out as it interferes with Bushido's trough switches,
	COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_5)\	and it doesn't seem to be connected on Jolly Park either!?
*/	COREPORT_BIT(     0x0008, "Ball Tilt",        KEYCODE_INSERT)\
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)\
    COREPORT_BIT(     0x0020, "Puesta A Cero",    KEYCODE_HOME)\
    COREPORT_BIT(     0x0040, "Test Economico",	  KEYCODE_9)\
    COREPORT_BIT(     0x0080, "Test Tecnico",	  KEYCODE_8)\
	COREPORT_BIT(     0x0100, "Test Contacto",	  KEYCODE_7)\
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coins Per Game") \
      COREPORT_DIPSET(0x0000, "25 ptas 2 Games" ) \
      COREPORT_DIPSET(0x0008, "25 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0004, "25 ptas 1 Game, 100 ptas 5 Games" ) \
      COREPORT_DIPSET(0x000c, "50 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0002, "50 ptas 1 Game, 200 ptas 5 Games" ) \
      COREPORT_DIPSET(0x000a, "50 ptas 1 Game, 100 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0006, "100 ptas 1 Game" ) \
      COREPORT_DIPSET(0x000e, "100 ptas 1 Game, 500 ptas 6 Games" ) \
      COREPORT_DIPSET(0x0001, "100 ptas 1 Game, 200 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0009, "50 esc 1 Game" ) \
      COREPORT_DIPSET(0x0005, "50 esc 1 Game, 100 esc 3 Games" ) \
      COREPORT_DIPSET(0x000d, "100 esc 1 Game" ) \
      COREPORT_DIPSET(0x0003, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000b, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x0007, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000f, "100 esc 2 Games, 200 esc 5 Games" ) \
	COREPORT_DIPNAME( 0x0030, 0x0000, "First Replay Score") \
      COREPORT_DIPSET(0x0000, "300 Million" ) \
      COREPORT_DIPSET(0x0010, "400 Million" ) \
      COREPORT_DIPSET(0x0020, "500 Million" ) \
      COREPORT_DIPSET(0x0030, "600 Million" ) \
	COREPORT_DIPNAME( 0x0040, 0x0000, "Reserved") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Game Type") \
      COREPORT_DIPSET(0x0000, "Normal" ) \
      COREPORT_DIPSET(0x0080, "Tournament" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Balls")\
      COREPORT_DIPSET(0x0000, "3" )\
      COREPORT_DIPSET(0x0100, "5" )\
    COREPORT_DIPNAME( 0x0200, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0200, "1" )\
    COREPORT_DIPNAME( 0x0c00, 0x0000, "Value of Handicap")\
      COREPORT_DIPSET(0x0000, "700 Million" )\
      COREPORT_DIPSET(0x0800, "750 Million" )\
      COREPORT_DIPSET(0x0400, "800 Million" )\
      COREPORT_DIPSET(0x0c00, "850 Million" )\
	COREPORT_DIPNAME( 0x3000, 0x0000, "Time of New Ticket")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x1000, "Short" )\
      COREPORT_DIPSET(0x2000, "Medium" )\
      COREPORT_DIPSET(0x3000, "Long" )\
	COREPORT_DIPNAME( 0x4000, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x4000, "1" )\
    COREPORT_DIPNAME( 0x8000, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x8000, "1" )\
  PORT_START /* 2 */\
    COREPORT_DIPNAME( 0x0001, 0x0000, "AutoPlunger")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x0001, "Enabled" )\
    COREPORT_DIPNAME( 0x0002, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0002, "1" )\
    COREPORT_DIPNAME( 0x0008, 0x0000, "Number of loops for Extra Ball")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0008, "1" )\
      COREPORT_DIPSET(0x0004, "2" )\
      COREPORT_DIPSET(0x000C, "3" )\
    COREPORT_DIPNAME( 0x0030, 0x0000, "Multiball Ballsaver or Restart")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x0010, "Short" )\
      COREPORT_DIPSET(0x0020, "Medium" )\
      COREPORT_DIPSET(0x0030, "Long" )\
    COREPORT_DIPNAME( 0x0040, 0x0000, "Special Difficulty")\
      COREPORT_DIPSET(0x0000, "Easy" )\
      COREPORT_DIPSET(0x0040, "Hard" )\
    COREPORT_DIPNAME( 0x0080, 0x0000, "Reserved")\
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

#define SPINB_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define SPINB_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define SPINB_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define SPINB_CSOL(x) ((x)+24)

/*-- switch numbers --*/
#define SPINB_SWTEST     -8

/*-- Memory regions --*/
#define SPINB_MEMREG_CPU		REGION_CPU1
#define SPINB_MEMREG_DMD        REGION_CPU2
#define SPINB_MEMREG_SND1       REGION_CPU3
#define SPINB_MEMREG_SND2       REGION_CPU4

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

/* 1 X 64K ROM */
#define SPINB_DMDROM1(n1, chk1) \
  NORMALREGION(0x32001, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1)
/* 2 X 64K ROM */
#define SPINB_DMDROM2(n1, chk1, n2, chk2) \
  NORMALREGION(0x32001, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
   ROM_LOAD(n2, 0x12000, 0x20000, chk2)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM23(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7) \
  NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
   ROM_LOAD(n3, 0x80000, 0x80000, chk3) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n4, 0x00000, 0x2000, chk4) \
  NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n5, 0x0000, 0x80000, chk5) \
   ROM_LOAD(n6, 0x80000, 0x80000, chk6) \
   ROM_LOAD(n7, 0x100000, 0x80000, chk7)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM22(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6) \
  NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
   ROM_LOAD(n3, 0x80000, 0x80000, chk3) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n4, 0x00000, 0x2000, chk4) \
  NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n5, 0x0000, 0x80000, chk5) \
   ROM_LOAD(n6, 0x80000, 0x80000, chk6)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM12(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n3, 0x00000, 0x2000, chk3) \
NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n4, 0x0000, 0x80000, chk4) \
   ROM_LOAD(n5, 0x80000, 0x80000, chk5)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(spinb);
extern MACHINE_DRIVER_EXTERN(spinbs1);
extern MACHINE_DRIVER_EXTERN(spinbs1n);

#define mSPINB        spinb
#define mSPINBS		  spinbs1
#define mSPINBSNMI	  spinbs1n

extern PINMAME_VIDEO_UPDATE(SPINBdmd_update);

#endif /* INC_SPINB */
