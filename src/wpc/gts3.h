#ifndef INC_GTS3
#define INC_GTS3

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GTS3_ROMEND	ROM_END

/*-- Common Inports for System 3 --*/
#define GTS3_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0008, IPT_START1,         IP_KEY_DEFAULT) \
	/* Defining the flipper keys the usual way won't work for 4 & 5 */ \
	COREPORT_BIT(     0x0010, "Left Flipper",     KEYCODE_LCONTROL) \
    COREPORT_BIT(     0x0020, "Right Flipper",	  KEYCODE_RCONTROL) \
	COREPORT_BIT(     0x0010, "Left Advance",     KEYCODE_7) \
	COREPORT_BIT(     0x0020, "Right Advance",    KEYCODE_8) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BITTOG(  0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME)

/*-- Standard input ports --*/
#define GTS3_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS3_COMPORTS

#define GTS3_INPUT_PORTS_END INPUT_PORTS_END

#define GTS31_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BITTOG(  0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME)

#define GTS31_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS31_COMPORTS

#define GTS32_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
	COREPORT_BIT(     0x0020, "Tournament",       KEYCODE_2) \
	COREPORT_BIT(     0x0040, "Coin Door",        KEYCODE_END) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BITTOG(  0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME)

#define GTS32_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls)\
    GTS32_COMPORTS


#define GTS3_COMINPORT       CORE_COREINPORT

#define GTS3_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define GTS3_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define GTS3_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- S80 switches are numbered from 1-64 (not column,row as WPC) --*/
#define GTS3_SWNO(x) (x)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define GTS3_CSOL(x) ((x)+24)

/*-- S80 switch numbers --*/
#define GTS3_SWDIAG     -10
#define GTS3_SWTILT     -9
#define GTS3_SWSLAM     -8



/*-- Memory regions --*/
#define GTS3_MEMREG_DROM1	REGION_USER1

#define GTS3_MEMREG_CPU		REGION_CPU1
#define GTS3_MEMREG_DCPU1	REGION_CPU2
#define GTS3_MEMREG_SCPU1	REGION_CPU3
#define GTS3_MEMREG_SCPU2	REGION_CPU4
#define GTS3_MEMREG_SROM1	REGION_SOUND1


/*-- Main CPU regions and ROM --*/

/*-- 32K CPU ROM --*/
#define GTS3ROMSTART32(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS3_MEMREG_CPU) \
       ROM_LOAD(n1, 0x8000, 0x8000, chk1)

/*-- 64K CPU ROM --*/
#define GTS3ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS3_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x10000, chk1)

/**************************************/
/** DMD (128x32) ROM 256K            **/
/**************************************/
#define GTS3_DMD256_ROMSTART(n1,chk1) \
	 NORMALREGION(0x10000, GTS3_MEMREG_DCPU1)\
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x40000, chk1)\
	   ROM_RELOAD(  0x40000, 0x40000)

/**************************************/
/** DMD (128x32) ROM 512K            **/
/**************************************/
#define GTS3_DMD512_ROMSTART(n1,chk1) \
	 NORMALREGION(0x10000, GTS3_MEMREG_DCPU1)\
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x80000, chk1)

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_GTS3_1A;
extern struct MachineDriver machine_driver_GTS3_1B;
extern struct MachineDriver machine_driver_GTS3_2;
extern struct MachineDriver machine_driver_GTS3_1AS;
extern struct MachineDriver machine_driver_GTS3_1BS;
extern struct MachineDriver machine_driver_GTS3_2S;

extern void UpdateSoundLEDS(int num,int data);

#define mGTS3         GTS3_1A
#define mGTS3S        GTS3_1AS
#define mGTS3B        GTS3_1B
#define mGTS3BS       GTS3_1BS
#define mGTS3DMD      GTS3_2
#define mGTS3DMDS     GTS3_2S

#endif /* INC_GTS3 */
