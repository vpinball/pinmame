#ifndef INC_GP
#define INC_GP

/* DIP SWITCH SETTINGS:

 05 04 03 02 01 (Coin Chute #1)		Credits Per Coin
 21 20 19 18 17 (Coin Chute #3)
 --------------
  0  0  0  0  0  3/2 Coins
  0  0  0  0  1  3/2 Coins (Typo in manual?)
  0  0  0  1  0  1/1 Coins
  ....
  1  1  1  1  0  15/1 Coins
  1  1  1  1  1  15/2 Coins

 12 11 10 09 (Coin Chute #2)		Credits Per Coin
 -----------
  0  0  0  0 Same As Coin Chute #1
  0  0  0  1 1/1 Coin
  .....
  1  1  1  1 15/1 Coin

  01 - 05 (See Coin Settings Above)
  08 =    FREE PLAY (On = YES, OFF = NO)
  09 - 12 (See Coin Settings Above)
  13 - 15 ??
  16 =	  TUNE OPTION (On = Play Tune for credit, Off = Play Chime?)
  17 - 21 (See Coin Settings Above)
  22 - 24 ??

  27 26 25 (MAX # of Credits)
  --------
   0  0  0  5 Credits
   0  0  1 10 Credits
   ....
   1  1  1 40 Credits

  28 =	  BALLS PER GAME (On = 5, Off = 3)
  29 =	  SPECIAL AWARD (On = REPLAY, Off = Extra Ball)
  30 =    MATCH FEATURE (On = YES, Off = NO)

  32 31 (Credits awarded for beating high score)
  -----
  0   0  0 Credits
  0   1  1 Credits
  1   0  2 Credits
  1   1  3 Credits
*/



#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define GP_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define GP_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define GP_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for GP Games --*/
#define GP_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1*/ \
    COREPORT_BIT(     0x0001, "Accounting Reset",   KEYCODE_8)  \
    COREPORT_BITDEF(  0x0002, IPT_START1,			IP_KEY_DEFAULT)\
    COREPORT_BIT(     0x0004, "Slam Tilt",          KEYCODE_HOME)\
    COREPORT_BITDEF(  0x0010, IPT_COIN2,            KEYCODE_4)\
    COREPORT_BITDEF(  0x0020, IPT_COIN3,            KEYCODE_5)\
    COREPORT_BITDEF(  0x0040, IPT_COIN1,            KEYCODE_3)\
    COREPORT_BIT(     0x0080, "Ball Tilt",			KEYCODE_2)\
	/* Switch Column 4 */ \
	COREPORT_BIT(     0x0100, "Diagnostic Switch",  KEYCODE_7)\
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0200, "Sound Diagnostic",   KEYCODE_0)\
    COREPORT_BIT(     0x0400, "Test Input",         KEYCODE_9)\
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S2") \
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
    COREPORT_DIPNAME( 0x8000, 0x8000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S18") \
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
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \

/*-- Standard input ports --*/
#define GP_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GP_COMPORTS

#define GP_INPUT_PORTS_END INPUT_PORTS_END

#define GP_COMINPORT       CORE_COREINPORT

/*-- GP switch numbers --*/
#define GP_SWTEST  -6
#define GP_SWSOUNDDIAG  -7

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GP_CPUNO   0
#define GP_SCPUNO  1

/*-- Memory regions --*/
#define GP_MEMREG_CPU	REGION_CPU1
#define GP_MEMREG_SCPU	(REGION_CPU1 + GP_SCPUNO)

/*-- Main CPU regions and ROM --*/

/* 2 X 2K ROMS */
#define GP_ROMSTART88(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x10000, GP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2)


/* 3 X 2K ROMS */
#define GP_ROMSTART888(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
	NORMALREGION(0x10000, GP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3)

/* 1 X 4K ROM & 1 X 2K ROM */
#define GP_ROMSTART08(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x10000, GP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x1000, chk1) \
      ROM_LOAD( n2, 0x1000, 0x0800, chk2)

/* 2 X 4K ROMS */
#define GP_ROMSTART00(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x10000, GP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x1000, chk1) \
      ROM_LOAD( n2, 0x1000, 0x1000, chk2)

/* 3 X 4K ROMS */
#define GP_ROMSTART000(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
	NORMALREGION(0x10000, GP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x1000, chk1) \
      ROM_LOAD( n2, 0x1000, 0x1000, chk2) \
      ROM_LOAD( n3, 0x2000, 0x1000, chk3)

#define GP_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(GP1);
extern MACHINE_DRIVER_EXTERN(GP1S1);
extern MACHINE_DRIVER_EXTERN(GP2);
extern MACHINE_DRIVER_EXTERN(GP2S1);
extern MACHINE_DRIVER_EXTERN(GP2S2);
extern MACHINE_DRIVER_EXTERN(GP2S4);
extern MACHINE_DRIVER_EXTERN(GP2SM);
extern MACHINE_DRIVER_EXTERN(GP2SM3);

#define mGP1     GP1
#define mGP1S1   GP1S1
#define mGP2     GP2
#define mGP2S1   GP2S1
#define mGP2S2   GP2S2
#define mGP2S4   GP2S4
#define mGP2SM   GP2SM
#define mGP2SM3  GP2SM3

#endif /* INC_GP */
