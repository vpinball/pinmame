#ifndef INC_GTS3
#define INC_GTS3

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GTS3_ROMEND	ROM_END

/*-- Common Inports for S80Games --*/
#define GTS3_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_TILT,           KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    COREPORT_BITDEF(  0x0004, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0040, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BIT(     0x0080, "Hiscore Reset",    KEYCODE_4) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Advance",          KEYCODE_8) \
    COREPORT_BITTOG(  0x0200, "Up/Down",          KEYCODE_7) \
    COREPORT_BIT(     0x0400, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0800, "Sound Diagnostic", KEYCODE_0) \

/*-- Standard input ports --*/
#define GTS3_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS3_COMPORTS

#define GTS3_INPUT_PORTS_END INPUT_PORTS_END

#define GTS3_COMINPORT       CORE_COREINPORT

#define GTS3_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define GTS3_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define GTS3_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- S80 switches are numbered from 1-64 (not column,row as WPC) --*/
#define GTS3_SWNO(x) ((((x)+7)/8)*10+(((x)-1)%8)+1)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define GTS3_CSOL(x) ((x)+24)

/*-- S80 switch numbers --*/
#define GTS3_SWADVANCE     1
#define GTS3_SWUPDN        2
#define GTS3_SWCPUDIAG     3
#define GTS3_SWSOUNDDIAG   4


/*-- Memory regions --*/
#define GTS3_MEMREG_DROM1	REGION_USER1

#define GTS3_MEMREG_CPU		REGION_CPU1
#define GTS3_MEMREG_DCPU1	REGION_CPU2
#define GTS3_MEMREG_SCPU1	REGION_CPU3
#define GTS3_MEMREG_SROM1	REGION_SOUND1


/*-- Main CPU regions and ROM --*/
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


/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_GTS3_1;
#define de_mGTS3         GTS3_1

#endif /* INC_GTS3 */

