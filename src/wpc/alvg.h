#ifndef INC_ALVG
#define INC_ALVG

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define ALVG_CPUNO	0

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define ALVG_ROMEND	ROM_END

/*-- Common Inports for Alvin G Games --*/
#define ALVG_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
	COREPORT_BITDEF(  0x0001, IPT_COIN1,          KEYCODE_3) \
	COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_5) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
	COREPORT_BIT(     0x0010, "Slam Tilt",        KEYCODE_HOME) \
	/* Switch Column 2 */ \
    COREPORT_BITDEF(  0x0020, IPT_START1,         IP_KEY_DEFAULT) \
	COREPORT_BIT(     0x0040, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0800, "Volume +",         KEYCODE_9) \
	COREPORT_BIT(     0x1000, "Volume -",         KEYCODE_0) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Test",			  KEYCODE_7) \
    COREPORT_BIT(     0x0200, "Enter",			  KEYCODE_8) \
    COREPORT_BITTOG(  0x0400, "Ticket",			  KEYCODE_END)

/*-- Standard input ports --*/
#define ALVG_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    ALVG_COMPORTS

#define ALVG_INPUT_PORTS_END INPUT_PORTS_END

#define ALVG_COMINPORT       CORE_COREINPORT

#define ALVG_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define ALVG_LAMPSMOOTH      6 /* Smooth the lamps over this number of VBLANKS */
#define ALVG_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define ALVG_CSOL(x) ((x)+24)

/*-- switch numbers --*/
#define ALVG_SWTEST     -7
#define ALVG_SWENTER    -6
#define ALVG_SWTICKET   -5

/*-- Memory regions --*/
#define ALVG_MEMREG_CPU		REGION_CPU1

/*-- Main CPU regions and ROM --*/

/*-- 64K CPU ROM --*/
#define ALVGROMSTART(name, n1, chk1) \
   ROM_START(name) \
   NORMALREGION(0x10000, ALVG_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x10000, chk1)

extern void alvg_UpdateSoundLEDS(int num,int data);

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(alvg);
extern MACHINE_DRIVER_EXTERN(alvgs1);
extern MACHINE_DRIVER_EXTERN(alvgs2);
extern MACHINE_DRIVER_EXTERN(alvgs2dmd);

#define mALVG         alvg
#define mALVGS1		  alvgs1
#define mALVGS2		  alvgs2
#define mALVGS2DMD    alvgs2dmd

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
  extern MACHINE_DRIVER_EXTERN(alvg_test8031);
  #define mTEST8031	  alvg_test8031
#endif

#endif /* INC_ALVG */
